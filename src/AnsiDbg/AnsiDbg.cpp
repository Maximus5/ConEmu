// AnsiDbg.cpp : Defines the entry point for the console application.
//

#include <windows.h>
#include <stdio.h>
#include <tchar.h>

bool gbExit = false;

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

CRITICAL_SECTION cs;
HANDLE hInThread = NULL;

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
			if (b) SetConsoleTextAttribute(hErr, 11);
			if (ch == 27)
				WriteFile(hErr, "\\e", 2, &nWrite, NULL);
			else
				WriteFile(hErr, &ch, 1, &nWrite, NULL);
			if (b) SetConsoleTextAttribute(hErr, csbi.wAttributes);
			LeaveCriticalSection(&cs);
		}
		else
		{
			Sleep(10);
		}
	}
	return 0;
}



int ProcessSrv(LPCTSTR asName)
{
	HANDLE hPipeIn;
	TCHAR szName[MAX_PATH] = _T("\\\\.\\pipe\\");
	_tcscat(szName, asName);

	DWORD  nThread;
	hInThread = CreateThread(NULL, 0, InputThread, NULL, 0, &nThread);

	HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
	BOOL b; DWORD nRead, nWrite; char c;

	hPipeIn = CreateNamedPipe(szName, PIPE_ACCESS_INBOUND, PIPE_TYPE_BYTE, 1, 80, 80, 0, NULL);
	if (!hPipeIn || hPipeIn == INVALID_HANDLE_VALUE)
	{
		return PrintOut("\n!!! CreateNamedPipe failed, code=%u !!!\n");
	}

	char sInfo[300] = "Waiting for client '";
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
		b = ReadFile(hPipeIn, &c, 1, &nRead, NULL);
		if (b && nRead)
		{
			EnterCriticalSection(&cs);
			b = WriteFile(hOut, &c, nRead, &nWrite, NULL);
			LeaveCriticalSection(&cs);
			if (!b || nWrite != nRead)
			{
				return PrintOut("\n!!! WriteFile(StdOut) failed, code=%u !!!\n");
			}
		}
		else
		{
			nRead = GetLastError();
			return PrintOut("\n!!! Pipe reading error, code=%u !!!\n");
		}
	}

	return 0;
}

const wchar_t gszAnalogues[32] =
{
	32, 9786, 9787, 9829, 9830, 9827, 9824, 8226, 9688, 9675, 9689, 9794, 9792, 9834, 9835, 9788,
	9658, 9668, 8597, 8252,  182,  167, 9632, 8616, 8593, 8595, 8594, 8592, 8735, 8596, 9650, 9660
};

void SendAnsi(HANDLE hPipe, HANDLE hOut, LPCSTR asSeq)
{
	if (!asSeq || !*asSeq) return;
	DWORD nWrite, nLen = lstrlenA(asSeq);
	if (hPipe)
		WriteFile(hPipe, asSeq, nLen, &nWrite, NULL);
	if (hOut)
	{
		if (*asSeq == 27)
		{
			WriteConsoleW(hOut, gszAnalogues+27, 1, &nWrite, NULL);
			asSeq++; nLen--;
		}
		if (nLen)
			WriteConsoleA(hOut, asSeq, nLen, &nWrite, NULL);
	}
}

int ProcessInput(LPCTSTR asName)
{
	HANDLE hIn = GetStdHandle(STD_INPUT_HANDLE);
	HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
	TCHAR szName[MAX_PATH] = _T("\\\\.\\pipe\\");
	_tcscat(szName, asName);
	DWORD nRead, nWrite;
	INPUT_RECORD r;
	BOOL b;

	char sInfo[300] = "Connecting to server '";
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
	printf("\nConnected! Alt+X - exit, Alt+L - clear, Alt+F - fill\n");

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
					SendAnsi(hPipe, hOut, "\x1B[1G"); continue;
				case VK_END:
					if (r.Event.KeyEvent.dwControlKeyState & (RIGHT_CTRL_PRESSED|LEFT_CTRL_PRESSED))
						SendAnsi(hPipe, hOut, "\x1B[9999T");
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
					SendAnsi(hPipe, hOut, "\x1B[S"); continue;
				case VK_NEXT:
					SendAnsi(hPipe, hOut, "\x1B[T"); continue;
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
				if (r.Event.KeyEvent.uChar.AsciiChar)
				{
					if (r.Event.KeyEvent.dwControlKeyState & (RIGHT_CTRL_PRESSED|LEFT_CTRL_PRESSED|LEFT_ALT_PRESSED|RIGHT_ALT_PRESSED))
					{
						char szText[100];
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
							for (int i = 1; i <= 25; i++)
							{
								wsprintfA(szText, "Line %u\tLine %u\tLine %u\tLine %u\tLine %u\tLine %u\tLine %u\tLine %u\r\n", i,i,i,i,i,i,i,i);
								SendAnsi(hPipe, hOut, szText);
							}
							break;
						case '1':
							SendAnsi(hPipe, NULL, "\r\nRequest terminal primary DA: ");
							SendAnsi(hPipe, hOut, "\x1B[c"); continue;
						case '2':
							SendAnsi(hPipe, NULL, "\r\nRequest terminal secondary DA: ");
							SendAnsi(hPipe, hOut, "\x1B[>c"); continue;
						case '3':
							SendAnsi(hPipe, NULL, "\r\nRequest terminal status: ");
							SendAnsi(hPipe, hOut, "\x1B[5n"); continue;
						case '4':
							SendAnsi(hPipe, NULL, "\r\nRequest cursor pos [row;col]: ");
							SendAnsi(hPipe, hOut, "\x1B[6n"); continue;
						case '5':
							SendAnsi(hPipe, NULL, "\r\nRequest text area size [height;width]: ");
							SendAnsi(hPipe, hOut, "\x1B[18t"); continue;
						case '6':
							SendAnsi(hPipe, NULL, "\r\nRequest screen area size [height;width]: ");
							SendAnsi(hPipe, hOut, "\x1B[19t"); continue;
						case '7':
							SendAnsi(hPipe, NULL, "\r\nRequest terminal title: ");
							SendAnsi(hPipe, hOut, "\x1B[21t"); continue;
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

	if (argc == 3 && _tcscmp(argv[1], _T("-srv")) == 0)
	{
		iRc = ProcessSrv(argv[2]);
	}
	else if (argc == 3 && _tcscmp(argv[1], _T("-in")) == 0)
	{
		iRc = ProcessInput(argv[2]);
	}
	else if (argc == 2 && _tcscmp(argv[1], _T("-read")) == 0)
	{
		iRc = InputThread(NULL);
	}
	else
	{
		printf("Invalid usage\n\tAnsiDbg -in <name>\n\tAnsiDbg -out <name>\n");
		iRc = 100;
	}

	if (hInThread)
	{
		TerminateThread(hInThread, 1);
	}
	return iRc;
}

