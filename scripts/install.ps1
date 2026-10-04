#Requires -Version 5.1
[CmdletBinding()]
param(
    [string]$InstallDir = "$env:LOCALAPPDATA\Programs\my-cleaner",
    [switch]$NoBuild,
    [switch]$NoProfile
)

$ErrorActionPreference = 'Stop'

# ------------------------------------------------------------
# 0. Определяем корень репозитория
# ------------------------------------------------------------
# install.ps1 может лежать:
#   - в scripts/  (репозиторий)   → корень на уровень выше
#   - в корне     (релизный zip)  → корень = папка скрипта
$RepoRoot = $PSScriptRoot
if ((Split-Path $RepoRoot -Leaf) -ieq 'scripts') {
    $RepoRoot = Split-Path $RepoRoot -Parent
}

# Страховка: если в корне нет CMakeLists.txt и нет TempCleaner.exe,
# но есть TempCleaner.exe рядом со скриптом — используем папку скрипта.
if (-not (Test-Path (Join-Path $RepoRoot 'CMakeLists.txt')) -and
    -not (Test-Path (Join-Path $RepoRoot 'TempCleaner.exe')) -and
    (Test-Path (Join-Path $PSScriptRoot 'TempCleaner.exe'))) {
    $RepoRoot = $PSScriptRoot
}

Write-Host "== my-cleaner installer ==" -ForegroundColor Cyan
Write-Host "   Источник    : $RepoRoot"
Write-Host "   Установка в : $InstallDir"
Write-Host ""

# ------------------------------------------------------------
# 1. Получаем exe: готовый или собираем
# ------------------------------------------------------------

$exeSrc = $null
$moduleSrc = $null
$version = "1.0.0"

$localExe = Join-Path $RepoRoot 'TempCleaner.exe'

if (Test-Path $localExe) {
    # --- Режим релиза: exe уже рядом со скриптом ---
    Write-Host "[*] Режим релиза: найден готовый exe" -ForegroundColor DarkGray
    $exeSrc = $localExe

    # Версия из CMakeLists.txt
    $cmakeFile = Join-Path $RepoRoot 'CMakeLists.txt'
    if (Test-Path $cmakeFile) {
        $m = Select-String -Path $cmakeFile -Pattern 'VERSION\s+([0-9]+\.[0-9]+\.[0-9]+)' |
             Select-Object -First 1
        if ($m) { $version = $m.Matches.Groups[1].Value }
    }

    # Готовый модуль cleaner.ps1
    $readyModule = Join-Path $RepoRoot 'cleaner.ps1'
    if (Test-Path $readyModule) {
        $moduleSrc = $readyModule
    }
}
else {
    # --- Режим разработчика: собираем из исходников ---
    if (-not (Get-Command cmake -ErrorAction SilentlyContinue)) {
        throw "cmake не найден в PATH. Либо установи CMake + MSVC, либо скачай релизный zip с готовым exe."
    }

    if (-not (Test-Path (Join-Path $RepoRoot 'CMakeLists.txt'))) {
        throw "Не найден CMakeLists.txt в $RepoRoot. Запусти install.ps1 из корня репозитория."
    }

    $buildDir = Join-Path $RepoRoot 'build'

    if (-not $NoBuild) {
        if (-not (Test-Path (Join-Path $buildDir 'CMakeCache.txt'))) {
            Write-Host "[*] Конфигурирую CMake..." -ForegroundColor DarkGray
            cmake -S $RepoRoot -B $buildDir
            if ($LASTEXITCODE -ne 0) { throw "cmake configure: exit $LASTEXITCODE" }
        }

        Write-Host "[*] Собираю Release..." -ForegroundColor DarkGray
        cmake --build $buildDir --config Release
        if ($LASTEXITCODE -ne 0) { throw "cmake build: exit $LASTEXITCODE" }
    }

    $exeSrc = Join-Path $buildDir 'bin\Release\TempCleaner.exe'
    if (-not (Test-Path $exeSrc)) {
        throw "Не найден $exeSrc после сборки"
    }

    $moduleSrc = Join-Path $buildDir 'cleaner.ps1'
    if (-not (Test-Path $moduleSrc)) {
        throw "Не найден сгенерированный модуль $moduleSrc"
    }
}

Write-Host "[+] Источник exe    : $exeSrc"    -ForegroundColor Green
if ($moduleSrc) {
    Write-Host "[+] Источник модуля : $moduleSrc" -ForegroundColor Green
}
Write-Host ""

# ------------------------------------------------------------
# 2. Копирование в папку установки
# ------------------------------------------------------------

$binDir = Join-Path $InstallDir 'bin'
New-Item -ItemType Directory -Force -Path $binDir | Out-Null

Copy-Item $exeSrc (Join-Path $binDir 'TempCleaner.exe') -Force

# Модуль cleaner.ps1: если готов — копируем; иначе генерируем из шаблона
if ($moduleSrc -and (Test-Path $moduleSrc)) {
    Copy-Item $moduleSrc (Join-Path $binDir 'cleaner.ps1') -Force
}
else {
    $template = Join-Path $RepoRoot 'cleaner.ps1.in'
    if (-not (Test-Path $template)) {
        $template = Join-Path $RepoRoot 'scripts\cleaner.ps1.in'
    }
    if (-not (Test-Path $template)) {
        throw "Не найден cleaner.ps1.in для генерации модуля"
    }
    $content = Get-Content $template -Raw
    $content = $content -replace '@PROJECT_VERSION@', $version
    Set-Content -Path (Join-Path $binDir 'cleaner.ps1') -Value $content -Encoding UTF8
}

# uninstall.ps1
$uninstallSrc = Join-Path $RepoRoot 'uninstall.ps1'
if (-not (Test-Path $uninstallSrc)) {
    $uninstallSrc = Join-Path $RepoRoot 'scripts\uninstall.ps1'
}
if (Test-Path $uninstallSrc) {
    Copy-Item $uninstallSrc (Join-Path $binDir 'uninstall.ps1') -Force
}

$exePath    = Join-Path $binDir 'TempCleaner.exe'
$modulePath = Join-Path $binDir 'cleaner.ps1'

if (-not (Test-Path $exePath))    { throw "Не найден $exePath после копирования" }
if (-not (Test-Path $modulePath)) { throw "Не найден $modulePath после копирования" }

Write-Host "[+] Установлено в $binDir" -ForegroundColor Green
Write-Host "      exe    : $exePath"    -ForegroundColor DarkGray
Write-Host "      модуль : $modulePath" -ForegroundColor DarkGray

# ------------------------------------------------------------
# 3. Прописывание в $PROFILE
# ------------------------------------------------------------

if ($NoProfile) {
    Write-Host ""
    Write-Host "[*] -NoProfile: пропускаю правку профиля" -ForegroundColor DarkGray
    Write-Host ""
    Write-Host "Подключить вручную — добавь в `$PROFILE строку:" -ForegroundColor Yellow
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

Write-Host "[+] Профиль обновлён: $profilePath" -ForegroundColor Green
Write-Host ""
Write-Host "Готово! Перезапусти PowerShell или выполни:" -ForegroundColor Cyan
Write-Host "  . `$PROFILE" -ForegroundColor White
Write-Host ""
Write-Host "Проверка:" -ForegroundColor Cyan
Write-Host "  cleaner version" -ForegroundColor White
Write-Host "  cleaner dry"     -ForegroundColor White
Write-Host ""