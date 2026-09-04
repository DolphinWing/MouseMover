@echo off
setlocal

set TASK_NAME=dolphin.apps.win32.MouseMover

echo ========================================================
echo  MouseMover - Scheduled Task Uninstaller
echo ========================================================
echo Removing scheduled task: %TASK_NAME%
echo.

schtasks /delete /tn "%TASK_NAME%" /f

if %ERRORLEVEL% equ 0 (
    echo.
    echo [SUCCESS] Scheduled task removed successfully!
) else (
    echo.
    echo [NOTE] Task "%TASK_NAME%" was not found or already removed.
)

echo.
pause
