// AnsiDbg.cpp : Defines the entry point for the console application.
//

#include <windows.h>
#include <stdio.h>
#include <tchar.h>
#include <TlHelp32.h>

#if defined(_MSC_VER) && (_MSC_VER >= 1900)
	#if defined(_DEBUG)
		#pragma comment(lib, "libvcruntimed.lib")
		#pragma comment(lib, "libucrtd.lib")
	#else
		#pragma comment(lib, "libvcruntime.lib")
		#pragma comment(lib, "libucrt.lib")
	#endif
#endif

bool gbExit = false;
CRITICAL_SECTION cs;
HANDLE hInThread = NULL;
HWND hMinTTY = NULL;

const wchar_t gszAnalogues[32] =
{
	32, 9786, 9787, 9829, 9830, 9827, 9824, 8226, 9688, 9675, 9689, 9794, 9792, 9834, 9835, 9788,
	9658, 9668, 8597, 8252,  182,  167, 9632, 8616, 8593, 8595, 8594, 8592, 8735, 8596, 9650, 9660
};


BOOL WINAPI HandlerRoutine(DWORD dwCtrlType)
{
	if ((dwCtrlType == CTRL_C_EVENT) || (dwCtrlType == CTRL_BREAK_EVENT))
	{
		gbExit = true;
		ExitThread(1);
	}
	return TRUE;
}

int PrintOut(LPCSTR asFormat)
{
	int nErr = GetLastError();
	char szErrText[100];
	HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
	wsprintfA(szErrText, asFormat, nErr);
	DWORD nWrite, nLen = strlen(szErrText);
	WriteFile(hOut, szErrText, nLen, &nWrite, NULL);
	return nErr;
}

BOOL CALLBACK EnumMinTTY(HWND hwnd, LPARAM lParam)
{
	if (hMinTTY)
		return FALSE;
	LPDWORD pdw = (LPDWORD)lParam;
	TCHAR szClass[100] = _T("");
	if (GetClassName(hwnd, szClass, 100))
	{
		if (lstrcmp(szClass, _T("mintty")) == 0)
		{
			DWORD nPID, nTID = GetWindowThreadProcessId(hwnd, &nPID);
			while (*pdw)
			{
				if (*pdw == nPID)
				{
					hMinTTY = hwnd;
					return FALSE; // stop
				}
				pdw++;
			}
		}
		else if (lstrcmp(szClass, _T("VirtualConsoleClass")) == 0)
		{
			EnumChildWindows(hwnd, EnumMinTTY, lParam);
		}
	}
	return TRUE; // continue
}

HWND FindMinTTY()
{
	if (hMinTTY)
		return hMinTTY;

	DWORD dwParentID[256] = {GetCurrentProcessId()}; DWORD nParents = 1;
	DWORD dwMinTTY[256] = {}; DWORD nMinTTYs = 0;
	DWORD nCountMax = 256, nCount = 0;
	LPPROCESSENTRY32 pi = (LPPROCESSENTRY32)malloc(nCountMax*sizeof(PROCESSENTRY32));

	HANDLE hSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
	if (hSnap != INVALID_HANDLE_VALUE)
	{
		pi[0].dwSize = sizeof(*pi);
		if (Process32First(hSnap, pi)) do
		{
			nCount++;
			if (nCount == nCountMax)
			{
				DWORD nNew = nCountMax*2;
				LPPROCESSENTRY32 piNew = (LPPROCESSENTRY32)realloc(pi, nNew*sizeof(PROCESSENTRY32));
				if (!piNew)
					break;
				pi = piNew;
				nCountMax = nNew;
			}
			pi[nCount].dwSize = sizeof(*pi);
		} while (Process32Next(hSnap, pi+nCount));
		CloseHandle(hSnap);

		if (!nCount)
		{
			free(pi);
			return NULL;
		}

		bool bMinTTY = false;
		for (int n = (int)(nCount - 1); n >= 0; n--)
		{
			bool bParent = false;
			for (DWORD p = 0; p < nParents; p++)
			{
				if (pi[n].th32ProcessID == dwParentID[p])
				{
					bParent = true;
					dwParentID[nParents++] = pi[n].th32ParentProcessID;
					if (nParents == 255)
						break;
				}
			}
		}
		for (DWORD n = 0; n < nCount; n++)
		{
			if (lstrcmpi(pi[n].szExeFile, _T("mintty.exe")) == 0)
			{
				for (DWORD p = 0; p < nParents; p++)
				{
					if (dwParentID[p] == pi[n].th32ProcessID)
					{
						dwMinTTY[nMinTTYs++] = pi[n].th32ProcessID;
						break;
					}
				}
			}
			if (nMinTTYs == 255)
				break;
		}

		EnumWindows(EnumMinTTY, (LPARAM)dwMinTTY);
	}
	free(pi);
	return hMinTTY;
}

