rem Usage: vc.build.options.cmd [9|10|11|12|14 [core] [x86|x64] [noclean] [dosign|nosign]]]

rem set WIDE=1
rem set IA64=
rem set AMD64=
rem set DEBUG=
rem set NO_RELEASE_PDB=

set VS_VERSION=
set VS_MAKE_SWITCH=/A /B /Y /F
rem set VS_MAKE_TARGETS=
rem set VS_MAKE_FULL=YES
set VS_CLEAN_DBG=YES
set VS_MAKE_CORE=NO
set VS_PLATFORM=ALL
set VS_SIGN=YES

:switches_loop
if "%~1" == "" goto switches_done
for %%i in (9,10,11,12,14) do (if "%%i" == "%~1" (set "VS_VERSION=%%i"))
if /I "%~1" == "nofull"  set VS_MAKE_FULL=NO
if /I "%~1" == "full"    set VS_MAKE_FULL=YES
if /I "%~1" == "noclean" set VS_CLEAN_DBG=NO
if /I "%~1" == "core"    set VS_MAKE_CORE=YES
if /I "%~1" == "x86"     set VS_PLATFORM=X86
if /I "%~1" == "x64"     set VS_PLATFORM=X64
if /I "%~1" == "dosign"  set VS_SIGN=DOSIGN
if /I "%~1" == "nosign"  set VS_SIGN=NOSIGN
shift /1
goto switches_loop

:switches_done

if "%VS_MAKE_FULL%" == "" VS_MAKE_FULL=YES

rem if "%VS_SIGN%" == "DOSIGN" goto do_sign

if "%VS_MAKE_CORE%" == "YES" (
  if "%VS_MAKE_FULL%" == "YES" (
    rem set "VS_MAKE_TARGETS=core"
  ) else (
    rem set "VS_MAKE_TARGETS=coreinc"
    set "VS_MAKE_SWITCH=/F"
  )
) else (
  if NOT "%VS_MAKE_FULL%" == "YES" (
    rem set "VS_MAKE_TARGETS=incremental"
    set "VS_MAKE_SWITCH=/F"
  )
)

if exist makefile_all_vc set MAKEFILE=makefile_all_vc
if exist makefile_lib_vc set MAKEFILE=makefile_lib_vc
if exist makefile_vc set MAKEFILE=makefile_vc
