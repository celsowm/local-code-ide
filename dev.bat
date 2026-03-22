@echo off
REM Quick build - uses setup.py
python setup.py
if %ERRORLEVEL% EQU 0 (
    echo.
    echo Build complete!
)
