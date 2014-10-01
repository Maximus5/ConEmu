@echo off
cd /d "%~dp0"
if not exist ..\..\Release\plugins\ConEmu\Lines md ..\..\Release\plugins\ConEmu\Lines
if exist ..\..\Release\plugins\ConEmu\Lines\ConEmuLn_*.lng del ..\..\Release\plugins\ConEmu\Lines\ConEmuLn_*.lng
..\Tools\lng.generator.exe -nc -i lang.ini -ol Lang.templ
for %%g in (..\..\Release\plugins\ConEmu\Lines\ConEmuLn_*.lng) do ..\Tools\Convert.exe UTF2OEM %%g
if NOT "%~1"=="" (
  echo Copying *.lng files to %1
  copy ..\..\Release\plugins\ConEmu\Lines\ConEmuLn_*.lng "%~1"
)
