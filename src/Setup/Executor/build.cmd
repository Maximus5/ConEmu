@echo off

setlocal
pushd "%~dp0"

call "%~dp0..\..\vc.build.set.x32.cmd" 1> nul
if errorlevel 1 goto err

set DEST_FILE=..\Setupper\Executor.exe
if exist Executor.exe del Executor.exe
if exist Executor.pdb del Executor.pdb
if exist obj del /q obj
if exist %DEST_FILE% del %DEST_FILE%

find "<UACExecutionLevel>RequireAdministrator</UACExecutionLevel>" Executor.vcxproj 1> nul
if errorlevel 1 (
  echo RequireAdministrator should be set in project, patching...
  powershell -noprofile -ExecutionPolicy RemoteSigned .\patch_project.ps1
)

find "<UACExecutionLevel>RequireAdministrator</UACExecutionLevel>" Executor.vcxproj 1> nul
if errorlevel 1 (
  echo Error: Executor.vcxproj should contain the key
  echo ^<UACExecutionLevel^>RequireAdministrator^</UACExecutionLevel^>
  echo Failed to build executor.exe
  exit /b 1
)

echo Preparing Executor binary
msbuild Executor.vcxproj /p:Configuration=Release,Platform=Win32,XPDeprecationWarning=false
if errorlevel 1 goto err

echo\
echo Copying bin\Executor.exe to %DEST_FILE%
copy Executor.exe %DEST_FILE%
if errorlevel 1 goto err

goto fin

:err
popd
endlocal
exit /b 1

:fin
popd
endlocal
