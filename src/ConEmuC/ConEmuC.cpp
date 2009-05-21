#include <Windows.h>
#include <stdio.h>
#include <Shlwapi.h>
#include <conio.h>
#include <vector>
#include "..\common\common.hpp"

WARNING("Наверное все-же стоит производить периодические чтения содержимого консоли, а не только по событию");

WARNING("Стоит именно здесь осуществлять проверку живости GUI окна (если оно было). Ведь может быть запущен не far, а CMD.exe");

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


/*  Global  */
DWORD   gnSelfPID = 0;
HANDLE  ghConIn = NULL, ghConOut = NULL;
HWND    ghConWnd = NULL;
HANDLE  ghExitEvent = NULL;
BOOL    gbAlwaysConfirmExit = FALSE;


enum {
	RM_UNDEFINED = 0,
	RM_SERVER,
	RM_COMSPEC
} gnRunMode = RM_UNDEFINED;

struct {
	HANDLE hServerThread;   DWORD dwServerThreadId;
	HANDLE hRefreshThread;  DWORD dwRefreshThread;
	HANDLE hWinEventThread; DWORD dwWinEventThread;
    HANDLE hInputThread;    DWORD dwInputThreadId;
	//
	CRITICAL_SECTION csProc;
	CRITICAL_SECTION csConBuf;
	//
	wchar_t szPipename[MAX_PATH], szInputname[MAX_PATH], szGuiPipeName[MAX_PATH];
	//
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
	BOOL  bNeedFullReload;
} srv = {NULL};

struct {
	DWORD dwFarPID;
} cmd = {0};

// Список процессов нам нужен, чтобы определить, когда консоль уже не нужна.
// Например, запустили FAR, он запустил Update, FAR перезапущен...
std::vector<DWORD> nProcesses;

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


int main()
{
	TODO("можно при ошибках показать консоль, предварительно поставив 80x25 и установив крупный шрифт");

	int iRc = 100;
	//wchar_t sComSpec[MAX_PATH];
	//wchar_t* psFilePart;
	//wchar_t* psCmdLine = GetCommandLineW();
	//size_t nCmdLine = lstrlenW(psCmdLine);
	wchar_t* psNewCmd = NULL;
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
	
#ifdef _DEBUG
	//if (!IsDebuggerPresent()) MessageBox(0,L"Loaded",L"ComEmuC",0);
#endif
	
	if ((iRc = ParseCommandLine(GetCommandLineW(), &psNewCmd)) != 0)
		goto wrap;
	
	/* ***************************** */
	/* *** "Общая" инициализация *** */
	/* ***************************** */
	
	
	// Событие используется для всех режимов
	ghExitEvent = CreateEvent(NULL, TRUE/*используется в нескольких нитях, manual*/, FALSE, NULL); ResetEvent(ghExitEvent);
	if (!ghExitEvent) {
		dwErr = GetLastError();
		wprintf(L"CreateEvent() failed, ErrCode=0x%08X\n", dwErr); 
		iRc = CERR_EXITEVENT; goto wrap;
	}

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
	
    SetHandleInformation(GetStdHandle(STD_INPUT_HANDLE), HANDLE_FLAG_INHERIT, HANDLE_FLAG_INHERIT);
    SetHandleInformation(GetStdHandle(STD_OUTPUT_HANDLE), HANDLE_FLAG_INHERIT, HANDLE_FLAG_INHERIT);
    SetHandleInformation(GetStdHandle(STD_ERROR_HANDLE), HANDLE_FLAG_INHERIT, HANDLE_FLAG_INHERIT);
	
    mode = 0;
    lb = GetConsoleMode(ghConIn, &mode);
    if (!(mode & ENABLE_MOUSE_INPUT)) {
        mode |= ENABLE_MOUSE_INPUT;
        lb = SetConsoleMode(ghConIn, mode);
    }


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
	
	lbRc = CreateProcessW(NULL, psNewCmd, NULL,NULL, TRUE, NORMAL_PRIORITY_CLASS, NULL, NULL, &si, &pi);
	dwErr = GetLastError();
	if (!lbRc)
	{
		wprintf (L"Can't create process, ErrCode=0x%08X! Command to be executed:\n%s\n", dwErr, psNewCmd);
		iRc = CERR_CREATEPROCESS; goto wrap;
	}
	delete psNewCmd; psNewCmd = NULL;

	
	
	/* *************************** */
	/* *** Ожидание завершения *** */
	/* *************************** */
	
	if (gnRunMode == RM_SERVER) {
		if (pi.hProcess) CloseHandle(pi.hProcess); 
		if (pi.hThread) CloseHandle(pi.hThread);
	
		// Ждем, пока в консоли не останется процессов (кроме нашего)
		TODO("Проверить, может ли так получиться, что CreateProcess прошел, а к консоли он не прицепился? Может, если процесс GUI");
		nWait = WaitForSingleObject(ghExitEvent, 6*1000); //Запуск процесса наверное может задержать антивирус
		if (nWait != WAIT_OBJECT_0) { // Если таймаут
			EnterCriticalSection(&srv.csProc);
			iRc = nProcesses.size();
			LeaveCriticalSection(&srv.csProc);
			// И процессов в консоли все еще нет
			if (iRc == 0) {
				wprintf (L"Process was not attached to console. Is it GUI?\nCommand to be executed:\n%s\n", psNewCmd);
				iRc = CERR_PROCESSTIMEOUT; goto wrap;
			}

			// По крайней мере один процесс в консоли запустился. Ждем пока в консоли не останется никого кроме нас
			WaitForSingleObject(ghExitEvent, INFINITE);
		}
	} else {
		// В режиме ComSpec нас интересует завершение ТОЛЬКО дочернего процесса
		WaitForSingleObject(pi.hProcess, INFINITE);
		// Сразу закрыть хэндлы
		if (pi.hProcess) CloseHandle(pi.hProcess); 
		if (pi.hThread) CloseHandle(pi.hThread);
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
	} else {
		ComspecDone(iRc);
	}

	
	/* ************************** */
	/* *** "Общее" завершение *** */
	/* ************************** */
	
	if (ghConIn && ghConIn!=INVALID_HANDLE_VALUE) {
		CloseHandle(ghConIn); ghConIn = NULL;
	}
	if (ghConOut && ghConOut!=INVALID_HANDLE_VALUE) {
		CloseHandle(ghConOut); ghConIn = NULL;
	}

	if (iRc!=0 || gbAlwaysConfirmExit) {
		wprintf(L"Press any key to close console");
		_getch();
		wprintf(L"\n");
	}
	
	if (psNewCmd) { delete psNewCmd; psNewCmd = NULL; }
    return iRc;
}

