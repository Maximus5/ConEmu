
/*
Copyright (c) 2009-2014 Maximus5
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
#include "../common/CmdLine.h"

#include "AboutDlg.h"
#include "ConEmu.h"
#include "Inside.h"
//TODO: "OptionsClass.h" на удаление, все методы должны быть здесь
#include "OptionsClass.h"
#include "OptionsCur.h"

#define DEBUGSTRSTARTUP(s) DEBUGSTR(WIN3264TEST(L"ConEmu.exe: ",L"ConEmu64.exe: ") s L"\n")

CSettingsCur* gpSetCur = NULL;

CSettingsCur::CSettingsCur()
{
	// Only one switch default value
	ClearTypeParm.Int = CLEARTYPE_NATURAL_QUALITY;
	// Some member variables
	isScript = false;
	isSwitch = false;
	cmdLinePtr = NULL;
	cmdLineRest = NULL;
	cmdNew = NULL;
	psUnknown = NULL;
}

CSettingsCur::~CSettingsCur()
{
}

// Load current value of "HKCU\Software\Microsoft\Command Processor" : "AutoRun"
// (bClear==true) - remove from it our "... Cmd_Autorun.cmd ..." part
wchar_t* CSettingsCur::LoadAutorunValue(HKEY hkCmd, bool bClear)
{
	size_t cchCmdMax = 65535;
	wchar_t *pszCmd = (wchar_t*)malloc(cchCmdMax*sizeof(*pszCmd));
	if (!pszCmd)
	{
		_ASSERTE(pszCmd!=NULL);
		return NULL;
	}

	_ASSERTE(hkCmd!=NULL);
	//HKEY hkCmd = NULL;
	//if (0 == RegOpenKeyEx(HKEY_CURRENT_USER, L"Software\\Microsoft\\Command Processor", 0, KEY_READ, &hkCmd))

	DWORD cbMax = (cchCmdMax-2) * sizeof(*pszCmd);
	if (0 == RegQueryValueEx(hkCmd, L"AutoRun", NULL, NULL, (LPBYTE)pszCmd, &cbMax))
	{
		pszCmd[cbMax>>1] = 0;

		if (bClear && *pszCmd)
		{
			// Просили почистить от "... Cmd_Autorun.cmd ..."
			wchar_t* pszFind = StrStrI(pszCmd, L"\\ConEmu\\Cmd_Autorun.cmd");
			if (pszFind)
			{
				// "... Cmd_Autorun.cmd ..." found, need to find possible start and end of our part ('&' separated)
				wchar_t* pszStart = pszFind;
				while ((pszStart > pszCmd) && (*(pszStart-1) != L'&'))
					pszStart--;

				const wchar_t* pszEnd = wcschr(pszFind, L'&');
				if (!pszEnd)
				{
					pszEnd = pszFind + _tcslen(pszFind);
				}
				else
				{
					while (*pszEnd == L'&')
						pszEnd++;
				}

				// Ok, There are another commands?
				if ((pszStart > pszCmd) || *pszEnd)
				{
					// Possibilities
					if (!*pszEnd)
					{
						// app1.exe && Cmd_Autorun.cmd
						while ((pszStart > pszCmd) && ((*(pszStart-1) == L'&') || (*(pszStart-1) == L' ')))
							pszStart--;
						_ASSERTE(pszStart > pszCmd); // Command to left is empty?
						*pszStart = 0; // just trim
					}
					else
					{
						// app1.exe && Cmd_Autorun.cmd & app2.exe
						// app1.exe & Cmd_Autorun.cmd && app2.exe
						// Cmd_Autorun.cmd & app2.exe
						if (pszStart == pszCmd)
						{
							pszEnd = SkipNonPrintable(pszEnd);
						}
						size_t cchLeft = _tcslen(pszEnd)+1;
						// move command (from right) to the 'Cmd_Autorun.cmd' place
						memmove(pszStart, pszEnd, cchLeft*sizeof(wchar_t));
					}
				}
				else
				{
					// No, we are alone
					*pszCmd = 0;
				}
			}
		}

		// Skip spaces?
		LPCWSTR pszChar = SkipNonPrintable(pszCmd);
		if (!pszChar || !*pszChar)
		{
			*pszCmd = 0;
		}
	}
	else
	{
		*pszCmd = 0;
	}

	// Done
	if (pszCmd && (*pszCmd == 0))
	{
		SafeFree(pszCmd);
	}
	return pszCmd;
}

int CSettingsCur::RegisterCmdAutorun(LPCWSTR pszBatchFile, bool bForceNewWnd, bool bForceClear/*if (!pszBatchFile)*/)
{
	int  iRc = 1;

	HKEY hkCmd = NULL; DWORD dw;
	LONG lRegRc = RegCreateKeyEx(HKEY_CURRENT_USER, L"Software\\Microsoft\\Command Processor",
						0, NULL, 0, KEY_READ|KEY_WRITE, NULL, &hkCmd, &dw);

	if (0 == lRegRc)
	{
		wchar_t* pszCur = NULL;
		wchar_t* pszBuf = NULL;
		LPCWSTR  pszSet = L"";
		wchar_t szCmd[MAX_PATH+128] = L"\"";

		if (pszBatchFile)
		{
			pszCur = LoadAutorunValue(hkCmd, true);
			pszSet = pszCur;

			wcscat_c(szCmd, pszBatchFile);

			if (FileExists(szCmd+1))
			{
				wcscat_c(szCmd, bForceNewWnd ? L"\" \"/GHWND=NEW\"" : L"\"");

				if (pszCur == NULL)
				{
					pszSet = szCmd;
				}
				else
				{
					// Current "AutoRun" is not empty, need concatenate
					size_t cchAll = _tcslen(szCmd) + _tcslen(pszCur) + 5;
					pszBuf = (wchar_t*)malloc(cchAll*sizeof(*pszBuf));
					_ASSERTE(pszBuf);
					if (pszBuf)
					{
						_wcscpy_c(pszBuf, cchAll, szCmd);
						_wcscat_c(pszBuf, cchAll, L" & "); // conveyer next command indifferent to %errorlevel%
						_wcscat_c(pszBuf, cchAll, pszCur);
						// Ok, Set
						pszSet = pszBuf;
					}
				}
			}
			else
			{
				MsgBox(szCmd, MB_ICONSTOP, L"File not found", ghOpWnd);

				pszSet = pszCur ? pszCur : L"";

				iRc = 104;
			}
		}
		else
		{
			pszCur = bForceClear ? NULL : LoadAutorunValue(hkCmd, true);
			pszSet = pszCur ? pszCur : L"";
		}

		DWORD cchLen = _tcslen(pszSet)+1;
		lRegRc = RegSetValueEx(hkCmd, L"AutoRun", 0, REG_SZ, (LPBYTE)pszSet, cchLen*sizeof(wchar_t));
		if (lRegRc != 0)
		{
			DisplayLastError(L"Failed to open HKCU\\Software\\Microsoft\\Command Processor’", lRegRc);
			iRc = 105;
		}

		RegCloseKey(hkCmd);

		if (pszBuf && (pszBuf != pszCur) && (pszBuf != szCmd))
			free(pszBuf);
		SafeFree(pszCur);
	}
	else
	{
		DisplayLastError(L"Failed to open HKCU\\Software\\Microsoft\\Command Processor’", lRegRc);
		iRc = 106;
	}

	return iRc;
}

