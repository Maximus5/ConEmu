rem @echo off

setlocal
pushd "%~dp0"

call "%~dp0..\..\vc.build.set.x32.cmd" 1> nul
if errorlevel 1 goto err

if exist obj del /q obj

echo Preparing Setupper binary
msbuild Setupper.vcxproj /p:Configuration=Release,Platform=Win32,XPDeprecationWarning=false
if errorlevel 1 goto err

goto fin

:err
popd
endlocal
exit /b 1

:fin
popd
endlocal
