
/*
Copyright (c) 2009-present Maximus5
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

class CVirtualConsole;

class GlobalHotkeys final
{
public:
	GlobalHotkeys() = default;
	~GlobalHotkeys() = default;

	bool IsKeyboardHookRegistered() const;
	bool IsActiveHotkeyRegistered() const;
	void RegisterHotKeys();
	void RegisterGlobalHotKeys(bool bRegister);
	void UnRegisterHotKeys(bool abFinal=false);
	void GlobalHotKeyChanged();
	void RegisterMinRestore(bool abRegister) const;
	void RegisterHooks();
	void UnRegisterHooks(bool abFinal=false);
	void OnWmHotkey(WPARAM wParam, DWORD nTime = 0) const;
	void UpdateWinHookSettings() const;
	void UpdateActiveGhost(CVirtualConsole* apVCon) const;
	void OnTerminate() const;

	struct RegisteredHotKeys
	{
		int DescrID = 0;
		int RegisteredID = 0; // wParam for WM_HOTKEY
		UINT VK = 0, MOD = 0;     // to react on changes
		BOOL IsRegistered = FALSE;
	};
	
protected:
	HMODULE LoadConEmuCD();
	bool RegisterHotkey(RegisteredHotKeys& hotkey, DWORD vk, DWORD mod, DWORD globalId, const wchar_t* description) const;
	void UnregisterHotkey(RegisteredHotKeys& hotkey) const;

	// hotkeys, which are registered only during ConEmu has focus
	bool mb_HotKeyRegistered = false;

	// variables to support global keyboard hooks
	HMODULE mh_LLKeyHookDll = nullptr;
	HHOOK mh_LLKeyHook = nullptr;
	HWND* mph_HookedGhostWnd = nullptr;
};
