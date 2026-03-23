@echo off
setlocal
if /I "%LOCALCODEIDE_LSP_SMOKE_CHECK%"=="1" exit /b 0
set "DIR=%~dp0"
if not exist "%DIR%node\pyright-langserver.cmd" (
  echo Missing bundled language server binary for pyright: %DIR%node\pyright-langserver.cmd
  exit /b 1
)
"%DIR%node\pyright-langserver.cmd" %*
exit /b %ERRORLEVEL%
