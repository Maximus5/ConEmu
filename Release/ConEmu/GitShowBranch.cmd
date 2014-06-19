@echo off

rem *** Usage ***
rem call 'GitShowBranch /i' for prompt initialization
rem you may change ConEmuGitPath variable, if git.exe/git.cmd is not in your %PATH%

rem predefined dir where git binaries are stored
if NOT DEFINED ConEmuGitPath set ConEmuGitPath="%~d0\Utils\Lans\GIT\bin\git.exe"
if NOT exist %ConEmuGitPath% (
  set ConEmuGitPath=git
)

if /I "%~1" == "/i" (
  call %ConEmuGitPath% --version
  if errorlevel 1 (
    call cecho "GIT not found, change your ConEmuGitPath environment variable"
    goto :EOF
  )
  prompt $P$E]9;7;"cmd /c%~nx0"$e\$E]9;8;"gitbranch"$e\$g
  goto :EOF
) else if /I "%~1" == "/u" (
  prompt $P$G
  goto :EOF
)

if /I "%~1" == "/?" goto help
if /I "%~1" == "-?" goto help
if /I "%~1" == "-h" goto help
if /I "%~1" == "--help" goto help


goto run


:help
setlocal
call "%~dp0SetEscChar.cmd"
if "%ConEmuANSI%"=="ON" (
set white=%ESC%[1;37;40m
set red=%ESC%[1;31;40m
set normal=%ESC%[0m
) else (
set white=
set red=
set normal=
)
echo %white%%~nx0 description%normal%
echo You may see your current git branch in %white%cmd prompt%normal%
echo Just run `%red%%~nx0 /i%normal%` to setup it in the current cmd instance
echo And you will see smth like `%white%T:\ConEmu [daily +0 ~7 -0]^>%normal%`
echo If you see double `%red%^>^>%normal%` unset your `%white%FARHOME%normal%` env.variable
echo You may use it in %white%Far Manager%normal%,
echo set your Far %white%prompt%normal% to `%red%$p%%gitbranch%%%normal%` and call `%red%%~nx0%normal%`
echo after each command which can change your working directory state
echo Example: "%~dp0Addons\git.cmd"
goto :EOF



:inc_add
set /A gitbranch_add=%gitbranch_add%+1
goto :EOF
:inc_chg
set /A gitbranch_chg=%gitbranch_chg%+1
goto :EOF
:inc_del
set /A gitbranch_del=%gitbranch_del%+1
goto :EOF

:eval
rem Calculate changes count
if "%gitbranch_ln%" == "##" (
  rem save first line, this must be branch
) else if "%gitbranch_ln:~0,2%" == "??" (
  call :inc_add
) else if "%gitbranch_ln:~0,1%" == "A" (
  call :inc_add
) else if "%gitbranch_ln:~0,1%" == "D" (
  call :inc_del
) else if "%gitbranch_ln:~0,1%" == "M" (
  call :inc_chg
)
goto :EOF

:calc
rem Ensure that gitbranch_ln has no line returns
set gitbranch_ln=%~1
call :eval
goto :EOF

:drop_ext
for /F "delims=." %%l in ("%gitbranch%") do set gitbranch=%%l
goto :EOF

:run


rem let gitlogpath be folder to store git output
if "%TEMP:~-1%" == "\" (set gitlogpath=%TEMP:~0,-1%) else (set gitlogpath=%TEMP%)
set git_out=%gitlogpath%\conemu_git_1.log
set git_err=%gitlogpath%\conemu_git_2.log

call %ConEmuGitPath% -c color.status=false status --short --branch --porcelain 1>"%git_out%" 2>"%git_err%"
if errorlevel 1 (
del "%git_out%">nul
del "%git_err%">nul
set gitbranch=
goto prepare
)

set gitbranch_add=0
set gitbranch_chg=0
set gitbranch_del=0

rem Set "gitbranch" to full contents of %git_out% file
set /P gitbranch=<"%git_out%"
rem But we need only first line of it
set gitbranch=%gitbranch%
rem To ensure that %git_out% does not contain brackets
pushd %gitlogpath%
for /F %%l in (conemu_git_1.log) do call :calc "%%l"
popd
rem echo done ':calc', br='%gitbranch%', ad='%gitbranch_add%', ch='%gitbranch_chg%', dl='%gitbranch_del%'

del "%git_out%">nul
del "%git_err%">nul

if NOT "%gitbranch:~0,3%" == "## " (
rem call "%~dp0cecho" "Not `## `?"
set gitbranch=
goto prepare
)

rem "set" does not like brackets
if "%gitbranch%" == "## HEAD (no branch)" (
set gitbranch=## HEAD.no_branch
) else (
rem Drop external branch name (alpha...origin/alpha)
call :drop_ext
)

rem Are there changes? Or we need to display branch name only?
if "%gitbranch_add% %gitbranch_chg% %gitbranch_del%" == "0 0 0" (
  set gitbranch= [%gitbranch:~3%]
) else (
  set gitbranch= [%gitbranch:~3% +%gitbranch_add% ~%gitbranch_chg% -%gitbranch_del%]
)
rem echo "%gitbranch%"

:prepare
if NOT "%FARHOME%" == "" set gitbranch=%gitbranch%^>

:export
rem Export to parent Far Manager or cmd.exe processes
"%ConEmuBaseDir%\ConEmuC.exe" /export=CON gitbranch
