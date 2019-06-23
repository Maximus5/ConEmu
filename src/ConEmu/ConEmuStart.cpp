
/*
Copyright (c) 2015-present Maximus5
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
#include "CreateProcess.h"
#include "Inside.h"
#include "LngRc.h"
#include "Options.h"
#include "OptionsClass.h"
#include "SetCmdTask.h"

#include "../common/EnvVar.h"
#include "../common/Monitors.h"
#include "../common/StartupEnvDef.h"
#include "../common/WFiles.h"
#include "../ConEmuCD/ExitCodes.h"

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

	m_StartDetached = crb_Undefined;
	mb_ConEmuHere = false;
	mb_ForceQuitOnClose = false;
	mb_SettingsRequested = false;
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

		swprintf_c(szDefCmd,
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
	if (mp_ConEmu->GetStartupStage() >= CConEmuMain::ss_Started)
	{
		if ((pszCmd = GetDefaultTask()) != NULL)
			return pszCmd;
	}

	// Current command line, specified with "-run","-runlist" switches (or old "-cmd and "-cmdlist")
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
	if (mp_ConEmu->GetStartupStage() < CConEmuMain::ss_Started)
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

// If ConEmu was started with "-single -run <shell with args>", than when loading finishes,
// drop the command that comes with "-run" and load command from settings
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
	CmdArg szGetCfgParmTemp;

	if (!(cmdLineRest = NextArg(cmdLineRest, szGetCfgParmTemp)))
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
			swprintf_c(psz, nCchSize/*#SECURELEN*/, L"Too long %s value (%i chars).\r\n", pszName, nLen);
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

