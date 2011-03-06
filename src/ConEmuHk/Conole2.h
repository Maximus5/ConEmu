
#pragma once

// This portion of code inherited from Console2

//////////////////////////////////////////////////////////////////////////////

int InjectHookDLL(PROCESS_INFORMATION pi, UINT_PTR fnLoadLibrary, int ImageBits/*32/64*/, LPCWSTR apszHookDllPath, DWORD_PTR* ptrAllocated, DWORD* pnAllocated)
{
	int         iRc = -1000;
	CONTEXT		context; memset(&context, 0, sizeof(context));
	void*		mem		 = NULL;
	size_t		memLen	 = 0;
	size_t		codeSize = 0;
	BYTE* 		code	 = NULL;
	wchar_t 	strHookDllPath[MAX_PATH*2];
	DWORD 		dwErrCode = 0;
#ifndef _WIN64
	// starting a 32-bit process
	LPCWSTR		pszDllName = L"\\ConEmuHk.dll";
#else
	// starting a 64-bit process
	LPCWSTR		pszDllName = L"\\ConEmuHk64.dll";
#endif

	if (ptrAllocated)
		*ptrAllocated = NULL;
	if (pnAllocated)
		*pnAllocated = 0;

#ifdef _WIN64
	if (ImageBits != 64)
	{
		_ASSERTE(ImageBits==64);
		dwErrCode = GetLastError();
		iRc = -801;
		goto wrap;
	}

	codeSize = 91;
#else

	if (ImageBits != 32)
	{
		_ASSERTE(ImageBits==32);
		dwErrCode = GetLastError();
		iRc = -802;
		goto wrap;
	}

	codeSize = 20;
#endif

	if (!apszHookDllPath || (lstrlen(apszHookDllPath) >= (MAX_PATH - lstrlen(pszDllName) - 1)))
	{
		iRc = -803;
		goto wrap;
	}
	wcscpy_c(strHookDllPath, apszHookDllPath);
	wcscat_c(strHookDllPath, pszDllName);

	memLen = (lstrlen(strHookDllPath)+1)*sizeof(wchar_t);

	if (memLen > MAX_PATH*sizeof(wchar_t))
	{
		dwErrCode = GetLastError();
		iRc = -602;
		goto wrap;
	}

	code = (BYTE*)malloc(codeSize + memLen);
	memmove(code + codeSize, strHookDllPath, memLen);
	memLen += codeSize;

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
		iRc = -710;
		goto wrap;
	}
	// strHookDllPath уже скопирован, поэтому его можно заюзать для DebugString
	#ifdef _WIN64
	msprintf(strHookDllPath, countof(strHookDllPath),
		L"GetThreadContext(x64) for PID=%u: ContextFlags=0x%08X, Rip=0x%08X%08X\n",
		pi.dwProcessId,
		(DWORD)context.ContextFlags, (DWORD)(context.Rip>>32), (DWORD)(context.Rip & 0xFFFFFFFF));
	#else
	msprintf(strHookDllPath, countof(strHookDllPath),
		L"GetThreadContext(x86) for PID=%u: ContextFlags=0x%08X, Eip=0x%08X\n",
		pi.dwProcessId,
		(DWORD)context.ContextFlags, (DWORD)context.Eip);
	#endif
	OutputDebugString(strHookDllPath);

	//mem = ::VirtualAllocEx(pi.hProcess, NULL, memLen, MEM_COMMIT, PAGE_EXECUTE_READWRITE);
	//#ifdef _WIN64
	//if (!mem) -- не работает, нужен NULL
	//	mem = ::VirtualAllocEx(pi.hProcess, (LPVOID)0x6FFFFF0000000000, memLen, MEM_COMMIT, PAGE_EXECUTE_READWRITE);
	//#endif
	//if (!mem) -- не работает, нужен NULL
	//	mem = ::VirtualAllocEx(pi.hProcess, (LPVOID)0x6FFFFF00, memLen, MEM_COMMIT, PAGE_EXECUTE_READWRITE);
	//if (!mem)
	// MEM_TOP_DOWN - память выделяется в верхних адресах, разницы в работе не заметил
	mem = ::VirtualAllocEx(pi.hProcess, NULL, memLen, 
			MEM_COMMIT|MEM_RESERVE/*|MEM_TOP_DOWN*/, PAGE_EXECUTE_READWRITE|PAGE_NOCACHE);

	if (!mem)
	{
		dwErrCode = GetLastError();
		iRc = -703;
		goto wrap;
	}

	// strHookDllPath уже скопирован, поэтому его можно заюзать для DebugString
	#ifdef _WIN64
	msprintf(strHookDllPath, countof(strHookDllPath),
		L"VirtualAllocEx(x64) for PID=%u: 0x%08X%08X\n",
		pi.dwProcessId,
		(DWORD)(((DWORD_PTR)mem)>>32), (DWORD)(((DWORD_PTR)mem) & 0xFFFFFFFF));
	#else
	msprintf(strHookDllPath, countof(strHookDllPath),
		L"VirtualAllocEx(x86) for PID=%u: 0x%08X\n",
		pi.dwProcessId,
		(DWORD)mem);
	#endif
	OutputDebugString(strHookDllPath);


	union
	{
		PBYTE  pB;
		PINT   pI;
		PULONGLONG pL;
	} ip;
	ip.pB = code;
