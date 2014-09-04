
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
#include "../Common/WinObjects.h"
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
extern WCHAR gszRootKey[MAX_PATH*2];
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

void InitHWND(/*HWND ahFarHwnd*/);
void InitRootKey();

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

void ExitFarCmn();
extern BOOL gbExitFarCalled;

int FUNC_X(ProcessEditorInputW)(LPCVOID Rec);
int FUNC_Y1(ProcessEditorInputW)(LPCVOID Rec);
int FUNC_Y2(ProcessEditorInputW)(LPCVOID Rec);
int FUNC_X(ProcessEditorEventW)(int Event, void *Param);
int FUNC_Y1(ProcessEditorEventW)(int Event, void *Param);
int FUNC_Y2(ProcessEditorEventW)(int Event, void *Param);
int FUNC_X(ProcessViewerEventW)(int Event, void *Param);
int FUNC_Y1(ProcessViewerEventW)(int Event, void *Param);
int FUNC_Y2(ProcessViewerEventW)(int Event, void *Param);
void StopThread(void);
void FUNC_X(ExitFARW)(void);
void FUNC_Y1(ExitFARW)(void);
void FUNC_Y2(ExitFARW)(void);
void FUNC_X(SetStartupInfoW)(void *aInfo);
void FUNC_Y1(SetStartupInfoW)(void *aInfo);
void FUNC_Y2(SetStartupInfoW)(void *aInfo);
void FUNC_X(ProcessDragFromW)();
void FUNC_Y1(ProcessDragFromW)();
void FUNC_Y2(ProcessDragFromW)();
void ProcessDragFromA();
void FUNC_X(ProcessDragToW)();
void FUNC_Y1(ProcessDragToW)();
void FUNC_Y2(ProcessDragToW)();
void ProcessDragToA();
void SetWindowA(int nTab);
void FUNC_X(SetWindowW)(int nTab);
void FUNC_Y1(SetWindowW)(int nTab);
void FUNC_Y2(SetWindowW)(int nTab);

void CheckResources(BOOL abFromStartup);
void InitResources();
void CloseTabs();

HWND AtoH(WCHAR *Str, int Len);
bool UpdateConEmuTabs(bool abSendChanges);

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
LPCWSTR GetMsgW(int aiMsg);
void GetMsgA(int aiMsg, wchar_t (&rsMsg)[MAX_PATH]);
LPCWSTR FUNC_X(GetMsgW)(int aiMsg);
LPCWSTR FUNC_Y1(GetMsgW)(int aiMsg);
LPCWSTR FUNC_Y2(GetMsgW)(int aiMsg);

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


BOOL EditOutputA(LPCWSTR asFileName, BOOL abView);
BOOL FUNC_X(EditOutputW)(LPCWSTR asFileName, BOOL abView);
BOOL FUNC_Y1(EditOutputW)(LPCWSTR asFileName, BOOL abView);
BOOL FUNC_Y2(EditOutputW)(LPCWSTR asFileName, BOOL abView);

BOOL Attach2Gui();
BOOL StartDebugger();
void ShowConsoleInfo();

//#define DEFAULT_SYNCHRO_TIMEOUT 10000
//BOOL FUNC_X(CallSynchro)(SynchroArg *Param, DWORD nTimeout /*= 10000*/);
//BOOL FUNC_Y1(CallSynchro)(SynchroArg *Param, DWORD nTimeout /*= 10000*/);
//BOOL FUNC_Y2(CallSynchro)(SynchroArg *Param, DWORD nTimeout /*= 10000*/);

bool isMacroActive(int& iMacroActive);
BOOL IsMacroActive();
BOOL IsMacroActiveA();
BOOL FUNC_X(IsMacroActiveW)();
BOOL FUNC_Y1(IsMacroActiveW)();
BOOL FUNC_Y2(IsMacroActiveW)();

