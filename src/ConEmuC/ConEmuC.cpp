
#ifdef _DEBUG
//  Раскомментировать, чтобы сразу после запуска процесса (conemuc.exe) показать MessageBox, чтобы прицепиться дебаггером
//  #define SHOW_STARTED_MSGBOX
//  #define SHOW_STARTED_ASSERT
// Раскомментировать для вывода в консоль информации режима Comspec
    #define PRINT_COMSPEC(f,a) //wprintf(f,a)
#elif defined(__GNUC__)
//  Раскомментировать, чтобы сразу после запуска процесса (conemuc.exe) показать MessageBox, чтобы прицепиться дебаггером
//  #define SHOW_STARTED_MSGBOX
    #define PRINT_COMSPEC(f,a) //wprintf(f,a)
#else
	#define PRINT_COMSPEC(f,a)
#endif

#define CSECTION_NON_RAISE

#include <Windows.h>
#include <WinCon.h>
#include <stdio.h>
#include <Shlwapi.h>
#include <Tlhelp32.h>
#include <vector>
#include "..\common\common.hpp"
#include "..\common\ConEmuCheck.h"

WARNING("Обязательно после запуска сделать SetForegroundWindow на GUI окно, если в фокусе консоль");
WARNING("Обязательно получить код и имя родительского процесса");

WARNING("При запуске как ComSpec получаем ошибку: {crNewSize.X>=MIN_CON_WIDTH && crNewSize.Y>=MIN_CON_HEIGHT}");
//E:\Source\FARUnicode\trunk\unicode_far\Debug.32.vc\ConEmuC.exe /c tools\gawk.exe -f .\scripts\gendate.awk


#ifdef _DEBUG
wchar_t gszDbgModLabel[6] = {0};
#endif

#define START_MAX_PROCESSES 1000
#define CHECK_PROCESSES_TIMEOUT 500
#define CHECK_ANTIVIRUS_TIMEOUT 6*1000
#define CHECK_ROOTSTART_TIMEOUT 10*1000
#define CHECK_ROOTOK_TIMEOUT 10*1000
#define MIN_FORCEREFRESH_INTERVAL 100
#define MAX_FORCEREFRESH_INTERVAL 1000
#define MAX_SYNCSETSIZE_WAIT 1000
#define GUI_PIPE_TIMEOUT 300

#define IMAGE_SUBSYSTEM_DOS_EXECUTABLE  255

WARNING("!!!! Пока можно при появлении события запоминать текущий тик");
// и проверять его в RefreshThread. Если он не 0 - и дельта больше (100мс?)
// то принудительно перечитать консоль и сбросить тик в 0.

#ifdef _DEBUG
//CRITICAL_ SECTION gcsHeap;
//#define MCHKHEAP { Enter CriticalSection(&gcsHeap); int MDEBUG_CHK=_CrtCheckMemory(); _ASSERTE(MDEBUG_CHK); LeaveCriticalSection(&gcsHeap); }
#define MCHKHEAP HeapValidate(ghHeap, 0, NULL);
//#define HEAP_LOGGING
#define DEBUGLOG(s) //OutputDebugString(s)
#define DEBUGLOGINPUT(s) OutputDebugString(s)
#else
#define MCHKHEAP
#define DEBUGLOG(s)
#endif

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


WARNING("Наверное все-же стоит производить периодические чтения содержимого консоли, а не только по событию");

WARNING("Стоит именно здесь осуществлять проверку живости GUI окна (если оно было). Ведь может быть запущен не far, а CMD.exe");

WARNING("Если GUI умер, или не подцепился по таймауту - показать консольное окно и наверное установить шрифт поболе");

WARNING("В некоторых случаях не срабатывает ни EVENT_CONSOLE_UPDATE_SIMPLE ни EVENT_CONSOLE_UPDATE_REGION");
// Пример. Запускаем cmd.exe. печатаем какую-то муйню в командной строке и нажимаем 'Esc'
// При Esc никаких событий ВООБЩЕ не дергается, а экран в консоли изменился!

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


#ifndef CONEMUC_DLL_MODE
DWORD WINAPI InstanceThread(LPVOID);
DWORD WINAPI ServerThread(LPVOID lpvParam);
DWORD WINAPI InputThread(LPVOID lpvParam);
DWORD WINAPI InputPipeThread(LPVOID lpvParam);
BOOL GetAnswerToRequest(CESERVER_REQ& in, CESERVER_REQ** out); 
DWORD WINAPI WinEventThread(LPVOID lpvParam);
void WINAPI WinEventProc(HWINEVENTHOOK hWinEventHook, DWORD event, HWND hwnd, LONG idObject, LONG idChild, DWORD dwEventThread, DWORD dwmsEventTime);
void CheckCursorPos();
void SendConsoleChanges(CESERVER_REQ* pOut);
CESERVER_REQ* CreateConsoleInfo(CESERVER_CHAR* pRgnOnly, BOOL bCharAttrBuff);
BOOL ReloadConsoleInfo(CESERVER_CHAR* pChangedRgn=NULL); // возвращает TRUE в случае изменений
BOOL ReloadFullConsoleInfo(/*CESERVER_CHAR* pCharOnly=NULL*/); // В том числе перечитывает содержимое
DWORD WINAPI RefreshThread(LPVOID lpvParam); // Нить, перечитывающая содержимое консоли
BOOL ReadConsoleData(CESERVER_CHAR* pCheck = NULL); //((LPRECT)1) или реальный LPRECT
void SetConsoleFontSizeTo(HWND inConWnd, int inSizeY, int inSizeX, wchar_t *asFontName);
int ServerInit(); // Создать необходимые события и нити
void ServerDone(int aiRc);
int ComspecInit();
void ComspecDone(int aiRc);
BOOL SetConsoleSize(USHORT BufferHeight, COORD crNewSize, SMALL_RECT rNewRect, LPCSTR asLabel = NULL);
void CreateLogSizeFile(int nLevel);
void LogSize(COORD* pcrSize, LPCSTR pszLabel);
BOOL WINAPI HandlerRoutine(DWORD dwCtrlType);
int GetProcessCount(DWORD *rpdwPID, UINT nMaxCount);
SHORT CorrectTopVisible(int nY);
void CorrectVisibleRect(CONSOLE_SCREEN_BUFFER_INFO* pSbi);
WARNING("Вместо GetConsoleScreenBufferInfo нужно использовать MyGetConsoleScreenBufferInfo!");
BOOL MyGetConsoleScreenBufferInfo(HANDLE ahConOut, PCONSOLE_SCREEN_BUFFER_INFO apsc);
void EnlargeRegion(CESERVER_CHAR_HDR& rgn, const COORD crNew);
void CmdOutputStore();
void CmdOutputRestore();
LPVOID Alloc(size_t nCount, size_t nSize);
void Free(LPVOID ptr);
void CheckConEmuHwnd();
HWND FindConEmuByPID();
typedef BOOL (__stdcall *FGetConsoleKeyboardLayoutName)(wchar_t*);
FGetConsoleKeyboardLayoutName pfnGetConsoleKeyboardLayoutName = NULL;
void CheckKeyboardLayout();
int CALLBACK FontEnumProc(ENUMLOGFONTEX *lpelfe, NEWTEXTMETRICEX *lpntme, DWORD FontType, LPARAM lParam);
typedef DWORD (WINAPI* FGetConsoleProcessList)(LPDWORD lpdwProcessList, DWORD dwProcessCount);
FGetConsoleProcessList pfnGetConsoleProcessList = NULL;
BOOL HookWinEvents(int abEnabled);
BOOL CheckProcessCount(BOOL abForce=FALSE);
BOOL IsNeedCmd(LPCWSTR asCmdLine, BOOL *rbNeedCutStartEndQuot);
BOOL FileExists(LPCWSTR asFile);
extern bool GetImageSubsystem(const wchar_t *FileName,DWORD& ImageSubsystem);
void SendStarted();
BOOL SendConsoleEvent(INPUT_RECORD* pr, UINT nCount);


#else

PHANDLER_ROUTINE HandlerRoutine = NULL;

#endif

int ParseCommandLine(LPCWSTR asCmdLine, wchar_t** psNewCmd); // Разбор параметров командной строки
void Help();
void ExitWaitForKey(WORD vkKey, LPCWSTR asConfirm, BOOL abNewLine, BOOL abDontShowConsole);


/* Console Handles */
class MConHandle {
private:
	wchar_t   ms_Name[10];
	HANDLE    mh_Handle;
	MSection  mcs_Handle;

public:
	operator const HANDLE()
    {
    	if (mh_Handle == INVALID_HANDLE_VALUE)
		{
    		// Чтобы случайно не открыть хэндл несколько раз в разных потоках
    		MSectionLock CS; CS.Lock(&mcs_Handle, TRUE);
    		// Во время ожидания хэндл мог быт открыт в другом потоке
			if (mh_Handle == INVALID_HANDLE_VALUE) {
    			mh_Handle = CreateFile(ms_Name, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_READ,
                	0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
				if (mh_Handle == INVALID_HANDLE_VALUE) {
					DWORD dwErr = GetLastError();
					wprintf(L"CreateFile(%s) failed, ErrCode=0x%08X\n", ms_Name, dwErr); 
				}
    		}
    	}
   		return mh_Handle;
    };
    
public:
	void Close()
	{
		if (mh_Handle != INVALID_HANDLE_VALUE) {
			HANDLE h = mh_Handle;
			mh_Handle = INVALID_HANDLE_VALUE;
			CloseHandle(h);
		}
	};
	
public:
	MConHandle(LPCWSTR asName)
	{
		mh_Handle = INVALID_HANDLE_VALUE;
		lstrcpynW(ms_Name, asName, 9);
	};
	
	~MConHandle()
	{
		Close();
	};
	
};


/*  Global  */
DWORD   gnSelfPID = 0;
//HANDLE  ghConIn = NULL, ghConOut = NULL;
MConHandle ghConIn ( L"CONIN$" );
MConHandle ghConOut ( L"CONOUT$" );
HWND    ghConWnd = NULL;
HWND    ghConEmuWnd = NULL; // Root! window
HANDLE  ghExitEvent = NULL;
HANDLE  ghFinalizeEvent = NULL;
BOOL    gbAlwaysConfirmExit = FALSE, gbInShutdown = FALSE, gbAutoDisableConfirmExit = FALSE;
int     gbRootWasFoundInCon = 0;
BOOL    gbAttachMode = FALSE;
BOOL    gbForceHideConWnd = FALSE;
DWORD   gdwMainThreadId = 0;
//int       gnBufferHeight = 0;
wchar_t* gpszRunCmd = NULL;
DWORD   gnImageSubsystem = 0;
//HANDLE  ghCtrlCEvent = NULL, ghCtrlBreakEvent = NULL;
HANDLE ghHeap = NULL; //HeapCreate(HEAP_GENERATE_EXCEPTIONS, nMinHeapSize, 0);
#ifdef _DEBUG
size_t gnHeapUsed = 0, gnHeapMax = 0;
#endif

enum tag_RunMode {
    RM_UNDEFINED = 0,
    RM_SERVER,
    RM_COMSPEC
} gnRunMode = RM_UNDEFINED;

BOOL gbNoCreateProcess = FALSE;
int  gnCmdUnicodeMode = 0;
BOOL gbRootIsCmdExe = TRUE;

#ifdef WIN64
	#pragma message("ComEmuC compiled in X64 mode")
	#define NTVDMACTIVE FALSE
#else
	#pragma message("ComEmuC compiled in X86 mode")
	#define NTVDMACTIVE (srv.bNtvdmActive)
#endif

struct tag_Srv {
    HANDLE hRootProcess;    DWORD dwRootProcess;  DWORD dwRootStartTime;
    //
    HANDLE hServerThread;   DWORD dwServerThreadId;
    HANDLE hRefreshThread;  DWORD dwRefreshThread;
    HANDLE hWinEventThread; DWORD dwWinEventThread;
    HANDLE hInputThread;    DWORD dwInputThreadId;
    HANDLE hInputPipeThread;DWORD dwInputPipeThreadId; // Needed in Vista & administrator
    //
    UINT nMsgHookEnableDisable;
    UINT nMaxFPS;
    //
    MSection *csProc;
    CRITICAL_SECTION csConBuf;
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
    wchar_t szPipename[MAX_PATH], szInputname[MAX_PATH], szGuiPipeName[MAX_PATH];
    //
    HANDLE hConEmuGuiAttached;
    HWINEVENTHOOK hWinHook, hWinHookStartEnd; BOOL bWinHookAllow; int nWinHookMode;
    //BOOL bContentsChanged; // Первое чтение параметров должно быть ПОЛНЫМ
    wchar_t* psChars;
    WORD* pnAttrs;
        DWORD nBufCharCount;  // максимальный размер (объем выделенной памяти)
        DWORD nOneBufferSize; // размер для отсылки в GUI (текущий размер)
    WORD* ptrLineCmp;
        DWORD nLineCmpSize;
    DWORD dwCiRc; CONSOLE_CURSOR_INFO ci; // GetConsoleCursorInfo
    DWORD dwConsoleCP, dwConsoleOutputCP, dwConsoleMode;
    DWORD dwSbiRc; CONSOLE_SCREEN_BUFFER_INFO sbi; // MyGetConsoleScreenBufferInfo
    //USHORT nUsedHeight; // Высота, используемая в GUI - вместо него используем gcrBufferSize.Y
    SHORT nTopVisibleLine; // Прокрутка в GUI может быть заблокирована. Если -1 - без блокировки, используем текущее значение
    SHORT nVisibleHeight;  // По идее, должен быть равен (gcrBufferSize.Y). Это гарантированное количество строк psChars & pnAttrs
    DWORD nMainTimerElapse;
    BOOL  bConsoleActive;
    HANDLE hRefreshEvent; // ServerMode, перечитать консоль, и если есть изменения - отослать в GUI
    //HANDLE hChangingSize; // FALSE на время смены размера консоли
    //CRITICAL_ SECTION csChangeSize; DWORD ncsTChangeSize;
    MSection cChangeSize;
	HANDLE hAllowInputEvent; BOOL bInSyncResize;
    BOOL  bNeedFullReload;  // Нужен полный скан консоли
    BOOL  bForceFullSend; // Необходимо отослать ПОЛНОЕ содержимое консоли, а не только измененное
    BOOL  bRequestPostFullReload; // Во время чтения произошел ресайз - нужно запустить повторный цикл!
    //DWORD nLastUpdateTick; // Для FORCE_REDRAW_FIX
    DWORD nLastPacketID; // ИД пакета для отправки в GUI
    // Если меняется только один символ... (но перечитаем всю линию)
    //BOOL bCharChangedSet; 
    CESERVER_CHAR CharChanged; CRITICAL_SECTION csChar;
    
    // Буфер для отсылки в консоль
    DWORD nChangedBufferSize;
    CESERVER_CHAR *pChangedBuffer;

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
} srv = {0};

#define USER_IDLE_TIMEOUT ((DWORD)1000)
#define CHECK_IDLE_TIMEOUT 250 /* 1000 / 4 */
#define USER_ACTIVITY ((gnBufferHeight == 0) || ((GetTickCount() - srv.dwLastUserTick) <= USER_IDLE_TIMEOUT))


#pragma pack(push, 1)
CESERVER_CONSAVE* gpStoredOutput = NULL;
#pragma pack(pop)

struct tag_Cmd {
    DWORD dwFarPID;
    BOOL  bK;
    BOOL  bNonGuiMode; // Если запущен НЕ в консоли, привязанной к GUI. Может быть из-за того, что работает как COMSPEC
    CONSOLE_SCREEN_BUFFER_INFO sbi;
    BOOL  bNewConsole;
	DWORD nExitCode;
} cmd = {0};

COORD gcrBufferSize = {80,25};
BOOL  gbParmBufferSize = FALSE;
SHORT gnBufferHeight = 0;
wchar_t* gpszPrevConTitle = NULL;

HANDLE ghLogSize = NULL;
wchar_t* wpszLogSizeFile = NULL;


BOOL gbInRecreateRoot = FALSE;

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


int main()
{
    TODO("можно при ошибках показать консоль, предварительно поставив 80x25 и установив крупный шрифт");

	//#ifdef _DEBUG
    //InitializeCriticalSection(&gcsHeap);
	//#endif

    int iRc = 100;
    PROCESS_INFORMATION pi; memset(&pi, 0, sizeof(pi));
    STARTUPINFOW si; memset(&si, 0, sizeof(si)); si.cb = sizeof(si);
    DWORD dwErr = 0, nWait = 0;
    BOOL lbRc = FALSE;
    DWORD mode = 0;
    //BOOL lb = FALSE;

    ghHeap = HeapCreate(HEAP_GENERATE_EXCEPTIONS, 200000, 0);
    
    gpNullSecurity = NullSecurity();
    
    HMODULE hKernel = GetModuleHandleW (L"kernel32.dll");
    
    if (hKernel) {
        pfnGetConsoleKeyboardLayoutName = (FGetConsoleKeyboardLayoutName)GetProcAddress (hKernel, "GetConsoleKeyboardLayoutNameW");
        pfnGetConsoleProcessList = (FGetConsoleProcessList)GetProcAddress (hKernel, "GetConsoleProcessList");
    }
    

    // Хэндл консольного окна
    ghConWnd = GetConsoleWindow();
    _ASSERTE(ghConWnd!=NULL);
    if (!ghConWnd) {
        dwErr = GetLastError();
        wprintf(L"ghConWnd==NULL, ErrCode=0x%08X\n", dwErr); 
        iRc = CERR_GETCONSOLEWINDOW; goto wrap;
    }
    // PID
    gnSelfPID = GetCurrentProcessId();
    gdwMainThreadId = GetCurrentThreadId();
    
#ifdef SHOW_STARTED_MSGBOX
	if (!IsDebuggerPresent()) {
		wchar_t szTitle[100]; wsprintf(szTitle, L"ConEmuC Loaded (PID=%i)", gnSelfPID);
		const wchar_t* pszCmdLine = GetCommandLineW();
		MessageBox(NULL,pszCmdLine,szTitle,0);
	}
#endif
#ifdef SHOW_STARTED_ASSERT
	if (!IsDebuggerPresent()) {
		_ASSERT(FALSE);
	}
#endif

    PRINT_COMSPEC(L"ConEmuC started: %s\n", GetCommandLineW());
    
    if ((iRc = ParseCommandLine(GetCommandLineW(), &gpszRunCmd)) != 0)
        goto wrap;
    //#ifdef _DEBUG
    //CreateLogSizeFile();
    //#endif
    
    /* ***************************** */
    /* *** "Общая" инициализация *** */
    /* ***************************** */
    
    
    // Событие используется для всех режимов
    ghExitEvent = CreateEvent(NULL, TRUE/*используется в нескольких нитях, manual*/, FALSE, NULL);
    if (!ghExitEvent) {
        dwErr = GetLastError();
        wprintf(L"CreateEvent() failed, ErrCode=0x%08X\n", dwErr); 
        iRc = CERR_EXITEVENT; goto wrap;
    }
    ResetEvent(ghExitEvent);
    ghFinalizeEvent = CreateEvent(NULL, TRUE/*используется в нескольких нитях, manual*/, FALSE, NULL);
    if (!ghFinalizeEvent) {
        dwErr = GetLastError();
        wprintf(L"CreateEvent() failed, ErrCode=0x%08X\n", dwErr); 
        iRc = CERR_EXITEVENT; goto wrap;
    }
    ResetEvent(ghFinalizeEvent);

    // Дескрипторы
    //ghConIn  = CreateFile(L"CONIN$", GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_READ,
    //            0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
    if ((HANDLE)ghConIn == INVALID_HANDLE_VALUE) {
        dwErr = GetLastError();
        wprintf(L"CreateFile(CONIN$) failed, ErrCode=0x%08X\n", dwErr); 
        iRc = CERR_CONINFAILED; goto wrap;
    }
    // Дескрипторы
    //ghConOut = CreateFile(L"CONOUT$", GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_READ,
    //            0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
    if ((HANDLE)ghConOut == INVALID_HANDLE_VALUE) {
        dwErr = GetLastError();
        wprintf(L"CreateFile(CONOUT$) failed, ErrCode=0x%08X\n", dwErr); 
        iRc = CERR_CONOUTFAILED; goto wrap;
    }
    
    //2009-05-30 попробуем без этого ?
    //SetHandleInformation(GetStdHandle(STD_INPUT_HANDLE), HANDLE_FLAG_INHERIT, HANDLE_FLAG_INHERIT);
    //SetHandleInformation(GetStdHandle(STD_OUTPUT_HANDLE), HANDLE_FLAG_INHERIT, HANDLE_FLAG_INHERIT);
    //SetHandleInformation(GetStdHandle(STD_ERROR_HANDLE), HANDLE_FLAG_INHERIT, HANDLE_FLAG_INHERIT);
    
    mode = 0;
    /*lb = GetConsoleMode(ghConIn, &mode);
    if (!(mode & ENABLE_MOUSE_INPUT)) {
        mode |= ENABLE_MOUSE_INPUT;
        lb = SetConsoleMode(ghConIn, mode);
    }*/

    // Обязательно, иначе по CtrlC мы свалимся
    SetConsoleCtrlHandler((PHANDLER_ROUTINE)HandlerRoutine, true);
    //SetConsoleMode(ghConIn, 0);

    /* ******************************** */
    /* *** "Режимная" инициализация *** */
    /* ******************************** */
    if (gnRunMode == RM_SERVER) {
        if ((iRc = ServerInit()) != 0)
            goto wrap;
    } else {
        if ((iRc = ComspecInit()) != 0)
            goto wrap;
    }

    
    
    
    /* ********************************* */
    /* *** Запуск дочернего процесса *** */
    /* ********************************* */
    
    // CREATE_NEW_PROCESS_GROUP - низя, перестает работать Ctrl-C
	// Перед CreateProcessW нужно ставить 0, иначе из-за антивирусов может наступить
	// timeout ожидания окончания процесса еще ДО выхода из CreateProcessW
	srv.nProcessStartTick = 0;
	if (gbNoCreateProcess) {
		lbRc = TRUE; // Процесс уже запущен, просто цепляемся к ConEmu (GUI)
		pi.hProcess = srv.hRootProcess;
		pi.dwProcessId = srv.dwRootProcess;
	} else {
        lbRc = CreateProcessW(NULL, gpszRunCmd, NULL,NULL, TRUE, 
                NORMAL_PRIORITY_CLASS/*|CREATE_NEW_PROCESS_GROUP*/, 
                NULL, NULL, &si, &pi);
        dwErr = GetLastError();
		if (!lbRc && dwErr == 0x000002E4) {
			PRINT_COMSPEC(L"Vista: The requested operation requires elevation (ErrCode=0x%08X).\n", dwErr);
			// Vista: The requested operation requires elevation.
			LPCWSTR pszCmd = gpszRunCmd;
			wchar_t szVerb[10], szExec[MAX_PATH+1];
			if (NextArg(&pszCmd, szExec) == 0) {
				SHELLEXECUTEINFO sei = {sizeof(SHELLEXECUTEINFO)};
				sei.hwnd = ghConEmuWnd;
				sei.fMask = SEE_MASK_NO_CONSOLE; //SEE_MASK_NOCLOSEPROCESS; -- смысла ждать завершения нет - процесс запускается в новой консоли
				sei.lpVerb = wcscpy(szVerb, L"open");
				sei.lpFile = szExec;
				sei.lpParameters = pszCmd;
				sei.nShow = SW_SHOWNORMAL;
				if ((lbRc = ShellExecuteEx(&sei)) == FALSE) {
					dwErr = GetLastError();
				} else {
					// OK
					//pi.hProcess = sei.hProcess;
					//typedef DWORD (WINAPI* FGetProcessId)(HANDLE);
					//FGetProcessId fGetProcessId = NULL;
					//HMODULE hKernel = GetModuleHandle(L"kernel32.dll");
					//if (hKernel)
					//	fGetProcessId = (FGetProcessId)GetProcAddress(hKernel, "GetProcessId");
					//if (fGetProcessId) {
					//	pi.dwProcessId = fGetProcessId(sei.hProcess);
					//} else {
					//	// Это конечно неправильно, но произойти ошибка 0x000002E4 может только в Vista,
					//	// а там есть функция GetProcessId, так что эта ветка активироваться не должна
					//	_ASSERTE(fGetProcessId!=NULL);
					//	pi.dwProcessId = GetCurrentProcessId();
					//}
					pi.hProcess = NULL; pi.dwProcessId = 0;
					pi.hThread = NULL; pi.dwThreadId = 0;
					gbAlwaysConfirmExit = FALSE;
					iRc = 0; goto wrap;
				}
			}
		}
    }
    if (!lbRc)
    {
		wchar_t* lpMsgBuf = NULL;
		if (dwErr == 5) {
			lpMsgBuf = (wchar_t*)LocalAlloc(LPTR, 255);
			wcscpy(lpMsgBuf, L"Access is denied.\nThis may be cause of antiviral or file permissions denial.");
		} else {
			FormatMessage( FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM, NULL, dwErr, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPWSTR)&lpMsgBuf, 0, NULL );
		}
		
        wprintf (L"Can't create process, ErrCode=0x%08X, Description:\n%s\nCommand to be executed:\n%s\n", 
        	dwErr, (lpMsgBuf == NULL) ? L"<Unknown error>" : lpMsgBuf, gpszRunCmd);
        
        if (lpMsgBuf) LocalFree(lpMsgBuf);
        iRc = CERR_CREATEPROCESS; goto wrap;
    }
	srv.nProcessStartTick = GetTickCount();
    //delete psNewCmd; psNewCmd = NULL;
    AllowSetForegroundWindow(pi.dwProcessId);


    
    /* ************************ */
    /* *** Ожидание запуска *** */
    /* ************************ */
    
    if (gnRunMode == RM_SERVER) {
        srv.hRootProcess  = pi.hProcess; // Required for Win2k
        srv.dwRootProcess = pi.dwProcessId;
		srv.dwRootStartTime = GetTickCount();

		// Скорее всего процесс в консольном списке уже будет
		CheckProcessCount(TRUE);
		#ifdef _DEBUG
		if (srv.nProcessCount) {
			_ASSERTE(srv.pnProcesses[srv.nProcessCount-1]!=0);
		}
		#endif

        //if (pi.hProcess) SafeCloseHandle(pi.hProcess); 
        if (pi.hThread) SafeCloseHandle(pi.hThread);

        if (srv.hConEmuGuiAttached) {
            if (WaitForSingleObject(srv.hConEmuGuiAttached, 1000) == WAIT_OBJECT_0) {
                // GUI пайп готов
                wsprintf(srv.szGuiPipeName, CEGUIPIPENAME, L".", (DWORD)ghConWnd); // был gnSelfPID
            }
        }
    
        // Ждем, пока в консоли не останется процессов (кроме нашего)
        TODO("Проверить, может ли так получиться, что CreateProcess прошел, а к консоли он не прицепился? Может, если процесс GUI");
        nWait = WaitForSingleObject(ghFinalizeEvent, CHECK_ANTIVIRUS_TIMEOUT); //Запуск процесса наверное может задержать антивирус
        if (nWait != WAIT_OBJECT_0) { // Если таймаут
            iRc = srv.nProcessCount;
            // И процессов в консоли все еще нет
            if (iRc == 1) {
                wprintf (L"Process was not attached to console. Is it GUI?\nCommand to be executed:\n%s\n", gpszRunCmd);
                iRc = CERR_PROCESSTIMEOUT; goto wrap;
            }
        }
    } else {
        // В режиме ComSpec нас интересует завершение ТОЛЬКО дочернего процесса

        //wchar_t szEvtName[128];
        //
        //wsprintf(szEvtName, CESIGNAL_C, pi.dwProcessId);
        //ghCtrlCEvent = CreateEvent(NULL, FALSE, FALSE, szEvtName);
        //wsprintf(szEvtName, CESIGNAL_BREAK, pi.dwProcessId);
        //ghCtrlBreakEvent = CreateEvent(NULL, FALSE, FALSE, szEvtName);
    }

    /* *************************** */
    /* *** Ожидание завершения *** */
    /* *************************** */
wait:    
    if (gnRunMode == RM_SERVER) {
        // По крайней мере один процесс в консоли запустился. Ждем пока в консоли не останется никого кроме нас
        nWait = WaitForSingleObject(ghFinalizeEvent, INFINITE);
		#ifdef _DEBUG
		if (nWait == WAIT_OBJECT_0) {
			DEBUGSTR(L"*** FinilizeEvent was set!\n");
		}
		#endif
    } else {
        //HANDLE hEvents[3];
        //hEvents[0] = pi.hProcess;
        //hEvents[1] = ghCtrlCEvent;
        //hEvents[2] = ghCtrlBreakEvent;
        //WaitForSingleObject(pi.hProcess, INFINITE);
        DWORD dwWait = 0;
        dwWait = WaitForSingleObject(pi.hProcess, INFINITE);
		// Получить ExitCode
		GetExitCodeProcess(pi.hProcess, &cmd.nExitCode);

		// Сразу закрыть хэндлы
        if (pi.hProcess) SafeCloseHandle(pi.hProcess); 
        if (pi.hThread) SafeCloseHandle(pi.hThread);
    }
    
    
    
    /* ************************* */
    /* *** Завершение работы *** */
    /* ************************* */
    
    iRc = 0;
wrap:
    // К сожалению, HandlerRoutine может быть еще не вызван, поэтому
	// в самой процедуре ExitWaitForKey вставлена проверка флага gbInShutdown
    PRINT_COMSPEC(L"Finalizing. gbInShutdown=%i\n", gbInShutdown);
	#ifdef SHOW_STARTED_MSGBOX
	MessageBox(GetConsoleWindow(), L"Finalizing", L"ConEmuC.ComSpec", 0);
	#endif
    if (!gbInShutdown
		&& ((iRc!=0 && iRc!=CERR_RUNNEWCONSOLE) || gbAlwaysConfirmExit)
		)
	{
		BOOL lbProcessesLeft = FALSE, lbDontShowConsole = FALSE;
		if (pfnGetConsoleProcessList) {
			DWORD nProcesses[10];
			DWORD nProcCount = pfnGetConsoleProcessList ( nProcesses, 10 );
			if (nProcCount > 1)
				lbProcessesLeft = TRUE;
		}

		LPCWSTR pszMsg = NULL;
		if (lbProcessesLeft) {
			pszMsg = L"\n\nPress Enter to exit...";
			lbDontShowConsole = gnRunMode != RM_SERVER;
		} else {
			if (gbRootWasFoundInCon == 1)
				pszMsg = L"\n\nPress Enter to close console...";
		}
		if (!pszMsg) // Иначе - сообщение по умолчанию
			pszMsg = L"\n\nPress Enter to close console, or wait...";

        ExitWaitForKey(VK_RETURN, pszMsg, TRUE, lbDontShowConsole);
        if (iRc == CERR_PROCESSTIMEOUT) {
            int nCount = srv.nProcessCount;
            if (nCount > 1) {
                // Процесс таки запустился!
                goto wait;
            }
        }
    }

