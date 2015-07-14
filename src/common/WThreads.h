
#pragma once

HANDLE apiCreateThread(LPTHREAD_START_ROUTINE lpStartAddress, LPVOID lpParameter, LPDWORD lpThreadId, LPCSTR asThreadNameFormat = NULL, int anFormatArg = 0);
BOOL apiTerminateThread(HANDLE hThread, DWORD dwExitCode);

#if defined(_MSC_VER) && !defined(CONEMU_MINIMAL)
void SetThreadName(DWORD dwThreadID, char* threadName);
#else
#define SetThreadName(dwThreadID, threadName)
#endif
