
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
#include <WinCon.h>
#ifdef _DEBUG
#include <stdio.h>
#endif
#include <Shlwapi.h>

#include "../common/Common.h"
#include "../common/MConHandle.h"
#include "../common/MFileMapping.h"
#include "../common/MSection.h"
#include "../common/ConsoleAnnotation.h"
#include "../common/InQueue.h"



BOOL ReloadFullConsoleInfo(BOOL abForceSend);
bool CheckWasFullScreen();
DWORD WINAPI RefreshThread(LPVOID lpvParam); // Thread reloading console contents
bool FreezeRefreshThread();
bool ThawRefreshThread();
int ServerInit(); // Создать необходимые события и нити
void ServerDone(int aiRc, bool abReportShutdown = false);
BOOL ServerInitConsoleMode();
BOOL MyReadConsoleOutput(HANDLE hOut, CHAR_INFO *pData, COORD& bufSize, SMALL_RECT& rgn);
BOOL MyWriteConsoleOutput(HANDLE hOut, CHAR_INFO *pData, COORD& bufSize, COORD& crBufPos, SMALL_RECT& rgn);

void CmdOutputStore(bool abCreateOnly = false);
void CmdOutputRestore(bool abSimpleMode);

void CheckConEmuHwnd();

HWND Attach2Gui(DWORD nTimeout);

bool AltServerWasStarted(DWORD nPID, HANDLE hAltServer, bool ForceThaw = false);

int CreateMapHeader();
void CloseMapHeader();
void CopySrvMapFromGuiMap();
void UpdateConsoleMapHeader(LPCWSTR asReason = NULL);
void InitAnsiLog(const ConEmuAnsiLog& AnsiLog);
int Compare(const CESERVER_CONSOLE_MAPPING_HDR* p1, const CESERVER_CONSOLE_MAPPING_HDR* p2);
void FixConsoleMappingHdr(CESERVER_CONSOLE_MAPPING_HDR *pMap);

int CreateColorerHeader(bool bForceRecreate = false);

int MySetWindowRgn(CESERVER_REQ_SETWINDOWRGN* pRgn);

bool IsAutoAttachAllowed();

/* Console Handles */
extern MConHandle ghConOut;
extern MConHandle gPrimaryBuffer, gAltBuffer;
extern USHORT gnPrimaryBufferLastRow;
void ConOutCloseHandle();
bool CmdOutputOpenMap(CONSOLE_SCREEN_BUFFER_INFO& lsbi, CESERVER_CONSAVE_MAPHDR*& pHdr, CESERVER_CONSAVE_MAP*& pData);
bool isReopenHandleAllowed();

#ifdef WIN64
#ifndef __GNUC__
#pragma message("ComEmuC compiled in X64 mode")
#endif
#define NTVDMACTIVE FALSE
#else
#ifndef __GNUC__
#pragma message("ComEmuC compiled in X86 mode")
#endif
#define NTVDMACTIVE (gpSrv->processes->bNtvdmActive)
#endif

#include "../common/PipeServer.h"
#include "../common/MArray.h"
#include "../common/MMap.h"

struct AltServerInfo
{
	DWORD  nPID; // informational
	HANDLE hPrev;
	DWORD  nPrevPID;
};

struct ConProcess;

#include "Debugger.h"

struct SrvInfo
{
	void InitFields();
	void FinalizeFields();

	HANDLE hRootProcess, hRootThread;
	DWORD dwRootProcess, dwRootThread; DWORD dwRootStartTime;
	DWORD dwParentFarPID;

	// Full information about our console processes
	ConProcess* processes;

	CESERVER_REQ_PORTABLESTARTED Portable;

	struct {
		HWND  hGuiWnd;
		BOOL  bCallRc;
		DWORD nTryCount, nErr, nDupErrCode;
		DWORD nInitTick, nStartTick, nEndTick, nDelta, nConnectDelta;
		BOOL  bConnected;
	} ConnectInfo;

	HANDLE hMainServer; DWORD dwMainServerPID;
	HANDLE hServerStartedEvent;