    // На всякий случай - выставим событие
    if (ghExitEvent) SetEvent(ghExitEvent);
    
    
    /* ***************************** */
    /* *** "Режимное" завершение *** */
    /* ***************************** */
    
    if (gnRunMode == RM_SERVER) {
        ServerDone(iRc);
        //MessageBox(0,L"Server done...",L"ConEmuC",0);
    } else {
        ComspecDone(iRc);
        //MessageBox(0,L"Comspec done...",L"ConEmuC",0);
    }

    
    /* ************************** */
    /* *** "Общее" завершение *** */
    /* ************************** */
    
    if (gpszPrevConTitle && ghConWnd) {
        SetConsoleTitleW(gpszPrevConTitle);
        Free(gpszPrevConTitle);
    }
    
    ghConIn.Close();
	ghConOut.Close();

    SafeCloseHandle(ghLogSize);
    if (wpszLogSizeFile) {
        //DeleteFile(wpszLogSizeFile);
        Free(wpszLogSizeFile); wpszLogSizeFile = NULL;
    }
    
    if (gpszRunCmd) { delete gpszRunCmd; gpszRunCmd = NULL; }

    if (ghHeap) {
        HeapDestroy(ghHeap);
        ghHeap = NULL;
    }

	// Если режим ComSpec - вернуть код возврата из запущенного процесса
	if (iRc == 0 && gnRunMode == RM_COMSPEC)
		iRc = cmd.nExitCode;

    return iRc;
}

void Help()
{
    wprintf(
        L"ConEmuC. Copyright (c) 2009, Maximus5\n"
        L"This is a console part of ConEmu product.\n"
        L"Usage: ConEmuC [switches] [/U | /A] /C <command line, passed to %%COMSPEC%%>\n"
        L"   or: ConEmuC [switches] /CMD <program with arguments, far.exe for example>\n"
        L"   or: ConEmuC /ATTACH /NOCMD\n"
        L"   or: ConEmuC /?\n"
        L"Switches:\n"
        L"        /CONFIRM  - confirm closing console on program termination\n"
        L"        /ATTACH   - auto attach to ConEmu GUI\n"
        L"        /NOCMD    - attach current (existing) console to GUI\n"
        L"        /B{W|H|Z} - define buffer width, height, window height\n"
        L"        /LOG[0]   - create (debug) log file\n"
    );
}

#pragma warning( push )
#pragma warning(disable : 6400)
BOOL IsExecutable(LPCWSTR aszFilePathName)
{
	#pragma warning( push )
	#pragma warning(disable : 6400)
	LPCWSTR pwszDot = wcsrchr(aszFilePathName, L'.');
	if (pwszDot) { // Если указан .exe или .com файл
		if (lstrcmpiW(pwszDot, L".exe")==0 || lstrcmpiW(pwszDot, L".com")==0) {
			if (FileExists(aszFilePathName))
				return TRUE;
		}
	}
	return FALSE;
}
#pragma warning( pop )

BOOL IsNeedCmd(LPCWSTR asCmdLine, BOOL *rbNeedCutStartEndQuot)
{
	_ASSERTE(asCmdLine && *asCmdLine);

	gbRootIsCmdExe = TRUE;

	if (!asCmdLine || *asCmdLine == 0)
		return TRUE;

	if (wcschr(asCmdLine, L'&') || 
		wcschr(asCmdLine, L'>') || 
		wcschr(asCmdLine, L'<') || 
		wcschr(asCmdLine, L'|'))
	{
		// Если есть одна из команд перенаправления, или слияния - нужен CMD.EXE
		return TRUE;
	}

	wchar_t szArg[MAX_PATH+10] = {0};
	int iRc = 0;
	BOOL lbFirstWasGot = FALSE;
	LPCWSTR pwszCopy = asCmdLine;

	// cmd /c ""c:\program files\arc\7z.exe" -?"   // да еще и внутри могут быть двойными...
	// cmd /c "dir c:\"
	int nLastChar = lstrlenW(pwszCopy) - 1;
	if (pwszCopy[0] == L'"' && pwszCopy[nLastChar] == L'"') {
		if (pwszCopy[1] == L'"' && pwszCopy[2]) {
			pwszCopy ++; // Отбросить первую кавычку в командах типа: ""c:\program files\arc\7z.exe" -?"
			if (rbNeedCutStartEndQuot) *rbNeedCutStartEndQuot = TRUE;
		} else
		// глючила на ""F:\VCProject\FarPlugin\#FAR180\far.exe  -new_console""
		//if (wcschr(pwszCopy+1, L'"') == (pwszCopy+nLastChar)) {
		//	LPCWSTR pwszTemp = pwszCopy;
		//	// Получим первую команду (исполняемый файл?)
		//	if ((iRc = NextArg(&pwszTemp, szArg)) != 0) {
		//		//Parsing command line failed
		//		return TRUE;
		//	}
		//	pwszCopy ++; // Отбросить первую кавычку в командах типа: "c:\arc\7z.exe -?"
		//	lbFirstWasGot = TRUE;
		//	if (rbNeedCutStartEndQuot) *rbNeedCutStartEndQuot = TRUE;
		//} else
		{
			// отбросить первую кавычку в: "C:\GCC\msys\bin\make.EXE -f "makefile" COMMON="../../../plugins/common""
			LPCWSTR pwszTemp = pwszCopy + 1;
			// Получим первую команду (исполняемый файл?)
			if ((iRc = NextArg(&pwszTemp, szArg)) != 0) {
				//Parsing command line failed
				return TRUE;
			}
			LPCWSTR pwszQ = pwszCopy + 1 + wcslen(szArg);
			if (*pwszQ != L'"' && IsExecutable(szArg)) {
				pwszCopy ++; // отбрасываем
				lbFirstWasGot = TRUE;
				if (rbNeedCutStartEndQuot) *rbNeedCutStartEndQuot = TRUE;
			}
		}
	}

	// Получим первую команду (исполняемый файл?)
	if (!lbFirstWasGot) {
		if ((iRc = NextArg(&pwszCopy, szArg)) != 0) {
			//Parsing command line failed
			return TRUE;
		}
	}
	pwszCopy = wcsrchr(szArg, L'\\'); if (!pwszCopy) pwszCopy = szArg; else pwszCopy ++;
	//2009-08-27
	wchar_t *pwszEndSpace = szArg + lstrlenW(szArg) - 1;
	while (*pwszEndSpace == L' ' && pwszEndSpace > szArg)
		*(pwszEndSpace--) = 0;

	#pragma warning( push )
	#pragma warning(disable : 6400)

	if (lstrcmpiW(pwszCopy, L"cmd")==0 || lstrcmpiW(pwszCopy, L"cmd.exe")==0)
	{
		gbRootIsCmdExe = TRUE; // уже должен быть выставлен, но проверим
	    return FALSE; // уже указан командный процессор, cmd.exe в начало добавлять не нужно
	}

	if (lstrcmpiW(pwszCopy, L"far")==0 || lstrcmpiW(pwszCopy, L"far.exe")==0)
	{
		gbAutoDisableConfirmExit = TRUE;
		gbRootIsCmdExe = FALSE; // FAR!
		return FALSE; // уже указан командный процессор, cmd.exe в начало добавлять не нужно
	}

	if (IsExecutable(szArg)) {
		gbRootIsCmdExe = FALSE; // Для других программ - буфер не включаем
		return FALSE; // Запускается конкретная консольная программа. cmd.exe не требуется
	}

	//Можно еще Доделать поиски с: SearchPath, GetFullPathName, добавив расширения .exe & .com
	//хотя фар сам формирует полные пути к командам, так что можно не заморачиваться

	gbRootIsCmdExe = TRUE;
	#pragma warning( pop )
	return TRUE;
}

