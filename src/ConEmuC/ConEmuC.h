
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

#ifdef _DEBUG
	// Раскомментировать для вывода в консоль информации режима Comspec
	#define PRINT_COMSPEC(f,a) //wprintf(f,a)
	//#define DEBUGSTR(s) OutputDebugString(s)
#elif defined(__GNUC__)
	//  Раскомментировать, чтобы сразу после запуска процесса (conemuc.exe) показать MessageBox, чтобы прицепиться дебаггером
	//  #define SHOW_STARTED_MSGBOX
	#define PRINT_COMSPEC(f,a) //wprintf(f,a)
	//#define DEBUGSTR(s)
	#define CRTPRINTF
#else
	#define PRINT_COMSPEC(f,a)
	#define DEBUGSTR(s)
#endif

#define DEBUGLOG(s) //DEBUGSTR(s)
#define DEBUGLOGINPUT(s) DEBUGSTR(s)
#define DEBUGLOGSIZE(s) DEBUGSTR(s)
#define DEBUGLOGLANG(s) //DEBUGSTR(s) //; Sleep(2000)

#ifdef _DEBUG
//CRITICAL_ SECTION gcsHeap;
//#define MCHKHEAP { Enter CriticalSection(&gcsHeap); int MDEBUG_CHK=_CrtCheckMemory(); _ASSERTE(MDEBUG_CHK); LeaveCriticalSection(&gcsHeap); }
#define MCHKHEAP HeapValidate(ghHeap, 0, NULL);
//#define MCHKHEAP { int MDEBUG_CHK=_CrtCheckMemory(); _ASSERTE(MDEBUG_CHK); }
//#define HEAP_LOGGING
#else
#define MCHKHEAP
#endif




#define CSECTION_NON_RAISE

#include <Windows.h>
#include <WinCon.h>
#ifdef _DEBUG
#include <stdio.h>
#endif
#include <Shlwapi.h>
#include <Tlhelp32.h>


/*  Global  */
extern DWORD   gnSelfPID;
//HANDLE  ghConIn = NULL, ghConOut = NULL;
extern HWND    ghConWnd;
extern HWND    ghConEmuWnd; // Root! window
extern HANDLE  ghExitQueryEvent; // выставляется когда в консоли не остается процессов
extern HANDLE  ghQuitEvent;      // когда мы в процессе закрытия (юзер уже нажал кнопку "Press to close console")
extern bool    gbQuit;           // когда мы в процессе закрытия (юзер уже нажал кнопку "Press to close console")
extern int     gnConfirmExitParm;
extern BOOL    gbAlwaysConfirmExit, gbInShutdown, gbAutoDisableConfirmExit;
extern int     gbRootWasFoundInCon;
extern BOOL    gbAttachMode;
extern BOOL    gbForceHideConWnd;
extern DWORD   gdwMainThreadId;
//int       gnBufferHeight = 0;
extern wchar_t* gpszRunCmd;
extern DWORD   gnImageSubsystem;
//HANDLE  ghCtrlCEvent = NULL, ghCtrlBreakEvent = NULL;
extern HANDLE ghHeap; //HeapCreate(HEAP_GENERATE_EXCEPTIONS, nMinHeapSize, 0);
#ifdef _DEBUG
extern size_t gnHeapUsed, gnHeapMax;
extern HANDLE ghFarInExecuteEvent;
#endif

//#include <vector>
#include "../common/common.hpp"
#include "../common/ConEmuCheck.h"
#include "../Common/WinObjects.h"

#ifdef _DEBUG
extern wchar_t gszDbgModLabel[6];
#endif

#define START_MAX_PROCESSES 1000
#define CHECK_PROCESSES_TIMEOUT 500
#define CHECK_ANTIVIRUS_TIMEOUT 6*1000
#define CHECK_ROOTSTART_TIMEOUT 10*1000
#define CHECK_ROOTOK_TIMEOUT 10*1000
#define MAX_FORCEREFRESH_INTERVAL 1000
#define MAX_SYNCSETSIZE_WAIT 1000
#define GUI_PIPE_TIMEOUT 300
#define MAX_CONREAD_SIZE 30000 // в байтах
#define RELOAD_INFO_TIMEOUT 500
#define REQSIZE_TIMEOUT 5000
#define GUIREADY_TIMEOUT 10000
#define UPDATECONHANDLE_TIMEOUT 1000
#define GUIATTACH_TIMEOUT 10000
#define INPUT_QUEUE_TIMEOUT 100

