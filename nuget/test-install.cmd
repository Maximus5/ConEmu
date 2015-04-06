@echo off
cd /d "%~dp0"

setlocal
set PATH=%~d0\Chocolatey\bin;%PATH%

choco install ConEmu -s '%cd%' -fy --version=%1
