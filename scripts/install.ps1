#Requires -Version 5.1
[CmdletBinding()]
param(
    [string]$InstallDir = "$env:LOCALAPPDATA\Programs\my-cleaner",
    [switch]$NoBuild,
    [switch]$NoProfile
)

$ErrorActionPreference = 'Stop'

# ------------------------------------------------------------
# 0. Detect repository root
# ------------------------------------------------------------
# install.ps1 may live in:
#   - scripts/  (git repo)      -> root is one level up
#   - root      (release zip)   -> root is script folder
$RepoRoot = $PSScriptRoot
if ((Split-Path $RepoRoot -Leaf) -ieq 'scripts') {
    $RepoRoot = Split-Path $RepoRoot -Parent
}

# Safety net for flat release zip
if (-not (Test-Path (Join-Path $RepoRoot 'CMakeLists.txt')) -and
    -not (Test-Path (Join-Path $RepoRoot 'TempCleaner.exe')) -and
    (Test-Path (Join-Path $PSScriptRoot 'TempCleaner.exe'))) {
    $RepoRoot = $PSScriptRoot
}

Write-Host "== my-cleaner installer ==" -ForegroundColor Cyan
Write-Host "   Source    : $RepoRoot"
Write-Host "   Install to: $InstallDir"
Write-Host ""

# ------------------------------------------------------------
# 1. Get exe: prebuilt or build from source
# ------------------------------------------------------------

$exeSrc    = $null
$moduleSrc = $null
$version   = "1.0.0"

$localExe = Join-Path $RepoRoot 'TempCleaner.exe'

if (Test-Path $localExe) {
    # --- Release mode ---
    Write-Host "[*] Release mode: using prebuilt exe" -ForegroundColor DarkGray
    $exeSrc = $localExe

    $cmakeFile = Join-Path $RepoRoot 'CMakeLists.txt'
    if (Test-Path $cmakeFile) {
        $m = Select-String -Path $cmakeFile -Pattern 'VERSION\s+([0-9]+\.[0-9]+\.[0-9]+)' |
             Select-Object -First 1
        if ($m) { $version = $m.Matches.Groups[1].Value }
    }

    $readyModule = Join-Path $RepoRoot 'cleaner.ps1'
    if (Test-Path $readyModule) {
        $moduleSrc = $readyModule
    }
}
else {
    # --- Developer mode: build from source ---
    if (-not (Get-Command cmake -ErrorAction SilentlyContinue)) {
        throw "cmake not found in PATH. Install CMake + MSVC, or download a release zip with a prebuilt exe."
    }

    if (-not (Test-Path (Join-Path $RepoRoot 'CMakeLists.txt'))) {
        throw "CMakeLists.txt not found in $RepoRoot. Run install.ps1 from the repository root."
    }

    $buildDir = Join-Path $RepoRoot 'build'

    if (-not $NoBuild) {
        if (-not (Test-Path (Join-Path $buildDir 'CMakeCache.txt'))) {
            Write-Host "[*] Configuring CMake..." -ForegroundColor DarkGray
            cmake -S $RepoRoot -B $buildDir
            if ($LASTEXITCODE -ne 0) { throw "cmake configure: exit $LASTEXITCODE" }
        }

        Write-Host "[*] Building Release..." -ForegroundColor DarkGray
        cmake --build $buildDir --config Release
        if ($LASTEXITCODE -ne 0) { throw "cmake build: exit $LASTEXITCODE" }
    }

    $exeSrc = Join-Path $buildDir 'bin\Release\TempCleaner.exe'
    if (-not (Test-Path $exeSrc)) {
        throw "Not found after build: $exeSrc"
    }

    $moduleSrc = Join-Path $buildDir 'cleaner.ps1'
    if (-not (Test-Path $moduleSrc)) {
        throw "Generated module not found: $moduleSrc"
    }
}

Write-Host "[+] exe source   : $exeSrc" -ForegroundColor Green
if ($moduleSrc) {
    Write-Host "[+] module source: $moduleSrc" -ForegroundColor Green
}
Write-Host ""

