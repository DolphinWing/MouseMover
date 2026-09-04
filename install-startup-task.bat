@echo off
setlocal enabledelayedexpansion

set TASK_NAME=dolphin.apps.win32.MouseMover
set TARGET_EXE=%~dp0MouseMover.exe

if not exist "%TARGET_EXE%" (
    set TARGET_EXE=%~dp0Release\MouseMover.exe
)

if not exist "%TARGET_EXE%" (
    echo [ERROR] MouseMover.exe not found in:
    echo   - %~dp0
    echo   - %~dp0Release\
    echo Please build the project or place MouseMover.exe alongside this script.
    echo.
    pause
    exit /b 1
)

echo ========================================================
echo  MouseMover - Scheduled Task Installer
echo ========================================================
echo Task Name : %TASK_NAME%
echo Executable: %TARGET_EXE%
echo Trigger   : At user logon (delayed by 2 minutes)
echo Privilege : Limited (Standard user, no UAC prompt)
echo ========================================================
echo.

schtasks /create /tn "%TASK_NAME%" /tr "\"%TARGET_EXE%\"" /sc onlogon /delay 0002:00 /rl limited /f

if %ERRORLEVEL% equ 0 (
    echo.
    echo [SUCCESS] Scheduled task created successfully!
    echo MouseMover will launch in background 2 minutes after you log on.
) else (
    echo.
    echo [ERROR] Failed to create scheduled task. Exit code: %ERRORLEVEL%
)

echo.
pause
