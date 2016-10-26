@echo off

cd /d "%~dp0"

if "%~1" == "" (
  cmd /c ""%~0" build_x86 -new_console"
  rem TODO: somehow wait for created consoles
  exit /b 1
)


if errorlevel 1 goto err

if "%~1" == "build_x86" (
  cmd /c ""%~0" build_x64 -new_console:sVb"
  call "C:\Program Files (x86)\Microsoft Visual Studio 14.0\VC\vcvarsall.bat" x64
  msbuild /m:3 /p:Configuration=Release,Platform=Win32 CE14.sln
  goto err
)

if "%~1" == "build_x64" (
  call "C:\Program Files (x86)\Microsoft Visual Studio 14.0\VC\vcvarsall.bat" x64
  msbuild /m:3 /p:Configuration=Release,Platform=x64 CE14.sln
  goto err
)

goto :EOF
:err
pause
