
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
#include "../common/FarVersion.h"
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
void WINAPI GetPluginInfoWcmn(void *piv)
{
	Plugin()->GetPluginInfo(piv);
}

HANDLE WINAPI OpenPluginW(int OpenFrom, INT_PTR Item)
{
	CPluginBase* p = Plugin();
	HANDLE hPlugin = p->OpenPluginCommon(OpenFrom, Item, ((OpenFrom & p->of_FromMacro) == p->of_FromMacro));
	return hPlugin;
}

DWORD gnPeekReadCount = 0;






BOOL OnPanelViewCallbacks(HookCallbackArg* pArgs, PanelViewInputCallback pfnLeft, PanelViewInputCallback pfnRight)
{
	if (!pArgs->bMainThread || !(pfnLeft || pfnRight))
	{
		_ASSERTE(pArgs->bMainThread && (pfnLeft || pfnRight));
		return TRUE; // перехват делаем только в основной нити
	}

	BOOL lbNewResult = FALSE, lbContinue = TRUE;
	HANDLE hInput = (HANDLE)(pArgs->lArguments[0]);
	PINPUT_RECORD p = (PINPUT_RECORD)(pArgs->lArguments[1]);
	DWORD nBufSize = (DWORD)(pArgs->lArguments[2]);
	LPDWORD pCount = (LPDWORD)(pArgs->lArguments[3]);

	if (lbContinue && pfnLeft)
	{
		_ASSERTE(gPanelRegLeft.bRegister);
		lbContinue = pfnLeft(hInput,p,nBufSize,pCount,&lbNewResult);

		if (!lbContinue)
			*((BOOL*)pArgs->lpResult) = lbNewResult;
	}

	// Если есть только правая панель, или на правой панели задана другая функция
	if (lbContinue && pfnRight && pfnRight != pfnLeft)
	{
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
		ShowMessage(CEShellExecuteException,0);
	}

	*((LPBOOL*)pArgs->lpResult) = FALSE;
	SetLastError(E_UNEXPECTED);
}


// Для определения "живости" фара
VOID WINAPI OnGetNumberOfConsoleInputEventsPost(HookCallbackArg* pArgs)
{
	if (pArgs->bMainThread && gpConMapInfo && gpFarInfo && gpFarInfoMapping)
	{
		TouchReadPeekConsoleInputs(-1);
	}
}

BOOL UngetDummyMouseEvent(BOOL abRead, HookCallbackArg* pArgs)
{
	if (!(pArgs->lArguments[1] && pArgs->lArguments[2] && pArgs->lArguments[3]))
	{
		_ASSERTE(pArgs->lArguments[1] && pArgs->lArguments[2] && pArgs->lArguments[3]);
	}
	else if ((gLastMouseReadEvent.dwButtonState & (RIGHTMOST_BUTTON_PRESSED|FROM_LEFT_1ST_BUTTON_PRESSED)) || (gnDummyMouseEventFromMacro > 0))
	{
		// Такой финт нужен только в случае:
		// в редакторе идет скролл мышкой (скролл - зажатой кнопкой на заголовке/кейбаре)
		// нужно заставить фар остановить скролл, иначе активация Synchro невозможна

		// Или второй случай
		//FAR BUGBUG: Макрос не запускается на исполнение, пока мышкой не дернем :(
		//  Это чаще всего проявляется при вызове меню по RClick
		//  Если курсор на другой панели, то RClick сразу по пассивной
		//  не вызывает отрисовку :(

		if ((gnAllowDummyMouseEvent < 1) && (gnDummyMouseEventFromMacro < 1))
		{
			_ASSERTE(gnAllowDummyMouseEvent >= 1);
			if (gnAllowDummyMouseEvent < 0)
				gnAllowDummyMouseEvent = 0;
			gbUngetDummyMouseEvent = FALSE;
			return FALSE;
		}

		// Сообщить в GUI что мы "пустое" сообщение фару кидаем
		if (gFarMode.bFarHookMode && gFarMode.bMonitorConsoleInput)
		{
			CESERVER_REQ *pIn = ExecuteNewCmd(CECMD_PEEKREADINFO, sizeof(CESERVER_REQ_HDR)+sizeof(CESERVER_REQ_PEEKREADINFO));
			if (pIn)
			{
				pIn->PeekReadInfo.nCount = (WORD)1;
				pIn->PeekReadInfo.cPeekRead = '*';
				pIn->PeekReadInfo.cUnicode = 'U';
				pIn->PeekReadInfo.h = (HANDLE)pArgs->lArguments[1];
				pIn->PeekReadInfo.nTID = GetCurrentThreadId();
				pIn->PeekReadInfo.nPID = GetCurrentProcessId();
				pIn->PeekReadInfo.bMainThread = (pIn->PeekReadInfo.nTID == gnMainThreadId);

				pIn->PeekReadInfo.Buffer->EventType = MOUSE_EVENT;
				pIn->PeekReadInfo.Buffer->Event.MouseEvent = gLastMouseReadEvent;
				pIn->PeekReadInfo.Buffer->Event.MouseEvent.dwButtonState = 0;
				pIn->PeekReadInfo.Buffer->Event.MouseEvent.dwEventFlags = MOUSE_MOVED;

				CESERVER_REQ* pOut = ExecuteGuiCmd(FarHwnd, pIn, FarHwnd);
				if (pOut) ExecuteFreeResult(pOut);
				ExecuteFreeResult(pIn);
			}
		}

		PINPUT_RECORD p = (PINPUT_RECORD)(pArgs->lArguments[1]);
		LPDWORD pCount = (LPDWORD)(pArgs->lArguments[3]);
		*pCount = 1;
		p->EventType = MOUSE_EVENT;
		p->Event.MouseEvent = gLastMouseReadEvent;
		p->Event.MouseEvent.dwButtonState = 0;
		p->Event.MouseEvent.dwEventFlags = MOUSE_MOVED;
		*((LPBOOL)pArgs->lpResult) = TRUE;

		if ((gnDummyMouseEventFromMacro > 0) && abRead)
		{
			TODO("А если в очередь фара закинуто несколько макросов? По одному мышиному события выполнится только один, или все?");
			//InterlockedDecrement(&gnDummyMouseEventFromMacro);
			gnDummyMouseEventFromMacro = 0;
		}

		return TRUE;
	}
	else
	{
		gbUngetDummyMouseEvent = FALSE; // Не требуется, фар сам кнопку "отпустил"
	}
	return FALSE;
}