# ------------------------------------------------------------
# 2. Copy to install dir
# ------------------------------------------------------------

$binDir = Join-Path $InstallDir 'bin'
New-Item -ItemType Directory -Force -Path $binDir | Out-Null

Copy-Item $exeSrc (Join-Path $binDir 'TempCleaner.exe') -Force

if ($moduleSrc -and (Test-Path $moduleSrc)) {
    Copy-Item $moduleSrc (Join-Path $binDir 'cleaner.ps1') -Force
}
else {
    $template = Join-Path $RepoRoot 'cleaner.ps1.in'
    if (-not (Test-Path $template)) {
        $template = Join-Path $RepoRoot 'scripts\cleaner.ps1.in'
    }
    if (-not (Test-Path $template)) {
        throw "cleaner.ps1.in not found; cannot generate module"
    }
    $content = Get-Content $template -Raw
    $content = $content -replace '@PROJECT_VERSION@', $version
    Set-Content -Path (Join-Path $binDir 'cleaner.ps1') -Value $content -Encoding UTF8
}

$uninstallSrc = Join-Path $RepoRoot 'uninstall.ps1'
if (-not (Test-Path $uninstallSrc)) {
    $uninstallSrc = Join-Path $RepoRoot 'scripts\uninstall.ps1'
}
if (Test-Path $uninstallSrc) {
    Copy-Item $uninstallSrc (Join-Path $binDir 'uninstall.ps1') -Force
}

$exePath    = Join-Path $binDir 'TempCleaner.exe'
$modulePath = Join-Path $binDir 'cleaner.ps1'

if (-not (Test-Path $exePath))    { throw "Missing after copy: $exePath" }
if (-not (Test-Path $modulePath)) { throw "Missing after copy: $modulePath" }

Write-Host "[+] Installed to $binDir" -ForegroundColor Green
Write-Host "      exe   : $exePath"    -ForegroundColor DarkGray
Write-Host "      module: $modulePath" -ForegroundColor DarkGray

# ------------------------------------------------------------
# 3. Register in $PROFILE
# ------------------------------------------------------------

if ($NoProfile) {
    Write-Host ""
    Write-Host "[*] -NoProfile: skipping profile update" -ForegroundColor DarkGray
    Write-Host ""
    Write-Host "To enable manually, add to your `$PROFILE:" -ForegroundColor Yellow
    Write-Host "  . `"$modulePath`"" -ForegroundColor White
    return
}

$marker  = '# >>> my-cleaner >>>'
$endMark = '# <<< my-cleaner <<<'
$line    = ". `"$modulePath`""

$profilePath = $PROFILE.CurrentUserAllHosts
$profileDir  = Split-Path $profilePath -Parent

if (-not (Test-Path $profileDir)) {
    New-Item -ItemType Directory -Path $profileDir -Force | Out-Null
}
if (-not (Test-Path $profilePath)) {
    New-Item -ItemType File -Path $profilePath -Force | Out-Null
}

$content = Get-Content $profilePath -Raw -ErrorAction SilentlyContinue
if ($null -eq $content) { $content = '' }

$pattern = [regex]::Escape($marker) + '.*?' + [regex]::Escape($endMark)
$content = [regex]::Replace($content, $pattern, '', 'Singleline').TrimEnd()

$block = "$marker`n$line`n$endMark"
if ($content.Length -gt 0) {
    $content = $content + "`n`n" + $block
} else {
    $content = $block
}

Set-Content -Path $profilePath -Value $content -Encoding UTF8

Write-Host "[+] Profile updated: $profilePath" -ForegroundColor Green
Write-Host ""
Write-Host "Done. Restart PowerShell or run:" -ForegroundColor Cyan
Write-Host "  . `$PROFILE" -ForegroundColor White
Write-Host ""
Write-Host "Verify:" -ForegroundColor Cyan
Write-Host "  cleaner version" -ForegroundColor White
Write-Host "  cleaner dry"     -ForegroundColor White
Write-Host ""
