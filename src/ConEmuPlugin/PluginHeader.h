#pragma once

extern int lastModifiedStateW;

void AddTab(ConEmuTab* tabs, int &tabCount, bool losingFocus, bool editorSave, 
			int Type, LPCWSTR Name, LPCWSTR FileName, int Current, int Modified);

void SendTabs(ConEmuTab* tabs, int &tabCount);


#ifdef _DEBUG
    int __stdcall _MDEBUG_TRAP(LPCSTR asFile, int anLine);
    extern int MDEBUG_CHK;
    extern char gsz_MDEBUG_TRAP_MSG_APPEND[2000];
    #define MDEBUG_TRAP1(S1) {lstrcpyA(gsz_MDEBUG_TRAP_MSG_APPEND,(S1));_MDEBUG_TRAP(__FILE__,__LINE__);}
    #include <crtdbg.h>
    #define MCHKHEAP { MDEBUG_CHK=_CrtCheckMemory(); if (!MDEBUG_CHK) { MDEBUG_TRAP1("_CrtCheckMemory failed"); } }
    //#define MCHKHEAP
#else
    #define MCHKHEAP
#endif
