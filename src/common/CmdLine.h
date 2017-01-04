
/*
Copyright (c) 2013-2017 Maximus5
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

int NextArg(const wchar_t** asCmdLine, CEStr &rsArg, const wchar_t** rsArgStart=NULL);
LPCWSTR QueryNextArg(const wchar_t* asCmdLine, CEStr &rsArg, const wchar_t** rsArgStart=NULL);
bool DemangleArg(CEStr& rsDemangle, bool bDeQuote = true, bool bDeEscape = false);

typedef DWORD NEXTLINEFLAGS;
const NEXTLINEFLAGS
	NLF_SKIP_EMPTY_LINES = 1,
	NLF_TRIM_SPACES = 2,
	NLF_NONE = 0;
int NextLine(const wchar_t** asLines, CEStr &rsLine, NEXTLINEFLAGS Flags = NLF_TRIM_SPACES|NLF_SKIP_EMPTY_LINES);
LPCWSTR QueryNextLine(const wchar_t* asLines, CEStr &rsLine, NEXTLINEFLAGS Flags = NLF_TRIM_SPACES|NLF_SKIP_EMPTY_LINES);

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

bool IsFilePath(LPCWSTR asFilePath, bool abFullRequired = false);

const wchar_t* PointToName(const wchar_t* asFullPath);
const char* PointToName(const char* asFileOrPath);
const wchar_t* PointToExt(const wchar_t* asFullPath);
const wchar_t* Unquote(wchar_t* asParm, bool abFirstQuote = false);

bool IsNewConsoleArg(LPCWSTR lsCmdLine, LPCWSTR pszArg = L"-new_console");
