
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

#define HIDE_USE_EXCEPTION_INFO

#include "Header.h"
#include "HotkeyList.h"
#include "ConEmuCtrl.h"
#include "Options.h"
#include "../ConEmuCD/GuiHooks.h"

ConEmuHotKey* ConEmuHotKeyList::Add(int DescrLangID, ConEmuHotKeyType HkType, HotkeyEnabled_t Enabled, LPCWSTR Name, HotkeyFKey_t fkey, bool OnKeyUp, LPCWSTR GuiMacro)
{
	static ConEmuHotKey dummy = {};
	ConEmuHotKey* p = &dummy;

	#ifdef _DEBUG
	for (INT_PTR i = 0; i < size(); i++)
	{
		if ((*this)[i].DescrLangID == DescrLangID)
		{
			_ASSERTE(FALSE && "This item was already added!");
		}
	}
	#endif

	INT_PTR iNew = this->push_back(dummy);
	if (iNew >= 0)
	{
		p = &((*this)[iNew]);
		memset(p, 0, sizeof(*p));
		p->DescrLangID = DescrLangID;
		p->HkType = HkType;
		p->Enabled = Enabled;
		lstrcpyn(p->Name, Name, countof(p->Name));
		p->fkey = fkey;
		p->OnKeyUp = OnKeyUp;
		p->GuiMacro = GuiMacro ? lstrdup(GuiMacro) : NULL;
	}

	return p;
}

void ConEmuHotKeyList::UpdateNumberModifier()
{
	ConEmuModifiers Mods = cvk_NumHost|CEVkMatch::GetFlagsFromMod(gpSet->HostkeyNumberModifier());

	for (INT_PTR i = this->size(); i >= 0; i--)
	{
		ConEmuHotKey& hk = (*this)[i];
		if (hk.HkType == chk_NumHost)
			hk.Key.Mod = Mods;
	}
}

void ConEmuHotKeyList::UpdateArrowModifier()
{
	ConEmuModifiers Mods = cvk_ArrHost|CEVkMatch::GetFlagsFromMod(gpSet->HostkeyArrowModifier());

	for (INT_PTR i = this->size(); i >= 0; i--)
	{
		ConEmuHotKey& hk = (*this)[i];
		if (hk.HkType == chk_ArrHost)
			hk.Key.Mod = Mods;
	}
}

void ConEmuHotKeyList::ReleaseHotkeys()
{
	for (int i = size() - 1; i >= 0; i--)
	{
		SafeFree((*this)[i].GuiMacro);
	}
	clear();
}

const ConEmuHotKey* ConEmuHotKeyList::GetHotKeyPtr(int idx)
{
	const ConEmuHotKey* pHK = NULL;

	if (idx >= 0 && this)
	{
		if (idx < size())
		{
			pHK = &((*this)[idx]);
		}
		else
		{
			int iHotkeys = size();
			const CommandTasks* pCmd = gpSet->CmdTaskGet(idx - iHotkeys);
			if (pCmd)
			{
				_ASSERTE(pCmd->HotKey.HkType==chk_Task && pCmd->HotKey.GetTaskIndex()==(idx-iHotkeys));
				pHK = &pCmd->HotKey;
			}
		}
	}

	return pHK;
}

// pRCon may be NULL
// Returns either NULL or valid pointer to ConEmuHotKey
const ConEmuHotKey* ConEmuHotKeyList::FindHotKey(const ConEmuChord& VkState, CRealConsole* pRCon)
{
	// На сами модификаторы - действий не вешается
	switch (VkState.Vk)
	{
	case VK_LWIN: case VK_RWIN:
	case VK_SHIFT: case VK_LSHIFT: case VK_RSHIFT:
	case VK_CONTROL: case VK_LCONTROL: case VK_RCONTROL:
	case VK_MENU: case VK_LMENU: case VK_RMENU:
		return NULL;
	case 0:
		_ASSERTE(VkState.Vk!=0);
		return NULL;
	}

	const ConEmuHotKey* p = NULL;

	// Теперь бежим по mp_HotKeys и сравниваем требуемые модификаторы
	for (int i = 0;; i++)
	{
		const ConEmuHotKey* pi = GetHotKeyPtr(i);
		if (!pi)
			break;

		if (pi->HkType == chk_Modifier)
			continue;

		// Hotkey was not set in settings?
		if (!pi->Key.Vk)
			continue;

		// May be disabled by settings or context?
		if (pi->Enabled)
		{
			if (!pi->Enabled())
				continue;
		}

		// Do compare (chord keys are planned)
		if (!pi->Key.IsEqual(VkState))
			continue;

		// The function
		if (pi->fkey)
		{
			// Допускается ли этот хоткей в текущем контексте?
			if (pi->fkey(VkState, true, pi, pRCon))
			{
				p = pi;
				break; // Нашли
			}
		}
		else
		{
			// Хоткей должен знать, что он "делает"
			_ASSERTE(pi->fkey!=NULL);
		}
	}

	return p;
}

// pRCon may be NULL
// Returns: NULL, ConEmuSkipHotKey, or valid pointer to ConEmuHotKey
const ConEmuHotKey* ConEmuHotKeyList::GetHotKeyInfo(const ConEmuChord& VkState, bool bKeyDown, CRealConsole* pRCon)
{
	const ConEmuHotKey* p = FindHotKey(VkState, pRCon);

	// Некоторые комбинации нужно обрабатывать "на отпускание" во избежание глюков с интерфейсом
	if (p)
	{
		// Поэтому проверяем, совпадает ли требование "нажатости"
		if (p->OnKeyUp == bKeyDown)
			p = ConEmuSkipHotKey;
	}

	return p;
}

