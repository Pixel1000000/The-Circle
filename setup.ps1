<#
    Установка зависимостей и первая сборка The Circle.

    Скрипт сам проверяет, что на этом ПК установлено (Conan, CMake, Ninja,
    Visual Studio с компонентом C++), подбирает подходящую версию MSVC и
    vs_version для конкретной машины, ставит зависимости через Conan и
    выполняет первую сборку проекта.

    Если чего-то не хватает или версии не подходят - скрипт остановится и
    выведет понятный список того, что нужно установить/обновить.

    Запуск (из обычного PowerShell, без "x64 Native Tools Command Prompt"):
        .\setup.ps1
        .\setup.ps1 -Configuration Debug
#>

[CmdletBinding()]
param(
    [ValidateSet("Release", "Debug")]
    [string]$Configuration = "Release"
)

$ErrorActionPreference = "Stop"
$root = Split-Path -Parent $MyInvocation.MyCommand.Path
Set-Location $root

function Write-Step($msg) { Write-Host "`n==> $msg" -ForegroundColor Cyan }
function Write-Info($msg) { Write-Host "    $msg" }
function Write-Ok($msg)   { Write-Host "    [OK] $msg" -ForegroundColor Green }
function Write-Warn($msg) { Write-Host "    [!]  $msg" -ForegroundColor Yellow }

$problems = New-Object System.Collections.Generic.List[string]

