
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


//#include <stdio.h>
#include <windows.h>
//#include <windowsx.h>
//#include <string.h>
//#include <tchar.h>
#include "..\common\common.hpp"
#include "..\common\SetHook.h"
#include "..\common\pluginW1007.hpp"
#include "..\common\ConsoleAnnotation.h"
#include "..\common\WinObjects.h"
#include "..\common\TerminalMode.h"
#include "..\ConEmu\version.h"
#include "PluginHeader.h"
#include "PluginBackground.h"
#include <Tlhelp32.h>
#include <Dbghelp.h>
//#include <vector>

//WARNING("Перевести ghCommandThreads с vector<> на обычный класс. Это позволит собрать с минимальным размером файла");

#include "../common/ConEmuCheck.h"

#define Free free
#define Alloc calloc

#define MAKEFARVERSION(major,minor,build) ( ((major)<<8) | (minor) | ((build)<<16))

#define ConEmu_SysID 0x43454D55 // 'CEMU'
#define SETWND_CALLPLUGIN_SENDTABS 100
#define SETWND_CALLPLUGIN_BASE (SETWND_CALLPLUGIN_SENDTABS+1)
#define CHECK_RESOURCES_INTERVAL 5000
#define CHECK_FARINFO_INTERVAL 2000

#define CMD__EXTERNAL_CALLBACK 0x80001
//typedef void (WINAPI* SyncExecuteCallback_t)(LONG_PTR lParam);
struct SyncExecuteArg
{
	DWORD nCmd;
	HMODULE hModule;
	SyncExecuteCallback_t CallBack;
	LONG_PTR lParam;
};

#ifdef _DEBUG
wchar_t gszDbgModLabel[6] = {0};
#endif

#if defined(__GNUC__)
extern "C"{
  BOOL WINAPI DllMain( HANDLE hModule, DWORD  ul_reason_for_call, LPVOID lpReserved );
  HWND WINAPI GetFarHWND();
  HWND WINAPI GetFarHWND2(BOOL abConEmuOnly);
  void WINAPI GetFarVersion ( FarVersion* pfv );
  int  WINAPI ProcessEditorInputW(void* Rec);
  void WINAPI SetStartupInfoW(void *aInfo);
  int  WINAPI ProcessSynchroEventW(int Event,void *Param);
  BOOL WINAPI IsTerminalMode();
  BOOL WINAPI IsConsoleActive();
  int  WINAPI RegisterPanelView(PanelViewInit *ppvi);
  int  WINAPI RegisterBackground(RegisterBackgroundArg *pbk);
  int  WINAPI ActivateConsole();
  int  WINAPI SyncExecute(HMODULE ahModule, SyncExecuteCallback_t CallBack, LONG_PTR lParam);
};
#endif


HMODULE ghPluginModule = NULL; // ConEmu.dll - сам плагин
HWND ConEmuHwnd = NULL; // Содержит хэндл окна отрисовки. Это ДОЧЕРНЕЕ окно.
DWORD gdwServerPID = 0;
BOOL TerminalMode = FALSE;
HWND FarHwnd = NULL;
//WARNING("Убрать, заменить ghConIn на GetStdHandle()"); // Иначе в Win7 будет буфер разрушаться
//HANDLE ghConIn = NULL;
DWORD gnMainThreadId = 0;
//HANDLE hEventCmd[MAXCMDCOUNT], hEventAlive=NULL, hEventReady=NULL;
HANDLE ghMonitorThread = NULL; DWORD gnMonitorThreadId = 0;
//HANDLE ghInputThread = NULL; DWORD gnInputThreadId = 0;
HANDLE ghSetWndSendTabsEvent = NULL;
FarVersion gFarVersion = {0};
WCHAR gszDir1[CONEMUTABMAX], gszDir2[CONEMUTABMAX];
WCHAR gszRootKey[MAX_PATH*2]; // НЕ ВКЛЮЧАЯ "\\Plugins"
int maxTabCount = 0, lastWindowCount = 0, gnCurTabCount = 0;
CESERVER_REQ* gpTabs = NULL; //(ConEmuTab*) Alloc(maxTabCount, sizeof(ConEmuTab));
BOOL gbIgnoreUpdateTabs = FALSE; // выставляется на время CMD_SETWINDOW
BOOL gbRequestUpdateTabs = FALSE; // выставляется при получении события FOCUS/KILLFOCUS
BOOL gbClosingModalViewerEditor = FALSE; // выставляется при закрытии модального редактора/вьювера


//CRITICAL_SECTION csData;
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
//WCHAR gcPlugKey=0;
BOOL  gbPlugKeyChanged=FALSE;
HKEY  ghRegMonitorKey=NULL; HANDLE ghRegMonitorEvt=NULL;
//HMODULE ghFarHintsFix = NULL;
WCHAR gszPluginServerPipe[MAX_PATH];
#define MAX_SERVER_THREADS 3
//HANDLE ghServerThreads[MAX_SERVER_THREADS] = {NULL,NULL,NULL};
//HANDLE ghActiveServerThread = NULL;
HANDLE ghPlugServerThread = NULL;
DWORD  gnPlugServerThreadId = 0;
//DWORD  gnServerThreadsId[MAX_SERVER_THREADS] = {0,0,0};
HANDLE ghServerTerminateEvent = NULL;
HANDLE ghPluginSemaphore = NULL;
wchar_t gsFarLang[64] = {0};
BOOL FindServerCmd(DWORD nServerCmd, DWORD &dwServerPID);
BOOL gbNeedPostTabSend = FALSE;
BOOL gbNeedPostEditCheck = FALSE; // проверить, может в активном редакторе изменился статус
//BOOL gbNeedBgUpdate = FALSE; // требуется перерисовка Background
int lastModifiedStateW = -1;
BOOL gbNeedPostReloadFarInfo = FALSE;
DWORD gnNeedPostTabSendTick = 0;
#define NEEDPOSTTABSENDDELTA 100
wchar_t gsMonitorEnvVar[0x1000];
bool gbMonitorEnvVar = false;
#define MONITORENVVARDELTA 1000
void UpdateEnvVar(const wchar_t* pszList);
BOOL StartupHooks();
BOOL gbFARuseASCIIsort = FALSE; // попытаться перехватить строковую сортировку в FAR
//HANDLE ghFileMapping = NULL;
HANDLE ghColorMapping = NULL; // Создается при детаче консоли сразу после AllocConsole
BOOL gbHasColorMapping = FALSE; // Чтобы знать, что буфер True-Colorer создан
//#ifdef TRUE_COLORER_OLD_SUPPORT
//HANDLE ghColorMappingOld = NULL;
//BOOL gbHasColorMappingOld = FALSE;
//#endif
MFileMapping<CESERVER_REQ_CONINFO_HDR> *gpConMap;
const CESERVER_REQ_CONINFO_HDR *gpConMapInfo = NULL;
//AnnotationInfo *gpColorerInfo = NULL;
BOOL gbStartedUnderConsole2 = FALSE;
void CheckColorerHeader();
int CreateColorerHeader();
void CloseColorerHeader();
BOOL ReloadFarInfo(BOOL abForce);
DWORD gnSelfPID = 0; //GetCurrentProcessId();
//BOOL  gbNeedReloadFarInfo = FALSE;
HANDLE ghFarInfoMapping = NULL;
CEFAR_INFO *gpFarInfo = NULL, *gpFarInfoMapping = NULL;
HANDLE ghFarAliveEvent = NULL;
PanelViewRegInfo gPanelRegLeft = {NULL};
PanelViewRegInfo gPanelRegRight = {NULL};
// Для плагинов PicView & MMView нужно знать, нажат ли CtrlShift при F3
HANDLE ghConEmuCtrlPressed = NULL, ghConEmuShiftPressed = NULL;
BOOL gbWaitConsoleInputEmpty = FALSE, gbWaitConsoleWrite = FALSE; //, gbWaitConsoleInputPeek = FALSE;
HANDLE ghConsoleInputEmpty = NULL, ghConsoleWrite = NULL; //, ghConsoleInputWasPeek = NULL;
// SEE_MASK_NOZONECHECKS
BOOL gbShellNoZoneCheck = FALSE;
DWORD GetMainThreadId();
wchar_t gsLogCreateProcess[MAX_PATH+1] = {0};
int gnSynchroCount = 0;
bool gbSynchroProhibited = false;
bool gbInputSynchroPending = false;
void SetConsoleFontSizeTo(HWND inConWnd, int inSizeY, int inSizeX, const wchar_t *asFontName);


//std::vector<HANDLE> ghCommandThreads;
class CommandThreads
{
public:
	HANDLE h[20];
public:
	CommandThreads()
	{
		memset(h, 0, sizeof(h));
	};
	~CommandThreads()
	{
	};
	void CheckTerminated()
	{
		// Проверить, может какие-то командные нити уже завершились
		for (size_t i = 0; i < sizeof(h)/sizeof(h[0]); i++)
		{
			if (h[i] == NULL) continue;
			if (WaitForSingleObject(h[i], 0) == WAIT_OBJECT_0)
			{
				CloseHandle ( h[i] );
				h[i] = NULL;
			}
		}
	};
	void push_back(HANDLE hThread)
	{
		for (size_t i = 0; i < sizeof(h)/sizeof(h[0]); i++)
		{
			if (h[i] == NULL)
			{
				h[i] = hThread;
				return;
			}
		}
		_ASSERT(FALSE);
	};
	void KillAllThreads()
	{
		// Прибивание всех запущенных нитей
		for (size_t i = 0; i < sizeof(h)/sizeof(h[0]); i++)
		{
			if (h[i] == NULL) continue;
			if (WaitForSingleObject(h[i], 100) != WAIT_OBJECT_0)
			{
				TerminateThread(h[i], 100);
			}
			CloseHandle ( h[i] );
			h[i] = NULL;
		}
	};
};

CommandThreads *ghCommandThreads;


// minimal(?) FAR version 2.0 alpha build FAR_X_VER
int WINAPI _export GetMinFarVersionW(void)
{
	return MAKEFARVERSION(2,0,FAR_X_VER);
}

/* COMMON - пока структуры не различаются */
void WINAPI _export GetPluginInfoW(struct PluginInfo *pi)
{
    static WCHAR *szMenu[1], szMenu1[255];
    szMenu[0]=szMenu1; //lstrcpyW(szMenu[0], L"[&\x2560] ConEmu"); -> 0x2584
    //szMenu[0][1] = L'&';
    //szMenu[0][2] = 0x2560;

	// Проверить, не изменилась ли горячая клавиша плагина, и если да - пересоздать макросы
	//IsKeyChanged(TRUE); -- в FAR2 устарело, используем Synchro
	//if (gcPlugKey) szMenu1[0]=0; else lstrcpyW(szMenu1, L"[&\x2584] ");
	//lstrcpynW(szMenu1+lstrlenW(szMenu1), GetMsgW(2), 240);
	lstrcpynW(szMenu1, GetMsgW(CEPluginName), 240);


	pi->StructSize = sizeof(struct PluginInfo);
	pi->Flags = PF_EDITOR | PF_VIEWER | PF_DIALOG | PF_PRELOAD;
	pi->DiskMenuStrings = NULL;
	pi->DiskMenuNumbers = 0;
	pi->PluginMenuStrings = szMenu;
	pi->PluginMenuStringsNumber = 1;
	pi->PluginConfigStrings = NULL;
	pi->PluginConfigStringsNumber = 0;
	pi->CommandPrefix = L"ConEmu";
	pi->Reserved = ConEmu_SysID; // 'CEMU'
}

void CheckConEmuDetached()
{
	if (ConEmuHwnd)
	{
		// ConEmu могло подцепиться
		MFileMapping<CESERVER_REQ_CONINFO_HDR> ConMap;
		ConMap.InitName(CECONMAPNAME, (DWORD)FarHwnd);
		if (ConMap.Open())
		{
			if (ConMap.Ptr()->hConEmuWnd == NULL)
			{
				ConEmuHwnd = NULL;
			}
			ConMap.CloseMap();
		}
		else
		{
			ConEmuHwnd = NULL;
		}
	}
}

BOOL gbInfoW_OK = FALSE;
HANDLE WINAPI _export OpenPluginW(int OpenFrom,INT_PTR Item)
{
	if (!gbInfoW_OK)
		return INVALID_HANDLE_VALUE;

	if (OpenFrom == OPEN_COMMANDLINE && Item)
	{
		if (gFarVersion.dwBuild>=FAR_Y_VER)
			FUNC_Y(ProcessCommandLine)((wchar_t*)Item);
		else
			FUNC_X(ProcessCommandLine)((wchar_t*)Item);
		return INVALID_HANDLE_VALUE;
	}

	if (gnReqCommand != (DWORD)-1)
	{
		gnPluginOpenFrom = (OpenFrom && 0xFFFF);
		ProcessCommand(gnReqCommand, FALSE/*bReqMainThread*/, gpReqCommandData);
	}
	else
	{
		//if (!gbCmdCallObsolete) {
			INT_PTR nID = -1; // выбор из меню
			if ((OpenFrom & OPEN_FROMMACRO) == OPEN_FROMMACRO)
			{
				if (Item >= 1 && Item <= 8)
				{
					nID = Item - 1; // Будет сразу выполнена команда
					
				}
				else if (Item >= SETWND_CALLPLUGIN_BASE)
				{
					gnPluginOpenFrom = OPEN_PLUGINSMENU;
					DWORD nTab = (DWORD)(Item - SETWND_CALLPLUGIN_BASE);
					ProcessCommand(CMD_SETWINDOW, FALSE, &nTab);
					SetEvent(ghSetWndSendTabsEvent);
					return INVALID_HANDLE_VALUE;
					
				}
				else if (Item == SETWND_CALLPLUGIN_SENDTABS)
				{
					// Force Send tabs to ConEmu
					//MSectionLock SC; SC.Lock(csTabs, TRUE);
					//SendTabs(gnCurTabCount, TRUE);
					//SC.Unlock();
					UpdateConEmuTabs(0,false,false);
					SetEvent(ghSetWndSendTabsEvent);
					return INVALID_HANDLE_VALUE;
				}
			}
			ShowPluginMenu((int)nID);
		//} else {
		//	gbCmdCallObsolete = FALSE;
		//}
	}
	return INVALID_HANDLE_VALUE;
}

void TouchReadPeekConsoleInputs(int Peek = -1)
{
#ifdef _DEBUG
	_ASSERTE(GetCurrentThreadId() == gnMainThreadId);
#endif
	if (!gpFarInfo || !gpFarInfoMapping || !gpConMapInfo) {
		_ASSERTE(gpFarInfo);
		return;
	}

	SetEvent(ghFarAliveEvent);
	//gpFarInfo->nFarReadIdx++;
	//gpFarInfoMapping->nFarReadIdx = gpFarInfo->nFarReadIdx;

#ifdef _DEBUG
	if (Peek == -1)
		return;
	if ((GetKeyState(VK_SCROLL)&1) == 0)
		return;
	static DWORD nLastTick;
	DWORD nCurTick = GetTickCount();
	DWORD nDelta = nCurTick - nLastTick;
	static CONSOLE_SCREEN_BUFFER_INFO sbi;
	HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
	if (nDelta > 1000) {
		GetConsoleScreenBufferInfo(hOut, &sbi);
		nCurTick = nCurTick;
	}
	static wchar_t Chars[] = L"-\\|/-\\|/";
	int nNextChar = 0;
	if (Peek) {
		static int nPeekChar = 0;
		nNextChar = nPeekChar++;
		if (nPeekChar >= 8) nPeekChar = 0;
	} else {
		static int nReadChar = 0;
		nNextChar = nReadChar++;
		if (nReadChar >= 8) nReadChar = 0;
	}
	CHAR_INFO chi;
	chi.Char.UnicodeChar = Chars[nNextChar];
	chi.Attributes = 15;
	COORD crBufSize = {1,1};
	COORD crBufCoord = {0,0};
	// Cell[0] лучше не трогать - GUI ориентируется на наличие "1" в этой ячейке при проверке активности фара
	SHORT nShift = (Peek?1:2);
	SMALL_RECT rc = {sbi.srWindow.Left+nShift,sbi.srWindow.Bottom,sbi.srWindow.Left+nShift,sbi.srWindow.Bottom};
	WriteConsoleOutputW(hOut, &chi, crBufSize, crBufCoord, &rc);
#endif
}

// Вызывается из ACTL_SYNCHRO для FAR2
// или при ConsoleReadInput(1) в FAR1
void OnMainThreadActivated()
{
	// Теоретически, в FAR2 мы сюда можем попасть и не из основной нити,
	// если таки будет переделана "thread-safe" активация.

	if (gbNeedPostEditCheck)
	{
		DWORD currentModifiedState = GetEditorModifiedState();
		if (lastModifiedStateW != (int)currentModifiedState)
		{
			lastModifiedStateW = (int)currentModifiedState;
			gbRequestUpdateTabs = TRUE;
		}
		// 100909 - не было
		gbNeedPostEditCheck = FALSE;
	}
	if (!gbRequestUpdateTabs && gbNeedPostTabSend)
	{
		if (!IsMacroActive())
		{
			gbRequestUpdateTabs = TRUE; gbNeedPostTabSend = FALSE;
		}
	}
	if (gbRequestUpdateTabs && !IsMacroActive())
	{
		gbRequestUpdateTabs = gbNeedPostTabSend = FALSE;
		UpdateConEmuTabs(0,false,false);
		if (gbClosingModalViewerEditor)
		{
			gbClosingModalViewerEditor = FALSE;
			gbRequestUpdateTabs = TRUE;
		}
	}


	// !!! Это только чисто в OnConsolePeekReadInput, т.к. FAR Api тут не используется
	//if (gpConMapInfo && gpFarInfo && gpFarInfoMapping)
	//	TouchReadPeekConsoleInputs(abPeek ? 1 : 0);

	if (gbNeedPostReloadFarInfo)
	{
		gbNeedPostReloadFarInfo = FALSE;
		ReloadFarInfo(FALSE);
	}


	// !!! Это только чисто в OnConsolePeekReadInput, т.к. FAR Api тут не используется
	//// В некоторых случаях (CMD_LEFTCLKSYNC,CMD_CLOSEQSEARCH,...) нужно дождаться, пока очередь опустеет
	//if (gbWaitConsoleInputEmpty)
	//{
	//	DWORD nTestEvents = 0;
	//	HANDLE h = GetStdHandle(STD_INPUT_HANDLE);
	//	if (GetNumberOfConsoleInputEvents(h, &nTestEvents))
	//	{
	//		if (nTestEvents == 0)
	//		{
	//			gbWaitConsoleInputEmpty = FALSE;
	//			SetEvent(ghConsoleInputEmpty);
	//		}
	//	}
	//}

	
	// Если был запрос на обновление Background
	if (gbNeedBgActivate) // выставляется в gpBgPlugin->SetForceCheck() или SetForceUpdate()
	{
		gbNeedBgActivate = FALSE;
		if (gpBgPlugin)
			gpBgPlugin->OnMainThreadActivated();
	}
	

	// Проверяем, надо ли "активировать" плагин?
	if (!gbReqCommandWaiting || gnReqCommand == (DWORD)-1)
		return; // активация в данный момент не требуется
	
	gbReqCommandWaiting = FALSE; // чтобы ожидающая нить случайно не удалила параметры, когда мы работаем

	TODO("Определить текущую область... (panel/editor/viewer/menu/...");
	gnPluginOpenFrom = 0;
	
	// заглушка для Ansi
	if (gnReqCommand == CMD_SETWINDOW && (gFarVersion.dwVerMajor==1))
	{
		gnPluginOpenFrom = OPEN_PLUGINSMENU;
	}

	//
	if ((gnReqCommand == CMD_SETWINDOW) && (gFarVersion.dwVerMajor==2))
	{
		// Необходимо быть в panel/editor/viewer
		wchar_t szMacro[255];
		DWORD nTabShift = SETWND_CALLPLUGIN_BASE + *((DWORD*)gpReqCommandData);
		
		// Если панели-редактор-вьювер - сменить окно. Иначе - отослать в GUI табы
		wsprintf(szMacro, L"$if (Search) Esc $end $if (Shell||Viewer||Editor) callplugin(0x%08X,%i) $else callplugin(0x%08X,%i) $end",
			ConEmu_SysID, nTabShift, ConEmu_SysID, SETWND_CALLPLUGIN_SENDTABS);
			
		gnReqCommand = -1;
		gpReqCommandData = NULL;
		PostMacro(szMacro);
		// Done
	}
	else
	{
		// Результата ожидает вызывающая нить, поэтому передаем параметр
		//CESERVER_REQ* pCmdRet = NULL;
		ProcessCommand(gnReqCommand, FALSE/*bReqMainThread*/, gpReqCommandData, &gpCmdRet);
		//// Но не освобождаем его (pCmdRet) - это сделает ожидающая нить
		//_ASSERTE(gpCmdRet == NULL);
		//gpCmdRet = pCmdRet;
	}

	// Мы закончили
	SetEvent(ghReqCommandEvent);
}

// Вызывается только в основной нити
// и ТОЛЬКО если фар считывает один (1) INPUT_RECORD
void OnConsolePeekReadInput(BOOL abPeek)
{
#ifdef _DEBUG
	_ASSERTE(GetCurrentThreadId() == gnMainThreadId);
#endif

	bool lbNeedSynchro = false;



	if (gpConMapInfo && gpFarInfo && gpFarInfoMapping)
		TouchReadPeekConsoleInputs(abPeek ? 1 : 0);

	if (/*gbNeedReloadFarInfo &&*/ abPeek == FALSE)
	{
		//gbNeedReloadFarInfo = FALSE;
		bool bNeedReload = false;
		if (gpConMapInfo)
		{
			DWORD nMapPID = gpConMapInfo->nFarPID;
			static DWORD dwLastTickCount = 0;
			if (nMapPID == 0 || nMapPID != gnSelfPID)
			{
				bNeedReload = true;
				dwLastTickCount = GetTickCount();
			}
			else
			{
				DWORD dwCurTick = GetTickCount();
				if ((dwCurTick - dwLastTickCount) >= CHECK_FARINFO_INTERVAL)
				{
					bNeedReload = true;
					dwLastTickCount = dwCurTick;
				}
			}
		}
		if (bNeedReload)
		{
			//ReloadFarInfo(FALSE);
			gbNeedPostReloadFarInfo = TRUE;
		}
	}


	if (gbNeedPostReloadFarInfo || gbNeedPostEditCheck || gbRequestUpdateTabs || gbNeedPostTabSend || gbNeedBgActivate)
	{
		lbNeedSynchro = true;
	}
	
	

	// В некоторых случаях (CMD_LEFTCLKSYNC,CMD_CLOSEQSEARCH,...) нужно дождаться, пока очередь опустеет
	if (gbWaitConsoleInputEmpty)
	{
		DWORD nTestEvents = 0;
		HANDLE h = GetStdHandle(STD_INPUT_HANDLE);
		if (GetNumberOfConsoleInputEvents(h, &nTestEvents))
		{
			if (nTestEvents == 0)
			{
				gbWaitConsoleInputEmpty = FALSE;
				SetEvent(ghConsoleInputEmpty);
			}
		}
	}

	if (IS_SYNCHRO_ALLOWED)
	{
		// Требуется дернуть Synchro, чтобы корректно активироваться
		if (lbNeedSynchro && !gbInputSynchroPending)
		{
			gbInputSynchroPending = true;
			ExecuteSynchro();
		}
	}
	else
	{
		// Для Far1 зовем сразу
		_ASSERTE(gFarVersion.dwVerMajor == 1);
		OnMainThreadActivated();
	}

	//// Проверяем, надо ли "активировать" плагин?
	//if (!gbReqCommandWaiting || gnReqCommand == (DWORD)-1)
	//	return; // активация в данный момент не требуется
	//// Если есть ACTL_SYNCHRO - работаем только через него
	//if (IS_SYNCHRO_ALLOWED)
	//	return;
	//
	//gbReqCommandWaiting = FALSE; // чтобы ожидающая нить случайно не удалила параметры, когда мы работаем
	//
	//TODO("Определить текущую область... (panel/editor/viewer/menu/...");
	//gnPluginOpenFrom = 0;
	//
	//// заглушка для Ansi
	//if (gnReqCommand == CMD_SETWINDOW && (gFarVersion.dwVerMajor==1))
	//{
	//	gnPluginOpenFrom = OPEN_PLUGINSMENU;
	//}
	//
	////
	//if ((gnReqCommand == CMD_SETWINDOW) && (gFarVersion.dwVerMajor==2)) {
	//	// Необходимо быть в panel/editor/viewer
	//	wchar_t szMacro[255];
	//	DWORD nTabShift = SETWND_CALLPLUGIN_BASE + *((DWORD*)gpReqCommandData);
	//	
	//	// Если панели-редактор-вьювер - сменить окно. Иначе - отослать в GUI табы
	//	wsprintf(szMacro, L"$if (Search) Esc $end $if (Shell||Viewer||Editor) callplugin(0x%08X,%i) $else callplugin(0x%08X,%i) $end",
	//		ConEmu_SysID, nTabShift, ConEmu_SysID, SETWND_CALLPLUGIN_SENDTABS);
	//		
	//	gnReqCommand = -1;
	//	gpReqCommandData = NULL;
	//	PostMacro(szMacro);
	//	// Done
	//} else {
	//	ProcessCommand(gnReqCommand, FALSE/*bReqMainThread*/, gpReqCommandData);
	//}
	//
	//// Мы закончили
	//SetEvent(ghReqCommandEvent);
	//
	//return; // продолжить
}

#ifdef _DEBUG
BOOL DebugGetKeyboardState(LPBYTE pKeyStates)
{
	short v = 0;
	BYTE b = 0;
	int nKeys[] = {VK_SHIFT,VK_LSHIFT,VK_RSHIFT,
		         VK_MENU,VK_LMENU,VK_RMENU,
				 VK_CONTROL,VK_LCONTROL,VK_RCONTROL,
				 VK_LWIN,VK_RWIN,
				 VK_CAPITAL,VK_NUMLOCK,VK_SCROLL};
	int nKeyCount = sizeof(nKeys)/sizeof(nKeys[0]);
	for (int i=0; i<nKeyCount; i++) {
		v = GetAsyncKeyState(nKeys[i]);
		b = v & 1;
		if ((v & 0x8000) == 0x8000)
			b |= 0x80;
		pKeyStates[nKeys[i]] = b;
	}
	return TRUE;
}

typedef BOOL (__stdcall *FGetConsoleKeyboardLayoutName)(wchar_t*);
FGetConsoleKeyboardLayoutName pfnGetConsoleKeyboardLayoutName = NULL;

DWORD DebugCheckKeyboardLayout()
{
	DWORD dwLayout = 0x04090409;

	if (!pfnGetConsoleKeyboardLayoutName)
		pfnGetConsoleKeyboardLayoutName = (FGetConsoleKeyboardLayoutName)GetProcAddress (GetModuleHandleW (L"kernel32.dll"), "GetConsoleKeyboardLayoutNameW");

    if (pfnGetConsoleKeyboardLayoutName)
	{
        wchar_t szCurKeybLayout[KL_NAMELENGTH+1];
        if (pfnGetConsoleKeyboardLayoutName(szCurKeybLayout))
		{
            wchar_t *pszEnd = szCurKeybLayout+8;
            dwLayout = wcstoul(szCurKeybLayout, &pszEnd, 16);
        }
    }
	return dwLayout;
}

