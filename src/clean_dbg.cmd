@echo on

echo Cleaning dbg files ...

del /S /Q "%~dp0..\Release\*.map"
del /S /Q "%~dp0..\Release\*.pdb"

echo Cleaning dbg files done

@echo off