void SendAnsi(HANDLE hPipe, HANDLE hOut, LPCSTR asSeq)
{
	if (!asSeq || !*asSeq) return;
	DWORD nWrite, nLen = lstrlenA(asSeq);
	if (hPipe)
		WriteFile(hPipe, asSeq, nLen, &nWrite, NULL);
	if (hOut)
	{
		LPCSTR pszNextEsc = strchr(asSeq, 27);
		while (pszNextEsc)
		{
			if (pszNextEsc > asSeq)
				WriteConsoleA(hOut, asSeq, (pszNextEsc - asSeq), &nWrite, NULL);
			WriteConsoleW(hOut, gszAnalogues+27, 1, &nWrite, NULL);
			asSeq = pszNextEsc + 1;
			pszNextEsc = strchr(asSeq, 27);
		}
		if (asSeq && *asSeq)
		{
			nLen = lstrlenA(asSeq);
			WriteConsoleA(hOut, asSeq, nLen, &nWrite, NULL);
		}
	}
}

void SendFile(HANDLE hPipe, HANDLE hOut, LPCTSTR asFileName)
{
	TCHAR szPath[MAX_PATH] = _T("");

	if (asFileName)
	{
		GetModuleFileName(NULL, szPath, MAX_PATH);
		TCHAR* pszSlash = _tcsrchr(szPath, _T('\\'));
		lstrcpy(pszSlash ? pszSlash+1 : szPath, asFileName);
	}
	else
	{
		OPENFILENAME ofn = {sizeof(ofn)};
		ofn.hwndOwner = NULL;
		ofn.lpstrFilter = _T("All files (*.*)\0*.*\0\0");
		ofn.lpstrFile = szPath;
		ofn.nMaxFile = ARRAYSIZE(szPath);
		ofn.lpstrTitle = _T("Choose file to send");
		ofn.Flags = OFN_ENABLESIZING|OFN_NOCHANGEDIR
			| OFN_PATHMUSTEXIST|OFN_EXPLORER|OFN_HIDEREADONLY|OFN_FILEMUSTEXIST;
		if (!GetOpenFileName(&ofn))
			return;
	}

	HANDLE hFile = CreateFile(szPath, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
	if (hFile == INVALID_HANDLE_VALUE)
	{
		DWORD nWrite;
		PrintOut("File was not found, code=%u\n");
		WriteConsole(hOut, szPath, lstrlen(szPath), &nWrite, NULL);
		return;
	}
	DWORD nSize = GetFileSize(hFile, NULL);
	char* pszData = (char*)calloc(nSize+1,1);
	if (nSize && pszData && ReadFile(hFile, pszData, nSize, &nSize, NULL))
	{
		SendAnsi(hPipe, hOut, pszData);
	}
	CloseHandle(hFile);
}

DWORD WINAPI InputThread(LPVOID lpParameter)
{
	char ch; DWORD nRead = 0, nWrite;
	HANDLE hIn = GetStdHandle(STD_INPUT_HANDLE);
	HANDLE hErr = GetStdHandle(STD_OUTPUT_HANDLE);
	CONSOLE_SCREEN_BUFFER_INFO csbi;
	while (!gbExit)
	{
		// Following allows to view what user (or terminal) is sending to input
		if (ReadFile(hIn, &ch, 1, &nRead, NULL) && nRead)
		{
			EnterCriticalSection(&cs);
			BOOL b = GetConsoleScreenBufferInfo(hErr, &csbi);
			if (b)
				SetConsoleTextAttribute(hErr, 11);
			else
				SendAnsi(hErr, NULL, "\x1B[1;36m");
			if (ch == 27)
				WriteFile(hErr, "\\e", 2, &nWrite, NULL);
			else
				WriteFile(hErr, &ch, 1, &nWrite, NULL);
			if (b)
				SetConsoleTextAttribute(hErr, csbi.wAttributes);
			else
				SendAnsi(hErr, NULL, "\x1B[m");
			LeaveCriticalSection(&cs);
		}
		else
		{
			Sleep(10);
		}
	}
	return 0;
}



DWORD WINAPI ProcessSrvThread(LPVOID lpParameter)
{
	LPCTSTR asName = (LPCTSTR)lpParameter;
	HANDLE hPipeIn;
	TCHAR szName[MAX_PATH] = _T("\\\\.\\pipe\\");
	_tcscat_s(szName, asName);

	BOOL b; DWORD nRead, nWrite; char c;

	hPipeIn = CreateNamedPipe(szName, PIPE_ACCESS_INBOUND, PIPE_TYPE_BYTE, 1, 80, 80, 0, NULL);
	if (!hPipeIn || hPipeIn == INVALID_HANDLE_VALUE)
	{
		return PrintOut("\n!!! CreateNamedPipe failed, code=%u !!!\n");
	}

	char sInfo[300] = "";
	wsprintfA(sInfo, "Server started, PID=%u\nWaiting for client '", GetCurrentProcessId());
#ifdef _UNICODE
	WideCharToMultiByte(CP_OEMCP, 0, asName, -1, sInfo+lstrlenA(sInfo), 200, 0, 0);
#else
	lstrcat(sInfo, asName);
#endif
	lstrcatA(sInfo, "'...\n");
	PrintOut(sInfo);
	b = ConnectNamedPipe(hPipeIn, NULL);
	if (!b)
	{
		return PrintOut("\n!!! ConnectNamedPipe failed, code=%u !!!\n");
	}
	PrintOut("Connected!\n");

	while (true)
	{
		HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
		b = ReadFile(hPipeIn, &c, 1, &nRead, NULL);
		if (b && nRead)
		{
			EnterCriticalSection(&cs);
			switch (c)
			{
			case 1:
			{
				Sleep(200);
				HANDLE hIn = GetStdHandle(STD_INPUT_HANDLE);
				INPUT_RECORD ir = {KEY_EVENT};
				ir.Event.KeyEvent.bKeyDown = TRUE;
				ir.Event.KeyEvent.wVirtualKeyCode = VK_RETURN;
				ir.Event.KeyEvent.uChar.AsciiChar = '\r';
				b = WriteConsoleInputA(hIn, &ir, 1, &nWrite);
				nRead = GetLastError();
				if (!b && FindMinTTY())
				{
					PostMessage(hMinTTY, WM_CHAR, '\r', 0);
				}
				b = TRUE; nRead = nWrite;
				break;
			}
			case 2:
			{
				int width = 80, height = 25;
				CONSOLE_SCREEN_BUFFER_INFO csbi;
				if (GetConsoleScreenBufferInfo(hOut, &csbi))
				{
					width = max(csbi.dwSize.X,20);
					height = max(25,(csbi.dwSize.Y-csbi.dwCursorPosition.Y-1));
				}
				size_t cchMax = width+20;
				char* szText = (char*)malloc(cchMax);
				for (int i = 1; i <= height; i++)
				{
					sprintf_s(szText, cchMax, "Line %-4u ", i);
					char* psz = szText + strlen(szText);
					for (int j = 1; (psz - szText) < width; j++)
					{
						sprintf_s(psz, cchMax - (psz - szText), "Word %u ", j);
						psz += strlen(psz);
					}
					psz = szText+width-1;
					*(psz++) = L'\r'; *(psz++) = L'\n'; *psz = 0;
					WriteFile(hOut, szText, psz - szText, &nWrite, NULL);
				}
				b = TRUE; nRead = nWrite;
				break;
			}
			default:
				b = WriteFile(hOut, &c, nRead, &nWrite, NULL);
			}
			LeaveCriticalSection(&cs);
			if (!b || nWrite != nRead)
			{
				PrintOut("\n!!! WriteFile(StdOut) failed, code=%u !!!\n");
				break;
			}
		}
		else
		{
			nRead = GetLastError();
			PrintOut("\n!!! Pipe reading error, code=%u !!!\n");
			break;
		}
	}

	// CygWin/mintty hack to exit process
	TerminateProcess(GetCurrentProcess(), 1);
	return 0;
}

int ProcessSrv(LPCTSTR asName)
{
	// Under cygwin/mintty ReadFile(STD_INPUT_HANDLE) works only in the main thread
	DWORD  nThread;
	hInThread = CreateThread(NULL, 0, ProcessSrvThread, (LPVOID)asName, 0, &nThread);
	return InputThread(NULL);
}

int ProcessInput(LPCTSTR asName)
{
	HANDLE hIn = GetStdHandle(STD_INPUT_HANDLE);
	HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
	TCHAR szName[MAX_PATH] = _T("\\\\.\\pipe\\");
	_tcscat_s(szName, asName);
	DWORD nRead, nWrite;
	INPUT_RECORD r;
	BOOL b;

	char sInfo[300] = "";
	wsprintfA(sInfo, "Client started, PID=%u\nConnecting to server '", GetCurrentProcessId());
#ifdef _UNICODE
	WideCharToMultiByte(CP_OEMCP, 0, asName, -1, sInfo+lstrlenA(sInfo), 200, 0, 0);
#else
	lstrcat(sInfo, asName);
#endif
	lstrcatA(sInfo, "'");
	PrintOut(sInfo);

	DWORD nStart = GetTickCount();
	while ((GetTickCount() - nStart) < 30000)
	{
		if (WaitNamedPipe(szName, 500))
			break;
		printf(".");
		Sleep(2000);
	}

	HANDLE hPipe = CreateFile(szName, GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);
	if (!hPipe || hPipe == INVALID_HANDLE_VALUE)
	{
		nRead = GetLastError();
		printf("\n!!! OpenPipe failed, code=%u !!!\n", nRead);
		return nRead;
	}
	printf("\nConnected! Alt+X - exit, Alt+L - clear, Alt+F - fill, Ctrl+F - paste file\n");

	SetConsoleCtrlHandler((PHANDLER_ROUTINE)HandlerRoutine, true);

	while (!gbExit)
	{
		if (ReadConsoleInputA(hIn, &r, 1, &nRead) && nRead)
		{
			if (r.EventType == KEY_EVENT && r.Event.KeyEvent.bKeyDown)
			{
				switch (r.Event.KeyEvent.wVirtualKeyCode)
				{
				case VK_UP:
					SendAnsi(hPipe, hOut, "\x1B[A"); continue;
				case VK_DOWN:
					SendAnsi(hPipe, hOut, "\x1B[B"); continue;
				case VK_RIGHT:
					SendAnsi(hPipe, hOut, "\x1B[C"); continue;
				case VK_LEFT:
					SendAnsi(hPipe, hOut, "\x1B[D"); continue;
				case VK_HOME:
					if (r.Event.KeyEvent.dwControlKeyState & (RIGHT_CTRL_PRESSED|LEFT_CTRL_PRESSED))
						SendAnsi(hPipe, hOut, "\x1B[1;1H");
					else
						SendAnsi(hPipe, hOut, "\x1B[1G");
					continue;
				case VK_END:
					if (r.Event.KeyEvent.dwControlKeyState & (RIGHT_CTRL_PRESSED|LEFT_CTRL_PRESSED))
						SendAnsi(hPipe, hOut, "\x1B[9999;1H");
					else
					{
						CONSOLE_SCREEN_BUFFER_INFO csbi;
						if (GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &csbi))
						{
							char chEsc[32]; wsprintfA(chEsc, "\x1B[%uG", csbi.dwSize.X);
							SendAnsi(hPipe, hOut, chEsc);
						}
					}
					continue;
				case VK_PRIOR: // PgUp
					if (r.Event.KeyEvent.dwControlKeyState & (RIGHT_CTRL_PRESSED|LEFT_CTRL_PRESSED))
						SendAnsi(hPipe, hOut, "\x1B[9999S");
					else
						SendAnsi(hPipe, hOut, "\x1B[S");
					continue;
				case VK_NEXT: // PgDn
					if (r.Event.KeyEvent.dwControlKeyState & (RIGHT_CTRL_PRESSED|LEFT_CTRL_PRESSED))
						SendAnsi(hPipe, hOut, "\x1B[9999T");
					else
						SendAnsi(hPipe, hOut, "\x1B[T");
					continue;
				case VK_DELETE:
					if (r.Event.KeyEvent.dwControlKeyState & (RIGHT_CTRL_PRESSED|LEFT_CTRL_PRESSED))
						SendAnsi(hPipe, hOut, "\x1B[M");
					else if (r.Event.KeyEvent.dwControlKeyState & (RIGHT_ALT_PRESSED|LEFT_ALT_PRESSED))
						SendAnsi(hPipe, hOut, "\x1B[P");
					else if (r.Event.KeyEvent.dwControlKeyState & (SHIFT_PRESSED))
						SendAnsi(hPipe, hOut, "\x1B[X");
					else
						PrintOut("\nCtrl+Del - delete line, Alt+Del - delete char, Shift+Del - clear char\n");
					continue;
				case VK_INSERT:
					if (r.Event.KeyEvent.dwControlKeyState & (RIGHT_CTRL_PRESSED|LEFT_CTRL_PRESSED))
						SendAnsi(hPipe, hOut, "\x1B[L");
					else if (r.Event.KeyEvent.dwControlKeyState & (RIGHT_ALT_PRESSED|LEFT_ALT_PRESSED))
						SendAnsi(hPipe, hOut, "\x1B[@");
					else
						PrintOut("\nCtrl+Ins - insert line, Alt+Ins - insert char\n");
					continue;
				}
				if (r.Event.KeyEvent.dwControlKeyState & (LEFT_CTRL_PRESSED|RIGHT_CTRL_PRESSED))
				{
					switch (r.Event.KeyEvent.wVirtualKeyCode)
					{
					case '1':
						SendFile(hPipe, hOut, _T("AnsiColors16.ans")); continue;
					case '2':
						SendFile(hPipe, hOut, _T("AnsiColors16t.ans")); continue;
					case '3':
						SendFile(hPipe, hOut, _T("AnsiColors256.ans")); continue;
					case 'F': case 'f':
						SendFile(hPipe, hOut, NULL); continue;
					}
				}
				if (r.Event.KeyEvent.uChar.AsciiChar)
				{
					if (r.Event.KeyEvent.dwControlKeyState & (LEFT_ALT_PRESSED|RIGHT_ALT_PRESSED))
					{
						switch (r.Event.KeyEvent.uChar.AsciiChar)
						{
						case 'x': case 'X':
							printf("\nExit session\n");
							gbExit = true;
							break;
						case 'l': case 'L':
							SendAnsi(hPipe, hOut, "\x1B[2J"); SendAnsi(hPipe, hOut, "\x1B[1;1H");
							break;
						case 'f': case 'F':
							SendAnsi(NULL, hOut, "\r\nFilling server console width text...\r\n");
							SendAnsi(hPipe, NULL, "\x02");
							break;
						case '1':
							SendAnsi(NULL, hOut, "{Alt+1}");
							SendAnsi(hPipe, NULL, "\r\nRequest terminal primary DA: ");
							SendAnsi(hPipe, hOut, "\x1B[c");
							SendAnsi(hPipe, NULL, "\x01");
							break;
						case '2':
							SendAnsi(NULL, hOut, "{Alt+2}");
							SendAnsi(hPipe, NULL, "\r\nRequest terminal secondary DA: ");
							SendAnsi(hPipe, hOut, "\x1B[>c");
							SendAnsi(hPipe, NULL, "\x01");
							break;
						case '3':
							SendAnsi(NULL, hOut, "{Alt+3}");
							SendAnsi(hPipe, NULL, "\r\nRequest terminal status: ");
							SendAnsi(hPipe, hOut, "\x1B[5n");
							SendAnsi(hPipe, NULL, "\x01");
							break;
						case '4':
							SendAnsi(NULL, hOut, "{Alt+4}");
							SendAnsi(hPipe, NULL, "\r\nRequest cursor pos [row;col]: ");
							SendAnsi(hPipe, hOut, "\x1B[6n");
							SendAnsi(hPipe, NULL, "\x01");
							break;
						case '5':
							SendAnsi(NULL, hOut, "{Alt+5}");
							SendAnsi(hPipe, NULL, "\r\nRequest text area size [height;width]: ");
							SendAnsi(hPipe, hOut, "\x1B[18t");
							SendAnsi(hPipe, NULL, "\x01");
							break;
						case '6':
							SendAnsi(NULL, hOut, "{Alt+6}");
							SendAnsi(hPipe, NULL, "\r\nRequest screen area size [height;width]: ");
							SendAnsi(hPipe, hOut, "\x1B[19t");
							SendAnsi(hPipe, NULL, "\x01");
							break;
						case '7':
							SendAnsi(NULL, hOut, "{Alt+7}");
							SendAnsi(hPipe, NULL, "\r\nRequest terminal title: ");
							SendAnsi(hPipe, hOut, "\x1B[21t");
							SendAnsi(hPipe, NULL, "\x01");
							break;
						}
						continue;
					}
						

					if (r.Event.KeyEvent.uChar.AsciiChar == 27)
						WriteConsoleW(hOut, gszAnalogues+27, 1, &nWrite, NULL);
					else
						WriteConsoleA(hOut, &r.Event.KeyEvent.uChar.AsciiChar, 1, &nWrite, NULL);


					b = WriteFile(hPipe, &r.Event.KeyEvent.uChar.AsciiChar, 1, &nWrite, NULL);
					if (b && r.Event.KeyEvent.uChar.AsciiChar == '\r')
					{
						WriteConsoleA(hOut, "\n", 1, &nWrite, NULL);
						b = WriteFile(hPipe, "\n", 1, &nWrite, NULL);
					}

					if (!b)
					{
						nRead = GetLastError();
						printf("\n!!! WritePipe failed, code=%u !!!\n", nRead);
						return nRead;
					}
				}
			}
		}
	}

	return 0;
}

