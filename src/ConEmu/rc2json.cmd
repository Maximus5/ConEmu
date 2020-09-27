@echo off
setlocal

rem Update ConEmu.l10n using current language and dialog resources

powershell -NoProfile -ExecutionPolicy RemoteSigned -command "%~dp0..\..\Deploy\rc2json.ps1"
if errorlevel 1 exit /B 1

call "%~dp0..\..\Deploy\l10n_write_pseudo.cmd"
call "%~dp0..\..\Deploy\l10n_write_yaml.cmd"

cd /d "%~dp0"
set LANG=en_US.UTF-8
call cecho /blue "git diff ConEmu.l10n [begin]"
call git diff ../../Release/ConEmu/ConEmu.l10n
call cecho /blue "git diff ConEmu.l10n [end]"
