@echo off
call "%VS90COMNTOOLS%vsvars32.bat" x86
cd /d "%~dp0"

set PLATFORM=x86
set RELEASE=1

nmake /f makefile_vc
if errorlevel 1 goto :EOF

call \VCProject\FarPlugin\ConEmu\ConEmu-key\sign_any.bat Release.x86\Setupper.exe
move Release.x86\Setupper.exe .
rd /S /Q %~dp0Release.x86
