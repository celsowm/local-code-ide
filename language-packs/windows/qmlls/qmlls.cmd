@echo off
setlocal
if /I "%LOCALCODEIDE_LSP_SMOKE_CHECK%"=="1" exit /b 0
set "DIR=%~dp0"
if not exist "%DIR%bin\qmlls.exe" (
  echo Missing bundled language server binary for qmlls: %DIR%bin\qmlls.exe
  exit /b 1
)
"%DIR%bin\qmlls.exe" %*
exit /b %ERRORLEVEL%
