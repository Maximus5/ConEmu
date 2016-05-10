@set /P CurVerBuild=<"%~dp0..\src\ConEmu\version.h"
@set CurVerBuild=%CurVerBuild%
@set CurVerBuild=%CurVerBuild:~3%

:stable
@%windir%\system32\find /C "ConEmuVersionStage CEVS_STABLE" "%~dp0..\src\ConEmu\version_stage.h" > nul
@if errorlevel 1 goto preview
@set CurVerStage=STABLE
@goto done
:preview
@%windir%\system32\find /C "ConEmuVersionStage CEVS_PREVIEW" "%~dp0..\src\ConEmu\version_stage.h" > nul
@if errorlevel 1 goto alpha
@set CurVerStage=PREVIEW
@goto done
:alpha
@rem %windir%\system32\find /C "ConEmuVersionStage CEVS_ALPHA" "%~dp0..\src\ConEmu\version_stage.h" > nul
@set CurVerStage=ALPHA


:done

@if "%~1" == "-v" (
  echo CurVerBuild = `%CurVerBuild%`
  echo CurVerStage = `%CurVerStage%`
)
