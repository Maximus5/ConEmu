#include <Windows.h>
#include <stdio.h>
#include <Shlwapi.h>
//#include <conio.h>
#include <vector>
#include "..\common\common.hpp"
#include "..\common\ConEmuCheck.h"

#ifdef _DEBUG
//  Раскомментировать, чтобы сразу после запуска процесса (conemuc.exe) показать MessageBox, чтобы прицепиться дебаггером
//	#define SHOW_STARTED_MSGBOX
#endif

WARNING("!!!! Пока можно при появлении события запоминать текущий тик");
// и проверять его в RefreshThread. Если он не 0 - и дельта больше (100мс?)
// то принудительно перечитать консоль и сбросить тик в 0.

#ifdef _DEBUG
#define MCHKHEAP { int MDEBUG_CHK=_CrtCheckMemory(); _ASSERTE(MDEBUG_CHK); }
#else
#define MCHKHEAP
#endif

WARNING("Наверное все-же стоит производить периодические чтения содержимого консоли, а не только по событию");

WARNING("Стоит именно здесь осуществлять проверку живости GUI окна (если оно было). Ведь может быть запущен не far, а CMD.exe");

WARNING("Если GUI умер, или не подцепился по таймауту - показать консольное окно и наверное установить шрифт поболе");

#if defined(__GNUC__)
    //#include "assert.h"
    #define _ASSERTE(x)
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
DWORD WINAPI InputThread(LPVOID lpvParam);
BOOL GetAnswerToRequest(CESERVER_REQ& in, CESERVER_REQ** out); 
DWORD WINAPI WinEventThread(LPVOID lpvParam);
void CALLBACK WinEventProc(HWINEVENTHOOK hWinEventHook, DWORD event, HWND hwnd, LONG idObject, LONG idChild, DWORD dwEventThread, DWORD dwmsEventTime);
void CheckCursorPos();
void SendConsoleChanges(CESERVER_REQ* pOut);
CESERVER_REQ* CreateConsoleInfo(CESERVER_CHAR* pCharOnly, int bCharAttrBuff);
BOOL ReloadConsoleInfo(); // возвращает TRUE в случае изменений
void ReloadFullConsoleInfo(CESERVER_CHAR* pCharOnly=NULL); // В том числе перечитывает содержимое
DWORD WINAPI RefreshThread(LPVOID lpvParam); // Нить, перечитывающая содержимое консоли
DWORD ReadConsoleData(CESERVER_CHAR** pCheck = NULL, BOOL* pbDataChanged = NULL); //((LPRECT)1) или реальный LPRECT
void SetConsoleFontSizeTo(HWND inConWnd, int inSizeX, int inSizeY);
int ParseCommandLine(LPCWSTR asCmdLine, wchar_t** psNewCmd); // Разбор параметров командной строки
int NextArg(LPCWSTR &asCmdLine, wchar_t* rsArg/*[MAX_PATH+1]*/);
int ServerInit(); // Создать необходимые события и нити
void ServerDone(int aiRc);
int ComspecInit();
void ComspecDone(int aiRc);
void Help();
BOOL SetConsoleSize(USHORT BufferHeight, COORD crNewSize, SMALL_RECT rNewRect, LPCSTR asLabel = NULL);
void CreateLogSizeFile();
void LogSize(COORD* pcrSize, LPCSTR pszLabel);
BOOL WINAPI HandlerRoutine(DWORD dwCtrlType);
int GetProcessCount(DWORD **rpdwPID);


/*  Global  */
DWORD   gnSelfPID = 0;
HANDLE  ghConIn = NULL, ghConOut = NULL;
HWND    ghConWnd = NULL;
HANDLE  ghExitEvent = NULL;
BOOL    gbAlwaysConfirmExit = FALSE;
BOOL    gbAttachMode = FALSE;
DWORD   gdwMainThreadId = 0;
//int       gnBufferHeight = 0;
wchar_t* gpszRunCmd = NULL;
HANDLE  ghCtrlCEvent = NULL, ghCtrlBreakEvent = NULL;

enum {
    RM_UNDEFINED = 0,
    RM_SERVER,
    RM_COMSPEC
} gnRunMode = RM_UNDEFINED;

struct {
	DWORD dwProcessGroup;
	//
    HANDLE hServerThread;   DWORD dwServerThreadId;
    HANDLE hRefreshThread;  DWORD dwRefreshThread;
    HANDLE hWinEventThread; DWORD dwWinEventThread;
    HANDLE hInputThread;    DWORD dwInputThreadId;
    //
    CRITICAL_SECTION csProc;
    CRITICAL_SECTION csConBuf;
	// Список процессов нам нужен, чтобы определить, когда консоль уже не нужна.
	// Например, запустили FAR, он запустил Update, FAR перезапущен...
	std::vector<DWORD> nProcesses;
    //
    wchar_t szPipename[MAX_PATH], szInputname[MAX_PATH], szGuiPipeName[MAX_PATH];
    //
    HANDLE hConEmuGuiAttached;
    HWINEVENTHOOK hWinHook;
    BOOL bContentsChanged; // Первое чтение параметров должно быть ПОЛНЫМ
    wchar_t* psChars;
    WORD* pnAttrs;
    DWORD nBufCharCount;
    DWORD dwSelRc; CONSOLE_SELECTION_INFO sel; // GetConsoleSelectionInfo
    DWORD dwCiRc; CONSOLE_CURSOR_INFO ci; // GetConsoleCursorInfo
    DWORD dwConsoleCP, dwConsoleOutputCP, dwConsoleMode;
    DWORD dwSbiRc; CONSOLE_SCREEN_BUFFER_INFO sbi; // GetConsoleScreenBufferInfo
    DWORD nMainTimerElapse;
    BOOL  bConsoleActive;
    HANDLE hRefreshEvent; // ServerMode, перечитать консоль, и если есть изменения - отослать в GUI
    HANDLE hChangingSize; // FALSE на время смены размера консоли
    BOOL  bNeedFullReload;
    DWORD nLastUpdateTick;
} srv = {0};

struct {
	DWORD dwProcessGroup;
	DWORD dwFarPID;
	CONSOLE_SCREEN_BUFFER_INFO sbi;
} cmd = {0};

COORD gcrBufferSize = {80,25};
BOOL  gbParmBufferSize = FALSE;
SHORT gnBufferHeight = 0;

HANDLE ghLogSize = NULL;
wchar_t* wpszLogSizeFile = NULL;


BOOL gbInRecreateRoot = FALSE;

//#define CES_NTVDM 0x10 -- common.hpp
DWORD dwActiveFlags = 0;

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
#define CERR_CMDLINE 117
#define CERR_HELPREQUESTED 118
#define CERR_ATTACHFAILED 119