LPCWSTR CSettingsCur::SkipToCmdArg(LPCWSTR pszRest)
{
	CmdArg szArg;
	while (NextArg(&pszRest, szArg) == 0)
	{
		if (lstrcmpi(szArg, L"/cmd") == 0 || lstrcmpi(szArg, L"-cmd") == 0)
			return pszRest;
	}
	return NULL;
}

void CSettingsCur::ResetConman()
{
	HKEY hk = 0;
	DWORD dw = 0;

	// 24.09.2010 Maks - Только если ключ конмана уже создан!
	// сбрость CreateInNewEnvironment для ConMan
	//if (0 == RegCreateKeyEx(HKEY_CURRENT_USER, _T("Software\\HoopoePG_2x"),
	//        NULL, NULL, NULL, KEY_ALL_ACCESS, NULL, &hk, &dw))
	if (0 == RegOpenKeyEx(HKEY_CURRENT_USER, _T("Software\\HoopoePG_2x"), 0, KEY_ALL_ACCESS, &hk))
	{
		RegSetValueEx(hk, _T("CreateInNewEnvironment"), 0, REG_DWORD,
		              (LPBYTE)&(dw=0), sizeof(dw));
		RegCloseKey(hk);
	}
}

bool CSettingsCur::HasWindowParms()
{
	bool bRc = (WindowModeParm.Exists || WindowMinParm.Exists || WindowMinTSAParm.Exists || WindowStartTSAParm.Exists);
	return bRc;
}

bool CSettingsCur::HasWindowStartMinimized()
{
	bool bRc = (WindowMinParm.GetBool() || WindowMinTSAParm.GetBool() || WindowStartTSAParm.GetBool());
	return bRc;
}

bool CSettingsCur::HasWindowStartTSA()
{
	bool bRc = (WindowMinTSAParm.GetBool() || WindowStartTSAParm.GetBool());
	return bRc;
}

bool CSettingsCur::HasWindowStartNoClose()
{
	bool bRc = (WindowStartTSAParm.GetBool());
	return bRc;
}

bool CSettingsCur::HasResetSettings()
{
	bool bRc = (ResetParm.GetBool() || ResetDefaultParm.GetBool() || BasicParm.GetBool());
	return bRc;
}

SingleInstanceArgEnum CSettingsCur::SingleInstance()
{
	SingleInstanceArgEnum sgl = sgl_Default;
}

SingleInstanceShowHideType CSettingsCur::SingleInstanceShowHide()
{
	SingleInstanceShowHideType sht = sih_None;
}

int CSettingsCur::Arg_AutoSetup(CESwitch& Sw, LPCWSTR pszArg, LPCWSTR& pszRest)
{
	// Called from "Cmd_Autorun.cmd /i"
	// ... /autosetup 1 "%~0" %FORCE_NEW_WND_CMD%
	// or "Cmd_Autorun.cmd /u"
	// ... /autosetup 0
	bool lbTurnOn = true;

	if (!*pszRest)
		return 101;

	CmdArg szBatch, szNew;
	bool bForceNewWnd = false;

	if (*pszRest == _T('0'))
	{
		lbTurnOn = false;
	}
	else
	{
		if (NextArg(&pszRest, szBatch) != 0)
			return 102;

		if (!FileExists(szBatch.ms_Arg))
			return 103;
	}

	if (lbTurnOn)
	{
		if (NextArg(&pszRest, szNew) == 0)
		{
			bForceNewWnd = (lstrcmpi(szNew, L"/GHWND=NEW") == 0);
		}
	}

	int nSetupRc = RegisterCmdAutorun(lbTurnOn ? szBatch.ms_Arg : NULL, bForceNewWnd, false);
	_ASSERTE(nSetupRc==1 || nSetupRc>103);

	// сбрость CreateInNewEnvironment для ConMan
	ResetConman();

	return nSetupRc;
}

