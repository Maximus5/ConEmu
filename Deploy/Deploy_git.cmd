@echo off

rem deploy.cmd <Version>   : Pack, Src, Dbg

set ex7zlist=-x!*.7z -x!*.aps -x!*.bak -x!*.bdf -x!*.cache -x!*.cod -x!.codeblocks -x!*.exp -x!*.gz -x!*.hab -x!*.ilk -x!*.ipch -x!*.lib -x!*.log -x!*.msi -x!*.ncb -x!*.new -x!*.obj -x!*.opt -x!*.pdb -x!*.plg -x!*.plog -x!*.psess -x!*.pvd -x!*.sln.cache -x!*.suo -x!*.tgz -x!*.ttf -x!*.user -x!*.vsp -x!*.zip -x!*684.* -x!*sdf -x!.gitignore -x!.svn -x!curl-7.21.4 -x!Debug -x!enc_temp_folder -x!enc_temp_folder -x!gcc -x!kl_parts_gcc.* -x!makefile_lt_vc -x!MouseWheelTilt.reg -x!rebar.bmp -x!Thumbs.db -x!time2.exe -x!toolbar -x!VTune -x!WhatsNew-ConEmu-Portable.txt -x!PortableApps_Installer -x!UnitTests.cmd -x!.svnignore -x!*.o -x!!*.txt -x!*.VC.db
set ex7zpack=-x!ConEmu.Gui.map -x!ConEmu64.Gui.map -x!ConEmu\*.map -x!ConEmu\ConEmu.Addons -x!ConEmu\Portable -x!UnitTests -x!Logs -x!*.VC.db

setlocal
set ConEmuHooks=OFF

set "PATH=%~d0..\..\Tools\Arch;%PATH%"

cd /d "%~dp0..\"

if "%~1"=="" goto noparm

set BUILD_NO=%~1
:oneparm

if "%~2"=="-" goto skipmingw
if "%~2"=="*" goto skipmingw
call "%~dp0MinGW_deploy.cmd" %BUILD_NO%
cd /d "%~dp0..\"
:skipmingw

if "%2"=="*" goto notfull
if "%2"=="-" goto min
if "%2"=="+" goto full
:notfull
rem pause
set ex7zlist=%ex7zlist% -x!*.bat -x!*todo*.txt -x!Reviews.txt
goto run
:full
echo Using full deploy
rem pause
set ConEmu_full_7z=..\ConEmu-Deploy\Debug\ConEmu.%BUILD_NO%.7z
if exist %ConEmu_full_7z% del %ConEmu_full_7z%
7z a -r %ConEmu_full_7z% * %ex7zlist% -x!Release.x64
if errorlevel 1 goto err7z
rem if exist T:\XChange\$Old (echo deploying to XChange & copy "..\..\ConEmu.%BUILD_NO%.7z" T:\XChange\$Old\)
goto fin

:min
echo Using source-only deploy
set ex7zlist=*.bat *.cmd *.dat *.reg  %ex7zlist% -x!*.htm -x!*.html -x!*.mht -x!*.pdf -x!*.png -x!*.psd
goto src

:run
echo Using medium deploy

rem copy Release.x64\plugins\ConEmu\conemu.x64.dll plugins\ConEmu\
rem if errorlevel 1 goto errcpy
rem copy plugins\ConEmu\conemu.dll Release.x64\plugins\ConEmu\
rem if errorlevel 1 goto errcpy

set ConEmu_Maximus5_dbg_7z=..\ConEmu-Deploy\Debug\ConEmu.Maximus5.%BUILD_NO%.dbg.7z
if exist %ConEmu_Maximus5_dbg_7z% del %ConEmu_Maximus5_dbg_7z%
7z a -r %ConEmu_Maximus5_dbg_7z% Release\*.map Release\*.pdb
if errorlevel 1 goto err7z

set ConEmuPack_7z=..\ConEmu-Deploy\Pack\ConEmuPack.%BUILD_NO%.7z
if exist %ConEmuPack_7z% del %ConEmuPack_7z%
cd Release
rem if exist ConEmu.map (
rem   if exist ConEmu1.map del ConEmu1.map
rem   ren ConEmu.map ConEmu1.map
rem )
7z a -r -mx=9 -ms=255f64m ..\%ConEmuPack_7z% ConEmu*.exe ConEmu*.map ConEmu\* plugins\* %ex7zlist% %ex7zpack%
if errorlevel 1 goto err7z
rem if exist ConEmu1.map ren ConEmu1.map ConEmu.map
cd ..

:src
set ConEmu_Maximus5_src_7z=..\ConEmu-Deploy\Debug\ConEmu.Maximus5.%BUILD_NO%.src.7z
if exist %ConEmu_Maximus5_src_7z% del %ConEmu_Maximus5_src_7z%
set ex7z_src=License.txt cmd_autorun.cmd About-ConEmu.txt FAQ-ConEmu.txt Settings-ConEmu.reg WhatsNew-ConEmu.txt -x!Release.x64
set ex7z_src=%ex7z_src% -x!src\dosbox -x!src\*.plog -x!src\ConEmu_Org -x!\MinGW\ -x!src\_Profiler -x!src\_ToDo -x!*.diff -x!*.cab -x!src\Setup\setup_exe
set ex7z_src=%ex7z_src% -x!src\ConEmuTh\Modules\mp3 -x!src\ConEmuTh\Modules\pe\ver_c0*.dll -x!src\ConEmuLn\x64 -x!src\ConEmuDW\Test\ConEmu
set ex7z_src=%ex7z_src% -x!src\ConEmuDW\Test.std -x!src\ConEmuDW\95.mht -x!src\ConEmuDW\viewtopic.htm -x!src\ConEmuBg\x64 -x!_VCBUILD  -x!*.VC.db-*
7z a -r %ConEmu_Maximus5_src_7z% src\* *.lng %ex7zlist% %ex7z_src%
7z a -r %ConEmu_Maximus5_src_7z% PortableApps\*
if errorlevel 1 goto err7z

if "%2"=="-" goto fin


rem echo .
rem echo .
rem echo Creating installer
rem call "%~dp0Deploy_msi.cmd" %BUILD_NO%


rem echo .
rem echo .
rem echo Creating PortableApps.com installer
rem call "%~dp0PortableApps_Deploy.cmd" %BUILD_NO%
rem if errorlevel 1 goto errpaf

goto fin

:noparm
echo Usage:    deploy.cmd ^<Version^> [+]
echo Example:  deploy.cmd 090121
echo .
set BUILD_NO=
set /P BUILD_NO="Deploy build number: "
set BUILD_NO
if NOT "%BUILD_NO%"=="" goto oneparm
goto fin

:errcpy
echo Can't copy x64 & x86 versions of plugin
pause
goto fin

:err7z
echo Can't create archive
pause
goto fin

:errmsi
echo Can't create msi-package or exe-package
pause
goto fin

:errpaf
echo Creating PortableApps.com package failed
pause
goto fin

:fin
