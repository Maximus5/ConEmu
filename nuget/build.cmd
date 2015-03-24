@echo off
cd /d "%~dp0"

setlocal
set PATH=%~d0\Chocolatey\bin;%PATH%

if exist "ConEmu.*.nupkg" del "ConEmu.*.nupkg"
if exist "chocolatey\ConEmu.*.nupkg" del "chocolatey\ConEmu.*.nupkg"

cd /d chocolatey
call choco pack
if errorlevel 1 (
  call cecho "Creation Chocolatey package was failed"
  exit /b 100
)
move "%~dp0chocolatey\ConEmu.*.nupkg" "%~dp0"
if errorlevel 1 (
  call cecho "Moving Chocolatey package was failed"
  exit /b 100
)
