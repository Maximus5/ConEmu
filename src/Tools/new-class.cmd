@echo off

if "%~2" == "" (
echo Usage: new-class FileName ClassName
echo Examp: new-class RealConsole CRealCosole
exit /b 1
)

powershell -noprofile -command "& {%~dp0NewClass.ps1 '%~1' '%~2'}"

call "%~dp0add-src.cmd" "%~1.cpp" "%~1.h"
