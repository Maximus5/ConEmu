@echo off

rem This is main "script" to create all ConEmu's binaries
rem Build supports Visual Studio versions:
rem   9:  Visual Studio 2008 (SDK 7.0 is required)
rem   10: Visual Studio 2010
rem   11: Visual Studio 2012
rem   12: Visual Studio 2013
rem   14: Visual Studio 2015
rem   15: Visual Studio 2017
rem Script supports optional arguments:
rem   [VS_VERSION [full|nofull] [core] [x86|x64] [noclean] [dosign|nosign]]
rem Example:
rem   vc.build.release.cmd 15 nofull

setlocal
cd /d "%~dp0"

rem Check arguments (default to FULL make)
set VS_MAKE_FULL=YES
set VS_MAKE_TARGETS=
call vc.build.options.cmd %*

if "%VS_SIGN%" == "DOSIGN" goto do_sign

rem rem Check arguments
rem set VS_VERSION=
rem set VS_MAKE_SWITCH=/A /B /Y /F
rem set VS_MAKE_TARGETS=
rem set VS_MAKE_FULL=YES
rem set VS_CLEAN_DBG=YES
rem set VS_MAKE_CORE=NO
rem set VS_PLATFORM=ALL
rem set VS_SIGN=YES
rem 
rem :switches_loop
rem if "%~1" == "" goto switches_done
rem for %%i in (9,10,11,12,14) do (if "%%i" == "%~1" (set "VS_VERSION=%%i"))
rem if /I "%~1" == "nofull"  set VS_MAKE_FULL=NO
rem if /I "%~1" == "noclean" set VS_CLEAN_DBG=NO
rem if /I "%~1" == "core"    set VS_MAKE_CORE=YES
rem if /I "%~1" == "x86"     set VS_PLATFORM=X86
rem if /I "%~1" == "x64"     set VS_PLATFORM=X64
rem if /I "%~1" == "dosign"  set VS_SIGN=DOSIGN
rem if /I "%~1" == "nosign"  set VS_SIGN=NOSIGN
rem shift /1
rem goto switches_loop
rem 
rem :switches_done
rem 
rem if "%VS_SIGN%" == "DOSIGN" goto do_sign

if "%VS_MAKE_CORE%" == "YES" (
  if "%VS_MAKE_FULL%" == "YES" (
    set "VS_MAKE_TARGETS=core"
  ) else (
    set "VS_MAKE_TARGETS=coreinc"
    rem set "VS_MAKE_SWITCH=/F"
  )
) else (
  if NOT "%VS_MAKE_FULL%" == "YES" (
    set "VS_MAKE_TARGETS=incremental"
    rem set "VS_MAKE_SWITCH=/F"
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
call "%~dp0clean.cmd"
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
  
  if exist "%~dp0scan.release.avp.bat" (
    call "%~dp0scan.release.avp.bat"
    cd /d "%~dp0"
  )
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