int main()
{
    TODO("можно при ошибках показать консоль, предварительно поставив 80x25 и установив крупный шрифт");

    int iRc = 100;
    //wchar_t sComSpec[MAX_PATH];
    //wchar_t* psFilePart;
    //wchar_t* psCmdLine = GetCommandLineW();
    //size_t nCmdLine = lstrlenW(psCmdLine);
    //wchar_t* psNewCmd = NULL;
    HANDLE hWait[2]={NULL,NULL};
    //BOOL bViaCmdExe = TRUE;
    PROCESS_INFORMATION pi; memset(&pi, 0, sizeof(pi));
    STARTUPINFOW si; memset(&si, 0, sizeof(si)); si.cb = sizeof(si);
    DWORD dwErr = 0, nWait = 0;
    BOOL lbRc = FALSE;
    DWORD mode = 0;
    BOOL lb = FALSE;

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
    if (!IsDebuggerPresent()) MessageBox(0,GetCommandLineW(),L"ComEmuC Loaded",0);
#endif
    
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

    // Дескрипторы
    ghConIn  = CreateFile(L"CONIN$", GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_READ,
                0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
    if (ghConIn == INVALID_HANDLE_VALUE) {
        dwErr = GetLastError();
        wprintf(L"CreateFile(CONIN$) failed, ErrCode=0x%08X\n", dwErr); 
        iRc = CERR_CONINFAILED; goto wrap;
    }
    // Дескрипторы
    ghConOut = CreateFile(L"CONOUT$", GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_READ,
                0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
    if (ghConOut == INVALID_HANDLE_VALUE) {
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
    lbRc = CreateProcessW(NULL, gpszRunCmd, NULL,NULL, TRUE, 
			NORMAL_PRIORITY_CLASS/*|CREATE_NEW_PROCESS_GROUP*/, 
			NULL, NULL, &si, &pi);
    dwErr = GetLastError();
    if (!lbRc)
    {
        wprintf (L"Can't create process, ErrCode=0x%08X! Command to be executed:\n%s\n", dwErr, gpszRunCmd);
        iRc = CERR_CREATEPROCESS; goto wrap;
    }
    //delete psNewCmd; psNewCmd = NULL;

	// Обязательно, иначе по CtrlC мы свалимся
	//SetConsoleCtrlHandler(HandlerRoutine, TRUE);

    
    /* *************************** */
    /* *** Ожидание завершения *** */
    /* *************************** */
    
    if (gnRunMode == RM_SERVER) {
		srv.dwProcessGroup = pi.dwProcessId;

        if (pi.hProcess) SafeCloseHandle(pi.hProcess); 
        if (pi.hThread) SafeCloseHandle(pi.hThread);

        if (srv.hConEmuGuiAttached) {
            if (WaitForSingleObject(srv.hConEmuGuiAttached, 1000) == WAIT_OBJECT_0) {
                // GUI пайп готов
                wsprintf(srv.szGuiPipeName, CEGUIPIPENAME, L".", (DWORD)ghConWnd); // был gnSelfPID
            }
        }
    
        // Ждем, пока в консоли не останется процессов (кроме нашего)
        TODO("Проверить, может ли так получиться, что CreateProcess прошел, а к консоли он не прицепился? Может, если процесс GUI");
        nWait = WaitForSingleObject(ghExitEvent, 6*1000); //Запуск процесса наверное может задержать антивирус
        if (nWait != WAIT_OBJECT_0) { // Если таймаут
            EnterCriticalSection(&srv.csProc);
            iRc = srv.nProcesses.size();
            LeaveCriticalSection(&srv.csProc);
            // И процессов в консоли все еще нет
            if (iRc == 0) {
                wprintf (L"Process was not attached to console. Is it GUI?\nCommand to be executed:\n%s\n", gpszRunCmd);
                iRc = CERR_PROCESSTIMEOUT; goto wrap;
            }

            // По крайней мере один процесс в консоли запустился. Ждем пока в консоли не останется никого кроме нас
            WaitForSingleObject(ghExitEvent, INFINITE);
        }
    } else {
        // В режиме ComSpec нас интересует завершение ТОЛЬКО дочернего процесса

		wchar_t szEvtName[128];

		wsprintf(szEvtName, CESIGNAL_C, pi.dwProcessId);
		ghCtrlCEvent = CreateEvent(NULL, FALSE, FALSE, szEvtName);
		wsprintf(szEvtName, CESIGNAL_BREAK, pi.dwProcessId);
		ghCtrlBreakEvent = CreateEvent(NULL, FALSE, FALSE, szEvtName);

		HANDLE hEvents[3];
		hEvents[0] = pi.hProcess;
		hEvents[1] = ghCtrlCEvent;
		hEvents[2] = ghCtrlBreakEvent;
        //WaitForSingleObject(pi.hProcess, INFINITE);
		DWORD dwWait = 0;
		BOOL lbGenRc = FALSE;
		while ((dwWait = WaitForMultipleObjects(3, hEvents, FALSE, INFINITE)) != WAIT_OBJECT_0)
		{
			if (dwWait == (WAIT_OBJECT_0+1) || dwWait == (WAIT_OBJECT_0+2)) {
				DWORD dwEvent = (dwWait == (WAIT_OBJECT_0+1)) ? CTRL_C_EVENT : CTRL_BREAK_EVENT;
				/*DWORD dwMode = 0;
				HANDLE hConIn  = CreateFile(L"CONIN$", GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_READ,
					0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
				GetConsoleMode(hConIn, &dwMode);
				SetConsoleMode(hConIn, dwMode);
				CloseHandle(hConIn);*/

				lbGenRc = GenerateConsoleCtrlEvent(dwEvent, pi.dwProcessId);
			}
		}
        // Сразу закрыть хэндлы
        if (pi.hProcess) SafeCloseHandle(pi.hProcess); 
        if (pi.hThread) SafeCloseHandle(pi.hThread);
    }

    
    
    
    /* ************************* */
    /* *** Завершение работы *** */
    /* ************************* */
    
    iRc = 0;
wrap:
    // На всякий случай - выставим событие
    if (ghExitEvent) SetEvent(ghExitEvent);
    
    
    /* ***************************** */
    /* *** "Режимное" завершение *** */
    /* ***************************** */
    
    if (gnRunMode == RM_SERVER) {
        ServerDone(iRc);
        //MessageBox(0,L"Server done...",L"ComEmuC",0);
    } else {
        ComspecDone(iRc);
        //MessageBox(0,L"Comspec done...",L"ComEmuC",0);
    }

    
    /* ************************** */
    /* *** "Общее" завершение *** */
    /* ************************** */
    
    if (ghConIn && ghConIn!=INVALID_HANDLE_VALUE) {
        SafeCloseHandle(ghConIn);
    }
    if (ghConOut && ghConOut!=INVALID_HANDLE_VALUE) {
        SafeCloseHandle(ghConOut);
    }

    if (iRc!=0 || gbAlwaysConfirmExit) {
        // Чтобы ошибку было нормально видно
        BOOL lbNeedVisible = FALSE;
        if (!ghConWnd) ghConWnd = GetConsoleWindow();
        if (ghConWnd) { // Если консоль была скрыта
            if (!IsWindowVisible(ghConWnd)) {
                lbNeedVisible = TRUE;
                // поставить "стандартный" 80x25, или то, что было передано к ком.строке
                SMALL_RECT rcNil = {0}; SetConsoleSize(0, gcrBufferSize, rcNil, ":Exiting");
                SetConsoleFontSizeTo(ghConWnd, 8, 12); // установим шрифт побольше
                ShowWindow(ghConWnd, SW_SHOWNORMAL); // и покажем окошко
            }
        }

        // Сначала почистить буфер
        INPUT_RECORD r = {0}; DWORD dwCount = 0;
        FlushConsoleInputBuffer(GetStdHandle(STD_INPUT_HANDLE));

        //
        wprintf(L"Press Enter to close console");

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
            if (r.EventType == KEY_EVENT && r.Event.KeyEvent.wVirtualKeyCode == VK_RETURN)
                break;
        }
        //MessageBox(0,L"Debug message...............1",L"ComEmuC",0);
        //int nCh = _getch();
        wprintf(L"\n");
    }
    
    SafeCloseHandle(ghLogSize);
    if (wpszLogSizeFile) {
        DeleteFile(wpszLogSizeFile);
        free(wpszLogSizeFile); wpszLogSizeFile = NULL;
    }
    
    if (gpszRunCmd) { delete gpszRunCmd; gpszRunCmd = NULL; }
    return iRc;
}

void Help()
{
    wprintf(
        L"ConEmuC. Copyright (c) 2009, Maximus5\n"
        L"This is a console part of ConEmu product.\n"
        L"Usage: ComEmuC [switches] /C <command line, passed to %%COMSPEC%%>\n"
        L"   or: ComEmuC [switches] /CMD <program with arguments, far.exe for example>\n"
        L"   or: ComEmuC /?\n"
        L"Switches:\n"
        L"        /CONFIRM  - confirm closing console on program termination\n"
        L"        /ATTACH   - auto attach to ConEmu GUI\n"
        L"        /B{W|H|Z} - define buffer width, height, window height\n"
        L"        /LOG      - create (debug) log file\n"
    );
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
    size_t nCmdLine = 0;
    
    if (!asCmdLine || !*asCmdLine)
    {
        DWORD dwErr = GetLastError();
        wprintf (L"GetCommandLineW failed! ErrCode=0x%08X\n", dwErr);
        return CERR_GETCOMMANDLINE;
    }

    gnRunMode = RM_UNDEFINED;
    
    while ((iRc = NextArg(asCmdLine, szArg)) == 0)
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

        if (wcsncmp(szArg, L"/B", 2)==0) {
            if (wcsncmp(szArg, L"/BW=", 4)==0) {
                gcrBufferSize.X = _wtoi(szArg+4); gbParmBufferSize = TRUE;
            } else if (wcsncmp(szArg, L"/BH=", 4)==0) {
                gcrBufferSize.Y = _wtoi(szArg+4); gbParmBufferSize = TRUE;
            } else if (wcsncmp(szArg, L"/BZ=", 4)==0) {
                gnBufferHeight = _wtoi(szArg+4); gbParmBufferSize = TRUE;
            }
        } else
        
        if (wcscmp(szArg, L"/LOG")==0) {
            CreateLogSizeFile();
        } else

        // После этих аргументов - идет то, что передается в CreateProcess!
        if (wcscmp(szArg, L"/C")==0 || wcscmp(szArg, L"/c")==0) {
            gnRunMode = RM_COMSPEC;
            break; // asCmdLine уже указывает на запускаемую программу
        } else if (wcscmp(szArg, L"/CMD")==0 || wcscmp(szArg, L"/cmd")==0) {
            gnRunMode = RM_SERVER;
            break; // asCmdLine уже указывает на запускаемую программу
        }
    }
    
    if (iRc != 0) {
        wprintf (L"Parsing command line failed:\n%s\n", asCmdLine);
        return iRc;
    }
    if (gnRunMode == RM_UNDEFINED) {
        wprintf (L"Parsing command line failed (/C argument not found):\n%s\n", GetCommandLineW());
        return CERR_CARGUMENT;
    }

    if (gnRunMode == RM_COMSPEC) {
        pwszCopy = asCmdLine;
        if ((iRc = NextArg(pwszCopy, szArg)) != 0) {
            wprintf (L"Parsing command line failed:\n%s\n", asCmdLine);
            return iRc;
        }
        pwszCopy = wcsrchr(szArg, L'\\'); if (!pwszCopy) pwszCopy = szArg;
    
        #pragma warning( push )
        #pragma warning(disable : 6400)
        if (lstrcmpiW(pwszCopy, L"cmd")==0 || lstrcmpiW(pwszCopy, L"cmd.exe")==0) {
            bViaCmdExe = FALSE; // уже указан командный процессор, cmd.exe в начало добавлять не нужно
        }
        #pragma warning( pop )
    } else {
        bViaCmdExe = FALSE; // командным процессором выступает сам ConEmuC (серверный режим)
    }
    
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

        nCmdLine += lstrlenW(szComSpec)+10; // "/C" и кавычки
    }

    *psNewCmd = new wchar_t[nCmdLine];
    if (!(*psNewCmd))
    {
        wprintf (L"Can't allocate %i wchars!\n", nCmdLine);
        return CERR_NOTENOUGHMEM1;
    }
    
    if (!bViaCmdExe)
    {
        lstrcpyW( *psNewCmd, asCmdLine );
    } else {
        if (wcschr(szComSpec, L' ')) {
            (*psNewCmd)[0] = L'"';
            lstrcpyW( (*psNewCmd)+1, szComSpec );
            lstrcatW( (*psNewCmd), L"\" /C " );
        } else {
            lstrcpyW( (*psNewCmd), szComSpec );
            lstrcatW( (*psNewCmd), L" /C " );
        }
        lstrcatW( (*psNewCmd), asCmdLine );
    }
    
    return 0;
}

int NextArg(LPCWSTR &asCmdLine, wchar_t* rsArg/*[MAX_PATH+1]*/)
{
    LPCWSTR psCmdLine = asCmdLine, pch = NULL;
    wchar_t ch = *psCmdLine;
    int nArgLen = 0;
    
    while (ch == L' ' || ch == L'\t' || ch == L'\r' || ch == L'\n') ch = *(++psCmdLine);
    if (ch == 0) return CERR_CMDLINE;

    // аргумент начинается с "
    if (ch == L'"') {
        psCmdLine++;
        pch = wcschr(psCmdLine, L'"');
        if (!pch) return CERR_CMDLINE;
        while (pch[1] == L'"') {
            pch += 2;
            pch = wcschr(pch, L'"');
            if (!pch) return CERR_CMDLINE;
        }
        // Теперь в pch ссылка на последнюю "
    } else {
        // До конца строки или до первого пробела
        pch = wcschr(psCmdLine, L' ');
        if (!pch) pch = psCmdLine + wcslen(psCmdLine); // до конца строки
    }
    
    nArgLen = pch - psCmdLine;
    if (nArgLen > MAX_PATH) return CERR_CMDLINE;

    // Вернуть аргумент
    memcpy(rsArg, psCmdLine, nArgLen*sizeof(wchar_t));
    rsArg[nArgLen] = 0;

    psCmdLine = pch;
    
    // Finalize
    ch = *psCmdLine; // может указывать на закрывающую кавычку
	if (ch == L'"') ch = *(++psCmdLine);
    while (ch == L' ' || ch == L'\t' || ch == L'\r' || ch == L'\n') ch = *(++psCmdLine);
    asCmdLine = psCmdLine;
    
    return 0;
}