void __INPUT_RECORD_Dump(INPUT_RECORD *rec, wchar_t* pszRecord);
void DebugInputPrint(INPUT_RECORD r)
{
	static wchar_t szDbg[1100]; szDbg[0] = 0;
	
	SYSTEMTIME st; GetLocalTime(&st);
	wsprintfW(szDbg, L"%02i:%02i:%02i ", st.wHour,st.wMinute,st.wSecond);
	__INPUT_RECORD_Dump(&r, szDbg+9);
	lstrcatW(szDbg, L"\n");

	//switch (r.EventType)
	//{
	//case FOCUS_EVENT:
	//	wsprintf(szDbg, L"--FOCUS_EVENT(%i)\n", (int)r.Event.FocusEvent.bSetFocus);
	//	break;
	//case MENU_EVENT:
	//	wsprintf(szDbg, L"--MENU_EVENT\n");
	//	break;
	//case MOUSE_EVENT: //wprintf(L"--MOUSE_EVENT\n");
	//	{
	//		SYSTEMTIME st; GetLocalTime(&st);
	//		wsprintf(szDbg, L"%i:%02i:%02i {%ix%i} BtnState:0x%08X, CtrlState:0x%08X, Flags:0x%08X\n",
	//			st.wHour, st.wMinute, st.wSecond,
	//			r.Event.MouseEvent.dwMousePosition.X, r.Event.MouseEvent.dwMousePosition.Y,
	//			r.Event.MouseEvent.dwButtonState, r.Event.MouseEvent.dwControlKeyState,
	//			r.Event.MouseEvent.dwEventFlags);
	//	}
	//	break;
	//case WINDOW_BUFFER_SIZE_EVENT:
	//	wsprintf(szDbg, L"--WINDOW_BUFFER_SIZE_EVENT\n");
	//	break;
	//case KEY_EVENT:
	//	{
	//		wchar_t szLocks[32]; szLocks[0] = 0;
	//		if (r.Event.KeyEvent.wVirtualKeyCode == VK_UP || r.Event.KeyEvent.wVirtualKeyCode == VK_DOWN) {
	//			if (1 & GetKeyState(VK_NUMLOCK))
	//				lstrcat(szLocks, L" <Num>");
	//			if (1 & GetKeyState(VK_CAPITAL))
	//				lstrcat(szLocks, L" <Cap>");
	//			if (1 & GetKeyState(VK_SCROLL))
	//				lstrcat(szLocks, L" <Scr>");
	//		}
	//		SYSTEMTIME st; GetLocalTime(&st);
	//		wsprintf(szDbg, L"%i:%02i:%02i '%c' %s count=%i, VK=%i, SC=%i, CH=\\x%X, State=0x%08x %s%s\n",
	//			st.wHour, st.wMinute, st.wSecond,
	//			(r.Event.KeyEvent.uChar.UnicodeChar > 0x100) ? L'?' :
	//			(r.Event.KeyEvent.uChar.UnicodeChar 
	//			 ? r.Event.KeyEvent.uChar.UnicodeChar : L' '),
	//			r.Event.KeyEvent.bKeyDown ? L"Down," : L"Up,  ",
	//			r.Event.KeyEvent.wRepeatCount,
	//			r.Event.KeyEvent.wVirtualKeyCode,
	//			r.Event.KeyEvent.wVirtualScanCode,
	//			r.Event.KeyEvent.uChar.UnicodeChar,
	//			r.Event.KeyEvent.dwControlKeyState,
	//			(r.Event.KeyEvent.dwControlKeyState & ENHANCED_KEY) ?
	//			L"<Enhanced>" : L"",
	//			szLocks);
	//		if (r.Event.KeyEvent.uChar.UnicodeChar) {
	//			BYTE KeyStates[256] = {0};
	//			wchar_t szBuff[10];
	//			HKL hkl = (HKL)DebugCheckKeyboardLayout();
	//			BOOL lbkRc = DebugGetKeyboardState(KeyStates);
	//			int nuRc = ToUnicodeEx(r.Event.KeyEvent.wVirtualKeyCode,
	//				r.Event.KeyEvent.wVirtualScanCode,
	//				KeyStates, szBuff, 10, 0, hkl);
	//			if (nuRc>0) szBuff[nuRc] = 0; else szBuff[0] = 0;
	//			wchar_t szTemp[256] = {0};
	//			for (int i=0; i<nuRc; i++)
	//				wsprintf(szTemp+wcslen(szTemp), L"\\x%04X", (WORD)szBuff[i]);
	//			wsprintf(szDbg+lstrlen(szDbg), L"         -- GKS=%i; TUE=%i; <%s>\n", lbkRc, nuRc, szTemp);
	//		}
	//	} break;
	//default:
	//	{
	//		wsprintf(szDbg, L"Unknown event type (%i)\n", r.EventType);
	//	}
	//}
	
	OutputDebugString(szDbg);
}
#endif





BOOL OnPanelViewCallbacks(HookCallbackArg* pArgs, PanelViewInputCallback pfnLeft, PanelViewInputCallback pfnRight)
{
	if (!pArgs->bMainThread || !(pfnLeft || pfnRight)) {
		_ASSERTE(pArgs->bMainThread && (pfnLeft || pfnRight));
		return TRUE; // перехват делаем только в основной нити
	}

	BOOL lbNewResult = FALSE, lbContinue = TRUE;
	HANDLE hInput = (HANDLE)(pArgs->lArguments[0]);
	PINPUT_RECORD p = (PINPUT_RECORD)(pArgs->lArguments[1]);
	DWORD nBufSize = (DWORD)(pArgs->lArguments[2]);
	LPDWORD pCount = (LPDWORD)(pArgs->lArguments[3]);
	
	if (lbContinue && pfnLeft) {
		_ASSERTE(gPanelRegLeft.bRegister);
		lbContinue = pfnLeft(hInput,p,nBufSize,pCount,&lbNewResult);
		if (!lbContinue)
			*((BOOL*)pArgs->lpResult) = lbNewResult;
	}
	// Если есть только правая панель, или на правой панели задана другая функция
	if (lbContinue && pfnRight && pfnRight != pfnLeft) {
		_ASSERTE(gPanelRegRight.bRegister);
		lbContinue = pfnRight(hInput,p,nBufSize,pCount,&lbNewResult);
		if (!lbContinue)
			*((BOOL*)pArgs->lpResult) = lbNewResult;
	}
	
	return lbContinue;
}


VOID WINAPI OnShellExecuteExW_Except(HookCallbackArg* pArgs)
{
	if (pArgs->bMainThread)
	{
		ShowMessage(CEShellExecuteException,1);
	}
	*((LPBOOL*)pArgs->lpResult) = FALSE;
	SetLastError(E_UNEXPECTED);
}


// Для определения "живости" фара
VOID WINAPI OnGetNumberOfConsoleInputEventsPost(HookCallbackArg* pArgs)
{
	if (pArgs->bMainThread && gpConMapInfo && gpFarInfo && gpFarInfoMapping) {
		TouchReadPeekConsoleInputs(-1);
	}
}

// Если функция возвращает FALSE - реальный ReadConsoleInput вызван не будет,
// и в вызывающую функцию (ФАРа?) вернется то, что проставлено в pArgs->lpResult & pArgs->lArguments[...]
BOOL WINAPI OnConsolePeekInput(HookCallbackArg* pArgs)
{
	if (!pArgs->bMainThread) return TRUE; // обработку делаем только в основной нити
	
	if (pArgs->lArguments[2] == 1)
	{
		OnConsolePeekReadInput(TRUE/*abPeek*/);
	}

	// Если зарегистрирован callback для графической панели
	if (gPanelRegLeft.pfnPeekPreCall || gPanelRegRight.pfnPeekPreCall) {
		// Если функция возвращает FALSE - реальное чтение не будет вызвано
		if (!OnPanelViewCallbacks(pArgs, gPanelRegLeft.pfnPeekPreCall, gPanelRegRight.pfnPeekPreCall))
			return FALSE;
	}
	
	return TRUE; // продолжить
}

VOID WINAPI OnConsolePeekInputPost(HookCallbackArg* pArgs)
{
	if (!pArgs->bMainThread) return; // обработку делаем только в основной нити

#ifdef _DEBUG
	if (*(LPDWORD)(pArgs->lArguments[3]))
	{
		wchar_t szDbg[255]; 
		PINPUT_RECORD p = (PINPUT_RECORD)(pArgs->lArguments[1]);
		LPDWORD pCount = (LPDWORD)(pArgs->lArguments[3]);
		DWORD nLeft = 0; GetNumberOfConsoleInputEvents(GetStdHandle(STD_INPUT_HANDLE), &nLeft);
		wsprintfW(szDbg, L"*** OnConsolePeekInputPost(Events=%i, KeyCount=%i, LeftInConBuffer=%i)\n", 
			*pCount, (p->EventType==KEY_EVENT) ? p->Event.KeyEvent.wRepeatCount : 0, nLeft);
		DEBUGSTRINPUT(szDbg);
		// Если под дебагом включен ScrollLock - вывести информацию о считанных событиях
		if (GetKeyState(VK_SCROLL) & 1)
		{
			PINPUT_RECORD p = (PINPUT_RECORD)(pArgs->lArguments[1]);
			LPDWORD pCount = (LPDWORD)(pArgs->lArguments[3]);
			_ASSERTE(*pCount <= pArgs->lArguments[2]);
			UINT nCount = *pCount;
			for (UINT i = 0; i < nCount; i++)
				DebugInputPrint(p[i]);
		}
	}
#endif

	// Если зарегистрирован callback для графической панели
	if (gPanelRegLeft.pfnPeekPostCall || gPanelRegRight.pfnPeekPostCall) {
		// Если функция возвращает FALSE - реальное чтение не будет вызвано
		if (!OnPanelViewCallbacks(pArgs, gPanelRegLeft.pfnPeekPostCall, gPanelRegRight.pfnPeekPostCall))
			return;
	}
}

// Если функция возвращает FALSE - реальный ReadConsoleInput вызван не будет,
// и в вызывающую функцию (ФАРа?) вернется то, что проставлено в pArgs->lpResult & pArgs->lArguments[...]
BOOL WINAPI OnConsoleReadInput(HookCallbackArg* pArgs)
{
	if (!pArgs->bMainThread) return TRUE; // обработку делаем только в основной нити
	
	if (pArgs->lArguments[2] == 1)
	{
		OnConsolePeekReadInput(FALSE/*abPeek*/);
	}

	// Если зарегистрирован callback для графической панели
	if (gPanelRegLeft.pfnReadPreCall || gPanelRegRight.pfnReadPreCall)
	{
		// Если функция возвращает FALSE - реальное чтение не будет вызвано
		if (!OnPanelViewCallbacks(pArgs, gPanelRegLeft.pfnReadPreCall, gPanelRegRight.pfnReadPreCall))
		{
			// это вызвается перед реальным чтением - информация может быть разве что от "PanelViews"
			// Если под дебагом включен ScrollLock - вывести информацию о считанных событиях
			#ifdef _DEBUG
			if (GetKeyState(VK_SCROLL) & 1)
			{
				PINPUT_RECORD p = (PINPUT_RECORD)(pArgs->lArguments[1]);
				LPDWORD pCount = (LPDWORD)(pArgs->lArguments[3]);
				_ASSERTE(*pCount <= pArgs->lArguments[2]);
				UINT nCount = *pCount;
				for (UINT i = 0; i < nCount; i++)
					DebugInputPrint(p[i]);
			}
			#endif
			return FALSE;
		}
	}
	
	return TRUE; // продолжить
}

VOID WINAPI OnConsoleReadInputPost(HookCallbackArg* pArgs)
{
	if (!pArgs->bMainThread) return; // обработку делаем только в основной нити

#ifdef _DEBUG
	{
		wchar_t szDbg[255]; 
		PINPUT_RECORD p = (PINPUT_RECORD)(pArgs->lArguments[1]);
		LPDWORD pCount = (LPDWORD)(pArgs->lArguments[3]);
		DWORD nLeft = 0; GetNumberOfConsoleInputEvents(GetStdHandle(STD_INPUT_HANDLE), &nLeft);
		wsprintfW(szDbg, L"*** OnConsoleReadInputPost(Events=%i, KeyCount=%i, LeftInConBuffer=%i)\n", 
			*pCount, (p->EventType==KEY_EVENT) ? p->Event.KeyEvent.wRepeatCount : 0, nLeft);
		//if (*pCount) {
		//	wsprintfW(szDbg+lstrlen(szDbg), L", type=%i", p->EventType);
		//	if (p->EventType == MOUSE_EVENT) {
		//		wsprintfW(L", {%ix%i} BtnState:0x%08X, CtrlState:0x%08X, Flags:0x%08X",
		//			p->Event.MouseEvent.dwMousePosition.X, p->Event.MouseEvent.dwMousePosition.Y,
		//			p->Event.MouseEvent.dwButtonState, p->Event.MouseEvent.dwControlKeyState,
		//			p->Event.MouseEvent.dwEventFlags);
		//	} else if (p->EventType == KEY_EVENT) {
		//		wsprintfW(L", '%c' %s count=%i, VK=%i, SC=%i, CH=\\x%X, State=0x%08x %s",
		//			(p->Event.KeyEvent.uChar.UnicodeChar > 0x100) ? L'?' :
		//			(p->Event.KeyEvent.uChar.UnicodeChar 
		//			? p->Event.KeyEvent.uChar.UnicodeChar : L' '),
		//			p->Event.KeyEvent.bKeyDown ? L"Down," : L"Up,  ",
		//			p->Event.KeyEvent.wRepeatCount,
		//			p->Event.KeyEvent.wVirtualKeyCode,
		//			p->Event.KeyEvent.wVirtualScanCode,
		//			p->Event.KeyEvent.uChar.UnicodeChar,
		//			p->Event.KeyEvent.dwControlKeyState,
		//			(p->Event.KeyEvent.dwControlKeyState & ENHANCED_KEY) ?
		//			L"<Enhanced>" : L"");
		//	}
		//}
		//lstrcatW(szDbg, L")\n");
		DEBUGSTRINPUT(szDbg);
		// Если под дебагом включен ScrollLock - вывести информацию о считанных событиях
		if (GetKeyState(VK_SCROLL) & 1) {
			PINPUT_RECORD p = (PINPUT_RECORD)(pArgs->lArguments[1]);
			LPDWORD pCount = (LPDWORD)(pArgs->lArguments[3]);
			_ASSERTE(*pCount <= pArgs->lArguments[2]);
			UINT nCount = *pCount;
			for (UINT i = 0; i < nCount; i++)
				DebugInputPrint(p[i]);
		}
	}
#endif

	// Если зарегистрирован callback для графической панели
	if (gPanelRegLeft.pfnReadPostCall || gPanelRegRight.pfnReadPostCall) {
		// Если функция возвращает FALSE - реальное чтение не будет вызвано
		if (!OnPanelViewCallbacks(pArgs, gPanelRegLeft.pfnReadPostCall, gPanelRegRight.pfnReadPostCall))
			return;
	}

	// Чтобы ФАР сразу прекратил ходить по каталогам при отпускании Enter
	{
		HANDLE h = (HANDLE)(pArgs->lArguments[0]);
		PINPUT_RECORD p = (PINPUT_RECORD)(pArgs->lArguments[1]);
		LPDWORD pCount = (LPDWORD)(pArgs->lArguments[3]);
		if (*pCount == 1 && p->EventType == KEY_EVENT && p->Event.KeyEvent.bKeyDown
			&& (p->Event.KeyEvent.wVirtualKeyCode == VK_RETURN
				|| p->Event.KeyEvent.wVirtualKeyCode == VK_NEXT
				|| p->Event.KeyEvent.wVirtualKeyCode == VK_PRIOR)
			)
		{
			INPUT_RECORD ir[10]; DWORD nRead = 0, nInc = 0;
			if (PeekConsoleInputW(h, ir, countof(ir), &nRead) && nRead) {
				for (DWORD n = 0; n < nRead; n++) {
					if (ir[n].EventType == KEY_EVENT && ir[n].Event.KeyEvent.bKeyDown
						&& ir[n].Event.KeyEvent.wVirtualKeyCode == p->Event.KeyEvent.wVirtualKeyCode
						&& ir[n].Event.KeyEvent.dwControlKeyState == p->Event.KeyEvent.dwControlKeyState)
					{
						nInc++;
					} else {
						break; // дубли в буфере кончились
					}
				}
				if (nInc > 0) {
					if (ReadConsoleInputW(h, ir, nInc, &nRead) && nRead) {
						p->Event.KeyEvent.wRepeatCount += (WORD)nRead;
					}
				}
			}
		}
	}
}


BOOL WINAPI OnWriteConsoleOutput(HookCallbackArg* pArgs)
{
	if (!pArgs->bMainThread)
		return TRUE; // обработку делаем только в основной нити

	// Если зарегистрирован callback для графической панели
	if (gPanelRegLeft.pfnWriteCall || gPanelRegRight.pfnWriteCall)
	{
		HANDLE hOutput = (HANDLE)(pArgs->lArguments[0]);
		const CHAR_INFO *lpBuffer = (const CHAR_INFO *)(pArgs->lArguments[1]);
		COORD dwBufferSize = *(COORD*)(pArgs->lArguments[2]);
		COORD dwBufferCoord = *(COORD*)(pArgs->lArguments[3]);
		PSMALL_RECT lpWriteRegion = (PSMALL_RECT)(pArgs->lArguments[4]);
		
		if (gPanelRegLeft.pfnWriteCall)
		{
			_ASSERTE(gPanelRegLeft.bRegister);
			gPanelRegLeft.pfnWriteCall(hOutput,lpBuffer,dwBufferSize,dwBufferCoord,lpWriteRegion);
		}
		// Если есть только правая панель, или на правой панели задана другая функция
		if (gPanelRegRight.pfnWriteCall && gPanelRegRight.pfnWriteCall != gPanelRegLeft.pfnWriteCall)
		{
			_ASSERTE(gPanelRegRight.bRegister);
			gPanelRegRight.pfnWriteCall(hOutput,lpBuffer,dwBufferSize,dwBufferCoord,lpWriteRegion);
		}
	}

	//if (gpBgPlugin)
	//	gpBgPlugin->SetForceCheck();

	if (gbWaitConsoleWrite)
	{
		gbWaitConsoleWrite = FALSE;
		SetEvent(ghConsoleWrite);
	}

	return TRUE;
}


int WINAPI _export ProcessSynchroEventW(int Event,void *Param)
{
	if (Event == SE_COMMONSYNCHRO)
	{
		if (gbInputSynchroPending)
			gbInputSynchroPending = false;

		#ifdef _DEBUG
		{
			static int nLastType = -1;
			int nCurType = GetActiveWindowType();
			if (nCurType != nLastType)
			{
				LPCWSTR pszCurType = NULL;
				switch (nCurType)
				{
					case WTYPE_PANELS: pszCurType = L"WTYPE_PANELS"; break;
					case WTYPE_VIEWER: pszCurType = L"WTYPE_VIEWER"; break;
					case WTYPE_EDITOR: pszCurType = L"WTYPE_EDITOR"; break;
					case WTYPE_DIALOG: pszCurType = L"WTYPE_DIALOG"; break;
					case WTYPE_VMENU:  pszCurType = L"WTYPE_VMENU"; break;
					case WTYPE_HELP:   pszCurType = L"WTYPE_HELP"; break;
					default:           pszCurType = L"Unknown";
				}
				LPCWSTR pszLastType = NULL;
				switch (nLastType)
				{
					case WTYPE_PANELS: pszLastType = L"WTYPE_PANELS"; break;
					case WTYPE_VIEWER: pszLastType = L"WTYPE_VIEWER"; break;
					case WTYPE_EDITOR: pszLastType = L"WTYPE_EDITOR"; break;
					case WTYPE_DIALOG: pszLastType = L"WTYPE_DIALOG"; break;
					case WTYPE_VMENU:  pszLastType = L"WTYPE_VMENU"; break;
					case WTYPE_HELP:   pszLastType = L"WTYPE_HELP"; break;
					default:           pszLastType = L"Undefined";
				}
				wchar_t szDbg[255];
				wsprintfW(szDbg, L"FarWindow: %s activated (was %s)\n", pszCurType, pszLastType);
				OutputDebugStringW(szDbg);
				nLastType = nCurType;
			}
		}
		#endif

		if (!gbSynchroProhibited)
		{
			OnMainThreadActivated();
		}

		if (gnSynchroCount > 0)
			gnSynchroCount--;

		if (gbSynchroProhibited && (gnSynchroCount == 0))
		{
			if (gFarVersion.dwBuild>=FAR_Y_VER)
				FUNC_Y(StopWaitEndSynchro)();
			//else
			//	FUNC_X(StopWaitEndSynchro)();
		}
	}
	return 0;
}
//    	if (!gbInfoW_OK) {
//    		if (Param) free(Param);
//    		return 0;
//		}
//
//		SynchroArg *pArg = (SynchroArg*)Param;
//		_ASSERTE(pArg!=NULL);
//
//		// Если предыдущая активация отвалилась по таймауту - не выполнять
//		if (pArg->Obsolete) {
//			free(pArg);
//			return 0;
//		}
//
//		if (pArg->SynchroType == SynchroArg::eCommand) {
//    		if (gnReqCommand != (DWORD)-1) {
//    			_ASSERTE(gnReqCommand==(DWORD)-1);
//    		} else {
//    			TODO("Определить текущую область... (panel/editor/viewer/menu/...");
//    			gnPluginOpenFrom = 0;
//    			gnReqCommand = (DWORD)pArg->Param1;
//				gpReqCommandData = (LPVOID)pArg->Param2;
//				
//				if (gnReqCommand == CMD_SETWINDOW) {
//					// Необходимо быть в panel/editor/viewer
//					wchar_t szMacro[255];
//					DWORD nTabShift = SETWND_CALLPLUGIN_BASE + *((DWORD*)gpReqCommandData);
//					
//					// Если панели-редактор-вьювер - сменить окно. Иначе - отослать в GUI табы
//					wsprintf(szMacro, L"$if (Shell||Viewer||Editor) callplugin(0x%08X,%i) $else callplugin(0x%08X,%i) $end",
//						ConEmu_SysID, nTabShift, ConEmu_SysID, SETWND_CALLPLUGIN_SENDTABS);
//						
//					gnReqCommand = -1;
//					gpReqCommandData = NULL;
//					PostMacro(szMacro);
//					// Done
//				} else {
//	    			ProcessCommand(gnReqCommand, FALSE/*bReqMainThread*/, gpReqCommandData);
//    			}
//    		}
//		} else if (pArg->SynchroType == SynchroArg::eInput) {
//			INPUT_RECORD *pRec = (INPUT_RECORD*)(pArg->Param1);
//			UINT nCount = (UINT)pArg->Param2;
//
//			if (nCount>0) {
//				DWORD cbWritten = 0;
//				_ASSERTE(ghConIn);
//				WARNING("Убрать, заменить ghConIn на GetStdHandle()"); // Иначе в Win7 будет буфер разрушаться
//				BOOL fSuccess = WriteConsoleInput(ghConIn, pRec, nCount, &cbWritten);
//				if (!fSuccess || cbWritten != nCount) {
//					_ASSERTE(fSuccess && cbWritten==nCount);
//				}
//			}
//		}
//
//		if (pArg->hEvent)
//			SetEvent(pArg->hEvent);
//		//pArg->Processed = TRUE;
//		//pArg->Executed = TRUE;
//		free(pArg);
//	}
//	return 0;
//}

/* COMMON - end */


//#if defined(__GNUC__)
//typedef HWND (APIENTRY *FGetConsoleWindow)();
//FGetConsoleWindow GetConsoleWindow = NULL;
//#endif
//extern void SetConsoleFontSizeTo(HWND inConWnd, int inSizeX, int inSizeY);


