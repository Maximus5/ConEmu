@echo off
setlocal

rem set ConEmuRelyDT=2013-12-01 00:00
echo Today: %DATE%

if "%~1"=="" goto usage
if "%~1"=="/?" goto usage

rem Parsing version
set VR=%~1

set MVV_1=%VR:~0,2%
if "%MVV_1%"=="" goto err_parm
if "%MVV_1:~0,1%"=="0" goto err_parm
if "%MVV_1:~1,1%"=="" goto err_parm
set chk=
set /A chk=%MVV_1%+1-1
if errorlevel 1 goto err_parm
if NOT "%chk%"=="%MVV_1%" goto err_parm
if /I %MVV_1% GEQ 99 goto err_parm

set MVV_2=%VR:~2,2%
if "%MVV_2:~0,1%"=="0" set MVV_2=%MVV_2:~1,1%
if "%MVV_2%"=="" goto err_parm
if "%MVV_2%"=="0" goto err_parm
set chk=
set /A chk=%MVV_2%+1-1
if errorlevel 1 goto err_parm
if NOT "%chk%"=="%MVV_2%" goto err_parm
if /I %MVV_2% GTR 12 goto err_parm

set MVV_3=%VR:~4,2%
if "%MVV_3:~0,1%"=="0" set MVV_3=%MVV_3:~1,1%
if "%MVV_3%"=="" goto err_parm
if "%MVV_3%"=="0" goto err_parm
set chk=
set /A chk=%MVV_3%+1-1
if errorlevel 1 goto err_parm
if NOT "%chk%"=="%MVV_3%" goto err_parm
if /I %MVV_3% GTR 31 goto err_parm

set MVV_4=0
set MVV_4a=%VR:~6,1%
set MVV_git_def=#undef MVV_git

if /I "%~2" == "ALPHA" (
  call :gen_stage ALPHA
  goto check_4
)
if /I "%~2" == "PREVIEW" (
  call :gen_stage PREVIEW
  goto check_4
)
if /I "%~2" == "STABLE" (
  call :gen_stage STABLE
  goto check_4
)

if NOT "%~2"=="" goto git_ci

:check_4
if "%MVV_4a%"=="" goto run
rem translate letter in build no into 4-d number
set Found=N
FOR /D %%L IN ("a","b","c","d","e","f","g","h","i","j","k","l","m","n","o","p","q","r","s","t","u","v","w","x","y","z","--") DO call :chk_4 %%L
if /I %MVV_4% GTR 26 goto err_parm
goto run
:chk_4
if %Found%==Y goto :EOF
set /A MVV_4=%MVV_4%+1
if "%~1"=="%MVV_4a%" set Found=Y
goto :EOF
:inc_4
goto :EOF

:git_ci
set MVV_git_def=#define MVV_git
set /A MVV_4=%~2
if errorlevel 1 (
  echo Numeric minor-build-no was not specified for git commit [arg #2]
  goto err_parm
) else (
  echo Minor build no for git commit: '%MVV_4%'
)
set MVV_4a=%~3
if "%MVV_4a%"=="" (
  echo git commit was not specified [arg #3]
  goto err_parm
) else (
  echo git commit '%MVV_4a%'
)
goto run

:run

rem echo '%MVV_1%' '%MVV_2%' '%MVV_3%' '%MVV_4%' '%MVV_4a%'
rem goto :EOF

rem Creating version.ini
set verh="%~dp0version.h"
call :printver>%verh%
exit /B 0
goto :EOF

:printver
@echo // %VR%
@echo #define MVV_1 %MVV_1%
@echo #define MVV_2 %MVV_2%
@echo #define MVV_3 %MVV_3%
@echo #define MVV_4 %MVV_4%
@echo #define MVV_4a "%MVV_4a%"
@echo %MVV_git_def%
@echo.
@echo #include "version_stage.h"
@echo #include "version_macro.h"
@goto :EOF

:gen_stage
set verh="%~dp0version_stage.h"
call :printstage %1 > %verh%
goto :EOF

:printstage
@echo #pragma once
@echo.
@echo #define CEVS_STABLE   0
@echo #define CEVS_PREVIEW  1
@echo #define CEVS_ALPHA    2
@echo.
@echo #define ConEmuVersionStage CEVS_%1
@goto :EOF

:usage
echo Usage:   "%~nx0" ^<BuildNo^> [minor_for_git git_commit]
echo Example: "%~nx0" 131113c
echo Example: "%~nx0" 140404  21 4069b5f
goto ex99
:err_parm
echo Bad version was specified: '%*'
:ex99
exit /B 99
goto :EOF
