#pragma once
#include "DragDrop.h"
#include "VirtualConsole.h"
#include "options.h"

extern VirtualConsole *pVCon;
extern HINSTANCE g_hInstance;
extern TCHAR Title[0x400];
extern bool isLBDown, /*isInDrag,*/ isDragProcessed;

extern HANDLE hPipe, hPipeEvent;

extern gSettings gSet;
extern TCHAR temp[MAX_PATH];
extern uint cBlinkNext;
extern DWORD WindowMode;


bool SetWindowMode(uint inMode);
HFONT CreateFontIndirectMy(LOGFONT *inFont);

void SyncWindowToConsole();
void SyncConsoleToWindow();

void SetConsoleSizeTo(HWND inConWnd, int inSizeX, int inSizeY);

#ifdef _DEBUG
    int __stdcall _MDEBUG_TRAP(LPCSTR asFile, int anLine);
    extern int MDEBUG_CHK;
    extern char gsz_MDEBUG_TRAP_MSG_APPEND[2000];
    #define MDEBUG_TRAP1(S1) {strcpy(gsz_MDEBUG_TRAP_MSG_APPEND,(S1));_MDEBUG_TRAP(__FILE__,__LINE__);}
    #include <crtdbg.h>
    #define MCHKHEAP { MDEBUG_CHK=_CrtCheckMemory(); if (!MDEBUG_CHK) { MDEBUG_TRAP1("_CrtCheckMemory failed"); } }
    //#define MCHKHEAP
#else
    #define MCHKHEAP
#endif
