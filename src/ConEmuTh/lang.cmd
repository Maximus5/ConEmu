@echo off
cd /d "%~dp0"
if exist ..\..\Release\plugins\ConEmu\Thumbs\ConEmuTh_*.lng del ..\..\Release\plugins\ConEmu\Thumbs\ConEmuTh_*.lng
..\Tools\lng.generator.exe -nc -i lang.ini -ol Lang.templ
for %%g in (..\..\Release\plugins\ConEmu\Thumbs\ConEmuTh_*.lng) do ..\Tools\Convert.exe UTF2OEM %%g
if NOT "%~1"=="" (
  echo Copying *.lng files to %1
  copy ..\..\Release\plugins\ConEmu\Thumbs\ConEmuTh_*.lng "%~1"
)
