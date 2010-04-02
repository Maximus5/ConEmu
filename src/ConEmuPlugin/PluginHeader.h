
/*
Copyright (c) 2009-2010 Maximus5
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions
are met:
1. Redistributions of source code must retain the above copyright
   notice, this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright
   notice, this list of conditions and the following disclaimer in the
   documentation and/or other materials provided with the distribution.
3. The name of the authors may not be used to endorse or promote products
   derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/


#pragma once

#if !defined(_MSC_VER)
#include <wchar.h>
#include <tchar.h>
#endif


#define SafeCloseHandle(h) { if ((h)!=NULL) { HANDLE hh = (h); (h) = NULL; if (hh!=INVALID_HANDLE_VALUE) CloseHandle(hh); } }
#ifdef _DEBUG
#define OUTPUTDEBUGSTRING(m) OutputDebugString(m)
#else
#define OUTPUTDEBUGSTRING(m)
#endif

#define ISALPHA(c) ((((c) >= (BYTE)'c') && ((c) <= (BYTE)'z')) || (((c) >= (BYTE)'C') && ((c) <= (BYTE)'Z')))
#define isPressed(inp) ((GetKeyState(inp) & 0x8000) == 0x8000)

// X - меньшая, Y - большая
#define FAR_X_VER 995
#define FAR_Y_VER 995
#define FUNC_X(fn) fn##995
#define FUNC_Y(fn) fn##995


extern int lastModifiedStateW;
extern bool gbHandleOneRedraw; //, gbHandleOneRedrawCh;
extern WCHAR gszDir1[CONEMUTABMAX], gszDir2[CONEMUTABMAX], gszRootKey[MAX_PATH*2];
extern int maxTabCount, lastWindowCount;
extern CESERVER_REQ* tabs; //(ConEmuTab*) calloc(maxTabCount, sizeof(ConEmuTab));
extern CESERVER_REQ* gpCmdRet;
extern HWND ConEmuHwnd;
extern HWND FarHwnd;
extern FarVersion gFarVersion;
extern int lastModifiedStateW;
//extern HANDLE hEventCmd[MAXCMDCOUNT];
extern HANDLE hThread;
//extern WCHAR gcPlugKey;
//WARNING("Убрать, заменить ghConIn на GetStdHandle()"); // Иначе в Win7 будет буфер разрушаться
//extern HANDLE ghConIn;
extern BOOL gbNeedPostTabSend;
extern HANDLE ghServerTerminateEvent;
extern CESERVER_REQ_CONINFO_HDR *gpConsoleInfo;
extern DWORD gnSelfPID;

typedef struct tag_PanelViewRegInfo {
	PanelViewEventCallback pfnReadCall;
} PanelViewRegInfo;
extern PanelViewRegInfo gPanelRegLeft, gPanelRegRight;

//typedef struct tag_SynchroArg {
//	enum {
//		eCommand,
//		eInput
//	} SynchroType;
//	HANDLE hEvent;
//	LPARAM Result;
//	LPARAM Param1, Param2;
//	BOOL Obsolete;
//	//BOOL Processed;
//} SynchroArg;

BOOL CreateTabs(int windowCount);

BOOL AddTab(int &tabCount, bool losingFocus, bool editorSave, 
			int Type, LPCWSTR Name, LPCWSTR FileName, int Current, int Modified);

void SendTabs(int tabCount, BOOL abForceSend=FALSE);

void InitHWND(HWND ahFarHwnd);

int FUNC_X(ProcessEditorInputW)(LPCVOID Rec);
int FUNC_Y(ProcessEditorInputW)(LPCVOID Rec);
int FUNC_X(ProcessEditorEventW)(int Event, void *Param);
int FUNC_Y(ProcessEditorEventW)(int Event, void *Param);
int FUNC_X(ProcessViewerEventW)(int Event, void *Param);
int FUNC_Y(ProcessViewerEventW)(int Event, void *Param);
void StopThread(void);
void FUNC_X(ExitFARW)(void);
void FUNC_Y(ExitFARW)(void);
void FUNC_X(UpdateConEmuTabsW)(int event, bool losingFocus, bool editorSave, void* Param/*=NULL*/);
void FUNC_Y(UpdateConEmuTabsW)(int event, bool losingFocus, bool editorSave, void* Param/*=NULL*/);
void FUNC_X(SetStartupInfoW)(void *aInfo);
void FUNC_Y(SetStartupInfoW)(void *aInfo);
void FUNC_X(ProcessDragFrom)();
void FUNC_Y(ProcessDragFrom)();
void ProcessDragFromA();
void FUNC_X(ProcessDragTo)();
void FUNC_Y(ProcessDragTo)();
void ProcessDragToA();
void SetWindowA(int nTab);
void FUNC_X(SetWindow)(int nTab);
void FUNC_Y(SetWindow)(int nTab);

