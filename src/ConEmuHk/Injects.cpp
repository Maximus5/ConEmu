
/*
Copyright (c) 2009-2014 Maximus5
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

#include <windows.h>
#include "../common/common.hpp"
#include "../common/ConEmuCheck.h"
#include "../common/execute.h"
#include "../ConEmuCD/ExitCodes.h"
#include "../common/WObjects.h"
//#include "ConEmuHooks.h"
#include "Console2.h"

extern HMODULE ghOurModule;
extern HWND ghConWnd;
UINT_PTR gfnLoadLibrary = 0;
UINT_PTR gfnLdrGetDllHandleByName = 0;
//extern HMODULE ghPsApi;

HANDLE ghSkipSetThreadContextForThread = NULL;

HANDLE ghInjectsInMainThread = NULL;

// Проверить, что gfnLoadLibrary лежит в пределах модуля hKernel!
UINT_PTR GetLoadLibraryAddress()
{
	if (gfnLoadLibrary)
		return gfnLoadLibrary;

	UINT_PTR fnLoadLibrary = 0;
	HMODULE hKernel = ::GetModuleHandle(L"kernel32.dll");
	if (!hKernel || LDR_IS_RESOURCE(hKernel))
	{
		_ASSERTE(hKernel && !LDR_IS_RESOURCE(hKernel));
		return 0;
	}

	HMODULE hConEmuHk = ::GetModuleHandle(WIN3264TEST(L"ConEmuHk.dll", L"ConEmuHk64.dll"));
	if (hConEmuHk && (hConEmuHk != ghOurModule))
	{
		typedef FARPROC (WINAPI* GetLoadLibraryW_t)();
		GetLoadLibraryW_t GetLoadLibraryW = (GetLoadLibraryW_t)::GetProcAddress(hConEmuHk, "GetLoadLibraryW");
		if (GetLoadLibraryW)
		{
			fnLoadLibrary = (UINT_PTR)GetLoadLibraryW();
		}
	}

	if (!fnLoadLibrary)
	{
		fnLoadLibrary = (UINT_PTR)::GetProcAddress(hKernel, "LoadLibraryW");
	}

	// Функция должна быть именно в Kernel32.dll (а не в какой либо другой библиотеке, мало ли кто захукал...)
	if (!CheckCallbackPtr(hKernel, 1, (FARPROC*)&fnLoadLibrary, TRUE))
	{
		// _ASSERTE уже был
		return 0;
	}

	gfnLoadLibrary = fnLoadLibrary;
	return fnLoadLibrary;
}

UINT_PTR GetLdrGetDllHandleByNameAddress()
{
	if (gfnLdrGetDllHandleByName)
		return gfnLdrGetDllHandleByName;

	UINT_PTR fnLdrGetDllHandleByName = 0;
	HMODULE hNtDll = ::GetModuleHandle(L"ntdll.dll");
	if (!hNtDll || LDR_IS_RESOURCE(hNtDll))
	{
		_ASSERTE(hNtDll&& !LDR_IS_RESOURCE(hNtDll));
		return 0;
	}

	fnLdrGetDllHandleByName = (UINT_PTR)::GetProcAddress(hNtDll, "LdrGetDllHandleByName");

	// Функция должна быть именно в ntdll.dll (а не в какой либо другой библиотеке, мало ли кто захукал...)
	if (!CheckCallbackPtr(hNtDll, 1, (FARPROC*)&fnLdrGetDllHandleByName, TRUE))
	{
		// _ASSERTE уже был
		return 0;
	}

	gfnLdrGetDllHandleByName = fnLdrGetDllHandleByName;
	return fnLdrGetDllHandleByName;
}

// The handle must have the PROCESS_CREATE_THREAD, PROCESS_QUERY_INFORMATION, PROCESS_VM_OPERATION, PROCESS_VM_WRITE, and PROCESS_VM_READ
// The function may start appropriate bitness of ConEmuC.exe with "/SETHOOKS=..." switch
// If bitness matches, use WriteProcessMemory and SetThreadContext immediately
CINJECTHK_EXIT_CODES InjectHooks(PROCESS_INFORMATION pi, BOOL abLogProcess)
{
	CINJECTHK_EXIT_CODES iRc = CIH_OK/*0*/;
#ifndef CONEMUHK_EXPORTS
	_ASSERTE(FALSE)
