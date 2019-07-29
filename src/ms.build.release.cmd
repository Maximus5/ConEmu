@echo off

setlocal

cd /d "%~dp0"

set "x32_build_flag=%~dp0.x32_build_flag"
set "x64_build_flag=%~dp0.x64_build_flag"
set build_flag=
set build_multi=/m
rem set build_multi=/m

set target=clean,build
rem set target=build

if errorlevel 1 goto err

if "%~1" == "" goto do_both
if "%~1" == "build_x32" goto build_x32
if "%~1" == "build_x64" goto build_x64
echo Unsupported switch: "%~1"
goto err



:do_both
set VSInstallDir=
for /f "usebackq tokens=1* delims=: " %%i in (`tools\vswhere -latest -requires Microsoft.Component.MSBuild`) do (
  if /i "%%i"=="installationPath" set VSInstallDir=%%j
)
if not defined VSInstallDir (
  if exist "C:\Program Files (x86)\Microsoft Visual Studio\2017\BuildTools" (
    set "VSInstallDir=C:\Program Files (x86)\Microsoft Visual Studio\2017\BuildTools"
  )
)
if not defined VSInstallDir (
  echo Visual Studio 2017 not found
  exit /b 1
)
echo %date% %time%
if exist "%x32_build_flag%.success" del "%x32_build_flag%.success" > nul
if exist "%x64_build_flag%.success" del "%x64_build_flag%.success" > nul
echo Waiting for x32 build > "%x32_build_flag%"
echo Waiting for x64 build > "%x64_build_flag%"
set errorlevel=
ConEmuC.exe -IsConEmu
if "%errorlevel%" == "1" (
  cmd /d /c ""%~0" build_x64" -new_console:s40Vbnt:"64-bit"
  cmd /d /c ""%~0" build_x32" -new_console:s68Vbnt:"32-bit"
) else (
  start cmd /d /c ""%~0" build_x64"
  start cmd /d /c ""%~0" build_x32"
)
Echo Waiting for build process
:do_wait
timeout /t 2 > nul
if exist "%x32_build_flag%" goto do_wait
if exist "%x64_build_flag%" goto do_wait
:all_done
echo %date% %time%
set SUCCESS=YES
if not exist "%x32_build_flag%.success" (
  set SUCCESS=NO
  echo 32-bit build failed
) else (
  del "%x32_build_flag%.success" > nul
)
if not exist "%x64_build_flag%.success" (
  set SUCCESS=NO
  echo 64-bit build failed
) else (
  del "%x64_build_flag%.success" > nul
)
if %SUCCESS% == YES (
  echo Build process succeeded
  exit /b 0
)
exit /b 1




:build_x32
set "build_flag=%x32_build_flag%"
call "%VSInstallDir%\VC\Auxiliary\Build\vcvars32.bat"
cd /d "%~dp0"
msbuild CE.sln %build_multi% /p:Configuration=Release,Platform=Win32 /t:%target%
if errorlevel 1 goto err
del "%build_flag%" > nul
echo Success > "%build_flag%.success"
goto done


:build_x64
set "build_flag=%x64_build_flag%"
call "%VSInstallDir%\VC\Auxiliary\Build\vcvars64.bat"
cd /d "%~dp0"
msbuild CE.sln %build_multi% /p:Configuration=Release,Platform=x64 /t:%target%
if errorlevel 1 goto err
del "%build_flag%" > nul
echo Success > "%build_flag%.success"
goto done


:err
if defined build_flag del "%build_flag%" > nul
pause
:done
endlocal
