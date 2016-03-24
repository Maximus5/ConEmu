@echo off

setlocal

set new_con=

rem Failed to pass complex strings as ps1 argument
rem So, we use env var instead...
if "%~1" == "" (
  set "ce_add_id=-"
  set new_con=-new_console:s30V
) else (
  set "ce_add_id=%~1"
)
set "ce_add_str=%~2"

powershell -noprofile -command "%~dp0..\..\Deploy\rc2json.ps1" -mode add -id "%ce_add_id%" %3 %new_con%