int ConEmuHotKeyList::AllocateHotkeys()
{
	// All hotkeys list

	// Ранее у nLDragKey,nRDragKey был тип DWORD

	_ASSERTE(this->empty());

	/*
		*** User (Keys, Global) -- Добавить chk_Global недостаточно, нужно еще и gRegisteredHotKeys обработать
	*/
	Add(vkMinimizeRestore,chk_Global, NULL,   L"MinimizeRestore",       CConEmuCtrl::key_MinimizeRestore)
		->SetHotKey(VK_OEM_3/*~*/,VK_CONTROL);
	Add(vkMinimizeRestor2,chk_Global, NULL,   L"MinimizeRestore2",      CConEmuCtrl::key_MinimizeRestore)
		;
	Add(vkGlobalRestore,  chk_Global, NULL,   L"GlobalRestore",         CConEmuCtrl::key_GlobalRestore)
		;
	Add(vkCdExplorerPath, chk_Global, NULL,   L"CdExplorerPath",        CConEmuCtrl::key_GuiMacro, false, L"PasteExplorerPath(1,1)")
		;
	Add(vkForceFullScreen,chk_Global, NULL,   L"ForcedFullScreen",      CConEmuCtrl::key_ForcedFullScreen)
		->SetHotKey(VK_RETURN,VK_CONTROL,VK_LWIN,VK_MENU);
	/*
		-- Добавить chk_Local недостаточно, нужно еще и gActiveOnlyHotKeys обработать
	*/
	Add(vkSetFocusSwitch, chk_Local,  NULL,   L"SwitchGuiFocus",        CConEmuCtrl::key_SwitchGuiFocus)
		->SetHotKey('Z',VK_LWIN);
	Add(vkSetFocusGui,    chk_Local,  NULL,   L"SetFocusGui",           CConEmuCtrl::key_SwitchGuiFocus)
		;
	Add(vkSetFocusChild,  chk_Local,  NULL,   L"SetFocusChild",         CConEmuCtrl::key_SwitchGuiFocus)
		;
	Add(vkChildSystemMenu,chk_Local,  NULL,   L"ChildSystemMenu",       CConEmuCtrl::key_ChildSystemMenu);
	/*
		*** User (Keys)
	*/
	Add(vkMultiNew,       chk_User,  NULL,    L"Multi.NewConsole",      CConEmuCtrl::key_MultiNew) // it can be used to create multiple consoles by holding Win+W
		->SetHotKey('W',VK_LWIN);
	Add(vkMultiNewShift,  chk_User,  NULL,    L"Multi.NewConsoleShift", CConEmuCtrl::key_MultiNewShift)
		->SetHotKey('W',VK_LWIN,VK_SHIFT);
	Add(vkMultiCmd,       chk_User,  NULL,    L"Multi.CmdKey",          CConEmuCtrl::key_MultiCmd)
		->SetHotKey('X',VK_LWIN);
	Add(vkMultiNewWnd,    chk_User,  NULL,    L"Multi.NewWindow",       CConEmuCtrl::key_MultiNewWindow)
		;
	Add(vkMultiNewPopup,  chk_User,  NULL,    L"Multi.NewConsolePopup", CConEmuCtrl::key_MultiNewPopupMenu)
		->SetHotKey('N',VK_LWIN);
	Add(vkMultiNewPopup2, chk_User,  NULL,    L"Multi.NewConsolePopup2",CConEmuCtrl::key_MultiNewPopupMenu2)
		;
	Add(vkMultiNewAttach, chk_User,  NULL,    L"Multi.NewAttach",       CConEmuCtrl::key_MultiNewAttach, true/*OnKeyUp*/)
		->SetHotKey('G',VK_LWIN,VK_SHIFT);
	/*
			Splitters
	*/
	Add(vkSplitNewConV,   chk_User,  NULL,    L"Multi.NewSplitV",       CConEmuCtrl::key_GuiMacro, false, L"Split(0,0,50)")
		->SetHotKey('O',VK_CONTROL,VK_SHIFT);
	Add(vkSplitNewConH,   chk_User,  NULL,    L"Multi.NewSplitH",       CConEmuCtrl::key_GuiMacro, false, L"Split(0,50,0)")
		->SetHotKey('E',VK_CONTROL,VK_SHIFT);
	Add(vkMaximizePane,  chk_User,  NULL,     L"Multi.SplitMaximize",   CConEmuCtrl::key_GuiMacro, false, L"Split(3)") // Maximize/restore active split
		->SetHotKey(VK_RETURN, VK_APPS);
	Add(vkSplitSizeVup,   chk_User,  NULL,    L"Multi.SplitSizeVU",     CConEmuCtrl::key_GuiMacro, false, L"Split(1,0,-1)")
		->SetHotKey(VK_UP,VK_APPS,VK_SHIFT);
	Add(vkSplitSizeVdown, chk_User,  NULL,    L"Multi.SplitSizeVD",     CConEmuCtrl::key_GuiMacro, false, L"Split(1,0,1)")
		->SetHotKey(VK_DOWN,VK_APPS,VK_SHIFT);
	Add(vkSplitSizeHleft, chk_User,  NULL,    L"Multi.SplitSizeHL",     CConEmuCtrl::key_GuiMacro, false, L"Split(1,-1,0)")
		->SetHotKey(VK_LEFT,VK_APPS,VK_SHIFT);
	Add(vkSplitSizeHright,chk_User,  NULL,    L"Multi.SplitSizeHR",     CConEmuCtrl::key_GuiMacro, false, L"Split(1,1,0)")
		->SetHotKey(VK_RIGHT,VK_APPS,VK_SHIFT);
	Add(vkTabPane,        chk_User,  NULL,    L"Key.TabPane1",          CConEmuCtrl::key_GuiMacro, false/*OnKeyUp*/, L"Tab(10,1)") // Next visible pane
		->SetHotKey(VK_TAB,VK_APPS);
	Add(vkTabPaneShift,   chk_User,  NULL,    L"Key.TabPane2",          CConEmuCtrl::key_GuiMacro, false/*OnKeyUp*/, L"Tab(10,-1)") // Prev visible pane
		->SetHotKey(VK_TAB,VK_APPS,VK_SHIFT);
	Add(vkSplitFocusUp,   chk_User,  NULL,    L"Multi.SplitFocusU",     CConEmuCtrl::key_GuiMacro, false, L"Split(2,0,-1)")
		->SetHotKey(VK_UP,VK_APPS);
	Add(vkSplitFocusDown, chk_User,  NULL,    L"Multi.SplitFocusD",     CConEmuCtrl::key_GuiMacro, false, L"Split(2,0,1)")
		->SetHotKey(VK_DOWN,VK_APPS);
	Add(vkSplitFocusLeft, chk_User,  NULL,    L"Multi.SplitFocusL",     CConEmuCtrl::key_GuiMacro, false, L"Split(2,-1,0)")
		->SetHotKey(VK_LEFT,VK_APPS);
	Add(vkSplitFocusRight,chk_User,  NULL,    L"Multi.SplitFocusR",     CConEmuCtrl::key_GuiMacro, false, L"Split(2,1,0)")
		->SetHotKey(VK_RIGHT,VK_APPS);
	Add(vkSplitSwap,      chk_User,  NULL,    L"Multi.SplitSwap",       CConEmuCtrl::key_GuiMacro, false, L"Split(4)")
		->SetHotKey('X',VK_APPS,VK_MENU);
	Add(vkSplitSwapUp,    chk_User,  NULL,    L"Multi.SplitSwapU",      CConEmuCtrl::key_GuiMacro, false, L"Split(4,0,-1)")
		->SetHotKey(VK_UP,VK_APPS,VK_MENU);
	Add(vkSplitSwapDown,  chk_User,  NULL,    L"Multi.SplitSwapD",      CConEmuCtrl::key_GuiMacro, false, L"Split(4,0,1)")
		->SetHotKey(VK_DOWN,VK_APPS,VK_MENU);
	Add(vkSplitSwapLeft,  chk_User,  NULL,    L"Multi.SplitSwapL",      CConEmuCtrl::key_GuiMacro, false, L"Split(4,-1,0)")
		->SetHotKey(VK_LEFT,VK_APPS,VK_MENU);
	Add(vkSplitSwapRight, chk_User,  NULL,    L"Multi.SplitSwapR",      CConEmuCtrl::key_GuiMacro, false, L"Split(4,1,0)")
		->SetHotKey(VK_RIGHT,VK_APPS,VK_MENU);
	Add(vkMultiNext,      chk_User,  NULL,    L"Multi.Next",            CConEmuCtrl::key_MultiNext)
		->SetHotKey('Q',VK_LWIN,VK_SHIFT);
	Add(vkMultiNextShift, chk_User,  NULL,    L"Multi.NextShift",       CConEmuCtrl::key_MultiPrev)
		;
	Add(vkMultiRecreate,  chk_User,  NULL,    L"Multi.Recreate",        CConEmuCtrl::key_MultiRecreate)
		->SetHotKey(192/*VK_тильда*/,VK_LWIN);
	Add(vkMultiAltCon,    chk_User,  NULL,    L"Multi.AltCon",          CConEmuCtrl::key_AlternativeBuffer)
		->SetHotKey('A',VK_LWIN);
	Add(vkMultiPause,     chk_User,  NULL,    L"Multi.Pause",           CConEmuCtrl::key_GuiMacro, false, L"Pause")
		->SetHotKey(VK_PAUSE);
	Add(vkMultiBuffer,    chk_User,  NULL,    L"Multi.Scroll",          CConEmuCtrl::key_MultiBuffer)
		;
	Add(vkMultiGroup,     chk_User,  NULL,    L"Multi.GroupInput",      CConEmuCtrl::key_GuiMacro, false, L"GroupInput(0)")
		->SetHotKey('G', VK_APPS);
	Add(vkMultiGroupAll,  chk_User,  NULL,    L"Multi.GroupInputAll",   CConEmuCtrl::key_GuiMacro, false, L"GroupInput(3)")
		->SetHotKey('G', VK_APPS, VK_SHIFT);
	Add(vkMultiGroupKey,  chk_User,  NULL,    L"Multi.GroupInputKey",   CConEmuCtrl::key_GuiMacro, false, L"GroupInput(6)")
		->SetHotKey('G', VK_APPS, VK_MENU);
	Add(vkConDetach,      chk_User,  NULL,    L"Multi.Detach",          CConEmuCtrl::key_GuiMacro, false, L"Detach")
		;
	Add(vkConUnfasten,    chk_User,  NULL,    L"Multi.Unfasten",        CConEmuCtrl::key_GuiMacro, false, L"Unfasten")
		;
	Add(vkMultiClose,     chk_User,  NULL,    L"Multi.Close",           CConEmuCtrl::key_GuiMacro, false, L"Close(0)")
		->SetHotKey(VK_DELETE,VK_LWIN); // Close active console
	Add(vkCloseTab,       chk_User,  NULL,    L"CloseTabKey",           CConEmuCtrl::key_GuiMacro, false, L"Close(6)")
		->SetHotKey(VK_DELETE,VK_LWIN,VK_MENU); // Close tab (may be editor/viewer too)
	Add(vkCloseGroup,     chk_User,  NULL,    L"CloseGroupKey",         CConEmuCtrl::key_GuiMacro, false, L"Close(4)")
		->SetHotKey(VK_DELETE,VK_LWIN,VK_CONTROL); // Close active group
	Add(vkCloseGroupPrc,  chk_User,  NULL,    L"CloseGroupPrcKey",      CConEmuCtrl::key_GuiMacro, false, L"Close(7)")
		;
	Add(vkCloseAllCon,    chk_User,  NULL,    L"CloseAllConKey",        CConEmuCtrl::key_GuiMacro, false, L"Close(8)")
		;
	Add(vkCloseZombies,    chk_User,  NULL,   L"CloseZombiesKey",       CConEmuCtrl::key_GuiMacro, false, L"Close(9)")
		;
	Add(vkCloseExceptCon, chk_User,  NULL,    L"CloseExceptConKey",     CConEmuCtrl::key_GuiMacro, false, L"Close(5)")
		;
	Add(vkTerminateApp,   chk_User,  NULL,    L"KillProcessKey",        CConEmuCtrl::key_GuiMacro, false, L"Close(1)")
		->SetHotKey(VK_CANCEL,VK_CONTROL,VK_MENU); // Ctrl+Alt+Break
	Add(vkTermButShell,   chk_User,  NULL,    L"KillAllButShellKey",    CConEmuCtrl::key_GuiMacro, false, L"Close(10,1)")
		->SetHotKey(VK_PAUSE,VK_LWIN,VK_MENU); // Win+Alt+Break
	Add(vkDuplicateRoot,  chk_User,  NULL,    L"DuplicateRootKey",      CConEmuCtrl::key_DuplicateRoot)
		->SetHotKey('S',VK_LWIN);
	Add(vkCloseConEmu,    chk_User,  NULL,    L"CloseConEmuKey",        CConEmuCtrl::key_GuiMacro, false, L"Close(2)")
		->SetHotKey(VK_F4,VK_LWIN); // sort of AltF4 for GUI apps
	Add(vkRenameTab,      chk_User,  NULL,    L"Multi.Rename",          CConEmuCtrl::key_RenameTab, true/*OnKeyUp*/)
		->SetHotKey('R',VK_APPS);
	Add(vkAffinity,       chk_User,  NULL,    L"AffinityPriorityKey",   CConEmuCtrl::key_GuiMacro, true/*OnKeyUp*/, L"AffinityPriority")
		->SetHotKey('A',VK_APPS);
	Add(vkMoveTabLeft,    chk_User,  NULL,    L"Multi.MoveLeft",        CConEmuCtrl::key_MoveTabLeft)
		->SetHotKey(VK_LEFT,VK_LWIN,VK_MENU);
	Add(vkMoveTabRight,   chk_User,  NULL,    L"Multi.MoveRight",       CConEmuCtrl::key_MoveTabRight)
		->SetHotKey(VK_RIGHT,VK_LWIN,VK_MENU);
	Add(vkCTSVkBlockStart,chk_User,  NULL,    L"CTS.VkBlockStart",      CConEmuCtrl::key_CTSVkBlockStart) // запуск выделения блока
		;
	Add(vkCTSVkTextStart, chk_User,  NULL,    L"CTS.VkTextStart",       CConEmuCtrl::key_CTSVkTextStart)  // запуск выделения текста
		;
	Add(vkCTSCopyHtml0,   chk_User,  ConEmuHotKey::InSelection, L"CTS.VkCopyFmt0",    CConEmuCtrl::key_GuiMacro, false, L"Copy(0,0)")
		->SetHotKey('C',VK_CONTROL);
	Add(vkCTSCopyHtml1,   chk_User,  ConEmuHotKey::InSelection, L"CTS.VkCopyFmt1",    CConEmuCtrl::key_GuiMacro, false, L"Copy(0,1)")
		->SetHotKey('C',VK_CONTROL,VK_SHIFT);
	Add(vkCTSCopyHtml2,   chk_User,  ConEmuHotKey::InSelection, L"CTS.VkCopyFmt2",    CConEmuCtrl::key_GuiMacro, false, L"Copy(0,2)")
		;
	Add(vkCTSVkCopyAll,   chk_User,  NULL,    L"CTS.VkCopyAll",         CConEmuCtrl::key_GuiMacro, false, L"Copy(1)")
		;
	Add(vkHighlightMouse, chk_User,  NULL,    L"HighlightMouseSwitch",  CConEmuCtrl::key_GuiMacro, false, L"HighlightMouse(1)")
		->SetHotKey('L',VK_APPS);
	Add(vkHighlightMouseX,chk_User,  NULL,    L"HighlightMouseSwitchX", CConEmuCtrl::key_GuiMacro, false, L"HighlightMouse(3)")
		->SetHotKey('X',VK_APPS);
	Add(vkShowTabsList,   chk_User,  NULL,    L"Multi.ShowTabsList",    CConEmuCtrl::key_ShowTabsList)
		;
	Add(vkShowTabsList2,  chk_User,  NULL,    L"Multi.ShowTabsList2",   CConEmuCtrl::key_GuiMacro, false, L"Tabs(8)")
		->SetHotKey(VK_F12,VK_APPS);
	Add(vkPasteText,      chk_User,  NULL,    L"ClipboardVkAllLines",   CConEmuCtrl::key_PasteText)
		->SetHotKey(VK_INSERT,VK_SHIFT);
	Add(vkPasteFirstLine, chk_User,  NULL,    L"ClipboardVkFirstLine",  CConEmuCtrl::key_PasteFirstLine)
		->SetHotKey('V',VK_CONTROL);
	Add(vkAltNumpad,      chk_User,  NULL,    L"Key.AltNumpad",         CConEmuCtrl::key_GuiMacro, false, L"AltNumber(16)")
		;
	Add(vkDeleteLeftWord, chk_User,  ConEmuHotKey::UseCtrlBS, L"DeleteWordToLeft", CConEmuCtrl::key_DeleteWordToLeft)
		->SetHotKey(VK_BACK,VK_CONTROL);
	Add(vkFindTextDlg,    chk_User,  NULL,    L"FindTextKey",           CConEmuCtrl::key_FindTextDlg)
		->SetHotKey('F',VK_APPS);
	Add(vkScreenshot,     chk_User,  NULL,    L"ScreenshotKey",         CConEmuCtrl::key_Screenshot/*, true/ *OnKeyUp*/)
		->SetHotKey('H',VK_LWIN);
	Add(vkScreenshotFull, chk_User,  NULL,    L"ScreenshotFullKey",     CConEmuCtrl::key_ScreenshotFull/*, true/ *OnKeyUp*/)
		->SetHotKey('H',VK_LWIN,VK_SHIFT);
	Add(vkShowStatusBar,  chk_User,  NULL,    L"ShowStatusBarKey",      CConEmuCtrl::key_ShowStatusBar)
		->SetHotKey('S',VK_APPS);
	Add(vkShowTabBar,     chk_User,  NULL,    L"ShowTabBarKey",         CConEmuCtrl::key_ShowTabBar)
		->SetHotKey('T',VK_APPS);
	Add(vkShowCaption,    chk_User,  NULL,    L"ShowCaptionKey",        CConEmuCtrl::key_ShowCaption)
		->SetHotKey('C',VK_APPS);
	Add(vkAlwaysOnTop,    chk_User,  NULL,    L"AlwaysOnTopKey",        CConEmuCtrl::key_AlwaysOnTop)
		;
	Add(vkTransparencyInc,chk_User,  NULL,    L"TransparencyInc",       CConEmuCtrl::key_GuiMacro, false, L"Transparency(1,-20)")
		;
	Add(vkTransparencyDec,chk_User,  NULL,    L"TransparencyDec",       CConEmuCtrl::key_GuiMacro, false, L"Transparency(1,+20)")
		;
	Add(vkTabMenu,        chk_User,  NULL,    L"Key.TabMenu",           CConEmuCtrl::key_TabMenu, true/*OnKeyUp*/) // Tab menu
		->SetHotKey(VK_SPACE,VK_APPS);
	Add(vkTabMenu2,       chk_User,  NULL,    L"Key.TabMenu2",          CConEmuCtrl::key_TabMenu, true/*OnKeyUp*/) // Tab menu
		->SetHotKey(VK_RBUTTON,VK_SHIFT);
	Add(vkMaximize,       chk_User,  NULL,    L"Key.Maximize",          CConEmuCtrl::key_GuiMacro, false, L"WindowMaximize()") // Maximize window
		->SetHotKey(VK_F9,VK_MENU);
	Add(vkMaximizeWidth,  chk_User,  NULL,    L"Key.MaximizeWidth",     CConEmuCtrl::key_GuiMacro, false, L"WindowMode(11)") // Maximize window width
		->SetHotKey(VK_DOWN,VK_LWIN,VK_SHIFT);
	Add(vkMaximizeHeight, chk_User,  NULL,    L"Key.MaximizeHeight",    CConEmuCtrl::key_GuiMacro, false, L"WindowMode(8)") // Maximize window height
		->SetHotKey(VK_UP,VK_LWIN,VK_SHIFT);
	Add(vkTileToLeft,     chk_User,  NULL,    L"Key.TileToLeft",        CConEmuCtrl::key_GuiMacro, false, L"WindowMode(6)"/*,  DontHookJumps*/)
		->SetHotKey(VK_LEFT,VK_LWIN);
	Add(vkTileToRight,    chk_User,  NULL,    L"Key.TileToRight",       CConEmuCtrl::key_GuiMacro, false, L"WindowMode(7)"/*, DontHookJumps*/)
		->SetHotKey(VK_RIGHT,VK_LWIN);
	Add(vkJumpActiveMonitor,chk_User,  NULL,  L"Key.JumpActiveMonitor", CConEmuCtrl::key_GuiMacro, false, L"WindowMode(12)"/*,  DontHookJumps*/)
		;
	Add(vkJumpPrevMonitor,chk_User,  NULL,    L"Key.JumpPrevMonitor",   CConEmuCtrl::key_GuiMacro, false, L"WindowMode(9)"/*,  DontHookJumps*/)
		->SetHotKey(VK_LEFT,VK_LWIN,VK_SHIFT);
	Add(vkJumpNextMonitor,chk_User,  NULL,    L"Key.JumpNextMonitor",   CConEmuCtrl::key_GuiMacro, false, L"WindowMode(10)"/*, DontHookJumps*/)
		->SetHotKey(VK_RIGHT,VK_LWIN,VK_SHIFT);
	Add(vkAltEnter,       chk_User,  NULL,    L"Key.FullScreen",        CConEmuCtrl::key_GuiMacro, false, L"WindowFullscreen()") // Full screen
		->SetHotKey(VK_RETURN,VK_MENU);
	Add(vkSystemMenu,     chk_User,  NULL,    L"Key.SysMenu",           CConEmuCtrl::key_SystemMenu, true/*OnKeyUp*/) // System menu
		->SetHotKey(VK_SPACE,VK_MENU);
	Add(vkSystemMenu2,    chk_User,  NULL,    L"Key.SysMenu2",          CConEmuCtrl::key_SystemMenu, true/*OnKeyUp*/) // System menu
		->SetHotKey(VK_RBUTTON,VK_CONTROL);
	Add(vkDebugProcess,   chk_User,  NULL,    L"Key.DebugProcess",      CConEmuCtrl::key_GuiMacro, true/*OnKeyUp*/, L"Debug(0)")
		->SetHotKey('D',VK_LWIN,VK_SHIFT);
	Add(vkDumpProcess,    chk_User,  NULL,    L"Key.DumpProcess",       CConEmuCtrl::key_GuiMacro, true/*OnKeyUp*/, L"Debug(1)")
		;
	Add(vkDumpTree,       chk_User,  NULL,    L"Key.DumpTree",          CConEmuCtrl::key_GuiMacro, true/*OnKeyUp*/, L"Debug(2)")
		;
	/*
			Scrolling
	*/
	Add(vkCtrlUp,         chk_User,  NULL,    L"Key.BufUp",             CConEmuCtrl::key_BufferScrollUp) // Buffer scroll
		->SetHotKey(VK_UP,VK_CONTROL);
	Add(vkCtrlDown,       chk_User,  NULL,    L"Key.BufDn",             CConEmuCtrl::key_BufferScrollDown) // Buffer scroll
		->SetHotKey(VK_DOWN,VK_CONTROL);
	Add(vkCtrlPgUp,       chk_User,  NULL,    L"Key.BufPgUp",           CConEmuCtrl::key_BufferScrollPgUp) // Buffer scroll
		->SetHotKey(VK_PRIOR,VK_CONTROL);
	Add(vkCtrlPgDn,       chk_User,  NULL,    L"Key.BufPgDn",           CConEmuCtrl::key_BufferScrollPgDn) // Buffer scroll
		->SetHotKey(VK_NEXT,VK_CONTROL);
	Add(vkAppsPgUp,       chk_User,  NULL,    L"Key.BufHfPgUp",         CConEmuCtrl::key_GuiMacro, false, L"Scroll(2,-1)") // Buffer scroll
		->SetHotKey(VK_PRIOR,VK_APPS);
	Add(vkAppsPgDn,       chk_User,  NULL,    L"Key.BufHfPgDn",         CConEmuCtrl::key_GuiMacro, false, L"Scroll(2,+1)") // Buffer scroll
		->SetHotKey(VK_NEXT,VK_APPS);
	Add(vkAppsHome,       chk_User,  NULL,    L"Key.BufTop",            CConEmuCtrl::key_GuiMacro, false, L"Scroll(3,-1)") // Buffer scroll
		->SetHotKey(VK_HOME,VK_APPS);
	Add(vkAppsEnd,        chk_User,  NULL,    L"Key.BufBottom",         CConEmuCtrl::key_GuiMacro, false, L"Scroll(3,+1)") // Buffer scroll
		->SetHotKey(VK_END,VK_APPS);
	Add(vkFindPrevPrompt, chk_User, ConEmuHotKey::UsePromptFind, L"Key.BufPrUp", CConEmuCtrl::key_GuiMacro, false, L"Scroll(5,-1)") // Buffer scroll: Find prompt upward
		->SetHotKey(VK_PRIOR,VK_CONTROL,VK_MENU);
	Add(vkFindNextPrompt, chk_User, ConEmuHotKey::UsePromptFind, L"Key.BufPrDn", CConEmuCtrl::key_GuiMacro, false, L"Scroll(5,+1)") // Buffer scroll: Find prompt downward
		->SetHotKey(VK_NEXT,VK_CONTROL,VK_MENU);
	Add(vkAppsBS,         chk_User,  NULL,    L"Key.BufCursor",         CConEmuCtrl::key_GuiMacro, false, L"Scroll(4)") // Buffer scroll
		->SetHotKey(VK_BACK,VK_APPS);
	Add(vkResetTerminal,  chk_User,  NULL,    L"Key.ResetTerm",         CConEmuCtrl::key_GuiMacro, false, L"Write(\"\\ec\")") // Reset terminal
		;
	/*
			Some internal keys for Far Manager PicView plugin
	*/
	#if 0
	Add(vkPicViewSlide,   chk_User,  NULL,    L"Key.PicViewSlide",      CConEmuCtrl::key_PicViewSlideshow, true/*OnKeyUp*/) // Slideshow in PicView2
		->SetHotKey(VK_PAUSE);
	Add(vkPicViewSlower,  chk_User,  NULL,    L"Key.PicViewSlower",     CConEmuCtrl::key_PicViewSlideshow) // Slideshow in PicView2
		->SetHotKey(0xbd/* -_ */);
	Add(vkPicViewFaster,  chk_User,  NULL,    L"Key.PicViewFaster",     CConEmuCtrl::key_PicViewSlideshow) // Slideshow in PicView2
		->SetHotKey(0xbb/* =+ */);
	#endif
	/*
			Usability
	*/
	Add(vkFontLarger,     chk_User,  NULL,    L"FontLargerKey",         CConEmuCtrl::key_GuiMacro, false, L"FontSetSize(1,2)")
		->SetHotKey(VK_WHEEL_UP,VK_CONTROL);
	Add(vkFontSmaller,    chk_User,  NULL,    L"FontSmallerKey",        CConEmuCtrl::key_GuiMacro, false, L"FontSetSize(1,-2)")
		->SetHotKey(VK_WHEEL_DOWN,VK_CONTROL);
	Add(vkFontOriginal,   chk_User,  NULL,    L"FontOriginalKey",       CConEmuCtrl::key_GuiMacro, false, L"Zoom(100)")
		->SetHotKey(VK_MBUTTON,VK_CONTROL);
	Add(vkPasteFilePath,  chk_User,  NULL,    L"PasteFileKey",          CConEmuCtrl::key_GuiMacro, false, L"Paste(4)")
		->SetHotKey('F',VK_CONTROL,VK_SHIFT);
	Add(vkPasteDirectory, chk_User,  NULL,    L"PastePathKey",          CConEmuCtrl::key_GuiMacro, false, L"Paste(5)")
		->SetHotKey('D',VK_CONTROL,VK_SHIFT);
	Add(vkPasteCygwin,    chk_User,  NULL,    L"PasteCygwinKey",        CConEmuCtrl::key_GuiMacro, false, L"Paste(8)")
		->SetHotKey(VK_INSERT,VK_APPS);
	/*
		*** GUI Macros
	*/
	Add(vkGuiMacro01,      chk_Macro, NULL,    L"KeyMacro01", CConEmuCtrl::key_GuiMacro);
	Add(vkGuiMacro02,      chk_Macro, NULL,    L"KeyMacro02", CConEmuCtrl::key_GuiMacro);
	Add(vkGuiMacro03,      chk_Macro, NULL,    L"KeyMacro03", CConEmuCtrl::key_GuiMacro);
	Add(vkGuiMacro04,      chk_Macro, NULL,    L"KeyMacro04", CConEmuCtrl::key_GuiMacro);
	Add(vkGuiMacro05,      chk_Macro, NULL,    L"KeyMacro05", CConEmuCtrl::key_GuiMacro);
	Add(vkGuiMacro06,      chk_Macro, NULL,    L"KeyMacro06", CConEmuCtrl::key_GuiMacro);
	Add(vkGuiMacro07,      chk_Macro, NULL,    L"KeyMacro07", CConEmuCtrl::key_GuiMacro);
	Add(vkGuiMacro08,      chk_Macro, NULL,    L"KeyMacro08", CConEmuCtrl::key_GuiMacro);
	Add(vkGuiMacro09,      chk_Macro, NULL,    L"KeyMacro09", CConEmuCtrl::key_GuiMacro);
	Add(vkGuiMacro10,      chk_Macro, NULL,    L"KeyMacro10", CConEmuCtrl::key_GuiMacro);
	Add(vkGuiMacro11,      chk_Macro, NULL,    L"KeyMacro11", CConEmuCtrl::key_GuiMacro);
	Add(vkGuiMacro12,      chk_Macro, NULL,    L"KeyMacro12", CConEmuCtrl::key_GuiMacro);
	Add(vkGuiMacro13,      chk_Macro, NULL,    L"KeyMacro13", CConEmuCtrl::key_GuiMacro);
	Add(vkGuiMacro14,      chk_Macro, NULL,    L"KeyMacro14", CConEmuCtrl::key_GuiMacro);
	Add(vkGuiMacro15,      chk_Macro, NULL,    L"KeyMacro15", CConEmuCtrl::key_GuiMacro);
	Add(vkGuiMacro16,      chk_Macro, NULL,    L"KeyMacro16", CConEmuCtrl::key_GuiMacro);
	Add(vkGuiMacro17,      chk_Macro, NULL,    L"KeyMacro17", CConEmuCtrl::key_GuiMacro);
	Add(vkGuiMacro18,      chk_Macro, NULL,    L"KeyMacro18", CConEmuCtrl::key_GuiMacro);
	Add(vkGuiMacro19,      chk_Macro, NULL,    L"KeyMacro19", CConEmuCtrl::key_GuiMacro);
	Add(vkGuiMacro20,      chk_Macro, NULL,    L"KeyMacro20", CConEmuCtrl::key_GuiMacro);
	Add(vkGuiMacro21,      chk_Macro, NULL,    L"KeyMacro21", CConEmuCtrl::key_GuiMacro);
	Add(vkGuiMacro22,      chk_Macro, NULL,    L"KeyMacro22", CConEmuCtrl::key_GuiMacro);
	Add(vkGuiMacro23,      chk_Macro, NULL,    L"KeyMacro23", CConEmuCtrl::key_GuiMacro);
	Add(vkGuiMacro24,      chk_Macro, NULL,    L"KeyMacro24", CConEmuCtrl::key_GuiMacro);
	Add(vkGuiMacro25,      chk_Macro, NULL,    L"KeyMacro25", CConEmuCtrl::key_GuiMacro);
	Add(vkGuiMacro26,      chk_Macro, NULL,    L"KeyMacro26", CConEmuCtrl::key_GuiMacro);
	Add(vkGuiMacro27,      chk_Macro, NULL,    L"KeyMacro27", CConEmuCtrl::key_GuiMacro);
	Add(vkGuiMacro28,      chk_Macro, NULL,    L"KeyMacro28", CConEmuCtrl::key_GuiMacro);
	Add(vkGuiMacro29,      chk_Macro, NULL,    L"KeyMacro29", CConEmuCtrl::key_GuiMacro);
	Add(vkGuiMacro30,      chk_Macro, NULL,    L"KeyMacro30", CConEmuCtrl::key_GuiMacro);
	Add(vkGuiMacro31,      chk_Macro, NULL,    L"KeyMacro31", CConEmuCtrl::key_GuiMacro);
	Add(vkGuiMacro32,      chk_Macro, NULL,    L"KeyMacro32", CConEmuCtrl::key_GuiMacro);
	/*
		*** User (Modifiers)
	*/
	Add(vkCTSVkBlock,     chk_Modifier, NULL, L"CTS.VkBlock") // модификатор запуска выделения мышкой
		->SetHotKey(VK_LMENU);
	Add(vkCTSVkText,      chk_Modifier, NULL, L"CTS.VkText")       // модификатор запуска выделения мышкой
		->SetHotKey(VK_LSHIFT);
	Add(vkCTSVkAct,       chk_Modifier, NULL, L"CTS.VkAct")        // модификатор разрешения действий правой и средней кнопки мышки
		;
	Add(vkCTSVkPromptClk, chk_Modifier, NULL, L"CTS.VkPrompt") // Модификатор позиционирования курсора мышки кликом (cmd.exe prompt)
		;
	Add(vkFarGotoEditorVk,chk_Modifier, NULL, L"FarGotoEditorVk") // модификатор для isFarGotoEditor
		->SetHotKey(VK_LCONTROL);
	Add(vkLDragKey,       chk_Modifier, ConEmuHotKey::UseDndLKey, L"DndLKey")         // модификатор драга левой кнопкой
		;
	Add(vkRDragKey,       chk_Modifier, ConEmuHotKey::UseDndRKey, L"DndRKey")        // модификатор драга правой кнопкой
		->SetHotKey(VK_LCONTROL);
	Add(vkWndDragKey,     chk_Modifier2,ConEmuHotKey::UseWndDragKey, L"WndDragKey", CConEmuCtrl::key_WinDragStart) // модификатор таскания окна мышкой за любое место
		->SetHotKey(VK_LBUTTON,VK_CONTROL,VK_MENU);
	/*
		*** System (predefined, fixed)
	*/
	Add(vkWinAltA,        chk_System, NULL, L"", CConEmuCtrl::key_GuiMacro, true/*OnKeyUp*/, L"About()") // Show ‘About’ dialog
		->SetHotKey('A',VK_LWIN,VK_MENU);
	Add(vkWinAltP,        chk_System, NULL, L"", CConEmuCtrl::key_GuiMacro, true/*OnKeyUp*/, L"Settings()") // Settings dialog
		->SetHotKey('P',VK_LWIN,VK_MENU);
	Add(vkWinAltK,        chk_System, NULL, L"", CConEmuCtrl::key_GuiMacro, true/*OnKeyUp*/, L"Settings(171)") // Setup ‘Hotkeys’
		->SetHotKey('K',VK_LWIN,VK_MENU);
	Add(vkWinAltT,        chk_System, NULL, L"", CConEmuCtrl::key_GuiMacro, true/*OnKeyUp*/, L"Settings(157)") // Setup ‘Tasks’
		->SetHotKey('T',VK_LWIN,VK_MENU);
	Add(vkWinAltH,        chk_System, NULL, L"", CConEmuCtrl::key_GuiMacro, true/*OnKeyUp*/, L"Wiki()") // Show online wiki
		->SetHotKey('H',VK_LWIN,VK_MENU);
	Add(vkWinAltSpace,    chk_System, NULL, L"", CConEmuCtrl::key_SystemMenu, true/*OnKeyUp*/) // Show ConEmu menu
		->SetHotKey(VK_SPACE,VK_LWIN,VK_MENU);
	Add(vkCtrlWinAltSpace,chk_System, NULL, L"", CConEmuCtrl::key_ShowRealConsole) // Show real console
		->SetHotKey(VK_SPACE,VK_CONTROL,VK_LWIN,VK_MENU);
	Add(vkCtrlWinEnter,   chk_System, NULL, L"", CConEmuCtrl::key_GuiMacro, false, L"WindowFullscreen()")
		->SetHotKey(VK_RETURN,VK_LWIN,VK_CONTROL);
	Add(vkCtrlTab,        chk_System, ConEmuHotKey::UseCtrlTab, L"", CConEmuCtrl::key_CtrlTab) // Tab switch
		->SetHotKey(VK_TAB,VK_CONTROL);
	Add(vkCtrlShiftTab,   chk_System, ConEmuHotKey::UseCtrlTab, L"", CConEmuCtrl::key_CtrlShiftTab) // Tab switch
		->SetHotKey(VK_TAB,VK_CONTROL,VK_SHIFT);
	Add(vkCtrlTab_Left,   chk_System, NULL, L"", CConEmuCtrl::key_CtrlTab_Prev) // Tab switch
		->SetHotKey(VK_LEFT,VK_CONTROL);
	Add(vkCtrlTab_Up,     chk_System, NULL, L"", CConEmuCtrl::key_CtrlTab_Prev) // Tab switch
		->SetHotKey(VK_UP,VK_CONTROL);
	Add(vkCtrlTab_Right,  chk_System, NULL, L"", CConEmuCtrl::key_CtrlTab_Next) // Tab switch
		->SetHotKey(VK_RIGHT,VK_CONTROL);
	Add(vkCtrlTab_Down,   chk_System, NULL, L"", CConEmuCtrl::key_CtrlTab_Next) // Tab switch
		->SetHotKey(VK_DOWN,VK_CONTROL);
	Add(vkEscNoConsoles,  chk_System, NULL, L"", CConEmuCtrl::key_MinimizeByEsc, false/*OnKeyUp*/) // Minimize ConEmu by Esc when no open consoles left
		->SetHotKey(VK_ESCAPE);
	Add(vkCTSShiftLeft,   chk_System, ConEmuHotKey::UseCTSShiftArrow, L"", CConEmuCtrl::key_GuiMacro, false, L"Select(0,-1)")
		->SetHotKey(VK_LEFT,VK_SHIFT);
	Add(vkCTSShiftRight,  chk_System, ConEmuHotKey::UseCTSShiftArrow, L"", CConEmuCtrl::key_GuiMacro, false, L"Select(0,1)")
		->SetHotKey(VK_RIGHT,VK_SHIFT);
	Add(vkCTSShiftHome,   chk_System, ConEmuHotKey::UseCTSShiftArrow, L"", CConEmuCtrl::key_GuiMacro, false, L"Select(0,-1,0,-1)")
		->SetHotKey(VK_HOME,VK_SHIFT);
	Add(vkCTSShiftEnd,    chk_System, ConEmuHotKey::UseCTSShiftArrow, L"", CConEmuCtrl::key_GuiMacro, false, L"Select(0,1,0,1)")
		->SetHotKey(VK_END,VK_SHIFT);
	Add(vkCTSShiftUp,     chk_System, ConEmuHotKey::UseCTSShiftArrow, L"", CConEmuCtrl::key_GuiMacro, false, L"Select(1,0,-1)")
		->SetHotKey(VK_UP,VK_SHIFT);
	Add(vkCTSShiftDown,   chk_System, ConEmuHotKey::UseCTSShiftArrow, L"", CConEmuCtrl::key_GuiMacro, false, L"Select(1,0,1)")
		->SetHotKey(VK_DOWN,VK_SHIFT);
	// Все что ниже - было привязано к "HostKey"
	/*
		*** [HostKey]+Arrows. Only [HostKey] (modifiers actually) can be configured
			Resize ConEmu window with Win+Arrows
	*/
	Add(vkWinLeft,    chk_ArrHost, ConEmuHotKey::UseWinArrows, L"", CConEmuCtrl::key_WinWidthDec)  // Decrease window width
		->SetVkMod(VK_LEFT|CEHOTKEY_ARRHOSTKEY);
	Add(vkWinRight,   chk_ArrHost, ConEmuHotKey::UseWinArrows, L"", CConEmuCtrl::key_WinWidthInc)  // Increase window width
		->SetVkMod(VK_RIGHT|CEHOTKEY_ARRHOSTKEY);
	Add(vkWinUp,      chk_ArrHost, ConEmuHotKey::UseWinArrows, L"", CConEmuCtrl::key_WinHeightDec) // Decrease window height
		->SetVkMod(VK_UP|CEHOTKEY_ARRHOSTKEY);
	Add(vkWinDown,    chk_ArrHost, ConEmuHotKey::UseWinArrows, L"", CConEmuCtrl::key_WinHeightInc) // Increase window height
		->SetVkMod(VK_DOWN|CEHOTKEY_ARRHOSTKEY);
	/*
		*** [HostKey]+Number. Only [HostKey] (modifiers actually) can be configured
			Console activate by number
	*/
	Add(vkConsole_1,  chk_NumHost, ConEmuHotKey::UseWinNumber, L"", CConEmuCtrl::key_ConsoleNum)
		->SetVkMod('1'|CEHOTKEY_NUMHOSTKEY);
	Add(vkConsole_2,  chk_NumHost, ConEmuHotKey::UseWinNumber, L"", CConEmuCtrl::key_ConsoleNum)
		->SetVkMod('2'|CEHOTKEY_NUMHOSTKEY);
	Add(vkConsole_3,  chk_NumHost, ConEmuHotKey::UseWinNumber, L"", CConEmuCtrl::key_ConsoleNum)
		->SetVkMod('3'|CEHOTKEY_NUMHOSTKEY);
	Add(vkConsole_4,  chk_NumHost, ConEmuHotKey::UseWinNumber, L"", CConEmuCtrl::key_ConsoleNum)
		->SetVkMod('4'|CEHOTKEY_NUMHOSTKEY);
	Add(vkConsole_5,  chk_NumHost, ConEmuHotKey::UseWinNumber, L"", CConEmuCtrl::key_ConsoleNum)
		->SetVkMod('5'|CEHOTKEY_NUMHOSTKEY);
	Add(vkConsole_6,  chk_NumHost, ConEmuHotKey::UseWinNumber, L"", CConEmuCtrl::key_ConsoleNum)
		->SetVkMod('6'|CEHOTKEY_NUMHOSTKEY);
	Add(vkConsole_7,  chk_NumHost, ConEmuHotKey::UseWinNumber, L"", CConEmuCtrl::key_ConsoleNum)
		->SetVkMod('7'|CEHOTKEY_NUMHOSTKEY);
	Add(vkConsole_8,  chk_NumHost, ConEmuHotKey::UseWinNumber, L"", CConEmuCtrl::key_ConsoleNum)
		->SetVkMod('8'|CEHOTKEY_NUMHOSTKEY);
	Add(vkConsole_9,  chk_NumHost, ConEmuHotKey::UseWinNumber, L"", CConEmuCtrl::key_ConsoleNum)
		->SetVkMod('9'|CEHOTKEY_NUMHOSTKEY);
	Add(vkConsole_10, chk_NumHost, ConEmuHotKey::UseWinNumber, L"", CConEmuCtrl::key_ConsoleNum)
		->SetVkMod('0'|CEHOTKEY_NUMHOSTKEY);

	// Reapply actual cvk_XXX flags to Win+Arrows
	UpdateNumberModifier();
	// and Win+Numbers
	UpdateArrowModifier();

	// Чтобы не возникло проблем с инициализацией хуков (для обработки Win+<key>)
	int nHotKeyCount = this->size();
	// these constant is used to allocate array of keys in keyhook proc
	// Settings -> Controls -> Install keyboard hooks
	_ASSERTE(nHotKeyCount<(HookedKeysMaxCount-1));

	// Informational
	return nHotKeyCount;
}