BOOL WINAPI DllMain( HANDLE hModule, DWORD  ul_reason_for_call, LPVOID lpReserved )
{
	switch (ul_reason_for_call) {
		case DLL_PROCESS_ATTACH:
			{
				ghPluginModule = (HMODULE)hModule;
				gnSelfPID = GetCurrentProcessId();
				
				_ASSERTE(FAR_X_VER<=FAR_Y_VER);
				#ifdef SHOW_STARTED_MSGBOX
				if (!IsDebuggerPresent()) MessageBoxA(NULL, "ConEmu*.dll loaded", "ConEmu plugin", 0);
				#endif
				//#if defined(__GNUC__)
				//GetConsoleWindow = (FGetConsoleWindow)GetProcAddress(GetModuleHandle(L"kernel32.dll"),"GetConsoleWindow");
				//#endif
				
				gpNullSecurity = NullSecurity();
				
				csTabs = new MSection();
				csData = new MSection();
				ghCommandThreads = new CommandThreads();
				
				HWND hConWnd = GetConsoleWindow();
				// Текущая нить не обязана быть главной! Поэтому ищем первую нить процесса!
				gnMainThreadId = GetMainThreadId();
				InitHWND(hConWnd);
				
				//TODO("перенести инициализацию фаровских callback'ов в SetStartupInfo, т.к. будет грузиться как Inject!");
				//if (!StartupHooks(ghPluginModule)) {
				//	_ASSERT(FALSE);
				//	OutputDebugString(L"!!! Can't install injects!!!\n");
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
			}
			break;
		case DLL_PROCESS_DETACH:
			if (gnSynchroCount > 0)
			{
				//if (gFarVersion.dwVerMajor == 2 && gFarVersion.dwBuild < 1735) -- в фаре пока не чинили, поэтому всегда ругаемся, если что...
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
			if (ghCommandThreads)
			{
				delete ghCommandThreads;
				ghCommandThreads = NULL;
			}
			if (gpBgPlugin)
			{
				delete gpBgPlugin;
				gpBgPlugin = NULL;
			}
			break;
	}
	return TRUE;
}

#if defined(CRTSTARTUP)
extern "C"{
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
	if (ConEmuHwnd) {
		if (IsWindow(ConEmuHwnd)) {
			HWND hParent = GetParent(ConEmuHwnd);
			if (hParent) {
				HWND hTest = (HWND)GetWindowLongPtr(hParent, GWLP_USERDATA);
				return (hTest == FarHwnd);
			}
		}
	}
	return TRUE;
}

HWND WINAPI GetFarHWND2(BOOL abConEmuOnly)
{
	if (ConEmuHwnd) {
		if (IsWindow(ConEmuHwnd))
			return ConEmuHwnd;
		ConEmuHwnd = NULL;
		//
		SetConEmuEnvVar(NULL);
	}
	if (abConEmuOnly)
		return NULL;
	return FarHwnd;
}

HWND WINAPI _export GetFarHWND()
{
    return GetFarHWND2(FALSE);
}

BOOL WINAPI IsTerminalMode()
{
    return TerminalMode;
}

void WINAPI _export GetFarVersion ( FarVersion* pfv )
{
	if (!pfv)
		return;

	*pfv = gFarVersion;
}

BOOL LoadFarVersion()
{
    BOOL lbRc=FALSE;
    WCHAR FarPath[MAX_PATH+1], ErrText[512]; ErrText[0] = 0; DWORD dwErr = 0;
    if (GetModuleFileName(0,FarPath,MAX_PATH)) {
		DWORD dwRsrvd = 0;
		DWORD dwSize = GetFileVersionInfoSize(FarPath, &dwRsrvd);
		if (dwSize>0) {
			void *pVerData = Alloc(dwSize, 1);
			if (pVerData) {
				VS_FIXEDFILEINFO *lvs = NULL;
				UINT nLen = sizeof(lvs);
				if (GetFileVersionInfo(FarPath, 0, dwSize, pVerData)) {
					TCHAR szSlash[3]; lstrcpyW(szSlash, L"\\");
					if (VerQueryValue ((void*)pVerData, szSlash, (void**)&lvs, &nLen)) {
						gFarVersion.dwVer = lvs->dwFileVersionMS;
						gFarVersion.dwBuild = lvs->dwFileVersionLS;
						lbRc = TRUE;
					} else {
						dwErr = GetLastError(); lstrcpyW(ErrText, L"LoadFarVersion.VerQueryValue(\"\\\") failed!\n");
					}
				} else {
					dwErr = GetLastError(); lstrcpyW(ErrText, L"LoadFarVersion.GetFileVersionInfo() failed!\n");
				}
				Free(pVerData);
			} else {
				wsprintfW(ErrText, L"LoadFarVersion failed! Can't allocate %n bytes!\n", dwSize);
			}
		} else {
			dwErr = GetLastError(); lstrcpyW(ErrText, L"LoadFarVersion.GetFileVersionInfoSize() failed!\n");
		}
		if (ErrText[0]) lstrcatW(ErrText, FarPath);
	} else {
		dwErr = GetLastError(); lstrcpyW(ErrText, L"LoadFarVersion.GetModuleFileName() failed!");
	}
	if (ErrText[0]) {
		if (dwErr) wsprintfW(ErrText+lstrlenW(ErrText), L"\nErrCode=0x%08X", dwErr);
		MessageBox(0, ErrText, L"ConEmu plugin", MB_OK|MB_ICONSTOP|MB_SETFOREGROUND);
	}

	if (!lbRc) {
		gFarVersion.dwVerMajor = 2;
		gFarVersion.dwVerMinor = 0;
		gFarVersion.dwBuild = FAR_X_VER;
	}

	return lbRc;
}

int WINAPI RegisterPanelView(PanelViewInit *ppvi)
{
	if (!ppvi) {
		_ASSERTE(ppvi->cbSize == sizeof(PanelViewInit));
		return -2;
	}
	if (ppvi->cbSize != sizeof(PanelViewInit)) {
		_ASSERTE(ppvi->cbSize == sizeof(PanelViewInit));
		return -2;
	}
	
	PanelViewRegInfo *pp = (ppvi->bLeftPanel) ? &gPanelRegLeft : &gPanelRegRight;
	
	if (ppvi->bRegister) {
		pp->pfnPeekPreCall = ppvi->pfnPeekPreCall.f;
		pp->pfnPeekPostCall = ppvi->pfnPeekPostCall.f;
		pp->pfnReadPreCall = ppvi->pfnReadPreCall.f;
		pp->pfnReadPostCall = ppvi->pfnReadPostCall.f;
		pp->pfnWriteCall = ppvi->pfnWriteCall.f;
	} else {
		pp->pfnPeekPreCall = pp->pfnPeekPostCall = pp->pfnReadPreCall = pp->pfnReadPostCall = NULL;
		pp->pfnWriteCall = NULL;
	}
	pp->bRegister = ppvi->bRegister;
	
	CESERVER_REQ In;
	int nSize = sizeof(CESERVER_REQ_HDR) + sizeof(In.PVI);
	ExecutePrepareCmd(&In, CECMD_REGPANELVIEW, nSize);
	In.PVI = *ppvi;
	CESERVER_REQ* pOut = ExecuteGuiCmd(FarHwnd, &In, FarHwnd);
	if (!pOut) {
		pp->pfnPeekPreCall = pp->pfnPeekPostCall = pp->pfnReadPreCall = pp->pfnReadPostCall = NULL;
		pp->pfnWriteCall = NULL;
		pp->bRegister = FALSE;
		return -3;
	}
	*ppvi = pOut->PVI;
	ExecuteFreeResult(pOut);

	if (ppvi->cbSize == 0) {
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

//BOOL IsKeyChanged(BOOL abAllowReload)
//{
//	BOOL lbKeyChanged = FALSE;
//	if (ghRegMonitorEvt) {
//		if (WaitForSingleObject(ghRegMonitorEvt, 0) == WAIT_OBJECT_0) {
//			lbKeyChanged = CheckPlugKey();
//			if (lbKeyChanged) gbPlugKeyChanged = TRUE;
//		}
//	}
//
//	if (abAllowReload && gbPlugKeyChanged) {
//		// Вообще-то его бы вызывать в главной нити...
//		CheckMacro(TRUE);
//		gbPlugKeyChanged = FALSE;
//	}
//	return lbKeyChanged;
//}


//BOOL ActivatePluginA(DWORD nCmd, LPVOID pCommandData)
//{
//	BOOL lbRc = FALSE;
//	gnReqCommand = nCmd; gnPluginOpenFrom = -1;
//	gpReqCommandData = pCommandData;
//	ResetEvent(ghReqCommandEvent);
//
//	// Проверить, не изменилась ли горячая клавиша плагина, и если да - пересоздать макросы
//	IsKeyChanged(TRUE);
//
//	INPUT_RECORD evt[10];
//	DWORD dwStartWait, dwCur, dwTimeout, dwInputs = 0, dwInputsFirst = 0, dwWritten = 0;
//	//BOOL  lbInputs = FALSE;
//	HANDLE hInput = GetStdHandle(STD_INPUT_HANDLE);
//
//
//	// Нужен вызов плагина в основной нити
//	//WARNING("Переделать на WriteConsoleInput");
//	gbCmdCallObsolete = FALSE;
//	//SendMessage(FarHwnd, WM_KEYDOWN, VK_F14, 0);
//	//SendMessage(FarHwnd, WM_KEYUP, VK_F14, (LPARAM)(3<<30));
//	evt[0].EventType = evt[1].EventType = KEY_EVENT;
//	evt[0].Event.KeyEvent.bKeyDown = TRUE; evt[1].Event.KeyEvent.bKeyDown = FALSE;
//	evt[0].Event.KeyEvent.uChar.UnicodeChar = evt[1].Event.KeyEvent.uChar.UnicodeChar = 0;
//	evt[0].Event.KeyEvent.wVirtualKeyCode   = evt[1].Event.KeyEvent.wVirtualKeyCode = VK_F14;
//	evt[0].Event.KeyEvent.wVirtualScanCode  = evt[1].Event.KeyEvent.wVirtualScanCode = 0;
//	evt[0].Event.KeyEvent.dwControlKeyState = evt[1].Event.KeyEvent.dwControlKeyState = 0;
//	evt[0].Event.KeyEvent.wRepeatCount = evt[1].Event.KeyEvent.wRepeatCount = 1;
//	if (!WriteConsoleInput(hInput, evt, 2, &dwWritten)) {
//		DWORD dwErr = GetLastError();
//		_ASSERTE(FALSE);
//		SendMessage(FarHwnd, WM_KEYDOWN, VK_F14, 0);
//		SendMessage(FarHwnd, WM_KEYUP, VK_F14, (LPARAM)(3<<30));
//	}
//
//	BOOL lbInput = PeekConsoleInput(hInput, evt, 10, &dwInputsFirst);
//
//	
//	DWORD dwWait;
//	HANDLE hEvents[2];
//	hEvents[0] = ghReqCommandEvent;
//	hEvents[1] = ghServerTerminateEvent;
//
//	dwTimeout = CONEMUFARTIMEOUT;
//	dwStartWait = GetTickCount();
//	//DuplicateHandle(GetCurrentProcess(), ghReqCommandEvent, GetCurrentProcess(), hEvents, 0, 0, DUPLICATE_SAME_ACCESS);
//	do {
//		dwWait = WaitForMultipleObjects ( 2, hEvents, FALSE, 200 );
//		if (dwWait == WAIT_TIMEOUT) {
//			lbInput = PeekConsoleInput(hInput, evt, 10, &dwInputs);
//			if (lbInput && dwInputs == 0) {
//				//Раз из буфера ввода убралось "Отпускание F14" - значит ловить уже нечего, выходим
//				gbCmdCallObsolete = TRUE; // иначе может всплыть меню, когда не надо 
//				break;
//			}
//
//			dwCur = GetTickCount();
//			if ((dwCur - dwStartWait) > dwTimeout)
//				break;
//		}
//	} while (dwWait == WAIT_TIMEOUT);
//	if (dwWait)
//		ResetEvent(ghReqCommandEvent); // Сразу сбросим, вдруг не дождались?
//	else
//		lbRc = TRUE;
//
//	gpReqCommandData = NULL;
//	gnReqCommand = -1; gnPluginOpenFrom = -1;
//
//	return lbRc;
//}
//
//BOOL ActivatePluginW(DWORD nCmd, LPVOID pCommandData, DWORD nTimeout = CONEMUFARTIMEOUT)
//{
//	BOOL lbRc = FALSE;
//	gnReqCommand = -1; gnPluginOpenFrom = -1; gpReqCommandData = NULL;
//	ResetEvent(ghReqCommandEvent);
//
//	// Нужен вызов плагина в остновной нити
//	gbCmdCallObsolete = FALSE;
//
//	SynchroArg *Param = (SynchroArg*)calloc(sizeof(SynchroArg),1);
//	Param->SynchroType = SynchroArg::eCommand;
//	Param->Param1 = nCmd;
//	Param->Param2 = (LPARAM)pCommandData;
//	Param->hEvent = ghReqCommandEvent;
//	Param->Obsolete = FALSE;
//
//	lbRc = CallSynchro995(Param, nTimeout);
//	
//	if (!lbRc)
//		ResetEvent(ghReqCommandEvent); // Сразу сбросим, вдруг не дождались?
//
//	gpReqCommandData = NULL;
//	gnReqCommand = -1; gnPluginOpenFrom = -1;
//
//	return lbRc;
//}

// Внимание! Теоретически, из этой функции Far2 может сразу вызвать ProcessSynchroEventW.
// Но в текущей версии Far2 она работает асинхронно и сразу выходит, а сама
// ProcessSynchroEventW зовется потом в главной нити (где-то при чтении буфера консоли)
void ExecuteSynchro()
{
	if (IS_SYNCHRO_ALLOWED)
	{
		if (gbSynchroProhibited)
		{
			_ASSERT(gbSynchroProhibited==false);
			return;
		}

		//psi.AdvControl(psi.ModuleNumber,ACTL_SYNCHRO,NULL);
		if (gFarVersion.dwBuild>=FAR_Y_VER)
			FUNC_Y(ExecuteSynchro)();
		else
			FUNC_X(ExecuteSynchro)();
	}
}

// Должна вызываться ТОЛЬКО из нитей уже заблокированных семафором ghPluginSemaphore
static BOOL ActivatePlugin (
		DWORD nCmd, LPVOID pCommandData,
		DWORD nTimeout = CONEMUFARTIMEOUT // Release=10сек, Debug=2мин.
	)
{
	BOOL lbRc = FALSE;
	ResetEvent(ghReqCommandEvent);

	//gbCmdCallObsolete = FALSE;
	
	gnReqCommand = nCmd; gpReqCommandData = pCommandData;
	gnPluginOpenFrom = -1;
	// Нужен вызов плагина в остновной нити
	gbReqCommandWaiting = TRUE;

	DWORD nWait = 100; // если тут останется (!=0) - функция вернут ошибку
	HANDLE hEvents[] = {ghServerTerminateEvent, ghReqCommandEvent};
	int nCount = countof(hEvents);

	DEBUGSTRMENU(L"*** Waiting for plugin activation\n");
	
	// Если есть ACTL_SYNCHRO - позвать его, иначе - "активация" в главной нити
	// выполняется тогда, когда фар зовет ReadConsoleInput(1).
	//if (gFarVersion.dwVerMajor = 2 && gFarVersion.dwBuild >= 1006)
	if (IS_SYNCHRO_ALLOWED)
	{
		ExecuteSynchro();
	}

	if (nCmd == CMD_REDRAWFAR || nCmd == CMD_FARPOST)
		nTimeout = min(1000,nTimeout); // чтобы не зависало при попытке ресайза, если фар не отзывается.

	// Подождать активации. Сколько ждать - может указать вызывающая функция
	nWait = WaitForMultipleObjects(nCount, hEvents, FALSE, nTimeout);
	if (nWait != WAIT_OBJECT_0 && nWait != (WAIT_OBJECT_0+1))
	{
		_ASSERTE(nWait==WAIT_OBJECT_0);
		if (nWait == (WAIT_OBJECT_0+1))
		{
			if (!gbReqCommandWaiting)
			{
				// Значит плагин в основной нити все-таки активировался, подождем еще?
				OutputDebugString(L"!!! Plugin execute timeout !!!\n");
				nWait = WaitForMultipleObjects(nCount, hEvents, FALSE, nTimeout);
			}

			//// Таймаут, эту команду плагин должен пропустить, когда фар таки соберется ее выполнить
			//Param->Obsolete = TRUE;
		}
	}
	else
	{
		DEBUGSTRMENU(L"*** DONE\n");
	}

	lbRc = (nWait == (WAIT_OBJECT_0+1));
	
	if (!lbRc)
	{
		// Сразу сбросим, вдруг не дождались?
		gbReqCommandWaiting = FALSE;
		ResetEvent(ghReqCommandEvent);
	}

	gpReqCommandData = NULL;
	gnReqCommand = -1; gnPluginOpenFrom = -1;

	return lbRc;
}

typedef HANDLE (WINAPI *OpenPlugin_t)(int OpenFrom,INT_PTR Item);

WARNING("Обязательно сделать возможность отваливаться по таймауту, если плагин не удалось активировать");
// Проверку можно сделать чтением буфера ввода - если там еще есть событие отпускания F11 - значит
// меню плагинов еще загружается. Иначе можно еще чуть-чуть подождать, и отваливаться - активироваться не получится
BOOL ProcessCommand(DWORD nCmd, BOOL bReqMainThread, LPVOID pCommandData, CESERVER_REQ** ppResult /*= NULL*/)
{
	BOOL lbSucceeded = FALSE;
	CESERVER_REQ* pCmdRet = NULL;
	
	if (ppResult) // сначала - сбросить
		*ppResult = NULL;

	// Некоторые команды можно выполнять в любой нити
	if (nCmd == CMD_SET_CON_FONT)
		bReqMainThread = FALSE;

	//PRAGMA_ERROR("Это нужно делать только тогда, когда семафор уже заблокирован!");
	//if (gpCmdRet) { Free(gpCmdRet); gpCmdRet = NULL; }
	//gpData = NULL; gpCursor = NULL;

	WARNING("Тут нужно сделать проверку содержимого консоли");
	// Если отображено меню - плагин не запустится
	// Не перепутать меню с пустым экраном (Ctrl-O)

	if (bReqMainThread && (gnMainThreadId != GetCurrentThreadId()))
	{
		_ASSERTE(ghPluginSemaphore!=NULL);
		_ASSERTE(ghServerTerminateEvent!=NULL);
		
		// Issue 198: Redraw вызывает отрисовку фаром (1.7x) UserScreen-a (причем без кейбара)
		if (gFarVersion.dwVerMajor < 2 && nCmd == CMD_REDRAWFAR)
		{
			return FALSE; // лучше его просто пропустить
		}
		
		if (nCmd == CMD_FARPOST)
		{
			return FALSE; // Это просто проверка, что фар отработал цикл
		}
			
		// Запомним, чтобы знать, были ли созданы данные?
		CESERVER_REQ* pOldCmdRet = gpCmdRet;

		//// Некоторые команды можно выполнить сразу
		//if (nCmd == CMD_SETSIZE) {
		//	DWORD nHILO = *((DWORD*)pCommandData);
		//	SHORT nWidth = LOWORD(nHILO);
		//	SHORT nHeight = HIWORD(nHILO);
		//	WARNING("Низя CONOUT$ открывать/закрывать - у Win7 крышу сносит");
		//	MConHandle hConOut ( L"CONOUT$" );
		//	CONSOLE_SCREEN_BUFFER_INFO csbi = {{0,0}};
		//	BOOL lbRc = GetConsoleScreenBufferInfo(hConOut, &csbi);
		//	hConOut.Close();
		//	if (lbRc) {
		//		// Если размер консоли менять вообще не нужно
		//		if (csbi.dwSize.X == nWidth && csbi.dwSize.Y == nHeight) {
		//			OutDataAlloc(sizeof(nHILO));
		//			OutDataWrite(&nHILO, sizeof(nHILO));
		//			return gpCmdRet;
		//		}
		//	}
		//}

		if (/*nCmd == CMD_LEFTCLKSYNC ||*/ nCmd == CMD_CLOSEQSEARCH)
		{
			ResetEvent(ghConsoleWrite);
			gbWaitConsoleWrite = TRUE;
		}
		
		// Засемафорить, чтобы несколько команд одновременно не пошли...
		{
			HANDLE hEvents[2] = {ghServerTerminateEvent, ghPluginSemaphore};
			DWORD dwWait = WaitForMultipleObjects(2, hEvents, FALSE, INFINITE);
			if (dwWait == WAIT_OBJECT_0)
			{
				// Плагин завершается
				return FALSE;
			}
			if (nCmd == CMD_REDRAWFAR)
				gbNeedBgActivate = TRUE;
			lbSucceeded = ActivatePlugin(nCmd, pCommandData);
			if (lbSucceeded && /*pOldCmdRet !=*/ gpCmdRet)
			{
				pCmdRet = gpCmdRet; // запомнить результат!
				if (ppResult != &gpCmdRet)
					gpCmdRet = NULL;
			}
			ReleaseSemaphore(ghPluginSemaphore, 1, NULL);
		}
		// конец семафора
		
		
		if (nCmd == CMD_LEFTCLKSYNC || nCmd == CMD_CLOSEQSEARCH)
		{
			ResetEvent(ghConsoleInputEmpty);
			gbWaitConsoleInputEmpty = TRUE;
			DWORD nWait = WaitForSingleObject(ghConsoleInputEmpty, 2000);
			if (nWait == WAIT_OBJECT_0)
			{
				if (nCmd == CMD_CLOSEQSEARCH)
				{
					// И подождать, пока Фар обработает это событие (то есть до следующего чтения [Peek])
					nWait = WaitForSingleObject(ghConsoleWrite, 1000);
					lbSucceeded = (nWait == WAIT_OBJECT_0);
				}
			}
			else
			{
				#ifdef _DEBUG
				DEBUGSTRMENU((nWait != 0) ? L"*** QUEUE IS NOT EMPTY\n" : L"*** QUEUE IS EMPTY\n");
				#endif
				gbWaitConsoleInputEmpty = FALSE;
				lbSucceeded = (nWait == WAIT_OBJECT_0);
			}
			//DWORD nTestEvents = 0, dwTicks = GetTickCount();
			//DEBUGSTRMENU(L"*** waiting for queue empty\n");
			//HANDLE h = GetStdHandle(STD_INPUT_HANDLE);
			//GetNumberOfConsoleInputEvents(h, &nTestEvents);
			//while (nTestEvents > 0 /*&& (dwTicks - GetTickCount()) < 300*/) {
			//	Sleep(10);
			//	GetNumberOfConsoleInputEvents(h, &nTestEvents);
			//	DWORD nCurTick = GetTickCount();
			//	if ((nCurTick - dwTicks) > 300)
			//		break;
			//}
		}
		
		// Собственно Redraw фар выполнит не тогда, когда его функцию позвали,
		// а когда к нему управление вернется
		if (nCmd == CMD_REDRAWFAR)
		{
			HANDLE hEvents[2] = {ghServerTerminateEvent, ghPluginSemaphore};
			DWORD dwWait = WaitForMultipleObjects(2, hEvents, FALSE, INFINITE);
			if (dwWait == WAIT_OBJECT_0) {
				// Плагин завершается
				return FALSE;
			}
			// Передернуть Background плагины
			if (gpBgPlugin) gpBgPlugin->SetForceUpdate();
			WARNING("После перехода на Synchro для FAR2 есть опасение, что следующий вызов может произойти до окончания предыдущего цикла обработки Synchro в Far");
			lbSucceeded = ActivatePlugin(CMD_FARPOST, NULL);
			if (lbSucceeded && /*pOldCmdRet !=*/ gpCmdRet)
			{
				pCmdRet = gpCmdRet; // запомнить результат!
				if (ppResult != &gpCmdRet)
					gpCmdRet = NULL;
			}
			ReleaseSemaphore(ghPluginSemaphore, 1, NULL);
		}
		
		if (ppResult)
		{
			if (ppResult != &gpCmdRet)
			{
				*ppResult = pCmdRet;
			}
		}
		else
		{
			if (pCmdRet && pCmdRet != gpTabs && pCmdRet != gpCmdRet)
			{
				Free(pCmdRet);
			}
		}
		

		//gpReqCommandData = NULL;
		//gnReqCommand = -1; gnPluginOpenFrom = -1;
		return lbSucceeded; // Результат выполнения команды
	}
	
	/*if (gbPlugKeyChanged) {
		gbPlugKeyChanged = FALSE;
		CheckMacro(TRUE);
		gbPlugKeyChanged = FALSE;
	}*/
	
	// Некоторые команды "асинхронные", блокировки не нужны
	if (nCmd == CMD_LOG_SHELL 
		|| nCmd == CMD_SET_CON_FONT
		|| FALSE)
	{
		if (nCmd == CMD_LOG_SHELL)
		{
			TODO("Путь передается аргументом через pipe!");
			LogCreateProcessCheck((wchar_t*)pCommandData);
		}
		else if (nCmd == CMD_SET_CON_FONT)
		{
			CESERVER_REQ_SETFONT* pFont = (CESERVER_REQ_SETFONT*)pCommandData;
			if (pFont && pFont->cbSize == sizeof(CESERVER_REQ_SETFONT))
			{
				SetConsoleFontSizeTo(GetConsoleWindow(), pFont->inSizeY, pFont->inSizeX, pFont->sFontName);
			}
		}

		// Ставим и выходим
		if (ghReqCommandEvent)
			SetEvent(ghReqCommandEvent);

		return TRUE;
	}
	//if (nCmd == CMD_QUITFAR)
	//{
	//	// Ставим сразу, чтобы GUI не повис в ожидании.
	//	if (ghReqCommandEvent)
	//		SetEvent(ghReqCommandEvent);
	//	// т.к. фар может запросить подтверждения...
	//	ExecuteQuitFar();
	//	return TRUE;
	//}

	//EnterCriticalSection(&csData);
	MSectionLock CSD; CSD.Lock(csData, TRUE);

	//if (gpCmdRet) { Free(gpCmdRet); gpCmdRet = NULL; } // !!! Освобождается ТОЛЬКО вызывающей функцией!
	gpCmdRet = NULL; gpData = NULL; gpCursor = NULL;
	
	// Раз дошли сюда - считаем что OK
	lbSucceeded = TRUE;
	
	switch (nCmd)
	{
		case (CMD__EXTERNAL_CALLBACK):
		{
			lbSucceeded = FALSE;
			if (pCommandData 
				&& ((SyncExecuteArg*)pCommandData)->nCmd == CMD__EXTERNAL_CALLBACK
				&& ((SyncExecuteArg*)pCommandData)->CallBack != NULL)
			{
				SyncExecuteArg* pExec = (SyncExecuteArg*)pCommandData;
				BOOL lbCallbackValid = CheckCallbackPtr(pExec->hModule, (FARPROC)pExec->CallBack, FALSE);
				if (lbCallbackValid)
				{
					pExec->CallBack(pExec->lParam);
					lbSucceeded = TRUE;
				}
				else
				{
					lbSucceeded = FALSE;
				}
			}
			break;
		}
		case (CMD_DRAGFROM):
		{
			//BOOL  *pbClickNeed = (BOOL*)pCommandData;
			//COORD *crMouse = (COORD *)(pbClickNeed+1);
			//ProcessCommand(CMD_LEFTCLKSYNC, TRUE/*bReqMainThread*/, pCommandData);

			if (gFarVersion.dwVerMajor==1) {
				ProcessDragFromA();
				ProcessDragToA();
			} else if (gFarVersion.dwBuild>=FAR_Y_VER) {
				FUNC_Y(ProcessDragFrom)();
				FUNC_Y(ProcessDragTo)();
			} else {
				FUNC_X(ProcessDragFrom)();
				FUNC_X(ProcessDragTo)();
			}
			break;
		}
		case (CMD_DRAGTO):
		{
			if (gFarVersion.dwVerMajor==1)
				ProcessDragToA();
			else if (gFarVersion.dwBuild>=FAR_Y_VER)
				FUNC_Y(ProcessDragTo)();
			else
				FUNC_X(ProcessDragTo)();
			break;
		}
		case (CMD_SETWINDOW):
		{
			int nTab = 0;
			// Окно мы можем сменить только если:
			if (gnPluginOpenFrom == OPEN_VIEWER || gnPluginOpenFrom == OPEN_EDITOR 
				|| gnPluginOpenFrom == OPEN_PLUGINSMENU)
			{
				_ASSERTE(pCommandData!=NULL);
				if (pCommandData!=NULL)
					nTab = *((DWORD*)pCommandData);

				gbIgnoreUpdateTabs = TRUE;

				if (gFarVersion.dwVerMajor==1) {
					SetWindowA(nTab);
				} else {
					if (gFarVersion.dwBuild>=FAR_Y_VER)
						FUNC_Y(SetWindow)(nTab);
					else
						FUNC_X(SetWindow)(nTab);
				}

				gbIgnoreUpdateTabs = FALSE;
				UpdateConEmuTabs(0, false, false);
			}
			//SendTabs(gnCurTabCount, FALSE); // Обновить размер передаваемых данных
			pCmdRet = gpTabs;
			break;
		}
		case (CMD_POSTMACRO):
		{
			_ASSERTE(pCommandData!=NULL);
			if (pCommandData!=NULL)
				PostMacro((wchar_t*)pCommandData);
			break;
		}
		case (CMD_CLOSEQSEARCH):
		{
			PostMacro(L"$if (Search) Esc $end");
			break;
		}
		case (CMD_LEFTCLKSYNC):
		{
			//COORD *crMouse = (COORD *)pCommandData;
			BOOL  *pbClickNeed = (BOOL*)pCommandData;
			COORD *crMouse = (COORD *)(pbClickNeed+1);
			
			INPUT_RECORD clk[2] = {{MOUSE_EVENT},{MOUSE_EVENT}};
			int i = 0;
			if (*pbClickNeed) {
				clk[i].Event.MouseEvent.dwButtonState = FROM_LEFT_1ST_BUTTON_PRESSED;
				clk[i].Event.MouseEvent.dwMousePosition = *crMouse;
				i++;
			}
			clk[i].Event.MouseEvent.dwMousePosition = *crMouse;
			i++;
			
			DWORD cbWritten = 0;
			HANDLE h = GetStdHandle(STD_INPUT_HANDLE);
			_ASSERTE(h!=INVALID_HANDLE_VALUE && h!=NULL);
			BOOL fSuccess = WriteConsoleInput(h, clk, 2, &cbWritten);
			if (!fSuccess || cbWritten != 2) {
				_ASSERTE(fSuccess && cbWritten==2);
			}
			break;
		}
		case (CMD_EMENU): //RMENU
		{
			COORD *crMouse = (COORD *)pCommandData;
			const wchar_t *pszUserMacro = (wchar_t*)(crMouse+1);
			
			// Чтобы на чистой системе менюшка всплывала под курсором и не выскакивало сообщение ""
			HKEY hRClkKey = NULL;
			DWORD disp = 0;
			WCHAR szEMenuKey[MAX_PATH*2+64];
			lstrcpyW(szEMenuKey, gszRootKey);
			lstrcatW(szEMenuKey, L"\\Plugins\\RightClick");
			// Ключа может и не быть, если настройки ни разу не сохранялись
			if (0 == RegCreateKeyExW(HKEY_CURRENT_USER, szEMenuKey, 0, 0, 0, KEY_ALL_ACCESS, 0, &hRClkKey, &disp))
			{
				if (disp == REG_CREATED_NEW_KEY) {
					RegSetValueExW(hRClkKey, L"WaitToContinue", 0, REG_DWORD, (LPBYTE)&(disp = 0), sizeof(disp));
					RegSetValueExW(hRClkKey, L"GuiPos", 0, REG_DWORD, (LPBYTE)&(disp = 0), sizeof(disp));
				}
				RegCloseKey(hRClkKey);
			}

			// Иначе в некторых случаях (Win7 & FAR2x64) не отрисовывается сменившийся курсор
			// В FAR 1.7x это приводит к зачернению экрана??? Решается посылкой
			// "пустого" события движения мышки в консоль сразу после ACTL_KEYMACRO
			RedrawAll();
			
			//PostMacro((wchar_t*)L"@F11 %N=Menu.Select(\"EMenu\",0); $if (%N==0) %N=Menu.Select(\"EMenu\",2); $end $if(%N>0) Enter $while (Menu) Enter $end $else $MMode 1 MsgBox(\"ConEmu\",\"EMenu not found in F11\",0x00010001) $end");
			const wchar_t* pszMacro = L"@$If (!CmdLine.Empty) %Flg_Cmd=1; %CmdCurPos=CmdLine.ItemCount-CmdLine.CurPos+1; %CmdVal=CmdLine.Value; Esc $Else %Flg_Cmd=0; $End $Text \"rclk_gui:\" Enter $If (%Flg_Cmd==1) $Text %CmdVal %Flg_Cmd=0; %Num=%CmdCurPos; $While (%Num!=0) %Num=%Num-1; CtrlS $End $End";
			if (*pszUserMacro)
				pszMacro = pszUserMacro;

			PostMacro((wchar_t*)pszMacro);
			
			//// Чтобы GUI не дожидался окончания всплытия EMenu
			//LeaveCriticalSection(&cs Data);
			//SetEvent(ghReqCommandEvent);
			////
			//HMODULE hEMenu = GetModuleHandle(L"emenu.dll");
			//if (!hEMenu)
			//{
			//	if (gFarVersion.dwVerMajor==2) {
			//		TCHAR temp[NM*5];
			//		ExpandEnvironmentStringsW(L"%FARHOME%\\Plugins\\emenu\\EMenu.dll",temp,countof(temp));
			//		if (gFarVersion.dwBuild>=FAR_Y_VER) {
			//			FUNC_Y(LoadPlugin)(temp);
			//		} else {
			//			FUNC_X(LoadPlugin)(temp);
			//		}
			//		// Фактически FAR НЕ загружает длл-ку к сожалению, так что тут мы обломаемся
			//		hEMenu = GetModuleHandle(L"emenu.dll");
			//	}
			//
			//	if (!hEMenu) {
			//		PostMacro((wchar_t*)L"@F11 %N=Menu.Select(\"EMenu\",0); $if (%N==0) %N=Menu.Select(\"EMenu\",2); $end $if(%N>0) Enter $while (Menu) Enter $end $else $MMode 1 MsgBox(\"ConEmu\",\"EMenu not found in F11\",0x00010001) $end");
			//		break; // уже все что мог - сделал макрос
			//	}
			//}
			//if (hEMenu)
			//{
			//	OpenPlugin_t fnOpenPluginW = (OpenPlugin_t)GetProcAddress(hEMenu, (gFarVersion.dwVerMajor==1) ? "OpenPlugin" : "OpenPluginW");
			//	_ASSERTE(fnOpenPluginW);
			//	if (fnOpenPluginW) {
			//		if (gFarVersion.dwVerMajor==1) {
			//			fnOpenPluginW(OPEN_COMMANDLINE, (INT_PTR)"rclk_gui:");
			//		} else {
			//			fnOpenPluginW(OPEN_COMMANDLINE, (INT_PTR)L"rclk_gui:");
			//		}
			//	} else {
			//		// Ругнуться?
			//	}
			//}
			//return NULL;
			break;
		}
		case (CMD_REDRAWFAR):
			if (gFarVersion.dwVerMajor==2) RedrawAll();
			break;
		case (CMD_CHKRESOURCES):
			CheckResources(TRUE);
			break;
		//case (CMD_SETSIZE):
		//{
		//	_ASSERTE(pCommandData!=NULL);
		//	//BOOL lbNeedChange = TRUE;
		//	DWORD nHILO = *((DWORD*)pCommandData);
		//	SHORT nWidth = LOWORD(nHILO);
		//	SHORT nHeight = HIWORD(nHILO);

		//	BOOL lbRc = SetConsoleSize(nWidth, nHeight);

		//	MConHandle hConOut ( L"CONOUT$" );
		//	CONSOLE_SCREEN_BUFFER_INFO csbi = {{0,0}};
		//	lbRc = GetConsoleScreenBufferInfo(hConOut, &csbi);
		//	hConOut.Close();
		//	if (lbRc) {
		//		OutDataAlloc(sizeof(nHILO));
		//		nHILO = ((WORD)csbi.dwSize.X) | (((DWORD)(WORD)csbi.dwSize.Y) << 16);
		//		OutDataWrite(&nHILO, sizeof(nHILO));
		//	}

		//	//REDRAWALL
		//}
		case CMD_FARPOST:
		{
			// просто сигнализация о том, что фар получил управление.
			lbSucceeded = TRUE;
			break;
		}
		default:
			// Неизвестная команда!
			_ASSERTE(nCmd == 1);
			lbSucceeded = FALSE;
	}

	// Функция выполняется в том время, пока заблокирован ghPluginSemaphore,
	// поэтому gpCmdRet можно пользовать
	if (lbSucceeded && !pCmdRet) // pCmdRet может уже содержать gpTabs
	{
		pCmdRet = gpCmdRet;
		gpCmdRet = NULL;
	}
	
	if (ppResult)
	{
		*ppResult = pCmdRet;
	}
	else if (pCmdRet && pCmdRet != gpTabs)
	{
		Free(pCmdRet);
	}
	
		
	//LeaveCriticalSection(&csData);
	CSD.Unlock();
	

#ifdef _DEBUG
	_ASSERTE(_CrtCheckMemory());
#endif

	if (ghReqCommandEvent)
		SetEvent(ghReqCommandEvent);

	return lbSucceeded;
}

// Изменить размер консоли. Собственно сам ресайз - выполняется сервером!
BOOL FarSetConsoleSize(SHORT nNewWidth, SHORT nNewHeight)
{
	BOOL lbRc = FALSE;
	if (!gdwServerPID) {
		_ASSERTE(gdwServerPID!=0);
	} else {
		CESERVER_REQ In;
		ExecutePrepareCmd((CESERVER_REQ*)&In, CECMD_SETSIZENOSYNC, sizeof(CESERVER_REQ_HDR)+sizeof(CESERVER_REQ_SETSIZE));
		memset(&In.SetSize, 0, sizeof(In.SetSize));

		// Для 'far /w' нужно оставить высоту буфера!
		In.SetSize.nBufferHeight = gpFarInfo->bBufferSupport ? -1 : 0;
		In.SetSize.size.X = nNewWidth; In.SetSize.size.Y = nNewHeight;

		CESERVER_REQ* pOut = ExecuteSrvCmd(gdwServerPID, &In, GetConsoleWindow());
		if (pOut) {
			ExecuteFreeResult(pOut);
		}

		RedrawAll();
	}

//#ifdef _DEBUG
//	if (GetCurrentThreadId() != gnMainThreadId) {
//		_ASSERTE(GetCurrentThreadId() == gnMainThreadId);
//	}
//#endif
//
//	BOOL lbRc = FALSE, lbNeedChange = TRUE;
//	SHORT nWidth = nNewWidth; if (nWidth</*4*/MIN_CON_WIDTH) nWidth = /*4*/MIN_CON_WIDTH;
//	SHORT nHeight = nNewHeight; if (nHeight</*3*/MIN_CON_HEIGHT) nHeight = /*3*/MIN_CON_HEIGHT;
//	MConHandle hConOut ( L"CONOUT$" );
//	COORD crMax = GetLargestConsoleWindowSize(hConOut);
//	if (crMax.X && nWidth > crMax.X) nWidth = crMax.X;
//	if (crMax.Y && nHeight > crMax.Y) nHeight = crMax.Y;
//
//	CONSOLE_SCREEN_BUFFER_INFO csbi = {{0,0}};
//	if (GetConsoleScreenBufferInfo(hConOut, &csbi)) {
//		if (csbi.dwSize.X == nWidth && csbi.dwSize.Y == nHeight
//			&& csbi.srWindow.Top == 0 && csbi.srWindow.Left == 0
//			&& csbi.srWindow.Bottom == (nWidth-1) 
//			&& csbi.srWindow.Bottom == (nHeight-1))
//		{
//			lbNeedChange = FALSE;
//		}
//	}
//
//	if (lbNeedChange) {
//		DWORD dwErr = 0;
//
//		// Если этого не сделать - размер консоли нельзя УМЕНЬШИТЬ
//		RECT rcConPos = {0}; GetWindowRect(FarHwnd, &rcConPos);
//		MoveWindow(FarHwnd, rcConPos.left, rcConPos.top, 1, 1, 1);
//
//		//specified width and height cannot be less than the width and height of the console screen buffer's window
//		COORD crNewSize = {nWidth, nHeight};
//		lbRc = SetConsoleScreenBufferSize(hConOut, crNewSize);
//		if (!lbRc) dwErr = GetLastError();
//
//		SMALL_RECT rNewRect = {0,0,nWidth-1,nHeight-1};
//		SetConsoleWindowInfo(hConOut, TRUE, &rNewRect);
//
//		RedrawAll();
//	}

	return lbRc;
}

void EmergencyShow()
{
	SetWindowPos(FarHwnd, HWND_TOP, 50,50,0,0, SWP_NOSIZE);
	apiShowWindowAsync(FarHwnd, SW_SHOWNORMAL);
	EnableWindow(FarHwnd, true);
}

static BOOL gbTryOpenMapHeader = FALSE;
int OpenMapHeader();
void CloseMapHeader();
//void ResetExeHooks();

void CheckColorerHeader()
{
	wchar_t szMapName[64];
	HWND lhConWnd = NULL;
	HANDLE h;
	
	lhConWnd = GetConsoleWindow();
	
	wsprintf(szMapName, AnnotationShareName, sizeof(AnnotationInfo), (DWORD)lhConWnd);
	
	h = OpenFileMapping(FILE_MAP_READ, FALSE, szMapName);
	gbHasColorMapping = (h!=NULL);
	if (h) CloseHandle(h);
		
	//#ifdef TRUE_COLORER_OLD_SUPPORT
	//wsprintf(szMapName, L"Console2_annotationInfo_%d_%d", sizeof(AnnotationInfo), (DWORD)GetCurrentProcessId());
	//h = OpenFileMapping(FILE_MAP_READ, FALSE, szMapName);
	//gbHasColorMappingOld = (h!=NULL);
	//if (h) CloseHandle(h);
	//#endif
}

// Функции вызываются в основной нити, вполне можно дергать FAR-API
int CreateColorerHeader()
{
	int iRc = -1;
	wchar_t szMapName[64];
	#ifdef _DEBUG
	DWORD dwErr = 0;
	#endif
	//int nConInfoSize = sizeof(CESERVER_REQ_CONINFO_HDR);
	DWORD nMapCells = 0, nMapSize = 0;
	HWND lhConWnd = NULL;
	
	if (ghColorMapping)
	{
		CloseHandle(ghColorMapping); ghColorMapping = NULL;
	}
	//#ifdef TRUE_COLORER_OLD_SUPPORT
	//	if (ghColorMappingOld) {
	//		CloseHandle(ghColorMappingOld); ghColorMappingOld = NULL;
	//	}
	//#endif

	COORD crMaxSize = GetLargestConsoleWindowSize(GetStdHandle(STD_OUTPUT_HANDLE));
	nMapCells = max(crMaxSize.X,200) * max(crMaxSize.Y,200);
	nMapSize = nMapCells * sizeof(AnnotationInfo) + sizeof(AnnotationHeader);
	lhConWnd = GetConsoleWindow();
	
	if (gbHasColorMapping)
	{
		wsprintf(szMapName, AnnotationShareName, sizeof(AnnotationInfo), (DWORD)lhConWnd);
		
		// Создаем! т.к. должна вызываться только после Detach!
		ghColorMapping = CreateFileMapping(INVALID_HANDLE_VALUE, 
			gpNullSecurity, PAGE_READWRITE, 0, nMapSize, szMapName);
		
		if (!ghColorMapping)
		{
			#ifdef _DEBUG
			dwErr = GetLastError();
			#endif
			// Функции вызываются в основной нити, вполне можно дергать FAR-API
			TODO("Показать ошибку создания MAP для Colorer.AnnotationInfo");
		}
		else
		{
			// Заголовок мэппинга содержит информацию о размере, нужно заполнить!
			AnnotationHeader* pHdr = (AnnotationHeader*)MapViewOfFile(ghColorMapping, FILE_MAP_ALL_ACCESS,0,0,0);
			if (!pHdr)
			{
				#ifdef _DEBUG
				dwErr = GetLastError();
				#endif
				CloseHandle(ghColorMapping); ghColorMapping = NULL;
				// Функции вызываются в основной нити, вполне можно дергать FAR-API
				TODO("Показать ошибку создания MAP для Colorer.AnnotationInfo");
			}
			else
			{
				pHdr->struct_size = sizeof(AnnotationHeader);
				pHdr->bufferSize = nMapCells;
				pHdr->locked = 0; pHdr->flushCounter = 0;
				// В плагине - данные не нужны
				UnmapViewOfFile(pHdr);
			}
		}
	}
	
	//#ifdef TRUE_COLORER_OLD_SUPPORT
	//if (gbHasColorMappingOld)
	//{
	//	wsprintf(szMapName, L"Console2_annotationInfo_%d_%d", sizeof(AnnotationInfo), (DWORD)GetCurrentProcessId());
	//	nMapSize = nMapCells * sizeof(AnnotationInfo);
	//	ghColorMappingOld = CreateFileMapping(INVALID_HANDLE_VALUE, 
	//		gpNullSecurity, PAGE_READWRITE, 0, nMapSize, szMapName);
	//}
	//#endif

	return iRc;
}

void CloseColorerHeader()
{
	if (ghColorMapping)
	{
		CloseHandle(ghColorMapping);
		ghColorMapping = NULL;
	}
	//#ifdef TRUE_COLORER_OLD_SUPPORT
	//if (ghColorMappingOld)
	//{
	//	CloseHandle(ghColorMappingOld);
	//	ghColorMappingOld = NULL;
	//}
	//#endif
}



BOOL gbWasDetached = FALSE;
CONSOLE_SCREEN_BUFFER_INFO gsbiDetached;

BOOL WINAPI OnConsoleDetaching(HookCallbackArg* pArgs)
{
	if (ghMonitorThread)
	{
		SuspendThread(ghMonitorThread);
		// ResumeThread выполняется в конце OnConsoleWasAttached
	}

	// Выполним сразу после SuspendThread, чтобы нить не посчитала, что мы подцепились обратно
	gbWasDetached = (ConEmuHwnd!=NULL && IsWindow(ConEmuHwnd));

	if (gbWasDetached)
	{
		HANDLE hOutput = GetStdHandle(STD_OUTPUT_HANDLE);
		GetConsoleScreenBufferInfo(hOutput, &gsbiDetached);

		// Нужно уведомить ТЕКУЩИЙ сервер, что закрываться по окончании команды не нужно
		if (gdwServerPID == 0)
		{
			_ASSERTE(gdwServerPID != NULL);
		}
		else
		{
			CESERVER_REQ In, *pOut = NULL;
			ExecutePrepareCmd(&In, CECMD_SETDONTCLOSE, sizeof(CESERVER_REQ_HDR));
			pOut = ExecuteSrvCmd(gdwServerPID, &In, FarHwnd);
			if (pOut) ExecuteFreeResult(pOut);
		}
	}

	CloseColorerHeader(); // Если было
	CloseMapHeader();
	ConEmuHwnd = NULL;
	SetConEmuEnvVar(NULL);

	// Потом еще и FarHwnd сбросить нужно будет... Ну этим MonitorThreadProcW займется
	
	return TRUE; // продолжить выполнение функции
}
// Функции вызываются в основной нити, вполне можно дергать FAR-API
VOID WINAPI OnConsoleWasAttached(HookCallbackArg* pArgs)
{
	FarHwnd = GetConsoleWindow();

	if (gbWasDetached)
	{
		// Сразу спрятать окошко
		//apiShowWindow(FarHwnd, SW_HIDE);
	}

	// Если ранее были созданы мэппинги для цвета - пересоздать
	CreateColorerHeader();
	
	if (gbWasDetached)
	{
		/*
		HANDLE hOutput = GetStdHandle(STD_OUTPUT_HANDLE);
		SetConsoleScreenBufferSize(hOutput,sbi.dwSize);
		SetConsoleWindowInfo(hOutput,TRUE,&sbi.srWindow);
		SetConsoleScreenBufferSize(hOutput,sbi.dwSize);
		*/

		// сразу переподцепимся к GUI
		if (!Attach2Gui())
		{
			EmergencyShow();
		}
		
		// Сбрасываем после Attach2Gui, чтобы MonitorThreadProcW случайно
		// не среагировал раньше времени
		gbWasDetached = FALSE;
	}

	if (ghMonitorThread)
		ResumeThread(ghMonitorThread);

}

// Отключил пока. пусть только при Peek... считывается
//void ReloadOnWrite()
//{
//	if (gpConMapInfo)
//		ReloadFarInfo();
//}
//VOID WINAPI OnWasWriteConsoleOutputA(HookCallbackArg* pArgs)
//{
//	if (!pArgs->bMainThread || gFarVersion.dwVerMajor!=1) return;
//	ReloadOnWrite();	
//}
//VOID WINAPI OnWasWriteConsoleOutputW(HookCallbackArg* pArgs)
//{
//	if (!pArgs->bMainThread || gFarVersion.dwVerMajor!=2) return;
//	ReloadOnWrite();	
//}

// Эту нить нужно оставить, чтобы была возможность отобразить консоль при падении ConEmu
DWORD WINAPI MonitorThreadProcW(LPVOID lpParameter)
{
	//DWORD dwProcId = GetCurrentProcessId();

	DWORD dwStartTick = GetTickCount();
	DWORD dwMonitorTick = dwStartTick;
	BOOL lbStartedNoConEmu = (ConEmuHwnd == NULL) && !gbStartedUnderConsole2;
	//BOOL lbTryOpenMapHeader = FALSE;
	//_ASSERTE(ConEmuHwnd!=NULL); -- ConEmu может подцепиться позднее!
	
	while (true)
	{
		DWORD dwWait = 0;
		DWORD dwTimeout = 500;
		/*#ifdef _DEBUG
		dwTimeout = INFINITE;
		#endif*/

		//dwWait = WaitForMultipleObjects(MAXCMDCOUNT, hEventCmd, FALSE, dwTimeout);
		dwWait = WaitForSingleObject(ghServerTerminateEvent, dwTimeout);
		if (dwWait == WAIT_OBJECT_0)
			break; // завершение плагина

		// Если FAR запущен в "невидимом" режиме и по истечении таймаута
		// так и не подцепились к ConEmu - всплыть окошко консоли
		if (lbStartedNoConEmu && ConEmuHwnd == NULL && FarHwnd != NULL)
		{
			DWORD dwCurTick = GetTickCount();
			DWORD dwDelta = dwCurTick - dwStartTick;
			if (dwDelta > GUI_ATTACH_TIMEOUT)
			{
				lbStartedNoConEmu = FALSE;
				if (!TerminalMode && !IsWindowVisible(FarHwnd))
				{
					EmergencyShow();
				}
			}
		}

		// Теоретически, нить обработки может запуститься и без ConEmuHwnd (под телнетом)
	    if (ConEmuHwnd && FarHwnd && (dwWait>=(WAIT_OBJECT_0+MAXCMDCOUNT)))
	    {
	    
	    	// Может быть ConEmu свалилось
	    	if (!IsWindow(ConEmuHwnd) && ConEmuHwnd)
	    	{
	    		HWND hConWnd = GetConsoleWindow();
	    		if (hConWnd && !IsWindow(hConWnd))
	    		{
					// hConWnd не валидно
					MessageBox(0, L"ConEmu was abnormally termintated!\r\nExiting from FAR", L"ConEmu plugin", MB_OK|MB_ICONSTOP|MB_SETFOREGROUND);
					TerminateProcess(GetCurrentProcess(), 100);
					return 0;
	    		}

	    		if (!TerminalMode && !IsWindowVisible(FarHwnd))
	    		{
					EmergencyShow();
				}
		    }
	    }


		if (!gbWasDetached && !ConEmuHwnd)
		{
			// ConEmu могло подцепиться
			if (gpConMapInfo && gpConMapInfo->hConEmuWnd)
			{
				gbWasDetached = FALSE;
				ConEmuHwnd = (HWND)gpConMapInfo->hConEmuWnd;
			}
			//MFileMapping<CESERVER_REQ_CONINFO_HDR> ConMap;
			//ConMap.InitName(CECONMAPNAME, (DWORD)FarHwnd);
			//if (ConMap.Open())
			//{
			//	ConEmuHwnd = (HWND)ConMap.Ptr()->hConEmuWnd;
			//	ConMap.CloseMap();
			//}

			if (ConEmuHwnd)
			{
                SetConEmuEnvVar(ConEmuHwnd);
			
				InitResources();

				// Обновить ТАБЫ после детача!
				if (gnCurTabCount && gpTabs)
					SendTabs(gnCurTabCount, TRUE);
			}
		}

		if (ConEmuHwnd && gbMonitorEnvVar && gsMonitorEnvVar[0]
			&& (GetTickCount() - dwMonitorTick) > MONITORENVVARDELTA)
		{
			UpdateEnvVar(gsMonitorEnvVar);

			dwMonitorTick = GetTickCount();
		}

		if (gbNeedPostTabSend)
		{
			DWORD nDelta = GetTickCount() - gnNeedPostTabSendTick;
			if (nDelta > NEEDPOSTTABSENDDELTA)
			{
				if (IsMacroActive())
				{
					gnNeedPostTabSendTick = GetTickCount();
				}
				else
				{
					// Force Send tabs to ConEmu
					MSectionLock SC; SC.Lock(csTabs, TRUE); // блокируем exclusively, чтобы во время пересылки данные не поменялись из другого потока
					SendTabs(gnCurTabCount, TRUE);
					SC.Unlock();
				}
			}
		}

		if (/*ConEmuHwnd &&*/ gbTryOpenMapHeader)
		{
			if (gpConMapInfo)
			{
				_ASSERTE(gpConMapInfo == NULL);
				gbTryOpenMapHeader = FALSE;
			}
			else if (OpenMapHeader() == 0)
			{
				// OK, переподцепились
				gbTryOpenMapHeader = FALSE;
			}
			if (gpConMapInfo)
			{
				// 04.03.2010 Maks - Если мэппинг открыли - принудительно передернуть ресурсы и информацию
				//CheckResources(TRUE); -- должен выполняться в основной нити, поэтому - через Activate
				// 22.09.2010 Maks - вызывать ActivatePlugin - некорректно!
				//ActivatePlugin(CMD_CHKRESOURCES, NULL);
				ProcessCommand(CMD_CHKRESOURCES, TRUE/*bReqMainThread*/, NULL);
			}
		}

		if (gpBgPlugin)
		{
			gpBgPlugin->MonitorBackground();
		}
		
		//if (gpConMapInfo) {
		//	if (gpConMapInfo->nFarPID == 0)
		//		gbNeedReloadFarInfo = TRUE;
		//}
	}


	return 0;
}

//BOOL SendConsoleEvent(INPUT_RECORD* pr, UINT nCount)
//{
//	_ASSERTE(nCount>0);
//	BOOL fSuccess = FALSE;
//
//	WARNING("Убрать, заменить ghConIn на GetStdHandle()"); // Иначе в Win7 будет буфер разрушаться
//	if (!ghConIn) {
//		ghConIn  = CreateFile(L"CONIN$", GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_READ,
//			0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
//		if (ghConIn == INVALID_HANDLE_VALUE) {
//			#ifdef _DEBUG
//			DWORD dwErr = GetLastError();
//			_ASSERTE(ghConIn!=INVALID_HANDLE_VALUE);
//			#endif
//			ghConIn = NULL;
//			return FALSE;
//		}
//	}
//	if (!ghInputSynchroExecuted)
//		ghInputSynchroExecuted = CreateEvent(NULL, FALSE, FALSE, NULL);
//
//	if (gFarVersion.dwVerMajor>1 && (gFarVersion.dwVerMinor>0 || gFarVersion.dwBuild>=1006))
//	{
//		static SynchroArg arg = {SynchroArg::eInput};
//		arg.hEvent = ghInputSynchroExecuted;
//		arg.Param1 = (LPARAM)pr;
//		arg.Param2 = nCount;
//
//		if (gFarVersion.dwBuild>=FAR_Y_VER)
//			fSuccess = FUNC_Y(CallSynchro)(&arg,DEFAULT_SYNCHRO_TIMEOUT);
//		else
//			fSuccess = FUNC_X(CallSynchro)(&arg,DEFAULT_SYNCHRO_TIMEOUT);
//	} 
//	
//	if (!fSuccess)
//	{
//		DWORD nCurInputCount = 0, cbWritten = 0;
//		INPUT_RECORD irDummy[2] = {{0},{0}};
//
//		// 27.06.2009 Maks - If input queue is not empty - wait for a while, to avoid conflicts with FAR reading queue
//		WARNING("Убрать, заменить ghConIn на GetStdHandle()"); // Иначе в Win7 будет буфер разрушаться
//		if (PeekConsoleInput(ghConIn, irDummy, 1, &(nCurInputCount = 0)) && nCurInputCount > 0) {
//			DWORD dwStartTick = GetTickCount();
//			WARNING("Do NOT wait, but place event in Cyclic queue");
//			do {
//				Sleep(5);
//				if (!PeekConsoleInput(ghConIn, irDummy, 1, &(nCurInputCount = 0)))
//					nCurInputCount = 0;
//			} while ((nCurInputCount > 0) && ((GetTickCount() - dwStartTick) < MAX_INPUT_QUEUE_EMPTY_WAIT));
//		}
//
//		fSuccess = WriteConsoleInput(ghConIn, pr, nCount, &cbWritten);
//		_ASSERTE(fSuccess && cbWritten==nCount);
//	}
//
//	return fSuccess;
//} 

//DWORD WINAPI InputThreadProcW(LPVOID lpParameter)
//{
//	MSG msg;
//	static INPUT_RECORD recs[10] = {{0}}; // переменная должна быть глобальной? SynchoApi...
//
//	while (GetMessage(&msg,0,0,0)) {
//		if (msg.message == WM_QUIT) return 0;
//		if (ghServerTerminateEvent) {
//			if (WaitForSingleObject(ghServerTerminateEvent, 0) == WAIT_OBJECT_0) return 0;
//		}
//		if (msg.message == 0) continue;
//
//		if (msg.message == INPUT_THREAD_ALIVE_MSG) {
//			//pRCon->mn_FlushOut = msg.wParam;
//			TODO("INPUT_THREAD_ALIVE_MSG");
//			continue;
//
//		} else {
//
//			INPUT_RECORD *pRec = recs;
//			int nCount = 0, nMaxCount = countof(recs);
//			memset(recs, 0, sizeof(recs));
//
//			do {
//				if (UnpackInputRecord(&msg, pRec)) {
//					TODO("Сделать обработку пачки сообщений, вдруг они накопились в очереди?");
//
//					if (pRec->EventType == KEY_EVENT && pRec->Event.KeyEvent.bKeyDown &&
//						(pRec->Event.KeyEvent.wVirtualKeyCode == 'C' || pRec->Event.KeyEvent.wVirtualKeyCode == VK_CANCEL)
//						)
//					{
//						#define ALL_MODIFIERS (LEFT_ALT_PRESSED|RIGHT_ALT_PRESSED|LEFT_CTRL_PRESSED|RIGHT_CTRL_PRESSED|SHIFT_PRESSED)
//						#define CTRL_MODIFIERS (LEFT_CTRL_PRESSED|RIGHT_CTRL_PRESSED)
//
//						BOOL lbRc = FALSE;
//						DWORD dwEvent = (pRec->Event.KeyEvent.wVirtualKeyCode == 'C') ? CTRL_C_EVENT : CTRL_BREAK_EVENT;
//						//&& (srv.dwConsoleMode & ENABLE_PROCESSED_INPUT)
//
//						//The SetConsoleMode function can disable the ENABLE_PROCESSED_INPUT mode for a console's input buffer, 
//						//so CTRL+C is reported as keyboard input rather than as a signal. 
//						// CTRL+BREAK is always treated as a signal
//						if ( // Удерживается ТОЛЬКО Ctrl
//							(pRec->Event.KeyEvent.dwControlKeyState & CTRL_MODIFIERS) &&
//							((pRec->Event.KeyEvent.dwControlKeyState & ALL_MODIFIERS) 
//							== (pRec->Event.KeyEvent.dwControlKeyState & CTRL_MODIFIERS))
//							)
//						{
//							// Вроде работает, Главное не запускать процесс с флагом CREATE_NEW_PROCESS_GROUP
//							// иначе у микрософтовской консоли (WinXP SP3) сносит крышу, и она реагирует
//							// на Ctrl-Break, но напрочь игнорирует Ctrl-C
//							lbRc = GenerateConsoleCtrlEvent(dwEvent, 0);
//
//							// Это событие (Ctrl+C) в буфер помещается(!) иначе до фара не дойдет собственно клавиша C с нажатым Ctrl
//						}
//					}
//					nCount++; pRec++;
//				}
//				// Если в буфере есть еще сообщения, а recs еще не полностью заполнен
//				if (nCount < nMaxCount)
//				{
//					if (!PeekMessage(&msg, 0,0,0, PM_REMOVE))
//						break;
//					if (msg.message == 0) continue;
//					if (msg.message == WM_QUIT) return 0;
//					if (ghServerTerminateEvent) {
//						if (WaitForSingleObject(ghServerTerminateEvent, 0) == WAIT_OBJECT_0) return 0;
//					}
//				}
//			} while (nCount < nMaxCount);
//			if (nCount > 0)
//				SendConsoleEvent(recs, nCount);
//		}
//	}
//
//	return 0;
//}

void CommonPluginStartup()
{
	gbBgPluginsAllowed = TRUE;

	//2010-12-13 информацию (начальную) о фаре грузим всегда, а отсылаем в GUI только если в ConEmu
	CheckResources(TRUE);

	// здесь же и ReloadFarInfo() позовется
	if (gpConMapInfo) //2010-03-04 Имеет смысл только при запуске из-под ConEmu
	{
		//CheckResources(TRUE);
		LogCreateProcessCheck((LPCWSTR)-1);
	}

	TODO("перенести инициализацию фаровских callback'ов в SetStartupInfo, т.к. будет грузиться как Inject!");
	if (!StartupHooks(ghPluginModule))
	{
		_ASSERT(FALSE);
		OutputDebugString(L"!!! Can't install injects!!!\n");
	}
}

void WINAPI _export SetStartupInfoW(void *aInfo)
{
	if (!gFarVersion.dwVerMajor) LoadFarVersion();

	if (gFarVersion.dwBuild>=FAR_Y_VER)
		FUNC_Y(SetStartupInfoW)(aInfo);
	else
		FUNC_X(SetStartupInfoW)(aInfo);

	gbInfoW_OK = TRUE;

	CommonPluginStartup();
	
	//gbBgPluginsAllowed = TRUE;

	// в FAR2 устарело - Synchro
	//CheckMacro(TRUE);

	//// здесь же и ReloadFarInfo() позовется
	//if (gpConMapInfo) //2010-03-04 Имеет смысл только при запуске из-под ConEmu
	//{
	//	CheckResources(TRUE);
	//	LogCreateProcessCheck((LPCWSTR)-1);
	//}
}

//#define CREATEEVENT(fmt,h) 
//		wsprintf(szEventName, fmt, dwCurProcId ); 
//		h = CreateEvent(NULL, FALSE, FALSE, szEventName); 
//		if (h==INVALID_HANDLE_VALUE) h=NULL;

void CloseMapHeader()
{
	if (gpConMap)
		gpConMap->CloseMap();
	// delete для gpConMap здесь не делаем, может использоваться в других нитях!
	gpConMapInfo = NULL;

	//if (gpConMapInfo)
	//{
	//	UnmapViewOfFile(gpConMapInfo);
	//	gpConMapInfo = NULL;
	//}
	//if (ghFileMapping)
	//{
	//	CloseHandle(ghFileMapping);
	//	ghFileMapping = NULL;
	//}
}

//void InstallTrapHandler();

int OpenMapHeader()
{
	int iRc = -1;
	//wchar_t szMapName[64];
	#ifdef _DEBUG
	DWORD dwErr = 0;
	#endif
	//int nConInfoSize = sizeof(CESERVER_REQ_CONINFO_HDR);
	
	CloseMapHeader();
	
	if (FarHwnd)
	{
		if (!gpConMap)
			gpConMap = new MFileMapping<CESERVER_REQ_CONINFO_HDR>;
		gpConMap->InitName(CECONMAPNAME, (DWORD)FarHwnd);
		if (gpConMap->Open())
		{
			gpConMapInfo = gpConMap->Ptr();
			if (gpConMapInfo)
			{
				//if (gpConMapInfo->nLogLevel)
				//	InstallTrapHandler();
				iRc = 0;
			}
		}
		else
		{
			gpConMapInfo = NULL;
		}

		//wsprintf(szMapName, CECONMAPNAME, (DWORD)FarHwnd);
		//ghFileMapping = OpenFileMapping(FILE_MAP_READ, FALSE, szMapName);
		//if (ghFileMapping)
		//{
		//	gpConMapInfo = (const CESERVER_REQ_CONINFO_HDR*)MapViewOfFile(ghFileMapping, FILE_MAP_READ,0,0,0);
		//	if (gpConMapInfo)
		//	{
		//		//ReloadFarInfo(); -- смысла нет. SetStartupInfo еще не вызывался
		//		iRc = 0;
		//	}
		//	else
		//	{
		//		#ifdef _DEBUG
		//		dwErr = GetLastError();
		//		#endif
		//		CloseHandle(ghFileMapping);
		//		ghFileMapping = NULL;
		//	}
		//}
		//else
		//{
		//	#ifdef _DEBUG
		//	dwErr = GetLastError();
		//	#endif
		//}
	}

	return iRc;
}
	
	
void InitHWND(HWND ahFarHwnd)
{
	gsFarLang[0] = 0;
	//InitializeCriticalSection(csTabs);
	//InitializeCriticalSection(&csData);
	if (!gFarVersion.dwVerMajor) LoadFarVersion(); // пригодится уже здесь!

	// начальная инициализация. в SetStartupInfo поправим
	wsprintfW(gszRootKey, L"Software\\%s", (gFarVersion.dwVerMajor==2) ? L"FAR2" : L"FAR");
	// Нужно учесть, что FAR мог запуститься с ключом /u (выбор конфигурации)
	wchar_t* pszUserSlash = gszRootKey+lstrlenW(gszRootKey);
	lstrcpyW(pszUserSlash, L"\\Users\\");
	wchar_t* pszUserAdd = pszUserSlash+lstrlenW(pszUserSlash);
	if (GetEnvironmentVariable(L"FARUSER", pszUserAdd, MAX_PATH) == 0)
		*pszUserSlash = 0;

	ConEmuHwnd = NULL;
	FarHwnd = ahFarHwnd;
	
	{
		// Console2 check
		wchar_t szMapName[64];
		wsprintf(szMapName, L"Console2_consoleBuffer_%d", (DWORD)GetCurrentProcessId());
		HANDLE hConsole2 = OpenFileMapping(FILE_MAP_READ, FALSE, szMapName);
		gbStartedUnderConsole2 = (hConsole2 != NULL);
		if (hConsole2)
			CloseHandle(hConsole2);
	}

	// CtrlShiftF3 - для MMView & PicView
	if (!ghConEmuCtrlPressed)
	{
		wchar_t szName[64];

		wsprintf(szName, CEKEYEVENT_CTRL, gnSelfPID);
		ghConEmuCtrlPressed = CreateEvent(NULL, TRUE, FALSE, szName);
		if (ghConEmuCtrlPressed) ResetEvent(ghConEmuCtrlPressed); else { _ASSERTE(ghConEmuCtrlPressed); }

		wsprintf(szName, CEKEYEVENT_SHIFT, gnSelfPID);
		ghConEmuShiftPressed = CreateEvent(NULL, TRUE, FALSE, szName);
		if (ghConEmuShiftPressed) ResetEvent(ghConEmuShiftPressed); else { _ASSERTE(ghConEmuShiftPressed); }
	}
	
	OpenMapHeader();
	
	// Проверить, созданы ли буферы для True-Colorer
	// Это для того, чтобы пересоздать их при детаче
	CheckColorerHeader();

	//memset(hEventCmd, 0, sizeof(HANDLE)*MAXCMDCOUNT);
	
	//int nChk = 0;
	ConEmuHwnd = GetConEmuHWND ( FALSE/*abRoot*/  /*, &nChk*/ );

    SetConEmuEnvVar(ConEmuHwnd);
	
	gnMsgTabChanged = RegisterWindowMessage(CONEMUTABCHANGED);

	if (!ghSetWndSendTabsEvent) ghSetWndSendTabsEvent = CreateEvent(0,0,0,0);

	// Даже если мы не в ConEmu - все равно запустить нить, т.к. в ConEmu теперь есть возможность /Attach!
	//WCHAR szEventName[128];
	DWORD dwCurProcId = GetCurrentProcessId();

	if (!ghReqCommandEvent) {
		ghReqCommandEvent = CreateEvent(NULL,FALSE,FALSE,NULL);
		_ASSERTE(ghReqCommandEvent!=NULL);
	}

	// Запустить сервер команд
	wsprintf(gszPluginServerPipe, CEPLUGINPIPENAME, L".", dwCurProcId);
	ghServerTerminateEvent = CreateEvent(NULL,TRUE,FALSE,NULL);
	_ASSERTE(ghServerTerminateEvent!=NULL);
	if (ghServerTerminateEvent) ResetEvent(ghServerTerminateEvent);
	ghPluginSemaphore = CreateSemaphore(NULL, 1, 1, NULL);
    gnPlugServerThreadId = 0;
    ghPlugServerThread = CreateThread(NULL, 0, PlugServerThread, (LPVOID)NULL, 0, &gnPlugServerThreadId);
    _ASSERTE(ghPlugServerThread!=NULL);
	ghConsoleWrite = CreateEvent(NULL,FALSE,FALSE,NULL);
	_ASSERTE(ghConsoleWrite!=NULL);
	ghConsoleInputEmpty = CreateEvent(NULL,FALSE,FALSE,NULL);
	_ASSERTE(ghConsoleInputEmpty!=NULL);


	ghMonitorThread = CreateThread(NULL, 0, MonitorThreadProcW, 0, 0, &gnMonitorThreadId);

	//ghInputThread = CreateThread(NULL, 0, InputThreadProcW, 0, 0, &gnInputThreadId);


	// Если мы не под эмулятором - больше ничего делать не нужно
	if (ConEmuHwnd) {
		//
		DWORD dwPID, dwThread;
		dwThread = GetWindowThreadProcessId(ConEmuHwnd, &dwPID);
		typedef BOOL (WINAPI* AllowSetForegroundWindowT)(DWORD);
		HMODULE hUser32 = GetModuleHandle(L"user32.dll");
		if (hUser32) {
			AllowSetForegroundWindowT AllowSetForegroundWindowF = (AllowSetForegroundWindowT)GetProcAddress(hUser32, "AllowSetForegroundWindow");
			if (AllowSetForegroundWindowF) AllowSetForegroundWindowF(dwPID);
		}
		// дернуть табы, если они нужны
		int tabCount = 0;
		MSectionLock SC; SC.Lock(csTabs);
		CreateTabs(1);
		AddTab(tabCount, true, false, WTYPE_PANELS, NULL, NULL, 0, 0, 0);
		SendTabs(tabCount=1, TRUE);
		SC.Unlock();
	}
}

//void NotifyChangeKey()
//{
//	if (ghRegMonitorKey) { RegCloseKey(ghRegMonitorKey); ghRegMonitorKey = NULL; }
//	if (ghRegMonitorEvt) ResetEvent(ghRegMonitorEvt);
//	
//	WCHAR szKeyName[MAX_PATH*2];
//	lstrcpyW(szKeyName, gszRootKey);
//	lstrcatW(szKeyName, L"\\PluginHotkeys");
//	// Ключа может и не быть, если ни для одного плагина не было зарегистрировано горячей клавиши
//	if (0 == RegOpenKeyEx(HKEY_CURRENT_USER, szKeyName, 0, KEY_NOTIFY, &ghRegMonitorKey)) {
//		if (!ghRegMonitorEvt) ghRegMonitorEvt = CreateEvent(NULL,FALSE,FALSE,NULL);
//		RegNotifyChangeKeyValue(ghRegMonitorKey, TRUE, REG_NOTIFY_CHANGE_LAST_SET, ghRegMonitorEvt, TRUE);
//		return;
//	}
//	// Если их таки нет - пробуем подцепиться к корневому ключу
//	if (0 == RegOpenKeyEx(HKEY_CURRENT_USER, gszRootKey, 0, KEY_NOTIFY, &ghRegMonitorKey)) {
//		if (!ghRegMonitorEvt) ghRegMonitorEvt = CreateEvent(NULL,FALSE,FALSE,NULL);
//		RegNotifyChangeKeyValue(ghRegMonitorKey, TRUE, REG_NOTIFY_CHANGE_LAST_SET, ghRegMonitorEvt, TRUE);
//		return;
//	}
//}

//abCompare=TRUE вызывается после загрузки плагина, если юзер изменил горячую клавишу...
//BOOL CheckPlugKey()
//{
//	WCHAR cCurKey = gcPlugKey;
//	gcPlugKey = 0;
//	BOOL lbChanged = FALSE;
//	HKEY hkey=NULL;
//	WCHAR szMacroKey[2][MAX_PATH], szCheckKey[32];
//	
//	//Прочитать назначенные плагинам клавиши, и если для ConEmu*.dll указана клавиша активации - запомнить ее
//	wsprintfW(szMacroKey[0], L"%s\\PluginHotkeys", gszRootKey/*, szCheckKey*/);
//	if (0==RegOpenKeyExW(HKEY_CURRENT_USER, szMacroKey[0], 0, KEY_READ, &hkey))
//	{
//		DWORD dwIndex = 0, dwSize; FILETIME ft;
//		while (0==RegEnumKeyEx(hkey, dwIndex++, szMacroKey[1], &(dwSize=MAX_PATH), NULL, NULL, NULL, &ft)) {
//			WCHAR* pszSlash = szMacroKey[1]+lstrlenW(szMacroKey[1])-1;
//			while (pszSlash>szMacroKey[1] && *pszSlash!=L'/') pszSlash--;
//			#if !defined(__GNUC__)
//			#pragma warning(disable : 6400)
//			#endif
//			if (lstrcmpiW(pszSlash, L"/conemu.dll")==0 || lstrcmpiW(pszSlash, L"/conemu.x64.dll")==0) {
//				WCHAR lsFullPath[MAX_PATH*2];
//				lstrcpy(lsFullPath, szMacroKey[0]);
//				lstrcat(lsFullPath, L"\\");
//				lstrcat(lsFullPath, szMacroKey[1]);
//
//				RegCloseKey(hkey); hkey=NULL;
//
//				if (0==RegOpenKeyExW(HKEY_CURRENT_USER, lsFullPath, 0, KEY_READ, &hkey)) {
//					dwSize = sizeof(szCheckKey);
//					if (0==RegQueryValueExW(hkey, L"Hotkey", NULL, NULL, (LPBYTE)szCheckKey, &dwSize)) {
//						gcPlugKey = szCheckKey[0];
//					}
//				}
//				//
//				//
//				break;
//			}
//		}
//
//		// Закончили
//		if (hkey) {RegCloseKey(hkey); hkey=NULL;}
//	}
//	
//	lbChanged = (gcPlugKey != cCurKey);
//	
//	return lbChanged;
//}

//void CheckMacro(BOOL abAllowAPI)
//{
//	// и не под эмулятором нужно проверять макросы, иначе потом активация не сработает...
//	//// Если мы не под эмулятором - больше ничего делать не нужно
//	//if (!ConEmuHwnd) return;
//
//
//	// Проверка наличия макроса
//	BOOL lbMacroAdded = FALSE, lbNeedMacro = FALSE;
//	HKEY hkey=NULL;
//	#define MODCOUNT 4
//	int n;
//	char szValue[1024];
//	WCHAR szMacroKey[MODCOUNT][MAX_PATH], szCheckKey[32];
//	DWORD dwSize = 0;
//	//bool lbMacroDontCheck = false;
//
//	//Прочитать назначенные плагинам клавиши, и если для ConEmu*.dll указана клавиша активации - запомнить ее
//	CheckPlugKey();
//
//
//	for (n=0; n<MODCOUNT; n++) {
//		switch(n){
//			case 0: lstrcpyW(szCheckKey, L"F14"); break;
//			case 1: lstrcpyW(szCheckKey, L"CtrlF14"); break;
//			case 2: lstrcpyW(szCheckKey, L"AltF14"); break;
//			case 3: lstrcpyW(szCheckKey, L"ShiftF14"); break;
//		}
//		wsprintfW(szMacroKey[n], L"%s\\KeyMacros\\Common\\%s", gszRootKey, szCheckKey);
//	}
//	if (gFarVersion.dwVerMajor==1) {
//		lstrcpyW(szCheckKey, L"F11  "); //TODO: для ANSI может другой код по умолчанию?
//		szCheckKey[4] = (wchar_t)(gcPlugKey ? gcPlugKey : ((gFarVersion.dwVerMajor==1) ? 0x42C/*0xDC - аналог для OEM*/ : 0x2584));
//	} else {
//		// Пока можно так. (пока GUID не появились)
//		wsprintfW(szCheckKey, L"callplugin(0x%08X,0)", ConEmu_SysID);
//	}
//
//	//if (!lbMacroDontCheck)
//	for (n=0; n<MODCOUNT && !lbNeedMacro; n++)
//	{
//		if (0==RegOpenKeyExW(HKEY_CURRENT_USER, szMacroKey[n], 0, KEY_READ, &hkey))
//		{
//			/*if (gFarVersion.dwVerMajor==1) {
//				if (0!=RegQueryValueExA(hkey, "Sequence", 0, 0, (LPBYTE)szValue, &(dwSize=1022))) {
//					lbNeedMacro = TRUE; // Значение отсутсвует
//				} else {
//					lbNeedMacro = lstrcmpA(szValue, (char*)szCheckKey)!=0;
//				}
//			} else {*/
//				if (0!=RegQueryValueExW(hkey, L"Sequence", 0, 0, (LPBYTE)szValue, &(dwSize=1022))) {
//					lbNeedMacro = TRUE; // Значение отсутсвует
//				} else {
//					//TODO: проверить, как себя ведет VC & GCC на 2х байтовых символах?
//					lbNeedMacro = lstrcmpW((WCHAR*)szValue, szCheckKey)!=0;
//				}
//			//}
//			//	szValue[dwSize]=0;
//			//	#pragma message("ERROR: нужна проверка. В Ansi и Unicode это разные строки!")
//			//	//if (strcmpW(szValue, "F11 \xCC")==0)
//			//		lbNeedMacro = TRUE; // Значение некорректное
//			//}
//			RegCloseKey(hkey); hkey=NULL;
//		} else {
//			lbNeedMacro = TRUE;
//		}
//	}
//
//	if (lbNeedMacro) {
//		//int nBtn = ShowMessage(0, 3);
//		//if (nBtn == 1) { // Don't disturb
//		//	DWORD disp=0;
//		//	lbMacroDontCheck = true;
//		//	if (0==RegCreateKeyExA(HKEY_CURRENT_USER, "Software\\ConEmu", 0, 0, 
//		//		0, KEY_ALL_ACCESS, 0, &hkey, &disp))
//		//	{
//		//		RegSetValueExA(hkey, szCheckKey, 0, REG_BINARY, (LPBYTE)&lbMacroDontCheck, (dwSize=sizeof(lbMacroDontCheck)));
//		//		RegCloseKey(hkey); hkey=NULL;
//		//	}
//		//} else if (nBtn == 0) 
//		for (n=0; n<MODCOUNT; n++)
//		{
//			DWORD disp=0;
//			lbMacroAdded = TRUE;
//			if (0==RegCreateKeyExW(HKEY_CURRENT_USER, szMacroKey[n], 0, 0, 
//				0, KEY_ALL_ACCESS, 0, &hkey, &disp))
//			{
//				lstrcpyA(szValue, "ConEmu support");
//				RegSetValueExA(hkey, "", 0, REG_SZ, (LPBYTE)szValue, (dwSize=(DWORD)lstrlenA(szValue)+1));
//
//				//lstrcpyA(szValue, 
//				//	"$If (Shell || Info || QView || Tree || Viewer || Editor) F12 $Else waitkey(100) $End");
//				//RegSetValueExA(hkey, "Sequence", 0, REG_SZ, (LPBYTE)szValue, (dwSize=lstrlenA(szValue)+1));
//				
//				/*if (gFarVersion.dwVerMajor==1) {
//					RegSetValueExA(hkey, "Sequence", 0, REG_SZ, (LPBYTE)szCheckKey, (dwSize=lstrlenA((char*)szCheckKey)+1));
//				} else {*/
//					RegSetValueExW(hkey, L"Sequence", 0, REG_SZ, (LPBYTE)szCheckKey, (dwSize=2*((DWORD)lstrlenW((WCHAR*)szCheckKey)+1)));
//				//}
//
//				lstrcpyA(szValue, "For ConEmu - plugin activation from listening thread");
//				RegSetValueExA(hkey, "Description", 0, REG_SZ, (LPBYTE)szValue, (dwSize=(DWORD)lstrlenA(szValue)+1));
//
//				RegSetValueExA(hkey, "DisableOutput", 0, REG_DWORD, (LPBYTE)&(disp=1), (dwSize=4));
//
//				RegCloseKey(hkey); hkey=NULL;
//			}
//		}
//	}
//
//
//	// Перечитать макросы в FAR?
//	if (lbMacroAdded && abAllowAPI) {
//		if (gFarVersion.dwVerMajor==1)
//			ReloadMacroA();
//		else if (gFarVersion.dwBuild>=FAR_Y_VER)
//			FUNC_Y(ReloadMacro)();
//		else
//			FUNC_X(ReloadMacro)();
//	}
//
//	// First call
//	if (abAllowAPI) {
//		NotifyChangeKey();
//	}
//}

BOOL ReloadFarInfo(BOOL abForce)
{
	if (!gpFarInfoMapping)
	{
		DWORD dwErr = 0;

		// Создать мэппинг для gpFarInfoMapping
		wchar_t szMapName[MAX_PATH];
		wsprintfW(szMapName, CEFARMAPNAME, gnSelfPID);

		DWORD nMapSize = sizeof(CEFAR_INFO);

		TODO("Заменить на MFileMapping");
		ghFarInfoMapping = CreateFileMapping(INVALID_HANDLE_VALUE, 
			gpNullSecurity, PAGE_READWRITE, 0, nMapSize, szMapName);

		if (!ghFarInfoMapping)
		{
			dwErr = GetLastError();
			//TODO("Показать ошибку создания MAP для ghFarInfoMapping");
			_ASSERTE(ghFarInfoMapping!=NULL);
		}
		else
		{
			gpFarInfoMapping = (CEFAR_INFO*)MapViewOfFile(ghFarInfoMapping, FILE_MAP_ALL_ACCESS,0,0,0);
			if (!gpFarInfoMapping)
			{
				dwErr = GetLastError();
				CloseHandle(ghFarInfoMapping); ghFarInfoMapping = NULL;
				//TODO("Показать ошибку создания MAP для ghFarInfoMapping");
				_ASSERTE(gpFarInfoMapping!=NULL);
			}
			else
			{
				gpFarInfoMapping->cbSize = 0;
			}
		}
	}

	if (!ghFarAliveEvent)
	{
		wchar_t szEventName[64];
		wsprintfW(szEventName, CEFARALIVEEVENT, gnSelfPID);
		ghFarAliveEvent = CreateEvent(gpNullSecurity, FALSE, FALSE, szEventName);				
	}
	
	if (!gpFarInfo)
	{
		gpFarInfo = (CEFAR_INFO*)Alloc(sizeof(CEFAR_INFO),1);

		if (!gpFarInfo)
		{
			_ASSERTE(gpFarInfo!=NULL);
			return FALSE;
		}

		gpFarInfo->cbSize = sizeof(CEFAR_INFO);
		gpFarInfo->nFarInfoIdx = 0;
		gpFarInfo->FarVer = gFarVersion;
		gpFarInfo->nFarPID = gnSelfPID;
		gpFarInfo->nFarTID = gnMainThreadId;
		gpFarInfo->nProtocolVersion = CESERVER_REQ_VER;
		if (gFarVersion.dwVerMajor < 2 || (gFarVersion.dwVerMajor == 2 && gFarVersion.dwBuild < 1564))
		{
			gpFarInfo->bBufferSupport = FALSE;
		}
		else
		{
			// Нужно проверить
			if (gFarVersion.dwBuild>=FAR_Y_VER)
				gpFarInfo->bBufferSupport = FUNC_Y(CheckBufferEnabled)();
			else
				gpFarInfo->bBufferSupport = FUNC_X(CheckBufferEnabled)();
		}
		
		// Загрузить из реестра настройки PanelTabs
		gpFarInfo->PanelTabs.SeparateTabs = gpFarInfo->PanelTabs.ButtonColor = -1;
		if (*gszRootKey)
		{
			WCHAR szTabsKey[MAX_PATH*2+32];
			lstrcpyW(szTabsKey, gszRootKey);
			int nLen = lstrlenW(szTabsKey);
			lstrcpyW(szTabsKey+nLen, (szTabsKey[nLen-1] == L'\\') ? L"Plugins\\PanelTabs" : L"\\Plugins\\PanelTabs");
			HKEY hk;
			if (0 == RegOpenKeyExW(HKEY_CURRENT_USER, szTabsKey, 0, KEY_READ, &hk))
			{
				DWORD dwVal, dwSize;
				if (!RegQueryValueExW(hk, L"SeparateTabs", NULL, NULL, (LPBYTE)&dwVal, &(dwSize = 4)))
					gpFarInfo->PanelTabs.SeparateTabs = dwVal ? 1 : 0;
				if (!RegQueryValueExW(hk, L"ButtonColor", NULL, NULL, (LPBYTE)&dwVal, &(dwSize = 4)))
					gpFarInfo->PanelTabs.ButtonColor = dwVal & 0xFF;
				RegCloseKey(hk);
			}
		}
	}

	BOOL lbChanged = FALSE, lbSucceded = FALSE;
	
	if (gFarVersion.dwVerMajor==1)
		lbSucceded = ReloadFarInfoA(/*abFull*/);
	else if (gFarVersion.dwBuild>=FAR_Y_VER)
		lbSucceded = FUNC_Y(ReloadFarInfo)();
	else
		lbSucceded = FUNC_X(ReloadFarInfo)();
	
	if (lbSucceded)
	{
		if (abForce || memcmp(gpFarInfoMapping, gpFarInfo, sizeof(CEFAR_INFO))!=0)
		{
			lbChanged = TRUE;
			gpFarInfo->nFarInfoIdx++;
			*gpFarInfoMapping = *gpFarInfo;
		}
	}

	return lbChanged;
}

bool UpdateConEmuTabsW(int anEvent, bool losingFocus, bool editorSave, void* Param/*=NULL*/)
{
	if (!gbInfoW_OK || gbIgnoreUpdateTabs)
		return false;

	if (gbRequestUpdateTabs)
		gbRequestUpdateTabs = FALSE;

	if (ConEmuHwnd && FarHwnd)
		CheckResources(FALSE);

	MSectionLock SC; SC.Lock(csTabs);

	extern bool FUNC_X(UpdateConEmuTabsW)(int anEvent, bool losingFocus, bool editorSave, void* Param/*=NULL*/);
	extern bool FUNC_Y(UpdateConEmuTabsW)(int anEvent, bool losingFocus, bool editorSave, void* Param/*=NULL*/);

	bool lbCh;

	if (gFarVersion.dwBuild>=FAR_Y_VER)
		lbCh = FUNC_Y(UpdateConEmuTabsW)(anEvent, losingFocus, editorSave, Param);
	else
		lbCh = FUNC_X(UpdateConEmuTabsW)(anEvent, losingFocus, editorSave, Param);

	SC.Unlock();

	return lbCh;
}

bool UpdateConEmuTabs(int anEvent, bool losingFocus, bool editorSave, void* Param/*=NULL*/)
{
	extern bool UpdateConEmuTabsA(int anEvent, bool losingFocus, bool editorSave, void *Param);

	bool lbCh;

	// Блокируем сразу, т.к. ниже по коду gpTabs тоже используется
	MSectionLock SC; SC.Lock(csTabs);

	// На случай, если текущее окно заблокировано диалогом - не получится точно узнать
	// какое окно фара активно. Поэтому вернем последнее известное.
	int nLastCurrentTab = -1, nLastCurrentType = -1;
	if (gpTabs && gpTabs->Tabs.nTabCount > 0)
	{
		nLastCurrentTab = gpTabs->Tabs.CurrentIndex;
		nLastCurrentType = gpTabs->Tabs.CurrentType;
	}

	if (gpTabs)
		gpTabs->Tabs.CurrentIndex = -1; // для строгости

	if (gFarVersion.dwVerMajor==1)
		lbCh = UpdateConEmuTabsA(anEvent, losingFocus, editorSave, Param);
	else
		lbCh = UpdateConEmuTabsW(anEvent, losingFocus, editorSave, Param);

	if (gpTabs && gpTabs->Tabs.CurrentIndex == -1 && nLastCurrentTab != -1 && gpTabs->Tabs.nTabCount > 0)
	{
		// Активное окно определить не удалось
		if ((UINT)nLastCurrentTab >= gpTabs->Tabs.nTabCount)
			nLastCurrentTab = (gpTabs->Tabs.nTabCount - 1);
		gpTabs->Tabs.CurrentIndex = nLastCurrentTab;
		gpTabs->Tabs.tabs[nLastCurrentTab].Current = TRUE;
	}

	SendTabs(gpTabs->Tabs.nTabCount, lbCh && (gnReqCommand==(DWORD)-1));

	if (lbCh && gpBgPlugin)
	{
		gpBgPlugin->SetForceUpdate();
		gpBgPlugin->OnMainThreadActivated();
		gbNeedBgActivate = FALSE;
	}

	return lbCh;
}

BOOL CreateTabs(int windowCount)
{
	if (gpTabs && maxTabCount > (windowCount + 1))
	{
		// пересоздавать не нужно, секцию не трогаем. только запомним последнее кол-во окон
		lastWindowCount = windowCount;
		return TRUE; 
	}

	//Enter CriticalSection(csTabs);

	if ((gpTabs==NULL) || (maxTabCount <= (windowCount + 1)))
	{
		MSectionLock SC; SC.Lock(csTabs, TRUE);

		maxTabCount = windowCount + 20; // с запасом
		if (gpTabs)
		{
			Free(gpTabs); gpTabs = NULL;
		}
		
		gpTabs = (CESERVER_REQ*) Alloc(sizeof(CESERVER_REQ_HDR) + maxTabCount*sizeof(ConEmuTab), 1);
	}
	
	lastWindowCount = windowCount;
	
	//if (!gpTabs) LeaveCriticalSection(csTabs);
	return gpTabs!=NULL;
}


	#ifndef max
	#define max(a,b)            (((a) > (b)) ? (a) : (b))
	#endif

	#ifndef min
	#define min(a,b)            (((a) < (b)) ? (a) : (b))
	#endif

BOOL AddTab(int &tabCount, bool losingFocus, bool editorSave, 
			int Type, LPCWSTR Name, LPCWSTR FileName, int Current, int Modified, int EditViewId)
{
    BOOL lbCh = FALSE;
	DEBUGSTR(L"--AddTab\n");

	if (Type == WTYPE_PANELS)
	{
	    lbCh = (gpTabs->Tabs.tabs[0].Current != (Current/*losingFocus*/ ? 1 : 0)) ||
	           (gpTabs->Tabs.tabs[0].Type != WTYPE_PANELS);
		gpTabs->Tabs.tabs[0].Current = Current/*losingFocus*/ ? 1 : 0;
		//lstrcpyn(gpTabs->Tabs.tabs[0].Name, FUNC_Y(GetMsgW)(0), CONEMUTABMAX-1);
		gpTabs->Tabs.tabs[0].Name[0] = 0;
		gpTabs->Tabs.tabs[0].Pos = 0;
		gpTabs->Tabs.tabs[0].Type = WTYPE_PANELS;
		gpTabs->Tabs.tabs[0].Modified = 0; // Иначе GUI может ошибочно считать, что есть несохраненные редакторы
		gpTabs->Tabs.tabs[0].EditViewId = 0;
		if (!tabCount)
			tabCount++;
		if (Current)
		{
			gpTabs->Tabs.CurrentType = Type;
			gpTabs->Tabs.CurrentIndex = 0;
		}
	}
	else if (Type == WTYPE_EDITOR || Type == WTYPE_VIEWER)
	{
		// Первое окно - должно быть панели. Если нет - значит фар открыт в режиме редактора
		if (tabCount == 1)
		{
			// 04.06.2009 Maks - Не, чего-то не то... при открытии редактора из панелей - он заменяет панели
			//gpTabs->Tabs.tabs[0].Type = Type;
		}

		// when receiving saving event receiver is still reported as modified
		if (editorSave && lstrcmpi(FileName, Name) == 0)
			Modified = 0;
	
	    lbCh = (gpTabs->Tabs.tabs[tabCount].Current != (Current/*losingFocus*/ ? 1 : 0)/*(losingFocus ? 0 : Current)*/) ||
	           (gpTabs->Tabs.tabs[tabCount].Type != Type) ||
	           (gpTabs->Tabs.tabs[tabCount].Modified != Modified) ||
	           (lstrcmp(gpTabs->Tabs.tabs[tabCount].Name, Name) != 0);
	
		// when receiving losing focus event receiver is still reported as current
		gpTabs->Tabs.tabs[tabCount].Type = Type;
		gpTabs->Tabs.tabs[tabCount].Current = (Current/*losingFocus*/ ? 1 : 0)/*losingFocus ? 0 : Current*/;
		gpTabs->Tabs.tabs[tabCount].Modified = Modified;
		gpTabs->Tabs.tabs[tabCount].EditViewId = EditViewId;

		if (gpTabs->Tabs.tabs[tabCount].Current != 0)
		{
			lastModifiedStateW = Modified != 0 ? 1 : 0;

			gpTabs->Tabs.CurrentType = Type;
			gpTabs->Tabs.CurrentIndex = tabCount;
		}
		//else
		//{
		//	lastModifiedStateW = -1; //2009-08-17 при наличии более одного редактора - сносит крышу
		//}

		int nLen = min(lstrlen(Name),(CONEMUTABMAX-1));
		lstrcpyn(gpTabs->Tabs.tabs[tabCount].Name, Name, nLen+1);
		gpTabs->Tabs.tabs[tabCount].Name[nLen]=0;

		gpTabs->Tabs.tabs[tabCount].Pos = tabCount;
		tabCount++;
	}
	
	return lbCh;
}

void SendTabs(int tabCount, BOOL abForceSend/*=FALSE*/)
{
	MSectionLock SC; SC.Lock(csTabs);

	if (!gpTabs)
	{
		_ASSERTE(gpTabs!=NULL);
		return;
	}

	gnCurTabCount = tabCount; // сразу запомним!, А то при ретриве табов количество еще старым будет...

	gpTabs->Tabs.nTabCount = tabCount;
	gpTabs->hdr.cbSize = sizeof(CESERVER_REQ_HDR) + sizeof(CESERVER_REQ_CONEMUTAB) 
		+ sizeof(ConEmuTab) * ((tabCount > 1) ? (tabCount - 1) : 0);

	// Обновляем структуру сразу, чтобы она была готова к отправке в любой момент
	ExecutePrepareCmd(gpTabs, CECMD_TABSCHANGED, gpTabs->hdr.cbSize);

	// Это нужно делать только если инициировано ФАРОМ. Если запрос прислал ConEmu - не посылать...
	if (tabCount && ConEmuHwnd && IsWindow(ConEmuHwnd) && abForceSend)
	{
		gpTabs->Tabs.bMacroActive = IsMacroActive();
		gpTabs->Tabs.bMainThread = (GetCurrentThreadId() == gnMainThreadId);
		// Если выполняется макрос и отложенная отсылка (по окончанию) уже запрошена
		if (gpTabs->Tabs.bMacroActive && gbNeedPostTabSend)
		{
			gnNeedPostTabSendTick = GetTickCount(); // Обновить тик
			return;
		}

		gbNeedPostTabSend = FALSE;

		CESERVER_REQ* pOut =
			ExecuteGuiCmd(FarHwnd, gpTabs, FarHwnd);
		if (pOut)
		{
			if (pOut->hdr.cbSize >= (sizeof(CESERVER_REQ_HDR) + sizeof(CESERVER_REQ_CONEMUTAB_RET))) {
				if (gpTabs->Tabs.bMacroActive && pOut->TabsRet.bNeedPostTabSend)
				{
					// Отослать после того, как макрос завершится
					gbNeedPostTabSend = TRUE;
					gnNeedPostTabSendTick = GetTickCount();
				}
				else if (pOut->TabsRet.bNeedResize)
				{
					// Если это отложенная отсылка табов после выполнения макросов
					if (GetCurrentThreadId() == gnMainThreadId)
					{
						FarSetConsoleSize(pOut->TabsRet.crNewSize.X, pOut->TabsRet.crNewSize.Y);
					}
				}
			}
			ExecuteFreeResult(pOut);
		}
	}
    
	SC.Unlock();
}

// watch non-modified -> modified editor status change

//int lastModifiedStateW = -1;
//bool gbHandleOneRedraw = false; //, gbHandleOneRedrawCh = false;

int WINAPI _export ProcessEditorInputW(void* Rec)
{
	// Даже если мы не под эмулятором - просто запомним текущее состояние
	//if (!ConEmuHwnd) return 0; // Если мы не под эмулятором - ничего
	if (gFarVersion.dwBuild>=FAR_Y_VER)
		return FUNC_Y(ProcessEditorInputW)((LPCVOID)Rec);
	else
		return FUNC_X(ProcessEditorInputW)((LPCVOID)Rec);
}

int WINAPI _export ProcessEditorEventW(int Event, void *Param)
{
	if (!gbRequestUpdateTabs)
	{
		if (Event == EE_READ || Event == EE_CLOSE || Event == EE_GOTFOCUS || Event == EE_KILLFOCUS || Event == EE_SAVE)
		{
			gbRequestUpdateTabs = TRUE;
		//} else if (Event == EE_REDRAW && gbHandleOneRedraw) {
		//	gbHandleOneRedraw = false; gbRequestUpdateTabs = TRUE;
		}
	}

	if (gpTabs && Event == EE_CLOSE && gpTabs->Tabs.nTabCount
		&& gpTabs->Tabs.tabs[0].Type != WTYPE_PANELS)
		gbClosingModalViewerEditor = TRUE;


	if (gpBgPlugin && (Event != EE_REDRAW))
	{
		gpBgPlugin->OnMainThreadActivated(Event, -1);
	}

	return 0;
}

int WINAPI _export ProcessViewerEventW(int Event, void *Param)
{
	if (!gbRequestUpdateTabs &&
		(Event == VE_CLOSE || Event == VE_GOTFOCUS || Event == VE_KILLFOCUS || Event == VE_READ))
	{
		gbRequestUpdateTabs = TRUE;
	}

	if (gpTabs && Event == VE_CLOSE && gpTabs->Tabs.nTabCount
		&& gpTabs->Tabs.tabs[0].Type != WTYPE_PANELS)
	{
		gbClosingModalViewerEditor = TRUE;
	}


	if (gpBgPlugin)
	{
		gpBgPlugin->OnMainThreadActivated(-1, Event);
	}

	return 0;
}

void FillLoadedParm(struct ConEmuLoadedArg* pArg, HMODULE hSubPlugin, BOOL abLoaded)
{
	memset(pArg, 0, sizeof(struct ConEmuLoadedArg));
	pArg->cbSize = (DWORD)sizeof(struct ConEmuLoadedArg);
	pArg->nBuildNo = ((MVV_1 % 100)*100000) + (MVV_2*1000) + (MVV_3*10) + (MVV_4 % 10);
	pArg->hConEmu = ghPluginModule;
	pArg->hPlugin = hSubPlugin;
	pArg->bLoaded = abLoaded;
	pArg->bGuiActive = abLoaded && (ConEmuHwnd != NULL);

	// Сервисные функции
	if (abLoaded)
	{
		pArg->GetFarHWND = GetFarHWND;
		pArg->GetFarHWND2 = GetFarHWND2;
		pArg->GetFarVersion = GetFarVersion;
		pArg->IsTerminalMode = IsTerminalMode;
		pArg->IsConsoleActive = IsConsoleActive;
		pArg->RegisterPanelView = RegisterPanelView;
		pArg->RegisterBackground = RegisterBackground;
		pArg->ActivateConsole = ActivateConsole;
		pArg->SyncExecute = SyncExecute;
	}
}

void NotifyConEmuUnloaded()
{
	OnConEmuLoaded_t fnOnConEmuLoaded = NULL;
	BOOL lbSucceded = FALSE;

	HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, 0);
	if(snapshot != INVALID_HANDLE_VALUE)
	{
		MODULEENTRY32 module = {sizeof(MODULEENTRY32)};

		for(BOOL res = Module32First(snapshot, &module); res; res = Module32Next(snapshot, &module))
		{
			if ((fnOnConEmuLoaded = (OnConEmuLoaded_t)GetProcAddress(module.hModule, "OnConEmuLoaded")) != NULL)
			{
				// Наверное, только для плагинов фара
				if (GetProcAddress(module.hModule, "SetStartupInfoW") || GetProcAddress(module.hModule, "SetStartupInfo"))
				{
					struct ConEmuLoadedArg arg = {sizeof(struct ConEmuLoadedArg)};
					FillLoadedParm(&arg, module.hModule, FALSE); // плагин conemu.dll выгружается!
					//arg.hPlugin = module.hModule;
					//arg.nBuildNo = ((MVV_1 % 100)*10000) + (MVV_2*100) + (MVV_3);
					//arg.hConEmu = ghPluginModule;
					//arg.bLoaded = FALSE;

					lbSucceded = FALSE;
					SAFETRY {
						fnOnConEmuLoaded(&arg);
						lbSucceded = TRUE;
					} SAFECATCH {
						// Failed
						_ASSERTE(lbSucceded == TRUE);
					}
				}
			}
		}

		CloseHandle(snapshot);
	}
}

void StopThread(void)
{
	LPCVOID lpPtrConInfo = gpConMapInfo; gpConMapInfo = NULL;
	//LPVOID lpPtrColorInfo = gpColorerInfo; gpColorerInfo = NULL;

	gbBgPluginsAllowed = FALSE;
	NotifyConEmuUnloaded();

	CloseTabs();

	//if (hEventCmd[CMD_EXIT])
	//	SetEvent(hEventCmd[CMD_EXIT]); // Завершить нить

	if (ghServerTerminateEvent)
	{
		SetEvent(ghServerTerminateEvent);
	}
	//if (gnInputThreadId) {
	//	PostThreadMessage(gnInputThreadId, WM_QUIT, 0, 0);
	//}

	if (ghPlugServerThread)
	{
		HANDLE hPipe = INVALID_HANDLE_VALUE;
		DWORD dwWait = 0;
		// Передернуть пайп, чтобы нить сервера завершилась
		OutputDebugString(L"Plugin: Touching our server pipe\n");
		hPipe = CreateFile(gszPluginServerPipe,GENERIC_WRITE,0,NULL,OPEN_EXISTING,0,NULL);
		if (hPipe == INVALID_HANDLE_VALUE) {
			OutputDebugString(L"Plugin: All pipe instances closed?\n");
		}
		else
		{
			OutputDebugString(L"Plugin: Waiting server pipe thread\n");
			dwWait = WaitForSingleObject(ghPlugServerThread, 300); // пытаемся дождаться, пока нить завершится
			// Просто закроем пайп - его нужно было передернуть
			CloseHandle(hPipe);
			hPipe = INVALID_HANDLE_VALUE;
		}
		dwWait = WaitForSingleObject(ghPlugServerThread, 0);
		if (dwWait != WAIT_OBJECT_0)
		{
			#if !defined(__GNUC__)
			#pragma warning (disable : 6258)
			#endif
			TerminateThread(ghPlugServerThread, 100);
		}
		SafeCloseHandle(ghPlugServerThread);
	}
	SafeCloseHandle(ghPluginSemaphore);

	if (ghMonitorThread) // подождем чуть-чуть, или принудительно прибъем нить ожидания
	{
		if (WaitForSingleObject(ghMonitorThread,1000))
		{
			#if !defined(__GNUC__)
			#pragma warning (disable : 6258)
			#endif
			TerminateThread(ghMonitorThread, 100);
		}
		SafeCloseHandle(ghMonitorThread);
	}

	//if (ghInputThread) { // подождем чуть-чуть, или принудительно прибъем нить ожидания
	//	if (WaitForSingleObject(ghInputThread,1000)) {
	//		#if !defined(__GNUC__)
	//		#pragma warning (disable : 6258)
	//		#endif
	//		TerminateThread(ghInputThread, 100);
	//	}
	//	SafeCloseHandle(ghInputThread);
	//}

    if (gpTabs)
	{
	    Free(gpTabs);
	    gpTabs = NULL;
    }
    
    if (ghReqCommandEvent)
	{
	    CloseHandle(ghReqCommandEvent); ghReqCommandEvent = NULL;
    }

	if (gpFarInfo)
	{
		LPVOID ptr = gpFarInfo; gpFarInfo = NULL;
		Free(ptr);
	}
	if (gpFarInfoMapping)
	{
		UnmapViewOfFile(gpFarInfoMapping);
		CloseHandle(ghFarInfoMapping);
		ghFarInfoMapping = NULL;
	}
	if (ghFarAliveEvent)
	{
		CloseHandle(ghFarAliveEvent);
		ghFarAliveEvent = NULL;
	}

	//DeleteCriticalSection(csTabs); memset(csTabs,0,sizeof(csTabs));
	//DeleteCriticalSection(&csData); memset(&csData,0,sizeof(csData));
	
	if (ghRegMonitorKey) { RegCloseKey(ghRegMonitorKey); ghRegMonitorKey = NULL; }
	SafeCloseHandle(ghRegMonitorEvt);
	SafeCloseHandle(ghServerTerminateEvent);
	//WARNING("Убрать, заменить ghConIn на GetStdHandle()"); // Иначе в Win7 будет буфер разрушаться
	//SafeCloseHandle(ghConIn);
	//SafeCloseHandle(ghInputSynchroExecuted);
	SafeCloseHandle(ghSetWndSendTabsEvent);
	SafeCloseHandle(ghConsoleInputEmpty);
	SafeCloseHandle(ghConsoleWrite);
	
	if (gpConMap)
	{
		gpConMap->CloseMap();
		delete gpConMap;
		gpConMap = NULL;
	}
	//if (lpPtrConInfo)
	//{
	//	UnmapViewOfFile(lpPtrConInfo);
	//}
	//if (ghFileMapping)
	//{
	//	CloseHandle(ghFileMapping);
	//	ghFileMapping = NULL;
	//}
	
	CloseColorerHeader();
	
	CommonShutdown();
}


int WINAPI _export ProcessDialogEventW(int Event, void *Param)
{
#ifdef _DEBUG
	static struct
	{ int Evt; LPCWSTR pszName; } sDlgEvents[]
	=
	{
		//{ DM_FIRST,		L"DM_FIRST"},
		{ DM_CLOSE,		L"DM_CLOSE"},
		{ DM_ENABLE,		L"DM_ENABLE"},
		{ DM_ENABLEREDRAW,		L"DM_ENABLEREDRAW"},
		{ DM_GETDLGDATA,		L"DM_GETDLGDATA"},
		{ DM_GETDLGITEM,		L"DM_GETDLGITEM"},
		{ DM_GETDLGRECT,		L"DM_GETDLGRECT"},
		{ DM_GETTEXT,		L"DM_GETTEXT"},
		{ DM_GETTEXTLENGTH,		L"DM_GETTEXTLENGTH"},
		{ DM_KEY,		L"DM_KEY"},
		{ DM_MOVEDIALOG,		L"DM_MOVEDIALOG"},
		{ DM_SETDLGDATA,		L"DM_SETDLGDATA"},
		{ DM_SETDLGITEM,		L"DM_SETDLGITEM"},
		{ DM_SETFOCUS,		L"DM_SETFOCUS"},
		{ DM_REDRAW,		L"DM_REDRAW"},
		{ DM_SETREDRAW,		L"DM_SETREDRAW"},
		{ DM_SETTEXT,		L"DM_SETTEXT"},
		{ DM_SETMAXTEXTLENGTH,		L"DM_SETMAXTEXTLENGTH"},
		{ DM_SETTEXTLENGTH,		L"DM_SETTEXTLENGTH"},
		{ DM_SHOWDIALOG,		L"DM_SHOWDIALOG"},
		{ DM_GETFOCUS,		L"DM_GETFOCUS"},
		{ DM_GETCURSORPOS,		L"DM_GETCURSORPOS"},
		{ DM_SETCURSORPOS,		L"DM_SETCURSORPOS"},
		{ DM_GETTEXTPTR,		L"DM_GETTEXTPTR"},
		{ DM_SETTEXTPTR,		L"DM_SETTEXTPTR"},
		{ DM_SHOWITEM,		L"DM_SHOWITEM"},
		{ DM_ADDHISTORY,		L"DM_ADDHISTORY"},
		{ DM_GETCHECK,		L"DM_GETCHECK"},
		{ DM_SETCHECK,		L"DM_SETCHECK"},
		{ DM_SET3STATE,		L"DM_SET3STATE"},
		{ DM_LISTSORT,		L"DM_LISTSORT"},
		{ DM_LISTGETITEM,		L"DM_LISTGETITEM"},
		{ DM_LISTGETCURPOS,		L"DM_LISTGETCURPOS"},
		{ DM_LISTSETCURPOS,		L"DM_LISTSETCURPOS"},
		{ DM_LISTDELETE,		L"DM_LISTDELETE"},
		{ DM_LISTADD,		L"DM_LISTADD"},
		{ DM_LISTADDSTR,		L"DM_LISTADDSTR"},
		{ DM_LISTUPDATE,		L"DM_LISTUPDATE"},
		{ DM_LISTINSERT,		L"DM_LISTINSERT"},
		{ DM_LISTFINDSTRING,		L"DM_LISTFINDSTRING"},
		{ DM_LISTINFO,		L"DM_LISTINFO"},
		{ DM_LISTGETDATA,		L"DM_LISTGETDATA"},
		{ DM_LISTSETDATA,		L"DM_LISTSETDATA"},
		{ DM_LISTSETTITLES,		L"DM_LISTSETTITLES"},
		{ DM_LISTGETTITLES,		L"DM_LISTGETTITLES"},
		{ DM_RESIZEDIALOG,		L"DM_RESIZEDIALOG"},
		{ DM_SETITEMPOSITION,		L"DM_SETITEMPOSITION"},
		{ DM_GETDROPDOWNOPENED,		L"DM_GETDROPDOWNOPENED"},
		{ DM_SETDROPDOWNOPENED,		L"DM_SETDROPDOWNOPENED"},
		{ DM_SETHISTORY,		L"DM_SETHISTORY"},
		{ DM_GETITEMPOSITION,		L"DM_GETITEMPOSITION"},
		{ DM_SETMOUSEEVENTNOTIFY,		L"DM_SETMOUSEEVENTNOTIFY"},
		{ DM_EDITUNCHANGEDFLAG,		L"DM_EDITUNCHANGEDFLAG"},
		{ DM_GETITEMDATA,		L"DM_GETITEMDATA"},
		{ DM_SETITEMDATA,		L"DM_SETITEMDATA"},
		{ DM_LISTSET,		L"DM_LISTSET"},
		{ DM_LISTSETMOUSEREACTION,		L"DM_LISTSETMOUSEREACTION"},
		{ DM_GETCURSORSIZE,		L"DM_GETCURSORSIZE"},
		{ DM_SETCURSORSIZE,		L"DM_SETCURSORSIZE"},
		{ DM_LISTGETDATASIZE,		L"DM_LISTGETDATASIZE"},
		{ DM_GETSELECTION,		L"DM_GETSELECTION"},
		{ DM_SETSELECTION,		L"DM_SETSELECTION"},
		{ DM_GETEDITPOSITION,		L"DM_GETEDITPOSITION"},
		{ DM_SETEDITPOSITION,		L"DM_SETEDITPOSITION"},
		{ DM_SETCOMBOBOXEVENT,		L"DM_SETCOMBOBOXEVENT"},
		{ DM_GETCOMBOBOXEVENT,		L"DM_GETCOMBOBOXEVENT"},
		{ DM_GETCONSTTEXTPTR,		L"DM_GETCONSTTEXTPTR"},
		{ DM_GETDLGITEMSHORT,		L"DM_GETDLGITEMSHORT"},
		{ DM_SETDLGITEMSHORT,		L"DM_SETDLGITEMSHORT"},
		//{ DM_GETDIALOGINFO,		L"DM_GETDIALOGINFO"},
		//{ DN_FIRST,		L"DN_FIRST"},
		{ DN_BTNCLICK,		L"DN_BTNCLICK"},
		{ DN_CTLCOLORDIALOG,		L"DN_CTLCOLORDIALOG"},
		{ DN_CTLCOLORDLGITEM,		L"DN_CTLCOLORDLGITEM"},
		{ DN_CTLCOLORDLGLIST,		L"DN_CTLCOLORDLGLIST"},
		{ DN_DRAWDIALOG,		L"DN_DRAWDIALOG"},
		{ DN_DRAWDLGITEM,		L"DN_DRAWDLGITEM"},
		{ DN_EDITCHANGE,		L"DN_EDITCHANGE"},
		{ DN_ENTERIDLE,		L"DN_ENTERIDLE"},
		{ DN_GOTFOCUS,		L"DN_GOTFOCUS"},
		{ DN_HELP,		L"DN_HELP"},
		{ DN_HOTKEY,		L"DN_HOTKEY"},
		{ DN_INITDIALOG,		L"DN_INITDIALOG"},
		{ DN_KILLFOCUS,		L"DN_KILLFOCUS"},
		{ DN_LISTCHANGE,		L"DN_LISTCHANGE"},
		{ DN_MOUSECLICK,		L"DN_MOUSECLICK"},
		{ DN_DRAGGED,		L"DN_DRAGGED"},
		{ DN_RESIZECONSOLE,		L"DN_RESIZECONSOLE"},
		{ DN_MOUSEEVENT,		L"DN_MOUSEEVENT"},
		{ DN_DRAWDIALOGDONE,		L"DN_DRAWDIALOGDONE"},
		{ DN_LISTHOTKEY,		L"DN_LISTHOTKEY"},
		//{ DN_GETDIALOGINFO,		L"DN_GETDIALOGINFO"},
		{ DN_CLOSE,		L"DN_CLOSE"},
		{ DN_KEY,		L"DN_KEY"},
		{ DM_USER,		L"DM_USER"},
	};
	
	if (Event == DE_DLGPROCINIT || Event == DE_DEFDLGPROCINIT || Event == DE_DLGPROCEND)
	{
		FarDialogEvent* p = (FarDialogEvent*)Param;
		if (p->Msg != DN_ENTERIDLE)
		{
			wchar_t szDbg[512]; szDbg[0] = 0;
			LPCWSTR pszName = NULL; wchar_t szEvtTemp[32];
			for (int i = 0; i < countof(sDlgEvents); i++)
			{
				if (sDlgEvents[i].Evt == p->Msg)
				{
					pszName = sDlgEvents[i].pszName;
					break;
				}
			}
			if (!pszName)
			{
				if (p->Msg >= DM_USER)
					wsprintfW(szEvtTemp, L"DM_USER+%u", (p->Msg - DM_USER));
				else
					wsprintfW(szEvtTemp, L"(Msg=%u)", p->Msg);
				pszName = szEvtTemp;
			}
			
			switch (Event)
			{
			case DE_DLGPROCINIT:
				{
					wsprintfW(szDbg, L"FarDlgEvent->Prc: %s; hDlg=x%08X; P1=%i; P1=0x%08X\n",
						pszName, (DWORD)p->hDlg, p->Param1, (DWORD)p->Param2);
				} break;
			case DE_DEFDLGPROCINIT:
				{
					wsprintfW(szDbg, L"FarDlgEvent->Def: %s; hDlg=x%08X; P1=%i; P1=0x%08X\n",
						pszName, (DWORD)p->hDlg, p->Param1, (DWORD)p->Param2);
				} break;
			case DE_DLGPROCEND:
				{
					wsprintfW(szDbg, L"FarDlgEvent->Res: %s; hDlg=x%08X; P1=%i; P1=0x%08X; Result=%i\n",
						pszName, (DWORD)p->hDlg, p->Param1, (DWORD)p->Param2, (int)p->Result);
				} break;
			};
			
			if (szDbg[0])
				OutputDebugStringW(szDbg);
		}
	}

#endif
	return FALSE; // разрешение обработки фаром/другими плагинами
}


void   WINAPI _export ExitFARW(void)
{
	// Плагин выгружается, Вызывать Syncho больше нельзя
	gbSynchroProhibited = true;

	ShutdownHooks();
	
	StopThread();

	if (gFarVersion.dwBuild>=FAR_Y_VER)
		FUNC_Y(ExitFARW)();
	else
		FUNC_X(ExitFARW)();
}

// Вызывается при инициализации из SetStartupInfo[W] и при обновлении табов UpdateConEmuTabs[???]
// То есть по идее, это происходит только когда фар явно вызывает плагин (legal api calls)
void CheckResources(BOOL abFromStartup)
{
#ifdef _DEBUG
	_ASSERTE(GetCurrentThreadId() == gnMainThreadId);
#endif

	if (gsFarLang[0] && !abFromStartup)
	{
		static DWORD dwLastTickCount = GetTickCount();
		DWORD dwCurTick = GetTickCount();
		if ((dwCurTick - dwLastTickCount) < CHECK_RESOURCES_INTERVAL)
			return;
		dwLastTickCount = dwCurTick;
	}
	
	//if (abFromStartup) {
	//	_ASSERTE(gpConMapInfo!=NULL);
	//	if (!gpFarInfo)
	//		gpFarInfo = (CEFAR_INFO*)Alloc(sizeof(CEFAR_INFO),1);
	//}
	//if (gpConMapInfo)
	// Теперь он отвязан от gpConMapInfo
	ReloadFarInfo(TRUE);


	if (gpConMapInfo) //2010-12-13 Имеет смысл только при запуске из-под ConEmu
	{
		wchar_t szLang[64];
		GetEnvironmentVariable(L"FARLANG", szLang, 63);
		if (lstrcmpW(szLang, gsFarLang))
		{
			wchar_t szTitle[1024] = {0};
			GetConsoleTitleW(szTitle, 1024);
			SetConsoleTitleW(L"ConEmuC: CheckResources started");

			InitResources();
			
			DWORD dwServerPID = 0;
			FindServerCmd(CECMD_FARLOADED, dwServerPID);
			gdwServerPID = dwServerPID;

			SetConsoleTitleW(szTitle);
		}
	}
}

// Передать в ConEmu строки с ресурсами
void InitResources()
{
	if (!ConEmuHwnd || !FarHwnd) return;
	// В ConEmu нужно передать следущие ресурсы
	//
	int nSize = sizeof(CESERVER_REQ) + sizeof(DWORD) 
		+ 4*(MAX_PATH+1)*2; // + 4 строковых ресурса
	CESERVER_REQ *pIn = (CESERVER_REQ*)Alloc(nSize,1);;
	if (pIn) {
		ExecutePrepareCmd(pIn, CECMD_RESOURCES, nSize);
		pIn->dwData[0] = GetCurrentProcessId();
		//pIn->dwData[1] = gnInputThreadId;
		wchar_t* pszRes = (wchar_t*)&(pIn->dwData[1]);
		int nTempSize = sizeof(gpFarInfo->sLngEdit)/sizeof(gpFarInfo->sLngEdit[0]);
		if (gFarVersion.dwVerMajor==1) {
			GetMsgA(CELngEdit, gpFarInfo->sLngEdit); gpFarInfo->sLngEdit[nTempSize-1] = 0;
			GetMsgA(CELngView, gpFarInfo->sLngView); gpFarInfo->sLngView[nTempSize-1] = 0;
			GetMsgA(CELngTemp, gpFarInfo->sLngTemp); gpFarInfo->sLngTemp[nTempSize-1] = 0;
			//GetMsgA(CELngName, gpFarInfo->sLngName); gpFarInfo->sLngName[nTempSize-1] = 0;
		} else {
			lstrcpynW(gpFarInfo->sLngEdit, GetMsgW(CELngEdit), nTempSize);
			lstrcpynW(gpFarInfo->sLngView, GetMsgW(CELngView), nTempSize);
			lstrcpynW(gpFarInfo->sLngTemp, GetMsgW(CELngTemp), nTempSize);
			//lstrcpynW(gpFarInfo->sLngName, GetMsgW(CELngName), nTempSize);
		}
		lstrcpyW(pszRes, gpFarInfo->sLngEdit); pszRes += lstrlenW(pszRes)+1;
		lstrcpyW(pszRes, gpFarInfo->sLngView); pszRes += lstrlenW(pszRes)+1;
		lstrcpyW(pszRes, gpFarInfo->sLngTemp); pszRes += lstrlenW(pszRes)+1;
		//lstrcpyW(pszRes, gpFarInfo->sLngName); pszRes += lstrlenW(pszRes)+1;
		// Поправить nSize (он должен быть меньше)
		_ASSERTE(pIn->hdr.cbSize >= (DWORD)(((LPBYTE)pszRes) - ((LPBYTE)pIn)));
		pIn->hdr.cbSize = (DWORD)(((LPBYTE)pszRes) - ((LPBYTE)pIn));
		CESERVER_REQ* pOut = ExecuteGuiCmd(FarHwnd, pIn, FarHwnd);
		if (pOut) ExecuteFreeResult(pOut);
		Free(pIn);

		GetEnvironmentVariable(L"FARLANG", gsFarLang, 63);
	}
}

void CloseTabs()
{
	if (ConEmuHwnd && IsWindow(ConEmuHwnd) && FarHwnd) {
		CESERVER_REQ in; // Пустая команда - значит FAR закрывается
		ExecutePrepareCmd(&in, CECMD_TABSCHANGED, sizeof(CESERVER_REQ_HDR));

		CESERVER_REQ* pOut = ExecuteGuiCmd(FarHwnd, &in, FarHwnd);
		if (pOut) ExecuteFreeResult(pOut);
	}
}

// Если не вызывать - буфер увеличивается автоматически. Размер в БАЙТАХ
// Возвращает FALSE при ошибках выделения памяти
BOOL OutDataAlloc(DWORD anSize)
{
	_ASSERTE(gpCmdRet==NULL);
	// + размер заголовка gpCmdRet
	gpCmdRet = (CESERVER_REQ*)Alloc(sizeof(CESERVER_REQ_HDR)+anSize,1);
	if (!gpCmdRet)
		return FALSE;

	// Код команды пока не известен - установит вызывающая функция
	ExecutePrepareCmd(gpCmdRet, 0, anSize+sizeof(CESERVER_REQ_HDR));

	gpData = gpCmdRet->Data;
	gnDataSize = anSize;
	gpCursor = gpData;

	return TRUE;
}

// Размер в БАЙТАХ. вызывается автоматически из OutDataWrite
// Возвращает FALSE при ошибках выделения памяти
BOOL OutDataRealloc(DWORD anNewSize)
{
	if (!gpCmdRet)
		return OutDataAlloc(anNewSize);

	if (anNewSize < gnDataSize)
		return FALSE; // нельзя выделять меньше памяти, чем уже есть

	// realloc иногда не работает, так что даже и не пытаемся
	CESERVER_REQ* lpNewCmdRet = (CESERVER_REQ*)Alloc(sizeof(CESERVER_REQ_HDR)+anNewSize,1);
	if (!lpNewCmdRet)
		return FALSE;

	ExecutePrepareCmd(lpNewCmdRet, gpCmdRet->hdr.nCmd, anNewSize+sizeof(CESERVER_REQ_HDR));

	LPBYTE lpNewData = lpNewCmdRet->Data;
	if (!lpNewData)
		return FALSE;

	// скопировать существующие данные
	memcpy(lpNewData, gpData, gnDataSize);
	// запомнить новую позицию курсора
	gpCursor = lpNewData + (gpCursor - gpData);
	// И новый буфер с размером
	Free(gpCmdRet);
	gpCmdRet = lpNewCmdRet;
	gpData = lpNewData;
	gnDataSize = anNewSize;

	return TRUE;
}

// Размер в БАЙТАХ
// Возвращает FALSE при ошибках выделения памяти
BOOL OutDataWrite(LPVOID apData, DWORD anSize)
{
	if (!gpData) {
		if (!OutDataAlloc(max(1024, (anSize+128))))
			return FALSE;
	} else if (((gpCursor-gpData)+anSize)>gnDataSize) {
		if (!OutDataRealloc(gnDataSize+max(1024, (anSize+128))))
			return FALSE;
	}

	// Скопировать данные
	memcpy(gpCursor, apData, anSize);
	gpCursor += anSize;

	return TRUE;
}

int ShowMessage(int aiMsg, int aiButtons)
{
	if (gFarVersion.dwVerMajor==1)
		return ShowMessageA(aiMsg, aiButtons);
	else if (gFarVersion.dwBuild>=FAR_Y_VER)
		return FUNC_Y(ShowMessage)(aiMsg, aiButtons);
	else
		return FUNC_X(ShowMessage)(aiMsg, aiButtons);
}
int ShowMessageGui(int aiMsg, int aiButtons)
{
	wchar_t pwszBuf[MAX_PATH];
	LPCWSTR pwszMsg = NULL; //GetMsgW(aiMsg);
	if (gFarVersion.dwVerMajor==1)
	{
		GetMsgA(aiMsg, pwszBuf);
		pwszMsg = pwszBuf;
	}
	else
	{
		pwszMsg = GetMsgW(aiMsg);
	}
	wchar_t szTitle[128];
	wsprintfW(szTitle, L"ConEmu plugin (PID=%u)", GetCurrentProcessId());
	if (!pwszMsg || !*pwszMsg)
	{
		wsprintfW(pwszBuf, L"<MsgID=%i>", aiMsg);
		pwszMsg = pwszBuf;
	}
	int nRc = MessageBoxW(NULL, pwszMsg, szTitle, aiButtons);
	return nRc;
}

LPCWSTR GetMsgW(int aiMsg)
{
	if (gFarVersion.dwVerMajor==1)
		return L"";
	else if (gFarVersion.dwBuild>=FAR_Y_VER)
		return FUNC_Y(GetMsg)(aiMsg);
	else
		return FUNC_X(GetMsg)(aiMsg);
}

void PostMacro(wchar_t* asMacro)
{
	if (!asMacro || !*asMacro)
		return;
		
	if (gFarVersion.dwVerMajor==1) {
		int nLen = lstrlenW(asMacro);
		char* pszMacro = (char*)Alloc(nLen+1,1);
		if (pszMacro) {
			WideCharToMultiByte(CP_OEMCP,0,asMacro,nLen+1,pszMacro,nLen+1,0,0);
			PostMacroA(pszMacro);
			Free(pszMacro);
		}
	} else if (gFarVersion.dwBuild>=FAR_Y_VER) {
		FUNC_Y(PostMacro)(asMacro);
	} else {
		FUNC_X(PostMacro)(asMacro);
	}

	//FAR BUGBUG: Макрос не запускается на исполнение, пока мышкой не дернем :(
	//  Это чаще всего проявляется при вызове меню по RClick
	//  Если курсор на другой панели, то RClick сразу по пассивной
	//  не вызывает отрисовку :(
	//if (!mcr.Param.PlainText.Flags) {
	INPUT_RECORD ir[2] = {{MOUSE_EVENT},{MOUSE_EVENT}};
	if (isPressed(VK_CAPITAL))
		ir[0].Event.MouseEvent.dwControlKeyState |= CAPSLOCK_ON;
	if (isPressed(VK_NUMLOCK))
		ir[0].Event.MouseEvent.dwControlKeyState |= NUMLOCK_ON;
	if (isPressed(VK_SCROLL))
		ir[0].Event.MouseEvent.dwControlKeyState |= SCROLLLOCK_ON;
	ir[0].Event.MouseEvent.dwEventFlags = MOUSE_MOVED;
	ir[1].Event.MouseEvent.dwControlKeyState = ir[0].Event.MouseEvent.dwControlKeyState;
	ir[1].Event.MouseEvent.dwEventFlags = MOUSE_MOVED;
	ir[1].Event.MouseEvent.dwMousePosition.X = 1;
	ir[1].Event.MouseEvent.dwMousePosition.Y = 1;

	//2010-01-29 попробуем STD_OUTPUT
	//if (!ghConIn) {
	//	ghConIn  = CreateFile(L"CONIN$", GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_READ,
	//		0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
	//	if (ghConIn == INVALID_HANDLE_VALUE) {
	//		#ifdef _DEBUG
	//		DWORD dwErr = GetLastError();
	//		_ASSERTE(ghConIn!=INVALID_HANDLE_VALUE);
	//		#endif
	//		ghConIn = NULL;
	//		return;
	//	}
	//}

	TODO("Необязательно выполнять реальную запись в консольный буфер. Можно обойтись подстановкой в наших функциях перехвата чтения буфера.");

	HANDLE hIn = GetStdHandle(STD_INPUT_HANDLE);
	DWORD cbWritten = 0;
#ifdef _DEBUG
	BOOL fSuccess = 
#endif
	WriteConsoleInput(hIn/*ghConIn*/, ir, 1, &cbWritten);
	_ASSERTE(fSuccess && cbWritten==1);
	//}
	//InfoW995->AdvControl(InfoW995->ModuleNumber,ACTL_REDRAWALL,NULL);

}

DWORD WINAPI PlugServerThread(LPVOID lpvParam) 
{ 
    BOOL fConnected = FALSE;
    DWORD dwErr = 0;
    HANDLE hPipe = NULL; 
    //HANDLE hWait[2] = {NULL,NULL};
	//DWORD dwTID = GetCurrentThreadId();
	//std::vector<HANDLE>::iterator iter;

    _ASSERTE(gszPluginServerPipe[0]!=0);
    //_ASSERTE(ghServerSemaphore!=NULL);

    // The main loop creates an instance of the named pipe and 
    // then waits for a client to connect to it. When the client 
    // connects, a thread is created to handle communications 
    // with that client, and the loop is repeated. 
    
    //hWait[0] = ghServerTerminateEvent;
    //hWait[1] = ghServerSemaphore;

    // Пока не затребовано завершение консоли
    do {
		fConnected = FALSE; // Новый пайп
        while (!fConnected)
        { 
            _ASSERTE(hPipe == NULL);

			// Проверить, может какие-то командные нити уже завершились
			ghCommandThreads->CheckTerminated();
			//iter = ghCommandThreads.begin();
			//while (iter != ghCommandThreads.end()) {
			//	HANDLE hThread = *iter; dwErr = 0;
			//	if (WaitForSingleObject(hThread, 0) == WAIT_OBJECT_0) {
			//		CloseHandle ( hThread );
			//		iter = ghCommandThreads.erase(iter);
			//	} else {
			//		iter++;
			//	}
			//}

            //// Дождаться разрешения семафора, или закрытия консоли
            //dwErr = WaitForMultipleObjects ( 2, hWait, FALSE, INFINITE );
            //if (dwErr == WAIT_OBJECT_0) {
            //    return 0; // Консоль закрывается
            //}

			//for (int i=0; i<MAX_SERVER_THREADS; i++) {
			//	if (gnServerThreadsId[i] == dwTID) {
			//		ghActiveServerThread = ghServerThreads[i]; break;
			//	}
			//}

            hPipe = CreateNamedPipe( gszPluginServerPipe, PIPE_ACCESS_DUPLEX, 
				PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE | PIPE_WAIT, PIPE_UNLIMITED_INSTANCES,
                PIPEBUFSIZE, PIPEBUFSIZE, 0, gpNullSecurity);

            _ASSERTE(hPipe != INVALID_HANDLE_VALUE);

            if (hPipe == INVALID_HANDLE_VALUE) 
            {
                //DisplayLastError(L"CreateNamedPipe failed"); 
                hPipe = NULL;
                Sleep(50);
                continue;
            }

            // Wait for the client to connect; if it succeeds, 
            // the function returns a nonzero value. If the function
            // returns zero, GetLastError returns ERROR_PIPE_CONNECTED. 

            fConnected = ConnectNamedPipe(hPipe, NULL) ? TRUE : ((dwErr = GetLastError()) == ERROR_PIPE_CONNECTED); 

            // Консоль закрывается!
            if (WaitForSingleObject ( ghServerTerminateEvent, 0 ) == WAIT_OBJECT_0) {
                //FlushFileBuffers(hPipe); -- это не нужно, мы ничего не возвращали
                //DisconnectNamedPipe(hPipe); 
				//ReleaseSemaphore(ghServerSemaphore, 1, NULL);
                SafeCloseHandle(hPipe);
                goto wrap;
            }

            if (!fConnected)
                SafeCloseHandle(hPipe);
        }

        if (fConnected) {
            // сразу сбросим, чтобы не забыть
            //fConnected = FALSE;
            // разрешить другой нити принять вызов
            //ReleaseSemaphore(ghServerSemaphore, 1, NULL);
            
            //ServerThreadCommand ( hPipe ); // При необходимости - записывает в пайп результат сама
			DWORD dwThread = 0;
			HANDLE hThread = CreateThread(NULL, 0, PlugServerThreadCommand, (LPVOID)hPipe, 0, &dwThread);
			_ASSERTE(hThread!=NULL);
			if (hThread==NULL) {
				// Раз не удалось запустить нить - можно попробовать так обработать...
				PlugServerThreadCommand((LPVOID)hPipe);
			} else {
				ghCommandThreads->push_back ( hThread );
			}
			hPipe = NULL; // закрывает ServerThreadCommand
        }

		if (hPipe) {
			if (hPipe != INVALID_HANDLE_VALUE) {
				FlushFileBuffers(hPipe); 
				//DisconnectNamedPipe(hPipe); 
				CloseHandle(hPipe);
			}
			hPipe = NULL;
		}
    } // Перейти к открытию нового instance пайпа
    while (WaitForSingleObject ( ghServerTerminateEvent, 0 ) != WAIT_OBJECT_0);

wrap:
	// Прибивание всех запущенных нитей
	ghCommandThreads->KillAllThreads();
	//iter = ghCommandThreads.begin();
	//while (iter != ghCommandThreads.end()) {
	//	HANDLE hThread = *iter; dwErr = 0;
	//	if (WaitForSingleObject(hThread, 100) != WAIT_OBJECT_0) {
	//		TerminateThread(hThread, 100);
	//	}
	//	CloseHandle ( hThread );
	//	iter = ghCommandThreads.erase(iter);
	//}

    return 0; 
}

DWORD WINAPI PlugServerThreadCommand(LPVOID ahPipe)
{
	HANDLE hPipe = (HANDLE)ahPipe;
    CESERVER_REQ *pIn=NULL;
	BYTE cbBuffer[64]; // Для большей части команд нам хватит
    DWORD cbRead = 0, cbWritten = 0, dwErr = 0;
    BOOL fSuccess = FALSE;

	MSectionThread SCT(csTabs);

    // Send a message to the pipe server and read the response. 
    fSuccess = ReadFile( hPipe, cbBuffer, sizeof(cbBuffer), &cbRead, NULL);
	dwErr = GetLastError();

    if (!fSuccess && (dwErr != ERROR_MORE_DATA)) 
    {
        _ASSERTE("ReadFile(pipe) failed"==NULL);
        CloseHandle(hPipe);
        return 0;
    }
	pIn = (CESERVER_REQ*)cbBuffer; // Пока cast, если нужно больше - выделим память
    _ASSERTE(pIn->hdr.cbSize>=sizeof(CESERVER_REQ_HDR) && cbRead>=sizeof(CESERVER_REQ_HDR));
    _ASSERTE(pIn->hdr.nVersion == CESERVER_REQ_VER);
    if (cbRead < sizeof(CESERVER_REQ_HDR) || /*in.nSize < cbRead ||*/ pIn->hdr.nVersion != CESERVER_REQ_VER) {
        CloseHandle(hPipe);
        return 0;
    }

    int nAllSize = pIn->hdr.cbSize;
    pIn = (CESERVER_REQ*)Alloc(nAllSize,1);
    _ASSERTE(pIn!=NULL);
	if (!pIn) {
		CloseHandle(hPipe);
		return 0;
	}
    memmove(pIn, cbBuffer, cbRead);
    _ASSERTE(pIn->hdr.nVersion==CESERVER_REQ_VER);

    LPBYTE ptrData = ((LPBYTE)pIn)+cbRead;
    nAllSize -= cbRead;

    while(nAllSize>0)
    { 
        //_tprintf(TEXT("%s\n"), chReadBuf);

        // Break if TransactNamedPipe or ReadFile is successful
        if(fSuccess)
            break;

        // Read from the pipe if there is more data in the message.
        fSuccess = ReadFile( 
            hPipe,      // pipe handle 
            ptrData,    // buffer to receive reply 
            nAllSize,   // size of buffer 
            &cbRead,    // number of bytes read 
            NULL);      // not overlapped 

        // Exit if an error other than ERROR_MORE_DATA occurs.
        if( !fSuccess && ((dwErr = GetLastError()) != ERROR_MORE_DATA)) 
            break;
        ptrData += cbRead;
        nAllSize -= cbRead;
    }

    TODO("Может возникнуть ASSERT, если консоль была закрыта в процессе чтения");
    _ASSERTE(nAllSize==0);
    if (nAllSize>0) {
		if (((LPVOID)cbBuffer) != ((LPVOID)pIn))
			Free(pIn);
        CloseHandle(hPipe);
        return 0; // удалось считать не все данные
    }

	UINT nDataSize = pIn->hdr.cbSize - sizeof(CESERVER_REQ_HDR);

    // Все данные из пайпа получены, обрабатываем команду и возвращаем (если нужно) результат
	//fSuccess = WriteFile( hPipe, pOut, pOut->nSize, &cbWritten, NULL);

	if (pIn->hdr.nCmd == CMD_LANGCHANGE) {
		_ASSERTE(nDataSize>=4);
		// LayoutName: "00000409", "00010409", ...
		// А HKL от него отличается, так что передаем DWORD
		// HKL в x64 выглядит как: "0x0000000000020409", "0xFFFFFFFFF0010409"
		DWORD hkl = pIn->dwData[0];
		if (hkl) {
			DWORD dwLastError = 0;
			WCHAR szLoc[10]; wsprintf(szLoc, L"%08x", hkl);
			HKL hkl1 = LoadKeyboardLayout(szLoc, KLF_ACTIVATE|KLF_REORDER|KLF_SUBSTITUTE_OK|KLF_SETFORPROCESS);
			HKL hkl2 = ActivateKeyboardLayout(hkl1, KLF_SETFORPROCESS|KLF_REORDER);
			if (!hkl2) {
				dwLastError = GetLastError();
			}
			dwLastError = dwLastError;
		}

	//} else if (pIn->hdr.nCmd == CMD_DEFFONT) {
	//	// исключение - асинхронный, результат не требуется
	//	SetConsoleFontSizeTo(FarHwnd, 4, 6);
	//	MoveWindow(FarHwnd, 0, 0, GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN), 1); // чтобы убрать возможные полосы прокрутки...

	} else if (pIn->hdr.nCmd == CMD_REQTABS || pIn->hdr.nCmd == CMD_SETWINDOW) {
		MSectionLock SC; SC.Lock(csTabs, FALSE, 1000);

		if (pIn->hdr.nCmd == CMD_SETWINDOW)
		{
			ResetEvent(ghSetWndSendTabsEvent);

			// Для FAR2 - сброс QSearch выполняется в том же макро, в котором актирируется окно
			if (gFarVersion.dwVerMajor == 1 && pIn->dwData[1])
			{
				// А вот для FAR1 - нужно шаманить
				ProcessCommand(CMD_CLOSEQSEARCH, TRUE/*bReqMainThread*/, pIn->dwData/*хоть и не нужно?*/);
			}

			// Пересылается 2 DWORD
			ProcessCommand(pIn->hdr.nCmd, TRUE/*bReqMainThread*/, pIn->dwData);

			if (gFarVersion.dwVerMajor == 2)
			{
				WARNING("Почему для FAR1 не ждем?");
				DWORD nTimeout = 2000;
				#ifdef _DEBUG
				if (IsDebuggerPresent()) nTimeout = 120000;
				#endif
				WaitForSingleObject(ghSetWndSendTabsEvent, nTimeout);
			}
		}

		if (gpTabs) { 
			fSuccess = WriteFile( hPipe, gpTabs, gpTabs->hdr.cbSize, &cbWritten, NULL);
		}

		SC.Unlock();

	} else if (pIn->hdr.nCmd == CMD_SETENVVAR) {
		// Установить переменные окружения
		// Плагин это получает в ответ на CECMD_RESOURCES, посланное в GUI при загрузке плагина
		_ASSERTE(nDataSize>=8);
		//wchar_t *pszName  = (wchar_t*)pIn->Data;
		
	    FAR_REQ_SETENVVAR *pSetEnvVar = (FAR_REQ_SETENVVAR*)pIn->Data;
	    wchar_t *pszName = pSetEnvVar->szEnv;
	    
	    gbFARuseASCIIsort = pSetEnvVar->bFARuseASCIIsort;
		gbShellNoZoneCheck = pSetEnvVar->bShellNoZoneCheck;
		

		_ASSERTE(nDataSize<sizeof(gsMonitorEnvVar));
		gbMonitorEnvVar = false;
		// Плагин FarCall "нарушает" COMSPEC (копирует содержимое запускаемого процесса)
		bool lbOk = false;
		if (nDataSize<sizeof(gsMonitorEnvVar)) {
			memcpy(gsMonitorEnvVar, pszName, nDataSize);
			lbOk = true;
		}

		UpdateEnvVar(pszName);

		//while (*pszName && *pszValue) {
		//	const wchar_t* pszChanged = pszValue;
		//	// Для ConEmuOutput == AUTO выбирается по версии ФАРа
		//	if (!lstrcmpi(pszName, L"ConEmuOutput") && !lstrcmp(pszChanged, L"AUTO")) {
		//		if (gFarVersion.dwVerMajor==1)
		//			pszChanged = L"ANSI";
		//		else
		//			pszChanged = L"UNICODE";
		//	}
		//	// Если в pszValue пустая строка - удаление переменной
		//	SetEnvironmentVariableW(pszName, (*pszChanged != 0) ? pszChanged : NULL);
		//	//
		//	pszName = pszValue + lstrlenW(pszValue) + 1;
		//	if (*pszName == 0) break;
		//	pszValue = pszName + lstrlenW(pszName) + 1;
		//}

		gbMonitorEnvVar = lbOk;

	} else if (pIn->hdr.nCmd == CMD_DRAGFROM ) {
		#ifdef _DEBUG
			BOOL  *pbClickNeed = (BOOL*)pIn->Data;
			COORD *crMouse = (COORD *)(pbClickNeed+1);
		#endif
		
		ProcessCommand(CMD_LEFTCLKSYNC, TRUE/*bReqMainThread*/, pIn->Data);

		CESERVER_REQ* pCmdRet = NULL;
		ProcessCommand(pIn->hdr.nCmd, TRUE/*bReqMainThread*/, pIn->Data, &pCmdRet);

		if (pCmdRet)
		{
			fSuccess = WriteFile( hPipe, pCmdRet, pCmdRet->hdr.cbSize, &cbWritten, NULL);
			Free(pCmdRet);
		}
		//if (gpCmdRet && gpCmdRet == pCmdRet)
		//{
		//	Free(gpCmdRet);
		//	gpCmdRet = NULL; gpData = NULL; gpCursor = NULL;
		//}
		
	} else if (pIn->hdr.nCmd == CMD_EMENU) {
		COORD *crMouse = (COORD *)pIn->Data;

		#ifdef _DEBUG
		const wchar_t *pszUserMacro = (wchar_t*)(crMouse+1);
		#endif

		struct {
			BOOL  bClickNeed;
			COORD crMouse;
		} DragArg;
		DragArg.bClickNeed = TRUE;
		DragArg.crMouse = *crMouse;

		// Выделить файл под курсором
		DEBUGSTRMENU(L"\n*** ServerThreadCommand->ProcessCommand(CMD_LEFTCLKSYNC) begin\n");
		ProcessCommand(CMD_LEFTCLKSYNC, TRUE/*bReqMainThread*/, &DragArg/*pIn->Data*/);
		DEBUGSTRMENU(L"\n*** ServerThreadCommand->ProcessCommand(CMD_LEFTCLKSYNC) done\n");
		
		// А теперь, собственно вызовем меню
		DEBUGSTRMENU(L"\n*** ServerThreadCommand->ProcessCommand(CMD_EMENU) begin\n");
		ProcessCommand(pIn->hdr.nCmd, TRUE/*bReqMainThread*/, pIn->Data);
		DEBUGSTRMENU(L"\n*** ServerThreadCommand->ProcessCommand(CMD_EMENU) done\n");


	}
	else
	{
		CESERVER_REQ* pCmdRet = NULL;
		ProcessCommand(pIn->hdr.nCmd, TRUE/*bReqMainThread*/, pIn->Data, &pCmdRet);

		if (pCmdRet)
		{
			fSuccess = WriteFile( hPipe, pCmdRet, pCmdRet->hdr.cbSize, &cbWritten, NULL);
			Free(pCmdRet);
		}
		//if (gpCmdRet && gpCmdRet == pCmdRet) {
		//	Free(gpCmdRet);
		//	gpCmdRet = NULL; gpData = NULL; gpCursor = NULL;
		//}
	}


    // Освободить память
	if (((LPVOID)cbBuffer) != ((LPVOID)pIn))
		Free(pIn);

    CloseHandle(hPipe);
    return 0;
}

void ShowPluginMenu(int nID /*= -1*/)
{
	int nItem = -1;
	if (!FarHwnd)
	{
		ShowMessage(CEInvalidConHwnd,1); // "ConEmu plugin\nGetConsoleWindow()==FarHwnd is NULL\nOK"
		return;
	}

	if (IsTerminalMode())
	{
		ShowMessage(CEUnavailableInTerminal,1); // "ConEmu plugin\nConEmu is not available in terminal mode\nCheck TERM environment variable\nOK"
		return;
	}

	CheckConEmuDetached();

	if (nID != -1)
	{
		nItem = nID;
		if (nItem >= 2) nItem++; //Separator
		if (nItem >= 7) nItem++; //Separator
		if (nItem >= 9) nItem++; //Separator
		SHOWDBGINFO(L"*** ShowPluginMenu used default item\n");
	}
	else if (gFarVersion.dwVerMajor==1)
	{
		SHOWDBGINFO(L"*** calling ShowPluginMenuA\n");
		nItem = ShowPluginMenuA();
	}
	else if (gFarVersion.dwBuild>=FAR_Y_VER)
	{
		SHOWDBGINFO(L"*** calling ShowPluginMenuWY\n");
		nItem = FUNC_Y(ShowPluginMenu)();
	}
	else
	{
		SHOWDBGINFO(L"*** calling ShowPluginMenuWX\n");
		nItem = FUNC_X(ShowPluginMenu)();
	}

	if (nItem < 0) {
		SHOWDBGINFO(L"*** ShowPluginMenu cancelled, nItem < 0\n");
		return;
	}

	#ifdef _DEBUG
	wchar_t szInfo[128]; wsprintf(szInfo, L"*** ShowPluginMenu done, nItem == %i\n", nItem);
	SHOWDBGINFO(szInfo);
	#endif

	switch (nItem) {
		case 0: case 1:
		{ // Открыть в редакторе вывод последней консольной программы
			CESERVER_REQ* pIn = (CESERVER_REQ*)calloc(sizeof(CESERVER_REQ_HDR)+4,1);
			if (!pIn) return;
			CESERVER_REQ* pOut = NULL;
			ExecutePrepareCmd(pIn, CECMD_GETOUTPUTFILE, sizeof(CESERVER_REQ_HDR)+4);
			pIn->OutputFile.bUnicode = (gFarVersion.dwVerMajor>=2);
			pOut = ExecuteGuiCmd(FarHwnd, pIn, FarHwnd);
			if (pOut) {
				if (pOut->OutputFile.szFilePathName[0]) {
					BOOL lbRc = FALSE;
					if (gFarVersion.dwVerMajor==1)
						lbRc = EditOutputA(pOut->OutputFile.szFilePathName, (nItem==1));
					else if (gFarVersion.dwBuild>=FAR_Y_VER)
						lbRc = FUNC_Y(EditOutput)(pOut->OutputFile.szFilePathName, (nItem==1));
					else
						lbRc = FUNC_X(EditOutput)(pOut->OutputFile.szFilePathName, (nItem==1));
					if (!lbRc) {
						DeleteFile(pOut->OutputFile.szFilePathName);
					}
				}
				ExecuteFreeResult(pOut);
			}
			free(pIn);
		} break;
		case 3: // Показать/спрятать табы
		case 4: case 5: case 6:
		{
			CESERVER_REQ in, *pOut = NULL;
			ExecutePrepareCmd(&in, CECMD_TABSCMD, sizeof(CESERVER_REQ_HDR)+1);
			in.Data[0] = nItem - 3;
			pOut = ExecuteGuiCmd(FarHwnd, &in, FarHwnd);
			if (pOut) ExecuteFreeResult(pOut);
		} break;
		case 8: // Attach to GUI (если FAR был CtrlAltTab)
		{
			if (TerminalMode) break; // низзя
			if (ConEmuHwnd && IsWindow(ConEmuHwnd)) break; // Мы и так подключены?
			Attach2Gui();
		} break;
		//#ifdef _DEBUG
		//case 11: // Start "ConEmuC.exe /DEBUGPID="
		//#else
		case 10: // Start "ConEmuC.exe /DEBUGPID="
		//#endif
		{
			if (TerminalMode) break; // низзя
			StartDebugger();
		}
	}
}

BOOL FindServerCmd(DWORD nServerCmd, DWORD &dwServerPID)
{
	BOOL lbRc = FALSE;
	DWORD nProcessCount = 0, nProcesses[100] = {0};
	
	dwServerPID = 0;

	typedef DWORD (WINAPI* FGetConsoleProcessList)(LPDWORD lpdwProcessList, DWORD dwProcessCount);
	FGetConsoleProcessList pfnGetConsoleProcessList = NULL;
	HMODULE hKernel = GetModuleHandleW (L"kernel32.dll");

	if (hKernel)
	{
		pfnGetConsoleProcessList = (FGetConsoleProcessList)GetProcAddress (hKernel, "GetConsoleProcessList");
	}

	
	BOOL lbWin2kMode = (pfnGetConsoleProcessList == NULL);
    
	if (!lbWin2kMode)
	{

		if (pfnGetConsoleProcessList)
		{
    		nProcessCount = pfnGetConsoleProcessList(nProcesses, countof(nProcesses));
    		if (nProcessCount && nProcessCount > countof(nProcesses)) {
    			_ASSERTE(nProcessCount <= countof(nProcesses));
    			nProcessCount = 0;
    		}
		}
	}

	if (lbWin2kMode)
	{
		DWORD nSelfPID = GetCurrentProcessId();
		HANDLE hSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS,0);
		if (lbWin2kMode)
		{
			if (hSnap != INVALID_HANDLE_VALUE)
			{
				PROCESSENTRY32 prc = {sizeof(PROCESSENTRY32)};
				if (Process32First(hSnap, &prc))
				{
					do
					{
						if (prc.th32ProcessID == nSelfPID)
						{
							nProcesses[0] = prc.th32ParentProcessID;
							nProcesses[1] = nSelfPID;
							nProcessCount = 2;
							break;
						}
					} while (!dwServerPID && Process32Next(hSnap, &prc));
				}
				CloseHandle(hSnap);
			}
		}
	}
	
	if (nProcessCount >= 2)
	{
		DWORD nParentPID = 0;
		DWORD nSelfPID = GetCurrentProcessId();
		HANDLE hSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS,0);
		if (hSnap != INVALID_HANDLE_VALUE)
		{
			PROCESSENTRY32 prc = {sizeof(PROCESSENTRY32)};
			if (Process32First(hSnap, &prc))
			{
				do
				{
            		for (UINT i = 0; i < nProcessCount; i++)
					{
            			if (prc.th32ProcessID != nSelfPID
							&& prc.th32ProcessID == nProcesses[i])
        				{
        					if (lstrcmpiW(prc.szExeFile, L"conemuc.exe")==0 
        						/*|| lstrcmpiW(prc.szExeFile, L"conemuc64.exe")==0*/)
							{
        						CESERVER_REQ* pIn = ExecuteNewCmd(nServerCmd, sizeof(CESERVER_REQ_HDR)+sizeof(DWORD));
        						pIn->dwData[0] = GetCurrentProcessId();
        						CESERVER_REQ* pOut = ExecuteSrvCmd(prc.th32ProcessID, pIn, FarHwnd);
        						if (pOut) dwServerPID = prc.th32ProcessID;
        						ExecuteFreeResult(pIn); ExecuteFreeResult(pOut);
        						// Если команда успешно выполнена - выходим
        						if (dwServerPID)
								{
        							lbRc = TRUE;
        							break;
    							}
        					}
            			}
            		}
				} while (!dwServerPID && Process32Next(hSnap, &prc));
			}
			CloseHandle(hSnap);
		}
	}
	
	return lbRc;
}

