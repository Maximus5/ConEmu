
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

#include "AboutDlg.h"
#include "ConEmu.h"
#include "ConEmuStart.h"
#include "Inside.h"
#include "Options.h"
#include "OptionsClass.h"
#include "SetCmdTask.h"

#include "../common/WFiles.h"

#define DEBUGSTRSTARTUP(s) DEBUGSTR(WIN3264TEST(L"ConEmu.exe: ",L"ConEmu64.exe: ") s L"\n")

// *****************
CESwitch::CESwitch()
{
	Str = NULL;
	Type = sw_None;
	Exists = false;
}

CESwitch::~CESwitch()
{
	Clear();
}

// Helpers

void CESwitch::Clear()
{
	if (GetStr())
		free(Str);
	Str = NULL; // empty most wide variable from union
	Type = sw_None;
	Exists = false;
}

void CESwitch::Undefine()
{
	Exists = false;
}

CESwitch::operator bool()
{
	return GetBool();
}

CESwitch& CESwitch::operator=(bool NewVal)
{
	SetBool(NewVal);
	return *this;
}

CESwitch& CESwitch::operator=(int NewVal)
{
	SetInt(NewVal);
	return *this;
}

CESwitch& CESwitch::operator=(LPCWSTR NewVal)
{
	SetStr(NewVal);
	return *this;
}

bool CESwitch::GetBool()
{
	_ASSERTE(Type==sw_Simple);
	return (Exists && Bool);
}

void CESwitch::SetBool(bool NewVal)
{
	Bool = NewVal;
	Exists = true;
	if (Type != sw_Simple)
	{
		_ASSERTE(Type == sw_Simple || Type == sw_None);
		Type = sw_Simple;
	}
}

int CESwitch::GetInt()
{
	_ASSERTE(Type==sw_Int);
	return (Exists && (Type == sw_Int)) ? Int : 0;
}

void CESwitch::SetInt(int NewVal)
{
	Int = NewVal;
	Exists = true;
	if (Type != sw_Int)
	{
		_ASSERTE(Type == sw_Int || Type == sw_None);
		Type = sw_Int;
	}
}

void CESwitch::SetInt(LPCWSTR NewVal, int Radix /*= 10*/)
{
	wchar_t* EndPtr = NULL;
	int iVal = wcstol(NewVal, &EndPtr, Radix);
	SetInt(iVal);
}

LPCWSTR CESwitch::GetStr()
{
	if (!Exists || !(Type == sw_Str || Type == sw_EnvStr || Type == sw_PathStr))
		return NULL;
	if (!Str || !*Str)
		return NULL;
	return Str;
}

void CESwitch::SetStr(LPCWSTR NewVal, CESwitchType NewType /*= sw_Str*/)
{
	if (GetStr())
		free(Str);
	Str = (NewVal && *NewVal) ? lstrdup(NewVal) : NULL;
	Exists = true;
	if (Type != NewType)
	{
		_ASSERTE(Type == sw_Str || Type == sw_EnvStr || Type == sw_PathStr || Type == sw_None);
		Type = NewType;
	}
}


// ********************************************
CConEmuStart::CConEmuStart(CConEmuMain* pOwner)
{
	mp_ConEmu = pOwner;
	_ASSERTE(mp_ConEmu!=NULL);

	m_StartDetached = crb_Undefined;
	mb_ConEmuHere = false;
	mb_ForceQuitOnClose = false;
	isCurCmdList = false;
	SetDefaultCmd(L"far");
	mn_ShellExitCode = STILL_ACTIVE;

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
	SafeFree(opt.cmdLine);
}

void CConEmuStart::SetDefaultCmd(LPCWSTR asCmd)
{
	// !!! gpConEmu may be NULL due starting time !!!
	if (gpConEmu && gpConEmu->isMingwMode() && gpConEmu->isMSysStartup())
	{
		CEStr szSearch;
		FindBashLocation(szSearch);

		_wsprintf(szDefCmd, SKIPLEN(countof(szDefCmd))
			(wcschr(szSearch, L' ') != NULL)
				? L"\"%s\" --login -i" /* -new_console:n" */
				: L"%s --login -i" /* -new_console:n" */,
			(LPCWSTR)szSearch);
	}
	else
	{
		wcscpy_c(szDefCmd, asCmd ? asCmd : L"cmd");
	}
}

void CConEmuStart::SetCurCmd(LPCWSTR pszNewCmd, bool bIsCmdList)
{
	_ASSERTE((pszNewCmd || isCurCmdList) && szCurCmd.ms_Arg != pszNewCmd);

	szCurCmd.Set(pszNewCmd);
	isCurCmdList = bIsCmdList;
}