// These variables `ConEmuArgs` and `ConEmuArgs2` are required
// to (re)start ConEmu instance from batch with same arguments
// Called from CConEmuStart::ParseCommandLine
void CConEmuStart::ProcessConEmuArgsVar()
{
	// Full command line: config switches AND -cmd/-cmdlist
	_ASSERTE(!opt.cmdLine.IsEmpty());

	// Don't set `NULL`, it would remove var from env block!
	LPCWSTR pszValue;

	pszValue = opt.cfgSwitches.IsEmpty() ? L"" : opt.cfgSwitches.ms_Val;
	SetEnvironmentVariableW(L"ConEmuArgs", pszValue);

	if (opt.runCommand.IsEmpty())
	{
		SetEnvironmentVariableW(L"ConEmuArgs2", L"");
		_ASSERTE(opt.cmdRunCommand.IsEmpty());
	}
	else
	{
		opt.cmdRunCommand.Attach(lstrmerge(opt.isScript ? L"-cmd " : L"-cmdlist ", opt.runCommand));
		_ASSERTE(!opt.cmdRunCommand.IsEmpty());
		SetEnvironmentVariableW(L"ConEmuArgs2", opt.cmdRunCommand);
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
	CmdArg  szArg, szNext;
	CEStr   szExeName, szExeNameOnly;

	// Set %ConEmuArgs% env var
	// It may be useful if we need to restart ConEmu
	// from batch/script with the same arguments (selfupdate etc.)
	LPCWSTR pszCopyToEnvStart = NULL;

	// Have to get our exectuable name and name without extension
	szExeName.Set(PointToName(gpConEmu->ms_ConEmuExe));
	szExeNameOnly.Set(szExeName);
	wchar_t* pszDot = (wchar_t*)PointToExt(szExeNameOnly.ms_Val);
	_ASSERTE(pszDot);
	if (pszDot) *pszDot = 0;


	// Check the first argument in the command line (most probably it will be our executable path/name)
	if (!(pszTemp = NextArg(pszTemp, szArg)))
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


	// Must be empty at the moment
	_ASSERTE(opt.runCommand.IsEmpty());

	// Does the command line contain our switches?
	// Or we need to append all switches to starting shell?
	if (cmdLineRest && *cmdLineRest)
	{
		pszTemp = cmdLineRest;
		if ((pszTemp = NextArg(pszTemp, szArg)))
		{
			if ((*szArg.ms_Val != L'/')
				&& (*szArg.ms_Val != L'-')
				/*&& !wcschr(szArg.ms_Val, L'/')*/
				)
			{
				// Save it for further use
				opt.runCommand.Set(cmdLineRest);
				// And do not process it (no switches at all)
				cmdLineRest = NULL;
				opt.params = -1;
			}
		}
	}

	struct RunAsAdmin
	{
		static bool Check(LPCWSTR asSwitch)
		{
			bool bRunAsAdmin = false; // isPressed(VK_SHIFT);
			return bRunAsAdmin;
		};
	};


	// Let parse the reset
	szArg.Empty();
	szNext.Empty();

	// Processing loop begin
	if (cmdLineRest && *cmdLineRest)
	{
		pszCopyToEnvStart = cmdLineRest;
		opt.cfgSwitches.Set(pszCopyToEnvStart);

		while ((cmdLineRest = NextArg(cmdLineRest, szArg, &pszArgStart)))
		{
			bool lbNotFound = false;

			TODO("Replace NeedNextArg with GetCfgParm?")
			#define NeedNextArg() \
				if (!(cmdLineRest = NextArg(cmdLineRest, szNext))) { iResult = CERR_CARGUMENT; goto wrap; }


			if (!szArg.IsPossibleSwitch())
			{
				// -- // continue; // Try next switch?

				// Show error on unknown switch
				psUnknown = pszArgStart;
				break;
			}

			// Main processing cycle
			{
				opt.params++;

				if (szArg.IsSwitch(L"-autosetup"))
				{
					BOOL lbTurnOn = TRUE;

					NeedNextArg();

					if (szNext.Compare(L"0") == 0)
					{
						lbTurnOn = FALSE;
					}
					else if (szNext.Compare(L"1") == 0)
					{
						NeedNextArg();

						DWORD dwAttr = GetFileAttributes(szNext);

						if (dwAttr == (DWORD)-1 || (dwAttr & FILE_ATTRIBUTE_DIRECTORY))
						{
							iResult = 102;
							goto wrap;
						}
					}
					else
					{
						iResult = CERR_CARGUMENT;
						goto wrap;
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
						size_t cchMax = szNext.GetLen();
						LPCWSTR pszArg1 = NULL;
						if (*cmdLineRest)
						{
							// May be ‘/GHWND=NEW’ or smth else
							pszArg1 = cmdLineRest;
							cchMax += _tcslen(pszArg1);
						}
						cchMax += 16; // + quotations, spaces and so on

						wchar_t* pszCmd = (wchar_t*)calloc(cchMax, sizeof(*pszCmd));
						swprintf_c(pszCmd, cchMax/*#SECURELEN*/, L"\"%s\"%s%s%s", szNext.ms_Val,
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
				else if (szArg.OneOfSwitches(L"-bypass", L"-apparent", L"-system:", L"-interactive:", L"-demote"))
				{
					// -bypass
					// Этот ключик был придуман для прозрачного запуска консоли
					// в режиме администратора
					// (т.е. чтобы окно UAC нормально всплывало, но не мелькало консольное окно)
					// Но не получилось, пока требуются хэндлы процесса, а их не получается
					// передать в НЕ приподнятый процесс (исходный ConEmu GUI).

					// -apparent
					// Same as -bypass, but run the process as SW_SHOWNORMAL

					// -demote
					// Запуск процесса (ком.строка после "/demote") в режиме простого юзера,
					// когда текущий процесс уже запущен "под админом". "Понизить" текущие
					// привилегии просто так нельзя, поэтому запуск идет через TaskSheduler.

					// -system
					// Non-interactive process, started as System account
					// It's used when starting consoles, our server works fine as non-interactive

					// -interactive
					// Used when ConEmu.exe is started under System account,
					// but we need to give starting process interactive capabilities.

					_ASSERTE(opt.runCommand.IsEmpty());
					pszTemp = cmdLineRest;
					if ((pszTemp = NextArg(pszTemp, szNext))
						&& szNext.OneOfSwitches(L"-run",L"-cmd"))
					{
						opt.runCommand.Set(pszTemp);
					}
					else
					{
						opt.runCommand.Set(cmdLineRest);
					}

					if (opt.runCommand.IsEmpty())
					{
						CEStr lsMsg(L"Invalid command line. '", szArg, L"' exists, command line is empty");
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
					// Only `-demote` and `-apparent` switches were implemented to start application visible
					// All others are intended to run our server process, without blinking of course
					if (szArg.OneOfSwitches(L"-demote", L"-apparent"))
						si.wShowWindow = SW_SHOWNORMAL;
					else
						si.wShowWindow = SW_HIDE;

					wchar_t szCurDir[MAX_PATH+1] = L"";
					GetCurrentDirectory(countof(szCurDir), szCurDir);

					BOOL b;
					DWORD nErr = 0;

					// if we were started from TaskScheduler, it would be nice to wait a little
					// to let parent (creator of the scheduler task) know we were started successfully
					bool bFromScheduler = false;

					// Log the command to be started
					{
						CEStr lsLog(
							L"Starting process",
							L": ", szArg,
							L" `", opt.runCommand.ms_Val, L"`");
						LogString(lsLog);
					}

					if (szArg.IsSwitch(L"-demote"))
					{
						b = CreateProcessDemoted(opt.runCommand.ms_Val, NULL, NULL, FALSE, NORMAL_PRIORITY_CLASS, NULL,
							szCurDir, &si, &pi, &nErr);
					}
					else if (szArg.IsSwitch(L"-system:"))
					{
						DWORD nSessionID = wcstoul(szArg.ms_Val+wcslen(L"-system:"), NULL, 10);
						b = CreateProcessSystem(nSessionID, opt.runCommand.ms_Val, NULL, NULL, FALSE, NORMAL_PRIORITY_CLASS, NULL,
							szCurDir, &si, &pi);
					}
					else if (szArg.IsSwitch(L"-interactive:"))
					{
						DWORD nSessionID = wcstoul(szArg.ms_Val+wcslen(L"-interactive:"), NULL, 10);
						b = CreateProcessInteractive(nSessionID, NULL, opt.runCommand.ms_Val, NULL, NULL, TRUE, NORMAL_PRIORITY_CLASS, NULL,
							szCurDir, &si, &pi, &nErr);
						bFromScheduler = true;
					}
					else // -bypass, -apparent
					{
						b = CreateProcess(NULL, opt.runCommand.ms_Val, NULL, NULL, TRUE, NORMAL_PRIORITY_CLASS, NULL,
							NULL, &si, &pi);
						nErr = b ? 0 : GetLastError();
						bFromScheduler = true;
					}

					// Log the result
					{
						CEStr lsLog; wchar_t szExtra[32] = L"";
						if (b)
						{
							if (pi.dwProcessId)
								swprintf_c(szExtra, L", PID=%u", pi.dwProcessId);
							lsLog = lstrmerge(
								L"Process was created successfully",
								szExtra);
						}
						else
						{
							swprintf_c(szExtra, L", ErrorCode=%u", nErr);
							lsLog = lstrmerge(
								L"Failed to start process",
								szExtra);
						}
						LogString(lsLog);
					}

					// If the error was not shown yet
					if (nErr) DisplayLastError(opt.runCommand, nErr);

					// if we were started from TaskScheduler, it would be nice to wait a little
					// to let parent (creator of the scheduler task) know we were started successfully
					if (bFromScheduler)
					{
						LogString(L"Sleeping for 5 seconds");
						Sleep(5*1000);
					}

					// Success?
					if (b)
					{
						iResult = 0;
					}

					// Done, close handles, if they were opened
					SafeCloseHandle(pi.hProcess);
					SafeCloseHandle(pi.hThread);

					goto wrap;
				}
				else if (szArg.IsSwitch(L"-multi"))
				{
					gpConEmu->AppendExtraArgs(szArg);
					gpConEmu->opt.MultiConValue = true;
				}
				else if (szArg.IsSwitch(L"-NoMulti"))
				{
					gpConEmu->AppendExtraArgs(szArg);
					gpConEmu->opt.MultiConValue = false;
				}
				else if (szArg.IsSwitch(L"-visible"))
				{
					gpConEmu->opt.VisValue = true;
				}
				else if (szArg.OneOfSwitches(L"-ct", L"-cleartype", L"-ct0", L"-ct1", L"-ct2"))
				{
					switch (szArg[3])
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
				else if (szArg.IsSwitch(L"-lng"))
				{
					NeedNextArg();

					if (!gpConEmu->opt.Language.Exists)
					{
						gpConEmu->opt.Language = (LPCWSTR)szNext;
						gpConEmu->AppendExtraArgs(L"-lng", szNext);
					}
				}
				// Optional specific "ConEmu.l10n"
				else if (szArg.IsSwitch(L"-lngfile"))
				{
					NeedNextArg();

					if (!gpConEmu->opt.LanguageFile.Exists)
					{
						gpConEmu->opt.LanguageFile = (LPCWSTR)szNext;
						gpConEmu->AppendExtraArgs(L"-lngfile", szNext);
					}
				}
				// Change font name
				else if (szArg.IsSwitch(L"-Font"))
				{
					NeedNextArg();

					if (!gpConEmu->opt.FontVal.Exists)
					{
						gpConEmu->opt.FontVal = (LPCWSTR)szNext;
						gpConEmu->AppendExtraArgs(L"-font", szNext);
					}
				}
				// Change font height
				else if (szArg.IsSwitch(L"-FontSize") || szArg.IsSwitch(L"-Size"))
				{
					NeedNextArg();

					if (!gpConEmu->opt.SizeVal.Exists)
					{
						gpConEmu->opt.SizeVal.SetInt(szNext);
					}
				}
				// ADD fontname; by Mors
				else if (szArg.IsSwitch(L"-FontFile"))
				{
					CESwitch szFile(sw_Str);
					if (!GetCfgParm(cmdLineRest, szFile, MAX_PATH))
					{
						goto wrap;
					}
					gpConEmu->AppendExtraArgs(L"-FontFile", szFile.GetStr());
					gpFontMgr->RegisterFont(szFile.GetStr(), TRUE);
				}
				// Register all fonts from specified directory
				else if (szArg.IsSwitch(L"-FontDir"))
				{
					CESwitch szDir(sw_Str);
					if (!GetCfgParm(cmdLineRest, szDir, MAX_PATH))
					{
						goto wrap;
					}
					gpConEmu->AppendExtraArgs(L"-FontDir", szDir.GetStr());
					gpFontMgr->RegisterFontsDir(szDir.GetStr());
				}
				else if (szArg.IsSwitch(L"-fs"))
				{
					gpConEmu->opt.WindowModeVal = wmFullScreen;
				}
				else if (szArg.IsSwitch(L"-max"))
				{
					gpConEmu->opt.WindowModeVal = wmMaximized;
				}
				else if (szArg.OneOfSwitches(L"-nor", L"-normal"))
				{
					gpConEmu->opt.WindowModeVal = wmNormal;
				}
				else if (szArg.OneOfSwitches(L"-min", L"-MinTSA", L"-StartTSA"))
				{
					gpConEmu->WindowStartMinimized = true;
					if (!szArg.IsSwitch(L"-min"))
					{
						gpConEmu->WindowStartTsa = true;
						gpConEmu->WindowStartNoClose = szArg.IsSwitch(L"-MinTSA");
					}
				}
				else if (szArg.OneOfSwitches(L"-tsa", L"-tray"))
				{
					gpConEmu->ForceMinimizeToTray = true;
				}
				else if (szArg.IsSwitch(L"-detached"))
				{
					gpConEmu->m_StartDetached = crb_On;
					opt.Detached = true;
				}
				else if (szArg.IsSwitch(L"-NoAutoClose"))
				{
					opt.NoAutoClose = true;
				}
				else if (szArg.IsSwitch(L"-NoAutoEnvReload"))
				{
					opt.NoAutoEnvReload = true;
				}
				else if (szArg.IsSwitch(L"-here"))
				{
					gpConEmu->mb_ConEmuHere = true;
					gpConEmu->StoreWorkDir();
				}
				else if (szArg.IsSwitch(L"-update"))
				{
					gpConEmu->opt.AutoUpdateOnStart = true;
				}
				else if (szArg.IsSwitch(L"-NoUpdate"))
				{
					// This one has more weight than AutoUpdateOnStart
					gpConEmu->opt.DisableAutoUpdate = true;
				}
				else if (szArg.IsSwitch(L"-NoHooksWarn"))
				{
					// Don't try to warn users about known problems with third-party detours
					gpConEmu->opt.NoHooksWarn = true;
				}
				else if (szArg.OneOfSwitches(L"-NoKeyHook", L"-NoKeyHooks", L"-NoKeybHook", L"-NoKeybHooks"))
				{
					gpConEmu->DisableKeybHooks = true;
				}
				else if (szArg.IsSwitch(L"-NoCloseConfirm"))
				{
					gpConEmu->DisableCloseConfirm = true;
				}
				else if (szArg.IsSwitch(L"-NoMacro"))
				{
					gpConEmu->DisableAllMacro = true;
				}
				else if (szArg.OneOfSwitches(L"-NoHotkey", L"-NoHotkeys"))
				{
					gpConEmu->DisableAllHotkeys = true;
				}
				else if (szArg.OneOfSwitches(L"-NoDefTrm", L"-NoDefTerm"))
				{
					gpConEmu->DisableSetDefTerm = true;
				}
				else if (szArg.OneOfSwitches(L"-NoRegFont", L"-NoRegFonts"))
				{
					gpConEmu->DisableRegisterFonts = true;
				}
				else if (szArg.OneOfSwitches(L"-inside", L"-inside="))
				{
					bool bRunAsAdmin = RunAsAdmin::Check(szArg.ms_Val);
					bool bSyncDir = false;
					LPCWSTR pszSyncFmt = NULL;

					gpConEmu->mb_ConEmuHere = true;
					gpConEmu->StoreWorkDir();

					// Both `-inside:...` and `-inside=...` are supported
					if (szArg.IsSwitch(L"-inside="))
					{
						bSyncDir = true;
						pszSyncFmt = szArg.ms_Val+8; // \eCD /d %1 - \e - ESC, \b - BS, \n - ENTER, %1 - "dir", %2 - "bash dir"
					}

					CConEmuInside::InitInside(bRunAsAdmin, bSyncDir, pszSyncFmt, 0, NULL);
				}
				else if (szArg.IsSwitch(L"-InsidePID"))
				{
					NeedNextArg();

					bool bRunAsAdmin = RunAsAdmin::Check(szArg.ms_Val);

					wchar_t* pszEnd;
					// Здесь указывается PID, в который нужно внедриться.
					DWORD nInsideParentPID = wcstol(szNext, &pszEnd, 10);
					if (nInsideParentPID)
					{
						CConEmuInside::InitInside(bRunAsAdmin, false, NULL, nInsideParentPID, NULL);
					}
				}
				else if (szArg.IsSwitch(L"-InsideWnd"))
				{
					NeedNextArg();

					LPCWSTR pszHWnd = szNext.ms_Val;

					if (pszHWnd[0] == L'0' && (pszHWnd[1] == L'x' || pszHWnd[1] == L'X'))
						pszHWnd += 2;
					else if (pszHWnd[0] == L'x' || pszHWnd[0] == L'X')
						pszHWnd ++;

					bool bRunAsAdmin = RunAsAdmin::Check(szArg.ms_Val);

					wchar_t* pszEnd;
					// Здесь указывается HWND, в котором нужно создаваться.
					HWND hParent = (HWND)(DWORD_PTR)wcstoul(pszHWnd, &pszEnd, 16);
					if (hParent && IsWindow(hParent))
					{
						CConEmuInside::InitInside(bRunAsAdmin, false, NULL, 0, hParent);
					}
				}
				else if (szArg.IsSwitch(L"-icon"))
				{
					NeedNextArg();

					if (!gpConEmu->opt.IconPrm.Exists && !szNext.IsEmpty())
					{
						gpConEmu->opt.IconPrm = true;
						gpConEmu->mps_IconPath = ExpandEnvStr(szNext);
					}
				}
				else if (szArg.IsSwitch(L"-dir"))
				{
					NeedNextArg();

					if (!szNext.IsEmpty())
					{
						// Например, "%USERPROFILE%"
						CEStr szExpand;
						if (wcschr(szNext, L'%') && ((szExpand = ExpandEnvStr(szNext)) != NULL))
						{
							gpConEmu->StoreWorkDir(szExpand);
						}
						else
						{
							gpConEmu->StoreWorkDir(szNext);
						}
					}
				}
				else if (szArg.IsSwitch(L"-UpdateJumpList"))
				{
					// Copy current Task list to Win7 Jump list (Taskbar icon)
					gpConEmu->mb_UpdateJumpListOnStartup = true;
				}
				else if (szArg.OneOfSwitches(L"-log", L"-log0", L"-log1", L"-log2", L"-log3", L"-log4"))
				{
					if (szArg.OneOfSwitches(L"-log", L"-log0"))
						gpConEmu->opt.AdvLogging.SetInt(1);
					else
						gpConEmu->opt.AdvLogging.SetInt((BYTE)(szArg[4] - L'0')); // 1..4
					// Do create logging service
					DEBUGSTRSTARTUP(L"Creating log file");
					gpConEmu->CreateLog();
				}
				else if (szArg.OneOfSwitches(L"-Single", L"-Reuse"))
				{
					// "/reuse" switch to be remastered
					gpConEmu->AppendExtraArgs(szArg);
					gpSetCls->SingleInstanceArg = sgl_Enabled;
				}
				else if (szArg.IsSwitch(L"-NoSingle"))
				{
					gpConEmu->AppendExtraArgs(szArg);
					gpSetCls->SingleInstanceArg = sgl_Disabled;
				}
				else if (szArg.IsSwitch(L"-DesktopMode"))
				{
					gpConEmu->opt.DesktopMode = true;
				}
				else if (szArg.OneOfSwitches(L"-Quake", L"-QuakeAuto", L"-NoQuake"))
				{
					if (szArg.IsSwitch(L"-Quake"))
						gpConEmu->opt.QuakeMode = 1;
					else if (szArg.IsSwitch(L"-QuakeAuto"))
						gpConEmu->opt.QuakeMode = 2;
					else
					{
						gpConEmu->opt.QuakeMode = 0;
						if (gpSetCls->SingleInstanceArg == sgl_Default)
							gpSetCls->SingleInstanceArg = sgl_Disabled;
					}
				}
				else if (szArg.OneOfSwitches(L"-FrameWidth", L"-Frame"))
				{
					NeedNextArg();

					if (!gpConEmu->opt.FrameWidth.Exists)
						gpConEmu->opt.FrameWidth.SetInt(szNext);
				}
				else if (szArg.OneOfSwitches(L"-ShowHide", L"-ShowHideTSA"))
				{
					gpSetCls->SingleInstanceArg = sgl_Enabled;
					gpSetCls->SingleInstanceShowHide = szArg.IsSwitch(L"-ShowHide")
						? sih_ShowMinimize : sih_ShowHideTSA;
				}
				else if (szArg.OneOfSwitches(L"-Reset", L"-ResetDefault", L"-Basic"))
				{
					gpConEmu->opt.ResetSettings = true;
					if (szArg.IsSwitch(L"-ResetDefault"))
					{
						gpSetCls->isFastSetupDisabled = true;
					}
					else if (szArg.IsSwitch(L"-Basic"))
					{
						gpSetCls->isFastSetupDisabled = true;
						gpSetCls->isResetBasicSettings = true;
					}
				}
				else if (szArg.OneOfSwitches(L"-NoCascade", L"-DontCascade"))
				{
					gpConEmu->AppendExtraArgs(szArg);
					gpSetCls->isDontCascade = true;
				}
				else if (szArg.OneOfSwitches(L"-WndX", L"-WndY", L"-WndW", L"-WndWidth", L"-WndH", L"-WndHeight"))
				{
					wchar_t ch = szArg[4];
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
				else if (szArg.IsSwitch(L"-Monitor"))
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
				else if (szArg.IsSwitch(L"-Theme"))
				{
					const wchar_t* kDefaultTheme = L"DarkMode_Explorer";
					bool bParm = false;
					if (!cmdLineRest || (*cmdLineRest == L'-' || *cmdLineRest == L'/')
						|| !GetCfgParm(cmdLineRest, bParm, gpConEmu->opt.WindowTheme, 128))
					{
						gpConEmu->opt.WindowTheme.SetStr(kDefaultTheme);
					}
				}
				else if (szArg.OneOfSwitches(L"-Buffer", L"-BufferHeight"))
				{
					NeedNextArg();

					if (!gpConEmu->opt.BufferHeightVal.Exists)
					{
						gpConEmu->opt.BufferHeightVal.SetInt(szNext);

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
				else if (szArg.IsSwitch(L"-Config"))
				{
					// -- используем последний из параметров, если их несколько
					if (!GetCfgParm(cmdLineRest, gpConEmu->opt.ConfigVal, 127))
					{
						goto wrap;
					}
				}
				else if (szArg.IsSwitch(L"-Palette"))
				{
					// -- используем последний из параметров, если их несколько
					if (!GetCfgParm(cmdLineRest, gpConEmu->opt.PaletteVal, MAX_PATH))
					{
						goto wrap;
					}
				}
				else if (szArg.IsSwitch(L"-LoadRegistry"))
				{
					gpConEmu->AppendExtraArgs(szArg);
					gpConEmu->opt.ForceUseRegistryPrm = true;
				}
				else if (szArg.OneOfSwitches(L"-LoadCfgFile", L"-LoadXmlFile"))
				{
					// -- используем последний из параметров, если их несколько
					if (!GetCfgParm(cmdLineRest, gpConEmu->opt.LoadCfgFile, MAX_PATH, true))
					{
						goto wrap;
					}
				}
				else if (szArg.OneOfSwitches(L"-SaveCfgFile", L"-SaveXmlFile"))
				{
					// -- используем последний из параметров, если их несколько
					if (!GetCfgParm(cmdLineRest, gpConEmu->opt.SaveCfgFile, MAX_PATH, true))
					{
						goto wrap;
					}
				}
				else if (szArg.IsSwitch(L"-GuiMacro"))
				{
					// -- выполняется только последний
					if (!GetCfgParm(cmdLineRest, gpConEmu->opt.ExecGuiMacro, 0x8000, false))
					{
						goto wrap;
					}
				}
				else if (szArg.IsSwitch(L"-UpdateSrcSet"))
				{
					// -- используем последний из параметров, если их несколько
					if (!GetCfgParm(cmdLineRest, gpConEmu->opt.UpdateSrcSet, MAX_PATH*4, false))
					{
						goto wrap;
					}
				}
				else if (szArg.IsSwitch(L"-AnsiLog"))
				{
					// -- используем последний из параметров, если их несколько
					if (!GetCfgParm(cmdLineRest, gpConEmu->opt.AnsiLogPath, MAX_PATH-40, true))
					{
						goto wrap;
					}
				}
				else if (szArg.IsSwitch(L"-SetDefTerm"))
				{
					gpConEmu->opt.SetUpDefaultTerminal = true;
				}
				else if (szArg.IsSwitch(L"-ZoneId"))
				{
					gpConEmu->opt.FixZoneId = true;
				}
				else if (szArg.IsSwitch(L"-Exit"))
				{
					gpConEmu->opt.ExitAfterActionPrm = true;
				}
				else if (szArg.IsSwitch(L"-QuitOnClose"))
				{
					gpConEmu->mb_ForceQuitOnClose = true;
				}
				else if (szArg.IsSwitch(L"-Title"))
				{
					bool bOk = false;
					CESwitch pszTitle(sw_Str);
					if (!GetCfgParm(cmdLineRest, bOk, pszTitle, 127))
					{
						goto wrap;
					}
					gpConEmu->SetTitleTemplate(pszTitle.GetStr());
				}
				else if (szArg.IsSwitch(L"-Settings"))
				{
					gpConEmu->mb_SettingsRequested = true;
				}
				else if (szArg.IsSwitch(L"-FindBugMode"))
				{
					gpConEmu->mb_FindBugMode = true;
				}
				else if (szArg.OneOfSwitches(L"-debug", L"-debugw", L"-debugi"))
				{
					// These switches were already processed
				}
				else if (szArg.OneOfSwitches(L"-?", L"-h", L"-help"))
				{
					if (gpLng) gpLng->Reload();
					ConEmuAbout::OnInfo_About();
					iResult = -1;
					goto wrap;
				}
				// Final `-run ...` or `-runlist ...` (old names `-cmd ...` or `-cmdlist ...`)
				else if (
					szArg.OneOfSwitches(L"-run", L"-runlist", L"-cmd", L"-cmdlist")
					)
				{
					if (opt.cfgSwitches.ms_Val)
					{
						_ASSERTE(pszArgStart>=pszCopyToEnvStart); // If there is only "-run cmd" in arguments
						_ASSERTE((INT_PTR)(pszArgStart - pszCopyToEnvStart) <= opt.cfgSwitches.GetLen());
						opt.cfgSwitches.ms_Val[pszArgStart - pszCopyToEnvStart] = 0;
					}

					opt.runCommand.Set(SkipNonPrintable(cmdLineRest));
					opt.isScript = szArg.OneOfSwitches(L"-runlist", L"-cmdlist");
					break;
				}
				else
				{
					// Show error on unknown switch
					psUnknown = pszArgStart;
					break;
				}
			}
			// Main processing cycle end

			// Avoid assertions in NextArg
			szArg.Empty(); szNext.Empty();
		} // while (NextArg(&cmdLineRest, szArg, &pszArgStart) == 0)
	}
	// Processing loop end

	if (psUnknown)
	{
		DEBUGSTRSTARTUP(L"Unknown switch, exiting!");
		if (gpSet->isLogging())
		{
			// For direct logging we do not use lng resources
			CEStr lsLog(L"\r\n", L"Unknown switch specified: ", psUnknown, L"\r\n\r\n");
			gpConEmu->LogString(lsLog, false, false);
		}

		CEStr szNewConWarn;
		LPCWSTR pszTestSwitch =
			(psUnknown[0] == L'-' || psUnknown[0] == L'/')
				? ((psUnknown[1] == L'-' || psUnknown[1] == L'/')
					? (psUnknown+2) : (psUnknown+1))
				: psUnknown;
		if ((lstrcmpni(pszTestSwitch, L"new_console", 11) == 0)
			|| (lstrcmpni(pszTestSwitch, L"cur_console", 11) == 0))
		{
			szNewConWarn = lstrmerge(L"\r\n\r\n",
				CLngRc::getRsrc(lng_UnknownSwitch4/*"Switch -new_console must be specified *after* -run or -runlist"*/)
				);
		}

		CEStr lsMsg(
			CLngRc::getRsrc(lng_UnknownSwitch1/*"Unknown switch specified:"*/),
			L"\r\n\r\n",
			psUnknown,
			szNewConWarn,
			L"\r\n\r\n",
			CLngRc::getRsrc(lng_UnknownSwitch2/*"Visit website to get thorough switches description:"*/),
			L"\r\n"
			CEGUIARGSPAGE
			L"\r\n\r\n",
			CLngRc::getRsrc(lng_UnknownSwitch3/*"Or run ‘ConEmu.exe -?’ to get the brief."*/)
			);

		MBoxA(lsMsg);
		goto wrap;
	}

	// Set "ConEmuArgs" and "ConEmuArgs2"
	ProcessConEmuArgsVar();

	// Continue normal startup
	bRc = true;
wrap:
	return bRc;
}
