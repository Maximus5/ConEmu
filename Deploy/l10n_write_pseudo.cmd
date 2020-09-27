@echo off

setlocal

if exist "%~dp0user_env.cmd" (
  call "%~dp0user_env.cmd"
) else (
  call "%~dp0user_env.default.cmd"
)

call "%UPLOADERS%init_env.cmd"

pushd "%~dp0.."

set L10N_PATH=Release/ConEmu/ConEmu.l10n
set PSEUDO_PATH=src\l10n\ConEmu.l10n

call "%PYTHON3%" "%~dp0l10n.py" --l10n %L10N_PATH% --write-pseudo %PSEUDO_PATH%
if errorlevel 1 (
  echo [1;31;40mFailed to write l10n file[0m
  goto :err
)

goto fin

:err
endlocal
popd
exit /b 1

:fin
endlocal
popd
