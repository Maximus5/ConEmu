
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

#define HIDE_USE_EXCEPTION_INFO

#include "Header.h"
#include "Hotkeys.h"
#include "ConEmu.h"
#include "LngRc.h"
#include "Options.h"
#include "OptionsClass.h"
#include "SetCmdTask.h"
#include "VirtualConsole.h"

// Некоторые комбинации нужно обрабатывать "на отпускание" во избежание глюков с интерфейсом
const struct ConEmuHotKey* ConEmuSkipHotKey = ((ConEmuHotKey*)INVALID_HANDLE_VALUE);

extern CEVkMatch gvkMatchList[];

// That is what is stored in the settings
DWORD ConEmuHotKey::GetVkMod() const
{
	return Key.GetVkMod(HkType);
}

ConEmuHotKey& ConEmuHotKey::SetVkMod(DWORD VkMod)
{
	Key.SetVkMod(HkType, VkMod);
	return *this;
}

bool ConEmuHotKey::CanChangeVK() const
{
	//chk_System - пока не настраивается
	return (HkType==chk_User || HkType==chk_Global || HkType==chk_Local || HkType==chk_Macro || HkType==chk_Task);
}

bool ConEmuHotKey::IsTaskHotKey() const
{
	return (HkType==chk_Task && DescrLangID<0);
}

// 0-based
int ConEmuHotKey::GetTaskIndex() const
{
	if (IsTaskHotKey())
		return -(DescrLangID+1);
	return -1;
}

// 0-based
void ConEmuHotKey::SetTaskIndex(int iTaskIdx)
{
	if (iTaskIdx >= 0)
	{
		DescrLangID = -(iTaskIdx+1);
	}
	else
	{
		_ASSERTE(iTaskIdx>=0);
		DescrLangID = 0;
	}
}

LPCWSTR ConEmuHotKey::GetDescription(wchar_t* pszDescr, int cchMaxLen, bool bAddMacroIndex /*= false*/) const
{
	if (!pszDescr)
		return L"";

	_ASSERTE(cchMaxLen>200);

	LPCWSTR pszRc = pszDescr;
	bool lbColon = false;

	*pszDescr = 0;

	if (this->Enabled)
	{
		if (this->Enabled == InSelection)
		{
			lstrcpyn(pszDescr, L"[InSelection] ", cchMaxLen);
		}
		else if (!this->Enabled())
		{
			lstrcpyn(pszDescr, L"[Disabled] ", cchMaxLen);
		}

		if (*pszDescr)
		{
			int nLen = lstrlen(pszDescr);
			pszDescr += nLen;
			cchMaxLen -= nLen;
		}
	}

	if (bAddMacroIndex && (HkType == chk_Macro))
	{
		swprintf_c(pszDescr, cchMaxLen/*#SECURELEN*/, L"Macro %02i: ", DescrLangID-vkGuiMacro01+1);
		int nLen = lstrlen(pszDescr);
		pszDescr += nLen;
		cchMaxLen -= nLen;
		lbColon = true;
	}

	if (IsTaskHotKey())
	{
		const CommandTasks* pCmd = gpSet->CmdTaskGet(GetTaskIndex());
		if (pCmd)
			lstrcpyn(pszDescr, pCmd->pszName ? pCmd->pszName : L"", cchMaxLen);
	}
	else if ((HkType != chk_Macro) && !CLngRc::getHint(DescrLangID, pszDescr, cchMaxLen))
	{
		if ((HkType == chk_User) && GuiMacro && *GuiMacro)
			lstrcpyn(pszDescr, GuiMacro, cchMaxLen);
		else
			swprintf_c(pszDescr, cchMaxLen/*#SECURELEN*/, L"#%i", DescrLangID);
	}
	else if ((cchMaxLen >= 16) && GuiMacro && *GuiMacro)
	{
		size_t nLen = _tcslen(pszDescr);
		pszDescr += nLen;
		cchMaxLen -= nLen;

		if (!lbColon && (cchMaxLen > 2) && (pszDescr > pszRc))
		{
			lstrcpyn(pszDescr, L": ", cchMaxLen);
			pszDescr += 2;
			cchMaxLen -= 2;
		}
		lstrcpyn(pszDescr, GuiMacro, cchMaxLen);
	}

	if ((DescrLangID == vkMultiCmd)
		&& wcsstr(pszDescr, L"%s"))
	{
		RConStartArgsEx args; CEStr lsTitle;
		if (int shell_rc = gpSet->CmdTaskGetDefaultShell(args, lsTitle))
		{
			CEStr lsFormat;
			INT_PTR cchBufMax = wcslen(pszDescr) + wcslen(lsTitle) + 3;
			if (lsFormat.GetBuffer(cchBufMax))
			{
				msprintf(lsFormat.ms_Val, cchBufMax, pszDescr, lsTitle.c_str());
				lstrcpyn(pszDescr, lsFormat, cchMaxLen);
			}
		}
	}

	return pszRc;
}

void ConEmuHotKey::Free()
{
	SafeFree(GuiMacro);
}

ConEmuHotKey& ConEmuHotKey::SetHotKey(const BYTE vk, const BYTE vkMod1/*=0*/, const BYTE vkMod2/*=0*/, const BYTE vkMod3/*=0*/)
{
	Key.SetHotKey(HkType, vk, vkMod1, vkMod2, vkMod3);
	return*this;
}