int CSettingsCur::Arg_ByPassDemote(CESwitch& Sw, LPCWSTR pszArg, LPCWSTR& pszRest)
{
	// ConEmu /bypass /cmd <command line>
	//	Этот ключик был придуман для прозрачного запуска консоли
	//	в режиме администратора
	//	(т.е. чтобы окно UAC нормально всплывало, но не мелькало консольное окно)
	//	Но не получилось, пока требуются хэндлы процесса, а их не получается
	//	передать в НЕ приподнятый процесс (исходный ConEmu GUI).

	// ConEmu /demote /cmd <command line>
	//	Запуск процесса (ком.строка после "/demote") в режиме простого юзера,
	//	когда текущий процесс уже запущен "под админом". "Понизить" текущие
	//	привилегии просто так нельзя, поэтому запуск идет через TaskSheduler.

	LPCWSTR pszCmd = SkipToCmdArg(pszRest);
	if (!pszCmd || !*pszCmd)
	{
		DisplayLastError(L"Invalid cmd line. '/bypass' exists, '/cmd' not", -1);
		return 100;
	}

	// Information
	#ifdef _DEBUG
	STARTUPINFO siOur = {sizeof(siOur)};
	GetStartupInfo(&siOur);
	#endif

	STARTUPINFO si = {sizeof(si)};
	si.dwFlags = STARTF_USESHOWWINDOW;

	wchar_t szCurDir[MAX_PATH+1] = L"";
	GetCurrentDirectory(countof(szCurDir), szCurDir);

	PROCESS_INFORMATION pi = {};

	BOOL b;
	DWORD nErr = 0;

	wchar_t* pszCopy = lstrdup(pszCmd);

	if (lstrcmpi(pszArg, L"/bypass") == 0)
	{
		b = CreateProcess(NULL, pszCopy, NULL, NULL, TRUE, 0, NULL, szCurDir, &si, &pi);
		nErr = GetLastError();
	}
	else
	{
		b = CreateProcessDemoted(NULL, pszCopy, NULL, NULL, FALSE, 0, NULL, szCurDir, &si, &pi, &nErr);
	}

	SafeFree(pszCopy);

	if (b)
	{
		CloseHandle(pi.hProcess);
		CloseHandle(pi.hThread);
		return 0;
	}

	// Failed
	DisplayLastError(pszCmd, nErr);
	return 100;
}

int CSettingsCur::Arg_ClearType(CESwitch& Sw, LPCWSTR pszArg, LPCWSTR& pszRest)
{
	// "/ct" | "/cleartype" | "/ct0" | "/ct1" | "/ct2"
	// set ClearTypeParm.Int
	switch (pszArg[3])
	{
	case L'0':
		Sw.Int = NONANTIALIASED_QUALITY; break;
	case L'1':
		Sw.Int = ANTIALIASED_QUALITY; break;
	default:
		Sw.Int = CLEARTYPE_NATURAL_QUALITY;
	}
	// anyway
	return 0;
}

int CSettingsCur::Arg_FontFileDir(CESwitch& Sw, LPCWSTR pszArg, LPCWSTR& pszRest)
{
	CmdArg szPath;
	if (NextArg(&pszRest, szPath) != 0)
		return 100;

	if (lstrcmpi(pszArg, L"/fontfile") == 0)
		gpSetCls->RegisterFont(szPath, TRUE);
	else if (lstrcmpi(pszArg, L"/fontdir") == 0)
		gpSetCls->RegisterFontsDir(szPath);

	return 0;
}

int CSettingsCur::Arg_WindowMode(CESwitch& Sw, LPCWSTR pszArg, LPCWSTR& pszRest)
{
	if (lstrcmpi(pszArg, L"/fs") == 0)
		Sw.Int = rFullScreen;
	else if (lstrcmpi(pszArg, L"/max") == 0)
		Sw.Int = rMaximized;

	return 0;
}

int CSettingsCur::Arg_Inside(CESwitch& Sw, LPCWSTR pszArg, LPCWSTR& pszRest)
{
	CmdArg szNext;

	bool bRunAsAdmin = isPressed(VK_SHIFT);
	bool bSyncDir = false;
	LPCWSTR pszSyncFmt = NULL;
	DWORD nInsideParentPID = 0;
	HWND hParent = NULL;
	wchar_t* pszEnd;

	if (!lstrcmpi(pszArg, L"/inside")
		|| !lstrcmpni(pszArg, L"/inside=", 8))
	{
		if (pszArg[7] == L'=')
		{
			bSyncDir = true;
			pszSyncFmt = pszArg+8; // \eCD /d %1 - \e - ESC, \b - BS, \n - ENTER, %1 - "dir", %2 - "bash dir"
		}
	}
	else if (!lstrcmpi(pszArg, L"/insidepid") && !NextArg(&pszRest, szNext))
	{
		wchar_t* pszEnd;
		// Здесь указывается PID, в который нужно внедриться.
		nInsideParentPID = wcstol(szNext, &pszEnd, 10);
		if (!nInsideParentPID)
		{
			return 100;
		}
	}
	else if (!lstrcmpi(pszArg, L"/insidewnd") && !NextArg(&pszRest, szNext))
	{
		LPCWSTR pszNext = szNext;

		if (pszNext[0] == L'0' && (pszNext[1] == L'x' || pszNext[1] == L'X'))
			pszNext += 2;
		else if (pszNext[0] == L'x' || pszNext[0] == L'X')
			pszNext ++;


		// Здесь указывается HWND, в котором нужно создаваться.
		hParent = (HWND)wcstol(pszNext, &pszEnd, 16);
		if (hParent && !IsWindow(hParent))
		{
			return 100;
		}
	}

	CConEmuInside::InitInside(bRunAsAdmin, bSyncDir, pszSyncFmt, nInsideParentPID, hParent);

	return 0;
}

