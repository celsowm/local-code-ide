@echo off
setlocal
if /I "%LOCALCODEIDE_LSP_SMOKE_CHECK%"=="1" exit /b 0
set "DIR=%~dp0"
if not exist "%DIR%bin\clangd.exe" (
  echo Missing bundled language server binary for clangd: %DIR%bin\clangd.exe
  exit /b 1
)
"%DIR%bin\clangd.exe" %*
exit /b %ERRORLEVEL%