int _tmain(int argc, _TCHAR* argv[])
{
	int iRc = 0;

	InitializeCriticalSection(&cs);

	bool valid_args = false;
	for (int i = 0; i < argc; ++i)
	{
		if (_tcscmp(argv[i], _T("-utf8")) == 0)
		{
			SetConsoleCP(CP_UTF8);
			SetConsoleOutputCP(CP_UTF8);
			continue;
		}
		if (((i + 1) < argc) && _tcscmp(argv[i], _T("-srv")) == 0)
		{
			valid_args = true;
			iRc = ProcessSrv(argv[i+1]);
			break;
		}
		if (((i + 1) < argc) && _tcscmp(argv[i], _T("-in")) == 0)
		{
			valid_args = true;
			iRc = ProcessInput(argv[i+1]);
			break;
		}
		if (_tcscmp(argv[i], _T("-read")) == 0)
		{
			valid_args = true;
			iRc = InputThread(NULL);
			break;
		}
	}

	if (!valid_args)
	{
		printf(
			"Invalid usage\n"
			"\tAnsiDbg [-utf8] -in <name>\n"
			"\tAnsiDbg [-utf8] -out <name>\n"
			"Called as:\n"
		);
		for (int i = 0; i < argc; ++i)
			wprintf(L"\t#%i: %s\n", i, argv[i]);
		char buffer[80];
		printf("Press Enter to exit...");
		fgets(buffer, 80, stdin);
		iRc = 100;
	}

	if (hInThread)
	{
		TerminateThread(hInThread, 1);
	}
	return iRc;
}

