
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


#define HIDE_USE_EXCEPTION_INFO
#define SHOWDEBUGSTR

#include "Header.h"

#include "ConEmu.h"
#include "ConEmuStart.h"
#include "Options.h"
#include "SetCmdTask.h"

CConEmuStart::CConEmuStart(CConEmuMain* pOwner)
{
	mp_ConEmu = pOwner;
	_ASSERTE(mp_ConEmu!=NULL);

	mb_StartDetached = false;
	pszCurCmd = NULL; isCurCmdList = false;
	SetDefaultCmd(L"far");

	ZeroStruct(ourSI);
	ourSI.cb = sizeof(ourSI);
	try
	{
		GetStartupInfoW(&ourSI);
	}
	catch(...)
	{
		ZeroStruct(ourSI);
	}
}

CConEmuStart::~CConEmuStart()
{
	SafeFree(pszCurCmd);
}

void CConEmuStart::SetDefaultCmd(LPCWSTR asCmd)
{
	// !!! gpConEmu may be NULL due starting time !!!
	if (gpConEmu && gpConEmu->isMingwMode() && gpConEmu->isMSysStartup())
	{
		wchar_t szSearch[MAX_PATH+32], *pszFile;
		wcscpy_c(szSearch, gpConEmu->ms_ConEmuExeDir);
		pszFile = wcsrchr(szSearch, L'\\');
		if (pszFile)
			*pszFile = 0;
		pszFile = szSearch + _tcslen(szSearch);
		wcscat_c(szSearch, L"\\msys\\1.0\\bin\\sh.exe");
		if (!FileExists(szSearch))
		{
			// Git-Bash mode
			*pszFile = 0;
			wcscat_c(szSearch, L"\\bin\\sh.exe");
			if (!FileExists(szSearch))
			{
				// Last chance, without path
				wcscpy_c(szSearch, L"sh.exe");
			}
		}

		_wsprintf(szDefCmd, SKIPLEN(countof(szDefCmd))
			(wcschr(szSearch, L' ') != NULL)
				? L"\"%s\" --login -i" /* -new_console:n" */
				: L"%s --login -i" /* -new_console:n" */,
			szSearch);
	}
	else
	{
		wcscpy_c(szDefCmd, asCmd ? asCmd : L"cmd");
	}
}

void CConEmuStart::SetCurCmd(wchar_t*& pszNewCmd, bool bIsCmdList)
{
	_ASSERTE((pszNewCmd || isCurCmdList) && pszCurCmd != pszNewCmd);

	if (pszNewCmd != pszCurCmd)
		SafeFree(pszCurCmd);

	pszCurCmd = pszNewCmd;
	isCurCmdList = bIsCmdList;
}

LPCTSTR CConEmuStart::GetCurCmd(bool *pIsCmdList /*= NULL*/)
{
	if (pszCurCmd && *pszCurCmd)
	{
		if (pIsCmdList)
		{
			*pIsCmdList = isCurCmdList;
		}
		else
		{
			//_ASSERTE(isCurCmdList == false);
		}
		return pszCurCmd;
	}

	return NULL;
}