BOOL FileExists(LPCWSTR asFile)
{
	WIN32_FIND_DATA fnd; memset(&fnd, 0, sizeof(fnd));
	HANDLE h = FindFirstFile(asFile, &fnd);
	if (h != INVALID_HANDLE_VALUE) {
		FindClose(h);
		return (fnd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0;
	}
	return FALSE;
}

void CheckUnicodeMode()
{
	if (gnCmdUnicodeMode) return;
	wchar_t szValue[16] = {0};
	if (GetEnvironmentVariable(L"ConEmuOutput", szValue, sizeof(szValue)/sizeof(szValue[0]))) {
		if (lstrcmpi(szValue, L"UNICODE") == 0)
			gnCmdUnicodeMode = 2;
		else if (lstrcmpi(szValue, L"ANSI") == 0)
			gnCmdUnicodeMode = 1;
	}
}

// Разбор параметров командной строки
int ParseCommandLine(LPCWSTR asCmdLine, wchar_t** psNewCmd)
{
    int iRc = 0;
    wchar_t szArg[MAX_PATH+1] = {0};
    wchar_t szComSpec[MAX_PATH+1] = {0};
    LPCWSTR pwszCopy = NULL;
    wchar_t* psFilePart = NULL;
    BOOL bViaCmdExe = TRUE;
	gbRootIsCmdExe = TRUE;
    size_t nCmdLine = 0;
    LPCWSTR pwszStartCmdLine = asCmdLine;
	BOOL lbNeedCutStartEndQuot = FALSE;
    
    if (!asCmdLine || !*asCmdLine)
    {
        DWORD dwErr = GetLastError();
        wprintf (L"GetCommandLineW failed! ErrCode=0x%08X\n", dwErr);
        return CERR_GETCOMMANDLINE;
    }

    gnRunMode = RM_UNDEFINED;
    
    
    while ((iRc = NextArg(&asCmdLine, szArg)) == 0)
    {
        if (wcscmp(szArg, L"/?")==0 || wcscmp(szArg, L"-?")==0 || wcscmp(szArg, L"/h")==0 || wcscmp(szArg, L"-h")==0) {
            Help();
            return CERR_HELPREQUESTED;
        } else 
        
        if (wcscmp(szArg, L"/CONFIRM")==0) {
            gbAlwaysConfirmExit = TRUE;
        } else

        if (wcscmp(szArg, L"/ATTACH")==0) {
            gbAttachMode = TRUE;
            gnRunMode = RM_SERVER;
        } else

		if (wcscmp(szArg, L"/HIDE")==0) {
			gbForceHideConWnd = TRUE;
		} else

        if (wcsncmp(szArg, L"/B", 2)==0) {
            if (wcsncmp(szArg, L"/BW=", 4)==0) {
                gcrBufferSize.X = _wtoi(szArg+4); gbParmBufferSize = TRUE;
            } else if (wcsncmp(szArg, L"/BH=", 4)==0) {
                gcrBufferSize.Y = _wtoi(szArg+4); gbParmBufferSize = TRUE;
            } else if (wcsncmp(szArg, L"/BZ=", 4)==0) {
                gnBufferHeight = _wtoi(szArg+4); gbParmBufferSize = TRUE;
            }
        } else

        if (wcsncmp(szArg, L"/F", 2)==0) {
            if (wcsncmp(szArg, L"/FN=", 4)==0) {
                lstrcpynW(srv.szConsoleFont, szArg+4, 32);
            } else if (wcsncmp(szArg, L"/FW=", 4)==0) {
                srv.nConFontWidth = _wtoi(szArg+4);
            } else if (wcsncmp(szArg, L"/FH=", 4)==0) {
                srv.nConFontHeight = _wtoi(szArg+4);
            //} else if (wcsncmp(szArg, L"/FF=", 4)==0) {
            //  lstrcpynW(srv.szConsoleFontFile, szArg+4, MAX_PATH);
            }
        } else
        
        if (wcscmp(szArg, L"/LOG")==0) {
            CreateLogSizeFile(1);
        } else
        if (wcscmp(szArg, L"/LOG0")==0) {
            CreateLogSizeFile(0);
        } else
        
        if (wcscmp(szArg, L"/NOCMD")==0) {
        	gnRunMode = RM_SERVER;
        	gbNoCreateProcess = TRUE;
        } else

        if (wcsncmp(szArg, L"/PID=", 5)==0) {
        	gnRunMode = RM_SERVER;
        	gbNoCreateProcess = TRUE;
			srv.dwRootProcess = _wtol(szArg+5);
			if (srv.dwRootProcess == 0) {
				wprintf (L"Attach to GUI was requested, but invalid PID specified:\n%s\n", GetCommandLineW());
				return CERR_CARGUMENT;
			}
        } else
        
		if (wcscmp(szArg, L"/A")==0 || wcscmp(szArg, L"/a")==0) {
			gnCmdUnicodeMode = 1;
		} else
        if (wcscmp(szArg, L"/U")==0 || wcscmp(szArg, L"/u")==0) {
        	gnCmdUnicodeMode = 2;
        } else

        // После этих аргументов - идет то, что передается в CreateProcess!
        if (wcscmp(szArg, L"/C")==0 || wcscmp(szArg, L"/c")==0 || wcscmp(szArg, L"/K")==0 || wcscmp(szArg, L"/k")==0) {
            gnRunMode = RM_COMSPEC; gbNoCreateProcess = FALSE;
            cmd.bK = (wcscmp(szArg, L"/K")==0 || wcscmp(szArg, L"/k")==0);
            break; // asCmdLine уже указывает на запускаемую программу
        } else if (wcscmp(szArg, L"/CMD")==0 || wcscmp(szArg, L"/cmd")==0) {
            gnRunMode = RM_SERVER; gbNoCreateProcess = FALSE;
            break; // asCmdLine уже указывает на запускаемую программу
        }
    }
    
    if (gnRunMode == RM_SERVER && gbNoCreateProcess && gbAttachMode) {
		if (pfnGetConsoleProcessList==NULL) {
            wprintf (L"Attach to GUI was requested, but required WinXP or higher:\n%s\n", GetCommandLineW());
            return CERR_CARGUMENT;
		}
		DWORD nProcesses[10];
    	DWORD nProcCount = pfnGetConsoleProcessList ( nProcesses, 10 );
    	if (nProcCount < 2) {
            wprintf (L"Attach to GUI was requested, but there is no console processes:\n%s\n", GetCommandLineW());
            return CERR_CARGUMENT;
    	}
    	// Если cmd.exe запущен из cmd.exe (в консоли уже больше двух процессов) - ничего не делать
    	if (nProcCount > 2) {
    		// И ругаться только под отладчиком
    		wchar_t szProc[128] ={0}, szTmp[10]; //wsprintfW(szProc, L"%i, %i, %i", nProcesses[0], nProcesses[1], nProcesses[2]);
    		for (DWORD n=0; n<nProcCount; n++) {
    			if (n) lstrcatW(szProc, L", ");
    			lstrcatW(szProc, _ltow(nProcesses[0], szTmp, 10));
    		}
    		PRINT_COMSPEC(L"Attach to GUI was requested, but there is more then 2 console processes: %s\n", szProc);
    		return CERR_CARGUMENT;
    	}

        wchar_t* pszNewCmd = new wchar_t[1];
        if (!pszNewCmd) {
            wprintf (L"Can't allocate 1 wchar!\n");
            return CERR_NOTENOUGHMEM1;
        }
        pszNewCmd[0] = 0;
        return 0;
    }

    if (iRc != 0) {
        if (iRc == CERR_CMDLINEEMPTY) {
            Help();
            wprintf (L"\n\nParsing command line failed (/C argument not found):\n%s\n", GetCommandLineW());
        } else {
            wprintf (L"Parsing command line failed:\n%s\n", asCmdLine);
        }
        return iRc;
    }
    if (gnRunMode == RM_UNDEFINED) {
        wprintf (L"Parsing command line failed (/C argument not found):\n%s\n", GetCommandLineW());
        return CERR_CARGUMENT;
    }

    if (gnRunMode == RM_COMSPEC) {
    
        // Может просили открыть новую консоль?
        int nArgLen = lstrlenA(" -new_console");
        pwszCopy = (wchar_t*)wcsstr(asCmdLine, L" -new_console");
        // Если после -new_console идет пробел, или это вообще конец строки
        if (pwszCopy && 
            (pwszCopy[nArgLen]==L' ' || pwszCopy[nArgLen]==0
             || (pwszCopy[nArgLen]==L'"' || pwszCopy[nArgLen+1]==0)))
        {
            // тогда обрабатываем
            cmd.bNewConsole = TRUE;
            //
            size_t nNewLen = wcslen(pwszStartCmdLine) + 130;
            //
            BOOL lbIsNeedCmd = IsNeedCmd(asCmdLine, &lbNeedCutStartEndQuot);
            
            // Font, size, etc.
            
    	    CESERVER_REQ *pIn = NULL, *pOut = NULL;
    	    wchar_t* pszAddNewConArgs = NULL;
    	    if ((pIn = ExecuteNewCmd(CECMD_GETNEWCONPARM, sizeof(CESERVER_REQ_HDR)+2*sizeof(DWORD))) != NULL) {
    	        pIn->dwData[0] = gnSelfPID;
    	        pIn->dwData[1] = lbIsNeedCmd;
    	        
                PRINT_COMSPEC(L"Retrieve new console add args (begin)\n",0);
                pOut = ExecuteGuiCmd(ghConWnd, pIn, ghConWnd);
                PRINT_COMSPEC(L"Retrieve new console add args (begin)\n",0);
                
                if (pOut) {
                    pszAddNewConArgs = (wchar_t*)pOut->Data;
                    if (*pszAddNewConArgs == 0) {
                        ExecuteFreeResult(pOut); pOut = NULL; pszAddNewConArgs = NULL;
                    } else {
                        nNewLen += wcslen(pszAddNewConArgs) + 1;
                    }
                }
                ExecuteFreeResult(pIn); pIn = NULL;
    	    }
            //
            wchar_t* pszNewCmd = new wchar_t[nNewLen];
            if (!pszNewCmd) {
                wprintf (L"Can't allocate %i wchars!\n", nNewLen);
                return CERR_NOTENOUGHMEM1;
            }
            // Сначала скопировать все, что было ДО /c
            const wchar_t* pszC = asCmdLine;
            while (*pszC != L'/') pszC --;
            nNewLen = pszC - pwszStartCmdLine;
            _ASSERTE(nNewLen>0);
            wcsncpy(pszNewCmd, pwszStartCmdLine, nNewLen);
            pszNewCmd[nNewLen] = 0; // !!! wcsncpy не ставит завершающий '\0'
            // Поправим режимы открытия
            if (!gbAttachMode) // Если ключа еще нет в ком.строке - добавим
                wcscat(pszNewCmd, L" /ATTACH ");
            if (!gbAlwaysConfirmExit) // Если ключа еще нет в ком.строке - добавим
                wcscat(pszNewCmd, L" /CONFIRM ");
            if (pszAddNewConArgs) {
                wcscat(pszNewCmd, L" ");
                wcscat(pszNewCmd, pszAddNewConArgs);
            }
            // Размеры должны быть такими-же
			//2009-08-13 было закомментарено (в режиме ComSpec аргументы /BW /BH /BZ отсутствуют, т.к. запуск идет из FAR)
			//			 иногда получалось, что требуемый размер (он запрашивается из GUI) 
			//			 не успевал установиться и в некоторых случаях возникал
			//			 глюк размера (видимой высоты в GUI) в ReadConsoleData
			if (MyGetConsoleScreenBufferInfo(ghConOut, &cmd.sbi)) {
				int nBW = cmd.sbi.dwSize.X;
				int nBH = cmd.sbi.srWindow.Bottom - cmd.sbi.srWindow.Top + 1;
				int nBZ = cmd.sbi.dwSize.Y;
				if (nBZ <= nBH) nBZ = 0;
				wsprintf(pszNewCmd+wcslen(pszNewCmd), L" /BW=%i /BH=%i /BZ=%i ", nBW, nBH, nBZ);
			}
            //wcscat(pszNewCmd, L" </BW=9999 /BH=9999 /BZ=9999> ");
            // Сформировать новую команду
            // "cmd" потому что пока не хочется обрезать кавычки и думать, реально ли он нужен
            // cmd /c ""c:\program files\arc\7z.exe" -?"   // да еще и внутри могут быть двойными...
            // cmd /c "dir c:\"
            // и пр.
			// Попытаться определить необходимость cmd
			if (lbIsNeedCmd) {
				CheckUnicodeMode();
				if (gnCmdUnicodeMode == 2)
					wcscat(pszNewCmd, L" /CMD cmd /U /C ");
				else if (gnCmdUnicodeMode == 1)
					wcscat(pszNewCmd, L" /CMD cmd /A /C ");
				else
					wcscat(pszNewCmd, L" /CMD cmd /C ");
			} else {
				wcscat(pszNewCmd, L" /CMD ");
			}
			// убрать из запускаемой команды "-new_console"
            nNewLen = pwszCopy - asCmdLine;
            psFilePart = pszNewCmd + lstrlenW(pszNewCmd);
            wcsncpy(psFilePart, asCmdLine, nNewLen);
			psFilePart[nNewLen] = 0; // !!! wcsncpy не ставит завершающий '\0'
			psFilePart += nNewLen;
            pwszCopy += nArgLen;
			// добавить в команду запуска собственно программу с аргументами
            if (*pwszCopy) wcscpy(psFilePart, pwszCopy);
            //MessageBox(NULL, pszNewCmd, L"CmdLine", 0);
            //return 200;
            // Можно запускаться
            *psNewCmd = pszNewCmd;
            // 26.06.2009 Maks - чтобы сразу выйти - вся обработка будет в новой консоли.
            gbAlwaysConfirmExit = FALSE;
			srv.nProcessStartTick = GetTickCount() - 2*CHECK_ROOTSTART_TIMEOUT;
            return 0;
        }

        //pwszCopy = asCmdLine;
        //if ((iRc = NextArg(&pwszCopy, szArg)) != 0) {
        //    wprintf (L"Parsing command line failed:\n%s\n", asCmdLine);
        //    return iRc;
        //}
        //pwszCopy = wcsrchr(szArg, L'\\'); if (!pwszCopy) pwszCopy = szArg;
    
        //#pragma warning( push )
        //#pragma warning(disable : 6400)
        //if (lstrcmpiW(pwszCopy, L"cmd")==0 || lstrcmpiW(pwszCopy, L"cmd.exe")==0) {
        //    bViaCmdExe = FALSE; // уже указан командный процессор, cmd.exe в начало добавлять не нужно
        //}
        //#pragma warning( pop )
    //} else {
    //    bViaCmdExe = FALSE; // командным процессором выступает сам ConEmuC (серверный режим)
    }


	bViaCmdExe = IsNeedCmd(asCmdLine, &lbNeedCutStartEndQuot);
    
    nCmdLine = lstrlenW(asCmdLine);

    if (!bViaCmdExe) {
        nCmdLine += 1; // только место под 0
    } else {
        // Если определена ComSpecC - значит ConEmuC переопределил стандартный ComSpec
        if (!GetEnvironmentVariable(L"ComSpecC", szComSpec, MAX_PATH) || szComSpec[0] == 0)
            if (!GetEnvironmentVariable(L"ComSpec", szComSpec, MAX_PATH) || szComSpec[0] == 0)
                szComSpec[0] = 0;
        if (szComSpec[0] != 0) {
            // Только если это (случайно) не conemuc.exe
            pwszCopy = wcsrchr(szComSpec, L'\\'); if (!pwszCopy) pwszCopy = szComSpec;
            #pragma warning( push )
            #pragma warning(disable : 6400)
            if (lstrcmpiW(pwszCopy, L"ConEmuC")==0 || lstrcmpiW(pwszCopy, L"ConEmuC.exe")==0)
                szComSpec[0] = 0;
            #pragma warning( pop )
        }
        
        // ComSpec/ComSpecC не определен, используем cmd.exe
        if (szComSpec[0] == 0) {
            if (!SearchPathW(NULL, L"cmd.exe", NULL, MAX_PATH, szComSpec, &psFilePart))
            {
                wprintf (L"Can't find cmd.exe!\n");
                return CERR_CMDEXENOTFOUND;
            }
        }

        nCmdLine += lstrlenW(szComSpec)+15; // "/C", кавычки и возможный "/U"
    }

    *psNewCmd = new wchar_t[nCmdLine];
    if (!(*psNewCmd))
    {
        wprintf (L"Can't allocate %i wchars!\n", nCmdLine);
        return CERR_NOTENOUGHMEM1;
    }
    
	// это нужно для смены заголовка консоли. при необходимости COMSPEC впишем ниже, после смены
    lstrcpyW( *psNewCmd, asCmdLine );

    
    // Сменим заголовок консоли
    if (*asCmdLine == L'"') {
        if (asCmdLine[1]) {
            wchar_t *pszTitle = *psNewCmd;
            wchar_t *pszEndQ = pszTitle + lstrlenW(pszTitle) - 1;
            if (pszEndQ > (pszTitle+1) && *pszEndQ == L'"'
				&& wcschr(pszTitle+1, L'"') == pszEndQ)
			{
                *pszEndQ = 0; pszTitle ++; lbNeedCutStartEndQuot = TRUE;
            } else {
                pszEndQ = NULL;
            }
            int nLen = 4096; //GetWindowTextLength(ghConWnd); -- KIS2009 гундит "Посылка оконного сообщения"...
            if (nLen > 0) {
                gpszPrevConTitle = (wchar_t*)Alloc(nLen+1,2);
                if (gpszPrevConTitle) {
                    if (!GetConsoleTitleW(gpszPrevConTitle, nLen+1)) {
                        Free(gpszPrevConTitle); gpszPrevConTitle = NULL;
                    }
                }
            }
            SetConsoleTitleW(pszTitle);
            if (pszEndQ) *pszEndQ = L'"';
        }
    } else if (*asCmdLine) {
        int nLen = 4096; //GetWindowTextLength(ghConWnd); -- KIS2009 гундит "Посылка оконного сообщения"...
        if (nLen > 0) {
            gpszPrevConTitle = (wchar_t*)Alloc(nLen+1,2);
            if (gpszPrevConTitle) {
                if (!GetConsoleTitleW(gpszPrevConTitle, nLen+1)) {
                    Free(gpszPrevConTitle); gpszPrevConTitle = NULL;
                }
            }
        }
        SetConsoleTitleW(asCmdLine);
    }
    
    if (bViaCmdExe)
    {
		CheckUnicodeMode();
        if (wcschr(szComSpec, L' ')) {
            (*psNewCmd)[0] = L'"';
            lstrcpyW( (*psNewCmd)+1, szComSpec );
            if (gnCmdUnicodeMode)
				lstrcatW( (*psNewCmd), (gnCmdUnicodeMode == 2) ? L" /U" : L" /A");
            lstrcatW( (*psNewCmd), cmd.bK ? L"\" /K " : L"\" /C " );
        } else {
            lstrcpyW( (*psNewCmd), szComSpec );
			if (gnCmdUnicodeMode)
				lstrcatW( (*psNewCmd), (gnCmdUnicodeMode == 2) ? L" /U" : L" /A");
            lstrcatW( (*psNewCmd), cmd.bK ? L" /K " : L" /C " );
        }
		BOOL lbNeedQuatete = TRUE;
		// Команды в cmd.exe лучше передавать так:
		// ""c:\program files\arc\7z.exe" -?"
		int nLastChar = lstrlenW(asCmdLine) - 1;
		if (asCmdLine[0] == L'"' && asCmdLine[nLastChar] == L'"') {
			// Наверное можно положиться на фар, и не кавычить самостоятельно
			if (gnRunMode == RM_COMSPEC)
				lbNeedQuatete = FALSE;
			//if (asCmdLine[1] == L'"' && asCmdLine[2])
			//	lbNeedQuatete = FALSE; // уже
			//else if (wcschr(asCmdLine+1, L'"') == (asCmdLine+nLastChar))
			//	lbNeedQuatete = FALSE; // не требуется. внутри кавычек нет
		} 
		if (lbNeedQuatete) { // надо
			lstrcatW( (*psNewCmd), L"\"" );
		}
		// Собственно, командная строка
        lstrcatW( (*psNewCmd), asCmdLine );
		if (lbNeedQuatete)
			lstrcatW( (*psNewCmd), L"\"" );
	} else if (lbNeedCutStartEndQuot) {
		// ""c:\arc\7z.exe -?"" - не запустится!
		lstrcpyW( *psNewCmd, asCmdLine+1 );
		wchar_t *pszEndQ = *psNewCmd + lstrlenW(*psNewCmd) - 1;
		_ASSERTE(pszEndQ && *pszEndQ == L'"');
		if (pszEndQ && *pszEndQ == L'"') *pszEndQ = 0;
	}

#ifdef _DEBUG
	OutputDebugString(*psNewCmd); OutputDebugString(L"\n");
#endif
    
    return 0;
}

//int NextArg(LPCWSTR &asCmdLine, wchar_t* rsArg/*[MAX_PATH+1]*/)
//{
//    LPCWSTR psCmdLine = asCmdLine, pch = NULL;
//    wchar_t ch = *psCmdLine;
//    int nArgLen = 0;
//    
//    while (ch == L' ' || ch == L'\t' || ch == L'\r' || ch == L'\n') ch = *(++psCmdLine);
//    if (ch == 0) return CERR_CMDLINEEMPTY;
//
//    // аргумент начинается с "
//    if (ch == L'"') {
//        psCmdLine++;
//        pch = wcschr(psCmdLine, L'"');
//        if (!pch) return CERR_CMDLINE;
//        while (pch[1] == L'"') {
//            pch += 2;
//            pch = wcschr(pch, L'"');
//            if (!pch) return CERR_CMDLINE;
//        }
//        // Теперь в pch ссылка на последнюю "
//    } else {
//        // До конца строки или до первого пробела
//        //pch = wcschr(psCmdLine, L' ');
//        // 09.06.2009 Maks - обломался на: cmd /c" echo Y "
//        pch = psCmdLine;
//        while (*pch && *pch!=L' ' && *pch!=L'"') pch++;
//        //if (!pch) pch = psCmdLine + wcslen(psCmdLine); // до конца строки
//    }
//    
//    nArgLen = pch - psCmdLine;
//    if (nArgLen > MAX_PATH) return CERR_CMDLINE;
//
//    // Вернуть аргумент
//    memcpy(rsArg, psCmdLine, nArgLen*sizeof(wchar_t));
//    rsArg[nArgLen] = 0;
//
//    psCmdLine = pch;
//    
//    // Finalize
//    ch = *psCmdLine; // может указывать на закрывающую кавычку
//    if (ch == L'"') ch = *(++psCmdLine);
//    while (ch == L' ' || ch == L'\t' || ch == L'\r' || ch == L'\n') ch = *(++psCmdLine);
//    asCmdLine = psCmdLine;
//    
//    return 0;
//}

void EmergencyShow()
{
	SetWindowPos(ghConWnd, HWND_TOP, 50,50,0,0, SWP_NOSIZE);
	ShowWindowAsync(ghConWnd, SW_SHOWNORMAL);
	EnableWindow(ghConWnd, true);
}

void ExitWaitForKey(WORD vkKey, LPCWSTR asConfirm, BOOL abNewLine, BOOL abDontShowConsole)
{
    // Чтобы ошибку было нормально видно
	if (!abDontShowConsole)
	{
		BOOL lbNeedVisible = FALSE;
		if (!ghConWnd) ghConWnd = GetConsoleWindow();
		if (ghConWnd) { // Если консоль была скрыта
			WARNING("Если GUI жив - отвечает на запросы SendMessageTimeout - показывать консоль не нужно. Не красиво получается");
			if (!IsWindowVisible(ghConWnd)) {
				BOOL lbGuiAlive = FALSE;
				if (ghConEmuWnd && IsWindow(ghConEmuWnd)) {
					DWORD_PTR dwLRc = 0;
					if (SendMessageTimeout(ghConEmuWnd, WM_NULL, 0, 0, SMTO_ABORTIFHUNG, 1000, &dwLRc))
						lbGuiAlive = TRUE;
				}
				if (!lbGuiAlive && !IsWindowVisible(ghConWnd)) {
					lbNeedVisible = TRUE;
					// не надо наверное... // поставить "стандартный" 80x25, или то, что было передано к ком.строке
					//SMALL_RECT rcNil = {0}; SetConsoleSize(0, gcrBufferSize, rcNil, ":Exiting");
					//SetConsoleFontSizeTo(ghConWnd, 8, 12); // установим шрифт побольше
					//ShowWindow(ghConWnd, SW_SHOWNORMAL); // и покажем окошко
					EmergencyShow();
				}
			}
		}
	}

    // Сначала почистить буфер
    INPUT_RECORD r = {0}; DWORD dwCount = 0;
    FlushConsoleInputBuffer(GetStdHandle(STD_INPUT_HANDLE));

    PRINT_COMSPEC(L"Finalizing. gbInShutdown=%i\n", gbInShutdown);
    if (gbInShutdown)
        return; // Event закрытия мог припоздниться
    //
    wprintf(asConfirm);

    //if (lbNeedVisible)
    // Если окошко опять было скрыто - значит GUI часть жива, и опять отображаться не нужно
    //while (PeekConsoleInput(GetStdHandle(STD_INPUT_HANDLE), &r, 1, &dwCount)) {
    //    if (dwCount)
    //        ReadConsoleInput(GetStdHandle(STD_INPUT_HANDLE), &r, 1, &dwCount);
    //    else
    //        Sleep(100);
    //    if (lbNeedVisible && !IsWindowVisible(ghConWnd)) {
    //        ShowWindow(ghConWnd, SW_SHOWNORMAL); // и покажем окошко
    //    }
    while (ReadConsoleInput(GetStdHandle(STD_INPUT_HANDLE), &r, 1, &dwCount)) {
        if (gnRunMode == RM_SERVER) {
            int nCount = srv.nProcessCount;
            if (nCount > 1) {
                // ! Процесс таки запустился, закрываться не будем. Вернуть событие в буфер!
                WriteConsoleInput(GetStdHandle(STD_INPUT_HANDLE), &r, 1, &dwCount);
                break;
            }
        }
    
        if (gbInShutdown ||
                (r.EventType == KEY_EVENT && r.Event.KeyEvent.bKeyDown 
                 && r.Event.KeyEvent.wVirtualKeyCode == vkKey))
            break;
    }
    //MessageBox(0,L"Debug message...............1",L"ConEmuC",0);
    //int nCh = _getch();
    if (abNewLine)
        wprintf(L"\n");
}


#ifndef CONEMUC_DLL_MODE

int ComspecInit()
{
    TODO("Определить код родительского процесса, и если это FAR - запомнить его (для подключения к пайпу плагина)");
    TODO("Размер получить из GUI, если оно есть, иначе - по умолчанию");
    TODO("GUI может скорректировать размер с учетом полосы прокрутки");

    // Размер должен менять сам GUI, через серверный ConEmuC!
    #ifdef SHOW_STARTED_MSGBOX
    MessageBox(GetConsoleWindow(), L"ConEmuC (comspec mode) is about to START", L"ConEmuC.ComSpec", 0);
    #endif


    //int nNewBufferHeight = 0;
    //COORD crNewSize = {0,0};
    //SMALL_RECT rNewWindow = cmd.sbi.srWindow;
    BOOL lbSbiRc = FALSE;
    
	gbRootWasFoundInCon = 2; // не добавлять к "Press Enter to close console" - "or wait"

    // Это наверное и не нужно, просто для информации...
    lbSbiRc = MyGetConsoleScreenBufferInfo(ghConOut, &cmd.sbi);
    
    
    // Сюда мы попадаем если был ключик -new_console
    // А этом случае нужно завершить ЭТОТ экземпляр и запустить в ConEmu новую вкладку
    if (cmd.bNewConsole) {
        PROCESS_INFORMATION pi; memset(&pi, 0, sizeof(pi));
        STARTUPINFOW si; memset(&si, 0, sizeof(si)); si.cb = sizeof(si);
        si.dwFlags = STARTF_USESHOWWINDOW|STARTF_USECOUNTCHARS;
        si.dwXCountChars = cmd.sbi.dwSize.X;
        si.dwYCountChars = cmd.sbi.dwSize.Y;
        si.wShowWindow = SW_HIDE;
        
        PRINT_COMSPEC(L"Creating new console for:\n%s\n", gpszRunCmd);
    
        // CREATE_NEW_PROCESS_GROUP - низя, перестает работать Ctrl-C
        BOOL lbRc = CreateProcessW(NULL, gpszRunCmd, NULL,NULL, TRUE, 
                NORMAL_PRIORITY_CLASS|CREATE_NEW_CONSOLE, 
                NULL, NULL, &si, &pi);
        DWORD dwErr = GetLastError();
        if (!lbRc)
        {
            wprintf (L"Can't create process, ErrCode=0x%08X! Command to be executed:\n%s\n", dwErr, gpszRunCmd);
            return CERR_CREATEPROCESS;
        }
        //delete psNewCmd; psNewCmd = NULL;
        AllowSetForegroundWindow(pi.dwProcessId);
        PRINT_COMSPEC(L"New console created. PID=%i. Exiting...\n", pi.dwProcessId);
        SafeCloseHandle(pi.hProcess); SafeCloseHandle(pi.hThread);
        gbAlwaysConfirmExit = FALSE;
		srv.nProcessStartTick = GetTickCount() - 2*CHECK_ROOTSTART_TIMEOUT;
        return CERR_RUNNEWCONSOLE;
    }
    
    
    // Если определена ComSpecC - значит ConEmuC переопределил стандартный ComSpec
    // Вернем его
    wchar_t szComSpec[MAX_PATH+1];
    if (GetEnvironmentVariable(L"ComSpecC", szComSpec, MAX_PATH) && szComSpec[0] != 0)
    {
        // Только если это (случайно) не conemuc.exe
        wchar_t* pwszCopy = wcsrchr(szComSpec, L'\\'); 
        if (!pwszCopy) pwszCopy = szComSpec;
        #pragma warning( push )
        #pragma warning(disable : 6400)
        if (lstrcmpiW(pwszCopy, L"ConEmuC")==0 || lstrcmpiW(pwszCopy, L"ConEmuC.exe")==0)
            szComSpec[0] = 0;
        #pragma warning( pop )
        if (szComSpec[0]) {
	        SetEnvironmentVariable(L"ComSpec", szComSpec);
	        SetEnvironmentVariable(L"ComSpecC", NULL);
        }
    }
    
    SendStarted();
    
    return 0;
}

void SendStarted()
{    
	static bool bSent = false;
	if (bSent)
		return; // отсылать только один раз
	
    //crNewSize = cmd.sbi.dwSize;
    //_ASSERTE(crNewSize.X>=MIN_CON_WIDTH && crNewSize.Y>=MIN_CON_HEIGHT);
    
    CESERVER_REQ *pIn = NULL, *pOut = NULL;
    int nSize = sizeof(CESERVER_REQ_HDR)+sizeof(CESERVER_REQ_STARTSTOP);
	pIn = ExecuteNewCmd(CECMD_CMDSTARTSTOP, nSize);
    if (pIn) {
		pIn->StartStop.nStarted = (gnRunMode == RM_COMSPEC) ? 2 : 0; // Cmd/Srv режим начат
		pIn->StartStop.hWnd = ghConWnd;
		pIn->StartStop.dwPID = gnSelfPID;
		pIn->StartStop.dwInputTID = (gnRunMode == RM_SERVER) ? srv.dwInputThreadId : 0;

		// Перед запуском 16бит приложений нужно подресайзить консоль...
		gnImageSubsystem = 0;
        LPCWSTR pszTemp = gpszRunCmd;
        wchar_t lsRoot[MAX_PATH+1] = {0};
		if (!gpszRunCmd) {
			// Аттач из фар-плагина
			gnImageSubsystem = 0x100;
		} else
        if (0 == NextArg(&pszTemp, lsRoot)) {
        	PRINT_COMSPEC(L"Starting: <%s>", lsRoot);
        	if (!GetImageSubsystem(lsRoot, gnImageSubsystem))
				gnImageSubsystem = 0;
       		PRINT_COMSPEC(L", Subsystem: <%i>\n", gnImageSubsystem);
        }
		pIn->StartStop.nSubSystem = gnImageSubsystem;
		pIn->StartStop.bRootIsCmdExe = gbRootIsCmdExe; //2009-09-14
		// НЕ MyGet..., а то можем заблокироваться...
		GetConsoleScreenBufferInfo(ghConOut, &pIn->StartStop.sbi);

        PRINT_COMSPEC(L"Starting comspec mode (ExecuteGuiCmd started)\n",0);
        pOut = ExecuteGuiCmd(ghConWnd, pIn, ghConWnd);
        PRINT_COMSPEC(L"Starting comspec mode (ExecuteGuiCmd finished)\n",0);
        if (pOut) {
			bSent = true;

			BOOL  bAlreadyBufferHeight = pOut->StartStopRet.bWasBufferHeight;

			DWORD nGuiPID = pOut->StartStopRet.dwPID;
			ghConEmuWnd = pOut->StartStopRet.hWnd;

            AllowSetForegroundWindow(nGuiPID);
            
            gnBufferHeight  = (SHORT)pOut->StartStopRet.nBufferHeight;
            gcrBufferSize.X = (SHORT)pOut->StartStopRet.nWidth;
            gcrBufferSize.Y = (SHORT)pOut->StartStopRet.nHeight;
			gbParmBufferSize = TRUE;

            if (gnRunMode == RM_SERVER) {
            	SMALL_RECT rcNil = {0};
            	SetConsoleSize(gnBufferHeight, gcrBufferSize, rcNil, "::SendStarted");
            } else
            // Может так получиться, что один COMSPEC запущен из другого.
            if (bAlreadyBufferHeight)
                cmd.bNonGuiMode = TRUE; // Не посылать ExecuteGuiCmd при выходе - прокрутка должна остаться

            //nNewBufferHeight = ((DWORD*)(pOut->Data))[0];
            //crNewSize.X = (SHORT)((DWORD*)(pOut->Data))[1];
            //crNewSize.Y = (SHORT)((DWORD*)(pOut->Data))[2];
            TODO("Если он запущен как COMSPEC - то к GUI никакого отношения иметь не должен");
            //if (rNewWindow.Right >= crNewSize.X) // размер был уменьшен за счет полосы прокрутки
            //    rNewWindow.Right = crNewSize.X-1;
            ExecuteFreeResult(pOut); pOut = NULL;

            //gnBufferHeight = nNewBufferHeight;
        } else {
            cmd.bNonGuiMode = TRUE; // Не посылать ExecuteGuiCmd при выходе. Это не наша консоль
        }
        ExecuteFreeResult(pIn); pIn = NULL;
    }
}

void ComspecDone(int aiRc)
{
    //WARNING("Послать в GUI CONEMUCMDSTOPPED");

    //TODO("Уведомить плагин через пайп (если родитель - FAR) что процесс завершен. Плагин должен считать и запомнить содержимое консоли и только потом вернуть управление в ConEmuC!");

    BOOL lbRc1 = FALSE, lbRc2 = FALSE;
    CONSOLE_SCREEN_BUFFER_INFO sbi1 = {{0,0}}, sbi2 = {{0,0}};
    // Тут нужна реальная, а не скорректированная информация!
    if (!cmd.bNonGuiMode) // Если GUI не сможет через сервер вернуть высоту буфера - это нужно сделать нам!
        lbRc1 = GetConsoleScreenBufferInfo(ghConOut, &sbi1);


    //PRAGMA_ERROR("Размер должен возвращать сам GUI, через серверный ConEmuC!");
    #ifdef SHOW_STARTED_MSGBOX
    MessageBox(GetConsoleWindow(), L"ConEmuC (comspec mode) is about to TERMINATE", L"ConEmuC.ComSpec", 0);
    #endif
    
    if (!cmd.bNonGuiMode)
    {
        //// Вернуть размер буфера (высота И ширина)
        //if (cmd.sbi.dwSize.X && cmd.sbi.dwSize.Y) {
        //    SMALL_RECT rc = {0};
        //    SetConsoleSize(0, cmd.sbi.dwSize, rc, "ComspecDone");
        //}

        CESERVER_REQ *pIn = NULL, *pOut = NULL;
        int nSize = sizeof(CESERVER_REQ_HDR)+sizeof(CESERVER_REQ_STARTSTOP);
        pIn = ExecuteNewCmd(CECMD_CMDSTARTSTOP,nSize);
        if (pIn) {
			pIn->StartStop.nStarted = 3; // Cmd режим завершен
			pIn->StartStop.hWnd = ghConWnd;
			pIn->StartStop.dwPID = gnSelfPID;
			pIn->StartStop.nSubSystem = gnImageSubsystem;
			// НЕ MyGet..., а то можем заблокироваться...
			// ghConOut может быть NULL, если ошибка произошла во время разбора аргументов
			GetConsoleScreenBufferInfo(ghConOut ? ghConOut : GetStdHandle(STD_OUTPUT_HANDLE),
				&pIn->StartStop.sbi);

            PRINT_COMSPEC(L"Finalizing comspec mode (ExecuteGuiCmd started)\n",0);
            pOut = ExecuteGuiCmd(ghConWnd, pIn, ghConWnd);
            PRINT_COMSPEC(L"Finalizing comspec mode (ExecuteGuiCmd finished)\n",0);
            if (pOut) {
				if (!pOut->StartStopRet.bWasBufferHeight) {
					//cmd.sbi.dwSize = pIn->StartStop.sbi.dwSize;
					lbRc1 = FALSE; // Консольное приложение самостоятельно сбросило буферный режим. Не дергаться...
				}
                ExecuteFreeResult(pOut); pOut = NULL;
            }
			ExecuteFreeResult(pIn); pIn = NULL; // не освобождалось
        }

        lbRc2 = GetConsoleScreenBufferInfo(ghConOut, &sbi2);
        if (lbRc1 && lbRc2 && sbi2.dwSize.Y == sbi1.dwSize.Y) {
            // GUI не смог вернуть высоту буфера... 
            // Это плохо, т.к. фар высоту буфера не меняет и будет сильно глючить на N сотнях строк...
            if (sbi2.dwSize.Y != cmd.sbi.dwSize.Y) {
            	PRINT_COMSPEC(L"Error: BufferHeight was not changed from %i\n", sbi2.dwSize.Y);
                SMALL_RECT rc = {0};
                sbi2.dwSize.Y = cmd.sbi.dwSize.Y;
                SetConsoleSize(0, sbi2.dwSize, rc, "ComspecDone.Force");
            }
        }
    }

    //SafeCloseHandle(ghCtrlCEvent);
    //SafeCloseHandle(ghCtrlBreakEvent);
}

WARNING("Добавить LogInput(INPUT_RECORD* pRec) но имя файла сделать 'ConEmuC-input-%i.log'");
void CreateLogSizeFile(int nLevel)
{
    if (ghLogSize) return; // уже
    
    DWORD dwErr = 0;
    wchar_t szFile[MAX_PATH+64], *pszDot;
    if (!GetModuleFileName(NULL, szFile, MAX_PATH)) {
        dwErr = GetLastError();
        wprintf(L"GetModuleFileName failed! ErrCode=0x%08X\n", dwErr);
        return; // не удалось
    }
    if ((pszDot = wcsrchr(szFile, L'.')) == NULL) {
        wprintf(L"wcsrchr failed!\n%s\n", szFile);
        return; // ошибка
    }
    wsprintfW(pszDot, L"-size-%i.log", gnSelfPID);
    
    ghLogSize = CreateFileW ( szFile, GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (ghLogSize == INVALID_HANDLE_VALUE) {
        ghLogSize = NULL;
        dwErr = GetLastError();
        wprintf(L"CreateFile failed! ErrCode=0x%08X\n%s\n", dwErr, szFile);
        return;
    }
    
    wpszLogSizeFile = _wcsdup(szFile);
    // OK, лог создали
    LPCSTR pszCmdLine = GetCommandLineA();
    if (pszCmdLine) {
        WriteFile(ghLogSize, pszCmdLine, (DWORD)strlen(pszCmdLine), &dwErr, 0);
        WriteFile(ghLogSize, "\r\n", 2, &dwErr, 0);
    }
    LogSize(NULL, "Startup");
}

void LogSize(COORD* pcrSize, LPCSTR pszLabel)
{
    if (!ghLogSize) return;
    
    CONSOLE_SCREEN_BUFFER_INFO lsbi = {{0,0}};
    // В дебажный лог помещаем реальный значения
    GetConsoleScreenBufferInfo(ghConOut ? ghConOut : GetStdHandle(STD_OUTPUT_HANDLE), &lsbi);
    
    char szInfo[192] = {0};
    LPCSTR pszThread = "<unknown thread>";
    
    DWORD dwId = GetCurrentThreadId();
    if (dwId == gdwMainThreadId)
            pszThread = "MainThread";
            else
    if (dwId == srv.dwServerThreadId)
            pszThread = "ServerThread";
            else
    if (dwId == srv.dwRefreshThread)
            pszThread = "RefreshThread";
            else
    if (dwId == srv.dwWinEventThread)
            pszThread = "WinEventThread";
            else
    if (dwId == srv.dwInputThreadId)
            pszThread = "InputThread";
            else
    if (dwId == srv.dwInputPipeThreadId)
    		pszThread = "InputPipeThread";
            
    /*HDESK hDesk = GetThreadDesktop ( GetCurrentThreadId() );
    HDESK hInp = OpenInputDesktop ( 0, FALSE, GENERIC_READ );*/
    
            
    SYSTEMTIME st; GetLocalTime(&st);
    if (pcrSize) {
        sprintf(szInfo, "%i:%02i:%02i CurSize={%ix%i} ChangeTo={%ix%i} %s %s\r\n",
            st.wHour, st.wMinute, st.wSecond,
            lsbi.dwSize.X, lsbi.dwSize.Y, pcrSize->X, pcrSize->Y, pszThread, (pszLabel ? pszLabel : ""));
    } else {
        sprintf(szInfo, "%i:%02i:%02i CurSize={%ix%i} %s %s\r\n",
            st.wHour, st.wMinute, st.wSecond,
            lsbi.dwSize.X, lsbi.dwSize.Y, pszThread, (pszLabel ? pszLabel : ""));
    }
    
    //if (hInp) CloseDesktop ( hInp );
    
    DWORD dwLen = 0;
    WriteFile(ghLogSize, szInfo, (DWORD)strlen(szInfo), &dwLen, 0);
    FlushFileBuffers(ghLogSize);
}

HWND Attach2Gui(DWORD nTimeout)
{
    HWND hGui = NULL, hDcWnd = NULL;
    UINT nMsg = RegisterWindowMessage(CONEMUMSG_ATTACH);
    BOOL bNeedStartGui = FALSE;
    DWORD dwErr = 0;
    
    hGui = FindWindowEx(NULL, hGui, VirtualConsoleClassMain, NULL);
    if (!hGui)
    {
    	DWORD dwGuiPID = 0;
		HANDLE hSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS,0);
		if (hSnap != INVALID_HANDLE_VALUE) {
			PROCESSENTRY32 prc = {sizeof(PROCESSENTRY32)};
			if (Process32First(hSnap, &prc)) {
				do {
            		for (UINT i = 0; i < srv.nProcessCount; i++) {
    					if (lstrcmpiW(prc.szExeFile, L"conemu.exe")==0) {
    						dwGuiPID = prc.th32ProcessID;
    						break;
    					}
            		}
					if (dwGuiPID) break;
				} while (Process32Next(hSnap, &prc));
			}
			CloseHandle(hSnap);
			
			if (!dwGuiPID) bNeedStartGui = TRUE;
		}
	}
    
    
    if (bNeedStartGui) {
		wchar_t szSelf[MAX_PATH+100];
		wchar_t* pszSelf = szSelf+1, *pszSlash = NULL;
		if (!GetModuleFileName(NULL, pszSelf, MAX_PATH)) {
			dwErr = GetLastError();
			wprintf (L"GetModuleFileName failed, ErrCode=0x%08X\n", dwErr);
			return NULL;
		}
		pszSlash = wcsrchr(pszSelf, L'\\');
		if (!pszSlash) {
			wprintf (L"Invalid GetModuleFileName, backslash not found!\n%s\n", pszSelf);
			return NULL;
		}
		pszSlash++;
		if (wcschr(pszSelf, L' ')) {
			*(--pszSelf) = L'"';
			lstrcpyW(pszSlash, L"ConEmu.exe\"");
		} else {
			lstrcpyW(pszSlash, L"ConEmu.exe");
		}
		
		lstrcatW(pszSelf, L" /detached");
		
		PROCESS_INFORMATION pi; memset(&pi, 0, sizeof(pi));
		STARTUPINFOW si; memset(&si, 0, sizeof(si)); si.cb = sizeof(si);
        
		PRINT_COMSPEC(L"Starting GUI:\n%s\n", pszSelf);
    
		// CREATE_NEW_PROCESS_GROUP - низя, перестает работать Ctrl-C
		BOOL lbRc = CreateProcessW(NULL, pszSelf, NULL,NULL, TRUE, 
				NORMAL_PRIORITY_CLASS, NULL, NULL, &si, &pi);
		dwErr = GetLastError();
		if (!lbRc)
		{
			wprintf (L"Can't create process, ErrCode=0x%08X! Command to be executed:\n%s\n", dwErr, pszSelf);
			return NULL;
		}
		//delete psNewCmd; psNewCmd = NULL;
		AllowSetForegroundWindow(pi.dwProcessId);
		PRINT_COMSPEC(L"Detached GUI was started. PID=%i, Attaching...\n", pi.dwProcessId);
		WaitForInputIdle(pi.hProcess, nTimeout);
		SafeCloseHandle(pi.hProcess); SafeCloseHandle(pi.hThread);
		
		if (nTimeout > 1000) nTimeout = 1000;
    }
    
    
    DWORD dwStart = GetTickCount(), dwDelta = 0, dwCur = 0;
    BOOL lbNeedSetFont = TRUE;
    // Нужно сбросить. Могли уже искать...
    hGui = NULL;
    // Если с первого раза не получится (GUI мог еще не загрузиться) пробуем еще
    while (!hDcWnd && dwDelta <= nTimeout) {
        while ((hGui = FindWindowEx(NULL, hGui, VirtualConsoleClassMain, NULL)) != NULL) {
        	if (lbNeedSetFont) {
        		lbNeedSetFont = FALSE;
        		
                if (ghLogSize) LogSize(NULL, ":SetConsoleFontSizeTo.before");
                SetConsoleFontSizeTo(ghConWnd, srv.nConFontHeight, srv.nConFontWidth, srv.szConsoleFont);
                if (ghLogSize) LogSize(NULL, ":SetConsoleFontSizeTo.after");
        	}
        
            hDcWnd = (HWND)SendMessage(hGui, nMsg, (WPARAM)ghConWnd, (LPARAM)gnSelfPID);
            if (hDcWnd != NULL) {
                ghConEmuWnd = hGui;

				// В принципе, консоль может действительно запуститься видимой. В этом случае ее скрывать не нужно
				// Но скорее всего, консоль запущенная под Админом в Win7 будет отображена ошибочно
				if (gbForceHideConWnd)
					ShowWindow(ghConWnd, SW_HIDE);
                
                // Установить переменную среды с дескриптором окна
                SetConEmuEnvVar(ghConEmuWnd);
				CheckConEmuHwnd();
                
                break;
            }
        }
        if (hDcWnd) break;

        dwCur = GetTickCount(); dwDelta = dwCur - dwStart;
        if (dwDelta > nTimeout) break;
        
        Sleep(500);
        dwCur = GetTickCount(); dwDelta = dwCur - dwStart;
    }
    
    return hDcWnd;
}

// Создать необходимые события и нити
int ServerInit()
{
    int iRc = 0;
    DWORD dwErr = 0;
    wchar_t szComSpec[MAX_PATH+1], szSelf[MAX_PATH+3];
    wchar_t* pszSelf = szSelf+1;
    //HMODULE hKernel = GetModuleHandleW (L"kernel32.dll");

	//2009-08-27 Перенес снизу
	if (!srv.hConEmuGuiAttached) {
		wchar_t szTempName[MAX_PATH];
		wsprintfW(szTempName, CEGUIRCONSTARTED, (DWORD)ghConWnd);
		//srv.hConEmuGuiAttached = OpenEvent(EVENT_ALL_ACCESS, FALSE, szTempName);
		//if (srv.hConEmuGuiAttached == NULL)
   		srv.hConEmuGuiAttached = CreateEvent(gpNullSecurity, TRUE, FALSE, szTempName);
		_ASSERTE(srv.hConEmuGuiAttached!=NULL);
		//if (srv.hConEmuGuiAttached) ResetEvent(srv.hConEmuGuiAttached); -- низя. может уже быть создано/установлено в GUI
	}
    
    srv.nMaxFPS = 10;
    
    //if (hKernel) {
    //    pfnGetConsoleKeyboardLayoutName = (FGetConsoleKeyboardLayoutName)GetProcAddress (hKernel, "GetConsoleKeyboardLayoutNameW");
    //    pfnGetConsoleProcessList = (FGetConsoleProcessList)GetProcAddress (hKernel, "GetConsoleProcessList");
    //}
    
	srv.csProc = new MSection();

    // Инициализация имен пайпов
    wsprintfW(srv.szPipename, CESERVERPIPENAME, L".", gnSelfPID);
    wsprintfW(srv.szInputname, CESERVERINPUTNAME, L".", gnSelfPID);

	srv.nMaxProcesses = START_MAX_PROCESSES; srv.nProcessCount = 0;
	srv.pnProcesses = (DWORD*)Alloc(START_MAX_PROCESSES, sizeof(DWORD));
	srv.pnProcessesCopy = (DWORD*)Alloc(START_MAX_PROCESSES, sizeof(DWORD));
	MCHKHEAP
	if (srv.pnProcesses == NULL || srv.pnProcessesCopy == NULL) {
		wprintf (L"Can't allocate %i DWORDS!\n", srv.nMaxProcesses);
		iRc = CERR_NOTENOUGHMEM1; goto wrap;
	}
	CheckProcessCount(TRUE); // Сначала добавит себя
	// в принципе, серверный режим может быть вызван из фара, чтобы подцепиться к GUI 
	// но больше двух процессов в консоли не ожидается!
	_ASSERTE(srv.nProcessCount<=2); 


	// Запустить нить обработки событий (клавиатура, мышь, и пр.)
	srv.hInputThread = CreateThread( 
		NULL,              // no security attribute 
		0,                 // default stack size 
		InputThread,       // thread proc
		NULL,              // thread parameter 
		0,                 // not suspended 
		&srv.dwInputThreadId);      // returns thread ID 

	if (srv.hInputThread == NULL) 
	{
		dwErr = GetLastError();
		wprintf(L"CreateThread(InputThread) failed, ErrCode=0x%08X\n", dwErr); 
		iRc = CERR_CREATEINPUTTHREAD; goto wrap;
	}
	//SetThreadPriority(srv.hInputThread, THREAD_PRIORITY_ABOVE_NORMAL);
	// Запустить нить обработки событий (клавиатура, мышь, и пр.)
	srv.hInputPipeThread = CreateThread( 
		NULL,              // no security attribute 
		0,                 // default stack size 
		InputPipeThread,   // thread proc
		NULL,              // thread parameter 
		0,                 // not suspended 
		&srv.dwInputPipeThreadId);      // returns thread ID 

	if (srv.hInputPipeThread == NULL) 
	{
		dwErr = GetLastError();
		wprintf(L"CreateThread(InputPipeThread) failed, ErrCode=0x%08X\n", dwErr); 
		iRc = CERR_CREATEINPUTTHREAD; goto wrap;
	}

    //InitializeCriticalSection(&srv.csChangeSize);
    InitializeCriticalSection(&srv.csConBuf);
    InitializeCriticalSection(&srv.csChar);

    if (!gbAttachMode) {
		HWND hConEmuWnd = FindConEmuByPID();
		if (hConEmuWnd) {
			UINT nMsgSrvStarted = RegisterWindowMessage(CONEMUMSG_SRVSTARTED);
			DWORD_PTR nRc = 0;
			SendMessageTimeout(hConEmuWnd, nMsgSrvStarted, (WPARAM)ghConWnd, gnSelfPID, 
				SMTO_BLOCK, 500, &nRc);
		}
        if (srv.hConEmuGuiAttached) {
            WaitForSingleObject(srv.hConEmuGuiAttached, 500);
		}
        CheckConEmuHwnd();
    }




    if (gbNoCreateProcess && gbAttachMode) {
    	if (!IsWindowVisible(ghConWnd)) {
			PRINT_COMSPEC(L"Console windows is not visible. Attach is unavailable. Exiting...\n", 0);
			gbAlwaysConfirmExit = FALSE;
			srv.nProcessStartTick = GetTickCount() - 2*CHECK_ROOTSTART_TIMEOUT;
			return CERR_RUNNEWCONSOLE;
    	}
    	
		if (srv.dwRootProcess == 0) {
    		// Нужно попытаться определить PID корневого процесса.
    		// Родительским может быть cmd (comspec, запущенный из FAR)
    		DWORD dwParentPID = 0, dwFarPID = 0;
    		DWORD dwServerPID = 0; // Вдруг в этой консоли уже есть сервер?
	    	
    		if (srv.nProcessCount >= 2) {
				HANDLE hSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS,0);
				if (hSnap != INVALID_HANDLE_VALUE) {
					PROCESSENTRY32 prc = {sizeof(PROCESSENTRY32)};
					if (Process32First(hSnap, &prc)) {
						do {
                    		for (UINT i = 0; i < srv.nProcessCount; i++) {
                    			if (prc.th32ProcessID != gnSelfPID
                    				&& prc.th32ProcessID == srv.pnProcesses[i])
                				{
                					if (lstrcmpiW(prc.szExeFile, L"conemuc.exe")==0) {
                						CESERVER_REQ* pIn = ExecuteNewCmd(CECMD_ATTACH2GUI, 0);
                						CESERVER_REQ* pOut = ExecuteSrvCmd(prc.th32ProcessID, pIn, ghConWnd);
                						if (pOut) dwServerPID = prc.th32ProcessID;
                						ExecuteFreeResult(pIn); ExecuteFreeResult(pOut);
                						// Если команда успешно выполнена - выходим
                						if (dwServerPID)
                							break;
                					}
                		    		if (!dwFarPID && lstrcmpiW(prc.szExeFile, L"far.exe")==0) {
                		    			dwFarPID = prc.th32ProcessID;
                		    		}
                		    		if (!dwParentPID)
                		    			dwParentPID = prc.th32ProcessID;
                    			}
                    		}
                    		// Если уже выполнили команду в сервере - выходим, перебор больше не нужен
    						if (dwServerPID)
    							break;
						} while (Process32Next(hSnap, &prc));
					}
					CloseHandle(hSnap);
					
					if (dwFarPID) dwParentPID = dwFarPID;
				}
			}
			
			if (dwServerPID) {
    			AllowSetForegroundWindow(dwServerPID);
    			PRINT_COMSPEC(L"Server was already started. PID=%i. Exiting...\n", dwServerPID);
    			gbAlwaysConfirmExit = FALSE;
				srv.nProcessStartTick = GetTickCount() - 2*CHECK_ROOTSTART_TIMEOUT;
    			return CERR_RUNNEWCONSOLE;
			}
	        
    		if (!dwParentPID) {
				wprintf (L"Attach to GUI was requested, but there is no console processes:\n%s\n", GetCommandLineW());
				return CERR_CARGUMENT;
    		}
	    	
    		// Нужно открыть HANDLE корневого процесса
    		srv.hRootProcess = OpenProcess(PROCESS_QUERY_INFORMATION|SYNCHRONIZE, FALSE, dwParentPID);
    		if (!srv.hRootProcess) {
    			dwErr = GetLastError();
    			wchar_t* lpMsgBuf = NULL;
	    		
   				FormatMessage( FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM, NULL, dwErr, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPWSTR)&lpMsgBuf, 0, NULL );
	    		
				wprintf (L"Can't open process (%i) handle, ErrCode=0x%08X, Description:\n%s\n", 
            		dwParentPID, dwErr, (lpMsgBuf == NULL) ? L"<Unknown error>" : lpMsgBuf);
	            
				if (lpMsgBuf) LocalFree(lpMsgBuf);
				return CERR_CREATEPROCESS;
    		}
    		srv.dwRootProcess = dwParentPID;

			// Запустить вторую копию ConEmuC НЕМОДАЛЬНО!
			wchar_t szSelf[MAX_PATH+100];
			wchar_t* pszSelf = szSelf+1;
			if (!GetModuleFileName(NULL, pszSelf, MAX_PATH)) {
				dwErr = GetLastError();
				wprintf (L"GetModuleFileName failed, ErrCode=0x%08X\n", dwErr);
				return CERR_CREATEPROCESS;
			}
			if (wcschr(pszSelf, L' ')) {
				*(--pszSelf) = L'"';
				lstrcatW(pszSelf, L"\"");
			}
			
			wsprintf(pszSelf+wcslen(pszSelf), L" /ATTACH /PID=%i", dwParentPID);

			PROCESS_INFORMATION pi; memset(&pi, 0, sizeof(pi));
			STARTUPINFOW si; memset(&si, 0, sizeof(si)); si.cb = sizeof(si);
	        
			PRINT_COMSPEC(L"Starting modeless:\n%s\n", pszSelf);
	    
			// CREATE_NEW_PROCESS_GROUP - низя, перестает работать Ctrl-C
			BOOL lbRc = CreateProcessW(NULL, pszSelf, NULL,NULL, TRUE, 
					NORMAL_PRIORITY_CLASS, NULL, NULL, &si, &pi);
			dwErr = GetLastError();
			if (!lbRc)
			{
				wprintf (L"Can't create process, ErrCode=0x%08X! Command to be executed:\n%s\n", dwErr, pszSelf);
				return CERR_CREATEPROCESS;
			}
			//delete psNewCmd; psNewCmd = NULL;
			AllowSetForegroundWindow(pi.dwProcessId);
			PRINT_COMSPEC(L"Modeless server was started. PID=%i. Exiting...\n", pi.dwProcessId);
			SafeCloseHandle(pi.hProcess); SafeCloseHandle(pi.hThread);
			gbAlwaysConfirmExit = FALSE;
			srv.nProcessStartTick = GetTickCount() - 2*CHECK_ROOTSTART_TIMEOUT;
			return CERR_RUNNEWCONSOLE;

		} else {
    		// Нужно открыть HANDLE корневого процесса
    		srv.hRootProcess = OpenProcess(PROCESS_QUERY_INFORMATION|SYNCHRONIZE, FALSE, srv.dwRootProcess);
    		if (!srv.hRootProcess) {
    			dwErr = GetLastError();
    			wchar_t* lpMsgBuf = NULL;
	    		
   				FormatMessage( FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM, NULL, dwErr, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPWSTR)&lpMsgBuf, 0, NULL );
	    		
				wprintf (L"Can't open process (%i) handle, ErrCode=0x%08X, Description:\n%s\n", 
            		srv.dwRootProcess, dwErr, (lpMsgBuf == NULL) ? L"<Unknown error>" : lpMsgBuf);
	            
				if (lpMsgBuf) LocalFree(lpMsgBuf);
				return CERR_CREATEPROCESS;
    		}
		}
    }


	srv.hAllowInputEvent = CreateEvent(NULL, TRUE, TRUE, NULL);
	if (!srv.hAllowInputEvent) SetEvent(srv.hAllowInputEvent);

    TODO("Сразу проверить, может ComSpecC уже есть?");
    if (GetEnvironmentVariable(L"ComSpec", szComSpec, MAX_PATH)) {
        wchar_t* pszSlash = wcsrchr(szComSpec, L'\\');
        if (pszSlash) {
            if (_wcsnicmp(pszSlash, L"\\conemuc.", 9)) {
                // Если это НЕ мы - сохранить в ComSpecC
                SetEnvironmentVariable(L"ComSpecC", szComSpec);
            }
        }
    }
    if (GetModuleFileName(NULL, pszSelf, MAX_PATH)) {
		wchar_t *pszShort = NULL;
		if (pszSelf[0] != L'\\')
			pszShort = GetShortFileNameEx(pszSelf);
	    if (!pszShort && wcschr(pszSelf, L' ')) {
		    *(--pszSelf) = L'"';
		    lstrcatW(pszSelf, L"\"");
	    }
		if (pszShort) {
			SetEnvironmentVariable(L"ComSpec", pszShort);
			free(pszShort);
		} else {
			SetEnvironmentVariable(L"ComSpec", pszSelf);
		}
    }

    //srv.bContentsChanged = TRUE;
    srv.nMainTimerElapse = 10;
    srv.bConsoleActive = TRUE; TODO("Обрабатывать консольные события Activate/Deactivate");
    srv.bNeedFullReload = FALSE; srv.bForceFullSend = TRUE;
    srv.nTopVisibleLine = -1; // блокировка прокрутки не включена

    


    // Размер шрифта и Lucida. Обязательно для серверного режима.
    if (srv.szConsoleFont[0]) {
        // Требуется проверить наличие такого шрифта!
        LOGFONT fnt = {0};
        lstrcpynW(fnt.lfFaceName, srv.szConsoleFont, LF_FACESIZE);
        srv.szConsoleFont[0] = 0; // сразу сбросим. Если шрифт есть - имя будет скопировано в FontEnumProc
        HDC hdc = GetDC(NULL);
        EnumFontFamiliesEx(hdc, &fnt, (FONTENUMPROCW) FontEnumProc, (LPARAM)&fnt, 0);
        DeleteDC(hdc);
    }
    if (srv.szConsoleFont[0] == 0) {
        lstrcpyW(srv.szConsoleFont, L"Lucida Console");
        srv.nConFontWidth = 4; srv.nConFontHeight = 6;
    }
    if (srv.nConFontHeight<6) srv.nConFontHeight = 6;
    if (srv.nConFontWidth==0 && srv.nConFontHeight==0) {
        srv.nConFontWidth = 4; srv.nConFontHeight = 6;
    } else if (srv.nConFontWidth==0) {
        srv.nConFontWidth = srv.nConFontHeight * 2 / 3;
    } else if (srv.nConFontHeight==0) {
        srv.nConFontHeight = srv.nConFontWidth * 3 / 2;
    }
    if (srv.nConFontHeight<6 || srv.nConFontWidth <4) {
        srv.nConFontWidth = 4; srv.nConFontHeight = 6;
    }
    //if (srv.szConsoleFontFile[0])
    //  AddFontResourceEx(srv.szConsoleFontFile, FR_PRIVATE, NULL);
    if (ghLogSize) LogSize(NULL, ":SetConsoleFontSizeTo.before");
    SetConsoleFontSizeTo(ghConWnd, srv.nConFontHeight, srv.nConFontWidth, srv.szConsoleFont);
    if (ghLogSize) LogSize(NULL, ":SetConsoleFontSizeTo.after");
    if (gbParmBufferSize && gcrBufferSize.X && gcrBufferSize.Y) {
        SMALL_RECT rc = {0};
        SetConsoleSize(gnBufferHeight, gcrBufferSize, rc, ":ServerInit.SetFromArg"); // может обломаться? если шрифт еще большой
    }

    if (IsIconic(ghConWnd)) { // окошко нужно развернуть!
        WINDOWPLACEMENT wplCon = {sizeof(wplCon)};
        GetWindowPlacement(ghConWnd, &wplCon);
        wplCon.showCmd = SW_RESTORE;
        SetWindowPlacement(ghConWnd, &wplCon);
    }


    // Сразу получить текущее состояние консоли
    ReloadConsoleInfo();

    //
    srv.hRefreshEvent = CreateEvent(NULL,FALSE,FALSE,NULL);
    if (!srv.hRefreshEvent) {
        dwErr = GetLastError();
        wprintf(L"CreateEvent(hRefreshEvent) failed, ErrCode=0x%08X\n", dwErr); 
        iRc = CERR_REFRESHEVENT; goto wrap;
    }


    
    // Запустить нить наблюдения за консолью
    srv.hRefreshThread = CreateThread( 
        NULL,              // no security attribute 
        0,                 // default stack size 
        RefreshThread,     // thread proc
        NULL,              // thread parameter 
        0,                 // not suspended 
        &srv.dwRefreshThread); // returns thread ID 

    if (srv.hRefreshThread == NULL) 
    {
        dwErr = GetLastError();
        wprintf(L"CreateThread(RefreshThread) failed, ErrCode=0x%08X\n", dwErr); 
        iRc = CERR_CREATEREFRESHTHREAD; goto wrap;
    }
    
    
    srv.nMsgHookEnableDisable = RegisterWindowMessage(L"ConEmuC::HookEnableDisable");
    // The client thread that calls SetWinEventHook must have a message loop in order to receive events.");
    srv.hWinEventThread = CreateThread( NULL, 0, WinEventThread, NULL, 0, &srv.dwWinEventThread);
    if (srv.hWinEventThread == NULL) 
    {
        dwErr = GetLastError();
        wprintf(L"CreateThread(WinEventThread) failed, ErrCode=0x%08X\n", dwErr); 
        iRc = CERR_WINEVENTTHREAD; goto wrap;
    }

    // Запустить нить обработки команд  
    srv.hServerThread = CreateThread( NULL, 0, ServerThread, NULL, 0, &srv.dwServerThreadId);
    if (srv.hServerThread == NULL) 
    {
        dwErr = GetLastError();
        wprintf(L"CreateThread(ServerThread) failed, ErrCode=0x%08X\n", dwErr); 
        iRc = CERR_CREATESERVERTHREAD; goto wrap;
    }

    if (gbAttachMode) {
        HWND hDcWnd = Attach2Gui(5000);
        
		// 090719 попробуем в сервере это делать всегда. Нужно передать в GUI - TID нити ввода
        //// Если это НЕ новая консоль (-new_console) и не /ATTACH уже существующей консоли
        //if (!gbNoCreateProcess)
        //	SendStarted();

        if (!hDcWnd) {
            wprintf(L"Available ConEmu GUI window not found!\n");
            iRc = CERR_ATTACHFAILED; goto wrap;
        }
    }

	SendStarted();

    CheckConEmuHwnd();


