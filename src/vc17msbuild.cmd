@echo off

for /f "usebackq tokens=1* delims=: " %%i in (`"%~dp0Tools\vswhere.exe" -latest -requires Microsoft.Component.MSBuild`) do (
  if /i "%%i"=="installationPath" set VSInstallDir=%%j
)

call "%VSInstallDir%\VC\Auxiliary\Build\vcvars32.bat" > NUL

MSBuild.exe %*