#define IMAGE_SUBSYSTEM_DOS_EXECUTABLE  255


//#ifndef _DEBUG
// Релизный режим
#define FORCE_REDRAW_FIX
#define RELATIVE_TRANSMIT_DISABLE
//#else
//// Отладочный режим
////#define FORCE_REDRAW_FIX
//#endif

#if !defined(CONSOLE_APPLICATION_16BIT)
#define CONSOLE_APPLICATION_16BIT       0x0001
#endif


#if defined(__GNUC__)
	//#include "assert.h"
	#ifndef _ASSERTE
		#define _ASSERTE(x)
	#endif
	#ifndef _ASSERT
		#define _ASSERT(x)
	#endif
#else
	#include <crtdbg.h>
#endif

#ifndef EVENT_CONSOLE_CARET
	#define EVENT_CONSOLE_CARET             0x4001
	#define EVENT_CONSOLE_UPDATE_REGION     0x4002
	#define EVENT_CONSOLE_UPDATE_SIMPLE     0x4003
	#define EVENT_CONSOLE_UPDATE_SCROLL     0x4004
	#define EVENT_CONSOLE_LAYOUT            0x4005
	#define EVENT_CONSOLE_START_APPLICATION 0x4006
	#define EVENT_CONSOLE_END_APPLICATION   0x4007
#endif

#define SafeCloseHandle(h) { if ((h)!=NULL) { HANDLE hh = (h); (h) = NULL; if (hh!=INVALID_HANDLE_VALUE) CloseHandle(hh); } }


DWORD WINAPI InstanceThread(LPVOID);
DWORD WINAPI ServerThread(LPVOID lpvParam);
//DWORD WINAPI InputThread(LPVOID lpvParam);
DWORD WINAPI InputPipeThread(LPVOID lpvParam);
DWORD WINAPI GetDataThread(LPVOID lpvParam);
BOOL GetAnswerToRequest(CESERVER_REQ& in, CESERVER_REQ** out); 
DWORD WINAPI WinEventThread(LPVOID lpvParam);
void WINAPI WinEventProc(HWINEVENTHOOK hWinEventHook, DWORD anEvent, HWND hwnd, LONG idObject, LONG idChild, DWORD dwEventThread, DWORD dwmsEventTime);
void CheckCursorPos();
//void SendConsoleChanges(CESERVER_REQ* pOut);
//CESERVER_REQ* CreateConsoleInfo(CESERVER_CHAR* pRgnOnly, BOOL bCharAttrBuff);
//BOOL ReloadConsoleInfo(CESERVER_CHAR* pChangedRgn=NULL); // возвращает TRUE в случае изменений
//BOOL ReloadFullConsoleInfo(/*CESERVER_CHAR* pCharOnly=NULL*/); // В том числе перечитывает содержимое
BOOL ReloadFullConsoleInfo(BOOL abForceSend);
DWORD WINAPI RefreshThread(LPVOID lpvParam); // Нить, перечитывающая содержимое консоли
//BOOL ReadConsoleData(CESERVER_CHAR* pCheck = NULL); //((LPRECT)1) или реальный LPRECT
void SetConsoleFontSizeTo(HWND inConWnd, int inSizeY, int inSizeX, const wchar_t *asFontName);
int ServerInit(); // Создать необходимые события и нити
void ServerDone(int aiRc);
int ComspecInit();
void ComspecDone(int aiRc);
BOOL SetConsoleSize(USHORT BufferHeight, COORD crNewSize, SMALL_RECT rNewRect, LPCSTR asLabel = NULL);
void CreateLogSizeFile(int nLevel);
void LogSize(COORD* pcrSize, LPCSTR pszLabel);
void LogString(LPCSTR asText);
BOOL WINAPI HandlerRoutine(DWORD dwCtrlType);
int GetProcessCount(DWORD *rpdwPID, UINT nMaxCount);
SHORT CorrectTopVisible(int nY);
BOOL CorrectVisibleRect(CONSOLE_SCREEN_BUFFER_INFO* pSbi);
WARNING("Вместо GetConsoleScreenBufferInfo нужно использовать MyGetConsoleScreenBufferInfo!");
BOOL MyGetConsoleScreenBufferInfo(HANDLE ahConOut, PCONSOLE_SCREEN_BUFFER_INFO apsc);
//void EnlargeRegion(CESERVER_CHAR_HDR& rgn, const COORD crNew);
void CmdOutputStore();
void CmdOutputRestore();
LPVOID Alloc(size_t nCount, size_t nSize);
void Free(LPVOID ptr);
void CheckConEmuHwnd();
HWND FindConEmuByPID();
typedef BOOL (__stdcall *FGetConsoleKeyboardLayoutName)(wchar_t*);
extern FGetConsoleKeyboardLayoutName pfnGetConsoleKeyboardLayoutName;
void CheckKeyboardLayout();
int CALLBACK FontEnumProc(ENUMLOGFONTEX *lpelfe, NEWTEXTMETRICEX *lpntme, DWORD FontType, LPARAM lParam);
typedef DWORD (WINAPI* FGetConsoleProcessList)(LPDWORD lpdwProcessList, DWORD dwProcessCount);
extern FGetConsoleProcessList pfnGetConsoleProcessList;
//BOOL HookWinEvents(int abEnabled);
BOOL CheckProcessCount(BOOL abForce=FALSE);
BOOL IsNeedCmd(LPCWSTR asCmdLine, BOOL *rbNeedCutStartEndQuot);
BOOL FileExists(LPCWSTR asFile);
extern bool GetImageSubsystem(const wchar_t *FileName,DWORD& ImageSubsystem);
void SendStarted();
BOOL SendConsoleEvent(INPUT_RECORD* pr, UINT nCount);
typedef BOOL (WINAPI *FDebugActiveProcessStop)(DWORD dwProcessId);
extern FDebugActiveProcessStop pfnDebugActiveProcessStop;
typedef BOOL (WINAPI *FDebugSetProcessKillOnExit)(BOOL KillOnExit);
extern FDebugSetProcessKillOnExit pfnDebugSetProcessKillOnExit;
void ProcessDebugEvent();
BOOL IsUserAdmin();
#ifdef CRTPRINTF
void _wprintf(LPCWSTR asBuffer);
void _printf(LPCSTR asBuffer);
void _printf(LPCSTR asFormat, DWORD dwErr);
void _printf(LPCSTR asFormat, DWORD dwErr, LPCWSTR asAddLine);
void _printf(LPCSTR asFormat, DWORD dw1, DWORD dw2, LPCWSTR asAddLine=NULL);
#else
#define _printf printf
#define _wprintf(s) wprintf(L"%s",s)
#endif
const wchar_t* PointToName(const wchar_t* asFullPath);
HWND Attach2Gui(DWORD nTimeout);


