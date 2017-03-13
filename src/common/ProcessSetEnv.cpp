
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
#include "Common.h"
#include "CEStr.h"
#include "CmdLine.h"
#include "MStrDup.h"
#include "ProcessSetEnv.h"
#include "WObjects.h"
#include "WThreads.h"

// Return true if "SetEnvironmentVariable" was processed
// if (bDoSet==false) - just skip all "set" commands
// Supported commands:
//  set abc=val
//  "set PATH=C:\Program Files;%PATH%"
//  chcp [utf8|ansi|oem|<cp_no>]
//  title "Console init title"
bool ProcessSetEnvCmd(LPCWSTR& asCmdLine, CProcessEnvCmd* pSetEnv /*= NULL*/, CStartEnvBase* pDoSet /*= NULL*/)
{
	LPCWSTR lsCmdLine = asCmdLine;

	CProcessEnvCmd tmp;
	CProcessEnvCmd* lpSetEnv = pSetEnv ? pSetEnv : &tmp;

	lpSetEnv->AddCommands(asCmdLine, &lsCmdLine);

	bool bEnvChanged = (lpSetEnv->mn_SetCount != 0);

	// Ask to be changed?
	if (pDoSet)
		lpSetEnv->Apply(pDoSet);

	// Return naked command
	asCmdLine = lsCmdLine;

	// Fin, we must have something to execute
	_ASSERTE(asCmdLine && *asCmdLine);

	return bEnvChanged;
}


CStartEnvTitle::CStartEnvTitle(wchar_t** ppszTitle)
	: mppsz_Title(ppszTitle)
	, mps_Title(NULL)
{
}

CStartEnvTitle::CStartEnvTitle(CEStr* psTitle)
	: mppsz_Title(NULL)
	, mps_Title(psTitle)
{
}

void CStartEnvTitle::Title(LPCWSTR asTitle)
{
	if (!asTitle || !*asTitle)
	{
		_ASSERTE(asTitle && *asTitle);
		return;
	}

	if (mppsz_Title)
	{
		SafeFree(*mppsz_Title);
		*mppsz_Title = lstrdup(asTitle);
	}
	else if (mps_Title)
	{
		mps_Title->Set(asTitle);
	}
	else
	{
		_ASSERTE(mppsz_Title || mps_Title);
	}
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
		if (p)
		{
			SafeFree(p->pszName);
			SafeFree(p->pszValue);
			SafeFree(p);
		}
	}
	m_Commands.clear();
}

// May come from Task or ConEmu's -run switch
// or from Setting\Environment page where one line is a single command (bAlone == true)
bool CProcessEnvCmd::AddCommands(LPCWSTR asCommands, LPCWSTR* ppszEnd/*= NULL*/, bool bAlone /*= false*/, INT_PTR anBefore /*= -1*/)
{
	bool bNew = false;
	LPCWSTR lsCmdLine = asCommands;
	CEStr lsSet, lsAmp, lsCmd;

	if (ppszEnd)
		*ppszEnd = asCommands;

	// Example: "set PATH=C:\Program Files;%PATH%" & set abc=def & cmd
	while (NextArg(&lsCmdLine, lsSet) == 0)
	{
		bool bTokenOk = false;
		wchar_t* lsNameVal = NULL;

		// It may contain only "set" or "alias" if was not quoted
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
						bNew |= (NULL != Add(lsCmd, lsSet, NULL, anBefore));
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
					bNew |= (NULL != Add(lsCmd, lsSet, NULL, anBefore));
				}
			}
		}
		// Echo the "text" before console starts
		// Type the "text file" before console starts
		else if ((lstrcmpi(lsSet, L"echo") == 0) || (lstrcmpi(lsSet, L"type") == 0))
		{
			lsCmd.Set(lsSet);
			// echo [-r] [-n] [-x] [-b] "String to echo"
			// type [-b] [-CP] "Path to text file to echo"
			CEStr lsSwitches;
			while (*lsCmdLine == L'-')
			{
				if (NextArg(&lsCmdLine, lsSet) != 0)
					break;
				lstrmerge(&lsSwitches.ms_Val, lsSwitches.IsEmpty() ? NULL : L" ", lsSet.ms_Val);
			}
			// Rest arguments are expected to be processed text or file
			CEStr lsAdd;
			while (NextArg(&lsCmdLine, lsSet) == 0)
			{
				bTokenOk = true;
				lstrmerge(&lsAdd.ms_Val,
					lsAdd.IsEmpty() ? NULL : L" ",
					lsSet.mb_Quoted ? L"\"" : NULL,
					lsSet.ms_Val,
					lsSet.mb_Quoted ? L"\"" : NULL);
				lsCmdLine = SkipNonPrintable(lsCmdLine);
				if (!*lsCmdLine || (*lsCmdLine == L'&') || (*lsCmdLine == L'|'))
					break;

			}
			if (!lsAdd.IsEmpty())
			{
				bNew |= (NULL != Add(lsCmd, lsSwitches, lsAdd, anBefore));
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

			bNew |= (NULL != Add(lsCmd, lsNameVal, pszEq ? pszEq : L"", anBefore));
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

	return bNew;
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
			Add(L"set", pszName, pszValue, -1 /*append*/);
		}
	}
}

// Comes from ConEmu's settings (Environment setting page)
void CProcessEnvCmd::AddLines(LPCWSTR asLines, bool bPriority)
{
	LPCWSTR pszLines = asLines;
	CEStr lsLine;
	INT_PTR nBefore = bPriority ? 0 : -1;

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
		if (AddCommands(pszLine, NULL, true, nBefore) && bPriority)
			nBefore++;
	}
}

