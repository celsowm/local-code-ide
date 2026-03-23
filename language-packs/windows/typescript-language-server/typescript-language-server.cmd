@echo off
setlocal
if /I "%LOCALCODEIDE_LSP_SMOKE_CHECK%"=="1" exit /b 0
set "DIR=%~dp0"
if not exist "%DIR%node\typescript-language-server.cmd" (
  echo Missing bundled language server binary for typescript-language-server: %DIR%node\typescript-language-server.cmd
  exit /b 1
)
"%DIR%node\typescript-language-server.cmd" %*
exit /b %ERRORLEVEL%
