
/*
Copyright (c) 2009-present Maximus5
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

// Return true if "SetEnvironmentVariable" was processed
// if (bDoSet==false) - just skip all "set" commands
// Supported commands:
//  set abc=val
//  "set PATH=C:\Program Files;%PATH%"
//  chcp [utf8|ansi|oem|<cp_no>]
//  title "Console init title"
bool ProcessSetEnvCmd(LPCWSTR& asCmdLine, CProcessEnvCmd* pSetEnv /*= nullptr*/, CStartEnvBase* pDoSet /*= nullptr*/)
{
	LPCWSTR lsCmdLine = asCmdLine;

	CProcessEnvCmd tmp;
	CProcessEnvCmd* lpSetEnv = pSetEnv ? pSetEnv : &tmp;

	lpSetEnv->AddCommands(asCmdLine, &lsCmdLine);

	const bool bEnvChanged = (lpSetEnv->mn_SetCount != 0);

	// Ask to be changed?
	if (pDoSet)
		lpSetEnv->Apply(pDoSet);

	// Run simple "cmd" if there were no actual command?
	_ASSERTE(lsCmdLine && *lsCmdLine);

	// Return naked command
	asCmdLine = lsCmdLine ? lsCmdLine : L"";

	// Fin, we must have something to execute
	_ASSERTE(asCmdLine && *asCmdLine);

	return bEnvChanged;
}


CStartEnvTitle::CStartEnvTitle(wchar_t** ppszTitle)
	: ppszTitle_(ppszTitle)
	, psTitle_(nullptr)
{
}

CStartEnvTitle::CStartEnvTitle(CEStr* psTitle)
	: ppszTitle_(nullptr)
	, psTitle_(psTitle)
{
}

// ReSharper disable once CppParameterMayBeConst
void CStartEnvTitle::Title(LPCWSTR asTitle)
{
	if (!asTitle || !*asTitle)
	{
		_ASSERTE(asTitle && *asTitle);
		return;
	}

	if (ppszTitle_)
	{
		SafeFree(*ppszTitle_);
		*ppszTitle_ = lstrdup(asTitle);
	}
	else if (psTitle_)
	{
		psTitle_->Set(asTitle);
	}
	else
	{
		_ASSERTE(ppszTitle_ || psTitle_);
	}
}



CProcessEnvCmd::CProcessEnvCmd()
	: mch_Total(0)
	, mn_SetCount(0)
	, mp_CmdTitle(nullptr)
	, mp_CmdChCp(nullptr)
{
}

CProcessEnvCmd::~CProcessEnvCmd()
{
	for (auto* pCommand : m_Commands)
	{
		if (pCommand)
		{
			SafeFree(pCommand->pszName);
			SafeFree(pCommand->pszValue);
			SafeFree(pCommand);
		}
	}
	m_Commands.clear();
}

