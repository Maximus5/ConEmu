@echo off

if not exist "%~dp0sign_any.cmd" (
  echo **********************************
  echo **********************************
  echo **     SKIPPING code signing    **
  echo **********************************
  echo **********************************
  goto :EOF
)

echo Signing code
if exist "%~1ConEmu\ConEmuC64.exe" set RELDIR=%~1&goto RelDirSet
set RELDIR=..\Release\
:RelDirSet


set RELDIRCTH=%RELDIR%plugins\ConEmu\Thumbs

set SIGFILES=%RELDIR%ConEmu.exe %RELDIR%ConEmu64.exe
set SIGFILES=%SIGFILES% %RELDIR%ConEmu\ConEmuC.exe %RELDIR%ConEmu\ConEmuC64.exe
set SIGFILES=%SIGFILES% %RELDIR%ConEmu\ConEmuCD.dll %RELDIR%ConEmu\ConEmuCD64.dll
set SIGFILES=%SIGFILES% %RELDIR%ConEmu\ConEmuHk.dll %RELDIR%ConEmu\ConEmuHk64.dll
set SIGFILES=%SIGFILES% %RELDIR%ConEmu\ExtendedConsole.dll %RELDIR%ConEmu\ExtendedConsole64.dll
set SIGFILES=%SIGFILES% %RELDIR%plugins\ConEmu\conemu.dll %RELDIR%plugins\ConEmu\Lines\ConEmuLn.dll
set SIGFILES=%SIGFILES% %RELDIRCTH%\ConEmuTh.dll %RELDIRCTH%\gdi+.t32 %RELDIRCTH%\ico.t32 %RELDIRCTH%\pe.t32

if "%~2"=="/NOSIGN64PLUG" goto skip_64
set SIGFILES=%SIGFILES% %RELDIR%plugins\ConEmu\conemu.x64.dll %RELDIR%plugins\ConEmu\Lines\ConEmuLn.x64.dll
set SIGFILES=%SIGFILES% %RELDIRCTH%\ConEmuTh.x64.dll %RELDIRCTH%\gdi+.t64 %RELDIRCTH%\ico.t64 %RELDIRCTH%\pe.t64
:skip_64

call :dosign %SIGFILES%
if errorlevel 1 goto failsign

if "%TS"=="" (
  echo **********************************
  echo **********************************
  echo ** Binaries was NOT timestamped **
  echo **********************************
  echo **********************************
)

goto end

:dosign
echo %*
call "%~dp0sign_any.cmd" %*
if errorlevel 1 goto :EOF
goto :EOF


:failsign
echo .
echo !!! Signing code failed !!!
echo .
pause

:end
