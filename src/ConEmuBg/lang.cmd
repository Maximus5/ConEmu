@echo off
cd /d "%~dp0"
if not exist ..\..\Release\plugins\ConEmu\Background md ..\..\Release\plugins\ConEmu\Background
if exist ..\..\Release\plugins\ConEmu\Background\ConEmuBg_*.lng del ..\..\Release\plugins\ConEmu\Background\ConEmuBg_*.lng
..\Tools\lng.generator.exe -nc -i lang.ini -ol Lang.templ
for %%g in (..\..\Release\plugins\ConEmu\Background\ConEmuBg_*.lng) do ..\Tools\Convert.exe UTF2OEM %%g
if NOT "%~1"=="" (
  echo Copying *.lng files to %1
  copy ..\..\Release\plugins\ConEmu\Background\ConEmuBg_*.lng "%~1"
)