LPCTSTR CConEmuStart::GetCurCmd(bool *pIsCmdList /*= NULL*/)
{
	if (!szCurCmd.IsEmpty())
	{
		if (pIsCmdList)
			*pIsCmdList = isCurCmdList;
		return szCurCmd;
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

	// User've choosed default task?
	if (mp_ConEmu->mn_StartupFinished == CConEmuMain::ss_Started)
	{
		if ((pszCmd = GetDefaultTask()) != NULL)
			return pszCmd;
	}

	// Current command line, specified with "/cmd" or "/cmdlist" switches
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

	// User've choosed default task?
	if (mp_ConEmu->mn_StartupFinished <= CConEmuMain::ss_PostCreate2Called)
	{
		if ((pszCmd = GetDefaultTask()) != NULL)
			return pszCmd;
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

LPCTSTR CConEmuStart::GetDefaultTask()
{
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

	return NULL;
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

bool CConEmuStart::FindBashLocation(CEStr& lsBash)
{
	wchar_t szRoot[MAX_PATH+32], *pszSlash;
	wcscpy_c(szRoot, gpConEmu->ms_ConEmuExeDir);

	LPCWSTR pszPlaces[] = {
		L"\\msys\\1.0\\bin\\sh.exe",  // Msys/MinGW
		L"\\bin\\sh.exe",             // Git-Bash
		L"\\usr\\bin\\sh.exe",        // Git-For-Windows
		NULL
	};

	// Before ConEmu.exe was intended to be in /bin/ folder
	// With Git-For-Windows it may be places in /opt/bin/ subfolder
	// So we do searching with two steps
	for (size_t i = 0; i <= 1; i++)
	{
		pszSlash = wcsrchr(szRoot, L'\\');
		if (pszSlash)
			*pszSlash = 0;

		for (size_t j = 0; pszPlaces[j]; j++)
		{
			lsBash = JoinPath(szRoot, pszPlaces[j]);
			if (FileExists(lsBash))
			{
				return true;
			}
		}
	}

	// Last chance, without path
	lsBash = L"sh.exe";
	return false;
}

void CConEmuStart::ResetConman()
{
	HKEY hk = 0;
	DWORD dw = 0;

	// 24.09.2010 Maks - Только если ключ конмана уже создан!
	// сбросить CreateInNewEnvironment для ConMan
	//if (0 == RegCreateKeyEx(HKEY_CURRENT_USER, _T("Software\\HoopoePG_2x"),
	//        NULL, NULL, NULL, KEY_ALL_ACCESS, NULL, &hk, &dw))
	if (0 == RegOpenKeyEx(HKEY_CURRENT_USER, _T("Software\\HoopoePG_2x"), 0, KEY_ALL_ACCESS, &hk))
	{
		RegSetValueEx(hk, _T("CreateInNewEnvironment"), 0, REG_DWORD,
		              (LPBYTE)&(dw=0), sizeof(dw));
		RegCloseKey(hk);
	}
}

bool CConEmuStart::GetCfgParm(int& i, wchar_t*& curCommand, bool& Prm, wchar_t*& Val, int nMaxLen, bool bExpandAndDup /*= false*/)
{
	if (!curCommand || !*curCommand)
	{
		_ASSERTE(curCommand && *curCommand);
		return false;
	}

	// Сохраним, может для сообщения об ошибке понадобится
	wchar_t* pszName = curCommand;

	curCommand += _tcslen(curCommand) + 1; i++;

	int nLen = _tcslen(curCommand);

	if (nLen >= nMaxLen)
	{
		int nCchSize = nLen+100+_tcslen(pszName);
		wchar_t* psz = (wchar_t*)calloc(nCchSize,sizeof(wchar_t));
		if (psz)
		{
			_wsprintf(psz, SKIPLEN(nCchSize) L"Too long %s value (%i chars).\r\n", pszName, nLen);
			_wcscat_c(psz, nCchSize, curCommand);
			MBoxA(psz);
			free(psz);
		}
		return false;
	}

	// Ok
	Prm = true;

	// We need independent absolute file paths, Working dir changes during ConEmu session
	if (bExpandAndDup)
		Val = GetFullPathNameEx(curCommand);
	else
		Val = curCommand;

	return true;
}

bool CConEmuStart::GetCfgParm(int& i, wchar_t*& curCommand, CESwitch& Val, int nMaxLen, bool bExpandAndDup /*= false*/)
{
	if (Val.Type == sw_Str || Val.Type == sw_EnvStr || Val.Type == sw_PathStr)
	{
		SafeFree(Val.Str);
	}
	else
	{
		Val.Type = bExpandAndDup ? sw_EnvStr : sw_Str;
	}

	return GetCfgParm(i, curCommand, Val.Exists, Val.Str, nMaxLen, bExpandAndDup);
}

/* С командной строкой (GetCommandLineW) у нас засада */
/*

ShellExecute("open", "ShowArg.exe", "\"test1\" test2");
GetCommandLineW(): "T:\XChange\VCProject\TestRunArg\ShowArg.exe" "test1" test2

CreateProcess("ShowArg.exe", "\"test1\" test2");
GetCommandLineW(): "test1" test2

CreateProcess(NULL, "\"ShowArg.exe\" \"test1\" test2");
GetCommandLineW(): "ShowArg.exe" "test1" test2

*/

//Result:
//  cmdLine - указатель на буфер с аргументами (!) он будет освобожден через free(cmdLine)
//  cmdNew  - то что запускается (после аргумента /cmd)
//  params  - количество аргументов
//            0 - ком.строка пустая
//            (-1) - весь cmdNew должен быть ПРИКЛЕЕН к строке запуска по умолчанию
bool CConEmuStart::PrepareCommandLine(wchar_t*& cmdLine, wchar_t*& cmdNew, bool& isScript, int& params)
{
	params = 0;
	cmdNew = NULL;
	isScript = false;

	LPCWSTR pszCmdLine = GetCommandLine();
	int nInitLen = _tcslen(pszCmdLine);
	cmdLine = lstrdup(pszCmdLine);
	// Имя исполняемого файла (conemu.exe)
	const wchar_t* pszExeName = PointToName(gpConEmu->ms_ConEmuExe);
	wchar_t* pszExeNameOnly = lstrdup(pszExeName);
	wchar_t* pszDot = (wchar_t*)PointToExt(pszExeNameOnly);
	_ASSERTE(pszDot);
	if (pszDot) *pszDot = 0;

	wchar_t *pszNext = NULL, *pszStart = NULL, chSave = 0;

	if (*cmdLine == L' ')
	{
		// Исполняемого файла нет - сразу начинаются аргументы
		pszNext = NULL;
	}
	else if (*cmdLine == L'"')
	{
		// Имя между кавычками
		pszStart = cmdLine+1;
		pszNext = wcschr(pszStart, L'"');

		if (!pszNext)
		{
			MBoxA(L"Invalid command line: quotes are not balanced");
			SafeFree(pszExeNameOnly);
			return false;
		}

		chSave = *pszNext;
		*pszNext = 0;
	}
	else
	{
		pszStart = cmdLine;
		pszNext = wcschr(pszStart, L' ');

		if (!pszNext) pszNext = pszStart + _tcslen(pszStart);

		chSave = *pszNext;
		*pszNext = 0;
	}

	if (pszNext)
	{
		wchar_t* pszFN = wcsrchr(pszStart, L'\\');
		if (pszFN) pszFN++; else pszFN = pszStart;

		// Если первый параметр - наш conemu.exe или его путь - нужно его выбросить
		if (!lstrcmpi(pszFN, pszExeName) || !lstrcmpi(pszFN, pszExeNameOnly))
		{
			// Нужно отрезать
			INT_PTR nCopy = (nInitLen - (pszNext - cmdLine)) * sizeof(wchar_t);
			TODO("Проверить, чтобы длину корректно посчитать");

			if (nCopy > 0)
				memmove(cmdLine, pszNext+1, nCopy);
			else
				*cmdLine = 0;
		}
		else
		{
			*pszNext = chSave;
		}
	}

	// AI. Если первый аргумент начинается НЕ с '/' - считаем что эта строка должна полностью передаваться в
	// запускаемую программу (прилепляться к концу ком.строки по умолчанию)
	pszStart = cmdLine;

	while (*pszStart == L' ' || *pszStart == L'"') pszStart++;  // пропустить пробелы и кавычки

	if (*pszStart == 0)
	{
		params = 0;
		*cmdLine = 0;
		cmdNew = NULL;
		// Эта переменная нужна для того, чтобы conemu можно было перезапустить
		// из cmd файла с теми же аргументами (selfupdate)
		SetEnvironmentVariableW(L"ConEmuArgs", L"");
	}
	else
	{
		// Эта переменная нужна для того, чтобы conemu можно было перезапустить
		// из cmd файла с теми же аргументами (selfupdate)
		gpConEmu->mpsz_ConEmuArgs = lstrdup(SkipNonPrintable(cmdLine));
		SetEnvironmentVariableW(L"ConEmuArgs", gpConEmu->mpsz_ConEmuArgs);

		// Теперь проверяем наличие слеша
		if (*pszStart != L'/' && *pszStart != L'-' && !wcschr(pszStart, L'/'))
		{
			params = -1;
			cmdNew = cmdLine;
		}
		else
		{
			CEStr lsArg;
			wchar_t* pszSrc = cmdLine;
			wchar_t* pszDst = cmdLine;
			wchar_t* pszArgStart = NULL;
			params = 0;
			// Strip quotes, de-escape arguments
			while (0 == NextArg((const wchar_t**)&pszSrc, lsArg, (const wchar_t**)&pszArgStart))
			{
				// If command line contains "/cmd" - the trailer is used (without changes!) to create new process
				// Optional "/cmdlist cmd1 | cmd2 | ..." can be used to start bunch of consoles at once
				if ((lsArg.ms_Arg[0] == L'-') || (lsArg.ms_Arg[0] == L'/'))
				{
					if (lstrcmpi(lsArg.ms_Arg+1, L"cmd") == 0)
					{
						cmdNew = (wchar_t*)SkipNonPrintable(pszSrc);
						*pszDst = 0;
						break;
					}
					else if (lstrcmpi(lsArg.ms_Arg+1, L"cmdlist") == 0)
					{
						isScript = true;
						cmdNew = (wchar_t*)SkipNonPrintable(pszSrc);
						*pszDst = 0;
						break;
					}
				}

				// Historically. Command line was splitted into "Arg1\0Arg2\0Arg3\0\0"
				int iLen = lstrlen(lsArg.ms_Arg);
				wmemcpy(pszDst, lsArg.ms_Arg, iLen+1); // Include trailing zero
				pszDst += iLen+1;
				params++;
			}
		}
	}
	SafeFree(pszExeNameOnly);

	return true;
}

// Returns:
//   true  - continue normal startup
//   false - exit process with iResult code
bool CConEmuStart::ParseCommandLine(int& iResult)
{
//------------------------------------------------------------------------
///| Parsing the command line |///////////////////////////////////////////
//------------------------------------------------------------------------
	wchar_t* psUnknown = NULL;

	DEBUGSTRSTARTUP(L"Parsing command line");

	// [OUT] params = (-1), если первый аргумент не начинается с '/'
	// т.е. комстрока такая "ConEmu.exe c:\tools\far.exe",
	// а не такая "ConEmu.exe /cmd c:\tools\far.exe",
	if (!PrepareCommandLine(/*OUT*/opt.cmdLine, /*OUT*/opt.cmdNew, /*OUT*/opt.isScript, /*OUT*/opt.params))
	{
		iResult = 100;
		return false;
	}

	if (opt.params > 0)
	{
		TCHAR *curCommand = opt.cmdLine;
		TODO("Если первый (после запускаемого файла) аргумент начинается НЕ с '/' - завершить разбор параметров и не заменять '""' на пробелы");
		//if (params < 1) {
		//	curCommand = NULL;
		//}
		// Parse parameters.
		// Duplicated parameters are permitted, the first value is used.
		int i = 0;

		while ((i < opt.params) && curCommand && *curCommand)
		{
			bool lbNotFound = false;
			bool lbSwitchChanged = false;

			// ':' removed from checks because otherwise it will not warn
			// on invalid usage of "-new_console:a" for example
			if (*curCommand == L'-' && curCommand[1] && !wcspbrk(curCommand+1, L"\\//|.&<>^"))
			{
				// Seems this is to be the "switch" too
				*curCommand = L'/';
				lbSwitchChanged = true;
			}

			if (*curCommand == L'/')
			{
				if (!klstricmp(curCommand, _T("/autosetup")))
				{
					BOOL lbTurnOn = TRUE;

					if ((i + 1) >= opt.params)
					{
						iResult = 101;
						return false;
					}

					curCommand += _tcslen(curCommand) + 1; i++;

					if (*curCommand == _T('0'))
						lbTurnOn = FALSE;
					else
					{
						if ((i + 1) >= opt.params)
						{
							iResult = 101;
							return false;
						}

						curCommand += _tcslen(curCommand) + 1; i++;
						DWORD dwAttr = GetFileAttributes(curCommand);

						if (dwAttr == (DWORD)-1 || (dwAttr & FILE_ATTRIBUTE_DIRECTORY))
						{
							iResult = 102;
							return false;
						}
					}

					HKEY hk = NULL; DWORD dw;
					int nSetupRc = 100;

					if (0 != RegCreateKeyEx(HKEY_CURRENT_USER, _T("Software\\Microsoft\\Command Processor"),
										   0, NULL, 0, KEY_ALL_ACCESS, NULL, &hk, &dw))
					{
						iResult = 103;
						return false;
					}

					if (lbTurnOn)
					{
						size_t cchMax = _tcslen(curCommand);
						LPCWSTR pszArg1 = NULL;
						if ((i + 1) < opt.params)
						{
							// Здесь может быть "/GHWND=NEW"
							pszArg1 = curCommand + cchMax + 1;
							if (!*pszArg1)
								pszArg1 = NULL;
							else
								cchMax += _tcslen(pszArg1);
						}
						cchMax += 16; // + кавычки и пробелы всякие

						wchar_t* pszCmd = (wchar_t*)calloc(cchMax, sizeof(*pszCmd));
						_wsprintf(pszCmd, SKIPLEN(cchMax) L"\"%s\"%s%s%s", curCommand,
							pszArg1 ? L" \"" : L"", pszArg1 ? pszArg1 : L"", pszArg1 ? L"\"" : L"");


						if (0 == RegSetValueEx(hk, _T("AutoRun"), 0, REG_SZ, (LPBYTE)pszCmd,
											(DWORD)sizeof(TCHAR)*(_tcslen(pszCmd)+1))) //-V220
							nSetupRc = 1;

						free(pszCmd);
					}
					else
					{
						if (0==RegDeleteValue(hk, _T("AutoRun")))
							nSetupRc = 1;
					}

					RegCloseKey(hk);
					// сбросить CreateInNewEnvironment для ConMan
					ResetConman();
					iResult = nSetupRc;
					return false;
				}
				else if (!klstricmp(curCommand, _T("/bypass")))
				{
					// Этот ключик был придуман для прозрачного запуска консоли
					// в режиме администратора
					// (т.е. чтобы окно UAC нормально всплывало, но не мелькало консольное окно)
					// Но не получилось, пока требуются хэндлы процесса, а их не получается
					// передать в НЕ приподнятый процесс (исходный ConEmu GUI).

					if (!opt.cmdNew || !*opt.cmdNew)
					{
						DisplayLastError(L"Invalid cmd line. '/bypass' exists, '/cmd' not", -1);
						iResult = 100;
						return false;
					}

					// Information
					#ifdef _DEBUG
					STARTUPINFO siOur = {sizeof(siOur)};
					GetStartupInfo(&siOur);
					#endif

					STARTUPINFO si = {sizeof(si)};
					si.dwFlags = STARTF_USESHOWWINDOW;

					PROCESS_INFORMATION pi = {};

					BOOL b = CreateProcess(NULL, opt.cmdNew, NULL, NULL, TRUE, NORMAL_PRIORITY_CLASS, NULL, NULL, &si, &pi);
					if (b)
					{
						CloseHandle(pi.hProcess);
						CloseHandle(pi.hThread);
						iResult = 0;
						return false;
					}

					// Failed
					DisplayLastError(opt.cmdNew);
					iResult = 100;
					return false;
				}
				else if (!klstricmp(curCommand, _T("/demote")))
				{
					// Запуск процесса (ком.строка после "/demote") в режиме простого юзера,
					// когда текущий процесс уже запущен "под админом". "Понизить" текущие
					// привилегии просто так нельзя, поэтому запуск идет через TaskSheduler.

					if (!opt.cmdNew || !*opt.cmdNew)
					{
						DisplayLastError(L"Invalid cmd line. '/demote' exists, '/cmd' not", -1);
						iResult = 100;
						return false;
					}


					// Information
					#ifdef _DEBUG
					STARTUPINFO siOur = {sizeof(siOur)};
					GetStartupInfo(&siOur);
					#endif

					STARTUPINFO si = {sizeof(si)};
					PROCESS_INFORMATION pi = {};
					si.dwFlags = STARTF_USESHOWWINDOW;
					si.wShowWindow = SW_SHOWNORMAL;

					wchar_t szCurDir[MAX_PATH+1] = L"";
					GetCurrentDirectory(countof(szCurDir), szCurDir);

					BOOL b;
					DWORD nErr = 0;


					b = CreateProcessDemoted(opt.cmdNew, NULL, NULL, FALSE, NORMAL_PRIORITY_CLASS, NULL,
							szCurDir, &si, &pi, &nErr);


					if (b)
					{
						SafeCloseHandle(pi.hProcess);
						SafeCloseHandle(pi.hThread);
						iResult = 0;
						return false;
					}

					// If the error was not shown yet
					if (nErr) DisplayLastError(opt.cmdNew, nErr);
					iResult = 100;
					return false;
				}
				else if (!klstricmp(curCommand, _T("/multi")))
				{
					gpConEmu->AppendExtraArgs(curCommand);
					gpConEmu->opt.MultiConValue = true;
				}
				else if (!klstricmp(curCommand, _T("/nomulti")))
				{
					gpConEmu->AppendExtraArgs(curCommand);
					gpConEmu->opt.MultiConValue = false;
				}
				else if (!klstricmp(curCommand, _T("/visible")))
				{
					gpConEmu->opt.VisValue = true;
				}
				else if (!klstricmp(curCommand, _T("/ct")) || !klstricmp(curCommand, _T("/cleartype"))
					|| !klstricmp(curCommand, _T("/ct0")) || !klstricmp(curCommand, _T("/ct1")) || !klstricmp(curCommand, _T("/ct2")))
				{
					switch (curCommand[3])
					{
					case L'0':
						gpConEmu->opt.ClearTypeVal = NONANTIALIASED_QUALITY; break;
					case L'1':
						gpConEmu->opt.ClearTypeVal = ANTIALIASED_QUALITY; break;
					default:
						gpConEmu->opt.ClearTypeVal = CLEARTYPE_NATURAL_QUALITY;
					}
				}
				// имя шрифта
				else if (!klstricmp(curCommand, _T("/font")) && ((i + 1) < opt.params))
				{
					curCommand += _tcslen(curCommand) + 1; i++;

					if (!gpConEmu->opt.FontVal.Exists)
					{
						gpConEmu->opt.FontVal = curCommand;
						gpConEmu->AppendExtraArgs(L"/font", curCommand);
					}
				}
				// Высота шрифта
				else if (!klstricmp(curCommand, _T("/size")) && ((i + 1) < opt.params))
				{
					curCommand += _tcslen(curCommand) + 1; i++;

					if (!gpConEmu->opt.SizeVal.Exists)
					{
						gpConEmu->opt.SizeVal.SetInt(curCommand);
					}
				}
				#if 0
				//120714 - аналогичные параметры работают в ConEmuC.exe, а в GUI они и не работали. убрал пока
				else if (!klstricmp(curCommand, _T("/attach")) /*&& i + 1 < params*/)
				{
					//curCommand += _tcslen(curCommand) + 1; i++;
					if (!AttachPrm)
					{
						AttachPrm = true; AttachVal = -1;

						if ((i + 1) < params)
						{
							TCHAR *nextCommand = curCommand + _tcslen(curCommand) + 1;

							if (*nextCommand != _T('/'))
							{
								curCommand = nextCommand; i++;
								AttachVal = klatoi(curCommand);
							}
						}

						// интеллектуальный аттач - если к текущей консоли уже подцеплена другая копия
						if (AttachVal == -1)
						{
							HWND hCon = GetForegroundWindow();

							if (!hCon)
							{
								// консоли нет
								iResult = 100;
								return false;
							}
							else
							{
								TCHAR sClass[128];

								if (GetClassName(hCon, sClass, 128))
								{
									if (_tcscmp(sClass, VirtualConsoleClassMain)==0)
									{
										// Сверху УЖЕ другая копия ConEmu
										iResult = 1;
										return false;
									}

									// Если на самом верху НЕ консоль - это может быть панель проводника,
									// или другое плавающее окошко... Поищем ВЕРХНЮЮ консоль
									if (isConsoleClass(sClass))
									{
										wcscpy_c(sClass, RealConsoleClass);
										hCon = FindWindow(RealConsoleClass, NULL);
										if (!hCon)
											hCon = FindWindow(WineConsoleClass, NULL);

										if (!hCon)
										{
											iResult = 100;
											return false;
										}
									}

									if (isConsoleClass(sClass))
									{
										// перебрать все ConEmu, может кто-то уже подцеплен?
										HWND hEmu = NULL;

										while ((hEmu = FindWindowEx(NULL, hEmu, VirtualConsoleClassMain, NULL)) != NULL)
										{
											if (hCon == (HWND)GetWindowLongPtr(hEmu, GWLP_USERDATA))
											{
												// к этой консоли уже подцеплен ConEmu
												iResult = 1;
												return false;
											}
										}
									}
									else
									{
										// верхнее окно - НЕ консоль
										iResult = 100;
										return false;
									}
								}
							}

							gpSetCls->hAttachConWnd = hCon;
						}
					}
				}
				#endif
				// ADD fontname; by Mors
				else if (!klstricmp(curCommand, _T("/fontfile")) && ((i + 1) < opt.params))
				{
					bool bOk = false; wchar_t* pszFile = NULL;
					if (!GetCfgParm(i, curCommand, bOk, pszFile, MAX_PATH))
					{
						iResult = 100;
						return false;
					}
					gpConEmu->AppendExtraArgs(L"/fontfile", pszFile);
					gpSetCls->RegisterFont(pszFile, TRUE);
				}
				// Register all fonts from specified directory
				else if (!klstricmp(curCommand, _T("/fontdir")) && ((i + 1) < opt.params))
				{
					bool bOk = false; wchar_t* pszDir = NULL;
					if (!GetCfgParm(i, curCommand, bOk, pszDir, MAX_PATH))
					{
						iResult = 100;
						return false;
					}
					gpConEmu->AppendExtraArgs(L"/fontdir", pszDir);
					gpSetCls->RegisterFontsDir(pszDir);
				}
				else if (!klstricmp(curCommand, _T("/fs")))
				{
					gpConEmu->opt.WindowModeVal = rFullScreen;
				}
				else if (!klstricmp(curCommand, _T("/max")))
				{
					gpConEmu->opt.WindowModeVal = rMaximized;
				}
				else if (!klstricmp(curCommand, _T("/min"))
					|| !klstricmp(curCommand, _T("/mintsa"))
					|| !klstricmp(curCommand, _T("/starttsa")))
				{
					gpConEmu->WindowStartMinimized = true;
					if (klstricmp(curCommand, _T("/min")) != 0)
					{
						gpConEmu->WindowStartTsa = true;
						gpConEmu->WindowStartNoClose = (klstricmp(curCommand, _T("/mintsa")) == 0);
					}
				}
				else if (!klstricmp(curCommand, _T("/tsa")) || !klstricmp(curCommand, _T("/tray")))
				{
					gpConEmu->ForceMinimizeToTray = true;
				}
				else if (!klstricmp(curCommand, _T("/detached")))
				{
					gpConEmu->m_StartDetached = crb_On;
				}
				else if (!klstricmp(curCommand, _T("/here")))
				{
					gpConEmu->mb_ConEmuHere = true;
					gpConEmu->StoreWorkDir();
				}
				else if (!klstricmp(curCommand, _T("/noupdate")))
				{
					gpConEmu->DisableAutoUpdate = true;
				}
				else if (!klstricmp(curCommand, _T("/nokeyhook"))
					|| !klstricmp(curCommand, _T("/nokeyhooks"))
					|| !klstricmp(curCommand, _T("/nokeybhook"))
					|| !klstricmp(curCommand, _T("/nokeybhooks")))
				{
					gpConEmu->DisableKeybHooks = true;
				}
				else if (!klstricmp(curCommand, _T("/nocloseconfirm")))
				{
					gpConEmu->DisableCloseConfirm = true;
				}
				else if (!klstricmp(curCommand, _T("/nomacro")))
				{
					gpConEmu->DisableAllMacro = true;
				}
				else if (!klstricmp(curCommand, _T("/nohotkey"))
					|| !klstricmp(curCommand, _T("/nohotkeys")))
				{
					gpConEmu->DisableAllHotkeys = true;
				}
				else if (!klstricmp(curCommand, _T("/nodeftrm"))
					|| !klstricmp(curCommand, _T("/nodefterm")))
				{
					gpConEmu->DisableSetDefTerm = true;
				}
				else if (!klstricmp(curCommand, _T("/noregfont"))
					|| !klstricmp(curCommand, _T("/noregfonts")))
				{
					gpConEmu->DisableRegisterFonts = true;
				}
				else if (!klstricmp(curCommand, _T("/inside"))
					|| !lstrcmpni(curCommand, _T("/inside="), 8))
				{
					bool bRunAsAdmin = isPressed(VK_SHIFT);
					bool bSyncDir = false;
					LPCWSTR pszSyncFmt = NULL;

					gpConEmu->mb_ConEmuHere = true;
					gpConEmu->StoreWorkDir();

					if (curCommand[7] == _T('='))
					{
						bSyncDir = true;
						pszSyncFmt = curCommand+8; // \eCD /d %1 - \e - ESC, \b - BS, \n - ENTER, %1 - "dir", %2 - "bash dir"
					}

					CConEmuInside::InitInside(bRunAsAdmin, bSyncDir, pszSyncFmt, 0, NULL);
				}
				else if (!klstricmp(curCommand, _T("/insidepid")) && ((i + 1) < opt.params))
				{
					curCommand += _tcslen(curCommand) + 1; i++;

					bool bRunAsAdmin = isPressed(VK_SHIFT);

					wchar_t* pszEnd;
					// Здесь указывается PID, в который нужно внедриться.
					DWORD nInsideParentPID = wcstol(curCommand, &pszEnd, 10);
					if (nInsideParentPID)
					{
						CConEmuInside::InitInside(bRunAsAdmin, false, NULL, nInsideParentPID, NULL);
					}
				}
				else if (!klstricmp(curCommand, _T("/insidewnd")) && ((i + 1) < opt.params))
				{
					curCommand += _tcslen(curCommand) + 1; i++;
					if (curCommand[0] == L'0' && (curCommand[1] == L'x' || curCommand[1] == L'X'))
						curCommand += 2;
					else if (curCommand[0] == L'x' || curCommand[0] == L'X')
						curCommand ++;

					bool bRunAsAdmin = isPressed(VK_SHIFT);

					wchar_t* pszEnd;
					// Здесь указывается HWND, в котором нужно создаваться.
					HWND hParent = (HWND)wcstol(curCommand, &pszEnd, 16);
					if (hParent && IsWindow(hParent))
					{
						CConEmuInside::InitInside(bRunAsAdmin, false, NULL, 0, hParent);
					}
				}
				else if (!klstricmp(curCommand, _T("/icon")) && ((i + 1) < opt.params))
				{
					curCommand += _tcslen(curCommand) + 1; i++;

					if (!gpConEmu->opt.IconPrm.Exists && *curCommand)
					{
						gpConEmu->opt.IconPrm = true;
						gpConEmu->mps_IconPath = ExpandEnvStr(curCommand);
					}
				}
				else if (!klstricmp(curCommand, _T("/dir")) && ((i + 1) < opt.params))
				{
					curCommand += _tcslen(curCommand) + 1; i++;

					if (*curCommand)
					{
						// Например, "%USERPROFILE%"
						wchar_t* pszExpand = NULL;
						if (wcschr(curCommand, L'%') && ((pszExpand = ExpandEnvStr(curCommand)) != NULL))
						{
							gpConEmu->StoreWorkDir(pszExpand);
							SafeFree(pszExpand);
						}
						else
						{
							gpConEmu->StoreWorkDir(curCommand);
						}
					}
				}
				else if (!klstricmp(curCommand, _T("/updatejumplist")))
				{
					// Copy current Task list to Win7 Jump list (Taskbar icon)
					gpConEmu->mb_UpdateJumpListOnStartup = true;
				}
				else if (!klstricmp(curCommand, L"/log") || !klstricmp(curCommand, L"/log0"))
				{
					gpSetCls->isAdvLogging = 1;
				}
				else if (!klstricmp(curCommand, L"/log1") || !klstricmp(curCommand, L"/log2")
					|| !klstricmp(curCommand, L"/log3") || !klstricmp(curCommand, L"/log4"))
				{
					gpSetCls->isAdvLogging = (BYTE)(curCommand[4] - L'0'); // 1..4
				}
				else if (!klstricmp(curCommand, _T("/single")) || !klstricmp(curCommand, _T("/reuse")))
				{
					// "/reuse" switch to be remastered
					gpConEmu->AppendExtraArgs(curCommand);
					gpSetCls->SingleInstanceArg = sgl_Enabled;
				}
				else if (!klstricmp(curCommand, _T("/nosingle")))
				{
					gpConEmu->AppendExtraArgs(curCommand);
					gpSetCls->SingleInstanceArg = sgl_Disabled;
				}
				else if (!klstricmp(curCommand, _T("/quake"))
					|| !klstricmp(curCommand, _T("/quakeauto"))
					|| !klstricmp(curCommand, _T("/noquake")))
				{
					if (!klstricmp(curCommand, _T("/quake")))
						gpConEmu->opt.QuakeMode = 1;
					else if (!klstricmp(curCommand, _T("/quakeauto")))
						gpConEmu->opt.QuakeMode = 2;
					else
					{
						gpConEmu->opt.QuakeMode = 0;
						if (gpSetCls->SingleInstanceArg == sgl_Default)
							gpSetCls->SingleInstanceArg = sgl_Disabled;
					}
				}
				else if (!klstricmp(curCommand, _T("/showhide")) || !klstricmp(curCommand, _T("/showhideTSA")))
				{
					gpSetCls->SingleInstanceArg = sgl_Enabled;
					gpSetCls->SingleInstanceShowHide = !klstricmp(curCommand, _T("/showhide"))
						? sih_ShowMinimize : sih_ShowHideTSA;
				}
				else if (!klstricmp(curCommand, _T("/reset"))
					|| !klstricmp(curCommand, _T("/resetdefault"))
					|| !klstricmp(curCommand, _T("/basic")))
				{
					gpConEmu->opt.ResetSettings = true;
					if (!klstricmp(curCommand, _T("/resetdefault")))
					{
						gpSetCls->isFastSetupDisabled = true;
					}
					else if (!klstricmp(curCommand, _T("/basic")))
					{
						gpSetCls->isFastSetupDisabled = true;
						gpSetCls->isResetBasicSettings = true;
					}
				}
				else if (!klstricmp(curCommand, _T("/nocascade"))
					|| !klstricmp(curCommand, _T("/dontcascade")))
				{
					gpConEmu->AppendExtraArgs(curCommand);
					gpSetCls->isDontCascade = true;
				}
				else if (!klstricmp(curCommand, _T("/WndX")) || !klstricmp(curCommand, _T("/WndY"))
					|| !klstricmp(curCommand, _T("/WndW")) || !klstricmp(curCommand, _T("/WndWidth"))
					|| !klstricmp(curCommand, _T("/WndH")) || !klstricmp(curCommand, _T("/WndHeight")))
				{
					TCHAR ch = curCommand[4], *psz = NULL;
					CharUpperBuff(&ch, 1);
					if (!GetCfgParm(i, curCommand, gpConEmu->opt.SizePosPrm, psz, 32))
					{
						iResult = 100;
						return false;
					}

					// Direct X/Y implies /nocascade
					if (ch == _T('X') || ch == _T('Y'))
						gpSetCls->isDontCascade = true;

					switch (ch)
					{
					case _T('X'): gpConEmu->opt.sWndX = psz; break;
					case _T('Y'): gpConEmu->opt.sWndY = psz; break;
					case _T('W'): gpConEmu->opt.sWndW = psz; break;
					case _T('H'): gpConEmu->opt.sWndH = psz; break;
					}
				}
				else if ((!klstricmp(curCommand, _T("/Buffer")) || !klstricmp(curCommand, _T("/BufferHeight")))
					&& ((i + 1) < opt.params))
				{
					curCommand += _tcslen(curCommand) + 1; i++;

					if (!gpConEmu->opt.BufferHeightVal.Exists)
					{
						gpConEmu->opt.BufferHeightVal.SetInt(curCommand);

						if (gpConEmu->opt.BufferHeightVal.GetInt() < 0)
						{
							//setParent = true; -- Maximus5 - нефиг, все ручками
							gpConEmu->opt.BufferHeightVal = -gpConEmu->opt.BufferHeightVal.GetInt();
						}

						if (gpConEmu->opt.BufferHeightVal.GetInt() < LONGOUTPUTHEIGHT_MIN)
							gpConEmu->opt.BufferHeightVal = LONGOUTPUTHEIGHT_MIN;
						else if (gpConEmu->opt.BufferHeightVal.GetInt() > LONGOUTPUTHEIGHT_MAX)
							gpConEmu->opt.BufferHeightVal = LONGOUTPUTHEIGHT_MAX;
					}
				}
				else if (!klstricmp(curCommand, _T("/Config")) && ((i + 1) < opt.params))
				{
					//if (!ConfigPrm) -- используем последний из параметров, если их несколько
					if (!GetCfgParm(i, curCommand, gpConEmu->opt.ConfigVal, 127))
					{
						iResult = 100;
						return false;
					}
				}
				else if (!klstricmp(curCommand, _T("/Palette")) && ((i + 1) < opt.params))
				{
					//if (!ConfigPrm) -- используем последний из параметров, если их несколько
					if (!GetCfgParm(i, curCommand, gpConEmu->opt.PaletteVal, MAX_PATH))
					{
						iResult = 100;
						return false;
					}
				}
				else if (!klstricmp(curCommand, _T("/LoadRegistry")))
				{
					gpConEmu->AppendExtraArgs(curCommand);
					gpConEmu->opt.ForceUseRegistryPrm = true;
				}
				else if (!klstricmp(curCommand, _T("/LoadCfgFile")) && ((i + 1) < opt.params))
				{
					// используем последний из параметров, если их несколько
					if (!GetCfgParm(i, curCommand, gpConEmu->opt.LoadCfgFile, MAX_PATH, true))
					{
						iResult = 100;
						return false;
					}
				}
				else if (!klstricmp(curCommand, _T("/SaveCfgFile")) && ((i + 1) < opt.params))
				{
					// используем последний из параметров, если их несколько
					if (!GetCfgParm(i, curCommand, gpConEmu->opt.SaveCfgFile, MAX_PATH, true))
					{
						iResult = 100;
						return false;
					}
				}
				else if (!klstricmp(curCommand, _T("/GuiMacro")) && ((i + 1) < opt.params))
				{
					// выполняется только последний
					if (!GetCfgParm(i, curCommand, gpConEmu->opt.ExecGuiMacro, 0x8000, false))
					{
						iResult = 100;
						return false;
					}
				}
				else if (!klstricmp(curCommand, _T("/UpdateSrcSet")) && ((i + 1) < opt.params))
				{
					// используем последний из параметров, если их несколько
					if (!GetCfgParm(i, curCommand, gpConEmu->opt.UpdateSrcSet, MAX_PATH*4, false))
					{
						iResult = 100;
						return false;
					}
				}
				else if (!klstricmp(curCommand, _T("/AnsiLog")) && ((i + 1) < opt.params))
				{
					// используем последний из параметров, если их несколько
					if (!GetCfgParm(i, curCommand, gpConEmu->opt.AnsiLogPath, MAX_PATH-40, true))
					{
						iResult = 100;
						return false;
					}
				}
				else if (!klstricmp(curCommand, _T("/SetDefTerm")))
				{
					gpConEmu->opt.SetUpDefaultTerminal = true;
				}
				else if (!klstricmp(curCommand, _T("/Exit")))
				{
					gpConEmu->opt.ExitAfterActionPrm = true;
				}
				else if (!klstricmp(curCommand, _T("/QuitOnClose")))
				{
					gpConEmu->mb_ForceQuitOnClose = true;
				}
				else if (!klstricmp(curCommand, _T("/Title")) && ((i + 1) < opt.params))
				{
					bool bOk = false; wchar_t* pszTitle = NULL;
					if (!GetCfgParm(i, curCommand, bOk, pszTitle, 127))
					{
						iResult = 100;
						return false;
					}
					gpConEmu->SetTitleTemplate(pszTitle);
				}
				else if (!klstricmp(curCommand, _T("/FindBugMode")))
				{
					gpConEmu->mb_FindBugMode = true;
				}
				else if (!klstricmp(curCommand, _T("/debug")) || !klstricmp(curCommand, _T("/debugi")))
				{
					// These switches were already processed
				}
				else if (!klstricmp(curCommand, _T("/?")) || !klstricmp(curCommand, _T("/h")) || !klstricmp(curCommand, _T("/help")))
				{
					//MessageBox(NULL, pHelp, L"About ConEmu...", MB_ICONQUESTION);
					ConEmuAbout::OnInfo_About();
					iResult = -1;
					return false;
				}
				else
				{
					lbNotFound = true;

					if (lbSwitchChanged)
					{
						*curCommand = L'-';
					}
				}
			}

			if (lbNotFound && /*i>0 &&*/ !psUnknown && (*curCommand == L'-' || *curCommand == L'/'))
			{
				// ругнуться на неизвестную команду
				psUnknown = curCommand;
			}

			curCommand += _tcslen(curCommand) + 1; i++;
		}
	}

	if (gpSetCls->isAdvLogging)
	{
		DEBUGSTRSTARTUP(L"Creating log file");
		gpConEmu->CreateLog();
	}

	//if (setParentDisabled && (gpConEmu->setParent || gpConEmu->setParent2)) {
	//	gpConEmu->setParent=false; gpConEmu->setParent2=false;
	//}

	if (psUnknown)
	{
		DEBUGSTRSTARTUP(L"Unknown switch, exiting!");
		CEStr lsFail(lstrmerge(L"Unknown switch specified:\r\n", psUnknown));
		gpConEmu->LogString(lsFail, false, false);

		LPCWSTR pszNewConWarn = NULL;
		if (wcsstr(psUnknown, L"new_console") || wcsstr(psUnknown, L"cur_console"))
			pszNewConWarn = L"\r\n\r\n" L"Switch -new_console must be specified *after* /cmd or /cmdlist";

		CEStr lsMsg(lstrmerge(
			lsFail,
			pszNewConWarn,
			L"\r\n\r\n"
			L"Visit website to get thorough switches description:\r\n"
			CEGUIARGSPAGE
			L"\r\n\r\n"
			L"Or run ‘ConEmu.exe -?’ to get the brief."
			));

		MBoxA(lsMsg);
		iResult = 100;
		return false;
	}

	// Continue normal startup
	return true;
}
