@echo off
cd /d "%~dp0"

set cever=
if not "%~1"=="" set cever=.%~1
set inc_List=
set exc_List=

echo Setup=ConEmu.exe>ConEmuPortableRarInfo.txt
echo Silent=1 1>>ConEmuPortableRarInfo.txt
echo Overwrite=1 1>>ConEmuPortableRarInfo.txt
if "%~1"=="" echo TempMode>>ConEmuPortableRarInfo.txt
if not "%~1"=="" echo TempMode=Run ConEmu %~1 portable?, ConEmu %~1 1>>ConEmuPortableRarInfo.txt

if not exist Portable_Test md Portable_Test

cd Release

set inc_List=%inc_List% ConEmu.exe ConEmu\ConEmuC.exe ConEmu\ConEmuCD.dll ConEmu\ConEmuHk.dll
set inc_List=%inc_List% ConEmu\ConEmuC64.exe ConEmu\ConEmuCD64.dll ConEmu\ConEmuHk64.dll
set inc_List=%inc_List% ..\Portable_Test\*.*

if exist "..\ConEmuPortable%cever%.exe" del "..\ConEmuPortable%cever%.exe"

rar a -r -ep1 -sfxDefault.sfx -z..\ConEmuPortableRarInfo.txt "..\ConEmuPortable%cever%.exe"   %inc_List% %exc_List%
if errorlevel 1 pause & goto :EOF

if exist "..\..\ConEmu-key\sign_any.bat" call "..\..\ConEmu-key\sign_any.bat" "..\ConEmuPortable%cever%.exe"
