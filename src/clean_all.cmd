@echo off

call "%~dp0clean.cmd"
call "%~dp0clean_dbg.cmd"

echo Cleaning PE files ...

del /S /Q "%~dp0..\Release\ConEmu*.exe" > nul 2> nul
del /S /Q "%~dp0..\Release\*.dll" > nul 2> nul
del /S /Q "%~dp0..\Release\*.t32" > nul 2> nul
del /S /Q "%~dp0..\Release\*.t64" > nul 2> nul

echo Cleaning PE files done
