@echo off
setlocal
if /I "%LOCALCODEIDE_LSP_SMOKE_CHECK%"=="1" exit /b 0
set "DIR=%~dp0"
if not exist "%DIR%python\Scripts\cmake-language-server.exe" (
  echo Missing bundled language server binary for cmake-language-server: %DIR%python\Scripts\cmake-language-server.exe
  exit /b 1
)
"%DIR%python\Scripts\cmake-language-server.exe" %*
exit /b %ERRORLEVEL%
