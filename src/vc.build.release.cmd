@echo off

rem This is main "script" to create all ConEmu's binaries
rem Build supports Visual Studio versions:
rem   9:  Visual Studio 2008 (SDK 7.0 is required)
rem   10: Visual Studio 2010
rem   11: Visual Studio 2012
rem   12: Visual Studio 2013
rem   14: Visual Studio 2015
rem Script supports optional arguments:
rem   [VS_VERSION [nofull] [core] [x86|x64] [noclean] [dosign|nosign]]
rem Example:
rem   vc.build.release.cmd 14 nofull

setlocal
cd /d "%~dp0"

rem Check arguments
set VS_VERSION=
set VS_MAKE_SWITCH=/A /B /Y /F
set VS_MAKE_TARGETS=
set VS_MAKE_FULL=YES
set VS_CLEAN_DBG=YES
set VS_MAKE_CORE=NO
set VS_PLATFORM=ALL
set VS_SIGN=YES

:switches_loop
if "%~1" == "" goto switches_done
for %%i in (9,10,11,12,14) do (if "%%i" == "%~1" (set "VS_VERSION=%%i"))
if /I "%~1" == "nofull"  set VS_MAKE_FULL=NO
if /I "%~1" == "noclean" set VS_CLEAN_DBG=NO
if /I "%~1" == "core"    set VS_MAKE_CORE=YES
if /I "%~1" == "x86"     set VS_PLATFORM=X86
if /I "%~1" == "x64"     set VS_PLATFORM=X64
if /I "%~1" == "dosign"  set VS_SIGN=DOSIGN
if /I "%~1" == "nosign"  set VS_SIGN=NOSIGN
shift /1
goto switches_loop

:switches_done

if "%VS_SIGN%" == "DOSIGN" goto do_sign

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

if defined ConEmuPID (
  call ConEmuC -GuiMacro Progress 3
)

if /I "%VS_CLEAN_DBG%" == "NO" goto skip_clean_dbg
call "%~dp0clean_dbg.cmd"
:skip_clean_dbg

echo Start compilation time
echo Start compilation time>"%~dp0compile.log"
echo %DATE% %TIME%
echo %DATE% %TIME% 1>>"%~dp0compile.log"

if exist makefile_all_vc set MAKEFILE=makefile_all_vc

if /I "%VS_PLATFORM%" == "X64" goto x64

call "%~dp0vc.build.set.x32.cmd" %VS_VERSION%
if errorlevel 1 goto end

nmake %VS_MAKE_SWITCH% %MAKEFILE% %VS_MAKE_TARGETS%
if errorlevel 1 (
call :end
exit /b 1
)

if /I "%VS_PLATFORM%" == "X86" goto skip_x64

:x64

call "%~dp0vc.build.set.x64.cmd" %VS_VERSION%
if errorlevel 1 goto end

nmake %VS_MAKE_SWITCH% %MAKEFILE% %VS_MAKE_TARGETS%
if errorlevel 1 (
call :end
exit /b 1
)

:skip_x64


:do_sign
if "%VS_SIGN%" == "NOSIGN" goto skip_sign
Echo .
Echo Signing release code
cd /d "%~dp0"
if exist "%~dp0sign2.bat" (
  call "%~dp0sign2.bat"
  cd /d "%~dp0"
  
  call "%~dp0scan.release.avp.bat"
  cd /d "%~dp0"
)
:skip_sign

echo Compilation finished time
echo Compilation finished time>>"%~dp0compile.log"
echo %DATE% %TIME%
echo %DATE% %TIME% 1>>"%~dp0compile.log"

:end
if defined ConEmuPID (
  call ConEmuC -GuiMacro Progress 0
)
