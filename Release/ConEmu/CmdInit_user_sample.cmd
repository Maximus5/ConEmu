@rem !!! Sample user specific settings that will persist in upgrade !!!
@rem !!! Save as "%USERPROFILE%\.conemu\CmdInit.cmd" to activate    !!!

@rem @echo Loading ConEmu user settings...
@echo off

rem Allow (some) Unicode (https://ss64.com/nt/chcp.html)
chcp 65001

rem {Shell type} {path} right-angle-quote over two lines and no hostname:
rem		 CMD C:\Users\Me\Code
rem		 »
set PROMPT=$E[m$E[32mCMD$E\$S$E[92m$P$E[90m$_$E[90m»$E[m$S$E]9;12$E\