// May come from Task or ConEmu's -run switch
// or from Setting\Environment page where one line is a single command (bAlone == true)
bool CProcessEnvCmd::AddCommands(LPCWSTR asCommands, LPCWSTR* ppszEnd/*= nullptr*/, bool bAlone /*= false*/, INT_PTR anBefore /*= -1*/)
{
	bool bNew = false;
	LPCWSTR lsCmdLine = asCommands;
	CmdArg lsSet, lsAmp, lsCmd;

	if (ppszEnd)
		*ppszEnd = asCommands;

	// Example: "set PATH=C:\Program Files;%PATH%" & set abc=def & cmd
	while ((lsCmdLine = NextArg(lsCmdLine, lsSet)))
	{
		bool bTokenOk = false;
		wchar_t* lsNameVal = nullptr;

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
				LPCWSTR pszAmp = bAlone ? nullptr : wcschr(lsCmdLine, L'&');
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
				lsSet.Clear(); // to avoid debug asserts
				lsSet.Set(lsCmdLine, pszValEnd - lsCmdLine);
				lsCmdLine = SkipNonPrintable(pszAmp); // Leave possible '&' at pointer
				bProcessed = true;
			}
			// OK, lets get token like "name=var value"
			if (!bProcessed)
			{
				bProcessed = (lsCmdLine = NextArg(lsCmdLine, lsSet));
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
				lsNameVal = const_cast<wchar_t*>(psz);
			}
		}
		// Support "alias token=value" too
		else if (lstrcmpni(lsSet, L"alias ", 6) == 0)
		{
			lsCmd = L"alias";
			LPCWSTR psz = SkipNonPrintable(lsSet.ms_Val+6);
			if (wcschr(psz, L'=') > psz)
			{
				lsNameVal = const_cast<wchar_t*>(psz);
			}
		}
		// Process "chcp <cp>" too
		else if (lstrcmpi(lsSet, L"chcp") == 0)
		{
			lsCmd = L"chcp";
			if ((lsCmdLine = NextArg(lsCmdLine, lsSet)))
			{
				UINT nCP = GetCpFromString(lsSet);
				if (nCP > 0 && nCP <= 0xFFFF)
				{
					bTokenOk = true;
					_ASSERTE(lsNameVal == nullptr);
					if (mp_CmdChCp)
					{
						SafeFree(mp_CmdChCp->pszName);
						mp_CmdChCp->pszName = lstrdup(lsSet);
					}
					else
					{
						bNew |= (nullptr != Add(lsCmd, lsSet, nullptr, anBefore));
					}
				}
			}
		}
		// Change title without need of cmd.exe
		else if (lstrcmpi(lsSet, L"title") == 0)
		{
			lsCmd = L"title";
			if ((lsCmdLine = NextArg(lsCmdLine, lsSet)))
			{
				bTokenOk = true;
				_ASSERTE(lsNameVal == nullptr);
				if (mp_CmdTitle)
				{
					SafeFree(mp_CmdTitle->pszName);
					mp_CmdTitle->pszName = lstrdup(lsSet);
				}
				else
				{
					bNew |= (nullptr != Add(lsCmd, lsSet, nullptr, anBefore));
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
			while (lsCmdLine && *lsCmdLine == L'-')
			{
				if (!((lsCmdLine = NextArg(lsCmdLine, lsSet))))
					break;
				lstrmerge(&lsSwitches.ms_Val, lsSwitches.IsEmpty() ? nullptr : L" ", lsSet.ms_Val);
			}
			// Rest arguments are expected to be processed text or file
			CmdArg lsAdd;
			while ((lsCmdLine = NextArg(lsCmdLine, lsSet)))
			{
				bTokenOk = true;
				lstrmerge(&lsAdd.ms_Val,
					lsAdd.IsEmpty() ? nullptr : L" ",
					lsSet.m_bQuoted ? L"\"" : nullptr,
					lsSet.ms_Val,
					lsSet.m_bQuoted ? L"\"" : nullptr);
				lsCmdLine = SkipNonPrintable(lsCmdLine);
				if (!*lsCmdLine || (*lsCmdLine == L'&') || (*lsCmdLine == L'|'))
					break;

			}
			if (!lsAdd.IsEmpty())
			{
				bNew |= (nullptr != Add(lsCmd, lsSwitches, lsAdd, anBefore));
			}
		}
		else
		{
			lsCmd.Clear();
		}

		// Well, known command was detected. What is next?
		if (lsNameVal || bTokenOk)
		{
			lsAmp.LoadPosFrom(lsSet);
			if (!((lsCmdLine = NextArg(lsCmdLine, lsAmp))))
			{
				// End of command? User called only "set" without following app? Run simple "cmd" in that case
				_ASSERTE(bAlone || (lsCmdLine!=nullptr && *lsCmdLine==0));
				bTokenOk = true; // And process SetEnvironmentVariable
			}
			else
			{
				if (lstrcmp(lsAmp, L"&") == 0)
				{
					// Only simple conveyor is supported!
					bTokenOk = true; // And process SetEnvironmentVariable
				}
				// Update last pointer (debug and asserts purposes)
				lsSet.LoadPosFrom(lsAmp);
			}
		}

		if (!bTokenOk)
		{
			break; // Stop processing command line
		}
		else if (lsNameVal)
		{
			// And split name/value
			_ASSERTE(lsNameVal!=nullptr);

			wchar_t* pszEq = wcschr(lsNameVal, L'=');
			if (!pszEq)
			{
				_ASSERTE(pszEq!=nullptr);
				break;
			}

			*(pszEq++) = 0;

			bNew |= (nullptr != Add(lsCmd, lsNameVal, pszEq ? pszEq : L"", anBefore));
		}

		// Remember processed position
		if (ppszEnd)
		{
			*ppszEnd = lsCmdLine;
		}
	} // end of "while ((lsCmdLine = NextArg(lsCmdLine, lsSet)))"

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
		const auto* const pszName = pszSrc;
		_ASSERTE(wcschr(pszName, L'=') == nullptr);
		const auto* const pszValue = pszName + lstrlen(pszName) + 1;
		const auto* const pszNext = pszValue + lstrlen(pszValue) + 1;
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

	while ((pszLines = NextLine(pszLines, lsLine)))
	{
		// Skip empty lines
		const auto* const pszLine = SkipNonPrintable(lsLine);
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
		if (AddCommands(pszLine, nullptr, true, nBefore) && bPriority)
			nBefore++;
	}
}

