
/*
Copyright (c) 2013-2015 Maximus5
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

#if defined(_MSC_VER) && (_MSC_VER <= 1500)
	#define RVAL_REF
#elif defined(__GNUC__)
	#define RVAL_REF
#else
	#define RVAL_REF &&
#endif


// CEStr
typedef struct CmdArg
{
public:
	wchar_t *ms_Arg;
	INT_PTR mn_MaxLen; // Including termination \0

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

	bool mb_RestoreEnvVar; // Если используется для сохранения переменной окружения, восстановить при закрытии объекта
	wchar_t ms_RestoreVarName[32];

private:
	// Not copyable, not implemented, use explicit Set method
	CmdArg& operator=(const CmdArg &);
	LPCWSTR AttachInt(wchar_t*& asPtr);

public:
	operator LPCWSTR() const { return ms_Arg; };
	CmdArg& operator=(wchar_t* RVAL_REF asPtr);
	CmdArg& operator=(const wchar_t* asPtr);

	wchar_t* GetBuffer(INT_PTR cchMaxLen);
	wchar_t* Detach();
	LPCWSTR  Attach(wchar_t* RVAL_REF asPtr);
	void Empty();
	bool IsEmpty();
	LPCWSTR Set(LPCWSTR asNewValue, INT_PTR anChars = -1);
	void SavePathVar(LPCWSTR asCurPath);
	void SaveEnvVar(LPCWSTR asVarName, LPCWSTR asNewValue);
	void SetAt(INT_PTR nIdx, wchar_t wc);

	void GetPosFrom(const CmdArg& arg);

	CmdArg();
	CmdArg(wchar_t* RVAL_REF asPtr);
	~CmdArg();
} CEStr;