	HANDLE hAltServer, hCloseAltServer, hAltServerChanged;
	DWORD dwAltServerPID; DWORD dwPrevAltServerPID;
	MMap<DWORD,AltServerInfo> AltServers;

	// Refresh thread
	HANDLE hRefreshThread;
	DWORD  dwRefreshThread;
	BOOL   bRefreshTermination;
	LONG   nRefreshFreezeRequests;
	LONG   nRefreshIsFreezed; // atomic
	HANDLE hFreezeRefreshThread;
	MSectionSimple csRefreshControl;

	DWORD  nPrevAltServer; // Informational, only for RM_ALTSERVER

	// CECMD_SETCONSCRBUF
	HANDLE hWaitForSetConBufThread;    // Remote thread (check it for abnormal termination)
	HANDLE hInWaitForSetConBufThread;  // signal that RefreshThread is ready to wait for hWaitForSetConBufThread
	HANDLE hOutWaitForSetConBufThread; // signal that RefreshThread may continue

	HWND   hRootProcessGui; // Если работаем в Gui-режиме (Notepad, Putty, ...), ((HWND)-1) пока фактичеки окно еще не создано, но exe-шник уже есть
	DebuggerInfo DbgInfo;
	DWORD  dwGuiAID; // ConEmu internal ID of started CRealConsole
	HWND   hGuiWnd; // передается через аргумент "/GHWND=%08X", чтобы окно не искать
	BOOL   bRequestNewGuiWnd;
	DWORD  nActiveFarPID; // PID последнего активного Far
	BOOL   bWasDetached; // Выставляется в TRUE при получении CECMD_DETACHCON
	BOOL   bWasReattached; // Если TRUE - то при следующем цикле нужно передернуть ReloadFullConsoleInfo(true)
	BOOL   bStationLocked; // Don't read console output while station is locked
	//
	PipeServer<CESERVER_REQ> CmdServer;
	PipeServer<MSG64> InputServer;
	PipeServer<CESERVER_REQ> DataServer;
	bool   bServerForcedTermination;
	//
	OSVERSIONINFO osv;
	UINT nMaxFPS;
	//
	MSection *csAltSrv;
	//
	wchar_t szPipename[MAX_PATH], szInputname[MAX_PATH], szGuiPipeName[MAX_PATH], szQueryname[MAX_PATH];
	wchar_t szGetDataPipe[MAX_PATH], szDataReadyEvent[64];
	HANDLE /*hInputPipe,*/ hQueryPipe/*, hGetDataPipe*/;
	//
	MFileMapping<CESERVER_CONSAVE_MAPHDR> *pStoredOutputHdr;
	MFileMapping<CESERVER_CONSAVE_MAP> *pStoredOutputItem;
	//
	MFileMapping<CESERVER_CONSOLE_MAPPING_HDR> *pConsoleMap;
	MFileMapping<CESERVER_CONSOLE_APP_MAPPING> *pAppMap;
	DWORD guiSettingsChangeNum;
	DWORD nLastConsoleActiveTick;
	HWND hGuiInfoMapWnd;
	MFileMapping<ConEmuGuiMapping>* pGuiInfoMap;
	ConEmuGuiMapping guiSettings;
	CESERVER_REQ_CONINFO_FULL *pConsole;
	CHAR_INFO *pConsoleDataCopy; // Local (Alloc)
	MSectionSimple csReadConsoleInfo;
	bool wasFullscreenMode;
	// Input
	HANDLE hInputThread;
	DWORD dwInputThread; BOOL bInputTermination;
	HANDLE hInputEvent;   // Выставляется в InputThread, флаг появления новых событий в очереди
	HANDLE hInputWasRead; // Выставляется в InputThread после чтения InputQueue.ReadInputQueue
	//int nInputQueue, nMaxInputQueue;
	//INPUT_RECORD* pInputQueue;
	//INPUT_RECORD* pInputQueueEnd;
	//INPUT_RECORD* pInputQueueRead;
	//INPUT_RECORD* pInputQueueWrite;
	InQueue InputQueue;
	// TrueColorer buffer
	//HANDLE hColorerMapping;
	MSectionSimple csColorerMappingCreate;
	MFileMapping<const AnnotationHeader>* pColorerMapping; // поддержка Colorer TrueMod
	AnnotationHeader ColorerHdr; // для сравнения индексов
	//
	HANDLE hConEmuGuiAttached;
	HWINEVENTHOOK /*hWinHook,*/ hWinHookStartEnd; //BOOL bWinHookAllow; int nWinHookMode;
	DWORD dwCiRc; CONSOLE_CURSOR_INFO ci; // GetConsoleCursorInfo
	DWORD dwConsoleCP, dwConsoleOutputCP;
	WORD dwConsoleInMode, dwConsoleOutMode;
	DWORD dwSbiRc; CONSOLE_SCREEN_BUFFER_INFO sbi; // MyGetConsoleScreenBufferInfo
	CESERVER_CONSOLE_PALETTE ConsolePalette;
	DWORD dwDisplayMode;
	BOOL  bAltBufferEnabled;
	//USHORT nUsedHeight; // Высота, используемая в GUI - вместо него используем gcrBufferSize.Y
	TOPLEFTCOORD TopLeft; // Прокрутка в GUI может быть заблокирована. Если -1 - без блокировки, используем текущее значение
	SHORT nVisibleHeight;  // По идее, должен быть равен (gcrBufferSize.Y). Это гарантированное количество строк psChars & pnAttrs
	DWORD nMainTimerElapse;
	HANDLE hRefreshEvent; // ServerMode, перечитать консоль, и если есть изменения - отослать в GUI
	HANDLE hRefreshDoneEvent; // ServerMode, выставляется после hRefreshEvent
	HANDLE hDataReadyEvent; // Флаг, что в сервере есть изменения (GUI должен перечитать данные)
	HANDLE hFarCommitEvent; // ServerMode, перечитать консоль, т.к. Far вызвал Commit в ExtConsole, - отослать в GUI
	HANDLE hCursorChangeEvent; // ServerMode, перечитать консоль (облегченный режим), т.к. был изменен курсор, - отослать в GUI
	BOOL   bFarCommitRegistered; // Загружен (в этом! процессе) ExtendedConsole.dll
	BOOL   bCursorChangeRegistered; // Загружен (в этом! процессе) ExtendedConsole.dll
	BOOL bForceConsoleRead; // Пнуть нить опроса консоли RefreshThread чтобы она без задержек перечитала содержимое
	// Смена размера консоли через RefreshThread
	LONG nRequestChangeSize;
	BOOL bRequestChangeSizeResult;
	bool bReqSizeForceLog;
	USHORT nReqSizeBufferHeight;
	COORD crReqSizeNewSize;
	SMALL_RECT rReqSizeNewRect;
	LPCSTR sReqSizeLabel;
	HANDLE hReqSizeChanged;
	MSection* pReqSizeSection;
	//
	HANDLE hAllowInputEvent; BOOL bInSyncResize;
	//
	LONG nLastPacketID; // ИД пакета для отправки в GUI

	// Keyboard layout name
	wchar_t szKeybLayout[KL_NAMELENGTH+1];

	// Optional console font (may be specified in registry)
	wchar_t szConsoleFont[LF_FACESIZE];
	//wchar_t szConsoleFontFile[MAX_PATH]; -- не помогает
	SHORT nConFontWidth, nConFontHeight;

	// Limited logging of console contents (same output as processed by CECF_ProcessAnsi)
	ConEmuAnsiLog AnsiLog;

	// Когда была последняя пользовательская активность
	DWORD dwLastUserTick;

	// Если нужно заблокировать нить RefreshThread
	//HANDLE hLockRefreshBegin, hLockRefreshReady;

	// Console Aliases
	wchar_t* pszAliases; DWORD nAliasesSize;

	// ComSpec mode
	BOOL  bK;
	BOOL  bNewConsole;
	wchar_t szComSpecName[32];
	wchar_t szSelfName[32];
	wchar_t *pszPreAliases;
	DWORD nPreAliasSize;

};
