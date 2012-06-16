@echo off

rem This batch creates in the current folder simple portable executable
rem Requirements:
rem  1) WinRar must be installed
rem  2) rar.exe must be in %PATH%
rem  3) Default.sfx must exists near to rar.exe
rem 
rem Batch creates folder "Portable_Test" in the current folder
rem You may create it manually before and put there any files or subfolders.

cd /d "%~dp0"

set cever=
if not "%~1"=="" (
set cever=.%~1
goto ver_ok
)

set /P Lbl="Choose label for 'ConEmuPortable.label.exe' [without label] "
if not "%Lbl%"=="" set cever=.%Lbl%

:ver_ok

set inc_List=
set exc_List=-xDeploy

echo Setup=ConEmu.exe>ConEmuPortableRarInfo.txt
echo Silent=1 1>>ConEmuPortableRarInfo.txt
echo Overwrite=1 1>>ConEmuPortableRarInfo.txt
if "%~1"=="" echo TempMode>>ConEmuPortableRarInfo.txt
if not "%~1"=="" echo TempMode=Run ConEmu %~1 portable?, ConEmu %~1 1>>ConEmuPortableRarInfo.txt

if not exist Portable_Test md Portable_Test

if not exist "%~dp0Portable_Test\ConEmu.xml" (
echo .
echo .
echo ConEmu.xml not found in "%~dp0Portable_Test"
echo .
set /P CreateXml="Create default ConEmu.xml (Y/N) ? [Y] "
if "%CreateXml%"=="N" goto noxml
if "%CreateXml%"=="n" goto noxml

goto default_xml

:default_xml
echo ^<?xml version="1.0" encoding="utf-8"?^> > "%~dp0Portable_Test\ConEmu.xml"
echo ^<key name="Software"^> >> "%~dp0Portable_Test\ConEmu.xml"
echo 	^<key name="ConEmu"^> >> "%~dp0Portable_Test\ConEmu.xml"
echo 		^<key name=".Vanilla" modified="2012-06-15 00:00:00"^> >> "%~dp0Portable_Test\ConEmu.xml"
echo 			^<value name="KeyboardHooks" type="hex" data="01"/^> >> "%~dp0Portable_Test\ConEmu.xml"
echo 			^<value name="UseInjects" type="hex" data="01"/^> >> "%~dp0Portable_Test\ConEmu.xml"
echo 			^<value name="Update.CheckOnStartup" type="hex" data="00"/^> >> "%~dp0Portable_Test\ConEmu.xml"
echo 			^<value name="Update.CheckHourly" type="hex" data="00"/^> >> "%~dp0Portable_Test\ConEmu.xml"
echo 			^<value name="Update.ConfirmDownload" type="hex" data="01"/^> >> "%~dp0Portable_Test\ConEmu.xml"
echo 			^<value name="Update.UseBuilds" type="hex" data="02"/^> >> "%~dp0Portable_Test\ConEmu.xml"
echo 			^<value name="TabMargins" type="hex" data="00,00,00,00,19,00,00,00,00,00,00,00,00,00,00,00"/^> >> "%~dp0Portable_Test\ConEmu.xml"
echo 		^</key^> >> "%~dp0Portable_Test\ConEmu.xml"
echo 	^</key^> >> "%~dp0Portable_Test\ConEmu.xml"
echo ^</key^> >> "%~dp0Portable_Test\ConEmu.xml"
goto xml_created

) else (

goto xml_created

)

:xml_created
if not exist "%~dp0Portable_Test\msxml3.dll" goto no_msxml
if not exist "%~dp0Portable_Test\msxml3r.dll" goto no_msxml
goto xml_ok
:no_msxml
echo .
echo .
echo ConEmu.xml exist, but "msxml3.dll" and "msxml3r.dll"
echo not found in "%~dp0Portable_Test"
echo .
echo Warning! These files are strongly recommended for "Portable" configurations!
echo Warning! Windows XP versions of these files are preferred!
echo .
set /P CopyMsXml="Do you want to copy these files manually (Y/N) ? [N] "
if "%CopyMsXml%"=="Y" goto :EOF
if "%CopyMsXml%"=="y" goto :EOF

if exist "%WinDir%\SysWOW64\msxml3.dll" (
set xml_src_dir=%WinDir%\SysWOW64
) else (
set xml_src_dir=%WinDir%\System32
)
copy "%xml_src_dir%\msxml3.dll" "%~dp0Portable_Test\msxml3.dll"
copy "%xml_src_dir%\msxml3r.dll" "%~dp0Portable_Test\msxml3r.dll"
rem One more check
goto xml_created
)
:xml_ok

:noxml
if exist "%~dp0ConEmu.exe" (
set RelDir=
) else if exist "%~dp0Release\ConEmu.exe" (
cd Release
set RelDir=..\
)

set inc_List=%inc_List% ConEmu.exe ConEmu\ConEmuC.exe ConEmu\ConEmuCD.dll ConEmu\ConEmuHk.dll
set inc_List=%inc_List% %RelDir%Portable_Test\*.*

echo .
echo .
set /P Inc64="Include 64-bit console libraries (Y/N) ? [Y] "
if "%Inc64%"=="N" goto skip64
if "%Inc64%"=="n" goto skip64
set inc_List=%inc_List% ConEmu\ConEmuC64.exe ConEmu\ConEmuCD64.dll ConEmu\ConEmuHk64.dll
:skip64

if exist "%RelDir%ConEmuPortable%cever%.exe" del "%RelDir%ConEmuPortable%cever%.exe"

echo .
echo All ready, starting rar.exe
echo .

rar a -s -m5 -r -ep1 -sfxDefault.sfx -z%RelDir%ConEmuPortableRarInfo.txt "%RelDir%ConEmuPortable%cever%.exe"   %inc_List% %exc_List%
if errorlevel 1 pause & goto :EOF

if exist "%RelDir%..\ConEmu-key\sign_any.bat" (
call "%RelDir%..\ConEmu-key\sign_any.bat" "%RelDir%ConEmuPortable%cever%.exe"
) else if exist "%RelDir%..\sign_any.bat" (
call "%RelDir%..\sign_any.bat" "%RelDir%ConEmuPortable%cever%.exe"
)
