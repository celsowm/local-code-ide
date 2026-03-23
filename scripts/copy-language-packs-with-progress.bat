@echo off
setlocal

if "%~3"=="" (
    echo Usage: %~nx0 ^<resources-source^> ^<packs-source^> ^<destination-root^>
    exit /b 2
)

set "SCRIPT_DIR=%~dp0"
powershell -NoProfile -ExecutionPolicy Bypass -File "%SCRIPT_DIR%copy-language-packs-with-progress.ps1" ^
    -ResourcesSource "%~1" ^
    -PacksSource "%~2" ^
    -DestinationRoot "%~3"

exit /b %ERRORLEVEL%
