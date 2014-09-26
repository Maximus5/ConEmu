
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


#ifdef _DEBUG
//  Раскомментировать, чтобы сразу после загрузки плагина показать MessageBox, чтобы прицепиться дебаггером
//  #define SHOW_STARTED_MSGBOX
#endif

//#define TRUE_COLORER_OLD_SUPPORT

#define SHOWDEBUGSTR
//#define MCHKHEAP
#define DEBUGSTRMENU(s) //DEBUGSTR(s)
#define DEBUGSTRINPUT(s) //DEBUGSTR(s)
#define DEBUGSTRDLGEVT(s) //DEBUGSTR(s)
#define DEBUGSTRCMD(s) DEBUGSTR(s)
#define DEBUGSTRACTIVATE(s) DEBUGSTR(s)


#include <windows.h>
#include "../common/common.hpp"
#include "../common/MWow64Disable.h"
#include "../ConEmuHk/SetHook.h"
#ifdef _DEBUG
#pragma warning( disable : 4995 )
#endif
#include "../common/pluginW1761.hpp"
#ifdef _DEBUG
#pragma warning( default : 4995 )
#endif
#include "../common/ConEmuCheckEx.h"
#include "../common/ConsoleAnnotation.h"
#include "../common/SetEnvVar.h"
#include "../common/WinObjects.h"
#include "../common/WinConsole.h"
#include "../common/TerminalMode.h"
#include "../common/MFileMapping.h"
#include "../common/MSection.h"
#include "../ConEmu/version.h"
#include "PluginHeader.h"
#include "ConEmuPluginBase.h"
#include "PluginBackground.h"
#include <Tlhelp32.h>

#ifndef __GNUC__
	#include <Dbghelp.h>
#else
#endif

#include "../common/ConEmuCheck.h"
#include "PluginSrv.h"

#define Free free
#define Alloc calloc

#define MAKEFARVERSION(major,minor,build) ( ((major)<<8) | (minor) | ((build)<<16))

#define CHECK_RESOURCES_INTERVAL 5000
#define CHECK_FARINFO_INTERVAL 2000
#define ATTACH_START_SERVER_TIMEOUT 10000

#if defined(__GNUC__)
extern "C" {
	BOOL WINAPI DllMain(HANDLE hModule, DWORD  ul_reason_for_call, LPVOID lpReserved);
	HWND WINAPI GetFarHWND();
	HWND WINAPI GetFarHWND2(int anConEmuOnly);
	void WINAPI GetFarVersion(FarVersion* pfv);
	int  WINAPI ProcessEditorInputW(void* Rec);
	void WINAPI SetStartupInfoW(void *aInfo);
	BOOL WINAPI IsTerminalMode();
	BOOL WINAPI IsConsoleActive();
	int  WINAPI RegisterPanelView(PanelViewInit *ppvi);
	int  WINAPI RegisterBackground(RegisterBackgroundArg *pbk);
	int  WINAPI ActivateConsole();
	int  WINAPI SyncExecute(HMODULE ahModule, SyncExecuteCallback_t CallBack, LONG_PTR lParam);
	void WINAPI GetPluginInfoWcmn(void *piv);
};
#endif


HMODULE ghPluginModule = NULL; // ConEmu.dll - сам плагин
HWND ghConEmuWndDC = NULL; // Содержит хэндл окна отрисовки. Это ДОЧЕРНЕЕ окно.
DWORD gdwPreDetachGuiPID = 0;
DWORD gdwServerPID = 0;
BOOL TerminalMode = FALSE;
HWND FarHwnd = NULL;
DWORD gnMainThreadId = 0, gnMainThreadIdInitial = 0;
HANDLE ghMonitorThread = NULL; DWORD gnMonitorThreadId = 0;
HANDLE ghSetWndSendTabsEvent = NULL;
FarVersion gFarVersion = {};
WCHAR gszDir1[CONEMUTABMAX], gszDir2[CONEMUTABMAX];
int maxTabCount = 0, lastWindowCount = 0, gnCurTabCount = 0;
CESERVER_REQ* gpTabs = NULL; //(ConEmuTab*) Alloc(maxTabCount, sizeof(ConEmuTab));
BOOL gbForceSendTabs = FALSE;
int  gnCurrentWindowType = 0; // WTYPE_PANELS / WTYPE_VIEWER / WTYPE_EDITOR
BOOL gbIgnoreUpdateTabs = FALSE; // выставляется на время CMD_SETWINDOW
BOOL gbRequestUpdateTabs = FALSE; // выставляется при получении события FOCUS/KILLFOCUS
CurPanelDirs gPanelDirs = {};
BOOL gbClosingModalViewerEditor = FALSE; // выставляется при закрытии модального редактора/вьювера
MOUSE_EVENT_RECORD gLastMouseReadEvent = {{0,0}};
BOOL gbUngetDummyMouseEvent = FALSE;
LONG gnAllowDummyMouseEvent = 0;
LONG gnDummyMouseEventFromMacro = 0;