# ---------------------------------------------------------------------------
# 1. Conan
# ---------------------------------------------------------------------------
Write-Step "Conan"
$conanCmd = Get-Command conan -ErrorAction SilentlyContinue
if (-not $conanCmd) {
    $problems.Add("Conan не найден.`n      Установите Python (https://www.python.org/downloads/ или `"winget install Python.Python.3.12`"),`n      затем выполните: pip install conan")
} else {
    $verText = (& conan --version) -join " "
    if ($verText -match '(\d+)\.(\d+)\.(\d+)') {
        $conanVersion = [version]"$($Matches[1]).$($Matches[2]).$($Matches[3])"
        if ($conanVersion.Major -lt 2) {
            $problems.Add("Установлен Conan $conanVersion, а проекту нужен Conan 2.x.`n      Обновите: pip install --upgrade `"conan>=2.0`"")
        } else {
            Write-Ok "Conan $conanVersion"
        }
    } else {
        $problems.Add("Не удалось определить версию Conan (вывод: '$verText').")
    }
}

# ---------------------------------------------------------------------------
# 2. CMake
# ---------------------------------------------------------------------------
Write-Step "CMake"
$cmakeCmd = Get-Command cmake -ErrorAction SilentlyContinue
if (-not $cmakeCmd) {
    $problems.Add("CMake не найден.`n      Установите: winget install Kitware.CMake (или https://cmake.org/download/)")
} else {
    $verText = (& cmake --version | Select-Object -First 1)
    if ($verText -match '(\d+)\.(\d+)\.(\d+)') {
        $cmakeVersion = [version]"$($Matches[1]).$($Matches[2]).$($Matches[3])"
        if ($cmakeVersion -lt [version]"3.20.0") {
            $problems.Add("Установлен CMake $cmakeVersion, а проекту нужен >= 3.20.`n      Обновите: winget install Kitware.CMake")
        } else {
            Write-Ok "CMake $cmakeVersion"
        }
    } else {
        $problems.Add("Не удалось определить версию CMake (вывод: '$verText').")
    }
}

# ---------------------------------------------------------------------------
# 3. Visual Studio + компонент "Разработка классических приложений на C++"
# ---------------------------------------------------------------------------
Write-Step "Visual Studio / MSVC"
$vsInfo = $null
$vswhere = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe"
if (Test-Path $vswhere) {
    $raw = (& $vswhere -latest -prerelease -products "*" `
        -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 `
        -format json 2>$null) -join "`n"
    $raw = $raw.Trim()
    if ($raw -and $raw -ne "[]") {
        $parsed = $raw | ConvertFrom-Json
        if ($parsed -is [System.Array]) { $vsInfo = $parsed[0] } else { $vsInfo = $parsed }
    }
}

$vsVersionMajor = $null
if (-not $vsInfo) {
    $problems.Add("Не найдена Visual Studio с компонентом 'Desktop development with C++' (MSVC).`n      Установите Visual Studio (Community/Build Tools, https://visualstudio.microsoft.com/)`n      и включите рабочую нагрузку 'Разработка классических приложений на C++'.")
} else {
    $vsVersionMajor = ($vsInfo.installationVersion -split '\.')[0]
    Write-Ok "$($vsInfo.displayName) $($vsInfo.installationVersion) -> tools.microsoft.msbuild:vs_version=$vsVersionMajor"
}

# ---------------------------------------------------------------------------
# 4. Ninja
# ---------------------------------------------------------------------------
Write-Step "Ninja"
$ninjaCmd = Get-Command ninja -ErrorAction SilentlyContinue
if ($ninjaCmd) {
    Write-Ok "Ninja найден ($($ninjaCmd.Source))"
} elseif ($vsInfo) {
    $bundled = Join-Path $vsInfo.installationPath "Common7\IDE\CommonExtensions\Microsoft\CMake\Ninja\ninja.exe"
    if (Test-Path $bundled) {
        Write-Warn "Ninja не найден в PATH, использую версию из Visual Studio: $bundled"
        $env:PATH = "$(Split-Path $bundled);$env:PATH"
    } else {
        $problems.Add("Ninja не найден.`n      Установите: winget install Ninja-build.Ninja`n      (или включите компонент 'C++ CMake tools for Windows' в Visual Studio Installer)")
    }
} else {
    $problems.Add("Ninja не найден.`n      Установите: winget install Ninja-build.Ninja")
}

# ---------------------------------------------------------------------------
# Если чего-то не хватает - сразу понятный список и выход
# ---------------------------------------------------------------------------
if ($problems.Count -gt 0) {
    Write-Host "`n========================================================" -ForegroundColor Red
    Write-Host " Сборку нельзя запустить - не хватает инструментов:" -ForegroundColor Red
    Write-Host "========================================================" -ForegroundColor Red
    foreach ($p in $problems) {
        Write-Host "`n - $p" -ForegroundColor Red
    }
    Write-Host ""
    exit 1
}

# ---------------------------------------------------------------------------
# 5. Подбор профиля Conan под текущий ПК
# ---------------------------------------------------------------------------
Write-Step "Профиль Conan"

New-Item -ItemType Directory -Force -Path (Join-Path $root "build") | Out-Null

function Get-SettingValue($content, $key) {
    if ($content -match "(?m)^\s*$key\s*=\s*(\S+)") { return $Matches[1] }
    return $null
}

$trackedProfilePath = "conan/profiles/windows-msvc"
$profilePath = $trackedProfilePath

try {
    # ErrorActionPreference=Stop превращает любую строку, которую conan пишет
    # в stderr (например, информационную "Found msvc ..."), в завершающую
    # ошибку при перенаправлении вывода - временно отключаем это для вызова.
    $prevEap = $ErrorActionPreference
    $ErrorActionPreference = "Continue"
    & conan profile detect --force --name circle-host *> $null
    $detectExit = $LASTEXITCODE
    $ErrorActionPreference = $prevEap
    if ($detectExit -ne 0) { throw "conan profile detect завершился с кодом $detectExit" }

    $detectedProfilePath = (& conan profile path circle-host).Trim()
    $detectedContent = Get-Content $detectedProfilePath -Raw
    $trackedContent  = Get-Content (Join-Path $root $trackedProfilePath) -Raw

    $detectedCompilerVersion = Get-SettingValue $detectedContent "compiler.version"
    $trackedCompilerVersion  = Get-SettingValue $trackedContent  "compiler.version"

    Write-Info "MSVC на этом ПК (compiler.version): $detectedCompilerVersion"
    Write-Info "Профиль репозитория ($trackedProfilePath) рассчитан на compiler.version: $trackedCompilerVersion"

    if ($detectedCompilerVersion -and $detectedCompilerVersion -ne $trackedCompilerVersion) {
        $hostProfilePath = "build/conan-profile-host.txt"
        $lines = ($detectedContent -split "`r?`n") | ForEach-Object {
            if ($_ -match '^\s*compiler\.cppstd\s*=')      { "compiler.cppstd=17" }
            elseif ($_ -match '^\s*build_type\s*=')        { "build_type=$Configuration" }
            else { $_ }
        }
        Set-Content -Path (Join-Path $root $hostProfilePath) -Value $lines -Encoding ascii
        $profilePath = $hostProfilePath
        Write-Warn "MSVC на этом ПК ($detectedCompilerVersion) отличается от версии, под которую собран $trackedProfilePath ($trackedCompilerVersion)."
        Write-Warn "Создан локальный профиль $hostProfilePath под установленный у вас компилятор."
    } else {
        Write-Ok "Версии MSVC совпадают, использую $trackedProfilePath"
    }
} catch {
    Write-Warn "Не удалось определить версию MSVC через 'conan profile detect': $($_.Exception.Message)"
    Write-Warn "Использую профиль из репозитория: $trackedProfilePath"
}

# ---------------------------------------------------------------------------
# 6. conan install
# ---------------------------------------------------------------------------
Write-Step "conan install (зависимости проекта)"

$conanArgs = @(
    "install", ".",
    "--output-folder=build",
    "--build=missing",
    "-pr:h", $profilePath,
    "-pr:b", $profilePath,
    "-s", "build_type=$Configuration",
    "-c", "tools.cmake.cmaketoolchain:generator=Ninja",
    "-c", "tools.microsoft.msbuild:vs_version=$vsVersionMajor"
)
Write-Info "conan $($conanArgs -join ' ')"
& conan @conanArgs
if ($LASTEXITCODE -ne 0) {
    Write-Host "`n========================================================" -ForegroundColor Red
    Write-Host " conan install завершился с ошибкой (код $LASTEXITCODE)." -ForegroundColor Red
    Write-Host "========================================================" -ForegroundColor Red
    Write-Host @"

Частые причины и что делать:
 - Нет доступа к conancenter / ошибка SSL-сертификата:
   проверьте интернет, корпоративный proxy и антивирус; при ошибках
   сертификата обновите корневые сертификаты Windows и повторите запуск.
 - Для одного из пакетов нет готового бинарника под вашу версию MSVC,
   а сборка из исходников не удалась - смотрите сообщение Conan выше,
   там указано, какой пакет и почему.
"@ -ForegroundColor Yellow
    exit 1
}

# ---------------------------------------------------------------------------
# 7. Окружение MSVC (cl.exe x64) для сборки через Ninja
#
#    Подключаем vcvarsall.bat x64 всегда, а не только если cl.exe не найден:
#    на некоторых ПК в PATH уже лежит x86-версия cl.exe (например, от старой
#    установки VS), из-за чего CMake выбирает её, а Conan при этом ставит
#    зависимости под x86_64 - сборка падает на этапе линковки (LNK2019).
#    vcvarsall.bat добавляет нужные x64-каталоги в начало PATH, перекрывая
#    такие "случайные" записи.
# ---------------------------------------------------------------------------
Write-Step "Подключение окружения MSVC x64 (vcvarsall.bat)"
$vcvarsall = Join-Path $vsInfo.installationPath "VC\Auxiliary\Build\vcvarsall.bat"
if (-not (Test-Path $vcvarsall)) {
    Write-Host "Не найден $vcvarsall - запустите этот скрипт из 'x64 Native Tools Command Prompt for VS'." -ForegroundColor Red
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
$clPath = (Get-Command cl.exe -ErrorAction SilentlyContinue).Source
Write-Ok "Окружение MSVC подключено (cl.exe: $clPath)"

# ---------------------------------------------------------------------------
# 8. CMake configure + build
# ---------------------------------------------------------------------------
$presetName = "conan-$($Configuration.ToLower())"

# Debug-сборка автоматически включает оверлей отладки (F1 в игре);
# Release собирается без него (флаг TC_DEBUG нигде не определён).
$tcDebugMode = if ($Configuration -eq "Debug") { "ON" } else { "OFF" }

Write-Step "cmake --preset $presetName (TC_DEBUG_MODE=$tcDebugMode)"
& cmake --preset $presetName "-DTC_DEBUG_MODE=$tcDebugMode"
if ($LASTEXITCODE -ne 0) {
    Write-Host "`nКонфигурация CMake завершилась с ошибкой (код $LASTEXITCODE)." -ForegroundColor Red
    exit 1
}

Write-Step "cmake --build --preset $presetName"
& cmake --build --preset $presetName
if ($LASTEXITCODE -ne 0) {
    Write-Host "`nСборка завершилась с ошибкой (код $LASTEXITCODE) - см. ошибки компиляции выше." -ForegroundColor Red
    exit 1
}

# ---------------------------------------------------------------------------
# Готово
# ---------------------------------------------------------------------------
$exePath = Join-Path $root "build\TheCircle.exe"
Write-Host "`n========================================================" -ForegroundColor Green
if (Test-Path $exePath) {
    Write-Host " Сборка готова: $exePath" -ForegroundColor Green
    Write-Host " Запуск: build\TheCircle.exe" -ForegroundColor Green
} else {
    Write-Host " Сборка завершена, но $exePath не найден - проверьте вывод выше." -ForegroundColor Yellow
}
Write-Host "========================================================"

# ---------------------------------------------------------------------------
# Ярлык на .exe в корне проекта
# ---------------------------------------------------------------------------
if (Test-Path $exePath) {
    Write-Step "Ярлык на TheCircle.exe"
    $shortcutPath = Join-Path $root "TheCircle.lnk"
    $shell = New-Object -ComObject WScript.Shell
    $shortcut = $shell.CreateShortcut($shortcutPath)
    $shortcut.TargetPath = $exePath
    $shortcut.WorkingDirectory = Split-Path $exePath
    $shortcut.Save()
    Write-Ok "Создан: $shortcutPath"
}
