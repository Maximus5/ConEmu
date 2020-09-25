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
set YAML_PATH=src\l10n

call "%PYTHON3%" "%~dp0l10n.py" --l10n %L10N_PATH% --write-yaml %YAML_PATH%
if errorlevel 1 (
  echo [1;31;40mFailed to write yaml files[0m
  goto :err
)

echo [1;33;40mPlease manually upload file ConEmu_en.yaml to transifex at[0m
echo [1;33;40mhttps://www.transifex.com/conemu/conemu-sources/conemu-en-yaml--daily/[0m

goto fin

:err
endlocal
popd
exit /b 1

:fin
endlocal
popd
