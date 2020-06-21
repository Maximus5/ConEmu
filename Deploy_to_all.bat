@echo off

setlocal

call SetEscChar.cmd

if exist "%~dp0Deploy\user_env.cmd" (
  call "%~dp0Deploy\user_env.cmd"
) else (
  call "%~dp0Deploy\user_env.default.cmd"
)
call "%GIT%" --version 1> nul 2> nul
if errorlevel 1 (
  call cecho "git not found, please configure user_env.cmd"
  exit /b 1
)

cd /d "%~dp0"

"%GIT%" log -5 --decorate=short --oneline 
"%GIT%" status

rem Returns env.var 'CurVerBuild' from version.h
call :getcurbuild

echo .
echo .
set /P do_REL=%ESC%[1;31;40mCreate "%CurVerBuild% release files" commit [Y/n/q]: %ESC%[m
if /I "%do_REL%" == "q" exit /B 1
set /P do_TAG=%ESC%[1;31;40mCreate "v%CurVerBuild:~0,2%.%CurVerBuild:~2,2%.%CurVerBuild:~4%" tag [Y/n/q]: %ESC%[m
if /I "%do_TAG%" == "q" exit /B 1
set /P do_MST=%ESC%[1;31;40mMerge Daily branch into Master [Y/n/q]: %ESC%[m
if /I "%do_MST%" == "q" exit /B 1
rem set /P do_MPV=%ESC%[1;31;40mMerge Daily branch into Preview [y/N/q]: %ESC%[m
rem if /I "%do_MPV%" == "q" exit /B 1
set /P do_GHB=%ESC%[1;31;40mDeploy Branches to github^&sourgeforge [Y/n/q]: %ESC%[m
if /I "%do_GHB%" == "q" exit /B 1
set /P do_GRL=%ESC%[1;31;40mDeploy Release to github [Y/n/q]: %ESC%[m
if /I "%do_GRL%" == "q" exit /B 1
set /P do_CKL=%ESC%[1;31;40mDeploy Release to chocolatey [Y/n/q]: %ESC%[m
if /I "%do_CKL%" == "q" exit /B 1
set /P do_NGT=%ESC%[1;31;40mDeploy Release to nuget [Y/n/q]: %ESC%[m
if /I "%do_NGT%" == "q" exit /B 1
set /P do_FSH=%ESC%[1;31;40mDeploy Release to fosshub [Y/n/q]: %ESC%[m
if /I "%do_FSH%" == "q" exit /B 1
set /P do_VER=%ESC%[1;31;40mBinaries [(A)lpha/(p)review/(pa)preview+alpha/(s)table/n/q]: %ESC%[m
if /I "%do_VER%" == "q" exit /B 1

echo .%ESC%[1;32;40m
pause
echo %ESC%[m.




if /I "%do_CKL%" == "N" goto skip_CHOCO1
pushd "%~dp0nuget"
call cecho /yellow "Creating Chocolatey package"
call "%~dp0nuget\build.cmd"
if errorlevel 1 goto err
popd
:skip_CHOCO1

if /I "%do_NGT%" == "N" goto skip_NUGET1
pushd "%~dp0nuget"
call cecho /yellow "Creating NuGet package"
call "%~dp0nuget\build.nuget-org.cmd"
if errorlevel 1 goto err
popd
:skip_NUGET1



if /I "%do_FSH%" == "N" goto skip_FOSS
pushd "%CONEMU_DEPLOY%"
call "%CONEMU_DEPLOY%foss.cmd" %CurVerBuild%
if errorlevel 1 goto err
popd
:skip_FOSS



if /I "%do_REL%" == "N" goto skip_REL
"%GIT%" commit -am "%CurVerBuild% release files"
if errorlevel 1 goto err
:skip_REL

if /I "%do_TAG%" == "N" goto skip_TAG
"%GIT%" tag v%CurVerBuild:~0,2%.%CurVerBuild:~2,2%.%CurVerBuild:~4%
if errorlevel 1 goto err
rem Move "v-preview" or "v-alpha" tag to the current location
if /I "%do_VER%" == "N" goto skip_TAG
"%GIT%" tag -f v-release
if /I "%do_VER%" == "S" (
  "%GIT%" tag -f v-stable
) else if /I "%do_VER%" == "P" (
  "%GIT%" tag -f v-preview
) else if /I "%do_VER%" == "PA" (
  "%GIT%" tag -f v-preview
) else (
  "%GIT%" tag -f v-alpha
)
rem Stable build is not available yet, so there is no v-stable tag
:skip_TAG

rem if /I NOT "%do_MPV%" == "Y" goto skip_MPV
rem "%GIT%" checkout preview
rem if errorlevel 1 goto err
rem "%GIT%" merge daily
rem if errorlevel 1 goto err
:skip_MPV

if /I "%do_MST%" == "N" goto skip_MST
"%GIT%" checkout master
if errorlevel 1 goto err
"%GIT%" merge daily
if errorlevel 1 goto err
"%GIT%" checkout daily
:skip_MST

"%GIT%" log -5 --decorate=short --oneline 



rem \Utils\Lans\GIT\bin\git.exe branch
rem call cecho "Press enter to deploy `%CurVerBuild% Alpha` to github/sourgeforge"
rem pause

if /I "%do_GHB%" == "N" goto skip_GHB
"%GIT%" push origin master daily
if errorlevel 1 goto err
"%GIT%" push -f origin --tags
if errorlevel 1 goto err
if /I "%do_GRL%" == "N" (
  echo Release creating skipped!
) else (
  echo Creating release: v%CurVerBuild:~0,2%.%CurVerBuild:~2,2%.%CurVerBuild:~4% %CurVerBuild%
  call "%UPLOADERS%ConEmuGithubRelease.cmd" v%CurVerBuild:~0,2%.%CurVerBuild:~2,2%.%CurVerBuild:~4% %CurVerBuild%
  if errorlevel 1 goto err
)
"%GIT%" push forge
:skip_GHB


if /I "%do_CKL%" == "N" goto skip_CHOCO2
pushd "%~dp0nuget"
call cecho /yellow "Uploading Chocolatey package"
call "%~dp0nuget\upload.cmd"
if errorlevel 1 goto err
popd
:skip_CHOCO2

if /I "%do_NGT%" == "N" goto skip_NUGET2
pushd "%~dp0nuget"
call cecho /yellow "Uploading NuGet package"
call "%~dp0nuget\upload.nuget-org.cmd"
if errorlevel 1 goto err
popd
:skip_NUGET2

if /I "%do_VER%" == "N" goto skip_AUTO
pushd "%CONEMU_DEPLOY%"
if /I "%do_VER%" == "S" (
call "%CONEMU_DEPLOY%upld.cmd" %CurVerBuild% "Stable"
) else if /I "%do_VER%" == "P" (
call "%CONEMU_DEPLOY%upld.cmd" %CurVerBuild% "Preview"
) else if /I "%do_VER%" == "PA" (
call "%CONEMU_DEPLOY%upld.cmd" %CurVerBuild% "Preview+Alpha"
) else (
call "%CONEMU_DEPLOY%upld.cmd" %CurVerBuild% "Alpha"
)
if errorlevel 1 goto err
popd
:skip_AUTO

rem call cecho "Press enter to deploy `%CurVerBuild% Alpha` to googlecode/ConEmu-alpha"
rem pause

rem if "%~1"=="" pause
goto :EOF


:getcurbuild
call "%~dp0Deploy\GetCurVer.cmd"
if "%CurVerBuild%" == "" (
call cecho "Can't find valid version in src\ConEmu\version.h"
exit /B 100
)
goto :EOF

:err
pause
