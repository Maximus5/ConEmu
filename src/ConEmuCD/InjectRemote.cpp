
/*
Copyright (c) 2009-2012 Maximus5
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

THIS SOFTWARE IS PROVIDED BY THE AUTHOR ''AS IS'' AND ANY EXPRESS OR
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

#include "ConEmuC.h"
#include "../common/MAssert.h"
#include "../common/WinObjects.h"
#include "Infiltrate.h"
#include "../ConEmuHk/Injects.h"

// 0 - OK, иначе - ошибка
int InfiltrateDll(HANDLE hProcess, LPCWSTR asConEmuHk)
{
	int iRc = -150;

	//if (iRc != -150)
	//{
	//	InfiltrateProc(NULL); InfiltrateEnd();
	//}
	//const size_t cb = ((size_t)InfiltrateEnd) - ((size_t)InfiltrateProc);

	InfiltrateArg dat = {};
	HMODULE hKernel = NULL;
	HANDLE hThread = NULL;
	DWORD id = 0;
	LPTHREAD_START_ROUTINE pRemoteProc = NULL;
	PVOID pRemoteDat = NULL;
	CreateRemoteThread_t _CreateRemoteThread = NULL;
	char FuncName[20];
	void* ptrCode;
	size_t cbCode;

	//_ASSERTE("InfiltrateDll"==(void*)TRUE);

	cbCode = GetInfiltrateProc(&ptrCode);
	// Примерно, проверка размера кода созданного компилятором
	if (cbCode != WIN3264TEST(68,79))
	{
		_ASSERTE(cbCode == WIN3264TEST(68,79));
		iRc = -100;
		goto wrap;
	}

	if (lstrlen(asConEmuHk) >= (int)countof(dat.szConEmuHk))
	{
		iRc = -101;
		goto wrap;
	}

	// Исполняемый код загрузки библиотеки
	pRemoteProc = (LPTHREAD_START_ROUTINE) VirtualAllocEx(
		hProcess,	// Target process
		NULL,		// Let the VMM decide where
		cbCode,		// Size
		MEM_COMMIT,	// Commit the memory
		PAGE_EXECUTE_READWRITE); // Protections
	if (!pRemoteProc)
	{
		iRc = -102;
		goto wrap;
	}
	if (!WriteProcessMemory(
		hProcess,		// Target process
		(void*)pRemoteProc,	// Source for code
		ptrCode,		// The code
		cbCode,			// Code length
		NULL))			// We don't care
	{
		iRc = -103;
		goto wrap;
	}

	// Путь к нашей библиотеке
	lstrcpyn(dat.szConEmuHk, asConEmuHk, countof(dat.szConEmuHk));

	// Kernel-процедуры
	hKernel = LoadLibrary(L"Kernel32.dll");
	if (!hKernel)
	{
		iRc = -104;
		goto wrap;
	}

	// Избежать статической линковки и строки "CreateRemoteThread" в бинарнике
	FuncName[ 0] = 'C'; FuncName[ 2] = 'e'; FuncName[ 4] = 't'; FuncName[ 6] = 'R'; FuncName[ 8] = 'm';
	FuncName[ 1] = 'r'; FuncName[ 3] = 'a'; FuncName[ 5] = 'e'; FuncName[ 7] = 'e'; FuncName[ 9] = 'o';
	FuncName[10] = 't'; FuncName[12] = 'T'; FuncName[14] = 'r'; FuncName[16] = 'a';
	FuncName[11] = 'e'; FuncName[13] = 'h'; FuncName[15] = 'e'; FuncName[17] = 'd'; FuncName[18] = 0;
	_CreateRemoteThread = (CreateRemoteThread_t)GetProcAddress(hKernel, FuncName);

	// Functions for external process. MUST BE SAME ADDRESSES AS CURRENT PROCESS.
	// kernel32.dll компонуется таким образом, что всегда загружается по одному определенному адресу в памяти
	// Поэтому адреса процедур для приложений одинаковой битности совпадают (в разных процессах)
	dat._GetLastError = (GetLastError_t)GetProcAddress(hKernel, "GetLastError");
	dat._SetLastError = (SetLastError_t)GetProcAddress(hKernel, "SetLastError");
	dat._LoadLibraryW = (LoadLibraryW_t)GetLoadLibraryAddress(); // GetProcAddress(hKernel, "LoadLibraryW");
	if (!_CreateRemoteThread || !dat._LoadLibraryW || !dat._SetLastError || !dat._GetLastError)
	{
		iRc = -105;
		goto wrap;
	}
	else
	{
		// Проверим, что адреса этих функций действительно лежат в модуле Kernel32.dll
		// и не были кем-то перехвачены до нас.
		FARPROC proc[] = {(FARPROC)dat._GetLastError, (FARPROC)dat._SetLastError, (FARPROC)dat._LoadLibraryW};
		if (!CheckCallbackPtr(hKernel, countof(proc), proc, TRUE, TRUE))
		{
			// Если функции перехвачены - попытка выполнить код по этим адресам
			// скорее всего приведет к ошибке доступа, что не есть гут.
			iRc = -111;
			goto wrap;
		}
	}

	// Копируем параметры в процесс
	pRemoteDat = VirtualAllocEx(hProcess, NULL, sizeof(InfiltrateArg), MEM_COMMIT, PAGE_READWRITE);
	if(!pRemoteDat)
	{
		iRc = -106;
		goto wrap;
	}
	if (!WriteProcessMemory(hProcess, pRemoteDat, &dat, sizeof(InfiltrateArg), NULL))
	{
		iRc = -107;
		goto wrap;
	}

	// Запускаем поток в процессе hProcess
	// В принципе, на эту функцию могут ругаться антивирусы
	hThread = _CreateRemoteThread(
		hProcess,		// Target process
		NULL,			// No security
		4096 * 16,		// 16 pages of stack
		pRemoteProc,	// Thread routine address
		pRemoteDat,		// Data
		0,				// Run NOW
		&id);
	if (!hThread)
	{
		iRc = -108;
		goto wrap;
	}

	// Дождаться пока поток завершится
	WaitForSingleObject(hThread, INFINITE);

	// И считать результат
	if (!ReadProcessMemory(
		hProcess,		// Target process
		pRemoteDat,		// Their data
		&dat,			// Our data
		sizeof(InfiltrateArg),	// Size
		NULL))			// We don't care
	{
		iRc = -109;
		goto wrap;
	}

	// Вернуть результат загрузки
	SetLastError((dat.hInst != NULL) ? 0 : (DWORD)dat.ErrCode);
	iRc = (dat.hInst != NULL) ? 0 : -110;
wrap:
	if (hKernel)
		FreeLibrary(hKernel);
	if (hThread)
		CloseHandle(hThread);
	if(pRemoteProc)
		VirtualFreeEx(hProcess, (void*)pRemoteProc, cbCode, MEM_RELEASE);
	if(pRemoteDat)
		VirtualFreeEx(hProcess, pRemoteDat, sizeof(InfiltrateArg), MEM_RELEASE);
	return iRc;
}

int InjectRemote(DWORD nRemotePID)
{
	int iRc = -1;
	BOOL lbWin64 = WIN3264TEST(IsWindows64(),TRUE);
	BOOL is32bit;
	DWORD nWrapperWait = (DWORD)-1, nWrapperResult = (DWORD)-1;
	HANDLE hProc = NULL;
	wchar_t szSelf[MAX_PATH+16], *pszNamePtr, szArgs[32];

	if (!GetModuleFileName(NULL, szSelf, MAX_PATH) || !(pszNamePtr = (wchar_t*)PointToName(szSelf)))
	{
		iRc = -200;
		goto wrap;
	}

	hProc = OpenProcess(PROCESS_CREATE_THREAD|PROCESS_QUERY_INFORMATION|PROCESS_VM_OPERATION|PROCESS_VM_WRITE|PROCESS_VM_READ, FALSE, nRemotePID);
	if (hProc == NULL)
	{
		iRc = -201;
		goto wrap;
	}

	// Определить битность процесса, Если он 32битный, а текущий - ConEmuC64.exe
	// Перезапустить 32битную версию ConEmuC.exe
	if (!lbWin64)
	{
		is32bit = TRUE; // x86 OS!
	}
	else
	{
		is32bit = FALSE; // x64 OS!

		// Проверяем, кто такой nRemotePID
		HMODULE hKernel = GetModuleHandleW(L"kernel32.dll");

		if (hKernel)
		{
			typedef BOOL (WINAPI* IsWow64Process_t)(HANDLE hProcess, PBOOL Wow64Process);
			IsWow64Process_t IsWow64Process_f = (IsWow64Process_t)GetProcAddress(hKernel, "IsWow64Process");

			if (IsWow64Process_f)
			{
				BOOL bWow64 = FALSE;

				if (IsWow64Process_f(hProc, &bWow64) && bWow64)
				{
					// По идее, такого быть не должно. ConEmu должен был запустить 32битный conemuC.exe
					#ifdef _WIN64
					_ASSERTE(bWow64==FALSE);
					#endif
					is32bit = TRUE;
				}
			}
		}
	}

	if (is32bit != WIN3264TEST(TRUE,FALSE))
	{
		// По идее, такого быть не должно. ConEmu должен был запустить соответствующий conemuC*.exe
		_ASSERTE(is32bit == WIN3264TEST(TRUE,FALSE));
		PROCESS_INFORMATION pi = {};
		STARTUPINFO si = {sizeof(si)};

		_wcscpy_c(pszNamePtr, 16, is32bit ? L"ConEmuC.exe" : L"ConEmuC64.exe");
		_wsprintf(szArgs, SKIPLEN(countof(szArgs)) L" /INJECT=%u", nRemotePID);

		if (!CreateProcess(szSelf, szArgs, NULL, NULL, FALSE, NORMAL_PRIORITY_CLASS, NULL, NULL, &si, &pi))
		{
			iRc = -202;
			goto wrap;
		}
		nWrapperWait = WaitForSingleObject(pi.hProcess, INFINITE);
		GetExitCodeProcess(pi.hProcess, &nWrapperResult);
		CloseHandle(pi.hProcess);
		CloseHandle(pi.hThread);
		if (nWrapperResult != 0)
		{
			iRc = -203;
			SetLastError(nWrapperResult);
			goto wrap;
		}
		// Значит всю работу сделал враппер
		iRc = 0;
		goto wrap;
	}

	// Поехали
	_wcscpy_c(pszNamePtr, 16, is32bit ? L"ConEmuHk.dll" : L"ConEmuHk64.dll");
	if (!FileExists(szSelf))
	{
		iRc = -250;
	}

	iRc = InfiltrateDll(hProc, szSelf);
wrap:
	if (hProc != NULL)
		CloseHandle(hProc);
	return iRc;
}
