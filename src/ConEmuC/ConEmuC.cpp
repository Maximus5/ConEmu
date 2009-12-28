
#ifdef _DEBUG
//  Раскомментировать, чтобы сразу после запуска процесса (conemuc.exe) показать MessageBox, чтобы прицепиться дебаггером
//  #define SHOW_STARTED_MSGBOX
  #define SHOW_COMSPEC_STARTED_MSGBOX
//  #define SHOW_STARTED_ASSERT
#elif defined(__GNUC__)
//  Раскомментировать, чтобы сразу после запуска процесса (conemuc.exe) показать MessageBox, чтобы прицепиться дебаггером
//  #define SHOW_STARTED_MSGBOX
#else
//
#endif

#include "ConEmuC.h"

WARNING("Обязательно после запуска сделать SetForegroundWindow на GUI окно, если в фокусе консоль");
WARNING("Обязательно получить код и имя родительского процесса");

WARNING("При запуске как ComSpec получаем ошибку: {crNewSize.X>=MIN_CON_WIDTH && crNewSize.Y>=MIN_CON_HEIGHT}");
//E:\Source\FARUnicode\trunk\unicode_far\Debug.32.vc\ConEmuC.exe /c tools\gawk.exe -f .\scripts\gendate.awk


#ifdef _DEBUG
wchar_t gszDbgModLabel[6] = {0};
#endif

WARNING("!!!! Пока можно при появлении события запоминать текущий тик");
// и проверять его в RefreshThread. Если он не 0 - и дельта больше (100мс?)
// то принудительно перечитать консоль и сбросить тик в 0.


WARNING("Наверное все-же стоит производить периодические чтения содержимого консоли, а не только по событию");

WARNING("Стоит именно здесь осуществлять проверку живости GUI окна (если оно было). Ведь может быть запущен не far, а CMD.exe");

WARNING("Если GUI умер, или не подцепился по таймауту - показать консольное окно и наверное установить шрифт поболе");

WARNING("В некоторых случаях не срабатывает ни EVENT_CONSOLE_UPDATE_SIMPLE ни EVENT_CONSOLE_UPDATE_REGION");
// Пример. Запускаем cmd.exe. печатаем какую-то муйню в командной строке и нажимаем 'Esc'
// При Esc никаких событий ВООБЩЕ не дергается, а экран в консоли изменился!



FGetConsoleKeyboardLayoutName pfnGetConsoleKeyboardLayoutName = NULL;
FGetConsoleProcessList pfnGetConsoleProcessList = NULL;
FDebugActiveProcessStop pfnDebugActiveProcessStop = NULL;
FDebugSetProcessKillOnExit pfnDebugSetProcessKillOnExit = NULL;


/* Console Handles */
MConHandle ghConIn ( L"CONIN$" );
MConHandle ghConOut ( L"CONOUT$" );


/*  Global  */
DWORD   gnSelfPID = 0;
//HANDLE  ghConIn = NULL, ghConOut = NULL;
HWND    ghConWnd = NULL;
HWND    ghConEmuWnd = NULL; // Root! window
HANDLE  ghExitQueryEvent = NULL;
HANDLE  ghQuitEvent = NULL;
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
HANDLE ghFarInExecuteEvent;
#endif

RunMode gnRunMode = RM_UNDEFINED;

BOOL gbNoCreateProcess = FALSE;
BOOL gbDebugProcess = FALSE;
int  gnCmdUnicodeMode = 0;
BOOL gbRootIsCmdExe = TRUE;


SrvInfo srv = {0};

#pragma pack(push, 1)
CESERVER_CONSAVE* gpStoredOutput = NULL;
#pragma pack(pop)

CmdInfo cmd = {0};

COORD gcrBufferSize = {80,25};
BOOL  gbParmBufferSize = FALSE;
SHORT gnBufferHeight = 0;
wchar_t* gpszPrevConTitle = NULL;

HANDLE ghLogSize = NULL;
wchar_t* wpszLogSizeFile = NULL;


BOOL gbInRecreateRoot = FALSE;




// 04.12.2009 Maks + "__cdecl"
int __cdecl main()
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
        _printf("ghConWnd==NULL, ErrCode=0x%08X\n", dwErr);
        iRc = CERR_GETCONSOLEWINDOW; goto wrap;
    }
    // PID
    gnSelfPID = GetCurrentProcessId();
    gdwMainThreadId = GetCurrentThreadId();

#ifdef _DEBUG
	// Это событие дергается в отладочной (мной поправленной) версии фара
	wchar_t szEvtName[64]; wsprintf(szEvtName, L"FARconEXEC:%08X", (DWORD)ghConWnd);
	ghFarInExecuteEvent = CreateEvent(0, TRUE, FALSE, szEvtName);
