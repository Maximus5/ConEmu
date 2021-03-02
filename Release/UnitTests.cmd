@echo off
set ESC=

if NOT "%~1" == "" (
 goto :%~1
 pause
)

@call :clear_env
set ConEmu

cd /d "%~dp0"
start "ConEmu64" "%~dp0ConEmu64.exe" -log -basic -nocascade -wndX 340 -wndY 200 -wndW 150 -wndH 30 -FontDir "%~dp0UnitTests" -run cmd /k "%~0" in_gui
goto :EOF

:clear_env
@echo %ESC%[1;33;40mClearing all ConEmu* environment variables%ESC%[0m
@FOR /F "usebackq delims==" %%i IN (`set ConEmu`) DO @call :clear_var %%i
@goto :EOF

:clear_var
@set %1=
@goto :EOF

:in_gui
call RenameTab "Root"
call cecho /Green "This is first tab, running new tab with two splits"
echo on
c:\windows\system32\cmd.exe -new_console /k "%~0" tests
c:\windows\system32\cmd.exe -new_console /k "%~0" cmd64
c:\windows\syswow64\cmd.exe -new_console:s3TV /k "%~0" cmd32
c:\windows\syswow64\cmd.exe -new_console:b:h999 /k "%~0" ansi
c:\windows\syswow64\cmd.exe -new_console:ba /k "%~0" multi
c:\windows\syswow64\cmd.exe -new_console:bt:"Far":C:"%FARHOME%\Far.exe" /c "%~0" far
c:\windows\syswow64\cmd.exe -new_console:abP:"<Monokai>":t:TabX /k "%~0" monokai
c:\windows\syswow64\cmd.exe -new_console:b:t:VimX /k "%~0" vim
c:\windows\syswow64\cmd.exe -new_console:b:t:BashX /k "%~0" wsl
goto fin

:tests
call RenameTab "Tests"
rem Source cl tests
if exist "%~dp0..\src\UnitTests\run-tests.cmd" call "%~dp0..\src\UnitTests\run-tests.cmd"
goto fin

:cmd64
call RenameTab "cmd64"
call cecho /Green "This is tab1 running x64 cmd"
set Progr
goto fin

:cmd32
call RenameTab "cmd32"
call cecho /Yellow "This is tab2 running x32 cmd"
set Progr
goto fin

:ansi
rem This test for 256color and SEVERAL pause/long line prints
call RenameTab "Ansi"
type "%ConEmuDir%\UnitTests\AnsiColors256.ans"
pause
333333333333333333333333333333333333333333333333333333333333333333333333333333333333333333333333333333333333333333333333333333 3333333
pause
goto fin

:multi
call RenameTab "multi"
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

:far
echo This tab starts Far Manager asynchronously.
echo Root cmd.exe is expected to terminate normally.
conemuc /async /c "%FARHOME%\Far.exe" /w- /x /p"%ConEmuDir%\Plugins\ConEmu;%FARHOME%\Plugins"
goto :EOF

:monokai
echo This tab is elevated cmd.exe started with ^<Monokai^> color scheme.
goto :EOF

:vim
cd /d %~d0\Utils\Alternative\ViM74
call vx.cmd
goto :EOF

:wsl
ConEmuC64.exe -c {bash}
goto :EOF

:fin
pause