void CheckResources(BOOL abFromStartup);
void InitResources();
void CloseTabs();

HWND AtoH(WCHAR *Str, int Len);
void UpdateConEmuTabsW(int event, bool losingFocus, bool editorSave, void* Param=NULL);

BOOL LoadFarVersion();

BOOL OutDataAlloc(DWORD anSize); // необязательно
BOOL OutDataWrite(LPVOID apData, DWORD anSize);

//void CheckMacro(BOOL abAllowAPI);
//BOOL IsKeyChanged(BOOL abAllowReload);
int ShowMessage(int aiMsg, int aiButtons);
int ShowMessageA(int aiMsg, int aiButtons);
int FUNC_X(ShowMessage)(int aiMsg, int aiButtons);
int FUNC_Y(ShowMessage)(int aiMsg, int aiButtons);
//void ReloadMacroA();
//void FUNC_X(ReloadMacro)();
//void FUNC_Y(ReloadMacro)();
extern CEFAR_INFO *gpFarInfo;
void ReloadFarInfoA(BOOL abFull = FALSE);
void FUNC_X(ReloadFarInfo)(BOOL abFull = FALSE);
#if (FAR_X_VER!=FAR_Y_VER)
void FUNC_Y(ReloadFarInfo)(BOOL abFull = FALSE);
#endif
void PostMacro(wchar_t* asMacro);
void PostMacroA(char* asMacro);
void FUNC_X(PostMacro)(wchar_t* asMacro);
void FUNC_Y(PostMacro)(wchar_t* asMacro);
LPCWSTR GetMsgW(int aiMsg);
void GetMsgA(int aiMsg, wchar_t* rsMsg/*MAX_PATH*/);
LPCWSTR FUNC_Y(GetMsg)(int aiMsg);
LPCWSTR FUNC_X(GetMsg)(int aiMsg);

extern DWORD gnReqCommand;
extern int gnPluginOpenFrom;
//extern HANDLE ghInputSynchroExecuted;
//extern BOOL gbCmdCallObsolete;
extern LPVOID gpReqCommandData;
CESERVER_REQ* ProcessCommand(DWORD nCmd, BOOL bReqMainThread, LPVOID pCommandData);
BOOL CheckPlugKey();
void NotifyChangeKey();


#if defined(__GNUC__)
extern "C"{
#endif
BOOL WINAPI IsTerminalMode();
#if defined(__GNUC__)
}
#endif


DWORD WINAPI ServerThread(LPVOID lpvParam);
DWORD WINAPI ServerThreadCommand(LPVOID ahPipe);

void ShowPluginMenu(int nID = -1);
int ShowPluginMenuA();
int FUNC_Y(ShowPluginMenu)();
int FUNC_X(ShowPluginMenu)();

BOOL EditOutputA(LPCWSTR asFileName, BOOL abView);
BOOL FUNC_Y(EditOutput)(LPCWSTR asFileName, BOOL abView);
BOOL FUNC_X(EditOutput)(LPCWSTR asFileName, BOOL abView);

BOOL Attach2Gui();
BOOL StartDebugger();

//#define DEFAULT_SYNCHRO_TIMEOUT 10000
//BOOL FUNC_X(CallSynchro)(SynchroArg *Param, DWORD nTimeout /*= 10000*/);
//BOOL FUNC_Y(CallSynchro)(SynchroArg *Param, DWORD nTimeout /*= 10000*/);

BOOL IsMacroActive();
BOOL IsMacroActiveA();
BOOL FUNC_X(IsMacroActive)();
BOOL FUNC_Y(IsMacroActive)();

//BOOL SendConsoleEvent(INPUT_RECORD* pr, UINT nCount);

void RedrawAll();
void RedrawAllA();
void FUNC_Y(RedrawAll)();
void FUNC_X(RedrawAll)();

bool FUNC_Y(LoadPlugin)(wchar_t* pszPluginPath);
bool FUNC_X(LoadPlugin)(wchar_t* pszPluginPath);

DWORD FUNC_Y(GetEditorModifiedState)();
DWORD FUNC_X(GetEditorModifiedState)();
DWORD GetEditorModifiedStateA();
DWORD GetEditorModifiedState();

BOOL FarSetConsoleSize(SHORT nNewWidth, SHORT nNewHeight);

BOOL StartupHooks(HMODULE ahOurDll);
void ShutdownHooks();

bool FUNC_Y(ProcessCommandLine)(wchar_t* pszCommand);
bool FUNC_X(ProcessCommandLine)(wchar_t* pszCommand);
bool ProcessCommandLineA(char* pszCommand);

#ifdef _DEBUG
	#define SHOWDBGINFO(x) OutputDebugStringW(x)
#else
	#define SHOWDBGINFO(x)
#endif