#endif
	//DWORD dwErr = 0; //, dwWait = 0;
	wchar_t szPluginPath[MAX_PATH*2], *pszSlash;
	//HANDLE hFile = NULL;
	//wchar_t* pszPathInProcess = NULL;
	//SIZE_T write = 0;
	//HANDLE hThread = NULL; DWORD nThreadID = 0;
	//LPTHREAD_START_ROUTINE ptrLoadLibrary = NULL;
	_ASSERTE(ghOurModule!=NULL);
	BOOL is64bitOs = FALSE;
	//BOOL isWow64process = FALSE;
	int  ImageBits = 32; //-V112
	//DWORD ImageSubsystem = 0;
	//isWow64process = FALSE;
#ifdef WIN64
	is64bitOs = TRUE;
#endif
	// для проверки IsWow64Process
	HMODULE hKernel = GetModuleHandle(L"kernel32.dll");
	HMODULE hNtDll = NULL;
	DEBUGTEST(int iFindAddress = 0);
	//bool lbInj = false;
	//UINT_PTR fnLoadLibrary = NULL;
	//DWORD fLoadLibrary = 0;
	DWORD nErrCode = 0, nWait = 0;
	int SelfImageBits = WIN3264TEST(32,64);

	DWORDLONG const dwlConditionMask = VerSetConditionMask(VerSetConditionMask(0, VER_MAJORVERSION, VER_GREATER_EQUAL), VER_MINORVERSION, VER_GREATER_EQUAL);
	_ASSERTE(_WIN32_WINNT_WIN7==0x601);
	OSVERSIONINFOEXW osvi7 = {sizeof(osvi7), HIBYTE(_WIN32_WINNT_WIN7), LOBYTE(_WIN32_WINNT_WIN7)};
	BOOL bOsWin7 = VerifyVersionInfoW(&osvi7, VER_MAJORVERSION | VER_MINORVERSION, dwlConditionMask);

	if (!hKernel)
	{
		iRc = CIH_KernelNotLoaded/*-510*/;
		goto wrap;
	}
	//if (!nOsVer)
	//{
	//	iRc = CIH_OsVerFailed/*-511*/;
	//	goto wrap;
	//}
	if (bOsWin7)
	{
		hNtDll = GetModuleHandle(L"ntdll.dll");
		// Windows7 +
		if (!hNtDll)
		{
			iRc = CIH_NtdllNotLoaded/*-512*/;
			goto wrap;
		}
	}

	// Процесс не был стартован, или уже завершился
	nWait = WaitForSingleObject(pi.hProcess, 0);
	if (nWait == WAIT_OBJECT_0)
	{
		iRc = CIH_ProcessWasTerminated/*-506*/;
		goto wrap;
	}

	if (!GetModuleFileName(ghOurModule, szPluginPath, MAX_PATH))
	{
		#ifdef _DEBUG
		DWORD dwErr = GetLastError();
		_CrtDbgBreak();
		#endif
		//_printf("GetModuleFileName failed! ErrCode=0x%08X\n", dwErr);
		iRc = CIH_GetModuleFileName/*-501*/;
		goto wrap;
	}

	pszSlash = wcsrchr(szPluginPath, L'\\');


	//if (pszSlash) pszSlash++; else pszSlash = szPluginPath;
	if (!pszSlash)
		pszSlash = szPluginPath;

	*pszSlash = 0;
	//TODO("x64 injects");
	//lstrcpy(pszSlash, L"ConEmuHk.dll");
	//
	//hFile = CreateFile(szPluginPath, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
	//if (hFile == INVALID_HANDLE_VALUE)
	//{
	//	dwErr = GetLastError();
	//	_printf("\".\\ConEmuHk.dll\" not found! ErrCode=0x%08X\n", dwErr);
	//	goto wrap;
	//}
	//CloseHandle(hFile); hFile = NULL;
	//BOOL is64bitOs = FALSE, isWow64process = FALSE;
	//int  ImageBits = 32;
	//DWORD ImageSubsystem = 0;
	//isWow64process = FALSE;
	//#ifdef WIN64
	//is64bitOs = TRUE;
	//#endif
	//HMODULE hKernel = GetModuleHandle(L"kernel32.dll");

	if (hKernel)
	{
		typedef BOOL (WINAPI* IsWow64Process_t)(HANDLE, PBOOL);
		IsWow64Process_t IsWow64Process_f = (IsWow64Process_t)GetProcAddress(hKernel, "IsWow64Process");

		if (IsWow64Process_f)
		{
			BOOL bWow64 = FALSE;
#ifndef WIN64

			// Текущий процесс - 32-битный, (bWow64==TRUE) будет означать что OS - 64битная
			if (IsWow64Process_f(GetCurrentProcess(), &bWow64) && bWow64)
				is64bitOs = TRUE;

#else
			_ASSERTE(is64bitOs);
#endif
			// Теперь проверить запущенный процесс
			bWow64 = FALSE;

			if (is64bitOs && IsWow64Process_f(pi.hProcess, &bWow64) && !bWow64)
				ImageBits = 64;
		}
	}

	//int iFindAddress = 0;
	//bool lbInj = false;
	////UINT_PTR fnLoadLibrary = NULL;
	////DWORD fLoadLibrary = 0;
	//DWORD nErrCode = 0;
	//int SelfImageBits;
	//#ifdef WIN64
	//SelfImageBits = 64;
	//#else
	//SelfImageBits = 32;
	//#endif

	//#ifdef WIN64
	//	fnLoadLibrary = (UINT_PTR)::GetProcAddress(::GetModuleHandle(L"kernel32.dll"), "LoadLibraryW");

	//	// 64битный conemuc сможет найти процедуру и в 32битном процессе
	//	iFindAddress = FindKernelAddress(pi.hProcess, pi.dwProcessId, &fLoadLibrary);

	//#else
	// Если битность не совпадает - нужен helper
	if (ImageBits != SelfImageBits)
	{
		DWORD dwPidWait = WaitForSingleObject(pi.hProcess, 0);
		if (dwPidWait == WAIT_OBJECT_0)
		{
			_ASSERTE(dwPidWait != WAIT_OBJECT_0);
		}
		// Требуется 64битный(32битный?) comspec для установки хука
		DEBUGTEST(iFindAddress = -1);
		HANDLE hProcess = NULL, hThread = NULL;
		DuplicateHandle(GetCurrentProcess(), pi.hProcess, GetCurrentProcess(), &hProcess, 0, TRUE, DUPLICATE_SAME_ACCESS);
		DuplicateHandle(GetCurrentProcess(), pi.hThread, GetCurrentProcess(), &hThread, 0, TRUE, DUPLICATE_SAME_ACCESS);
		_ASSERTE(hProcess && hThread);
		#ifdef _WIN64
		// Если превышение DWORD в Handle - то запускаемый 32битный обломится. Но вызывается ли он вообще?
		if (((DWORD)hProcess != (DWORD_PTR)hProcess) || ((DWORD)hThread != (DWORD_PTR)hThread))
		{
			_ASSERTE(((DWORD)hProcess == (DWORD_PTR)hProcess) && ((DWORD)hThread == (DWORD_PTR)hThread));
			iRc = CIH_WrongHandleBitness/*-509*/;
			goto wrap;
		}
		#endif
		wchar_t sz64helper[MAX_PATH*2];
		msprintf(sz64helper, countof(sz64helper),
		          L"\"%s\\ConEmuC%s.exe\" /SETHOOKS=%X,%u,%X,%u",
		          szPluginPath, (ImageBits==64) ? L"64" : L"",
		          LODWORD(hProcess), pi.dwProcessId, LODWORD(hThread), pi.dwThreadId);
		STARTUPINFO si = {sizeof(STARTUPINFO)};
		PROCESS_INFORMATION pi64 = {NULL};
		LPSECURITY_ATTRIBUTES lpNotInh = LocalSecurity();
		SECURITY_ATTRIBUTES SecInh = {sizeof(SECURITY_ATTRIBUTES)};
		SecInh.lpSecurityDescriptor = lpNotInh->lpSecurityDescriptor;
		SecInh.bInheritHandle = TRUE;

		// Добавил DETACHED_PROCESS, чтобы helper не появлялся в списке процессов консоли,
		// а то у сервера может крышу сорвать, когда helper исчезнет, а приложение еще не появится.
		BOOL lbHelper = CreateProcess(NULL, sz64helper, &SecInh, &SecInh, TRUE, HIGH_PRIORITY_CLASS|DETACHED_PROCESS, NULL, NULL, &si, &pi64);

		if (!lbHelper)
		{
			nErrCode = GetLastError();
			// Ошибки показывает вызывающая функция/процесс
			iRc = CIH_CreateProcess/*-502*/;

			CloseHandle(hProcess); CloseHandle(hThread);
			goto wrap;
		}
		else
		{
			WaitForSingleObject(pi64.hProcess, INFINITE);

			if (!GetExitCodeProcess(pi64.hProcess, &nErrCode))
				nErrCode = -1;

			CloseHandle(pi64.hProcess); CloseHandle(pi64.hThread);
			CloseHandle(hProcess); CloseHandle(hThread);

			if (((int)nErrCode == CERR_HOOKS_WAS_SET) || ((int)nErrCode == CERR_HOOKS_WAS_ALREADY_SET))
			{
				iRc = CIH_OK/*0*/;
				goto wrap;
			}
			else if ((int)nErrCode == CERR_HOOKS_FAILED)
			{
				iRc = CIH_WrapperFailed/*-505*/;
				goto wrap;
			}

			// Ошибки показывает вызывающая функция/процесс
		}

		// Уже все ветки должны были быть обработаны!
		_ASSERTE(FALSE);
		iRc = CIH_WrapperGeneral/*-504*/;
		goto wrap;

	}
	else
	{
		//iFindAddress = FindKernelAddress(pi.hProcess, pi.dwProcessId, &fLoadLibrary);
		//fnLoadLibrary = (UINT_PTR)::GetProcAddress(::GetModuleHandle(L"kernel32.dll"), "LoadLibraryW");
		if (!GetLoadLibraryAddress())
		{
			_ASSERTE(gfnLoadLibrary!=NULL);
			iRc = CIH_GetLoadLibraryAddress/*-503*/;
			goto wrap;
		}
		else if (bOsWin7 && !GetLdrGetDllHandleByNameAddress())
		{
			_ASSERTE(gfnLdrGetDllHandleByName!=NULL);
			iRc = CIH_GetLdrHandleAddress/*-514*/;
			goto wrap;
		}
		else
		{
			// -- не имеет смысла. процесс еще "не отпущен", поэтому CreateToolhelp32Snapshot(TH32CS_SNAPMODULE) обламывается
			//// Проверить, а не Гуй ли это?
			//BOOL lbIsGui = FALSE;
			//if (!abForceGui)
			//{
			//	DWORD nBits = 0;
			//	if (GetImageSubsystem(pi, ImageSubsystem, nBits))
			//		lbIsGui = (ImageSubsystem == IMAGE_SUBSYSTEM_WINDOWS_GUI);
			//	_ASSERTE(nBits == ImageBits);
			//	if (lbIsGui)
			//	{
			//		iRc = CIH_OK/*0*/;
			//		goto wrap;
			//	}
			//}

			// ??? Сначала нужно проверить, может есть проблема с адресами (ASLR) ???
			//-- ReadProcessMemory возвращает ошибку 299, и cch_dos_read==0, так что не катит...
			//IMAGE_DOS_HEADER dos_hdr = {}; SIZE_T cch_dos_read = 0;
			//BOOL bRead = ::ReadProcessMemory(pi.hProcess, (LPVOID)(DWORD_PTR)hKernel, &dos_hdr, sizeof(dos_hdr), &cch_dos_read);

			DWORD_PTR ptrAllocated = 0; DWORD nAllocated = 0;
			//iRc = bRead ? InjectHookDLL(pi, gfnLoadLibrary, ImageBits, szPluginPath, &ptrAllocated, &nAllocated) : -1000;
			InjectHookFunctions fnArg = {hKernel, gfnLoadLibrary, hNtDll, gfnLdrGetDllHandleByName};
			iRc = InjectHookDLL(pi, &fnArg, ImageBits, szPluginPath, &ptrAllocated, &nAllocated);

			if (abLogProcess || (iRc != CIH_OK/*0*/))
			{
				int ImageSystem = 0;
				wchar_t szInfo[128];
				if (iRc != CIH_OK/*0*/)
				{
					DWORD nErr = GetLastError();
					msprintf(szInfo, countof(szInfo), L"InjectHookDLL failed, code=%i:0x%08X", iRc, nErr);
				}
				#ifdef _WIN64
				_ASSERTE(SelfImageBits == 64);
				if (iRc == CIH_OK/*0*/)
				{
					if ((DWORD)(ptrAllocated >> 32)) //-V112
					{
						msprintf(szInfo, countof(szInfo), L"Alloc: 0x%08X%08X, Size: %u",
							(DWORD)(ptrAllocated >> 32), (DWORD)(ptrAllocated & 0xFFFFFFFF), nAllocated); //-V112
					}
					else
					{
						msprintf(szInfo, countof(szInfo), L"Alloc: 0x%08X, Size: %u",
							(DWORD)(ptrAllocated & 0xFFFFFFFF), nAllocated); //-V112
					}
				}
				#else
				_ASSERTE(SelfImageBits == 32);
				if (iRc == CIH_OK/*0*/)
				{
					msprintf(szInfo, countof(szInfo), L"Alloc: 0x" WIN3264TEST(L"%08X",L"%X%08X") L", Size: %u", WIN3264WSPRINT(ptrAllocated), nAllocated);
				}
				#endif

				CESERVER_REQ* pIn = ExecuteNewCmdOnCreate(
					NULL, ghConWnd, eSrvLoaded,
					L"", szInfo, L"", NULL, NULL, NULL, NULL,
					SelfImageBits, ImageSystem, NULL, NULL, NULL);
				if (pIn)
				{
					CESERVER_REQ* pOut = ExecuteGuiCmd(ghConWnd, pIn, ghConWnd);
					ExecuteFreeResult(pIn);
					if (pOut) ExecuteFreeResult(pOut);
				}
			}
		}
	}

