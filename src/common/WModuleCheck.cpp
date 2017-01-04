
/*
Copyright (c) 2009-2017 Maximus5
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

#include "WModuleCheck.h"



// Проверить, валиден ли модуль?
bool IsModuleValid(HMODULE module, BOOL abTestVirtual /*= TRUE*/)
{
	if ((module == NULL) || (module == INVALID_HANDLE_VALUE))
		return false;
	if (LDR_IS_RESOURCE(module))
		return false;

	bool lbValid = true;
	#ifdef USE_SEH
	IMAGE_DOS_HEADER dos;
	IMAGE_NT_HEADERS nt;
	#endif

	static bool bSysInfoRetrieved = false;
	static SYSTEM_INFO si = {};
	if (!bSysInfoRetrieved)
	{
		GetSystemInfo(&si);
		bSysInfoRetrieved = true;
	}

	LPBYTE lpTest;
	SIZE_T cbCommitSize = max(max(4096,sizeof(IMAGE_DOS_HEADER)),si.dwPageSize);

	// If module is hooked by ConEmuHk, we get excess debug "assertion" from ConEmu.dll (Far plugin)
	if (abTestVirtual)
	{
		// Issue 881
		lpTest = (LPBYTE)VirtualAlloc((LPVOID)module, cbCommitSize, MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);
		if (lpTest)
		{
			// If we can lock mem region with (IMAGE_DOS_HEADER) of checking module
			if ((lpTest <= (LPBYTE)module) && ((lpTest + cbCommitSize) >= (((LPBYTE)module) + sizeof(IMAGE_DOS_HEADER))))
			{
				// That means, it was unloaded
				lbValid = false;
			}
			VirtualFree(lpTest, 0, MEM_RELEASE);

			if (!lbValid)
				goto wrap;
		}
		else
		{
			// Memory is used (supposing by module)
		}
	}

#ifdef USE_SEH
	SAFETRY
	{
		memmove(&dos, (void*)module, sizeof(dos));
		if (dos.e_magic != IMAGE_DOS_SIGNATURE /*'ZM'*/)
		{
			lbValid = false;
		}
		else
		{
			memmove(&nt, (IMAGE_NT_HEADERS*)((char*)module + ((IMAGE_DOS_HEADER*)module)->e_lfanew), sizeof(nt));
			if (nt.Signature != 0x004550)
				lbValid = false;
		}
	}
	SAFECATCH
	{
		lbValid = false;
	}

#else
	if (IsBadReadPtr((void*)module, sizeof(IMAGE_DOS_HEADER)))
	{
		lbValid = false;
	}
	else if (((IMAGE_DOS_HEADER*)module)->e_magic != IMAGE_DOS_SIGNATURE /*'ZM'*/)
	{
		lbValid = false;
	}
	else
	{
		IMAGE_NT_HEADERS* nt_header = (IMAGE_NT_HEADERS*)((char*)module + ((IMAGE_DOS_HEADER*)module)->e_lfanew);
		if (IsBadReadPtr(nt_header, sizeof(IMAGE_NT_HEADERS)))
		{
			lbValid = false;
		}
		else if (nt_header->Signature != 0x004550)
		{
			return false;
		}
	}
#endif

wrap:
	return lbValid;
}

bool CheckCallbackPtr(HMODULE hModule, size_t ProcCount, FARPROC* CallBack, BOOL abCheckModuleInfo, BOOL abAllowNTDLL, BOOL abTestVirtual /*= TRUE*/)
{
	if ((hModule == NULL) || (hModule == INVALID_HANDLE_VALUE) || LDR_IS_RESOURCE(hModule))
	{
		_ASSERTE((hModule != NULL) && (hModule != INVALID_HANDLE_VALUE) && !LDR_IS_RESOURCE(hModule));
		return false;
	}
	if (!CallBack || !ProcCount)
	{
		_ASSERTE(CallBack && ProcCount);
		return false;
	}

	DWORD_PTR nModulePtr = (DWORD_PTR)hModule;
	DWORD_PTR nModuleSize = (4<<20);
	//BOOL lbModuleInformation = FALSE;

	DWORD_PTR nModulePtr2 = 0;
	DWORD_PTR nModuleSize2 = 0;
	if (abAllowNTDLL)
	{
		nModulePtr2 = (DWORD_PTR)GetModuleHandle(L"ntdll.dll");
		nModuleSize2 = (4<<20);
	}

	// Если разрешили - попробовать определить размер модуля, чтобы CallBack не выпал из его тела
	if (abCheckModuleInfo)
	{
		if (!IsModuleValid(hModule, abTestVirtual))
		{
			_ASSERTE("!IsModuleValid(hModule)" && 0);
			return false;
		}

		IMAGE_NT_HEADERS* nt_header = (IMAGE_NT_HEADERS*)((char*)hModule + ((IMAGE_DOS_HEADER*)hModule)->e_lfanew);

		// Получить размер модуля из OptionalHeader
		nModuleSize = nt_header->OptionalHeader.SizeOfImage;

		if (nModulePtr2)
		{
			nt_header = (IMAGE_NT_HEADERS*)((char*)nModulePtr2 + ((IMAGE_DOS_HEADER*)nModulePtr2)->e_lfanew);
			nModuleSize2 = nt_header->OptionalHeader.SizeOfImage;
		}
	}

	for (size_t i = 0; i < ProcCount; i++)
	{
		if (!(CallBack[i]))
		{
			_ASSERTE((CallBack[i])!=NULL);
			return false;
		}

		if ((((DWORD_PTR)(CallBack[i])) < nModulePtr)
			&& (!nModulePtr2 || (((DWORD_PTR)(CallBack[i])) < nModulePtr2)))
		{
			_ASSERTE(((DWORD_PTR)(CallBack[i])) >= nModulePtr);
			return false;
		}

		if ((((DWORD_PTR)(CallBack[i])) > (nModuleSize + nModulePtr))
			&& (!nModulePtr2 || (((DWORD_PTR)(CallBack[i])) > (nModuleSize2 + nModulePtr2))))
		{
			_ASSERTE(((DWORD_PTR)(CallBack[i])) <= (nModuleSize + nModulePtr));
			return false;
		}
	}

	return true;
}