CProcessEnvCmd::Command* CProcessEnvCmd::Add(LPCWSTR asCmd, LPCWSTR asName, LPCWSTR asValue, INT_PTR anBefore)
{
	if (!asCmd || !*asCmd)
	{
		_ASSERTE(asCmd && *asCmd);
		return nullptr;
	}

	if ((lstrcmp(asCmd, L"echo") == 0) || (lstrcmp(asCmd, L"type") == 0))
	{
		// These are intended to be an options for echo/type
		if (asName == nullptr)
			asName = L"";
	}
	else
	{
		if (!asName || !*asName)
		{
			_ASSERTE(asName && *asName);
			return nullptr;
		}
	}

	Command* p = static_cast<Command*>(malloc(sizeof(Command)));
	if (!p)
		return nullptr;

	wcscpy_c(p->szCmd, asCmd);
	p->pszName = lstrdup(asName);
	p->pszValue = (asValue && *asValue) ? lstrdup(asValue) : nullptr;

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
		_ASSERTE(mp_CmdChCp==nullptr);
		mp_CmdChCp = p;
		cchAdd = 4; // chcp "cp"\0
	}
	else if (lstrcmp(asCmd, L"title") == 0)
	{
		_ASSERTE(mp_CmdTitle==nullptr);
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
		return nullptr;
	}

	mch_Total += (lstrlen(asCmd) + cchAdd + lstrlen(asName) + (asValue ? lstrlen(asValue) : 0));

	m_Commands.insert(anBefore, p);

	return p;
}

bool CProcessEnvCmd::Apply(CStartEnvBase* pSetEnv)
{
	bool bEnvChanged = false;

	if (!pSetEnv)
	{
		_ASSERTE(pSetEnv!=nullptr);
		return false;
	}

	for (auto* pCommand : m_Commands)
	{
		if (lstrcmp(pCommand->szCmd, L"set") == 0)
		{
			bEnvChanged = true;
			// Expand value
			pSetEnv->Set(pCommand->pszName, pCommand->pszValue);
		}
		else if (lstrcmpi(pCommand->szCmd, L"alias") == 0)
		{
			// nullptr will remove alias
			// We set aliases for "cmd.exe" executable, as Far Manager supports too
			pSetEnv->Alias(pCommand->pszName, pCommand->pszValue);
		}
		else if (lstrcmp(pCommand->szCmd, L"chcp") == 0)
		{
			pSetEnv->ChCp(pCommand->pszName);
		}
		else if (lstrcmp(pCommand->szCmd, L"title") == 0)
		{
			pSetEnv->Title(pCommand->pszName);
		}
		else if (lstrcmp(pCommand->szCmd, L"echo") == 0)
		{
			pSetEnv->Echo(pCommand->pszName, pCommand->pszValue);
		}
		else if (lstrcmp(pCommand->szCmd, L"type") == 0)
		{
			pSetEnv->Type(pCommand->pszName, pCommand->pszValue);
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
	wchar_t* pszData = nullptr;
	size_t cchData = 0;

	if (mch_Total)
	{
		pszData = static_cast<wchar_t*>(malloc((mch_Total + 1) * sizeof(*pszData)));
		if (pszData)
		{
			wchar_t* pszDst = pszData;

			for (auto* pCommand : m_Commands)
			{
				if ((lstrcmp(pCommand->szCmd, L"set") == 0) || (lstrcmp(pCommand->szCmd, L"alias") == 0))
				{
					cpyadv(pszDst, pCommand->szCmd);
					*(pszDst++) = L' ';
					*(pszDst++) = L'\"';
					cpyadv(pszDst, pCommand->pszName);
					*(pszDst++) = L'='; // name=value
					if (pCommand->pszValue)
						cpyadv(pszDst, pCommand->pszValue);
					else
						*pszDst = 0;
					*(pszDst++) = L'\"';
					*(pszDst++) = L'\n';
				}
				else if (lstrcmp(pCommand->szCmd, L"chcp") == 0)
				{
					cpyadv(pszDst, L"chcp");
					*(pszDst++) = L' ';
					*(pszDst++) = L'\"';
					cpyadv(pszDst, pCommand->pszName);
					*(pszDst++) = L'\"';
					*(pszDst++) = L'\n';
				}
				else if (lstrcmp(pCommand->szCmd, L"title") == 0)
				{
					cpyadv(pszDst, L"title");
					*(pszDst++) = L' ';
					*(pszDst++) = L'\"';
					cpyadv(pszDst, pCommand->pszName);
					*(pszDst++) = L'\"';
					*(pszDst++) = L'\n';
				}
				else if ((lstrcmp(pCommand->szCmd, L"echo") == 0) || (lstrcmp(pCommand->szCmd, L"type") == 0))
				{
					cpyadv(pszDst, pCommand->szCmd);
					*(pszDst++) = L' ';
					if (pCommand->pszName && *pCommand->pszName)
					{
						// type/echo options (like in `ConEmuC -t/-e ...`)
						cpyadv(pszDst, pCommand->pszName);
						*(pszDst++) = L' ';
					}
					//*(pszDst++) = L'\"';
					cpyadv(pszDst, pCommand->pszValue);
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

// ReSharper disable once CppInconsistentNaming, IdentifierTypo
void CProcessEnvCmd::cpyadv(wchar_t*& pszDst, LPCWSTR asStr)
{
	if (!asStr || !*asStr)
		return;
	const size_t nLen = wcslen(asStr);
	memmove(pszDst, asStr, (nLen+1)*sizeof(*pszDst));
	pszDst += nLen; // set pointer to \0
}
