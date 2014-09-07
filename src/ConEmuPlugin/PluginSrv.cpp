
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


#define SHOWDEBUGSTR
//#define MCHKHEAP
#define DEBUGSTRMENU(s) //DEBUGSTR(s)
#define DEBUGSTRINPUT(s) //DEBUGSTR(s)
#define DEBUGSTRDLGEVT(s) //OutputDebugStringW(s)
#define DEBUGSTRCMD(s) DEBUGSTR(s)


//#include <stdio.h>
#include <windows.h>
//#include <windowsx.h>
//#include <string.h>
//#include <tchar.h>
#include "../common/common.hpp"
#include "../ConEmuHk/SetHook.h"
#ifdef _DEBUG
#pragma warning( disable : 4995 )
#endif
#include "../common/pluginW1761.hpp"
#ifdef _DEBUG
#pragma warning( default : 4995 )
#endif
#include "../common/ConsoleAnnotation.h"
#include "../common/WinObjects.h"
#include "../common/TerminalMode.h"
#include "../common/MSection.h"
#include "../ConEmu/version.h"
#include "PluginHeader.h"
#include "PluginBackground.h"
#include <Tlhelp32.h>

#ifndef __GNUC__
	#include <Dbghelp.h>
#else
#endif

#include "../common/ConEmuCheck.h"
#include "../common/PipeServer.h"

#ifdef USEPIPELOG
namespace PipeServerLogger
{
    Event g_events[BUFFER_SIZE];
    LONG g_pos = -1;
}
#endif

#define Free free
#define Alloc calloc

//#define MAKEFARVERSION(major,minor,build) ( ((major)<<8) | (minor) | ((build)<<16))

//#define ConEmu_SysID 0x43454D55 // 'CEMU'
#define SETWND_CALLPLUGIN_SENDTABS 100
#define SETWND_CALLPLUGIN_BASE (SETWND_CALLPLUGIN_SENDTABS+1)
#define CHECK_RESOURCES_INTERVAL 5000
#define CHECK_FARINFO_INTERVAL 2000