BOOL Attach2Gui()
{
	DWORD dwServerPID = 0;
	
	if (FindServerCmd(CECMD_ATTACH2GUI, dwServerPID) && dwServerPID != 0)
	{
		// "Server was already started. PID=%i. Exiting...\n", dwServerPID
		gdwServerPID = dwServerPID;
		gbTryOpenMapHeader = (gpConMapInfo==NULL);
		if (gpConMapInfo) // 04.03.2010 Maks - Если мэппинг уже открыт - принудительно передернуть ресурсы и информацию
			CheckResources(TRUE);
		return TRUE;
	}
	gdwServerPID = 0;

	//TODO("У сервера пока не получается менять шрифт в консоли, которую создал FAR");
	//SetConsoleFontSizeTo(GetConsoleWindow(), 6, 4, L"Lucida Console");
	
	// Create process, with flag /Attach GetCurrentProcessId()
	// Sleep for sometimes, try InitHWND(hConWnd); several times
	WCHAR  szExe[0x200] = {0};
	BOOL lbRc = FALSE;
	
	DWORD nLen = 0;
	PROCESS_INFORMATION pi; memset(&pi, 0, sizeof(pi));
	STARTUPINFO si; memset(&si, 0, sizeof(si));
	si.cb = sizeof(si);
	DWORD dwSelfPID = GetCurrentProcessId();
	wchar_t* pszSlash = NULL;
	
	szExe[0] = L'"';
	if ((nLen = GetEnvironmentVariableW(L"ConEmuBaseDir", szExe+1, MAX_PATH)) > 0)
	{
		if (szExe[nLen] != L'\\') { szExe[nLen+1] = L'\\'; szExe[nLen+2] = 0; }
	}
	else
	{
		if (!GetModuleFileName(0, szExe+1, MAX_PATH) || !(pszSlash = wcsrchr ( szExe, L'\\' )))
		{
			ShowMessageGui(CECantStartServer2, MB_ICONSTOP|MB_SYSTEMMODAL);
			return FALSE;
		}
		//if ((nLen=GetModuleFileName(0, szExe+1, MAX_PATH)) > 0)
		//wchar_t* pszSlash = wcsrchr ( szExe, L'\\' );
		//if (pszSlash)
		//{
		pszSlash[1] = 0;
		//lstrcpyW(pszSlash+1, L"ConEmu\\ConEmuC.exe");
		//if (!FileExists(szExe+1))
		//{
		//	lstrcpyW(pszSlash+1, L"ConEmuC.exe");
		//}
		//}
	}
	pszSlash = szExe + lstrlenW(szExe);

	BOOL lbFound = FALSE;

	// Для фанатов 64-битных версий
	#ifdef WIN64
	if (!lbFound)
	{
		lstrcpyW(pszSlash, L"ConEmu\\ConEmuC64.exe");
		lbFound = FileExists(szExe+1);
	}
	if (!lbFound)
	{
		lstrcpyW(pszSlash, L"ConEmuC64.exe");
		lbFound = FileExists(szExe+1);
	}
	#endif
	if (!lbFound)
	{
		lstrcpyW(pszSlash, L"ConEmu\\ConEmuC.exe");
		lbFound = FileExists(szExe+1);
	}
	if (!lbFound)
	{
		lstrcpyW(pszSlash, L"ConEmuC.exe");
		lbFound = FileExists(szExe+1);
	}
	if (!lbFound)
	{
		TODO("GUI MessageBox! ServerNotFound!");
		ShowMessageGui(CECantStartServer3, MB_ICONSTOP|MB_SYSTEMMODAL);
		return FALSE;
	}
	
	//if (IsWindows64())
	//	wsprintf(szExe+lstrlenW(szExe), L"ConEmuC64.exe\" /ATTACH /PID=%i", dwSelfPID);
	//else

	wsprintf(szExe+lstrlenW(szExe), L"\" /ATTACH /FARPID=%i", dwSelfPID);
	
	
	if (!CreateProcess(NULL, szExe, NULL, NULL, FALSE, NORMAL_PRIORITY_CLASS, NULL,
			NULL, &si, &pi))
	{
		// Хорошо бы ошибку показать?
		ShowMessageGui(CECantStartServer, MB_ICONSTOP|MB_SYSTEMMODAL); // "ConEmu plugin\nCan't start console server process (ConEmuC.exe)\nOK"
	}
	else
	{
		gdwServerPID = pi.dwProcessId;
		lbRc = TRUE;
		// Чтобы MonitorThread пытался открыть Mapping
		gbTryOpenMapHeader = (gpConMapInfo==NULL);
	}
	
	return lbRc;
}

