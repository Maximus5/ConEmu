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

int ProcessSrv(LPCTSTR asName)
{
	HANDLE hPipeIn;
	TCHAR szName[MAX_PATH] = _T("\\\\.\\pipe\\");
	_tcscat(szName, asName);

	HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
	BOOL b; DWORD nRead, nWrite; char c;

	hPipeIn = CreateNamedPipe(szName, PIPE_ACCESS_INBOUND, PIPE_TYPE_BYTE, 1, 80, 80, 0, NULL);
	if (!hPipeIn || hPipeIn == INVALID_HANDLE_VALUE)
	{
		return PrintOut("\n!!! CreateNamedPipe failed, code=%u !!!\n");
	}

	PrintOut("Waiting for client...\n");
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
			b = WriteFile(hOut, &c, nRead, &nWrite, NULL);
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

int ProcessInput(LPCTSTR asName)
{
	HANDLE hIn = GetStdHandle(STD_INPUT_HANDLE);
	HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
	TCHAR szName[MAX_PATH] = _T("\\\\.\\pipe\\");
	_tcscat(szName, asName);
	DWORD nRead, nWrite;
	INPUT_RECORD r;
	BOOL b;
	printf("Connecting to server...\n");
	WaitNamedPipe(szName, 30000);
	HANDLE hPipe = CreateFile(szName, GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);
	if (!hPipe || hPipe == INVALID_HANDLE_VALUE)
	{
		nRead = GetLastError();
		printf("\n!!! OpenPipe failed, code=%u !!!\n", nRead);
		return nRead;
	}
	printf("Connected, press Alt+X to exit!\n");

	SetConsoleCtrlHandler((PHANDLER_ROUTINE)HandlerRoutine, true);

	while (!gbExit)
	{
		if (ReadConsoleInputA(hIn, &r, 1, &nRead) && nRead)
		{
			if (r.EventType == KEY_EVENT && r.Event.KeyEvent.bKeyDown)
			{
				if (r.Event.KeyEvent.uChar.AsciiChar)
				{
					if ((r.Event.KeyEvent.dwControlKeyState & (LEFT_ALT_PRESSED|RIGHT_ALT_PRESSED))
						&& (r.Event.KeyEvent.uChar.AsciiChar == 'x' || r.Event.KeyEvent.uChar.AsciiChar == 'X'))
					{
						printf("\nExit session\n");
						break;
					}
					if (r.Event.KeyEvent.dwControlKeyState & (RIGHT_CTRL_PRESSED|LEFT_CTRL_PRESSED|LEFT_ALT_PRESSED|RIGHT_ALT_PRESSED))
						continue;

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
	if (argc == 3)
	{
		if (_tcscmp(argv[1], _T("-srv")) == 0)
			return ProcessSrv(argv[2]);
		else if (_tcscmp(argv[1], _T("-in")) == 0)
			return ProcessInput(argv[2]);
	}

	printf("Invalid usage\n\tAnsiDbg -in <name>\n\tAnsiDbg -out <name>\n");
	return 100;
}

