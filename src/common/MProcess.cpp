
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
#include "Common.h"
#include "MProcess.h"
#include "MToolHelp.h"

bool GetProcessInfo(DWORD nPID, PROCESSENTRY32W* Info)
{
	MToolHelpProcess prc;
	return prc.Find(nPID, Info);
}

bool GetProcessInfo(LPCWSTR asExeName, PROCESSENTRY32W* Info)
{
	MToolHelpProcess prc;
	return prc.Find(asExeName, Info);
}

bool isTerminalMode()
{
	static bool TerminalMode = false, TerminalChecked = false;

	if (!TerminalChecked)
	{
		// -- Environment variable "TERM" may be set by user
		//TCHAR szVarValue[64];
		//szVarValue[0] = 0;
		//if (GetEnvironmentVariable(_T("TERM"), szVarValue, 63) && szVarValue[0])
		//{
		//	TerminalMode = true;
		//}
		//TerminalChecked = true;

		PROCESSENTRY32 P = {sizeof(PROCESSENTRY32)};
		MToolHelpProcess prc;

		if (!prc.Find(GetCurrentProcessId(), &P))
		{
			_ASSERTE(FALSE && "Failed to load self-process-information");
		}
		else
		{
			int nSteps = 128; // protection from recursion
			DWORD nParentPID = P.th32ParentProcessID;
			DEBUGTEST(DWORD nSelfParentPID = P.th32ParentProcessID);
			while (nSteps-- > 0)
			{
				if (!prc.Find(nParentPID, &P))
				{
					_ASSERTE((nParentPID != nSelfParentPID) && "Failed to load parent process information");
					break;
				}

				if ((0 == lstrcmpi(P.szExeFile, L"tlntsess.exe"))
					|| (0 == lstrcmpi(P.szExeFile, L"tlntsvr.exe")))
				{
					TerminalMode = TerminalChecked = true;
					break;
				}

				// ...grand parent
				nParentPID = P.th32ParentProcessID;
			}
		}
	}

	// No sense to check again
	TerminalChecked = true;
	return TerminalMode;
}
