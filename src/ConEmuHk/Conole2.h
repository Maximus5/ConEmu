
#pragma once

// This portion of code inherited from Console2

//////////////////////////////////////////////////////////////////////////////

int InjectHookDLL(PROCESS_INFORMATION pi, UINT_PTR fnLoadLibrary, int ImageBits/*32/64*/, LPCWSTR apszHookDllPath)
{
	int         iRc = -1000;
	CONTEXT		context;
	void*		mem				= NULL;
	size_t		memLen			= 0;
	size_t		codeSize;
	//BOOL		isWow64Process	= FALSE;
	BYTE* code = NULL;
	wchar_t strHookDllPath[MAX_PATH*2];
	DWORD dwErrCode = 0;
#ifdef _WIN64
	//WOW64_CONTEXT 	wow64Context;
	//DWORD			fnWow64LoadLibrary	= 0;

	//::ZeroMemory(&wow64Context, sizeof(WOW64_CONTEXT));
	//::IsWow64Process(pi.hProcess, &isWow64Process);
	//codeSize = /*isWow64Process*/ (ImageBits == 32) ? 20 : 91;

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
	//isWow64Process = TRUE; // 32 bit
#endif
	wcscpy_c(strHookDllPath, apszHookDllPath);
	//if (isWow64Process)
	//if (ImageBits == 32)
#ifndef _WIN64
	{
		// starting a 32-bit process
		wcscat_c(strHookDllPath, L"\\ConEmuHk.dll");
	}
#else
	{
		// starting a 64-bit process
		wcscat_c(strHookDllPath, L"\\ConEmuHk64.dll");
	}
#endif
	::ZeroMemory(&context, sizeof(CONTEXT));
	memLen = (lstrlen(strHookDllPath)+1)*sizeof(wchar_t);

	if (memLen > MAX_PATH*sizeof(wchar_t))
	{
		dwErrCode = GetLastError();
		iRc = -602;
		goto wrap;
	}

	code = (BYTE*)malloc(codeSize + (MAX_PATH*sizeof(wchar_t)));
	::CopyMemory(code + codeSize, strHookDllPath, memLen);
	memLen += codeSize;
#ifdef _WIN64
	//if (isWow64Process)
	//if (ImageBits == 32)
	//{
	//	wow64Context.ContextFlags = CONTEXT_FULL;
	//	if (!::Wow64GetThreadContext(pi.hThread, &wow64Context))
	//	{
	//		iRc = -711; dwErrCode = GetLastError();
	//		goto wrap;
	//	}
	//
	//	mem = ::VirtualAllocEx(pi.hProcess, NULL, memLen, MEM_COMMIT, PAGE_EXECUTE_READWRITE);
	//	if (mem == NULL)
	//	{
	//		iRc = -702; dwErrCode = GetLastError();
	//		goto wrap;
	//	}
	//
	//	// fnLoadLibrary - argument
	//	fnWow64LoadLibrary = (DWORD)fnLoadLibrary;
	//
	//	//// get 32-bit kernel32
	//	//wstring strConsoleWowPath(GetModulePath(NULL) + wstring(L"\\ConsoleWow.exe"));
	//	//
	//	//STARTUPINFO siWow;
	//	//::ZeroMemory(&siWow, sizeof(STARTUPINFO));
	//	//
	//	//siWow.cb			= sizeof(STARTUPINFO);
	//	//siWow.dwFlags		= STARTF_USESHOWWINDOW;
	//	//siWow.wShowWindow	= SW_HIDE;
	//	//
	//	//PROCESS_INFORMATION piWow;
	//	//
	//	//if (!::CreateProcess(
	//	//		NULL,
	//	//		const_cast<wchar_t*>(strConsoleWowPath.c_str()),
	//	//		NULL,
	//	//		NULL,
	//	//		FALSE,
	//	//		0,
	//	//		NULL,
	//	//		NULL,
	//	//		&siWow,
	//	//		&piWow))
	//	//{
	//	//	iRc = -603; dwErrCode = GetLastError();
	//	//	goto wrap;
	//	//}
	//	//
	//	//shared_ptr<void> wowProcess(piWow.hProcess, ::CloseHandle);
	//	//shared_ptr<void> wowThread(piWow.hThread, ::CloseHandle);
	//	//
	//	//if (::WaitForSingleObject(wowProcess.get(), 5000) == WAIT_TIMEOUT)
	//	//{
	//	//	iRc = -604; dwErrCode = GetLastError();
	//	//	goto wrap;
	//	//}
	//	//
	//	//::GetExitCodeProcess(wowProcess.get(), reinterpret_cast<DWORD*>(&fnWow64LoadLibrary));
	//}
	//else
	{
		context.ContextFlags = CONTEXT_FULL;
		::GetThreadContext(pi.hThread, &context);
		mem = ::VirtualAllocEx(pi.hProcess, NULL, memLen, MEM_COMMIT, PAGE_EXECUTE_READWRITE);

		if (mem == NULL)
		{
			dwErrCode = GetLastError();
			iRc = -701;
			goto wrap;
		}

		// fnLoadLibrary - argument
		//fnLoadLibrary = (UINT_PTR)::GetProcAddress(::GetModuleHandle(L"kernel32.dll"), "LoadLibraryW");
	}
#else
	context.ContextFlags = CONTEXT_FULL;

	if (!::GetThreadContext(pi.hThread, &context))
	{
		dwErrCode = GetLastError();
		iRc = -710;
		goto wrap;
	}

	mem = ::VirtualAllocEx(pi.hProcess, NULL, memLen, MEM_COMMIT, PAGE_EXECUTE_READWRITE);

	if (!mem)
	{
		dwErrCode = GetLastError();
		iRc = -703;
		goto wrap;
	}

	// fnLoadLibrary - argument
	//fnLoadLibrary = (UINT_PTR)::GetProcAddress(::GetModuleHandle(L"kernel32.dll"), "LoadLibraryW");
#endif
	union
	{
		PBYTE  pB;
		PINT   pI;
		PULONGLONG pL;
	} ip;
	ip.pB = code;
#ifdef _WIN64
	//if (isWow64Process)
	//{
	//	*ip.pB++ = 0x68;			// push  eip
	//	*ip.pI++ = wow64Context.Eip;
	//	*ip.pB++ = 0x9c;			// pushf
	//	*ip.pB++ = 0x60;			// pusha
	//	*ip.pB++ = 0x68;			// push  "path\to\our.dll"
	//	*ip.pI++ = (DWORD)mem + codeSize;
	//	*ip.pB++ = 0xe8;			// call  LoadLibraryW
	//	*ip.pI++ = (DWORD)fnWow64LoadLibrary - (DWORD)(mem + (ip.pB+4 - code));
	//	*ip.pB++ = 0x61;			// popa
	//	*ip.pB++ = 0x9d;			// popf
	//	*ip.pB++ = 0xc3;			// ret
	//
	//	if (!::WriteProcessMemory(pi.hProcess, mem, code, memLen, NULL))
	//	{
	//		iRc = -720; dwErrCode = GetLastError();
	//		goto wrap;
	//	}
	//	if (!::FlushInstructionCache(pi.hProcess, mem, memLen))
	//	{
	//		iRc = -721; dwErrCode = GetLastError();
	//		goto wrap;
	//	}
	//	wow64Context.Eip = (DWORD)mem;
	//	if (!::Wow64SetThreadContext(pi.hThread, &wow64Context))
	//	{
	//		iRc = -722; dwErrCode = GetLastError();
	//		goto wrap;
	//	}
	//}
	//else
	{
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

		context.Rip = (UINT_PTR)mem + 16;

		if (!::SetThreadContext(pi.hThread, &context))
		{
			dwErrCode = GetLastError();
			iRc = -732;
			goto wrap;
		}
	}
#else
	*ip.pB++ = 0x68;			// push  eip
	*ip.pI++ = context.Eip;
	*ip.pB++ = 0x9c;			// pushf
	*ip.pB++ = 0x60;			// pusha
	*ip.pB++ = 0x68;			// push  "path\to\our.dll"
	*ip.pI++ = (UINT_PTR)mem + codeSize;
	*ip.pB++ = 0xe8;			// call  LoadLibraryW
	*ip.pI++ = (UINT_PTR)fnLoadLibrary - ((UINT_PTR)mem + (ip.pB+4 - code));
	*ip.pB++ = 0x61;			// popa
	*ip.pB++ = 0x9d;			// popf
	*ip.pB++ = 0xc3;			// ret

	if (!::WriteProcessMemory(pi.hProcess, mem, code, memLen, NULL))
	{
		dwErrCode = GetLastError();
		iRc = -740;
		goto wrap;
	}

	if (!::FlushInstructionCache(pi.hProcess, mem, memLen))
	{
		dwErrCode = GetLastError();
		iRc = -741;
		goto wrap;
	}

	context.Eip = (UINT_PTR)mem;

	if (!::SetThreadContext(pi.hThread, &context))
	{
		dwErrCode = GetLastError();
		iRc = -742;
		goto wrap;
	}

#endif
	iRc = 0; // OK
wrap:

	if (code != NULL)
		free(code);

	return iRc;
}
