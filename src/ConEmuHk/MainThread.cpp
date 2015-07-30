
/*
Copyright (c) 2015 Maximus5
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


#include "../common/common.hpp"
#include <Tlhelp32.h>

DWORD gnHookMainThreadId = 0;

DWORD GetMainThreadId(bool bUseCurrentAsMain)
{
	// Найти ID основной нити
	if (!gnHookMainThreadId)
	{
		if (bUseCurrentAsMain)
		{
			gnHookMainThreadId = GetCurrentThreadId();
		}
		else
		{
			DWORD dwPID = GetCurrentProcessId();
			HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, dwPID);

			if (snapshot != INVALID_HANDLE_VALUE)
			{
				THREADENTRY32 module = {sizeof(THREADENTRY32)};

				if (Thread32First(snapshot, &module))
				{
					while (!gnHookMainThreadId)
					{
						if (module.th32OwnerProcessID == dwPID)
						{
							gnHookMainThreadId = module.th32ThreadID;
							break;
						}

						if (!Thread32Next(snapshot, &module))
							break;
					}
				}

				CloseHandle(snapshot);
			}
		}
	}

	#ifdef _DEBUG
	char szInfo[100];
	msprintf(szInfo, countof(szInfo), "GetMainThreadId()=%u, TID=%u\n", gnHookMainThreadId, GetCurrentThreadId());
	//OutputDebugStringA(szInfo);
	#endif

	_ASSERTE(gnHookMainThreadId!=0);
	return gnHookMainThreadId;
}