BOOL StartDebugger()
{
	if (IsDebuggerPresent())
	{
		ShowMessage(CEAlreadyDebuggerPresent,1); // "ConEmu plugin\nDebugger is already attached to current process\nOK"
		return FALSE; // Уже
	}
	if (IsTerminalMode())
	{
		ShowMessage(CECantDebugInTerminal,1); // "ConEmu plugin\nDebugger is not available in terminal mode\nOK"
		return FALSE; // Уже
	}
		
	//DWORD dwServerPID = 0;
	
	// Create process, with flag /Attach GetCurrentProcessId()
	// Sleep for sometimes, try InitHWND(hConWnd); several times
	wchar_t  szExe[MAX_PATH*3] = {0};
	wchar_t  szConEmuC[MAX_PATH];
	BOOL lbRc = FALSE;
	
	DWORD nLen = 0;
	PROCESS_INFORMATION pi; memset(&pi, 0, sizeof(pi));
	STARTUPINFO si; memset(&si, 0, sizeof(si));
	si.cb = sizeof(si);
	DWORD dwSelfPID = GetCurrentProcessId();
	
	if ((nLen = GetEnvironmentVariableW(L"ConEmuBaseDir", szConEmuC, MAX_PATH-16)) < 1)
	{
		ShowMessage(CECantDebugNotEnvVar,1); // "ConEmu plugin\nEnvironment variable 'ConEmuBaseDir' not defined\nDebugger is not available\nOK"
		return FALSE; // Облом
	}
	lstrcatW(szConEmuC, L"\\ConEmuC.exe");
	if (!FileExists(szConEmuC))
	{
		wchar_t* pszSlash = NULL;
		if (((nLen=GetModuleFileName(0, szConEmuC, MAX_PATH-24)) < 1) || ((pszSlash = wcsrchr ( szConEmuC, L'\\' )) == NULL))
		{
			ShowMessage(CECantDebugNotEnvVar,1); // "ConEmu plugin\nEnvironment variable 'ConEmuBaseDir' not defined\nDebugger is not available\nOK"
			return FALSE; // Облом
		}
		lstrcpyW(pszSlash, L"\\ConEmu\\ConEmuC.exe");
		if (!FileExists(szConEmuC))
		{
			lstrcpyW(pszSlash, L"\\ConEmuC.exe");
			if (!FileExists(szConEmuC))
			{
				ShowMessage(CECantDebugNotEnvVar,1); // "ConEmu plugin\nEnvironment variable 'ConEmuBaseDir' not defined\nDebugger is not available\nOK"
				return FALSE; // Облом
			}
		}		
	}
	
	int w = 80, h = 25;
	CONSOLE_SCREEN_BUFFER_INFO csbi;
	if (GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &csbi))
	{
		w = csbi.srWindow.Right - csbi.srWindow.Left + 1;
		h = csbi.srWindow.Bottom - csbi.srWindow.Top + 1;
	}
	
	if (ConEmuHwnd)
	{
		wsprintf(szExe, L"\"%s\" /ATTACH /ROOT \"%s\" /DEBUGPID=%i /BW=%i /BH=%i /BZ=9999", 
			szConEmuC, szConEmuC, dwSelfPID, w, h);
	}
	else
	{
		wsprintf(szExe, L"\"%s\" /DEBUGPID=%i /BW=%i /BH=%i /BZ=9999", 
			szConEmuC, dwSelfPID, w, h);
	}
	
	if (ConEmuHwnd)
	{
		si.dwFlags |= STARTF_USESHOWWINDOW;
		si.wShowWindow = SW_HIDE;
	}
	
	if (!CreateProcess(NULL, szExe, NULL, NULL, FALSE, NORMAL_PRIORITY_CLASS|CREATE_NEW_CONSOLE, NULL,
			NULL, &si, &pi))
	{
		// Хорошо бы ошибку показать?
		#ifdef _DEBUG
		DWORD dwErr = GetLastError();
		#endif
		ShowMessage(CECantStartDebugger,1); // "ConEmu plugin\nНе удалось запустить процесс отладчика\nOK"
	} else {
		lbRc = TRUE;
	}
	
	return lbRc;
}