extern HMODULE ghHooksModule;
extern BOOL gbHooksModuleLoaded; // TRUE, если был вызов LoadLibrary("ConEmuHk.dll"), тогда его нужно FreeLibrary при выходе


MSection *csData = NULL;
// результат выполнения команды (пишется функциями OutDataAlloc/OutDataWrite)
CESERVER_REQ* gpCmdRet = NULL;
// инициализируется как "gpData = gpCmdRet->Data;"
LPBYTE gpData = NULL, gpCursor = NULL;
DWORD  gnDataSize=0;

int gnPluginOpenFrom = -1;
DWORD gnReqCommand = -1;
LPVOID gpReqCommandData = NULL;
static HANDLE ghReqCommandEvent = NULL;
static BOOL   gbReqCommandWaiting = FALSE;


UINT gnMsgTabChanged = 0;
MSection *csTabs = NULL;
BOOL  gbPlugKeyChanged=FALSE;
HKEY  ghRegMonitorKey=NULL; HANDLE ghRegMonitorEvt=NULL;
HANDLE ghPluginSemaphore = NULL;
wchar_t gsFarLang[64] = {0};
BOOL FindServerCmd(DWORD nServerCmd, DWORD &dwServerPID, bool bFromAttach = false);
BOOL gbNeedPostTabSend = FALSE;
BOOL gbNeedPostEditCheck = FALSE; // проверить, может в активном редакторе изменился статус
int lastModifiedStateW = -1;
BOOL gbNeedPostReloadFarInfo = FALSE;
DWORD gnNeedPostTabSendTick = 0;
#define NEEDPOSTTABSENDDELTA 100
#define MONITORENVVARDELTA 1000
void UpdateEnvVar(const wchar_t* pszList);
BOOL StartupHooks();
MFileMapping<CESERVER_CONSOLE_MAPPING_HDR> *gpConMap;
const CESERVER_CONSOLE_MAPPING_HDR *gpConMapInfo = NULL;
//AnnotationInfo *gpColorerInfo = NULL;
BOOL gbStartedUnderConsole2 = FALSE;
DWORD gnSelfPID = 0; //GetCurrentProcessId();
HANDLE ghFarInfoMapping = NULL;
CEFAR_INFO_MAPPING *gpFarInfo = NULL, *gpFarInfoMapping = NULL;
HANDLE ghFarAliveEvent = NULL;
PanelViewRegInfo gPanelRegLeft = {NULL};
PanelViewRegInfo gPanelRegRight = {NULL};
// Для плагинов PicView & MMView нужно знать, нажат ли CtrlShift при F3
HANDLE ghConEmuCtrlPressed = NULL, ghConEmuShiftPressed = NULL;
BOOL gbWaitConsoleInputEmpty = FALSE, gbWaitConsoleWrite = FALSE; //, gbWaitConsoleInputPeek = FALSE;
HANDLE ghConsoleInputEmpty = NULL, ghConsoleWrite = NULL; //, ghConsoleInputWasPeek = NULL;
DWORD GetMainThreadId();
int gnSynchroCount = 0;
bool gbSynchroProhibited = false;
bool gbInputSynchroPending = false;

struct HookModeFar gFarMode = {sizeof(HookModeFar), TRUE/*bFarHookMode*/};
extern SetFarHookMode_t SetFarHookMode;

// export

void WINAPI GetPluginInfo(void *piv)
{
	if (gFarVersion.dwVerMajor != 1)
	{
		gFarVersion.dwVerMajor = 1;
		gFarVersion.dwVerMinor = 75;
	}

	Plugin()->GetPluginInfo(piv);
}

void WINAPI GetPluginInfoWcmn(void *piv)
{
	Plugin()->GetPluginInfo(piv);
}

DWORD gnPeekReadCount = 0;







/* COMMON - end */



void WINAPI ExitFARW(void);
void WINAPI ExitFARW3(void*);

