
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
	/// Point to the end dblquot, if we need drop first and last quotation marks
	LPCWSTR m_pszDequoted = nullptr;
	/// true if it's a double-quoted argument from NextArg
	bool m_bQuoted = false;
	/// if 0 - this is must be first call (first token of command line)
	/// so, we need to test for mpsz_Dequoted
	int m_nTokenNo = 0;

	enum class CmdCall { Undefined, CmdExeFound, CmdK, CmdC };
	/// To be able correctly parse double quotes in commands like <br>
	/// "C:\Windows\system32\cmd.exe" /C ""C:\Python27\python.EXE"" <br>
	/// "reg.exe add "HKEY_CLASSES_ROOT\Directory\Background\shell\Command Prompt\command" /ve /t REG_EXPAND_SZ /d "\"D:\Applications\ConEmu\ConEmuPortable.exe\" /Dir \"%V\" /cmd \"cmd.exe\" \"-new_console:nC:cmd.exe\" \"-cur_console:d:%V\"" /f"
	CmdCall m_nCmdCall = CmdCall::Undefined;

	#ifdef _DEBUG
	// Debug to catch dirty calls
	LPCWSTR m_sLastTokenEnd = nullptr;
	wchar_t m_sLastTokenSave[32] = L"";
	#endif

private:
	bool CompareSwitch(LPCWSTR asSwitch, bool caseSensitive = false) const;

public:
	void Empty();

	/// Copy service info from other CmdArg
	/// Used during NextArg evaluation to use different CmdArg objects
	void LoadPosFrom(const CmdArg& arg);

	/// If this may be supported switch like "-run", "/run", "/inside=...", "-regfont:...", etc.
	bool IsPossibleSwitch() const;
	/// For example, compare if ms_Val is "-run", "/run", "/inside=...", "-regfont:...", etc.
	bool IsSwitch(LPCWSTR asSwitch, bool caseSensitive = false) const;
	/// Stops checking on first nullptr
	bool OneOfSwitches(LPCWSTR asSwitch1, LPCWSTR asSwitch2 = nullptr, LPCWSTR asSwitch3 = nullptr, LPCWSTR asSwitch4 = nullptr, LPCWSTR asSwitch5 = nullptr, LPCWSTR asSwitch6 = nullptr, LPCWSTR asSwitch7 = nullptr, LPCWSTR asSwitch8 = nullptr, LPCWSTR asSwitch9 = nullptr, LPCWSTR asSwitch10 = nullptr) const;
	/// <summary>
	/// Returns tail of argument after ":" or "=". <br>
	/// Examples: "0xABC" for switch "-inside:0xABC"
	/// </summary>
	/// <returns>non-null string</returns>
	LPCWSTR GetExtra() const;

	CmdArg(CmdArg&&) = delete;
	CmdArg(const CmdArg&) = delete;

	CmdArg& operator=(const wchar_t* str);
	CmdArg& operator=(wchar_t*&& str) = delete;
	CmdArg& operator=(CmdArg&& str) = delete;
	CmdArg& operator=(const CmdArg&) = delete;

	CmdArg();
	CmdArg(const wchar_t* str);
	~CmdArg();
};


const wchar_t* NextArg(const wchar_t* asCmdLine, CmdArg& rsArg, const wchar_t** rsArgStart=nullptr);
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

bool IsExecutable(LPCWSTR aszFilePathName, wchar_t** rsExpandedVars = nullptr);
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
			   LPCWSTR* rsArguments = nullptr, BOOL* rpbNeedCutStartEndQuot = nullptr,
			   BOOL* rpbRootIsCmdExe = nullptr, BOOL* rpbAlwaysConfirmExit = nullptr, BOOL* rpbAutoDisableConfirmExit = nullptr);

bool IsQuotationNeeded(LPCWSTR pszPath);
const wchar_t* SkipNonPrintable(const wchar_t* asParams);

int AddEndSlash(wchar_t* rsPath, int cchMax);
wchar_t* MergeCmdLine(LPCWSTR asExe, LPCWSTR asParams);
wchar_t* JoinPath(LPCWSTR asPath, LPCWSTR asPart1, LPCWSTR asPart2 = nullptr);
wchar_t* GetParentPath(LPCWSTR asPath);

bool IsFilePath(LPCWSTR asFilePath, bool abFullRequired = false);

const wchar_t* PointToName(const wchar_t* asFullPath);
const char* PointToName(const char* asFileOrPath);
const wchar_t* PointToExt(const wchar_t* asFullPath);
const wchar_t* Unquote(wchar_t* asParm, bool abFirstQuote = false);

bool IsNewConsoleArg(LPCWSTR lsCmdLine, LPCWSTR pszArg = L"-new_console");
