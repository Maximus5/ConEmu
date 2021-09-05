@setlocal
@call "%~dp0GetCurDate.cmd"
@powershell -noprofile -ExecutionPolicy RemoteSigned -command "%~dp0github-release-downloads.ps1" > "%~dp0downloads-%curdt%.txt"
@type "%~dp0downloads-%curdt%.txt"
