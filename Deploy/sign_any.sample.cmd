@echo off

setlocal

if exist "%~dp0user_env.cmd" (
  call "%~dp0user_env.cmd"
) else (
  call "%~dp0user_env.default.cmd"
)

signtool sign /fd SHA256 %*

endlocal