#define CMD__EXTERNAL_CALLBACK 0x80001
////typedef void (WINAPI* SyncExecuteCallback_t)(LONG_PTR lParam);
//struct SyncExecuteArg
//{
//	DWORD nCmd;
//	HMODULE hModule;
//	SyncExecuteCallback_t CallBack;
//	LONG_PTR lParam;
//};
//
//#ifdef _DEBUG
//wchar_t gszDbgModLabel[6] = {0};
//#endif
//
//#if defined(__GNUC__)
//extern "C" {
//	BOOL WINAPI DllMain(HANDLE hModule, DWORD  ul_reason_for_call, LPVOID lpReserved);
//	HWND WINAPI GetFarHWND();
//	HWND WINAPI GetFarHWND2(int anConEmuOnly);
//	void WINAPI GetFarVersion(FarVersion* pfv);
//	int  WINAPI ProcessEditorInputW(void* Rec);
//	void WINAPI SetStartupInfoW(void *aInfo);
//	//int  WINAPI ProcessSynchroEventW(int Event,void *Param);
//	BOOL WINAPI IsTerminalMode();
//	BOOL WINAPI IsConsoleActive();
//	int  WINAPI RegisterPanelView(PanelViewInit *ppvi);
//	int  WINAPI RegisterBackground(RegisterBackgroundArg *pbk);
//	int  WINAPI ActivateConsole();
//	int  WINAPI SyncExecute(HMODULE ahModule, SyncExecuteCallback_t CallBack, LONG_PTR lParam);
//	void WINAPI GetPluginInfoWcmn(void *piv);
//};
//#endif
//
//
//HMODULE ghPluginModule = NULL; // ConEmu.dll - сам плагин
//HWND ConEmuHwnd = NULL; // Содержит хэндл окна отрисовки. Это ДОЧЕРНЕЕ окно.
//DWORD gdwPreDetachGuiPID = 0;
//DWORD gdwServerPID = 0;
//BOOL TerminalMode = FALSE;
//HWND FarHwnd = NULL;
////WARNING("Убрать, заменить ghConIn на GetStdHandle()"); // Иначе в Win7 будет буфер разрушаться
////HANDLE ghConIn = NULL;
//DWORD gnMainThreadId = 0, gnMainThreadIdInitial = 0;
////HANDLE hEventCmd[MAXCMDCOUNT], hEventAlive=NULL, hEventReady=NULL;
//HANDLE ghMonitorThread = NULL; DWORD gnMonitorThreadId = 0;
////HANDLE ghInputThread = NULL; DWORD gnInputThreadId = 0;
extern HANDLE ghSetWndSendTabsEvent;
//FarVersion gFarVersion = {};
//WCHAR gszDir1[CONEMUTABMAX], gszDir2[CONEMUTABMAX];
//// gszRootKey используется ТОЛЬКО для ЧТЕНИЯ настроек PanelTabs (SeparateTabs/ButtonColors)
//WCHAR gszRootKey[MAX_PATH*2]; // НЕ ВКЛЮЧАЯ "\\Plugins"
//int maxTabCount = 0, lastWindowCount = 0, gnCurTabCount = 0;
//CESERVER_REQ* gpTabs = NULL; //(ConEmuTab*) Alloc(maxTabCount, sizeof(ConEmuTab));
//BOOL gbIgnoreUpdateTabs = FALSE; // выставляется на время CMD_SETWINDOW
//BOOL gbRequestUpdateTabs = FALSE; // выставляется при получении события FOCUS/KILLFOCUS
//BOOL gbClosingModalViewerEditor = FALSE; // выставляется при закрытии модального редактора/вьювера
//MOUSE_EVENT_RECORD gLastMouseReadEvent = {{0,0}};
//BOOL gbUngetDummyMouseEvent = FALSE;
//LONG gnAllowDummyMouseEvent = 0;
//LONG gnDummyMouseEventFromMacro = 0;
//
//extern HMODULE ghHooksModule;
//extern BOOL gbHooksModuleLoaded; // TRUE, если был вызов LoadLibrary("ConEmuHk.dll"), тогда его нужно FreeLibrary при выходе
//
//
////CRITICAL_SECTION csData;
//MSection *csData = NULL;
//// результат выполнения команды (пишется функциями OutDataAlloc/OutDataWrite)
//CESERVER_REQ* gpCmdRet = NULL;
//// инициализируется как "gpData = gpCmdRet->Data;"
//LPBYTE gpData = NULL, gpCursor = NULL;
//DWORD  gnDataSize=0;
//
//int gnPluginOpenFrom = -1;
//DWORD gnReqCommand = -1;
//LPVOID gpReqCommandData = NULL;
//static HANDLE ghReqCommandEvent = NULL;
//static BOOL   gbReqCommandWaiting = FALSE;
//
//
//UINT gnMsgTabChanged = 0;
extern MSection *csTabs;
////WCHAR gcPlugKey=0;
//BOOL  gbPlugKeyChanged=FALSE;
//HKEY  ghRegMonitorKey=NULL; HANDLE ghRegMonitorEvt=NULL;
////HMODULE ghFarHintsFix = NULL;
WCHAR gszPluginServerPipe[MAX_PATH];
#define MAX_SERVER_THREADS 3
//HANDLE ghServerThreads[MAX_SERVER_THREADS] = {NULL,NULL,NULL};
//HANDLE ghActiveServerThread = NULL;
//HANDLE ghPlugServerThread = NULL;
//DWORD  gnPlugServerThreadId = 0;
PipeServer<CESERVER_REQ> *gpPlugServer = NULL;
////DWORD  gnServerThreadsId[MAX_SERVER_THREADS] = {0,0,0};
HANDLE ghServerTerminateEvent = NULL;
//HANDLE ghPluginSemaphore = NULL;
//wchar_t gsFarLang[64] = {0};
//BOOL FindServerCmd(DWORD nServerCmd, DWORD &dwServerPID);
//BOOL gbNeedPostTabSend = FALSE;
//BOOL gbNeedPostEditCheck = FALSE; // проверить, может в активном редакторе изменился статус
////BOOL gbNeedBgUpdate = FALSE; // требуется перерисовка Background
//int lastModifiedStateW = -1;
//BOOL gbNeedPostReloadFarInfo = FALSE;
//DWORD gnNeedPostTabSendTick = 0;
//#define NEEDPOSTTABSENDDELTA 100
////wchar_t gsMonitorEnvVar[0x1000];
////bool gbMonitorEnvVar = false;
//#define MONITORENVVARDELTA 1000
//void UpdateEnvVar(const wchar_t* pszList);
//BOOL StartupHooks();
////BOOL gbFARuseASCIIsort = FALSE; // попытаться перехватить строковую сортировку в FAR
////HANDLE ghFileMapping = NULL;
////HANDLE ghColorMapping = NULL; // Создается при детаче консоли сразу после AllocConsole
////BOOL gbHasColorMapping = FALSE; // Чтобы знать, что буфер True-Colorer создан
////int gnColorMappingMaxCells = 0;
////MFileMapping<AnnotationHeader>* gpColorMapping = NULL;
////#ifdef TRUE_COLORER_OLD_SUPPORT
////HANDLE ghColorMappingOld = NULL;
////BOOL gbHasColorMappingOld = FALSE;
////#endif
//MFileMapping<CESERVER_CONSOLE_MAPPING_HDR> *gpConMap;
//const CESERVER_CONSOLE_MAPPING_HDR *gpConMapInfo = NULL;
////AnnotationInfo *gpColorerInfo = NULL;
//BOOL gbStartedUnderConsole2 = FALSE;
////void CheckColorerHeader();
////int CreateColorerHeader();
////void CloseColorerHeader();
//BOOL ReloadFarInfo(BOOL abForce);
//DWORD gnSelfPID = 0; //GetCurrentProcessId();
////BOOL  gbNeedReloadFarInfo = FALSE;
//HANDLE ghFarInfoMapping = NULL;
//CEFAR_INFO_MAPPING *gpFarInfo = NULL, *gpFarInfoMapping = NULL;
//HANDLE ghFarAliveEvent = NULL;
//PanelViewRegInfo gPanelRegLeft = {NULL};
//PanelViewRegInfo gPanelRegRight = {NULL};
//// Для плагинов PicView & MMView нужно знать, нажат ли CtrlShift при F3
//HANDLE ghConEmuCtrlPressed = NULL, ghConEmuShiftPressed = NULL;
//BOOL gbWaitConsoleInputEmpty = FALSE, gbWaitConsoleWrite = FALSE; //, gbWaitConsoleInputPeek = FALSE;
//HANDLE ghConsoleInputEmpty = NULL, ghConsoleWrite = NULL; //, ghConsoleInputWasPeek = NULL;
//// SEE_MASK_NOZONECHECKS
////BOOL gbShellNoZoneCheck = FALSE;
//DWORD GetMainThreadId();
////wchar_t gsLogCreateProcess[MAX_PATH+1] = {0};
//int gnSynchroCount = 0;
//bool gbSynchroProhibited = false;
//bool gbInputSynchroPending = false;
//void SetConsoleFontSizeTo(HWND inConWnd, int inSizeY, int inSizeX, const wchar_t *asFontName);
//
extern struct HookModeFar gFarMode;
//extern SetFarHookMode_t SetFarHookMode;

