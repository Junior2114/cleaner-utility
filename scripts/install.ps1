#Requires -Version 5.1
[CmdletBinding()]
param(
    [string]$InstallDir = "$env:LOCALAPPDATA\Programs\my-cleaner",
    [switch]$NoBuild,
    [switch]$NoProfile
)

$ErrorActionPreference = 'Stop'
$RepoRoot = $PSScriptRoot

Write-Host "== my-cleaner installer ==" -ForegroundColor Cyan
Write-Host "   Репозиторий : $RepoRoot"
Write-Host "   Установка в : $InstallDir"
Write-Host ""

# ------------------------------------------------------------
# 1. Проверки окружения
# ------------------------------------------------------------

if (-not (Get-Command cmake -ErrorAction SilentlyContinue)) {
    throw "cmake не найден в PATH. Установи CMake и повтори."
}

if (-not (Test-Path (Join-Path $RepoRoot 'CMakeLists.txt'))) {
    throw "Не найден CMakeLists.txt в $RepoRoot. Запусти install.ps1 из корня репозитория."
}

# ------------------------------------------------------------
# 2. Сборка Release
# ------------------------------------------------------------

$buildDir = Join-Path $RepoRoot 'build'

if (-not $NoBuild) {
    if (-not (Test-Path (Join-Path $buildDir 'CMakeCache.txt'))) {
        Write-Host "[*] Конфигурирую CMake (Release)..." -ForegroundColor DarkGray
        cmake -S $RepoRoot -B $buildDir -DCMAKE_BUILD_TYPE=Release
        if ($LASTEXITCODE -ne 0) { throw "cmake configure: exit $LASTEXITCODE" }
    }

    Write-Host "[*] Собираю Release..." -ForegroundColor DarkGray
    cmake --build $buildDir --config Release
    if ($LASTEXITCODE -ne 0) { throw "cmake build: exit $LASTEXITCODE" }
} else {
    Write-Host "[*] Пропускаю сборку (-NoBuild)" -ForegroundColor DarkGray
}

# ------------------------------------------------------------
# 3. Установка через cmake --install
# ------------------------------------------------------------

Write-Host "[*] Устанавливаю в $InstallDir..." -ForegroundColor DarkGray
cmake --install $buildDir --config Release --prefix $InstallDir
if ($LASTEXITCODE -ne 0) { throw "cmake install: exit $LASTEXITCODE" }

$exePath    = Join-Path $InstallDir 'bin\TempCleaner.exe'
$modulePath = Join-Path $InstallDir 'bin\cleaner.ps1'

if (-not (Test-Path $exePath))    { throw "Не найден exe: $exePath" }
if (-not (Test-Path $modulePath)) { throw "Не найден модуль: $modulePath" }

Write-Host "[+] Установлено:" -ForegroundColor Green
Write-Host "      exe    : $exePath"    -ForegroundColor DarkGray
Write-Host "      модуль : $modulePath" -ForegroundColor DarkGray

# ------------------------------------------------------------
# 4. Подключение к $PROFILE
# ------------------------------------------------------------

if ($NoProfile) {
    Write-Host "[*] -NoProfile: пропускаю правку профиля" -ForegroundColor DarkGray
    Write-Host ""
    Write-Host "Чтобы подключить вручную, добавь в свой `$PROFILE строку:" -ForegroundColor Yellow
    Write-Host "  . `"$modulePath`"" -ForegroundColor White
    return
}

$marker  = '# >>> my-cleaner >>>'
$endMark = '# <<< my-cleaner <<<'
$line    = ". `"$modulePath`""

# Используем CurrentUserAllHosts — работает и в powershell.exe, и в pwsh.exe
$profilePath = $PROFILE.CurrentUserAllHosts
$profileDir  = Split-Path $profilePath -Parent

if (-not (Test-Path $profileDir)) {
    New-Item -ItemType Directory -Path $profileDir -Force | Out-Null
}
if (-not (Test-Path $profilePath)) {
    New-Item -ItemType File -Path $profilePath -Force | Out-Null
}

# Читаем содержимое (если файл пустой — получим $null)
$content = Get-Content $profilePath -Raw -ErrorAction SilentlyContinue
if ($null -eq $content) { $content = '' }

# Удаляем старый блок, если он есть
$pattern = [regex]::Escape($marker) + '.*?' + [regex]::Escape($endMark)
$content = [regex]::Replace($content, $pattern, '', 'Singleline')
$content = $content.TrimEnd()

# Добавляем новый
$block = "$marker`n$line`n$endMark"
if ($content.Length -gt 0) {
    $content = $content + "`n`n" + $block
} else {
    $content = $block
}

Set-Content -Path $profilePath -Value $content -Encoding UTF8

Write-Host "[+] Профиль обновлён: $profilePath" -ForegroundColor Green
Write-Host ""
Write-Host "Готово! Перезапусти PowerShell или выполни:" -ForegroundColor Cyan
Write-Host "  . `$PROFILE" -ForegroundColor White
Write-Host ""
Write-Host "Проверка:" -ForegroundColor Cyan
Write-Host "  cleaner version" -ForegroundColor White
Write-Host "  cleaner dry"     -ForegroundColor White
Write-Host ""