wrap:
    return iRc;
}

// Завершить все нити и закрыть дескрипторы
void ServerDone(int aiRc)
{
	// На всякий случай - выставим событие
	if (ghExitEvent) SetEvent(ghExitEvent);

	// Пошлем события сразу во все нити, а потом будем ждать
	if (srv.dwWinEventThread && srv.hWinEventThread)
		PostThreadMessage(srv.dwWinEventThread, WM_QUIT, 0, 0);
	if (srv.dwInputThreadId && srv.hInputThread)
		PostThreadMessage(srv.dwInputThreadId, WM_QUIT, 0, 0);
	// Передернуть пайп серверной нити
	HANDLE hPipe = CreateFile(srv.szPipename,GENERIC_WRITE,0,NULL,OPEN_EXISTING,0,NULL);
	if (hPipe == INVALID_HANDLE_VALUE) {
		DEBUGSTR(L"All pipe instances closed?\n");
	}
	// Передернуть нить ввода
	HANDLE hInputPipe = CreateFile(srv.szInputname,GENERIC_WRITE,0,NULL,OPEN_EXISTING,0,NULL);
	if (hInputPipe == INVALID_HANDLE_VALUE) {
		DEBUGSTR(L"Input pipe was not created?\n");
	} else {
		MSG msg = {NULL}; msg.message = 0xFFFF; DWORD dwOut = 0;
		WriteFile(hInputPipe, &msg, sizeof(msg), &dwOut, 0);
	}


    // Закрываем дескрипторы и выходим
    if (srv.dwWinEventThread && srv.hWinEventThread) {
        // Подождем немножко, пока нить сама завершится
        if (WaitForSingleObject(srv.hWinEventThread, 500) != WAIT_OBJECT_0) {
            #pragma warning( push )
            #pragma warning( disable : 6258 )
            TerminateThread ( srv.hWinEventThread, 100 ); // раз корректно не хочет...
            #pragma warning( pop )
        }
        SafeCloseHandle(srv.hWinEventThread);
        srv.dwWinEventThread = 0;
    }
    if (srv.hInputThread) {
		// Подождем немножко, пока нить сама завершится
		if (WaitForSingleObject(srv.hInputThread, 500) != WAIT_OBJECT_0) {
			#pragma warning( push )
			#pragma warning( disable : 6258 )
			TerminateThread ( srv.hInputThread, 100 ); // раз корректно не хочет...
			#pragma warning( pop )
		}
		SafeCloseHandle(srv.hInputThread);
		srv.dwInputThreadId = 0;
    }
    if (srv.hInputPipeThread) {
		// Подождем немножко, пока нить сама завершится
		if (WaitForSingleObject(srv.hInputPipeThread, 500) != WAIT_OBJECT_0) {
			#pragma warning( push )
			#pragma warning( disable : 6258 )
			TerminateThread ( srv.hInputPipeThread, 100 ); // раз корректно не хочет...
			#pragma warning( pop )
		}
		SafeCloseHandle(srv.hInputPipeThread);
		srv.dwInputPipeThreadId = 0;
    }
    SafeCloseHandle(hInputPipe);

    if (srv.hServerThread) {
		// Подождем немножко, пока нить сама завершится
		if (WaitForSingleObject(srv.hServerThread, 500) != WAIT_OBJECT_0) {
			#pragma warning( push )
			#pragma warning( disable : 6258 )
			TerminateThread ( srv.hServerThread, 100 ); // раз корректно не хочет...
			#pragma warning( pop )
		}
        SafeCloseHandle(srv.hServerThread);
    }
    SafeCloseHandle(hPipe);
    if (srv.hRefreshThread) {
        if (WaitForSingleObject(srv.hRefreshThread, 100)!=WAIT_OBJECT_0) {
            _ASSERT(FALSE);
            #pragma warning( push )
            #pragma warning( disable : 6258 )
            TerminateThread(srv.hRefreshThread, 100);
            #pragma warning( pop )
        }
        SafeCloseHandle(srv.hRefreshThread);
    }
    
    if (srv.hRefreshEvent) {
        SafeCloseHandle(srv.hRefreshEvent);
    }
    //if (srv.hChangingSize) {
    //    SafeCloseHandle(srv.hChangingSize);
    //}
    // Отключить все хуки
    srv.bWinHookAllow = FALSE; srv.nWinHookMode = 0;
    HookWinEvents ( -1 );
    
    if (gpStoredOutput) { Free(gpStoredOutput); gpStoredOutput = NULL; }
    if (srv.psChars) { Free(srv.psChars); srv.psChars = NULL; }
    if (srv.pnAttrs) { Free(srv.pnAttrs); srv.pnAttrs = NULL; }
    if (srv.ptrLineCmp) { Free(srv.ptrLineCmp); srv.ptrLineCmp = NULL; }
    DeleteCriticalSection(&srv.csConBuf);
    DeleteCriticalSection(&srv.csChar);
    //DeleteCriticalSection(&srv.csChangeSize);

	SafeCloseHandle(srv.hAllowInputEvent);

    SafeCloseHandle(srv.hRootProcess); 

	if (srv.csProc) {
		delete srv.csProc;
		srv.csProc = NULL;
	}

	if (srv.pnProcesses) {
		Free(srv.pnProcesses); srv.pnProcesses = NULL;
	}
	if (srv.pnProcessesCopy) {
		Free(srv.pnProcessesCopy); srv.pnProcessesCopy = NULL;
	}
}

BOOL CheckProcessCount(BOOL abForce/*=FALSE*/)
{
	static DWORD dwLastCheckTick = GetTickCount();

	UINT nPrevCount = srv.nProcessCount;
	if (srv.nProcessCount <= 0) {
		abForce = TRUE;
	}

	if (!abForce) {
		DWORD dwCurTick = GetTickCount();
		if ((dwCurTick - dwLastCheckTick) < (DWORD)CHECK_PROCESSES_TIMEOUT)
			return FALSE;
	}

	BOOL lbChanged = FALSE;
	MSectionLock CS; CS.Lock(srv.csProc);

	if (srv.nProcessCount == 0) {
		srv.pnProcesses[0] = gnSelfPID;
		srv.nProcessCount = 1;
	}

	if (!pfnGetConsoleProcessList) {

		if (srv.hRootProcess) {
			if (WaitForSingleObject(srv.hRootProcess, 0) == WAIT_OBJECT_0) {
				srv.pnProcesses[1] = 0;
				lbChanged = srv.nProcessCount != 1;
				srv.nProcessCount = 1;
			} else {
				srv.pnProcesses[1] = srv.dwRootProcess;
				lbChanged = srv.nProcessCount != 2;
				srv.nProcessCount = 2;
			}
		}

	} else {
		DWORD nCurCount = 0;

		nCurCount = pfnGetConsoleProcessList(srv.pnProcesses, srv.nMaxProcesses);
		lbChanged = srv.nProcessCount != nCurCount;

		if (nCurCount > srv.nMaxProcesses) {
			DWORD nSize = nCurCount + 100;
			DWORD* pnPID = (DWORD*)Alloc(nSize, sizeof(DWORD));
			if (pnPID) {
				
				CS.RelockExclusive(200);

				nCurCount = pfnGetConsoleProcessList(pnPID, nSize);
				if (nCurCount > 0 && nCurCount <= nSize) {
					Free(srv.pnProcesses);
					srv.pnProcesses = pnPID; pnPID = NULL;
					srv.nProcessCount = nCurCount;
					srv.nMaxProcesses = nSize;
				}

				if (pnPID)
					Free(pnPID);
			}
		} else {
			// Сбросить в 0 ячейки со старыми процессами
			_ASSERTE(srv.nProcessCount < srv.nMaxProcesses);
			if (nCurCount < srv.nProcessCount) {
				UINT nSize = sizeof(DWORD)*(srv.nProcessCount - nCurCount);
				memset(srv.pnProcesses + nCurCount, 0, nSize);
			}
			srv.nProcessCount = nCurCount;
		}

		if (!lbChanged) {
			UINT nSize = sizeof(DWORD)*min(srv.nMaxProcesses,START_MAX_PROCESSES);
			#ifdef _DEBUG
			_ASSERTE(!IsBadWritePtr(srv.pnProcessesCopy,nSize));
			_ASSERTE(!IsBadWritePtr(srv.pnProcesses,nSize));
			#endif
			lbChanged = memcmp(srv.pnProcessesCopy, srv.pnProcesses, nSize) != 0;
			MCHKHEAP
			if (lbChanged)
				memmove(srv.pnProcessesCopy, srv.pnProcesses, nSize);
			MCHKHEAP
		}
		
		if (lbChanged)
		{
			BOOL lbFarExists = FALSE, lbTelnetExist = FALSE;
			if (srv.nProcessCount > 1)
			{
				HANDLE hSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS,0);
				if (hSnap != INVALID_HANDLE_VALUE) {
					PROCESSENTRY32 prc = {sizeof(PROCESSENTRY32)};
					if (Process32First(hSnap, &prc))
					{
						do
						{
                    		for (UINT i = 0; i < srv.nProcessCount; i++) {
                    			if (prc.th32ProcessID != gnSelfPID
                    				&& prc.th32ProcessID == srv.pnProcesses[i])
                				{
                		    		if (lstrcmpiW(prc.szExeFile, L"far.exe")==0) {
                		    			lbFarExists = TRUE;
										if (srv.nProcessCount <= 2)
											break; // возможно, в консоли еще есть и telnet?
                		    		}
									// Во время работы Telnet тоже нужно ловить все события!
                		    		if (lstrcmpiW(prc.szExeFile, L"telnet.exe")==0) {
										// сразу хэндлы передернуть
										ghConIn.Close(); ghConOut.Close();
										srv.bWinHookAllow = TRUE; // Попробуем разрешить события для телнета
                		    			lbFarExists = TRUE; lbTelnetExist = TRUE; break;
                		    		}
                    			}
                    		}
						} while (!(lbFarExists && lbTelnetExist) && Process32Next(hSnap, &prc));
					}
					CloseHandle(hSnap);
				}
			}
			srv.bTelnetActive = lbTelnetExist;
			
			if (srv.nProcessCount >= 2
				&& ( (srv.hWinHook == NULL && srv.bWinHookAllow) || (srv.hWinHook != NULL) )
			    )
			{
				if (lbFarExists) srv.nWinHookMode = 2; else srv.nWinHookMode = 1;
				
    			if (lbFarExists && srv.hWinHook == NULL && srv.bWinHookAllow) {
    				HookWinEvents(2);
    			} else if (!lbFarExists && srv.hWinHook) {
    				HookWinEvents(0);
    			}
			}
		}
	}

	dwLastCheckTick = GetTickCount();

	// Если корень - фар, и он проработал достаточно (10 сек), значит он живой и gbAlwaysConfirmExit можно сбросить
	if (gbAlwaysConfirmExit && gbAutoDisableConfirmExit && nPrevCount > 1 && srv.hRootProcess) {
		if ((dwLastCheckTick - srv.nProcessStartTick) > CHECK_ROOTOK_TIMEOUT) {
			if (WaitForSingleObject(srv.hRootProcess, 0) == WAIT_OBJECT_0) {
				// Корневой процесс завершен, возможно, была какая-то проблема?
				gbAutoDisableConfirmExit = FALSE; gbAlwaysConfirmExit = TRUE;
			} else {
				// Корневой процесс все еще работает, считаем что все ок и подтверждения закрытия консоли не потребуется
				gbAutoDisableConfirmExit = FALSE; gbAlwaysConfirmExit = FALSE;
				srv.nProcessStartTick = GetTickCount() - 2*CHECK_ROOTSTART_TIMEOUT;
			}
		}
	}
	if (gbRootWasFoundInCon == 0 && srv.nProcessCount > 1 && srv.hRootProcess && srv.dwRootProcess) {
		if (WaitForSingleObject(srv.hRootProcess, 0) == WAIT_OBJECT_0) {
			gbRootWasFoundInCon = 2; // в консоль процесс не попал, и уже завершился
		} else {
			for (UINT n = 0; n < srv.nProcessCount; n++) {
				if (srv.dwRootProcess == srv.pnProcesses[n]) {
					// Процесс попал в консоль
					gbRootWasFoundInCon = 1; break;
				}
			}			
		}
	}

	// Процессов в консоли не осталось?
	#ifndef WIN64
	if (srv.nProcessCount == 2 && !srv.bNtvdmActive && srv.nNtvdmPID) {
		// Возможно было запущено 16битное приложение, а ntvdm.exe не выгружается при его закрытии
		// gnSelfPID не обязательно будет в srv.pnProcesses[0]
		if ((srv.pnProcesses[0] == gnSelfPID && srv.pnProcesses[1] == srv.nNtvdmPID)
			|| (srv.pnProcesses[1] == gnSelfPID && srv.pnProcesses[0] == srv.nNtvdmPID))
		{
			// Послать в нашу консоль команду закрытия
			PostMessage(ghConWnd, WM_CLOSE, 0, 0);
		}
	}
	#endif
	WARNING("Если в консоли ДО этого были процессы - все условия вида 'srv.nProcessCount == 1' обломаются");
	// Пример - запускаемся из фара. Количество процессов ИЗНАЧАЛЬНО - 5
	// cmd вываливается сразу (path not found)
	// количество процессов ОСТАЕТСЯ 5 и ни одно из ниже условий не проходит
	if (nPrevCount == 1 && srv.nProcessCount == 1 && srv.nProcessStartTick &&
		((dwLastCheckTick - srv.nProcessStartTick) > CHECK_ROOTSTART_TIMEOUT) &&
		WaitForSingleObject(ghFinalizeEvent,0) == WAIT_TIMEOUT)
	{
		nPrevCount = 2; // чтобы сработало следующее условие
		if (!gbAlwaysConfirmExit) gbAlwaysConfirmExit = TRUE; // чтобы консоль не схлопнулась
	}
	if (nPrevCount > 1 && srv.nProcessCount == 1 && srv.pnProcesses[0] == gnSelfPID) {
		CS.Unlock();
		if (!gbAlwaysConfirmExit && (dwLastCheckTick - srv.nProcessStartTick) <= CHECK_ROOTSTART_TIMEOUT)
			gbAlwaysConfirmExit = TRUE; // чтобы консоль не схлопнулась
		SetEvent(ghFinalizeEvent);
	}

	return lbChanged;
}

int CALLBACK FontEnumProc(ENUMLOGFONTEX *lpelfe, NEWTEXTMETRICEX *lpntme, DWORD FontType, LPARAM lParam)
{
    if ((FontType & TRUETYPE_FONTTYPE) == TRUETYPE_FONTTYPE) {
        // OK, подходит
        lstrcpyW(srv.szConsoleFont, lpelfe->elfLogFont.lfFaceName);
        return 0;
    }
    return TRUE; // ищем следующий фонт
}

HWND FindConEmuByPID()
{
	HWND hConEmuWnd = NULL;
	DWORD dwGuiThreadId = 0, dwGuiProcessId = 0;

    // GUI может еще "висеть" в ожидании или в отладчике, так что пробуем и через Snapshoot
    DWORD dwGuiPID = 0;
    HANDLE hSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS,0);
    if (hSnap != INVALID_HANDLE_VALUE) {
        PROCESSENTRY32 prc = {sizeof(PROCESSENTRY32)};
        if (Process32First(hSnap, &prc)) {
            do {
                if (prc.th32ProcessID == gnSelfPID) {
                    dwGuiPID = prc.th32ParentProcessID;
                    break;
                }
            } while (Process32Next(hSnap, &prc));
        }
        CloseHandle(hSnap);
    }
    if (dwGuiPID) {
        HWND hGui = NULL;
        while ((hGui = FindWindowEx(NULL, hGui, VirtualConsoleClassMain, NULL)) != NULL) {
            dwGuiThreadId = GetWindowThreadProcessId(hGui, &dwGuiProcessId);
            if (dwGuiProcessId == dwGuiPID) {
                hConEmuWnd = hGui;
                
                break;
            }
        }
    }

	return hConEmuWnd;
}

void CheckConEmuHwnd()
{
    //HWND hWndFore = GetForegroundWindow();
    //HWND hWndFocus = GetFocus();
    DWORD dwGuiThreadId = 0, dwGuiProcessId = 0;

    if (ghConEmuWnd == NULL) {
		SendStarted(); // Он и окно проверит, и параметры перешлет и размер консоли скорректирует
    }
    // GUI может еще "висеть" в ожидании или в отладчике, так что пробуем и через Snapshoot
    if (ghConEmuWnd == NULL) {
		ghConEmuWnd = FindConEmuByPID();
    }
    if (ghConEmuWnd == NULL) { // Если уж ничего не помогло...
        ghConEmuWnd = GetConEmuHWND(TRUE/*abRoot*/);
    }
    if (ghConEmuWnd) {
        // Установить переменную среды с дескриптором окна
        SetConEmuEnvVar(ghConEmuWnd);

		dwGuiThreadId = GetWindowThreadProcessId(ghConEmuWnd, &dwGuiProcessId);

		AllowSetForegroundWindow(dwGuiProcessId);

		//if (hWndFore == ghConWnd || hWndFocus == ghConWnd)
		//if (hWndFore != ghConEmuWnd)

		if (GetForegroundWindow() == ghConWnd)
			SetForegroundWindow(ghConEmuWnd); // 2009-09-14 почему-то было было ghConWnd ?

    } else {
		// да и фиг сним. нас могли просто так, без gui запустить
        //_ASSERTE(ghConEmuWnd!=NULL);
    }
}



DWORD WINAPI ServerThread(LPVOID lpvParam) 
{ 
   BOOL fConnected = FALSE;
   DWORD dwInstanceThreadId = 0, dwErr = 0;
   HANDLE hPipe = NULL, hInstanceThread = NULL;
   
 
// The main loop creates an instance of the named pipe and 
// then waits for a client to connect to it. When the client 
// connects, a thread is created to handle communications 
// with that client, and the loop is repeated. 
 
   for (;;) 
   { 
      MCHKHEAP
      hPipe = CreateNamedPipe( 
          srv.szPipename,               // pipe name 
          PIPE_ACCESS_DUPLEX,       // read/write access 
          PIPE_TYPE_MESSAGE |       // message type pipe 
          PIPE_READMODE_MESSAGE |   // message-read mode 
          PIPE_WAIT,                // blocking mode 
          PIPE_UNLIMITED_INSTANCES, // max. instances  
          PIPEBUFSIZE,              // output buffer size 
          PIPEBUFSIZE,              // input buffer size 
          0,                        // client time-out 
          gpNullSecurity);          // default security attribute 

      _ASSERTE(hPipe != INVALID_HANDLE_VALUE);
      
      if (hPipe == INVALID_HANDLE_VALUE) 
      {
          dwErr = GetLastError();
          wprintf(L"CreateNamedPipe failed, ErrCode=0x%08X\n", dwErr); 
          Sleep(50);
          //return 99;
          continue;
      }
 
      // Wait for the client to connect; if it succeeds, 
      // the function returns a nonzero value. If the function
      // returns zero, GetLastError returns ERROR_PIPE_CONNECTED. 
 
      fConnected = ConnectNamedPipe(hPipe, NULL) ? 
         TRUE : (GetLastError() == ERROR_PIPE_CONNECTED); 

	  if (WaitForSingleObject(ghExitEvent, 0) == WAIT_OBJECT_0) break;
 
      MCHKHEAP
      if (fConnected) 
      { 
      // Create a thread for this client. 
         hInstanceThread = CreateThread( 
            NULL,              // no security attribute 
            0,                 // default stack size 
            InstanceThread,    // thread proc
            (LPVOID) hPipe,    // thread parameter 
            0,                 // not suspended 
            &dwInstanceThreadId);      // returns thread ID 

         if (hInstanceThread == NULL) 
         {
            dwErr = GetLastError();
            wprintf(L"CreateThread(Instance) failed, ErrCode=0x%08X\n", dwErr);
            Sleep(50);
            //return 0;
            continue;
         }
         else {
             SafeCloseHandle(hInstanceThread); 
         }
       } 
      else {
        // The client could not connect, so close the pipe. 
         SafeCloseHandle(hPipe); 
      }
      MCHKHEAP
   } 
   return 1; 
} 