LPCTSTR CConEmuStart::GetCmd(bool *pIsCmdList, bool bNoTask /*= false*/)
{
	LPCWSTR pszCmd = NULL;

	// true - если передали "скрипт" (как бы содержимое Task вытянутое в строку)
	// например: "ConEmu.exe -cmdlist cmd ||| powershell ||| far"
	if (pIsCmdList)
		*pIsCmdList = false;

	// User choosed default task?
	int nGroup = 0;
	const CommandTasks* pGrp = NULL;
	while ((pGrp = gpSet->CmdTaskGet(nGroup++)))
	{
		if (pGrp->pszName && *pGrp->pszName
			&& (pGrp->Flags & CETF_NEW_DEFAULT))
		{
			ms_DefNewTaskName.Attach(lstrdup(pGrp->pszName));
			return ms_DefNewTaskName.ms_Arg;
		}
	}

	// Old style otherwise
	if ((pszCmd = GetCurCmd(pIsCmdList)) != NULL)
		return pszCmd;

	switch (gpSet->nStartType)
	{
	case 0:
		if (gpSet->psStartSingleApp && *gpSet->psStartSingleApp)
			return gpSet->psStartSingleApp;
		break;
	case 1:
		if (bNoTask)
			return NULL;
		if (gpSet->psStartTasksFile && *gpSet->psStartTasksFile)
			return gpSet->psStartTasksFile;
		break;
	case 2:
		if (bNoTask)
			return NULL;
		if (gpSet->psStartTasksName && *gpSet->psStartTasksName)
			return gpSet->psStartTasksName;
		break;
	case 3:
		if (bNoTask)
			return NULL;
		return AutoStartTaskName;
	}

	wchar_t* pszNewCmd = NULL;

	// Хорошо бы более корректно определить версию фара, но это не всегда просто
	// Например x64 файл сложно обработать в x86 ConEmu.
	DWORD nFarSize = 0;

	if (lstrcmpi(GetDefaultCmd(), L"far") == 0)
	{
		// Ищем фар. (1) В папке ConEmu, (2) в текущей директории, (2) на уровень вверх от папки ConEmu
		wchar_t szFar[MAX_PATH*2], *pszSlash;
		szFar[0] = L'"';
		wcscpy_add(1, szFar, gpConEmu->ms_ConEmuExeDir); // Теперь szFar содержит путь запуска программы
		pszSlash = szFar + _tcslen(szFar);
		_ASSERTE(pszSlash > szFar);
		BOOL lbFound = FALSE;

		// (1) В папке ConEmu
		if (!lbFound)
		{
			wcscpy_add(pszSlash, szFar, L"\\Far.exe");

			if (FileExists(szFar+1, &nFarSize))
				lbFound = TRUE;
		}

		// (2) в текущей директории
		if (!lbFound && lstrcmpi(gpConEmu->WorkDir(), gpConEmu->ms_ConEmuExeDir))
		{
			szFar[0] = L'"';
			wcscpy_add(1, szFar, gpConEmu->WorkDir());
			wcscat_add(1, szFar, L"\\Far.exe");

			if (FileExists(szFar+1, &nFarSize))
				lbFound = TRUE;
		}

		// (3) на уровень вверх
		if (!lbFound)
		{
			szFar[0] = L'"';
			wcscpy_add(1, szFar, gpConEmu->ms_ConEmuExeDir);
			pszSlash = szFar + _tcslen(szFar);
			*pszSlash = 0;
			pszSlash = wcsrchr(szFar, L'\\');

			if (pszSlash)
			{
				wcscpy_add(pszSlash+1, szFar, L"Far.exe");

				if (FileExists(szFar+1, &nFarSize))
					lbFound = TRUE;
			}
		}

		if (lbFound)
		{
			// 110124 - нафиг, если пользователю надо - сам или параметр настроит, или реестр
			//// far чаще всего будет установлен в "Program Files", поэтому для избежания проблем - окавычиваем
			//// Пока тупо - если far.exe > 1200K - считаем, что это Far2
			//wcscat_c(szFar, (nFarSize>1228800) ? L"\" /w" : L"\"");
			wcscat_c(szFar, L"\"");

			// Finally - Result
			pszNewCmd = lstrdup(szFar);
		}
		else
		{
			// Если Far.exe не найден рядом с ConEmu - запустить cmd.exe
			pszNewCmd = GetComspec(&gpSet->ComSpec);
			//wcscpy_c(szFar, L"cmd");
		}

	}
	else
	{
		// Simple Copy
		pszNewCmd = lstrdup(GetDefaultCmd());
	}

	SetCurCmd(pszNewCmd, false);

	return GetCurCmd(pIsCmdList);
}

// TODO: Option for default task?
LPCTSTR CConEmuStart::GetDefaultCmd()
{
	_ASSERTE(szDefCmd[0]!=0);
	return szDefCmd;
}

// Если ConEmu был запущен с ключом "/single /cmd xxx" то после окончания
// загрузки - сбросить команду, которая пришла из "/cmd" - загрузить настройку
void CConEmuStart::ResetCmdArg()
{
	//SingleInstanceArg = sgl_Default;
	//// Сбросить нужно только gpSet->psCurCmd, gpSet->psCmd не меняется - загружается только из настройки
	//SafeFree(gpSet->psCurCmd);
	//gpSet->isCurCmdList = false;

	if (isCurCmdList)
	{
		wchar_t* pszReset = NULL;
		SetCurCmd(pszReset, false);
	}
}
