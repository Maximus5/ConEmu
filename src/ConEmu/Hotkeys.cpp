
/*
Copyright (c) 2013-2014 Maximus5
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

#include "Header.h"
#include "Hotkeys.h"
#include "ConEmu.h"
#include "ConEmuCtrl.h"
#include "Options.h"
#include "VirtualConsole.h"
#include "../ConEmuCD/GuiHooks.h"

/* *********** Hotkey editor dialog *********** */
bool CHotKeyDialog::EditHotKey(HWND hParent, DWORD& VkMod)
{
	CHotKeyDialog Dlg(hParent, VkMod);
	// Modal dialog (CreateDialog)
	INT_PTR iRc = DialogBoxParam(g_hInstance, MAKEINTRESOURCE(IDD_HOTKEY), hParent, hkDlgProc, (LPARAM)&Dlg);
	bool bOk = (iRc == IDOK);
	if (bOk)
	{
		VkMod = Dlg.GetVkMod();
	}
	return bOk;
}

CHotKeyDialog::CHotKeyDialog(HWND hParent, DWORD aVkMod)
{
	mh_Dlg = NULL;
	mh_Parent = hParent;

	ZeroStruct(m_HK);
	m_HK.HkType = chk_User;
	m_HK.VkMod = aVkMod;

	mp_DpiAware = new CDpiForDialog();
}

CHotKeyDialog::~CHotKeyDialog()
{
	SafeDelete(mp_DpiAware);
}

DWORD CHotKeyDialog::GetVkMod()
{
	return m_HK.VkMod;
}

DWORD CHotKeyDialog::dlgGetHotkey(HWND hDlg, UINT iEditCtrl /*= hkHotKeySelect*/, UINT iListCtrl /*= lbHotKeyList*/)
{
	DWORD nHotKey = 0xFF & SendDlgItemMessage(hDlg, iEditCtrl, HKM_GETHOTKEY, 0, 0);

	bool bList = false;
	CSettings::ListBoxItem* pItems = NULL;
	uint nKeyCount = CSettings::GetHotKeyListItems(CSettings::eHkKeysHot, &pItems);
	for (size_t i = 0; i < nKeyCount; i++)
	{
		if (pItems[i].nValue == nHotKey)
		{
			SendDlgItemMessage(hDlg, iListCtrl, CB_SETCURSEL, i, 0);
			bList = true;
			break;
		}
	}

	if (!bList)
		SendDlgItemMessage(hDlg, iListCtrl, CB_SETCURSEL, 0, 0);

	return nHotKey;
}

INT_PTR CHotKeyDialog::hkDlgProc(HWND hDlg, UINT messg, WPARAM wParam, LPARAM lParam)
{
	CHotKeyDialog* pDlg = NULL;
	if (messg == WM_INITDIALOG)
	{
		pDlg = (CHotKeyDialog*)lParam;
		pDlg->mh_Dlg = hDlg;
		SetWindowLongPtr(hDlg, DWLP_USER, lParam);
	}
	else
	{
		pDlg = (CHotKeyDialog*)GetWindowLongPtr(hDlg, DWLP_USER);
	}
	if (!pDlg)
	{
		return FALSE;
	}

	switch (messg)
	{
		case WM_INITDIALOG:
		{
			SendMessage(hDlg, WM_SETICON, ICON_BIG, (LPARAM)hClassIcon);
			SendMessage(hDlg, WM_SETICON, ICON_SMALL, (LPARAM)hClassIconSm);

			if (pDlg->mp_DpiAware)
			{
				pDlg->mp_DpiAware->Attach(hDlg, ghWnd);
			}

			// Ensure, it will be "on screen"
			RECT rect; GetWindowRect(hDlg, &rect);
			RECT rcCenter = CenterInParent(rect, pDlg->mh_Parent);
			MoveWindow(hDlg, rcCenter.left, rcCenter.top,
			           rect.right - rect.left, rect.bottom - rect.top, false);

			HWND hHk = GetDlgItem(hDlg, hkHotKeySelect);
			SendMessage(hHk, HKM_SETRULES, HKCOMB_A|HKCOMB_C|HKCOMB_CA|HKCOMB_S|HKCOMB_SA|HKCOMB_SC|HKCOMB_SCA, 0);

			BYTE vk = ConEmuHotKey::GetHotkey(pDlg->m_HK.VkMod);
			CSettings::SetHotkeyField(hHk, vk);

			// Warning! Если nVK не указан в SettingsNS::nKeysHot - nVK будет обнулен
			CSettings::FillListBoxHotKeys(GetDlgItem(hDlg, lbHotKeyList), CSettings::eHkKeysHot, vk);

			for (int n = 0; n < 3; n++)
			{
				BYTE b = pDlg->m_HK.VkMod ? ConEmuHotKey::GetModifier(pDlg->m_HK.VkMod,n+1) : 0;
				CSettings::FillListBoxHotKeys(GetDlgItem(hDlg, lbHotKeyMod1+n), CSettings::eHkModifiers, b);
			}

			SetFocus(GetDlgItem(hDlg,hkHotKeySelect));

			return FALSE;
		} // WM_INITDIALOG

		case WM_COMMAND:
		{
			switch (HIWORD(wParam))
			{
				case BN_CLICKED:
				{
					switch (LOWORD(wParam))
					{
					case IDOK:
					case IDCANCEL:
					case IDCLOSE:
						EndDialog(hDlg, LOWORD(wParam));
						return 1;
					}
					break;
				} // BN_CLICKED

				case EN_CHANGE:
				{
					UINT nHotKey = dlgGetHotkey(hDlg, hkHotKeySelect, lbHotKeyList);

					DWORD nCurMods = (CEHOTKEY_MODMASK & pDlg->m_HK.VkMod);
					if (!nCurMods)
						nCurMods = CEHOTKEY_NOMOD;

					pDlg->m_HK.VkMod = nHotKey | nCurMods;

					break;
				} // EN_CHANGE

				case CBN_SELCHANGE:
				{
					switch (LOWORD(wParam))
					{
						case lbHotKeyList:
						{
							BYTE vk = 0;
							CSettings::GetListBoxHotKey(GetDlgItem(hDlg, lbHotKeyList), CSettings::eHkKeysHot, vk);

							CSettings::SetHotkeyField(GetDlgItem(hDlg, hkHotKeySelect), vk);

							DWORD nMod = (CEHOTKEY_MODMASK & pDlg->m_HK.VkMod);
							if (nMod == 0)
							{
								// Если модификатора вообще не было - ставим Win
								BYTE b = VK_LWIN;
								CSettings::FillListBoxHotKeys(GetDlgItem(hDlg, lbHotKeyMod1), CSettings::eHkModifiers, b);
								nMod = (VK_LWIN << 8);
							}
							pDlg->m_HK.VkMod = ((DWORD)vk) | nMod;

							break;
						} // lbHotKeyList

						case lbHotKeyMod1:
						case lbHotKeyMod2:
						case lbHotKeyMod3:
						{
							DWORD nModifers = 0;

							for (UINT i = 0; i < 3; i++)
							{
								BYTE vk = 0;
								CSettings::GetListBoxHotKey(GetDlgItem(hDlg,lbHotKeyMod1+i),CSettings::eHkModifiers,vk);
								if (vk)
									nModifers = ConEmuHotKey::SetModifier(nModifers, vk, false);
							}

							_ASSERTE((nModifers & 0xFF) == 0); // Модификаторы должны быть строго в старших 3-х байтах

							if (!nModifers)
								nModifers = CEHOTKEY_NOMOD;

							pDlg->m_HK.VkMod = (cvk_VK_MASK & pDlg->m_HK.VkMod) | nModifers;

							break;
						} // lbHotKeyMod1, lbHotKeyMod2, lbHotKeyMod3

					} // switch (LOWORD(wParam))

					break;
				} // CBN_SELCHANGE

			} // switch (HIWORD(wParam))

			break;
		} // WM_COMMAND

		default:
		if (pDlg->mp_DpiAware && pDlg->mp_DpiAware->ProcessDpiMessages(hDlg, messg, wParam, lParam))
		{
			return TRUE;
		}
	}

	return FALSE;
}








/* *********** Hotkey list processing *********** */


// Некоторые комбинации нужно обрабатывать "на отпускание" во избежание глюков с интерфейсом
const struct ConEmuHotKey* ConEmuSkipHotKey = ((ConEmuHotKey*)INVALID_HANDLE_VALUE);

static CEVkMatch gvkMatchList[] = {
	{VK_CONTROL,  false, cvk_Ctrl,  (cvk_LCtrl|cvk_RCtrl)},
	{VK_LCONTROL, true,  cvk_LCtrl},
	{VK_RCONTROL, true,  cvk_RCtrl},
	{VK_MENU,     false, cvk_Alt,   (cvk_LAlt|cvk_RAlt)},
	{VK_LMENU,    true,  cvk_LAlt},
	{VK_RMENU,    true,  cvk_RAlt},
	{VK_SHIFT,    false, cvk_Shift, (cvk_LShift|cvk_RShift)},
	{VK_LSHIFT,   true,  cvk_LShift},
	{VK_RSHIFT,   true,  cvk_RShift},
	{VK_LWIN,     true,  cvk_Win},
	{VK_APPS,     true,  cvk_Apps},
};