void ProcessInputMessage(MSG &msg)
{
	INPUT_RECORD r = {0};

	if (!UnpackInputRecord(&msg, &r)) {
		_ASSERT(FALSE);
		
	} else {
		TODO("Сделать обработку пачки сообщений, вдруг они накопились в очереди?");

		if (r.EventType == KEY_EVENT && r.Event.KeyEvent.bKeyDown &&
			(r.Event.KeyEvent.wVirtualKeyCode == 'C' || r.Event.KeyEvent.wVirtualKeyCode == VK_CANCEL)
			)
		{
			#define ALL_MODIFIERS (LEFT_ALT_PRESSED|RIGHT_ALT_PRESSED|LEFT_CTRL_PRESSED|RIGHT_CTRL_PRESSED|SHIFT_PRESSED)
			#define CTRL_MODIFIERS (LEFT_CTRL_PRESSED|RIGHT_CTRL_PRESSED)

			BOOL lbRc = FALSE;
			DWORD dwEvent = (r.Event.KeyEvent.wVirtualKeyCode == 'C') ? CTRL_C_EVENT : CTRL_BREAK_EVENT;
			//&& (srv.dwConsoleMode & ENABLE_PROCESSED_INPUT)

			//The SetConsoleMode function can disable the ENABLE_PROCESSED_INPUT mode for a console's input buffer, 
			//so CTRL+C is reported as keyboard input rather than as a signal. 
			// CTRL+BREAK is always treated as a signal
			if ( // Удерживается ТОЛЬКО Ctrl
				(r.Event.KeyEvent.dwControlKeyState & CTRL_MODIFIERS) &&
				((r.Event.KeyEvent.dwControlKeyState & ALL_MODIFIERS) 
				== (r.Event.KeyEvent.dwControlKeyState & CTRL_MODIFIERS))
				)
			{
				// Вроде работает, Главное не запускать процесс с флагом CREATE_NEW_PROCESS_GROUP
				// иначе у микрософтовской консоли (WinXP SP3) сносит крышу, и она реагирует
				// на Ctrl-Break, но напрочь игнорирует Ctrl-C
				lbRc = GenerateConsoleCtrlEvent(dwEvent, 0);

				// Это событие (Ctrl+C) в буфер помещается(!) иначе до фара не дойдет собственно клавиша C с нажатым Ctrl
			}
		}

		#ifdef _DEBUG
		if (r.EventType == KEY_EVENT && r.Event.KeyEvent.bKeyDown &&
			r.Event.KeyEvent.wVirtualKeyCode == VK_F11)
		{
			DEBUGSTR(L"  ---  F11 recieved\n");
		}
		#endif
		#ifdef _DEBUG
		if (r.EventType == MOUSE_EVENT) {
			wchar_t szDbg[60]; wsprintf(szDbg, L"ConEmuC.MouseEvent(X=%i,Y=%i,Btns=0x%04x,Moved=%i)\n", r.Event.MouseEvent.dwMousePosition.X, r.Event.MouseEvent.dwMousePosition.Y, r.Event.MouseEvent.dwButtonState, (r.Event.MouseEvent.dwEventFlags & MOUSE_MOVED));
			DEBUGLOGINPUT(szDbg);
		}
		#endif

		// Запомнить, когда была последняя активность пользователя
		if (r.EventType == KEY_EVENT
			|| (r.EventType == MOUSE_EVENT 
			&& (r.Event.MouseEvent.dwButtonState || r.Event.MouseEvent.dwEventFlags 
			|| r.Event.MouseEvent.dwEventFlags == DOUBLE_CLICK)))
		{
			srv.dwLastUserTick = GetTickCount();
		}

		SendConsoleEvent(&r, 1);
	}
}

DWORD WINAPI InputThread(LPVOID lpvParam) 
{
	MSG msg;
	while (GetMessage(&msg,0,0,0)) {
		if (msg.message == WM_QUIT) break;
		if (ghExitEvent) {
			if (WaitForSingleObject(ghExitEvent, 0) == WAIT_OBJECT_0) break;
		}
		if (msg.message == 0) continue;

		if (msg.message == INPUT_THREAD_ALIVE_MSG) {
			//pRCon->mn_FlushOut = msg.wParam;
			TODO("INPUT_THREAD_ALIVE_MSG");
			continue;

		} else {

			TODO("Сделать обработку пачки сообщений, вдруг они накопились в очереди?");
			ProcessInputMessage(msg);

		}
	}

	return 0;
}

DWORD WINAPI InputPipeThread(LPVOID lpvParam) 
{ 
   BOOL fConnected, fSuccess; 
   //DWORD nCurInputCount = 0;
   //DWORD srv.dwServerThreadId;
   HANDLE hPipe = NULL; 
   DWORD dwErr = 0;
   
 
// The main loop creates an instance of the named pipe and 
// then waits for a client to connect to it. When the client 
// connects, a thread is created to handle communications 
// with that client, and the loop is repeated. 
 
   for (;;) 
   { 
      MCHKHEAP
      hPipe = CreateNamedPipe( 
          srv.szInputname,          // pipe name 
          PIPE_ACCESS_INBOUND,      // goes from client to server only
          PIPE_TYPE_MESSAGE |       // message type pipe 
          PIPE_READMODE_MESSAGE |   // message-read mode 
          PIPE_WAIT,                // blocking mode 
          PIPE_UNLIMITED_INSTANCES, // max. instances  
          PIPEBUFSIZE,              // output buffer size 
          PIPEBUFSIZE,              // input buffer size 
          0,                        // client time-out
          gpNullSecurity);          // default security attribute 

      if (hPipe == INVALID_HANDLE_VALUE) 
      {
          dwErr = GetLastError();
		  _ASSERTE(hPipe != INVALID_HANDLE_VALUE);
          wprintf(L"CreatePipe failed, ErrCode=0x%08X\n", dwErr);
          Sleep(50);
          //return 99;
          continue;
      }
 
      // Wait for the client to connect; if it succeeds, 
      // the function returns a nonzero value. If the function
      // returns zero, GetLastError returns ERROR_PIPE_CONNECTED. 
 
      fConnected = ConnectNamedPipe(hPipe, NULL) ? 
         TRUE : (GetLastError() == ERROR_PIPE_CONNECTED); 
 
      MCHKHEAP
      if (fConnected) 
      { 
          //TODO:
          DWORD cbBytesRead; //, cbWritten;
          MSG imsg; memset(&imsg,0,sizeof(imsg));
          while ((fSuccess = ReadFile( 
             hPipe,        // handle to pipe 
             &imsg,        // buffer to receive data 
             sizeof(imsg), // size of buffer 
             &cbBytesRead, // number of bytes read 
             NULL)) != FALSE)        // not overlapped I/O 
          {
              // предусмотреть возможность завершения нити
              if (imsg.message == 0xFFFF) {
                  SafeCloseHandle(hPipe);
                  break;
              }
              MCHKHEAP
              if (imsg.message) {
                  // Если есть возможность - сразу пошлем его в dwInputThreadId
                  if (srv.dwInputThreadId) {
                     if (!PostThreadMessage(srv.dwInputThreadId, imsg.message, imsg.wParam, imsg.lParam)) {
				    	DWORD dwErr = GetLastError();
				    	wchar_t szErr[100];
				    	wsprintfW(szErr, L"ConEmuC: PostThreadMessage(%i) failed, code=0x%08X", srv.dwInputThreadId, dwErr);
				    	SetConsoleTitle(szErr);
                     }
                  } else {
                     ProcessInputMessage(imsg);
                  }
                  MCHKHEAP
              }
              // next
              memset(&imsg,0,sizeof(imsg));
              MCHKHEAP
          }
      } 
      else 
        // The client could not connect, so close the pipe. 
         SafeCloseHandle(hPipe);
   } 
   MCHKHEAP
   return 1; 
} 

BOOL SendConsoleEvent(INPUT_RECORD* pr, UINT nCount)
{
	BOOL fSuccess = FALSE;

	// Если сейчас идет ресайз - нежелательно помещение в буфер событий
	if (srv.bInSyncResize)
		WaitForSingleObject(srv.hAllowInputEvent, MAX_SYNCSETSIZE_WAIT);

	DWORD nCurInputCount = 0, cbWritten = 0;
	INPUT_RECORD irDummy[2] = {{0},{0}};

	// 27.06.2009 Maks - If input queue is not empty - wait for a while, to avoid conflicts with FAR reading queue
	if (PeekConsoleInput(ghConIn, irDummy, 1, &(nCurInputCount = 0)) && nCurInputCount > 0) {
		DWORD dwStartTick = GetTickCount();
		WARNING("Do NOT wait, but place event in Cyclic queue");
		do {
			Sleep(5);
			if (!PeekConsoleInput(ghConIn, irDummy, 1, &(nCurInputCount = 0)))
				nCurInputCount = 0;
		} while ((nCurInputCount > 0) && ((GetTickCount() - dwStartTick) < MAX_INPUT_QUEUE_EMPTY_WAIT));
	}

	fSuccess = WriteConsoleInput(ghConIn, pr, nCount, &cbWritten);
	_ASSERTE(fSuccess && cbWritten==nCount);

	return fSuccess;
} 
 
DWORD WINAPI InstanceThread(LPVOID lpvParam) 
{ 
    CESERVER_REQ in={0}, *pIn=NULL, *pOut=NULL;
    DWORD cbBytesRead, cbWritten, dwErr = 0;
    BOOL fSuccess; 
    HANDLE hPipe; 

    // The thread's parameter is a handle to a pipe instance. 
    hPipe = (HANDLE) lpvParam; 

    MCHKHEAP
 
    // Read client requests from the pipe. 
    memset(&in, 0, sizeof(in));
    fSuccess = ReadFile(
        hPipe,        // handle to pipe 
        &in,          // buffer to receive data 
        sizeof(in),   // size of buffer 
        &cbBytesRead, // number of bytes read 
        NULL);        // not overlapped I/O 

    if ((!fSuccess && ((dwErr = GetLastError()) != ERROR_MORE_DATA)) ||
            cbBytesRead < sizeof(CESERVER_REQ_HDR) || in.hdr.nSize < sizeof(CESERVER_REQ_HDR))
    {
        goto wrap;
    }

    if (in.hdr.nSize > cbBytesRead)
    {
        DWORD cbNextRead = 0;
		// Тут именно Alloc, а не ExecuteNewCmd, т.к. данные пришли снаружи, а не заполняются здесь
        pIn = (CESERVER_REQ*)Alloc(in.hdr.nSize, 1);
        if (!pIn)
            goto wrap;
		memmove(pIn, &in, in.hdr.nSize); // стояло ошибочное присвоение

		fSuccess = ReadFile(
            hPipe,        // handle to pipe 
            ((LPBYTE)pIn)+cbBytesRead,  // buffer to receive data 
            in.hdr.nSize - cbBytesRead,   // size of buffer 
            &cbNextRead, // number of bytes read 
            NULL);        // not overlapped I/O 
        if (fSuccess)
            cbBytesRead += cbNextRead;
    }

    if (!GetAnswerToRequest(pIn ? *pIn : in, &pOut) || pOut==NULL) {
    	// Если результата нет - все равно что-нибудь запишем, иначе TransactNamedPipe может виснуть?
    	CESERVER_REQ_HDR Out={0};
        Out.nCmd = in.hdr.nCmd;
        Out.nSrcThreadId = GetCurrentThreadId();
        Out.nSize = sizeof(Out);
        Out.nVersion = CESERVER_REQ_VER;
    	
	    fSuccess = WriteFile( 
	        hPipe,        // handle to pipe 
	        &Out,         // buffer to write from 
	        Out.nSize,    // number of bytes to write 
	        &cbWritten,   // number of bytes written 
	        NULL);        // not overlapped I/O 
        
    } else {
	    MCHKHEAP
	    // Write the reply to the pipe. 
	    fSuccess = WriteFile( 
	        hPipe,        // handle to pipe 
	        pOut,         // buffer to write from 
	        pOut->hdr.nSize,  // number of bytes to write 
	        &cbWritten,   // number of bytes written 
	        NULL);        // not overlapped I/O 

	    // освободить память
	    if ((LPVOID)pOut != (LPVOID)gpStoredOutput) // Если это НЕ сохраненный вывод
			ExecuteFreeResult(pOut);
    }

	if (pIn) { // не освобождалась, хотя, таких длинных команд наверное не было
		Free(pIn); pIn = NULL;
	}

    MCHKHEAP
    //if (!fSuccess || pOut->hdr.nSize != cbWritten) break; 

// Flush the pipe to allow the client to read the pipe's contents 
// before disconnecting. Then disconnect the pipe, and close the 
// handle to this pipe instance. 

wrap: // Flush и Disconnect делать всегда
    FlushFileBuffers(hPipe); 
    DisconnectNamedPipe(hPipe);
    SafeCloseHandle(hPipe); 

    return 1;
}

// На результат видимо придется не обращать внимания - не блокировать же выполнение
// сообщениями об ошибках... Возможно, что информацию об ошибке стоит записать
// в сам текстовый буфер.
BOOL ReadConsoleData(CESERVER_CHAR* pCheck /*= NULL*/)
{
    WARNING("Если передан prcDataChanged(!=0 && !=1) - считывать из консоли только его, с учетом прокрутки");
    WARNING("Для оптимизации вызовов - можно считать, что левый и правый край совпадают с краями консоли");
    WARNING("И только если в одно чтение это не удалось - читать строками у учетом реальных краев");
    WARNING("Если передан prcChanged - в него заносится РЕАЛЬНО изменный прямоугольник, для последующей пересылки в GUI");
    BOOL lbRc = TRUE, lbChanged = FALSE;
    //DWORD cbDataSize = 0; // Size in bytes of ONE buffer
    //srv.bContentsChanged = FALSE;
    EnterCriticalSection(&srv.csConBuf);
    //RECT rcReadRect = {0};
    // Если во время чтения консоли приходит ресайз - консоль глючит
    // может отдать совершенно левые данные (например не с той строки, с которой просили)
    //CSection cs(&srv.csChangeSize, &srv.ncsTChangeSize);
    MSectionLock CSCS; CSCS.Lock(&srv.cChangeSize);

    BOOL lbFirstLoop = TRUE;

Loop1:

    USHORT TextWidth=0, TextHeight=0;
    DWORD TextLen=0;
    COORD coord;

    MCHKHEAP

	#ifdef _DEBUG
	CONSOLE_SCREEN_BUFFER_INFO dbgSbi = srv.sbi;
	#endif
    
    TextWidth = srv.sbi.dwSize.X;
    // Уже должно быть поправлено в CorrectVisibleRect
    //if (gnBufferHeight == 0) {
    //  // Сервер мог еще не успеть среагировать на изменение режима BufferHeight
    //  if (srv.sbi.dwMaximumWindowSize.Y < srv.sbi.dwSize.Y)
    //      gnBufferHeight = srv.sbi.dwSize.Y; // Это однозначно буферный режим
    //}
    if (gnBufferHeight == 0 || NTVDMACTIVE)
	{
        TextHeight = srv.sbi.dwSize.Y; //srv.sbi.srWindow.Bottom - srv.sbi.srWindow.Top + 1;
    } else {
        //Для режима BufferHeight нужно считать по другому!
        TextHeight = min(gcrBufferSize.Y, srv.sbi.dwSize.Y);
    }
    srv.nVisibleHeight = TextHeight;

#ifdef _DEBUG
    // Вдруг случайно gnBufferHeight не установлен?
	if (TextHeight>150) {
		_ASSERTE(TextHeight<=150);
	}
    // Высота окна в консоли должна быть равна высоте в GUI, иначе мы будем терять строку с курсором при прокрутке
	if (!NTVDMACTIVE) {
		if (TextHeight != (srv.sbi.srWindow.Bottom-srv.sbi.srWindow.Top+1)) {
			_ASSERTE(TextHeight==(srv.sbi.srWindow.Bottom-srv.sbi.srWindow.Top+1));
		}
	}
#endif
    
    TextLen = TextWidth * TextHeight;
    // Если для чтения всех требуемых строк требуется больший объем данных, нежели уже выделен
    if (TextLen > srv.nBufCharCount) {
        lbChanged = TRUE;
        Free(srv.psChars);
        srv.psChars = (wchar_t*)Alloc(TextLen*2,sizeof(wchar_t));
        _ASSERTE(srv.psChars!=NULL);
        Free(srv.pnAttrs);
        srv.pnAttrs = (WORD*)Alloc(TextLen*2,sizeof(WORD));
        _ASSERTE(srv.pnAttrs!=NULL);
        if (srv.psChars && srv.pnAttrs) {
            srv.nBufCharCount = TextLen;
        } else {
            srv.nBufCharCount = 0; // ошибка выделения памяти
            lbRc = FALSE;
        }
    }
    srv.nOneBufferSize = TextLen*2; // размер в байтах!

    MCHKHEAP

    // Если для чтения одной строки (используется для сравнения места курсора) требуется больший объем данных
    if (TextWidth > srv.nLineCmpSize) {
        Free(srv.ptrLineCmp);
        srv.ptrLineCmp = (WORD*)Alloc(TextWidth*2,sizeof(WORD));
        _ASSERTE(srv.ptrLineCmp!=NULL);
        if (srv.ptrLineCmp) {
            srv.nLineCmpSize = TextWidth*2;
        } else {
            srv.nLineCmpSize = 0;
        }
    }

    MCHKHEAP

    coord.X = 0;
    coord.Y = (srv.nTopVisibleLine == -1) ? srv.sbi.srWindow.Top : srv.nTopVisibleLine;

    //TODO: перечитывать содержимое ТОЛЬКО по srv.bContentsChanged
    // ПЕРЕД чтением - сбросить srv.bContentsChanged в FALSE
    // буфер сделать глобальным, 
    // его использование - закрыть через CriticalSection
    
    // Читаем консоль только если требуется
    if (srv.psChars && srv.pnAttrs && (srv.bForceFullSend || srv.bNeedFullReload || pCheck != NULL))
    {
        //dwAllSize += TextWidth*TextHeight*4;
        
        // Get attributes (first) and text (second)
        // [Roman Kuzmin]
        // In FAR Manager source code this is mentioned as "fucked method". Yes, it is.
        // Functions ReadConsoleOutput* fail if requested data size exceeds their buffer;
        // MSDN says 64K is max but it does not say how much actually we can request now.
        // Experiments show that this limit is floating and it can be much less than 64K.
        // The solution below is not optimal when a user sets small font and large window,
        // but it is safe and practically optimal, because most of users set larger fonts
        // for large window and ReadConsoleOutput works OK. More optimal solution for all
        // cases is not that difficult to develop but it will be increased complexity and
        // overhead often for nothing, not sure that we really should use it.

        DWORD nbActuallyRead;

        TODO("Если передан непустой прямоугольник pCheck - скорее всего можно считывать только его");
        if (!srv.bForceFullSend && pCheck && pCheck->hdr.nSize)
        {
            // Читаем только прямоугольник, определенный в pCheck->hdr.
            WARNING("Координаты в pCheck->hdr должны быть абсолютными, а не видимыми");
            // Читаем только видимую (в GUI) часть! ( {0,coord.Y} - {srv.sbi.dwSize.X-1,coord.Y+TextHeight-1} )
            // Скорректировать измененный прямоугольник. Если данные не менялись - ставим (pCheck->hdr.nSize=0)

            int nTop = coord.Y;
            int nLines = pCheck->hdr.cr2.Y - pCheck->hdr.cr1.Y + 1;

            // Если мы попали во время ресайза...
            if (pCheck->hdr.cr1.Y < nTop)
                pCheck->hdr.cr1.Y = nTop;
            if (pCheck->hdr.cr2.Y >= (nTop+srv.nVisibleHeight)) {
                srv.bRequestPostFullReload = TRUE;

                if (lbFirstLoop) {
                    lbFirstLoop = FALSE;
                    srv.bForceFullSend = TRUE;
                    if (pCheck)
                        memset(pCheck, 0, sizeof(CESERVER_CHAR_HDR));
                    goto Loop1;
                }
                pCheck->hdr.cr2.Y = (nTop+srv.nVisibleHeight-1);
            }
            _ASSERTE(pCheck->hdr.cr1.Y>=0 && pCheck->hdr.cr1.Y<srv.sbi.dwSize.Y);
            _ASSERTE(pCheck->hdr.cr2.Y>=0 && pCheck->hdr.cr2.Y<srv.sbi.dwSize.Y);
            //_ASSERTE(pCheck->hdr.cr2.Y<=(nTop+srv.nVisibleHeight));

            coord.X = 0; coord.Y = pCheck->hdr.cr1.Y;
            int nShift = TextWidth * (pCheck->hdr.cr1.Y - nTop);
            wchar_t* ConCharNow = srv.psChars + nShift;
            wchar_t* ConCharEnd = srv.psChars + srv.nOneBufferSize;
            WORD* ConAttrNow = srv.pnAttrs + nShift;
            int nRectLen = TextWidth * nLines;
            int y;

            MCHKHEAP

            // Если мы попали во время ресайза...
            if (((DWORD)(nShift+nRectLen)) > TextLen) {
                srv.bRequestPostFullReload = TRUE;

                if (lbFirstLoop) {
                    lbFirstLoop = FALSE;
                    srv.bForceFullSend = TRUE;
                    if (pCheck)
                        memset(pCheck, 0, sizeof(CESERVER_CHAR_HDR));
                    goto Loop1;
                }

                // Попытаемся ограничить чтение требуемым размером
                TextWidth = srv.sbi.dwSize.X;
                TextHeight = (gnBufferHeight == 0) ? srv.sbi.dwSize.Y : gcrBufferSize.Y;
                srv.nVisibleHeight = TextHeight;
                TextLen = TextWidth * TextHeight;
                if (TextLen > srv.nBufCharCount) {
                    TextLen = srv.nBufCharCount;
                    TextHeight = (USHORT)(TextLen / TextWidth);
                }
                pCheck->hdr.cr1.X = min(pCheck->hdr.cr1.X,(TextWidth-1));
                pCheck->hdr.cr2.X = min(pCheck->hdr.cr2.X,(TextWidth-1));
                pCheck->hdr.cr1.Y = min(pCheck->hdr.cr1.Y,(TextHeight-1));
                pCheck->hdr.cr2.Y = min(pCheck->hdr.cr2.Y,(TextHeight-1));

                nShift = TextWidth * (pCheck->hdr.cr1.Y - nTop);
                ConCharNow = srv.psChars + nShift;
                ConCharEnd = srv.psChars + srv.nOneBufferSize;
                ConAttrNow = srv.pnAttrs + nShift;
                nRectLen = TextWidth * nLines;
            }
            _ASSERTE(((DWORD)(nShift+nRectLen))<=TextLen);

            if (!ReadConsoleOutputAttribute(ghConOut, ConAttrNow, nRectLen, coord, &nbActuallyRead)
                || !ReadConsoleOutputCharacter(ghConOut, ConCharNow, nRectLen, coord, &nbActuallyRead) 
                )
            {
                DEBUGSTR(L" !!! Can't read full console screen. Read line by line\n");
                for (y = 0; y < nLines; y++, coord.Y ++)
                {
                    _ASSERTE(ConCharNow<ConCharEnd);
                    if (ConCharNow>=ConCharEnd) break;
                    ReadConsoleOutputAttribute(ghConOut, ConAttrNow, TextWidth, coord, &nbActuallyRead);
                        ConAttrNow += TextWidth;
                    ReadConsoleOutputCharacter(ghConOut, ConCharNow, TextWidth, coord, &nbActuallyRead);
                        ConCharNow += TextWidth;
                }
            }
            TODO("Во время ресайза консоль может подглючивать - отдает не то что нужно...");
            if (gnBufferHeight && *srv.psChars!=9553) {
                //_ASSERTE(*srv.psChars!=9553);
            }

            MCHKHEAP
            // Теперь коррекция измененного прямоугольника
            COORD cr1 = {TextWidth,TextHeight};
            COORD cr2 = {-1,-1};
            ConCharNow = srv.psChars + nShift;
            ConAttrNow = srv.pnAttrs + nShift;
            wchar_t* ConCharCmp = srv.psChars + nShift + srv.nBufCharCount; // Именно srv.nBufCharCount, а не TextLen!
            WORD* ConAttrCmp = srv.pnAttrs + nShift + srv.nBufCharCount;
            int x, LineSize = TextWidth*2;
            for (y = 0; y < nLines; y++)
            {
                MCHKHEAP
                if (memcmp(ConCharCmp, ConCharNow, LineSize) != 0 
                    || memcmp(ConAttrCmp, ConAttrNow, LineSize) != 0)
                {
                    if (cr1.X > 0 || cr2.X < (TextWidth-1))
                    {
                        int x1 = TextWidth, x2 = -1;
                        for (x = 0; x < TextWidth; x++) {
                            if (ConCharCmp[x] != ConCharNow[x] || ConAttrCmp[x] != ConAttrNow[x]) {
                                _ASSERTE(x1>x);
                                x1 = x;
                                break;
                            }
                        }
                        for (x = TextWidth-1; x >= 0; x--) {
                            if (ConCharCmp[x] != ConCharNow[x] || ConAttrCmp[x] != ConAttrNow[x]) {
                                _ASSERTE(x2 < x);
                                x2 = x;
                                break;
                            }
                        }
                        if (x1>x2) {
                            _ASSERTE(x1<=x2);
                        }

                        if (x1 < cr1.X) cr1.X = x1;
                        if (x2 > cr2.X) cr2.X = x2;
                    }

                    if (y < cr1.Y) cr1.Y = y;
                    if (y > cr2.Y) cr2.Y = y;
                }

                // Next line
                ConCharCmp += TextWidth;
                ConCharNow += TextWidth;
                ConAttrCmp += TextWidth;
                ConAttrNow += TextWidth;


                //// Левый край
                //WARNING("Где-то тут глючит определение измененного прямоугольника!");
                //for (x = 0; x < cr1.X; x++) {
                //  if (ConCharCmp[x] != ConCharNow[x] || ConAttrCmp[x] != ConAttrNow[x]) {
                //      cr1.X = x;
                //      if (x > cr2.X)
                //          cr2.X = x;
                //      if (y < cr1.Y)
                //          cr1.Y = y;
                //      if (y > cr2.Y)
                //          cr2.Y = y;
                //      break;
                //  }
                //}
                //// Правый край
                //for (x = cr2.X-1; x > cr1.X; x--) {
                //  if (ConCharCmp[x] != ConCharNow[x] || ConAttrCmp[x] != ConAttrNow[x]) {
                //      if (x < cr1.X)
                //          cr1.X = x;
                //      cr2.X = x;
                //      if (y < cr1.Y)
                //          cr1.Y = y;
                //      if (y > cr2.Y)
                //          cr2.Y = y;
                //      break;
                //  }
                //}
            }
            MCHKHEAP
            if (cr2.X != -1) {
                // Добавить номер строки, с которой начали считать
                // Теперь у нас АБСОЛЮТНЫЕ координаты изменений но в ВИДИМОЙ области
                cr1.Y += pCheck->hdr.cr1.Y; cr2.Y += pCheck->hdr.cr1.Y;
                _ASSERTE(cr1.Y>=0 && cr1.Y<srv.sbi.dwSize.Y);
                _ASSERTE(cr2.Y>=0 && cr2.Y<srv.sbi.dwSize.Y);

                lbChanged = TRUE;
                _ASSERTE(cr2.X>=cr1.X && cr2.Y>=cr1.Y);
                //cr1.Y += nTop; cr2.Y += nTop;
                if (cr1.X == 0 && cr1.Y == 0 && cr2.X == (TextWidth-1) && cr2.Y == (TextHeight-1)) {
                    // Консоль изменилась полностью - не маяться с передачей прямоугольника
                    srv.bForceFullSend = TRUE;
                    memset(&pCheck->hdr, 0, sizeof(pCheck->hdr));
                    goto Loop1;
                } else {
                    pCheck->hdr.cr1 = cr1;
                    pCheck->hdr.cr2 = cr2;
                }
            } else {
                memset(&pCheck->hdr, 0, sizeof(pCheck->hdr));
            }
            
        } else {
            MCHKHEAP
            // Читаем всю видимую в GUI область
            if (!ReadConsoleOutputAttribute(ghConOut, srv.pnAttrs, TextLen, coord, &nbActuallyRead)
                || !ReadConsoleOutputCharacter(ghConOut, srv.psChars, TextLen, coord, &nbActuallyRead)
                )
            {
                DEBUGSTR(L" !!! Can't read full console screen. Read line by line\n");
                WORD* ConAttrNow = srv.pnAttrs; wchar_t* ConCharNow = srv.psChars;
                for(int y = 0; y < (int)TextHeight; y++, coord.Y++)
                {
                    MCHKHEAP
                    ReadConsoleOutputAttribute(ghConOut, ConAttrNow, TextWidth, coord, &nbActuallyRead);
                        ConAttrNow += TextWidth;
                    ReadConsoleOutputCharacter(ghConOut, ConCharNow, TextWidth, coord, &nbActuallyRead);
                        ConCharNow += TextWidth;
                }
            }
            TODO("Во время ресайза консоль может подглючивать - отдает не то что нужно...");
            //_ASSERTE(*srv.psChars!=9553);
            if (srv.bForceFullSend)
                lbChanged = TRUE;

            if (!lbChanged)
            {
                MCHKHEAP
                // Тут сдвиг именно srv.nBufCharCount, а не TextLen, чтобы не рассчитывать его каждый раз
                if (memcmp(srv.psChars, srv.psChars+srv.nBufCharCount, TextLen*sizeof(wchar_t)))
                    lbChanged = TRUE;
                else if (memcmp(srv.pnAttrs, srv.pnAttrs+srv.nBufCharCount, TextLen*sizeof(WORD)))
                    lbChanged = TRUE;
                MCHKHEAP
            }
        }

        if (TextWidth != srv.sbi.dwSize.X)
            srv.bRequestPostFullReload = TRUE;

        //cbDataSize = TextLen * 2; // Size in bytes of ONE buffer


    } else {
        lbRc = FALSE;
    }

    MCHKHEAP
    //if (pbDataChanged)
    //    *pbDataChanged = lbChanged;

    LeaveCriticalSection(&srv.csConBuf);
    //return cbDataSize;
    return lbChanged;
}

