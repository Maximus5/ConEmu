
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

#include "HotkeyChord.h"

// Forward
struct ConEmuHotKey;
class CHotKeyDialog;
class CRealConsole;

// Некоторые комбинации нужно обрабатывать "на отпускание" во избежание глюков с интерфейсом
extern const struct ConEmuHotKey* ConEmuSkipHotKey; // = ((ConEmuHotKey*)INVALID_HANDLE_VALUE)

// Check if enabled in current context
typedef bool (*HotkeyEnabled_t)();
typedef bool (*WinHookEnabled_t)(const ConEmuHotKey* pHK);
// true - processed, false - pass to console
typedef bool (WINAPI *HotkeyFKey_t)(const ConEmuChord& VkState, bool TestOnly, const ConEmuHotKey* hk, CRealConsole* pRCon);

struct ConEmuHotKey
{
public:
	// >0 StringTable resource ID
	// <0 TaskIdx, 1-based
	int DescrLangID{};

	// 0 - hotkey, 1 - modifier (для драга, например), 2 - system hotkey (настройка nMultiHotkeyModifier)
	ConEmuHotKeyType HkType{};

	// May be NULL
	HotkeyEnabled_t Enabled{ nullptr };

	wchar_t Name[64] = L"";

	ConEmuChord Key{};

	HotkeyFKey_t fkey{nullptr}; // true-обработали, false-пропустить в консоль
	bool OnKeyUp{false}; // Некоторые комбинации нужно обрабатывать "на отпускание" (показ диалогов, меню, ...)

	wchar_t* GuiMacro{nullptr};

	// Internal
	size_t cchGuiMacroMax{};
	bool   NotChanged{};

public:
	DWORD GetVkMod() const;
	bool CanChangeVK() const;
	bool IsTaskHotKey() const;
	int GetTaskIndex() const; // 0-based
	void SetTaskIndex(int iTaskIdx); // 0-based
	bool Equal(BYTE vk, BYTE vkMod1=0, BYTE vkMod2=0, BYTE vkMod3=0) const;

	ConEmuHotKey& SetVkMod(DWORD VkMod);
	ConEmuHotKey& SetHotKey(BYTE vk, BYTE vkMod1 = 0, BYTE vkMod2 = 0, BYTE vkMod3 = 0);
	ConEmuHotKey& SetEnabled(HotkeyEnabled_t enabledFunc);
	ConEmuHotKey& SetOnKeyUp();
	ConEmuHotKey& SetMacro(const wchar_t* guiMacro);
	

	LPCWSTR GetDescription(wchar_t* pszDescr, int cchMaxLen, bool bAddMacroIndex = false) const;

	void Free();

	// Return user-friendly key name (e.g. "Apps+Space")
	LPCWSTR GetHotkeyName(wchar_t (&szFull)[128], bool bShowNone = true) const;

	// Duplicate hotkeys
	static LPCWSTR CreateNotUniqueWarning(LPCWSTR asHotkey, LPCWSTR asDescr1, LPCWSTR asDescr2, CEStr& rsWarning);

	static bool UseWinNumber();
	static bool UseWinArrows();
	/// \brief Process hotkeys to tile window and move window accross monitors
	static bool UseWinMove();
	static bool UseCTSShiftArrow(); // { return gpSet->isUseWinArrows; }; // { return (OverrideClipboard || !AppNames) ? isCTSShiftArrowStart : gpSet->AppStd.isCTSShiftArrowStart; };
	static bool UseCtrlTab();
	static bool UseCtrlBS();
	static bool InSelection();
	static bool UseDndLKey();
	static bool UseDndRKey();
	static bool UseWndDragKey();
	static bool UsePromptFind();
};
