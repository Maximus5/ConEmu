
#pragma once

#ifdef _DEBUG
//#define CONEMUHK_BEEP_ON_THREAD_START
#undef CONEMUHK_BEEP_ON_THREAD_START
#else
#undef CONEMUHK_BEEP_ON_THREAD_START
#endif

// This portion of code inherited from Console2

//////////////////////////////////////////////////////////////////////////////

extern HANDLE ghSkipSetThreadContextForThread;

struct InjectHookFunctions
{
	HMODULE  hKernel;
	UINT_PTR fnLoadLibrary;
	LPCWSTR  szKernelName;
	// Win7+
	HMODULE  hNtDll;
	UINT_PTR fnLdrGetDllHandleByName;
};

// Use WriteProcessMemory and SetThreadContext
int InjectHookDLL(PROCESS_INFORMATION pi, InjectHookFunctions* pfn /*UINT_PTR fnLoadLibrary*/, int ImageBits/*32/64*/, LPCWSTR apszHookDllPath, DWORD_PTR* ptrAllocated, DWORD* pnAllocated)
{
	CINJECTHK_EXIT_CODES iRc = CIH_AsmGeneralError/*-1000*/;
	CONTEXT		context = {};
	void*		mem		 = NULL;
	size_t		memLen	 = 0;
	size_t		codeSize = 0;
	size_t		cbTotal  = 0;
	BYTE* 		code	 = NULL;
	wchar_t 	strHookDllPath[MAX_PATH*2] = {};
	DWORD 		dwErrCode = 0;
#ifndef _WIN64
	// starting a 32-bit process
	LPCWSTR		pszDllName = L"\\ConEmuHk.dll";
#else
	// starting a 64-bit process
	LPCWSTR		pszDllName = L"\\ConEmuHk64.dll";
	DWORD_PTR   nLoadLibraryProcShift;
#endif


	// Placeholder for "kernel32.dll" or "kernelbase.dll"
	typedef struct _UNICODE_STRING {
		USHORT Length;
		USHORT MaximumLength;
		#ifdef _WIN64
		DWORD  Pad;
		#endif
		PWSTR  Buffer;
	} USTR, *PUSTR;

	PUSTR pStr = NULL;

	_ASSERTE(pfn->szKernelName && *pfn->szKernelName);
	size_t pnKernelNameLen = lstrlen(pfn->szKernelName);
	size_t pstrSize = sizeof(USTR) + 8/*alignment*/ + sizeof(wchar_t)*(pnKernelNameLen+1); // UNICODE_STRING ( "kernel32.dll" | "kernelbase.dll" )


	//OSVERSIONINFO osv = {sizeof(osv)};
	//GetVersionEx(&osv);
	//DWORD nOsVer = (osv.dwMajorVersion << 8) | (osv.dwMinorVersion & 0xFF);


	if (ptrAllocated)
		*ptrAllocated = 0;
	if (pnAllocated)
		*pnAllocated = 0;

#ifdef _WIN64
	if (ImageBits != 64)
	{
		_ASSERTE(ImageBits==64);
		dwErrCode = GetLastError();
		iRc = CIH_AsmBitnessNot64/*-801*/;
		goto wrap;
	}

	//UINT_PTR fnGetDllByName = 0;
	nLoadLibraryProcShift = 0;
	//HMODULE hNtDll, hKernel;
	//if (nOsVer >= 0x0602)
	if (pfn->fnLdrGetDllHandleByName)
	{
		//hNtDll = GetModuleHandle(L"ntdll.dll");
		//fnGetDllByName = (UINT_PTR)GetProcAddress(hNtDll, "LdrGetDllHandleByName");
		//CheckCallbackPtr(hNtDll, 1, (FARPROC*)&fnGetDllByName, TRUE);
		//hKernel = GetModuleHandle(L"kernel32.dll");
		//nProcShift = ((LPBYTE)fnLoadLibrary) - (LPBYTE)hKernel;
		nLoadLibraryProcShift = ((LPBYTE)pfn->fnLoadLibrary) - (LPBYTE)pfn->hKernel;
		if (nLoadLibraryProcShift != (DWORD)nLoadLibraryProcShift)
		{
			_ASSERTE(nLoadLibraryProcShift == (DWORD)nLoadLibraryProcShift);
			iRc = CIH_AsmBadProcShift/*-814*/;
			goto wrap;
		}
	}

	// Адрес пути к ConEmuHk64 нужно выровнять на 8 байт!
	codeSize = 136;
#else

	if (ImageBits != 32)
	{
		_ASSERTE(ImageBits==32);
		dwErrCode = GetLastError();
		iRc = CIH_AsmBitmessNot32/*-802*/;
		goto wrap;
	}

	codeSize = 20;
#endif

	#ifdef CONEMUHK_BEEP_ON_THREAD_START
	codeSize += 128;
	#endif

	if (!apszHookDllPath || (lstrlen(apszHookDllPath) >= (MAX_PATH - lstrlen(pszDllName) - 1)))
	{
		iRc = CIH_AsmBadDllPathName/*-803*/;
		goto wrap;
	}
	wcscpy_c(strHookDllPath, apszHookDllPath);
	wcscat_c(strHookDllPath, pszDllName);
	// Dll must exist!
	{
		HANDLE hLib = CreateFile(strHookDllPath, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, 0, NULL);
		if (!hLib || (hLib == INVALID_HANDLE_VALUE))
		{
			iRc = CIH_AsmDllNotFound/*-804*/;
			goto wrap;
		}
		CloseHandle(hLib);
	}

	memLen = (lstrlen(strHookDllPath)+1)*sizeof(wchar_t);

	if (memLen > MAX_PATH*sizeof(wchar_t))
	{
		dwErrCode = GetLastError();
		iRc = CIH_AsmMemBadSize/*-602*/;
		goto wrap;
	}

	code = (BYTE*)malloc(codeSize + memLen + pstrSize);
	memmove(code + codeSize, strHookDllPath, memLen);

	pStr = (PUSTR)((((DWORD_PTR)(code + codeSize + memLen + 7))>>3)<<3);
	pStr->Length = pnKernelNameLen*sizeof(wchar_t);
	pStr->MaximumLength = (pnKernelNameLen+1)*sizeof(wchar_t);
	#ifdef _WIN64
	pStr->Pad = 0;
	#endif
	pStr->Buffer = (PWSTR)(((LPBYTE)pStr)+sizeof(*pStr)); // адрес будет обновлен после VirtualAlloc
	memmove(pStr->Buffer, pfn->szKernelName, pStr->MaximumLength);

	cbTotal = codeSize + memLen + pstrSize;

	// Query current context of suspended process
	context.ContextFlags = CONTEXT_FULL;

	SetLastError(0);
	if (!::GetThreadContext(pi.hThread, &context))
	{
		dwErrCode = GetLastError();
		#ifdef _DEBUG
		#ifdef CONEMU_MINIMAL
			GuiMessageBox
		#else
			MessageBoxW
		#endif
			(NULL, L"GetThreadContext failed", L"Injects", MB_OK|MB_SYSTEMMODAL);
		#endif
		iRc = CIH_AsmGetThreadContext/*-710*/;
		goto wrap;
	}

	#ifdef _DEBUG
	// strHookDllPath уже скопирован, поэтому его можно заюзать для DebugString
	#ifdef _WIN64
	msprintf(strHookDllPath, countof(strHookDllPath),
		L"GetThreadContext(x64) for PID=%u: ContextFlags=0x%08X, Rip=0x%08X%08X\n",
		pi.dwProcessId,
		(DWORD)context.ContextFlags, (DWORD)(context.Rip>>32), (DWORD)(context.Rip & 0xFFFFFFFF)); //-V112
	#else
	msprintf(strHookDllPath, countof(strHookDllPath),
		L"GetThreadContext(x86) for PID=%u: ContextFlags=0x%08X, Eip=0x%08X\n",
		pi.dwProcessId,
		(DWORD)context.ContextFlags, (DWORD)context.Eip);
	#endif
	OutputDebugString(strHookDllPath);
	#endif

	//mem = ::VirtualAllocEx(pi.hProcess, NULL, cbTotal, MEM_COMMIT, PAGE_EXECUTE_READWRITE);
	//#ifdef _WIN64
	//if (!mem) -- не работает, нужен NULL
	//	mem = ::VirtualAllocEx(pi.hProcess, (LPVOID)0x6FFFFF0000000000, cbTotal, MEM_COMMIT, PAGE_EXECUTE_READWRITE);
	//#endif
	//if (!mem) -- не работает, нужен NULL
	//	mem = ::VirtualAllocEx(pi.hProcess, (LPVOID)0x6FFFFF00, cbTotal, MEM_COMMIT, PAGE_EXECUTE_READWRITE);
	//if (!mem)
	// MEM_TOP_DOWN - память выделяется в верхних адресах, разницы в работе не заметил
	mem = ::VirtualAllocEx(pi.hProcess, NULL, cbTotal,
			MEM_COMMIT|MEM_RESERVE/*|MEM_TOP_DOWN*/, PAGE_EXECUTE_READWRITE|PAGE_NOCACHE);

	if (!mem)
	{
		dwErrCode = GetLastError();
		iRc = CIH_AsmVirtualAllocEx/*-703*/;
		goto wrap;
	}

	pStr->Buffer = (PWSTR)(((LPBYTE)mem) + (((LPBYTE)pStr->Buffer) - code));

	#ifdef _DEBUG
	// strHookDllPath уже скопирован, поэтому его можно заюзать для DebugString
	#ifdef _WIN64
	msprintf(strHookDllPath, countof(strHookDllPath),
		L"VirtualAllocEx(x64) for PID=%u: 0x%08X%08X\n",
		pi.dwProcessId,
		(DWORD)(((DWORD_PTR)mem)>>32), (DWORD)(((DWORD_PTR)mem) & 0xFFFFFFFF)); //-V112
	#else
	msprintf(strHookDllPath, countof(strHookDllPath),
		L"VirtualAllocEx(x86) for PID=%u: 0x%08X\n",
		pi.dwProcessId,
		(DWORD)mem);
	#endif
	OutputDebugString(strHookDllPath);
	#endif



	union
	{
		PBYTE  pB; //-V117
		PINT   pI; //-V117
		PULONGLONG pL; //-V117
	} ip;
	ip.pB = code;
#ifdef _WIN64
	*ip.pL++ = context.Rip;          // адрес возврата
	*ip.pL++ = pfn->fnLdrGetDllHandleByName ? pfn->fnLdrGetDllHandleByName : pfn->fnLoadLibrary;        // адрес вызываемой процедуры
	#ifdef CONEMUHK_BEEP_ON_THREAD_START
	*ip.pL++ = (ULONGLONG)Beep;
	#endif
	*ip.pB++ = 0x9C;					// pushfq
	*ip.pB++ = 0x50;					// push  rax
	*ip.pB++ = 0x51;					// push  rcx
	*ip.pB++ = 0x52;					// push  rdx
	*ip.pB++ = 0x53;					// push  rbx
	*ip.pB++ = 0x55;					// push  rbp
	*ip.pB++ = 0x56;					// push  rsi
	*ip.pB++ = 0x57;					// push  rdi
	*ip.pB++ = 0x41; *ip.pB++ = 0x50;	// push  r8
	*ip.pB++ = 0x41; *ip.pB++ = 0x51;	// push  r9
	*ip.pB++ = 0x41; *ip.pB++ = 0x52;	// push  r10
	*ip.pB++ = 0x41; *ip.pB++ = 0x53;	// push  r11
	*ip.pB++ = 0x41; *ip.pB++ = 0x54;	// push  r12
	*ip.pB++ = 0x41; *ip.pB++ = 0x55;	// push  r13
	*ip.pB++ = 0x41; *ip.pB++ = 0x56;	// push  r14
	*ip.pB++ = 0x41; *ip.pB++ = 0x57;	// push  r15
	*ip.pB++ = 0x48;					// sub   rsp, 40
	*ip.pB++ = 0x83;
	*ip.pB++ = 0xEC;
	*ip.pB++ = 0x28;

	#ifdef CONEMUHK_BEEP_ON_THREAD_START
	*ip.pB++ = 0xba;					// mov         edx,3E8h // Dur
	*ip.pI++ = 1000;
	*ip.pB++ = 0xb9;					// mov         ecx,4B0h // Freq
	*ip.pI++ = 1200;
	*ip.pB++ = 0xFF;					// call  Beep
	*ip.pB++ = 0x15;
	*ip.pI   = -(int)(ip.pB + 4 - code - 16);  ip.pI++; // -- указатель на адрес процедуры // GCC do the INC before rvalue eval
	#endif

	// Due to ASLR of Kernel32.dll in Windows 8 RC x64 we need this workaround
	// JIC expanded to Windows 7 too.
	if (pfn->fnLdrGetDllHandleByName)
	{
	*ip.pB++ = 0x4C;					// lea	       r8,&ptrProc
	*ip.pB++ = 0x8D;
	*ip.pB++ = 0x05;
	*ip.pI   = -(int)(ip.pB - code + 4 - 8);  ip.pI++;  // -- указатель на адрес процедуры (code+8) [OUT]  // GCC do the INC before rvalue eval
	*ip.pB++ = 0x33;                    // xor         rdx,rdx
	*ip.pB++ = 0xD2;
	*ip.pB++ = 0x48;                    // lea         rcx,&UNICODE_STRING
	*ip.pB++ = 0x8D;
	*ip.pB++ = 0x0D;
	*ip.pI   = (int)(((LPBYTE)pStr) - ip.pB - 4);  ip.pI++;  // &UNICODE_STRING  // GCC do the INC before rvalue eval
	*ip.pB++ = 0xFF;					// call  LdrGetDllHandleByName
	*ip.pB++ = 0x15;
	*ip.pI   = -(int)(ip.pB - code + 4 - 8);  ip.pI++;  // -- указатель на адрес процедуры (code+8)  // GCC do the INC before rvalue eval
	//
	*ip.pB++ = 0x48;                    // mov         rax,&ProcAddress
	*ip.pB++ = 0x8B;
	*ip.pB++ = 0x05;
	*ip.pI   = -(int)(ip.pB - code + 4 - 8);  ip.pI++;  // -- указатель на адрес процедуры (code+8)  // GCC do the INC before rvalue eval
	*ip.pB++ = 0x48;                    // add         rax,nProcShift
	*ip.pB++ = 0x05;
	*ip.pI++ = (DWORD)nLoadLibraryProcShift;
	*ip.pB++ = 0x48;                    // mov         &ProcAddress,rax
	*ip.pB++ = 0x89;
	*ip.pB++ = 0x05;
	*ip.pI   = -(int)(ip.pB - code + 4 - 8);  ip.pI++;  // -- указатель на адрес процедуры (code+8)  // GCC do the INC before rvalue eval
	}

	#ifdef CONEMUHK_BEEP_ON_THREAD_START
	*ip.pB++ = 0xba;					// mov         edx,3E8h // Dur
	*ip.pI++ = 1000;
	*ip.pB++ = 0xb9;					// mov         ecx,4B0h // Freq
	*ip.pI++ = 1800;
	*ip.pB++ = 0xFF;					// call  Beep
	*ip.pB++ = 0x15;
	*ip.pI   = -(int)(ip.pB + 4 - code - 16);  ip.pI++; // -- указатель на адрес процедуры // GCC do the INC before rvalue eval
	#endif

	*ip.pB++ = 0x48;					// lea	 rcx, "path\to\our.dll"
	*ip.pB++ = 0x8D;
	*ip.pB++ = 0x0D;
	*ip.pI   = (int)(codeSize + code - ip.pB - 4);  ip.pI++;  // 45; -- указатель на "path\to\our.dll"  // GCC do the INC before rvalue eval

	*ip.pB++ = 0xFF;					// call  LoadLibraryW
	*ip.pB++ = 0x15;
	*ip.pI   = -(int)(ip.pB - code + 4 - 8);  ip.pI++;  // -49; -- указатель на адрес процедуры (code+8)  // GCC do the INC before rvalue eval

	#ifdef CONEMUHK_BEEP_ON_THREAD_START
	*ip.pB++ = 0xba;					// mov         edx,3E8h // Dur
	*ip.pI++ = 1000;
	*ip.pB++ = 0xb9;					// mov         ecx,4B0h // Freq
	*ip.pI++ = 2400;
	*ip.pB++ = 0xFF;					// call  Beep
	*ip.pB++ = 0x15;
	*ip.pI   = -(int)(ip.pB + 4 - code - 16);  ip.pI++; // -- указатель на адрес процедуры // GCC do the INC before rvalue eval
	#endif

	*ip.pB++ = 0x48;					// add   rsp, 40
	*ip.pB++ = 0x83;
	*ip.pB++ = 0xC4;
	*ip.pB++ = 0x28;
	*ip.pB++ = 0x41; *ip.pB++ = 0x5F;	// pop   r15
	*ip.pB++ = 0x41; *ip.pB++ = 0x5E;	// pop   r14
	*ip.pB++ = 0x41; *ip.pB++ = 0x5D;	// pop   r13
	*ip.pB++ = 0x41; *ip.pB++ = 0x5C;	// pop   r12
	*ip.pB++ = 0x41; *ip.pB++ = 0x5B;	// pop   r11
	*ip.pB++ = 0x41; *ip.pB++ = 0x5A;	// pop   r10
	*ip.pB++ = 0x41; *ip.pB++ = 0x59;	// pop   r9
	*ip.pB++ = 0x41; *ip.pB++ = 0x58;	// pop   r8
	*ip.pB++ = 0x5F;					// pop	 rdi
	*ip.pB++ = 0x5E;					// pop	 rsi
	*ip.pB++ = 0x5D;					// pop	 rbp
	*ip.pB++ = 0x5B;					// pop	 rbx
	*ip.pB++ = 0x5A;					// pop	 rdx
	*ip.pB++ = 0x59;					// pop	 rcx
	*ip.pB++ = 0x58;					// pop	 rax
	*ip.pB++ = 0x9D;					// popfq
	*ip.pB++ = 0xff;					// jmp	 Rip
	*ip.pB++ = 0x25;
	*ip.pI   = -(int)(ip.pB - code + 4);  ip.pI++;  // -91;  // GCC do the INC before rvalue eval
	/* 0x5B */

	context.Rip = (UINT_PTR)mem + 16;	// начало (иструкция pushfq)
	#ifdef CONEMUHK_BEEP_ON_THREAD_START
	context.Rip += 8;
	#endif
#else
	*ip.pB++ = 0x68;			// push  eip
	*ip.pI++ = context.Eip;
	*ip.pB++ = 0x9c;			// pushf
	*ip.pB++ = 0x60;			// pusha
	*ip.pB++ = 0x68;			// push  "path\to\our.dll"
	*ip.pI++ = (UINT_PTR)mem + codeSize;
	*ip.pB++ = 0xe8;			// call  LoadLibraryW
	TODO("???");
	*ip.pI   = (UINT_PTR)pfn->fnLoadLibrary - ((UINT_PTR)mem + (ip.pB+4 - code));  ip.pI++;  // GCC do the INC before rvalue eval
	*ip.pB++ = 0x61;			// popa
	*ip.pB++ = 0x9d;			// popf
	*ip.pB++ = 0xc3;			// ret

	context.Eip = (UINT_PTR)mem;
#endif
	if ((INT_PTR)(ip.pB - code) > (INT_PTR)codeSize)
	{
		_ASSERTE((INT_PTR)(ip.pB - code) == (INT_PTR)codeSize);
		iRc = CIH_AsmBadCodePointer/*-601*/;
		goto wrap;
	}

	if (!::WriteProcessMemory(pi.hProcess, mem, code, cbTotal, NULL))
	{
		dwErrCode = GetLastError();
		iRc = CIH_AsmWriteProcessMemory/*-730*/;
		goto wrap;
	}

	if (!::FlushInstructionCache(pi.hProcess, mem, cbTotal))
	{
		dwErrCode = GetLastError();
		iRc = CIH_AsmFlushInstructionCode/*-731*/;
		goto wrap;
	}

	SetLastError(0);
	if (!::SetThreadContext(pi.hThread, &context))
	{
		dwErrCode = GetLastError();
		#ifdef _DEBUG
		CONTEXT context2; ::ZeroMemory(&context2, sizeof(context2));
		context2.ContextFlags = CONTEXT_FULL;
		::GetThreadContext(pi.hThread, &context2);
		#ifdef CONEMU_MINIMAL
			GuiMessageBox
		#else
			MessageBoxW
		#endif
			(NULL, L"SetThreadContext failed", L"Injects", MB_OK|MB_SYSTEMMODAL);
		#endif
		iRc = CIH_AsmSetThreadContext/*-732*/;
		goto wrap;
	}

	ghSkipSetThreadContextForThread = pi.hThread;

	#ifdef _DEBUG
	// strHookDllPath уже скопирован, поэтому его можно заюзать для DebugString
	#ifdef _WIN64
	msprintf(strHookDllPath, countof(strHookDllPath),
		L"SetThreadContext(x64) for PID=%u: ContextFlags=0x%08X, Rip=0x%08X%08X\n",
		pi.dwProcessId,
		(DWORD)context.ContextFlags, (DWORD)(context.Rip>>32), (DWORD)(context.Rip & 0xFFFFFFFF)); //-V112
	#else
	msprintf(strHookDllPath, countof(strHookDllPath),
		L"SetThreadContext(x86) for PID=%u: ContextFlags=0x%08X, Eip=0x%08X\n",
		pi.dwProcessId,
		(DWORD)context.ContextFlags, (DWORD)context.Eip);
	#endif
	OutputDebugString(strHookDllPath);
	#endif

	if (ptrAllocated)
		*ptrAllocated = (DWORD_PTR)mem;
	if (pnAllocated)
		*pnAllocated = (DWORD)cbTotal;

	iRc = CIH_OK/*0*/; // OK
wrap:

	if (code != NULL)
		free(code);

#ifdef _DEBUG
	if (iRc != CIH_OK/*0*/)
	{
		// Хуки не получится установить для некоторых системных процессов типа ntvdm.exe,
		// но при запуске dos приложений мы сюда дойти не должны
		_ASSERTE(iRc == CIH_OK/*0*/);
	}
#endif

	UNREFERENCED_PARAMETER(dwErrCode);

	return iRc;
}
