@echo off

setlocal

set ver_info="%~dp0PortableApps\App\AppInfo\appinfo.ini"
set ver_hdr="%~dp0src\ConEmu\version.h"

rem Set curdt variable to YYMMDD
call "%~dp0Deploy\GetCurDate.cmd"

set ConEmuHooks=Enabled

if exist "%~dp0Deploy\user_env.cmd" (
  call "%~dp0Deploy\user_env.cmd"
) else (
  call "%~dp0Deploy\user_env.default.cmd"
)

if not exist "%MINGWRT%head.exe" (
  echo head.exe tool not found, exiting
  exit /b 1
)
if not exist "%CONEMU_WWW%index.html" (
  echo conemu.github.io sources not found, exiting
  exit /b 1
)

set BUILD_NO=
set BUILD_STAGE=

if "%~1"=="" goto noparm

:loop_param
if /I "%~1" == "ALPHA" (
  set BUILD_STAGE=ALPHA
  shift /1
) else if /I "%~1" == "PREVIEW" (
  set BUILD_STAGE=PREVIEW
  shift /1
) else if /I "%~1" == "STABLE" (
  set BUILD_STAGE=STABLE
  shift /1
) else if /I "%~1" == "SIGN" (
  shift /1
  goto do_sign
) else (
  set BUILD_NO=%~1
  shift /1
)
if "%~1" NEQ "" goto loop_param

:oneparm

rem check date, must not bee too late or future ;)
set /A maxdt=%curdt%+1
set /A mindt=%curdt%-1
if "%BUILD_NO:~0,6%" == "%curdt%" goto buildok
if "%BUILD_NO:~0,6%" == "%maxdt%" goto buildok
if "%BUILD_NO:~0,6%" == "%mindt%" goto buildok
echo [1;31;40m------- Warning -------[0m
echo [1;31;40mCheck your build number[0m
echo [1;31;40m  Build: %BUILD_NO%  [0m
echo [1;31;40m  Today: %curdt%  [0m
echo [1;31;40m------- Warning -------[0m
pause
:buildok


call "%~dp0\src\ConEmu\gen_version.cmd" %BUILD_NO% %BUILD_STAGE%
if errorlevel 1 goto err

rem Download new translations from Transifex
call "%~dp0Deploy\l10n_refresh.cmd"

rem This will create ".daily.md"
call "%~dp0Deploy\git2log.cmd" -skip_upd
call :edit -e5 "%CONEMU_WWW%_posts\.daily.md"
call :edit -e23 "%~dp0Release\ConEmu\WhatsNew-ConEmu.txt"

if /I "%~2" == "-build" goto do_build
if /I "%~2" == "-deploy" goto do_deploy

rem echo on

rem Update versions in all release files (msi, portableapps, nuget, etc.)
powershell -noprofile -command "%~dp0Deploy\UpdateDeployVersions.ps1" %BUILD_NO%
if errorlevel 1 goto err


rem set ConEmuHooks=OFF

echo [93;40mVersion from WhatsNew-ConEmu.txt[0m
%MINGWRT%\head -n 30 "%~dp0Release\ConEmu\WhatsNew-ConEmu.txt" | %windir%\system32\find "20%BUILD_NO:~0,2%.%BUILD_NO:~2,2%.%BUILD_NO:~4,2%"
if errorlevel 1 (
%MINGWRT%\head -n 30 "%~dp0Release\ConEmu\WhatsNew-ConEmu.txt" | %MINGWRT%\tail -n -16
echo/
echo [1;31;40mBuild number was not described in WhatsNew-ConEmu.txt![0m
echo/
)

echo [93;40mVersion from PortableApps[0m
type %ver_info% | %MINGWRT%\grep -E "^(PackageVersion|DisplayVersion)"

echo [93;40mVersion from version.h[0m
type %ver_hdr% | %MINGWRT%\grep -G "^#define MVV_"

rem Don't wait for confirmation - build number was already confirmed...
rem echo/
rem echo Press Enter to continue if version is OK: "%BUILD_NO%"
rem pause>nul

rem Give a time to editors to be started
rem timeout /t 15
echo Press Enter to start build
pause > nul

:do_build
cd /d "%~dp0src"

rem touch
rem call :tch common *.cpp *.hpp *.h
rem call :tch ConEmu *.cpp *.h
rem call :tch ConEmuBg *.cpp *.h
rem call :tch ConEmuC *.cpp *.h
rem call :tch ConEmuCD *.cpp *.h
rem call :tch ConEmuDW *.cpp *.h
rem call :tch ConEmuHk *.cpp *.h
rem call :tch ConEmuLn *.cpp *.h
rem call :tch ConEmuPlugin *.cpp *.h
rem call :tch ConEmuTh *.cpp *.h

echo Removing intermediate files...
rd /S /Q "%~dp0src\_VCBUILD\Release"

rem Compile x86/x64
call "%~dp0src\ms.build.release.cmd"
if errorlevel 1 goto err
:do_sign
rem Sign code
call "%~dp0src\vc.build.release.cmd" dosign
if errorlevel 1 goto err

if exist "%UPLOADERS%Check-VirusTotal.cmd" (
	pushd "%~dp0Release"
	echo Starting Check-VirusTotal
	call cmd /c "%UPLOADERS%Check-VirusTotal.cmd" -new_console:bc
	popd
) else (
	echo Check-VirusTotal script not found
)

if exist "%~dp0..\Deploy\av_scan_release.cmd" (
  Echo Scanning release files with av engine
  call "%~dp0..\Deploy\av_scan_release.cmd"
)

if exist "%~dp0Release\UnitTests.cmd" call "%~dp0Release\UnitTests.cmd"

:do_deploy
cd /d "%~dp0"
call Deploy\Deploy.cmd %BUILD_NO%





goto fin


:tch
cd %1
%MINGWRT%\touch %2 %3 %4
cd ..
goto :EOF

:edit
if "%FARRUN_EXIST%" == "NO" (
  start notepad.exe %2
  goto :EOF
)
farrun -new_console:b %1 %2
goto :EOF

:noparm
call "%~dp0Deploy\GetCurVer.cmd"
if "%CurVerBuild%" NEQ "" goto build_found
set CurVerBuild=%curdt%

:build_found
echo Usage:    CreateRelease.cmd ^<Version^> [^<Stage^>]
echo Example:  CreateRelease.cmd %CurVerBuild% %CurVerStage%
echo/
set CurVerBuild=%curdt%
set BUILD_NO=
set BUILD_STAGE=
rem Version
set /P BUILD_NO="Deploy build number [%CurVerBuild%]: "
if "%BUILD_NO%" == "" set "BUILD_NO=%CurVerBuild%"
rem Version stage
set /P BUILD_STAGE="Build stage [%CurVerStage%]: "
if "%BUILD_STAGE%" == "" set "BUILD_STAGE=%CurVerStage%"
echo.

if NOT "%BUILD_NO%"=="" goto oneparm
goto fin

:err
Echo Deploy FAILED!!!

:fin
pause
