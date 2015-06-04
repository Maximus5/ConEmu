
/*
Copyright (c) 2009-2015 Maximus5
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
#include "common.hpp"
#include "CmdArg.h"
#include "CmdLine.h"
#include "WObjects.h"

//Issue 60: BUGBUG: На некоторых системых (Win2k3, WinXP) SetConsoleCP (и иже с ними) просто зависают
//Поэтому выполняем в отдельном потоке, и если он завис - просто зовем TerminateThread
static DWORD WINAPI OurSetConsoleCPThread(LPVOID lpParameter)
{
	UINT nCP = (UINT)lpParameter;
	SetConsoleCP(nCP);
	SetConsoleOutputCP(nCP);
	return 0;
}

bool SetConsoleCpHelper(UINT nCP)
{
	if (!nCP || nCP > 0xFFFF)
		return false;

	bool bOk = false;

	//Issue 60: BUGBUG: On some OS versions (Win2k3, WinXP) SetConsoleCP (and family) just hangs
	DWORD nTID;
	HANDLE hThread = CreateThread(NULL, 0, OurSetConsoleCPThread, (LPVOID)nCP, 0, &nTID);

	if (hThread)
	{
		DWORD nWait = WaitForSingleObject(hThread, 1000);

		if (nWait == WAIT_TIMEOUT)
		{
			// That is dangerous operation, however there is no other workaround
			// http://conemu.github.io/en/MicrosoftBugs.html#chcp_hung

			TerminateThread(hThread,100);

		}

		CloseHandle(hThread);
	}

	return bOk;
}

// Return true if "SetEnvironmentVariable" was processed
// if (bDoSet==false) - just skip all "set" commands
// Supported commands:
//  set abc=val
//  "set PATH=C:\Program Files;%PATH%"
//  chcp [utf8|ansi|oem|<cp_no>]
//  title "Console init title"
bool ProcessSetEnvCmd(LPCWSTR& asCmdLine, bool bDoSet, CmdArg* rpsTitle /*= NULL*/)
{
	LPCWSTR lsCmdLine = asCmdLine;
	bool bEnvChanged = false;
	CmdArg lsSet, lsAmp;

	// Example: "set PATH=C:\Program Files;%PATH%" & set abc=def & cmd
	while (NextArg(&lsCmdLine, lsSet) == 0)
	{
		bool bTokenOk = false;
		wchar_t* lsNameVal = NULL;

		// It may contains only "set" if was not quoted
		if (lstrcmpi(lsSet, L"set") == 0)
		{
			// Now we shell get in lsSet "abc=def" token
			if ((NextArg(&lsCmdLine, lsSet) == 0) && (wcschr(lsSet, L'=') > lsSet.ms_Arg))
			{
				lsNameVal = lsSet.ms_Arg;
			}
		}
		// Or full "set PATH=C:\Program Files;%PATH%" command (without quotes ATM)
		else if (lstrcmpni(lsSet, L"set ", 4) == 0)
		{
			LPCWSTR psz = SkipNonPrintable(lsSet.ms_Arg+4);
			if (wcschr(psz, L'=') > psz)
			{
				lsNameVal = (wchar_t*)psz;
			}
		}
		// Process "chcp <cp>" too
		else if (lstrcmpi(lsSet, L"chcp") == 0)
		{
			if (NextArg(&lsCmdLine, lsSet) == 0)
			{
				UINT nCP = GetCpFromString(lsSet);

				if (nCP > 0 && nCP <= 0xFFFF)
				{
					bTokenOk = true;
					_ASSERTE(lsNameVal == NULL);
					// Ask to be changed?
					if (bDoSet)
					{
						//Issue 60: BUGBUG: На некоторых системых (Win2k3, WinXP) SetConsoleCP (и иже с ними) просто зависают
						DWORD nTID;
						HANDLE hThread = CreateThread(NULL, 0, OurSetConsoleCPThread, (LPVOID)nCP, 0, &nTID);
						if (hThread)
						{
							DWORD nWait = WaitForSingleObject(hThread, 1000);
							if (nWait == WAIT_TIMEOUT)
							{
								TerminateThread(hThread,100);
							}
							CloseHandle(hThread);
						}
					}
				}
			}
		}
		// Change title without need of cmd.exe
		else if (lstrcmpi(lsSet, L"title") == 0)
		{
			if (NextArg(&lsCmdLine, lsSet) == 0)
			{
				bTokenOk = true;
				_ASSERTE(lsNameVal == NULL);
				// Ask to be changed?
				if (rpsTitle)
					rpsTitle->Set(lsSet);
			}
		}

		// Well, known command was detected. What is next?
		if (lsNameVal || bTokenOk)
		{
			lsAmp.GetPosFrom(lsSet);
			if (NextArg(&lsCmdLine, lsAmp) != 0)
			{
				// End of command? Use may call only "set" without following app? Run simple "cmd" in that case
				_ASSERTE(lsCmdLine!=NULL && *lsCmdLine==0);
				bTokenOk = true; // And process SetEnvironmentVariable
			}
			else
			{
				if (lstrcmp(lsAmp, L"&") == 0)
				{
					// Only simple conveyer is supported!
					bTokenOk = true; // And process SetEnvironmentVariable
				}
				// Update last pointer (debug and asserts purposes)
				lsSet.GetPosFrom(lsAmp);
			}
		}

		if (!bTokenOk)
		{
			break; // Stop processing command line
		}
		else if (lsNameVal)
		{
			// And split name/value
			_ASSERTE(lsNameVal!=NULL);

			wchar_t* pszEq = wcschr(lsNameVal, L'=');
			if (!pszEq)
			{
				_ASSERTE(pszEq!=NULL);
				break;
			}

			if (bDoSet)
			{
				*(pszEq++) = 0;
				// Expand value
				wchar_t* pszExpanded = ExpandEnvStr(pszEq);
				LPCWSTR pszSet = pszExpanded ? pszExpanded : pszEq;
				SetEnvironmentVariable(lsNameVal, (pszSet && *pszSet) ? pszSet : NULL);
				SafeFree(pszExpanded);
			}

			bEnvChanged = true;
		}

		// Remember processed position
		asCmdLine = lsCmdLine;

	} // end of "while (NextArg(&lsCmdLine, lsSet) == 0)"

	// Fin
	if (!asCmdLine || !*asCmdLine)
	{
		static wchar_t szSimple[] = L"cmd";
		asCmdLine = szSimple;
	}

	return bEnvChanged;
}