//#endif
	//if (iFindAddress != 0)
	//{
	////	iRc = CIH_GeneralError/*-1*/;
	//	goto wrap;
	//}
	//fnLoadLibrary = (UINT_PTR)fLoadLibrary;
	//if (!lbInj)
	//{
 	//	iRc = CIH_GeneralError/*-1*/;
	//	goto wrap;
	//}
	//WARNING("The process handle must have the PROCESS_VM_OPERATION access right!");
	//size = (lstrlen(szPluginPath)+1)*2;
	//TODO("Будет облом на DOS (16бит) приложениях");
	//TODO("Проверить, сможет ли ConEmu.x64 инжектиться в 32битные приложения?");
	//pszPathInProcess = (wchar_t*)VirtualAllocEx(hProcess, 0, size, MEM_COMMIT, PAGE_READWRITE);
	//if (!pszPathInProcess)
	//{
	//	dwErr = GetLastError();
	//	_printf("VirtualAllocEx failed! ErrCode=0x%08X\n", dwErr);
	//	goto wrap;
	//}
	//if (!WriteProcessMemory(hProcess, pszPathInProcess, szPluginPath, size, &write ) || size != write)
	//{
	//	dwErr = GetLastError();
	//	_printf("WriteProcessMemory failed! ErrCode=0x%08X\n", dwErr);
	//	goto wrap;
	//}
	//
	//TODO("Получить адрес LoadLibraryW в адресном пространстве запущенного процесса!");
	//ptrLoadLibrary = (LPTHREAD_START_ROUTINE)::GetProcAddress(::GetModuleHandle(L"Kernel32.dll"), "LoadLibraryW");
	//if (ptrLoadLibrary == NULL)
	//{
	//	dwErr = GetLastError();
	//	_printf("GetProcAddress(kernel32, LoadLibraryW) failed! ErrCode=0x%08X\n", dwErr);
	//	goto wrap;
	//}
	//
	//if (ptrLoadLibrary)
	//{
	//	// The handle must have the PROCESS_CREATE_THREAD, PROCESS_QUERY_INFORMATION, PROCESS_VM_OPERATION, PROCESS_VM_WRITE, and PROCESS_VM_READ
	//	hThread = CreateRemoteThread(hProcess, NULL, 0, ptrLoadLibrary, pszPathInProcess, 0, &nThreadID);
	//	if (!hThread)
	//	{
	//		dwErr = GetLastError();
	//		_printf("CreateRemoteThread failed! ErrCode=0x%08X\n", dwErr);
	//		goto wrap;
	//	}
	//	// Дождаться, пока отработает
	//	dwWait = WaitForSingleObject(hThread,
	//		#ifdef _DEBUG
	//					INFINITE
	//		#else
	//					10000
	//		#endif
	//		);
	//	if (dwWait != WAIT_OBJECT_0) {
	//		dwErr = GetLastError();
	//		_printf("Inject tread timeout!");
	//		goto wrap;
	//	}
	//}
	//
wrap:
//#endif
	if (iRc == CIH_OK/*0*/)
	{
		wchar_t szEvtName[64];
		SafeCloseHandle(ghInjectsInMainThread);

		// ResumeThread was not called yet, set event
		msprintf(szEvtName, countof(szEvtName), CECONEMUROOTTHREAD, pi.dwProcessId);
		ghInjectsInMainThread = CreateEvent(LocalSecurity(), TRUE, TRUE, szEvtName);
		if (ghInjectsInMainThread)
		{
			SetEvent(ghInjectsInMainThread);
		}
		else
		{
			_ASSERTEX(ghInjectsInMainThread!=NULL);
		}
	}
	return iRc;
}
