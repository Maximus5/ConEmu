@echo off
set ESC=

if NOT "%~1" == "" (
 goto :%~1
 pause
)

@call :clear_env
set ConEmu

cd /d "%~dp0"
rem start ConEmu /loadcfgfile "%~dp0UnitTests\UnitTests.xml" /cmd cmd /k "%~0" in_gui
rem start "ConEmu32" "%~dp0ConEmu.exe" -basic -nocascade -wndX 240 -wndY 100 -wndW 115 -wndH 30 -cmd cmd /k "%~0" in_gui
start "ConEmu64" "%~dp0ConEmu64.exe" -log -basic -nocascade -wndX 340 -wndY 200 -wndW 135 -wndH 30 -FontDir "%~dp0UnitTests" -cmd cmd /k "%~0" in_gui
goto :EOF

:clear_env
@echo %ESC%[1;31;40mClearing all ConEmu* environment variables%ESC%[0m
@FOR /F "usebackq delims==" %%i IN (`set ConEmu`) DO @call :clear_var %%i
@goto :EOF

:clear_var
@set %1=
@goto :EOF

:in_gui
call RenameTab "Root"
call cecho /Green "This is first tab, running new tab with two splits"
echo on
c:\windows\system32\cmd.exe -new_console /k "%~0" tab1
c:\windows\syswow64\cmd.exe -new_console:s2TV /k "%~0" tab2
c:\windows\syswow64\cmd.exe -new_console:b /k "%~0" tab3
c:\windows\syswow64\cmd.exe -new_console:ba /k "%~0" tab4
c:\windows\syswow64\cmd.exe -new_console:bt:"Far":C:"%ConEmuDrive%\Far30.Latest\far-bis\Far.exe" /c "%~0" tab5
c:\windows\syswow64\cmd.exe -new_console:abP:"<Monokai>":t:Tab6 /k "%~0" tab6
c:\windows\syswow64\cmd.exe -new_console:b:t:Tab7 /k "%~0" tab7
rem Source cl tests
if exist "%~dp0..\src\UnitTests\run-tests.cmd" call "%~dp0..\src\UnitTests\run-tests.cmd"
goto fin

:tab1
call RenameTab "Tab1"
call cecho /Green "This is tab1 running x64 cmd"
set Progr
goto fin

:tab2
call RenameTab "Tab2"
call cecho /Yellow "This is tab2 running x32 cmd"
set Progr
goto fin

:tab3
rem This test for 256color and SEVERAL pause/long line prints
call RenameTab "Ansi"
type "%ConEmuDir%\UnitTests\AnsiColors256.ans"
pause
333333333333333333333333333333333333333333333333333333333333333333333333333333333333333333333333333333333333333333333333333333 3333333
pause
goto fin

:tab4
call RenameTab "Test"
cd /d "%~dp0UnitTests"
MultiRun.exe /exit
echo Errorlevel=%errorlevel%
rem MultiRunX64.exe /exit
rem echo Errorlevel=%errorlevel%
MultiRun.exe /term
echo Errorlevel=%errorlevel%
MultiRunX64.exe /term
echo Errorlevel=%errorlevel%
rem MultiRun.exe
rem echo Errorlevel=%errorlevel%
MultiRunX64.exe
echo Errorlevel=%errorlevel%
goto fin

:tab5
conemuc /async /c %ConEmuDrive%\Far30.Latest\far-bis\Far.exe /w- /x /p%ConEmuDir%\Plugins\ConEmu;%ConEmuDrive%\Far30.Latest\far-bis\Plugins;%ConEmuDrive%\Far30.Latest\Plugins;%ConEmuDrive%\Far30.Latest\Plugins.My
goto :EOF

:tab6
goto :EOF

:tab7
cd /d %~d0\Utils\Alternative\ViM74
call vx.cmd
goto :EOF

:fin
pause
