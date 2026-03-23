@echo off
setlocal
if /I "%LOCALCODEIDE_LSP_SMOKE_CHECK%"=="1" exit /b 0
set "DIR=%~dp0"
if not exist "%DIR%bin\taplo.exe" (
  echo Missing bundled language server binary for taplo: %DIR%bin\taplo.exe
  exit /b 1
)
"%DIR%bin\taplo.exe" %*
exit /b %ERRORLEVEL%
