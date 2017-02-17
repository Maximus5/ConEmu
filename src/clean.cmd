@echo off

echo Cleaning obj files ...

if exist "%~dp0_VCBUILD" (
rem echo rd /Q /S "%~dp0_VCBUILD"
rd /Q /S "%~dp0_VCBUILD"
)

del /S /Q "%~dp0..\Release\*.lib" > nul 2> nul
del /S /Q "%~dp0..\Release\*.exp" > nul 2> nul
del /S /Q "%~dp0..\Release\*.iobj" > nul 2> nul
del /S /Q "%~dp0..\Release\*.ipdb" > nul 2> nul

echo Cleaning obj files done
