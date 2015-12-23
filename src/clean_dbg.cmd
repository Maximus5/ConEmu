@echo off

echo Cleaning dbg files ...

del /S /Q "%~dp0..\Release\*.map" > nul
del /S /Q "%~dp0..\Release\*.pdb" > nul

echo Cleaning dbg files done