#include "../common/SetExport.h"
ExportFunc Far3Func[] =
{
	{"ExitFARW", (void*)ExitFARW, (void*)ExitFARW3},
	{"ProcessEditorEventW", (void*)ProcessEditorEventW, (void*)ProcessEditorEventW3},
	{"ProcessViewerEventW", (void*)ProcessViewerEventW, (void*)ProcessViewerEventW3},
	{"ProcessSynchroEventW", (void*)ProcessSynchroEventW, (void*)ProcessSynchroEventW3},
	{NULL}
};

bool gbExitFarCalled = false;

BOOL WINAPI DllMain(HANDLE hModule, DWORD  ul_reason_for_call, LPVOID lpReserved)
{
	switch(ul_reason_for_call)
	{
		case DLL_PROCESS_ATTACH:
		{
			ghPluginModule = (HMODULE)hModule;
			ghWorkingModule = (u64)hModule;
			gnSelfPID = GetCurrentProcessId();
			HeapInitialize();
			_ASSERTE(FAR_X_VER<FAR_Y1_VER && FAR_Y1_VER<FAR_Y2_VER);
#ifdef SHOW_STARTED_MSGBOX

			if (!IsDebuggerPresent())
				MessageBoxA(NULL, "ConEmu*.dll loaded", "ConEmu plugin", 0);

#endif
			//#if defined(__GNUC__)
			//GetConsoleWindow = (FGetConsoleWindow)GetProcAddress(GetModuleHandle(L"kernel32.dll"),"GetConsoleWindow");
			//#endif
			gpLocalSecurity = LocalSecurity();
			csTabs = new MSection();
			csData = new MSection();
			gPanelDirs.ActiveDir = new CmdArg();
			gPanelDirs.PassiveDir = new CmdArg();
			PlugServerInit();
			//HWND hConWnd = GetConEmuHWND(2);
			// Текущая нить не обязана быть главной! Поэтому ищем первую нить процесса!
			gnMainThreadId = gnMainThreadIdInitial = GetMainThreadId();
			CPluginBase::InitHWND(/*hConWnd*/);
			//TODO("перенести инициализацию фаровских callback'ов в SetStartupInfo, т.к. будет грузиться как Inject!");
			//if (!StartupHooks(ghPluginModule)) {
			//	_ASSERTE(FALSE);
			//	DEBUGSTR(L"!!! Can't install injects!!!\n");
			//}
			// Check Terminal mode
			TerminalMode = isTerminalMode();
			//TCHAR szVarValue[MAX_PATH];
			//szVarValue[0] = 0;
			//if (GetEnvironmentVariable(L"TERM", szVarValue, 63)) {
			//    TerminalMode = TRUE;
			//}
			//2010-01-29 ConMan давно не поддерживается - все встроено
			//if (!TerminalMode) {
			//	// FarHints fix for multiconsole mode...
			//	if (GetModuleFileName((HMODULE)hModule, szVarValue, MAX_PATH)) {
			//		WCHAR *pszSlash = wcsrchr(szVarValue, L'\\');
			//		if (pszSlash) pszSlash++; else pszSlash = szVarValue;
			//		lstrcpyW(pszSlash, L"infis.dll");
			//		ghFarHintsFix = LoadLibrary(szVarValue);
			//	}
			//}

			if (!TerminalMode)
			{
				if (!StartupHooks(ghPluginModule))
				{
					if (ghConEmuWndDC)
					{
						_ASSERTE(FALSE);
						DEBUGSTR(L"!!! Can't install injects!!!\n");
					}
					else
					{
						DEBUGSTR(L"No GUI, injects was not installed!\n");
					}
				}
			}
		}
		break;
		case DLL_PROCESS_DETACH:
			CPluginBase::ShutdownPluginStep(L"DLL_PROCESS_DETACH");

			if (!gbExitFarCalled)
			{
				_ASSERTE(FALSE && "ExitFar was not called. Unsupported Far<->Plugin builds?");
				Plugin()->ExitFarCommon();
			}

			if (gnSynchroCount > 0)
			{
				//if (gFarVersion.dwVerMajor == 2 && gFarVersion.dwBuild < 1735) -- в фаре пока не чинили, поэтому всегда ругаемся, если что...
				BOOL lbSynchroSafe = FALSE;
				if ((gFarVersion.dwVerMajor == 2 && gFarVersion.dwVerMinor >= 1) || (gFarVersion.dwVerMajor >= 3))
					lbSynchroSafe = TRUE;
				if (!lbSynchroSafe)
				{
					MessageBox(NULL, L"Syncho events are pending!\nFar may crash after unloading plugin", L"ConEmu plugin", MB_OK|MB_ICONEXCLAMATION|MB_SETFOREGROUND|MB_SYSTEMMODAL);
				}
			}

			//if (ghFarHintsFix) {
			//	FreeLibrary(ghFarHintsFix);
			//	ghFarHintsFix = NULL;
			//}
			if (csTabs)
			{
				delete csTabs;
				csTabs = NULL;
			}

			if (csData)
			{
				delete csData;
				csData = NULL;
			}

			PlugServerStop(true);

			if (gpBgPlugin)
			{
				delete gpBgPlugin;
				gpBgPlugin = NULL;
			}

			HeapDeinitialize();

			CPluginBase::ShutdownPluginStep(L"DLL_PROCESS_DETACH - done");
			break;
	}

	return TRUE;
}

