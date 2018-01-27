@echo off

call "%~dp0vc.build.set.any.cmd" %*

if NOT defined VS_COMNTOOLS (
  echo VS_COMNTOOLS NOT DEFINED!
  exit /B 100
  goto :EOF
)

set LIB=

:aux
if exist "%VS_COMNTOOLS%\VC\Auxiliary\Build\vcvars32.bat" (
  echo trying vcvars32.bat
  call "%VS_COMNTOOLS%\VC\Auxiliary\Build\vcvars32.bat"
  if defined LIB goto next
)

:bin
if exist "%VS_COMNTOOLS%..\..\VC\BIN\vcvars32.bat" (
  echo trying BIN\vcvars32.bat
  call "%VS_COMNTOOLS%..\..\VC\BIN\vcvars32.bat"
  if defined LIB goto next
)

:vcvarsall
if exist "%VS_COMNTOOLS%..\..\VC\vcvarsall.bat" (
  echo trying vcvarsall.bat
  call "%VS_COMNTOOLS%..\..\VC\vcvarsall.bat" x86
  if defined LIB goto next
)

:not_found
echo !!! VC version %~1 (x32) not found !!!
exit /B 1
goto :EOF

:next
set VS
set LIB
if "%VS_VERSION%" NEQ "9" goto skip_ddk_check
if exist C:\WinDDK\7600.16385.1\lib\Crt\i386 set LIB=%LIB%;C:\WinDDK\7600.16385.1\lib\Crt\i386
:skip_ddk_check

set WIDE=1
set IA64=
set AMD64=
set DEBUG=
set NO_RELEASE_PDB=
