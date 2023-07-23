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

echo Calling "%PYTHON3%" "%~dp0l10n.py" --l10n %L10N_PATH% --tx-pull all --write-l10n
call "%PYTHON3%" "%~dp0l10n.py" --l10n %L10N_PATH% --tx-pull all --write-l10n
if errorlevel 1 (
  echo [1;31;40mFailed to load updated translations from transifex[0m
  goto :err
)

call "%GIT%" status --porcelain | %windir%\system32\find "%L10N_PATH%"
if errorlevel 1 (
  echo [1;31;40mConEmu.l10n was not updated[0m
  goto fin
)

call "%~dp0GetCurVer.cmd"

call "%GIT%" --no-pager diff %L10N_PATH% -new_console:cs50V

echo [1;33;40mPress Enter to create commit "l10n: translations were updated"[0m
pause > nul

call "%GIT%" reset
if errorlevel 1 (
  echo [1;31;40mGit reset failed, l10n was not committed[0m
  goto :err
)
call "%GIT%" add %L10N_PATH%
if errorlevel 1 (
  echo [1;31;40mGit add failed, l10n was not committed[0m
  goto :err
)
call "%GIT%" commit -m "l10n: translations were updated"
if errorlevel 1 (
  echo [1;31;40mGit commit failed, l10n was not committed[0m
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