BOOL WINAPI PlugServerCommand(LPVOID pInst, CESERVER_REQ* pIn, CESERVER_REQ* &ppReply, DWORD &pcbReplySize, DWORD &pcbMaxReplySize, LPARAM lParam);
void WINAPI PlugServerFree(CESERVER_REQ* pReply, LPARAM lParam);
VOID WINAPI OnCurDirChanged();

void PlugServerInit()
{
}

bool PlugServerStart()
{
	bool lbStarted = false;
	DWORD dwCurProcId = GetCurrentProcessId();

	_wsprintf(gszPluginServerPipe, SKIPLEN(countof(gszPluginServerPipe)) CEPLUGINPIPENAME, L".", dwCurProcId);

	ghServerTerminateEvent = CreateEvent(NULL,TRUE,FALSE,NULL);
	_ASSERTE(ghServerTerminateEvent!=NULL);

	if (ghServerTerminateEvent) ResetEvent(ghServerTerminateEvent);

	//gnPlugServerThreadId = 0;
	//ghPlugServerThread = CreateThread(NULL, 0, PlugServerThread, (LPVOID)NULL, 0, &gnPlugServerThreadId);
	//_ASSERTE(ghPlugServerThread!=NULL);
	if (!gpPlugServer)
		gpPlugServer = (PipeServer<CESERVER_REQ>*)calloc(1, sizeof(*gpPlugServer));
	if (gpPlugServer)
	{
		gpPlugServer->SetMaxCount(3);
		gpPlugServer->SetOverlapped(true);
		gpPlugServer->SetLoopCommands(false);
		gpPlugServer->SetDummyAnswerSize(sizeof(CESERVER_REQ_HDR));

		lbStarted = gpPlugServer->StartPipeServer(gszPluginServerPipe, NULL, LocalSecurity(), PlugServerCommand, PlugServerFree);
	}
	_ASSERTE(gpPlugServer!=NULL && lbStarted);

	return lbStarted;
}

