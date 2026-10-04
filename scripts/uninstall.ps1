#Requires -Version 5.1
[CmdletBinding()]
param(
    [string]$InstallDir = "$env:LOCALAPPDATA\Programs\my-cleaner",
    [switch]$KeepProfile
)

$ErrorActionPreference = 'Continue'

Write-Host "== my-cleaner uninstaller ==" -ForegroundColor Cyan

# ------------------------------------------------------------
# 1. Убираем блок из $PROFILE
# ------------------------------------------------------------

if (-not $KeepProfile) {
    $marker  = '# >>> my-cleaner >>>'
    $endMark = '# <<< my-cleaner <<<'
    $profilePath = $PROFILE.CurrentUserAllHosts

    if (Test-Path $profilePath) {
        $content = Get-Content $profilePath -Raw -ErrorAction SilentlyContinue
        if ($null -ne $content) {
            $pattern = [regex]::Escape($marker) + '.*?' + [regex]::Escape($endMark)
            $new = [regex]::Replace($content, $pattern, '', 'Singleline').TrimEnd()
            if ($new -ne $content) {
                Set-Content -Path $profilePath -Value $new -Encoding UTF8
                Write-Host "[+] Профиль очищен: $profilePath" -ForegroundColor Green
            } else {
                Write-Host "[*] Блок my-cleaner в профиле не найден" -ForegroundColor DarkGray
            }
        }
    } else {
        Write-Host "[*] Профиль не найден — пропускаю" -ForegroundColor DarkGray
    }
}

# ------------------------------------------------------------
# 2. Удаляем папку установки
# ------------------------------------------------------------

if (Test-Path $InstallDir) {
    # Проверим, что мы не в этой папке
    $cwd = (Get-Location).Path
    if ($cwd.StartsWith($InstallDir, [System.StringComparison]::OrdinalIgnoreCase)) {
        Set-Location $env:TEMP
    }

    try {
        Remove-Item -Recurse -Force $InstallDir -ErrorAction Stop
        Write-Host "[+] Удалено: $InstallDir" -ForegroundColor Green
    } catch {
        Write-Host "[x] Не удалось удалить $InstallDir" -ForegroundColor Red
        Write-Host "    $($_.Exception.Message)" -ForegroundColor DarkGray
        Write-Host "    Закрой окна, которые держат эту папку, и повтори." -ForegroundColor DarkGray
    }
} else {
    Write-Host "[*] Папка установки не найдена: $InstallDir" -ForegroundColor DarkGray
}

Write-Host ""
Write-Host "Готово. Перезапусти PowerShell, чтобы команда cleaner исчезла." -ForegroundColor Cyan