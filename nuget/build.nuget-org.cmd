@echo off
cd /d "%~dp0"

setlocal
set PATH=%~d0\Nuget\bin;%PATH%                           
rem ??? what would be the path to Nuget.exe cmdline tool?..

if exist "ConEmu.Core.*.nupkg" del "ConEmu.Core.*.nupkg"
if exist "ConEmu.Core\*.nupkg" del "ConEmu.Core\*.nupkg"

cd /d ConEmu.Core
call nuget pack -Verbosity detailed -NoDefaultExcludes
if errorlevel 1 (
  call cecho "Failed to create the package for Nuget.Org."
  exit /b 100
)
call cecho /green "A package for Nuget.Org were created."

move "%~dp0ConEmu.Core\ConEmu.Core.*.nupkg" "%~dp0"
if errorlevel 1 (
  call cecho "Moving ConEmu.Core package failed."
  exit /b 100
)
