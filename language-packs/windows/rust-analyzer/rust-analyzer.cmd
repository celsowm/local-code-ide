@echo off
setlocal
if /I "%LOCALCODEIDE_LSP_SMOKE_CHECK%"=="1" exit /b 0
set "DIR=%~dp0"
if not exist "%DIR%bin\rust-analyzer.exe" (
  echo Missing bundled language server binary for rust-analyzer: %DIR%bin\rust-analyzer.exe
  exit /b 1
)
"%DIR%bin\rust-analyzer.exe" %*
exit /b %ERRORLEVEL%
