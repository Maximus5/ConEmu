
/*
Copyright (c) 2013-present Maximus5
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

#include "CEStr.h"

#define CmdEscapeNeededChars  L"<>()&|^\""
#define QuotationNeededChars  (L" " CmdEscapeNeededChars)

// CmdArg
struct CmdArg : public CEStr
{
public:
	// Point to the end dblquot, if we need drop first and last quotation marks
	LPCWSTR mpsz_Dequoted = nullptr;
	// true if it's a double-quoted argument from NextArg
	bool mb_Quoted = false;
	// if 0 - this is must be first call (first token of command line)
	// so, we need to test for mpsz_Dequoted
	int mn_TokenNo = 0;
	// To bee able corretly parse double quotes in commands like
	// "C:\Windows\system32\cmd.exe" /C ""C:\Python27\python.EXE""
	// "reg.exe add "HKEY_CLASSES_ROOT\Directory\Background\shell\Command Prompt\command" /ve /t REG_EXPAND_SZ /d "\"D:\Applications\ConEmu\ConEmuPortable.exe\" /Dir \"%V\" /cmd \"cmd.exe\" \"-new_console:nC:cmd.exe\" \"-cur_console:d:%V\"" /f"
	enum { cc_Undefined, cc_CmdExeFound, cc_CmdCK, cc_CmdCommand } mn_CmdCall = cc_Undefined;

	#ifdef _DEBUG
	// Debug, для отлова "не сброшенных" вызовов
	LPCWSTR ms_LastTokenEnd = nullptr;
	wchar_t ms_LastTokenSave[32] = L"";
	#endif

private:
	bool CompareSwitch(LPCWSTR asSwitch) const;

public:
	void Empty();

	void GetPosFrom(const CmdArg& arg);

	// If this may be supported switch like "-run"
	bool IsPossibleSwitch() const;
	// For example, compare if ms_Val is "-run"
	bool IsSwitch(LPCWSTR asSwitch) const;
	// Stops checking on first NULL
	bool OneOfSwitches(LPCWSTR asSwitch1, LPCWSTR asSwitch2 = NULL, LPCWSTR asSwitch3 = NULL, LPCWSTR asSwitch4 = NULL, LPCWSTR asSwitch5 = NULL, LPCWSTR asSwitch6 = NULL, LPCWSTR asSwitch7 = NULL, LPCWSTR asSwitch8 = NULL, LPCWSTR asSwitch9 = NULL, LPCWSTR asSwitch10 = NULL) const;

	CmdArg(CmdArg&&) = delete;

	CmdArg& operator=(const wchar_t* str);
	CmdArg& operator=(wchar_t*&& str) = delete;
	CmdArg& operator=(CmdArg&& str) = delete;

	CmdArg();
	CmdArg(const wchar_t* str);
	~CmdArg();
};


const wchar_t* NextArg(const wchar_t* asCmdLine, CmdArg& rsArg, const wchar_t** rsArgStart=NULL);
bool DemangleArg(CmdArg& rsDemangle, bool bDeQuote = true, bool bDeEscape = false);

typedef DWORD NEXTLINEFLAGS;
const NEXTLINEFLAGS
	NLF_SKIP_EMPTY_LINES = 1,
	NLF_TRIM_SPACES = 2,
	NLF_NONE = 0;
const wchar_t* NextLine(const wchar_t* asLines, CEStr &rsLine, NEXTLINEFLAGS Flags = NLF_TRIM_SPACES|NLF_SKIP_EMPTY_LINES);

bool CompareFileMask(const wchar_t* asFileName, const wchar_t* asMask);
LPCWSTR GetDrive(LPCWSTR pszPath, wchar_t* szDrive, int/*countof(szDrive)*/ cchDriveMax);
LPCWSTR GetDirectory(CEStr& szDir);

bool CompareProcessNames(LPCWSTR pszProcess1, LPCWSTR pszProcess2);
bool CheckProcessName(LPCWSTR pszProcessName, LPCWSTR* lsNames);

bool IsExecutable(LPCWSTR aszFilePathName, wchar_t** rsExpandedVars = NULL);
bool IsFarExe(LPCWSTR asModuleName);
bool IsCmdProcessor(LPCWSTR asModuleName);
bool IsConEmuGui(LPCWSTR pszProcessName);
bool IsConsoleService(LPCWSTR pszProcessName);
bool IsConsoleServer(LPCWSTR pszProcessName);
bool IsConsoleHelper(LPCWSTR pszProcessName);
bool IsTerminalServer(LPCWSTR pszProcessName);
bool IsGitBashHelper(LPCWSTR pszProcessName);
bool IsSshAgentHelper(LPCWSTR pszProcessName);
bool IsNeedCmd(BOOL bRootCmd, LPCWSTR asCmdLine, CEStr &szExe,
			   LPCWSTR* rsArguments = NULL, BOOL* rpbNeedCutStartEndQuot = NULL,
			   BOOL* rpbRootIsCmdExe = NULL, BOOL* rpbAlwaysConfirmExit = NULL, BOOL* rpbAutoDisableConfirmExit = NULL);

bool IsQuotationNeeded(LPCWSTR pszPath);
const wchar_t* SkipNonPrintable(const wchar_t* asParams);

int AddEndSlash(wchar_t* rsPath, int cchMax);
wchar_t* MergeCmdLine(LPCWSTR asExe, LPCWSTR asParams);
wchar_t* JoinPath(LPCWSTR asPath, LPCWSTR asPart1, LPCWSTR asPart2 = NULL);
wchar_t* GetParentPath(LPCWSTR asPath);

bool IsFilePath(LPCWSTR asFilePath, bool abFullRequired = false);

const wchar_t* PointToName(const wchar_t* asFullPath);
const char* PointToName(const char* asFileOrPath);
const wchar_t* PointToExt(const wchar_t* asFullPath);
const wchar_t* Unquote(wchar_t* asParm, bool abFirstQuote = false);

bool IsNewConsoleArg(LPCWSTR lsCmdLine, LPCWSTR pszArg = L"-new_console");
