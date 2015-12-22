@echo off
cd /d "%~dp0"

setlocal
set PATH=%~d0\Nuget\bin;%PATH%                           
rem ??? what would be the path to Nuget.exe cmdline tool?..

if exist "ConEmu.Console.*.nupkg" del "ConEmu.Console.*.nupkg"
if exist "ConEmu.Console\*.nupkg" del "ConEmu.Console\*.nupkg"

cd /d ConEmu.Console
call nuget pack -Verbosity detailed -NoDefaultExcludes
if errorlevel 1 (
  call cecho "Failed to create the package for Nuget.Org."
  exit /b 100
)
call cecho /green "A package for Nuget.Org were created."

move "%~dp0ConEmu.Console\ConEmu.Console.*.nupkg" "%~dp0"
if errorlevel 1 (
  call cecho "Moving ConEmu.Console package failed."
  exit /b 100
)
