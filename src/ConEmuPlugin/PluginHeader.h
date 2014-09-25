
/*
Copyright (c) 2009-2014 Maximus5
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

THIS SOFTWARE IS PROVIDED BY THE AUTHOR ''AS IS'' AND ANY EXPRESS OR
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
#include "ConEmu_Lang.h"
#include "FarDefaultMacros.h"
#include "../common/WinObjects.h"
#include "../common/CmdLine.h"
#include "PluginSrv.h"

//#define SafeCloseHandle(h) { if ((h)!=NULL) { HANDLE hh = (h); (h) = NULL; if (hh!=INVALID_HANDLE_VALUE) CloseHandle(hh); } }
#ifdef _DEBUG
#define OUTPUTDEBUGSTRING(m) OutputDebugString(m)
#else
#define OUTPUTDEBUGSTRING(m)
#endif

//#define SafeFree(p) { if ((p)!=NULL) { LPVOID pp = (p); (p) = NULL; free(pp); } }

#define ISALPHA(c) ((((c) >= (BYTE)'c') && ((c) <= (BYTE)'z')) || (((c) >= (BYTE)'C') && ((c) <= (BYTE)'Z')))
#define isPressed(inp) ((GetKeyState(inp) & 0x8000) == 0x8000)

// X - меньшая, Y - большая
#define FAR_X_VER 995
#define FAR_Y1_VER 1900
#define FAR_Y2_VER 2800
#define FUNC_X(fn) fn##995
#define FUNC_Y1(fn) fn##1900
#define FUNC_Y2(fn) fn##2800


extern DWORD gnMainThreadId;
extern int lastModifiedStateW;
//extern bool gbHandleOneRedraw; //, gbHandleOneRedrawCh;
extern WCHAR gszDir1[CONEMUTABMAX], gszDir2[CONEMUTABMAX];
extern int maxTabCount, lastWindowCount;
extern CESERVER_REQ* tabs; //(ConEmuTab*) calloc(maxTabCount, sizeof(ConEmuTab));
extern CESERVER_REQ* gpCmdRet;
extern HWND ghConEmuWndDC;
extern HWND FarHwnd;
extern FarVersion gFarVersion;
//#define IsFarLua ((gFarVersion.dwVerMajor > 3) || ((gFarVersion.dwVerMajor == 3) && (gFarVersion.dwBuild >= 2851)))
extern int lastModifiedStateW;
//extern HANDLE hEventCmd[MAXCMDCOUNT];
extern HANDLE hThread;
//extern WCHAR gcPlugKey;
//WARNING("Убрать, заменить ghConIn на GetStdHandle()"); // Иначе в Win7 будет буфер разрушаться
//extern HANDLE ghConIn;
extern BOOL gbNeedPostTabSend, gbNeedPostEditCheck;
extern HANDLE ghServerTerminateEvent;
extern const CESERVER_CONSOLE_MAPPING_HDR *gpConMapInfo;
extern DWORD gnSelfPID;
extern BOOL gbIgnoreUpdateTabs;
extern BOOL gbRequestUpdateTabs;
extern BOOL gbClosingModalViewerEditor;
extern CESERVER_REQ* gpTabs;
extern BOOL gbForceSendTabs;
extern int gnCurrentWindowType; // WTYPE_PANELS / WTYPE_VIEWER / WTYPE_EDITOR

typedef struct tag_PanelViewRegInfo
{
	BOOL bRegister;
	PanelViewInputCallback pfnPeekPreCall, pfnPeekPostCall, pfnReadPreCall, pfnReadPostCall;
	PanelViewOutputCallback pfnWriteCall;
} PanelViewRegInfo;
extern PanelViewRegInfo gPanelRegLeft, gPanelRegRight;

struct CurPanelDirs
{
	CmdArg *ActiveDir, *PassiveDir;
};
extern CurPanelDirs gPanelDirs;

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

BOOL AddTab(int &tabCount, int WindowPos, bool losingFocus, bool editorSave,
            int Type, LPCWSTR Name, LPCWSTR FileName,
			int Current, int Modified, int Modal,
			int EditViewId);

void SendTabs(int tabCount, BOOL abForceSend=FALSE);

void cmd_FarSetChanged(FAR_REQ_FARSETCHANGED *pFarSet);

void WINAPI OnLibraryLoaded(HMODULE ahModule);

#define ConEmu_SysID 0x43454D55 // 'CEMU'
#define ConEmu_GuidS L"4b675d80-1d4a-4ea9-8436-fdc23f2fc14b"
extern GUID guid_ConEmu;
extern GUID guid_ConEmuPluginItems;
extern GUID guid_ConEmuPluginMenu;
extern GUID guid_ConEmuGuiMacroDlg;
extern GUID guid_ConEmuWaitEndSynchro;

HANDLE OpenPluginWcmn(int OpenFrom,INT_PTR Item,bool FromMacro);
HANDLE WINAPI OpenPluginW1(int OpenFrom,INT_PTR Item);
HANDLE WINAPI OpenPluginW2(int OpenFrom,const GUID* Guid,INT_PTR Data);

void FUNC_X(GetPluginInfoW)(void *piv);
void FUNC_Y1(GetPluginInfoW)(void *piv);
void FUNC_Y2(GetPluginInfoW)(void *piv);

extern bool gbExitFarCalled;

int FUNC_X(ProcessEditorInputW)(LPCVOID Rec);
int FUNC_Y1(ProcessEditorInputW)(LPCVOID Rec);
int FUNC_Y2(ProcessEditorInputW)(LPCVOID Rec);
void StopThread(void);
void FUNC_X(ExitFARW)(void);
void FUNC_Y1(ExitFARW)(void);
void FUNC_Y2(ExitFARW)(void);
void FUNC_X(ProcessDragFromW)();
void FUNC_Y1(ProcessDragFromW)();
void FUNC_Y2(ProcessDragFromW)();
void ProcessDragFromA();
void FUNC_X(ProcessDragToW)();
void FUNC_Y1(ProcessDragToW)();
void FUNC_Y2(ProcessDragToW)();
void ProcessDragToA();

HWND AtoH(WCHAR *Str, int Len);

BOOL LoadFarVersion();

BOOL OutDataAlloc(DWORD anSize); // необязательно
BOOL OutDataWrite(LPVOID apData, DWORD anSize);

//void CheckMacro(BOOL abAllowAPI);
//BOOL IsKeyChanged(BOOL abAllowReload);
int ShowMessageGui(int aiMsg, int aiButtons);
int ShowMessage(int aiMsg, int aiButtons);
int ShowMessageA(int aiMsg, int aiButtons);
int FUNC_X(ShowMessageW)(int aiMsg, int aiButtons);
int FUNC_Y1(ShowMessageW)(int aiMsg, int aiButtons);
int FUNC_Y2(ShowMessageW)(int aiMsg, int aiButtons);
int ShowMessage(LPCWSTR asMsg, int aiButtons, bool bWarning);
int ShowMessageA(LPCSTR asMsg, int aiButtons, bool bWarning);
int FUNC_X(ShowMessageW)(LPCWSTR asMsg, int aiButtons, bool bWarning);
int FUNC_Y1(ShowMessageW)(LPCWSTR asMsg, int aiButtons, bool bWarning);
int FUNC_Y2(ShowMessageW)(LPCWSTR asMsg, int aiButtons, bool bWarning);
extern CEFAR_INFO_MAPPING *gpFarInfo;
extern HANDLE ghFarAliveEvent;
BOOL ReloadFarInfoA(/*BOOL abFull = FALSE*/);
BOOL FUNC_X(ReloadFarInfoW)(/*BOOL abFull = FALSE*/);
BOOL FUNC_Y1(ReloadFarInfoW)(/*BOOL abFull = FALSE*/);
BOOL FUNC_Y2(ReloadFarInfoW)(/*BOOL abFull = FALSE*/);
void PostMacro(const wchar_t* asMacro, INPUT_RECORD* apRec);
void PostMacroA(char* asMacro, INPUT_RECORD* apRec);
void FUNC_X(PostMacroW)(const wchar_t* asMacro, INPUT_RECORD* apRec);
void FUNC_Y1(PostMacroW)(const wchar_t* asMacro, INPUT_RECORD* apRec);
void FUNC_Y2(PostMacroW)(const wchar_t* asMacro, INPUT_RECORD* apRec);