void PlugServerStop(bool abDelete)
{
	if (gpPlugServer)
	{
		gpPlugServer->StopPipeServer(true);

		if (abDelete)
		{
			SafeFree(gpPlugServer);
		}
	}
}

//DWORD WINAPI PlugServerThread(LPVOID lpvParam)
//{
//	BOOL fConnected = FALSE;
//	DWORD dwErr = 0;
//	HANDLE hPipe = NULL;
//	//HANDLE hWait[2] = {NULL,NULL};
//	//DWORD dwTID = GetCurrentThreadId();
//	//std::vector<HANDLE>::iterator iter;
//	_ASSERTE(gszPluginServerPipe[0]!=0);
//	//_ASSERTE(ghServerSemaphore!=NULL);
//
//	// The main loop creates an instance of the named pipe and
//	// then waits for a client to connect to it. When the client
//	// connects, a thread is created to handle communications
//	// with that client, and the loop is repeated.
//
//	//hWait[0] = ghServerTerminateEvent;
//	//hWait[1] = ghServerSemaphore;
//
//	// Пока не затребовано завершение консоли
//	do
//	{
//		fConnected = FALSE; // Новый пайп
//
//		while(!fConnected)
//		{
//			_ASSERTE(hPipe == NULL);
//			// Проверить, может какие-то командные нити уже завершились
//			ghCommandThreads->CheckTerminated();
//			//iter = ghCommandThreads.begin();
//			//while (iter != ghCommandThreads.end()) {
//			//	HANDLE hThread = *iter; dwErr = 0;
//			//	if (WaitForSingleObject(hThread, 0) == WAIT_OBJECT_0) {
//			//		CloseHandle ( hThread );
//			//		iter = ghCommandThreads.erase(iter);
//			//	} else {
//			//		iter++;
//			//	}
//			//}
//			//// Дождаться разрешения семафора, или закрытия консоли
//			//dwErr = WaitForMultipleObjects ( 2, hWait, FALSE, INFINITE );
//			//if (dwErr == WAIT_OBJECT_0) {
//			//    return 0; // Консоль закрывается
//			//}
//			//for (int i=0; i<MAX_SERVER_THREADS; i++) {
//			//	if (gnServerThreadsId[i] == dwTID) {
//			//		ghActiveServerThread = ghServerThreads[i]; break;
//			//	}
//			//}
//			hPipe = CreateNamedPipe(gszPluginServerPipe, PIPE_ACCESS_DUPLEX,
//			                        CE_PIPE_TYPE | CE_PIPE_READMODE | PIPE_WAIT, PIPE_UNLIMITED_INSTANCES,
//			                        PIPEBUFSIZE, PIPEBUFSIZE, 0, gpLocalSecurity);
//			_ASSERTE(hPipe != INVALID_HANDLE_VALUE);
//
//			if (hPipe == INVALID_HANDLE_VALUE)
//			{
//				//DisplayLastError(L"CreateNamedPipe failed");
//				hPipe = NULL;
//				Sleep(50);
//				continue;
//			}
//
//			// Wait for the client to connect; if it succeeds,
//			// the function returns a nonzero value. If the function
//			// returns zero, GetLastError returns ERROR_PIPE_CONNECTED.
//			fConnected = ConnectNamedPipe(hPipe, NULL) ? TRUE : ((dwErr = GetLastError()) == ERROR_PIPE_CONNECTED);
//
//			// Консоль закрывается!
//			if (WaitForSingleObject(ghServerTerminateEvent, 0) == WAIT_OBJECT_0)
//			{
//				//FlushFileBuffers(hPipe); -- это не нужно, мы ничего не возвращали
//				//DisconnectNamedPipe(hPipe);
//				//ReleaseSemaphore(ghServerSemaphore, 1, NULL);
//				SafeCloseHandle(hPipe);
//				goto wrap;
//			}
//
//			if (!fConnected)
//				SafeCloseHandle(hPipe);
//		}
//
//		if (fConnected)
//		{
//			// сразу сбросим, чтобы не забыть
//			//fConnected = FALSE;
//			// разрешить другой нити принять вызов
//			//ReleaseSemaphore(ghServerSemaphore, 1, NULL);
//			//ServerThreadCommand ( hPipe ); // При необходимости - записывает в пайп результат сама
//			DWORD dwThread = 0;
//			HANDLE hThread = CreateThread(NULL, 0, PlugServerThreadCommand, (LPVOID)hPipe, 0, &dwThread);
//			_ASSERTE(hThread!=NULL);
//
//			if (hThread==NULL)
//			{
//				// Раз не удалось запустить нить - можно попробовать так обработать...
//				PlugServerThreadCommand((LPVOID)hPipe);
//			}
//			else
//			{
//				ghCommandThreads->push_back(hThread);
//			}
//
//			hPipe = NULL; // закрывает ServerThreadCommand
//		}
//
//		if (hPipe)
//		{
//			if (hPipe != INVALID_HANDLE_VALUE)
//			{
//				FlushFileBuffers(hPipe);
//				//DisconnectNamedPipe(hPipe);
//				CloseHandle(hPipe);
//			}
//
//			hPipe = NULL;
//		}
//	} // Перейти к открытию нового instance пайпа
//
//	while(WaitForSingleObject(ghServerTerminateEvent, 0) != WAIT_OBJECT_0);
//
//wrap:
//	// Прибивание всех запущенных нитей
//	ghCommandThreads->KillAllThreads();
//	//iter = ghCommandThreads.begin();
//	//while (iter != ghCommandThreads.end()) {
//	//	HANDLE hThread = *iter; dwErr = 0;
//	//	if (WaitForSingleObject(hThread, 100) != WAIT_OBJECT_0) {
//	//		TerminateThread(hThread, 100);
//	//	}
//	//	CloseHandle ( hThread );
//	//	iter = ghCommandThreads.erase(iter);
//	//}
//	return 0;
//}