BOOL GetAnswerToRequest(CESERVER_REQ& in, CESERVER_REQ** out)
{
    BOOL lbRc = FALSE;

    MCHKHEAP

    switch (in.hdr.nCmd) {
        case CECMD_GETSHORTINFO:
        case CECMD_GETFULLINFO:
        {
            if (srv.szGuiPipeName[0] == 0) { // Серверный пайп в CVirtualConsole уже должен быть запущен
                wsprintf(srv.szGuiPipeName, CEGUIPIPENAME, L".", (DWORD)ghConWnd); // был gnSelfPID
            }

            _ASSERT(ghConOut && ghConOut!=INVALID_HANDLE_VALUE);
            if (ghConOut==NULL || ghConOut==INVALID_HANDLE_VALUE)
                return FALSE;

			if (in.hdr.nCmd == CECMD_GETFULLINFO) {
				// Размер консоли мог только что измениться, и еще не обновилась srv.sbi
				// Секцию блокируем заранее, чтобы не было расхождений в размерах
				//CSection cs(&srv.csChangeSize, &srv.ncsTChangeSize);
				MSectionLock CSCS; CSCS.Lock(&srv.cChangeSize);
				MyGetConsoleScreenBufferInfo(ghConOut, &srv.sbi);
                ReadConsoleData(NULL);
			}

            MCHKHEAP

            // На запрос из GUI (GetAnswerToRequest)
            *out = CreateConsoleInfo(NULL, (in.hdr.nCmd == CECMD_GETFULLINFO));

            MCHKHEAP

            lbRc = TRUE;
        } break;
        case CECMD_SETSIZE:
		case CECMD_SETSIZESYNC:
        case CECMD_CMDSTARTED:
        case CECMD_CMDFINISHED:
        {
            MCHKHEAP
            int nOutSize = sizeof(CESERVER_REQ_HDR) + sizeof(CONSOLE_SCREEN_BUFFER_INFO) + sizeof(DWORD);
            *out = ExecuteNewCmd(0,nOutSize);
            if (*out == NULL) return FALSE;
            MCHKHEAP
            if (in.hdr.nSize >= (sizeof(CESERVER_REQ_HDR) + sizeof(CESERVER_REQ_SETSIZE))) {
                USHORT nBufferHeight = 0;
                COORD  crNewSize = {0,0};
                SMALL_RECT rNewRect = {0};
                SHORT  nNewTopVisible = -1;
                //memmove(&nBufferHeight, in.Data, sizeof(USHORT));
                nBufferHeight = in.SetSize.nBufferHeight;
                //memmove(&crNewSize, in.Data+sizeof(USHORT), sizeof(COORD));
                crNewSize = in.SetSize.size;
                //memmove(&nNewTopVisible, in.Data+sizeof(USHORT)+sizeof(COORD), sizeof(SHORT));
                nNewTopVisible = in.SetSize.nSendTopLine;
                //memmove(&rNewRect, in.Data+sizeof(USHORT)+sizeof(COORD)+sizeof(SHORT), sizeof(SMALL_RECT));
                rNewRect = in.SetSize.rcWindow;

                MCHKHEAP

                (*out)->hdr.nCmd = in.hdr.nCmd;
                (*out)->hdr.nSrcThreadId = GetCurrentThreadId();

                //#ifdef _DEBUG
                if (in.hdr.nCmd == CECMD_CMDFINISHED) {
                	PRINT_COMSPEC(L"CECMD_CMDFINISHED, Set height to: %i\n", crNewSize.Y);
                    DEBUGSTR(L"\n!!! CECMD_CMDFINISHED !!!\n\n");
                    // Вернуть нотификатор
                    if (srv.dwWinEventThread != 0)
                    	PostThreadMessage(srv.dwWinEventThread, srv.nMsgHookEnableDisable, TRUE, 0);
                } else if (in.hdr.nCmd == CECMD_CMDSTARTED) {
                	PRINT_COMSPEC(L"CECMD_CMDSTARTED, Set height to: %i\n", nBufferHeight);
                    DEBUGSTR(L"\n!!! CECMD_CMDSTARTED !!!\n\n");
                    // Отключить нотификатор
                    if (srv.dwWinEventThread != 0)
                    	PostThreadMessage(srv.dwWinEventThread, srv.nMsgHookEnableDisable, FALSE, 0);
                }
                //#endif

                if (in.hdr.nCmd == CECMD_CMDFINISHED) {
                    // Сохранить данные ВСЕЙ консоли
                    PRINT_COMSPEC(L"Storing long output\n", 0);
                    CmdOutputStore();
                    PRINT_COMSPEC(L"Storing long output (done)\n", 0);
                }

                MCHKHEAP

				if (in.hdr.nCmd == CECMD_SETSIZESYNC) {
					ResetEvent(srv.hAllowInputEvent);
					srv.bInSyncResize = TRUE;
				}

                srv.nTopVisibleLine = nNewTopVisible;
                SetConsoleSize(nBufferHeight, crNewSize, rNewRect, ":CECMD_SETSIZE");

				if (in.hdr.nCmd == CECMD_SETSIZESYNC) {
					INPUT_RECORD r = {WINDOW_BUFFER_SIZE_EVENT};
					r.Event.WindowBufferSizeEvent.dwSize = crNewSize;
					DWORD dwWritten = 0;
					if (WriteConsoleInput(ghConIn, &r, 1, &dwWritten)) {
						if (PeekConsoleInput(ghConIn, &r, 1, &(dwWritten = 0)) && dwWritten > 0) {
							DWORD dwStartTick = GetTickCount();
							do {
								Sleep(5);
								if (!PeekConsoleInput(ghConIn, &r, 1, &(dwWritten = 0)))
									dwWritten = 0;
							} while ((dwWritten > 0) && ((GetTickCount() - dwStartTick) < MAX_SYNCSETSIZE_WAIT));
						}
					}

					SetEvent(srv.hAllowInputEvent);
					srv.bInSyncResize = FALSE;
				}

                MCHKHEAP

                if (in.hdr.nCmd == CECMD_CMDSTARTED) {
                    // Восстановить текст скрытой (прокрученной вверх) части консоли
                    CmdOutputRestore();
                }
            }
            MCHKHEAP
            
            PCONSOLE_SCREEN_BUFFER_INFO psc = &((*out)->SetSizeRet.SetSizeRet);
            MyGetConsoleScreenBufferInfo(ghConOut, psc);
            
            DWORD nPacketId = ++srv.nLastPacketID;
            (*out)->SetSizeRet.nNextPacketId = nPacketId;
            
            srv.bForceFullSend = TRUE;
            SetEvent(srv.hRefreshEvent);

            MCHKHEAP

            lbRc = TRUE;
        } break;

        case CECMD_GETOUTPUT:
        {
            if (gpStoredOutput) {
                gpStoredOutput->hdr.hdr.nCmd = CECMD_GETOUTPUT;
                gpStoredOutput->hdr.hdr.nSize = 
                    sizeof(CESERVER_CONSAVE_HDR)
                    + min((int)gpStoredOutput->hdr.cbMaxOneBufferSize, 
                          (gpStoredOutput->hdr.sbi.dwSize.X*gpStoredOutput->hdr.sbi.dwSize.Y*2));
                gpStoredOutput->hdr.hdr.nVersion = CESERVER_REQ_VER;
                gpStoredOutput->hdr.hdr.nSrcThreadId = GetCurrentThreadId();

                *out = (CESERVER_REQ*)gpStoredOutput;

                lbRc = TRUE;
            }
        } break;
        
        case CECMD_ATTACH2GUI:
        {
			HWND hDc = Attach2Gui(1000);
			if (hDc != NULL) {
				int nOutSize = sizeof(CESERVER_REQ_HDR) + sizeof(DWORD);
				*out = ExecuteNewCmd(CECMD_ATTACH2GUI,nOutSize);
				if (*out != NULL) {
					(*out)->dwData[0] = (DWORD)hDc;
					lbRc = TRUE;
				}
			}
        } break;
        
        case CECMD_FARLOADED:
        {
        	if (gbAutoDisableConfirmExit && srv.dwRootProcess == in.dwData[0]) {
				// FAR нормально запустился, считаем что все ок и подтверждения закрытия консоли не потребуется
				gbAutoDisableConfirmExit = FALSE; gbAlwaysConfirmExit = FALSE;
				srv.nProcessStartTick = GetTickCount() - 2*CHECK_ROOTSTART_TIMEOUT;
        	}
        } break;
        
        case CECMD_SHOWCONSOLE:
        {
        	ShowWindow(ghConWnd, in.dwData[0]);
        } break;

		case CECMD_POSTCONMSG:
		{
			if (in.Msg.bPost) {
				PostMessage(ghConWnd, in.Msg.nMsg, (WPARAM)in.Msg.wParam, (LPARAM)in.Msg.lParam);
			} else {
				LRESULT lRc = SendMessage(ghConWnd, in.Msg.nMsg, (WPARAM)in.Msg.wParam, (LPARAM)in.Msg.lParam);
				// Возвращаем результат
				int nOutSize = sizeof(CESERVER_REQ_HDR) + sizeof(u64);
				*out = ExecuteNewCmd(CECMD_POSTCONMSG,nOutSize);
				if (*out != NULL) {
					(*out)->qwData[0] = lRc;
					lbRc = TRUE;
				}
			}
		} break;
    }
    
    if (gbInRecreateRoot) gbInRecreateRoot = FALSE;
    return lbRc;
}

BOOL HookWinEvents(int abEnabled)
{
	if (srv.dwWinEventThread != 0 && GetCurrentThreadId() != srv.dwWinEventThread) {
		// Функция должна выполняться в потоке с GetMessage
       	PostThreadMessage(srv.dwWinEventThread, srv.nMsgHookEnableDisable, TRUE, 0);
       	
        return TRUE;
	}
	
	if (abEnabled == 1 || abEnabled == 2) {
		if (srv.hWinHook != NULL) {
			PRINT_COMSPEC(L"!!! HookWinEvents was already set !!!\n", 0);
			return TRUE;
		}
		
		if ((abEnabled == 2) && !srv.hWinHook) {
			// "Ловим" все консольные события (кроме Start/End)
			srv.hWinHook = SetWinEventHook(EVENT_CONSOLE_CARET,EVENT_CONSOLE_LAYOUT,
				NULL, (WINEVENTPROC)WinEventProc, 0,0, WINEVENT_OUTOFCONTEXT /*| WINEVENT_SKIPOWNPROCESS ?*/);

			if (!srv.hWinHook) {
        		PRINT_COMSPEC(L"!!! HookWinEvents FAILED, ErrCode=0x%08X\n", GetLastError());
        		return FALSE;
			}
			
			PRINT_COMSPEC(L"WinEventsHook was enabled\n", 0);
			#ifdef SHOW_STARTED_MSGBOX
			wchar_t szTitle[60]; wsprintf(szTitle, L"ConEmuC(PID=%i)", GetCurrentProcessId());
			MessageBox(NULL,L"WinEventsHook was enabled",szTitle,0);
			#endif
        }

		if (!srv.hWinHookStartEnd) {
			// "Ловим" (Start/End)
			srv.hWinHookStartEnd = SetWinEventHook(EVENT_CONSOLE_START_APPLICATION,EVENT_CONSOLE_END_APPLICATION,
				NULL, (WINEVENTPROC)WinEventProc, 0,0, WINEVENT_OUTOFCONTEXT /*| WINEVENT_SKIPOWNPROCESS ?*/);

			if (!srv.hWinHookStartEnd) {
        		PRINT_COMSPEC(L"!!! HookWinEvents(StartEnd) FAILED, ErrCode=0x%08X\n", GetLastError());
        		return FALSE;
			}
			
			PRINT_COMSPEC(L"WinEventsHook(StartEnd) was enabled\n", 0);
        }
        
	} else
	if (abEnabled != 1) {
		if (abEnabled == -1 && srv.hWinHookStartEnd) {
	        UnhookWinEvent(srv.hWinHookStartEnd); srv.hWinHookStartEnd = NULL;
	        PRINT_COMSPEC(L"WinEventsHook(StartEnd) was disabled\n", 0);
		}
	    if (srv.hWinHook) {
			#ifdef _DEBUG
			BOOL lbUnhookRc = 
			#endif
	        UnhookWinEvent(srv.hWinHook); srv.hWinHook = NULL;
	        PRINT_COMSPEC(L"WinEventsHook was disabled\n", 0);
			#ifdef SHOW_STARTED_MSGBOX
			wchar_t szTitle[60]; wsprintf(szTitle, L"ConEmuC(PID=%i)", GetCurrentProcessId());
			MessageBox(NULL,L"WinEventsHook was disabled",szTitle,0);
			#endif
	    }
	    return TRUE;
	}
	
	return FALSE;
}

DWORD WINAPI WinEventThread(LPVOID lpvParam)
{
    //DWORD dwErr = 0;
    //HANDLE hStartedEvent = (HANDLE)lpvParam;
    
    // На всякий случай
    srv.dwWinEventThread = GetCurrentThreadId();
    
    
    // По умолчанию - ловим только StartStop.
    // При появлении в консоли FAR'а - включим все события
    srv.bWinHookAllow = TRUE; srv.nWinHookMode = 1;
    HookWinEvents ( 1 );
    
    //
    //SetEvent(hStartedEvent); hStartedEvent = NULL; // здесь он более не требуется

    MSG lpMsg;
    while (GetMessage(&lpMsg, NULL, 0, 0))
    {
    	if (lpMsg.message == srv.nMsgHookEnableDisable) {
    		srv.bWinHookAllow = (lpMsg.wParam != 0);
			HookWinEvents ( srv.bWinHookAllow ? srv.nWinHookMode : 0 );
    		continue;
    	}
        MCHKHEAP
        //if (lpMsg.message == WM_QUIT) { // GetMessage возвращает FALSE при получении этого сообщения
        //  lbQuit = TRUE; break;
        //}
        TranslateMessage(&lpMsg);
        DispatchMessage(&lpMsg);
        MCHKHEAP
    }
    
    // Закрыть хук
    HookWinEvents ( -1 );

    MCHKHEAP
    
    return 0;
}

DWORD WINAPI RefreshThread(LPVOID lpvParam)
{
    DWORD /*dwErr = 0,*/ nWait = 0;
    HANDLE hEvents[2] = {ghExitEvent, srv.hRefreshEvent};
    CONSOLE_CURSOR_INFO lci = {0}; // GetConsoleCursorInfo
    CONSOLE_SCREEN_BUFFER_INFO lsbi = {{0,0}}; // MyGetConsoleScreenBufferInfo
    DWORD nDelayRefresh = MAX_FORCEREFRESH_INTERVAL;
    DWORD nDelta = 0;
    DWORD nLastUpdateTick = GetTickCount();
    DWORD nIdleTick = nLastUpdateTick;
    BOOL  lbEventualChange = FALSE;
    DWORD dwTimeout = 10; // задержка чтения информации об окне (размеров, курсора,...)
    BOOL  lbFirstForeground = TRUE;
    BOOL  lbLocalForced = FALSE;
    
    BOOL lbQuit = FALSE;

    while (!lbQuit)
    {
        nWait = WAIT_TIMEOUT;
        MCHKHEAP
        
        // Подождать немножко
        // !!! Здесь таймаут должен быть минимальным, ну разве что консоль неактивна
        nWait = WaitForMultipleObjects ( 2, hEvents, FALSE, /*srv.bConsoleActive ? srv.nMainTimerElapse :*/ dwTimeout );
        if (nWait == WAIT_OBJECT_0)
            break; // затребовано завершение нити
        #ifdef _DEBUG
        if (nWait == (WAIT_OBJECT_0+1)) {
            DEBUGSTR(L"*** hRefreshEvent was set, checking console...\n");
        }
        #endif
        
        if (gnBufferHeight != 0 || srv.nWinHookMode != 2) {
        	if (srv.nMaxFPS>0) {
        		dwTimeout = 1000 / srv.nMaxFPS;
        		if (dwTimeout < 50) dwTimeout = 50;
        	} else {
        		dwTimeout = 100;
        	}
        } else {
        	dwTimeout = 10;
       	}

		if (ghConEmuWnd && !IsWindow(ghConEmuWnd)) {
			ghConEmuWnd = NULL;
			//ShowWindowAsync(ghConWnd, SW_SHOWNORMAL);
			//EnableWindow(ghConWnd, true);
			EmergencyShow();
		}

        if (ghConEmuWnd && GetForegroundWindow() == ghConWnd) {
            if (lbFirstForeground || !IsWindowVisible(ghConWnd)) {
                DEBUGSTR(L"...SetForegroundWindow(ghConEmuWnd);\n");
                SetForegroundWindow(ghConEmuWnd);
                lbFirstForeground = FALSE;
            }
        }

        // Проверим, реальный это таймаут - или изменение по событию от консоли
        lbEventualChange = (nWait == (WAIT_OBJECT_0+1));
        
        // Если можем - проверим текущую раскладку в консоли
        if (pfnGetConsoleKeyboardLayoutName)
            CheckKeyboardLayout();

        // Если это таймаут и не буферный режим //и нет пользовательской активности
        if ((gnBufferHeight != 0) && !lbEventualChange /*&& !(USER_ACTIVITY)*/) {
        	srv.bRequestPostFullReload = TRUE; // считать данные полностью
            //DWORD nCurTick = GetTickCount();
            //nDelta = nCurTick - nIdleTick;
            //if (nDelta < CHECK_IDLE_TIMEOUT) {
            //    // еще подождем
            //    DWORD nSleep = CHECK_IDLE_TIMEOUT - nDelta;
            //    if (nSleep > CHECK_IDLE_TIMEOUT) nSleep = CHECK_IDLE_TIMEOUT;
            //    Sleep(nSleep);
            //    srv.bRequestPostFullReload = TRUE; // считать данные полностью
            //}
        }
        nIdleTick = GetTickCount();

        
        // В прошлый раз было запрошено полное повторное сканирование консоли
        if (srv.bRequestPostFullReload) {
            DEBUGSTR(L"...bRequestPostFullReload detected, full reload forces\n");
            srv.bNeedFullReload = TRUE;
            srv.bRequestPostFullReload = FALSE; // Сброс. FullReload уже выставили
            nWait = (WAIT_OBJECT_0+1); // Принудительно
            memset(&srv.CharChanged.hdr, 0, sizeof(srv.CharChanged.hdr));
            lbEventualChange = TRUE; // чтобы следующий интервал чтения не увеличился
        }


        // Проверка позиции курсора и размера буфера - это вызовет частичное или полное сканирование консоли
        lbLocalForced = FALSE;
		// Если все еще считаем, что изменений нет
        if (nWait == WAIT_TIMEOUT) {
            // К сожалению, исключительно курсорные события не приходят (если консоль не в фокусе)
            if (MyGetConsoleScreenBufferInfo(ghConOut, &lsbi)) {
                if (memcmp(&srv.sbi.dwCursorPosition, &lsbi.dwCursorPosition, sizeof(lsbi.dwCursorPosition))) {
                    DEBUGSTR(L"...dwCursorPosition changed\n");
                    lbLocalForced = TRUE;
                    nWait = (WAIT_OBJECT_0+1);
                }
            }
            if ((nWait == WAIT_TIMEOUT) && GetConsoleCursorInfo(ghConOut, &lci)) {
                if (memcmp(&srv.ci, &lci, sizeof(srv.ci))) {
                    DEBUGSTR(L"...CursorInfo changed\n");
                    lbLocalForced = TRUE;
                    nWait = (WAIT_OBJECT_0+1);
                }
            }
            
            MCHKHEAP

            #ifdef FORCE_REDRAW_FIX
            if (nWait == WAIT_TIMEOUT) { // Если изменений не выставлено - проверим таймаут обновления
                DWORD nCurTick = GetTickCount();
                nDelta = nCurTick - nLastUpdateTick;
                if (nDelta > MIN_FORCEREFRESH_INTERVAL/*nDelayRefresh*/) {
                    DEBUGSTR(L"...FORCE_REDRAW_FIX triggered\n");
                    srv.bNeedFullReload = TRUE;
                    //srv.bForceFullSend = TRUE;
                    nLastUpdateTick = nCurTick; // после рефреша возьмем новый tick
                    nWait = (WAIT_OBJECT_0+1);
                }
            }
            #endif
        }

		if (CheckProcessCount()) {
			TODO("Пока так, а потом можно будет отдельной командой только процессы передать");
			srv.bNeedFullReload = TRUE;
			srv.bForceFullSend = TRUE;
			nWait = (WAIT_OBJECT_0+1);
		}

        if (nWait == (WAIT_OBJECT_0+1) && !srv.bInSyncResize)
		{
            #ifdef _DEBUG
            wchar_t szDbg[128], szRgn[64];
            if (srv.CharChanged.hdr.nSize) {
                wsprintf(szRgn, L"{L:%i, T:%i, R:%i, B:%i}", 
                    srv.CharChanged.hdr.cr1.X, srv.CharChanged.hdr.cr1.Y,
                    srv.CharChanged.hdr.cr2.X, srv.CharChanged.hdr.cr2.Y);
            } else {
                lstrcpyW(szRgn, L"No");
            }
            wsprintf(szDbg, L"...Reloading(Eventual:%i, FullReload:%i, FullSend:%i, Rgn:%s)\n",
                lbEventualChange, srv.bNeedFullReload, srv.bForceFullSend, szRgn);
            DEBUGLOG(szDbg);
            #endif
            MCHKHEAP
            // Посмотреть, есть ли изменения в консоли
            // и если они есть - сбросить интервал таймаута в минимальный
            // Но если после выполнения будет выставлен srv.bRequestPostFullReload - 
            // значит во время чтения произошел ресайз и нужно СРАЗУ запустить второй цикл чтения
            if (ReloadFullConsoleInfo()) {
                DEBUGLOG(L"+++Changes was sent\n");
                if (lbLocalForced)
                    srv.bRequestPostFullReload = TRUE; // По любому чиху в консоли - перечитаем данные еще раз через минимум задержки
                nDelayRefresh = MIN_FORCEREFRESH_INTERVAL;
            } else {
                DEBUGLOG(L"---No changes in console\n");
            }
            // Запомнить, когда в последний раз проверили содержимое консоли
            nLastUpdateTick = GetTickCount();
            MCHKHEAP
            if (srv.bRequestPostFullReload) {
                DEBUGLOG(L"...bRequestPostFullReload was set\n");
                lbEventualChange = TRUE; // были изменения в консоли
                //srv.bRequestPostFullReload = FALSE;
                ResetEvent(srv.hRefreshEvent); // Чтобы консоль успела просраться
            }
            DEBUGLOG(L"\n");
        }
        
        WARNING("Win2k скорее всего события вообще не ходят, так что timeout нужно ставить вообще минимальный (10)");
        // Если Refresh пришел по событию из консоли - сбрасываем задержку в минимальную
        if (lbEventualChange) {
            nDelayRefresh = MIN_FORCEREFRESH_INTERVAL;
        } else {
            if (nDelayRefresh < MAX_FORCEREFRESH_INTERVAL) {
                #ifdef FORCE_REDRAW_FIX
                DEBUGLOG(L"...increasing delay refresh\n");
                #endif
                nDelayRefresh = (DWORD)(nDelayRefresh * 1.3);
            }
        }
        MCHKHEAP
    }
    
    return 0;
}

//Minimum supported client Windows 2000 Professional 
//Minimum supported server Windows 2000 Server 
void WINAPI WinEventProc(HWINEVENTHOOK hWinEventHook, DWORD event, HWND hwnd, LONG idObject, LONG idChild, DWORD dwEventThread, DWORD dwmsEventTime)
{
    if (hwnd != ghConWnd) {
        // если это не наше окно - выходим
        return;
    }

    //BOOL bNeedConAttrBuf = FALSE;
    CESERVER_CHAR ch = {{0,0}};
    #ifdef _DEBUG
    WCHAR szDbg[128];
    #endif

    switch(event)
    {
    case EVENT_CONSOLE_START_APPLICATION:
        //A new console process has started. 
        //The idObject parameter contains the process identifier of the newly created process. 
        //If the application is a 16-bit application, the idChild parameter is CONSOLE_APPLICATION_16BIT and idObject is the process identifier of the NTVDM session associated with the console.

        #ifdef _DEBUG
		#ifndef WIN64
        _ASSERTE(CONSOLE_APPLICATION_16BIT==1);
		#endif
        wsprintfW(szDbg, L"EVENT_CONSOLE_START_APPLICATION(PID=%i%s)\n", idObject, 
			(idChild == CONSOLE_APPLICATION_16BIT) ? L" 16bit" : L"");
        DEBUGSTR(szDbg);
        #endif

        if (((DWORD)idObject) != gnSelfPID) {
			CheckProcessCount(TRUE);
				/*
            EnterCriticalSection(&srv.csProc);
            srv.nProcesses.push_back(idObject);
            LeaveCriticalSection(&srv.csProc);
				*/

			#ifndef WIN64
			_ASSERTE(CONSOLE_APPLICATION_16BIT==1);
            if (idChild == CONSOLE_APPLICATION_16BIT) {
				srv.bNtvdmActive = TRUE;
				srv.nNtvdmPID = idObject;
                SetPriorityClass(GetCurrentProcess(), HIGH_PRIORITY_CLASS);

				// Тут менять высоту уже нельзя... смена размера не доходит до 16бит приложения...
				// Возможные значения высоты - 25/28/50 строк. При старте - обычно 28
				// Ширина - только 80 символов
            }
			#endif
            //
            //HANDLE hIn = CreateFile(L"CONIN$", GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_READ,
            //                  0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
            //if (hIn != INVALID_HANDLE_VALUE) {
            //  HANDLE hOld = ghConIn;
            //  ghConIn = hIn;
            //  SafeCloseHandle(hOld);
            //}
        }
        return; // обновление экрана не требуется

    case EVENT_CONSOLE_END_APPLICATION:
        //A console process has exited. 
        //The idObject parameter contains the process identifier of the terminated process.

        #ifdef _DEBUG
        wsprintfW(szDbg, L"EVENT_CONSOLE_END_APPLICATION(PID=%i%s)\n", idObject, 
			(idChild == CONSOLE_APPLICATION_16BIT) ? L" 16bit" : L"");
        DEBUGSTR(szDbg);
        #endif

        if (((DWORD)idObject) != gnSelfPID) {
			CheckProcessCount(TRUE);

			#ifndef WIN64
			_ASSERTE(CONSOLE_APPLICATION_16BIT==1);
			if (idChild == CONSOLE_APPLICATION_16BIT) {
				//DWORD ntvdmPID = idObject;
				//dwActiveFlags &= ~CES_NTVDM;
				srv.bNtvdmActive = FALSE;
				//TODO: возможно стоит прибить процесс NTVDM?
				SetPriorityClass(GetCurrentProcess(), NORMAL_PRIORITY_CLASS);
			}
			#endif

				/*
            std::vector<DWORD>::iterator iter;
            EnterCriticalSection(&srv.csProc);
            for (iter=srv.nProcesses.begin(); iter!=srv.nProcesses.end(); iter++) {
                if (((DWORD)idObject) == *iter) {
                    srv.nProcesses.erase(iter);
                    if (idChild == CONSOLE_APPLICATION_16BIT) {
                        //DWORD ntvdmPID = idObject;
                        dwActiveFlags &= ~CES_NTVDM;
                        //TODO: возможно стоит прибить процесс NTVDM?
                        SetPriorityClass(GetCurrentProcess(), NORMAL_PRIORITY_CLASS);
                    }
                    // Процессов в консоли не осталось?
                    if (srv.nProcesses.size() == 0 && !gbInRecreateRoot) {
                        LeaveCriticalSection(&srv.csProc);
                        SetEvent(ghFinalizeEvent);
                        return;
                    }
                    break;
                }
            }
            LeaveCriticalSection(&srv.csProc);
				*/
            //
            //HANDLE hIn = CreateFile(L"CONIN$", GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_READ,
            //                  0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
            //if (hIn != INVALID_HANDLE_VALUE) {
            //  HANDLE hOld = ghConIn;
            //  ghConIn = hIn;
            //  SafeCloseHandle(hOld);
            //}
        }
        return; // обновление экрана не требуется

    case EVENT_CONSOLE_UPDATE_REGION: // 0x4002 
        {
        //More than one character has changed.
        //The idObject parameter is a COORD structure that specifies the start of the changed region. 
        //The idChild parameter is a COORD structure that specifies the end of the changed region.
            #ifdef _DEBUG
            COORD crStart, crEnd; 
            //memmove(&crStart, &idObject, sizeof(idObject));
            crStart.X = LOWORD(idObject); crStart.Y = HIWORD(idObject);
            //memmove(&crEnd, &idChild, sizeof(idChild));
            crEnd.X = LOWORD(idChild); crEnd.Y = HIWORD(idChild);
            wsprintfW(szDbg, L"EVENT_CONSOLE_UPDATE_REGION({%i, %i} - {%i, %i})\n", crStart.X,crStart.Y, crEnd.X,crEnd.Y);
            DEBUGSTR(szDbg);
            #endif
            //bNeedConAttrBuf = TRUE;
            //// Перечитать размер, положение курсора, и пр.
            //ReloadConsoleInfo();
            srv.bNeedFullReload = TRUE;
            if (USER_ACTIVITY)
                SetEvent(srv.hRefreshEvent);
        }
        return; // Обновление по событию в нити

    case EVENT_CONSOLE_UPDATE_SCROLL: //0x4004
        {
        //The console has scrolled.
        //The idObject parameter is the horizontal distance the console has scrolled. 
        //The idChild parameter is the vertical distance the console has scrolled.
            #ifdef _DEBUG
            wsprintfW(szDbg, L"EVENT_CONSOLE_UPDATE_SCROLL(X=%i, Y=%i)\n", idObject, idChild);
            DEBUGSTR(szDbg);
            #endif
            //bNeedConAttrBuf = TRUE;
            //// Перечитать размер, положение курсора, и пр.
            //if (!ReloadConsoleInfo())
            //  return;
            srv.bNeedFullReload = TRUE;
            if (USER_ACTIVITY)
                SetEvent(srv.hRefreshEvent);
        }
        return; // Обновление по событию в нити

    case EVENT_CONSOLE_UPDATE_SIMPLE: //0x4003
        {
        //A single character has changed.
        //The idObject parameter is a COORD structure that specifies the character that has changed.
        //Warning! В писании от  микрософта тут ошибка!
        //The idChild parameter specifies the character in the low word and the character attributes in the high word.
            //memmove(&ch.hdr.cr1, &idObject, sizeof(idObject));
            ch.hdr.cr1.X = LOWORD(idObject); ch.hdr.cr1.Y = HIWORD(idObject);
            ch.hdr.cr2 = ch.hdr.cr1;
            ch.data[0] = (WCHAR)LOWORD(idChild); ch.data[1] = HIWORD(idChild);
            #ifdef _DEBUG
            wsprintfW(szDbg, L"EVENT_CONSOLE_UPDATE_SIMPLE({%i, %i} '%c'(\\x%04X) A=%i)\n", ch.hdr.cr1.X,ch.hdr.cr1.Y, ch.data[0], ch.data[0], ch.data[1]);
            DEBUGSTR(szDbg);
            #endif
            
            EnterCriticalSection(&srv.csChar);
            if (srv.CharChanged.hdr.nSize) {
                // Если измененная позиция уже другая - увеличиваем регион
                EnlargeRegion(srv.CharChanged.hdr, ch.hdr.cr1);
            } else {
                srv.CharChanged = ch; // data - ТОЛЬКО информационно: строка будет перечитана целиком
                srv.CharChanged.hdr.nSize = sizeof(CESERVER_CHAR);
                // srv.bNeedFullReload = TRUE; -- пусть попытается отослать только измененный блок
            }
            LeaveCriticalSection(&srv.csChar);
            
            // Дернуть нить (раз по одному символу - наверное дергаем всегда
            SetEvent(srv.hRefreshEvent);
            
            
            //// Перечитать размер, положение курсора, и пр.
            //BOOL lbLayoutChanged = ReloadConsoleInfo(TRUE);
            //
            //SHORT nYCorrected = CorrectTopVisible(ch.crStart.Y);
            //
            //// Если строка не попала с пересылаемый в GUI буфер - просто выйдем
            //if (nYCorrected < 0 || nYCorrected >= min(gcrBufferSize.Y,srv.sbi.dwSize.Y))
            //    return;
            //
            //int nIdx = ch.crStart.X + nYCorrected * srv.sbi.dwSize.X;
            //_ASSERTE(nIdx>=0 && (DWORD)nIdx<srv.nBufCharCount);
            //
            //if (!lbLayoutChanged) { // Если Layout не поменялся
            //    // Если Reload==FALSE, и ch не меняет предыдущего символа/атрибута в srv.psChars/srv.pnAttrs - сразу выйти");
            //    if (srv.psChars[nIdx] == (wchar_t)ch.data[0] && srv.pnAttrs[nIdx] == ch.data[1])
            //        return; // данные не менялись
            //}
            //
            //// Применить
            //srv.psChars[nIdx] = (wchar_t)ch.data[0];
            //// Чтобы не рассчитывать каждый раз сдвиг буфера - он всегда максимальный == srv.nBufCharCount
            //srv.psChars[nIdx+srv.nBufCharCount] = (wchar_t)ch.data[0];
            //srv.pnAttrs[nIdx] = (WORD)ch.data[1];
            //srv.pnAttrs[nIdx+srv.nBufCharCount] = (WORD)ch.data[1];
            //
            //// И отправить
            //ReloadFullConsoleInfo(&ch);
        } return;

    case EVENT_CONSOLE_CARET: //0x4001
        {
        //Warning! WinXPSP3. Это событие проходит ТОЛЬКО если консоль в фокусе. 
        //         А с ConEmu она НИКОГДА не в фокусе, так что курсор не обновляется.
        //The console caret has moved.
        //The idObject parameter is one or more of the following values:
        //      CONSOLE_CARET_SELECTION or CONSOLE_CARET_VISIBLE.
        //The idChild parameter is a COORD structure that specifies the cursor's current position.
            COORD crWhere; 
            //memmove(&crWhere, &idChild, sizeof(idChild));
            crWhere.X = LOWORD(idChild); crWhere.Y = HIWORD(idChild);
            #ifdef _DEBUG
            wsprintfW(szDbg, L"EVENT_CONSOLE_CARET({%i, %i} Sel=%c, Vis=%c\n", crWhere.X,crWhere.Y, 
                ((idObject & CONSOLE_CARET_SELECTION)==CONSOLE_CARET_SELECTION) ? L'Y' : L'N',
                ((idObject & CONSOLE_CARET_VISIBLE)==CONSOLE_CARET_VISIBLE) ? L'Y' : L'N');
            DEBUGSTR(szDbg);
            #endif
            if (srv.sbi.dwCursorPosition.X != crWhere.X || srv.sbi.dwCursorPosition.Y != crWhere.Y
                || srv.ci.bVisible != ((idObject & 2/*CONSOLE_CARET_VISIBLE*/)==2/*CONSOLE_CARET_VISIBLE*/))
            {
                // Шансов получить это событие мало, но
                if (USER_ACTIVITY)
                    SetEvent(srv.hRefreshEvent);
            }
            // Перечитать размер, положение курсора, и пр.
            //if (ReloadConsoleInfo())
            //  return;
        } return;

    case EVENT_CONSOLE_LAYOUT: //0x4005
        {
        //The console layout has changed.
        // Вызывается при изменении размеров или при автоскроллинге при выводе
            #ifdef _DEBUG
            DEBUGSTR(L"EVENT_CONSOLE_LAYOUT\n");
            #endif
            //bNeedConAttrBuf = TRUE;
            //// Перечитать размер, положение курсора, и пр.
            //if (!ReloadConsoleInfo())
            //  return;

			// Смена размера консоли или буфера (telnet, etc.)
			ghConIn.Close();
			ghConOut.Close();

            srv.bNeedFullReload = TRUE;
            if (USER_ACTIVITY)
                SetEvent(srv.hRefreshEvent);
        }
        return; // Обновление по событию в нити
    }


    //// Дальнейшее имеет смысл только если CVirtualConsole уже запустил серверный пайп
    //if (srv.szGuiPipeName[0] == 0)
    //  return;

    //CESERVER_REQ* pOut = 
    //  Create ConsoleInfo (
    //      (event == EVENT_CONSOLE_UPDATE_SIMPLE) ? &ch : NULL,
    //      FALSE/*bNeedConAttrBuf*/
    //  );
    //_ASSERTE(pOut!=NULL);
    //if (!pOut)
    //  return;

    ////Warning! WinXPSP3. EVENT_CONSOLE_CARET проходит ТОЛЬКО если консоль в фокусе. 
    ////         А с ConEmu она НИКОГДА не в фокусе, так что курсор не обновляется.
    //// Т.е. изменилось содержимое консоли - уведомить GUI часть
    ////if (event != EVENT_CONSOLE_CARET) { // Если изменилось только положение курсора - перечитывать содержимое не нужно
    ////    srv.bContentsChanged = TRUE;
    ////}
    ////SetEvent(hGlblUpdateEvt);

    ////WARNING("Все изменения пересылать в GUI через CEGUIPIPENAME");
    ////WARNING("Этот пайп обрабатывается в каждом CVirtualConsole по отдельности");

    //SendConsoleChanges ( pOut );

    //Free ( pOut );
}