int ParseCommandLine(LPCWSTR asCmdLine, wchar_t** psNewCmd); // Разбор параметров командной строки
void Help();
void ExitWaitForKey(WORD vkKey, LPCWSTR asConfirm, BOOL abNewLine, BOOL abDontShowConsole);

int CreateMapHeader();
void CloseMapHeader();

void EmergencyShow();

int CreateColorerHeader();

void DisableAutoConfirmExit();

int MySetWindowRgn(CESERVER_REQ_SETWINDOWRGN* pRgn);

int InjectHooks(HANDLE hProcess, DWORD nPID);

/* Console Handles */
//extern MConHandle ghConIn;
extern MConHandle ghConOut;



typedef enum tag_RunMode {
	RM_UNDEFINED = 0,
	RM_SERVER,
	RM_COMSPEC
} RunMode;

extern BOOL gbNoCreateProcess;
extern BOOL gbDebugProcess;
extern int  gnCmdUnicodeMode;
extern BOOL gbRootIsCmdExe;

#ifdef WIN64
#pragma message("ComEmuC compiled in X64 mode")
#define NTVDMACTIVE FALSE
#else
#pragma message("ComEmuC compiled in X86 mode")
#define NTVDMACTIVE (srv.bNtvdmActive)
#endif

typedef struct tag_SrvInfo {
	HANDLE hRootProcess, hRootThread; DWORD dwRootProcess, dwRootThread; DWORD dwRootStartTime; BOOL bDebuggerActive;
	DWORD  dwGuiPID; // GUI PID (ИД процесса графической части ConEmu)
	//
	HANDLE hServerThread;   DWORD dwServerThreadId;
	HANDLE hRefreshThread;  DWORD dwRefreshThread;
	HANDLE hWinEventThread; DWORD dwWinEventThread;
	//HANDLE hInputThread;    DWORD dwInputThreadId;
	HANDLE hInputPipeThread;DWORD dwInputPipeThreadId; // Needed in Vista & administrator
	HANDLE hGetDataPipeThread; DWORD dwGetDataPipeThreadId;
	//
	OSVERSIONINFO osv;
	BOOL bReopenHandleAllowed;
	//UINT nMsgHookEnableDisable;
	UINT nMaxFPS;
	//
	MSection *csProc;
	//CRITICAL_SECTION csConBuf;
	// Список процессов нам нужен, чтобы определить, когда консоль уже не нужна.
	// Например, запустили FAR, он запустил Update, FAR перезапущен...
	//std::vector<DWORD> nProcesses;
	UINT nProcessCount, nMaxProcesses;
	DWORD* pnProcesses, *pnProcessesCopy, nProcessStartTick;
#ifndef WIN64
	BOOL bNtvdmActive; DWORD nNtvdmPID;
#endif
	BOOL bTelnetActive;
	//
	wchar_t szPipename[MAX_PATH], szInputname[MAX_PATH], szGuiPipeName[MAX_PATH], szQueryname[MAX_PATH];
	wchar_t szGetDataPipe[MAX_PATH], szDataReadyEvent[64];
	HANDLE hInputPipe, hQueryPipe, hGetDataPipe;
	//
	//HANDLE hFileMapping, hFileMappingData;
	MFileMapping<CESERVER_REQ_CONINFO_HDR> *pConsoleMap;
	//CESERVER_REQ_CONINFO_HDR *pConsoleInfo;  // Mapping
	//CESERVER_REQ_CONINFO_DATA *pConsoleData; // Mapping
	CESERVER_REQ_CONINFO_FULL *pConsole;
	//CESERVER_REQ_CONINFO_HDR *pConsoleInfoCopy;  // Local (Alloc)
	CHAR_INFO *pConsoleDataCopy; // Local (Alloc)
	//DWORD nConsoleDataSize;
//	DWORD nFarInfoLastIdx;
	// Input
	HANDLE hInputThread, hInputEvent; DWORD dwInputThread;
	int nInputQueue, nMaxInputQueue;
	INPUT_RECORD* pInputQueue;
	INPUT_RECORD* pInputQueueEnd;
	INPUT_RECORD* pInputQueueRead;
	INPUT_RECORD* pInputQueueWrite;
	// TrueColorer buffer
	HANDLE hColorerMapping;
	//
	HANDLE hConEmuGuiAttached;
	HWINEVENTHOOK /*hWinHook,*/ hWinHookStartEnd; //BOOL bWinHookAllow; int nWinHookMode;
	//BOOL bContentsChanged; // Первое чтение параметров должно быть ПОЛНЫМ
	//wchar_t* psChars;
	//WORD* pnAttrs;
	//DWORD nBufCharCount;  // максимальный размер (объем выделенной памяти)
	//DWORD nOneBufferSize; // размер для отсылки в GUI (текущий размер)
	//WORD* ptrLineCmp;
	//DWORD nLineCmpSize;
	DWORD dwCiRc; CONSOLE_CURSOR_INFO ci; // GetConsoleCursorInfo
	DWORD dwConsoleCP, dwConsoleOutputCP, dwConsoleMode;
	DWORD dwSbiRc; CONSOLE_SCREEN_BUFFER_INFO sbi; // MyGetConsoleScreenBufferInfo
	//USHORT nUsedHeight; // Высота, используемая в GUI - вместо него используем gcrBufferSize.Y
	SHORT nTopVisibleLine; // Прокрутка в GUI может быть заблокирована. Если -1 - без блокировки, используем текущее значение
	SHORT nVisibleHeight;  // По идее, должен быть равен (gcrBufferSize.Y). Это гарантированное количество строк psChars & pnAttrs
	DWORD nMainTimerElapse;
	//BOOL  bConsoleActive;
	HANDLE hRefreshEvent; // ServerMode, перечитать консоль, и если есть изменения - отослать в GUI
	HANDLE hDataReadyEvent; // Флаг, что в сервере есть изменения (GUI должен перечитать данные)
	// Смена размера консоли через RefreshThread
	int nRequestChangeSize;
	USHORT nReqSizeBufferHeight;
	COORD crReqSizeNewSize;
	SMALL_RECT rReqSizeNewRect;
	LPCSTR sReqSizeLabel;
	HANDLE hReqSizeChanged;
	//
	//HANDLE hChangingSize; // FALSE на время смены размера консоли
	//CRITICAL_ SECTION csChangeSize; DWORD ncsTChangeSize;
	//MSection cChangeSize;
	HANDLE hAllowInputEvent; BOOL bInSyncResize;
	//BOOL  bNeedFullReload;  // Нужен полный скан консоли
	//BOOL  bForceFullSend; // Необходимо отослать ПОЛНОЕ содержимое консоли, а не только измененное
	//BOOL  bRequestPostFullReload; // Во время чтения произошел ресайз - нужно запустить повторный цикл!
	//DWORD nLastUpdateTick; // Для FORCE_REDRAW_FIX
	DWORD nLastPacketID; // ИД пакета для отправки в GUI
	// Если меняется только один символ... (но перечитаем всю линию)
	//BOOL bCharChangedSet; 
	//CESERVER_CHAR CharChanged; CRITICAL_SECTION csChar;

	// Буфер для отсылки в консоль
	//DWORD nChangedBufferSize;
	//CESERVER_CHAR *pChangedBuffer;

	// Сохранненый Output последнего cmd...
	//
	// Keyboard layout name
	wchar_t szKeybLayout[KL_NAMELENGTH+1];

	// Optional console font (may be specified in registry)
	wchar_t szConsoleFont[LF_FACESIZE];
	//wchar_t szConsoleFontFile[MAX_PATH]; -- не помогает
	SHORT nConFontWidth, nConFontHeight;

	// Когда была последняя пользовательская активность
	DWORD dwLastUserTick;

	// Если нужно заблокировать нить RefreshThread
	HANDLE hLockRefreshBegin, hLockRefreshReady;

	// Console Aliases
	wchar_t* pszAliases; DWORD nAliasesSize;
} SrvInfo;

