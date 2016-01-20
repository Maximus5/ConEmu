@echo off

call %~d0\gcc\gccsetvars.cmd

if "%ConEmuHttp%" == "" set ConEmuHttp=https://conemu.github.io/

cd /d "%~dp0"

if exist "%~dp0Setupper.exe" del "%~dp0Setupper.exe"
if exist "%~dp0Release.gcc" rd /S /Q "%~dp0Release.gcc"
if exist "%~dp0Release.x86" rd /S /Q "%~dp0Release.x86"

mingw32-make -f makefile_gcc WIDE=1

if errorlevel 1 goto :EOF

if not "%~1"=="/nosign" call "%~dp0..\..\..\..\ConEmu-key\sign_any.bat" /d "ConEmu ?????? Installer" /du %ConEmuHttp% Setupper.exe

rd /S /Q %~dp0Release.gcc
