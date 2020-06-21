@if "%~1" == "-v" echo preparing environment

@rem MSys2 breaks on 'TERM=msys'
@rem set TERM=msys
@set TERM=
@set LANG=en_US.UTF-8

@if exist %~d0\MinGW\msys\1.0\bin\head.exe set MINGWRT=%~d0\MinGW\msys\1.0\bin\
@if exist %~d0\MinGW\msys32\bin\head.exe set MINGWRT=%~d0\MinGW\msys32\bin\
@if exist "%~dp0..\..\Tools\MSYS\msys2-x64\usr\bin\head.exe" set "MINGWRT=%~dp0..\..\Tools\MSYS\msys2-x64\usr\bin\"
@if exist "%~dp0..\..\..\..\tools\msys64\usr\bin\head.exe" set "MINGWRT=%~dp0..\..\..\..\tools\msys64\usr\bin\"
@if "%~1" == "-v" echo\       MINGWRT: '%MINGWRT%'

@set CONEMU_WWW=
@if exist "%~dp0..\..\conemu.github.io\index.html" set "CONEMU_WWW=%~dp0..\..\conemu.github.io\"
@if exist "%~dp0..\..\ConEmu-GitHub-io\conemu.github.io\index.html" set "CONEMU_WWW=%~dp0..\..\ConEmu-GitHub-io\conemu.github.io\"
@if "%~1" == "-v" echo\    CONEMU_WWW: '%CONEMU_WWW%'

@set CONEMU_DEPLOY=
@if exist "%~dp0..\..\conemu-deploy\foss.cmd" set "CONEMU_DEPLOY=%~dp0..\..\conemu-deploy\"
@if "%~1" == "-v" echo\ CONEMU_DEPLOY: '%CONEMU_DEPLOY%'

@set UPLOADERS=
@if exist "%~dp0..\..\tools\Uploaders\Check-VirusTotal.cmd" set "UPLOADERS=%~dp0..\..\tools\Uploaders\"
@if "%~1" == "-v" echo\     UPLOADERS: '%UPLOADERS%'

@set FARRUN_EXIST=NO
@farrun.exe -? 1> nul 2> nul
@if errorlevel 1 (
  set FARRUN_EXIST=NO
) else (
  set FARRUN_EXIST=YES
)
@if "%~1" == "-v" echo\  FARRUN_EXIST: '%FARRUN_EXIST%'

@set GIT=git.exe
@if exist %~d0\gitsdk\cmd\git.exe set GIT=%~d0\gitsdk\cmd\git.exe
@if exist "C:\Program Files\Git\bin\git.exe" set "GIT=C:\Program Files\Git\bin\git.exe"
@if "%~1" == "-v" echo\           GIT: '%GIT%'

@set PYTHON3=python3.exe
@if exist "%USERPROFILE%\AppData\Local\Programs\Python\Python37-32\python3.exe" set "PYTHON3=%USERPROFILE%\AppData\Local\Programs\Python\Python37-32\python3.exe"
@if "%~1" == "-v" echo\       PYTHON3: '%PYTHON3%'

@set ZIP7=7z.exe
@if exist "%~dp0..\..\tools\Arch\7z.exe" set "ZIP7=%~dp0..\..\tools\Arch\7z.exe"
@if "%~1" == "-v" echo\          ZIP7: '%ZIP7%'
