@echo off

call %~d0\gcc\gccsetvars.cmd

cd /d "%~dp0"

if exist "%~dp0..\Setupper\Executor.exe" del "%~dp0..\Setupper\Executor.exe"
if exist "%~dp0Executor.gcc" rd /S /Q "%~dp0Executor.gcc"

mingw32-make -f makefile_gcc WIDE=1

if errorlevel 1 goto :EOF

if not "%~1"=="/nosign" call \VCProject\FarPlugin\ConEmu\ConEmu-key\sign_any.bat ..\Setupper\Executor.exe
rd /S /Q %~dp0Release.gcc