int ComspecInit()
{
    TODO("Определить код родительского процесса, и если это FAR - запомнить его (для подключения к пайпу плагина)");
	TODO("Размер получить из GUI, если оно есть, иначе - по умолчанию");
	TODO("GUI может скорректировать размер с учетом полосы прокрутки");

	WARNING("Послать в GUI CONEMUCMDSTARTED");

	if (GetConsoleScreenBufferInfo(ghConOut, &cmd.sbi)) {
		//SMALL_RECT rc = {0}; 
		//COORD crNew = {cmd.sbi.dwSize.X,cmd.sbi.dwSize.Y};
		SetConsoleSize(1000, cmd.sbi.dwSize, cmd.sbi.srWindow, "ComspecInit");
	}
    return 0;
}

void ComspecDone(int aiRc)
{
	WARNING("Послать в GUI CONEMUCMDSTOPPED");

    TODO("Уведомить плагин через пайп (если родитель - FAR) что процесс завершен. Плагин должен считать и запомнить содержимое консоли и только потом вернуть управление в ConEmuC!");
    
    // Вернуть размер буфера (высота И ширина)
	if (cmd.sbi.dwSize.X && cmd.sbi.dwSize.Y) {
		SMALL_RECT rc = {0};
		SetConsoleSize(0, cmd.sbi.dwSize, rc, "ComspecDone");
	}

	SafeCloseHandle(ghCtrlCEvent);
	SafeCloseHandle(ghCtrlBreakEvent);
}

WARNING("Добавить LogInput(INPUT_RECORD* pRec) но имя файла сделать 'ConEmuC-input-%i.log'");
void CreateLogSizeFile()
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
    
    wpszLogSizeFile = wcsdup(szFile);
    // OK, лог создали
    LPCSTR pszCmdLine = GetCommandLineA();
    if (pszCmdLine) {
        WriteFile(ghLogSize, pszCmdLine, strlen(pszCmdLine), &dwErr, 0);
        WriteFile(ghLogSize, "\r\n", 2, &dwErr, 0);
    }
    LogSize(NULL, "Startup");
}

void LogSize(COORD* pcrSize, LPCSTR pszLabel)
{
    if (!ghLogSize) return;
    
    CONSOLE_SCREEN_BUFFER_INFO lsbi = {{0,0}};
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
    WriteFile(ghLogSize, szInfo, strlen(szInfo), &dwLen, 0);
    FlushFileBuffers(ghLogSize);
}

// Создать необходимые события и нити
int ServerInit()
{
    int iRc = 0;
    DWORD dwErr = 0;
    HANDLE hWait[2] = {NULL,NULL};
	wchar_t szComSpec[MAX_PATH+1], szSelf[MAX_PATH+1];

	TODO("Сразу проверить, может ComSpecC уже есть?");
	if (GetEnvironmentVariable(L"ComSpec", szComSpec, MAX_PATH)) {
		wchar_t* pszSlash = wcsrchr(szComSpec, L'\\');
		if (pszSlash) {
			if (wcsnicmp(pszSlash, L"\\conemuc.", 9)) {
				// Если это НЕ мы - сохранить в ComSpecC
				SetEnvironmentVariable(L"ComSpecC", szComSpec);
			}
		}
	}
	if (GetModuleFileName(NULL, szSelf, MAX_PATH)) {
		SetEnvironmentVariable(L"ComSpec", szSelf);
	}

    srv.bContentsChanged = TRUE;
    srv.nMainTimerElapse = 10;
    srv.bConsoleActive = TRUE; TODO("Обрабатывать консольные события Activate/Deactivate");
    srv.bNeedFullReload = TRUE;

    
    InitializeCriticalSection(&srv.csConBuf);
    InitializeCriticalSection(&srv.csProc);

    // временно используем эту переменную, чтобы не плодить локальных
    wsprintfW(srv.szPipename, CEGUIATTACHED, (DWORD)ghConWnd);
    srv.hConEmuGuiAttached = CreateEvent(NULL, TRUE, FALSE, srv.szPipename);
    _ASSERTE(srv.hConEmuGuiAttached!=NULL);
    if (srv.hConEmuGuiAttached) ResetEvent(srv.hConEmuGuiAttached);
    
    // Инициализация имен пайпов
    wsprintfW(srv.szPipename, CESERVERPIPENAME, L".", gnSelfPID);
    wsprintfW(srv.szInputname, CESERVERINPUTNAME, L".", gnSelfPID);

    // Размер шрифта и Lucida. Обязательно для серверного режима.
    if (ghLogSize) LogSize(NULL, ":SetConsoleFontSizeTo.before");
    SetConsoleFontSizeTo(ghConWnd, 4, 6);
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

    srv.hChangingSize = CreateEvent(NULL,TRUE,FALSE,NULL);
    if (!srv.hChangingSize) {
        dwErr = GetLastError();
        wprintf(L"CreateEvent(hChangingSize) failed, ErrCode=0x%08X\n", dwErr); 
        iRc = CERR_REFRESHEVENT; goto wrap;
    }
    SetEvent(srv.hChangingSize);

    
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
    
    
    // The client thread that calls SetWinEventHook must have a message loop in order to receive events.");
    hWait[0] = CreateEvent(NULL,FALSE,FALSE,NULL);
    _ASSERTE(hWait[0]!=NULL);
    srv.hWinEventThread = CreateThread( 
        NULL,              // no security attribute 
        0,                 // default stack size 
        WinEventThread,      // thread proc
        hWait[0],              // thread parameter 
        0,                 // not suspended 
        &srv.dwWinEventThread);      // returns thread ID 
    if (srv.hWinEventThread == NULL) 
    {
        dwErr = GetLastError();
        wprintf(L"CreateThread(WinEventThread) failed, ErrCode=0x%08X\n", dwErr); 
        SafeCloseHandle(hWait[0]);
        hWait[0]=NULL; hWait[1]=NULL;
        iRc = CERR_WINEVENTTHREAD; goto wrap;
    }
    hWait[1] = srv.hWinEventThread;
    dwErr = WaitForMultipleObjects(2, hWait, FALSE, 10000);
    SafeCloseHandle(hWait[0]);
    hWait[0]=NULL; hWait[1]=NULL;
    if (!srv.hWinHook) {
        _ASSERT(dwErr == WAIT_TIMEOUT);
        if (dwErr == WAIT_TIMEOUT) { // по идее этого быть не должно
            #pragma warning( push )
            #pragma warning( disable : 6258 )
            TerminateThread(srv.hWinEventThread,100);
            #pragma warning( pop )
            SafeCloseHandle(srv.hWinEventThread);
        }
        // Ошибка на экран уже выведена, нить уже завершена, закрыть дескриптор
        SafeCloseHandle(srv.hWinEventThread);
        iRc = CERR_WINHOOKNOTCREATED; goto wrap;
    }

    // Запустить нить обработки команд  
    srv.hServerThread = CreateThread( 
        NULL,              // no security attribute 
        0,                 // default stack size 
        ServerThread,      // thread proc
        NULL,              // thread parameter 
        0,                 // not suspended 
        &srv.dwServerThreadId);      // returns thread ID 

    if (srv.hServerThread == NULL) 
    {
        dwErr = GetLastError();
        wprintf(L"CreateThread(ServerThread) failed, ErrCode=0x%08X\n", dwErr); 
        iRc = CERR_CREATESERVERTHREAD; goto wrap;
    }

    // Запустить нить обработки событий (клавиатура, мышь, и пр.)
    srv.hInputThread = CreateThread( 
        NULL,              // no security attribute 
        0,                 // default stack size 
        InputThread,      // thread proc
        NULL,              // thread parameter 
        0,                 // not suspended 
        &srv.dwInputThreadId);      // returns thread ID 

    if (srv.hInputThread == NULL) 
    {
        dwErr = GetLastError();
        wprintf(L"CreateThread(InputThread) failed, ErrCode=0x%08X\n", dwErr); 
        iRc = CERR_CREATEINPUTTHREAD; goto wrap;
    }

    if (gbAttachMode) {
        HWND hGui = NULL, hDcWnd = NULL;
        UINT nMsg = RegisterWindowMessage(CONEMUMSG_ATTACH);
        DWORD dwStart = GetTickCount(), dwDelta = 0, dwCur = 0;
        // Если с первого раза не получится (GUI мог еще не загрузиться) пробуем еще
        while (!hDcWnd && dwDelta <= 5000) {
            while ((hGui = FindWindowEx(NULL, hGui, VirtualConsoleClassMain, NULL)) != NULL) {
                hDcWnd = (HWND)SendMessage(hGui, nMsg, (WPARAM)ghConWnd, (LPARAM)gnSelfPID);
                if (hDcWnd != NULL) {
                    break;
                }
            }
            if (hDcWnd) break;

            Sleep(500);
            dwCur = GetTickCount(); dwDelta = dwCur - dwStart;
        }
        if (!hDcWnd) {
            wprintf(L"Available ConEmu GUI window not found!\n");
            iRc = CERR_ATTACHFAILED; goto wrap;
        }
    }

wrap:
    return iRc;
}

