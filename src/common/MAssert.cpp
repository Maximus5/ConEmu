
#include <windows.h>
#include "defines.h"
#include "MAssert.h"
#include "MStrSafe.h"

#ifdef _DEBUG
typedef struct tag_MyAssertInfo
{
	wchar_t szTitle[255];
	wchar_t szDebugInfo[4096];
} MyAssertInfo;
bool gbInMyAssertTrap = false;
bool gbAllowAssertThread = false;
HANDLE ghInMyAssertTrap = NULL;
DWORD gnInMyAssertThread = 0;
DWORD WINAPI MyAssertThread(LPVOID p)
{
	if (gbInMyAssertTrap)
	{
		// ≈сли уже в трапе - то повторно не вызывать, иначе может вывалитьс€ сери€ окон, что затруднит отладку
		return IDCANCEL;
	}

	MyAssertInfo* pa = (MyAssertInfo*)p;

	if (ghInMyAssertTrap == NULL)
	{
		ghInMyAssertTrap = CreateEvent(NULL, TRUE, FALSE, NULL);
	}

	gnInMyAssertThread = GetCurrentThreadId();
	ResetEvent(ghInMyAssertTrap);
	gbInMyAssertTrap = true;
	
	DWORD nRc = MessageBoxW(NULL, pa->szDebugInfo, pa->szTitle, MB_SETFOREGROUND|MB_SYSTEMMODAL|MB_RETRYCANCEL);

	gbInMyAssertTrap = false;
	SetEvent(ghInMyAssertTrap);
	gnInMyAssertThread = 0;
	return nRc;
}
void MyAssertTrap()
{
	_CrtDbgBreak();
}
int MyAssertProc(const wchar_t* pszFile, int nLine, const wchar_t* pszTest)
{
	MyAssertInfo a;
	wchar_t szExeName[MAX_PATH+1];
	if (!GetModuleFileNameW(NULL, szExeName, countof(szExeName))) szExeName[0] = 0;
	StringCbPrintfW(a.szTitle, countof(a.szTitle), L"CEAssert PID=%u TID=%u", GetCurrentProcessId(), GetCurrentThreadId());
	StringCbPrintfW(a.szDebugInfo, countof(a.szDebugInfo), L"Assertion in %s\n%s\n\n%s: %i\n\nPress 'Retry' to trap.",
	                szExeName, pszTest ? pszTest : L"", pszFile, nLine);
	DWORD dwCode = 0;

	if (gbAllowAssertThread)
	{
		DWORD dwTID;
		HANDLE hThread = CreateThread(NULL, 0, MyAssertThread, &a, 0, &dwTID);

		if (hThread == NULL)
		{
			return -1;
		}

		WaitForSingleObject(hThread, INFINITE);
		GetExitCodeThread(hThread, &dwCode);
		CloseHandle(hThread);
	}
	else
	{
		// ¬ Far попытка запустить MyAssertThread иногда зависает
		dwCode = MyAssertThread(&a);
	}

	return (dwCode == IDRETRY) ? -1 : 1;
}

void MyAssertShutdown()
{
	if (ghInMyAssertTrap != NULL)
	{
		if (gbInMyAssertTrap)
		{
			if (!gnInMyAssertThread)
			{
				_ASSERTE(gbInMyAssertTrap && gnInMyAssertThread);
			}
			else
			{
				HANDLE hHandles[2]; 
				hHandles[0] = OpenThread(SYNCHRONIZE, FALSE, gnInMyAssertThread);
				if (hHandles[0] == NULL)
				{
					_ASSERTE(hHandles[0] != NULL);
				}
				else
				{
					hHandles[1] = ghInMyAssertTrap;
					WaitForMultipleObjects(2, hHandles, FALSE, INFINITE);
					CloseHandle(hHandles[0]);
				}
			}
		}

		CloseHandle(ghInMyAssertTrap);
		ghInMyAssertTrap = NULL;
	}
}
#endif
