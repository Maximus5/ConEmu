
/*
Copyright (c) 2014 Maximus5
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

#define HIDE_USE_EXCEPTION_INFO
#define SHOWDEBUGSTR

#include "Header.h"
#include "Hotkeys.h"
#include "CmdTasks.h"

void CommandTasks::FreePtr()
{
	SafeFree(pszName);
	cchNameMax = 0;
	SafeFree(pszGuiArgs);
	cchGuiArgMax = 0;
	SafeFree(pszCommands);
	cchCmdMax = 0;
    CommandTasks* p = this;
    SafeFree(p);
}

void CommandTasks::SetName(LPCWSTR asName, int anCmdIndex)
{
	wchar_t szCmd[16];
	if (anCmdIndex == -1)
	{
		wcscpy_c(szCmd, AutoStartTaskName);
		asName = szCmd;
	}
	else if (!asName || !*asName)
	{
		_wsprintf(szCmd, SKIPLEN(countof(szCmd)) L"Group%i", (anCmdIndex+1));
		asName = szCmd;
	}

	// Для простоты дальнейшей работы - имя должно быть заключено в угловые скобки
	size_t iLen = wcslen(asName);

	if (!pszName || ((iLen+2) >= cchNameMax))
	{
		SafeFree(pszName);

		cchNameMax = iLen+16;
		pszName = (wchar_t*)malloc(cchNameMax*sizeof(wchar_t));
		if (!pszName)
		{
			_ASSERTE(pszName!=NULL);
			return;
		}
	}

	if (asName[0] == TaskBracketLeft)
	{
		_wcscpy_c(pszName, iLen+1, asName);
	}
	else
	{
		*pszName = TaskBracketLeft;
		_wcscpy_c(pszName+1, iLen+1, asName);
	}
	if (asName[iLen-1] != TaskBracketRight)
	{
		iLen = wcslen(pszName);
		pszName[iLen++] = TaskBracketRight; pszName[iLen] = 0;
	}
}

void CommandTasks::SetGuiArg(LPCWSTR asGuiArg)
{
	if (!asGuiArg)
		asGuiArg = L"";

	size_t iLen = wcslen(asGuiArg);

	if (!pszGuiArgs || (iLen >= cchGuiArgMax))
	{
		SafeFree(pszGuiArgs);

		cchGuiArgMax = iLen+256;
		pszGuiArgs = (wchar_t*)malloc(cchGuiArgMax*sizeof(wchar_t));
		if (!pszGuiArgs)
		{
			_ASSERTE(pszGuiArgs!=NULL);
			return;
		}
	}

	_wcscpy_c(pszGuiArgs, cchGuiArgMax, asGuiArg);
}

void CommandTasks::SetCommands(LPCWSTR asCommands)
{
	if (!asCommands)
		asCommands = L"";

	size_t iLen = wcslen(asCommands);

	if (!pszCommands || (iLen >= cchCmdMax))
	{
		SafeFree(pszCommands);

		cchCmdMax = iLen+1024;
		pszCommands = (wchar_t*)malloc(cchCmdMax*sizeof(wchar_t));
		if (!pszCommands)
		{
			_ASSERTE(pszCommands!=NULL);
			return;
		}
	}

	_wcscpy_c(pszCommands, cchCmdMax, asCommands);
}

void CommandTasks::ParseGuiArgs(RConStartArgs* pArgs) const
{
	if (!pArgs)
	{
		_ASSERTE(pArgs!=NULL);
		return;
	}

	LPCWSTR pszArgs = pszGuiArgs, pszOk = pszGuiArgs;
	CmdArg szArg;
	while (0 == NextArg(&pszArgs, szArg))
	{
		if (szArg.ms_Arg[0] == L'-')
			szArg.ms_Arg[0] = L'/';

		if (lstrcmpi(szArg, L"/dir") == 0)
		{
			if (0 != NextArg(&pszArgs, szArg))
				break;
			if (*szArg)
			{
				wchar_t* pszExpand = NULL;

				// Например, "%USERPROFILE%"
				if (wcschr(szArg, L'%'))
				{
					pszExpand = ExpandEnvStr(szArg);
				}

				SafeFree(pArgs->pszStartupDir);
				pArgs->pszStartupDir = pszExpand ? pszExpand : lstrdup(szArg);
			}
		}
		else if (lstrcmpi(szArg, L"/icon") == 0)
		{
			if (0 != NextArg(&pszArgs, szArg))
				break;
			if (*szArg)
			{
				wchar_t* pszExpand = NULL;

				// Например, "%USERPROFILE%"
				if (wcschr(szArg, L'%'))
				{
					pszExpand = ExpandEnvStr(szArg);
				}

				SafeFree(pArgs->pszIconFile);
				pArgs->pszIconFile = pszExpand ? pszExpand : lstrdup(szArg);
			}
		}
		else if (lstrcmpi(szArg, L"/single") == 0)
		{
			// Used in the other parts of code
		}
		else
		{
			break;
		}

		pszOk = pszArgs;
	}
	// Errors notification
	if (pszOk && *pszOk)
	{
		wchar_t* pszErr = lstrmerge(L"Unsupported task parameters\r\nTask name: ", pszName, L"\r\nParameters: ", pszOk);
		MsgBox(pszErr, MB_ICONSTOP);
		SafeFree(pszErr);
	}
}