// Если функция возвращает FALSE - реальный ReadConsoleInput вызван не будет,
// и в вызывающую функцию (ФАРа?) вернется то, что проставлено в pArgs->lpResult & pArgs->lArguments[...]
BOOL WINAPI OnConsolePeekInput(HookCallbackArg* pArgs)
{
	if (!pArgs->bMainThread)
		return TRUE;  // обработку делаем только в основной нити

	if (gbUngetDummyMouseEvent)
	{
		if (UngetDummyMouseEvent(FALSE, pArgs))
			return FALSE; // реальный ReadConsoleInput вызван не будет
	}

	//// Выставить флажок "Жив" можно и при вызове из плагина
	//if (gpConMapInfo && gpFarInfo && gpFarInfoMapping)
	//	TouchReadPeekConsoleInputs(1);

	//if (pArgs->IsExecutable != HEO_Executable)
	//	return TRUE;  // и только при вызове из far.exe

	if (pArgs->lArguments[2] == 1)
	{
		OnConsolePeekReadInput(TRUE/*abPeek*/);
	}

	// Если зарегистрирован callback для графической панели
	if (gPanelRegLeft.pfnPeekPreCall || gPanelRegRight.pfnPeekPreCall)
	{
		// Если функция возвращает FALSE - реальное чтение не будет вызвано
		if (!OnPanelViewCallbacks(pArgs, gPanelRegLeft.pfnPeekPreCall, gPanelRegRight.pfnPeekPreCall))
			return FALSE;
	}

	return TRUE; // продолжить
}

VOID WINAPI OnConsolePeekInputPost(HookCallbackArg* pArgs)
{
	if (!pArgs->bMainThread) return;  // обработку делаем только в основной нити

#ifdef _DEBUG

	if (*(LPDWORD)(pArgs->lArguments[3]))
	{
		wchar_t szDbg[255];
		PINPUT_RECORD p = (PINPUT_RECORD)(pArgs->lArguments[1]);
		LPDWORD pCount = (LPDWORD)(pArgs->lArguments[3]);
		DWORD nLeft = 0; GetNumberOfConsoleInputEvents(GetStdHandle(STD_INPUT_HANDLE), &nLeft);
		_wsprintf(szDbg, SKIPLEN(countof(szDbg)) L"*** OnConsolePeekInputPost(Events=%i, KeyCount=%i, LeftInConBuffer=%i)\n",
		          *pCount, (p->EventType==KEY_EVENT) ? p->Event.KeyEvent.wRepeatCount : 0, nLeft);
		DEBUGSTRINPUT(szDbg);

		// Если под дебагом включен ScrollLock - вывести информацию о считанных событиях
		if (GetKeyState(VK_SCROLL) & 1)
		{
			PINPUT_RECORD p = (PINPUT_RECORD)(pArgs->lArguments[1]);
			LPDWORD pCount = (LPDWORD)(pArgs->lArguments[3]);
			_ASSERTE(*pCount <= pArgs->lArguments[2]);
			UINT nCount = *pCount;

			for(UINT i = 0; i < nCount; i++)
				DebugInputPrint(p[i]);
		}
	}

#endif

	// Если зарегистрирован callback для графической панели
	if (gPanelRegLeft.pfnPeekPostCall || gPanelRegRight.pfnPeekPostCall)
	{
		// Если функция возвращает FALSE - реальное чтение не будет вызвано
		if (!OnPanelViewCallbacks(pArgs, gPanelRegLeft.pfnPeekPostCall, gPanelRegRight.pfnPeekPostCall))
			return;
	}
}