ConEmuHotKey& ConEmuHotKey::SetEnabled(const HotkeyEnabled_t enabledFunc)
{
	Enabled = enabledFunc;
	return *this;
}

ConEmuHotKey& ConEmuHotKey::SetOnKeyUp()
{
	OnKeyUp = true;
	return *this;
}

ConEmuHotKey& ConEmuHotKey::SetMacro(const wchar_t* guiMacro)
{
	if (GuiMacro && GuiMacro != guiMacro)
		free(GuiMacro);
	GuiMacro = guiMacro ? lstrdup(guiMacro) : nullptr;
	_ASSERTE(fkey == nullptr || fkey == CConEmuCtrl::key_GuiMacro);
	fkey = CConEmuCtrl::key_GuiMacro;
	return *this;
}

bool ConEmuHotKey::Equal(const BYTE vk, const BYTE vkMod1/*=0*/, const BYTE vkMod2/*=0*/, const BYTE vkMod3/*=0*/) const
{
	//TODO
	const DWORD vkMod = GetVkMod();
	const DWORD vkModCmp = ConEmuChord::MakeHotKey(vk, vkMod1, vkMod2, vkMod3);
	return (vkMod == vkModCmp);
}

// Return user-friendly key name
LPCWSTR ConEmuHotKey::GetHotkeyName(wchar_t(&szFull)[128], bool bShowNone /*= true*/) const
{
	_ASSERTE(this && this != ConEmuSkipHotKey);
	return ConEmuChord::GetHotkeyName(szFull, Key.GetVkMod(HkType), HkType, bShowNone);
}

LPCWSTR ConEmuHotKey::CreateNotUniqueWarning(LPCWSTR asHotkey, LPCWSTR asDescr1, LPCWSTR asDescr2, CEStr& rsWarning)
{
	CEStr lsFmt(CLngRc::getRsrc(lng_HotkeyDuplicated/*"Hotkey <%s> is not unique"*/));
	wchar_t* ptrPoint = lsFmt.ms_Val ? (wchar_t*)wcsstr(lsFmt.ms_Val, L"%s") : nullptr;
	if (!ptrPoint)
	{
		rsWarning.Attach(lstrmerge(L"Hotkey <", asHotkey, L"> is not unique",
			L"\n", asDescr1, L"\n", asDescr2));
	}
	else
	{
		_ASSERTE(ptrPoint[0] == L'%' && ptrPoint[1] == L's' && ptrPoint[2]);
		*ptrPoint = 0;
		rsWarning.Attach(lstrmerge(lsFmt.ms_Val, asHotkey, ptrPoint+2,
			L"\n", asDescr1, L"\n", asDescr2));
	}
	return rsWarning.ms_Val;
}

// Some statics - context checks
bool ConEmuHotKey::UseWinNumber()
{
	return gpSetCls->IsMulti() && gpSet->isUseWinNumber;
}

bool ConEmuHotKey::UseWinArrows()
{
	return gpSet->isUseWinArrows;
}

bool ConEmuHotKey::UseWinMove()
{
	if (gpConEmu->isInside())
		return false;
	if (gpConEmu->isCaptionHidden())
		return true;
	return false;
}

bool ConEmuHotKey::InSelection()
{
	CVConGuard VCon;
	if (gpConEmu->GetActiveVCon(&VCon) < 0)
		return false;
	if (!VCon->RCon()->isSelectionPresent())
		return false;
	return true;
}

bool ConEmuHotKey::UseCTSShiftArrow()
{
	CVConGuard VCon;
	if (gpConEmu->GetActiveVCon(&VCon) < 0)
		return false;

	CRealConsole* pRCon = VCon->RCon();
	if (!pRCon || pRCon->isFar() || pRCon->isSelectionPresent())
		return false;

	const AppSettings* pApp = gpSet->GetAppSettings(pRCon->GetActiveAppSettingsId());
	if (!pApp)
		return false;

	return pApp->CTSShiftArrowStart();
}

bool ConEmuHotKey::UseCtrlTab()
{
	return gpSet->isTabSelf;
}

bool ConEmuHotKey::UseCtrlBS()
{
	return (gpSet->GetAppSettings()->CTSDeleteLeftWord() != 0);
}

bool ConEmuHotKey::UseDndLKey()
{
	return ((gpSet->isDragEnabled & DRAG_L_ALLOWED) == DRAG_L_ALLOWED);
}

bool ConEmuHotKey::UseDndRKey()
{
	return ((gpSet->isDragEnabled & DRAG_R_ALLOWED) == DRAG_R_ALLOWED);
}

bool ConEmuHotKey::UseWndDragKey()
{
	return gpSet->isMouseDragWindow;
}

bool ConEmuHotKey::UsePromptFind()
{
	CVConGuard VCon;
	if (gpConEmu->GetActiveVCon(&VCon) < 0 || !VCon->RCon())
		return false;
	CRealConsole* pRCon = VCon->RCon();
	if (!pRCon->isFar())
		return true;
	// Even if Far we may search for the prompt, if we are in panels and they are OFF
	if (!pRCon->isFarPanelAllowed())
		return false;
	DWORD dwFlags = pRCon->GetActiveDlgFlags();
	if (((dwFlags & FR_FREEDLG_MASK) != 0)
		|| pRCon->isEditor() || pRCon->isViewer()
		|| pRCon->isFilePanel(true, true, true))
		return true;
	return false;
}
