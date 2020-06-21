@setlocal
@set dt=%date:~8,2%%date:~3,2%%date:~0,2%
@powershell -noprofile -command "%~dp0github-release-downloads.ps1" > "%~dp0downloads-%dt%.txt"
@type "%~dp0downloads-%dt%.txt"
