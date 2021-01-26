@echo off
setlocal
cd /d "%~dp0"
call "%~dp0..\..\vc.build.set.x32.cmd"

if not exist "1" mkdir "1"
if not exist "1 @" mkdir "1 @"
if not exist "!1&@" mkdir "!1&@"

cl /std:c++17 cmd_caller.cpp /link Shell32.lib
if errorlevel 1 goto :EOF

copy cmd_caller.exe "1\cmd_printer.exe"
copy cmd_caller.exe "1 @\cmd_printer.exe"
copy cmd_caller.exe "!1&@\cmd_printer.exe"
if exist cmd_caller.obj del cmd_caller.obj
