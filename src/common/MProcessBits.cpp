
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
#include <windows.h>
#include "MAssert.h"
#include "MModule.h"
#include "MProcessBits.h"
#include "WObjects.h"

/// Check running process bitness - 32/64
int GetProcessBits(DWORD nPID, HANDLE hProcess /*= NULL*/)
{
	if (!IsWindows64())
		return 32;

	int ImageBits = WIN3264TEST(32,64); //-V112

	static BOOL (WINAPI* IsWow64Process_f)(HANDLE, PBOOL) = NULL;

	if (!IsWow64Process_f)
	{
		// Kernel32.dll is always loaded due to static link
		MModule kernel(GetModuleHandle(L"kernel32.dll"));
		kernel.GetProcAddress("IsWow64Process", IsWow64Process_f);
		// 64-bit OS must have this function
		_ASSERTE(IsWow64Process_f != NULL);
	}

	if (IsWow64Process_f)
	{
		BOOL bWow64 = FALSE;
		HANDLE h = hProcess ? hProcess : OpenProcess(PROCESS_QUERY_INFORMATION, FALSE, nPID);

		// IsWow64Process would be succeessfull for PROCESS_QUERY_LIMITED_INFORMATION (Vista+)
		if ((h == NULL) && IsWin6())
		{
			// PROCESS_QUERY_LIMITED_INFORMATION not defined in GCC
			h = OpenProcess(0x1000/*PROCESS_QUERY_LIMITED_INFORMATION*/, FALSE, nPID);
		}

		if (h == NULL)
		{
			// If it is blocked due to access rights - try to find alternative ways (by path or PERF COUNTER)
			ImageBits = 0;
		}
		else if (IsWow64Process_f(h, &bWow64) && !bWow64)
		{
			ImageBits = 64;
		}
		else
		{
			ImageBits = 32;
		}

		if (h && (h != hProcess))
			CloseHandle(h);
	}

	return ImageBits;
}
