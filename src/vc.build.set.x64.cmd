@echo on

call "%~dp0vc.build.set.any.cmd" %*

if NOT defined VS_COMNTOOLS (
  echo VS_COMNTOOLS NOT DEFINED!
  exit /B 100
  goto :EOF
)

set LIB=

:aux
if exist "%VS_COMNTOOLS%\VC\Auxiliary\Build\vcvars64.bat" (
  echo trying vcvars64.bat
  call "%VS_COMNTOOLS%\VC\Auxiliary\Build\vcvars64.bat"
  if defined LIB goto next
)

:x86_amd64
if exist "%VS_COMNTOOLS%..\..\VC\BIN\x86_amd64\vcvarsx86_amd64.bat" (
  echo trying x86_amd64\vcvarsx86_amd64.bat
  call "%VS_COMNTOOLS%..\..\VC\BIN\x86_amd64\vcvarsx86_amd64.bat"
  if defined LIB goto next
)

:bin
if exist "%VS_COMNTOOLS%..\..\VC\BIN\vcvarsx86_amd64.bat" (
  echo trying BIN\vcvarsx86_amd64.bat
  call "%VS_COMNTOOLS%..\..\VC\BIN\vcvarsx86_amd64.bat"
  if defined LIB goto next
)

:vcvarsall
if exist "%VS_COMNTOOLS%..\..\VC\vcvarsall.bat" (
  echo trying vcvarsall.bat
  call "%VS_COMNTOOLS%..\..\VC\vcvarsall.bat" x64
  if defined LIB goto next
)

:not_found
echo !!! VC version %~1 (x64) not found !!!
exit /B 1
goto :EOF

:next
set VS
set LIB
if "%VS_VERSION%" NEQ "9" goto skip_ddk_check
if exist C:\WinDDK\7600.16385.1\lib\Crt\amd64 set LIB=%LIB%;C:\WinDDK\7600.16385.1\lib\Crt\amd64
:skip_ddk_check

set WIDE=1
set IA64=
set AMD64=1
set DEBUG=
set NO_RELEASE_PDB=
