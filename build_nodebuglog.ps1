[CmdletBinding()]
param()

$ErrorActionPreference = "Stop"
$root = Split-Path -Parent $MyInvocation.MyCommand.Path
Set-Location $root

function Write-Step($msg) { Write-Host "`n==> $msg" -ForegroundColor Cyan }
function Write-Ok($msg)   { Write-Host "    [OK] $msg" -ForegroundColor Green }

$presetName = "conan-release"

if (-not (Test-Path (Join-Path $root "build\CMakePresets.json"))) {
    Write-Host "build\CMakePresets.json not found - run .\setup.ps1 first" -ForegroundColor Red
    exit 1
}

if ($env:VSCMD_ARG_TGT_ARCH -ne "x64") {
    Write-Step "Setting up MSVC x64 environment (vcvarsall.bat)"

    $vswhere = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe"
    $vsInstallPath = $null
    if (Test-Path $vswhere) {
        $vsInstallPath = (& $vswhere -latest -prerelease -products "*" `
            -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 `
            -property installationPath 2>$null | Select-Object -First 1)
    }
    if (-not $vsInstallPath) {
        Write-Host "Visual Studio with C++ not found. Run .\setup.ps1 first." -ForegroundColor Red
        exit 1
    }

    $vcvarsall = Join-Path $vsInstallPath "VC\Auxiliary\Build\vcvarsall.bat"
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
    Write-Ok "MSVC x64 environment already active"
}

$cmakeCache = Join-Path $root "build\CMakeCache.txt"
if (Test-Path $cmakeCache) { Remove-Item $cmakeCache -Force }

Write-Step "cmake --preset $presetName -DTC_DEBUG_LOG=OFF"
& cmake --preset $presetName -DTC_DEBUG_LOG=OFF
if ($LASTEXITCODE -ne 0) {
    Write-Host "CMake configure failed (code $LASTEXITCODE)" -ForegroundColor Red
    exit 1
}

Write-Step "cmake --build --preset $presetName"
& cmake --build --preset $presetName
if ($LASTEXITCODE -ne 0) {
    Write-Host "Build failed (code $LASTEXITCODE)" -ForegroundColor Red
    exit 1
}

Write-Host ""
Write-Host "[OK] Release build without debug logging is ready." -ForegroundColor Green
