@echo off

rem Update ConEmu.l10n using current language and dialog resources

powershell -noprofile -command "%~dp0..\..\Deploy\rc2json.ps1"
if errorlevel 1 exit /B 1

cd /d "%~dp0"
call cecho /blue "git diff ConEmu.l10n [begin]"
call git diff ../../Release/ConEmu/ConEmu.l10n
call cecho /blue "git diff ConEmu.l10n [end]"
