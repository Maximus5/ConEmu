
/*
Copyright (c) 2014-present Maximus5
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
#define SHOWDEBUGSTR

#include "Header.h"
#include "../common/EnvVar.h"
#include "ConEmu.h" // Only for ParseScriptLineOptions - need to be moved...
#include "Hotkeys.h"
#include "SetCmdTask.h"
#include "Options.h"

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
		swprintf_c(szCmd, L"Group%i", (anCmdIndex+1));
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

// Returns `true` if changed
bool CommandTasks::SetGuiArg(LPCWSTR asGuiArg)
{
	if (!asGuiArg)
		asGuiArg = L"";

	if (0 == wcscmp(asGuiArg, (pszGuiArgs ? pszGuiArgs : L"")))
		return false;

	size_t iLen = wcslen(asGuiArg);

	if (!pszGuiArgs || (iLen >= cchGuiArgMax))
	{
		size_t cchNew = iLen+256;
		wchar_t* pszNew = (wchar_t*)malloc(cchNew*sizeof(wchar_t));
		if (!pszNew)
		{
			_ASSERTE(pszNew!=NULL);
			return false;
		}
		std::swap(pszNew, pszGuiArgs);
		cchGuiArgMax = cchNew;
		SafeFree(pszNew);
	}

	_wcscpy_c(pszGuiArgs, cchGuiArgMax, asGuiArg);

	return true;
}

// Returns `true` if changed
bool CommandTasks::SetCommands(LPCWSTR asCommands)
{
	if (!asCommands)
		asCommands = L"";

	if (0 == wcscmp(asCommands, (pszCommands ? pszCommands : L"")))
		return false;

	size_t iLen = wcslen(asCommands);

	if (!pszCommands || (iLen >= cchCmdMax))
	{
		size_t cchNew = iLen+1024;
		wchar_t* pszNew = (wchar_t*)malloc(cchNew*sizeof(wchar_t));
		if (!pszNew)
		{
			_ASSERTE(pszNew!=NULL);
			return false;
		}
		std::swap(pszNew, pszCommands);
		cchCmdMax = cchNew;
		SafeFree(pszNew);
	}

	_wcscpy_c(pszCommands, cchCmdMax, asCommands);

	return true;
}

void CommandTasks::ParseGuiArgs(RConStartArgsEx* pArgs) const
{
	if (!pArgs)
	{
		_ASSERTE(pArgs!=NULL);
		return;
	}

	LPCWSTR pszArgs = pszGuiArgs, pszOk = pszGuiArgs;
	CmdArg szArg;
	while ((pszArgs = NextArg(pszArgs, szArg)))
	{
		if (szArg.ms_Val[0] == L'-')
			szArg.ms_Val[0] = L'/';

		if (lstrcmpi(szArg, L"/dir") == 0)
		{
			if (!(pszArgs = NextArg(pszArgs, szArg)))
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
			if (!(pszArgs = NextArg(pszArgs, szArg)))
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
		else if ((lstrcmpi(szArg, L"/single") == 0) || (lstrcmpi(szArg, L"/reuse") == 0))
		{
			// Used in the other parts of code
		}
		else if (lstrcmpi(szArg, L"/nosingle") == 0)
		{
			// Force to run in new ConEmu window
			pArgs->aRecreate = cra_CreateWindow;
		}
		else if (lstrcmpi(szArg, L"/quake") == 0)
		{
			// Turn on quake mode in starting console?
			// Disallowed if current window is already in Quake mode.
			if (!gpSet->isQuakeStyle)
				lstrmerge(&pArgs->pszAddGuiArg, L"/quake ");
		}
		else if (lstrcmpi(szArg, L"/noquake") == 0)
		{
			// Disable quake in starting console
			lstrmerge(&pArgs->pszAddGuiArg, L"/noquake ");
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

bool CommandTasks::LoadCmdTask(SettingsBase* reg, int iIndex)
{
	bool lbRc = false;
	wchar_t szVal[32];
	int iCmdMax = 0, iCmdCount = 0;
	DWORD VkMod = 0;

	wchar_t* pszNameSet = NULL;
	if (iIndex >= 0)
	{
		if (!reg->Load(L"Name", &pszNameSet) || !*pszNameSet)
		{
			SafeFree(pszNameSet);
			goto wrap;
		}
	}
	else
	{
		//_ASSERTE(&pTask == &StartupTask);
		_ASSERTE(iIndex == -1 && "This must be StartupTask");
	}

	//pTask = (CommandTasks*)calloc(1, sizeof(CommandTasks));
	//if (!pTask)
	//{
	//	SafeFree(pszNameSet);
	//	goto wrap;
	//}

	_ASSERTE(pszName == NULL && pszGuiArgs == NULL && pszCommands == NULL);

	this->SetName(pszNameSet, iIndex);

	this->HotKey.HkType = chk_Task;
	if ((iIndex >= 0) && reg->Load(L"Hotkey", VkMod))
		this->HotKey.SetVkMod(VkMod);
	else
		this->HotKey.Key.Set();

	if (iIndex >= 0)
	{
		reg->Load(L"Flags", this->Flags);
	}

	if (!reg->Load(L"GuiArgs", &this->pszGuiArgs) || !*this->pszGuiArgs)
	{
		this->cchGuiArgMax = 0;
		SafeFree(this->pszGuiArgs);
	}
	else
	{
		this->cchGuiArgMax = _tcslen(this->pszGuiArgs)+1;
	}

	lbRc = true;

	if (reg->Load(L"Count", iCmdMax) && (iCmdMax > 0))
	{
		size_t nTotalLen = 1024; // с запасом, для редактирования через интерфейс
		wchar_t** pszCommands = (wchar_t**)calloc(iCmdMax, sizeof(wchar_t*));

		for (int j = 0; j < iCmdMax; j++)
		{
			swprintf_c(szVal, L"Cmd%i", j+1); // 1-based

			if (reg->Load(szVal, &(pszCommands[j])) && pszCommands[j] && *pszCommands[j])
			{
				iCmdCount++;
				nTotalLen += _tcslen(pszCommands[j])+8; // + ">\r\n\r\n"
			}
			else
				SafeFree(pszCommands[j]);
		}

		if ((iCmdCount > 0) && (nTotalLen))
		{
			this->cchCmdMax = nTotalLen+1;
			this->pszCommands = (wchar_t*)malloc(this->cchCmdMax*sizeof(wchar_t));
			if (this->pszCommands)
			{
				//this->nCommands = iCmdCount;

				int nActive = 0;
				reg->Load(L"Active", nActive); // 1-based

				wchar_t* psz = this->pszCommands; // dest script
				for (int k = 0; k < iCmdCount; k++)
				{
					bool bActive = false;
					gpConEmu->ParseScriptLineOptions(pszCommands[k], &bActive, NULL);

					if (((k+1) == nActive) && !bActive)
						*(psz++) = L'>';

					lstrcpy(psz, pszCommands[k]);
					SafeFree(pszCommands[k]);

					if ((k+1) < iCmdCount)
						lstrcat(psz, L"\r\n\r\n"); // для визуальности редактирования

					psz += lstrlen(psz);
				}
			}
		}

		SafeFree(pszCommands);
	}

wrap:
	SafeFree(pszNameSet);
	return lbRc;
}

bool CommandTasks::SaveCmdTask(SettingsBase* reg, bool isStartup)
{
	bool lbRc = true;
	int iCmdCount = 0, iOldCount = 0;
	int nActive = 0; // 1-based
	wchar_t szVal[32];

	reg->Load(L"Count", iOldCount);

	if (!isStartup)
	{
		reg->Save(L"Name", this->pszName);
		reg->Save(L"Flags", this->Flags);
		reg->Save(L"Hotkey", this->HotKey.GetVkMod());
	}

	reg->Save(L"GuiArgs", this->pszGuiArgs);

	MArray<wchar_t*> save_items;
	if (this->pszCommands)
	{
		wchar_t* pszCmd = this->pszCommands;
		while (*pszCmd == L'\r' || *pszCmd == L'\n' || *pszCmd == L'\t' || *pszCmd == L' ')
			pszCmd++;

		while (pszCmd && *pszCmd)
		{
			iCmdCount++;

			wchar_t* pszEnd = wcschr(pszCmd, L'\n');
			if (pszEnd && (*(pszEnd-1) == L'\r'))
				pszEnd--;
			wchar_t chSave = 0;
			if (pszEnd)
			{
				chSave = *pszEnd;
				*pszEnd = 0;
			}

			if (*pszCmd == L'>')
			{
				//pszCmd++;
				nActive = iCmdCount; // 1-based
			}

			save_items.push_back(lstrdup(pszCmd));

			if (pszEnd)
				*pszEnd = chSave;

			if (!pszEnd)
				break;
			pszCmd = pszEnd+1;
			while (*pszCmd == L'\r' || *pszCmd == L'\n' || *pszCmd == L'\t' || *pszCmd == L' ')
				pszCmd++;
		}

		reg->Save(L"Active", nActive); // 1-based
	}

	reg->Save(L"Count", iCmdCount);

	for (INT_PTR i = 0; i < save_items.size(); ++i)
	{
		wchar_t* pszData = save_items[i];
		swprintf_c(szVal, L"Cmd%i", i+1); // 1-based
		reg->Save(szVal, pszData);
		free(pszData);
	}
	for (INT_PTR i = save_items.size(); i < iOldCount; ++i)
	{
		swprintf_c(szVal, L"Cmd%i", i+1); // 1-based
		reg->Delete(szVal);
	}

	return lbRc;
}
