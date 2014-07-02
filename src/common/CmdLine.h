
/*
Copyright (c) 2013-2014 Maximus5
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

#define CmdEscapeNeededChars  L"<>()&|^\""
#define QuotationNeededChars  (L" " CmdEscapeNeededChars)

struct CmdArg
{
public:
	INT_PTR mn_MaxLen;
	wchar_t *ms_Arg;

	// Point to the end dblquot
	LPCWSTR mpsz_Dequoted;
	// if 0 - this is must be first call (first token of command line)
	// so, we need to test for mpsz_Dequoted
	int mn_TokenNo;
	// To bee able corretly parse double quotes in commands like
	// "C:\Windows\system32\cmd.exe" /C ""C:\Python27\python.EXE""
	// "reg.exe add "HKEY_CLASSES_ROOT\Directory\Background\shell\Command Prompt\command" /ve /t REG_EXPAND_SZ /d "\"D:\Applications\ConEmu\ConEmuPortable.exe\" /Dir \"%V\" /cmd \"cmd.exe\" \"-new_console:nC:cmd.exe\" \"-cur_console:d:%V\"" /f"
	enum { cc_Undefined, cc_CmdExeFound, cc_CmdCK, cc_CmdCommand } mn_CmdCall;

	#ifdef _DEBUG
	// Debug, для отлова "не сброшенных" вызовов
	LPCWSTR ms_LastTokenEnd;
	wchar_t ms_LastTokenSave[32];
	#endif

	bool mb_RestorePath; // Если используется для сохранения переменной %PATH%, восстановить при закрытии объекта

public:
	operator LPCWSTR() const { return ms_Arg; };

	wchar_t* GetBuffer(INT_PTR cchMaxLen);
	wchar_t* Detach();
	LPCWSTR  Attach(wchar_t* asPtr);
	void Empty();
	bool IsEmpty();
	LPCWSTR Set(LPCWSTR asNewValue, int anChars = -1);
	void SavePathVar(LPCWSTR asCurPath);
	void SetAt(INT_PTR nIdx, wchar_t wc);

	void GetPosFrom(const CmdArg& arg);

	CmdArg();
	~CmdArg();
};

int NextArg(const wchar_t** asCmdLine, CmdArg &rsArg, const wchar_t** rsArgStart=NULL);

int AddEndSlash(wchar_t* rsPath, int cchMax);
const wchar_t* SkipNonPrintable(const wchar_t* asParams);
bool CompareFileMask(const wchar_t* asFileName, const wchar_t* asMask);
LPCWSTR GetDrive(LPCWSTR pszPath, wchar_t* szDrive, int/*countof(szDrive)*/ cchDriveMax);

bool IsExecutable(LPCWSTR aszFilePathName, wchar_t** rsExpandedVars = NULL);
bool IsFarExe(LPCWSTR asModuleName);
bool IsNeedCmd(BOOL bRootCmd, LPCWSTR asCmdLine, CmdArg &szExe,
			   LPCWSTR* rsArguments = NULL, BOOL* rpbNeedCutStartEndQuot = NULL,
			   BOOL* rpbRootIsCmdExe = NULL, BOOL* rpbAlwaysConfirmExit = NULL, BOOL* rpbAutoDisableConfirmExit = NULL);
bool IsQuotationNeeded(LPCWSTR pszPath);
bool ProcessSetEnvCmd(LPCWSTR& asCmdLine, bool bDoSet, CmdArg* rpsTitle = NULL);

bool FileExistsSearch(LPCWSTR asFilePath, CmdArg& rsFound, bool abSetPath = true, bool abRegSearch = true);

#ifndef CONEMU_MINIMAL
bool SearchAppPaths(LPCWSTR asFilePath, CmdArg& rsFound, bool abSetPath, CmdArg* rpsPathRestore = NULL);
#endif

wchar_t* MergeCmdLine(LPCWSTR asExe, LPCWSTR asParams);
