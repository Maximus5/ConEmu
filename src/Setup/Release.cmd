@echo off

set ConEmuHttp=https://conemu.github.io/

if "%~1"=="" goto noparm
cd /d "%~dp0"

set path=%~d0..\..\..\Tools\WiX\bin;%PATH%

set "BUILD_NO=%~1"
echo Preparing version %BUILD_NO%...
GenVersion.exe %1

set CAB_NAME=ConEmu.cab

if "%~2"=="no_msi" goto no_msi
if "%~2"=="setupper" goto setupper

echo Creating x86 installer
set MSI_PLAT=x86
set MSI_NAME=ConEmu.%1.%MSI_PLAT%.msi
call :create_msi

echo .
echo Creating x64 installer
set MSI_PLAT=x64
set MSI_NAME=ConEmu.%1.%MSI_PLAT%.msi
call :create_msi

rem call "%~dp0..\..\..\ConEmu-key\sign_any.bat" /d "ConEmu %~1 Installer" /du %ConEmuHttp% %~dp0%CAB_NAME% %~dp0ConEmu.%1.x86.msi %~dp0ConEmu.%1.x64.msi

:no_msi

echo .
echo Creating ConEmuSetup.%1.exe

call "%~dp0Executor\gccbuild.cmd" /nosign
if errorlevel 1 goto errs
if not exist "%~dp0Setupper\Executor.exe" goto errs
call "%~dp0..\..\..\ConEmu-key\sign_any.bat" /d "ConEmu %~1 Installer" /du %ConEmuHttp% "%~dp0Setupper\Executor.exe" "%~dp0%CAB_NAME%" "%~dp0ConEmu.%1.x86.msi" "%~dp0ConEmu.%1.x64.msi"
if errorlevel 1 goto errs

:setupper
call "%~dp0Setupper\gccbuild.cmd" /nosign
if errorlevel 1 goto errs
if not exist "%~dp0Setupper\Setupper.exe" goto errs

set "ConEmuInstaller=%~dp0ConEmuSetup.%1.exe"
move "%~dp0Setupper\Setupper.exe" "%ConEmuInstaller%"
if errorlevel 1 (
  timeout /t 5
  move "%~dp0Setupper\Setupper.exe" "%ConEmuInstaller%"
  if errorlevel 1 goto errs
)
call "%~dp0..\..\..\ConEmu-key\sign_any.bat" /d "ConEmu %~1 Installer" /du %ConEmuHttp% "%ConEmuInstaller%"
if errorlevel 1 goto errs

goto fin

:create_msi
echo Compiling...
candle.exe -nologo -dConfiguration=Release -dOutDir=bin\Release\ -dPlatform=%MSI_PLAT% -dProjectDir=%~dp0 -dProjectExt=.wixproj -dProjectFileName=ConEmu.wixproj -dProjectName=ConEmu -dProjectPath=%~dp0ConEmu.wixproj -dTargetDir=%~dp0bin\Release\ -dTargetExt=.msi -dTargetFileName=%MSI_NAME% -dTargetName=ConEmu -dTargetPath=%~dp0bin\Release\%MSI_NAME% -out obj\Release\Product.wixobj -arch %MSI_PLAT% Product.wxs
if errorlevel 1 goto err
echo Linking...
Light.exe -nologo -dcl:high  -cultures:null -out %~dp0bin\Release\%MSI_NAME% -pdbout %~dp0bin\Release\ConEmu.wixpdb -ext WixUIExtension obj\Release\Product.wixobj
if errorlevel 1 goto err
rem echo Signing...
rem call "%~dp0..\..\..\ConEmu-key\sign_any.bat" /d "ConEmu %~1 Installer" /du %ConEmuHttp% %~dp0bin\Release\%MSI_NAME%
rem if errorlevel 1 goto err
echo Succeeded.

move %~dp0bin\Release\%MSI_NAME% %MSI_NAME%
move %~dp0bin\Release\%CAB_NAME% %CAB_NAME%
rd /S /Q %~dp0bin
rd /S /Q %~dp0obj

goto :EOF


:noparm
echo Usage:    Release.cmd ^<BuildNo^> [no_msi]
echo Example:  Release.cmd 090121
pause
goto fin

:err
echo Error, while creating msi!
pause
goto fin

:errs
echo Error, while creating setupper!
pause
goto fin

:fin