// Если функция возвращает FALSE - реальный ReadConsoleInput вызван не будет,
// и в вызывающую функцию (ФАРа?) вернется то, что проставлено в pArgs->lpResult & pArgs->lArguments[...]
BOOL OnConsoleReadInputWork(HookCallbackArg* pArgs)
{
	if (!pArgs->bMainThread)
		return TRUE;  // обработку делаем только в основной нити

	if (gbUngetDummyMouseEvent)
	{
		if (UngetDummyMouseEvent(TRUE, pArgs))
		{
			gbUngetDummyMouseEvent = FALSE;
			gLastMouseReadEvent.dwButtonState = 0; // будем считать, что "мышиную блокировку" успешно сняли
			return FALSE; // реальный ReadConsoleInput вызван не будет
		}
		_ASSERTE(gbUngetDummyMouseEvent == FALSE);
	}

	//// Выставить флажок "Жив" можно и при вызове из плагина
	//if (gpConMapInfo && gpFarInfo && gpFarInfoMapping)
	//	TouchReadPeekConsoleInputs(0);
	//
	//if (pArgs->IsExecutable != HEO_Executable)
	//	return TRUE;  // и только при вызове из far.exe

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

BOOL WINAPI OnConsoleReadInput(HookCallbackArg* pArgs)
{
	return OnConsoleReadInputWork(pArgs);
}

VOID WINAPI OnConsoleReadInputPost(HookCallbackArg* pArgs)
{
	if (!pArgs->bMainThread) return;  // обработку делаем только в основной нити

#ifdef _DEBUG
	{
		wchar_t szDbg[255];
		PINPUT_RECORD p = (PINPUT_RECORD)(pArgs->lArguments[1]);
		LPDWORD pCount = (LPDWORD)(pArgs->lArguments[3]);
		DWORD nLeft = 0; GetNumberOfConsoleInputEvents(GetStdHandle(STD_INPUT_HANDLE), &nLeft);
		_wsprintf(szDbg, SKIPLEN(countof(szDbg)) L"*** OnConsoleReadInputPost(Events=%i, KeyCount=%i, LeftInConBuffer=%i)\n",
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
		if (GetKeyState(VK_SCROLL) & 1)
		{
			PINPUT_RECORD p = (PINPUT_RECORD)(pArgs->lArguments[1]);
			LPDWORD pCount = (LPDWORD)(pArgs->lArguments[3]);
			_ASSERTE(*pCount <= pArgs->lArguments[2]);
			UINT nCount = *pCount;

			for(UINT i = 0; i < nCount; i++)
				DebugInputPrint(p[i]);
		}
	}
#endif

	HANDLE h = (HANDLE)(pArgs->lArguments[0]);
	PINPUT_RECORD p = (PINPUT_RECORD)(pArgs->lArguments[1]);
	LPDWORD pCount = (LPDWORD)(pArgs->lArguments[3]);

	//Чтобы не было зависаний при попытке активации плагина во время прокрутки
	//редактора, в плагине мониторить нажатие мыши. Если последнее МЫШИНОЕ событие
	//было с нажатой кнопкой - сначала пульнуть в консоль команду "отпускания" кнопки,
	//и только после этого - пытаться активироваться.
	if (pCount && *pCount)
	{
		for (int i = (*pCount) - 1; i >= 0; i--)
		{
			if (p[i].EventType == MOUSE_EVENT)
			{
				gLastMouseReadEvent = p[i].Event.MouseEvent;
				break;
			}
		}
	}

	// Если зарегистрирован callback для графической панели
	if (gPanelRegLeft.pfnReadPostCall || gPanelRegRight.pfnReadPostCall)
	{
		if (!OnPanelViewCallbacks(pArgs, gPanelRegLeft.pfnReadPostCall, gPanelRegRight.pfnReadPostCall))
			return;
	}

	// Чтобы ФАР сразу прекратил ходить по каталогам при отпускании Enter
	if (h != NULL)
	{
		if (*pCount == 1 && p->EventType == KEY_EVENT && p->Event.KeyEvent.bKeyDown
		        && (p->Event.KeyEvent.wVirtualKeyCode == VK_RETURN
		            || p->Event.KeyEvent.wVirtualKeyCode == VK_NEXT
		            || p->Event.KeyEvent.wVirtualKeyCode == VK_PRIOR)
		  )
		{
			INPUT_RECORD ir[10]; DWORD nRead = 0, nInc = 0;

			if (PeekConsoleInputW(h, ir, countof(ir), &nRead) && nRead)
			{
				for(DWORD n = 0; n < nRead; n++)
				{
					if (ir[n].EventType == KEY_EVENT && ir[n].Event.KeyEvent.bKeyDown
					        && ir[n].Event.KeyEvent.wVirtualKeyCode == p->Event.KeyEvent.wVirtualKeyCode
					        && ir[n].Event.KeyEvent.dwControlKeyState == p->Event.KeyEvent.dwControlKeyState)
					{
						nInc++;
					}
					else
					{
						break; // дубли в буфере кончились
					}
				}

				if (nInc > 0)
				{
					if (ReadConsoleInputW(h, ir, nInc, &nRead) && nRead)
					{
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
		return TRUE;  // обработку делаем только в основной нити
	//if (pArgs->IsExecutable != HEO_Executable)
	//	return TRUE;  // и только при вызове из far.exe

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

BOOL LoadFarVersion()
{
	wchar_t ErrText[512]; ErrText[0] = 0;
	BOOL lbRc = LoadFarVersion(gFarVersion, ErrText);

	if (ErrText[0])
	{
		MessageBox(0, ErrText, L"ConEmu plugin", MB_OK|MB_ICONSTOP|MB_SETFOREGROUND);
	}

	if (!lbRc)
	{
		gFarVersion.dwVerMajor = 2;
		gFarVersion.dwVerMinor = 0;
		gFarVersion.dwBuild = FAR_X_VER;
	}

	return lbRc;
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

BOOL WINAPI OnConsoleDetaching(HookCallbackArg* pArgs)
{
	if (ghMonitorThread)
	{
		SuspendThread(ghMonitorThread);
		// ResumeThread выполняется в конце OnConsoleWasAttached
	}

	// Выполним сразу после SuspendThread, чтобы нить не посчитала, что мы подцепились обратно
	gbWasDetached = (ghConEmuWndDC!=NULL && IsWindow(ghConEmuWndDC));

	if (ghConEmuWndDC)
	{
		// Запомним, для удобства аттача
		if (!GetWindowThreadProcessId(ghConEmuWndDC, &gdwPreDetachGuiPID))
			gdwPreDetachGuiPID = 0;
	}

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
			ExecutePrepareCmd(&In, CECMD_FARDETACHED, sizeof(CESERVER_REQ_HDR));
			pOut = ExecuteSrvCmd(gdwServerPID, &In, FarHwnd);

			if (pOut) ExecuteFreeResult(pOut);
		}
	}

	// -- теперь мэппинги создает GUI
	//CloseColorerHeader(); // Если было

	CloseMapHeader();
	ghConEmuWndDC = NULL;
	SetConEmuEnvVar(NULL);
	SetConEmuEnvVarChild(NULL,NULL);
	// Потом еще и FarHwnd сбросить нужно будет... Ну этим MonitorThreadProcW займется
	return TRUE; // продолжить выполнение функции
}
// Функции вызываются в основной нити, вполне можно дергать FAR-API
VOID WINAPI OnConsoleWasAttached(HookCallbackArg* pArgs)
{
	FarHwnd = GetConEmuHWND(2);

	if (gbWasDetached)
	{
		// Сразу спрятать окошко
		//apiShowWindow(FarHwnd, SW_HIDE);
	}

	// -- теперь мэппинги создает GUI
	//// Если ранее были созданы мэппинги для цвета - пересоздать
	//CreateColorerHeader();

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
			EmergencyShow(FarHwnd);
		}

		// Сбрасываем после Attach2Gui, чтобы MonitorThreadProcW случайно
		// не среагировал раньше времени
		gbWasDetached = FALSE;
	}

	if (ghMonitorThread)
		ResumeThread(ghMonitorThread);
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

//#define CREATEEVENT(fmt,h)
//		_wsprintf(szEventName, SKIPLEN(countof(szEventName)) fmt, dwCurProcId );
//		h = CreateEvent(NULL, FALSE, FALSE, szEventName);
//		if (h==INVALID_HANDLE_VALUE) h=NULL;

VOID WINAPI OnCurDirChanged()
{
	if ((gnCurrentWindowType == WTYPE_PANELS) && (IS_SYNCHRO_ALLOWED))
	{
		// Требуется дернуть Synchro, чтобы корректно активироваться
		if (!gbInputSynchroPending)
		{
			gbInputSynchroPending = true;
			ExecuteSynchro();
		}
	}
}

//#ifndef max
//#define max(a,b)            (((a) > (b)) ? (a) : (b))
//#endif
//
//#ifndef min
//#define min(a,b)            (((a) < (b)) ? (a) : (b))
//#endif


// watch non-modified -> modified editor status change

//int lastModifiedStateW = -1;
//bool gbHandleOneRedraw = false; //, gbHandleOneRedrawCh = false;

int WINAPI ProcessEditorInputW(void* Rec)
{
	// Даже если мы не под эмулятором - просто запомним текущее состояние
	//if (!ghConEmuWndDC) return 0; // Если мы не под эмулятором - ничего
	return Plugin()->ProcessEditorInput((LPCVOID)Rec);
}

void FillLoadedParm(struct ConEmuLoadedArg* pArg, HMODULE hSubPlugin, BOOL abLoaded)
{
	memset(pArg, 0, sizeof(struct ConEmuLoadedArg));
	pArg->cbSize = (DWORD)sizeof(struct ConEmuLoadedArg);
	//#define D(N) (1##N-100)
	// nBuildNo в формате YYMMDDX (YY - две цифры года, MM - месяц, DD - день, X - 0 и выше-номер подсборки)
	pArg->nBuildNo = ((MVV_1 % 100)*100000) + (MVV_2*1000) + (MVV_3*10) + (MVV_4 % 10);
	pArg->hConEmu = ghPluginModule;
	pArg->hPlugin = hSubPlugin;
	pArg->bLoaded = abLoaded;
	pArg->bGuiActive = abLoaded && (ghConEmuWndDC != NULL);

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

	if (snapshot != INVALID_HANDLE_VALUE)
	{
		MODULEENTRY32 module = {sizeof(MODULEENTRY32)};

		for (BOOL res = Module32First(snapshot, &module); res; res = Module32Next(snapshot, &module))
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
					SAFETRY
					{
						fnOnConEmuLoaded(&arg);
						lbSucceded = TRUE;
					} SAFECATCH
					{
						// Failed
						_ASSERTE(lbSucceded == TRUE);
					}
				}
			}
		}

		CloseHandle(snapshot);
	}
}


HANDLE WINAPI OpenW(const void* Info)
{
	return Plugin()->Open(Info);
}

#if 0
INT_PTR WINAPI ProcessConsoleInputW(void *Info)
{
	if (gFarVersion.dwBuild>=FAR_Y2_VER)
		return FUNC_Y2(ProcessConsoleInputW)(Info);
	else //if (gFarVersion.dwBuild>=FAR_Y1_VER)
		return FUNC_Y1(ProcessConsoleInputW)(Info);
}
#endif

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

// Определены в SetHook.h
//typedef void (WINAPI* OnLibraryLoaded_t)(HMODULE ahModule);
//extern OnLibraryLoaded_t gfOnLibraryLoaded;

// Вызывается при загрузке dll
void WINAPI OnLibraryLoaded(HMODULE ahModule)
{
	WARNING("Проверить, чтобы после новых хуков это два раза на один модуль не вызывалось");

	//#ifdef _DEBUG
	//wchar_t szModulePath[MAX_PATH]; szModulePath[0] = 0;
	//GetModuleFileName(ahModule, szModulePath, MAX_PATH);
	//#endif

	//// Если GUI неактивно (запущен standalone FAR) - сразу выйти
	//if (ghConEmuWndDC == NULL)
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
			//arg.bGuiActive = (ghConEmuWndDC != NULL);
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
			SAFETRY
			{
				fnOnConEmuLoaded(&arg);
				lbSucceeded = TRUE;
			} SAFECATCH
			{
				// Failed
				_ASSERTE(lbSucceeded == TRUE);
			}
		}
	}
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