BOOL IsMacroActive()
{
	if (!FarHwnd) return FALSE;

	BOOL lbActive = FALSE;
	if (gFarVersion.dwVerMajor==1)
		lbActive = IsMacroActiveA();
	else if (gFarVersion.dwBuild>=FAR_Y_VER)
		lbActive = FUNC_Y(IsMacroActive)();
	else
		lbActive = FUNC_X(IsMacroActive)();

	return lbActive;
}


void RedrawAll()
{
	if (!FarHwnd) return;

	if (gFarVersion.dwVerMajor==1)
		RedrawAllA();
	else if (gFarVersion.dwBuild>=FAR_Y_VER)
		FUNC_Y(RedrawAll)();
	else
		FUNC_X(RedrawAll)();
}

DWORD GetEditorModifiedState()
{
	if (gFarVersion.dwVerMajor==1)
		return GetEditorModifiedStateA();
	else if (gFarVersion.dwBuild>=FAR_Y_VER)
		return FUNC_Y(GetEditorModifiedState)();
	else
		return FUNC_X(GetEditorModifiedState)();
}

int GetActiveWindowType()
{
	if (gFarVersion.dwVerMajor==1)
		return GetActiveWindowTypeA();
	else if (gFarVersion.dwBuild>=FAR_Y_VER)
		return FUNC_Y(GetActiveWindowType)();
	else
		return FUNC_X(GetActiveWindowType)();
}

