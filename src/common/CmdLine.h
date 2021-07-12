
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
#include "CmdArg.h"

#define CmdEscapeNeededChars  L"<>()&|^\""
#define QuotationNeededChars  (L" ," CmdEscapeNeededChars)


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
bool IsVsNetHostExe(LPCWSTR processName);
bool IsVsDebugConsoleExe(LPCWSTR processName);
bool IsVsDebugger(LPCWSTR processName);
bool IsGDB(LPCWSTR processName);

/// @brief Try to extract valid file-path-name of starting executable from space-delimited string with lack of double quotes
/// @param commandLine [IN] Command line to parse, it could be not properly double quoted. Examples:
///   `C:\\Program Files\\Internet Explorer\\iexplore.exe http://google.com`
///   or `"C:\\Program Files\\Internet Explorer\\iexplore.exe http://google.com\"`.
/// @param szExe [OUT] the path to found executable
/// @param rsArguments [OUT] pointer to the rest of command line, arguments
/// @returns true - if file-path is found and szExe is not empty, false - on error
bool GetFilePathFromSpaceDelimitedString(const wchar_t* commandLine, CEStr& szExe, const wchar_t*& rsArguments);

/// @brief Describes should we do anything with double quotes around the command.
enum class StartEndQuot : int32_t
{
	/// @brief Execute command line as is
	DontChange = 0,
	/// @brief Remove quotation around the command.
	/// E.g. CreateProcess(L"\"\"c:\\program files\\arc\\7z.exe\" -?\"") will fail otherwise.
	NeedCut = 1,
	/// @brief Add quotation around the command.
	/// E.g. CreateProcess(L"cmd /c \"c:\\my tools\\script.cmd\" args") will fail otherwise.
	NeedAdd = 2,
};

/// @brief Output arguments for IsNeedCmd function
struct NeedCmdOptions
{
	bool isNeedCmd = false;
	StartEndQuot startEndQuot = StartEndQuot::DontChange;
	bool rootIsCmdExe = false;
	bool alwaysConfirmExit = false;
};

/// @brief Returns true of command line contains redirections or pipelines
/// @param asParameters The command line arguments, executable may be absent here
/// @return true/false
bool IsCmdRedirection(LPCWSTR asParameters);

/// <summary>
/// Determines if we need a cmd.exe to run the command line.
/// </summary>
/// <param name="bRootCmd">true if we are starting root console process (first after ConEmuC server)</param>
/// <param name="asCmdLine">Command line to examine</param>
/// <param name="szExe">[OUT] buffer receiving the first executable file (PE or script)</param>
/// <param name="options">[OUT] optional pointer to <ref>NeedCmdOptions</ref> struct</param>
/// <returns>true - if to execute a command we have to add "cmd.exe /c ...", false - asCmdLine could be executed intact</returns>
bool IsNeedCmd(bool bRootCmd, LPCWSTR asCmdLine, CEStr &szExe, NeedCmdOptions* options = nullptr);

/// @brief Checks if cmd is one of cmd.exe or tcc.exe internal commands, like "set", "echo", etc.
bool IsCmdInternalCommand(const wchar_t* cmd);

bool IsQuotationNeeded(LPCWSTR pszPath);
bool IsNonPrintable(wchar_t chr);
const wchar_t* SkipNonPrintable(const wchar_t* asParams);

int AddEndSlash(wchar_t* rsPath, int cchMax);
CEStr MergeCmdLine(LPCWSTR asExe, LPCWSTR asParams);
CEStr JoinPath(LPCWSTR asPath, LPCWSTR asPart1, LPCWSTR asPart2 = nullptr);
CEStr GetParentPath(LPCWSTR asPath);

/// <summary>
/// Simple check if the asFilePath could be valid path.
/// We don't check if file really exists in filesystem here.
/// </summary>
/// <param name="asFilePath">File path</param>
/// <param name="abFullRequired">if true we don't allow relative paths or bare file names</param>
/// <returns>true - if path is valid</returns>
bool IsFilePath(LPCWSTR asFilePath, bool abFullRequired = false);

const wchar_t* PointToName(const wchar_t* asFileOrPath);
const char* PointToName(const char* asFileOrPath);

const wchar_t* PointToExt(const wchar_t* asFullPath);
const wchar_t* Unquote(wchar_t* asParm, bool abFirstQuote = false);

bool IsNewConsoleArg(LPCWSTR lsCmdLine, LPCWSTR pszArg = L"-new_console");