#if defined(CRTSTARTUP)
#pragma message("!!!CRTSTARTUP defined!!!")
extern "C" {
	BOOL WINAPI _DllMainCRTStartup(HANDLE hDll,DWORD dwReason,LPVOID lpReserved);
};

BOOL WINAPI _DllMainCRTStartup(HANDLE hDll,DWORD dwReason,LPVOID lpReserved)
{
	DllMain(hDll, dwReason, lpReserved);
	return TRUE;
}
#endif


BOOL WINAPI IsConsoleActive()
{
	if (ghConEmuWndDC)
	{
		if (IsWindow(ghConEmuWndDC))
		{
			HWND hParent = GetParent(ghConEmuWndDC);

			if (hParent)
			{
				HWND hTest = (HWND)GetWindowLongPtr(hParent, GWLP_USERDATA);
				return (hTest == FarHwnd);
			}
		}
	}

	return TRUE;
}

// anConEmuOnly
//	0 - если в ConEmu - вернуть окно отрисовки, иначе - вернуть окно консоли
//	1 - вернуть окно отрисовки
//	2 - вернуть главное окно ConEmu
//	3 - вернуть окно консоли
HWND WINAPI GetFarHWND2(int anConEmuOnly)
{
	// Если просили реальное окно консоли - вернем сразу
	if (anConEmuOnly == 3)
	{
		return FarHwnd;
	}

	if (ghConEmuWndDC)
	{
		if (IsWindow(ghConEmuWndDC))
		{
			if (anConEmuOnly == 2)
				return GetConEmuHWND(1);
			return ghConEmuWndDC;
		}

		//
		ghConEmuWndDC = NULL;
		//
		SetConEmuEnvVar(NULL);
		SetConEmuEnvVarChild(NULL,NULL);
	}

	if (anConEmuOnly)
		return NULL;

	return FarHwnd;
}

HWND WINAPI GetFarHWND()
{
	return GetFarHWND2(FALSE);
}

BOOL WINAPI IsTerminalMode()
{
	return TerminalMode;
}

void WINAPI GetFarVersion(FarVersion* pfv)
{
	if (!pfv)
		return;

	*pfv = gFarVersion;
}

int WINAPI RegisterPanelView(PanelViewInit *ppvi)
{
	if (!ppvi)
	{
		_ASSERTE(ppvi->cbSize == sizeof(PanelViewInit));
		return -2;
	}

	if (ppvi->cbSize != sizeof(PanelViewInit))
	{
		_ASSERTE(ppvi->cbSize == sizeof(PanelViewInit));
		return -2;
	}

	PanelViewRegInfo *pp = (ppvi->bLeftPanel) ? &gPanelRegLeft : &gPanelRegRight;

	if (ppvi->bRegister)
	{
		pp->pfnPeekPreCall = ppvi->pfnPeekPreCall.f;
		pp->pfnPeekPostCall = ppvi->pfnPeekPostCall.f;
		pp->pfnReadPreCall = ppvi->pfnReadPreCall.f;
		pp->pfnReadPostCall = ppvi->pfnReadPostCall.f;
		pp->pfnWriteCall = ppvi->pfnWriteCall.f;
	}
	else
	{
		pp->pfnPeekPreCall = pp->pfnPeekPostCall = pp->pfnReadPreCall = pp->pfnReadPostCall = NULL;
		pp->pfnWriteCall = NULL;
	}

	pp->bRegister = ppvi->bRegister;
	CESERVER_REQ In;
	int nSize = sizeof(CESERVER_REQ_HDR) + sizeof(In.PVI);
	ExecutePrepareCmd(&In, CECMD_REGPANELVIEW, nSize);
	In.PVI = *ppvi;
	CESERVER_REQ* pOut = ExecuteGuiCmd(FarHwnd, &In, FarHwnd);

	if (!pOut)
	{
		pp->pfnPeekPreCall = pp->pfnPeekPostCall = pp->pfnReadPreCall = pp->pfnReadPostCall = NULL;
		pp->pfnWriteCall = NULL;
		pp->bRegister = FALSE;
		return -3;
	}

	*ppvi = pOut->PVI;
	ExecuteFreeResult(pOut);

	if (ppvi->cbSize == 0)
	{
		pp->pfnPeekPreCall = pp->pfnPeekPostCall = pp->pfnReadPreCall = pp->pfnReadPostCall = NULL;
		pp->pfnWriteCall = NULL;
		pp->bRegister = FALSE;
		return -1;
	}

	return 0;
}



