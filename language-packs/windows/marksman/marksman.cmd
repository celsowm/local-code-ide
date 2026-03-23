@echo off
setlocal
if /I "%LOCALCODEIDE_LSP_SMOKE_CHECK%"=="1" exit /b 0
set "DIR=%~dp0"
if not exist "%DIR%bin\marksman.exe" (
  echo Missing bundled language server binary for marksman: %DIR%bin\marksman.exe
  exit /b 1
)
"%DIR%bin\marksman.exe" %*
exit /b %ERRORLEVEL%
