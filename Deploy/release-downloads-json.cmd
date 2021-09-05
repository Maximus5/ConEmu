@setlocal
@call "%~dp0GetCurDate.cmd"
@conemuc -download https://api.github.com/repos/Maximus5/ConEmu/releases "%~dp0downloads-%curdt%.json" 2>nul
