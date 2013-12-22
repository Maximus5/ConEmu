set /P CurVerBuild=<"%~dp0..\src\ConEmu\version.h"
set CurVerBuild=%CurVerBuild%
set CurVerBuild=%CurVerBuild:~3%
rem echo `%CurVerBuild%`
