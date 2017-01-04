
/*
Copyright (c) 2016-2017 Maximus5
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

#include "GuiMacro.h"

enum ConEmuStateCheck
{
	ec_None = 0,
	ec_IsConEmu,
	ec_IsTerm,
	ec_IsAnsi,
	ec_IsAdmin,
	ec_IsRedirect,
};

enum ConEmuExecAction
{
	ea_None = 0,
	ea_RegConFont, // RegisterConsoleFontHKLM
	ea_InjectHooks,
	ea_InjectRemote,
	ea_InjectDefTrm,
	ea_GuiMacro,
	ea_CheckUnicodeFont,
	ea_PrintConsoleInfo,
	ea_TestUnicodeCvt,
	ea_OsVerInfo,
	ea_ExportCon,  // export env.vars to processes of active console
	ea_ExportTab,  // ea_ExportCon + ConEmu window
	ea_ExportGui,  // export env.vars to ConEmu window
	ea_ExportAll,  // export env.vars to all opened tabs of current ConEmu window
	ea_ParseArgs,  // debug test of NextArg function... print args to STDOUT
	ea_ErrorLevel, // return specified errorlevel
	ea_OutEcho,    // echo "string" with ANSI processing
	ea_OutType,    // print file contents with ANSI processing
	ea_StoreCWD,   // store current console work dir
	ea_DumpStruct, // dump file mapping contents
};

int  DoExecAction(ConEmuExecAction eExecAction, LPCWSTR asCmdArg /* rest of cmdline */, MacroInstance& Inst);
int  DoExportEnv(LPCWSTR asCmdArg, ConEmuExecAction eExecAction, bool bSilent = false);
int  DoOutput(ConEmuExecAction eExecAction, LPCWSTR asCmdArg);
bool DoStateCheck(ConEmuStateCheck eStateCheck);

int  WriteOutput(LPCWSTR pszText, DWORD cchLen, DWORD& dwWritten, bool bProcessed, bool bAsciiPrint, bool bStreamBy1, bool bToBottom);