// Скорректировать строку nY так, чтобы ее индекс соответствовал буферу, пересылаемому в GUI
// Пересылаемый буфер содержит видимую часть консоли (или видимую в GUI, если автопрокрутка 'заблокирована')
SHORT CorrectTopVisible(int nY)
{
    int nYCorrected = nY;
    if (srv.nTopVisibleLine != -1) {
        // верхняя строка в GUI зафиксирована
        nYCorrected = nY - srv.nTopVisibleLine;
    } else if (srv.sbi.dwSize.Y <= gcrBufferSize.Y) {
        // Если текущая высота буфера не больше используемой в GUI - строки не отнимаем
        nYCorrected = nY;
    } else {
        // Нужно из Y вычесть (0-based) индекс верхней видимой строки
        nYCorrected = nY - srv.sbi.srWindow.Top;
    }
    return nYCorrected;
}

void SendConsoleChanges(CESERVER_REQ* pOut)
{
    //srv.szGuiPipeName инициализируется только после того, как GUI узнал хэндл консольного окна
    if (srv.szGuiPipeName[0] == 0)
        return;

    HANDLE hPipe = NULL;
    //DWORD dwErr = 0; //, dwMode = 0;
    BOOL fSuccess = FALSE;
    wchar_t szErr[MAX_PATH*2];
    
    hPipe = ExecuteOpenPipe(srv.szGuiPipeName, szErr, L"ConEmuC");
    if (!hPipe || hPipe == INVALID_HANDLE_VALUE) {
		#ifdef _DEBUG
		SetConsoleTitle(szErr);
		#else
		srv.bForceFullSend = TRUE;
		srv.bRequestPostFullReload = TRUE;
		#endif
        return;
    }

	// Вдруг мы как раз попали в момент закрытия консоли?
	if (WaitForSingleObject(ghExitEvent,0) == WAIT_OBJECT_0) {
		SafeCloseHandle(hPipe);
		return;
	}


    // Собственно запись в пайп
    DWORD dwWritten = 0;
    fSuccess = WriteFile ( hPipe, pOut, pOut->hdr.nSize, &dwWritten, NULL);
    if (!fSuccess || dwWritten != pOut->hdr.nSize) {
        // GUI мог быть закрыт!
        if (ghConEmuWnd && IsWindow(ghConEmuWnd)) {
            // Окно разрушиться не успело, и все равно вывалился assert
            //_ASSERTE(fSuccess && dwWritten == pOut->hdr.nSize);
        } else {
            ghConEmuWnd = NULL;
            SetConEmuEnvVar(NULL);
            TODO("Какие-то действия... показать консольное окно, или еще что-то");
        }
    }
    
    SafeCloseHandle(hPipe);
}

CESERVER_REQ* CreateConsoleInfo(CESERVER_CHAR* pRgnOnly, BOOL bCharAttrBuff)
{
    // ИД пакета формируем сразу, чтобы не перемешались
    DWORD nPacketId = ++srv.nLastPacketID;

    CESERVER_REQ* pOut = NULL;
    DWORD dwAllSize = sizeof(CESERVER_REQ_HDR);
    DWORD nSize; // temp, для определения размера добавляемой переменной
    #ifdef _DEBUG
    BOOL  lbDataSent = FALSE;
    #endif

    dwAllSize += sizeof(CESERVER_REQ_CONINFO_HDR);

    // 9 -- Если pRgnOnly->hdr.nSize == 0 - значит данные в консоли не менялись
    dwAllSize += sizeof(DWORD) + (pRgnOnly ? pRgnOnly->hdr.nSize : 0);
    // 10
    DWORD OneBufferSize = 0; //srv.nOneBufferSize; -- если реально ничего не передаем! - 0
    dwAllSize += sizeof(DWORD);
    // Одновременная передача полного дампа и измененного прямоугольника не нужна
    if (pRgnOnly == NULL && bCharAttrBuff) {
        OneBufferSize = srv.nOneBufferSize;
        TODO("Доработать для передачи только изменившегося прямоугольника и BufferHeight");
        //if (bCharAttrBuff == 2 && OneBufferSize == (srv.nBufCharCount*2)) {
        //    _ASSERTE(srv.nBufCharCount>0);
        //    OneBufferSize = srv.nBufCharCount*2;
        //} else {
        WARNING("ReadConsoleData уже должен быть выполнен!");
        //OneBufferSize = ReadConsoleData(); // returns size in bytes of ONE buffer
        //}
        if (OneBufferSize > (200*100*2)) {
            //_ASSERTE(OneBufferSize && OneBufferSize<=(200*100*2));
        }
        #ifdef _DEBUG
        if (gnBufferHeight == 0) {
            if (OneBufferSize != (srv.sbi.dwSize.X*srv.sbi.dwSize.Y*2)) {
                // Это может случиться во время ресайза
                srv.bRequestPostFullReload = TRUE;
                OneBufferSize = OneBufferSize;
            }
            //_ASSERTE(OneBufferSize == (srv.sbi.dwSize.X*srv.sbi.dwSize.Y*2));
        }
        #endif
        if (OneBufferSize) {
            dwAllSize += OneBufferSize * 2;
        }
    }

    // Выделение буфера // Размер заголовка уже учтен!
    pOut = ExecuteNewCmd(0,dwAllSize); // размер считали в байтах
    _ASSERT(pOut!=NULL);
    if (pOut == NULL) {
        return FALSE;
    }

    // инициализация
    pOut->hdr.nCmd = bCharAttrBuff ? CECMD_GETFULLINFO : CECMD_GETSHORTINFO;

    // поехали

    // 1
    pOut->ConInfo.inf.hConWnd = GetConsoleWindow();

    // 2
    pOut->ConInfo.inf.nPacketId = nPacketId;
	pOut->ConInfo.inf.nInputTID = srv.dwInputThreadId;

    // 3
    // Если есть возможность (WinXP+) - получим реальный список процессов из консоли
	CheckProcessCount();
	GetProcessCount(pOut->ConInfo.inf.nProcesses, countof(pOut->ConInfo.inf.nProcesses));
	_ASSERTE(pOut->ConInfo.inf.nProcesses[0]);

    // 4
    nSize = sizeof(srv.ci);
    pOut->ConInfo.inf.dwCiSize = (srv.dwCiRc == 0) ? nSize : 0;
    if (srv.dwCiRc == 0) {
        //memmove(&pOut->ConInfo.inf.ci, &srv.ci, nSize);
        pOut->ConInfo.inf.ci = srv.ci;
    }

    // 5
    pOut->ConInfo.inf.dwConsoleCP = srv.dwConsoleCP;
    // 6
    pOut->ConInfo.inf.dwConsoleOutputCP = srv.dwConsoleOutputCP;
    // 7
    pOut->ConInfo.inf.dwConsoleMode = srv.dwConsoleMode;

    // 8
    nSize=sizeof(srv.sbi);
    pOut->ConInfo.inf.dwSbiSize = (srv.dwSbiRc == 0) ? nSize : 0;
    if (srv.dwSbiRc == 0) {
        //memmove(&pOut->ConInfo.inf.sbi, &srv.sbi, nSize);
        pOut->ConInfo.inf.sbi = srv.sbi;
    }

    // 9
    pOut->ConInfo.dwRgnInfoSize = pRgnOnly ? pRgnOnly->hdr.nSize : 0;
    if (pRgnOnly && pRgnOnly->hdr.nSize != 0) {
        #ifdef _DEBUG
        wchar_t szDbg[128];
        wsprintf(szDbg, L"+++Sending region {L:%i, T:%i, R:%i, B:%i}, DataSize: %i bytes\n", 
                    pRgnOnly->hdr.cr1.X, pRgnOnly->hdr.cr1.Y,
                    pRgnOnly->hdr.cr2.X, pRgnOnly->hdr.cr2.Y,
                    pRgnOnly->hdr.nSize);
        DEBUGLOG(szDbg);
        lbDataSent = TRUE;
        #endif
        memmove(&pOut->ConInfo.RgnInfo.RgnInfo, pRgnOnly, pRgnOnly->hdr.nSize);
    } else {
        // 10 - здесь будет 0, если текст в консоли не менялся
        pOut->ConInfo.FullData.dwOneBufferSize = OneBufferSize;
        if (OneBufferSize && OneBufferSize!=(DWORD)-1) { // OneBufferSize==0, если pRgnOnly!=0
            LPBYTE lpCur = (LPBYTE)(pOut->ConInfo.FullData.Data);
            #ifdef _DEBUG
            DEBUGLOG(L"---Sending full console data\n");
            TODO("Во время ресайза консоль может подглючивать - отдает не то что нужно...");
            //_ASSERTE(*srv.psChars!=9553);
            lbDataSent = TRUE;
            #endif
            memmove(lpCur, srv.psChars, OneBufferSize); lpCur += OneBufferSize;
            memmove(lpCur, srv.pnAttrs, OneBufferSize); lpCur += OneBufferSize;
        }
    }

    #ifdef _DEBUG
    if (!lbDataSent) {
        DEBUGLOG(L"---Sending only console information\n");
    }
    #endif
    
    if (ghLogSize) {
        static COORD scr_Last = {-1,-1};
        if (scr_Last.X != srv.sbi.dwSize.X || scr_Last.Y != srv.sbi.dwSize.Y) {
            LogSize(&srv.sbi.dwSize, ":CreateConsoleInfo(query only)");
            scr_Last = srv.sbi.dwSize;
        }
    }

    return pOut;
}

// pChangedRgn может прийти, если мы знаем, что какие-то символы точно менялись
// в этом случае pChangedRgn может быть увеличен
BOOL ReloadConsoleInfo(CESERVER_CHAR* pChangedRgn/*=NULL*/)
{
    BOOL lbChanged = FALSE;
    //CONSOLE_SELECTION_INFO lsel = {0}; // GetConsoleSelectionInfo
    CONSOLE_CURSOR_INFO lci = {0}; // GetConsoleCursorInfo
    DWORD ldwConsoleCP=0, ldwConsoleOutputCP=0, ldwConsoleMode=0;
    CONSOLE_SCREEN_BUFFER_INFO lsbi = {{0,0}}; // MyGetConsoleScreenBufferInfo

    MCHKHEAP

    if (!GetConsoleCursorInfo(ghConOut, &lci)) { srv.dwCiRc = GetLastError(); if (!srv.dwCiRc) srv.dwCiRc = -1; } else {
        srv.dwCiRc = 0;
        if (memcmp(&srv.ci, &lci, sizeof(srv.ci))) {
            srv.ci = lci;
            lbChanged = TRUE;
        }
    }

    ldwConsoleCP = GetConsoleCP(); if (srv.dwConsoleCP!=ldwConsoleCP) { srv.dwConsoleCP = ldwConsoleCP; lbChanged = TRUE; }
    ldwConsoleOutputCP = GetConsoleOutputCP(); if (srv.dwConsoleOutputCP!=ldwConsoleOutputCP) { srv.dwConsoleOutputCP = ldwConsoleOutputCP; lbChanged = TRUE; }
    ldwConsoleMode=0; GetConsoleMode(ghConIn, &ldwConsoleMode); if (srv.dwConsoleMode!=ldwConsoleMode) { srv.dwConsoleMode = ldwConsoleMode; lbChanged = TRUE; }

    MCHKHEAP

    if (!MyGetConsoleScreenBufferInfo(ghConOut, &lsbi)) { srv.dwSbiRc = GetLastError(); if (!srv.dwSbiRc) srv.dwSbiRc = -1; } else {
		// Консольное приложение могло изменить размер буфера
		if (!NTVDMACTIVE
			&& (gcrBufferSize.Y != lsbi.dwSize.Y || gnBufferHeight != 0)
			&& (lsbi.srWindow.Top == 0 && lsbi.dwSize.Y == (lsbi.srWindow.Bottom - lsbi.srWindow.Top + 1)))
		{
			// Это значит, что прокрутки нет, и консольное приложение изменило размер буфера
			gnBufferHeight = 0; gcrBufferSize = lsbi.dwSize;
		}

        srv.dwSbiRc = 0;
        if (memcmp(&srv.sbi, &lsbi, sizeof(srv.sbi))) {
            // Изменения в координатах / размерах есть (скопируем данные ниже. еще строки с курсором проверить нужно)
            
            // Изменился размер буфера. Перечитываем все без вариантов
            if (srv.sbi.dwSize.X != lsbi.dwSize.X || srv.sbi.dwSize.Y != lsbi.dwSize.Y) {
                srv.bForceFullSend = TRUE; // перечитаем и ОТОШЛЕМ все
            } else
            
            // Если сменилась верхняя строка (прокрутка) читаем все, пока без вариантов
            TODO("Для оптимизации можно было бы попробовать скроллировать ранее считанный буфер, но пока так");
            if (srv.sbi.srWindow.Top != lsbi.srWindow.Top) {
                srv.bForceFullSend = TRUE; // перечитаем и ОТОШЛЕМ все
            } else
            
            // При смене позиции курсора...
            if (srv.psChars && srv.pnAttrs /*&& !abSkipCursorCharCheck*/
                && !(srv.bForceFullSend || srv.bNeedFullReload) // Если данные и так будут перечитаны целиком - не дергаться
                && memcmp(&srv.sbi.dwCursorPosition, &lsbi.dwCursorPosition, sizeof(lsbi.dwCursorPosition))
                )
            {
                // В некоторых случаях не срабатывает ни EVENT_CONSOLE_UPDATE_SIMPLE ни EVENT_CONSOLE_UPDATE_REGION
                // Пример. Запускаем cmd.exe. печатаем какую-то муйню в командной строке и нажимаем 'Esc'
                // При Esc никаких событий ВООБЩЕ не дергается, а экран в консоли изменился!

                // Вобщем, если есть изменения в символах/атрибутах строки на которой БЫЛ курсор,
                // или на которую курсор помещен - перечитать консоль полностью - bForceFullSend=TRUE
                int nCount = min(lsbi.dwSize.X, (int)srv.nLineCmpSize);
                if (nCount && srv.ptrLineCmp) {
                    MCHKHEAP
                    DWORD nbActuallyRead = 0;
                    COORD coord = {0,0};
                    DWORD nBufferShift = 0;
                    for (int i=0; i<=1; i++) {
                        if (i==0) {
                            // Строка на которой БЫЛ курсор
                            coord.Y = srv.sbi.dwCursorPosition.Y;
                            // С учетом первой видимой строки...
                            nBufferShift = coord.Y - srv.sbi.srWindow.Top;
                        } else {
                            // Строка на которую встал курсор
                            if (coord.Y == lsbi.dwCursorPosition.Y) break; // строка не менялась, только позиция
                            coord.Y = lsbi.dwCursorPosition.Y;
                            // С учетом первой видимой строки...
                            nBufferShift = coord.Y - srv.sbi.srWindow.Top;
                        }
                        // размерность
                        if (nBufferShift < 0 || nBufferShift >= (USHORT)gcrBufferSize.Y)
                            continue; // вышел из видимой в GUI области
                        nBufferShift = nBufferShift*gcrBufferSize.X;

                        MCHKHEAP

                        if (ReadConsoleOutputAttribute(ghConOut, srv.ptrLineCmp, nCount, coord, &nbActuallyRead)) {
                            if (memcmp(srv.ptrLineCmp, srv.pnAttrs+nBufferShift, nbActuallyRead*2)) {
                                //srv.bForceFullSend = TRUE; break;
                                if (pChangedRgn) {
                                    EnlargeRegion(pChangedRgn->hdr, coord);
                                    coord.X = gcrBufferSize.X -1;
                                    EnlargeRegion(pChangedRgn->hdr, coord);
                                }
                            }
                        }

                        MCHKHEAP

                        coord.X = 0;
                        if (ReadConsoleOutputCharacter(ghConOut, (wchar_t*)srv.ptrLineCmp, nCount, coord, &nbActuallyRead)) {
                            if (memcmp(srv.ptrLineCmp, srv.psChars+nBufferShift, nbActuallyRead*2)) {
                                //srv.bForceFullSend = TRUE; break;
                                if (pChangedRgn) {
                                    EnlargeRegion(pChangedRgn->hdr, coord);
                                    coord.X = gcrBufferSize.X -1;
                                    EnlargeRegion(pChangedRgn->hdr, coord);
                                }
                            }
                        }
                    }
                }
            }
            if ((lsbi.srWindow.Bottom - lsbi.srWindow.Top)>lsbi.dwMaximumWindowSize.Y) {
                _ASSERTE((lsbi.srWindow.Bottom - lsbi.srWindow.Top)<lsbi.dwMaximumWindowSize.Y);
            }

            MCHKHEAP
            
            // Изменения в координатах / размерах есть, запоминаем их
            srv.sbi = lsbi;
            // Поправить GUI'шные nTopVisibleLine, nVisibleHeight, и пр., пока вреда не случилось
            if (gnBufferHeight == 0) {
                _ASSERTE(srv.sbi.dwSize.Y<=200);
                // В режиме без прокрутки - видимая часть должна быть такой!
                srv.nVisibleHeight = srv.sbi.dwSize.Y;
                srv.nTopVisibleLine = -1; // Да и верхняя видимая линия должна быть первой
            } else {
                // С прокруткой сложнее...
                if ((lsbi.srWindow.Bottom - lsbi.srWindow.Top + 1) >= MIN_CON_HEIGHT) {
                    srv.nVisibleHeight = (lsbi.srWindow.Bottom - lsbi.srWindow.Top + 1);
                }
                if (srv.nTopVisibleLine != -1) {
                    if ((srv.nTopVisibleLine + srv.nVisibleHeight) > srv.sbi.dwSize.Y)
                        srv.nTopVisibleLine = max(0, (srv.sbi.dwSize.Y - srv.nVisibleHeight));
                    if ((srv.nTopVisibleLine + srv.nVisibleHeight) > srv.sbi.dwSize.Y)
                        srv.nVisibleHeight = srv.sbi.dwSize.Y - srv.nTopVisibleLine;
                }
            }
            lbChanged = TRUE;
        }
    }

    return lbChanged;
}

// Действует аналогично функции WinApi (GetConsoleScreenBufferInfo), но в режиме сервера:
// 1. запрещает (то есть отменяет) максимизацию консольного окна
// 2. корректирует srWindow: сбрасывает горизонтальную прокрутку,
//    а если не обнаружен "буферный режим" - то и вертикальную.
BOOL MyGetConsoleScreenBufferInfo(HANDLE ahConOut, PCONSOLE_SCREEN_BUFFER_INFO apsc)
{
    BOOL lbRc = FALSE;

    //CSection cs(NULL,NULL);
    MSectionLock CSCS;
    if (gnRunMode == RM_SERVER)
    	CSCS.Lock(&srv.cChangeSize);
        //cs.Enter(&srv.csChangeSize, &srv.ncsTChangeSize);

    if (gnRunMode == RM_SERVER) // ComSpec окно менять НЕ ДОЛЖЕН!
    {
        // Если юзер случайно нажал максимизацию, когда консольное окно видимо - ничего хорошего не будет
        if (IsZoomed(ghConWnd)) {
            SendMessage(ghConWnd, WM_SYSCOMMAND, SC_RESTORE, 0);
            Sleep(200);
            //lbRc = GetConsoleScreenBufferInfo(ahConOut, apsc);
            //SetConsoleSize(gnBufferHeight, gcrBufferSize, srv.sbi.srWindow);
            // Если этого не сделать - размер консоли нельзя УМЕНЬШИТЬ
            RECT rcConPos;
            GetWindowRect(ghConWnd, &rcConPos);
            MoveWindow(ghConWnd, rcConPos.left, rcConPos.top, 1, 1, 1);
            if (gnBufferHeight == 0)
            {
                //specified width and height cannot be less than the width and height of the console screen buffer's window
                lbRc = SetConsoleScreenBufferSize(ghConOut, gcrBufferSize);
            } else {
                // Начался ресайз для BufferHeight
                COORD crHeight = {gcrBufferSize.X, gnBufferHeight};
                MoveWindow(ghConWnd, rcConPos.left, rcConPos.top, 1, 1, 1);
                lbRc = SetConsoleScreenBufferSize(ghConOut, crHeight); // а не crNewSize - там "оконные" размеры
            }
            SetConsoleWindowInfo(ghConOut, TRUE, &srv.sbi.srWindow);
        }
    }

    lbRc = GetConsoleScreenBufferInfo(ahConOut, apsc);
    _ASSERTE((apsc->srWindow.Bottom-apsc->srWindow.Top)<150);
    if (lbRc && gnRunMode == RM_SERVER) // ComSpec окно менять НЕ ДОЛЖЕН!
    {
                // Перенесено в SetConsoleSize
                //     if (gnBufferHeight) {
                //// Если мы знаем о режиме BufferHeight - можно подкорректировать размер (зачем это было сделано?)
                //         if (gnBufferHeight <= (apsc->dwMaximumWindowSize.Y * 1.2))
                //             gnBufferHeight = max(300, (SHORT)(apsc->dwMaximumWindowSize.Y * 1.2));
                //     }

        // Если прокрутки быть не должно - по возможности уберем ее, иначе при запуске FAR
        // запустится только в ВИДИМОЙ области
        BOOL lbNeedCorrect = FALSE;
        if (apsc->srWindow.Left > 0) {
            lbNeedCorrect = TRUE; apsc->srWindow.Left = 0;
        }
        if ((apsc->srWindow.Right+1) < apsc->dwSize.X) {
            lbNeedCorrect = TRUE; apsc->srWindow.Right = (apsc->dwSize.X - 1);
        }
        BOOL lbBufferHeight = FALSE;
        if (apsc->dwSize.Y >= (apsc->dwMaximumWindowSize.Y * 1.2))
            lbBufferHeight = TRUE;

        if (!lbBufferHeight) {
            _ASSERTE((apsc->srWindow.Bottom-apsc->srWindow.Top)<200);

            if (apsc->srWindow.Top > 0) {
                lbNeedCorrect = TRUE; apsc->srWindow.Top = 0;
            }
            if ((apsc->srWindow.Bottom+1) < apsc->dwSize.Y) {
                lbNeedCorrect = TRUE; apsc->srWindow.Bottom = (apsc->dwSize.Y - 1);
            }
        }
        if (lbNeedCorrect) {
            lbRc = SetConsoleWindowInfo(ghConOut, TRUE, &apsc->srWindow);
            lbRc = GetConsoleScreenBufferInfo(ahConOut, apsc);
        }
        CorrectVisibleRect(apsc);
    }

    //cs.Leave();
    CSCS.Unlock();

    #ifdef _DEBUG
    if ((apsc->srWindow.Bottom - apsc->srWindow.Top)>apsc->dwMaximumWindowSize.Y) {
        _ASSERTE((apsc->srWindow.Bottom - apsc->srWindow.Top)<apsc->dwMaximumWindowSize.Y);
    }
    #endif

    return lbRc;
}

void CorrectVisibleRect(CONSOLE_SCREEN_BUFFER_INFO* pSbi)
{
    _ASSERTE(gcrBufferSize.Y<200);

    // Игнорируем горизонтальный скроллинг
    pSbi->srWindow.Left = 0; pSbi->srWindow.Right = pSbi->dwSize.X - 1;
    if (gnBufferHeight == 0) {
        // Сервер мог еще не успеть среагировать на изменение режима BufferHeight
        if (pSbi->dwMaximumWindowSize.Y < pSbi->dwSize.Y)
            gnBufferHeight = pSbi->dwSize.Y; // Это однозначно буферный режим
    }
    // Игнорируем вертикальный скроллинг для обычного режима
    if (gnBufferHeight == 0) {
        pSbi->srWindow.Top = 0; pSbi->srWindow.Bottom = pSbi->dwSize.Y - 1;
    } else if (srv.nTopVisibleLine!=-1) {
        // А для 'буферного' режима позиция может быть заблокирована
        pSbi->srWindow.Top = srv.nTopVisibleLine;
        pSbi->srWindow.Bottom = min( (pSbi->dwSize.Y-1), (srv.nTopVisibleLine+gcrBufferSize.Y-1) );
    } else {
        // Просто корректируем нижнюю строку по отображаемому в GUI региону
        TODO("Вообще-то хорошо бы эту коррекцию сделать так, чтобы курсор был видим");
        pSbi->srWindow.Bottom = min( (pSbi->dwSize.Y-1), (pSbi->srWindow.Top+gcrBufferSize.Y-1) );
    }

    #ifdef _DEBUG
    if ((pSbi->srWindow.Bottom - pSbi->srWindow.Top)>pSbi->dwMaximumWindowSize.Y) {
        _ASSERTE((pSbi->srWindow.Bottom - pSbi->srWindow.Top)<pSbi->dwMaximumWindowSize.Y);
    }
    #endif
}