//void ExecuteQuitFar()
//{
//	if (gFarVersion.dwVerMajor==1 || gFarVersion.dwBuild < 1348)
//		ExecuteQuitFarA();
//	else if (gFarVersion.dwBuild>=FAR_Y_VER)
//		FUNC_Y(ExecuteQuitFar)();
//	else
//		FUNC_X(ExecuteQuitFar)();
//}


bool RunExternalProgramW(wchar_t* pszCommand, wchar_t* pszCurDir)
{	
	//wchar_t strCmd[MAX_PATH+1];
	//wchar_t* strArgs = pszCommand;
	//NextArg((const wchar_t**)&strArgs, strCmd);
	//wchar_t strDir[10]; lstrcpy(strDir, L"C:\\");
	STARTUPINFO cif={sizeof(STARTUPINFO)};
	PROCESS_INFORMATION pri={0};
	
	HANDLE hStdin = GetStdHandle(STD_INPUT_HANDLE);
	DWORD oldConsoleMode;
	DWORD nErr = 0;
	DWORD nExitCode = 0;
	GetConsoleMode(hStdin, &oldConsoleMode);
	SetConsoleMode(hStdin, ENABLE_ECHO_INPUT | ENABLE_LINE_INPUT | ENABLE_PROCESSED_INPUT); // подбиралось методом тыка

	#ifdef _DEBUG
	WARNING("Посмотреть, как Update в консоль выводит.");
	wprintf(L"\nCmd: <%s>\nDir: <%s>\n\n", pszCommand, pszCurDir);
	#endif
	
	MWow64Disable wow; wow.Disable();
	
	SetLastError(0);
	BOOL lb = CreateProcess( /*strCmd, strArgs,*/ NULL, pszCommand, NULL, NULL, TRUE, 
		NORMAL_PRIORITY_CLASS|CREATE_DEFAULT_ERROR_MODE, NULL, pszCurDir, &cif, &pri );
	nErr = GetLastError();
	
	wow.Restore();
	
	if (lb)
	{
		WaitForSingleObject( pri.hProcess, INFINITE );
		GetExitCodeProcess(pri.hProcess, &nExitCode);
		CloseHandle( pri.hProcess );
		CloseHandle( pri.hThread );         
		#ifdef _DEBUG
		wprintf(L"\nConEmuC: Process was terminated, ExitCode=%i\n\n", nExitCode);
		#endif
	}
	else
	{
		#ifdef _DEBUG
		wprintf(L"\nConEmuC: CreateProcess failed, ErrCode=0x%08X\n\n", nErr);
		#endif
	}
	//wprintf(L"Cmd: <%s>\nArg: <%s>\nDir: <%s>\n\n", strCmd, strArgs, pszCurDir);

	SetConsoleMode(hStdin, oldConsoleMode);
	return true;
}



