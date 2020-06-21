@setlocal
@set dt=%date:~8,2%%date:~3,2%%date:~0,2%
@conemuc -download https://api.github.com/repos/Maximus5/ConEmu/releases "%~dp0downloads-%dt%.json" 2>nul