int CSettingsCur::Arg_WorkDir(CESwitch& Sw, LPCWSTR pszArg, LPCWSTR& pszRest)
{
	CmdArg szNext;
	if (NextArg(&pszRest, szNext) != 0)
		return 100;

	Sw.Str = ExpandEnvStr(szNext);
	Sw.Type = sw_EnvStr;
	gpConEmu->StoreWorkDir(Sw.Str);

	return 0;
}

int CSettingsCur::Arg_Log(CESwitch& Sw, LPCWSTR pszArg, LPCWSTR& pszRest)
{
	if (!lstrcmpi(pszArg, L"/log") || !lstrcmpi(pszArg, L"/log0"))
	{
		gpSetCls->isAdvLogging = 1;
	}
	else if (!lstrcmpi(pszArg, L"/log1") || !lstrcmpi(pszArg, L"/log2")
		|| !lstrcmpi(pszArg, L"/log3") || !lstrcmpi(pszArg, L"/log4"))
	{
		gpSetCls->isAdvLogging = (BYTE)(pszArg[4] - L'0'); // 1..4
	}

	return 0;
}

int CSettingsCur::Arg_Title(CESwitch& Sw, LPCWSTR pszArg, LPCWSTR& pszRest)
{
	CmdArg szNext;
	if (NextArg(&pszRest, szNext) != 0)
		return 100;

	gpConEmu->SetTitleTemplate(szNext);

	return 0;
}

int CSettingsCur::Arg_Title(CESwitch& Sw, LPCWSTR pszArg, LPCWSTR& pszRest)
{
	ConEmuAbout::OnInfo_About();
	return -1; // Сразу выйти, по тихому!
}