extern SrvInfo srv;

#define USER_IDLE_TIMEOUT ((DWORD)1000)
#define CHECK_IDLE_TIMEOUT 250 /* 1000 / 4 */
#define USER_ACTIVITY ((gnBufferHeight == 0) || ((GetTickCount() - srv.dwLastUserTick) <= USER_IDLE_TIMEOUT))


#pragma pack(push, 1)
extern CESERVER_CONSAVE* gpStoredOutput;
#pragma pack(pop)

typedef struct tag_CmdInfo {
    DWORD dwFarPID;
	DWORD dwSrvPID;
    BOOL  bK;
    BOOL  bNonGuiMode; // Если запущен НЕ в консоли, привязанной к GUI. Может быть из-за того, что работает как COMSPEC
    CONSOLE_SCREEN_BUFFER_INFO sbi;
    BOOL  bNewConsole;
	DWORD nExitCode;
	wchar_t szComSpecName[32];
	wchar_t szSelfName[32];
	wchar_t *pszPreAliases;
	DWORD nPreAliasSize;
} CmdInfo;

extern CmdInfo cmd;

extern COORD gcrBufferSize;
extern BOOL  gbParmBufferSize;
extern SHORT gnBufferHeight;
extern wchar_t* gpszPrevConTitle;

extern HANDLE ghLogSize;
extern wchar_t* wpszLogSizeFile;