#endif

    
#if defined(SHOW_STARTED_MSGBOX) || defined(SHOW_COMSPEC_STARTED_MSGBOX)
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
    ghExitQueryEvent = CreateEvent(NULL, TRUE/*используется в нескольких нитях, manual*/, FALSE, NULL);
    if (!ghExitQueryEvent) {
        dwErr = GetLastError();
        _printf("CreateEvent() failed, ErrCode=0x%08X\n", dwErr); 
        iRc = CERR_EXITEVENT; goto wrap;
    }
    ResetEvent(ghExitQueryEvent);
    ghQuitEvent = CreateEvent(NULL, TRUE/*используется в нескольких нитях, manual*/, FALSE, NULL);
    if (!ghQuitEvent) {
        dwErr = GetLastError();
        _printf("CreateEvent() failed, ErrCode=0x%08X\n", dwErr); 
        iRc = CERR_EXITEVENT; goto wrap;
    }
    ResetEvent(ghQuitEvent);

    // Дескрипторы
    //ghConIn  = CreateFile(L"CONIN$", GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_READ,
    //            0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
    if ((HANDLE)ghConIn == INVALID_HANDLE_VALUE) {
        dwErr = GetLastError();
        _printf("CreateFile(CONIN$) failed, ErrCode=0x%08X\n", dwErr); 
        iRc = CERR_CONINFAILED; goto wrap;
    }
    // Дескрипторы
    //ghConOut = CreateFile(L"CONOUT$", GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_READ,
    //            0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
    if ((HANDLE)ghConOut == INVALID_HANDLE_VALUE) {
        dwErr = GetLastError();
        _printf("CreateFile(CONOUT$) failed, ErrCode=0x%08X\n", dwErr); 
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

		#ifdef _DEBUG
		if (ghFarInExecuteEvent && wcsstr(gpszRunCmd,L"far.exe"))
			ResetEvent(ghFarInExecuteEvent);
		#endif

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
		
        _printf("Can't create process, ErrCode=0x%08X, Description:\n", dwErr);
        _wprintf((lpMsgBuf == NULL) ? L"<Unknown error>" : lpMsgBuf);
        _printf("\nCommand to be executed:\n");
        _wprintf(gpszRunCmd);
        _printf("\n");
        
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
		if (srv.nProcessCount && !srv.bDebuggerActive) {
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
        nWait = WaitForSingleObject(ghExitQueryEvent, CHECK_ANTIVIRUS_TIMEOUT); //Запуск процесса наверное может задержать антивирус
        if (nWait != WAIT_OBJECT_0) { // Если таймаут
            iRc = srv.nProcessCount;
            // И процессов в консоли все еще нет
            if (iRc == 1 && !srv.bDebuggerActive) {
                _printf ("Process was not attached to console. Is it GUI?\nCommand to be executed:\n");
                _wprintf (gpszRunCmd);
                _printf ("\n");
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
		nWait = WAIT_TIMEOUT;
		if (!srv.bDebuggerActive) {
	        while (nWait == WAIT_TIMEOUT) {
				nWait = WaitForSingleObject(ghExitQueryEvent, 10);
			}
		} else {
			while (nWait == WAIT_TIMEOUT) {
				ProcessDebugEvent();
				nWait = WaitForSingleObject(ghExitQueryEvent, 0);
			}
			gbAlwaysConfirmExit = TRUE;
		}
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
	MessageBox(GetConsoleWindow(), L"Finalizing", (gnRunMode == RM_SERVER) ? L"ConEmuC.Server" : L"ConEmuC.ComSpec", 0);
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
            if (nCount > 1 || srv.bDebuggerActive) {
                // Процесс таки запустился!
                goto wait;
            }
        }
    }

    // На всякий случай - выставим событие
    if (ghExitQueryEvent) SetEvent(ghExitQueryEvent);
    
    // Завершение RefreshThread, InputThread
    if (ghQuitEvent) SetEvent(ghQuitEvent);
    
    
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

#ifdef _DEBUG
	SafeCloseHandle(ghFarInExecuteEvent);
#endif
    
    if (gpszRunCmd) { delete gpszRunCmd; gpszRunCmd = NULL; }
    
    CommonShutdown();

    if (ghHeap) {
        HeapDestroy(ghHeap);
        ghHeap = NULL;
    }

	// Если режим ComSpec - вернуть код возврата из запущенного процесса
	if (iRc == 0 && gnRunMode == RM_COMSPEC)
		iRc = cmd.nExitCode;

	#ifdef SHOW_STARTED_MSGBOX
	MessageBox(GetConsoleWindow(), L"Exiting", (gnRunMode == RM_SERVER) ? L"ConEmuC.Server" : L"ConEmuC.ComSpec", 0);
	#endif
		
    return iRc;
}

//#if defined(CRTSTARTUP)
//extern "C"{
//  BOOL WINAPI _DllMainCRTStartup(HANDLE hDll,DWORD dwReason,LPVOID lpReserved);
//};
//
//BOOL WINAPI mainCRTStartup(HANDLE hDll,DWORD dwReason,LPVOID lpReserved)
//{
//  DllMain(hDll, dwReason, lpReserved);
//  return TRUE;
//}
//#endif


void Help()
{
    _printf(
        "ConEmuC. Copyright (c) 2009, Maximus5\n"
        "This is a console part of ConEmu product.\n"
        "Usage: ConEmuC [switches] [/U | /A] /C <command line, passed to %COMSPEC%>\n"
        "   or: ConEmuC [switches] /ROOT <program with arguments, far.exe for example>\n"
        "   or: ConEmuC /ATTACH /NOCMD\n"
        "   or: ConEmuC /?\n"
        "Switches:\n"
        "        /CONFIRM  - confirm closing console on program termination\n"
        "        /ATTACH   - auto attach to ConEmu GUI\n"
        "        /NOCMD    - attach current (existing) console to GUI\n"
        "        /B{W|H|Z} - define window width, height and buffer height\n"
        "        /F{N|W|H} - define console font name, width, height\n"
        "        /LOG[N]   - create (debug) log file, N is number from 0 to 3\n"
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

	// Если есть одна из команд перенаправления, или слияния - нужен CMD.EXE
	if (wcschr(asCmdLine, L'&') || 
		wcschr(asCmdLine, L'>') || 
		wcschr(asCmdLine, L'<') || 
		wcschr(asCmdLine, L'|') ||
		wcschr(asCmdLine, L'^') // или экранирования
		)
	{
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
	//pwszCopy = wcsrchr(szArg, L'\\'); if (!pwszCopy) pwszCopy = szArg; else pwszCopy ++;
	pwszCopy = PointToName(szArg);
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
    LPCWSTR pszArgStarts = NULL;
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
        _printf("GetCommandLineW failed! ErrCode=0x%08X\n", dwErr);
        return CERR_GETCOMMANDLINE;
    }

    gnRunMode = RM_UNDEFINED;
    
    
    while ((iRc = NextArg(&asCmdLine, szArg, &pszArgStarts)) == 0)
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
        	wchar_t* pszEnd = NULL;
            if (wcsncmp(szArg, L"/BW=", 4)==0) {
                gcrBufferSize.X = /*_wtoi(szArg+4);*/(SHORT)wcstol(szArg+4,&pszEnd,10); gbParmBufferSize = TRUE;
            } else if (wcsncmp(szArg, L"/BH=", 4)==0) {
                gcrBufferSize.Y = /*_wtoi(szArg+4);*/(SHORT)wcstol(szArg+4,&pszEnd,10); gbParmBufferSize = TRUE;
            } else if (wcsncmp(szArg, L"/BZ=", 4)==0) {
                gnBufferHeight = /*_wtoi(szArg+4);*/(SHORT)wcstol(szArg+4,&pszEnd,10); gbParmBufferSize = TRUE;
            }
        } else

        if (wcsncmp(szArg, L"/F", 2)==0) {
        	wchar_t* pszEnd = NULL;
            if (wcsncmp(szArg, L"/FN=", 4)==0) {
                lstrcpynW(srv.szConsoleFont, szArg+4, 32);
            } else if (wcsncmp(szArg, L"/FW=", 4)==0) {
                srv.nConFontWidth = /*_wtoi(szArg+4);*/(SHORT)wcstol(szArg+4,&pszEnd,10);
            } else if (wcsncmp(szArg, L"/FH=", 4)==0) {
                srv.nConFontHeight = /*_wtoi(szArg+4);*/(SHORT)wcstol(szArg+4,&pszEnd,10);
            //} else if (wcsncmp(szArg, L"/FF=", 4)==0) {
            //  lstrcpynW(srv.szConsoleFontFile, szArg+4, MAX_PATH);
            }
        } else
        
        if (wcsncmp(szArg, L"/LOG",4)==0) {
        	int nLevel = 0;
        	if (szArg[4]==L'1') nLevel = 1; else if (szArg[4]>=L'2') nLevel = 2;
            CreateLogSizeFile(nLevel);
        } else
        
        if (wcscmp(szArg, L"/NOCMD")==0) {
        	gnRunMode = RM_SERVER;
        	gbNoCreateProcess = TRUE;
        } else

        if (wcsncmp(szArg, L"/PID=", 5)==0) {
        	gnRunMode = RM_SERVER;
        	gbNoCreateProcess = TRUE;
        	wchar_t* pszEnd = NULL;
			//srv.dwRootProcess = _wtol(szArg+5);
			srv.dwRootProcess = wcstol(szArg+5, &pszEnd, 10);
			if (srv.dwRootProcess == 0) {
				_printf ("Attach to GUI was requested, but invalid PID specified:\n");
				_wprintf (GetCommandLineW());
				_printf ("\n");
				return CERR_CARGUMENT;
			}
        } else

        if (wcsncmp(szArg, L"/DEBUGPID=", 10)==0) {
        	gnRunMode = RM_SERVER;
        	gbNoCreateProcess = gbDebugProcess = TRUE;
        	wchar_t* pszEnd = NULL;
			//srv.dwRootProcess = _wtol(szArg+10);
			srv.dwRootProcess = wcstol(szArg+10, &pszEnd, 10);
			if (srv.dwRootProcess == 0) {
				_printf ("Debug of process was requested, but invalid PID specified:\n");
				_wprintf (GetCommandLineW());
				_printf ("\n");
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
        if (wcscmp(szArg, L"/ROOT")==0 || wcscmp(szArg, L"/root")==0) {
            gnRunMode = RM_SERVER; gbNoCreateProcess = FALSE;
            break; // asCmdLine уже указывает на запускаемую программу
        } else

        // После этих аргументов - идет то, что передается в COMSPEC (CreateProcess)!
        if (wcscmp(szArg, L"/C")==0 || wcscmp(szArg, L"/c")==0 || wcscmp(szArg, L"/K")==0 || wcscmp(szArg, L"/k")==0) {
            gnRunMode = RM_COMSPEC; gbNoCreateProcess = FALSE;
            cmd.bK = (wcscmp(szArg, L"/K")==0 || wcscmp(szArg, L"/k")==0);
            // Поддержка дебильной возможности "cmd /cecho xxx"
            asCmdLine = pszArgStarts + 2;
			while (*asCmdLine==L' ' || *asCmdLine==L'\t') asCmdLine++;
            break; // asCmdLine уже указывает на запускаемую программу
        }
    }
    
    if (gnRunMode == RM_SERVER) {
    	if (gbDebugProcess) {
    		if (!DebugActiveProcess(srv.dwRootProcess)) {
    			DWORD dwErr = GetLastError();
    			_printf("Can't start debugger! ErrCode=0x%08X\n", dwErr);
    			return CERR_CANTSTARTDEBUGGER;
    		}
    		pfnDebugActiveProcessStop = (FDebugActiveProcessStop)GetProcAddress(GetModuleHandle(L"kernel32.dll"),"DebugActiveProcessStop");
    		pfnDebugSetProcessKillOnExit = (FDebugSetProcessKillOnExit)GetProcAddress(GetModuleHandle(L"kernel32.dll"),"DebugSetProcessKillOnExit");
    		if (pfnDebugSetProcessKillOnExit) pfnDebugSetProcessKillOnExit(FALSE/*KillOnExit*/);
    		srv.bDebuggerActive = TRUE;
    	
	        wchar_t* pszNewCmd = new wchar_t[1];
	        if (!pszNewCmd) {
	            _printf ("Can't allocate 1 wchar!\n");
	            return CERR_NOTENOUGHMEM1;
	        }
	        pszNewCmd[0] = 0;
	        return 0;
    	} else
    	if (gbNoCreateProcess && gbAttachMode) {
			if (pfnGetConsoleProcessList==NULL) {
	            _printf ("Attach to GUI was requested, but required WinXP or higher:\n");
	            _wprintf (GetCommandLineW());
	            _printf ("\n");
	            return CERR_CARGUMENT;
			}
			DWORD nProcesses[10];
	    	DWORD nProcCount = pfnGetConsoleProcessList ( nProcesses, 10 );
	    	if (nProcCount < 2) {
	            _printf ("Attach to GUI was requested, but there is no console processes:\n");
	            _wprintf (GetCommandLineW());
	            _printf ("\n");
	            return CERR_CARGUMENT;
	    	}
	    	// Если cmd.exe запущен из cmd.exe (в консоли уже больше двух процессов) - ничего не делать
	    	if (nProcCount > 2) {
	    		// И ругаться только под отладчиком
	    		wchar_t szProc[255] ={0}, szTmp[10]; //wsprintfW(szProc, L"%i, %i, %i", nProcesses[0], nProcesses[1], nProcesses[2]);
	    		for (DWORD n=0; n<nProcCount && n<20; n++) {
	    			if (n) lstrcatW(szProc, L", ");
	    			wsprintf(szTmp, L"%i", nProcesses[0]);
	    			//lstrcatW(szProc, _ltow(nProcesses[0], szTmp, 10));
	    			lstrcatW(szProc, szTmp);
	    		}
	    		PRINT_COMSPEC(L"Attach to GUI was requested, but there is more then 2 console processes: %s\n", szProc);
	    		return CERR_CARGUMENT;
	    	}

	        wchar_t* pszNewCmd = new wchar_t[1];
	        if (!pszNewCmd) {
	            _printf ("Can't allocate 1 wchar!\n");
	            return CERR_NOTENOUGHMEM1;
	        }
	        pszNewCmd[0] = 0;
	        return 0;
        }
    }

    if (iRc != 0) {
        if (iRc == CERR_CMDLINEEMPTY) {
            Help();
            _printf ("\n\nParsing command line failed (/C argument not found):\n");
            _wprintf (GetCommandLineW());
            _printf ("\n");
        } else {
            _printf ("Parsing command line failed:\n");
            _wprintf (asCmdLine);
            _printf ("\n");
        }
        return iRc;
    }
    if (gnRunMode == RM_UNDEFINED) {
        _printf ("Parsing command line failed (/C argument not found):\n");
        _wprintf (GetCommandLineW());
        _printf ("\n");
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
            size_t nNewLen = wcslen(pwszStartCmdLine) + 135;
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
                _printf ("Can't allocate %i wchars!\n", (DWORD)nNewLen);
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
					wcscat(pszNewCmd, L" /ROOT cmd /U /C ");
				else if (gnCmdUnicodeMode == 1)
					wcscat(pszNewCmd, L" /ROOT cmd /A /C ");
				else
					wcscat(pszNewCmd, L" /ROOT cmd /C ");
			} else {
				wcscat(pszNewCmd, L" /ROOT ");
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
            //pwszCopy = wcsrchr(szComSpec, L'\\'); if (!pwszCopy) pwszCopy = szComSpec;
			pwszCopy = PointToName(szComSpec);
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
                _printf ("Can't find cmd.exe!\n");
                return CERR_CMDEXENOTFOUND;
            }
        }

        nCmdLine += lstrlenW(szComSpec)+15; // "/C", кавычки и возможный "/U"
    }

    *psNewCmd = new wchar_t[nCmdLine];
    if (!(*psNewCmd))
    {
        _printf ("Can't allocate %i wchars!\n", (DWORD)nCmdLine);
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
		// Наверное можно положиться на фар, и не кавычить самостоятельно
		BOOL lbNeedQuatete = FALSE;
		// Команды в cmd.exe лучше передавать так:
		// ""c:\program files\arc\7z.exe" -?"
		//int nLastChar = lstrlenW(asCmdLine) - 1;
		//if (asCmdLine[0] == L'"' && asCmdLine[nLastChar] == L'"') {
		//	// Наверное можно положиться на фар, и не кавычить самостоятельно
		//	if (gnRunMode == RM_COMSPEC)
		//		lbNeedQuatete = FALSE;
		//	//if (asCmdLine[1] == L'"' && asCmdLine[2])
		//	//	lbNeedQuatete = FALSE; // уже
		//	//else if (wcschr(asCmdLine+1, L'"') == (asCmdLine+nLastChar))
		//	//	lbNeedQuatete = FALSE; // не требуется. внутри кавычек нет
		//} 
		//if (lbNeedQuatete) { // надо
		//	lstrcatW( (*psNewCmd), L"\"" );
		//}
		// Собственно, командная строка
        lstrcatW( (*psNewCmd), asCmdLine );
		//if (lbNeedQuatete)
		//	lstrcatW( (*psNewCmd), L"\"" );
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
    _wprintf(asConfirm);

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
        _printf("\n");
}


bool GetAliases(wchar_t* asExeName, wchar_t** rsAliases, LPDWORD rnAliasesSize)
{
	bool lbRc = false;
	DWORD nAliasRC, nAliasErr, nAliasAErr = 0, nSizeA = 0;

	_ASSERTE(asExeName && rsAliases && rnAliasesSize);
	_ASSERTE(*rsAliases == NULL);

	*rnAliasesSize = GetConsoleAliasesLength(asExeName);
	if (*rnAliasesSize == 0) {
		lbRc = true;
	} else {
		*rsAliases = (wchar_t*)calloc(*rnAliasesSize+2,1);
		nAliasRC = GetConsoleAliases(*rsAliases,*rnAliasesSize,asExeName);
		if (nAliasRC) {
			lbRc = true;
		} else {
			nAliasErr = GetLastError();
			if (nAliasErr == ERROR_NOT_ENOUGH_MEMORY) {
				// Попробовать ANSI функции
				UINT nCP = CP_OEMCP;
				char szExeName[MAX_PATH+1];
				char *pszAliases = NULL;
				WideCharToMultiByte(nCP,0,asExeName,-1,szExeName,MAX_PATH+1,0,0);
				nSizeA = GetConsoleAliasesLengthA(szExeName);
				if (nSizeA) {
					pszAliases = (char*)calloc(nSizeA+1,1);
					nAliasRC = GetConsoleAliasesA(pszAliases,nSizeA,szExeName);
					if (nAliasRC) {
						lbRc = true;
						MultiByteToWideChar(nCP,0,pszAliases,nSizeA,*rsAliases,((*rnAliasesSize)/2)+1);
					} else {
						nAliasAErr = GetLastError();
					}
					free(pszAliases);
				}
			}
			if (!nAliasRC) {
				if ((*rnAliasesSize) < 255) { free(*rsAliases); *rsAliases = (wchar_t*)calloc(128,2); }
				wsprintf(*rsAliases, L"\nConEmuC: GetConsoleAliases failed, ErrCode=0x%08X(0x%08X), AliasesLength=%i(%i)\n\n", nAliasErr, nAliasAErr, *rnAliasesSize, nSizeA);
			}
		}
	}
	return lbRc;
}


int ComspecInit()
{
    TODO("Определить код родительского процесса, и если это FAR - запомнить его (для подключения к пайпу плагина)");
    TODO("Размер получить из GUI, если оно есть, иначе - по умолчанию");
    TODO("GUI может скорректировать размер с учетом полосы прокрутки");
    
    WARNING("CreateFile(CONOUT$) по идее возвращает текущий ScreenBuffer. Можно его самим возвращать в ComspecDone");
    // Правда нужно проверить, что там происходит с ghConOut.Close(),... 

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
            _printf ("Can't create process, ErrCode=0x%08X! Command to be executed:\n", dwErr, gpszRunCmd);
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
    wchar_t szComSpec[MAX_PATH+1], *pszComSpecName;
    if (GetEnvironmentVariable(L"ComSpecC", szComSpec, MAX_PATH) && szComSpec[0] != 0)
    {
        // Только если это (случайно) не conemuc.exe
        wchar_t* pwszCopy = (wchar_t*)PointToName(szComSpec); //wcsrchr(szComSpec, L'\\'); 
        //if (!pwszCopy) pwszCopy = szComSpec;
        #pragma warning( push )
        #pragma warning(disable : 6400)
        if (lstrcmpiW(pwszCopy, L"ConEmuC")==0 || lstrcmpiW(pwszCopy, L"ConEmuC.exe")==0)
            szComSpec[0] = 0;
        #pragma warning( pop )
        if (szComSpec[0]) {
	        SetEnvironmentVariable(L"ComSpec", szComSpec);
	        SetEnvironmentVariable(L"ComSpecC", NULL);
        }
        pszComSpecName = (wchar_t*)PointToName(szComSpec);
    } else {
    	pszComSpecName = L"cmd.exe";
    }
    lstrcpyn(cmd.szComSpecName, pszComSpecName, countof(cmd.szComSpecName));


    if (pszComSpecName) {
    	wchar_t szSelf[MAX_PATH+1];
		if (GetModuleFileName(NULL, szSelf, MAX_PATH)) {
			lstrcpyn(cmd.szSelfName, (wchar_t*)PointToName(szSelf), countof(cmd.szSelfName));

			if (!GetAliases(cmd.szSelfName, &cmd.pszPreAliases, &cmd.nPreAliasSize)) {
				if (cmd.pszPreAliases) {
					wprintf(cmd.pszPreAliases);
					free(cmd.pszPreAliases);
					cmd.pszPreAliases = NULL;
				}
			}
		}
    }
    
    SendStarted();
    
    ghConOut.Close();
    
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
		if (gnRunMode == RM_SERVER)
			pIn->StartStop.bUserIsAdmin = IsUserAdmin();

		// Перед запуском 16бит приложений нужно подресайзить консоль...
		gnImageSubsystem = 0;
        LPCWSTR pszTemp = gpszRunCmd;
        wchar_t lsRoot[MAX_PATH+1] = {0};
        if (gnRunMode == RM_SERVER && srv.bDebuggerActive) {
			// "Отладчик"
			gnImageSubsystem = 0x101;
			gbRootIsCmdExe = TRUE; // Чтобы буфер появился
        } else
		if (!gpszRunCmd) {
			// Аттач из фар-плагина
			gnImageSubsystem = 0x100;
		} else
        if (0 == NextArg(&pszTemp, lsRoot)) {
        	PRINT_COMSPEC(L"Starting: <%s>", lsRoot);
        	if (!GetImageSubsystem(lsRoot, gnImageSubsystem))
				gnImageSubsystem = 0;
       		PRINT_COMSPEC(L", Subsystem: <%i>\n", gnImageSubsystem);
			PRINT_COMSPEC(L"  Args: %s\n", pszTemp);
        }
		pIn->StartStop.nSubSystem = gnImageSubsystem;
		pIn->StartStop.bRootIsCmdExe = gbRootIsCmdExe; //2009-09-14
		// НЕ MyGet..., а то можем заблокироваться...
		GetConsoleScreenBufferInfo(ghConOut, &pIn->StartStop.sbi);

		PRINT_COMSPEC(L"Starting %s mode (ExecuteGuiCmd started)\n",(RunMode==RM_SERVER) ? L"Server" : L"ComSpec");
        
		pOut = ExecuteGuiCmd(ghConWnd, pIn, ghConWnd);
		if (!pOut) {
			// При старте консоли GUI может не успеть создать командные пайпы, т.к.
			// их имена основаны на дескрипторе консольного окна, а его заранее GUI не знает
			// Поэтому нужно чуть-чуть подождать, пока GUI поймает событие 
			// (event == EVENT_CONSOLE_START_APPLICATION && idObject == (LONG)mn_ConEmuC_PID)
			DWORD dwStart = GetTickCount(), dwDelta = 0;
			while (!gbInShutdown && dwDelta < GUIREADY_TIMEOUT) {
				Sleep(10);
				pOut = ExecuteGuiCmd(ghConWnd, pIn, ghConWnd);
				if (pOut) break;

				dwDelta = GetTickCount() - dwStart;
			}
			if (!pOut) {
				_ASSERTE(pOut != NULL);
			}
		}

        PRINT_COMSPEC(L"Starting %s mode (ExecuteGuiCmd finished)\n",(RunMode==RM_SERVER) ? L"Server" : L"ComSpec");

        if (pOut) {
			bSent = true;

			BOOL  bAlreadyBufferHeight = pOut->StartStopRet.bWasBufferHeight;

			DWORD nGuiPID = pOut->StartStopRet.dwPID;
			ghConEmuWnd = pOut->StartStopRet.hWnd;

			if (gnRunMode == RM_COMSPEC) {
				cmd.dwSrvPID = pOut->StartStopRet.dwSrvPID;
			}

            AllowSetForegroundWindow(nGuiPID);
            
            gnBufferHeight  = (SHORT)pOut->StartStopRet.nBufferHeight;
            gcrBufferSize.X = (SHORT)pOut->StartStopRet.nWidth;
            gcrBufferSize.Y = (SHORT)pOut->StartStopRet.nHeight;
			gbParmBufferSize = TRUE;
			
            if (gnRunMode == RM_SERVER) {
            	if (srv.bDebuggerActive && !gnBufferHeight) gnBufferHeight = 1000;
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

	// Это необходимо делать, т.к. при смене буфера (SetConsoleActiveScreenBuffer) приложением,
	// дескриптор нужно закрыть, иначе conhost может не вернуть предыдущий буфер
	ghConOut.Close();

    // Поддержка алиасов
    if (cmd.szComSpecName[0] && cmd.szSelfName[0]) {
    	// Скопировать алиасы из cmd.exe в conemuc.exe
		wchar_t *pszPostAliases = NULL;
		DWORD nPostAliasSize;
		BOOL lbChanged = (cmd.pszPreAliases == NULL);

		if (!GetAliases(cmd.szComSpecName, &pszPostAliases, &nPostAliasSize)) {
			if (pszPostAliases) wprintf(pszPostAliases);
		} else {
			if (!lbChanged) {
				lbChanged = (cmd.nPreAliasSize!=nPostAliasSize);
			}
			if (!lbChanged && cmd.nPreAliasSize && cmd.pszPreAliases && pszPostAliases) {
				lbChanged = memcmp(cmd.pszPreAliases,pszPostAliases,cmd.nPreAliasSize)!=0;
			}
			if (lbChanged) {
				if (cmd.dwSrvPID) {
					MCHKHEAP;
					CESERVER_REQ* pIn = ExecuteNewCmd(CECMD_SAVEALIASES,sizeof(CESERVER_REQ_HDR)+nPostAliasSize);
					if (pIn) {
						MCHKHEAP;
						memmove(pIn->Data, pszPostAliases, nPostAliasSize);
						MCHKHEAP;
						CESERVER_REQ* pOut = ExecuteSrvCmd(cmd.dwSrvPID, pIn, GetConsoleWindow());
						MCHKHEAP;
						if (pOut) ExecuteFreeResult(pOut);
						ExecuteFreeResult(pIn);
						MCHKHEAP;
					}
				}

				wchar_t *pszNewName = pszPostAliases, *pszNewTarget, *pszNewLine;
				while (pszNewName && *pszNewName) {
					pszNewLine = pszNewName + lstrlen(pszNewName);
					pszNewTarget = wcschr(pszNewName, L'=');
					if (pszNewTarget) {
						*pszNewTarget = 0;
						pszNewTarget++;
					}
					if (*pszNewTarget == 0) pszNewTarget = NULL;
					AddConsoleAlias(pszNewName, pszNewTarget, cmd.szSelfName);
					pszNewName = pszNewLine+1;
				}
			}
		}
		if (pszPostAliases) { free(pszPostAliases); pszPostAliases = NULL; }
    }
    

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
        
        ghConOut.Close();

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
			HANDLE hOut = (HANDLE)ghConOut;
			if (hOut == INVALID_HANDLE_VALUE)
				hOut = GetStdHandle(STD_OUTPUT_HANDLE);
			GetConsoleScreenBufferInfo(hOut, &pIn->StartStop.sbi);
			ghConOut.Close();

            PRINT_COMSPEC(L"Finalizing comspec mode (ExecuteGuiCmd started)\n",0);
            pOut = ExecuteGuiCmd(ghConWnd, pIn, ghConWnd);
            PRINT_COMSPEC(L"Finalizing comspec mode (ExecuteGuiCmd finished)\n",0);
            if (pOut) {
				if (!pOut->StartStopRet.bWasBufferHeight) {
					//cmd.sbi.dwSize = pIn->StartStop.sbi.dwSize;
					lbRc1 = FALSE; // Консольное приложение самостоятельно сбросило буферный режим. Не дергаться...
				} else {
					lbRc1 = TRUE;
				}
                ExecuteFreeResult(pOut); pOut = NULL;
            }
			ExecuteFreeResult(pIn); pIn = NULL; // не освобождалось
        }

        lbRc2 = GetConsoleScreenBufferInfo(ghConOut, &sbi2);
		#ifdef _DEBUG
		if (sbi2.dwSize.Y > 200) {
			wchar_t szTitle[128]; wsprintfW(szTitle, L"ConEmuC (PID=%i)", GetCurrentProcessId());
			MessageBox(NULL, L"BufferHeight was not turned OFF", szTitle, MB_SETFOREGROUND|MB_SYSTEMMODAL);
		}
		#endif
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

    if (cmd.pszPreAliases) { free(cmd.pszPreAliases); cmd.pszPreAliases = NULL; }
    
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
        _printf("GetModuleFileName failed! ErrCode=0x%08X\n", dwErr);
        return; // не удалось
    }
    if ((pszDot = wcsrchr(szFile, L'.')) == NULL) {
        _printf("wcsrchr failed!\n", 0, szFile);
        return; // ошибка
    }
    wsprintfW(pszDot, L"-size-%i.log", gnSelfPID);
    
    ghLogSize = CreateFileW ( szFile, GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (ghLogSize == INVALID_HANDLE_VALUE) {
        ghLogSize = NULL;
        dwErr = GetLastError();
        _printf("CreateFile failed! ErrCode=0x%08X\n", dwErr, szFile);
        return;
    }
    
    int nLen = lstrlen(szFile);
    wpszLogSizeFile = /*_wcsdup(szFile);*/(wchar_t*)calloc(nLen+1,2);
    lstrcpy(wpszLogSizeFile, szFile);
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
        wsprintfA(szInfo, "%i:%02i:%02i CurSize={%ix%i} ChangeTo={%ix%i} %s %s\r\n",
            st.wHour, st.wMinute, st.wSecond,
            lsbi.dwSize.X, lsbi.dwSize.Y, pcrSize->X, pcrSize->Y, pszThread, (pszLabel ? pszLabel : ""));
    } else {
        wsprintfA(szInfo, "%i:%02i:%02i CurSize={%ix%i} %s %s\r\n",
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
			_printf ("GetModuleFileName failed, ErrCode=0x%08X\n", dwErr);
			return NULL;
		}
		pszSlash = wcsrchr(pszSelf, L'\\');
		if (!pszSlash) {
			_printf ("Invalid GetModuleFileName, backslash not found!\n", 0, pszSelf);
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
			_printf ("Can't create process, ErrCode=0x%08X! Command to be executed:\n", dwErr, pszSelf);
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
    
	// Было 10, чтобы не перенапрягать консоль при ее быстром обновлении ("dir /s" и т.п.)
    srv.nMaxFPS = 100;

#ifdef _DEBUG
	if (ghFarInExecuteEvent)
		SetEvent(ghFarInExecuteEvent);
#endif

	// Создать MapFile для заголовка (СРАЗУ!!!)
	iRc = CreateMapHeader();
	if (iRc != 0)
		goto wrap;


    
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
	MCHKHEAP;
	if (srv.pnProcesses == NULL || srv.pnProcessesCopy == NULL) {
		_printf ("Can't allocate %i DWORDS!\n", srv.nMaxProcesses);
		iRc = CERR_NOTENOUGHMEM1; goto wrap;
	}
	CheckProcessCount(TRUE); // Сначала добавит себя
	// в принципе, серверный режим может быть вызван из фара, чтобы подцепиться к GUI 
	// но больше двух процессов в консоли не ожидается!
	_ASSERTE(srv.bDebuggerActive || srv.nProcessCount<=2); 


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
		_printf("CreateThread(InputThread) failed, ErrCode=0x%08X\n", dwErr); 
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
		_printf("CreateThread(InputPipeThread) failed, ErrCode=0x%08X\n", dwErr);
		iRc = CERR_CREATEINPUTTHREAD; goto wrap;
	}

    //InitializeCriticalSection(&srv.csChangeSize);
    //InitializeCriticalSection(&srv.csConBuf);
    //InitializeCriticalSection(&srv.csChar);

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




    if (gbNoCreateProcess && (gbAttachMode || srv.bDebuggerActive)) {
    	if (!srv.bDebuggerActive && !IsWindowVisible(ghConWnd)) {
			PRINT_COMSPEC(L"Console windows is not visible. Attach is unavailable. Exiting...\n", 0);
			gbAlwaysConfirmExit = FALSE;
			srv.nProcessStartTick = GetTickCount() - 2*CHECK_ROOTSTART_TIMEOUT;
			return CERR_RUNNEWCONSOLE;
    	}
    	
		if (srv.dwRootProcess == 0 && !srv.bDebuggerActive) {
    		// Нужно попытаться определить PID корневого процесса.
    		// Родительским может быть cmd (comspec, запущенный из FAR)
    		DWORD dwParentPID = 0, dwFarPID = 0;
    		DWORD dwServerPID = 0; // Вдруг в этой консоли уже есть сервер?
    		
    		_ASSERTE(!srv.bDebuggerActive);
	    	
    		if (srv.nProcessCount >= 2 && !srv.bDebuggerActive) {
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
				_printf ("Attach to GUI was requested, but there is no console processes:\n", 0, GetCommandLineW());
				return CERR_CARGUMENT;
    		}
	    	
    		// Нужно открыть HANDLE корневого процесса
    		srv.hRootProcess = OpenProcess(PROCESS_QUERY_INFORMATION|SYNCHRONIZE, FALSE, dwParentPID);
    		if (!srv.hRootProcess) {
    			dwErr = GetLastError();
    			wchar_t* lpMsgBuf = NULL;
	    		
   				FormatMessage( FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM, NULL, dwErr, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPWSTR)&lpMsgBuf, 0, NULL );
	    		
				_printf("Can't open process (%i) handle, ErrCode=0x%08X, Description:\n", 
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
				_printf("GetModuleFileName failed, ErrCode=0x%08X\n", dwErr);
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
				_printf("Can't create process, ErrCode=0x%08X! Command to be executed:\n", dwErr, pszSelf);
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
    		DWORD dwFlags = PROCESS_QUERY_INFORMATION|SYNCHRONIZE;
    		if (srv.bDebuggerActive)
    			dwFlags |= PROCESS_VM_READ;
    		srv.hRootProcess = OpenProcess(dwFlags, FALSE, srv.dwRootProcess);
    		if (!srv.hRootProcess) {
    			dwErr = GetLastError();
    			wchar_t* lpMsgBuf = NULL;
	    		
   				FormatMessage( FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM, NULL, dwErr, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPWSTR)&lpMsgBuf, 0, NULL );
	    		
				_printf ("Can't open process (%i) handle, ErrCode=0x%08X, Description:\n", 
            		srv.dwRootProcess, dwErr, (lpMsgBuf == NULL) ? L"<Unknown error>" : lpMsgBuf);
	            
				if (lpMsgBuf) LocalFree(lpMsgBuf);
				return CERR_CREATEPROCESS;
    		}
    		
    		if (srv.bDebuggerActive) {
    			wchar_t szTitle[64];
    			wsprintf(szTitle, L"Debug PID=%i", srv.dwRootProcess);
    			SetConsoleTitleW(szTitle);
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
    //srv.bNeedFullReload = FALSE; srv.bForceFullSend = TRUE;
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
    if (!srv.bDebuggerActive || gbAttachMode) {
	    if (ghLogSize) LogSize(NULL, ":SetConsoleFontSizeTo.before");
	    SetConsoleFontSizeTo(ghConWnd, srv.nConFontHeight, srv.nConFontWidth, srv.szConsoleFont);
	    if (ghLogSize) LogSize(NULL, ":SetConsoleFontSizeTo.after");
    } else {
    	SetWindowPos(ghConWnd, HWND_TOPMOST, 0,0,0,0, SWP_NOSIZE|SWP_NOMOVE);
    }
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
	ReloadFullConsoleInfo(TRUE);


    //
    srv.hRefreshEvent = CreateEvent(NULL,FALSE,FALSE,NULL);
    if (!srv.hRefreshEvent) {
        dwErr = GetLastError();
        _printf("CreateEvent(hRefreshEvent) failed, ErrCode=0x%08X\n", dwErr); 
        iRc = CERR_REFRESHEVENT; goto wrap;
    }
	srv.hDataSentEvent = CreateEvent(NULL,FALSE,FALSE,NULL);
	if (!srv.hDataSentEvent) {
		dwErr = GetLastError();
		_printf("CreateEvent(hDataSentEvent) failed, ErrCode=0x%08X\n", dwErr); 
		iRc = CERR_REFRESHEVENT; goto wrap;
	}
    srv.hReqSizeChanged = CreateEvent(NULL,FALSE,FALSE,NULL);
    if (!srv.hReqSizeChanged) {
        dwErr = GetLastError();
        _printf("CreateEvent(hReqSizeChanged) failed, ErrCode=0x%08X\n", dwErr); 
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
        _printf("CreateThread(RefreshThread) failed, ErrCode=0x%08X\n", dwErr); 
        iRc = CERR_CREATEREFRESHTHREAD; goto wrap;
    }
    
    
    //srv.nMsgHookEnableDisable = RegisterWindowMessage(L"ConEmuC::HookEnableDisable");
    // The client thread that calls SetWinEventHook must have a message loop in order to receive events.");
    srv.hWinEventThread = CreateThread( NULL, 0, WinEventThread, NULL, 0, &srv.dwWinEventThread);
    if (srv.hWinEventThread == NULL) 
    {
        dwErr = GetLastError();
        _printf("CreateThread(WinEventThread) failed, ErrCode=0x%08X\n", dwErr); 
        iRc = CERR_WINEVENTTHREAD; goto wrap;
    }

    // Запустить нить обработки команд  
    srv.hServerThread = CreateThread( NULL, 0, ServerThread, NULL, 0, &srv.dwServerThreadId);
    if (srv.hServerThread == NULL) 
    {
        dwErr = GetLastError();
        _printf("CreateThread(ServerThread) failed, ErrCode=0x%08X\n", dwErr); 
        iRc = CERR_CREATESERVERTHREAD; goto wrap;
    }

    if (gbAttachMode) {
        HWND hDcWnd = Attach2Gui(5000);
        
		// 090719 попробуем в сервере это делать всегда. Нужно передать в GUI - TID нити ввода
        //// Если это НЕ новая консоль (-new_console) и не /ATTACH уже существующей консоли
        //if (!gbNoCreateProcess)
        //	SendStarted();

        if (!hDcWnd) {
            _printf("Available ConEmu GUI window not found!\n");
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
	if (ghExitQueryEvent) SetEvent(ghExitQueryEvent);

	// Остановить отладчик, иначе отлаживаемый процесс тоже схлопнется	
    if (srv.bDebuggerActive) {
    	if (pfnDebugActiveProcessStop) pfnDebugActiveProcessStop(srv.dwRootProcess);
    	srv.bDebuggerActive = FALSE;
    }

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
	if (srv.hDataSentEvent) {
		SafeCloseHandle(srv.hDataSentEvent);
	}
    //if (srv.hChangingSize) {
    //    SafeCloseHandle(srv.hChangingSize);
    //}
    // Отключить все хуки
    //srv.bWinHookAllow = FALSE; srv.nWinHookMode = 0;
    //HookWinEvents ( -1 );
    
    if (gpStoredOutput) { Free(gpStoredOutput); gpStoredOutput = NULL; }
    if (srv.pszAliases) { Free(srv.pszAliases); srv.pszAliases = NULL; }
    //if (srv.psChars) { Free(srv.psChars); srv.psChars = NULL; }
    //if (srv.pnAttrs) { Free(srv.pnAttrs); srv.pnAttrs = NULL; }
    //if (srv.ptrLineCmp) { Free(srv.ptrLineCmp); srv.ptrLineCmp = NULL; }
    //DeleteCriticalSection(&srv.csConBuf);
    //DeleteCriticalSection(&srv.csChar);
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

	CloseMapHeader();
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

	if (srv.bDebuggerActive) {
		//if (srv.hRootProcess) {
		//	if (WaitForSingleObject(srv.hRootProcess, 0) == WAIT_OBJECT_0) {
		//		srv.nProcessCount = 1;
		//		return TRUE;
		//	}
		//}
		//srv.pnProcesses[1] = srv.dwRootProcess;
		//srv.nProcessCount = 2;
		return FALSE;
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
			MCHKHEAP;
			if (lbChanged)
				memmove(srv.pnProcessesCopy, srv.pnProcesses, nSize);
			MCHKHEAP;
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
									//2009-12-28 убрал. все должно быть само...
									//if (lstrcmpiW(prc.szExeFile, L"telnet.exe")==0) {
									//	// сразу хэндлы передернуть
									//	ghConIn.Close(); ghConOut.Close();
									//	//srv.bWinHookAllow = TRUE; // Попробуем разрешить события для телнета
									//	lbFarExists = TRUE; lbTelnetExist = TRUE; break;
									//}
                    			}
                    		}
						} while (!(lbFarExists && lbTelnetExist) && Process32Next(hSnap, &prc));
					}
					CloseHandle(hSnap);
				}
			}
			srv.bTelnetActive = lbTelnetExist;
			
			//if (srv.nProcessCount >= 2
			//	&& ( (srv.hWinHook == NULL && srv.bWinHookAllow) || (srv.hWinHook != NULL) )
			//    )
			//{
			//	if (lbFarExists) srv.nWinHookMode = 2; else srv.nWinHookMode = 1;
			//	
			// 			if (lbFarExists && srv.hWinHook == NULL && srv.bWinHookAllow) {
			// 				HookWinEvents(2);
			// 			} else if (!lbFarExists && srv.hWinHook) {
			// 				HookWinEvents(0);
			// 			}
			//}
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
		WaitForSingleObject(ghExitQueryEvent,0) == WAIT_TIMEOUT)
	{
		nPrevCount = 2; // чтобы сработало следующее условие
		if (!gbAlwaysConfirmExit) gbAlwaysConfirmExit = TRUE; // чтобы консоль не схлопнулась
	}
	if (nPrevCount > 1 && srv.nProcessCount == 1 && srv.pnProcesses[0] == gnSelfPID) {
		CS.Unlock();
		if (!gbAlwaysConfirmExit && (dwLastCheckTick - srv.nProcessStartTick) <= CHECK_ROOTSTART_TIMEOUT)
			gbAlwaysConfirmExit = TRUE; // чтобы консоль не схлопнулась
		SetEvent(ghExitQueryEvent);
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

void ProcessDebugEvent()
{
	static wchar_t wszDbgText[1024];
	static char szDbgText[1024];
	BOOL lbNonContinuable = FALSE;

	DEBUG_EVENT evt = {0};
	if (WaitForDebugEvent(&evt,10)) {
		lbNonContinuable = FALSE;
		
		switch (evt.dwDebugEventCode) {
		case CREATE_PROCESS_DEBUG_EVENT:
		case CREATE_THREAD_DEBUG_EVENT:
		case EXIT_PROCESS_DEBUG_EVENT:
		case EXIT_THREAD_DEBUG_EVENT:
		case RIP_EVENT:
		{
			LPCSTR pszName = "Unknown";
			switch (evt.dwDebugEventCode) {
			case CREATE_PROCESS_DEBUG_EVENT: pszName = "CREATE_PROCESS_DEBUG_EVENT"; break;
			case CREATE_THREAD_DEBUG_EVENT: pszName = "CREATE_THREAD_DEBUG_EVENT"; break;
			case EXIT_PROCESS_DEBUG_EVENT: pszName = "EXIT_PROCESS_DEBUG_EVENT"; break;
			case EXIT_THREAD_DEBUG_EVENT: pszName = "EXIT_THREAD_DEBUG_EVENT"; break;
			case RIP_EVENT: pszName = "RIP_EVENT"; break;
			}
			wsprintfA(szDbgText, "{%i.%i} %s\n", evt.dwProcessId,evt.dwThreadId, pszName);
			_printf(szDbgText);
			break;
		} 

		
		case LOAD_DLL_DEBUG_EVENT:
		{
			LPCSTR pszName = "Unknown";
			switch (evt.dwDebugEventCode) {
			case LOAD_DLL_DEBUG_EVENT: pszName = "LOAD_DLL_DEBUG_EVENT"; break;
				//6 Reports a load-dynamic-link-library (DLL) debugging event. The value of u.LoadDll specifies a LOAD_DLL_DEBUG_INFO structure.
			case UNLOAD_DLL_DEBUG_EVENT: pszName = "UNLOAD_DLL_DEBUG_EVENT"; break;
				//7 Reports an unload-DLL debugging event. The value of u.UnloadDll specifies an UNLOAD_DLL_DEBUG_INFO structure.
			}
			wsprintfA(szDbgText, "{%i.%i} %s\n", evt.dwProcessId,evt.dwThreadId, pszName);
			_printf(szDbgText);
			break;
		}


		case EXCEPTION_DEBUG_EVENT:
		//1 Reports an exception debugging event. The value of u.Exception specifies an EXCEPTION_DEBUG_INFO structure.
		{
			lbNonContinuable = (evt.u.Exception.ExceptionRecord.ExceptionFlags&EXCEPTION_NONCONTINUABLE)==EXCEPTION_NONCONTINUABLE;
			switch(evt.u.Exception.ExceptionRecord.ExceptionCode)
			{
			case EXCEPTION_ACCESS_VIOLATION: // The thread tried to read from or write to a virtual address for which it does not have the appropriate access.
			if (evt.u.Exception.ExceptionRecord.NumberParameters>=2) {
				wsprintfA(szDbgText,"{%i.%i} EXCEPTION_ACCESS_VIOLATION at 0x%08X flags 0x%08X%s %s of 0x%08X\n", evt.dwProcessId,evt.dwThreadId,
					evt.u.Exception.ExceptionRecord.ExceptionAddress,
					evt.u.Exception.ExceptionRecord.ExceptionFlags,
					((evt.u.Exception.ExceptionRecord.ExceptionFlags&EXCEPTION_NONCONTINUABLE) ? "(EXCEPTION_NONCONTINUABLE)" : ""),
					((evt.u.Exception.ExceptionRecord.ExceptionInformation[0]==0) ? "Read" :
					(evt.u.Exception.ExceptionRecord.ExceptionInformation[0]==1) ? "Write" :
					(evt.u.Exception.ExceptionRecord.ExceptionInformation[0]==8) ? "DEP" : "???"),
					evt.u.Exception.ExceptionRecord.ExceptionInformation[1]
					);
			} else {
				wsprintfA(szDbgText,"{%i.%i} EXCEPTION_ACCESS_VIOLATION at 0x%08X flags 0x%08X%s\n", evt.dwProcessId,evt.dwThreadId,
					evt.u.Exception.ExceptionRecord.ExceptionAddress,
					evt.u.Exception.ExceptionRecord.ExceptionFlags,
					(evt.u.Exception.ExceptionRecord.ExceptionFlags&EXCEPTION_NONCONTINUABLE) ? "(EXCEPTION_NONCONTINUABLE)" : "");
			}
			_printf(szDbgText);
			break; 

			default:
			{
				char szName[32]; LPCSTR pszName; pszName = szName;
				#define EXCASE(s) case s: pszName = #s; break
				switch (evt.u.Exception.ExceptionRecord.ExceptionCode) {
				EXCASE(EXCEPTION_ARRAY_BOUNDS_EXCEEDED); // The thread tried to access an array element that is out of bounds and the underlying hardware supports bounds checking.
				EXCASE(EXCEPTION_BREAKPOINT); // A breakpoint was encountered.
				EXCASE(EXCEPTION_DATATYPE_MISALIGNMENT); // The thread tried to read or write data that is misaligned on hardware that does not provide alignment. For example, 16-bit values must be aligned on 2-byte boundaries; 32-bit values on 4-byte boundaries, and so on.
				EXCASE(EXCEPTION_FLT_DENORMAL_OPERAND); // One of the operands in a floating-point operation is denormal. A denormal value is one that is too small to represent as a standard floating-point value.
				EXCASE(EXCEPTION_FLT_DIVIDE_BY_ZERO); // The thread tried to divide a floating-point value by a floating-point divisor of zero.
				EXCASE(EXCEPTION_FLT_INEXACT_RESULT); // The result of a floating-point operation cannot be represented exactly as a decimal fraction.
				EXCASE(EXCEPTION_FLT_INVALID_OPERATION); // This exception represents any floating-point exception not included in this list.
				EXCASE(EXCEPTION_FLT_OVERFLOW); // The exponent of a floating-point operation is greater than the magnitude allowed by the corresponding type.
				EXCASE(EXCEPTION_FLT_STACK_CHECK); // The stack overflowed or underflowed as the result of a floating-point operation.
				EXCASE(EXCEPTION_FLT_UNDERFLOW); // The exponent of a floating-point operation is less than the magnitude allowed by the corresponding type.
				EXCASE(EXCEPTION_ILLEGAL_INSTRUCTION); // The thread tried to execute an invalid instruction.
				EXCASE(EXCEPTION_IN_PAGE_ERROR); // The thread tried to access a page that was not present, and the system was unable to load the page. For example, this exception might occur if a network connection is lost while running a program over the network.
				EXCASE(EXCEPTION_INT_DIVIDE_BY_ZERO); // The thread tried to divide an integer value by an integer divisor of zero.
				EXCASE(EXCEPTION_INT_OVERFLOW); // The result of an integer operation caused a carry out of the most significant bit of the result.
				EXCASE(EXCEPTION_INVALID_DISPOSITION); // An exception handler returned an invalid disposition to the exception dispatcher. Programmers using a high-level language such as C should never encounter this exception.
				EXCASE(EXCEPTION_NONCONTINUABLE_EXCEPTION); // The thread tried to continue execution after a noncontinuable exception occurred.
				EXCASE(EXCEPTION_PRIV_INSTRUCTION); // The thread tried to execute an instruction whose operation is not allowed in the current machine mode.
				EXCASE(EXCEPTION_SINGLE_STEP); // A trace trap or other single-instruction mechanism signaled that one instruction has been executed.
				EXCASE(EXCEPTION_STACK_OVERFLOW); // The thread used up its stack.
				default:
					wsprintfA(szName,"Exception 0x%08X",evt.u.Exception.ExceptionRecord.ExceptionCode);
				}
				wsprintfA(szDbgText,"{%i.%i} Exception 0x%08X at 0x%08X flags 0x%08X%s\n", evt.dwProcessId,evt.dwThreadId,
					pszName,
					evt.u.Exception.ExceptionRecord.ExceptionAddress,
					evt.u.Exception.ExceptionRecord.ExceptionFlags,
					(evt.u.Exception.ExceptionRecord.ExceptionFlags&EXCEPTION_NONCONTINUABLE) ? "(EXCEPTION_NONCONTINUABLE)" : "");
				_printf(szDbgText);
			}
			}
		}
		break;
		
		
		case OUTPUT_DEBUG_STRING_EVENT:
		//8 Reports an output-debugging-string debugging event. The value of u.DebugString specifies an OUTPUT_DEBUG_STRING_INFO structure.
		{
			wszDbgText[0] = 0;
			if (evt.u.DebugString.nDebugStringLength >= 1024) evt.u.DebugString.nDebugStringLength = 1023;
			DWORD_PTR nRead = 0;
			if (evt.u.DebugString.fUnicode) {
				if (!ReadProcessMemory(srv.hRootProcess, evt.u.DebugString.lpDebugStringData, wszDbgText, 2*evt.u.DebugString.nDebugStringLength, &nRead))
					lstrcpy(wszDbgText, L"???");
				else
					wszDbgText[min(1023,nRead+1)] = 0;
			} else {
				if (!ReadProcessMemory(srv.hRootProcess, evt.u.DebugString.lpDebugStringData, szDbgText, evt.u.DebugString.nDebugStringLength, &nRead))
					lstrcpy(wszDbgText, L"???");
				else {
					szDbgText[min(1023,nRead+1)] = 0;
					MultiByteToWideChar(CP_ACP, 0, szDbgText, -1, wszDbgText, 1024);
				}
			}
			_printf("{%i.%i} ", evt.dwProcessId,evt.dwThreadId, wszDbgText);
		}
		break;
		
		}
		
		// Продолжить отлаживаемый процесс
		ContinueDebugEvent(evt.dwProcessId, evt.dwThreadId, DBG_EXCEPTION_NOT_HANDLED);
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
      MCHKHEAP;
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
          _printf("CreateNamedPipe failed, ErrCode=0x%08X\n", dwErr); 
          Sleep(50);
          //return 99;
          continue;
      }
 
      // Wait for the client to connect; if it succeeds, 
      // the function returns a nonzero value. If the function
      // returns zero, GetLastError returns ERROR_PIPE_CONNECTED. 
 
      fConnected = ConnectNamedPipe(hPipe, NULL) ? 
         TRUE : (GetLastError() == ERROR_PIPE_CONNECTED); 

	  if (WaitForSingleObject(ghExitQueryEvent, 0) == WAIT_OBJECT_0) break;
 
      MCHKHEAP;
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
            _printf("CreateThread(Instance) failed, ErrCode=0x%08X\n", dwErr);
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
      MCHKHEAP;
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

		#define ALL_MODIFIERS (LEFT_ALT_PRESSED|RIGHT_ALT_PRESSED|LEFT_CTRL_PRESSED|RIGHT_CTRL_PRESSED|SHIFT_PRESSED)
		#define CTRL_MODIFIERS (LEFT_CTRL_PRESSED|RIGHT_CTRL_PRESSED)

		bool lbProcessEvent = false;
		bool lbIngoreKey = false;
		if (r.EventType == KEY_EVENT && r.Event.KeyEvent.bKeyDown &&
			(r.Event.KeyEvent.wVirtualKeyCode == 'C' || r.Event.KeyEvent.wVirtualKeyCode == VK_CANCEL)
			&&      ( // Удерживается ТОЛЬКО Ctrl
					(r.Event.KeyEvent.dwControlKeyState & CTRL_MODIFIERS) &&
					((r.Event.KeyEvent.dwControlKeyState & ALL_MODIFIERS) 
					== (r.Event.KeyEvent.dwControlKeyState & CTRL_MODIFIERS))
					)
			)
		{
			lbProcessEvent = true;

			DWORD dwMode = 0;
			GetConsoleMode(ghConIn, &dwMode);

			// CTRL+C (and Ctrl+Break?) is processed by the system and is not placed in the input buffer
			if ((dwMode & ENABLE_PROCESSED_INPUT) == ENABLE_PROCESSED_INPUT)
				lbIngoreKey = lbProcessEvent = true;
			else
				lbProcessEvent = false;


			if (lbProcessEvent) {

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

			if (lbIngoreKey)
				return;
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
	//while (GetMessage(&msg,0,0,0))
	while (TRUE) {
		if (!PeekMessage(&msg,0,0,0,PM_REMOVE)) {
			Sleep(10);
			continue;
		}
		if (msg.message == WM_QUIT)
			break;

		if (ghQuitEvent) {
			if (WaitForSingleObject(ghQuitEvent, 0) == WAIT_OBJECT_0)
				break;
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
      MCHKHEAP;
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
          _printf("CreatePipe failed, ErrCode=0x%08X\n", dwErr);
          Sleep(50);
          //return 99;
          continue;
      }
 
      // Wait for the client to connect; if it succeeds, 
      // the function returns a nonzero value. If the function
      // returns zero, GetLastError returns ERROR_PIPE_CONNECTED. 
 
      fConnected = ConnectNamedPipe(hPipe, NULL) ? 
         TRUE : (GetLastError() == ERROR_PIPE_CONNECTED); 
 
      MCHKHEAP;
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
              MCHKHEAP;
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
                  MCHKHEAP;
              }
              // next
              memset(&imsg,0,sizeof(imsg));
              MCHKHEAP;
          }
      } 
      else 
        // The client could not connect, so close the pipe. 
         SafeCloseHandle(hPipe);
   } 
   MCHKHEAP;
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

    MCHKHEAP;
 
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
		memmove(pIn, &in, cbBytesRead); // стояло ошибочное присвоение

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
		ExecutePrepareCmd((CESERVER_REQ*)&Out, in.hdr.nCmd, sizeof(Out));
    	
	    fSuccess = WriteFile( 
	        hPipe,        // handle to pipe 
	        &Out,         // buffer to write from 
	        Out.nSize,    // number of bytes to write 
	        &cbWritten,   // number of bytes written 
	        NULL);        // not overlapped I/O 
        
    } else {
	    MCHKHEAP;
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

    MCHKHEAP;
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


BOOL GetAnswerToRequest(CESERVER_REQ& in, CESERVER_REQ** out)
{
    BOOL lbRc = FALSE;

    MCHKHEAP;

    switch (in.hdr.nCmd) {
        case CECMD_GETCONSOLEINFO:
		case CECMD_REQUESTCONSOLEINFO:
        {
            if (srv.szGuiPipeName[0] == 0) { // Серверный пайп в CVirtualConsole уже должен быть запущен
                wsprintf(srv.szGuiPipeName, CEGUIPIPENAME, L".", (DWORD)ghConWnd); // был gnSelfPID
            }

            _ASSERT(ghConOut && ghConOut!=INVALID_HANDLE_VALUE);
            if (ghConOut==NULL || ghConOut==INVALID_HANDLE_VALUE)
                return FALSE;

            ReloadFullConsoleInfo(TRUE);

            MCHKHEAP;

            // На запрос из GUI (GetAnswerToRequest)
			if (in.hdr.nCmd == CECMD_GETCONSOLEINFO) {
            	//*out = CreateConsoleInfo(NULL, (in.hdr.nCmd == CECMD_GETFULLINFO));
	            int nOutSize = sizeof(CESERVER_REQ_HDR) + sizeof(CESERVER_REQ_CONINFO_HDR);
	            *out = ExecuteNewCmd(0,nOutSize);
	            (*out)->ConInfo = *srv.pConsoleInfo;
        	}

            MCHKHEAP;

            lbRc = TRUE;
        } break;
        case CECMD_SETSIZE:
		case CECMD_SETSIZESYNC:
        case CECMD_CMDSTARTED:
        case CECMD_CMDFINISHED:
        {
        	WARNING("Переделать нафиг, весь ресайз должен быть в RefreshThread");
        
            MCHKHEAP;
            int nOutSize = sizeof(CESERVER_REQ_HDR) + sizeof(CONSOLE_SCREEN_BUFFER_INFO) + sizeof(DWORD);
            *out = ExecuteNewCmd(0,nOutSize);
            if (*out == NULL) return FALSE;
            MCHKHEAP;
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

                MCHKHEAP;

                (*out)->hdr.nCmd = in.hdr.nCmd;
				// Все остальные поля заголовка уже заполнены в ExecuteNewCmd

                //#ifdef _DEBUG
                if (in.hdr.nCmd == CECMD_CMDFINISHED) {
                	PRINT_COMSPEC(L"CECMD_CMDFINISHED, Set height to: %i\n", crNewSize.Y);
                    DEBUGSTR(L"\n!!! CECMD_CMDFINISHED !!!\n\n");
                    // Вернуть нотификатор
					TODO("Смена режима рефреша консоли")
                    //if (srv.dwWinEventThread != 0)
                    //	PostThreadMessage(srv.dwWinEventThread, srv.nMsgHookEnableDisable, TRUE, 0);
                } else if (in.hdr.nCmd == CECMD_CMDSTARTED) {
                	PRINT_COMSPEC(L"CECMD_CMDSTARTED, Set height to: %i\n", nBufferHeight);
                    DEBUGSTR(L"\n!!! CECMD_CMDSTARTED !!!\n\n");
                    // Отключить нотификатор
					TODO("Смена режима рефреша консоли")
                    //if (srv.dwWinEventThread != 0)
                    //	PostThreadMessage(srv.dwWinEventThread, srv.nMsgHookEnableDisable, FALSE, 0);
                }
                //#endif

                if (in.hdr.nCmd == CECMD_CMDFINISHED) {
                    // Сохранить данные ВСЕЙ консоли
                    PRINT_COMSPEC(L"Storing long output\n", 0);
                    CmdOutputStore();
                    PRINT_COMSPEC(L"Storing long output (done)\n", 0);
                }

                MCHKHEAP;

				if (in.hdr.nCmd == CECMD_SETSIZESYNC) {
					ResetEvent(srv.hAllowInputEvent);
					srv.bInSyncResize = TRUE;
				}

                srv.nTopVisibleLine = nNewTopVisible;
                SetConsoleSize(nBufferHeight, crNewSize, rNewRect, ":CECMD_SETSIZE");

				if (in.hdr.nCmd == CECMD_SETSIZESYNC) {
					CESERVER_REQ *pPlgIn = NULL, *pPlgOut = NULL;
					if (in.SetSize.dwFarPID && !nBufferHeight) {
						// Команду можно выполнить через плагин FARа
						wchar_t szPipeName[128];
						wsprintf(szPipeName, CEPLUGINPIPENAME, L".", in.SetSize.dwFarPID);
						DWORD nHILO = ((DWORD)crNewSize.X) | (((DWORD)(WORD)crNewSize.Y) << 16);
						pPlgIn = ExecuteNewCmd(CMD_SETSIZE, sizeof(CESERVER_REQ_HDR)+sizeof(nHILO));
						pPlgIn->dwData[0] = nHILO;
						pPlgOut = ExecuteCmd(szPipeName, pPlgIn, 500, ghConWnd);
						if (pPlgOut) ExecuteFreeResult(pPlgOut);
					} else {
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
					}

					SetEvent(srv.hAllowInputEvent);
					srv.bInSyncResize = FALSE;
				}

                MCHKHEAP;

                if (in.hdr.nCmd == CECMD_CMDSTARTED) {
                    // Восстановить текст скрытой (прокрученной вверх) части консоли
                    CmdOutputRestore();
                }
            }
            MCHKHEAP;
            
            PCONSOLE_SCREEN_BUFFER_INFO psc = &((*out)->SetSizeRet.SetSizeRet);
            MyGetConsoleScreenBufferInfo(ghConOut, psc);
            
            DWORD nPacketId = ++srv.nLastPacketID;
            (*out)->SetSizeRet.nNextPacketId = nPacketId;
            
            //srv.bForceFullSend = TRUE;
            SetEvent(srv.hRefreshEvent);

            MCHKHEAP;

            lbRc = TRUE;
        } break;

        case CECMD_GETOUTPUT:
        {
            if (gpStoredOutput) {
				DWORD nSize = sizeof(CESERVER_CONSAVE_HDR)
					+ min((int)gpStoredOutput->hdr.cbMaxOneBufferSize, 
					(gpStoredOutput->hdr.sbi.dwSize.X*gpStoredOutput->hdr.sbi.dwSize.Y*2));
				ExecutePrepareCmd(
					(CESERVER_REQ*)&(gpStoredOutput->hdr), CECMD_GETOUTPUT, nSize);

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
			#ifdef _DEBUG
			WPARAM wParam = (WPARAM)in.Msg.wParam;
			LPARAM lParam = (LPARAM)in.Msg.lParam;
			
			if (in.Msg.nMsg == WM_INPUTLANGCHANGE || in.Msg.nMsg == WM_INPUTLANGCHANGEREQUEST) {
				unsigned __int64 l = lParam;
				wchar_t szDbg[255];
				wsprintf(szDbg, L"ConEmuC: %s(%s, CP:%i, HKL:0x%08I64X)\n",
					in.Msg.bPost ? L"PostMessage" : L"SendMessage",
					(in.Msg.nMsg == WM_INPUTLANGCHANGE) ? L"WM_INPUTLANGCHANGE" : L"WM_INPUTLANGCHANGEREQUEST",
					(DWORD)wParam, l);
				DEBUGLOGLANG(szDbg);
			}
			#endif

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

		case CECMD_SETCONSOLECP:
		{
			// 1. Заблокировать выполнение других нитей, работающих с консолью
			_ASSERTE(!srv.hLockRefreshBegin && !srv.hLockRefreshReady);
			srv.hLockRefreshReady = CreateEvent(0,0,0,0);
			srv.hLockRefreshBegin = CreateEvent(0,0,0,0);
			while (WaitForSingleObject(srv.hLockRefreshReady, 10) == WAIT_TIMEOUT) {
				if (WaitForSingleObject(srv.hRefreshThread, 0) == WAIT_OBJECT_0)
					break; // Нить RefreshThread завершилась!
			}

			// Может и не надо, но на всякий случай
			//MSectionLock CSCS; BOOL lbCSCS = FALSE;
			//lbCSCS = CSCS.Lock(&srv.cChangeSize, TRUE, 300);

			// 2. Закрытие всех дескрипторов консоли
			ghConIn.Close();
			ghConOut.Close();

			*out = ExecuteNewCmd(CECMD_SETCONSOLECP,sizeof(CESERVER_REQ_HDR)+sizeof(CESERVER_REQ_SETCONCP));

			// 3. Собственно, выполнение SetConsoleCP, SetConsoleOutputCP
			BOOL bCpRc = FALSE;
			if (in.SetConCP.bSetOutputCP)
				bCpRc = SetConsoleOutputCP(in.SetConCP.nCP);
			else
				bCpRc = SetConsoleCP(in.SetConCP.nCP);

			if (*out != NULL) {
				(*out)->SetConCP.bSetOutputCP = bCpRc;
				(*out)->SetConCP.nCP = GetLastError();
			}

			// 4. Отпускание других нитей
			//if (lbCSCS) CSCS.Unlock();
			SetEvent(srv.hLockRefreshBegin);
			Sleep(10);
			SafeCloseHandle(srv.hLockRefreshReady);
			SafeCloseHandle(srv.hLockRefreshBegin);

			lbRc = TRUE;
		} break;

		case CECMD_SAVEALIASES:
		{
			//wchar_t* pszAliases; DWORD nAliasesSize;
			// Запомнить алиасы
			DWORD nNewSize = in.hdr.nSize - sizeof(in.hdr);
			if (nNewSize > srv.nAliasesSize) {
				MCHKHEAP;
				wchar_t* pszNew = (wchar_t*)Alloc(nNewSize, 1);
				if (!pszNew) break; // не удалось выдеить память
				MCHKHEAP;
				if (srv.pszAliases) Free(srv.pszAliases);
				MCHKHEAP;
				srv.pszAliases = pszNew;
				srv.nAliasesSize = nNewSize;
			}
			if (nNewSize > 0 && srv.pszAliases) {
				MCHKHEAP;
				memmove(srv.pszAliases, in.Data, nNewSize);
				MCHKHEAP;
			}
		} break;
		case CECMD_GETALIASES:
		{
			// Возвращаем запомненные алиасы
			int nOutSize = sizeof(CESERVER_REQ_HDR) + srv.nAliasesSize;
			*out = ExecuteNewCmd(CECMD_GETALIASES,nOutSize);
			if (*out != NULL) {
				if (srv.pszAliases && srv.nAliasesSize)
					memmove((*out)->Data, srv.pszAliases, srv.nAliasesSize);
				lbRc = TRUE;
			}
		} break;
    }
    
    if (gbInRecreateRoot) gbInRecreateRoot = FALSE;
    return lbRc;
}







// Действует аналогично функции WinApi (GetConsoleScreenBufferInfo), но в режиме сервера:
// 1. запрещает (то есть отменяет) максимизацию консольного окна
// 2. корректирует srWindow: сбрасывает горизонтальную прокрутку,
//    а если не обнаружен "буферный режим" - то и вертикальную.
BOOL MyGetConsoleScreenBufferInfo(HANDLE ahConOut, PCONSOLE_SCREEN_BUFFER_INFO apsc)
{
    BOOL lbRc = FALSE;

    //CSection cs(NULL,NULL);
    //MSectionLock CSCS;
    //if (gnRunMode == RM_SERVER)
    	//CSCS.Lock(&srv.cChangeSize);
        //cs.Enter(&srv.csChangeSize, &srv.ncsTChangeSize);

    if (gnRunMode == RM_SERVER) // ComSpec окно менять НЕ ДОЛЖЕН!
    {
        // Если юзер случайно нажал максимизацию, когда консольное окно видимо - ничего хорошего не будет
        if (IsZoomed(ghConWnd)) {
            SendMessage(ghConWnd, WM_SYSCOMMAND, SC_RESTORE, 0);

			DWORD dwStartTick = GetTickCount();
			do {
				Sleep(20); // подождем чуть, но не больше секунды
			} while (IsZoomed(ghConWnd) && (GetTickCount()-dwStartTick)<=1000);
			Sleep(20); // и еще чуть-чуть, чтобы консоль прочухалась

            // Теперь нужно вернуть (вдруго он изменился) размер буфера консоли

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

			// И вернуть тот видимый прямоугольник, который был получен в последний раз (успешный раз)
            SetConsoleWindowInfo(ghConOut, TRUE, &srv.sbi.srWindow);
        }
    }

	CONSOLE_SCREEN_BUFFER_INFO csbi = {sizeof(CONSOLE_SCREEN_BUFFER_INFO)};

    lbRc = GetConsoleScreenBufferInfo(ahConOut, &csbi);
	// 
    _ASSERTE((csbi.srWindow.Bottom-csbi.srWindow.Top)<200);
    if (lbRc && gnRunMode == RM_SERVER) // ComSpec окно менять НЕ ДОЛЖЕН!
    {
                // Перенесено в SetConsoleSize
                //     if (gnBufferHeight) {
                //// Если мы знаем о режиме BufferHeight - можно подкорректировать размер (зачем это было сделано?)
                //         if (gnBufferHeight <= (csbi.dwMaximumWindowSize.Y * 12 / 10))
                //             gnBufferHeight = max(300, (SHORT)(csbi.dwMaximumWindowSize.Y * 12 / 10));
                //     }

        // Если прокрутки быть не должно - по возможности уберем ее, иначе при запуске FAR
        // запустится только в ВИДИМОЙ области

		// Левая и правая граница
        BOOL lbNeedCorrect = FALSE;
        if (csbi.srWindow.Left > 0) {
            lbNeedCorrect = TRUE; csbi.srWindow.Left = 0;
        }
        if ((csbi.srWindow.Right+1) < csbi.dwSize.X) {
            lbNeedCorrect = TRUE; csbi.srWindow.Right = (csbi.dwSize.X - 1);
        }

        BOOL lbBufferHeight = FALSE;
        if (csbi.dwSize.Y >= (csbi.dwMaximumWindowSize.Y * 12 / 10))
            lbBufferHeight = TRUE;

        if (!lbBufferHeight) {
            _ASSERTE((csbi.srWindow.Bottom-csbi.srWindow.Top)<200);

            if (csbi.srWindow.Top > 0) {
                lbNeedCorrect = TRUE; csbi.srWindow.Top = 0;
            }
            if ((csbi.srWindow.Bottom+1) < csbi.dwSize.Y) {
                lbNeedCorrect = TRUE; csbi.srWindow.Bottom = (csbi.dwSize.Y - 1);
            }
        }
		if (CorrectVisibleRect(&csbi))
			lbNeedCorrect = TRUE;
        if (lbNeedCorrect) {
            lbRc = SetConsoleWindowInfo(ghConOut, TRUE, &csbi.srWindow);
            lbRc = GetConsoleScreenBufferInfo(ahConOut, &csbi);
        }
    }

	// Возвращаем (возможно) скорректированные данные
	*apsc = csbi;

    //cs.Leave();
    //CSCS.Unlock();

    #ifdef _DEBUG
    if ((csbi.srWindow.Bottom - csbi.srWindow.Top)>csbi.dwMaximumWindowSize.Y) {
		// Теоретически, этого быть не может - сие значит глюк консоли?
        _ASSERTE((csbi.srWindow.Bottom - csbi.srWindow.Top)<csbi.dwMaximumWindowSize.Y);
    }
    #endif

    return lbRc;
}



// BufferHeight  - высота БУФЕРА (0 - без прокрутки)
// crNewSize     - размер ОКНА (ширина окна == ширине буфера)
// rNewRect      - для (BufferHeight!=0) определяет new upper-left and lower-right corners of the window
BOOL SetConsoleSize(USHORT BufferHeight, COORD crNewSize, SMALL_RECT rNewRect, LPCSTR asLabel)
{
    _ASSERTE(ghConWnd);
    if (!ghConWnd) return FALSE;

//#ifdef _DEBUG
//	if (gnRunMode != RM_SERVER || !srv.bDebuggerActive)
//	{
//		BOOL bFarInExecute = WaitForSingleObject(ghFarInExecuteEvent, 0) == WAIT_OBJECT_0;
//		if (BufferHeight) {
//			if (!bFarInExecute) {
//				_ASSERTE(BufferHeight && bFarInExecute);
//			}
//		}
//	}
//#endif

	DWORD dwCurThId = GetCurrentThreadId();
	DWORD dwWait = 0;

	if (gnRunMode == RM_SERVER && srv.dwRefreshThread && dwCurThId != srv.dwRefreshThread) {
		srv.nReqSizeBufferHeight = BufferHeight;
		srv.crReqSizeNewSize = crNewSize;
		srv.rReqSizeNewRect = rNewRect;
		srv.sReqSizeLabel = asLabel;
		ResetEvent(srv.hReqSizeChanged);
		srv.bRequestChangeSize = TRUE;
		
		//dwWait = WaitForSingleObject(srv.hReqSizeChanged, REQSIZE_TIMEOUT);
		// Ожидание, пока сработает RefreshThread
		HANDLE hEvents[2] = {ghQuitEvent, srv.hReqSizeChanged};
		DWORD nWait = WaitForMultipleObjects ( 2, hEvents, FALSE, REQSIZE_TIMEOUT );
		if (dwWait == (WAIT_OBJECT_0)) {
			// ghQuitEvent !!
			return FALSE;
		}
		if (dwWait == (WAIT_OBJECT_0+1)) {
			return TRUE;
		}
		// ?? Может быть стоит самим попробовать?
		return FALSE;
	}

    //CSection cs(NULL,NULL);
    //MSectionLock CSCS;
    //if (gnRunMode == RM_SERVER)
    //	CSCS.Lock(&srv.cChangeSize, TRUE, 10000);
    //    //cs.Enter(&srv.csChangeSize, &srv.ncsTChangeSize);
    
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
        if (gnBufferHeight <= (csbi.dwMaximumWindowSize.Y * 12 / 10))
            gnBufferHeight = max(300, (SHORT)(csbi.dwMaximumWindowSize.Y * 12 / 10));

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
        //srv.bForceFullSend = TRUE;
        SetEvent(srv.hRefreshEvent);
    }

    //cs.Leave();
    //CSCS.Unlock();

    return lbRc;
}

BOOL WINAPI HandlerRoutine(DWORD dwCtrlType)
{
    //PRINT_COMSPEC(L"HandlerRoutine triggered. Event type=%i\n", dwCtrlType);
	if (dwCtrlType >= CTRL_CLOSE_EVENT && dwCtrlType <= CTRL_SHUTDOWN_EVENT) {
    	PRINT_COMSPEC(L"Console about to be closed\n", 0);
		gbInShutdown = TRUE;
		
		// Остановить отладчик, иначе отлаживаемый процесс тоже схлопнется	
	    if (srv.bDebuggerActive) {
	    	if (pfnDebugActiveProcessStop) pfnDebugActiveProcessStop(srv.dwRootProcess);
	    	srv.bDebuggerActive = FALSE;
	    }
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

const wchar_t* PointToName(const wchar_t* asFullPath)
{
	const wchar_t* pszFile = wcsrchr(asFullPath, L'\\');
	if (!pszFile) pszFile = asFullPath; else pszFile++;
	return pszFile;
}

#ifdef CRTPRINTF
WARNING("Можно облегчить... заменить на wvsprintf");
//void _printf(LPCSTR asFormat, ...)
//{
//    va_list argList; va_start(argList, an_StrResId);
//    char szError[2000]; -- только нужно учесть длину %s
//    wvsprintf(szError, asFormat, argList);
//}

void _printf(LPCSTR asFormat, DWORD dw1, DWORD dw2, LPCWSTR asAddLine)
{
	char szError[MAX_PATH];
	wsprintfA(szError, asFormat, dw1, dw2);
	_printf(szError);
	if (asAddLine) {
		_wprintf(asAddLine);
		_printf("\n");
	}
}

void _printf(LPCSTR asFormat, DWORD dwErr, LPCWSTR asAddLine)
{
	char szError[MAX_PATH];
	wsprintfA(szError, asFormat, dwErr);
	_printf(szError);
	_wprintf(asAddLine);
	_printf("\n");
}

void _printf(LPCSTR asFormat, DWORD dwErr)
{
	char szError[MAX_PATH];
	wsprintfA(szError, asFormat, dwErr);
	_printf(szError);
}

void _printf(LPCSTR asBuffer)
{
	if (!asBuffer) return;
	int nAllLen = lstrlenA(asBuffer);
	HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
	DWORD dwWritten = 0;
	WriteFile(hOut, asBuffer, nAllLen, &dwWritten, 0);
}

void _wprintf(LPCWSTR asBuffer)
{
	if (!asBuffer) return;
	int nAllLen = lstrlenW(asBuffer);
	HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
	DWORD dwWritten = 0;
	UINT nOldCP = GetConsoleOutputCP();
	char* pszOEM = (char*)malloc(nAllLen+1);
	if (pszOEM) {
		WideCharToMultiByte(nOldCP,0, asBuffer, nAllLen, pszOEM, nAllLen, 0,0);
		pszOEM[nAllLen] = 0;
		WriteFile(hOut, pszOEM, nAllLen, &dwWritten, 0);
		free(pszOEM);
	} else {
		WriteFile(hOut, asBuffer, nAllLen*2, &dwWritten, 0);
	}
}
#endif

//BOOL IsUserAdmin()
//{
//	OSVERSIONINFO osv = {sizeof(OSVERSIONINFO)};
//	GetVersionEx(&osv);
//	// Проверять нужно только для висты, чтобы на XP лишний "Щит" не отображался
//	if (osv.dwMajorVersion < 6)
//		return FALSE;
//
//	BOOL b;
//	SID_IDENTIFIER_AUTHORITY NtAuthority = SECURITY_NT_AUTHORITY;
//	PSID AdministratorsGroup;
//
//	b = AllocateAndInitializeSid(
//		&NtAuthority,
//		2,
//		SECURITY_BUILTIN_DOMAIN_RID,
//		DOMAIN_ALIAS_RID_ADMINS,
//		0, 0, 0, 0, 0, 0,
//		&AdministratorsGroup); 
//	if(b) 
//	{
//		if (!CheckTokenMembership(NULL, AdministratorsGroup, &b)) 
//		{
//			b = FALSE;
//		}
//		FreeSid(AdministratorsGroup);
//	}
//	return(b);
//}

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
				wsprintfW(szDbg, 
					L"ConEmuC: InputLayoutChanged (GetConsoleKeyboardLayoutName returns) '%s'\n", 
					szCurKeybLayout);
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
	ghConOut.Close();
	WARNING("А вот это нужно бы делать в RefreshThread!!!");
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
	_ASSERTE(nWhole>0);
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