// Returns 0 if OK
int CSettingsCur::ParseCommandLine(LPCWSTR pszCmdLine)
{
	int iRc = 0;

	struct {
		LPCWSTR pszSwitch[8];
		CESwitchType Type;
		CESwitch* ValPtr;
		int (*SetSwitchFn)(CESwitch& Sw, LPCWSTR pszArg, LPCWSTR& pszRest);
		int iMin, iMax;
	} KnownSwitches[] = {
		{{L"?", L"h", L"help"}, sw_FnSet, &AboutParm, Arg_About},
		{{L"AnsiLog"}, sw_Str, &AnsiLogPathParm, NULL, 1, MAX_PATH-40},
		{{L"Autosetup"}, sw_FnSet, &AutoSetupParm, Arg_AutoSetup},
		{{L"Basic"}, sw_Simple, &BasicParm},
		{{L"Buffer", L"BufferHeight"}, sw_Int, &BufferHeightParm, NULL, LONGOUTPUTHEIGHT_MIN, LONGOUTPUTHEIGHT_MAX},
		{{L"Bypass"}, sw_FnSet, &ByPassParm, Arg_ByPassDemote},
		{{L"Config"}, sw_Str, &ConfigParm, NULL, 1, 127},
		{{L"Ct", L"ClearType", L"Ct0", L"Ct1", L"Ct2"}, sw_FnSet, &ClearTypeParm, Arg_ClearType},
		{{L"Demote"}, sw_FnSet, &DemoteParm, Arg_ByPassDemote},
		{{L"Detached"}, sw_Simple, &StartDetachedParm},
		{{L"Dir"}, sw_FnSet, &WorkDirParm},
		{{L"Exit"}, sw_Simple, &ExitAfterActionParm},
		{{L"FindBugMode"}, sw_Simple, &FindBugModeParm},
		{{L"Font"}, sw_Str, &FontParm},
		{{L"FontDir"}, sw_FnSet, &FontDirParm, Arg_FontFileDir, 1, MAX_PATH},
		{{L"FontFile"}, sw_FnSet, &FontFileParm, Arg_FontFileDir, 1, MAX_PATH},
		{{L"Fs"}, sw_FnSet, &WindowModeParm, Arg_WindowMode},
		{{L"Icon"}, sw_PathStr, &IconParm},
		{{L"Inside", L"Inside=", L"InsidePid", L"InsideWnd"}, sw_FnSet, &InsideParm, Arg_Inside},
		{{L"LoadCfgFile", L"LoadXmlFile"}, sw_PathStr, &LoadCfgFileParm, NULL, 1, MAX_PATH},
		{{L"Log", L"Log0", L"Log1", L"Log2", L"Log3", L"Log4"}, sw_FnSet, &LogParm, Arg_Log},
		{{L"Max"}, sw_FnSet, &WindowModeParm, Arg_WindowMode},
		{{L"Min"}, sw_Simple, &WindowMinParm},
		{{L"MinTsa"}, sw_Simple, &WindowMinTSAParm},
		{{L"Multi"}, sw_Simple, &MultiConParm},
		{{L"NoCascade", L"DontCascade"}, sw_Simple, &DontCascadeParm},
		{{L"NoCloseConfirm"}, sw_Simple, &DisableCloseConfirmParm},
		{{L"NoDefTrm", L"NoDefTerm"}, sw_Simple, &DisableSetDefTermParm},
		{{L"NoHotKey", L"NoHotKeys"}, sw_Simple, &DisableAllHotkeysParm},
		{{L"NoKeyHook", L"NoKeyHooks", L"NoKeybHook", L"NoKeybHooks"}, sw_Simple, &DisableKeybHooksParm},
		{{L"NoMacro"}, sw_Simple, &DisableAllMacroParm},
		{{L"NoMulti"}, sw_Simple, &NoMultiConParm},
		{{L"NoRegFont", L"NoRegFonts"}, sw_Simple, &DisableRegisterFontsParm},
		{{L"NoSingle"}, sw_Simple, &NoSingleParm}, // TODO: gpSetCls->SingleInstanceArg
		{{L"NoUpdate"}, sw_Simple, &DisableAutoUpdateParm},
		{{L"Palette"}, sw_Str, &PaletteParm, NULL, 1, MAX_PATH},
		{{L"Reset"}, sw_Simple, &ResetParm},
		{{L"ResetDefault"}, sw_Simple, &ResetDefaultParm},
		{{L"Reuse"}, sw_Simple, &ReuseParm},
		{{L"SaveCfgFile", L"SaveXmlFile"}, sw_PathStr, &SaveCfgFileParm, NULL, 1, MAX_PATH},
		{{L"Session"}, sw_Simple, &SessionParm},
		{{L"SetDefTerm"}, sw_Simple, &SetUpDefTermParm},
		{{L"ShowHide"}, sw_Simple, &ShowHideParm},
		{{L"ShowHideTSA"}, sw_Simple, &ShowHideTSAParm},
		{{L"Single"}, sw_Simple, &SingleParm},     // TODO: gpSetCls->SingleInstanceArg
		{{L"Size"}, sw_Str, &SizeParm},
		{{L"StartTSA"}, sw_Simple, &WindowStartTSAParm},
		{{L"Title"}, sw_Str, &TitleParm, Arg_Title, 1, 127},
		{{L"TSA", L"Tray"}, sw_Simple, &ForceMinTSAParm},
		{{L"UpdateJumpList"}, sw_Simple, &UpdateJumpListParm},
		{{L"UpdateSrcSet"}, sw_Str, &UpdateSrcSetParm, NULL, 1, MAX_PATH*4},
		{{L"Visible"}, sw_Simple, &VisParm},
		{NULL}
	};

	if (!pszCmdLine)
	{
		_ASSERTE(pszCmdLine!=NULL);
		pszCmdLine = L"";
	}

	// pszCmdLine может и не содержать путь исполняемого файла (первым аргументом)
	LPCWSTR pszTemp = pszCmdLine;
	LPCWSTR pszName;
	CmdArg  szArg, szNext;

	if (NextArg(&pszTemp, szArg) != 0)
	{
		_ASSERTE(FALSE && "GetCommandLine() is empty");
		goto wrap; // Treat as empty command line
	}

	pszName = PointToName(szArg);
	if ((lstrcmpi(pszName, WIN3264TEST(L"ConEmu.exe",L"ConEmu64.exe")) == 0)
		|| (lstrcmpi(pszName, WIN3264TEST(L"ConEmu",L"ConEmu64")) == 0))
	{
		// OK, our executable was specified properly in command line
		_ASSERTE(*pszTemp != L' ');
		pszCmdLine = pszTemp;
	}
	else
	{
		pszCmdLine = SkipNonPrintable(pszCmdLine);
	}

	// Check it
	if (*pszCmdLine == 0)
	{
		// Эта переменная нужна для того, чтобы conemu можно было перезапустить
		// из cmd файла с теми же аргументами (selfupdate)
		SetEnvironmentVariableW(L"ConEmuArgs", L"");

		// Empty command line, nothing to parse
		_ASSERTE(iRc == 0); // OK
		goto wrap;
	}
	else
	{
		// Эта переменная нужна для того, чтобы conemu можно было перезапустить
		// из cmd файла с теми же аргументами (selfupdate)
		gpConEmu->mpsz_ConEmuArgs = lstrdup(pszCmdLine);
		SetEnvironmentVariableW(L"ConEmuArgs", gpConEmu->mpsz_ConEmuArgs);
	}

	// Let parse the reset
	szArg.Empty();
	cmdLineRest = pszCmdLine;

	while (NextArg(&cmdLineRest, szArg) == 0)
	{
		// Use both notations ('-' and '/')
		if (((szArg.ms_Arg[0] != L'-') && (szArg.ms_Arg[0] != L'/')) || !szArg.ms_Arg[1])
		{
			psUnknown = szArg.Detach();
			break;
		}

		bool bKnown = false;
		LPCWSTR pszSwitch = szArg.ms_Arg+1;
		int iVal; wchar_t* pszIEnd;

		for (INT_PTR i = 0; KnownSwitches[i].pszSwitch[0]; i++)
		{
			// One switch can have aliases
			for (INT_PTR j = 0; (j < countof(KnownSwitches[i].pszSwitch)) && KnownSwitches[i].pszSwitch[j]; j++)
			{
				LPCWSTR pszCmp = KnownSwitches[i].pszSwitch[j];
				int nLen = lstrlen(pszCmp);
				if (pszCmp[nLen-1] == L'=')
				{
					if (lstrcmpni(pszSwitch, pszCmp, nLen) == 0)
					{
						bKnown = true;
						break;
					}
				}
				else
				{
					if (lstrcmpi(pszSwitch, pszCmp) == 0)
					{
						bKnown = true;
						break;
					}
				}
			}

			if (!bKnown)
			{
				psUnknown = szArg.Detach();
				break;
			}
			else
			{
				KnownSwitches[i].ValPtr->Exists = true;
				KnownSwitches[i].ValPtr->Type = KnownSwitches[i].Type;

				switch (KnownSwitches[i].Type)
				{

				case sw_Simple:
					KnownSwitches[i].ValPtr->Bool = (KnownSwitches[i].Type == sw_Simple);
					break;

				case sw_Int:
					if (NextArg(&cmdLineRest, szNext) != 0)
					{
						psUnknown = szArg.Detach();
						iRc = 100;
						break;
					}
					iVal = wcstol(szNext, &pszIEnd, 10);
					if (KnownSwitches[i].iMin < KnownSwitches[i].iMax)
					{
						TODO("Trap on out-of-bound?");
						if (iVal < KnownSwitches[i].iMin)
							iVal = KnownSwitches[i].iMin;
						else if (iVal > KnownSwitches[i].iMax)
							iVal = KnownSwitches[i].iMax;
					}
                    KnownSwitches[i].ValPtr->Int = iVal;
					break;

				case sw_FnSet:
					_ASSERTE(KnownSwitches[i].SetSwitchFn != NULL);
					iRc = KnownSwitches[i].SetSwitchFn(*KnownSwitches[i].ValPtr, szArg, cmdLineRest);
					if (iRc != 0)
						goto wrap;
					break;

				case sw_Str:
				case sw_EnvStr:
				case sw_PathStr:
					if (NextArg(&cmdLineRest, szNext) != 0)
					{
						psUnknown = szArg.Detach();
						iRc = 100;
						break;
					}
					iVal = lstrlen(szNext);
					if ((KnownSwitches[i].iMin < KnownSwitches[i].iMax)
						&& ((iVal < KnownSwitches[i].iMin) || (iVal > KnownSwitches[i].iMax)))
					{
						// Trap on out-of-bound?
						psUnknown = szArg.Detach();
						iRc = 100;
						goto wrap;
					}
					SafeFree(KnownSwitches[i].ValPtr->Str);
					if (KnownSwitches[i].Type == sw_Str)
						KnownSwitches[i].ValPtr->Str = szNext.Detach();
					else if (KnownSwitches[i].Type == sw_PathStr)
						KnownSwitches[i].ValPtr->Str = GetFullPathNameEx(szNext);
					else
						KnownSwitches[i].ValPtr->Str = ExpandEnvStr(szNext);
					break;

				case sw_Cmd:
				case sw_CmdList:
					// DONE
					cmdNew = lstrdup(cmdLineRest);
					isScript = (KnownSwitches[i].Type == sw_CmdList);
					goto wrap;

				} // switch (KnownSwitches[i].Type)

				break;
			}
		}

		if (iRc != 0)
			break;
	}

#if 0
	if (params && params != (uint)-1)
	{
		TCHAR *curCommand = cmdLine;
		TODO("Если первый (после запускаемого файла) аргумент начинается НЕ с '/' - завершить разбор параметров и не заменять '""' на пробелы");
		//uint params; SplitCommandLine(curCommand, &params);
		//if (params < 1) {
		//    curCommand = NULL;
		//}
		// Parse parameters.
		// Duplicated parameters are permitted, the first value is used.
		uint i = 0;

		while (i < params && curCommand && *curCommand)
		{
			bool lbNotFound = false;
			bool lbSwitchChanged = false;

			if (*curCommand == L'-' && curCommand[1] && !wcspbrk(curCommand+1, L"\\//|.&<>:^"))
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

					if ((i + 1) >= params)
						return 101;

					curCommand += _tcslen(curCommand) + 1; i++;

					if (*curCommand == _T('0'))
						lbTurnOn = FALSE;
					else
					{
						if ((i + 1) >= params)
							return 101;

						curCommand += _tcslen(curCommand) + 1; i++;
						DWORD dwAttr = GetFileAttributes(curCommand);

						if (dwAttr == (DWORD)-1 || (dwAttr & FILE_ATTRIBUTE_DIRECTORY))
							return 102;
					}

					HKEY hk = NULL; DWORD dw;
					int nSetupRc = 100;

					if (0 != RegCreateKeyEx(HKEY_CURRENT_USER, _T("Software\\Microsoft\\Command Processor"),
										   0, NULL, 0, KEY_ALL_ACCESS, NULL, &hk, &dw))
						return 103;

					if (lbTurnOn)
					{
						size_t cchMax = _tcslen(curCommand);
						LPCWSTR pszArg1 = NULL;
						if ((i + 1) < params)
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
					// сбрость CreateInNewEnvironment для ConMan
					ResetConman();
					return nSetupRc;
				}
				else if (!klstricmp(curCommand, _T("/bypass")))
				{
					// Этот ключик был придуман для прозрачного запуска консоли
					// в режиме администратора
					// (т.е. чтобы окно UAC нормально всплывало, но не мелькало консольное окно)
					// Но не получилось, пока требуются хэндлы процесса, а их не получается
					// передать в НЕ приподнятый процесс (исходный ConEmu GUI).

					if (!cmdNew || !*cmdNew)
					{
						DisplayLastError(L"Invalid cmd line. '/bypass' exists, '/cmd' not", -1);
						return 100;
					}

					// Information
					#ifdef _DEBUG
					STARTUPINFO siOur = {sizeof(siOur)};
					GetStartupInfo(&siOur);
					#endif

					STARTUPINFO si = {sizeof(si)};
					si.dwFlags = STARTF_USESHOWWINDOW;

					PROCESS_INFORMATION pi = {};

					BOOL b = CreateProcess(NULL, cmdNew, NULL, NULL, TRUE, 0, NULL, NULL, &si, &pi);
					if (b)
					{
						CloseHandle(pi.hProcess);
						CloseHandle(pi.hThread);
						return 0;
					}

					// Failed
					DisplayLastError(cmdNew);
					return 100;
				}
				else if (!klstricmp(curCommand, _T("/demote")))
				{
					// Запуск процесса (ком.строка после "/demote") в режиме простого юзера,
					// когда текущий процесс уже запущен "под админом". "Понизить" текущие
					// привилегии просто так нельзя, поэтому запуск идет через TaskSheduler.

					if (!cmdNew || !*cmdNew)
					{
						DisplayLastError(L"Invalid cmd line. '/demote' exists, '/cmd' not", -1);
						return 100;
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


					b = CreateProcessDemoted(NULL, cmdNew, NULL, NULL, FALSE, NORMAL_PRIORITY_CLASS, NULL,
							szCurDir, &si, &pi, &nErr);


					if (b)
					{
						SafeCloseHandle(pi.hProcess);
						SafeCloseHandle(pi.hThread);
						return 0;
					}

					// Failed
					DisplayLastError(cmdNew, nErr);
					return 100;
				}
				else if (!klstricmp(curCommand, _T("/multi")))
				{
					MultiConValue = true; MultiConPrm = true;
				}
				else if (!klstricmp(curCommand, _T("/nomulti")))
				{
					MultiConValue = false; MultiConPrm = true;
				}
				else if (!klstricmp(curCommand, _T("/visible")))
				{
					VisValue = true; VisPrm = true;
				}
				else if (!klstricmp(curCommand, _T("/ct")) || !klstricmp(curCommand, _T("/cleartype"))
					|| !klstricmp(curCommand, _T("/ct0")) || !klstricmp(curCommand, _T("/ct1")) || !klstricmp(curCommand, _T("/ct2")))
				{
					ClearTypePrm = true;
					switch (curCommand[3])
					{
					case L'0':
						ClearTypeVal = NONANTIALIASED_QUALITY; break;
					case L'1':
						ClearTypeVal = ANTIALIASED_QUALITY; break;
					default:
						ClearTypeVal = CLEARTYPE_NATURAL_QUALITY;
					}
				}
				// имя шрифта
				else if (!klstricmp(curCommand, _T("/font")) && i + 1 < params)
				{
					curCommand += _tcslen(curCommand) + 1; i++;

					if (!FontPrm)
					{
						FontPrm = true;
						FontVal = curCommand;
					}
				}
				// Высота шрифта
				else if (!klstricmp(curCommand, _T("/size")) && i + 1 < params)
				{
					curCommand += _tcslen(curCommand) + 1; i++;

					if (!SizePrm)
					{
						SizePrm = true;
						SizeVal = klatoi(curCommand);
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
								return 100;
							}
							else
							{
								TCHAR sClass[128];

								if (GetClassName(hCon, sClass, 128))
								{
									if (_tcscmp(sClass, VirtualConsoleClassMain)==0)
									{
										// Сверху УЖЕ другая копия ConEmu
										return 1;
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
											return 100;
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
												return 1;
											}
										}
									}
									else
									{
										// верхнее окно - НЕ консоль
										return 100;
									}
								}
							}

							gpSetCls->hAttachConWnd = hCon;
						}
					}
				}
				#endif
				// ADD fontname; by Mors
				else if (!klstricmp(curCommand, _T("/fontfile")) && i + 1 < params)
				{
					bool bOk = false; TCHAR* pszFile = NULL;
					if (!GetCfgParm(i, curCommand, bOk, pszFile, MAX_PATH))
					{
						return 100;
					}
					gpSetCls->RegisterFont(pszFile, TRUE);
				}
				// Register all fonts from specified directory
				else if (!klstricmp(curCommand, _T("/fontdir")) && i + 1 < params)
				{
					bool bOk = false; TCHAR* pszDir = NULL;
					if (!GetCfgParm(i, curCommand, bOk, pszDir, MAX_PATH))
					{
						return 100;
					}
					gpSetCls->RegisterFontsDir(pszDir);
				}
				else if (!klstricmp(curCommand, _T("/fs")))
				{
					WindowModeVal = rFullScreen; WindowPrm = true;
				}
				else if (!klstricmp(curCommand, _T("/max")))
				{
					WindowModeVal = rMaximized; WindowPrm = true;
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
					gpConEmu->mb_StartDetached = TRUE;
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

					if (curCommand[7] == _T('='))
					{
						bSyncDir = true;
						pszSyncFmt = curCommand+8; // \eCD /d %1 - \e - ESC, \b - BS, \n - ENTER, %1 - "dir", %2 - "bash dir"
					}

					CConEmuInside::InitInside(bRunAsAdmin, bSyncDir, pszSyncFmt, 0, NULL);
				}
				else if (!klstricmp(curCommand, _T("/insidepid")) && ((i + 1) < params))
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
				else if (!klstricmp(curCommand, _T("/insidewnd")) && ((i + 1) < params))
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
				else if (!klstricmp(curCommand, _T("/icon")) && ((i + 1) < params))
				{
					curCommand += _tcslen(curCommand) + 1; i++;

					if (!IconPrm && *curCommand)
					{
						IconPrm = true;
						gpConEmu->mps_IconPath = ExpandEnvStr(curCommand);
					}
				}
				else if (!klstricmp(curCommand, _T("/dir")) && i + 1 < params)
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
				else if (!klstricmp(curCommand, _T("/single")))
				{
					gpSetCls->SingleInstanceArg = sgl_Enabled;
				}
				else if (!klstricmp(curCommand, _T("/nosingle")))
				{
					gpSetCls->SingleInstanceArg = sgl_Disabled;
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
					ResetSettings = true;
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
					gpSetCls->isDontCascade = true;
				}
				else if ((!klstricmp(curCommand, _T("/Buffer")) || !klstricmp(curCommand, _T("/BufferHeight")))
					&& i + 1 < params)
				{
					curCommand += _tcslen(curCommand) + 1; i++;

					if (!BufferHeightPrm)
					{
						BufferHeightPrm = true;
						BufferHeightVal = klatoi(curCommand);

						if (BufferHeightVal < 0)
						{
							//setParent = true; -- Maximus5 - нефиг, все ручками
							BufferHeightVal = -BufferHeightVal;
						}

						if (BufferHeightVal < LONGOUTPUTHEIGHT_MIN)
							BufferHeightVal = LONGOUTPUTHEIGHT_MIN;
						else if (BufferHeightVal > LONGOUTPUTHEIGHT_MAX)
							BufferHeightVal = LONGOUTPUTHEIGHT_MAX;
					}
				}
				else if (!klstricmp(curCommand, _T("/Config")) && i + 1 < params)
				{
					// используем последний из параметров, если их несколько
					if (!GetCfgParm(i, curCommand, ConfigPrm, ConfigVal, 127))
					{
						return 100;
					}
				}
				else if (!klstricmp(curCommand, _T("/Palette")) && i + 1 < params)
				{
					// используем последний из параметров, если их несколько
					if (!GetCfgParm(i, curCommand, PalettePrm, PaletteVal, MAX_PATH))
					{
						return 100;
					}
				}
				else if (!klstricmp(curCommand, _T("/LoadCfgFile")) && i + 1 < params)
				{
					// используем последний из параметров, если их несколько
					if (!GetCfgParm(i, curCommand, LoadCfgFilePrm, LoadCfgFile, MAX_PATH, true))
					{
						return 100;
					}
				}
				else if (!klstricmp(curCommand, _T("/SaveCfgFile")) && i + 1 < params)
				{
					// используем последний из параметров, если их несколько
					if (!GetCfgParm(i, curCommand, SaveCfgFilePrm, SaveCfgFile, MAX_PATH, true))
					{
						return 100;
					}
				}
				else if (!klstricmp(curCommand, _T("/UpdateSrcSet")) && i + 1 < params)
				{
					// используем последний из параметров, если их несколько
					if (!GetCfgParm(i, curCommand, UpdateSrcSetPrm, UpdateSrcSet, MAX_PATH*4, false))
					{
						return 100;
					}
				}
				else if (!klstricmp(curCommand, _T("/AnsiLog")) && i + 1 < params)
				{
					// используем последний из параметров, если их несколько
					if (!GetCfgParm(i, curCommand, AnsiLogPathPrm, AnsiLogPath, MAX_PATH-40, true))
					{
						return 100;
					}
				}
				else if (!klstricmp(curCommand, _T("/SetDefTerm")))
				{
					SetUpDefaultTerminal = true;
				}
				else if (!klstricmp(curCommand, _T("/Exit")))
				{
					ExitAfterActionPrm = true;
				}
				else if (!klstricmp(curCommand, _T("/Title")) && i + 1 < params)
				{
					bool bOk = false; TCHAR* pszTitle = NULL;
					if (!GetCfgParm(i, curCommand, bOk, pszTitle, 127))
					{
						return 100;
					}
					gpConEmu->SetTitleTemplate(pszTitle);
				}
				else if (!klstricmp(curCommand, _T("/FindBugMode")))
				{
					gpConEmu->mb_FindBugMode = true;
				}
				else if (!klstricmp(curCommand, _T("/?")) || !klstricmp(curCommand, _T("/h")) || !klstricmp(curCommand, _T("/help")))
				{
					//MessageBox(NULL, pHelp, L"About ConEmu...", MB_ICONQUESTION);
					ConEmuAbout::OnInfo_About();
					free(cmdLine);
					return -1;
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
#endif

wrap:
	if (gpSetCls->isAdvLogging)
	{
		DEBUGSTRSTARTUP(L"Creating log file");
		gpConEmu->CreateLog();
	}

	if (psUnknown)
	{
		DEBUGSTRSTARTUP(L"Unknown switch, exiting!");
		wchar_t* psMsg = lstrmerge(L"Unknown switch specified:\r\n", psUnknown);
		if (psMsg)
		{
			gpConEmu->LogString(psMsg, false, false);
			MBoxA(psMsg);
			free(psMsg);
		}
		iRc = 100;
		goto wrap_exit;
	}

wrap_exit:
	return iRc;
}
