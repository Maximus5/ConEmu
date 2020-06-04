rem @echo off

setlocal
pushd "%~dp0"

call "%~dp0..\..\vc.build.set.x32.cmd" 1> nul
if errorlevel 1 goto err

if exist Setupper.obj del Setupper.obj
if exist Setupper.res del Setupper.res

echo Preparing Setupper binary
rc Setupper.rc 1> nul
if errorlevel 1 goto err

cl /nologo /O1 /D_UNICODE /DUNICODE Setupper.cpp /link /machine:x86 /release advapi32.lib Ole32.lib user32.lib shell32.lib Setupper.res 1> nul
if errorlevel 1 goto err

goto fin

:err
popd
endlocal
exit /b 1

:fin
popd
endlocal
