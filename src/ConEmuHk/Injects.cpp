
/*
Copyright (c) 2009-present Maximus5
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

#define HIDE_USE_EXCEPTION_INFO

#include "../common/Common.h"
#include "../common/ConEmuCheck.h"
#include "../common/execute.h"
#include "../common/WModuleCheck.h"
#include "Injects.h"
#include "InjectsBootstrap.h"
#include "hlpProcess.h"

extern HMODULE ghOurModule;
extern HWND ghConWnd;

InjectsFnPtr gfLoadLibrary;
InjectsFnPtr gfLdrGetDllHandleByName;

HANDLE ghSkipSetThreadContextForThread = NULL;

HANDLE ghInjectsInMainThread = NULL;

// Проверить, что gfLoadLibrary лежит в пределах модуля hKernel!
UINT_PTR GetLoadLibraryAddress()
{
	if (gfLoadLibrary.fnPtr)
		return gfLoadLibrary.fnPtr;

	UINT_PTR fnLoadLibrary = 0;
	HMODULE hKernel32 = ::GetModuleHandle(L"kernel32.dll");
	HMODULE hKernelBase = IsWin8() ? ::GetModuleHandle(L"KernelBase.dll") : NULL;
	if (!hKernel32 || LDR_IS_RESOURCE(hKernel32))
	{
		_ASSERTE(hKernel32 && !LDR_IS_RESOURCE(hKernel32));
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

	UINT_PTR fnKernel32 = (UINT_PTR)::GetProcAddress(hKernel32, "LoadLibraryW");
	UINT_PTR fnKernelBase = hKernelBase ? (UINT_PTR)::GetProcAddress(hKernelBase, "LoadLibraryW") : 0L;
	HMODULE hKernel = fnKernelBase ? hKernelBase : hKernel32;
	if (!fnLoadLibrary)
	{
		fnLoadLibrary = fnKernelBase ? fnKernelBase : fnKernel32;
	}

	// Функция должна быть именно в Kernel32.dll (а не в какой либо другой библиотеке, мало ли кто захукал...)
	if (!CheckCallbackPtr(hKernel, 1, (FARPROC*)&fnLoadLibrary, TRUE))
	{
		// _ASSERTE уже был
		return 0;
	}

	gfLoadLibrary = InjectsFnPtr(hKernel, fnLoadLibrary, (hKernel==hKernel32) ? L"Kernel32.dll" : L"KernelBase.dll");
	return fnLoadLibrary;
}

UINT_PTR GetLdrGetDllHandleByNameAddress()
{
	if (gfLdrGetDllHandleByName.fnPtr)
		return gfLdrGetDllHandleByName.fnPtr;

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

	gfLdrGetDllHandleByName = InjectsFnPtr(hNtDll, fnLdrGetDllHandleByName, L"ntdll.dll");
	return fnLdrGetDllHandleByName;
}

// The handle must have the PROCESS_CREATE_THREAD, PROCESS_QUERY_INFORMATION, PROCESS_VM_OPERATION, PROCESS_VM_WRITE, and PROCESS_VM_READ
// The function may start appropriate bitness of ConEmuC.exe with "/SETHOOKS=..." switch
// If bitness matches, use WriteProcessMemory and SetThreadContext immediately
CINJECTHK_EXIT_CODES InjectHooks(PROCESS_INFORMATION pi, BOOL abLogProcess, LPCWSTR asConEmuHkDir /*= NULL*/)
{
	CINJECTHK_EXIT_CODES iRc = CIH_OK/*0*/;
	wchar_t szDllDir[MAX_PATH*2];
	_ASSERTE(ghOurModule!=NULL);
	BOOL is64bitOs = FALSE;
	int  ImageBits = 32; //-V112
#ifdef WIN64
	is64bitOs = TRUE;
#endif
	// для проверки IsWow64Process
	HMODULE hKernel = GetModuleHandle(L"kernel32.dll");
	HMODULE hNtDll = NULL;
	DEBUGTEST(int iFindAddress = 0);
	DWORD nErrCode = 0, nWait = 0;
	int SelfImageBits = WIN3264TEST(32,64);

	DWORDLONG const dwlConditionMask = VerSetConditionMask(VerSetConditionMask(0, VER_MAJORVERSION, VER_GREATER_EQUAL), VER_MINORVERSION, VER_GREATER_EQUAL);
	_ASSERTE(_WIN32_WINNT_WIN7==0x601);
	OSVERSIONINFOEXW osvi7 = {sizeof(osvi7), HIBYTE(_WIN32_WINNT_WIN7), LOBYTE(_WIN32_WINNT_WIN7)};
	BOOL bOsWin7 = _VerifyVersionInfo(&osvi7, VER_MAJORVERSION | VER_MINORVERSION, dwlConditionMask);

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

	if (asConEmuHkDir && *asConEmuHkDir)
	{
		lstrcpyn(szDllDir, asConEmuHkDir, countof(szDllDir));
	}
	else
	{
		wchar_t* pszSlash;
		if (!GetModuleFileName(ghOurModule, szDllDir, MAX_PATH))
		{
			#ifdef _DEBUG
			DWORD dwErr = GetLastError();
			_CrtDbgBreak();
			#endif
			//_printf("GetModuleFileName failed! ErrCode=0x%08X\n", dwErr);
			iRc = CIH_GetModuleFileName/*-501*/;
			goto wrap;
		}
		pszSlash = wcsrchr(szDllDir, L'\\');
		if (!pszSlash)
			pszSlash = szDllDir;
		*pszSlash = 0;
	}

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
		if ((LODWORD(hProcess) != (DWORD_PTR)hProcess) || (LODWORD(hThread) != (DWORD_PTR)hThread))
		{
			_ASSERTE((LODWORD(hProcess) == (DWORD_PTR)hProcess) && (LODWORD(hThread) == (DWORD_PTR)hThread));
			iRc = CIH_WrongHandleBitness/*-509*/;
			goto wrap;
		}
		#endif
		wchar_t sz64helper[MAX_PATH*2];
		msprintf(sz64helper, countof(sz64helper),
		          L"\"%s\\ConEmuC%s.exe\" /SETHOOKS=%X,%u,%X,%u",
		          szDllDir, (ImageBits==64) ? L"64" : L"",
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
			_ASSERTE(gfLoadLibrary.fnPtr!=NULL);
			iRc = CIH_GetLoadLibraryAddress/*-503*/;
			goto wrap;
		}
		else if (bOsWin7 && !GetLdrGetDllHandleByNameAddress() && !IsWine())
		{
			_ASSERTE(gfLdrGetDllHandleByName.fnPtr!=NULL);
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
			InjectHookFunctions fnArg = {gfLoadLibrary.module, gfLoadLibrary.fnPtr, gfLoadLibrary.szModule, gfLdrGetDllHandleByName.module, gfLdrGetDllHandleByName.fnPtr};
			iRc = InjectHookDLL(pi, &fnArg, ImageBits, szDllDir, &ptrAllocated, &nAllocated);

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
					L"", szInfo, L"", L"", NULL, NULL, NULL, NULL,
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

wrap:
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

		extern CESERVER_CONSOLE_APP_MAPPING* GetAppMapPtr();
		CESERVER_CONSOLE_APP_MAPPING* pAppMap = GetAppMapPtr();
		if (pAppMap)
		{
			pAppMap->HookedPids.AddValue(pi.dwProcessId);
		}
	}
	return iRc;
}