//struct RegisterBackgroundArg gpBgPlugin = NULL;
//int gnBgPluginsCount = 0, gnBgPluginsMax = 0;
//MSection *csBgPlugins = NULL;

int WINAPI RegisterBackground(RegisterBackgroundArg *pbk)
{
	if (!pbk)
	{
		_ASSERTE(pbk != NULL);
		return esbr_InvalidArg;
	}

	if (!gbBgPluginsAllowed)
	{
		_ASSERTE(gbBgPluginsAllowed == TRUE);
		return esbr_PluginForbidden;
	}

	if (pbk->cbSize != sizeof(*pbk))
	{
		_ASSERTE(pbk->cbSize == sizeof(*pbk));
		return esbr_InvalidArgSize;
	}

#ifdef _DEBUG

	if (pbk->Cmd == rbc_Register)
	{
		_ASSERTE(pbk->dwPlaces != 0);
	}

#endif

	if (gpBgPlugin == NULL)
	{
		gpBgPlugin = new CPluginBackground;
	}

	return gpBgPlugin->RegisterSubplugin(pbk);
}

// export
// Возвращает TRUE в случае успешного выполнения
// (удалось активировать главную нить и выполнить в ней функцию CallBack)
// FALSE - в случае ошибки.
int WINAPI SyncExecute(HMODULE ahModule, SyncExecuteCallback_t CallBack, LONG_PTR lParam)
{
	BOOL bResult = FALSE;
	SyncExecuteArg args = {CMD__EXTERNAL_CALLBACK, ahModule, CallBack, lParam};
	bResult = ProcessCommand(CMD__EXTERNAL_CALLBACK, TRUE/*bReqMainThread*/, &args);
	return bResult;
}

// export
// Активировать текущую консоль в ConEmu
int WINAPI ActivateConsole()
{
	CESERVER_REQ In;
	int nSize = sizeof(CESERVER_REQ_HDR) + sizeof(In.ActivateCon);
	ExecutePrepareCmd(&In, CECMD_ACTIVATECON, nSize);
	In.ActivateCon.hConWnd = FarHwnd;
	CESERVER_REQ* pOut = ExecuteGuiCmd(FarHwnd, &In, FarHwnd);

	if (!pOut)
	{
		return FALSE;
	}

	BOOL lbSucceeded = (pOut->ActivateCon.hConWnd == FarHwnd);
	ExecuteFreeResult(pOut);
	return lbSucceeded;
}

static BOOL gbTryOpenMapHeader = FALSE;
static BOOL gbStartupHooksAfterMap = FALSE;
int OpenMapHeader();
void CloseMapHeader();

BOOL gbWasDetached = FALSE;
CONSOLE_SCREEN_BUFFER_INFO gsbiDetached;

//#ifndef max
//#define max(a,b)            (((a) > (b)) ? (a) : (b))
//#endif
//
//#ifndef min
//#define min(a,b)            (((a) < (b)) ? (a) : (b))
//#endif

void WINAPI SetStartupInfo(void *aInfo)
{
	if (gFarVersion.dwVerMajor != 1)
	{
		gFarVersion.dwVerMajor = 1;
		gFarVersion.dwVerMinor = 75;
	}

	Plugin()->SetStartupInfo(aInfo);

	Plugin()->CommonPluginStartup();
}

void WINAPI SetStartupInfoW(void *aInfo)
{
	#ifdef _DEBUG
	HMODULE h = LoadLibrary (L"Kernel32.dll");
	FreeLibrary(h);
	#endif

	Plugin()->SetStartupInfo(aInfo);

	Plugin()->CommonPluginStartup();
}

