@echo off

echo Cleaning dbg files ...

del /S /Q "%~dp0..\Release\*.map" > nul 2> nul
del /S /Q "%~dp0..\Release\*.pdb" > nul 2> nul

echo Cleaning dbg files done