#ifdef _WIN64
	*ip.pL++ = context.Rip;
	*ip.pL++ = fnLoadLibrary;
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
	*ip.pB++ = 0x48;					// lea	 ecx, "path\to\our.dll"
	*ip.pB++ = 0x8D;
	*ip.pB++ = 0x0D;
	*ip.pI++ = 40;
	*ip.pB++ = 0xFF;					// call  LoadLibraryW
	*ip.pB++ = 0x15;
	*ip.pI++ = -49;
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
	*ip.pI++ = -91;

	context.Rip = (UINT_PTR)mem + 16;	// начало (иструкция pushfq)
#else
	*ip.pB++ = 0x68;			// push  eip
	*ip.pI++ = context.Eip;
	*ip.pB++ = 0x9c;			// pushf
	*ip.pB++ = 0x60;			// pusha
	*ip.pB++ = 0x68;			// push  "path\to\our.dll"
	*ip.pI++ = (UINT_PTR)mem + codeSize;
	*ip.pB++ = 0xe8;			// call  LoadLibraryW
	TODO("???");
	*ip.pI++ = (UINT_PTR)fnLoadLibrary - ((UINT_PTR)mem + (ip.pB+4 - code));
	*ip.pB++ = 0x61;			// popa
	*ip.pB++ = 0x9d;			// popf
	*ip.pB++ = 0xc3;			// ret

	context.Eip = (UINT_PTR)mem;
#endif
	if ((INT_PTR)(ip.pB - code) > (INT_PTR)codeSize)
	{
		_ASSERTE((INT_PTR)(ip.pB - code) == (INT_PTR)codeSize);
		iRc = -601;
		goto wrap;
	}

	if (!::WriteProcessMemory(pi.hProcess, mem, code, memLen, NULL))
	{
		dwErrCode = GetLastError();
		iRc = -730;
		goto wrap;
	}

	if (!::FlushInstructionCache(pi.hProcess, mem, memLen))
	{
		dwErrCode = GetLastError();
		iRc = -731;
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
		iRc = -732;
		goto wrap;
	}

	// strHookDllPath уже скопирован, поэтому его можно заюзать для DebugString
	#ifdef _WIN64
	msprintf(strHookDllPath, countof(strHookDllPath),
		L"SetThreadContext(x64) for PID=%u: ContextFlags=0x%08X, Rip=0x%08X%08X\n",
		pi.dwProcessId,
		(DWORD)context.ContextFlags, (DWORD)(context.Rip>>32), (DWORD)(context.Rip & 0xFFFFFFFF));
	#else
	msprintf(strHookDllPath, countof(strHookDllPath),
		L"SetThreadContext(x86) for PID=%u: ContextFlags=0x%08X, Eip=0x%08X\n",
		pi.dwProcessId,
		(DWORD)context.ContextFlags, (DWORD)context.Eip);
	#endif
	OutputDebugString(strHookDllPath);

	if (ptrAllocated)
		*ptrAllocated = (DWORD_PTR)mem;
	if (pnAllocated)
		*pnAllocated = (DWORD)memLen;

	iRc = 0; // OK
wrap:

	if (code != NULL)
		free(code);
		
#ifdef _DEBUG
	if (iRc != 0)
	{
		// Хуки не получится установить для некоторых системных процессов типа ntvdm.exe,
		// но при запуске dos приложений мы сюда дойти не должны
		_ASSERTE(iRc == 0);
	}
#endif

	return iRc;
}
