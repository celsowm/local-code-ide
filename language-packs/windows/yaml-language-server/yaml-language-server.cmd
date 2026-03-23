@echo off
setlocal
if /I "%LOCALCODEIDE_LSP_SMOKE_CHECK%"=="1" exit /b 0
set "DIR=%~dp0"
if not exist "%DIR%node\yaml-language-server.cmd" (
  echo Missing bundled language server binary for yaml-language-server: %DIR%node\yaml-language-server.cmd
  exit /b 1
)
"%DIR%node\yaml-language-server.cmd" %*
exit /b %ERRORLEVEL%