extern DWORD gnReqCommand;
extern int gnPluginOpenFrom;
//extern HANDLE ghInputSynchroExecuted;
//extern BOOL gbCmdCallObsolete;
extern LPVOID gpReqCommandData;
BOOL ProcessCommand(DWORD nCmd, BOOL bReqMainThread, LPVOID pCommandData, CESERVER_REQ** ppResult = NULL, bool bForceSendTabs = false);
BOOL CheckPlugKey();
void NotifyChangeKey();


#if defined(__GNUC__)
extern "C" {
#endif
	BOOL WINAPI IsTerminalMode();
#if defined(__GNUC__)
}
#endif

BOOL FarSetConsoleSize(SHORT nNewWidth, SHORT nNewHeight);

BOOL StartupHooks(HMODULE ahOurDll);
void ShutdownHooks();

//void LogCreateProcessCheck(LPCWSTR asLogFileName);

BOOL FUNC_X(CheckBufferEnabledW)();
BOOL FUNC_Y1(CheckBufferEnabledW)();
BOOL FUNC_Y2(CheckBufferEnabledW)();

#define IS_SYNCHRO_ALLOWED \
	( \
	  gFarVersion.dwVerMajor > 2 || \
	  (gFarVersion.dwVerMajor == 2 && (gFarVersion.dwVerMinor>0 || gFarVersion.dwBuild>=1006)) \
	)

