
/*
Copyright (c) 2014-2017 Maximus5
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

struct ConEmuHotKey;

typedef DWORD CETASKFLAGS;
const CETASKFLAGS
	CETF_NEW_DEFAULT    = 0x0001,
	CETF_CMD_DEFAULT    = 0x0002,
	CETF_NO_TASKBAR     = 0x0004,
	CETF_ADD_TOOLBAR    = 0x0008,
	CETF_FLAGS_MASK     = 0xFFFF, // Supported flags mask
	CETF_DEFAULT4NEW    = CETF_NO_TASKBAR, // Default flags for newly created task
	CETF_MAKE_UNIQUE    = 0x40000000,
	CETF_DONT_CHANGE    = 0x80000000,
	CETF_NONE           = 0;

struct CommandTasks
{
	size_t   cchNameMax;
	wchar_t* pszName;
	size_t   cchGuiArgMax;
	wchar_t* pszGuiArgs;
	size_t   cchCmdMax;
	wchar_t* pszCommands; // "\r\n" separated commands

	ConEmuHotKey HotKey;

	CETASKFLAGS Flags;

	void FreePtr();

	void SetName(LPCWSTR asName, int anCmdIndex);

	void SetGuiArg(LPCWSTR asGuiArg);

	void SetCommands(LPCWSTR asCommands);

	void ParseGuiArgs(RConStartArgs* pArgs) const;

	bool LoadCmdTask(SettingsBase* reg, int iIndex);
	bool SaveCmdTask(SettingsBase* reg, bool isStartup);
};