int GetMacroArea();
int FUNC_X(GetMacroAreaW)();
int FUNC_Y1(GetMacroAreaW)();
int FUNC_Y2(GetMacroAreaW)();

//BOOL SendConsoleEvent(INPUT_RECORD* pr, UINT nCount);

void RedrawAll();
void RedrawAllA();
void FUNC_X(RedrawAllW)();
void FUNC_Y1(RedrawAllW)();
void FUNC_Y2(RedrawAllW)();

bool FUNC_X(LoadPluginW)(wchar_t* pszPluginPath);
bool FUNC_Y1(LoadPluginW)(wchar_t* pszPluginPath);
bool FUNC_Y2(LoadPluginW)(wchar_t* pszPluginPath);

DWORD GetEditorModifiedStateA();
DWORD FUNC_X(GetEditorModifiedStateW)();
DWORD FUNC_Y1(GetEditorModifiedStateW)();
DWORD FUNC_Y2(GetEditorModifiedStateW)();
DWORD GetEditorModifiedState();

int GetActiveWindowType();
int GetActiveWindowTypeA();
int FUNC_X(GetActiveWindowTypeW)();
int FUNC_Y1(GetActiveWindowTypeW)();
int FUNC_Y2(GetActiveWindowTypeW)();

BOOL FarSetConsoleSize(SHORT nNewWidth, SHORT nNewHeight);

BOOL StartupHooks(HMODULE ahOurDll);
void ShutdownHooks();

bool RunExternalProgramW(wchar_t* pszCommand, wchar_t* pszCurDir, bool bSilent=false);
bool FUNC_X(ProcessCommandLineW)(wchar_t* pszCommand);
bool FUNC_Y1(ProcessCommandLineW)(wchar_t* pszCommand);
bool FUNC_Y2(ProcessCommandLineW)(wchar_t* pszCommand);
bool ProcessCommandLineA(char* pszCommand);

//void ExecuteQuitFar();
//void ExecuteQuitFarA();
//void FUNC_X(ExecuteQuitFar)();
//void FUNC_Y1(ExecuteQuitFar)();
//void FUNC_Y2(ExecuteQuitFar)();

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

void CommonPluginStartup();

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

void ShowPluginMenu(PluginCallCommands nCallID = pcc_None);
int ShowPluginMenuA(ConEmuPluginMenuItem* apItems, int Count);
int FUNC_X(ShowPluginMenuW)(ConEmuPluginMenuItem* apItems, int Count);
int FUNC_Y1(ShowPluginMenuW)(ConEmuPluginMenuItem* apItems, int Count);
int FUNC_Y2(ShowPluginMenuW)(ConEmuPluginMenuItem* apItems, int Count);

void ShutdownPluginStep(LPCWSTR asInfo, int nParm1 = 0, int nParm2 = 0, int nParm3 = 0, int nParm4 = 0);

int WINAPI FUNC_Y1(ProcessEditorEventW)(void* p);
INT_PTR WINAPI FUNC_Y2(ProcessEditorEventW)(void* p);
int WINAPI FUNC_Y1(ProcessViewerEventW)(void* p);
INT_PTR WINAPI FUNC_Y2(ProcessViewerEventW)(void* p);
int WINAPI FUNC_Y1(ProcessDialogEventW)(void* p);
INT_PTR WINAPI FUNC_Y2(ProcessDialogEventW)(void* p);
int WINAPI FUNC_Y1(ProcessSynchroEventW)(void* p);
INT_PTR WINAPI FUNC_Y2(ProcessSynchroEventW)(void* p);

HANDLE WINAPI FUNC_Y1(OpenW)(const void* apInfo);
HANDLE WINAPI FUNC_Y2(OpenW)(const void* apInfo);

int WINAPI FUNC_Y1(ProcessConsoleInputW)(void *Info);
INT_PTR WINAPI FUNC_Y2(ProcessConsoleInputW)(void *Info);