void Help()
{
	wprintf(
		L"ConEmuC. Copyright (c) 2009, Maximus5\n"
		L"This is a console part of ConEmu product.\n"
		L"Usage: ComEmuC [/CONFIRM] /C <command line, passed to %%COMSPEC%%>\n"
		L"   or: ComEmuC [/CONFIRM] /CMD <program with arguments, far.exe for example>\n"
		L"   or: ComEmuC /?\n"
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
	
		if (lstrcmpiW(pwszCopy, L"cmd")==0 || lstrcmpiW(pwszCopy, L"cmd.exe")==0) {
			bViaCmdExe = FALSE; // уже указан командный процессор, cmd.exe в начало добавлять не нужно
		}
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
			if (lstrcmpiW(pwszCopy, L"ConEmuC")==0 || lstrcmpiW(pwszCopy, L"ConEmuC.exe")==0)
				szComSpec[0] = 0;
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
	
	// Finilize
	ch = *psCmdLine; // может указывать на закрывающую кавычку
	while (ch == L'"' || ch == L' ' || ch == L'\t' || ch == L'\r' || ch == L'\n') ch = *(++psCmdLine);
	asCmdLine = psCmdLine;
	
	return 0;
}

int ComspecInit()
{
	WARNING("Увеличить высоту буфера до 600+ строк, запомнить текущий размер (высота И ширина)");
	TODO("Определить код родительского процесса, и если это FAR - запомнить его (для подключения к пайпу плагина)");
	return 0;
}

void ComspecDone(int aiRc)
{
	TODO("Уведомить плагин через пайп (если родитель - FAR) что процесс завершен. Плагин должен считать и запомнить содержимое консоли и только потом вернуть управление в ConEmuC!");
	
	WARNING("Вернуть размер буфера (высота И ширина)");
}

// Создать необходимые события и нити
int ServerInit()
{
	int iRc = 0;
	DWORD dwErr = 0;
	HANDLE hWait[2] = {NULL,NULL};

	srv.bContentsChanged = TRUE;
	srv.nMainTimerElapse = 10;
	srv.bConsoleActive = TRUE; TODO("Обрабатывать консольные события Activate/Deactivate");
	srv.bNeedFullReload = TRUE;

	
	InitializeCriticalSection(&srv.csConBuf);
	InitializeCriticalSection(&srv.csProc);
	
	wsprintfW(srv.szPipename, CESERVERPIPENAME, L".", gnSelfPID);
	wsprintfW(srv.szInputname, CESERVERINPUTNAME, L".", gnSelfPID);

	// Размер шрифта и Lucida. Обязательно для сервеного режима.
    SetConsoleFontSizeTo(ghConWnd, 4, 6);

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
		wprintf(L"CreateEvent() failed, ErrCode=0x%08X\n", dwErr); 
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
		CloseHandle(hWait[0]); hWait[0]=NULL; hWait[1]=NULL;
		iRc = CERR_WINEVENTTHREAD; goto wrap;
	}
	hWait[1] = srv.hWinEventThread;
	dwErr = WaitForMultipleObjects(2, hWait, FALSE, 10000);
	CloseHandle(hWait[0]); hWait[0]=NULL; hWait[1]=NULL;
	if (!srv.hWinHook) {
		_ASSERT(dwErr == WAIT_TIMEOUT);
		if (dwErr == WAIT_TIMEOUT) { // по идее этого быть не должно
			TerminateThread(srv.hWinEventThread,100);
			CloseHandle(srv.hWinEventThread); srv.hWinEventThread = NULL;
		}
		// Ошибка на экран уже выведена, нить уже завершена, закрыть дескриптор
		CloseHandle(srv.hWinEventThread); srv.hWinEventThread = NULL;
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
			TerminateThread ( srv.hWinEventThread, 100 ); // раз корректно не хочет...
		}
		CloseHandle(srv.hWinEventThread); srv.hWinEventThread = NULL;
	}
	if (srv.hInputThread) {
		TerminateThread ( srv.hInputThread, 100 ); TODO("Сделать корректное завершение");
		CloseHandle(srv.hInputThread); srv.hInputThread = NULL;
	}

	if (srv.hServerThread) {
		TerminateThread ( srv.hServerThread, 100 ); TODO("Сделать корректное завершение");
		CloseHandle(srv.hServerThread); srv.hServerThread = NULL;
	}
	if (srv.hRefreshThread) {
		if (WaitForSingleObject(srv.hRefreshThread, 100)!=WAIT_OBJECT_0) {
			_ASSERT(FALSE);
			TerminateThread(srv.hRefreshThread, 100);
		}
		CloseHandle(srv.hRefreshThread); srv.hRefreshThread = NULL;
	}
	
    if (srv.hRefreshEvent) {
	    CloseHandle(srv.hRefreshEvent); srv.hRefreshEvent = NULL;
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
          wprintf(L"CreatePipe failed, ErrCode=0x%08X\n"); 
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
            wprintf(L"CreateThread(Instance) failed, ErrCode=0x%08X\n"); 
            Sleep(50);
            //return 0;
            continue;
         }
         else CloseHandle(hInstanceThread); 
       } 
      else 
        // The client could not connect, so close the pipe. 
         CloseHandle(hPipe); 
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
          wprintf(L"CreatePipe failed, ErrCode=0x%08X\n"); 
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
			      CloseHandle(hPipe);
			      break;
		      }
		      if (iRec.EventType) {
		      
			      fSuccess = WriteConsoleInput(ghConIn, &iRec, 1, &cbWritten);
			      _ASSERTE(fSuccess && cbWritten==1);
		      }
		      // next
		      memset(&iRec,0,sizeof(iRec));
	      }
      } 
      else 
        // The client could not connect, so close the pipe. 
         CloseHandle(hPipe);
   } 
   return 1; 
} 
 