// <Name>\0<Value>\0<Name2>\0<Value2>\0\0
void UpdateEnvVar(const wchar_t* pszList)
{
	const wchar_t *pszName  = (wchar_t*)pszList;
	const wchar_t *pszValue = pszName + lstrlenW(pszName) + 1;

	while (*pszName && *pszValue) {
		const wchar_t* pszChanged = pszValue;
		// Для ConEmuOutput == AUTO выбирается по версии ФАРа
		if (!lstrcmpi(pszName, L"ConEmuOutput") && !lstrcmp(pszChanged, L"AUTO")) {
			if (gFarVersion.dwVerMajor==1)
				pszChanged = L"ANSI";
			else
				pszChanged = L"UNICODE";
		}
		// Если в pszValue пустая строка - удаление переменной
		SetEnvironmentVariableW(pszName, (*pszChanged != 0) ? pszChanged : NULL);
		//
		pszName = pszValue + lstrlenW(pszValue) + 1;
		if (*pszName == 0) break;
		pszValue = pszName + lstrlenW(pszName) + 1;
	}
}

// Плагин может быть вызван в первый раз из фоновой нити.
// Поэтому простой "gnMainThreadId = GetCurrentThreadId();" не прокатит. Нужно искать первую нить процесса!
DWORD GetMainThreadId()
{
	DWORD nThreadID = 0;
	DWORD nProcID = GetCurrentProcessId();
	HANDLE h = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0);
	if (h != INVALID_HANDLE_VALUE)
	{
		THREADENTRY32 ti = {sizeof(THREADENTRY32)};
		if (Thread32First(h, &ti))
		{
			do {
				// Нужно найти ПЕРВУЮ нить процесса
				if (ti.th32OwnerProcessID == nProcID) {
					nThreadID = ti.th32ThreadID;
					break;
				}
			} while (Thread32Next(h, &ti));
		}
		CloseHandle(h);
	}

	// Нехорошо. Должна быть найдена. Вернем хоть что-то (текущую нить)
	if (!nThreadID) {
		_ASSERTE(nThreadID!=0);
		nThreadID = GetCurrentThreadId();
	}
	return nThreadID;
}

// Определены в SetHook.h
//typedef void (WINAPI* OnLibraryLoaded_t)(HMODULE ahModule);
//extern OnLibraryLoaded_t gfOnLibraryLoaded;

// Вызывается при загрузке dll
void WINAPI OnLibraryLoaded(HMODULE ahModule)
{
#ifdef _DEBUG
	wchar_t szModulePath[MAX_PATH]; szModulePath[0] = 0;
	GetModuleFileName(ahModule, szModulePath, MAX_PATH);
#endif

	//// Если GUI неактивно (запущен standalone FAR) - сразу выйти
	//if (ConEmuHwnd == NULL)
	//{
	//	return;
	//}

	WARNING("Нужно специально вызвать OnLibraryLoaded при аттаче к GUI");

	// Если определен калбэк инициализации ConEmu
	OnConEmuLoaded_t fnOnConEmuLoaded = NULL;
	BOOL lbSucceeded = FALSE;
	if ((fnOnConEmuLoaded = (OnConEmuLoaded_t)GetProcAddress(ahModule, "OnConEmuLoaded")) != NULL)
	{
		// Наверное, только для плагинов фара
		if (GetProcAddress(ahModule, "SetStartupInfoW") || GetProcAddress(ahModule, "SetStartupInfo"))
		{
			struct ConEmuLoadedArg arg; // = {sizeof(struct ConEmuLoadedArg)};
			FillLoadedParm(&arg, ahModule, TRUE);
			//arg.hPlugin = ahModule;
			//arg.hConEmu = ghPluginModule;
			//arg.hPlugin = ahModule;
			//arg.bLoaded = TRUE;
			//arg.bGuiActive = (ConEmuHwnd != NULL);
			//// Сервисные функции
			//arg.GetFarHWND = GetFarHWND;
			//arg.GetFarHWND2 = GetFarHWND2;
			//arg.GetFarVersion = GetFarVersion;
			//arg.IsTerminalMode = IsTerminalMode;
			//arg.IsConsoleActive = IsConsoleActive;
			//arg.RegisterPanelView = RegisterPanelView;
			//arg.RegisterBackground = RegisterBackground;
			//arg.ActivateConsole = ActivateConsole;
			//arg.SyncExecute = SyncExecute;

			SAFETRY {
				fnOnConEmuLoaded(&arg);
				lbSucceeded = TRUE;
			} SAFECATCH {
				// Failed
				_ASSERTE(lbSucceeded == TRUE);
			}
		}
	}
}

void LogCreateProcessCheck(LPCWSTR asLogFileName)
{
	if (asLogFileName == (LPCWSTR)-1)
	{
		//TODO: Загрузить из текущих параметров консоли <CESERVER_REQ_CONINFO_HDR>.sLogCreateProcess
		asLogFileName = NULL; // пока - только через CMD_LOG_SHELL
	}

	if (!ConEmuHwnd)
	{
		gsLogCreateProcess[0] = 0;
	}
	else
	{
		//DWORD dwGuiThreadId, dwGuiProcessId;
		//MFileMapping<ConEmuGuiInfo> GuiInfoMapping;
		//dwGuiThreadId = GetWindowThreadProcessId(ConEmuHwnd, &dwGuiProcessId);
		//if (!dwGuiThreadId)
		//{
		//	_ASSERTE(dwGuiProcessId);
		//	gsLogCreateProcess[0] = 0;
		//}
		//else
		//{
		//	GuiInfoMapping.InitName(CEGUIINFOMAPNAME, dwGuiProcessId);
		//	const ConEmuGuiInfo* pInfo = GuiInfoMapping.Open();
		//	if (pInfo && pInfo->cbSize == sizeof(ConEmuGuiInfo))
		//	{
		//		_ASSERTE(countof(gsLogCreateProcess)==(MAX_PATH+1));
		//		gsLogCreateProcess[MAX_PATH] = 0;
		//		lstrcpynW(gsLogCreateProcess, pInfo->sLogCreateProcess, MAX_PATH);
		//	}
		//}
		if (!asLogFileName || !*asLogFileName)
		{
			gsLogCreateProcess[0] = 0;
		}
		else
		{
			_ASSERTE(countof(gsLogCreateProcess)==(MAX_PATH+1));
			gsLogCreateProcess[MAX_PATH] = 0;
			lstrcpynW(gsLogCreateProcess, asLogFileName, MAX_PATH);

			TODO("Осталось включить калбэки для шелловских функций");
		}
	}
}

BOOL CheckCallbackPtr(HMODULE hModule, FARPROC CallBack, BOOL abCheckModuleInfo)
{
	if (!hModule)
	{
		_ASSERTE(hModule!=NULL);
		return FALSE;
	}

	DWORD_PTR nModulePtr = (DWORD_PTR)hModule;
	DWORD_PTR nModuleSize = (4<<20);
	BOOL lbModuleInformation = FALSE;

	if (abCheckModuleInfo)
	{
		static HMODULE hPsApi = NULL;
		if (hPsApi == NULL)
		{
			hPsApi = LoadLibrary(L"Psapi.dll");
			if (hPsApi == NULL)
				hPsApi = (HMODULE)-1;
		}
		if (hPsApi && hPsApi != (HMODULE)-1)
		{
			struct ModInfo {
			  LPVOID lpBaseOfDll;
			  DWORD  SizeOfImage;
			  LPVOID EntryPoint;
			} mi;
			typedef BOOL (WINAPI* GetModuleInformation_t)(HANDLE, HMODULE, struct ModInfo*, DWORD);
			GetModuleInformation_t GetModuleInformation = (GetModuleInformation_t)GetProcAddress(hPsApi, "GetModuleInformation");
			if (GetModuleInformation)
			{
				lbModuleInformation = GetModuleInformation(GetCurrentProcess(), hModule, &mi, sizeof(mi));
				if (!lbModuleInformation)
				{
					_ASSERTE(lbModuleInformation!=FALSE);
					return FALSE;
				}
				nModuleSize = mi.SizeOfImage;
			}
		}
	}

	if (!CallBack)
	{
		_ASSERTE(CallBack!=NULL);
		return FALSE;
	}
	if (((DWORD_PTR)CallBack) < nModulePtr)
	{
		_ASSERTE(((DWORD_PTR)CallBack) >= nModulePtr);
		return FALSE;
	}
	
	if (((DWORD_PTR)CallBack) > (nModuleSize + nModulePtr))
	{
		_ASSERTE(((DWORD_PTR)CallBack) <= (nModuleSize + nModulePtr));
		return FALSE;
	}
	return TRUE;
}

//LPTOP_LEVEL_EXCEPTION_FILTER gpPrevExFilter = NULL;
//// 
//LONG WINAPI MyExFilter(struct _EXCEPTION_POINTERS *ExceptionInfo)
//{
//	typedef BOOL (WINAPI* MiniDumpWriteDump_t)(HANDLE hProcess, DWORD ProcessId, HANDLE hFile, MINIDUMP_TYPE DumpType,
//		PMINIDUMP_EXCEPTION_INFORMATION ExceptionParam, PMINIDUMP_USER_STREAM_INFORMATION UserStreamParam,
//		PMINIDUMP_CALLBACK_INFORMATION CallbackParam);
//
//	HMODULE hDbghelp = NULL;
//	HANDLE hDmpFile = NULL;
//	MiniDumpWriteDump_t MiniDumpWriteDump_f = NULL;
//	wchar_t szDumpFile[MAX_PATH*2]; szDumpFile[0] = 0;
//	wchar_t* pszDumpName = szDumpFile;
//	if (GetTempPathW(MAX_PATH, szDumpFile))
//	{
//		int nLen = lstrlenW(szDumpFile);
//		if (nLen > 0 && szDumpFile[nLen-1] != L'\\')
//		{
//			szDumpFile[nLen++] = L'\\';
//			szDumpFile[nLen] = 0;
//			pszDumpName = szDumpFile+nLen;
//		}
//	}
//	wsprintfW(pszDumpName, L"Far-%u.mdmp", GetCurrentProcessId());
//
//	wchar_t szErrInfo[MAX_PATH*3], szTitle[128];
//	wsprintfW(szTitle, L"Far %i.%i build %i Unhandled Exception", gFarVersion.dwVerMajor, gFarVersion.dwVerMinor, gFarVersion.dwBuild);
//
//
//	hDmpFile = CreateFileW(szDumpFile, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL|FILE_FLAG_WRITE_THROUGH, NULL);
//	if (hDmpFile == INVALID_HANDLE_VALUE)
//	{
//		DWORD nErr = GetLastError();
//		wsprintfW(szErrInfo, L"Can't create debug dump file\n%s\nErrCode=0x%08X", szDumpFile, nErr);
//		MessageBoxW(NULL, szErrInfo, szTitle, MB_SYSTEMMODAL|MB_ICONSTOP);
//	}
//	else if ((hDbghelp = LoadLibraryW(L"Dbghelp.dll")) == NULL)
//	{
//		DWORD nErr = GetLastError();
//		wsprintfW(szErrInfo, L"Can't load debug library 'Dbghelp.dll'\nErrCode=0x%08X\n\nTry again?", nErr);
//		MessageBoxW(NULL, szErrInfo, szTitle, MB_SYSTEMMODAL|MB_ICONSTOP);
//	}
//	else
//	{
//		MiniDumpWriteDump_f = (MiniDumpWriteDump_t)GetProcAddress(hDbghelp, "MiniDumpWriteDump");
//		if (!MiniDumpWriteDump_f)
//		{
//			DWORD nErr = GetLastError();
//			wsprintfW(szErrInfo, L"Can't locate 'MiniDumpWriteDump' in library 'Dbghelp.dll'", nErr);
//			MessageBoxW(NULL, szErrInfo, szTitle, MB_SYSTEMMODAL|MB_ICONSTOP);
//		}
//		else
//		{
//			MINIDUMP_EXCEPTION_INFORMATION mei = {GetCurrentThreadId()};
//			mei.ExceptionPointers = ExceptionInfo;
//			mei.ClientPointers = TRUE;
//			PMINIDUMP_EXCEPTION_INFORMATION pmei = NULL; // пока
//			BOOL lbDumpRc = MiniDumpWriteDump_f(
//				GetCurrentProcess(), GetCurrentProcessId(),
//				hDmpFile, 
//				MiniDumpNormal /*MiniDumpWithDataSegs*/,
//				pmei,
//				NULL, NULL);
//			if (!lbDumpRc)
//			{
//				DWORD nErr = GetLastError();
//				wsprintfW(szErrInfo, L"MiniDumpWriteDump failed.\nErrorCode=0x%08X", nErr);
//				MessageBoxW(NULL, szErrInfo, szTitle, MB_SYSTEMMODAL|MB_ICONSTOP);
//			}
//			else
//			{
//				lstrcpyW(szErrInfo, L"MiniDump was saved\n");
//				lstrcatW(szErrInfo, szDumpFile);
//				MessageBoxW(NULL, szErrInfo, szTitle, MB_SYSTEMMODAL|MB_ICONSTOP);
//			}
//			CloseHandle(hDmpFile);
//		}
//	}
//
//	if (gpPrevExFilter)
//		return gpPrevExFilter(ExceptionInfo);
//	else
//		return EXCEPTION_EXECUTE_HANDLER;
//}
//
//
//void InstallTrapHandler()
//{
//	static bool bProcessed = false;
//	if (bProcessed)
//		return;
//	// выполняем только один раз
//	bProcessed = true;
//	//if (IsDebuggerPresent())
//	//	return; // под дебаггером - не нужно
//
//	if (gpConMapInfo->nLogLevel > 0)
//	{
//		gpPrevExFilter = SetUnhandledExceptionFilter(MyExFilter);
//	}
//}


/* Используются как extern в ConEmuCheck.cpp */
LPVOID _calloc(size_t nCount,size_t nSize) {
	return calloc(nCount,nSize);
}
LPVOID _malloc(size_t nCount) {
	return malloc(nCount);
}
void   _free(LPVOID ptr) {
	free(ptr);
}
