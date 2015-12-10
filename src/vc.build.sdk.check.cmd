@echo off

reg QUERY "HKLM\SOFTWARE\Microsoft\Microsoft SDKs\Windows" -v CurrentInstallFolder | %windir%\system32\find "v7.0"
@if errorlevel 1 goto sdk_error

@echo/    [1;32;40mSDK is OK[m
@goto :EOF

:sdk_error
echo .
if "%ConEmuANSI%"=="ON" (
echo [1;31;40mInvalid SDK selected in HKLM![0m
) else (
echo Invalid SDK selected in HKLM!
)
reg QUERY "HKLM\SOFTWARE\Microsoft\Microsoft SDKs\Windows" -v CurrentInstallFolder
exit 1
