@echo off

rem ****************************************************************
rem ** If you don't know about Chocolatey (apt-get style manager) **
rem ** read about it here: https://chocolatey.org/                **
rem ** If you are sure about installing it, execute the following **
rem ** one-line command:                                          **
rem ** powershell -NoProfile -ExecutionPolicy unrestricted        **
rem ** -Command "iex ((new-object net.webclient).DownloadString   **
rem ** ('https://chocolatey.org/install.ps1'))"                   **
rem ** && SET PATH=%PATH%;%systemdrive%\chocolatey\bin            **
rem ****************************************************************

setlocal
if "%ConEmuANSI%" == "ON" (
  set green=[1;32;40m
  set red=[1;31;40m
  set gray=[0m
  set white=[1;37;40m
) else (
  set green=
  set red=
  set gray=
  set white=
)
rem Do not print ending '**' because it's not so easy to pad it with spaces
echo %green%**************************************************************
echo %green%** %gray%If you%red% don't know about Chocolatey %gray%(apt-get style manager) %green%
echo %green%** %red%read about it%gray% here:%green% https://chocolatey.org/                %green%
if DEFINED ChocolateyInstall (
  if EXIST "%ChocolateyInstall%\choco.exe" (
    echo %green%** %gray%Chocolatey is already installed on your PC:                %green%
    echo %green%** %red%%ChocolateyInstall% %green%
    goto done
  )
)
echo %green%** %gray%If you are sure about installing it, execute the following %green%
echo %green%** %red%one-line command:                                          %green%
echo %green%** %white%powershell -NoProfile -ExecutionPolicy unrestricted        %green%
echo %green%**  %white%-Command "iex ((new-object net.webclient).                %green%
echo %green%**  %white%DownloadString('https://chocolatey.org/install.ps1'))"    %green%
echo %green%**  %white%^&^& SET PATH=%%PATH%%;%%ALLUSERSPROFILE%%\chocolatey\bin       %green%
:done
echo %green%**************************************************************%gray%
endlocal