BOOL ReloadFullConsoleInfo(/*CESERVER_CHAR* pCharOnly/ *=NULL*/)
{
    #ifdef _DEBUG
    DWORD dwCurThId = GetCurrentThreadId();
    // Должен вызываться ТОЛЬКО в главной нити (RefreshThread)
    _ASSERTE(dwCurThId == srv.dwRefreshThread);
    #endif

    BOOL lbChangesWasSent = FALSE;
    CESERVER_CHAR* pCheck = NULL;
    BOOL lbInfChanged = FALSE, lbDataChanged = FALSE;
    //DWORD dwBufSize = 0;
    BOOL lbCharChangedSet = FALSE;
    BOOL bForceFullSend = FALSE;
    CESERVER_CHAR lCharChanged = {0}; // CRITICAL_SECTION csChar;

    MCHKHEAP
    
    EnterCriticalSection(&srv.csChar);
    if (srv.bNeedFullReload || srv.bForceFullSend) {
        // Все равно будет перечинано все целиком
        //srv.bCharChangedSet = FALSE;
        memset(&srv.CharChanged, 0, sizeof(srv.CharChanged));
    } else {
        lbCharChangedSet = srv.CharChanged.hdr.nSize != 0;
        if (lbCharChangedSet) {
            lCharChanged = srv.CharChanged;
            //srv.bCharChangedSet = FALSE;
            memset(&srv.CharChanged, 0, sizeof(srv.CharChanged));
            pCheck = &lCharChanged;
        }
    }
    LeaveCriticalSection(&srv.csChar);

    MCHKHEAP


    // Заполняет актуальными данными: srv.sbi, srv.ci, srv.dwConsoleMode, srv.dwConsoleCP, srv.dwConsoleOutputCP
    // Функция может установить в TRUE srv.bForceFullSend.
    MCHKHEAP
    lbInfChanged = ReloadConsoleInfo(srv.bForceFullSend ? NULL : &lCharChanged); // lCharChanged может быть еще пустым
    #ifdef _DEBUG
        if (lCharChanged.hdr.nSize) {
            _ASSERTE(lCharChanged.hdr.cr1.Y>=0 && lCharChanged.hdr.cr1.Y<srv.sbi.dwSize.Y);
            _ASSERTE(lCharChanged.hdr.cr2.Y>=0 && lCharChanged.hdr.cr2.Y<srv.sbi.dwSize.Y);
        }
    #endif
    MCHKHEAP
    if (srv.bForceFullSend)
        pCheck = NULL; // Потребуется полное перечитывание содержимого консоли
    else if (!pCheck && lCharChanged.hdr.nSize)
        pCheck = &lCharChanged; // Значит ReloadConsoleInfo определила, что можно передать только измененную часть

    MCHKHEAP

    // srv.bNeedFullReload  - выставляется при старте или по событиям из консоли
    // srv.bForceFullSend - выставляет ReloadConsoleInfo
    if (srv.bNeedFullReload || srv.bForceFullSend || pCheck) {
        
        if (pCheck == NULL && !srv.bForceFullSend) { // Все равно попытаемся определить измененную область
            memset(&lCharChanged, 0, sizeof(lCharChanged));
            lCharChanged.hdr.cr1.Y = (srv.nTopVisibleLine == -1) ? srv.sbi.srWindow.Top : srv.nTopVisibleLine;
            lCharChanged.hdr.cr2.X = srv.sbi.dwSize.X - 1;
            lCharChanged.hdr.cr2.Y = lCharChanged.hdr.cr1.Y + srv.nVisibleHeight - 1;
            lCharChanged.hdr.nSize = sizeof(lCharChanged.hdr);
            // Видимая область из GUI могла еще не обновиться

            #ifdef _DEBUG
            if (lCharChanged.hdr.nSize) {
                _ASSERTE(lCharChanged.hdr.cr1.Y>=0 && lCharChanged.hdr.cr1.Y<srv.sbi.dwSize.Y);
                _ASSERTE(lCharChanged.hdr.cr2.Y>=0 && lCharChanged.hdr.cr2.Y<srv.sbi.dwSize.Y);
            }
            #endif
        }
        MCHKHEAP
        _ASSERTE(pCheck==NULL || pCheck==&lCharChanged);
        MCHKHEAP
        lbDataChanged = ReadConsoleData(srv.bForceFullSend ? NULL : &lCharChanged);
        MCHKHEAP
        #ifdef _DEBUG
            if (lCharChanged.hdr.nSize) {
                _ASSERTE(lCharChanged.hdr.cr1.Y>=0 && lCharChanged.hdr.cr1.Y<srv.sbi.dwSize.Y);
                _ASSERTE(lCharChanged.hdr.cr2.Y>=0 && lCharChanged.hdr.cr2.Y<srv.sbi.dwSize.Y);
            }
        #endif

        // Сброс переменных делаем ПОСЛЕ ReadConsoleData
        if (srv.bForceFullSend) {
            bForceFullSend = TRUE;
            pCheck = NULL; // не маяться с прямоугольником
        } else if (lbDataChanged && !pCheck) {
            pCheck = &lCharChanged; // Если не требуется отсылка полного прямоугольника - пошлем только измененные данные
        }
        srv.bNeedFullReload = FALSE;
        srv.bForceFullSend = FALSE;

        
        //if (lbDataChanged && pCheck != NULL) {
        //    pCharOnly = pCheck;
        //    lbDataChanged = FALSE; // Изменения передадутся через pCharOnly
        //} else 
        if (!lbDataChanged && (bForceFullSend || pCheck))
        {
            // его тоже сразу сбрасывать, но принудительно выставлять lbDataChanged
            // это потому, что по какой-то причине ИНОГДА в буфер попадают некорректные данные... где-то баг, пока не нашел
            lbDataChanged = TRUE;
            //DEBUGSTR(L"!!! Forced full console data send\n");
        }
    }

    if (lbInfChanged || lbDataChanged || pCheck) {
        //TODO("Можно отслеживать реально изменившийся прямоугольник и передавать только его");
        
        // Это из RefreshThread
        //CESERVER_REQ* pOut = CreateConsoleInfo(lbDataChanged ? NULL : pCharOnly, lbDataChanged ? 2 : 0); -- 2009.06.05 было так
        CESERVER_CHAR *pChangedBuffer = NULL;
        #ifdef RELATIVE_TRANSMIT_DISABLE
        bForceFullSend = TRUE;
        #endif
        // Если изменений в видимой области тут нет - (pCheck->hdr.nSize == 0)
        if (!bForceFullSend && pCheck && pCheck->hdr.nSize) {
            // Нужно сформировать pChangedBuffer по прямоугольнику из pCheck, если не bForceFullSend !!!
            // Иначе мы на каждую измененную букву будем гонять по пайпу полный дамп консоли
            //bForceFullSend = TRUE;
            MCHKHEAP
            _ASSERTE(pCheck->hdr.cr1.X<=pCheck->hdr.cr2.X && pCheck->hdr.cr1.Y<=pCheck->hdr.cr2.Y);
            // Координата Y уже скорректирована на видимую область
            int nSize = sizeof(CESERVER_CHAR_HDR) // Заголовок + количество строк * ширину * (символ + атрибут)
                + (pCheck->hdr.cr2.Y - pCheck->hdr.cr1.Y + 1)
                * (pCheck->hdr.cr2.X - pCheck->hdr.cr1.X + 1)
                * (sizeof(WORD)+sizeof(wchar_t));
            if (nSize > (int)srv.nChangedBufferSize) {
                Free(srv.pChangedBuffer);
                srv.pChangedBuffer = (CESERVER_CHAR*)Alloc(nSize, 1);
                srv.nChangedBufferSize = (srv.pChangedBuffer != NULL) ? nSize : 0;
            }
            pChangedBuffer = srv.pChangedBuffer;
            if (pChangedBuffer) {
                pChangedBuffer->hdr.nSize = nSize;
                // Здесь у нас АБСОЛЮТНЫЕ а не видимые координаты, но попадающие только в ВИДИМУЮ область
                pChangedBuffer->hdr.cr1 = pCheck->hdr.cr1;
                pChangedBuffer->hdr.cr2 = pCheck->hdr.cr2;

                // Готовим относительные координаты
                int nTop = (srv.nTopVisibleLine == -1) ? srv.sbi.srWindow.Top : srv.nTopVisibleLine;
                int nRelativeY1 = pChangedBuffer->hdr.cr1.Y - nTop;
                int nRelativeY2 = pChangedBuffer->hdr.cr2.Y - nTop;
                _ASSERTE(nRelativeY1>=0 && nRelativeY2>=nRelativeY1 && nRelativeY2<(nTop+srv.nVisibleHeight));
                    
                // Нужно заполнить pChangedBuffer->data
                int nLineLen = pChangedBuffer->hdr.cr2.X - pChangedBuffer->hdr.cr1.X + 1;
                _ASSERTE(nLineLen>0);
                int nLineSize = nLineLen * 2;
                int nLineCount = pChangedBuffer->hdr.cr2.Y - pChangedBuffer->hdr.cr1.Y + 1;
                _ASSERTE(nLineCount>0);
                _ASSERTE((nLineSize*nLineCount*2+sizeof(CESERVER_CHAR_HDR))==nSize);
                // Указатели на отсылаемые данные
                wchar_t* pszSend = (wchar_t*)(pChangedBuffer->data);
                WORD*    pnSend  = ((WORD*)pChangedBuffer->data)+(nLineLen*nLineCount);
                // Указатели на считанные из консоли данные
                wchar_t* pszCon = srv.psChars + pChangedBuffer->hdr.cr1.X + nRelativeY1 * srv.sbi.dwSize.X;
                WORD* pnCon = srv.pnAttrs + pChangedBuffer->hdr.cr1.X + nRelativeY1 * srv.sbi.dwSize.X;
                MCHKHEAP
                // ширина консоли
                int nConLineLen = srv.sbi.dwSize.X;
                //int nConLineSize = nConLineLen * 2; // wchar_t / WORD
                // Побежали по строкам (тут координата уже не играет - работаем с указателями)
                for (int y = nRelativeY1; y <= nRelativeY2; y++) {
                    #ifdef _DEBUG
                        _ASSERTE(!IsBadWritePtr(pszSend, nLineSize));
                        _ASSERTE(!IsBadWritePtr(pnSend, nLineSize));
                        _ASSERTE(!IsBadReadPtr(pszCon, nLineSize));
                        _ASSERTE(!IsBadReadPtr(pnCon, nLineSize));
                    #endif
                    memmove(pszSend, pszCon, nLineSize);
                        pszSend += nLineLen; pszCon += nConLineLen;
                    MCHKHEAP
                    memmove(pnSend, pnCon, nLineSize);
                        pnSend += nLineLen; pnCon += nConLineLen;
                    MCHKHEAP
                }

                // Это если захочется данные заново с консоли считать
                //COORD    coord = pChangedBuffer->hdr.cr1;
                //DWORD    nbActuallyRead = 0;
                //for (coord.Y = pChangedBuffer->hdr.cr1.Y; coord.Y <= pChangedBuffer->hdr.cr2.Y; coord.Y++) {
                //  ReadConsoleOutputCharacter(ghConOut, pszLine, nLineLen, coord, &nbActuallyRead); 
                //      pszLine += nLineLen;
                //  ReadConsoleOutputAttribute(ghConOut, pnLine, nLineLen, coord, &nbActuallyRead);
                //      pnLine += nLineLen;
                //}
            }
        } else {
            pCheck = NULL;
        }

        MCHKHEAP
        
        #ifdef RELATIVE_TRANSMIT_DISABLE
        bForceFullSend = TRUE;
        // память для pChangedBuffer не освобождать, а использовать повторно
        if (pChangedBuffer) 
            pChangedBuffer = NULL;
        #endif
        
        // Одновременно обе переменные выставлены быть не должны
        _ASSERTE((pChangedBuffer!=NULL)!=bForceFullSend || !bForceFullSend);
        // CreateConsoleInfo не выполняет чтение или копирование данных В pChangedBuffer
        // Она только выделяет память для отсылки и копирует в нее то что уже есть
        CESERVER_REQ* pOut = CreateConsoleInfo(pChangedBuffer, bForceFullSend);

        if (pChangedBuffer) {
            if (pChangedBuffer->hdr.nSize>((int)sizeof(CESERVER_CHAR_HDR)+4)) {
                srv.bRequestPostFullReload = TRUE; // Если изменилось больше одной буквы - перечитаем данные еще раз через минимум задержки
            }
        }

        MCHKHEAP

        // память для pChangedBuffer не освобождать, а использовать повторно
        //if (pChangedBuffer) { Free(pChangedBuffer); pChangedBuffer = NULL; }

        MCHKHEAP

        _ASSERTE(pOut!=NULL); 
        if (!pOut) {
            srv.bForceFullSend = TRUE;
            return TRUE; // изменения были, но почему-то не смогли их отправить
        }
        
        MCHKHEAP

        // !!! Отсылаем информацию в GUI
        SendConsoleChanges(pOut);
        lbChangesWasSent = TRUE;

        MCHKHEAP
        
        Free(pOut);
        
        // Запомнить последнее отправленное
        if (lbDataChanged && srv.nBufCharCount) {
            MCHKHEAP
            memmove(srv.psChars+srv.nBufCharCount, srv.psChars, srv.nBufCharCount*sizeof(wchar_t));
            memmove(srv.pnAttrs+srv.nBufCharCount, srv.pnAttrs, srv.nBufCharCount*sizeof(WORD));
            MCHKHEAP
        }
    }
    
    //
    //srv.nLastUpdateTick = GetTickCount();

    //if (pCheck) Free(pCheck); -- это ССЫЛКА на локальную переменную
    return lbChangesWasSent;
}

// BufferHeight  - высота БУФЕРА (0 - без прокрутки)
// crNewSize     - размер ОКНА (ширина окна == ширине буфера)
// rNewRect      - для (BufferHeight!=0) определяет new upper-left and lower-right corners of the window
BOOL SetConsoleSize(USHORT BufferHeight, COORD crNewSize, SMALL_RECT rNewRect, LPCSTR asLabel)
{
    _ASSERTE(ghConWnd);
    if (!ghConWnd) return FALSE;

    //CSection cs(NULL,NULL);
    MSectionLock CSCS;
    if (gnRunMode == RM_SERVER)
    	CSCS.Lock(&srv.cChangeSize, TRUE, 10000);
        //cs.Enter(&srv.csChangeSize, &srv.ncsTChangeSize);
    
    if (ghLogSize) LogSize(&crNewSize, asLabel);

    _ASSERTE(crNewSize.X>=MIN_CON_WIDTH && crNewSize.Y>=MIN_CON_HEIGHT);

    // Проверка минимального размера
    if (crNewSize.X</*4*/MIN_CON_WIDTH)
        crNewSize.X = /*4*/MIN_CON_WIDTH;
    if (crNewSize.Y</*3*/MIN_CON_HEIGHT)
        crNewSize.Y = /*3*/MIN_CON_HEIGHT;

    BOOL lbNeedChange = TRUE;
    CONSOLE_SCREEN_BUFFER_INFO csbi = {{0,0}};
    if (MyGetConsoleScreenBufferInfo(ghConOut, &csbi)) {
        lbNeedChange = (csbi.dwSize.X != crNewSize.X) || (csbi.dwSize.Y != crNewSize.Y);
		if (!lbNeedChange) {
			BufferHeight = BufferHeight;
		}
    }

	COORD crMax = GetLargestConsoleWindowSize(ghConOut);
	if (crMax.X && crNewSize.X > crMax.X)
		crNewSize.X = crMax.X;
	if (crMax.Y && crNewSize.Y > crMax.Y)
		crNewSize.Y = crMax.Y;

    // Делаем это ПОСЛЕ MyGetConsoleScreenBufferInfo, т.к. некоторые коррекции размера окна 
    // она делает ориентируясь на gnBufferHeight
    gnBufferHeight = BufferHeight;
    gcrBufferSize = crNewSize;
    _ASSERTE(gcrBufferSize.Y<200);

    if (gnBufferHeight) {
        // В режиме BufferHeight - высота ДОЛЖНА быть больше допустимого размера окна консоли
        // иначе мы запутаемся при проверках "буферный ли это режим"...
        if (gnBufferHeight <= (csbi.dwMaximumWindowSize.Y * 1.2))
            gnBufferHeight = max(300, (SHORT)(csbi.dwMaximumWindowSize.Y * 1.2));

        // В режиме cmd сразу уменьшим максимальный FPS
        srv.dwLastUserTick = GetTickCount() - USER_IDLE_TIMEOUT - 1;
    }

    RECT rcConPos = {0};
    GetWindowRect(ghConWnd, &rcConPos);

    BOOL lbRc = TRUE;
    //DWORD nWait = 0;

    //if (srv.hChangingSize) {
    //    nWait = WaitForSingleObject(srv.hChangingSize, 300);
    //    _ASSERTE(nWait == WAIT_OBJECT_0);
    //    ResetEvent(srv.hChangingSize);
    //}

    // case: simple mode
    if (BufferHeight == 0)
    {
        if (lbNeedChange) {
            DWORD dwErr = 0;
            // Если этого не сделать - размер консоли нельзя УМЕНЬШИТЬ
            MoveWindow(ghConWnd, rcConPos.left, rcConPos.top, 1, 1, 1);
            //specified width and height cannot be less than the width and height of the console screen buffer's window
            lbRc = SetConsoleScreenBufferSize(ghConOut, crNewSize);
            if (!lbRc) dwErr = GetLastError();
            //TODO: а если правый нижний край вылезет за пределы экрана?
            //WARNING("отключил для теста");
            //MoveWindow(ghConWnd, rcConPos.left, rcConPos.top, GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN), 1);
            rNewRect.Left = 0; rNewRect.Top = 0;
            rNewRect.Right = crNewSize.X - 1;
            rNewRect.Bottom = crNewSize.Y - 1;
            SetConsoleWindowInfo(ghConOut, TRUE, &rNewRect);
        }

    } else {
        // Начался ресайз для BufferHeight
        COORD crHeight = {crNewSize.X, BufferHeight};

        GetWindowRect(ghConWnd, &rcConPos);
        MoveWindow(ghConWnd, rcConPos.left, rcConPos.top, 1, 1, 1);
        lbRc = SetConsoleScreenBufferSize(ghConOut, crHeight); // а не crNewSize - там "оконные" размеры
        //окошко раздвигаем только по ширине!
        //RECT rcCurConPos = {0};
        //GetWindowRect(ghConWnd, &rcCurConPos); //X-Y новые, но высота - старая
        //MoveWindow(ghConWnd, rcCurConPos.left, rcCurConPos.top, GetSystemMetrics(SM_CXSCREEN), rcConPos.bottom-rcConPos.top, 1);

        rNewRect.Left = 0;
        rNewRect.Right = crHeight.X-1;
        rNewRect.Bottom = min( (crHeight.Y-1), (rNewRect.Top+gcrBufferSize.Y-1) );
        _ASSERTE((rNewRect.Bottom-rNewRect.Top)<200);
        SetConsoleWindowInfo(ghConOut, TRUE, &rNewRect);
    }

    //if (srv.hChangingSize) { // во время запуска ConEmuC
    //    SetEvent(srv.hChangingSize);
    //}
    
    if (gnRunMode == RM_SERVER) {
        srv.bForceFullSend = TRUE;
        SetEvent(srv.hRefreshEvent);
    }

    //cs.Leave();
    CSCS.Unlock();

    return lbRc;
}

BOOL WINAPI HandlerRoutine(DWORD dwCtrlType)
{
    //PRINT_COMSPEC(L"HandlerRoutine triggered. Event type=%i\n", dwCtrlType);
	if (dwCtrlType >= CTRL_CLOSE_EVENT && dwCtrlType <= CTRL_SHUTDOWN_EVENT) {
    	PRINT_COMSPEC(L"Console about to be closed\n", 0);
		gbInShutdown = TRUE;
	}
    /*SafeCloseHandle(ghLogSize);
    if (wpszLogSizeFile) {
        DeleteFile(wpszLogSizeFile);
        Free(wpszLogSizeFile); wpszLogSizeFile = NULL;
    }*/

    return TRUE;
}

int GetProcessCount(DWORD *rpdwPID, UINT nMaxCount)
{
	if (!rpdwPID || !nMaxCount) {
		_ASSERTE(rpdwPID && nMaxCount);
		return srv.nProcessCount;
	}

	MSectionLock CS;
	if (!CS.Lock(srv.csProc, 200)) {
		// Если не удалось заблокировать переменную - просто вернем себя
		*rpdwPID = gnSelfPID;
		return 1;
	}

	UINT nSize = srv.nProcessCount;
	if (nSize > nMaxCount) {
		memset(rpdwPID, 0, sizeof(DWORD)*nMaxCount);
		rpdwPID[0] = gnSelfPID;

		for (int i1=0, i2=(nMaxCount-1); i1<(int)nSize && i2>0; i1++, i2--)
			rpdwPID[i2] = srv.pnProcesses[i1];

		nSize = nMaxCount;

	} else {
		memmove(rpdwPID, srv.pnProcesses, sizeof(DWORD)*nSize);

		for (UINT i=nSize; i<nMaxCount; i++)
			rpdwPID[i] = 0;
	}

	_ASSERTE(rpdwPID[0]);

	return nSize;

	/*
    //DWORD dwErr = 0; BOOL lbRc = FALSE;
    DWORD *pdwPID = NULL; int nCount = 0, i;
    EnterCriticalSection(&srv.csProc);
    nCount = srv.nProcesses.size();
    if (nCount > 0 && rpdwPID) {
        pdwPID = (DWORD*)Alloc(nCount, sizeof(DWORD));
        _ASSERTE(pdwPID!=NULL);
        if (pdwPID) {
            std::vector<DWORD>::iterator iter = srv.nProcesses.begin();
            i = 0;
            while (iter != srv.nProcesses.end()) {
                pdwPID[i++] = *iter;
                iter ++;
            }
        }
    }
    LeaveCriticalSection(&srv.csProc);
    if (rpdwPID)
        *rpdwPID = pdwPID;
    return nCount;
	*/
}

// Если crNew выходит за пределы rgn - увеличить его
void EnlargeRegion(CESERVER_CHAR_HDR& rgn, const COORD crNew)
{
    //if (rgn.crStart.X == -1 && rgn.crStart.Y == -1 && rgn.crEnd.X == 0 && rgn.crEnd.Y == 0)
    if (rgn.nSize == 0)
    {   // Считаем, что регион не задавался
        rgn.nSize = sizeof(CESERVER_CHAR_HDR);
        rgn.cr1 = crNew; rgn.cr1 = crNew;
    } else
    
    // Если измененная позиция уже другая - увеличиваем регион
    if (crNew.X != rgn.cr1.X || crNew.Y != rgn.cr1.Y) {
        if ( (rgn.cr1.Y <= crNew.Y) && (crNew.Y <= rgn.cr2.Y) ) {
            // Y попадает в уже измененный регион. X - только информационно
        } else {
            if (crNew.Y < rgn.cr1.Y)
                rgn.cr1.Y = crNew.Y;
            else if (crNew.Y > rgn.cr2.Y)
                rgn.cr2.Y = crNew.Y;
        }
        if (crNew.X < rgn.cr1.X)
            rgn.cr1.X = crNew.X;
        else if (crNew.X > rgn.cr2.X)
            rgn.cr2.X = crNew.X;
    }
}

WARNING("BUGBUG: x64 US-Dvorak");
void CheckKeyboardLayout()
{
    if (pfnGetConsoleKeyboardLayoutName) {
        wchar_t szCurKeybLayout[32];
        // Возвращает строку в виде "00000419" -- может тут 16 цифр?
        if (pfnGetConsoleKeyboardLayoutName(szCurKeybLayout)) {
            if (lstrcmpW(szCurKeybLayout, srv.szKeybLayout)) {
				#ifdef _DEBUG
				wchar_t szDbg[128];
				wsprintfW(szDbg, L"ConEmuC: InputLayoutChanged to '%s'\n", szCurKeybLayout);
				OutputDebugString(szDbg);
				#endif
                // Сменился
                lstrcpyW(srv.szKeybLayout, szCurKeybLayout);
                // Отошлем в GUI
                wchar_t *pszEnd = szCurKeybLayout+8;
                WARNING("BUGBUG: 16 цифр не вернет");
				// LayoutName: "00000409", "00010409", ...
				// А HKL от него отличается, так что передаем DWORD
				// HKL в x64 выглядит как: "0x0000000000020409", "0xFFFFFFFFF0010409"
                DWORD dwLayout = wcstol(szCurKeybLayout, &pszEnd, 16);
                CESERVER_REQ* pIn = ExecuteNewCmd(CECMD_LANGCHANGE,sizeof(CESERVER_REQ_HDR)+sizeof(DWORD));
                if (pIn) {
                    //memmove(pIn->Data, &dwLayout, 4);
                    pIn->dwData[0] = dwLayout;

                    CESERVER_REQ* pOut = NULL;
                    pOut = ExecuteGuiCmd(ghConWnd, pIn, ghConWnd);
                    if (pOut) ExecuteFreeResult(pOut);
                    ExecuteFreeResult(pIn);
                }
            }
        }
    }
}

// Сохранить данные ВСЕЙ консоли в gpStoredOutput
void CmdOutputStore()
{
    DEBUGSTR(L"--- CmdOutputStore begin\n");
    CONSOLE_SCREEN_BUFFER_INFO lsbi = {{0,0}};
    // !!! Нас интересует реальное положение дел в консоли, 
    //     а не скорректированное функцией MyGetConsoleScreenBufferInfo
    if (!GetConsoleScreenBufferInfo(ghConOut, &lsbi)) {
        if (gpStoredOutput) { Free(gpStoredOutput); gpStoredOutput = NULL; }
        return; // Не смогли получить информацию о консоли...
    }
    int nOneBufferSize = lsbi.dwSize.X * lsbi.dwSize.Y * 2; // Читаем всю консоль целиком!
    // Если требуется увеличение размера выделенной памяти
    if (gpStoredOutput && gpStoredOutput->hdr.cbMaxOneBufferSize < (DWORD)nOneBufferSize) {
        Free(gpStoredOutput); gpStoredOutput = NULL;
    }
    if (gpStoredOutput == NULL) {
        // Выделяем память: заголовок + буфер текста (на атрибуты забьем)
        gpStoredOutput = (CESERVER_CONSAVE*)Alloc(sizeof(CESERVER_CONSAVE_HDR)+nOneBufferSize,1);
        _ASSERTE(gpStoredOutput!=NULL);
        if (gpStoredOutput == NULL)
            return; // Не смогли выделить память
        gpStoredOutput->hdr.cbMaxOneBufferSize = nOneBufferSize;
    }
    // Запомнить sbi
    //memmove(&gpStoredOutput->hdr.sbi, &lsbi, sizeof(lsbi));
    gpStoredOutput->hdr.sbi = lsbi;
    // Теперь читаем данные
    COORD coord = {0,0};
    DWORD nbActuallyRead = 0;
    DWORD nReadLen = lsbi.dwSize.X * lsbi.dwSize.Y;

    // [Roman Kuzmin]
    // In FAR Manager source code this is mentioned as "fucked method". Yes, it is.
    // Functions ReadConsoleOutput* fail if requested data size exceeds their buffer;
    // MSDN says 64K is max but it does not say how much actually we can request now.
    // Experiments show that this limit is floating and it can be much less than 64K.
    // The solution below is not optimal when a user sets small font and large window,
    // but it is safe and practically optimal, because most of users set larger fonts
    // for large window and ReadConsoleOutput works OK. More optimal solution for all
    // cases is not that difficult to develop but it will be increased complexity and
    // overhead often for nothing, not sure that we really should use it.

    if (!ReadConsoleOutputCharacter(ghConOut, gpStoredOutput->Data, nReadLen, coord, &nbActuallyRead)
        || (nbActuallyRead != nReadLen))
    {
        DEBUGSTR(L"--- Full block read failed: read line by line\n");
        wchar_t* ConCharNow = gpStoredOutput->Data;
        nReadLen = lsbi.dwSize.X;
        for(int y = 0; y < (int)lsbi.dwSize.Y; y++, coord.Y++)
        {
            ReadConsoleOutputCharacter(ghConOut, ConCharNow, nReadLen, coord, &nbActuallyRead);
                ConCharNow += lsbi.dwSize.X;
        }
    }
    DEBUGSTR(L"--- CmdOutputStore end\n");
}

void CmdOutputRestore()
{
    if (gpStoredOutput) {
        TODO("Восстановить текст скрытой (прокрученной вверх) части консоли");
        // Учесть, что ширина консоли могла измениться со времени выполнения предыдущей команды.
        // Сейчас у нас в верхней части консоли может оставаться кусочек предыдущего вывода (восстановил FAR).
        // 1) Этот кусочек нужно считать
        // 2) Скопировать в нижнюю часть консоли (до которой докрутилась предыдущая команда)
        // 3) прокрутить консоль до предыдущей команды (куда мы только что скопировали данные сверху)
        // 4) восстановить оставшуюся часть консоли. Учесть, что фар может
        //    выполнять некоторые команды сам и курсор вообще-то мог несколько уехать...
    }
}

LPVOID Alloc(size_t nCount, size_t nSize)
{
    #ifdef _DEBUG
    //HeapValidate(ghHeap, 0, NULL);
    #endif

    size_t nWhole = nCount * nSize;
    LPVOID ptr = HeapAlloc ( ghHeap, HEAP_GENERATE_EXCEPTIONS|HEAP_ZERO_MEMORY, nWhole );

    #ifdef HEAP_LOGGING
    wchar_t szDbg[64];
    wsprintfW(szDbg, L"%i: ALLOCATED   0x%08X..0x%08X   (%i bytes)\n", GetCurrentThreadId(), (DWORD)ptr, ((DWORD)ptr)+nWhole, nWhole);
    DEBUGSTR(szDbg);
    #endif

    #ifdef _DEBUG
    HeapValidate(ghHeap, 0, NULL);
    if (ptr) {
        gnHeapUsed += nWhole;
        if (gnHeapMax < gnHeapUsed)
            gnHeapMax = gnHeapUsed;
    }
    #endif

    return ptr;
}

void Free(LPVOID ptr)
{
    if (ptr && ghHeap) {
        #ifdef _DEBUG
        //HeapValidate(ghHeap, 0, NULL);
        size_t nMemSize = HeapSize(ghHeap, 0, ptr);
        #endif
        #ifdef HEAP_LOGGING
        wchar_t szDbg[64];
        wsprintfW(szDbg, L"%i: FREE BLOCK  0x%08X..0x%08X   (%i bytes)\n", GetCurrentThreadId(), (DWORD)ptr, ((DWORD)ptr)+nMemSize, nMemSize);
        DEBUGSTR(szDbg);
        #endif

        HeapFree ( ghHeap, 0, ptr );

        #ifdef _DEBUG
        HeapValidate(ghHeap, 0, NULL);
        if (gnHeapUsed > nMemSize)
            gnHeapUsed -= nMemSize;
        #endif
    }
}

/* Используются как extern в ConEmuCheck.cpp */
LPVOID _calloc(size_t nCount,size_t nSize) {
	return Alloc(nCount,nSize);
}
LPVOID _malloc(size_t nCount) {
	return Alloc(nCount,1);
}
void   _free(LPVOID ptr) {
	Free(ptr);
}

#endif
