@echo off
setlocal EnableDelayedExpansion

REM Incremental compile before running (builds only what changed)
if not exist build\CMakeCache.txt (
    echo Build not configured. Running full setup...
    python setup.py
    if %ERRORLEVEL% NEQ 0 exit /b %ERRORLEVEL%
) else (
    echo Checking for changes and compiling in parallel...
    REM Use --parallel to leverage all CPU cores for faster builds
    cmake --build build --config Release --parallel
    if %ERRORLEVEL% NEQ 0 exit /b %ERRORLEVEL%
)

REM Mandatory preflight checks before launching
echo Running doctor preflight checks...
set LOCALCODEIDE_DOCTOR_SKIP_STARTUP=1
if "%LOCALCODEIDE_LOG_LEVEL%"=="" set LOCALCODEIDE_LOG_LEVEL=info
if "%LOCALCODEIDE_KEEP_LOG_FILES%"=="" set LOCALCODEIDE_KEEP_LOG_FILES=20
if "%LOCALCODEIDE_ENABLE_MINIDUMP%"=="" set LOCALCODEIDE_ENABLE_MINIDUMP=0
REM Note: setup.py doctor internally runs linting ( Ruff and qmllint )
REM qmllint can be slow, but currently setup.py runs them sequentially.
python setup.py doctor
if %ERRORLEVEL% NEQ 0 exit /b %ERRORLEVEL%
set LOCALCODEIDE_DOCTOR_SKIP_STARTUP=

REM Run the application
python setup.py run
set EXITCODE=%ERRORLEVEL%
if %EXITCODE% NEQ 0 (
    echo.
    echo ============================================================
    echo LocalCodeIDE crashed or exited with error code %EXITCODE%.
    echo ============================================================
    set "LATEST_LOG_DIR="
    for /f "delims=" %%I in ('dir /ad /b /o-d build\logs 2^>nul') do (
        if not defined LATEST_LOG_DIR set "LATEST_LOG_DIR=build\logs\%%I"
    )
    if defined LATEST_LOG_DIR (
        echo Latest session: !LATEST_LOG_DIR!
        if exist "!LATEST_LOG_DIR!\session.json" (
            echo Session metadata: !LATEST_LOG_DIR!\session.json
        )
        if exist "!LATEST_LOG_DIR!\runtime.log" (
            echo.
            echo Last log lines:
            powershell -NoProfile -Command "Get-Content -Path '!LATEST_LOG_DIR!\runtime.log' -Tail 30"
            for %%S in ("!LATEST_LOG_DIR!\runtime.log") do set "RUNTIME_SIZE=%%~zS"
            if "!RUNTIME_SIZE!"=="0" (
                if exist "!LATEST_LOG_DIR!\app.log" (
                    echo.
                    echo runtime.log is empty. Last app.log lines:
                    powershell -NoProfile -Command "Get-Content -Path '!LATEST_LOG_DIR!\app.log' -Tail 30"
                ) else (
                    echo.
                    echo runtime.log and app.log are empty or missing.
                    echo This can happen on fail-fast native crashes before handlers run.
                    if "%LOCALCODEIDE_ENABLE_MINIDUMP%"=="0" (
                        echo Tip: set LOCALCODEIDE_ENABLE_MINIDUMP=1 to generate build\logs\...\crash.dmp.
                    )
                )
            )
        )
    ) else (
        echo No run session logs found in build\logs.
    )
    echo.
    pause
)

exit /b %EXITCODE%