void CEVkMatch::Set(BYTE aVk, bool aDistinct, ConEmuModifiers aMod, ConEmuModifiers aUnmask /*= cvk_NULL*/)
{
	Vk = aVk; Distinct = aDistinct; Mod = aMod; Unmask = aUnmask;
}

// Internal conversions between VK_xxx and cvk_xxx
bool ConEmuHotKey::GetMatchByVk(BYTE Vk, CEVkMatch& Match)
{
	switch (Vk)
	{
	case VK_LCONTROL: Match.Set(Vk, true,  cvk_LCtrl);  break;
	case VK_RCONTROL: Match.Set(Vk, true,  cvk_RCtrl);  break;
	case VK_LMENU:    Match.Set(Vk, true,  cvk_LAlt);   break;
	case VK_RMENU:    Match.Set(Vk, true,  cvk_RAlt);   break;
	case VK_LSHIFT:   Match.Set(Vk, true,  cvk_LShift); break;
	case VK_RSHIFT:   Match.Set(Vk, true,  cvk_RShift); break;
	case VK_LWIN:     Match.Set(Vk, true,  cvk_Win);    break;
	case VK_APPS:     Match.Set(Vk, true,  cvk_Apps);   break;
	case VK_CONTROL:  Match.Set(Vk, false, cvk_Ctrl,  (cvk_LCtrl|cvk_RCtrl));   break;
	case VK_MENU:     Match.Set(Vk, false, cvk_Alt,   (cvk_LAlt|cvk_RAlt));     break;
	case VK_SHIFT:    Match.Set(Vk, false, cvk_Shift, (cvk_LShift|cvk_RShift)); break;
	default:
		return false;
	}
	return true;
#if 0
	for (size_t i = 0; i < countof(gvkMatchList); i++)
	{
		if (Vk == gvkMatchList[i].Vk)
		{
			Match = gvkMatchList[i];
			return true;
		}
	}
	return false;
#endif
}
//// Internal conversions between VK_xxx and cvk_xxx
//bool ConEmuHotKey::GetMatchByMod(ConEmuModifiers Mod, CEVkMatch& Match)
//{
//}


void ConEmuChord::Clear()
{
	Vk = 0;
	Mod = cvk_NULL;
}

UINT ConEmuChord::GetModifiers(BYTE (&Mods)[3]) const
{
	UINT m = 0;
	for (size_t i = 0; i < countof(gvkMatchList); i++)
	{
		if (Mod & gvkMatchList[i].Mod)
		{
			Mods[m] = gvkMatchList[i].Vk;
			m++;
			if (m >= countof(Mods))
				break;
		}
	}
	for (size_t k = m; k < countof(Mods); k++)
	{
		Mods[k] = 0;
	}
	return m;
}

bool ConEmuChord::IsEmpty() const
{
	return (Vk == 0);
}

