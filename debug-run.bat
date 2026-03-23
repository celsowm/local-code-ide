@echo off
echo Starting LocalCodeIDE...
echo.

set QT_LOGGING_RULES=*.debug=true
set QT_QML_DEBUG=1

echo Current directory: %CD%
echo Checking executable...
dir build\Release\LocalCodeIDE.exe
echo.

echo Starting application...
cd build\Release
LocalCodeIDE.exe 2>error.log
set EXITCODE=%ERRORLEVEL%

echo.
echo Exit code: %EXITCODE%
echo.

if exist error.log (
    echo Error log:
    type error.log
) else (
    echo No error log generated
)

exit /b %EXITCODE%
