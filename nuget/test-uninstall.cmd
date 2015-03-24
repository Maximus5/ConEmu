@echo off
cd /d "%~dp0"

setlocal
set PATH=%~d0\Chocolatey\bin;%PATH%

choco uninstall -y ConEmu
