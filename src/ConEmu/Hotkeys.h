
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

#include "HotkeyChord.h"

// Strings
#define gsNoHotkey L"<None>"

// Forward
struct ConEmuHotKey;
class CHotKeyDialog;

// Некоторые комбинации нужно обрабатывать "на отпускание" во избежание глюков с интерфейсом
extern const struct ConEmuHotKey* ConEmuSkipHotKey; // = ((ConEmuHotKey*)INVALID_HANDLE_VALUE)

#define CEHOTKEY_MODMASK    0xFFFFFF00
#define CEHOTKEY_NUMHOSTKEY 0xFFFFFF00
#define CEHOTKEY_ARRHOSTKEY 0xFEFEFE00
#define CEHOTKEY_NOMOD      0x80808000

enum ConEmuHotKeyType
{
	chk_User = 0,  // обычный настраиваемый hotkey
	chk_Modifier,  // для драга, например
	chk_Modifier2, // для драга, например (когда нужно задать более одного модификатора)
	chk_NumHost,   // system hotkey (<HostKey>-Number, и БЫЛ РАНЬШЕ <HostKey>-Arrows)
	chk_ArrHost,   // system hotkey (<HostKey>-Number, и БЫЛ РАНЬШЕ <HostKey>-Arrows)
	chk_System,    // predefined hotkeys, ненастраиваемые (пока?)
	chk_Global,    // globally registered hotkey
	chk_Local,     // locally registered hotkey
	chk_Macro,     // GUI Macro
	chk_Task,      // Task hotkey
};

// Check if enabled in current context
typedef bool (*HotkeyEnabled_t)();
// true-обработали, false-пропустить в консоль
typedef bool (WINAPI *HotkeyFKey_t)(const ConEmuChord& VkState, bool TestOnly, const ConEmuHotKey* hk, CRealConsole* pRCon); // true-обработали, false-пропустить в консоль

struct ConEmuHotKey
{
	// >0 StringTable resource ID
	// <0 TaskIdx, 1-based
	int DescrLangID;

	// 0 - hotkey, 1 - modifier (для драга, например), 2 - system hotkey (настройка nMultiHotkeyModifier)
	ConEmuHotKeyType HkType;

	// May be NULL
	HotkeyEnabled_t Enabled;

	wchar_t Name[64];

	ConEmuChord Key;
	DWORD GetVkMod() const;
	void  SetVkMod(DWORD VkMod);

	HotkeyFKey_t fkey; // true-обработали, false-пропустить в консоль
	bool OnKeyUp; // Некоторые комбинации нужно обрабатывать "на отпускание" (показ диалогов, меню, ...)

	wchar_t* GuiMacro;

	// May be NULL. if "true" - dont intercept this key with KeyboardHooks, try to pass it to Windows.
	bool   (*DontWinHook)(const ConEmuHotKey* pHK);

	// Internal
	size_t cchGuiMacroMax;
	bool   NotChanged;

	bool CanChangeVK() const;
	bool IsTaskHotKey() const;
	int GetTaskIndex() const; // 0-based
	void SetTaskIndex(int iTaskIdx); // 0-based
	void SetHotKey(BYTE Vk, BYTE vkMod1=0, BYTE vkMod2=0, BYTE vkMod3=0);
	bool Equal(BYTE Vk, BYTE vkMod1=0, BYTE vkMod2=0, BYTE vkMod3=0);

	LPCWSTR GetDescription(wchar_t* pszDescr, int cchMaxLen, bool bAddMacroIndex = false) const;

	void Free();

	// *** Service functions ***
	// Вернуть имя модификатора (типа "Apps+Space")
	LPCWSTR GetHotkeyName(wchar_t (&szFull)[128], bool bShowNone = true) const;
	static LPCWSTR GetHotkeyName(DWORD aVkMod, wchar_t (&szFull)[128], bool bShowNone = true);

	// Duplicate hotkeys
	static LPCWSTR CreateNotUniqueWarning(LPCWSTR asHotkey, LPCWSTR asDescr1, LPCWSTR asDescr2, CEStr& rsWarning);

	#ifdef _DEBUG
	static void HotkeyNameUnitTests();
	#endif

	// nHostMod в младших 3-х байтах может содержать VK (модификаторы).
	// Функция проверяет, чтобы они не дублировались
	static void TestHostkeyModifiers(DWORD& nHostMod);
	// набор флагов MOD_xxx для RegisterHotKey
	static DWORD GetHotKeyMod(DWORD VkMod);
	// Сервисная функция для инициализации. Формирует готовый VkMod
	static DWORD MakeHotKey(BYTE Vk, BYTE vkMod1=0, BYTE vkMod2=0, BYTE vkMod3=0);
	// Извлечь сам VK
	static DWORD GetHotkey(DWORD VkMod);
	// Вернуть имя клавишы (Apps, Win, Pause, ...)
	static void GetVkKeyName(BYTE vk, wchar_t (&szName)[32], bool bSingle = true);
	// Вернуть VK по имени клавиши (Apps, Win, Pause, ...)
	static UINT GetVkByKeyName(LPCWSTR asName, int* pnScanCode = NULL, DWORD* pnControlState = NULL);
	// Есть ли в этом (VkMod) хоткее - модификатор Mod (VK)
	static bool HasModifier(DWORD VkMod, BYTE Mod/*VK*/);
	// Задать или сбросить модификатор в VkMod
	static DWORD SetModifier(DWORD VkMod, BYTE Mod/*VK*/, bool Xor=true);
	// Вернуть назначенные модификаторы (idx = 1..3). Возвращает 0 (нету) или VK
	static DWORD GetModifier(DWORD VkMod, int idx/*1..3*/);

	static bool UseWinNumber();
	static bool UseWinArrows();
	static bool UseCTSShiftArrow(); // { return gpSet->isUseWinArrows; }; // { return (OverrideClipboard || !AppNames) ? isCTSShiftArrowStart : gpSet->AppStd.isCTSShiftArrowStart; };
	static bool UseCtrlTab();
	static bool UseCtrlBS();
	static bool InSelection();
	static bool UseDndLKey();
	static bool UseDndRKey();
	static bool UseWndDragKey();
	static bool UsePromptFind();
	//static bool DontHookJumps(const ConEmuHotKey* pHK);
};
