@echo off

rem This batch creates in the current folder simple portable executable
rem Requirements:
rem  1) WinRar must be installed
rem  2) rar.exe must be in %PATH%
rem  3) Default.sfx must exists near to rar.exe
rem 
rem Batch creates folder "Portable_Test" in the current folder
rem You may create it manually before and put there any files or subfolders.

cd /d "%~dp0.."

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

set ConEmuPortableRarInfo=%~dp0ConEmuPortableRarInfo.txt
call :rar_info "%~1" 1>"%ConEmuPortableRarInfo%"
goto rar_ok

:rar_info
echo Setup=ConEmu.exe
echo Silent=1
echo Overwrite=1
if "%~1"=="" echo TempMode
if not "%~1"=="" echo TempMode=Run ConEmu %~1 portable?, ConEmu %~1
goto :EOF

:rar_ok
if not exist "%~dp0Portable_Test" md "%~dp0Portable_Test"

if not exist "%~dp0Portable_Test\ConEmu.xml" (
echo .
echo .
echo ConEmu.xml not found in "%~dp0Portable_Test"
echo .
set /P CreateXml="Create default ConEmu.xml (Y/N) ? [Y] "
if /I "%CreateXml%"=="N" goto noxml

call :default_xml > "%~dp0Portable_Test\ConEmu.xml"
goto xml_created

:default_xml
echo ^<?xml version="1.0" encoding="utf-8"?^>
echo ^<key name="Software"^>
echo 	^<key name="ConEmu"^>
echo 		^<key name=".Vanilla" modified="2012-06-15 00:00:00"^>
echo 			^<value name="KeyboardHooks" type="hex" data="01"/^>
echo 			^<value name="UseInjects" type="hex" data="01"/^>
echo 			^<value name="Update.CheckOnStartup" type="hex" data="00"/^>
echo 			^<value name="Update.CheckHourly" type="hex" data="00"/^>
echo 			^<value name="Update.ConfirmDownload" type="hex" data="01"/^>
echo 			^<value name="Update.UseBuilds" type="hex" data="02"/^>
echo 			^<value name="TabMargins" type="hex" data="00,00,00,00,19,00,00,00,00,00,00,00,00,00,00,00"/^>
echo 		^</key^>
echo 	^</key^>
echo ^</key^>
goto :EOF

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
set /P CopyMsXml="Do you want to copy these files manually (Y/N/S) ? [N] "
if /I "%CopyMsXml%"=="Y" goto :EOF
if /I "%CopyMsXml%"=="S" goto xml_ok

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
if exist "%~dp0..\Release\ConEmu.exe" (
set RelDir=%~dp0..\Release\
) else if exist "%~dp0ConEmu.exe" (
set RelDir=%~dp0
) else if exist "%~dp0Release\ConEmu.exe" (
cd Release
set RelDir=%~dp0Release
)

if not exist "%RelDir%ConEmu.exe" (
echo Executable not found! "%RelDir%ConEmu.exe"
goto :EOF
)

set inc_List=%inc_List% ConEmu.exe ConEmu\ConEmuC.exe ConEmu\ConEmuCD.dll ConEmu\ConEmuHk.dll
set inc_List=%inc_List% "%~dp0Portable_Test\*.*"

echo .
echo .
set /P Inc64="Include 64-bit console libraries (Y/N) ? [Y] "
if /I "%Inc64%"=="N" goto skip64
set inc_List=%inc_List% ConEmu\ConEmuC64.exe ConEmu\ConEmuCD64.dll ConEmu\ConEmuHk64.dll
:skip64

if exist "%~dp0ConEmuPortable%cever%.exe" del "%~dp0ConEmuPortable%cever%.exe"

echo .
echo All ready, starting rar.exe
echo .

cd /d "%RelDir%"

rar a -s -m5 -r -ep1 -sfxDefault.sfx "-z%ConEmuPortableRarInfo%" "%~dp0ConEmuPortable%cever%.exe"   %inc_List% %exc_List%
if errorlevel 1 pause & goto :EOF

if exist "%RelDir%..\ConEmu-key\sign_any.bat" (
call "%RelDir%..\ConEmu-key\sign_any.bat" "%~dp0ConEmuPortable%cever%.exe"
) else if exist "%RelDir%..\sign_any.bat" (
call "%RelDir%..\sign_any.bat" "%~dp0ConEmuPortable%cever%.exe"
)
