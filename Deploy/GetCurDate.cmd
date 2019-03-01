@for /f %%d in ('powershell -noprofile -command Get-Date -Format yyMMdd') do @set curdt=%%d
@if "%~1" == "-v" echo curdt=%curdt%
