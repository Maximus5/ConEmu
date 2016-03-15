@echo off
rem Compile x86
call "%~dp0vc.build.release.cmd" 9 x86 nosign
if errorlevel 1 goto err
rem Compile x64
call "%~dp0vc.build.release.cmd" 14 x64 nosign noclean
if errorlevel 1 goto err
rem Sign code
call "%~dp0vc.build.release.cmd" dosign
if errorlevel 1 goto err
goto :EOF
:err
pause
