@echo off

rem Usage: git2log [<commit-sha>] [-force] [-skip_upd]
rem   If <commit-sha> is specified, script will process range: HEAD...<commit>
rem   -force    - force overwrite .daily.md
rem   -skip_upd - Do not call UpdateDeployVersions.ps1

rem === Far menu ===
rem git2log.cmd
rem edit:...\_posts\.daily.md
rem edit:...\Release\ConEmu\WhatsNew-ConEmu.txt

setlocal

set FORCE_MD=NO
set SKIP_UPD=NO
set COMMIT_SHA=

:parm_loop
if "%~1" == "" goto parm_done
if /I "%~1" == "-force" (
  set FORCE_MD=YES
) else if /I "%~1" == "-skip_upd" (
  set SKIP_UPD=YES
) else (
  set "COMMIT_SHA=%~1"
)
shift /1
goto parm_loop
:parm_done

echo COMMIT_SHA=`%COMMIT_SHA%`

set git=%~d0\gitsdk\cmd\git.exe
rem set git=%~d0\Utils\Lans\GIT\cmd\git.exe
if NOT EXIST "%git%" set git=git.exe

cd /d "%~dp0.."
call "%~dp0GetCurVer.cmd"

rem Update PortableApps and Chocolatey versions
if "%SKIP_UPD%" == "YES" goto skip_upd
powershell -noprofile -command "%~dp0UpdateDeployVersions.ps1" %CurVerBuild%
:skip_upd

set daily_md=%~dp0..\..\ConEmu-GitHub-io\ConEmu.github.io\_posts\.daily.md
rem May be file was already created and prepared?
if exist "%daily_md%" (
  type "%daily_md%" | %windir%\system32\find "build: %CurVerBuild%"
  if errorlevel 1 goto yaml
  if "%FORCE_MD%" == "YES" goto yaml
  call cecho "### Changelog .daily.md was already created for build %CurVerBuild%"
  goto done
)
rem Create YAML header
:yaml
call cecho /yellow "Creating YAML header for %CurVerBuild% %CurVerStage%"
call :out_build > %daily_md%
rem Dump git commits into ".daily.md"
call :do_log %COMMIT_SHA% >> %daily_md%
call cecho /green "%daily_md%"
rem Done
:done
goto :EOF

:out_build
echo ---
echo build: %CurVerBuild%
if /I "%CurVerStage%" == "ALPHA"   echo stage: alpha
if /I "%CurVerStage%" == "PREVIEW" echo stage: preview
if /I "%CurVerStage%" == "STABLE"  echo stage: stable
rem add new line to the end of .daily.md
echo ---^



goto :EOF

:do_log

rem let gitlogpath be folder to store git output
if "%TEMP:~-1%" == "\" (set "gitlogpath=%TEMP:~0,-1%") else (set "gitlogpath=%TEMP%")
set git_out=%gitlogpath%\conemu_git_%ConEmuServerPID%_1.log
set git_err=%gitlogpath%\conemu_git_%ConEmuServerPID%_2.log

if "%~1" NEQ "" (
  call "%git%" log "%~1" -1 --oneline 1>"%git_out%" 2>"%git_err%"
  if errorlevel 1 (
    call cecho "Invalid commit-sha specified: %~1"
    exit /B 1
  )
  set commit_range=master...%~1
  goto commit_sha
)

"%git%" -c color.status=false status --short --branch --porcelain 1>"%git_out%" 2>"%git_err%"
"%WINDIR%\system32\find.exe" "## preview" "%git_out%" > nul
if errorlevel 1 (
  set commit_range=daily...origin/master
) else (
  set commit_range=preview...v-preview
)
:commit_sha
if exist "%git_out%" del "%git_out%" > nul
if exist "%git_err%" del "%git_err%" > nul

rem echo "%git%" log "--format=%%ci (%%cn)%%n* %%B" --reverse %commit_range% --invert-grep --grep="^Internal\. "
"%git%" log "--format=%%ci (%%cn)%%n* %%B" --reverse %commit_range% --invert-grep --grep="^Internal\. "
goto :EOF
