@echo off
setlocal

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
REM Note: setup.py doctor internally runs linting ( Ruff and qmllint )
REM qmllint can be slow, but currently setup.py runs them sequentially.
python setup.py doctor
if %ERRORLEVEL% NEQ 0 exit /b %ERRORLEVEL%
set LOCALCODEIDE_DOCTOR_SKIP_STARTUP=

REM Run the application
python setup.py run
exit /b %ERRORLEVEL%
