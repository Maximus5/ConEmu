@echo off

setlocal
pushd "%~dp0"

call "%~dp0..\..\vc.build.set.x32.cmd" 1> nul
if errorlevel 1 goto err

set DEST_FILE=..\Setupper\Executor.exe
if exist Executor.obj del Executor.obj
if exist Executor.res del Executor.res
if exist %DEST_FILE% del %DEST_FILE%

echo Preparing Executor binary
rc Executor.rc 1> nul
if errorlevel 1 goto err

cl /nologo /O1 /D_UNICODE /DUNICODE Executor.cpp /link /machine:x86 /release user32.lib shell32.lib Executor.res 1> nul
if errorlevel 1 goto err

move Executor.exe %DEST_FILE%
if errorlevel 1 goto err

goto fin

:err
popd
endlocal
exit /b 1

:fin
popd
endlocal