void cmd_FarSetChanged(FAR_REQ_FARSETCHANGED *pFarSet)
{
	// CMD_FARSETCHANGED, CECMD_RESOURCES

	// Установить переменные окружения
	// Плагин это получает в ответ на CECMD_RESOURCES, посланное в GUI при загрузке плагина
	gFarMode.bFARuseASCIIsort = pFarSet->bFARuseASCIIsort;
	gFarMode.bShellNoZoneCheck = pFarSet->bShellNoZoneCheck;
	gFarMode.bMonitorConsoleInput = pFarSet->bMonitorConsoleInput;
	gFarMode.bLongConsoleOutput = pFarSet->bLongConsoleOutput;
	gFarMode.OnCurDirChanged = OnCurDirChanged;

	UpdateComspec(&pFarSet->ComSpec); // ComSpec, isAddConEmu2Path, ...

	if (SetFarHookMode)
	{
		// Уведомить об изменениях библиотеку хуков (ConEmuHk.dll)
		SetFarHookMode(&gFarMode);
	}
	else
	{
		_ASSERTE(SetFarHookMode!=NULL);
	}
}

BOOL WINAPI PlugServerCommand(LPVOID pInst, CESERVER_REQ* pIn, CESERVER_REQ* &ppReply, DWORD &pcbReplySize, DWORD &pcbMaxReplySize, LPARAM lParam)
{
	BOOL lbRc = FALSE;
	//HANDLE hPipe = (HANDLE)ahPipe;
	//CESERVER_REQ *pIn=NULL;
	//BYTE cbBuffer[64]; // Для большей части команд нам хватит
	//DWORD cbRead = 0, cbWritten = 0, dwErr = 0;
	BOOL fSuccess = FALSE;
	MSectionThread SCT(csTabs);
	// Send a message to the pipe server and read the response.
	//fSuccess = ReadFile(hPipe, cbBuffer, sizeof(cbBuffer), &cbRead, NULL);
	//dwErr = GetLastError();

	//if (!fSuccess && (dwErr != ERROR_MORE_DATA))
	//{
	//	_ASSERTE("ReadFile(pipe) failed"==NULL);
	//	CloseHandle(hPipe);
	//	return 0;
	//}

	//pIn = (CESERVER_REQ*)cbBuffer; // Пока cast, если нужно больше - выделим память
	//_ASSERTE(pIn->hdr.cbSize>=sizeof(CESERVER_REQ_HDR) && cbRead>=sizeof(CESERVER_REQ_HDR));
	//_ASSERTE(pIn->hdr.nVersion == CESERVER_REQ_VER);

	if (pIn->hdr.cbSize < sizeof(CESERVER_REQ_HDR) || /*in.nSize < cbRead ||*/ pIn->hdr.nVersion != CESERVER_REQ_VER)
	{
		//CloseHandle(hPipe);
		gpPlugServer->BreakConnection(pInst);
		return FALSE;
	}

	//int nAllSize = pIn->hdr.cbSize;
	//pIn = (CESERVER_REQ*)Alloc(nAllSize,1);
	//_ASSERTE(pIn!=NULL);

	//if (!pIn)
	//{
	//	CloseHandle(hPipe);
	//	return 0;
	//}

	//memmove(pIn, cbBuffer, cbRead);
	//_ASSERTE(pIn->hdr.nVersion==CESERVER_REQ_VER);
	//LPBYTE ptrData = ((LPBYTE)pIn)+cbRead;
	//nAllSize -= cbRead;

	//while(nAllSize>0)
	//{
	//	//_tprintf(TEXT("%s\n"), chReadBuf);

	//	// Break if TransactNamedPipe or ReadFile is successful
	//	if (fSuccess)
	//		break;

	//	// Read from the pipe if there is more data in the message.
	//	fSuccess = ReadFile(
	//	               hPipe,      // pipe handle
	//	               ptrData,    // buffer to receive reply
	//	               nAllSize,   // size of buffer
	//	               &cbRead,    // number of bytes read
	//	               NULL);      // not overlapped

	//	// Exit if an error other than ERROR_MORE_DATA occurs.
	//	if (!fSuccess && ((dwErr = GetLastError()) != ERROR_MORE_DATA))
	//		break;

	//	ptrData += cbRead;
	//	nAllSize -= cbRead;
	//}

	//TODO("Может возникнуть ASSERT, если консоль была закрыта в процессе чтения");
	//_ASSERTE(nAllSize==0);

	//if (nAllSize>0)
	//{
	//	if (((LPVOID)cbBuffer) != ((LPVOID)pIn))
	//		Free(pIn);

	//	CloseHandle(hPipe);
	//	return 0; // удалось считать не все данные
	//}

	UINT nDataSize = pIn->hdr.cbSize - sizeof(CESERVER_REQ_HDR);

	// Все данные из пайпа получены, обрабатываем команду и возвращаем (если нужно) результат
	//fSuccess = WriteFile( hPipe, pOut, pOut->nSize, &cbWritten, NULL);

	if (pIn->hdr.nCmd == CMD_LANGCHANGE)
	{
		_ASSERTE(nDataSize>=4); //-V112
		// LayoutName: "00000409", "00010409", ...
		// А HKL от него отличается, так что передаем DWORD
		// HKL в x64 выглядит как: "0x0000000000020409", "0xFFFFFFFFF0010409"
		DWORD hkl = pIn->dwData[0];
		DWORD dwLastError = 0;
		HKL hkl1 = NULL, hkl2 = NULL;

		if (hkl)
		{
			WCHAR szLoc[10]; _wsprintf(szLoc, SKIPLEN(countof(szLoc)) L"%08x", hkl);
			hkl1 = LoadKeyboardLayout(szLoc, KLF_ACTIVATE|KLF_REORDER|KLF_SUBSTITUTE_OK|KLF_SETFORPROCESS);
			hkl2 = ActivateKeyboardLayout(hkl1, KLF_SETFORPROCESS|KLF_REORDER);

			if (!hkl2)
				dwLastError = GetLastError();
			else
				fSuccess = TRUE;
		}

		pcbReplySize = sizeof(CESERVER_REQ_HDR) + sizeof(DWORD)*2;
		if (ExecuteNewCmd(ppReply, pcbMaxReplySize, pIn->hdr.nCmd, pcbReplySize))
		{
			lbRc = TRUE;
			ppReply->dwData[0] = fSuccess;
			ppReply->dwData[1] = fSuccess ? ((DWORD)(LONG)(LONG_PTR)hkl2) : dwLastError;
		}

	}
	//} else if (pIn->hdr.nCmd == CMD_DEFFONT) {
	//	// исключение - асинхронный, результат не требуется
	//	SetConsoleFontSizeTo(FarHwnd, 4, 6);
	//	MoveWindow(FarHwnd, 0, 0, GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN), 1); // чтобы убрать возможные полосы прокрутки...
	else if (pIn->hdr.nCmd == CMD_REQTABS || pIn->hdr.nCmd == CMD_SETWINDOW)
	{
		MSectionLock SC; SC.Lock(csTabs, FALSE, 1000);
		DWORD nSetWindowWait = (DWORD)-1;

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
			BOOL bCmdRc = ProcessCommand(pIn->hdr.nCmd, TRUE/*bReqMainThread*/, pIn->dwData);

			DEBUGSTRCMD(L"Plugin: PlugServerThreadCommand: CMD_SETWINDOW waiting...\n");

			WARNING("Почему для FAR1 не ждем? Есть возможность заблокироваться в 1.7 или что?");
			if ((gFarVersion.dwVerMajor >= 2) && bCmdRc)
			{
				DWORD nTimeout = 2000;
				#ifdef _DEBUG
				if (IsDebuggerPresent()) nTimeout = 120000;
				#endif

				nSetWindowWait = WaitForSingleObject(ghSetWndSendTabsEvent, nTimeout);
			}

			if (nSetWindowWait == WAIT_TIMEOUT)
			{
				gbForceSendTabs = TRUE;
				DEBUGSTRCMD(L"Plugin: PlugServerThreadCommand: CMD_SETWINDOW timeout !!!\n");
			}
			else
			{
				DEBUGSTRCMD(L"Plugin: PlugServerThreadCommand: CMD_SETWINDOW finished\n");
			}
		}

		if (gpTabs && (nSetWindowWait != WAIT_TIMEOUT))
		{
			//fSuccess = WriteFile(hPipe, gpTabs, gpTabs->hdr.cbSize, &cbWritten, NULL);
			pcbReplySize = gpTabs->hdr.cbSize;
			if (ExecuteNewCmd(ppReply, pcbMaxReplySize, pIn->hdr.nCmd, pcbReplySize))
			{
				memmove(ppReply->Data, gpTabs->Data, pcbReplySize - sizeof(ppReply->hdr));
				lbRc = TRUE;
			}
		}

		SC.Unlock();
	}
	else if (pIn->hdr.nCmd == CMD_FARSETCHANGED)
	{
		// Установить переменные окружения
		// Плагин это получает в ответ на CECMD_RESOURCES, посланное в GUI при загрузке плагина
		_ASSERTE(nDataSize>=8);
		FAR_REQ_FARSETCHANGED *pFarSet = (FAR_REQ_FARSETCHANGED*)pIn->Data;

		cmd_FarSetChanged(pFarSet);

		pcbReplySize = sizeof(CESERVER_REQ_HDR) + sizeof(DWORD);
		if (ExecuteNewCmd(ppReply, pcbMaxReplySize, pIn->hdr.nCmd, pcbReplySize))
		{
			lbRc = TRUE;
			ppReply->dwData[0] = TRUE;
		}

		//_ASSERTE(nDataSize<sizeof(gsMonitorEnvVar));
		//gbMonitorEnvVar = false;
		//// Плагин FarCall "нарушает" COMSPEC (копирует содержимое запускаемого процесса)
		//bool lbOk = false;

		//if (nDataSize<sizeof(gsMonitorEnvVar))
		//{
		//	memcpy(gsMonitorEnvVar, pszName, nDataSize);
		//	lbOk = true;
		//}

		//UpdateEnvVar(pszName);
		////while (*pszName && *pszValue) {
		////	const wchar_t* pszChanged = pszValue;
		////	// Для ConEmuOutput == AUTO выбирается по версии ФАРа
		////	if (!lstrcmpi(pszName, L"ConEmuOutput") && !lstrcmp(pszChanged, L"AUTO")) {
		////		if (gFarVersion.dwVerMajor==1)
		////			pszChanged = L"ANSI";
		////		else
		////			pszChanged = L"UNICODE";
		////	}
		////	// Если в pszValue пустая строка - удаление переменной
		////	SetEnvironmentVariableW(pszName, (*pszChanged != 0) ? pszChanged : NULL);
		////	//
		////	pszName = pszValue + lstrlenW(pszValue) + 1;
		////	if (*pszName == 0) break;
		////	pszValue = pszName + lstrlenW(pszName) + 1;
		////}
		//gbMonitorEnvVar = lbOk;
	}
	else if (pIn->hdr.nCmd == CMD_DRAGFROM)
	{
		#ifdef _DEBUG
		BOOL  *pbClickNeed = (BOOL*)pIn->Data;
		COORD *crMouse = (COORD *)(pbClickNeed+1);
		#endif

		ProcessCommand(CMD_LEFTCLKSYNC, TRUE/*bReqMainThread*/, pIn->Data);
		CESERVER_REQ* pCmdRet = NULL;
		ProcessCommand(pIn->hdr.nCmd, TRUE/*bReqMainThread*/, pIn->Data, &pCmdRet);

		if (pCmdRet)
		{
			//fSuccess = WriteFile(hPipe, pCmdRet, pCmdRet->hdr.cbSize, &cbWritten, NULL);
			pcbReplySize = pCmdRet->hdr.cbSize;
			if (ExecuteNewCmd(ppReply, pcbMaxReplySize, pIn->hdr.nCmd, pcbReplySize))
			{
				lbRc = TRUE;
				memmove(ppReply->Data, pCmdRet->Data, pCmdRet->hdr.cbSize - sizeof(ppReply->hdr));
			}
			Free(pCmdRet);
		}

		//if (gpCmdRet && gpCmdRet == pCmdRet)
		//{
		//	Free(gpCmdRet);
		//	gpCmdRet = NULL; gpData = NULL; gpCursor = NULL;
		//}
	}
	else if (pIn->hdr.nCmd == CMD_EMENU)
	{
		COORD *crMouse = (COORD *)pIn->Data;
#ifdef _DEBUG
		const wchar_t *pszUserMacro = (wchar_t*)(crMouse+1);
#endif
		DWORD ClickArg[2] = {TRUE, MAKELONG(crMouse->X, crMouse->Y)};

		// Выделить файл под курсором
		DEBUGSTRMENU(L"\n*** ServerThreadCommand->ProcessCommand(CMD_LEFTCLKSYNC) begin\n");
		BOOL lb1 = ProcessCommand(CMD_LEFTCLKSYNC, TRUE/*bReqMainThread*/, ClickArg/*pIn->Data*/);
		DEBUGSTRMENU(L"\n*** ServerThreadCommand->ProcessCommand(CMD_LEFTCLKSYNC) done\n");

		// А теперь, собственно вызовем меню
		DEBUGSTRMENU(L"\n*** ServerThreadCommand->ProcessCommand(CMD_EMENU) begin\n");
		BOOL lb2 = ProcessCommand(pIn->hdr.nCmd, TRUE/*bReqMainThread*/, pIn->Data);
		DEBUGSTRMENU(L"\n*** ServerThreadCommand->ProcessCommand(CMD_EMENU) done\n");

		pcbReplySize = sizeof(CESERVER_REQ_HDR) + sizeof(DWORD)*2;
		if (ExecuteNewCmd(ppReply, pcbMaxReplySize, pIn->hdr.nCmd, pcbReplySize))
		{
			lbRc = TRUE;
			ppReply->dwData[0] = lb1;
			ppReply->dwData[1] = lb1;
		}
	}
	else if (pIn->hdr.nCmd == CMD_ACTIVEWNDTYPE)
	{
		int nWindowType = -1;
		//CESERVER_REQ Out;
		//ExecutePrepareCmd(&Out, CMD_ACTIVEWNDTYPE, sizeof(CESERVER_REQ_HDR)+sizeof(DWORD));

		if (gFarVersion.dwVerMajor>=2)
			nWindowType = GetActiveWindowType();

		//fSuccess = WriteFile(hPipe, &Out, Out.hdr.cbSize, &cbWritten, NULL);

		pcbReplySize = sizeof(CESERVER_REQ_HDR) + sizeof(DWORD);
		if (ExecuteNewCmd(ppReply, pcbMaxReplySize, pIn->hdr.nCmd, pcbReplySize))
		{
			lbRc = TRUE;
			ppReply->dwData[0] = nDataSize;
		}
	}
	else
	{
		CESERVER_REQ* pCmdRet = NULL;
		BOOL lbCmd = ProcessCommand(pIn->hdr.nCmd, TRUE/*bReqMainThread*/, pIn->Data, &pCmdRet);

		if (pCmdRet)
		{
			//fSuccess = WriteFile(hPipe, pCmdRet, pCmdRet->hdr.cbSize, &cbWritten, NULL);
			pcbReplySize = pCmdRet->hdr.cbSize;
			if (ExecuteNewCmd(ppReply, pcbMaxReplySize, pIn->hdr.nCmd, pcbReplySize))
			{
				lbRc = TRUE;
				memmove(ppReply->Data, pCmdRet->Data, pCmdRet->hdr.cbSize - sizeof(ppReply->hdr));
			}
			Free(pCmdRet);
		}

		//if (gpCmdRet && gpCmdRet == pCmdRet) {
		//	Free(gpCmdRet);
		//	gpCmdRet = NULL; gpData = NULL; gpCursor = NULL;
		//}
	}

	//// Освободить память
	//if (((LPVOID)cbBuffer) != ((LPVOID)pIn))
	//	Free(pIn);

	//CloseHandle(hPipe);
	return lbRc;
}

void WINAPI PlugServerFree(CESERVER_REQ* pReply, LPARAM lParam)
{
	ExecuteFreeResult(pReply);
}
