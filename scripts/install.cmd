@echo off
REM ============================================================
REM  install.cmd — обёртка для install.ps1
REM  Обходит ExecutionPolicy и передаёт все аргументы дальше.
REM
REM  Использование:
REM    install.cmd                    обычная установка
REM    install.cmd -NoProfile         без правки $PROFILE
REM    install.cmd -InstallDir C:\X   своя папка
REM ============================================================

setlocal

set "SCRIPT_DIR=%~dp0"
set "PS_SCRIPT=%SCRIPT_DIR%install.ps1"

if not exist "%PS_SCRIPT%" (
    echo [x] Не найден install.ps1 рядом с install.cmd
    echo     Ожидался: %PS_SCRIPT%
    pause
    exit /b 1
)

powershell.exe -NoProfile -ExecutionPolicy Bypass -File "%PS_SCRIPT%" %*

set "EXIT_CODE=%ERRORLEVEL%"

if %EXIT_CODE% neq 0 (
    echo.
    echo [x] Установка завершилась с кодом %EXIT_CODE%
)

REM Пауза только если запущено двойным кликом (нет родительской консоли)
echo %CMDCMDLINE% | find /i "/c" >nul
if not errorlevel 1 pause

exit /b %EXIT_CODE%