// Завершить все нити и закрыть дескрипторы
void ServerDone(int aiRc)
{
    // Закрываем дескрипторы и выходим
    if (srv.dwWinEventThread && srv.hWinEventThread) {
        PostThreadMessage(srv.dwWinEventThread, WM_QUIT, 0, 0);
        // Подождем немножко, пока нить сама завершится
        if (WaitForSingleObject(srv.hWinEventThread, 500) != WAIT_OBJECT_0) {
            #pragma warning( push )
            #pragma warning( disable : 6258 )
            TerminateThread ( srv.hWinEventThread, 100 ); // раз корректно не хочет...
            #pragma warning( pop )
        }
        SafeCloseHandle(srv.hWinEventThread);
    }
    if (srv.hInputThread) {
        #pragma warning( push )
        #pragma warning( disable : 6258 )
        TerminateThread ( srv.hInputThread, 100 ); TODO("Сделать корректное завершение");
        #pragma warning( pop )
        SafeCloseHandle(srv.hInputThread);
    }

    if (srv.hServerThread) {
        #pragma warning( push )
        #pragma warning( disable : 6258 )
        TerminateThread ( srv.hServerThread, 100 ); TODO("Сделать корректное завершение");
        #pragma warning( pop )
        SafeCloseHandle(srv.hServerThread);
    }
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
    if (srv.hChangingSize) {
        SafeCloseHandle(srv.hChangingSize);
    }
    if (srv.hWinHook) {
        UnhookWinEvent(srv.hWinHook); srv.hWinHook = NULL;
    }
    
    if (srv.psChars) { free(srv.psChars); srv.psChars = NULL; }
    if (srv.pnAttrs) { free(srv.pnAttrs); srv.pnAttrs = NULL; }
    DeleteCriticalSection(&srv.csConBuf);
    DeleteCriticalSection(&srv.csProc);
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
          NULL);                    // default security attribute 

      _ASSERTE(hPipe != INVALID_HANDLE_VALUE);
      
      if (hPipe == INVALID_HANDLE_VALUE) 
      {
          dwErr = GetLastError();
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
   } 
   return 1; 
} 

