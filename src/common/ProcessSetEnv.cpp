
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
#include "Common.h"
#include "CEStr.h"
#include "CmdLine.h"
#include "ProcessSetEnv.h"
#include "WObjects.h"
#include "WThreads.h"

//Issue 60: BUGBUG: На некоторых системах (Win2k3, WinXP) SetConsoleCP (и иже с ними) просто зависают
//Поэтому выполняем в отдельном потоке, и если он завис - просто зовем TerminateThread
static DWORD WINAPI OurSetConsoleCPThread(LPVOID lpParameter)
{
	UINT nCP = LODWORD(lpParameter);
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
	HANDLE hThread = apiCreateThread(OurSetConsoleCPThread, (LPVOID)(DWORD_PTR)nCP, &nTID, "OurSetConsoleCPThread(%u)", nCP);

	if (hThread)
	{
		DWORD nWait = WaitForSingleObject(hThread, 1000);

		if (nWait == WAIT_TIMEOUT)
		{
			// That is dangerous operation, however there is no other workaround
			// https://conemu.github.io/en/MicrosoftBugs.html#chcp_hung

			apiTerminateThread(hThread, 100);

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
bool ProcessSetEnvCmd(LPCWSTR& asCmdLine, bool bDoSet, CEStr* rpsTitle /*= NULL*/, CProcessEnvCmd* pSetEnv /*= NULL*/)
{
	LPCWSTR lsCmdLine = asCmdLine;

	CProcessEnvCmd tmp;
	if (!pSetEnv)
		pSetEnv = &tmp;

	pSetEnv->AddCommands(asCmdLine, &lsCmdLine);

	bool bEnvChanged = (pSetEnv->mn_SetCount != 0);

	// Ask to be changed?
	if (bDoSet)
		pSetEnv->Apply();

	// Return requested title?
	if (rpsTitle && pSetEnv->mp_CmdTitle)
		rpsTitle->Set(pSetEnv->mp_CmdTitle->pszName);

	asCmdLine = lsCmdLine;

	// Fin
	_ASSERTE(asCmdLine && *asCmdLine);

	return bEnvChanged;
}

CProcessEnvCmd::CProcessEnvCmd()
	: mch_Total(0)
	, mn_SetCount(0)
	, mp_CmdTitle(NULL)
	, mp_CmdChCp(NULL)
{
}

CProcessEnvCmd::~CProcessEnvCmd()
{
	for (INT_PTR i = 0; i < m_Commands.size(); i++)
	{
		Command* p = m_Commands[i];
		SafeFree(p->pszName);
		SafeFree(p->pszValue);
		SafeFree(p);
	}
	m_Commands.clear();
}

// May comes from Task or ConEmu's /cmd switch
// or from Setting\Environment page where one line is a single command (bAlone == true)
void CProcessEnvCmd::AddCommands(LPCWSTR asCommands, LPCWSTR* ppszEnd/*= NULL*/, bool bAlone /*= false*/)
{
	LPCWSTR lsCmdLine = asCommands;
	CEStr lsSet, lsAmp, lsCmd;

	if (ppszEnd)
		*ppszEnd = asCommands;

	// Example: "set PATH=C:\Program Files;%PATH%" & set abc=def & cmd
	while (NextArg(&lsCmdLine, lsSet) == 0)
	{
		bool bTokenOk = false;
		wchar_t* lsNameVal = NULL;

		// It may contains only "set" or "alias" if was not quoted
		if ((lstrcmpi(lsSet, L"set") == 0) || (lstrcmpi(lsSet, L"alias") == 0))
		{
			lsCmd.Set(lsSet);

			_ASSERTE(*lsCmdLine != L' ');

			// Now we shell get in lsSet "abc=def" token
			// Or to simplify usage, the rest of line is supported too,
			// so user may type in our Settings\Environment:
			//   set V1=From Settings
			// instead of
			//   set "V1=From Settings"
			bool bProcessed = false;
			if ((*lsCmdLine != L'"') || bAlone)
			{
				LPCWSTR pszAmp = bAlone ? NULL : wcschr(lsCmdLine, L'&');
				if (!pszAmp) // No ampersand or bAlone? Use var value as the rest of line
					pszAmp = lsCmdLine + lstrlen(lsCmdLine);
				// Set tail pointer
				LPCWSTR pszValEnd = pszAmp;
				// Trim trailing spaces (only \x20)
				while ((pszValEnd > lsCmdLine) && (*(pszValEnd-1) == L' '))
					pszValEnd--;
				// Trim possible leading/trailing quotes
				if (bAlone && (*lsCmdLine == L'"'))
				{
					lsCmdLine++;
					if (((pszValEnd-1) > lsCmdLine) && (*(pszValEnd-1) == L'"'))
					{
						_ASSERTE(*pszValEnd == 0);
						pszValEnd--;
					}
				}
				// OK, return it
				lsSet.Empty(); // to avoid debug asserts
				lsSet.Set(lsCmdLine, pszValEnd - lsCmdLine);
				lsCmdLine = SkipNonPrintable(pszAmp); // Leave possible '&' at pointer
				bProcessed = true;
			}
			// OK, lets get token like "name=var value"
			if (!bProcessed)
			{
				bProcessed = (NextArg(&lsCmdLine, lsSet) == 0);
			}
			if (bProcessed && (wcschr(lsSet, L'=') > lsSet.ms_Val))
			{
				lsNameVal = lsSet.ms_Val;
			}
		}
		// Or full "set PATH=C:\Program Files;%PATH%" command (without quotes ATM)
		else if (lstrcmpni(lsSet, L"set ", 4) == 0)
		{
			lsCmd = L"set";
			LPCWSTR psz = SkipNonPrintable(lsSet.ms_Val+4);
			if (wcschr(psz, L'=') > psz)
			{
				lsNameVal = (wchar_t*)psz;
			}
		}
		// Support "alias token=value" too
		else if (lstrcmpni(lsSet, L"alias ", 6) == 0)
		{
			lsCmd = L"alias";
			LPCWSTR psz = SkipNonPrintable(lsSet.ms_Val+6);
			if (wcschr(psz, L'=') > psz)
			{
				lsNameVal = (wchar_t*)psz;
			}
		}
		// Process "chcp <cp>" too
		else if (lstrcmpi(lsSet, L"chcp") == 0)
		{
			lsCmd = L"chcp";
			if (NextArg(&lsCmdLine, lsSet) == 0)
			{
				UINT nCP = GetCpFromString(lsSet);
				if (nCP > 0 && nCP <= 0xFFFF)
				{
					bTokenOk = true;
					_ASSERTE(lsNameVal == NULL);
					if (mp_CmdChCp)
					{
						SafeFree(mp_CmdChCp->pszName);
						mp_CmdChCp->pszName = lstrdup(lsSet);
					}
					else
					{
						Add(L"chcp", lsSet, NULL);
					}
				}
			}
		}
		// Change title without need of cmd.exe
		else if (lstrcmpi(lsSet, L"title") == 0)
		{
			lsCmd = L"title";
			if (NextArg(&lsCmdLine, lsSet) == 0)
			{
				bTokenOk = true;
				_ASSERTE(lsNameVal == NULL);
				if (mp_CmdTitle)
				{
					SafeFree(mp_CmdTitle->pszName);
					mp_CmdTitle->pszName = lstrdup(lsSet);
				}
				else
				{
					Add(L"title", lsSet, NULL);
				}
			}
		}
		else
		{
			lsCmd.Empty();
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

			*(pszEq++) = 0;

			Add(lsCmd, lsNameVal, pszEq ? pszEq : L"");
		}

		// Remember processed position
		if (ppszEnd)
		{
			*ppszEnd = lsCmdLine;
		}
	} // end of "while (NextArg(&lsCmdLine, lsSet) == 0)"

	// Fin
	if (ppszEnd && (!*ppszEnd || !**ppszEnd))
	{
		static wchar_t szSimple[] = L"cmd";
		*ppszEnd = szSimple;
	}
}

// Comes from CESERVER_REQ::NewCmd.GetEnvStrings()
void CProcessEnvCmd::AddZeroedPairs(LPCWSTR asNameValueSeq)
{
	if (!asNameValueSeq)
		return;
	LPCWSTR pszSrc = asNameValueSeq;
	// ZZ-terminated pairs
	while (*pszSrc)
	{
		// Pairs "Name\0Value\0\0"
		LPCWSTR pszName = pszSrc;
		_ASSERTE(wcschr(pszName, L'=') == NULL);
		LPCWSTR pszValue = pszName + lstrlen(pszName) + 1;
		LPCWSTR pszNext = pszValue + lstrlen(pszValue) + 1;
		// Next pair
		pszSrc = pszNext;

		if (IsEnvBlockVariableValid(pszName))
		{
			Add(L"set", pszName, pszValue);
		}
	}
}

// Comes from ConEmu's settings (Environment setting page)
void CProcessEnvCmd::AddLines(LPCWSTR asLines)
{
	LPCWSTR pszLines = asLines;
	CEStr lsLine;

	while (0 == NextLine(&pszLines, lsLine))
	{
		// Skip empty lines
		LPCWSTR pszLine = SkipNonPrintable(lsLine);
		if (!pszLine || !*pszLine)
			continue;
		// A comment?
		if ((pszLine[0] == L'#')
			|| ((pszLine[0] == L'/') && (pszLine[1] == L'/'))
			|| ((pszLine[0] == L'-') && (pszLine[1] == L'-'))
			|| (lstrcmpni(pszLine, L"REM ", 4) == 0)
			)
			continue;
		// Process this line
		AddCommands(pszLine, NULL, true);
	}
}

CProcessEnvCmd::Command* CProcessEnvCmd::Add(LPCWSTR asCmd, LPCWSTR asName, LPCWSTR asValue)
{
	if (!asCmd || !*asCmd || !asName || !*asName)
	{
		_ASSERTE(asCmd && *asCmd && asName && *asName);
		return NULL;
	}

	Command* p = (Command*)malloc(sizeof(Command));
	if (!p)
		return NULL;

	wcscpy_c(p->szCmd, asCmd);
	p->pszName = lstrdup(asName);
	p->pszValue = (asValue && *asValue) ? lstrdup(asValue) : NULL;

	if (lstrcmp(asCmd, L"set") == 0)
	{
		mn_SetCount++;
		mch_Total += (7 + lstrlen(asName) + (asValue ? lstrlen(asValue) : 0));
	}
	else if (lstrcmp(asCmd, L"alias") == 0)
	{
		mn_SetCount++;
		mch_Total += (9 + lstrlen(asName) + (asValue ? lstrlen(asValue) : 0));
	}
	else if (lstrcmp(asCmd, L"chcp") == 0)
	{
		_ASSERTE(mp_CmdChCp==NULL);
		mp_CmdChCp = p;
		mch_Total += (6 + lstrlen(asName));
	}
	else if (lstrcmp(asCmd, L"title") == 0)
	{
		_ASSERTE(mp_CmdTitle==NULL);
		mp_CmdTitle = p;
		mch_Total += (7 + lstrlen(asName));
	}
	else
	{
		_ASSERTE(FALSE && "Command was not counted!");
	}

	m_Commands.push_back(p);

	return p;
}

bool CProcessEnvCmd::Apply()
{
	bool bEnvChanged = false;
	Command* p;

	for (INT_PTR i = 0; i < m_Commands.size(); i++)
	{
		p = m_Commands[i];

		if (lstrcmp(p->szCmd, L"set") == 0)
		{
			bEnvChanged = true;
			// Expand value
			wchar_t* pszExpanded = ExpandEnvStr(p->pszValue);
			LPCWSTR pszSet = pszExpanded ? pszExpanded : p->pszValue;
			SetEnvironmentVariable(p->pszName, (pszSet && *pszSet) ? pszSet : NULL);
			SafeFree(pszExpanded);
		}
		else if (lstrcmpi(p->szCmd, L"alias") == 0)
		{
			// NULL will remove alias
			// We set aliases for "cmd.exe" executable, as Far Manager supports too
			AddConsoleAlias(p->pszName, (p->pszValue && *p->pszValue) ? p->pszValue : NULL, L"cmd.exe");
		}
		else if (lstrcmp(p->szCmd, L"chcp") == 0)
		{
			UINT nCP = GetCpFromString(p->pszName);
			if (nCP > 0 && nCP <= 0xFFFF)
			{
				//Issue 60: BUGBUG: On some OS versions (Win2k3, WinXP) SetConsoleCP (and family) just hangs
				SetConsoleCpHelper(nCP);
			}
		}
		else if (lstrcmp(p->szCmd, L"title") == 0)
		{
			// Do not do anything here...
		}
		else
		{
			_ASSERTE(FALSE && "Command was not implemented!");
		}
	}

	return bEnvChanged;
}

wchar_t* CProcessEnvCmd::Allocate(size_t* pchSize)
{
	wchar_t* pszData = NULL;
	size_t cchData = 0;

	if (mch_Total)
	{
		pszData = (wchar_t*)malloc((mch_Total+1)*sizeof(*pszData));
		if (pszData)
		{
			wchar_t* pszDst = pszData;

			for (INT_PTR i = 0; i < m_Commands.size(); i++)
			{
				Command* p = m_Commands[i];

				if ((lstrcmp(p->szCmd, L"set") == 0) || (lstrcmp(p->szCmd, L"alias") == 0))
				{
					cpyadv(pszDst, p->szCmd);
					pszDst++; // leave '\0'
					cpyadv(pszDst, p->pszName);
					*(pszDst++) = L'='; // replace '\0' with '='
					if (p->pszValue)
						cpyadv(pszDst, p->pszValue);
					else
						*pszDst = 0;
					pszDst++; // leave '\0'
				}
				else if (lstrcmp(p->szCmd, L"chcp") == 0)
				{
					cpyadv(pszDst, L"chcp");
					pszDst++; // leave '\0'
					cpyadv(pszDst, p->pszName);
					pszDst++; // leave '\0'
				}
				else if (lstrcmp(p->szCmd, L"title") == 0)
				{
					cpyadv(pszDst, L"title");
					pszDst++; // leave '\0'
					cpyadv(pszDst, p->pszName);
					pszDst++; // leave '\0'
				}
				else
				{
					_ASSERTE(FALSE && "Command was not implemented!");
				}
			}

			// Fin
			cchData = (pszDst - pszData + 1);
			_ASSERTE(cchData > 2 && pszData[cchData-2] == 0 && pszData[cchData-3] != 0 && pszData[cchData]);
			pszData[cchData-1] = 0; // MSZZ
		}
	}

	if (pchSize)
		*pchSize = cchData;
	return pszData;
}

void CProcessEnvCmd::cpyadv(wchar_t*& pszDst, LPCWSTR asStr)
{
	if (!asStr || !*asStr)
		return;
	size_t nLen = wcslen(asStr);
	memmove(pszDst, asStr, (nLen+1)*sizeof(*pszDst));
	pszDst += nLen; // set pointer to \0
}
