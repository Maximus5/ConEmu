@echo off

rem === Far menu ===
rem git2log.cmd
rem edit:...\_posts\.daily.md
rem edit:...\Release\ConEmu\WhatsNew-ConEmu.txt

setlocal

set git=%~d0\Utils\Lans\GIT\cmd\git.exe
if NOT EXIST "%git%" set git=git.exe

cd /d "%~dp0.."
call "%~dp0GetCurVer.cmd"

rem Update PortableApps and Chocolatey versions
powershell -noprofile -command "%~dp0UpdateDeployVersions.ps1" %CurVerBuild%

set daily_md=%~dp0..\..\ConEmu-GitHub-io\ConEmu.github.io\_posts\.daily.md
rem May be file was already created and prepared?
if exist "%daily_md%" (
  type "%daily_md%" | %windir%\system32\find "build: %CurVerBuild%"
  if errorlevel 1 goto yaml
  call cecho "### Changelog .daily.md was already created for build %CurVerBuild%"
  goto done
)
rem Create YAML header
:yaml
call :out_build > %daily_md%
rem Dump git commits into ".daily.md"
call :do_log >> %daily_md%
call cecho /green "%daily_md%"
rem Done
:done
goto :EOF

:out_build
echo ---
echo build: %CurVerBuild%
echo ---^



goto :EOF

:do_log

set commit_range=daily...origin/master

rem let gitlogpath be folder to store git output
if "%TEMP:~-1%" == "\" (set "gitlogpath=%TEMP:~0,-1%") else (set "gitlogpath=%TEMP%")
set git_out=%gitlogpath%\conemu_git_%ConEmuServerPID%_1.log
set git_err=%gitlogpath%\conemu_git_%ConEmuServerPID%_2.log
"%git%" -c color.status=false status --short --branch --porcelain 1>"%git_out%" 2>"%git_err%"
"%WINDIR%\system32\find.exe" "## preview" "%git_out%" > nul
if errorlevel 1 (
  set commit_range=daily...origin/master
) else (
  set commit_range=preview...v-preview
)
if exist "%git_out%" del "%git_out%"
if exist "%git_err%" del "%git_err%"

"%git%" log "--format=%%ci (%%cn)%%n* %%B" --reverse %commit_range%
goto :EOF