DWORD WINAPI InputThread(LPVOID lpvParam) 
{ 
   BOOL fConnected, fSuccess; 
   //DWORD srv.dwServerThreadId;
   HANDLE hPipe = NULL; 
   DWORD dwErr = 0;
   
 
// The main loop creates an instance of the named pipe and 
// then waits for a client to connect to it. When the client 
// connects, a thread is created to handle communications 
// with that client, and the loop is repeated. 
 
   for (;;) 
   { 
      hPipe = CreateNamedPipe( 
          srv.szInputname,              // pipe name 
          PIPE_ACCESS_INBOUND,      // goes from client to server only
          PIPE_TYPE_MESSAGE |       // message type pipe 
          PIPE_READMODE_MESSAGE |   // message-read mode 
          PIPE_WAIT,                // blocking mode 
          PIPE_UNLIMITED_INSTANCES, // max. instances  
          sizeof(INPUT_RECORD),     // output buffer size 
          sizeof(INPUT_RECORD),     // input buffer size 
          0,                        // client time-out
          NULL);                    // default security attribute 

      _ASSERTE(hPipe != INVALID_HANDLE_VALUE);
      
      if (hPipe == INVALID_HANDLE_VALUE) 
      {
          dwErr = GetLastError();
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
 
      if (fConnected) 
      { 
          //TODO:
          DWORD cbBytesRead, cbWritten;
          INPUT_RECORD iRec; memset(&iRec,0,sizeof(iRec));
          while (fSuccess = ReadFile( 
             hPipe,        // handle to pipe 
             &iRec,        // buffer to receive data 
             sizeof(iRec), // size of buffer 
             &cbBytesRead, // number of bytes read 
             NULL))        // not overlapped I/O 
          {
              // предусмотреть возможность завершения нити
              if (iRec.EventType == 0xFFFF) {
                  SafeCloseHandle(hPipe);
                  break;
              }
              if (iRec.EventType) {
	              // проверить ENABLE_PROCESSED_INPUT в GetConsoleMode
	              #define ALL_MODIFIERS (LEFT_ALT_PRESSED|RIGHT_ALT_PRESSED|LEFT_CTRL_PRESSED|RIGHT_CTRL_PRESSED|SHIFT_PRESSED)
	              #define CTRL_MODIFIERS (LEFT_CTRL_PRESSED|RIGHT_CTRL_PRESSED)
				//if (iRec.EventType == KEY_EVENT && iRec.Event.KeyEvent.wVirtualKeyCode == 'C' &&
				//  (iRec.Event.KeyEvent.dwControlKeyState & LEFT_CTRL_PRESSED)
				// )
				//{
				//  /*DWORD dwMode = 0;
				//  HANDLE hConIn  = CreateFile(L"CONIN$", GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_READ,
				//	  0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
				//  GetConsoleMode(hConIn, &dwMode);
				//  SetConsoleMode(hConIn, dwMode&~ENABLE_PROCESSED_INPUT);
				//  CloseHandle(hConIn);*/
				//
				//  PostMessage(ghConWnd, WM_KEYDOWN, 17,0x001D0001);
				//  PostMessage(ghConWnd, WM_KEYDOWN, 67,0x002E0001);
				//  PostMessage(ghConWnd, WM_KEYUP, 67,0xC02E0001);
				//  PostMessage(ghConWnd, WM_KEYUP, 17,0xC01D0001);
				//  iRec.EventType = 0;
				//}
	              if (iRec.EventType == KEY_EVENT && iRec.Event.KeyEvent.bKeyDown &&
					  (iRec.Event.KeyEvent.wVirtualKeyCode == 'C' || iRec.Event.KeyEvent.wVirtualKeyCode == VK_CANCEL)
					 )
				  {
						BOOL lbRc = FALSE;
						DWORD dwEvent = (iRec.Event.KeyEvent.wVirtualKeyCode == 'C') ? CTRL_C_EVENT : CTRL_BREAK_EVENT;
					  //&& (srv.dwConsoleMode & ENABLE_PROCESSED_INPUT)

						//The SetConsoleMode function can disable the ENABLE_PROCESSED_INPUT mode for a console's input buffer, 
						//so CTRL+C is reported as keyboard input rather than as a signal. 
						// CTRL+BREAK is always treated as a signal
						if (
							(iRec.Event.KeyEvent.dwControlKeyState & CTRL_MODIFIERS) &&
							((iRec.Event.KeyEvent.dwControlKeyState & ALL_MODIFIERS) 
							== (iRec.Event.KeyEvent.dwControlKeyState & CTRL_MODIFIERS))
							)
						{
							//iRec.EventType = 0;

						#ifdef xxxxxx // так работает, но в "ping" приходит только CTRL_BREAK_EVENT
							BOOL lbRc = FALSE;
							DWORD dwEvent =
							(iRec.Event.KeyEvent.wVirtualKeyCode == 'C') ?
								CTRL_C_EVENT : CTRL_BREAK_EVENT;
							lbRc = GenerateConsoleCtrlEvent(dwEvent, 0);
						#endif

						#ifdef xxxxx // не работает
							BOOL lbRc = FALSE;
							//SendMessage(ghConWnd, WM_KEYDOWN, VK_CANCEL, 0x01460001);
							lbRc = GenerateConsoleCtrlEvent(
							(iRec.Event.KeyEvent.wVirtualKeyCode == 'C') ? 0 : 1,
							0);
						#endif


						#ifdef xxxxx // Это пересылает событие в ConEmuC, который работает как ComSpec
						DWORD *pdwPID = NULL;
						int nCount = GetProcessCount(&pdwPID);
						wchar_t szEvtName[128];
						for (int i=nCount-1; pdwPID && i>=0; i--) {
							wsprintf(szEvtName, (iRec.Event.KeyEvent.wVirtualKeyCode == 'C') ? CESIGNAL_C : CESIGNAL_BREAK, pdwPID[i]);
							HANDLE hEvent = OpenEvent(EVENT_MODIFY_STATE, FALSE, szEvtName);
							if (hEvent) {
								lbRc = TRUE; PulseEvent(hEvent); CloseHandle(hEvent); break;
							}
						}

						if (!lbRc)
						#endif

						// Вроде работает, Главное не запускать процесс с флагом CREATE_NEW_PROCESS_GROUP
						// иначе у микрософтовской консоли (WinXP SP3) сносит крышу, и она реагирует
						// на Ctrl-Break, но напрочь игнорирует Ctrl-C
						lbRc = GenerateConsoleCtrlEvent(dwEvent, 0);

						/*BOOL lbRc = FALSE;
						DWORD dwEvent =
						(iRec.Event.KeyEvent.wVirtualKeyCode == 'C') ?
						CTRL_C_EVENT : CTRL_BREAK_EVENT;
						lbRc = GenerateConsoleCtrlEvent(dwEvent, 0);*/

						/*for (i = nCount-1; !lbRc && i>=0; i--) {
						if (pdwPID[i] == 0 || pdwPID[i] == gnSelfPID) continue;
						lbRc = GenerateConsoleCtrlEvent(
						(iRec.Event.KeyEvent.wVirtualKeyCode == 'C') ? 0 : 1,
						pdwPID[i]);
						if (!lbRc) dwErr = GetLastError();
						}
						if (!lbRc)
						lbRc = GenerateConsoleCtrlEvent(
						(iRec.Event.KeyEvent.wVirtualKeyCode == 'C') ? 0 : 1,
						srv.dwProcessGroup);
						if (!lbRc)
						lbRc = GenerateConsoleCtrlEvent(dwEvent, 0);*/
						//iRec.EventType = 0;
						/*PostMessage(ghConWnd, WM_KEYDOWN, VK_CONTROL, 0);
						PostMessage(ghConWnd, WM_KEYDOWN, iRec.Event.KeyEvent.wVirtualKeyCode, 0);
						PostMessage(ghConWnd, WM_KEYUP, iRec.Event.KeyEvent.wVirtualKeyCode, 0);
						PostMessage(ghConWnd, WM_KEYUP, VK_CONTROL, 0);*/
					  }
		          }
              
		          if (iRec.EventType) {
	                  fSuccess = WriteConsoleInput(ghConIn, &iRec, 1, &cbWritten);
	                  _ASSERTE(fSuccess && cbWritten==1);
	              }
              }
              // next
              memset(&iRec,0,sizeof(iRec));
          }
      } 
      else 
        // The client could not connect, so close the pipe. 
         SafeCloseHandle(hPipe);
   } 
   return 1; 
} 
 
DWORD WINAPI InstanceThread(LPVOID lpvParam) 
{ 
    CESERVER_REQ in={0}, *pIn=NULL, *pOut=NULL;
    DWORD cbBytesRead, cbWritten, dwErr = 0;
    BOOL fSuccess; 
    HANDLE hPipe; 

    // The thread's parameter is a handle to a pipe instance. 
    hPipe = (HANDLE) lpvParam; 
 
    // Read client requests from the pipe. 
    memset(&in, 0, sizeof(in));
    fSuccess = ReadFile(
        hPipe,        // handle to pipe 
        &in,          // buffer to receive data 
        sizeof(in),   // size of buffer 
        &cbBytesRead, // number of bytes read 
        NULL);        // not overlapped I/O 

    if ((!fSuccess && ((dwErr = GetLastError()) != ERROR_MORE_DATA)) ||
            cbBytesRead < 12 || in.nSize < 12)
    {
        goto wrap;
    }

    if (in.nSize > cbBytesRead)
    {
        DWORD cbNextRead = 0;
        pIn = (CESERVER_REQ*)calloc(in.nSize, 1);
        if (!pIn)
            goto wrap;
        *pIn = in;
        fSuccess = ReadFile(
            hPipe,        // handle to pipe 
            ((LPBYTE)pIn)+cbBytesRead,  // buffer to receive data 
            in.nSize - cbBytesRead,   // size of buffer 
            &cbNextRead, // number of bytes read 
            NULL);        // not overlapped I/O 
        if (fSuccess)
            cbBytesRead += cbNextRead;
    }

    if (!GetAnswerToRequest(pIn ? *pIn : in, &pOut) || pOut==NULL) {
        goto wrap;
    }

    // Write the reply to the pipe. 
    fSuccess = WriteFile( 
        hPipe,        // handle to pipe 
        pOut,         // buffer to write from 
        pOut->nSize,  // number of bytes to write 
        &cbWritten,   // number of bytes written 
        NULL);        // not overlapped I/O 

    // освободить память
    free(pOut);

    //if (!fSuccess || pOut->nSize != cbWritten) break; 

// Flush the pipe to allow the client to read the pipe's contents 
// before disconnecting. Then disconnect the pipe, and close the 
// handle to this pipe instance. 

    FlushFileBuffers(hPipe); 
    DisconnectNamedPipe(hPipe);
wrap:
    SafeCloseHandle(hPipe); 

    return 1;
}

// На результат видимо придется не обращать внимания - не блокировать же выполнение
// сообщениями об ошибках... Возможно, что информацию об ошибке стоит записать
// в сам текстовый буфер.
DWORD ReadConsoleData(CESERVER_CHAR** pCheck /*= NULL*/, BOOL* pbDataChanged /*= NULL*/)
{
    WARNING("Если передан prcDataChanged(!=0 && !=1) - считывать из консоли только его, с учетом прокрутки");
    WARNING("Для оптимизации вызовов - можно считать, что левый и правый край совпадают с краями консоли");
    WARNING("И только если в одно чтение это не удалось - читать строками у учетом реальных краев");
    WARNING("Если передан prcChanged - в него заносится РЕАЛЬНО изменный прямоугольник, для последующей пересылки в GUI");
    BOOL lbRc = TRUE, lbChanged = FALSE;
    DWORD cbDataSize = 0; // Size in bytes of ONE buffer
    srv.bContentsChanged = FALSE;
    EnterCriticalSection(&srv.csConBuf);
    RECT rcReadRect = {0};

    SHORT TextWidth=0, TextHeight=0;
    DWORD TextLen=0;
    COORD coord;
    
    //TODO: а точно по srWindow ширину нужно смотреть?
    TextWidth = srv.sbi.dwSize.X; //max(srv.sbi.dwSize.X, (srv.sbi.srWindow.Right - srv.sbi.srWindow.Left + 1));
    WARNING("Для режима BufferHeight нужно считать по другому!");
    TextHeight = srv.sbi.dwSize.Y; //srv.sbi.srWindow.Bottom - srv.sbi.srWindow.Top + 1;
    TextLen = TextWidth * TextHeight;
    if (TextLen > srv.nBufCharCount) {
        lbChanged = TRUE;
        free(srv.psChars);
        srv.psChars = (wchar_t*)calloc(TextLen*2,sizeof(wchar_t));
        _ASSERTE(srv.psChars!=NULL);
        free(srv.pnAttrs);
        srv.pnAttrs = (WORD*)calloc(TextLen*2,sizeof(WORD));
        _ASSERTE(srv.pnAttrs!=NULL);
        if (srv.psChars && srv.pnAttrs) {
            srv.nBufCharCount = TextLen;
        } else {
            srv.nBufCharCount = 0; // ошибка выделения памяти
            lbRc = FALSE;
        }
    }
    //TODO: может все-таки из {0,srv.sbi.srWindow.Top} начинать нужно?
    coord.X = srv.sbi.srWindow.Left; coord.Y = srv.sbi.srWindow.Top;

    //TODO: перечитывать содержимое ТОЛЬКО по srv.bContentsChanged
    // ПЕРЕД чтением - сбросить srv.bContentsChanged в FALSE
    // буфер сделать глобальным, 
    // его использование - закрыть через CriticalSection
    
    if (srv.psChars && srv.pnAttrs) {
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
        if (!ReadConsoleOutputCharacter(ghConOut, srv.psChars, TextLen, coord, &nbActuallyRead) || !ReadConsoleOutputAttribute(ghConOut, srv.pnAttrs, TextLen, coord, &nbActuallyRead))
        {
            WORD* ConAttrNow = srv.pnAttrs; wchar_t* ConCharNow = srv.psChars;
            for(int y = 0; y < (int)TextHeight; y++, coord.Y++)
            {
                ReadConsoleOutputCharacter(ghConOut, ConCharNow, TextWidth, coord, &nbActuallyRead); ConCharNow += TextWidth;
                ReadConsoleOutputAttribute(ghConOut, ConAttrNow, TextWidth, coord, &nbActuallyRead); ConAttrNow += TextWidth;
            }
        }

        cbDataSize = TextLen * 2; // Size in bytes of ONE buffer

        if (!lbChanged) {
            if (memcmp(srv.psChars, srv.psChars+TextLen, TextLen*sizeof(wchar_t)))
                lbChanged = TRUE;
            else if (memcmp(srv.pnAttrs, srv.pnAttrs+TextLen, TextLen*sizeof(WORD)))
                lbChanged = TRUE;
        }

    } else {
        lbRc = FALSE;
    }

    if (pbDataChanged)
        *pbDataChanged = lbChanged;

    LeaveCriticalSection(&srv.csConBuf);
    return cbDataSize;
}

BOOL GetAnswerToRequest(CESERVER_REQ& in, CESERVER_REQ** out)
{
    BOOL lbRc = FALSE;
    int nContentsChanged = FALSE;

    switch (in.nCmd) {
        case CECMD_GETSHORTINFO:
        case CECMD_GETFULLINFO:
        {
            if (srv.szGuiPipeName[0] == 0) { // Серверный пайп в CVirtualConsole уже должен быть запущен
                wsprintf(srv.szGuiPipeName, CEGUIPIPENAME, L".", (DWORD)ghConWnd); // был gnSelfPID
            }
            nContentsChanged = (in.nCmd == CECMD_GETFULLINFO) ? 1 : 0;

            _ASSERT(ghConOut && ghConOut!=INVALID_HANDLE_VALUE);
            if (ghConOut==NULL || ghConOut==INVALID_HANDLE_VALUE)
                return FALSE;

            *out = CreateConsoleInfo(NULL, nContentsChanged);

            lbRc = TRUE;
        } break;
        case CECMD_SETSIZE:
        {
            int nOutSize = sizeof(CESERVER_REQ) + sizeof(CONSOLE_SCREEN_BUFFER_INFO) - 1;
            *out = (CESERVER_REQ*)calloc(nOutSize,1);
            if (*out == NULL) return FALSE;
            (*out)->nCmd = 0;
            (*out)->nSize = nOutSize;
            (*out)->nVersion = CESERVER_REQ_VER;
            if (in.nSize >= (12 + sizeof(USHORT)+sizeof(COORD)+sizeof(SMALL_RECT))) {
                USHORT nBufferHeight = 0;
                COORD  crNewSize = {0,0};
                SMALL_RECT rNewRect = {0};
                memmove(&nBufferHeight, in.Data, sizeof(USHORT));
                memmove(&crNewSize, in.Data+sizeof(USHORT), sizeof(COORD));
                memmove(&rNewRect, in.Data+sizeof(USHORT)+sizeof(COORD), sizeof(SMALL_RECT));

                (*out)->nCmd = CECMD_SETSIZE;

                SetConsoleSize(nBufferHeight, crNewSize, rNewRect, ":CECMD_SETSIZE");
            }
            PCONSOLE_SCREEN_BUFFER_INFO psc = (PCONSOLE_SCREEN_BUFFER_INFO)(*out)->Data;
            GetConsoleScreenBufferInfo(ghConOut, psc);
            WARNING("Игнорируем горизонтальный скроллинг");
            psc->srWindow.Left = 0; psc->srWindow.Right = psc->dwSize.X - 1;
            WARNING("Игнорируем вертикальный скроллинг для обычного режима");
            if (gnBufferHeight == 0) {
                psc->srWindow.Top = 0; psc->srWindow.Bottom = psc->dwSize.Y - 1;
            }
            lbRc = TRUE;
        } break;
        
        case CECMD_RECREATE:
        {
	        lbRc = FALSE; // возврат результата не требуется
			// как-то оно не особо хорошо...
#ifdef xxxxxx
			EnterCriticalSection(&srv.csProc);
	        int i, nCount = (srv.nProcesses.size()+5); // + небольшой резерв
	        _ASSERTE(nCount>0);
			if (nCount <= 0) {
				LeaveCriticalSection(&srv.csProc);
				break;
			}
	        
	        DWORD *pdwPID = (DWORD*)calloc(nCount, sizeof(DWORD));
	        _ASSERTE(pdwPID!=NULL);
	        
	        // Сначала сделаем копию списка, а то при приходе событий WinEvent может подраться за vector
	        std::vector<DWORD>::iterator iter = srv.nProcesses.begin();
	        i = 0;
	        while (iter != srv.nProcesses.end() && i < nCount) {
		        DWORD dwPID = *iter;
		        if (dwPID && dwPID != gnSelfPID) {
			        pdwPID[i] = dwPID;
			    }
			    iter ++;
	        }
			LeaveCriticalSection(&srv.csProc); // здесь мы его больше не меняем
	        
			gbInRecreateRoot = TRUE;

	        // Теперь можно килять
	        BOOL fSuccess = TRUE;
	        DWORD dwErr = 0;
	        for (i=nCount-1; i>=0; i--) {
		        if (pdwPID[i] == 0) continue;
		        HANDLE hProcess = OpenProcess ( PROCESS_TERMINATE, FALSE, pdwPID[i] );
		        if (hProcess == NULL) {
			        dwErr = GetLastError(); fSuccess = FALSE; break;
		        }
		        fSuccess = TerminateProcess(hProcess, 100);
		        dwErr = GetLastError();
		        CloseHandle(hProcess);
		        if (!fSuccess) break;
	        }
			free(pdwPID); pdwPID = NULL; // больше не требуется

	        if (fSuccess) {
		        // Clear console!
			    CONSOLE_SCREEN_BUFFER_INFO lsbi = {{0,0}};
			    HANDLE hCon = ghConOut ? ghConOut : GetStdHandle(STD_OUTPUT_HANDLE);
			    if (GetConsoleScreenBufferInfo(hCon, &lsbi)) {
				    DWORD dwWritten = 0; COORD cr = {0,0};
				    FillConsoleOutputCharacter(hCon, L' ', lsbi.dwSize.X*lsbi.dwSize.Y, cr, &dwWritten);
				    FillConsoleOutputAttribute(hCon, 7, lsbi.dwSize.X*lsbi.dwSize.Y, cr, &dwWritten);
		        }
		        // Create process!
			    PROCESS_INFORMATION pi; memset(&pi, 0, sizeof(pi));
			    STARTUPINFOW si; memset(&si, 0, sizeof(si)); si.cb = sizeof(si);
			    wprintf(L"\r\nRestarting root process: %s\r\n", gpszRunCmd);
			    fSuccess = CreateProcessW(NULL, gpszRunCmd, NULL,NULL, TRUE, NORMAL_PRIORITY_CLASS, NULL, NULL, &si, &pi);
			    dwErr = GetLastError();
			    if (!fSuccess)
			    {
			        wprintf (L"Can't create process, ErrCode=0x%08X!\n", dwErr);
					gbAlwaysConfirmExit = TRUE; // чтобы консоль сразу не схлопнулась, а было видно ошибку
					SetEvent(ghExitEvent); // завершение...
			    }
	        }
#endif
        } break;
    }
    
	if (gbInRecreateRoot) gbInRecreateRoot = FALSE;
    return lbRc;
}


DWORD WINAPI WinEventThread(LPVOID lpvParam)
{
    DWORD dwErr = 0;
    HANDLE hStartedEvent = (HANDLE)lpvParam;
    
    
    // "Ловим" все консольные события
    srv.hWinHook = SetWinEventHook(EVENT_CONSOLE_CARET,EVENT_CONSOLE_END_APPLICATION,
        NULL, WinEventProc, 0,0, WINEVENT_OUTOFCONTEXT /*| WINEVENT_SKIPOWNPROCESS ?*/);
    dwErr = GetLastError();
    if (!srv.hWinHook) {
        dwErr = GetLastError();
        wprintf(L"SetWinEventHook failed, ErrCode=0x%08X\n", dwErr); 
        SetEvent(hStartedEvent);
        return 100;
    }
    SetEvent(hStartedEvent); hStartedEvent = NULL; // здесь он более не требуется

    MSG lpMsg;
    while (GetMessage(&lpMsg, NULL, 0, 0))
    {
        //if (lpMsg.message == WM_QUIT) { // GetMessage возвращает FALSE при получении этого сообщения
        //  lbQuit = TRUE; break;
        //}
        TranslateMessage(&lpMsg);
        DispatchMessage(&lpMsg);
    }
    
    // Закрыть хук
    if (srv.hWinHook) {
        UnhookWinEvent(srv.hWinHook); srv.hWinHook = NULL;
    }
    
    return 0;
}

DWORD WINAPI RefreshThread(LPVOID lpvParam)
{
    DWORD dwErr = 0, nWait = 0;
    HANDLE hEvents[2] = {ghExitEvent, srv.hRefreshEvent};
    CONSOLE_CURSOR_INFO lci = {0}; // GetConsoleCursorInfo
    CONSOLE_SCREEN_BUFFER_INFO lsbi = {{0,0}}; // GetConsoleScreenBufferInfo
    BOOL lbQuit = FALSE;

    while (!lbQuit)
    {
        nWait = WAIT_TIMEOUT;

        // Подождать немножко
        TODO("Здесь можно увеличить время ожидания, если консоль неактивна");
        nWait = WaitForMultipleObjects ( 2, hEvents, FALSE, /*srv.bConsoleActive ? srv.nMainTimerElapse :*/ 10 );
        if (nWait == WAIT_OBJECT_0)
            break; // затребовано завершение нити

        if (nWait == WAIT_TIMEOUT) {
            // К сожалению, исключительно курсорные события не приходят (если консоль не в фокусе)
            if (GetConsoleScreenBufferInfo(ghConOut, &lsbi)) {
                if (memcmp(&srv.sbi.dwCursorPosition, &lsbi.dwCursorPosition, sizeof(lsbi.dwCursorPosition))) {
                    nWait = (WAIT_OBJECT_0+1);
                }
            }
            if ((nWait == WAIT_TIMEOUT) && GetConsoleCursorInfo(ghConOut, &lci)) {
                if (memcmp(&srv.ci, &lci, sizeof(srv.ci))) {
                    nWait = (WAIT_OBJECT_0+1);
                }
            }
            //if (srv.nLastUpdateTick) 
			{
	            DWORD dwCurTick = GetTickCount();
	            DWORD dwDelta = dwCurTick - srv.nLastUpdateTick;
	            TODO("По какой-то причине, иногда, в GUI приходят не все изменения консоли... попробуем так?");
	            if (dwDelta > 1000) {
		            srv.nLastUpdateTick = GetTickCount();
		            nWait = (WAIT_OBJECT_0+1);
	            }
            }
        }

        if (nWait == (WAIT_OBJECT_0+1)) {
            // Посмотреть, есть ли изменения в консоли
            ReloadFullConsoleInfo();
        }
    }
    
    return 0;
}

//Minimum supported client Windows 2000 Professional 
//Minimum supported server Windows 2000 Server 
void CALLBACK WinEventProc(HWINEVENTHOOK hWinEventHook, DWORD event, HWND hwnd, LONG idObject, LONG idChild, DWORD dwEventThread, DWORD dwmsEventTime)
{
    if (hwnd != ghConWnd) {
        _ASSERTE(hwnd); // по идее, тут должен быть хэндл консольного окна, проверим
        return;
    }

    BOOL bNeedConAttrBuf = FALSE;
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
        wsprintfW(szDbg, L"EVENT_CONSOLE_START_APPLICATION(PID=%i%s)\n", idObject, (idChild == CONSOLE_APPLICATION_16BIT) ? L" 16bit" : L"");
        OutputDebugString(szDbg);
        #endif

        if (idObject != gnSelfPID) {
            EnterCriticalSection(&srv.csProc);
            srv.nProcesses.push_back(idObject);
            LeaveCriticalSection(&srv.csProc);

            if (idChild == CONSOLE_APPLICATION_16BIT) {
                DWORD ntvdmPID = idObject;
                dwActiveFlags |= CES_NTVDM;
                SetPriorityClass(GetCurrentProcess(), HIGH_PRIORITY_CLASS);
            }
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
        wsprintfW(szDbg, L"EVENT_CONSOLE_END_APPLICATION(PID=%i%s)\n", idObject, (idChild == CONSOLE_APPLICATION_16BIT) ? L" 16bit" : L"");
        OutputDebugString(szDbg);
        #endif

        if (idObject != gnSelfPID) {
            std::vector<DWORD>::iterator iter;
            EnterCriticalSection(&srv.csProc);
            for (iter=srv.nProcesses.begin(); iter!=srv.nProcesses.end(); iter++) {
                if (idObject == *iter) {
                    srv.nProcesses.erase(iter);
                    if (idChild == CONSOLE_APPLICATION_16BIT) {
                        DWORD ntvdmPID = idObject;
                        dwActiveFlags &= ~CES_NTVDM;
                        //TODO: возможно стоит прибить процесс NTVDM?
                        SetPriorityClass(GetCurrentProcess(), NORMAL_PRIORITY_CLASS);
                    }
                    // Процессов в консоли не осталось?
                    if (srv.nProcesses.size() == 0 && !gbInRecreateRoot) {
                        LeaveCriticalSection(&srv.csProc);
                        SetEvent(ghExitEvent);
                        return;
                    }
                    break;
                }
            }
            LeaveCriticalSection(&srv.csProc);
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
            COORD crStart, crEnd; memmove(&crStart, &idObject, sizeof(idObject)); memmove(&crEnd, &idChild, sizeof(idChild));
            wsprintfW(szDbg, L"EVENT_CONSOLE_UPDATE_REGION({%i, %i} - {%i, %i})\n", crStart.X,crStart.Y, crEnd.X,crEnd.Y);
            OutputDebugString(szDbg);
            #endif
            //bNeedConAttrBuf = TRUE;
            //// Перечитать размер, положение курсора, и пр.
            //ReloadConsoleInfo();
            srv.bNeedFullReload = TRUE;
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
            OutputDebugString(szDbg);
            #endif
            //bNeedConAttrBuf = TRUE;
            //// Перечитать размер, положение курсора, и пр.
            //if (!ReloadConsoleInfo())
            //  return;
            srv.bNeedFullReload = TRUE;
            SetEvent(srv.hRefreshEvent);
        }
        return; // Обновление по событию в нити

    case EVENT_CONSOLE_UPDATE_SIMPLE: //0x4003
        {
        //A single character has changed.
        //The idObject parameter is a COORD structure that specifies the character that has changed.
        //Warning! В писании от  микрософта тут ошибка!
        //The idChild parameter specifies the character in the low word and the character attributes in the high word.
            memmove(&ch.crStart, &idObject, sizeof(idObject));
            ch.crEnd = ch.crStart;
            ch.data[0] = (WCHAR)LOWORD(idChild); ch.data[1] = HIWORD(idChild);
            #ifdef _DEBUG
            wsprintfW(szDbg, L"EVENT_CONSOLE_UPDATE_SIMPLE({%i, %i} '%c'(\\x%04X) A=%i)\n", ch.crStart.X,ch.crStart.Y, ch.data[0], ch.data[0], ch.data[1]);
            OutputDebugString(szDbg);
            #endif
            // Перечитать размер, положение курсора, и пр.
            if (!ReloadConsoleInfo()) { // Если Layout не поменялся
                TODO("Если Reload==FALSE, и ch не меняет предыдущего символа/атрибута в srv.psChars/srv.pnAttrs - сразу выйти");
                
                int nIdx = ch.crStart.X + ch.crStart.Y * srv.sbi.dwSize.X;
                _ASSERTE(nIdx>=0 && (DWORD)nIdx<srv.nBufCharCount);
                if (srv.psChars[nIdx] == (wchar_t)ch.data[0] && srv.pnAttrs[nIdx] == ch.data[1])
                    return; // данные не менялись
            }
            // Применить
            int nIdx = ch.crStart.X + ch.crStart.Y * srv.sbi.dwSize.X;
            srv.psChars[nIdx] = (wchar_t)ch.data[0];
            srv.psChars[nIdx+srv.nBufCharCount] = (wchar_t)ch.data[0];
            // И отправить
            ReloadFullConsoleInfo(&ch);
        } return;

    case EVENT_CONSOLE_CARET: //0x4001
        {
        //Warning! WinXPSP3. Это событие проходит ТОЛЬКО если консоль в фокусе. 
        //         А с ConEmu она НИКОГДА не в фокусе, так что курсор не обновляется.
        //The console caret has moved.
        //The idObject parameter is one or more of the following values:
        //      CONSOLE_CARET_SELECTION or CONSOLE_CARET_VISIBLE.
        //The idChild parameter is a COORD structure that specifies the cursor's current position.
            #ifdef _DEBUG
            COORD crWhere; memmove(&crWhere, &idChild, sizeof(idChild));
            wsprintfW(szDbg, L"EVENT_CONSOLE_CARET({%i, %i} Sel=%c, Vis=%c\n", crWhere.X,crWhere.Y, 
                ((idObject & CONSOLE_CARET_SELECTION)==CONSOLE_CARET_SELECTION) ? L'Y' : L'N',
                ((idObject & CONSOLE_CARET_VISIBLE)==CONSOLE_CARET_VISIBLE) ? L'Y' : L'N');
            OutputDebugString(szDbg);
            #endif
            SetEvent(srv.hRefreshEvent);
            // Перечитать размер, положение курсора, и пр.
            //if (ReloadConsoleInfo())
            //  return;
        } return;

    case EVENT_CONSOLE_LAYOUT: //0x4005
        {
        //The console layout has changed.
            #ifdef _DEBUG
            OutputDebugString(L"EVENT_CONSOLE_LAYOUT\n");
            #endif
            //bNeedConAttrBuf = TRUE;
            TODO("А когда вообще это событие вызывается?");
            //// Перечитать размер, положение курсора, и пр.
            //if (!ReloadConsoleInfo())
            //  return;
            srv.bNeedFullReload = TRUE;
            SetEvent(srv.hRefreshEvent);
        }
        return; // Обновление по событию в нити
    }


    //// Дальнейшее имеет смысл только если CVirtualConsole уже запустил серверный пайп
    //if (srv.szGuiPipeName[0] == 0)
    //  return;

    //CESERVER_REQ* pOut = 
    //  CreateConsoleInfo (
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

    //free ( pOut );
}

void SendConsoleChanges(CESERVER_REQ* pOut)
{
    //srv.szGuiPipeName инициализируется только после того, как GUI узнал хэндл консольного окна
    if (srv.szGuiPipeName[0] == 0)
        return;

    HANDLE hPipe = NULL;
    DWORD dwErr = 0, dwMode = 0;
    BOOL fSuccess = FALSE;

    // Try to open a named pipe; wait for it, if necessary. 
    while (1) 
    { 
        hPipe = CreateFile( 
            srv.szGuiPipeName,  // pipe name 
            GENERIC_WRITE, 
            0,              // no sharing 
            NULL,           // default security attributes
            OPEN_EXISTING,  // opens existing pipe 
            0,              // default attributes 
            NULL);          // no template file 

        // Break if the pipe handle is valid. 
        if (hPipe != INVALID_HANDLE_VALUE) 
            break; // OK, открыли

        // Exit if an error other than ERROR_PIPE_BUSY occurs. 
        dwErr = GetLastError();
        if (dwErr != ERROR_PIPE_BUSY) 
            return;

        // All pipe instances are busy, so wait for 100 ms.
        if (!WaitNamedPipe(srv.szGuiPipeName, 100) ) 
            return;
    }

    // The pipe connected; change to message-read mode. 
    dwMode = PIPE_READMODE_MESSAGE; 
    fSuccess = SetNamedPipeHandleState( 
        hPipe,    // pipe handle 
        &dwMode,  // new pipe mode 
        NULL,     // don't set maximum bytes 
        NULL);    // don't set maximum time 
    _ASSERT(fSuccess);
    if (!fSuccess)
        return;


    // Собственно запись в пайп
    DWORD dwWritten = 0;
    fSuccess = WriteFile ( hPipe, pOut, pOut->nSize, &dwWritten, NULL);
    _ASSERTE(fSuccess && dwWritten == pOut->nSize);
}

CESERVER_REQ* CreateConsoleInfo(CESERVER_CHAR* pCharOnly, int bCharAttrBuff)
{
    CESERVER_REQ* pOut = NULL;
    DWORD dwAllSize = sizeof(CESERVER_REQ), nSize;

    // 1
    HWND hWnd = NULL;
    dwAllSize += (nSize=sizeof(hWnd)); _ASSERTE(nSize==4);
    // 2
    // Тут нужно вставить GetTickCount(), чтобы GUI случайно не 'обновил' экран старыми данными
    dwAllSize += (nSize=sizeof(DWORD));
    // 3
    // во время чтения содержимого консоли может увеличиться количество процессов
    // поэтому РЕАЛЬНОЕ количество - выставим после чтения и CriticalSection(srv.csProc);
    //EnterCriticalSection(&srv.csProc);
    //DWORD dwProcCount = srv.nProcesses.size()+20;
    //LeaveCriticalSection(&srv.csProc);
    //dwAllSize += sizeof(DWORD)*(dwProcCount+1); // PID процессов + их количество
    dwAllSize += (nSize=sizeof(DWORD)); // список процессов формируется в GUI, так что пока - просто 0 (Reserved)
    // 4
    //DWORD srv.dwSelRc = 0; CONSOLE_SELECTION_INFO srv.sel = {0}; // GetConsoleSelectionInfo
    dwAllSize += sizeof(srv.dwSelRc)+((srv.dwSelRc==0) ? (nSize=sizeof(srv.sel)) : 0);
    // 5
    //DWORD srv.dwCiRc = 0; CONSOLE_CURSOR_INFO srv.ci = {0}; // GetConsoleCursorInfo
    dwAllSize += sizeof(srv.dwCiRc)+((srv.dwCiRc==0) ? (nSize=sizeof(srv.ci)) : 0);
    // 6, 7, 8
    //DWORD srv.dwConsoleCP=0, srv.dwConsoleOutputCP=0, srv.dwConsoleMode=0;
    dwAllSize += 3*sizeof(DWORD);
    // 9
    //DWORD srv.dwSbiRc = 0; CONSOLE_SCREEN_BUFFER_INFO srv.sbi = {{0,0}}; // GetConsoleScreenBufferInfo
    //if (!GetConsoleScreenBufferInfo(ghConOut, &srv.sbi)) { srv.dwSbiRc = GetLastError(); if (!srv.dwSbiRc) srv.dwSbiRc = -1; }
    dwAllSize += sizeof(srv.dwSbiRc)+((srv.dwSbiRc==0) ? (nSize=sizeof(srv.sbi)) : 0);
    // 10
    dwAllSize += sizeof(DWORD) + (pCharOnly ? (nSize=sizeof(CESERVER_CHAR)) : 0);
    // 11
    DWORD OneBufferSize = 0;
    dwAllSize += sizeof(DWORD);
    if (bCharAttrBuff) {
        TODO("Доработать для передачи только изменившегося прямоугольника и BufferHeight");
        if (bCharAttrBuff == 2 && OneBufferSize == (srv.nBufCharCount*2)) {
            _ASSERTE(srv.nBufCharCount>0);
            OneBufferSize = srv.nBufCharCount*2;
        } else {
            OneBufferSize = ReadConsoleData(); // returns size in bytes of ONE buffer
        }
        #ifdef _DEBUG
        if (gnBufferHeight == 0) {
            _ASSERTE(OneBufferSize == (srv.sbi.dwSize.X*srv.sbi.dwSize.Y*2));
        }
        #endif
        if (OneBufferSize) {
            dwAllSize += OneBufferSize * 2;
        }
    }

    // Выделение буфера
    pOut = (CESERVER_REQ*)calloc(dwAllSize+sizeof(CESERVER_REQ), 1); // размер считали в байтах
    _ASSERT(pOut!=NULL);
    if (pOut == NULL) {
        return FALSE;
    }

    // инициализация
    pOut->nSize = dwAllSize;
    pOut->nCmd = bCharAttrBuff ? CECMD_GETFULLINFO : CECMD_GETSHORTINFO;
    pOut->nVersion = CESERVER_REQ_VER;

    // поехали
    LPBYTE lpCur = (LPBYTE)(pOut->Data);

    // 1
    hWnd = GetConsoleWindow();
    _ASSERTE(hWnd == ghConWnd);
    *((DWORD*)lpCur) = (DWORD)hWnd;
    lpCur += sizeof(hWnd);

    // 2
    *((DWORD*)lpCur) = GetTickCount();
    lpCur += sizeof(DWORD);

    // 3
    // во время чтения содержимого консоли может увеличиться количество процессов
    // поэтому РЕАЛЬНОЕ количество - выставим после чтения и CriticalSection(srv.csProc);
    *((DWORD*)lpCur) = 0; lpCur += sizeof(DWORD);
    //EnterCriticalSection(&srv.csProc);
    //DWORD dwTestCount = srv.nProcesses.size();
    //_ASSERTE(dwTestCount<=dwProcCount);
    //if (dwTestCount < dwProcCount) dwProcCount = dwTestCount;
    //*((DWORD*)lpCur) = dwProcCount; lpCur += sizeof(DWORD);
    //for (DWORD n=0; n<dwProcCount; n++) {
    //  *((DWORD*)lpCur) = srv.nProcesses[n];
    //  lpCur += sizeof(DWORD);
    //}
    //LeaveCriticalSection(&srv.csProc);

    // 4
    nSize=sizeof(srv.sel); *((DWORD*)lpCur) = (srv.dwSelRc == 0) ? nSize : 0; lpCur += sizeof(DWORD);
    if (srv.dwSelRc == 0) {
        memmove(lpCur, &srv.sel, nSize); lpCur += nSize;
    }

    // 5
    nSize=sizeof(srv.ci); *((DWORD*)lpCur) = (srv.dwCiRc == 0) ? nSize : 0; lpCur += sizeof(DWORD);
    if (srv.dwCiRc == 0) {
        memmove(lpCur, &srv.ci, nSize); lpCur += nSize;
    }

    // 6
    *((DWORD*)lpCur) = srv.dwConsoleCP; lpCur += sizeof(DWORD);
    // 7
    *((DWORD*)lpCur) = srv.dwConsoleOutputCP; lpCur += sizeof(DWORD);
    // 8
    *((DWORD*)lpCur) = srv.dwConsoleMode; lpCur += sizeof(DWORD);

    // 9
    //if (!GetConsoleScreenBufferInfo(ghConOut, &srv.sbi)) { srv.dwSbiRc = GetLastError(); if (!srv.dwSbiRc) srv.dwSbiRc = -1; }
    nSize=sizeof(srv.sbi); *((DWORD*)lpCur) = (srv.dwSbiRc == 0) ? nSize : 0; lpCur += sizeof(DWORD);
    if (srv.dwSbiRc == 0) {
        memmove(lpCur, &srv.sbi, nSize); lpCur += nSize;
    }

    // 10
    *((DWORD*)lpCur) = pCharOnly ? sizeof(CESERVER_CHAR) : 0; lpCur += sizeof(DWORD);
    if (pCharOnly) {
        memmove(lpCur, pCharOnly, sizeof(CESERVER_CHAR)); lpCur += (nSize=sizeof(CESERVER_CHAR));
    }

    // 11 - здесь будет 0, если текст в консоли не менялся
    *((DWORD*)lpCur) = OneBufferSize; lpCur += sizeof(DWORD);
    if (OneBufferSize && OneBufferSize!=-1) {
        memmove(lpCur, srv.psChars, OneBufferSize); lpCur += OneBufferSize;
        memmove(lpCur, srv.pnAttrs, OneBufferSize); lpCur += OneBufferSize;
    }
    
    if (ghLogSize) {
        static COORD scr_Last = {-1,-1};
        if (scr_Last.X != srv.sbi.dwSize.X || scr_Last.Y != srv.sbi.dwSize.Y) {
            LogSize(&srv.sbi.dwSize, ":CreateConsoleInfo(query only)");
            scr_Last = srv.sbi.dwSize;
        }
    }

    return pOut;
}

BOOL ReloadConsoleInfo()
{
    BOOL lbChanged = FALSE;
    CONSOLE_SELECTION_INFO lsel = {0}; // GetConsoleSelectionInfo
    CONSOLE_CURSOR_INFO lci = {0}; // GetConsoleCursorInfo
    DWORD ldwConsoleCP=0, ldwConsoleOutputCP=0, ldwConsoleMode=0;
    CONSOLE_SCREEN_BUFFER_INFO lsbi = {{0,0}}; // GetConsoleScreenBufferInfo

    if (!GetConsoleSelectionInfo(&lsel)) { srv.dwSelRc = GetLastError(); if (!srv.dwSelRc) srv.dwSelRc = -1; } else {
        srv.dwSelRc = 0;
        if (memcmp(&srv.sel, &lsel, sizeof(srv.sel))) {
            srv.sel = lsel;
            lbChanged = TRUE;
        }
    }

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

    if (!GetConsoleScreenBufferInfo(ghConOut, &lsbi)) { srv.dwSbiRc = GetLastError(); if (!srv.dwSbiRc) srv.dwSbiRc = -1; } else {
        srv.dwSbiRc = 0;
        WARNING("Игнорируем горизонтальный скроллинг");
        lsbi.srWindow.Left = 0; lsbi.srWindow.Right = lsbi.dwSize.X - 1;
        WARNING("Игнорируем вертикальный скроллинг для обычного режима");
        if (gnBufferHeight == 0) {
            lsbi.srWindow.Top = 0; lsbi.srWindow.Bottom = lsbi.dwSize.Y - 1;
        }
        if (memcmp(&srv.sbi, &lsbi, sizeof(srv.sbi))) {
            srv.sbi = lsbi;
            lbChanged = TRUE;
        }
    }

    return lbChanged;
}

void ReloadFullConsoleInfo(CESERVER_CHAR* pCharOnly/*=NULL*/)
{
    CESERVER_CHAR* pCheck = NULL;
    BOOL lbInfChanged = FALSE, lbDataChanged = FALSE;
    DWORD dwBufSize = 0;

    lbInfChanged = ReloadConsoleInfo();

    if (srv.bNeedFullReload) {
        srv.bNeedFullReload = FALSE;
        dwBufSize = ReadConsoleData(&pCheck, &lbDataChanged);
        if (lbDataChanged && pCheck != NULL) {
            pCharOnly = pCheck;
            lbDataChanged = FALSE; // Изменения передадутся через pCharOnly
        }
    }

    if (lbInfChanged || lbDataChanged || pCharOnly) {
        TODO("Можно отслеживать реально изменившийся прямоугольник и передавать только его");
        CESERVER_REQ* pOut = CreateConsoleInfo(lbDataChanged ? NULL : pCharOnly, lbDataChanged ? 2 : 0);
        _ASSERTE(pOut!=NULL); if (!pOut) return;
        SendConsoleChanges(pOut);
        free(pOut);
        // Запомнить последнее отправленное
        if (lbDataChanged && srv.nBufCharCount) {
            MCHKHEAP
            memmove(srv.psChars+srv.nBufCharCount, srv.psChars, srv.nBufCharCount*sizeof(wchar_t));
            memmove(srv.pnAttrs+srv.nBufCharCount, srv.pnAttrs, srv.nBufCharCount*sizeof(WORD));
            MCHKHEAP
        }
    }
    
    //
    srv.nLastUpdateTick = GetTickCount();

    if (pCheck) free(pCheck);
}

BOOL SetConsoleSize(USHORT BufferHeight, COORD crNewSize, SMALL_RECT rNewRect, LPCSTR asLabel)
{
    _ASSERTE(ghConWnd);
    if (!ghConWnd) return FALSE;
    
    if (ghLogSize) LogSize(&crNewSize, asLabel);

    _ASSERTE(crNewSize.X>=MIN_CON_WIDTH && crNewSize.Y>=MIN_CON_HEIGHT);

    // Проверка минимального размера
    if (crNewSize.X</*4*/MIN_CON_WIDTH)
        crNewSize.X = /*4*/MIN_CON_WIDTH;
    if (crNewSize.Y</*3*/MIN_CON_HEIGHT)
        crNewSize.Y = /*3*/MIN_CON_HEIGHT;

    gnBufferHeight = BufferHeight;
    gcrBufferSize = crNewSize;

    RECT rcConPos = {0};
    GetWindowRect(ghConWnd, &rcConPos);

    BOOL lbRc = TRUE;
    DWORD nWait = 0;
    BOOL lbNeedChange = TRUE;
    CONSOLE_SCREEN_BUFFER_INFO csbi = {{0,0}};
    if (GetConsoleScreenBufferInfo(ghConOut, &csbi)) {
        lbNeedChange = (csbi.dwSize.X != crNewSize.X) || (csbi.dwSize.Y != crNewSize.Y);
    }

    if (srv.hChangingSize) { // во время запуска ConEmuC
        nWait = WaitForSingleObject(srv.hChangingSize, 300);
        _ASSERTE(nWait == WAIT_OBJECT_0);
        ResetEvent(srv.hChangingSize);
    }

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
            MoveWindow(ghConWnd, rcConPos.left, rcConPos.top, GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN), 1);
        }

    } else {
        // Начался ресайз для BufferHeight
		COORD crHeight = {crNewSize.X, BufferHeight};

		GetWindowRect(ghConWnd, &rcConPos);
        MoveWindow(ghConWnd, rcConPos.left, rcConPos.top, 1, 1, 1);
        lbRc = SetConsoleScreenBufferSize(ghConOut, crHeight); // а не crNewSize - там "оконные" размеры
        //окошко раздвигаем только по ширине!
		RECT rcCurConPos = {0};
		GetWindowRect(ghConWnd, &rcCurConPos); //X-Y новые, но высота - старая
        MoveWindow(ghConWnd, rcCurConPos.left, rcCurConPos.top, GetSystemMetrics(SM_CXSCREEN), rcConPos.bottom-rcConPos.top, 1);

		if (rNewRect.Right && rNewRect.Bottom) {
            SetConsoleWindowInfo(ghConOut, TRUE, &rNewRect);
		}
    }

    if (srv.hChangingSize) { // во время запуска ConEmuC
        SetEvent(srv.hChangingSize);
    }

    return lbRc;
}

BOOL WINAPI HandlerRoutine(DWORD dwCtrlType)
{
    /*SafeCloseHandle(ghLogSize);
    if (wpszLogSizeFile) {
        DeleteFile(wpszLogSizeFile);
        free(wpszLogSizeFile); wpszLogSizeFile = NULL;
    }*/

    return TRUE;
}

int GetProcessCount(DWORD **rpdwPID)
{
	DWORD dwErr = 0; BOOL lbRc = FALSE;
	DWORD *pdwPID = NULL; int nCount = 0, i;
	EnterCriticalSection(&srv.csProc);
	nCount = srv.nProcesses.size();
	if (nCount > 0 && rpdwPID) {
		pdwPID = (DWORD*)calloc(nCount, sizeof(DWORD));
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
}
