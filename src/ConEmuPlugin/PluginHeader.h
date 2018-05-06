
/*
Copyright (c) 2009-present Maximus5
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

#include "../common/defines.h"
#include "ConEmu_Lang.h"
#include "FarDefaultMacros.h"
#include "../common/WObjects.h"
#include "../common/CmdLine.h"
#include "PluginSrv.h"

#ifdef _DEBUG
#define OUTPUTDEBUGSTRING(m) OutputDebugString(m)
#else
#define OUTPUTDEBUGSTRING(m)
#endif

#define isPressed(inp) ((GetKeyState(inp) & 0x8000) == 0x8000)

#define ConEmu_SysID 0x43454D55 // 'CEMU'
#define ConEmu_GuidS L"4b675d80-1d4a-4ea9-8436-fdc23f2fc14b"

#define MIN_FAR2_BUILD 1765 // svs 19.12.2010 22:52:53 +0300 - build 1765: Новая команда в FARMACROCOMMAND - MCMD_GETAREA
#define MIN_FAR3_BUILD 2578

#define CMD__EXTERNAL_CALLBACK 0x80001

#define CHECK_RESOURCES_INTERVAL 5000
#define CHECK_FARINFO_INTERVAL 2000
#define ATTACH_START_SERVER_TIMEOUT 10000

struct SyncExecuteArg
{
	DWORD nCmd;
	HMODULE hModule;
	SyncExecuteCallback_t CallBack;
	LONG_PTR lParam;
};

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
	pcc_ShowTabsList = 10,
	pcc_ShowPanelsList = 11,
	//
	pcc_LastAuto,
	//
	pcc_First = 1,
	pcc_Last = pcc_LastAuto-1,
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
	menu_ShowPanelsList,
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

struct PanelViewRegInfo
{
	BOOL bRegister;
	PanelViewInputCallback pfnPeekPreCall, pfnPeekPostCall, pfnReadPreCall, pfnReadPostCall;
	PanelViewOutputCallback pfnWriteCall;
};

struct MSectionSimple;
class MSection;

extern HMODULE ghPluginModule;
extern DWORD gnMainThreadId;
extern WCHAR gszDir1[CONEMUTABMAX], gszDir2[CONEMUTABMAX];
extern int maxTabCount, lastWindowCount, gnCurTabCount;
extern CESERVER_REQ* gpCmdRet;
extern HWND ghConEmuWndDC;
extern HWND FarHwnd;
extern FarVersion gFarVersion;
extern const CESERVER_CONSOLE_MAPPING_HDR *gpConMapInfo;
extern BOOL gbIgnoreUpdateTabs;
extern CESERVER_REQ* gpTabs;
extern BOOL gbForceSendTabs;
extern int gnCurrentWindowType; // WTYPE_PANELS / WTYPE_VIEWER / WTYPE_EDITOR
extern PanelViewRegInfo gPanelRegLeft, gPanelRegRight;

#if defined(__GNUC__)
extern "C" {
#endif
// Some our exports
BOOL WINAPI IsConsoleActive();
BOOL WINAPI IsTerminalMode();
HWND WINAPI GetFarHWND2(int anConEmuOnly);
HWND WINAPI GetFarHWND();
void WINAPI GetFarVersion(FarVersion* pfv);
int WINAPI RegisterPanelView(PanelViewInit *ppvi);
int WINAPI RegisterBackground(RegisterBackgroundArg *pbk);
int WINAPI SyncExecute(HMODULE ahModule, SyncExecuteCallback_t CallBack, LONG_PTR lParam);
int WINAPI ActivateConsole();
// Plugin API exports
void WINAPI GetPluginInfo(void *piv);
void WINAPI GetPluginInfoW(void *piv);
void WINAPI SetStartupInfo(void *aInfo);
void WINAPI SetStartupInfoW(void *aInfo);
int WINAPI GetMinFarVersion();
int WINAPI GetMinFarVersionW();
int WINAPI ProcessSynchroEventW(int Event, void *Param);
INT_PTR WINAPI ProcessSynchroEventW3(void* p);
int WINAPI ProcessEditorEvent(int Event, void *Param);
int WINAPI ProcessEditorEventW(int Event, void *Param);
INT_PTR WINAPI ProcessEditorEventW3(void* p);
int WINAPI ProcessViewerEvent(int Event, void *Param);
int WINAPI ProcessViewerEventW(int Event, void *Param);
INT_PTR WINAPI ProcessViewerEventW3(void* p);
int WINAPI ProcessEditorInput(const INPUT_RECORD *Rec);
int WINAPI ProcessEditorInputW(void* Rec);
HANDLE WINAPI OpenPlugin(int OpenFrom,INT_PTR Item);
HANDLE WINAPI OpenPluginW(int OpenFrom, INT_PTR Item);
HANDLE WINAPI OpenW(const void* Info);
void WINAPI ExitFAR(void);
void WINAPI ExitFARW(void);
void WINAPI ExitFARW3(void*);
#if defined(__GNUC__)
}
#endif

void SetupExportsFar3();

bool StartupHooks(HMODULE ahOurDll);
void StartupConsoleHooks();
void ShutdownHooks();

extern HMODULE ghExtConModule;


#define IS_SYNCHRO_ALLOWED \
	( \
	  gFarVersion.dwVerMajor > 2 || \
	  (gFarVersion.dwVerMajor == 2 && (gFarVersion.dwVerMinor>0 || gFarVersion.dwBuild>=1006)) \
	)

extern int gnSynchroCount;
extern BOOL TerminalMode;

struct HookCallbackArg;

#ifdef _DEBUG
#define SHOWDBGINFO(x) OutputDebugStringW(x)
#else
#define SHOWDBGINFO(x)
#endif
