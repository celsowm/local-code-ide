@echo off
setlocal
if /I "%LOCALCODEIDE_LSP_SMOKE_CHECK%"=="1" exit /b 0
set "DIR=%~dp0"
if not exist "%DIR%bin\Start-EditorServices.ps1" (
  echo Missing bundled language server binary for powershell-editor-services: %DIR%bin\Start-EditorServices.ps1
  exit /b 1
)
pwsh -NoLogo -NoProfile -ExecutionPolicy Bypass -File "%DIR%bin\Start-EditorServices.ps1" %*
exit /b %ERRORLEVEL%