CProcessEnvCmd::Command* CProcessEnvCmd::Add(LPCWSTR asCmd, LPCWSTR asName, LPCWSTR asValue, INT_PTR anBefore)
{
	if (!asCmd || !*asCmd)
	{
		_ASSERTE(asCmd && *asCmd);
		return NULL;
	}

	if ((lstrcmp(asCmd, L"echo") == 0) || (lstrcmp(asCmd, L"type") == 0))
	{
		// These are intended to be an options for echo/type
		if (asName == NULL)
			asName = L"";
	}
	else
	{
		if (!asName || !*asName)
		{
			_ASSERTE(asName && *asName);
			return NULL;
		}
	}

	Command* p = (Command*)malloc(sizeof(Command));
	if (!p)
		return NULL;

	wcscpy_c(p->szCmd, asCmd);
	p->pszName = lstrdup(asName);
	p->pszValue = (asValue && *asValue) ? lstrdup(asValue) : NULL;

	size_t cchAdd = 0;

	if (lstrcmp(asCmd, L"set") == 0)
	{
		mn_SetCount++;
		cchAdd = 5; // set "name=value"\0
	}
	else if (lstrcmp(asCmd, L"alias") == 0)
	{
		mn_SetCount++;
		cchAdd = 5; // alias "name=value"\0
	}
	else if (lstrcmp(asCmd, L"chcp") == 0)
	{
		_ASSERTE(mp_CmdChCp==NULL);
		mp_CmdChCp = p;
		cchAdd = 4; // chcp "cp"\0
	}
	else if (lstrcmp(asCmd, L"title") == 0)
	{
		_ASSERTE(mp_CmdTitle==NULL);
		mp_CmdTitle = p;
		cchAdd = 4; // title "new title"\0
	}
	else if ((lstrcmp(asCmd, L"echo") == 0) || (lstrcmp(asCmd, L"type") == 0))
	{
		cchAdd = 5; // echo <options> "text"\0 | type <options> "file"\0
	}
	else
	{
		_ASSERTE(FALSE && "Command was not counted!");
		return NULL;
	}

	mch_Total += (lstrlen(asCmd) + cchAdd + lstrlen(asName) + (asValue ? lstrlen(asValue) : 0));

	m_Commands.insert(anBefore, p);

	return p;
}

bool CProcessEnvCmd::Apply(CStartEnvBase* pSetEnv)
{
	bool bEnvChanged = false;
	Command* p;

	if (!pSetEnv)
	{
		_ASSERTE(pSetEnv!=NULL);
		return false;
	}

	for (INT_PTR i = 0; i < m_Commands.size(); i++)
	{
		p = m_Commands[i];

		if (lstrcmp(p->szCmd, L"set") == 0)
		{
			bEnvChanged = true;
			// Expand value
			pSetEnv->Set(p->pszName, p->pszValue);
		}
		else if (lstrcmpi(p->szCmd, L"alias") == 0)
		{
			// NULL will remove alias
			// We set aliases for "cmd.exe" executable, as Far Manager supports too
			pSetEnv->Alias(p->pszName, p->pszValue);
		}
		else if (lstrcmp(p->szCmd, L"chcp") == 0)
		{
			pSetEnv->ChCp(p->pszName);
		}
		else if (lstrcmp(p->szCmd, L"title") == 0)
		{
			pSetEnv->Title(p->pszName);
		}
		else if (lstrcmp(p->szCmd, L"echo") == 0)
		{
			pSetEnv->Echo(p->pszName, p->pszValue);
		}
		else if (lstrcmp(p->szCmd, L"type") == 0)
		{
			pSetEnv->Type(p->pszName, p->pszValue);
		}
		else
		{
			_ASSERTE(FALSE && "Command was not implemented!");
		}
	}

	return bEnvChanged;
}

// Helper to fill CESERVER_REQ_SRVSTARTSTOPRET::pszCommands
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
					*(pszDst++) = L' ';
					*(pszDst++) = L'\"';
					cpyadv(pszDst, p->pszName);
					*(pszDst++) = L'='; // name=value
					if (p->pszValue)
						cpyadv(pszDst, p->pszValue);
					else
						*pszDst = 0;
					*(pszDst++) = L'\"';
					*(pszDst++) = L'\n';
				}
				else if (lstrcmp(p->szCmd, L"chcp") == 0)
				{
					cpyadv(pszDst, L"chcp");
					*(pszDst++) = L' ';
					*(pszDst++) = L'\"';
					cpyadv(pszDst, p->pszName);
					*(pszDst++) = L'\"';
					*(pszDst++) = L'\n';
				}
				else if (lstrcmp(p->szCmd, L"title") == 0)
				{
					cpyadv(pszDst, L"title");
					*(pszDst++) = L' ';
					*(pszDst++) = L'\"';
					cpyadv(pszDst, p->pszName);
					*(pszDst++) = L'\"';
					*(pszDst++) = L'\n';
				}
				else if ((lstrcmp(p->szCmd, L"echo") == 0) || (lstrcmp(p->szCmd, L"type") == 0))
				{
					cpyadv(pszDst, p->szCmd);
					*(pszDst++) = L' ';
					if (p->pszName && *p->pszName)
					{
						// type/echo options (like in `ConEmuC -t/-e ...`)
						cpyadv(pszDst, p->pszName);
						*(pszDst++) = L' ';
					}
					//*(pszDst++) = L'\"';
					cpyadv(pszDst, p->pszValue);
					//*(pszDst++) = L'\"';
					*(pszDst++) = L'\n';
				}
				else
				{
					_ASSERTE(FALSE && "Command was not implemented!");
				}
			}

			// Fin
			cchData = (pszDst - pszData + 1);
			_ASSERTE(cchData > 2 && pszData[cchData-2] == L'\n' && pszData[cchData-3] != 0 && pszData[cchData]);
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
