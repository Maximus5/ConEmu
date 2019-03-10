@echo off
cd /d "%~dp0"

setlocal
set "PATH=%~dp0..\..\Tools\Chocolatey\bin;%PATH%"

call "%~dp0..\Deploy\GetCurVer.cmd"
powershell -noprofile -command "%~dp0..\Deploy\UpdatePackageVersions.ps1" %CurVerBuild%
if errorlevel 1 (
  call cecho "Failed to update Chocolatey and Nuget packages"
  exit /b 100
)

call git add chocolatey/ConEmu.nuspec chocolatey/tools/chocolateyInstall.ps1 chocolatey/tools/chocolateyUninstall.ps1 ConEmu.Core/ConEmu.Core.nuspec
call git commit -m "%CurVerBuild% Chocolatey and Nuget"

if exist "ConEmu.*.nupkg" del "ConEmu.*.nupkg"
if exist "chocolatey\ConEmu.*.nupkg" del "chocolatey\ConEmu.*.nupkg"

cd /d chocolatey
call choco pack -y
if errorlevel 1 (
  call cecho "Creation Chocolatey package was failed"
  exit /b 100
)
call cecho /green "Chocolatey package was created"

move "%~dp0chocolatey\ConEmu.*.nupkg" "%~dp0"
if errorlevel 1 (
  call cecho "Moving Chocolatey package was failed"
  exit /b 100
)