DWORD WINAPI InstanceThread(LPVOID lpvParam) 
{ 
   CESERVER_REQ in={0}, *pOut=NULL;
   DWORD cbBytesRead, cbWritten; 
   BOOL fSuccess; 
   HANDLE hPipe; 
 
// The thread's parameter is a handle to a pipe instance. 
 
   hPipe = (HANDLE) lpvParam; 
 
   while (1) 
   { 
   // Read client requests from the pipe. 
      memset(&in, 0, sizeof(in));
      fSuccess = ReadFile( 
         hPipe,        // handle to pipe 
         &in,          // buffer to receive data 
         sizeof(in),   // size of buffer 
         &cbBytesRead, // number of bytes read 
         NULL);        // not overlapped I/O 

      if (!fSuccess || cbBytesRead < 12 || in.nSize < 12)
         break;
         
      if (!GetAnswerToRequest(in, &pOut) || pOut==NULL)
	     break;
 
   // Write the reply to the pipe. 
      fSuccess = WriteFile( 
         hPipe,        // handle to pipe 
         pOut,         // buffer to write from 
         pOut->nSize,  // number of bytes to write 
         &cbWritten,   // number of bytes written 
         NULL);        // not overlapped I/O 

      // освободить память
      free(pOut);
         
      if (!fSuccess || pOut->nSize != cbWritten) break; 
  } 
 
// Flush the pipe to allow the client to read the pipe's contents 
// before disconnecting. Then disconnect the pipe, and close the 
// handle to this pipe instance. 
 
   FlushFileBuffers(hPipe); 
   DisconnectNamedPipe(hPipe); 
   CloseHandle(hPipe); 

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
	TextWidth = max(srv.sbi.dwSize.X, (srv.sbi.srWindow.Right - srv.sbi.srWindow.Left + 1));
	TextHeight = srv.sbi.srWindow.Bottom - srv.sbi.srWindow.Top + 1;
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
				wsprintf(srv.szGuiPipeName, CEGUIPIPENAME, L".", (HWND)ghConWnd); // был gnSelfPID
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
			//TODO:
		} break;
		case CECMD_WRITEINPUT:
		{
		} break;
	}
	
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
		//	lbQuit = TRUE; break;
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
	BOOL lbQuit = FALSE;

	while (!lbQuit)
	{
		nWait = WAIT_TIMEOUT;

		// Подождать немножко
		nWait = WaitForMultipleObjects ( 2, hEvents, FALSE, /*srv.bConsoleActive ? srv.nMainTimerElapse :*/ 60000 );
		if (nWait == WAIT_OBJECT_0)
			break; // затребовано завершение нити

		// Посмотреть, есть ли изменения в консоли
		ReloadFullConsoleInfo();
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

    switch(event)
    {
    case EVENT_CONSOLE_START_APPLICATION:
		//A new console process has started. 
		//The idObject parameter contains the process identifier of the newly created process. 
		//If the application is a 16-bit application, the idChild parameter is CONSOLE_APPLICATION_16BIT and idObject is the process identifier of the NTVDM session associated with the console.

		#ifdef _DEBUG
		WCHAR szDbg[128]; wsprintfW(szDbg, L"EVENT_CONSOLE_START_APPLICATION(PID=%i%s)\n", idObject, (idChild == CONSOLE_APPLICATION_16BIT) ? L" 16bit" : L"");
		OutputDebugString(szDbg);
		#endif

		if (idObject != gnSelfPID) {
			EnterCriticalSection(&srv.csProc);
			nProcesses.push_back(idObject);
			LeaveCriticalSection(&srv.csProc);

			if (idChild == CONSOLE_APPLICATION_16BIT) {
				DWORD ntvdmPID = idObject;
				dwActiveFlags |= CES_NTVDM;
				SetPriorityClass(GetCurrentProcess(), HIGH_PRIORITY_CLASS);
			}
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
			for (iter=nProcesses.begin(); iter!=nProcesses.end(); iter++) {
				if (idObject == *iter) {
					nProcesses.erase(iter);
					if (idChild == CONSOLE_APPLICATION_16BIT) {
						DWORD ntvdmPID = idObject;
						dwActiveFlags &= ~CES_NTVDM;
						//TODO: возможно стоит прибить процесс NTVDM?
						SetPriorityClass(GetCurrentProcess(), NORMAL_PRIORITY_CLASS);
					}
					// Процессов в консоли не осталось?
					if (nProcesses.size() == 0) {
						LeaveCriticalSection(&srv.csProc);
						SetEvent(ghExitEvent);
						return;
					}
					break;
				}
			}
			LeaveCriticalSection(&srv.csProc);
		}
        return; // обновление экрана не требуется

	case EVENT_CONSOLE_UPDATE_REGION: // 0x4002 
		{
		//More than one character has changed.
		//The idObject parameter is a COORD structure that specifies the start of the changed region. 
		//The idChild parameter is a COORD structure that specifies the end of the changed region.
			#ifdef _DEBUG
			COORD crStart, crEnd; memmove(&crStart, &idObject, sizeof(idObject)); memmove(&crEnd, &idChild, sizeof(idChild));
			WCHAR szDbg[128]; wsprintfW(szDbg, L"EVENT_CONSOLE_UPDATE_REGION({%i, %i} - {%i, %i})\n", crStart.X,crStart.Y, crEnd.X,crEnd.Y);
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
			WCHAR szDbg[128]; wsprintfW(szDbg, L"EVENT_CONSOLE_UPDATE_SCROLL(X=%i, Y=%i)\n", idObject, idChild);
			OutputDebugString(szDbg);
			#endif
			//bNeedConAttrBuf = TRUE;
			//// Перечитать размер, положение курсора, и пр.
			//if (!ReloadConsoleInfo())
			//	return;
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
			WCHAR szDbg[128]; wsprintfW(szDbg, L"EVENT_CONSOLE_UPDATE_SIMPLE({%i, %i} '%c'(\\x%04X) A=%i)\n", ch.crStart.X,ch.crStart.Y, ch.data[0], ch.data[0], ch.data[1]);
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
		//		CONSOLE_CARET_SELECTION or CONSOLE_CARET_VISIBLE.
		//The idChild parameter is a COORD structure that specifies the cursor's current position.
			#ifdef _DEBUG
			COORD crWhere; memmove(&crWhere, &idChild, sizeof(idChild));
			WCHAR szDbg[128]; wsprintfW(szDbg, L"EVENT_CONSOLE_CARET({%i, %i} Sel=%c, Vis=%c\n", crWhere.X,crWhere.Y, 
				((idObject & CONSOLE_CARET_SELECTION)==CONSOLE_CARET_SELECTION) ? L'Y' : L'N',
				((idObject & CONSOLE_CARET_VISIBLE)==CONSOLE_CARET_VISIBLE) ? L'Y' : L'N');
			OutputDebugString(szDbg);
			#endif
			SetEvent(srv.hRefreshEvent);
			// Перечитать размер, положение курсора, и пр.
			//if (ReloadConsoleInfo())
			//	return;
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
			//	return;
			srv.bNeedFullReload = TRUE;
			SetEvent(srv.hRefreshEvent);
		}
		return; // Обновление по событию в нити
    }


	//// Дальнейшее имеет смысл только если CVirtualConsole уже запустил серверный пайп
	//if (srv.szGuiPipeName[0] == 0)
	//	return;

	//CESERVER_REQ* pOut = 
	//	CreateConsoleInfo (
	//		(event == EVENT_CONSOLE_UPDATE_SIMPLE) ? &ch : NULL,
	//		FALSE/*bNeedConAttrBuf*/
	//	);
	//_ASSERTE(pOut!=NULL);
	//if (!pOut)
	//	return;

	////Warning! WinXPSP3. EVENT_CONSOLE_CARET проходит ТОЛЬКО если консоль в фокусе. 
	////         А с ConEmu она НИКОГДА не в фокусе, так что курсор не обновляется.
	//// Т.е. изменилось содержимое консоли - уведомить GUI часть
	////if (event != EVENT_CONSOLE_CARET) { // Если изменилось только положение курсора - перечитывать содержимое не нужно
	////	srv.bContentsChanged = TRUE;
	////}
	////SetEvent(hGlblUpdateEvt);

	////WARNING("Все изменения пересылать в GUI через CEGUIPIPENAME");
	////WARNING("Этот пайп обрабатывается в каждом CVirtualConsole по отдельности");

	//SendConsoleChanges ( pOut );

	//free ( pOut );
}

