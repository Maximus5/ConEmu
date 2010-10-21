
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


#ifdef _DEBUG
//  Раскомментировать, чтобы сразу после запуска процесса (conemuc.exe) показать MessageBox, чтобы прицепиться дебаггером
//  #define SHOW_STARTED_MSGBOX
//  #define SHOW_COMSPEC_STARTED_MSGBOX
//  #define SHOW_SERVER_STARTED_MSGBOX
//  #define SHOW_STARTED_ASSERT
#elif defined(__GNUC__)
//  Раскомментировать, чтобы сразу после запуска процесса (conemuc.exe) показать MessageBox, чтобы прицепиться дебаггером
//  #define SHOW_STARTED_MSGBOX
#else
//
#endif

#include "ConEmuC.h"

WARNING("Обязательно после запуска сделать apiSetForegroundWindow на GUI окно, если в фокусе консоль");
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
//MConHandle ghConIn ( L"CONIN$" );
MConHandle ghConOut ( L"CONOUT$" );


/*  Global  */
DWORD   gnSelfPID = 0;
BOOL    gbTerminateOnExit = FALSE;
//HANDLE  ghConIn = NULL, ghConOut = NULL;
HWND    ghConWnd = NULL;
HWND    ghConEmuWnd = NULL; // Root! window
HANDLE  ghExitQueryEvent = NULL;
HANDLE  ghQuitEvent = NULL;
bool    gbQuit = false;
BOOL	gbInShutdown = FALSE;
int     gnConfirmExitParm = 0; // 1 - CONFIRM, 2 - NOCONFIRM
BOOL    gbAlwaysConfirmExit = FALSE;
BOOL	gbAutoDisableConfirmExit = FALSE; // если корневой процесс проработал достаточно (10 сек) - будет сброшен gbAlwaysConfirmExit
BOOL    gbRootAliveLess10sec = FALSE; // корневой процесс проработал менее CHECK_ROOTOK_TIMEOUT
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
OSVERSIONINFO gOSVer;


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
	memset(&gOSVer, 0, sizeof(gOSVer));
	gOSVer.dwOSVersionInfoSize = sizeof(gOSVer);
	GetVersionEx(&gOSVer);
	
	gpNullSecurity = NullSecurity();
	
	HMODULE hKernel = GetModuleHandleW (L"kernel32.dll");
	
	if (hKernel)
	{
		pfnGetConsoleKeyboardLayoutName = (FGetConsoleKeyboardLayoutName)GetProcAddress (hKernel, "GetConsoleKeyboardLayoutNameW");
		pfnGetConsoleProcessList = (FGetConsoleProcessList)GetProcAddress (hKernel, "GetConsoleProcessList");
	}
	

	// Хэндл консольного окна
	ghConWnd = GetConsoleWindow();
	_ASSERTE(ghConWnd!=NULL);
	if (!ghConWnd)
	{
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
	if (!IsDebuggerPresent())
	{
		wchar_t szTitle[100]; wsprintf(szTitle, L"ConEmuC Loaded (PID=%i)", gnSelfPID);
		const wchar_t* pszCmdLine = GetCommandLineW();
		MessageBox(NULL,pszCmdLine,szTitle,0);
	}
#endif
#ifdef SHOW_STARTED_ASSERT
	if (!IsDebuggerPresent())
	{
		_ASSERT(FALSE);
	}
#endif

	PRINT_COMSPEC(L"ConEmuC started: %s\n", GetCommandLineW());
	
	if ((iRc = ParseCommandLine(GetCommandLineW(), &gpszRunCmd)) != 0)
		goto wrap;
	//#ifdef _DEBUG
	//CreateLogSizeFile();
	//#endif

#ifdef SHOW_SERVER_STARTED_MSGBOX
	if ((gnRunMode == RM_SERVER) && !IsDebuggerPresent())
	{
		wchar_t szTitle[100]; wsprintf(szTitle, L"ConEmuC [Server] started (PID=%i)", gnSelfPID);
		const wchar_t* pszCmdLine = GetCommandLineW();
		MessageBox(NULL,pszCmdLine,szTitle,0);
	}
#endif
	
	/* ***************************** */
	/* *** "Общая" инициализация *** */
	/* ***************************** */
	
	
	// Событие используется для всех режимов
	ghExitQueryEvent = CreateEvent(NULL, TRUE/*используется в нескольких нитях, manual*/, FALSE, NULL);
	if (!ghExitQueryEvent)
	{
		dwErr = GetLastError();
		_printf("CreateEvent() failed, ErrCode=0x%08X\n", dwErr); 
		iRc = CERR_EXITEVENT; goto wrap;
	}
	ResetEvent(ghExitQueryEvent);
	ghQuitEvent = CreateEvent(NULL, TRUE/*используется в нескольких нитях, manual*/, FALSE, NULL);
	if (!ghQuitEvent)
	{
		dwErr = GetLastError();
		_printf("CreateEvent() failed, ErrCode=0x%08X\n", dwErr); 
		iRc = CERR_EXITEVENT; goto wrap;
	}
	ResetEvent(ghQuitEvent);

	// Дескрипторы
	//ghConIn  = CreateFile(L"CONIN$", GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_READ,
	//            0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
	//if ((HANDLE)ghConIn == INVALID_HANDLE_VALUE) {
	//    dwErr = GetLastError();
	//    _printf("CreateFile(CONIN$) failed, ErrCode=0x%08X\n", dwErr); 
	//    iRc = CERR_CONINFAILED; goto wrap;
	//}
	// Дескрипторы
	//ghConOut = CreateFile(L"CONOUT$", GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_READ,
	//            0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
	if (gnRunMode == RM_SERVER)
	{
		if ((HANDLE)ghConOut == INVALID_HANDLE_VALUE)
		{
			dwErr = GetLastError();
			_printf("CreateFile(CONOUT$) failed, ErrCode=0x%08X\n", dwErr); 
			iRc = CERR_CONOUTFAILED; goto wrap;
		}
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
	if (gnRunMode == RM_SERVER)
	{
		if ((iRc = ServerInit()) != 0)
			goto wrap;
	}
	else
	{
		if ((iRc = ComspecInit()) != 0)
			goto wrap;
	}

	
	
	
	/* ********************************* */
	/* *** Запуск дочернего процесса *** */
	/* ********************************* */
	
	// CREATE_NEW_PROCESS_GROUP - низя, перестает работать Ctrl-C
	// Перед CreateProcessW нужно ставить 0, иначе из-за антивирусов может наступить
	// timeout ожидания окончания процесса еще ДО выхода из CreateProcessW
	if (!gbAttachMode)
		srv.nProcessStartTick = 0;
	if (gbNoCreateProcess)
	{
		lbRc = TRUE; // Процесс уже запущен, просто цепляемся к ConEmu (GUI)
		pi.hProcess = srv.hRootProcess;
		pi.dwProcessId = srv.dwRootProcess;
	}
	else
	{

		#ifdef _DEBUG
		if (ghFarInExecuteEvent && wcsstr(gpszRunCmd,L"far.exe"))
			ResetEvent(ghFarInExecuteEvent);
		#endif
		
		WARNING("The process handle must have the PROCESS_VM_OPERATION access right!");
		lbRc = CreateProcessW(NULL, gpszRunCmd, NULL,NULL, TRUE, 
				NORMAL_PRIORITY_CLASS/*|CREATE_NEW_PROCESS_GROUP*/
				|((gnRunMode == RM_SERVER) ? CREATE_SUSPENDED : 0), 
				NULL, NULL, &si, &pi);
		dwErr = GetLastError();
		
		if (lbRc && (gnRunMode == RM_SERVER))
		{
			TODO("Не только в сервере, но и в ComSpec, чтобы дочерние КОНСОЛЬНЫЕ процессы могли пользоваться редиректами");

			//""F:\VCProject\FarPlugin\ConEmu\Bugs\DOS\TURBO.EXE ""
			TODO("При выполнении DOS приложений - VirtualAllocEx(hProcess, обламывается!");
			TODO("В принципе - завелось, но в сочетании с Anamorphosis получается странное зацикливание far->conemu->anamorph->conemu");
			//InjectHooks(pi.hProcess, pi.dwProcessId);
		
			// Отпустить процесс
			ResumeThread(pi.hThread);
		}

		if (!lbRc && dwErr == 0x000002E4)
		{
			// Допустимо только в режиме comspec - тогда запустится новая консоль
			_ASSERTE(gnRunMode != RM_SERVER);
			PRINT_COMSPEC(L"Vista+: The requested operation requires elevation (ErrCode=0x%08X).\n", dwErr);
			// Vista: The requested operation requires elevation.
			LPCWSTR pszCmd = gpszRunCmd;
			wchar_t szVerb[10], szExec[MAX_PATH+1];
			if (NextArg(&pszCmd, szExec) == 0)
			{
				SHELLEXECUTEINFO sei = {sizeof(SHELLEXECUTEINFO)};
				sei.hwnd = ghConEmuWnd;
				sei.fMask = SEE_MASK_NO_CONSOLE; //SEE_MASK_NOCLOSEPROCESS; -- смысла ждать завершения нет - процесс запускается в новой консоли
				sei.lpVerb = wcscpy(szVerb, L"open");
				sei.lpFile = szExec;
				sei.lpParameters = pszCmd;
				sei.nShow = SW_SHOWNORMAL;
				if ((lbRc = ShellExecuteEx(&sei)) == FALSE)
				{
					dwErr = GetLastError();
				}
				else
				{
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
					// т.к. запустилась новая консоль - подтверждение на закрытие этой точно не нужно
					DisableAutoConfirmExit();
					iRc = 0; goto wrap;
				}
			}
		}
	}
	if (!lbRc)
	{
		wchar_t* lpMsgBuf = NULL;
		DWORD nFmtRc, nFmtErr = 0;
		if (dwErr == 5)
		{
			lpMsgBuf = (wchar_t*)LocalAlloc(LPTR, 255);
			wcscpy(lpMsgBuf, L"Access is denied.\nThis may be cause of antiviral or file permissions denial.");
		}
		else
		{
			nFmtRc = FormatMessage( FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, 
				NULL, dwErr, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPWSTR)&lpMsgBuf, 0, NULL );
			if (!nFmtRc)
				nFmtErr = GetLastError();
		}
		
		_printf("Can't create process, ErrCode=0x%08X, Description:\n", dwErr);
		_wprintf((lpMsgBuf == NULL) ? L"<Unknown error>" : lpMsgBuf);
		_printf("\nCommand to be executed:\n");
		_wprintf(gpszRunCmd);
		_printf("\n");
		
		if (lpMsgBuf) LocalFree(lpMsgBuf);
		iRc = CERR_CREATEPROCESS; goto wrap;
	}
	if (gbAttachMode)
	{
		// мы цепляемся к уже существующему процессу: 
		// аттач из фар плагина или запуск dos-команды в новой консоли через -new_console
		// в последнем случае отключение подтверждения закрытия однозначно некорректно
		// -- DisableAutoConfirmExit(); - низя
	}
	else
	{
		srv.nProcessStartTick = GetTickCount();
	}
	//delete psNewCmd; psNewCmd = NULL;
	AllowSetForegroundWindow(pi.dwProcessId);


	
	/* ************************ */
	/* *** Ожидание запуска *** */
	/* ************************ */
	
	if (gnRunMode == RM_SERVER)
	{
		srv.hRootProcess  = pi.hProcess; pi.hProcess = NULL; // Required for Win2k
		srv.hRootThread   = pi.hThread;  pi.hThread  = NULL;
		srv.dwRootProcess = pi.dwProcessId;
		srv.dwRootThread  = pi.dwThreadId;
		srv.dwRootStartTime = GetTickCount();

		// Скорее всего процесс в консольном списке уже будет
		CheckProcessCount(TRUE);
		#ifdef _DEBUG
		if (srv.nProcessCount && !srv.bDebuggerActive)
		{
			_ASSERTE(srv.pnProcesses[srv.nProcessCount-1]!=0);
		}
		#endif

		//if (pi.hProcess) SafeCloseHandle(pi.hProcess); 
		//if (pi.hThread) SafeCloseHandle(pi.hThread);

		if (srv.hConEmuGuiAttached)
		{
			DWORD dwWaitGui = WaitForSingleObject(srv.hConEmuGuiAttached, 1000);
			if (dwWaitGui == WAIT_OBJECT_0)
			{
				// GUI пайп готов
				wsprintf(srv.szGuiPipeName, CEGUIPIPENAME, L".", (DWORD)ghConWnd); // был gnSelfPID
			}
		}
	
		// Ждем, пока в консоли не останется процессов (кроме нашего)
		TODO("Проверить, может ли так получиться, что CreateProcess прошел, а к консоли он не прицепился? Может, если процесс GUI");
		nWait = WaitForSingleObject(ghExitQueryEvent, CHECK_ANTIVIRUS_TIMEOUT); //Запуск процесса наверное может задержать антивирус
		if (nWait != WAIT_OBJECT_0) // Если таймаут
		{
			iRc = srv.nProcessCount;
			// И процессов в консоли все еще нет
			if (iRc == 1 && !srv.bDebuggerActive)
			{
				_printf ("Process was not attached to console. Is it GUI?\nCommand to be executed:\n");
				_wprintf (gpszRunCmd);
				_printf ("\n");
				iRc = CERR_PROCESSTIMEOUT; goto wrap;
			}
		}
	}
	else
	{
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
	if (gnRunMode == RM_SERVER)
	{
		// По крайней мере один процесс в консоли запустился. Ждем пока в консоли не останется никого кроме нас
		nWait = WAIT_TIMEOUT;
		if (!srv.bDebuggerActive)
		{
			#ifdef _DEBUG
			while (nWait == WAIT_TIMEOUT)
			{
				nWait = WaitForSingleObject(ghExitQueryEvent, 100);
			}
			#else
			nWait = WaitForSingleObject(ghExitQueryEvent, INFINITE);
			#endif
		}
		else
		{
			while (nWait == WAIT_TIMEOUT)
			{
				ProcessDebugEvent();
				nWait = WaitForSingleObject(ghExitQueryEvent, 0);
			}
			//gbAlwaysConfirmExit = TRUE;
		}
		#ifdef _DEBUG
		if (nWait == WAIT_OBJECT_0)
		{
			DEBUGSTR(L"*** FinilizeEvent was set!\n");
		}
		#endif
	}
	else
	{
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
	if (!gbInShutdown // только если юзер не нажал крестик в заголовке окна
		&& ((iRc!=0 && iRc!=CERR_RUNNEWCONSOLE && iRc!=CERR_EMPTY_COMSPEC_CMDLINE) || gbAlwaysConfirmExit)
		)
	{
		BOOL lbProcessesLeft = FALSE, lbDontShowConsole = FALSE;
		if (pfnGetConsoleProcessList)
		{
			DWORD nProcesses[10];
			DWORD nProcCount = pfnGetConsoleProcessList ( nProcesses, 10 );
			if (nProcCount > 1)
				lbProcessesLeft = TRUE;
		}

		LPCWSTR pszMsg = NULL;
		if (lbProcessesLeft)
		{
			pszMsg = L"\n\nPress Enter to exit...";
			lbDontShowConsole = gnRunMode != RM_SERVER;
		}
		else
		{
			if (gbRootWasFoundInCon == 1)
			{
				if (gbRootAliveLess10sec) // корневой процесс проработал менее CHECK_ROOTOK_TIMEOUT
					pszMsg = L"\n\nConEmuC: Root process was alive less than 10 sec.\nPress Enter to close console...";
				else
					pszMsg = L"\n\nPress Enter to close console...";
			}
		}
		if (!pszMsg) // Иначе - сообщение по умолчанию
			pszMsg = L"\n\nPress Enter to close console, or wait...";

		ExitWaitForKey(VK_RETURN, pszMsg, TRUE, lbDontShowConsole);
		if (iRc == CERR_PROCESSTIMEOUT)
		{
			int nCount = srv.nProcessCount;
			if (nCount > 1 || srv.bDebuggerActive)
			{
				// Процесс таки запустился!
				goto wait;
			}
		}
	}

	// На всякий случай - выставим событие
	if (ghExitQueryEvent) SetEvent(ghExitQueryEvent);
	
	// Завершение RefreshThread, InputThread, ServerThread
	if (ghQuitEvent) SetEvent(ghQuitEvent);
	
	
	/* ***************************** */
	/* *** "Режимное" завершение *** */
	/* ***************************** */
	
	if (gnRunMode == RM_SERVER)
	{
		ServerDone(iRc, true);
		//MessageBox(0,L"Server done...",L"ConEmuC",0);
	}
	else if (gnRunMode == RM_COMSPEC)
	{
		ComspecDone(iRc);
		//MessageBox(0,L"Comspec done...",L"ConEmuC",0);
	}

	
	/* ************************** */
	/* *** "Общее" завершение *** */
	/* ************************** */
	
	if (gpszPrevConTitle && ghConWnd)
	{
		SetConsoleTitleW(gpszPrevConTitle);
		Free(gpszPrevConTitle);
	}

	LogSize(NULL, "Shutdown");
	
	//ghConIn.Close();
	ghConOut.Close();

	SafeCloseHandle(ghLogSize);
	if (wpszLogSizeFile)
	{
		//DeleteFile(wpszLogSizeFile);
		Free(wpszLogSizeFile); wpszLogSizeFile = NULL;
	}

#ifdef _DEBUG
	SafeCloseHandle(ghFarInExecuteEvent);
#endif
	
	if (gpszRunCmd) { delete gpszRunCmd; gpszRunCmd = NULL; }
	
	CommonShutdown();

	if (ghHeap)
	{
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
		"     /[NO]CONFIRM - [don't] confirm closing console on program termination\n"
		"     /ATTACH      - auto attach to ConEmu GUI\n"
		"     /NOCMD       - attach current (existing) console to GUI\n"
		"     /B{W|H|Z}    - define window width, height and buffer height\n"
		"     /F{N|W|H}    - define console font name, width, height\n"
		"     /LOG[N]      - create (debug) log file, N is number from 0 to 3\n"
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
	if (!lbFirstWasGot)
	{
		szArg[0] = 0;
		// 17.10.2010 - поддержка переданного исполняемого файла без параметров, но с пробелами в пути
		LPCWSTR pchEnd = pwszCopy + lstrlenW(pwszCopy);
		while (pchEnd > pwszCopy && *(pchEnd-1) == L' ') pchEnd--;
		if ((pchEnd - pwszCopy) < MAX_PATH)
		{
			memcpy(szArg, pwszCopy, (pchEnd - pwszCopy)*sizeof(wchar_t));
			szArg[(pchEnd - pwszCopy)] = 0;
			if (!FileExists(szArg))
				szArg[0] = 0;
		}

		if (szArg[0] == 0)
		{
			if ((iRc = NextArg(&pwszCopy, szArg)) != 0)
			{
				//Parsing command line failed
				return TRUE;
			}
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
		gbAlwaysConfirmExit = TRUE; gbAutoDisableConfirmExit = FALSE;
		return FALSE; // уже указан командный процессор, cmd.exe в начало добавлять не нужно
	}

	if (lstrcmpiW(pwszCopy, L"far")==0 || lstrcmpiW(pwszCopy, L"far.exe")==0)
	{
		gbAutoDisableConfirmExit = TRUE;
		gbRootIsCmdExe = FALSE; // FAR!
		return FALSE; // уже указан командный процессор, cmd.exe в начало добавлять не нужно
	}

	if (IsExecutable(szArg))
	{
		gbRootIsCmdExe = FALSE; // Для других программ - буфер не включаем
		return FALSE; // Запускается конкретная консольная программа. cmd.exe не требуется
	}

	//Можно еще Доделать поиски с: SearchPath, GetFullPathName, добавив расширения .exe & .com
	//хотя фар сам формирует полные пути к командам, так что можно не заморачиваться

	gbRootIsCmdExe = TRUE;
	#pragma warning( pop )
	return TRUE;
}

//BOOL FileExists(LPCWSTR asFile)
//{
//	WIN32_FIND_DATA fnd; memset(&fnd, 0, sizeof(fnd));
//	HANDLE h = FindFirstFile(asFile, &fnd);
//	if (h != INVALID_HANDLE_VALUE) {
//		FindClose(h);
//		return (fnd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0;
//	}
//	return FALSE;
//}

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

void RegisterConsoleFontHKLM(LPCWSTR pszFontFace)
{
	if (!pszFontFace || !*pszFontFace)
		return;

	HKEY hk;
	if (!RegOpenKeyExW(HKEY_LOCAL_MACHINE, L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Console\\TrueTypeFont",
		0, KEY_ALL_ACCESS, &hk))
	{
		wchar_t szId[32] = {0}, szFont[255]; DWORD dwLen, dwType;
		for (DWORD i = 0; i <20; i++) {
			szId[i] = L'0'; szId[i+1] = 0; wmemset(szFont, 0, 255);
			if (RegQueryValueExW(hk, szId, NULL, &dwType, (LPBYTE)szFont, &(dwLen = 255*2))) {
				RegSetValueExW(hk, szId, 0, REG_SZ, (LPBYTE)pszFontFace, (lstrlen(pszFontFace)+1)*2);
				break;
			}
			if (lstrcmpi(szFont, pszFontFace) == 0) {
				break; // он уже добавлен
			}
		}
		RegCloseKey(hk);
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
		if ((szArg[0] == L'/' || szArg[0] == L'-')
			&& (szArg[1] == L'?' || ((szArg[1] & ~0x20) == L'H'))
			&& szArg[2] == 0)
		{
			Help();
			return CERR_HELPREQUESTED;
		}

		// Далее - требуется чтобы у аргумента был "/"
		if (szArg[0] != L'/')
			continue;

		if (wcsncmp(szArg, L"/REGCONFONT=", 12)==0)
		{
			RegisterConsoleFontHKLM(szArg+12);
			return CERR_EMPTY_COMSPEC_CMDLINE;
		}
		else if (wcscmp(szArg, L"/CONFIRM")==0) {
			TODO("уточнить, что нужно в gbAutoDisableConfirmExit");
			gnConfirmExitParm = 1;
			gbAlwaysConfirmExit = TRUE; gbAutoDisableConfirmExit = TRUE;
		}
		else if (wcscmp(szArg, L"/NOCONFIRM")==0)
		{
			gnConfirmExitParm = 2;
			gbAlwaysConfirmExit = FALSE; gbAutoDisableConfirmExit = FALSE;
		}

		else if (wcscmp(szArg, L"/ATTACH")==0)
		{
			gbAttachMode = TRUE;
			gnRunMode = RM_SERVER;
		}

		else if (wcscmp(szArg, L"/HIDE")==0)
		{
			gbForceHideConWnd = TRUE;
		}

		else if (wcsncmp(szArg, L"/B", 2)==0)
		{
			wchar_t* pszEnd = NULL;
			if (wcsncmp(szArg, L"/BW=", 4)==0)
			{
				gcrBufferSize.X = /*_wtoi(szArg+4);*/(SHORT)wcstol(szArg+4,&pszEnd,10); gbParmBufferSize = TRUE;
			}
			else if (wcsncmp(szArg, L"/BH=", 4)==0)
			{
				gcrBufferSize.Y = /*_wtoi(szArg+4);*/(SHORT)wcstol(szArg+4,&pszEnd,10); gbParmBufferSize = TRUE;
			}
			else if (wcsncmp(szArg, L"/BZ=", 4)==0)
			{
				gnBufferHeight = /*_wtoi(szArg+4);*/(SHORT)wcstol(szArg+4,&pszEnd,10); gbParmBufferSize = TRUE;
			}
		}

		else if (wcsncmp(szArg, L"/F", 2)==0)
		{
			wchar_t* pszEnd = NULL;
			if (wcsncmp(szArg, L"/FN=", 4)==0)
			{
				lstrcpynW(srv.szConsoleFont, szArg+4, 32);
			}
			else if (wcsncmp(szArg, L"/FW=", 4)==0)
			{
				srv.nConFontWidth = /*_wtoi(szArg+4);*/(SHORT)wcstol(szArg+4,&pszEnd,10);
			}
			else if (wcsncmp(szArg, L"/FH=", 4)==0)
			{
				srv.nConFontHeight = /*_wtoi(szArg+4);*/(SHORT)wcstol(szArg+4,&pszEnd,10);
			//} else if (wcsncmp(szArg, L"/FF=", 4)==0) {
			//  lstrcpynW(srv.szConsoleFontFile, szArg+4, MAX_PATH);
			}
		}
		
		else if (wcsncmp(szArg, L"/LOG",4)==0)
		{
			int nLevel = 0;
			if (szArg[4]==L'1') nLevel = 1; else if (szArg[4]>=L'2') nLevel = 2;
			CreateLogSizeFile(nLevel);
		}
		
		else if (wcscmp(szArg, L"/NOCMD")==0)
		{
			gnRunMode = RM_SERVER;
			gbNoCreateProcess = TRUE;
		}

		else if (wcsncmp(szArg, L"/GID=", 5)==0)
		{
			gnRunMode = RM_SERVER;
			wchar_t* pszEnd = NULL;
			srv.dwGuiPID = wcstoul(szArg+5, &pszEnd, 10);
			if (srv.dwGuiPID == 0)
			{
				_printf ("Invalid GUI PID specified:\n");
				_wprintf (GetCommandLineW());
				_printf ("\n");
				return CERR_CARGUMENT;
			}
		}
		
		else if (wcsncmp(szArg, L"/PID=", 5)==0)
		{
			gnRunMode = RM_SERVER;
			gbNoCreateProcess = TRUE;
			wchar_t* pszEnd = NULL;
			//srv.dwRootProcess = _wtol(szArg+5);
			srv.dwRootProcess = wcstoul(szArg+5, &pszEnd, 10);
			if (srv.dwRootProcess == 0) {
				_printf ("Attach to GUI was requested, but invalid PID specified:\n");
				_wprintf (GetCommandLineW());
				_printf ("\n");
				return CERR_CARGUMENT;
			}
		}

		else if (wcsncmp(szArg, L"/DEBUGPID=", 10)==0)
		{
			gnRunMode = RM_SERVER;
			gbNoCreateProcess = gbDebugProcess = TRUE;
			wchar_t* pszEnd = NULL;
			//srv.dwRootProcess = _wtol(szArg+10);
			srv.dwRootProcess = wcstoul(szArg+10, &pszEnd, 10);
			if (srv.dwRootProcess == 0) {
				_printf ("Debug of process was requested, but invalid PID specified:\n");
				_wprintf (GetCommandLineW());
				_printf ("\n");
				return CERR_CARGUMENT;
			}
		}
		
		else if (wcscmp(szArg, L"/A")==0 || wcscmp(szArg, L"/a")==0)
		{
			gnCmdUnicodeMode = 1;
		}
		else if (wcscmp(szArg, L"/U")==0 || wcscmp(szArg, L"/u")==0)
		{
			gnCmdUnicodeMode = 2;
		} 
		
		// После этих аргументов - идет то, что передается в CreateProcess!
		else if (wcscmp(szArg, L"/ROOT")==0 || wcscmp(szArg, L"/root")==0)
		{
			gnRunMode = RM_SERVER; gbNoCreateProcess = FALSE;
			break; // asCmdLine уже указывает на запускаемую программу
		}

		// После этих аргументов - идет то, что передается в COMSPEC (CreateProcess)!
		//if (wcscmp(szArg, L"/C")==0 || wcscmp(szArg, L"/c")==0 || wcscmp(szArg, L"/K")==0 || wcscmp(szArg, L"/k")==0) {
		else if (szArg[0] == L'/' && (((szArg[1] & ~0x20) == L'C') || ((szArg[1] & ~0x20) == L'K')))
		{
			gbNoCreateProcess = FALSE;
			if (szArg[2] == 0) // "/c" или "/k"
				gnRunMode = RM_COMSPEC;
			if (gnRunMode == RM_UNDEFINED && szArg[4] == 0
				&& ((szArg[2] & ~0x20) == L'M') && ((szArg[3] & ~0x20) == L'D'))
				gnRunMode = RM_SERVER;
			// Если тип работа до сих пор не определили - считаем что режим ComSpec
			// и команда начинается сразу после /c (может быть "cmd /cecho xxx")
			if (gnRunMode == RM_UNDEFINED)
			{
				gnRunMode = RM_COMSPEC;
				// Поддержка дебильной возможности "cmd /cecho xxx"
				asCmdLine = pszArgStarts + 2;
				while (*asCmdLine==L' ' || *asCmdLine==L'\t') asCmdLine++;
			}
			if (gnRunMode == RM_COMSPEC)
			{
				cmd.bK = (szArg[1] & ~0x20) == L'K';
			}
			break; // asCmdLine уже указывает на запускаемую программу
		}
	}
	
	if (gnRunMode == RM_SERVER)
	{
		if (gbDebugProcess)
		{
			if (!DebugActiveProcess(srv.dwRootProcess))
			{
				DWORD dwErr = GetLastError();
				_printf("Can't start debugger! ErrCode=0x%08X\n", dwErr);
				return CERR_CANTSTARTDEBUGGER;
			}
			pfnDebugActiveProcessStop = (FDebugActiveProcessStop)GetProcAddress(GetModuleHandle(L"kernel32.dll"),"DebugActiveProcessStop");
			pfnDebugSetProcessKillOnExit = (FDebugSetProcessKillOnExit)GetProcAddress(GetModuleHandle(L"kernel32.dll"),"DebugSetProcessKillOnExit");
			if (pfnDebugSetProcessKillOnExit) pfnDebugSetProcessKillOnExit(FALSE/*KillOnExit*/);
			srv.bDebuggerActive = TRUE;
		
			*psNewCmd = (wchar_t*)Alloc(1,2);
			if (!*psNewCmd)
			{
				_printf ("Can't allocate 1 wchar!\n");
				return CERR_NOTENOUGHMEM1;
			}
			(*psNewCmd)[0] = 0;
			return 0;
		}
		else if (gbNoCreateProcess && gbAttachMode)
		{
			if (pfnGetConsoleProcessList==NULL)
			{
				_printf ("Attach to GUI was requested, but required WinXP or higher:\n");
				_wprintf (GetCommandLineW());
				_printf ("\n");
				return CERR_CARGUMENT;
			}
			DWORD nProcesses[10];
			DWORD nProcCount = pfnGetConsoleProcessList ( nProcesses, 10 );
			if (nProcCount < 2)
			{
				_printf ("Attach to GUI was requested, but there is no console processes:\n");
				_wprintf (GetCommandLineW());
				_printf ("\n");
				return CERR_CARGUMENT;
			}
			// Если cmd.exe запущен из cmd.exe (в консоли уже больше двух процессов) - ничего не делать
			if (nProcCount > 2)
			{
				// И ругаться только под отладчиком
				wchar_t szProc[255] ={0}, szTmp[10]; //wsprintfW(szProc, L"%i, %i, %i", nProcesses[0], nProcesses[1], nProcesses[2]);
				for (DWORD n=0; n<nProcCount && n<20; n++)
				{
					if (n) lstrcatW(szProc, L", ");
					wsprintf(szTmp, L"%i", nProcesses[0]);
					//lstrcatW(szProc, _ltow(nProcesses[0], szTmp, 10));
					lstrcatW(szProc, szTmp);
				}
				PRINT_COMSPEC(L"Attach to GUI was requested, but there is more then 2 console processes: %s\n", szProc);
				return CERR_CARGUMENT;
			}

			*psNewCmd = (wchar_t*)Alloc(1,2);
			if (!*psNewCmd)
			{
				_printf ("Can't allocate 1 wchar!\n");
				return CERR_NOTENOUGHMEM1;
			}
			(*psNewCmd)[0] = 0;
			return 0;
		}
	}

	if (iRc != 0)
	{
		if (iRc == CERR_CMDLINEEMPTY)
		{
			Help();
			_printf ("\n\nParsing command line failed (/C argument not found):\n");
			_wprintf (GetCommandLineW());
			_printf ("\n");
		}
		else
		{
			_printf ("Parsing command line failed:\n");
			_wprintf (asCmdLine);
			_printf ("\n");
		}
		return iRc;
	}
	if (gnRunMode == RM_UNDEFINED)
	{
		_printf ("Parsing command line failed (/C argument not found):\n");
		_wprintf (GetCommandLineW());
		_printf ("\n");
		return CERR_CARGUMENT;
	}

	if (gnRunMode == RM_COMSPEC)
	{
	
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
			size_t nNewLen = wcslen(pwszStartCmdLine) + 200;
			//
			BOOL lbIsNeedCmd = IsNeedCmd(asCmdLine, &lbNeedCutStartEndQuot);

			//Warning. ParseCommandLine вызывается ДО ComSpecInit, в котором зовется
			//         CECMD_CMDSTARTSTOP, поэтому высота буфера еще не была установлена.
			SendStarted();
			
			// Font, size, etc.
			
			CESERVER_REQ *pIn = NULL, *pOut = NULL;
			wchar_t* pszAddNewConArgs = NULL;
			// 
			if ((pIn = ExecuteNewCmd(CECMD_GETNEWCONPARM, sizeof(CESERVER_REQ_HDR)+2*sizeof(DWORD))) != NULL)
			{
				pIn->dwData[0] = gnSelfPID;
				pIn->dwData[1] = lbIsNeedCmd;
				
				PRINT_COMSPEC(L"Retrieve new console add args (begin)\n",0);
				pOut = ExecuteGuiCmd(ghConWnd, pIn, ghConWnd);
				PRINT_COMSPEC(L"Retrieve new console add args (begin)\n",0);
				
				if (pOut)
				{
					pszAddNewConArgs = (wchar_t*)pOut->Data;
					if (*pszAddNewConArgs == 0)
					{
						ExecuteFreeResult(pOut); pOut = NULL; pszAddNewConArgs = NULL;
					}
					else
					{
						nNewLen += wcslen(pszAddNewConArgs) + 1;
					}
				}
				ExecuteFreeResult(pIn); pIn = NULL;
			}
			//
			wchar_t* pszNewCmd = (wchar_t*)Alloc(nNewLen+1,2);
			if (!pszNewCmd)
			{
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

			if (pszAddNewConArgs)
			{
				wcscat(pszNewCmd, L" ");
				wcscat(pszNewCmd, pszAddNewConArgs);
			}
			else
			{
				
				// -new_console вызывается в режиме ComSpec. Хорошо бы сейчас открыть мэппинг консоли на чтение, получить GuiPID и добавить в аргументы
				MFileMapping<CESERVER_REQ_CONINFO_HDR> ConMap;
				ConMap.InitName(CECONMAPNAME, (DWORD)ghConWnd);
				const CESERVER_REQ_CONINFO_HDR* pConMap = ConMap.Open();
				if (pConMap)
				{
					if (pConMap->nGuiPID)
					{
						wsprintf(pszNewCmd+wcslen(pszNewCmd), L" /GID=%i ", pConMap->nGuiPID);
					}
					ConMap.CloseMap();
				}

				// Размеры должны быть такими-же
				//2009-08-13 было закомментарено (в режиме ComSpec аргументы /BW /BH /BZ отсутствуют, т.к. запуск идет из FAR)
				//			 иногда получалось, что требуемый размер (он запрашивается из GUI) 
				//			 не успевал установиться и в некоторых случаях возникал
				//			 глюк размера (видимой высоты в GUI) в ReadConsoleData
				if (MyGetConsoleScreenBufferInfo(ghConOut, &cmd.sbi))
				{
					int nBW = cmd.sbi.dwSize.X;
					int nBH = cmd.sbi.srWindow.Bottom - cmd.sbi.srWindow.Top + 1;
					int nBZ = cmd.sbi.dwSize.Y;
					if (nBZ <= nBH) nBZ = 0;
					wsprintf(pszNewCmd+wcslen(pszNewCmd), L" /BW=%i /BH=%i /BZ=%i ", nBW, nBH, nBZ);
				}
				//wcscat(pszNewCmd, L" </BW=9999 /BH=9999 /BZ=9999> ");
			}

			// Сформировать новую команду
			// "cmd" потому что пока не хочется обрезать кавычки и думать, реально ли он нужен
			// cmd /c ""c:\program files\arc\7z.exe" -?"   // да еще и внутри могут быть двойными...
			// cmd /c "dir c:\"
			// и пр.
			// Попытаться определить необходимость cmd
			if (lbIsNeedCmd)
			{
				CheckUnicodeMode();
				if (gnCmdUnicodeMode == 2)
					wcscat(pszNewCmd, L" /ROOT cmd /U /C ");
				else if (gnCmdUnicodeMode == 1)
					wcscat(pszNewCmd, L" /ROOT cmd /A /C ");
				else
					wcscat(pszNewCmd, L" /ROOT cmd /C ");
			}
			else
			{
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
			DisableAutoConfirmExit();
			//srv.nProcessStartTick = GetTickCount() - 2*CHECK_ROOTSTART_TIMEOUT; //2010-03-06
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

	if (gnRunMode == RM_COMSPEC && (!asCmdLine || !*asCmdLine))
	{
		if (cmd.bK)
		{
			bViaCmdExe = TRUE;
		}
		else
		{
			// В фаре могут повесить пустую ассоциацию на маску
			// *.ini -> "@" - тогда фар как бы ничего не делает при запуске этого файла, но ComSpec зовет...
			cmd.bNonGuiMode = TRUE;
			DisableAutoConfirmExit();
			return CERR_EMPTY_COMSPEC_CMDLINE;
		}
	}
	else
	{
		bViaCmdExe = IsNeedCmd(asCmdLine, &lbNeedCutStartEndQuot);
	}
	
	nCmdLine = lstrlenW(asCmdLine);

	if (!bViaCmdExe)
	{
		nCmdLine += 1; // только место под 0
	}
	else
	{
		// Если определена ComSpecC - значит ConEmuC переопределил стандартный ComSpec
		if (!GetEnvironmentVariable(L"ComSpecC", szComSpec, MAX_PATH) || szComSpec[0] == 0)
			if (!GetEnvironmentVariable(L"ComSpec", szComSpec, MAX_PATH) || szComSpec[0] == 0)
				szComSpec[0] = 0;

		if (szComSpec[0] != 0)
		{
			// Только если это (случайно) не conemuc.exe
			//pwszCopy = wcsrchr(szComSpec, L'\\'); if (!pwszCopy) pwszCopy = szComSpec;
			pwszCopy = PointToName(szComSpec);
			#pragma warning( push )
			#pragma warning(disable : 6400)
			if (lstrcmpiW(pwszCopy, L"ConEmuC")==0 || lstrcmpiW(pwszCopy, L"ConEmuC.exe")==0
				|| lstrcmpiW(pwszCopy, L"ConEmuC64")==0 || lstrcmpiW(pwszCopy, L"ConEmuC64.exe")==0)
				szComSpec[0] = 0;
			#pragma warning( pop )
		}
		
		// ComSpec/ComSpecC не определен, используем cmd.exe
		if (szComSpec[0] == 0)
		{
			if (!SearchPathW(NULL, L"cmd.exe", NULL, MAX_PATH, szComSpec, &psFilePart))
			{
				_printf ("Can't find cmd.exe!\n");
				return CERR_CMDEXENOTFOUND;
			}
		}

		nCmdLine += lstrlenW(szComSpec)+15; // "/C", кавычки и возможный "/U"
	}

	*psNewCmd = (wchar_t*)Alloc(nCmdLine+1,2);
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
				*pszEndQ = 0; pszTitle ++;
				// "C:\Program Files\FAR\far.exe" - кавычки нужны, иначе не запустится
				DWORD dwFileAttr = GetFileAttributes(pszTitle);
				if (dwFileAttr != INVALID_FILE_ATTRIBUTES && !(dwFileAttr & FILE_ATTRIBUTE_DIRECTORY))
					lbNeedCutStartEndQuot = FALSE;
				else
					lbNeedCutStartEndQuot = TRUE;
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
		//BOOL lbNeedQuatete = FALSE;
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
	if (!IsWindowVisible(ghConWnd)) {
		SetWindowPos(ghConWnd, HWND_TOP, 50,50,0,0, SWP_NOSIZE);
		apiShowWindowAsync(ghConWnd, SW_SHOWNORMAL);
	}
	if (!IsWindowEnabled(ghConWnd))
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
					//apiShowWindow(ghConWnd, SW_SHOWNORMAL); // и покажем окошко
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
	//        apiShowWindow(ghConWnd, SW_SHOWNORMAL); // и покажем окошко
	//    }
	while (TRUE) {
		if (!PeekConsoleInput(GetStdHandle(STD_INPUT_HANDLE), &r, 1, &dwCount))
			dwCount = 0;
	
		if (gnRunMode == RM_SERVER) {
			int nCount = srv.nProcessCount;
			if (nCount > 1) {
				// Теперь Peek, так что просто выходим
				//// ! Процесс таки запустился, закрываться не будем. Вернуть событие в буфер!
				//WriteConsoleInput(GetStdHandle(STD_INPUT_HANDLE), &r, 1, &dwCount);
				break;
			}
		}
		
		
		if (gbInShutdown) {
			break;
		}
		if (dwCount) {
			if (ReadConsoleInput(GetStdHandle(STD_INPUT_HANDLE), &r, 1, &dwCount) && dwCount) {
				if (r.EventType == KEY_EVENT && r.Event.KeyEvent.bKeyDown 
				 	&& r.Event.KeyEvent.wVirtualKeyCode == vkKey)
				break;
			}
		}
		Sleep(50);
	}
	//MessageBox(0,L"Debug message...............1",L"ConEmuC",0);
	//int nCh = _getch();
	if (abNewLine)
		_printf("\n");
}





void SendStarted()
{    
	static bool bSent = false;
	if (bSent)
		return; // отсылать только один раз
	
	//crNewSize = cmd.sbi.dwSize;
	//_ASSERTE(crNewSize.X>=MIN_CON_WIDTH && crNewSize.Y>=MIN_CON_HEIGHT);

	HWND hConWnd = GetConsoleWindow();
	_ASSERTE(hConWnd == ghConWnd);
	ghConWnd = hConWnd;
	// Для ComSpec-а сразу можно проверить, а есть-ли сервер в этой консоли...
	if (gnRunMode == RM_COMSPEC) {
		DWORD nServerPID = 0, nGuiPID = 0;

		MFileMapping<CESERVER_REQ_CONINFO_HDR> ConsoleMap;
		ConsoleMap.InitName(CECONMAPNAME, (DWORD)hConWnd);
		const CESERVER_REQ_CONINFO_HDR* pConsoleInfo = ConsoleMap.Open();
		//WCHAR sHeaderMapName[64];
		//wsprintf(sHeaderMapName, CECONMAPNAME, (DWORD)hConWnd);
		//HANDLE hFileMapping = OpenFileMapping(FILE_MAP_READ/*|FILE_MAP_WRITE*/, FALSE, sHeaderMapName);
		//if (hFileMapping) {
		//	const CESERVER_REQ_CONINFO_HDR* pConsoleInfo 
		//		= (CESERVER_REQ_CONINFO_HDR*)MapViewOfFile(hFileMapping, FILE_MAP_READ/*|FILE_MAP_WRITE*/,0,0,0);
			if (pConsoleInfo) {
				nServerPID = pConsoleInfo->nServerPID;
				nGuiPID = pConsoleInfo->nGuiPID;
				if (pConsoleInfo->cbSize >= sizeof(CESERVER_REQ_CONINFO_HDR)) {
					if (pConsoleInfo->nLogLevel)
						CreateLogSizeFile(pConsoleInfo->nLogLevel);
				}
				//UnmapViewOfFile(pConsoleInfo);
				ConsoleMap.CloseMap();
			}
		//	CloseHandle(hFileMapping);
		//}

		if (nServerPID == 0) {
			cmd.bNonGuiMode = TRUE; // Не посылать ExecuteGuiCmd при выходе. Это не наша консоль
			return; // Режим ComSpec, но сервера нет, соответственно, в GUI ничего посылать не нужно
		}
	}
	
	CESERVER_REQ *pIn = NULL, *pOut = NULL;
	int nSize = sizeof(CESERVER_REQ_HDR)+sizeof(CESERVER_REQ_STARTSTOP);
	pIn = ExecuteNewCmd(CECMD_CMDSTARTSTOP, nSize);
	if (pIn) {
		pIn->StartStop.nStarted = (gnRunMode == RM_COMSPEC) ? 2 : 0; // Cmd/Srv режим начат
		pIn->StartStop.hWnd = ghConWnd;
		pIn->StartStop.dwPID = gnSelfPID;
		//pIn->StartStop.dwInputTID = (gnRunMode == RM_SERVER) ? srv.dwInputThreadId : 0;
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
		HANDLE hOut = NULL;
		if (gnRunMode == RM_SERVER)
			hOut = (HANDLE)ghConOut;
		else
			hOut = GetStdHandle(STD_OUTPUT_HANDLE);
		DWORD dwErr1 = 0;
		BOOL lbRc1 = GetConsoleScreenBufferInfo(hOut, &pIn->StartStop.sbi);
		if (!lbRc1) dwErr1 = GetLastError();

		PRINT_COMSPEC(L"Starting %s mode (ExecuteGuiCmd started)\n",(RunMode==RM_SERVER) ? L"Server" : L"ComSpec");
		
		pOut = ExecuteGuiCmd(ghConWnd, pIn, ghConWnd);
		// Ждать при ошибке открытия пайпа наверное и не нужно - все что необходимо, сервер
		// уже передал в ServerInit, а ComSpec - не критично
		//if (!pOut) {
		//	// При старте консоли GUI может не успеть создать командные пайпы, т.к.
		//	// их имена основаны на дескрипторе консольного окна, а его заранее GUI не знает
		//	// Поэтому нужно чуть-чуть подождать, пока GUI поймает событие 
		//	// (anEvent == EVENT_CONSOLE_START_APPLICATION && idObject == (LONG)mn_ConEmuC_PID)
		//	DWORD dwStart = GetTickCount(), dwDelta = 0;
		//	while (!gbInShutdown && dwDelta < GUIREADY_TIMEOUT) {
		//		Sleep(10);
		//		pOut = ExecuteGuiCmd(ghConWnd, pIn, ghConWnd);
		//		if (pOut) break;

		//		dwDelta = GetTickCount() - dwStart;
		//	}
		//	if (!pOut) {
		//		// Возможно под отладчиком, или скорее всего GUI свалился
		//		_ASSERTE(pOut != NULL);
		//	}
		//}

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
			
			if (gnRunMode == RM_SERVER)
			{
				if (srv.bDebuggerActive && !gnBufferHeight) gnBufferHeight = 1000;
				SMALL_RECT rcNil = {0};
				SetConsoleSize(gnBufferHeight, gcrBufferSize, rcNil, "::SendStarted");
				// Смена раскладки клавиатуры
				if (pOut->StartStopRet.bNeedLangChange)
				{
					#ifndef INPUTLANGCHANGE_SYSCHARSET
					#define INPUTLANGCHANGE_SYSCHARSET 0x0001
					#endif
					WPARAM wParam = INPUTLANGCHANGE_SYSCHARSET;
					TODO("Проверить на x64, не будет ли проблем с 0xFFFFFFFFFFFFFFFFFFFFF");
					LPARAM lParam = (LPARAM)(DWORD_PTR)pOut->StartStopRet.NewConsoleLang;
					SendMessage(ghConWnd, WM_INPUTLANGCHANGEREQUEST, wParam, lParam);
				}
			}
			else
			{
				// Может так получиться, что один COMSPEC запущен из другого.
				// 100628 - неактуально. COMSPEC сбрасывается в cmd.exe
				//if (bAlreadyBufferHeight)
				//	cmd.bNonGuiMode = TRUE; // Не посылать ExecuteGuiCmd при выходе - прокрутка должна остаться
				cmd.bWasBufferHeight = bAlreadyBufferHeight;
			}

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
		_printf("Create console log file failed! ErrCode=0x%08X\n", dwErr, szFile);
		return;
	}
	
	int nLen = lstrlen(szFile);
	wpszLogSizeFile = /*_wcsdup(szFile);*/(wchar_t*)Alloc(nLen+1,2);
	lstrcpy(wpszLogSizeFile, szFile);
	// OK, лог создали
	LPCSTR pszCmdLine = GetCommandLineA();
	if (pszCmdLine) {
		WriteFile(ghLogSize, pszCmdLine, (DWORD)strlen(pszCmdLine), &dwErr, 0);
		WriteFile(ghLogSize, "\r\n", 2, &dwErr, 0);
	}
	LogSize(NULL, "Startup");
}

void LogString(LPCSTR asText)
{
	if (!ghLogSize) return;

	char szInfo[255]; szInfo[0] = 0;
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
	//if (dwId == srv.dwInputThreadId)
	//	pszThread = "InputThread";
	//else
	if (dwId == srv.dwInputPipeThreadId)
		pszThread = "InputPipeThread";


	SYSTEMTIME st; GetLocalTime(&st);
	wsprintfA(szInfo, "%i:%02i:%02i.%03i ",
		st.wHour, st.wMinute, st.wSecond, st.wMilliseconds);
	int nCur = lstrlenA(szInfo);
	lstrcpynA(szInfo+nCur, asText ? asText : "", 255-nCur-3);
	lstrcatA(szInfo, "\r\n");


	DWORD dwLen = 0;
	WriteFile(ghLogSize, szInfo, (DWORD)strlen(szInfo), &dwLen, 0);
	FlushFileBuffers(ghLogSize);
}

void LogSize(COORD* pcrSize, LPCSTR pszLabel)
{
	if (!ghLogSize) return;
	
	CONSOLE_SCREEN_BUFFER_INFO lsbi = {{0,0}};
	// В дебажный лог помещаем реальный значения
	GetConsoleScreenBufferInfo(ghConOut ? ghConOut : GetStdHandle(STD_OUTPUT_HANDLE), &lsbi);
	
	char szInfo[192]; szInfo[0] = 0;
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
	//if (dwId == srv.dwInputThreadId)
	//		pszThread = "InputThread";
	//		else
	if (dwId == srv.dwInputPipeThreadId)
			pszThread = "InputPipeThread";
			
	/*HDESK hDesk = GetThreadDesktop ( GetCurrentThreadId() );
	HDESK hInp = OpenInputDesktop ( 0, FALSE, GENERIC_READ );*/
	
			
	SYSTEMTIME st; GetLocalTime(&st);
	//char szMapSize[32]; szMapSize[0] = 0;
	//if (srv.pConsoleMap->IsValid()) {
	//	wsprintfA(szMapSize, " CurMapSize={%ix%ix%i}",
	//		srv.pConsoleMap->Ptr()->sbi.dwSize.X, srv.pConsoleMap->Ptr()->sbi.dwSize.Y,
	//		srv.pConsoleMap->Ptr()->sbi.srWindow.Bottom-srv.pConsoleMap->Ptr()->sbi.srWindow.Top+1);
	//}
	
	if (pcrSize)
	{
		wsprintfA(szInfo, "%i:%02i:%02i.%03i CurSize={%ix%i} ChangeTo={%ix%i} %s %s\r\n",
			st.wHour, st.wMinute, st.wSecond, st.wMilliseconds,
			lsbi.dwSize.X, lsbi.dwSize.Y, pcrSize->X, pcrSize->Y, pszThread, (pszLabel ? pszLabel : ""));
	}
	else
	{
		wsprintfA(szInfo, "%i:%02i:%02i.%03i CurSize={%ix%i} %s %s\r\n",
			st.wHour, st.wMinute, st.wSecond, st.wMilliseconds,
			lsbi.dwSize.X, lsbi.dwSize.Y, pszThread, (pszLabel ? pszLabel : ""));
	}
	
	//if (hInp) CloseDesktop ( hInp );
	
	DWORD dwLen = 0;
	WriteFile(ghLogSize, szInfo, (DWORD)strlen(szInfo), &dwLen, 0);
	FlushFileBuffers(ghLogSize);
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

		if (nCurCount == 0) {
			// Это значит в Win7 свалился conhost.exe
			#ifdef _DEBUG
			DWORD dwErr = GetLastError();
			#endif
			srv.nProcessCount = 1;
			SetEvent(ghQuitEvent);
			SetEvent(ghExitQueryEvent);
			return TRUE;
		}

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
			_ASSERTE(nCurCount>0);
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
									if (lstrcmpiW(prc.szExeFile, L"far.exe")==0)
									{
										lbFarExists = TRUE;
										//if (srv.nProcessCount <= 2) // нужно проверить и ntvdm
										//	break; // возможно, в консоли еще есть и telnet?
									}
									#ifndef WIN64
									else if (!srv.nNtvdmPID && lstrcmpiW(prc.szExeFile, L"ntvdm.exe")==0)
									{
										srv.nNtvdmPID = prc.th32ProcessID;
									}
									#endif
									// 23.04.2010 Maks - telnet нужно определять, т.к. у него проблемы с Ins и курсором
									else if (lstrcmpiW(prc.szExeFile, L"telnet.exe")==0)
									{
										lbTelnetExist = TRUE;
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

							if (lbFarExists && lbTelnetExist
								#ifndef WIN64
								&& srv.nNtvdmPID
								#endif
								)
								break; // чтобы условие выхода внятнее было
						} while (Process32Next(hSnap, &prc));
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

	// Если корневой процесс проработал достаточно (10 сек), значит он живой и gbAlwaysConfirmExit можно сбросить
	// Если gbAutoDisableConfirmExit==FALSE - сброс подтверждение закрытия консоли не выполняется
	if (gbAlwaysConfirmExit // если уже не сброшен
		&& gbAutoDisableConfirmExit // требуется ли вообще такая проверка (10 сек)
		&& nPrevCount > 1 // если в консоли был зафиксирован запущенный процесс
		&& srv.hRootProcess) // и корневой процесс был вообще запущен
	{
		if ((dwLastCheckTick - srv.nProcessStartTick) > CHECK_ROOTOK_TIMEOUT) {
			// эта проверка выполняется один раз
			gbAutoDisableConfirmExit = FALSE;

			// 10 сек. прошло, теперь необходимо проверить, а жив ли процесс?
			DWORD dwProcWait = WaitForSingleObject(srv.hRootProcess, 0);
			if (dwProcWait == WAIT_OBJECT_0) {
				// Корневой процесс завершен, возможно, была какая-то проблема?
				gbRootAliveLess10sec = TRUE; // корневой процесс проработал менее 10 сек
			} else {
				// Корневой процесс все еще работает, считаем что все ок и подтверждения закрытия консоли не потребуется
				DisableAutoConfirmExit();
				//srv.nProcessStartTick = GetTickCount() - 2*CHECK_ROOTSTART_TIMEOUT; // менять nProcessStartTick не нужно. проверка только по флажкам
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
		//2010-03-06 - установка флажка должна быть при старте сервера
		//if (!gbAlwaysConfirmExit) gbAlwaysConfirmExit = TRUE; // чтобы консоль не схлопнулась
	}
	if (nPrevCount > 1 && srv.nProcessCount == 1)
	{
		if (srv.pnProcesses[0] != gnSelfPID) {
			_ASSERTE(srv.pnProcesses[0] == gnSelfPID);
		} else {
			CS.Unlock();
			//2010-03-06 это не нужно, проверки делаются по другому
			//if (!gbAlwaysConfirmExit && (dwLastCheckTick - srv.nProcessStartTick) <= CHECK_ROOTSTART_TIMEOUT) {
			//	gbAlwaysConfirmExit = TRUE; // чтобы консоль не схлопнулась
			//}
			if (gbAlwaysConfirmExit && (dwLastCheckTick - srv.nProcessStartTick) <= CHECK_ROOTSTART_TIMEOUT)
				gbRootAliveLess10sec = TRUE; // корневой процесс проработал менее 10 сек
			SetEvent(ghExitQueryEvent);
		}
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
			switch (evt.dwDebugEventCode)
			{
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
			switch (evt.dwDebugEventCode)
			{
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
			if (evt.u.Exception.ExceptionRecord.NumberParameters>=2)
			{
				wsprintfA(szDbgText,"{%i.%i} EXCEPTION_ACCESS_VIOLATION at 0x%08X flags 0x%08X%s %s of 0x%08X\n", evt.dwProcessId,evt.dwThreadId,
					evt.u.Exception.ExceptionRecord.ExceptionAddress,
					evt.u.Exception.ExceptionRecord.ExceptionFlags,
					((evt.u.Exception.ExceptionRecord.ExceptionFlags&EXCEPTION_NONCONTINUABLE) ? "(EXCEPTION_NONCONTINUABLE)" : ""),
					((evt.u.Exception.ExceptionRecord.ExceptionInformation[0]==0) ? "Read" :
					(evt.u.Exception.ExceptionRecord.ExceptionInformation[0]==1) ? "Write" :
					(evt.u.Exception.ExceptionRecord.ExceptionInformation[0]==8) ? "DEP" : "???"),
					evt.u.Exception.ExceptionRecord.ExceptionInformation[1]
					);
			}
			else
			{
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
		  Sleep(10);
		  continue;
	  }
 
	  // Wait for the client to connect; if it succeeds, 
	  // the function returns a nonzero value. If the function
	  // returns zero, GetLastError returns ERROR_PIPE_CONNECTED. 
 
	  fConnected = ConnectNamedPipe(hPipe, NULL) ? 
		 TRUE : (GetLastError() == ERROR_PIPE_CONNECTED); 

	  if (WaitForSingleObject(ghQuitEvent, 0) == WAIT_OBJECT_0)
		  break;
 
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
			Sleep(10);
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
			cbBytesRead < sizeof(CESERVER_REQ_HDR) || in.hdr.cbSize < sizeof(CESERVER_REQ_HDR))
	{
		goto wrap;
	}

	if (in.hdr.cbSize > cbBytesRead)
	{
		DWORD cbNextRead = 0;
		// Тут именно Alloc, а не ExecuteNewCmd, т.к. данные пришли снаружи, а не заполняются здесь
		pIn = (CESERVER_REQ*)Alloc(in.hdr.cbSize, 1);
		if (!pIn)
			goto wrap;
		memmove(pIn, &in, cbBytesRead); // стояло ошибочное присвоение

		fSuccess = ReadFile(
			hPipe,        // handle to pipe 
			((LPBYTE)pIn)+cbBytesRead,  // buffer to receive data 
			in.hdr.cbSize - cbBytesRead,   // size of buffer 
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
			Out.cbSize,    // number of bytes to write 
			&cbWritten,   // number of bytes written 
			NULL);        // not overlapped I/O 
		
	} else {
		MCHKHEAP;
		// Write the reply to the pipe. 
		fSuccess = WriteFile( 
			hPipe,        // handle to pipe 
			pOut,         // buffer to write from 
			pOut->hdr.cbSize,  // number of bytes to write 
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
	//if (!fSuccess || pOut->hdr.cbSize != cbWritten) break; 

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
		//case CECMD_GETCONSOLEINFO:
		//case CECMD_REQUESTCONSOLEINFO:
		//{
		//	if (srv.szGuiPipeName[0] == 0) { // Серверный пайп в CVirtualConsole уже должен быть запущен
		//		wsprintf(srv.szGuiPipeName, CEGUIPIPENAME, L".", (DWORD)ghConWnd); // был gnSelfPID
		//	}

		//	_ASSERT(ghConOut && ghConOut!=INVALID_HANDLE_VALUE);
		//	if (ghConOut==NULL || ghConOut==INVALID_HANDLE_VALUE)
		//		return FALSE;

		//	ReloadFullConsoleInfo(TRUE);

		//	MCHKHEAP;

		//	// На запрос из GUI (GetAnswerToRequest)
		//	if (in.hdr.nCmd == CECMD_GETCONSOLEINFO) {
		//		//*out = CreateConsoleInfo(NULL, (in.hdr.nCmd == CECMD_GETFULLINFO));
		//		int nOutSize = sizeof(CESERVER_REQ_HDR) + sizeof(CESERVER_REQ_CONINFO_HDR);
		//		*out = ExecuteNewCmd(0,nOutSize);
		//		(*out)->ConInfo = srv.pConsole->info;
		//	}

		//	MCHKHEAP;

		//	lbRc = TRUE;
		//} break;
		//case CECMD_SETSIZE:
		case CECMD_SETSIZESYNC:
		case CECMD_SETSIZENOSYNC:
		case CECMD_CMDSTARTED:
		case CECMD_CMDFINISHED:
		{
			MCHKHEAP;
			int nOutSize = sizeof(CESERVER_REQ_HDR) + sizeof(CONSOLE_SCREEN_BUFFER_INFO) + sizeof(DWORD);
			*out = ExecuteNewCmd(0,nOutSize);
			if (*out == NULL) return FALSE;
			MCHKHEAP;
			if (in.hdr.cbSize >= (sizeof(CESERVER_REQ_HDR) + sizeof(CESERVER_REQ_SETSIZE))) {
				USHORT nBufferHeight = 0;
				COORD  crNewSize = {0,0};
				SMALL_RECT rNewRect = {0};
				SHORT  nNewTopVisible = -1;
				//memmove(&nBufferHeight, in.Data, sizeof(USHORT));
				nBufferHeight = in.SetSize.nBufferHeight;
				if (nBufferHeight == -1) {
					// Для 'far /w' нужно оставить высоту буфера!
					if (in.SetSize.size.Y < srv.sbi.dwSize.Y
						&& srv.sbi.dwSize.Y > (srv.sbi.srWindow.Bottom - srv.sbi.srWindow.Top + 1))
					{
						nBufferHeight = srv.sbi.dwSize.Y;
					} else {
						nBufferHeight = 0;
					}
				}
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
				WARNING("Если указан dwFarPID - это что-ли два раза подряд выполнится?");
				SetConsoleSize(nBufferHeight, crNewSize, rNewRect, ":CECMD_SETSIZESYNC");

				WARNING("!! Не может ли возникнуть конфликт с фаровским фиксом для убирания полос прокрутки?");
				if (in.hdr.nCmd == CECMD_SETSIZESYNC) {
					CESERVER_REQ *pPlgIn = NULL, *pPlgOut = NULL;
					//TODO("Пока закомментарим, чтобы GUI реагировало побыстрее");
					if (in.SetSize.dwFarPID /*&& !nBufferHeight*/) {
						// Во избежание каких-то накладок FAR (по крайней мере с /w)
						// стал ресайзить панели только после дерганья мышкой над консолью
						CONSOLE_SCREEN_BUFFER_INFO sc = {{0,0}};
						GetConsoleScreenBufferInfo(ghConOut, &sc);
						HANDLE hIn = GetStdHandle(STD_INPUT_HANDLE);
						INPUT_RECORD r = {MOUSE_EVENT};
						r.Event.MouseEvent.dwMousePosition.X = sc.srWindow.Right-1;
						r.Event.MouseEvent.dwMousePosition.Y = sc.srWindow.Bottom-1;
						r.Event.MouseEvent.dwEventFlags = MOUSE_MOVED;
						DWORD cbWritten = 0;
						WriteConsoleInput(hIn, &r, 1, &cbWritten);

						// Команду можно выполнить через плагин FARа
						wchar_t szPipeName[128];
						wsprintf(szPipeName, CEPLUGINPIPENAME, L".", in.SetSize.dwFarPID);
						//DWORD nHILO = ((DWORD)crNewSize.X) | (((DWORD)(WORD)crNewSize.Y) << 16);
						//pPlgIn = ExecuteNewCmd(CMD_SETSIZE, sizeof(CESERVER_REQ_HDR)+sizeof(nHILO));
						pPlgIn = ExecuteNewCmd(CMD_REDRAWFAR, sizeof(CESERVER_REQ_HDR));
						//pPlgIn->dwData[0] = nHILO;
						pPlgOut = ExecuteCmd(szPipeName, pPlgIn, 500, ghConWnd);
						if (pPlgOut) ExecuteFreeResult(pPlgOut);
					}

					SetEvent(srv.hAllowInputEvent);
					srv.bInSyncResize = FALSE;

					// Передернуть RefreshThread - перечитать консоль
					ReloadFullConsoleInfo(FALSE); // вызовет Refresh в нити Refresh
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
			
			DWORD lnNextPacketId = ++srv.nLastPacketID;
			(*out)->SetSizeRet.nNextPacketId = lnNextPacketId;
			
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
					// Чтобы не отображалась "Press any key to close console"
					DisableAutoConfirmExit();
					//
					(*out)->dwData[0] = (DWORD)hDc;
					lbRc = TRUE;
				}
			}
		}  break;

		case CECMD_FARLOADED:
		{
			if (gbAutoDisableConfirmExit && srv.dwRootProcess == in.dwData[0]) {
				// FAR нормально запустился, считаем что все ок и подтверждения закрытия консоли не потребуется
				DisableAutoConfirmExit();
			}
		} break;
		
		//case CECMD_SHOWCONSOLE:
		//{
		//	apiShowWindow(ghConWnd, in.dwData[0]);
		//	if (in.dwData[0] != SW_HIDE) {
		//		SetWindowPos(ghConWnd, HWND_TOPMOST, 0,0,0,0, SWP_NOMOVE|SWP_NOSIZE);
		//	}
		//} break;

		case CECMD_POSTCONMSG:
		{
			HWND hSendWnd = (HWND)in.Msg.hWnd;

			// Info & Log
			if (in.Msg.nMsg == WM_INPUTLANGCHANGE || in.Msg.nMsg == WM_INPUTLANGCHANGEREQUEST)
			{
				WPARAM wParam = (WPARAM)in.Msg.wParam;
				LPARAM lParam = (LPARAM)in.Msg.lParam;
				unsigned __int64 l = lParam;

				#ifdef _DEBUG
				wchar_t szDbg[255];
				wsprintf(szDbg, L"ConEmuC: %s(0x%08X, %s, CP:%i, HKL:0x%08I64X)\n",
					in.Msg.bPost ? L"PostMessage" : L"SendMessage", (DWORD)hSendWnd,
					(in.Msg.nMsg == WM_INPUTLANGCHANGE) ? L"WM_INPUTLANGCHANGE" :
					(in.Msg.nMsg == WM_INPUTLANGCHANGEREQUEST) ? L"WM_INPUTLANGCHANGEREQUEST" :
					(in.Msg.nMsg == WM_SHOWWINDOW) ? L"WM_SHOWWINDOW" :
					L"<Other message>",					
					(DWORD)wParam, l);
				DEBUGLOGLANG(szDbg);
				#endif
				
				if (ghLogSize)
				{
					char szInfo[255];
					wsprintfA(szInfo, "ConEmuC: %s(0x%08X, %s, CP:%i, HKL:0x%08I64X)",
						in.Msg.bPost ? "PostMessage" : "SendMessage", (DWORD)hSendWnd,
						(in.Msg.nMsg == WM_INPUTLANGCHANGE) ? "WM_INPUTLANGCHANGE" :
						(in.Msg.nMsg == WM_INPUTLANGCHANGEREQUEST) ? "WM_INPUTLANGCHANGEREQUEST" :
						"<Other message>",					
						(DWORD)wParam, l);
					LogString(szInfo);
				}
			}

			if (in.Msg.nMsg == WM_SHOWWINDOW)
			{
				DWORD lRc = 0;
				if (in.Msg.bPost)
					lRc = apiShowWindowAsync(hSendWnd, (int)(in.Msg.wParam & 0xFFFF));
				else
					lRc = apiShowWindow(hSendWnd, (int)(in.Msg.wParam & 0xFFFF));
				// Возвращаем результат
				DWORD dwErr = GetLastError();
				int nOutSize = sizeof(CESERVER_REQ_HDR) + 2*sizeof(DWORD);
				*out = ExecuteNewCmd(CECMD_POSTCONMSG,nOutSize);
				if (*out != NULL) {
					(*out)->dwData[0] = lRc;
					(*out)->dwData[1] = dwErr;
					lbRc = TRUE;
				}
			}
			else if (in.Msg.nMsg == WM_SIZE)
			{
				//
			}
			else if (in.Msg.bPost)
			{
				PostMessage(hSendWnd, in.Msg.nMsg, (WPARAM)in.Msg.wParam, (LPARAM)in.Msg.lParam);
			}
			else
			{
				LRESULT lRc = SendMessage(hSendWnd, in.Msg.nMsg, (WPARAM)in.Msg.wParam, (LPARAM)in.Msg.lParam);
				// Возвращаем результат
				int nOutSize = sizeof(CESERVER_REQ_HDR) + sizeof(u64);
				*out = ExecuteNewCmd(CECMD_POSTCONMSG,nOutSize);
				if (*out != NULL)
				{
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
			//ghConIn.Close();
			ghConOut.Close(); // попытка CECMD_SETCONSOLECP

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
			DWORD nNewSize = in.hdr.cbSize - sizeof(in.hdr);
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

		case CECMD_SETDONTCLOSE:
		{
			// После детача в фаре команда (например dir) схлопнется, чтобы
			// консоль неожиданно не закрылась...
			gbAutoDisableConfirmExit = FALSE;
			gbAlwaysConfirmExit = TRUE;
		} break;

		case CECMD_ONACTIVATION:
		{
			if (srv.pConsole)
			{
				srv.pConsole->hdr.bConsoleActive = in.dwData[0];
				srv.pConsole->hdr.bThawRefreshThread = in.dwData[1];
				srv.pConsoleMap->SetFrom(&(srv.pConsole->hdr));
			}
		} break;

		case CECMD_SETWINDOWPOS:
		{
			SetWindowPos(in.SetWndPos.hWnd, in.SetWndPos.hWndInsertAfter,
				in.SetWndPos.X, in.SetWndPos.Y, in.SetWndPos.cx, in.SetWndPos.cy,
				in.SetWndPos.uFlags);
		} break;
		
		case CECMD_SETWINDOWRGN:
		{
			MySetWindowRgn(&in.SetWndRgn);
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

	if (gnRunMode == RM_SERVER && ghConEmuWnd && IsWindow(ghConEmuWnd)) // ComSpec окно менять НЕ ДОЛЖЕН!
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
	if (GetConsoleDisplayMode(&srv.dwDisplayMode) && srv.dwDisplayMode) {
		_ASSERTE(!csbi.srWindow.Left && !csbi.srWindow.Top);
		csbi.dwSize.X = csbi.srWindow.Right+1;
		csbi.dwSize.Y = csbi.srWindow.Bottom+1;
	}
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
		WARNING("CorrectVisibleRect пока закомментарен, ибо все равно нифига не делает");
		//if (CorrectVisibleRect(&csbi))
		//	lbNeedCorrect = TRUE;
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

	if (gnRunMode == RM_SERVER) {
		// Запомним то, что последний раз установил сервер. пригодится
		srv.nReqSizeBufferHeight = BufferHeight;
		srv.crReqSizeNewSize = crNewSize;
		srv.rReqSizeNewRect = rNewRect;
		srv.sReqSizeLabel = asLabel;

		if (srv.dwRefreshThread && dwCurThId != srv.dwRefreshThread) {
			ResetEvent(srv.hReqSizeChanged);
			srv.nRequestChangeSize++;
			
			//dwWait = WaitForSingleObject(srv.hReqSizeChanged, REQSIZE_TIMEOUT);
			// Ожидание, пока сработает RefreshThread
			HANDLE hEvents[2] = {ghQuitEvent, srv.hReqSizeChanged};
			DWORD nSizeTimeout = REQSIZE_TIMEOUT;
			#ifdef _DEBUG
			if (IsDebuggerPresent())
				nSizeTimeout = INFINITE;
			#endif
			dwWait = WaitForMultipleObjects ( 2, hEvents, FALSE, nSizeTimeout );
			if (srv.nRequestChangeSize > 0) {
				srv.nRequestChangeSize --;
			} else {
				_ASSERTE(srv.nRequestChangeSize>0);
			}
			if (dwWait == WAIT_OBJECT_0) {
				// ghQuitEvent !!
				return FALSE;
			}
			if (dwWait == (WAIT_OBJECT_0+1)) {
				return TRUE;
			}
			// ?? Может быть стоит самим попробовать?
			return FALSE;
		}
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
#ifdef _DEBUG
		if (!lbNeedChange) {
			BufferHeight = BufferHeight;
		}
#endif
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
			//if (crNewSize.X < csbi.dwSize.X || crNewSize.Y < csbi.dwSize.Y)
			if (crNewSize.X <= (csbi.srWindow.Right-csbi.srWindow.Left) || crNewSize.Y <= (csbi.srWindow.Bottom-csbi.srWindow.Top))
			{
				//MoveWindow(ghConWnd, rcConPos.left, rcConPos.top, 1, 1, 1);
				rNewRect.Left = 0; rNewRect.Top = 0;
				rNewRect.Right = min((crNewSize.X - 1),(csbi.srWindow.Right-csbi.srWindow.Left));
				rNewRect.Bottom = min((crNewSize.Y - 1),(csbi.srWindow.Bottom-csbi.srWindow.Top));
				if (!SetConsoleWindowInfo(ghConOut, TRUE, &rNewRect))
					MoveWindow(ghConWnd, rcConPos.left, rcConPos.top, 1, 1, 1);
			}
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

		/*if (gOSVer.dwMajorVersion == 6 && gOSVer.dwMinorVersion == 1) {
			SetConsoleBufferSize(ghConWnd, crNewSize.X, crNewSize.Y, BufferHeight);
			Sleep(10);
			CONSOLE_SCREEN_BUFFER_INFO csbi2 = {{0,0}};
			DWORD dwStart = GetTickCount(), dwWait = 200;
			do {
				if (GetConsoleScreenBufferInfo(ghConOut, &csbi2)) {
					if (csbi2.dwSize.X == crNewSize.X && csbi2.dwSize.Y == BufferHeight) {
						break;
					}
				}
				Sleep(10);
			} while ((GetTickCount() - dwStart) < dwWait);

		} else {*/
		// Если этого не сделать - размер консоли нельзя УМЕНЬШИТЬ
		if (crNewSize.X <= (csbi.srWindow.Right-csbi.srWindow.Left) || crNewSize.Y <= (csbi.srWindow.Bottom-csbi.srWindow.Top))
		{
			//MoveWindow(ghConWnd, rcConPos.left, rcConPos.top, 1, 1, 1);
			rNewRect.Left = 0;
			rNewRect.Top = max(0,(csbi.srWindow.Bottom-crNewSize.Y+1));
			rNewRect.Right = min((crNewSize.X - 1),(csbi.srWindow.Right-csbi.srWindow.Left));
			rNewRect.Bottom = min((BufferHeight - 1),(rNewRect.Top+crNewSize.Y-1));//(csbi.srWindow.Bottom-csbi.srWindow.Top));
			if (!SetConsoleWindowInfo(ghConOut, TRUE, &rNewRect))
				MoveWindow(ghConWnd, rcConPos.left, rcConPos.top, 1, 1, 1);
		}

		//MoveWindow(ghConWnd, rcConPos.left, rcConPos.top, 1, 1, 1);

		//WaitForSingleObject(ghConOut, 200);
		//Sleep(100);
		/*if (gOSVer.dwMajorVersion == 6 && gOSVer.dwMinorVersion == 1) {
			const SHORT nMaxBuf = 600;
			if (crHeight.Y >= nMaxBuf)
				crHeight.Y = nMaxBuf;
		}*/

		lbRc = SetConsoleScreenBufferSize(ghConOut, crHeight); // а не crNewSize - там "оконные" размеры

		//}
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
	if (asAddLine)
	{
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

#endif

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

void DisableAutoConfirmExit()
{
	gbAutoDisableConfirmExit = FALSE; gbAlwaysConfirmExit = FALSE;
	// менять nProcessStartTick не нужно. проверка только по флажкам
	//srv.nProcessStartTick = GetTickCount() - 2*CHECK_ROOTSTART_TIMEOUT;
}

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

//WARNING("BUGBUG: x64 US-Dvorak"); - done
void CheckKeyboardLayout()
{
	if (pfnGetConsoleKeyboardLayoutName)
	{
		wchar_t szCurKeybLayout[32];
		//#ifdef _DEBUG
		//wchar_t szDbgKeybLayout[KL_NAMELENGTH/*==9*/];
		//BOOL lbGetRC = GetKeyboardLayoutName(szDbgKeybLayout); // -- не дает эффекта, поскольку "на процесс", а не на консоль
		//#endif
		// Возвращает строку в виде "00000419" -- может тут 16 цифр?
		if (pfnGetConsoleKeyboardLayoutName(szCurKeybLayout))
		{
			if (lstrcmpW(szCurKeybLayout, srv.szKeybLayout))
			{
				#ifdef _DEBUG
				wchar_t szDbg[128];
				wsprintfW(szDbg,
					L"ConEmuC: InputLayoutChanged (GetConsoleKeyboardLayoutName returns) '%s'\n", 
					szCurKeybLayout);
				OutputDebugString(szDbg);
				#endif

				if (ghLogSize)
				{
					char szInfo[128]; wchar_t szWide[128];
					wsprintfW(szWide, L"ConEmuC: ConsKeybLayout changed from %s to %s", srv.szKeybLayout, szCurKeybLayout);
					WideCharToMultiByte(CP_ACP,0,szWide,-1,szInfo,128,0,0);
					LogString(szInfo);
				}

				// Сменился
				lstrcpyW(srv.szKeybLayout, szCurKeybLayout);
				// Отошлем в GUI
				wchar_t *pszEnd = szCurKeybLayout+8;
				//WARNING("BUGBUG: 16 цифр не вернет"); -- тут именно 8 цифт. Это LayoutNAME, а не string(HKL)
				// LayoutName: "00000409", "00010409", ...
				// А HKL от него отличается, так что передаем DWORD
				// HKL в x64 выглядит как: "0x0000000000020409", "0xFFFFFFFFF0010409"
				DWORD dwLayout = wcstoul(szCurKeybLayout, &pszEnd, 16);
				CESERVER_REQ* pIn = ExecuteNewCmd(CECMD_LANGCHANGE,sizeof(CESERVER_REQ_HDR)+sizeof(DWORD));
				if (pIn)
				{
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
	if (ptr)
	{
		gnHeapUsed += nWhole;
		if (gnHeapMax < gnHeapUsed)
			gnHeapMax = gnHeapUsed;
	}
	#endif

	return ptr;
}

void Free(LPVOID ptr)
{
	if (ptr && ghHeap)
	{
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

void * __cdecl operator new(size_t _Size)
{
	void * p = Alloc(_Size,1);
	return p;
}

void __cdecl operator delete(void *p)
{
	Free(p);
}
