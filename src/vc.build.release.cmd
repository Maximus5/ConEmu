@echo off

rem This is main "script" to create all ConEmu's binaries
rem Build supports Visual Studio versions:
rem   9:  Visual Studio 2008 (SDK 7.0 is required)
rem   10: Visual Studio 2010
rem   11: Visual Studio 2012
rem   12: Visual Studio 2013
rem   14: Visual Studio 2015
rem Script supports optional arguments: [VS_VERSION [nofull] [core]]
rem Example: vc.build.release.cmd 14 nofull

setlocal
cd /d "%~dp0"

rem Check arguments
set VS_VERSION=
for %%i in (9,10,11,12,14) do (if "%%i" == "%~1" (set "VS_VERSION=%%i"))
set VS_MAKE_SWITCH=/A /B /Y /F
set VS_MAKE_TARGETS=
set VS_MAKE_FULL=YES
set VS_MAKE_CORE=NO

:switches_loop
if "%~2" == "" goto switches_done
if /I "%~2" == "nofull" set VS_MAKE_FULL=NO
if /I "%~2" == "core"   set VS_MAKE_CORE=YES
shift /2
goto switches_loop

:switches_done
if "%VS_MAKE_CORE%" == "YES" (
  if "%VS_MAKE_FULL%" == "YES" (
    set "VS_MAKE_TARGETS=core"
  ) else (
    set "VS_MAKE_TARGETS=coreinc"
    set "VS_MAKE_SWITCH=/F"
  )
) else (
  if NOT "%VS_MAKE_FULL%" == "YES" (
    set "VS_MAKE_TARGETS=incremental"
    set "VS_MAKE_SWITCH=/F"
  )
)

if "%VS_VERSION%" NEQ "9" goto skip_sdk_check
call "%~dp0vc.build.sdk.check.cmd"
if errorlevel 1 goto end
:skip_sdk_check

@echo off

if defined ConEmuPID (
  call ConEmuC -GuiMacro Progress 3
)

if /I "%~2" == "nofull" goto skip_clean_dbg
call "%~dp0clean_dbg.cmd"
:skip_clean_dbg

echo Start compilation time
echo Start compilation time>"%~dp0compile.log"
echo %DATE% %TIME%
echo %DATE% %TIME% 1>>"%~dp0compile.log"

if exist makefile_all_vc set MAKEFILE=makefile_all_vc

call "%~dp0vc.build.set.x32.cmd" %VS_VERSION%
if errorlevel 1 goto end

rem goto x64

nmake %VS_MAKE_SWITCH% %MAKEFILE% %VS_MAKE_TARGETS%
if errorlevel 1 (
call :end
exit /b 1
)

:x64

call "%~dp0vc.build.set.x64.cmd" %VS_VERSION%
if errorlevel 1 goto end

nmake %VS_MAKE_SWITCH% %MAKEFILE% %VS_MAKE_TARGETS%
if errorlevel 1 (
call :end
exit /b 1
)

rem if exist "%~dp0clean.cmd" call "%~dp0clean.cmd"

Echo .
Echo Signing release code
cd /d "%~dp0"
if exist "%~dp0sign2.bat" (
  call "%~dp0sign2.bat"
  cd /d "%~dp0"
  
  call "%~dp0scan.release.avp.bat"
  cd /d "%~dp0"
)

echo Compilation finished time
echo Compilation finished time>>"%~dp0compile.log"
echo %DATE% %TIME%
echo %DATE% %TIME% 1>>"%~dp0compile.log"

:end
if defined ConEmuPID (
  call ConEmuC -GuiMacro Progress 0
)
