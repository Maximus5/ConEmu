@echo off

setlocal

cd /d "%~dp0"

set x32_build_flag="%~dp0.x32_build_flag"
set x64_build_flag="%~dp0.x64_build_flag"

set target=clean,build
rem set target=build

if errorlevel 1 goto err

if "%~1" == "" goto do_both
if "%~1" == "build_x86" goto build_x86
if "%~1" == "build_x64" goto build_x64
echo Unsupported switch: "%~1"
goto err



:do_both
echo Waiting for x32 build > %x32_build_flag%
echo Waiting for x64 build > %x64_build_flag%
cmd /c ""%~0" build_x86 -new_console:s30Vb"
cmd /c ""%~0" build_x64 -new_console:s50Vb"
Echo Waiting for build process
:do_wait
timeout /t 5 > nul
if exist %x32_build_flag% goto do_wait
if exist %x64_build_flag% goto do_wait
:all_done
exit /b 1





:build_x86
call "C:\Program Files (x86)\Microsoft Visual Studio\2017\Professional\VC\Auxiliary\Build\vcvars32.bat"
msbuild CE17.sln /m:4 /p:Configuration=Release,Platform=Win32 /t:%target%
if errorlevel 1 goto err
del %x32_build_flag%
goto :EOF


:build_x64
call "C:\Program Files (x86)\Microsoft Visual Studio\2017\Professional\VC\Auxiliary\Build\vcvars64.bat"
msbuild CE17.sln /m:4 /p:Configuration=Release,Platform=x64 /t:%target%
if errorlevel 1 goto err
del %x64_build_flag%
goto :EOF


:err
endlocal
pause
