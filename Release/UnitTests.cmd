@echo off
set ESC=

if NOT "%~1" == "" (
 goto :%~1
 pause
)

@call :clear_env
set ConEmu

cd /d "%~dp0"
start ConEmu /loadcfgfile "%~dp0UnitTests\UnitTests.xml" /cmd cmd /k "%~0" in_gui
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
c:\windows\system32\cmd.exe -new_console /k "%~0" tab1
c:\windows\syswow64\cmd.exe -new_console:s2TV /k "%~0" tab2
c:\windows\syswow64\cmd.exe -new_console:b /k "%~0" tab3
c:\windows\syswow64\cmd.exe -new_console:b /k "%~0" tab4
c:\windows\syswow32\cmd.exe "-new_console:abP:^<Solarized^>" /k "%~0" tab5
c:\windows\syswow32\cmd.exe "-new_console:absVP:^<Solarized^>" /k "%~0" tab6
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
MultiRun.exe
echo Errorlevel=%errorlevel%
MultiRunX64.exe
echo Errorlevel=%errorlevel%
goto fin

:fin
pause