bool ConEmuChord::IsEqual(const ConEmuChord& Key) const
{
	return ((Vk == Key.Vk) && (Mod == Key.Mod));
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
		_wsprintf(pszDescr, SKIPLEN(cchMaxLen) L"Macro %02i: ", DescrLangID-vkGuiMacro01+1);
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
	else if ((HkType != chk_Macro) && !LoadString(g_hInstance, DescrLangID, pszDescr, cchMaxLen))
	{
		if ((HkType == chk_User) && GuiMacro && *GuiMacro)
			lstrcpyn(pszDescr, GuiMacro, cchMaxLen);
		else
			_wsprintf(pszDescr, SKIPLEN(cchMaxLen) L"#%i", DescrLangID);
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

	return pszRc;
}

void ConEmuHotKey::Free()
{
	SafeFree(GuiMacro);
}

// nHostMod в младших 3-х байтах может содержать VK (модификаторы).
// Функция проверяет, чтобы они не дублировались
void ConEmuHotKey::TestHostkeyModifiers(DWORD& nHostMod)
{
	//memset(mn_HostModOk, 0, sizeof(mn_HostModOk));
	//memset(mn_HostModSkip, 0, sizeof(mn_HostModSkip));

	if (!nHostMod)
	{
		nHostMod = VK_LWIN;
	}
	else
	{
		BYTE vk, vkList[3] = {};
		int i = 0;
		DWORD nTest = nHostMod;
		while (nTest && (i < 3))
		{
			vk = (nTest & 0xFF);
			nTest = nTest >> 8;

			switch (vk)
			{
			case 0:
				break;
			case VK_LWIN: case VK_RWIN:
				if (vkList[0]!=VK_LWIN && vkList[1]!=VK_LWIN && vkList[2]!=VK_LWIN)
					vkList[i++] = vk;
				break;
			case VK_APPS:
				if (vkList[0]!=VK_APPS && vkList[1]!=VK_APPS && vkList[2]!=VK_APPS)
					vkList[i++] = vk;
				break;
			case VK_LCONTROL:
			case VK_RCONTROL:
			case VK_CONTROL:
				for (int k = 0; k < 3; k++)
				{
					if (vkList[k]==VK_LCONTROL || vkList[k]==VK_RCONTROL || vkList[k]==VK_CONTROL)
					{
						vkList[k] = VK_CONTROL;
						vk = 0;
						break;
					}
				}
				if (vk)
					vkList[i++] = vk;
				break;
			case VK_LMENU:
			case VK_RMENU:
			case VK_MENU:
				for (int k = 0; k < 3; k++)
				{
					if (vkList[k]==VK_LMENU || vkList[k]==VK_RMENU || vkList[k]==VK_MENU)
					{
						vkList[k] = VK_MENU;
						vk = 0;
						break;
					}
				}
				if (vk)
					vkList[i++] = vk;
				break;
			case VK_LSHIFT:
			case VK_RSHIFT:
			case VK_SHIFT:
				for (int k = 0; k < 3; k++)
				{
					if (vkList[k]==VK_LSHIFT || vkList[k]==VK_RSHIFT || vkList[k]==VK_SHIFT)
					{
						vkList[k] = VK_SHIFT;
						vk = 0;
						break;
					}
				}
				if (vk)
					vkList[i++] = vk;
				break;
			}
		}

		nHostMod = (((DWORD)vkList[0]))
			| (((DWORD)vkList[1]) << 8)
			| (((DWORD)vkList[2]) << 16);
	}
}

// набор флагов MOD_xxx для RegisterHotKey
DWORD ConEmuHotKey::GetHotKeyMod(DWORD VkMod)
{
	DWORD nMOD = 0;

	for (int i = 1; i <= 3; i++)
	{
		switch (GetModifier(VkMod,i))
		{
			case VK_LWIN: case VK_RWIN:
				nMOD |= MOD_WIN;
				break;
			case VK_CONTROL: case VK_LCONTROL: case VK_RCONTROL:
				nMOD |= MOD_CONTROL;
				break;
			case VK_SHIFT: case VK_LSHIFT: case VK_RSHIFT:
				nMOD |= MOD_SHIFT;
				break;
			case VK_MENU: case VK_LMENU: case VK_RMENU:
				nMOD |= MOD_ALT;
				break;
		}
	}

#if 0
	// User want - user get
	if (!nMOD)
	{
		_ASSERTE(nMOD!=0);
		nMOD = MOD_WIN;
	}
#endif

	return nMOD;
}

// Сервисная функция для инициализации. Формирует готовый VkMod
DWORD ConEmuHotKey::MakeHotKey(BYTE Vk, BYTE vkMod1/*=0*/, BYTE vkMod2/*=0*/, BYTE vkMod3/*=0*/)
{
	DWORD vkHotKey = Vk;
	if (!vkMod1 && !vkMod2 && !vkMod3)
	{
		vkHotKey |= CEHOTKEY_NOMOD;
	}
	else
	{
		int iShift = 8;
		if (vkMod1)
		{
			vkHotKey |= (vkMod1<<iShift);
			iShift += 8;
		}
		if (vkMod2)
		{
			vkHotKey |= (vkMod2<<iShift);
			iShift += 8;
		}
		if (vkMod3)
		{
			vkHotKey |= (vkMod3<<iShift);
			iShift += 8;
		}
	}
	return vkHotKey;
}

// Задать или сбросить модификатор в VkMod
DWORD ConEmuHotKey::SetModifier(DWORD VkMod, BYTE Mod, bool Xor/*=true*/)
{
	DWORD AllMod = VkMod & CEHOTKEY_MODMASK;
	if ((VkMod == CEHOTKEY_NUMHOSTKEY) || (VkMod == CEHOTKEY_ARRHOSTKEY))
	{
		// Низя
		_ASSERTE(!((VkMod == CEHOTKEY_NUMHOSTKEY) || (VkMod == CEHOTKEY_ARRHOSTKEY)));
		return VkMod;
	}

	if (AllMod == CEHOTKEY_NOMOD)
		AllMod = 0;

	bool Processed = false;

	// Младший байт - VK. Старшие три - модификаторы. Их и трогаем
	for (int i = 1; i <= 3; i++)
	{
		DWORD vkExist = GetModifier(VkMod, i);
		if (isKey(vkExist,Mod) || isKey(Mod,vkExist))
		{
			Processed = true;

			if (Xor)
			{
				switch (i)
				{
				case 1:
					AllMod = (GetModifier(VkMod, 2)<<8) | (GetModifier(VkMod, 3)<<16);
					break;
				case 2:
					AllMod = (GetModifier(VkMod, 1)<<8) | (GetModifier(VkMod, 3)<<16);
					break;
				case 3:
					AllMod = (GetModifier(VkMod, 1)<<8) | (GetModifier(VkMod, 2)<<16);
					break;
				}
			}
			else if (vkExist != Mod)
			{
				// Например, заменить LShift на Shift
				switch (i)
				{
				case 1:
					AllMod = (VkMod & 0xFFFF0000) | (Mod << 8);
					break;
				case 2:
					AllMod = (VkMod & 0xFF00FF00) | (Mod << 16);
					break;
				case 3:
					AllMod = (VkMod & 0x00FFFF00) | (Mod << 24);
					break;
				}
			}

			break;
		}
	}
	//if (GetModifier(VkMod, 1) == Mod)
	//{
	//	AllMod = (GetModifier(VkMod, 2)<<8) | (GetModifier(VkMod, 3)<<16);
	//	Processed = true;
	//}
	//else if (GetModifier(VkMod, 2) == Mod)
	//{
	//	AllMod = (GetModifier(VkMod, 1)<<8) | (GetModifier(VkMod, 3)<<16);
	//	Processed = true;
	//}
	//else if (GetModifier(VkMod, 3) == Mod)
	//{
	//	AllMod = (GetModifier(VkMod, 1)<<8) | (GetModifier(VkMod, 2)<<16);
	//	Processed = true;
	//}

	if (!Processed)
	{
		DWORD AddMod = 0;

		if (!GetModifier(VkMod, 1))
			AddMod = (((DWORD)Mod) << 8);
		else if (!GetModifier(VkMod, 2))
			AddMod = (((DWORD)Mod) << 16);
		else if (!GetModifier(VkMod, 3))
			AddMod = (((DWORD)Mod) << 24);
		else
		{
			// Иначе - некуда модификатор пихать, а так уже три
			_ASSERTE(GetModifier(VkMod, 3) == 0);
		}

		if (AddMod != 0)
			AllMod |= AddMod;
	}

	// Нельзя сбрасывать единственный модификатор
	if (!AllMod)
	{
		_ASSERTE(AllMod!=0);
	}
	else
	{
		VkMod = GetHotkey(VkMod) | AllMod;
	}

	return VkMod;
}

// // Вернуть назначенные модификаторы (idx = 1..3). Возвращает 0 (нету) или VK
DWORD ConEmuHotKey::GetModifier(DWORD VkMod, int idx)
{
	DWORD Mod = VkMod & CEHOTKEY_MODMASK;

	if ((Mod == CEHOTKEY_NOMOD) || (Mod == 0))
	{
		_ASSERTE(((VkMod & CEHOTKEY_MODMASK) != 0) || (VkMod == 0));
		return 0;
	}
	else if (Mod == CEHOTKEY_NUMHOSTKEY)
	{
		// Только для цифирок!
		WARNING("CConEmuCtrl:: Убрать пережиток F11/F12");
		_ASSERTE((((VkMod & 0xFF)>='0' && ((VkMod & 0xFF)<='9'))) /*((VkMod & 0xFF)==VK_F11 || (VkMod & 0xFF)==VK_F12)*/);
		Mod = (gpSet->HostkeyNumberModifier() << 8);
	}
	else if (Mod == CEHOTKEY_ARRHOSTKEY)
	{
		// Только для стрелок!
		_ASSERTE(((VkMod & 0xFF)==VK_LEFT) || ((VkMod & 0xFF)==VK_RIGHT) || ((VkMod & 0xFF)==VK_UP) || ((VkMod & 0xFF)==VK_DOWN));
		Mod = (gpSet->HostkeyArrowModifier() << 8);
	}

	switch (idx)
	{
	case 1:
		Mod = ((Mod & 0xFF00) >> 8);
		break;
	case 2:
		Mod = ((Mod & 0xFF0000) >> 16);
		break;
	case 3:
		Mod = ((Mod & 0xFF000000) >> 24);
		break;
	default:
		_ASSERTE(idx==1 || idx==2 || idx==3);
		Mod = 0;
	}

	if (Mod == VK_RWIN)
	{
		_ASSERTE(Mod != VK_RWIN); // Храниться должен LWIN
		Mod = VK_LWIN;
	}

	return Mod;
}

// Есть ли в этом (VkMod) хоткее - модификатор Mod (VK)
bool ConEmuHotKey::HasModifier(DWORD VkMod, BYTE Mod/*VK*/)
{
	if (Mod && ((GetModifier(VkMod, 1) == Mod) || (GetModifier(VkMod, 2) == Mod) || (GetModifier(VkMod, 3) == Mod)))
		return true;
	return false;
}

// Вернуть имя модификатора (типа "Apps+Space")
LPCWSTR ConEmuHotKey::GetHotkeyName(wchar_t (&szFull)[128], bool bShowNone /*= true*/) const
{
	_ASSERTE(this && this!=ConEmuSkipHotKey);

	wchar_t szName[32];
	szFull[0] = 0;

	DWORD lVkMod = 0;

	switch (HkType)
	{
	case chk_Global:
	case chk_Local:
	case chk_User:
	case chk_Task:
	case chk_Modifier2:
		lVkMod = VkMod;
		break;
	case chk_Macro:
		lVkMod = VkMod;
		break;
	case chk_Modifier:
		lVkMod = VkMod;
		break;
	case chk_NumHost:
		_ASSERTE((VkMod & CEHOTKEY_MODMASK) == CEHOTKEY_NUMHOSTKEY);
		lVkMod = (VkMod & 0xFF) | (gpSet->HostkeyNumberModifier() << 8);
		break;
	case chk_ArrHost:
		_ASSERTE((VkMod & CEHOTKEY_MODMASK) == CEHOTKEY_ARRHOSTKEY);
		lVkMod = (VkMod & 0xFF) | (gpSet->HostkeyArrowModifier() << 8);
		break;
	case chk_System:
		lVkMod = VkMod;
		break;
	default:
		// Неизвестный тип!
		_ASSERTE(FALSE && "Unknown HkType");
		lVkMod = 0;
	}

	if (GetHotkey(lVkMod) == 0)
	{
		szFull[0] = 0; // Поле "Кнопка" оставляем пустым
	}
	else if (HkType != chk_Modifier)
	{
		for (int k = 1; k <= 3; k++)
		{
			DWORD vk = (HkType == chk_Modifier) ? lVkMod : GetModifier(lVkMod, k);
			if (vk)
			{
				GetVkKeyName(vk, szName);
				if (szFull[0])
					wcscat_c(szFull, L"+");
				wcscat_c(szFull, szName);
			}
		}
	}

	if (HkType != chk_Modifier2)
	{
		szName[0] = 0;
		GetVkKeyName(GetHotkey(lVkMod), szName);

		if (szName[0])
		{
			if (szFull[0])
				wcscat_c(szFull, L"+");
			wcscat_c(szFull, szName);
		}
		else if (bShowNone)
		{
			wcscpy_c(szFull, L"<None>");
		}
		else
		{
			szFull[0] = 0;
		}
	}

	return szFull;
}

// Return user-friendly key name
LPCWSTR ConEmuHotKey::GetHotkeyName(DWORD aVkMod, wchar_t (&szFull)[128], bool bShowNone /*= true*/)
{
	ConEmuHotKey hk = {0, chk_User, NULL, L"", aVkMod};
	return hk.GetHotkeyName(szFull, bShowNone);
}

void ConEmuHotKey::GetVkKeyName(BYTE vk, wchar_t (&szName)[32])
{
	szName[0] = 0;

	switch (vk)
	{
	case 0:
		break;
	case VK_LWIN:
	case VK_RWIN:
		wcscat_c(szName, L"Win"); break;
	case VK_CONTROL:
		wcscat_c(szName, L"Ctrl"); break;
	case VK_LCONTROL:
		wcscat_c(szName, L"LCtrl"); break;
	case VK_RCONTROL:
		wcscat_c(szName, L"RCtrl"); break;
	case VK_MENU:
		wcscat_c(szName, L"Alt"); break;
	case VK_LMENU:
		wcscat_c(szName, L"LAlt"); break;
	case VK_RMENU:
		wcscat_c(szName, L"RAlt"); break;
	case VK_SHIFT:
		wcscat_c(szName, L"Shift"); break;
	case VK_LSHIFT:
		wcscat_c(szName, L"LShift"); break;
	case VK_RSHIFT:
		wcscat_c(szName, L"RShift"); break;
	case VK_APPS:
		wcscat_c(szName, L"Apps"); break;
	case VK_LEFT:
		wcscat_c(szName, L"Left"); break;
	case VK_RIGHT:
		wcscat_c(szName, L"Right"); break;
	case VK_UP:
		wcscat_c(szName, L"Up"); break;
	case VK_DOWN:
		wcscat_c(szName, L"Down"); break;
	case VK_PRIOR:
		wcscat_c(szName, L"PgUp"); break;
	case VK_NEXT:
		wcscat_c(szName, L"PgDn"); break;
	case VK_SPACE:
		wcscat_c(szName, L"Space"); break;
	case VK_TAB:
		wcscat_c(szName, L"Tab"); break;
	case VK_ESCAPE:
		wcscat_c(szName, L"Esc"); break;
	case VK_INSERT:
		wcscat_c(szName, L"Insert"); break;
	case VK_DELETE:
		wcscat_c(szName, L"Delete"); break;
	case VK_HOME:
		wcscat_c(szName, L"Home"); break;
	case VK_END:
		wcscat_c(szName, L"End"); break;
	case VK_PAUSE:
		wcscat_c(szName, L"Pause"); break;
	case VK_RETURN:
		wcscat_c(szName, L"Enter"); break;
	case VK_BACK:
		wcscat_c(szName, L"Backspace"); break;
	case 0xbd:
		wcscat_c(szName, L"-_"); break;
	case 0xbb:
		wcscat_c(szName, L"+="); break;

	case VK_WHEEL_UP:
		wcscat_c(szName, L"WheelUp"); break;
	case VK_WHEEL_DOWN:
		wcscat_c(szName, L"WheelDown"); break;
	case VK_WHEEL_LEFT:
		wcscat_c(szName, L"WheelLeft"); break;
	case VK_WHEEL_RIGHT:
		wcscat_c(szName, L"WheelRight"); break;

	case VK_LBUTTON:
		wcscat_c(szName, L"LButton"); break;
	case VK_RBUTTON:
		wcscat_c(szName, L"RButton"); break;
	case VK_MBUTTON:
		wcscat_c(szName, L"MButton"); break;

	default:
		if (vk >= VK_F1 && vk <= VK_F24)
		{
			_wsprintf(szName, SKIPLEN(countof(szName)) L"F%u", (DWORD)vk-VK_F1+1);
		}
		else if ((vk >= (BYTE)'A' && vk <= (BYTE)'Z') || (vk >= (BYTE)'0' && vk <= (BYTE)'9'))
		{
			szName[0] = vk;
			szName[1] = 0;
		}
		else
		{
			szName[0] = MapVirtualKey(vk, MAPVK_VK_TO_CHAR);
			szName[1] = 0;
			//BYTE States[256] = {};
			//// Скорее всго не сработает
			//if (!ToUnicode(vk, 0, States, szName, countof(szName), 0))
			//	_wsprintf(szName, SKIPLEN(countof(szName)) L"<%u>", (DWORD)vk);
			// есть еще if (!GetKeyNameText((LONG)(DWORD)*m_HotKeys[i].VkPtr, szName, countof(szName)))
		}
	}
}

UINT ConEmuHotKey::GetVkByKeyName(LPCWSTR asName, int* pnScanCode/*=NULL*/, DWORD* pnControlState/*=NULL*/)
{
	if (!asName || !*asName)
		return 0;

	// pnScanCode и pnControlState заранее не трогаем специально, вызывающая функция может в них что-то хранить

	static struct NameToVk { LPCWSTR sName; UINT VK; } Keys[] =
	{
		{L"Win", VK_LWIN}, {L"LWin", VK_LWIN}, {L"RWin", VK_RWIN},
		{L"Ctrl", VK_CONTROL}, {L"LCtrl", VK_LCONTROL}, {L"RCtrl", VK_RCONTROL},
		{L"Alt", VK_MENU}, {L"LAlt", VK_LMENU}, {L"RAlt", VK_RMENU},
		{L"Shift", VK_SHIFT}, {L"LShift", VK_LSHIFT}, {L"RShift", VK_RSHIFT},
		{L"Apps", VK_APPS},
		{L"Left", VK_LEFT}, {L"Right", VK_RIGHT}, {L"Up", VK_UP}, {L"Down", VK_DOWN},
		{L"PgUp", VK_PRIOR}, {L"PgDn", VK_NEXT}, {L"Home", VK_HOME}, {L"End", VK_END},
		{L"Space", VK_SPACE}, {L" ", VK_SPACE}, {L"Tab", VK_TAB}, {L"\t", VK_TAB},
		{L"Esc", VK_ESCAPE}, {L"\x1B", VK_ESCAPE},
		{L"Insert", VK_INSERT}, {L"Delete", VK_DELETE}, {L"Backspace", VK_BACK},
		{L"Enter", VK_RETURN}, {L"Return", VK_RETURN}, {L"NumpadEnter", VK_RETURN},
		{L"Pause", VK_PAUSE},
		//{L"_", 0xbd}, {L"=", 0xbb},
		{L"Numpad0", VK_NUMPAD0}, {L"Numpad1", VK_NUMPAD1}, {L"Numpad2", VK_NUMPAD2}, {L"Numpad3", VK_NUMPAD3}, {L"Numpad4", VK_NUMPAD4},
		{L"Numpad5", VK_NUMPAD5}, {L"Numpad6", VK_NUMPAD6}, {L"Numpad7", VK_NUMPAD7}, {L"Numpad8", VK_NUMPAD8}, {L"Numpad9", VK_NUMPAD9},
		{L"NumpadDot", VK_DECIMAL}, {L"NumpadDiv", VK_DIVIDE}, {L"NumpadMult", VK_MULTIPLY}, {L"NumpadAdd", VK_ADD}, {L"NumpadSub", VK_SUBTRACT},
		// ConEmu internal codes
		{L"WheelUp", VK_WHEEL_UP}, {L"WheelDown", VK_WHEEL_DOWN}, {L"WheelLeft", VK_WHEEL_LEFT}, {L"WheelRight", VK_WHEEL_RIGHT},
		// End of predefined codes
		{NULL}
	};

	// Fn?
	if (asName[0] == L'F' && isDigit(asName[1]) && (asName[2] == 0 || isDigit(asName[2])))
	{
		wchar_t* pEnd;
		long iNo = wcstol(asName, &pEnd, 10);
		if (iNo < 1 || iNo > 24)
			return 0;
		return (VK_F1 + (iNo-1));
	}

	// From table
	for (NameToVk* pk = Keys; pk->sName; pk++)
	{
		if (lstrcmp(asName, pk->sName) == 0)
			return pk->VK;
	}

	// Now, only One-char (OEM-ASCII) keys
	if (asName[1] == 0)
	{
		UINT vkScan = VkKeyScan(*asName);
		if (LOBYTE(vkScan))
		{
			UINT VK = LOBYTE(vkScan);
			if (pnControlState)
			{
				if (HIBYTE(vkScan) & 1)
					*pnControlState |= SHIFT_PRESSED;
				if ((HIBYTE(vkScan) & 2) && !((*pnControlState) & RIGHT_CTRL_PRESSED))
					*pnControlState |= LEFT_CTRL_PRESSED;
				if ((HIBYTE(vkScan) & 4) && !((*pnControlState) & RIGHT_ALT_PRESSED))
					*pnControlState |= LEFT_ALT_PRESSED;
			}
			UINT SC = MapVirtualKey(VK, 0/*MAPVK_VK_TO_VSC*/);
			if (pnScanCode)
				*pnScanCode = SC;
			return VK;
		}
	}

	// Unknown key
	return 0;
}

// Извлечь сам VK
DWORD ConEmuHotKey::GetHotkey(DWORD VkMod)
{
	return (VkMod & 0xFF);
}


bool ConEmuHotKey::UseWinNumber()
{
	return gpSetCls->IsMulti() && gpSet->isUseWinNumber;
}

bool ConEmuHotKey::UseWinArrows()
{
	return gpSet->isUseWinArrows;
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

	const Settings::AppSettings* pApp = gpSet->GetAppSettings(pRCon->GetActiveAppSettingsId());
	if (!pApp)
		return false;

	return pApp->CTSShiftArrowStart();
}

bool ConEmuHotKey::UseCtrlTab()
{
	return gpSet->isTabSelf;
}

bool ConEmuHotKey::DontHookJumps(const ConEmuHotKey* pHK)
{
	bool bDontHook = false;
#if 0
	switch (pHK->DescrLangID)
	{
	case vkJumpPrevMonitor:
		bDontHook = (pHK->VkMod == MakeHotKey(VK_LEFT,VK_LWIN,VK_SHIFT));
		break;
	case vkJumpNextMonitor:
		bDontHook = (pHK->VkMod == MakeHotKey(VK_RIGHT,VK_LWIN,VK_SHIFT));
		break;
	}
#endif
	return bDontHook;
}





/* ************************************* */
int ConEmuHotKey::AllocateHotkeys(ConEmuHotKey** ppHotKeys)
{
	// Горячие клавиши

	TODO("Дополнить системные комбинации");
	WARNING("У nLDragKey,nRDragKey был тип DWORD");

	WARNING("ConEmuHotKey: Убрать нафиг все ссылки на переменные, обработка будет прозрачная, а нажатость chk_Modifier можно по DescrLangID определять");

	//static const wchar_t szGuiMacroIncreaseFont[] = L"FontSetSize(1,2)";
	//static const wchar_t szGuiMacroDecreaseFont[] = L"FontSetSize(1,-2)";

	ConEmuHotKey HotKeys[] =
	{
		// User (Keys, Global) -- Добавить chk_Global недостаточно, нужно еще и gRegisteredHotKeys обработать
		{vkMinimizeRestore,chk_Global, NULL,   L"MinimizeRestore",       MakeHotKey(VK_OEM_3/*~*/,VK_CONTROL), CConEmuCtrl::key_MinimizeRestore},
		{vkMinimizeRestor2,chk_Global, NULL,   L"MinimizeRestore2",      0, CConEmuCtrl::key_MinimizeRestore},
		{vkGlobalRestore,  chk_Global, NULL,   L"GlobalRestore",         0, CConEmuCtrl::key_GlobalRestore},
		{vkForceFullScreen,chk_Global, NULL,   L"ForcedFullScreen",      MakeHotKey(VK_RETURN,VK_CONTROL,VK_LWIN,VK_MENU), CConEmuCtrl::key_ForcedFullScreen},
		// -- Добавить chk_Local недостаточно, нужно еще и gActiveOnlyHotKeys обработать
		{vkSetFocusSwitch, chk_Local,  NULL,   L"SwitchGuiFocus",        0, CConEmuCtrl::key_SwitchGuiFocus},
		{vkSetFocusGui,    chk_Local,  NULL,   L"SetFocusGui",           0, CConEmuCtrl::key_SwitchGuiFocus},
		{vkSetFocusChild,  chk_Local,  NULL,   L"SetFocusChild",         0, CConEmuCtrl::key_SwitchGuiFocus},
		{vkChildSystemMenu,chk_Local,  NULL,   L"ChildSystemMenu",       0, CConEmuCtrl::key_ChildSystemMenu},
		// User (Keys)
		// Splitters
		{vkSplitNewConV,   chk_User,  NULL,    L"Multi.NewSplitV",       MakeHotKey('O',VK_CONTROL,VK_SHIFT), CConEmuCtrl::key_GuiMacro, false, lstrdup(L"Split(0,0,50)")},
		{vkSplitNewConH,   chk_User,  NULL,    L"Multi.NewSplitH",       MakeHotKey('E',VK_CONTROL,VK_SHIFT), CConEmuCtrl::key_GuiMacro, false, lstrdup(L"Split(0,50,0)")},
		{vkSplitSizeVup,   chk_User,  NULL,    L"Multi.SplitSizeVU",     MakeHotKey(VK_UP,VK_APPS,VK_SHIFT), CConEmuCtrl::key_GuiMacro, false, lstrdup(L"Split(1,0,-1)")},
		{vkSplitSizeVdown, chk_User,  NULL,    L"Multi.SplitSizeVD",     MakeHotKey(VK_DOWN,VK_APPS,VK_SHIFT), CConEmuCtrl::key_GuiMacro, false, lstrdup(L"Split(1,0,1)")},
		{vkSplitSizeHleft, chk_User,  NULL,    L"Multi.SplitSizeHL",     MakeHotKey(VK_LEFT,VK_APPS,VK_SHIFT), CConEmuCtrl::key_GuiMacro, false, lstrdup(L"Split(1,-1,0)")},
		{vkSplitSizeHright,chk_User,  NULL,    L"Multi.SplitSizeHR",     MakeHotKey(VK_RIGHT,VK_APPS,VK_SHIFT), CConEmuCtrl::key_GuiMacro, false, lstrdup(L"Split(1,1,0)")},
		{vkTabPane,        chk_User,  NULL,    L"Key.TabPane1",          MakeHotKey(VK_TAB,VK_APPS), CConEmuCtrl::key_GuiMacro, false/*OnKeyUp*/, lstrdup(L"Tab(10,1)")}, // Next visible pane
		{vkTabPaneShift,   chk_User,  NULL,    L"Key.TabPane2",          MakeHotKey(VK_TAB,VK_APPS,VK_SHIFT), CConEmuCtrl::key_GuiMacro, false/*OnKeyUp*/, lstrdup(L"Tab(10,-1)")}, // Prev visible pane
		{vkSplitFocusUp,   chk_User,  NULL,    L"Multi.SplitFocusU",     MakeHotKey(VK_UP,VK_APPS), CConEmuCtrl::key_GuiMacro, false, lstrdup(L"Split(2,0,-1)")},
		{vkSplitFocusDown, chk_User,  NULL,    L"Multi.SplitFocusD",     MakeHotKey(VK_DOWN,VK_APPS), CConEmuCtrl::key_GuiMacro, false, lstrdup(L"Split(2,0,1)")},
		{vkSplitFocusLeft, chk_User,  NULL,    L"Multi.SplitFocusL",     MakeHotKey(VK_LEFT,VK_APPS), CConEmuCtrl::key_GuiMacro, false, lstrdup(L"Split(2,-1,0)")},
		{vkSplitFocusRight,chk_User,  NULL,    L"Multi.SplitFocusR",     MakeHotKey(VK_RIGHT,VK_APPS), CConEmuCtrl::key_GuiMacro, false, lstrdup(L"Split(2,1,0)")},
		// Multi-console
		{vkMultiNew,       chk_User,  NULL,    L"Multi.NewConsole",      MakeHotKey('W',VK_LWIN), CConEmuCtrl::key_MultiNew},
		{vkMultiNewShift,  chk_User,  NULL,    L"Multi.NewConsoleShift", MakeHotKey('W',VK_LWIN,VK_SHIFT), CConEmuCtrl::key_MultiNewShift},
		{vkMultiNewPopup,  chk_User,  NULL,    L"Multi.NewConsolePopup", MakeHotKey('N',VK_LWIN), CConEmuCtrl::key_MultiNewPopupMenu},
		{vkMultiNewPopup2, chk_User,  NULL,    L"Multi.NewConsolePopup2",0, CConEmuCtrl::key_MultiNewPopupMenu2},
		{vkMultiNewWnd,    chk_User,  NULL,    L"Multi.NewWindow",       0, CConEmuCtrl::key_MultiNewWindow},
		{vkMultiNewAttach, chk_User,  NULL,    L"Multi.NewAttach",       MakeHotKey('G',VK_LWIN), CConEmuCtrl::key_MultiNewAttach, true/*OnKeyUp*/},
		{vkMultiNext,      chk_User,  NULL,    L"Multi.Next",            MakeHotKey('Q',VK_LWIN), CConEmuCtrl::key_MultiNext},
		{vkMultiNextShift, chk_User,  NULL,    L"Multi.NextShift",       MakeHotKey('Q',VK_LWIN,VK_SHIFT), CConEmuCtrl::key_MultiNextShift},
		{vkMultiRecreate,  chk_User,  NULL,    L"Multi.Recreate",        MakeHotKey(192/*VK_тильда*/,VK_LWIN), CConEmuCtrl::key_MultiRecreate},
		{vkMultiAltCon,    chk_User,  NULL,    L"Multi.AltCon",          MakeHotKey('A',VK_LWIN), CConEmuCtrl::key_AlternativeBuffer},
		{vkMultiBuffer,    chk_User,  NULL,    L"Multi.Scroll",          0, CConEmuCtrl::key_MultiBuffer},
		{vkMultiClose,     chk_User,  NULL,    L"Multi.Close",           MakeHotKey(VK_DELETE,VK_LWIN), CConEmuCtrl::key_MultiClose},
		{vkCloseTab,       chk_User,  NULL,    L"CloseTabKey",           MakeHotKey(VK_DELETE,VK_LWIN,VK_MENU), CConEmuCtrl::key_GuiMacro, false, lstrdup(L"Close(6)")},
		{vkCloseGroup,     chk_User,  NULL,    L"CloseGroupKey",         0, CConEmuCtrl::key_GuiMacro, false, lstrdup(L"Close(4)")},
		{vkCloseGroupPrc,  chk_User,  NULL,    L"CloseGroupPrcKey",      0, CConEmuCtrl::key_GuiMacro, false, lstrdup(L"Close(7)")},
		{vkCloseAllCon,    chk_User,  NULL,    L"CloseAllConKey",        0, CConEmuCtrl::key_GuiMacro, false, lstrdup(L"Close(8)")},
		{vkCloseExceptCon, chk_User,  NULL,    L"CloseExceptConKey",     0, CConEmuCtrl::key_GuiMacro, false, lstrdup(L"Close(5)")},
		{vkTerminateApp,   chk_User,  NULL,    L"TerminateProcessKey",   MakeHotKey(VK_DELETE,VK_LWIN,VK_SHIFT), CConEmuCtrl::key_TerminateProcess/*sort of Close*/},
		{vkDuplicateRoot,  chk_User,  NULL,    L"DuplicateRootKey",      MakeHotKey('S',VK_LWIN), CConEmuCtrl::key_DuplicateRoot},
		//{vkDuplicateRootAs,chk_User,  NULL,    L"DuplicateRootAsKey",    0, CConEmuCtrl::key_DuplicateRootAs},
		{vkCloseConEmu,    chk_User,  NULL,    L"CloseConEmuKey",        MakeHotKey(VK_F4,VK_LWIN), /*sort of AltF4 for GUI apps*/CConEmuCtrl::key_GuiMacro, false, lstrdup(L"Close(2)")},
		{vkRenameTab,      chk_User,  NULL,    L"Multi.Rename",          MakeHotKey('R',VK_APPS), CConEmuCtrl::key_RenameTab, true/*OnKeyUp*/},
		{vkMoveTabLeft,    chk_User,  NULL,    L"Multi.MoveLeft",        MakeHotKey(VK_LEFT,VK_LWIN,VK_MENU), CConEmuCtrl::key_MoveTabLeft},
		{vkMoveTabRight,   chk_User,  NULL,    L"Multi.MoveRight",       MakeHotKey(VK_RIGHT,VK_LWIN,VK_MENU), CConEmuCtrl::key_MoveTabRight},
		{vkMultiCmd,       chk_User,  NULL,    L"Multi.CmdKey",          MakeHotKey('X',VK_LWIN), CConEmuCtrl::key_MultiCmd},
		{vkCTSVkBlockStart,chk_User,  NULL,    L"CTS.VkBlockStart",      0, CConEmuCtrl::key_CTSVkBlockStart}, // запуск выделения блока
		{vkCTSVkTextStart, chk_User,  NULL,    L"CTS.VkTextStart",       0, CConEmuCtrl::key_CTSVkTextStart},   // запуск выделения текста
		{vkCTSCopyHtml0,   chk_User,  InSelection, L"CTS.VkCopyFmt0",    MakeHotKey('C',VK_CONTROL), CConEmuCtrl::key_GuiMacro, false, lstrdup(L"Copy(0,0)")},
		{vkCTSCopyHtml1,   chk_User,  InSelection, L"CTS.VkCopyFmt1",    MakeHotKey('C',VK_CONTROL,VK_SHIFT), CConEmuCtrl::key_GuiMacro, false, lstrdup(L"Copy(0,1)")},
		{vkCTSCopyHtml2,   chk_User,  InSelection, L"CTS.VkCopyFmt2",    0, CConEmuCtrl::key_GuiMacro, false, lstrdup(L"Copy(0,2)")},
		{vkCTSVkCopyAll,   chk_User,  NULL,    L"CTS.VkCopyAll",         0, CConEmuCtrl::key_GuiMacro, false, lstrdup(L"Copy(1)")},
		{vkHighlightMouse, chk_User,  NULL,    L"HighlightMouseSwitch",  MakeHotKey('L',VK_APPS), CConEmuCtrl::key_GuiMacro, false, lstrdup(L"HighlightMouse(1)")},
		{vkShowTabsList,   chk_User,  NULL,    L"Multi.ShowTabsList",    0, CConEmuCtrl::key_ShowTabsList},
		{vkShowTabsList2,  chk_User,  NULL,    L"Multi.ShowTabsList2",   MakeHotKey(VK_F12,VK_APPS), CConEmuCtrl::key_GuiMacro, false, lstrdup(L"Tabs(8)")},
		{vkPasteText,      chk_User,  NULL,    L"ClipboardVkAllLines",   MakeHotKey(VK_INSERT,VK_SHIFT), CConEmuCtrl::key_PasteText},
		{vkPasteFirstLine, chk_User,  NULL,    L"ClipboardVkFirstLine",  MakeHotKey('V',VK_CONTROL), CConEmuCtrl::key_PasteFirstLine},
		{vkDeleteLeftWord, chk_User,  NULL,    L"DeleteWordToLeft",      MakeHotKey(VK_BACK,VK_CONTROL), CConEmuCtrl::key_DeleteWordToLeft},
		{vkFindTextDlg,    chk_User,  NULL,    L"FindTextKey",           MakeHotKey('F',VK_APPS), CConEmuCtrl::key_FindTextDlg},
		{vkScreenshot,     chk_User,  NULL,    L"ScreenshotKey",         MakeHotKey('H',VK_LWIN), CConEmuCtrl::key_Screenshot/*, true/ *OnKeyUp*/},
		{vkScreenshotFull, chk_User,  NULL,    L"ScreenshotFullKey",     MakeHotKey('H',VK_LWIN,VK_SHIFT), CConEmuCtrl::key_ScreenshotFull/*, true/ *OnKeyUp*/},
		{vkShowStatusBar,  chk_User,  NULL,    L"ShowStatusBarKey",      MakeHotKey('S',VK_APPS), CConEmuCtrl::key_ShowStatusBar},
		{vkShowTabBar,     chk_User,  NULL,    L"ShowTabBarKey",         MakeHotKey('T',VK_APPS), CConEmuCtrl::key_ShowTabBar},
		{vkShowCaption,    chk_User,  NULL,    L"ShowCaptionKey",        MakeHotKey('C',VK_APPS), CConEmuCtrl::key_ShowCaption},
		{vkAlwaysOnTop,    chk_User,  NULL,    L"AlwaysOnTopKey",        0, CConEmuCtrl::key_AlwaysOnTop},
		{vkTransparencyInc,chk_User,  NULL,    L"TransparencyInc",       0, CConEmuCtrl::key_GuiMacro, false, lstrdup(L"Transparency(1,-20)")},
		{vkTransparencyDec,chk_User,  NULL,    L"TransparencyDec",       0, CConEmuCtrl::key_GuiMacro, false, lstrdup(L"Transparency(1,+20)")},
		{vkTabMenu,        chk_User,  NULL,    L"Key.TabMenu",           MakeHotKey(VK_SPACE,VK_APPS), CConEmuCtrl::key_TabMenu, true/*OnKeyUp*/}, // Tab menu
		{vkTabMenu2,       chk_User,  NULL,    L"Key.TabMenu2",          MakeHotKey(VK_RBUTTON,VK_SHIFT), CConEmuCtrl::key_TabMenu, true/*OnKeyUp*/}, // Tab menu
		{vkMaximize,       chk_User,  NULL,    L"Key.Maximize",          MakeHotKey(VK_F9,VK_MENU), CConEmuCtrl::key_GuiMacro, false, lstrdup(L"WindowMaximize()")}, // Maximize window
		{vkMaximizeWidth,  chk_User,  NULL,    L"Key.MaximizeWidth",     0, CConEmuCtrl::key_GuiMacro, false, lstrdup(L"WindowMaximize(1)")}, // Maximize window width
		{vkMaximizeHeight, chk_User,  NULL,    L"Key.MaximizeHeight",    0, CConEmuCtrl::key_GuiMacro, false, lstrdup(L"WindowMaximize(2)")}, // Maximize window height
		{vkAltEnter,       chk_User,  NULL,    L"Key.FullScreen",        MakeHotKey(VK_RETURN,VK_MENU), CConEmuCtrl::key_GuiMacro, false, lstrdup(L"WindowFullscreen()")}, // Full screen
		{vkSystemMenu,     chk_User,  NULL,    L"Key.SysMenu",           MakeHotKey(VK_SPACE,VK_MENU), CConEmuCtrl::key_SystemMenu, true/*OnKeyUp*/}, // System menu
		{vkSystemMenu2,    chk_User,  NULL,    L"Key.SysMenu2",          MakeHotKey(VK_RBUTTON,VK_CONTROL), CConEmuCtrl::key_SystemMenu, true/*OnKeyUp*/}, // System menu
		// Scrolling
		{vkCtrlUp,         chk_User,  NULL,    L"Key.BufUp",             MakeHotKey(VK_UP,VK_CONTROL), CConEmuCtrl::key_BufferScrollUp}, // Buffer scroll
		{vkCtrlDown,       chk_User,  NULL,    L"Key.BufDn",             MakeHotKey(VK_DOWN,VK_CONTROL), CConEmuCtrl::key_BufferScrollDown}, // Buffer scroll
		{vkCtrlPgUp,       chk_User,  NULL,    L"Key.BufPgUp",           MakeHotKey(VK_PRIOR,VK_CONTROL), CConEmuCtrl::key_BufferScrollPgUp}, // Buffer scroll
		{vkCtrlPgDn,       chk_User,  NULL,    L"Key.BufPgDn",           MakeHotKey(VK_NEXT,VK_CONTROL), CConEmuCtrl::key_BufferScrollPgDn}, // Buffer scroll
		{vkAppsPgUp,       chk_User,  NULL,    L"Key.BufHfPgUp",         MakeHotKey(VK_PRIOR,VK_APPS), CConEmuCtrl::key_GuiMacro, false, lstrdup(L"Scroll(2,-1)")}, // Buffer scroll
		{vkAppsPgDn,       chk_User,  NULL,    L"Key.BufHfPgDn",         MakeHotKey(VK_NEXT,VK_APPS), CConEmuCtrl::key_GuiMacro, false, lstrdup(L"Scroll(2,+1)")}, // Buffer scroll
		{vkAppsHome,       chk_User,  NULL,    L"Key.BufTop",            MakeHotKey(VK_HOME,VK_APPS), CConEmuCtrl::key_GuiMacro, false, lstrdup(L"Scroll(3,-1)")}, // Buffer scroll
		{vkAppsEnd,        chk_User,  NULL,    L"Key.BufBottom",         MakeHotKey(VK_END,VK_APPS), CConEmuCtrl::key_GuiMacro, false, lstrdup(L"Scroll(3,+1)")}, // Buffer scroll
		{vkAppsBS,         chk_User,  NULL,    L"Key.BufCursor",         MakeHotKey(VK_BACK,VK_APPS), CConEmuCtrl::key_GuiMacro, false, lstrdup(L"Scroll(4)")}, // Buffer scroll
		//
		{vkPicViewSlide,   chk_User,  NULL,    L"Key.PicViewSlide",      MakeHotKey(VK_PAUSE), CConEmuCtrl::key_PicViewSlideshow, true/*OnKeyUp*/}, // Slideshow in PicView2
		{vkPicViewSlower,  chk_User,  NULL,    L"Key.PicViewSlower",     MakeHotKey(0xbd/* -_ */), CConEmuCtrl::key_PicViewSlideshow}, // Slideshow in PicView2
		{vkPicViewFaster,  chk_User,  NULL,    L"Key.PicViewFaster",     MakeHotKey(0xbb/* =+ */), CConEmuCtrl::key_PicViewSlideshow}, // Slideshow in PicView2
		{vkFontLarger,     chk_User,  NULL,    L"FontLargerKey",         MakeHotKey(VK_WHEEL_UP,VK_CONTROL), CConEmuCtrl::key_GuiMacro, false, lstrdup(L"FontSetSize(1,2)")},
		{vkFontSmaller,    chk_User,  NULL,    L"FontSmallerKey",        MakeHotKey(VK_WHEEL_DOWN,VK_CONTROL), CConEmuCtrl::key_GuiMacro, false, lstrdup(L"FontSetSize(1,-2)")},
		{vkFontOriginal,   chk_User,  NULL,    L"FontOriginalKey",       MakeHotKey(VK_MBUTTON,VK_CONTROL), CConEmuCtrl::key_GuiMacro, false, lstrdup(L"Zoom(100)")},
		{vkPasteFilePath,  chk_User,  NULL,    L"PasteFileKey",          MakeHotKey('F',VK_CONTROL,VK_SHIFT), CConEmuCtrl::key_GuiMacro, false, lstrdup(L"Paste(4)")},
		{vkPasteDirectory, chk_User,  NULL,    L"PastePathKey",          MakeHotKey('D',VK_CONTROL,VK_SHIFT), CConEmuCtrl::key_GuiMacro, false, lstrdup(L"Paste(5)")},
		{vkPasteCygwin,    chk_User,  NULL,    L"PasteCygwinKey",        MakeHotKey(VK_INSERT,VK_APPS), CConEmuCtrl::key_GuiMacro, false, lstrdup(L"Paste(8)")},
		{vkJumpPrevMonitor,chk_User,  NULL,    L"Key.JumpPrevMonitor",   MakeHotKey(VK_LEFT,VK_LWIN,VK_SHIFT),  CConEmuCtrl::key_GuiMacro, false, lstrdup(L"WindowMode(9)"),  DontHookJumps},
		{vkJumpNextMonitor,chk_User,  NULL,    L"Key.JumpNextMonitor",   MakeHotKey(VK_RIGHT,VK_LWIN,VK_SHIFT), CConEmuCtrl::key_GuiMacro, false, lstrdup(L"WindowMode(10)"), DontHookJumps},
		{vkTileToLeft,     chk_User,  NULL,    L"Key.TileToLeft",        MakeHotKey(VK_LEFT,VK_LWIN),  CConEmuCtrl::key_GuiMacro, false, lstrdup(L"WindowMode(6)"),  DontHookJumps},
		{vkTileToRight,    chk_User,  NULL,    L"Key.TileToRIght",       MakeHotKey(VK_RIGHT,VK_LWIN), CConEmuCtrl::key_GuiMacro, false, lstrdup(L"WindowMode(7)"), DontHookJumps},
		// GUI Macros
		{vkGuiMacro01,      chk_Macro, NULL,    L"KeyMacro01", 0, CConEmuCtrl::key_GuiMacro},
		{vkGuiMacro02,      chk_Macro, NULL,    L"KeyMacro02", 0, CConEmuCtrl::key_GuiMacro},
		{vkGuiMacro03,      chk_Macro, NULL,    L"KeyMacro03", 0, CConEmuCtrl::key_GuiMacro},
		{vkGuiMacro04,      chk_Macro, NULL,    L"KeyMacro04", 0, CConEmuCtrl::key_GuiMacro},
		{vkGuiMacro05,      chk_Macro, NULL,    L"KeyMacro05", 0, CConEmuCtrl::key_GuiMacro},
		{vkGuiMacro06,      chk_Macro, NULL,    L"KeyMacro06", 0, CConEmuCtrl::key_GuiMacro},
		{vkGuiMacro07,      chk_Macro, NULL,    L"KeyMacro07", 0, CConEmuCtrl::key_GuiMacro},
		{vkGuiMacro08,      chk_Macro, NULL,    L"KeyMacro08", 0, CConEmuCtrl::key_GuiMacro},
		{vkGuiMacro09,      chk_Macro, NULL,    L"KeyMacro09", 0, CConEmuCtrl::key_GuiMacro},
		{vkGuiMacro10,      chk_Macro, NULL,    L"KeyMacro10", 0, CConEmuCtrl::key_GuiMacro},
		{vkGuiMacro11,      chk_Macro, NULL,    L"KeyMacro11", 0, CConEmuCtrl::key_GuiMacro},
		{vkGuiMacro12,      chk_Macro, NULL,    L"KeyMacro12", 0, CConEmuCtrl::key_GuiMacro},
		{vkGuiMacro13,      chk_Macro, NULL,    L"KeyMacro13", 0, CConEmuCtrl::key_GuiMacro},
		{vkGuiMacro14,      chk_Macro, NULL,    L"KeyMacro14", 0, CConEmuCtrl::key_GuiMacro},
		{vkGuiMacro15,      chk_Macro, NULL,    L"KeyMacro15", 0, CConEmuCtrl::key_GuiMacro},
		{vkGuiMacro16,      chk_Macro, NULL,    L"KeyMacro16", 0, CConEmuCtrl::key_GuiMacro},
		{vkGuiMacro17,      chk_Macro, NULL,    L"KeyMacro17", 0, CConEmuCtrl::key_GuiMacro},
		{vkGuiMacro18,      chk_Macro, NULL,    L"KeyMacro18", 0, CConEmuCtrl::key_GuiMacro},
		{vkGuiMacro19,      chk_Macro, NULL,    L"KeyMacro19", 0, CConEmuCtrl::key_GuiMacro},
		{vkGuiMacro20,      chk_Macro, NULL,    L"KeyMacro20", 0, CConEmuCtrl::key_GuiMacro},
		{vkGuiMacro21,      chk_Macro, NULL,    L"KeyMacro21", 0, CConEmuCtrl::key_GuiMacro},
		{vkGuiMacro22,      chk_Macro, NULL,    L"KeyMacro22", 0, CConEmuCtrl::key_GuiMacro},
		{vkGuiMacro23,      chk_Macro, NULL,    L"KeyMacro23", 0, CConEmuCtrl::key_GuiMacro},
		{vkGuiMacro24,      chk_Macro, NULL,    L"KeyMacro24", 0, CConEmuCtrl::key_GuiMacro},
		{vkGuiMacro25,      chk_Macro, NULL,    L"KeyMacro25", 0, CConEmuCtrl::key_GuiMacro},
		{vkGuiMacro26,      chk_Macro, NULL,    L"KeyMacro26", 0, CConEmuCtrl::key_GuiMacro},
		{vkGuiMacro27,      chk_Macro, NULL,    L"KeyMacro27", 0, CConEmuCtrl::key_GuiMacro},
		{vkGuiMacro28,      chk_Macro, NULL,    L"KeyMacro28", 0, CConEmuCtrl::key_GuiMacro},
		{vkGuiMacro29,      chk_Macro, NULL,    L"KeyMacro29", 0, CConEmuCtrl::key_GuiMacro},
		{vkGuiMacro30,      chk_Macro, NULL,    L"KeyMacro30", 0, CConEmuCtrl::key_GuiMacro},
		{vkGuiMacro31,      chk_Macro, NULL,    L"KeyMacro31", 0, CConEmuCtrl::key_GuiMacro},
		{vkGuiMacro32,      chk_Macro, NULL,    L"KeyMacro32", 0, CConEmuCtrl::key_GuiMacro},
		// User (Modifiers)
		{vkCTSVkBlock,     chk_Modifier, NULL, L"CTS.VkBlock",     VK_LMENU},      // модификатор запуска выделения мышкой
		{vkCTSVkText,      chk_Modifier, NULL, L"CTS.VkText",      VK_LSHIFT},       // модификатор запуска выделения мышкой
		{vkCTSVkAct,       chk_Modifier, NULL, L"CTS.VkAct",       0},        // модификатор разрешения действий правой и средней кнопки мышки
		{vkCTSVkPromptClk, chk_Modifier, NULL, L"CTS.VkPrompt",    0}, // Модификатор позиционирования курсора мышки кликом (cmd.exe prompt)
		{vkFarGotoEditorVk,chk_Modifier, NULL, L"FarGotoEditorVk", VK_LCONTROL}, // модификатор для isFarGotoEditor
		{vkLDragKey,       chk_Modifier, NULL, L"DndLKey",         0},         // модификатор драга левой кнопкой
		{vkRDragKey,       chk_Modifier, NULL, L"DndRKey",         VK_LCONTROL},         // модификатор драга правой кнопкой
		{vkWndDragKey,     chk_Modifier2,NULL, L"WndDragKey",      MakeHotKey(VK_LBUTTON,VK_CONTROL,VK_MENU), CConEmuCtrl::key_WinDragStart}, // модификатор таскания окна мышкой за любое место
		// System (predefined, fixed)
		{vkWinAltA,        chk_System, NULL, L"", MakeHotKey('A',VK_LWIN,VK_MENU), CConEmuCtrl::key_GuiMacro, true/*OnKeyUp*/, lstrdup(L"About()")}, // Settings
		{vkWinAltK,        chk_System, NULL, L"", MakeHotKey('K',VK_LWIN,VK_MENU), CConEmuCtrl::key_GuiMacro, true/*OnKeyUp*/, lstrdup(L"Settings(171)")}, // Settings -> Keys&Macro
		{vkWinAltP,        chk_System, NULL, L"", MakeHotKey('P',VK_LWIN,VK_MENU), CConEmuCtrl::key_GuiMacro, true/*OnKeyUp*/, lstrdup(L"Settings()")}, // Settings
		{vkWinAltSpace,    chk_System, NULL, L"", MakeHotKey(VK_SPACE,VK_LWIN,VK_MENU), CConEmuCtrl::key_SystemMenu, true/*OnKeyUp*/}, // System menu
		{vkCtrlWinAltSpace,chk_System, NULL, L"", MakeHotKey(VK_SPACE,VK_CONTROL,VK_LWIN,VK_MENU), CConEmuCtrl::key_ShowRealConsole}, // Show real console
		{vkCtrlWinEnter,   chk_System, NULL, L"", MakeHotKey(VK_RETURN,VK_LWIN,VK_CONTROL), CConEmuCtrl::key_GuiMacro, false, lstrdup(L"WindowFullscreen()")},
		{vkCtrlTab,        chk_System, UseCtrlTab, L"", MakeHotKey(VK_TAB,VK_CONTROL), CConEmuCtrl::key_CtrlTab}, // Tab switch
		{vkCtrlShiftTab,   chk_System, UseCtrlTab, L"", MakeHotKey(VK_TAB,VK_CONTROL,VK_SHIFT), CConEmuCtrl::key_CtrlShiftTab}, // Tab switch
		{vkCtrlTab_Left,   chk_System, NULL, L"", MakeHotKey(VK_LEFT,VK_CONTROL), CConEmuCtrl::key_CtrlTab_Prev}, // Tab switch
		{vkCtrlTab_Up,     chk_System, NULL, L"", MakeHotKey(VK_UP,VK_CONTROL), CConEmuCtrl::key_CtrlTab_Prev}, // Tab switch
		{vkCtrlTab_Right,  chk_System, NULL, L"", MakeHotKey(VK_RIGHT,VK_CONTROL), CConEmuCtrl::key_CtrlTab_Next}, // Tab switch
		{vkCtrlTab_Down,   chk_System, NULL, L"", MakeHotKey(VK_DOWN,VK_CONTROL), CConEmuCtrl::key_CtrlTab_Next}, // Tab switch
		{vkEscNoConsoles,  chk_System, NULL, L"", MakeHotKey(VK_ESCAPE), CConEmuCtrl::key_MinimizeByEsc, false/*OnKeyUp*/}, // Minimize ConEmu by Esc when no open consoles left
		{vkCTSShiftLeft,   chk_System, UseCTSShiftArrow, L"", MakeHotKey(VK_LEFT,VK_SHIFT),  CConEmuCtrl::key_GuiMacro, false, lstrdup(L"Select(0,-1)")},
		{vkCTSShiftRight,  chk_System, UseCTSShiftArrow, L"", MakeHotKey(VK_RIGHT,VK_SHIFT), CConEmuCtrl::key_GuiMacro, false, lstrdup(L"Select(0,1)")},
		{vkCTSShiftHome,   chk_System, UseCTSShiftArrow, L"", MakeHotKey(VK_HOME,VK_SHIFT),  CConEmuCtrl::key_GuiMacro, false, lstrdup(L"Select(0,-1,0,-1)")},
		{vkCTSShiftEnd,    chk_System, UseCTSShiftArrow, L"", MakeHotKey(VK_END,VK_SHIFT),   CConEmuCtrl::key_GuiMacro, false, lstrdup(L"Select(0,1,0,1)")},
		{vkCTSShiftUp,     chk_System, UseCTSShiftArrow, L"", MakeHotKey(VK_UP,VK_SHIFT),    CConEmuCtrl::key_GuiMacro, false, lstrdup(L"Select(1,0,-1)")},
		{vkCTSShiftDown,   chk_System, UseCTSShiftArrow, L"", MakeHotKey(VK_DOWN,VK_SHIFT),  CConEmuCtrl::key_GuiMacro, false, lstrdup(L"Select(1,0,1)")},
		// Все что ниже - было привязано к "HostKey"
		// Надо бы дать настроить модификатор, а сами кнопки - не трогать
		{vkWinLeft,    chk_ArrHost, UseWinArrows, L"", VK_LEFT|CEHOTKEY_ARRHOSTKEY,  CConEmuCtrl::key_WinWidthDec},  // Decrease window width
		{vkWinRight,   chk_ArrHost, UseWinArrows, L"", VK_RIGHT|CEHOTKEY_ARRHOSTKEY, CConEmuCtrl::key_WinWidthInc},  // Increase window width
		{vkWinUp,      chk_ArrHost, UseWinArrows, L"", VK_UP|CEHOTKEY_ARRHOSTKEY,    CConEmuCtrl::key_WinHeightDec}, // Decrease window height
		{vkWinDown,    chk_ArrHost, UseWinArrows, L"", VK_DOWN|CEHOTKEY_ARRHOSTKEY,  CConEmuCtrl::key_WinHeightInc}, // Increase window height
		// Console activate by number
		{vkConsole_1,  chk_NumHost, UseWinNumber, L"", '1'|CEHOTKEY_NUMHOSTKEY, CConEmuCtrl::key_ConsoleNum},
		{vkConsole_2,  chk_NumHost, UseWinNumber, L"", '2'|CEHOTKEY_NUMHOSTKEY, CConEmuCtrl::key_ConsoleNum},
		{vkConsole_3,  chk_NumHost, UseWinNumber, L"", '3'|CEHOTKEY_NUMHOSTKEY, CConEmuCtrl::key_ConsoleNum},
		{vkConsole_4,  chk_NumHost, UseWinNumber, L"", '4'|CEHOTKEY_NUMHOSTKEY, CConEmuCtrl::key_ConsoleNum},
		{vkConsole_5,  chk_NumHost, UseWinNumber, L"", '5'|CEHOTKEY_NUMHOSTKEY, CConEmuCtrl::key_ConsoleNum},
		{vkConsole_6,  chk_NumHost, UseWinNumber, L"", '6'|CEHOTKEY_NUMHOSTKEY, CConEmuCtrl::key_ConsoleNum},
		{vkConsole_7,  chk_NumHost, UseWinNumber, L"", '7'|CEHOTKEY_NUMHOSTKEY, CConEmuCtrl::key_ConsoleNum},
		{vkConsole_8,  chk_NumHost, UseWinNumber, L"", '8'|CEHOTKEY_NUMHOSTKEY, CConEmuCtrl::key_ConsoleNum},
		{vkConsole_9,  chk_NumHost, UseWinNumber, L"", '9'|CEHOTKEY_NUMHOSTKEY, CConEmuCtrl::key_ConsoleNum},
		{vkConsole_10, chk_NumHost, UseWinNumber, L"", '0'|CEHOTKEY_NUMHOSTKEY, CConEmuCtrl::key_ConsoleNum},
		//{vkConsole_11, chk_NumHost, &isUseWinNumber, L"", VK_F11|CEHOTKEY_NUMHOSTKEY, CConEmuCtrl::key_ConsoleNum, true/*OnKeyUp*/}, // Для WinF11 & WinF12 приходят только WM_KEYUP || WM_SYSKEYUP)
		//{vkConsole_12, chk_NumHost, &isUseWinNumber, L"", VK_F12|CEHOTKEY_NUMHOSTKEY, CConEmuCtrl::key_ConsoleNum, true/*OnKeyUp*/}, // Для WinF11 & WinF12 приходят только WM_KEYUP || WM_SYSKEYUP)
		// End
	};

	// Чтобы не возникло проблем с инициализацией хуков (для обработки Win+<key>)
	int nHotKeyCount = countof(HotKeys);
	_ASSERTE(countof(HotKeys)<(HookedKeysMaxCount-1));

	*ppHotKeys = (ConEmuHotKey*)malloc(sizeof(HotKeys));
	_ASSERTE(*ppHotKeys!=NULL);

	memmove(*ppHotKeys, HotKeys, sizeof(HotKeys));

	return nHotKeyCount;
}
