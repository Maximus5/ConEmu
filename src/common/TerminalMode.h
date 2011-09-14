
/*
Copyright (c) 2010-2011 Maximus5
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

#pragma once

#include <Tlhelp32.h>

static bool isTerminalMode()
{
	static bool TerminalMode = false, TerminalChecked = false;

	if (!TerminalChecked)
	{
		// -- переменная "TERM" может быть задана пользователем для каких-то специальных целей
		//TCHAR szVarValue[64];
		//szVarValue[0] = 0;
		//if (GetEnvironmentVariable(_T("TERM"), szVarValue, 63) && szVarValue[0])
		//{
		//	TerminalMode = true;
		//}
		//TerminalChecked = true;
		PROCESSENTRY32  P = {sizeof(PROCESSENTRY32)};
		HANDLE hSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);

		if (hSnap == INVALID_HANDLE_VALUE)
		{
			// будем считать, что не в telnet :)
		}
		else if (Process32First(hSnap, &P))
		{
			size_t nProcCount = 0, nProcMax = 1024;
			PROCESSENTRY32 *pProcesses = (PROCESSENTRY32*)calloc(nProcMax, sizeof(PROCESSENTRY32));
			DWORD nCurPID = GetCurrentProcessId();
			DWORD nParentPID = nCurPID;

			// Сначала загрузить список всех процессов, чтобы потом по нему выйти не корневой
			do
			{
				if (nProcCount == nProcMax)
				{
					nProcMax += 1024;
					PROCESSENTRY32 *p = (PROCESSENTRY32*)calloc(nProcMax, sizeof(PROCESSENTRY32));
					memmove(pProcesses, p, nProcCount*sizeof(PROCESSENTRY32)); //-V104
					free(pProcesses);
					pProcesses = p;
				}

				pProcesses[nProcCount] = P;

				if (P.th32ProcessID == nParentPID)
				{
					if (P.th32ProcessID != nCurPID)
					{
						if (!lstrcmpi(P.szExeFile, L"tlntsess.exe") || !lstrcmpi(P.szExeFile, L"tlntsvr.exe"))
						{
							TerminalMode = TerminalChecked = true;
							break;
						}
					}

					nParentPID = P.th32ParentProcessID;
				}

				nProcCount++;
			}
			while(Process32Next(hSnap, &P));

			// Snapshoot больше не нужен
			CloseHandle(hSnap);
			int nSteps = 128; // защита от зацикливания

			while(!TerminalMode && (--nSteps) > 0)
			{
				for(size_t i = 0; i < nProcCount; i++)
				{
					if (pProcesses[i].th32ProcessID == nParentPID)
					{
						if (P.th32ProcessID != nCurPID)
						{
							if (!lstrcmpi(pProcesses[i].szExeFile, L"tlntsess.exe") || !lstrcmpi(pProcesses[i].szExeFile, L"tlntsvr.exe"))
							{
								TerminalMode = TerminalChecked = true;
								break;
							}
						}

						nParentPID = pProcesses[i].th32ParentProcessID;
						break;
					}
				}
			}

			free(pProcesses);
		}
	}

	// В повторых проверках смысла нет
	TerminalChecked = true;
	return TerminalMode;
}
