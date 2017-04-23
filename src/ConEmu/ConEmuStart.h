
/*
Copyright (c) 2015-2017 Maximus5
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

#include <windows.h>

class CConEmuMain;

enum CESwitchType
{
	sw_None,
	sw_Simple,  // just a bool
	sw_Int,     // number, may be min/max specified
	sw_FnSet,   // "/log1" -> Int=1
	sw_Str,     // string, may be min/max length specified
	sw_EnvStr,  // expanded env string, may be min/max length specified
	sw_PathStr, // path, may contain env strings, may be min/max length specified
	sw_Cmd,     // "-run ..."
	sw_CmdList, // "-runlist ..."
};

struct CESwitch
{
public:
	// The value
	union
	{
		bool        Bool;
		int         Int;
		wchar_t*    Str;
		HMONITOR    Mon; // Treated as sw_Int
	};

	// Type
	CESwitchType Type;

	// Was specified in command line?
	bool Exists;

private:
	// Not copyable!
	CESwitch& operator=(const CESwitch &);

public:
	CESwitch(CESwitchType aType = sw_None);
	~CESwitch();

	void Clear();

public:
	// Helpers
	void Undefine();

	bool GetBool();
	void SetBool(bool NewVal);
	CESwitch& operator=(bool NewVal);

	int GetInt();
	void SetInt(int NewVal);
	void SetInt(LPCWSTR NewVal, int Radix = 10);
	CESwitch& operator=(int NewVal);

	LPCWSTR GetStr();
	void SetStr(LPCWSTR NewVal, CESwitchType NewType = sw_Str);
	CESwitch& operator=(LPCWSTR NewVal);

public:
	operator bool();
	// other operators are not defined to avoid unambiguous calls
};

class CConEmuStart
{
private:
	CConEmuMain* mp_ConEmu;

public:
	CConEmuStart(CConEmuMain* pOwner);
	virtual ~CConEmuStart();

private:
	/* 'Default' command line (if nor Registry, nor -run specified) */
	wchar_t  szDefCmd[MAX_PATH+32];
	CEStr ms_DefNewTaskName;

	/* Command or list stored with SetCurCmd */
	CEStr szCurCmd;
	bool isCurCmdList; // true for command list (`|||` delimited)

public:
	/* OUR(!) startup info */
	STARTUPINFOW ourSI;
	/* Last Shell exit code */
	int mn_ShellExitCode;

public:
	/* switch -detached in the ConEmu.exe arguments */
	RConBoolArg m_StartDetached;
	/* switch -here in the ConEmu.exe arguments (used with "ConEmu Here" feature) */
	bool mb_ConEmuHere;
	/* switch -QuitOnClose: close ConEmu with last tab or cross-clicking */
	bool mb_ForceQuitOnClose;

public:
	/* Store/retrieve command line, specified with "-run" or "-runlist" switches */
	void SetCurCmd(LPCWSTR pszNewCmd, bool bIsCmdList);
	LPCTSTR GetCurCmd(bool *pIsCmdList = NULL);

	/* "Active" command line */
	LPCTSTR GetCmd(bool *pIsCmdList = NULL, bool bNoTask = false);
	/* If some task was marked ad "Default for new consoles" */
	LPCTSTR GetDefaultTask();

	/* "Default" command line "far/cmd" */
	LPCTSTR GetDefaultCmd();
	void    SetDefaultCmd(LPCWSTR asCmd);

	/* Find bash (bash.exe) location. Different installations supported: MinGW, Git-For-Windows, Cygwin, ... */
	bool    FindBashLocation(CEStr& lsBash);

	void ResetCmdArg();

protected:
	/* Processing helpers */
	bool GetCfgParm(LPCWSTR& cmdLineRest, CESwitch& Val, int nMaxLen, bool bExpandAndDup = false);
	bool GetCfgParm(LPCWSTR& cmdLineRest, bool& Prm, CESwitch& Val, int nMaxLen, bool bExpandAndDup = false);
	void ProcessConEmuArgsVar();
public:
	bool ParseCommandLine(LPCWSTR pszCmdLine, int& iResult);
	void ResetConman();

public:
	/* Startup options configured via command line switches */
	struct StartOptions
	{
	public:
		// Some variables
		CEStr cmdLine;       // All arguments, except our executable
		CEStr cfgSwitches;   // e.g. "-lng ru -config test ..." (switches before "-cmd" or "-cmdlist")
		CEStr cmdRunCommand; // e.g. "-cmd cmd.exe" or "-cmdlist cmd ||| powershell"
		CEStr runCommand;    // The shell command line or script. e.g. "cmd.exe" or "cmd ||| powershell"
		bool  isScript;      // if true - than `-cmdlist` was specified
		int   params;
		// The options
		CESwitch Language; // sw_Str: en/ru/...
		CESwitch LanguageFile; // sw_Str: "...\ConEmu.l10n"
		CESwitch ClearTypeVal; // sw_Int: CLEARTYPE_NATURAL_QUALITY
		CESwitch FontVal; // sw_Str
		CESwitch IconPrm; // sw_Simple
		CESwitch SizeVal; // sw_Int
		CESwitch BufferHeightVal; // sw_Int
		CESwitch ConfigVal; // sw_Str
		CESwitch PaletteVal; // sw_Str
		CESwitch WindowModeVal; // sw_Int
		CESwitch ForceUseRegistryPrm;
		CESwitch LoadCfgFile; // sw_Str
		CESwitch SaveCfgFile; // sw_Str
		CESwitch DisableAutoUpdate; // sw_Simple /noupdate
		CESwitch AutoUpdateOnStart; // sw_Simple /update
		CESwitch UpdateSrcSet; // sw_Str
		CESwitch AnsiLogPath; // sw_Str
		CESwitch ExecGuiMacro; // sw_Str
		CESwitch QuakeMode; // sw_Int
		CESwitch FrameWidth; // sw_Int
		CESwitch DesktopMode; // sw_Simple
		bool SizePosPrm; // -WndX -WndY -WndW[idth] -WndH[eight]
		CESwitch sWndX, sWndY, sWndW, sWndH; // sw_Str
		CESwitch Monitor; // -Monitor <1 | x10001 | "\\.\DISPLAY1">
		CESwitch SetUpDefaultTerminal; // sw_Simple
		CESwitch FixZoneId; // sw_Simple
		CESwitch ExitAfterActionPrm; // sw_Simple
		CESwitch MultiConValue; // sw_Simple
		CESwitch VisValue; // sw_Simple
		CESwitch ResetSettings; // sw_Simple
		CESwitch AdvLogging; // sw_Int: -log[1|2|3|4]
		CESwitch Detached; // sw_Simple
		CESwitch NoAutoClose; // checked by isCloseOnLastTabClose()
	public:
		// Suppress intellisense warning:
		// the default constructor of "CConEmuStart::StartOptions" cannot be referenced -- it is a deleted function	ConEmu
		StartOptions()
		{
			params = 0;
			isScript = false; // true if switch ‘-cmdlist’ was used
			SizePosPrm = false;
		};
	} opt;
};
