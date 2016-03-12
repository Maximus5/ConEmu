
/*
Copyright (c) 2015-2016 Maximus5
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
#include "LngRc.h"
#include "Options.h"
#include "OptionsClass.h"
#include "SetCmdTask.h"

#include "../common/Monitors.h"
#include "../common/StartupEnvDef.h"
#include "../common/WFiles.h"

#define DEBUGSTRSTARTUP(s) DEBUGSTR(WIN3264TEST(L"ConEmu.exe: ",L"ConEmu64.exe: ") s L"\n")

// *****************
CESwitch::CESwitch(CESwitchType aType /*= sw_None*/)
{
	Str = NULL;
	Type = aType;
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
	_ASSERTE(Type==sw_Simple || (Type==sw_None && !Exists));
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
	_ASSERTE(Type==sw_Int || (Type==sw_None && !Exists));
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

	opt.params = 0;
	opt.isScript = false; // true if switch ‘-cmdlist’ was used

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
	_ASSERTE((pszNewCmd || isCurCmdList) && szCurCmd.ms_Val != pszNewCmd);

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
	if (mp_ConEmu->mn_StartupFinished >= CConEmuMain::ss_Started)
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
	if (mp_ConEmu->mn_StartupFinished < CConEmuMain::ss_Started)
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
			ms_DefNewTaskName = (LPCWSTR)pGrp->pszName;
			return ms_DefNewTaskName.ms_Val;
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
		L"\\msys\\1.0\\bin\\bash.exe",  // Msys/MinGW
		L"\\bin\\bash.exe",             // Git-Bash
		L"\\usr\\bin\\bash.exe",        // Git-For-Windows
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
	lsBash = L"bash.exe";
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

bool CConEmuStart::GetCfgParm(LPCWSTR& cmdLineRest, CESwitch& Val, int nMaxLen, bool bExpandAndDup /*= false*/)
{
	if (Val.Type == sw_Str || Val.Type == sw_EnvStr || Val.Type == sw_PathStr)
	{
		SafeFree(Val.Str);
	}
	else
	{
		Val.Type = bExpandAndDup ? sw_EnvStr : sw_Str;
	}
	Val.Exists = false;

	if (!cmdLineRest || !*cmdLineRest)
	{
		_ASSERTE(cmdLineRest && *cmdLineRest);
		return false;
	}

	// Сохраним, может для сообщения об ошибке понадобится
	LPCWSTR pszName = cmdLineRest;
	CEStr szGetCfgParmTemp;

	if (NextArg(&cmdLineRest, szGetCfgParmTemp) != 0)
	{
		return false;
	}

	LPCWSTR curCommand = szGetCfgParmTemp.ms_Val;
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

	// We need independent absolute file paths, Working dir changes during ConEmu session
	if (bExpandAndDup)
		Val.Str = GetFullPathNameEx(curCommand); // it allocates memory
	else
		Val.SetStr(curCommand, Val.Type);

	// Ok
	Val.Exists = (Val.Str && *Val.Str);

	return true;
}

bool CConEmuStart::GetCfgParm(LPCWSTR& cmdLineRest, bool& Prm, CESwitch& Val, int nMaxLen, bool bExpandAndDup /*= false*/)
{
	bool bRc = GetCfgParm(cmdLineRest, Val, nMaxLen, bExpandAndDup);
	Prm = Val.Exists;
	return bRc;
}

void CConEmuStart::ProcessConEmuArgsVar(LPCWSTR cmdLineRest)
{
	// Эта переменная нужна для того, чтобы conemu можно было перезапустить
	// из cmd файла с теми же аргументами (selfupdate)
	if (!cmdLineRest || !*cmdLineRest)
	{
		// Empty command line, nothing to set.
		// But we have to define the variable to be sure
		SetEnvironmentVariableW(L"ConEmuArgs", L"");
	}
	else
	{
		gpConEmu->mpsz_ConEmuArgs = lstrdup(cmdLineRest);
		SetEnvironmentVariableW(L"ConEmuArgs", gpConEmu->mpsz_ConEmuArgs);
	}
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


//------------------------------------------------------------------------
///| Parsing the command line |///////////////////////////////////////////
//------------------------------------------------------------------------
// Returns:
//   true  - continue normal startup
//   false - exit process with iResult code
bool CConEmuStart::ParseCommandLine(LPCWSTR pszCmdLine, int& iResult)
{
	bool bRc = false;
	iResult = 100;

	_ASSERTE(pszCmdLine!=NULL);
	opt.cmdLine.Set(pszCmdLine ? pszCmdLine : L"");

	// pszCmdLine *may* or *may not* start with our executable or full path to our executable
	LPCWSTR pszTemp = opt.cmdLine;
	LPCWSTR cmdLineRest = SkipNonPrintable(opt.cmdLine);
	LPCWSTR pszName, pszArgStart;
	LPCWSTR psUnknown = NULL;
	CEStr   szArg, szNext;
	CEStr   szExeName, szExeNameOnly;

	// Have to get our exectuable name and name without extension
	szExeName.Set(PointToName(gpConEmu->ms_ConEmuExe));
	szExeNameOnly.Set(szExeName);
	wchar_t* pszDot = (wchar_t*)PointToExt(szExeNameOnly.ms_Val);
	_ASSERTE(pszDot);
	if (pszDot) *pszDot = 0;


	// Check the first argument in the command line (most probably it will be our executable path/name)
	if (NextArg(&pszTemp, szArg) != 0)
	{
		_ASSERTE(FALSE && "GetCommandLine() is empty");
		// Treat as empty command line, allow to start
		bRc = true; iResult = 0;
		goto wrap;
	}
	pszName = PointToName(szArg);
	if ((lstrcmpi(pszName, szExeName) == 0)
		|| (lstrcmpi(pszName, szExeNameOnly) == 0))
	{
		// OK, our executable was specified properly in the command line
		_ASSERTE(*pszTemp != L' ');
		cmdLineRest = SkipNonPrintable(pszTemp);
	}


	// Set %ConEmuArgs% env var
	// It may be usefull if we need to restart ConEmu
	// from batch/script with the same arguments (selfupdate etc.)
	ProcessConEmuArgsVar(cmdLineRest);


	// Must be empty at the moment
	_ASSERTE(opt.cmdNew.IsEmpty());

	// Does the command line contain our switches?
	// Or we need to append all switches to starting shell?
	if (cmdLineRest && *cmdLineRest)
	{
		pszTemp = cmdLineRest;
		if (NextArg(&pszTemp, szArg) == 0)
		{
			if ((*szArg.ms_Val != L'/')
				&& (*szArg.ms_Val != L'-')
				/*&& !wcschr(szArg.ms_Val, L'/')*/
				)
			{
				// Save it for further use
				opt.cmdNew.Set(cmdLineRest);
				// And do not process it (no switches at all)
				cmdLineRest = NULL;
				opt.params = -1;
			}
		}
	}


	// Let parse the reset
	szArg.Empty();
	szNext.Empty();

	// Processing loop begin
	if (cmdLineRest && *cmdLineRest)
	{
		//TCHAR *curCommand = opt.cmdLine;
		//TODO("Если первый (после запускаемого файла) аргумент начинается НЕ с '/' - завершить разбор параметров и не заменять '""' на пробелы");
		//if (params < 1) {
		//	curCommand = NULL;
		//}
		// Parse parameters.
		// Duplicated parameters are permitted, the first value is used.
		//int i = 0;

		//while ((i < opt.params) && curCommand && *curCommand)
		while (NextArg(&cmdLineRest, szArg, &pszArgStart) == 0)
		{
			bool lbNotFound = false;

			// ':' removed from checks because otherwise it will not warn
			// on invalid usage of "-new_console:a" for example
			if (szArg.ms_Val[0] == L'-' && szArg.ms_Val[1] && !wcspbrk(szArg.ms_Val+1, L"\\//|.&<>^"))
			{
				// Seems this is to be the "switch" too
				// Use both notations ('-' and '/')
				*szArg.ms_Val = L'/';
			}

			LPCWSTR curCommand = szArg.ms_Val;

			#define NeedNextArg() \
				if (NextArg(&cmdLineRest, szNext) != 0) { iResult = 101; goto wrap; } \
				curCommand = szNext.ms_Val;


			#define AcquireCmdNew() \
				_ASSERTE(opt.cmdNew.IsEmpty()); \
				pszTemp = cmdLineRest; \
				if ((NextArg(&pszTemp, szNext) == 0) \
					&& (szNext.ms_Val[0] == L'-' || szNext.ms_Val[0] == L'/') \
					&& (lstrcmpi(szNext.ms_Val+1, L"cmd") == 0)) { \
					opt.cmdNew.Set(pszTemp); \
				} else { \
					opt.cmdNew.Set(cmdLineRest); \
				}


			if (*curCommand != L'/')
			{
				continue; // Try next switch?
			}
			else
			{
				opt.params++;

				if (!klstricmp(curCommand, _T("/autosetup")))
				{
					BOOL lbTurnOn = TRUE;

					NeedNextArg();

					if (*curCommand == _T('0'))
					{
						lbTurnOn = FALSE;
					}
					else
					{
						NeedNextArg();

						DWORD dwAttr = GetFileAttributes(curCommand);

						if (dwAttr == (DWORD)-1 || (dwAttr & FILE_ATTRIBUTE_DIRECTORY))
						{
							iResult = 102;
							goto wrap;
						}
					}

					HKEY hk = NULL; DWORD dw;
					int nSetupRc = 100;

					if (0 != RegCreateKeyEx(HKEY_CURRENT_USER, _T("Software\\Microsoft\\Command Processor"),
										   0, NULL, 0, KEY_ALL_ACCESS, NULL, &hk, &dw))
					{
						iResult = 103;
						goto wrap;
					}

					if (lbTurnOn)
					{
						size_t cchMax = _tcslen(curCommand);
						LPCWSTR pszArg1 = NULL;
						if (*cmdLineRest)
						{
							// May be ‘/GHWND=NEW’ or smth else
							pszArg1 = cmdLineRest;
							cchMax += _tcslen(pszArg1);
						}
						cchMax += 16; // + quotations, spaces and so on

						wchar_t* pszCmd = (wchar_t*)calloc(cchMax, sizeof(*pszCmd));
						_wsprintf(pszCmd, SKIPLEN(cchMax) L"\"%s\"%s%s%s", curCommand,
							pszArg1 ? L" \"" : L"", pszArg1 ? pszArg1 : L"", pszArg1 ? L"\"" : L"");


						if (0 == RegSetValueEx(hk, _T("AutoRun"), 0, REG_SZ, (LPBYTE)pszCmd,
											(DWORD)sizeof(*pszCmd)*(_tcslen(pszCmd)+1)))
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
					goto wrap;
				}
				else if (!klstricmp(curCommand, _T("/bypass"))
					|| !klstricmp(curCommand, _T("/demote")))
				{
					// -bypass
					// Этот ключик был придуман для прозрачного запуска консоли
					// в режиме администратора
					// (т.е. чтобы окно UAC нормально всплывало, но не мелькало консольное окно)
					// Но не получилось, пока требуются хэндлы процесса, а их не получается
					// передать в НЕ приподнятый процесс (исходный ConEmu GUI).

					// -demote
					// Запуск процесса (ком.строка после "/demote") в режиме простого юзера,
					// когда текущий процесс уже запущен "под админом". "Понизить" текущие
					// привилегии просто так нельзя, поэтому запуск идет через TaskSheduler.

					AcquireCmdNew();

					if (!opt.cmdNew || !*opt.cmdNew)
					{
						CEStr lsMsg(L"Invalid cmd line. '", curCommand, L"' exists, command line is empty");
						DisplayLastError(lsMsg, -1);
						goto wrap;
					}

					// Information
					#ifdef _DEBUG
					STARTUPINFO siOur = {sizeof(siOur)};
					GetStartupInfo(&siOur);
					#endif

					STARTUPINFO si = {sizeof(si)};
					PROCESS_INFORMATION pi = {};
					si.dwFlags = STARTF_USESHOWWINDOW;
					if (0 == klstricmp(curCommand, _T("/demote")))
						si.wShowWindow = SW_SHOWNORMAL;
					else
						si.wShowWindow = SW_HIDE;

					wchar_t szCurDir[MAX_PATH+1] = L"";
					GetCurrentDirectory(countof(szCurDir), szCurDir);

					BOOL b;
					DWORD nErr = 0;

					if (!klstricmp(curCommand, _T("/demote")))
					{
						b = CreateProcessDemoted(opt.cmdNew.ms_Val, NULL, NULL, FALSE, NORMAL_PRIORITY_CLASS, NULL,
							szCurDir, &si, &pi, &nErr);
					}
					else // -bypass
					{
						b = CreateProcess(NULL, opt.cmdNew.ms_Val, NULL, NULL, TRUE, NORMAL_PRIORITY_CLASS, NULL, NULL, &si, &pi);
						nErr = b ? 0 : GetLastError();
					}

					// Done, close handles, if they were opened
					if (b)
					{
						SafeCloseHandle(pi.hProcess);
						SafeCloseHandle(pi.hThread);
						iResult = 0;
						goto wrap;
					}

					// If the error was not shown yet
					if (nErr) DisplayLastError(opt.cmdNew, nErr);
					goto wrap;
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
				// Interface language
				else if (!klstricmp(curCommand, _T("/lng")))
				{
					NeedNextArg();

					if (!gpConEmu->opt.Language.Exists)
					{
						gpConEmu->opt.Language = curCommand;
						gpConEmu->AppendExtraArgs(L"/lng", curCommand);
					}
				}
				// Optional specific "ConEmu.l10n"
				else if (!klstricmp(curCommand, _T("/lngfile")))
				{
					NeedNextArg();

					if (!gpConEmu->opt.LanguageFile.Exists)
					{
						gpConEmu->opt.LanguageFile = curCommand;
						gpConEmu->AppendExtraArgs(L"/lngfile", curCommand);
					}
				}
				// имя шрифта
				else if (!klstricmp(curCommand, _T("/font")))
				{
					NeedNextArg();

					if (!gpConEmu->opt.FontVal.Exists)
					{
						gpConEmu->opt.FontVal = curCommand;
						gpConEmu->AppendExtraArgs(L"/font", curCommand);
					}
				}
				// Высота шрифта
				else if (!klstricmp(curCommand, _T("/size")))
				{
					NeedNextArg();

					if (!gpConEmu->opt.SizeVal.Exists)
					{
						gpConEmu->opt.SizeVal.SetInt(curCommand);
					}
				}
				// ADD fontname; by Mors
				else if (!klstricmp(curCommand, _T("/fontfile")))
				{
					CESwitch szFile(sw_Str);
					if (!GetCfgParm(cmdLineRest, szFile, MAX_PATH))
					{
						goto wrap;
					}
					gpConEmu->AppendExtraArgs(L"/fontfile", szFile.GetStr());
					gpSetCls->RegisterFont(szFile.GetStr(), TRUE);
				}
				// Register all fonts from specified directory
				else if (!klstricmp(curCommand, _T("/fontdir")))
				{
					CESwitch szDir(sw_Str);
					if (!GetCfgParm(cmdLineRest, szDir, MAX_PATH))
					{
						goto wrap;
					}
					gpConEmu->AppendExtraArgs(L"/fontdir", szDir.GetStr());
					gpSetCls->RegisterFontsDir(szDir.GetStr());
				}
				else if (!klstricmp(curCommand, _T("/fs")))
				{
					gpConEmu->opt.WindowModeVal = wmFullScreen;
				}
				else if (!klstricmp(curCommand, _T("/max")))
				{
					gpConEmu->opt.WindowModeVal = wmMaximized;
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
				else if (!klstricmp(curCommand, _T("/update")))
				{
					gpConEmu->opt.AutoUpdateOnStart = true;
				}
				else if (!klstricmp(curCommand, _T("/noupdate")))
				{
					// This one has more weight than AutoUpdateOnStart
					gpConEmu->opt.DisableAutoUpdate = true;
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
				else if (!klstricmp(curCommand, _T("/insidepid")))
				{
					NeedNextArg();

					bool bRunAsAdmin = isPressed(VK_SHIFT);

					wchar_t* pszEnd;
					// Здесь указывается PID, в который нужно внедриться.
					DWORD nInsideParentPID = wcstol(curCommand, &pszEnd, 10);
					if (nInsideParentPID)
					{
						CConEmuInside::InitInside(bRunAsAdmin, false, NULL, nInsideParentPID, NULL);
					}
				}
				else if (!klstricmp(curCommand, _T("/insidewnd")))
				{
					NeedNextArg();

					if (curCommand[0] == L'0' && (curCommand[1] == L'x' || curCommand[1] == L'X'))
						curCommand += 2;
					else if (curCommand[0] == L'x' || curCommand[0] == L'X')
						curCommand ++;

					bool bRunAsAdmin = isPressed(VK_SHIFT);

					wchar_t* pszEnd;
					// Здесь указывается HWND, в котором нужно создаваться.
					HWND hParent = (HWND)(DWORD_PTR)wcstoul(curCommand, &pszEnd, 16);
					if (hParent && IsWindow(hParent))
					{
						CConEmuInside::InitInside(bRunAsAdmin, false, NULL, 0, hParent);
					}
				}
				else if (!klstricmp(curCommand, _T("/icon")))
				{
					NeedNextArg();

					if (!gpConEmu->opt.IconPrm.Exists && *curCommand)
					{
						gpConEmu->opt.IconPrm = true;
						gpConEmu->mps_IconPath = ExpandEnvStr(curCommand);
					}
				}
				else if (!klstricmp(curCommand, _T("/dir")))
				{
					NeedNextArg();

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
				else if (!klstricmp(curCommand, _T("/DesktopMode")))
				{
					gpConEmu->opt.DesktopMode = true;
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
					TCHAR ch = curCommand[4];
					CharUpperBuff(&ch, 1);

					CESwitch psz(sw_Str); bool bParm = false;
					if (!GetCfgParm(cmdLineRest, bParm, psz, 32))
					{
						goto wrap;
					}
					gpConEmu->opt.SizePosPrm = true;

					// Direct X/Y implies /nocascade
					if (ch == _T('X') || ch == _T('Y'))
					{
						// TODO: isDontCascade must be in our opt struct !!!
						gpSetCls->isDontCascade = true;
					}

					switch (ch)
					{
					case _T('X'): gpConEmu->opt.sWndX.SetStr(psz.Str, sw_Str); break;
					case _T('Y'): gpConEmu->opt.sWndY.SetStr(psz.Str, sw_Str); break;
					case _T('W'): gpConEmu->opt.sWndW.SetStr(psz.Str, sw_Str); break;
					case _T('H'): gpConEmu->opt.sWndH.SetStr(psz.Str, sw_Str); break;
					}
				}
				else if (!klstricmp(curCommand, _T("/Monitor")))
				{
					CESwitch psz(sw_Str); bool bParm = false;
					if (!GetCfgParm(cmdLineRest, bParm, psz, 64))
					{
						goto wrap;
					}

					if ((gpConEmu->opt.Monitor.Mon = MonitorFromParam(psz.Str)) != NULL)
					{
						gpConEmu->opt.Monitor.Exists = true;
						gpConEmu->opt.Monitor.Type = sw_Int;
						gpStartEnv->hStartMon = gpConEmu->opt.Monitor.Mon;
					}
				}
				else if (!klstricmp(curCommand, _T("/Buffer")) || !klstricmp(curCommand, _T("/BufferHeight")))
				{
					NeedNextArg();

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
				else if (!klstricmp(curCommand, _T("/Config")))
				{
					// -- используем последний из параметров, если их несколько
					if (!GetCfgParm(cmdLineRest, gpConEmu->opt.ConfigVal, 127))
					{
						goto wrap;
					}
				}
				else if (!klstricmp(curCommand, _T("/Palette")))
				{
					// -- используем последний из параметров, если их несколько
					if (!GetCfgParm(cmdLineRest, gpConEmu->opt.PaletteVal, MAX_PATH))
					{
						goto wrap;
					}
				}
				else if (!klstricmp(curCommand, _T("/LoadRegistry")))
				{
					gpConEmu->AppendExtraArgs(curCommand);
					gpConEmu->opt.ForceUseRegistryPrm = true;
				}
				else if (!klstricmp(curCommand, _T("/LoadCfgFile")) || !klstricmp(curCommand, _T("/LoadXmlFile")))
				{
					// -- используем последний из параметров, если их несколько
					if (!GetCfgParm(cmdLineRest, gpConEmu->opt.LoadCfgFile, MAX_PATH, true))
					{
						goto wrap;
					}
				}
				else if (!klstricmp(curCommand, _T("/SaveCfgFile")) || !klstricmp(curCommand, _T("/SaveXmlFile")))
				{
					// -- используем последний из параметров, если их несколько
					if (!GetCfgParm(cmdLineRest, gpConEmu->opt.SaveCfgFile, MAX_PATH, true))
					{
						goto wrap;
					}
				}
				else if (!klstricmp(curCommand, _T("/GuiMacro")))
				{
					// -- выполняется только последний
					if (!GetCfgParm(cmdLineRest, gpConEmu->opt.ExecGuiMacro, 0x8000, false))
					{
						goto wrap;
					}
				}
				else if (!klstricmp(curCommand, _T("/UpdateSrcSet")))
				{
					// -- используем последний из параметров, если их несколько
					if (!GetCfgParm(cmdLineRest, gpConEmu->opt.UpdateSrcSet, MAX_PATH*4, false))
					{
						goto wrap;
					}
				}
				else if (!klstricmp(curCommand, _T("/AnsiLog")))
				{
					// -- используем последний из параметров, если их несколько
					if (!GetCfgParm(cmdLineRest, gpConEmu->opt.AnsiLogPath, MAX_PATH-40, true))
					{
						goto wrap;
					}
				}
				else if (!klstricmp(curCommand, _T("/SetDefTerm")))
				{
					gpConEmu->opt.SetUpDefaultTerminal = true;
				}
				else if (!klstricmp(curCommand, _T("/ZoneId")))
				{
					gpConEmu->opt.FixZoneId = true;
				}
				else if (!klstricmp(curCommand, _T("/Exit")))
				{
					gpConEmu->opt.ExitAfterActionPrm = true;
				}
				else if (!klstricmp(curCommand, _T("/QuitOnClose")))
				{
					gpConEmu->mb_ForceQuitOnClose = true;
				}
				else if (!klstricmp(curCommand, _T("/Title")))
				{
					bool bOk = false;
					CESwitch pszTitle(sw_Str);
					if (!GetCfgParm(cmdLineRest, bOk, pszTitle, 127))
					{
						goto wrap;
					}
					gpConEmu->SetTitleTemplate(pszTitle.GetStr());
				}
				else if (!klstricmp(curCommand, _T("/FindBugMode")))
				{
					gpConEmu->mb_FindBugMode = true;
				}
				else if (!klstricmp(curCommand, _T("/debug"))
					|| !klstricmp(curCommand, _T("/debugw"))
					|| !klstricmp(curCommand, _T("/debugi")))
				{
					// These switches were already processed
				}
				else if (!klstricmp(curCommand, _T("/?")) || !klstricmp(curCommand, _T("/h")) || !klstricmp(curCommand, _T("/help")))
				{
					if (gpLng) gpLng->Reload();
					ConEmuAbout::OnInfo_About();
					iResult = -1;
					goto wrap;
				}
				else if (!klstricmp(curCommand, _T("/cmd")))
				{
					opt.cmdNew.Set(SkipNonPrintable(cmdLineRest));
					opt.isScript = false;
					break;
				}
				else if (!klstricmp(curCommand, _T("/cmdlist")))
				{
					opt.cmdNew.Set(SkipNonPrintable(cmdLineRest));
					opt.isScript = true;
					break;
				}
				else
				{
					// Show error on unknown switch
					psUnknown = pszArgStart;
					break;
				}
			} // (*curCommand == L'/')

			// Avoid assertions in NextArg
			szArg.Empty(); szNext.Empty();
		} // while (NextArg(&cmdLineRest, szArg, &pszArgStart) == 0)
	}
	// Processing loop end

	if (gpSetCls->isAdvLogging)
	{
		DEBUGSTRSTARTUP(L"Creating log file");
		gpConEmu->CreateLog();
	}

	if (psUnknown)
	{
		DEBUGSTRSTARTUP(L"Unknown switch, exiting!");
		CEStr lsFail(L"Unknown switch specified:\r\n", psUnknown);
		gpConEmu->LogString(lsFail, false, false);

		LPCWSTR pszNewConWarn = NULL;
		LPCWSTR pszTestSwitch =
			(psUnknown[0] == L'-' || psUnknown[0] == L'/')
				? ((psUnknown[1] == L'-' || psUnknown[1] == L'/')
					? (psUnknown+2) : (psUnknown+1))
				: psUnknown;
		if ((lstrcmpni(pszTestSwitch, L"new_console", 11) == 0)
			|| (lstrcmpni(pszTestSwitch, L"cur_console", 11) == 0))
		{
			pszNewConWarn = L"\r\n\r\n" L"Switch -new_console must be specified *after* /cmd or /cmdlist";
		}

		CEStr lsMsg(
			lsFail,
			pszNewConWarn,
			L"\r\n\r\n"
			L"Visit website to get thorough switches description:\r\n"
			CEGUIARGSPAGE
			L"\r\n\r\n"
			L"Or run ‘ConEmu.exe -?’ to get the brief."
			);

		MBoxA(lsMsg);
		goto wrap;
	}

	// Continue normal startup
	bRc = true;
wrap:
	return bRc;
}
