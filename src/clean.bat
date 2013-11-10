@echo off

echo Cleaning obj files ...

if exist "%~dp0_VCBUILD" echo rd /Q /S "%~dp0_VCBUILD" & rd /Q /S "%~dp0_VCBUILD"

del /S /Q "%~dp0..\Release\*.lib">nul
del /S /Q "%~dp0..\Release\*.exp">nul

echo Cleaning obj files done
