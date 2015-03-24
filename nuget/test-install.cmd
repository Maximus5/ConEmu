@echo off
cd /d "%~dp0"

setlocal
set PATH=%~d0\Chocolatey\bin;%PATH%

choco install -y ConEmu -s '%cd%' -f