extern BOOL gbInRecreateRoot;

//#define CES_NTVDM 0x10 -- common.hpp
//DWORD dwActiveFlags = 0;

#define CERR_GETCOMMANDLINE 100
#define CERR_CARGUMENT 101
#define CERR_CMDEXENOTFOUND 102
#define CERR_NOTENOUGHMEM1 103
#define CERR_CREATESERVERTHREAD 104
#define CERR_CREATEPROCESS 105
#define CERR_WINEVENTTHREAD 106
#define CERR_CONINFAILED 107
#define CERR_GETCONSOLEWINDOW 108
#define CERR_EXITEVENT 109
#define CERR_GLOBALUPDATE 110
#define CERR_WINHOOKNOTCREATED 111
#define CERR_CREATEINPUTTHREAD 112
#define CERR_CONOUTFAILED 113
#define CERR_PROCESSTIMEOUT 114
#define CERR_REFRESHEVENT 115
#define CERR_CREATEREFRESHTHREAD 116
#define CERR_HELPREQUESTED 118
#define CERR_ATTACHFAILED 119
#define CERR_RUNNEWCONSOLE 121
#define CERR_CANTSTARTDEBUGGER 122
#define CERR_CREATEMAPPINGERR 123
#define CERR_MAPVIEWFILEERR 124
#define CERR_COLORERMAPPINGERR 125
#define CERR_EMPTY_COMSPEC_CMDLINE 126
