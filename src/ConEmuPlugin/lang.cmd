@echo off
cd /d "%~dp0"
if exist ..\..\Release\plugins\ConEmu\ConEmuPlugin_*.lng del ..\..\Release\plugins\ConEmu\ConEmuPlugin_*.lng
..\Tools\lng.generator.exe -nc -i lang.ini -ol Lang.templ
for %%g in (..\..\Release\plugins\ConEmu\ConEmuPlugin_*.lng) do ..\Tools\Convert.exe UTF2OEM %%g
if NOT "%~1"=="" (
  echo Copying *.lng files to %1
  copy ..\..\Release\plugins\ConEmu\ConEmuPlugin_*.lng "%~1"
)
