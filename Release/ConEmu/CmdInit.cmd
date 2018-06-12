@rem !!! Do not change this file in-place, change its copy instead !!!
@rem !!!  Otherwise you will lose your settings after next update  !!!

@echo off

rem Simple "ver" prints empty line before Windows version
rem Use this construction to print just a version info
cmd /d /c ver | "%windir%\system32\find.exe" "Windows"

rem Now we form the command prompt

rem This will start prompt with `User@PC `
if /I "%ConEmuPromptNames%" == "NO" (
  set ConEmuPrompt0=
) else (
  set ConEmuPrompt0=$E[m$E[32m$E]9;8;"USERNAME"$E\@$E]9;8;"COMPUTERNAME"$E\$S
)

rem Followed by colored `Path`
set ConEmuPrompt1=%ConEmuPrompt0%$E[92m$P$E[90m
if NOT "%PROCESSOR_ARCHITECTURE%" == "AMD64" (
  if "%PROCESSOR_ARCHITEW6432%" == "AMD64" if "%PROCESSOR_ARCHITECTURE%" == "x86" (
    rem Use another text color if cmd was run from SysWow64
    set ConEmuPrompt1=%ConEmuPrompt0%$E[93m$P$E[90m
  )
)

rem Carriage return and `$` or `#`
if /I "%ConEmuPromptNL%" == "NO" (set ConEmuPrompt2=) else (set ConEmuPrompt2=$_)
rem Spare `$E[90m` was specially added because of GitShowBranch.cmd
if "%ConEmuIsAdmin%" == "ADMIN" (
  set ConEmuPrompt2=%ConEmuPrompt2%$E[90m#
) else (
  set ConEmuPrompt2=%ConEmuPrompt2%$E[90m$$
)

rem Finally reset color and add notify ConEmu about prompt input start coords
set ConEmuPrompt3=$E[m$S$E]9;12$E\

if /I "%~1" == "/git" goto git
if /I "%~1" == "-git" goto git
goto no_git

:git
call "%~dp0GitShowBranch.cmd" /i
goto :EOF

:no_git
rem Set new prompt
PROMPT %ConEmuPrompt1%%ConEmuPrompt2%%ConEmuPrompt3%

:clean
set ConEmuPrompt0=
set ConEmuPrompt1=
set ConEmuPrompt2=
set ConEmuPrompt3=
