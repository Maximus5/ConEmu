
/*
Copyright (c) 2016-present Maximus5
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

#include "../common/defines.h"
#include "../common/CEStr.h"

struct MacroInstance
{
	HWND  hConEmuWnd;  // Root! window
	DWORD nTabIndex;   // Specially selected tab, 1-based
	DWORD nSplitIndex; // Specially selected split, 1-based
	DWORD nPID;
};

/// Bitmasked flags to execute GuiMacro
enum class GuiMacroFlags
{
	None = 0,
	SetEnvVar = 1,
	ExportEnvVar = 2,
	PrintResult = 4,
	/// Return result via EnvVar only
	PreferSilentMode = 8,
};

/// test if all flags of f2 are set in f1
bool operator&(GuiMacroFlags f1, GuiMacroFlags f2);
/// append flags from f2 to f1
GuiMacroFlags operator|(GuiMacroFlags f1, GuiMacroFlags f2);

void ArgGuiMacro(const CEStr& szArg, MacroInstance& inst);

int DoGuiMacro(LPCWSTR asCmdArg, MacroInstance& inst, GuiMacroFlags flags, BSTR* bsResult = nullptr);

// defined in ExportedFunctions.h
// int __stdcall GuiMacro(LPCWSTR asInstance, LPCWSTR asMacro, BSTR* bsResult = NULL);

int GuiMacroCommandLine(LPCWSTR asCmdLine);