#define MIN_FAR2_BUILD 1765 // svs 19.12.2010 22:52:53 +0300 - build 1765: Новая команда в FARMACROCOMMAND - MCMD_GETAREA
#define MAKEFARVERSION(major,minor,build) ( ((major)<<8) | (minor) | ((build)<<16))

int WINAPI GetMinFarVersion()
{
	// Однако, FAR2 до сборки 748 не понимал две версии плагина в одном файле
	bool bFar2 = false;

	if (!LoadFarVersion())
		bFar2 = true;
	else
		bFar2 = gFarVersion.dwVerMajor>=2;

	if (bFar2)
	{
		return MAKEFARVERSION(2,0,MIN_FAR2_BUILD);
	}

	return MAKEFARVERSION(1,71,2470);
}

int WINAPI GetMinFarVersionW()
{
	return MAKEFARVERSION(2,0,MIN_FAR2_BUILD);
}

int WINAPI ProcessSynchroEventW(int Event, void *Param)
{
	return Plugin()->ProcessSynchroEvent(Event, Param);
}

INT_PTR WINAPI ProcessSynchroEventW3(void* p)
{
	return Plugin()->ProcessSynchroEvent(p);
}

int WINAPI ProcessEditorEvent(int Event, void *Param)
{
	return Plugin()->ProcessEditorViewerEvent(Event, -1);
}

int WINAPI ProcessEditorEventW(int Event, void *Param)
{
	return Plugin()->ProcessEditorViewerEvent(Event, -1);
}

INT_PTR WINAPI ProcessEditorEventW3(void* p)
{
	return Plugin()->ProcessEditorEvent(p);
}

int WINAPI ProcessViewerEvent(int Event, void *Param)
{
	return Plugin()->ProcessEditorViewerEvent(-1, Event);
}

int WINAPI ProcessViewerEventW(int Event, void *Param)
{
	return Plugin()->ProcessEditorViewerEvent(-1, Event);
}

INT_PTR WINAPI ProcessViewerEventW3(void* p)
{
	return Plugin()->ProcessViewerEvent(p);
}


// watch non-modified -> modified editor status change

//int lastModifiedStateW = -1;
//bool gbHandleOneRedraw = false; //, gbHandleOneRedrawCh = false;

// watch non-modified -> modified editor status change
int WINAPI ProcessEditorInput(const INPUT_RECORD *Rec)
{
	// Даже если мы не под эмулятором - просто запомним текущее состояние
	//if (!ghConEmuWndDC) return 0; // Если мы не под эмулятором - ничего
	return Plugin()->ProcessEditorInput(*Rec);
}

int WINAPI ProcessEditorInputW(void* Rec)
{
	// Даже если мы не под эмулятором - просто запомним текущее состояние
	//if (!ghConEmuWndDC) return 0; // Если мы не под эмулятором - ничего
	return Plugin()->ProcessEditorInput((LPCVOID)Rec);
}

HANDLE WINAPI OpenPlugin(int OpenFrom,INT_PTR Item)
{
	return Plugin()->OpenPluginCommon(OpenFrom, Item, false);
}

HANDLE WINAPI OpenPluginW(int OpenFrom, INT_PTR Item)
{
	return Plugin()->OpenPluginCommon(OpenFrom, Item, ((OpenFrom & p->of_FromMacro) == p->of_FromMacro));
}

HANDLE WINAPI OpenW(const void* Info)
{
	return Plugin()->Open(Info);
}

void WINAPI ExitFAR(void)
{
	CPluginBase::ShutdownPluginStep(L"ExitFAR");

	Plugin()->ExitFarCommon();
	Plugin()->ExitFAR();

	CPluginBase::ShutdownPluginStep(L"ExitFAR - done");
}

void WINAPI ExitFARW(void)
{
	CPluginBase::ShutdownPluginStep(L"ExitFARW");

	Plugin()->ExitFarCommon();
	Plugin()->ExitFAR();

	CPluginBase::ShutdownPluginStep(L"ExitFARW - done");
}

void WINAPI ExitFARW3(void*)
{
	CPluginBase::ShutdownPluginStep(L"ExitFARW3");

	Plugin()->ExitFarCommon();
	Plugin()->ExitFAR();

	CPluginBase::ShutdownPluginStep(L"ExitFARW3 - done");
}



/* Используются как extern в ConEmuCheck.cpp */
LPVOID _calloc(size_t nCount,size_t nSize)
{
	return calloc(nCount,nSize);
}
LPVOID _malloc(size_t nCount)
{
	return malloc(nCount);
}
void   _free(LPVOID ptr)
{
	free(ptr);
}
