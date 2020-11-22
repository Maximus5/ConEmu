
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

// CmdArg
struct CmdArg : public CEStr
{
public:
	/// Point to the end dbl quot, if we need drop first and last quotation marks
	const wchar_t* m_pszDequoted = nullptr;
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
	const wchar_t* m_sLastTokenEnd = nullptr;
	wchar_t m_sLastTokenSave[32] = L"";
	#endif

private:
	bool CompareSwitch(const wchar_t* asSwitch, bool caseSensitive = false) const;
	void ReleaseInternal();

public:
	void Clear();
	void Release();

	/// Copy service info from other CmdArg
	/// Used during NextArg evaluation to use different CmdArg objects
	void LoadPosFrom(const CmdArg& arg);

	/// If this may be supported switch like "-run", "/run", "/inside=...", "-regfont:...", etc.
	bool IsPossibleSwitch() const;
	/// For example, compare if ms_Val is "-run", "/run", "/inside=...", "-regfont:...", etc.
	bool IsSwitch(const wchar_t* asSwitch, bool caseSensitive = false) const;
	/// Stops checking on first nullptr
	bool OneOfSwitches(const wchar_t* asSwitch1, const wchar_t* asSwitch2 = nullptr, const wchar_t* asSwitch3 = nullptr, const wchar_t* asSwitch4 = nullptr, const wchar_t* asSwitch5 = nullptr, const wchar_t* asSwitch6 = nullptr, const wchar_t* asSwitch7 = nullptr, const wchar_t* asSwitch8 = nullptr, const wchar_t* asSwitch9 = nullptr, const wchar_t* asSwitch10 = nullptr) const;
	/// <summary>
	/// Returns tail of argument after ":" or "=". <br>
	/// Examples: "0xABC" for switch "-inside:0xABC"
	/// </summary>
	/// <returns>non-null string</returns>
	const wchar_t* GetExtra() const;

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