void SendConsoleChanges(CESERVER_REQ* pOut)
{
	//srv.szGuiPipeName инициализируется только после первого RetrieveConsoleInfo в GUI. Это значит, что там запущена серверная нить
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
	//DWORD dwProcCount = nProcesses.size()+20;
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
		if (bCharAttrBuff == 2) {
			_ASSERTE(srv.nBufCharCount>0);
			OneBufferSize = srv.nBufCharCount*2;
		} else {
			OneBufferSize = ReadConsoleData(); // returns size in bytes of ONE buffer
		}
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
	//DWORD dwTestCount = nProcesses.size();
	//_ASSERTE(dwTestCount<=dwProcCount);
	//if (dwTestCount < dwProcCount) dwProcCount = dwTestCount;
	//*((DWORD*)lpCur) = dwProcCount; lpCur += sizeof(DWORD);
	//for (DWORD n=0; n<dwProcCount; n++) {
	//	*((DWORD*)lpCur) = nProcesses[n];
	//	lpCur += sizeof(DWORD);
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
			memmove(srv.psChars+srv.nBufCharCount, srv.psChars, srv.nBufCharCount*sizeof(wchar_t));
			memmove(srv.pnAttrs+srv.nBufCharCount, srv.pnAttrs, srv.nBufCharCount*sizeof(WORD));
		}
	}

	if (pCheck) free(pCheck);
}