extern int gnSynchroCount;
extern bool gbSynchroProhibited;
extern bool gbInputSynchroPending;
extern BOOL TerminalMode;
void ExecuteSynchro(); // если доступен - позовет ACTL_SYNCHRO (FAR2 only)
BOOL FUNC_X(ExecuteSynchroW)();
BOOL FUNC_Y1(ExecuteSynchroW)();
BOOL FUNC_Y2(ExecuteSynchroW)();
void FUNC_X(WaitEndSynchroW)();
void FUNC_Y1(WaitEndSynchroW)();
void FUNC_Y2(WaitEndSynchroW)();
void FUNC_X(StopWaitEndSynchroW)();
void FUNC_Y1(StopWaitEndSynchroW)();
void FUNC_Y2(StopWaitEndSynchroW)();

void GuiMacroDlgA();
void FUNC_X(GuiMacroDlgW)();
void FUNC_Y1(GuiMacroDlgW)();
void FUNC_Y2(GuiMacroDlgW)();

void FillUpdateBackgroundA(struct PaintBackgroundArg* pFar);
void FUNC_X(FillUpdateBackgroundW)(struct PaintBackgroundArg* pFar);
void FUNC_Y1(FillUpdateBackgroundW)(struct PaintBackgroundArg* pFar);
void FUNC_Y2(FillUpdateBackgroundW)(struct PaintBackgroundArg* pFar);

struct HookCallbackArg;
BOOL OnConsoleReadInputWork(HookCallbackArg* pArgs);
VOID WINAPI OnConsoleReadInputPost(HookCallbackArg* pArgs);

#ifdef _DEBUG
#define SHOWDBGINFO(x) OutputDebugStringW(x)
#else
#define SHOWDBGINFO(x)
#endif

enum PluginCallCommands
{
	pcc_None = 0,
	//
	pcc_EditConsoleOutput = 1,
	pcc_ViewConsoleOutput = 2,
	pcc_SwitchTabVisible = 3,
	pcc_SwitchTabNext = 4,
	pcc_SwitchTabPrev = 5,
	pcc_SwitchTabCommit = 6,
	pcc_AttachToConEmu = 7,
	pcc_StartDebug = 8,
	pcc_ConsoleInfo = 9,
	//
	pcc_First = 1,
	pcc_Last = pcc_ConsoleInfo,
};

enum PluginMenuCommands
{
	menu_EditConsoleOutput = 0,
	menu_ViewConsoleOutput,
	menu_Separator1,
	menu_SwitchTabVisible,
	menu_SwitchTabNext,
	menu_SwitchTabPrev,
	menu_SwitchTabCommit,
	menu_ShowTabsList,
	menu_Separator2,
	menu_ConEmuMacro, // должен вызываться "по настоящему", а не через callplugin
	menu_Separator3,
	menu_AttachToConEmu,
	menu_Separator4,
	menu_StartDebug,
	menu_ConsoleInfo,
	//
	menu_Last = (menu_ConsoleInfo+1)
};

struct PluginAndMenuCommands
{
	int LangID; // ID для GetMsg
	PluginMenuCommands MenuID; // по сути - индекс в векторе
	PluginCallCommands CallID; // ID для CallPlugin
};

struct ConEmuPluginMenuItem
{
	bool    Separator;
	bool    Selected;
	bool    Disabled;
	bool    Checked;

	int     MsgID;
	LPCWSTR MsgText;

	INT_PTR UserData;
};

extern PluginAndMenuCommands gpPluginMenu[menu_Last];
bool pcc_Selected(PluginMenuCommands nMenuID);
bool pcc_Disabled(PluginMenuCommands nMenuID);

HANDLE WINAPI FUNC_Y1(OpenW)(const void* apInfo);
HANDLE WINAPI FUNC_Y2(OpenW)(const void* apInfo);
