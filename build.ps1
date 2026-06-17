<#
    Быстрая пересборка The Circle после изменений в коде.

    В отличие от setup.ps1, не проверяет инструменты и не запускает
    "conan install" - предполагается, что .\setup.ps1 уже был выполнен хотя
    бы раз с той же -Configuration и build-<configuration>\CMakePresets.json
    существует. Release и Debug собираются в отдельных папках
    (build-release, build-debug).

    Подключает окружение MSVC x64 (vcvarsall.bat), если оно ещё не
    подключено в текущей сессии, и запускает
    cmake --build --preset conan-<Configuration>.

    Запуск (из обычного PowerShell):
        .\build.ps1
        .\build.ps1 -Configuration Debug
        .\build.ps1 -Run   # запустить TheCircle.exe после успешной сборки
#>

[CmdletBinding()]
param(
    [ValidateSet("Release", "Debug")]
    [string]$Configuration = "Release",
    [switch]$Run
)

$ErrorActionPreference = "Stop"
$root = Split-Path -Parent $MyInvocation.MyCommand.Path
Set-Location $root

function Write-Step($msg) { Write-Host "`n==> $msg" -ForegroundColor Cyan }
function Write-Ok($msg)   { Write-Host "    [OK] $msg" -ForegroundColor Green }

$presetName = "conan-$($Configuration.ToLower())"
$buildFolder = "build-$($Configuration.ToLower())"

if (-not (Test-Path (Join-Path $root "$buildFolder\CMakePresets.json"))) {
    Write-Host "$buildFolder\CMakePresets.json не найден - сначала выполните: .\setup.ps1 -Configuration $Configuration" -ForegroundColor Red
    exit 1
}

# ---------------------------------------------------------------------------
# Окружение MSVC x64 (cl.exe), если сборка запускается не из
# "x64 Native Tools Command Prompt" - см. комментарий в setup.ps1 про
# несовпадение архитектуры (LNK2019).
# ---------------------------------------------------------------------------
if ($env:VSCMD_ARG_TGT_ARCH -ne "x64") {
    Write-Step "Подключение окружения MSVC x64 (vcvarsall.bat)"

    $vswhere = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe"
    $vsInstallPath = $null
    if (Test-Path $vswhere) {
        $vsInstallPath = (& $vswhere -latest -prerelease -products "*" `
            -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 `
            -property installationPath 2>$null | Select-Object -First 1)
    }
    if (-not $vsInstallPath) {
        Write-Host "Не найдена Visual Studio с компонентом C++ (MSVC). Выполните .\setup.ps1." -ForegroundColor Red
        exit 1
    }

    $vcvarsall = Join-Path $vsInstallPath "VC\Auxiliary\Build\vcvarsall.bat"
    if (-not (Test-Path $vcvarsall)) {
        Write-Host "Не найден $vcvarsall" -ForegroundColor Red
        exit 1
    }

    $tmp = [System.IO.Path]::GetTempFileName()
    $prevEap = $ErrorActionPreference
    $ErrorActionPreference = "Continue"
    cmd /c "`"$vcvarsall`" x64 && set" 1> $tmp 2>$null
    $ErrorActionPreference = $prevEap
    Get-Content $tmp | ForEach-Object {
        if ($_ -match '^([^=]+)=(.*)$') {
            [System.Environment]::SetEnvironmentVariable($Matches[1], $Matches[2])
        }
    }
    Remove-Item $tmp -Force

    Write-Ok "cl.exe: $((Get-Command cl.exe -ErrorAction SilentlyContinue).Source)"
} else {
    Write-Ok "Окружение MSVC x64 уже подключено"
}

# ---------------------------------------------------------------------------
# Debug-сборка автоматически включает оверлей отладки (F1 в игре);
# Release собирается без него (флаг TC_DEBUG нигде не определён).
# Перенастраиваем кэш CMake под выбранный $Configuration на случай, если он
# отличается от того, с которым последний раз запускали .\setup.ps1.
# ---------------------------------------------------------------------------
$tcDebugMode = if ($Configuration -eq "Debug") { "ON" } else { "OFF" }

Write-Step "cmake --preset $presetName (TC_DEBUG_MODE=$tcDebugMode)"
& cmake --preset $presetName "-DTC_DEBUG_MODE=$tcDebugMode"
if ($LASTEXITCODE -ne 0) {
    Write-Host "`nКонфигурация CMake завершилась с ошибкой (код $LASTEXITCODE)." -ForegroundColor Red
    exit 1
}

# ---------------------------------------------------------------------------
# Сборка
# ---------------------------------------------------------------------------
Write-Step "cmake --build --preset $presetName"
& cmake --build --preset $presetName
if ($LASTEXITCODE -ne 0) {
    Write-Host "`nСборка завершилась с ошибкой (код $LASTEXITCODE) - см. ошибки компиляции выше." -ForegroundColor Red
    exit 1
}

$exePath = Join-Path $root "$buildFolder\TheCircle.exe"
Write-Host "`n========================================================" -ForegroundColor Green
if (Test-Path $exePath) {
    Write-Host " Сборка готова: $exePath" -ForegroundColor Green
} else {
    Write-Host " Сборка завершена, но $exePath не найден - проверьте вывод выше." -ForegroundColor Yellow
}
Write-Host "========================================================"

if ($Run -and (Test-Path $exePath)) {
    Write-Step "Запуск TheCircle.exe"
    & $exePath
}
