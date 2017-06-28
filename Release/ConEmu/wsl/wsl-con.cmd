@echo off

rem This sample file demonstrates ability to use 256 colors
rem in Windows subsystem for Linus started in ConEmu tab.
rem TAGS: ConEmu, cygwin/msys connector, wslbridge.

ConEmuC -osverinfo > nul
if errorlevel 2560 (
  rem Windows 10 detected, OK
) else (
  call cecho "Windows 10 is required"
  exit /b 100
)

if not exist "%windir%\system32\bash.exe" (
  call cecho "Windows subsystem for linux was not installed!"
  call cecho "https://conemu.github.io/en/BashOnWindows.html#TLDR"
  exit /b 100
)

setlocal

if exist "%~dp0wslbridge-backend" goto wsl_ready

call cecho /yellow "wslbridge is not installed!"
echo https://cygwin.com/licensing.html
echo https://github.com/rprichard/wslbridge/blob/master/LICENSE.txt
echo https://github.com/Maximus5/cygwin-connector/blob/master/License.txt
set /P confirm="Type YES to accept licenses: "
if /I "%confirm%" == "YES" goto do_install
goto err

:do_install
set "wsltty_url=https://github.com/mintty/wsltty/releases/download/0.6.0/wsltty-0.6.0-install.exe"
set "connector_url=https://github.com/Maximus5/cygwin-connector/releases/download/v0.7.4/terminals.v0.7.4.7z"
set "szip_url=https://github.com/ConEmu/ConEmu.github.io/releases/download/7zip-9.20/7za.exe"
set "boot_url=https://ConEmu.github.io/wsl-boot.sh"
set "clr_url=https://ConEmu.github.io/256colors2.pl"

set "dl_tmp=%~dp0tmp"
if not exist "%dl_tmp%" md "%dl_tmp%"
cd /d "%~dp0"

call :do_clean > nul

call cecho /green "Downloading wsltty"
ConEmuC -download "%wsltty_url%" "%dl_tmp%\wsltty.7z" 2>nul
if errorlevel 1 goto err
call cecho /green "Downloading cygwin-connector"
ConEmuC -download "%connector_url%" "%dl_tmp%\terminals.7z" 2>nul
if errorlevel 1 goto err
call cecho /green "Downloading 7zip tool"
ConEmuC -download "%szip_url%" "%dl_tmp%\7za.exe" 2>nul
if errorlevel 1 goto err
call cecho /green "Downloading bash startup script"
ConEmuC -download "%boot_url%" wsl-boot.sh 2>nul
if errorlevel 1 goto err
call cecho /green "Downloading 256colors sample"
ConEmuC -download "%clr_url%" 256colors2.pl 2>nul
if errorlevel 1 goto err

call cecho /green "Extracting files"
"%dl_tmp%\7za.exe" e "%dl_tmp%\terminals.7z" conemu-cyg-32.exe 1>nul
if errorlevel 1 goto err
"%dl_tmp%\7za.exe" e "%dl_tmp%\wsltty.7z" wslbridge.exe cygwin1.dll wslbridge-backend 1>nul
if errorlevel 1 goto err

call :do_clean > nul
rd "%dl_tmp%"

if exist "%~dp0wslbridge-backend" goto wsl_ready
goto err

:do_clean
if exist "%dl_tmp%\wsltty.7z" del "%dl_tmp%\wsltty.7z"
if exist "%dl_tmp%\terminals.7z" del "%dl_tmp%\terminals.7z"
if exist "%dl_tmp%\7za.exe" del "%dl_tmp%\7za.exe"
goto :EOF

:wsl_ready
if "%~1" == "-run" goto do_run
ConEmuC -c "-new_console:d:%~dp0" "%~0" -new_console:n:h9999:C:"%LOCALAPPDATA%\lxss\bash.ico" -run
goto :EOF

:do_run
call SetEscChar
echo %ESC%[9999H
conemu-cyg-32.exe ./wslbridge.exe -t ./wsl-boot.sh
goto :EOF

:err
call cecho "wslbridge-backend was not installed properly"
timeout 10
goto :EOF
