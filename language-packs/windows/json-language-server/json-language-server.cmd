@echo off
setlocal
if /I "%LOCALCODEIDE_LSP_SMOKE_CHECK%"=="1" exit /b 0
set "DIR=%~dp0"
if not exist "%DIR%node\vscode-json-language-server.cmd" (
  echo Missing bundled language server binary for json-language-server: %DIR%node\vscode-json-language-server.cmd
  exit /b 1
)
"%DIR%node\vscode-json-language-server.cmd" %*
exit /b %ERRORLEVEL%
