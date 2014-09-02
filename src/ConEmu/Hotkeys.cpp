
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
	m_HK.SetVkMod(aVkMod);

	mp_DpiAware = new CDpiForDialog();
}

CHotKeyDialog::~CHotKeyDialog()
{
	SafeDelete(mp_DpiAware);
}

DWORD CHotKeyDialog::GetVkMod()
{
	return m_HK.GetVkMod();
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

			BYTE vk = pDlg->m_HK.Key.Vk;
			CSettings::SetHotkeyField(hHk, vk);

			// Warning! Если nVK не указан в SettingsNS::nKeysHot - nVK будет обнулен
			CSettings::FillListBoxHotKeys(GetDlgItem(hDlg, lbHotKeyList), CSettings::eHkKeysHot, vk);

			BYTE Mods[3];
			pDlg->m_HK.Key.GetModifiers(Mods);
			for (int n = 0; n < 3; n++)
			{
				CSettings::FillListBoxHotKeys(GetDlgItem(hDlg, lbHotKeyMod1+n), CSettings::eHkModifiers, Mods[n]);
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
					pDlg->m_HK.Key.Set(LOBYTE(nHotKey), pDlg->m_HK.Key.Mod);
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

							if (pDlg->m_HK.Key.Mod == cvk_NULL)
							{
								// Если модификатора вообще не было - ставим Win
								BYTE b = VK_LWIN;
								CSettings::FillListBoxHotKeys(GetDlgItem(hDlg, lbHotKeyMod1), CSettings::eHkModifiers, b);
								pDlg->m_HK.Key.Mod = cvk_Win;
							}

							pDlg->m_HK.Key.Set(LOBYTE(vk), pDlg->m_HK.Key.Mod);

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

							pDlg->m_HK.SetVkMod(((DWORD)pDlg->m_HK.Key.Vk) | nModifers);

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

extern CEVkMatch gvkMatchList[];

// That is what is stored in the settings
DWORD ConEmuHotKey::GetVkMod() const
{
	DWORD VkMod = 0;
	DWORD Vk = (DWORD)(BYTE)Key.Vk;

	if (Vk)
	{
		// Let iterate
		int iPos = 8; // <<
		ConEmuModifiers Mod = Key.Mod;

		if (Mod & cvk_Naked)
		{
			// Nothing to add?
		}
		else if (Mod & cvk_NumHost)
		{
			VkMod = (gpSet->HostkeyNumberModifier() << 8);
		}
		else if (Mod & cvk_ArrHost)
		{
			VkMod = (gpSet->HostkeyArrowModifier() << 8);
		}
		else for (size_t i = 0; i < gvkMatchList[i].Vk; i++)
		{
			const CEVkMatch& k = gvkMatchList[i];

			if (Mod & k.Mod)
			{
				VkMod |= (k.Vk << iPos);
				iPos += 8;
				if (iPos > 24)
					break;

				if (!k.Distinct)
				{
					_ASSERTE((k.Left|k.Right) != cvk_NULL);
					i += 2;
				}
			}
		}

		if (!VkMod && (HkType != chk_Modifier))
			VkMod = CEHOTKEY_NOMOD;

		VkMod |= Vk;
	}

	return VkMod;
}

void ConEmuHotKey::SetVkMod(DWORD VkMod)
{
	// Init
	BYTE Vk = LOBYTE(VkMod);
	ConEmuModifiers Mod = cvk_NULL;

	// Check modifiers
	DWORD vkLeft = (VkMod & CEHOTKEY_MODMASK);

	if ((HkType == chk_NumHost) || (vkLeft == CEHOTKEY_NUMHOSTKEY))
	{
		_ASSERTE((HkType == chk_NumHost) && (vkLeft == CEHOTKEY_NUMHOSTKEY))
		Mod = cvk_NumHost;
	}
	else if ((HkType == chk_ArrHost) || (vkLeft == CEHOTKEY_ARRHOSTKEY))
	{
		_ASSERTE((HkType == chk_ArrHost) && (vkLeft == CEHOTKEY_ARRHOSTKEY))
		Mod = cvk_ArrHost;
	}
	else if (vkLeft == CEHOTKEY_NOMOD)
	{
		Mod = cvk_Naked;
	}
	else if (vkLeft)
	{
		Mod = CEVkMatch::GetFlagsFromMod(vkLeft);
	}
	else
	{
		Mod = cvk_NULL;
	}

	Key.Set(Vk, Mod);
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

void ConEmuHotKey::SetHotKey(BYTE Vk, BYTE vkMod1/*=0*/, BYTE vkMod2/*=0*/, BYTE vkMod3/*=0*/)
{
	SetVkMod(MakeHotKey(Vk, vkMod1, vkMod2, vkMod3));
}

bool ConEmuHotKey::Equal(BYTE Vk, BYTE vkMod1/*=0*/, BYTE vkMod2/*=0*/, BYTE vkMod3/*=0*/)
{
	//TODO
	DWORD VkMod = GetVkMod();
	DWORD VkModCmp = MakeHotKey(Vk, vkMod1, vkMod2, vkMod3);
	return (VkMod == VkModCmp);
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

#ifdef _DEBUG
void ConEmuHotKey::HotkeyNameUnitTests()
{
	// Key names order:
	//    Win, Apps, Ctrl, Alt, Shift
	// Modifiers:
	//    if "Ctrl" (any) - only cvk_Ctrl must be set
	//    if "RCtrl" (or L) - then cvk_Ctrl AND cvk_RCtrl must be set
	//    if both "LCtrl+RCtrl" - then ALL cvk_Ctrl, cvk_LCtrl, cvk_RCtrl must be set
	struct {
		BYTE Vk;
		BYTE Mod[3];
		ConEmuModifiers ModTest;
		LPCWSTR KeyName;
	} Tests[] = {
			{VK_OEM_5/*'|\'*/, {VK_LCONTROL, VK_RCONTROL}, cvk_Ctrl|cvk_LCtrl|cvk_RCtrl, L"LCtrl+RCtrl+\\"},
			{VK_SPACE, {VK_CONTROL, VK_LWIN, VK_MENU}, cvk_Ctrl|cvk_Alt|cvk_Win, L"Win+Ctrl+Alt+Space"},
			{'W', {VK_LWIN, VK_SHIFT}, cvk_Win|cvk_Shift, L"Win+Shift+W"},
			{'L', {VK_RCONTROL}, cvk_Ctrl|cvk_RCtrl, L"RCtrl+L"},
			{'C', {VK_CONTROL}, cvk_Ctrl, L"Ctrl+C"},
			{0, {}, cvk_Naked, L"<None>"},
	};

	ConEmuHotKey HK = {0, chk_User};
	wchar_t szFull[128];
	int iCmp;

	for (size_t i = 0; i < countof(Tests); i++)
	{
		HK.SetHotKey(Tests[i].Vk, Tests[i].Mod[0], Tests[i].Mod[1], Tests[i].Mod[2]);
		_ASSERTE(HK.Key.Mod == Tests[i].ModTest);
		iCmp = wcscmp(HK.GetHotkeyName(szFull, true), Tests[i].KeyName);
		_ASSERTE(iCmp == 0);
	}
}
#endif

// Вернуть имя модификатора (типа "Apps+Space")
LPCWSTR ConEmuHotKey::GetHotkeyName(wchar_t (&szFull)[128], bool bShowNone /*= true*/) const
{
	_ASSERTE(this && this!=ConEmuSkipHotKey);

	wchar_t szName[32];
	szFull[0] = 0;

	switch (HkType)
	{
	case chk_Global:
	case chk_Local:
	case chk_User:
	case chk_Macro:
	case chk_Task:
	case chk_Modifier2:
	case chk_System:
	case chk_Modifier:
	case chk_NumHost:
	case chk_ArrHost:
		for (size_t i = 0; i < gvkMatchList[i].Vk; i++)
		{
			const CEVkMatch& k = gvkMatchList[i];

			if ((Key.Mod & k.Mod) && !(Key.Mod & (k.Left|k.Right)))
			{
				GetVkKeyName(k.Vk, szName);
				if (szFull[0])
					wcscat_c(szFull, L"+");
				wcscat_c(szFull, szName);
			}
		}
		break;
	default:
		// Неизвестный тип!
		_ASSERTE(FALSE && "Unknown HkType");
	}

	// chk_Modifier2 is used (yet) only for "Drag window by client area"
	// This key by default is "Ctrl+Alt+LBtn"
	// And we show in the hotkeys list only "Ctrl+Alt"
	if (HkType != chk_Modifier2)
	{
		szName[0] = 0;
		GetVkKeyName(Key.Vk, szName);

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

bool ConEmuHotKey::UseDndLKey()
{
	return ((gpSet->isDragEnabled & DRAG_L_ALLOWED) == DRAG_L_ALLOWED);
}

bool ConEmuHotKey::UseDndRKey()
{
	return ((gpSet->isDragEnabled & DRAG_R_ALLOWED) == DRAG_R_ALLOWED);
}

/*
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
*/





/* ************************************* */
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

int ConEmuHotKeyList::AllocateHotkeys()
{
	// Горячие клавиши

	TODO("Дополнить системные комбинации");
	WARNING("У nLDragKey,nRDragKey был тип DWORD");

	WARNING("ConEmuHotKey: Убрать нафиг все ссылки на переменные, обработка будет прозрачная, а нажатость chk_Modifier можно по DescrLangID определять");

	//static const wchar_t szGuiMacroIncreaseFont[] = L"FontSetSize(1,2)";
	//static const wchar_t szGuiMacroDecreaseFont[] = L"FontSetSize(1,-2)";

	_ASSERTE(this->empty());

		// User (Keys, Global) -- Добавить chk_Global недостаточно, нужно еще и gRegisteredHotKeys обработать
	Add(vkMinimizeRestore,chk_Global, NULL,   L"MinimizeRestore",       CConEmuCtrl::key_MinimizeRestore)
		->SetHotKey(VK_OEM_3/*~*/,VK_CONTROL);
	Add(vkMinimizeRestor2,chk_Global, NULL,   L"MinimizeRestore2",      CConEmuCtrl::key_MinimizeRestore)
		;
	Add(vkGlobalRestore,  chk_Global, NULL,   L"GlobalRestore",         CConEmuCtrl::key_GlobalRestore)
		;
	Add(vkForceFullScreen,chk_Global, NULL,   L"ForcedFullScreen",      CConEmuCtrl::key_ForcedFullScreen)
		->SetHotKey(VK_RETURN,VK_CONTROL,VK_LWIN,VK_MENU);
		// -- Добавить chk_Local недостаточно, нужно еще и gActiveOnlyHotKeys обработать
	Add(vkSetFocusSwitch, chk_Local,  NULL,   L"SwitchGuiFocus",        CConEmuCtrl::key_SwitchGuiFocus)
		;
	Add(vkSetFocusGui,    chk_Local,  NULL,   L"SetFocusGui",           CConEmuCtrl::key_SwitchGuiFocus)
		;
	Add(vkSetFocusChild,  chk_Local,  NULL,   L"SetFocusChild",         CConEmuCtrl::key_SwitchGuiFocus)
		;
	Add(vkChildSystemMenu,chk_Local,  NULL,   L"ChildSystemMenu",       CConEmuCtrl::key_ChildSystemMenu);
		// User (Keys)
		// Splitters
	Add(vkSplitNewConV,   chk_User,  NULL,    L"Multi.NewSplitV",       CConEmuCtrl::key_GuiMacro, false, L"Split(0,0,50)")
		->SetHotKey('O',VK_CONTROL,VK_SHIFT);
	Add(vkSplitNewConH,   chk_User,  NULL,    L"Multi.NewSplitH",       CConEmuCtrl::key_GuiMacro, false, L"Split(0,50,0)")
		->SetHotKey('E',VK_CONTROL,VK_SHIFT);
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
		// Multi-console
	Add(vkMultiNew,       chk_User,  NULL,    L"Multi.NewConsole",      CConEmuCtrl::key_MultiNew)
		->SetHotKey('W',VK_LWIN);
	Add(vkMultiNewShift,  chk_User,  NULL,    L"Multi.NewConsoleShift", CConEmuCtrl::key_MultiNewShift)
		->SetHotKey('W',VK_LWIN,VK_SHIFT);
	Add(vkMultiNewPopup,  chk_User,  NULL,    L"Multi.NewConsolePopup", CConEmuCtrl::key_MultiNewPopupMenu)
		->SetHotKey('N',VK_LWIN);
	Add(vkMultiNewPopup2, chk_User,  NULL,    L"Multi.NewConsolePopup2",CConEmuCtrl::key_MultiNewPopupMenu2)
		;
	Add(vkMultiNewWnd,    chk_User,  NULL,    L"Multi.NewWindow",       CConEmuCtrl::key_MultiNewWindow)
		;
	Add(vkMultiNewAttach, chk_User,  NULL,    L"Multi.NewAttach",       CConEmuCtrl::key_MultiNewAttach, true/*OnKeyUp*/)
		->SetHotKey('G',VK_LWIN);
	Add(vkMultiNext,      chk_User,  NULL,    L"Multi.Next",            CConEmuCtrl::key_MultiNext)
		->SetHotKey('Q',VK_LWIN);
	Add(vkMultiNextShift, chk_User,  NULL,    L"Multi.NextShift",       CConEmuCtrl::key_MultiNextShift)
		->SetHotKey('Q',VK_LWIN,VK_SHIFT);
	Add(vkMultiRecreate,  chk_User,  NULL,    L"Multi.Recreate",        CConEmuCtrl::key_MultiRecreate)
		->SetHotKey(192/*VK_тильда*/,VK_LWIN);
	Add(vkMultiAltCon,    chk_User,  NULL,    L"Multi.AltCon",          CConEmuCtrl::key_AlternativeBuffer)
		->SetHotKey('A',VK_LWIN);
	Add(vkMultiBuffer,    chk_User,  NULL,    L"Multi.Scroll",          CConEmuCtrl::key_MultiBuffer)
		;
	Add(vkMultiClose,     chk_User,  NULL,    L"Multi.Close",           CConEmuCtrl::key_MultiClose)
		->SetHotKey(VK_DELETE,VK_LWIN);
	Add(vkCloseTab,       chk_User,  NULL,    L"CloseTabKey",           CConEmuCtrl::key_GuiMacro, false, L"Close(6)")
		->SetHotKey(VK_DELETE,VK_LWIN,VK_MENU);
	Add(vkCloseGroup,     chk_User,  NULL,    L"CloseGroupKey",         CConEmuCtrl::key_GuiMacro, false, L"Close(4)")
		;
	Add(vkCloseGroupPrc,  chk_User,  NULL,    L"CloseGroupPrcKey",      CConEmuCtrl::key_GuiMacro, false, L"Close(7)")
		;
	Add(vkCloseAllCon,    chk_User,  NULL,    L"CloseAllConKey",        CConEmuCtrl::key_GuiMacro, false, L"Close(8)")
		;
	Add(vkCloseExceptCon, chk_User,  NULL,    L"CloseExceptConKey",     CConEmuCtrl::key_GuiMacro, false, L"Close(5)")
		;
	Add(vkTerminateApp,   chk_User,  NULL,    L"TerminateProcessKey",   CConEmuCtrl::key_TerminateProcess/*sort of Close*/)
		->SetHotKey(VK_DELETE,VK_LWIN,VK_SHIFT);
	Add(vkDuplicateRoot,  chk_User,  NULL,    L"DuplicateRootKey",      CConEmuCtrl::key_DuplicateRoot)
		->SetHotKey('S',VK_LWIN);
	Add(vkCloseConEmu,    chk_User,  NULL,    L"CloseConEmuKey",        /*sort of AltF4 for GUI apps*/CConEmuCtrl::key_GuiMacro, false, L"Close(2)")
		->SetHotKey(VK_F4,VK_LWIN);
	Add(vkRenameTab,      chk_User,  NULL,    L"Multi.Rename",          CConEmuCtrl::key_RenameTab, true/*OnKeyUp*/)
		->SetHotKey('R',VK_APPS);
	Add(vkMoveTabLeft,    chk_User,  NULL,    L"Multi.MoveLeft",        CConEmuCtrl::key_MoveTabLeft)
		->SetHotKey(VK_LEFT,VK_LWIN,VK_MENU);
	Add(vkMoveTabRight,   chk_User,  NULL,    L"Multi.MoveRight",       CConEmuCtrl::key_MoveTabRight)
		->SetHotKey(VK_RIGHT,VK_LWIN,VK_MENU);
	Add(vkMultiCmd,       chk_User,  NULL,    L"Multi.CmdKey",          CConEmuCtrl::key_MultiCmd)
		->SetHotKey('X',VK_LWIN);
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
	Add(vkShowTabsList,   chk_User,  NULL,    L"Multi.ShowTabsList",    CConEmuCtrl::key_ShowTabsList)
		;
	Add(vkShowTabsList2,  chk_User,  NULL,    L"Multi.ShowTabsList2",   CConEmuCtrl::key_GuiMacro, false, L"Tabs(8)")
		->SetHotKey(VK_F12,VK_APPS);
	Add(vkPasteText,      chk_User,  NULL,    L"ClipboardVkAllLines",   CConEmuCtrl::key_PasteText)
		->SetHotKey(VK_INSERT,VK_SHIFT);
	Add(vkPasteFirstLine, chk_User,  NULL,    L"ClipboardVkFirstLine",  CConEmuCtrl::key_PasteFirstLine)
		->SetHotKey('V',VK_CONTROL);
	Add(vkDeleteLeftWord, chk_User,  NULL,    L"DeleteWordToLeft",      CConEmuCtrl::key_DeleteWordToLeft)
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
	Add(vkMaximizeWidth,  chk_User,  NULL,    L"Key.MaximizeWidth",     CConEmuCtrl::key_GuiMacro, false, L"WindowMaximize(1)") // Maximize window width
		;
	Add(vkMaximizeHeight, chk_User,  NULL,    L"Key.MaximizeHeight",    CConEmuCtrl::key_GuiMacro, false, L"WindowMaximize(2)") // Maximize window height
		;
	Add(vkAltEnter,       chk_User,  NULL,    L"Key.FullScreen",        CConEmuCtrl::key_GuiMacro, false, L"WindowFullscreen()") // Full screen
		->SetHotKey(VK_RETURN,VK_MENU);
	Add(vkSystemMenu,     chk_User,  NULL,    L"Key.SysMenu",           CConEmuCtrl::key_SystemMenu, true/*OnKeyUp*/) // System menu
		->SetHotKey(VK_SPACE,VK_MENU);
	Add(vkSystemMenu2,    chk_User,  NULL,    L"Key.SysMenu2",          CConEmuCtrl::key_SystemMenu, true/*OnKeyUp*/) // System menu
		->SetHotKey(VK_RBUTTON,VK_CONTROL);
		// Scrolling
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
	Add(vkAppsBS,         chk_User,  NULL,    L"Key.BufCursor",         CConEmuCtrl::key_GuiMacro, false, L"Scroll(4)") // Buffer scroll
		->SetHotKey(VK_BACK,VK_APPS);
		//
	Add(vkPicViewSlide,   chk_User,  NULL,    L"Key.PicViewSlide",      CConEmuCtrl::key_PicViewSlideshow, true/*OnKeyUp*/) // Slideshow in PicView2
		->SetHotKey(VK_PAUSE);
	Add(vkPicViewSlower,  chk_User,  NULL,    L"Key.PicViewSlower",     CConEmuCtrl::key_PicViewSlideshow) // Slideshow in PicView2
		->SetHotKey(0xbd/* -_ */);
	Add(vkPicViewFaster,  chk_User,  NULL,    L"Key.PicViewFaster",     CConEmuCtrl::key_PicViewSlideshow) // Slideshow in PicView2
		->SetHotKey(0xbb/* =+ */);
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
	Add(vkJumpPrevMonitor,chk_User,  NULL,    L"Key.JumpPrevMonitor",   CConEmuCtrl::key_GuiMacro, false, L"WindowMode(9)"/*,  DontHookJumps*/)
		->SetHotKey(VK_LEFT,VK_LWIN,VK_SHIFT);
	Add(vkJumpNextMonitor,chk_User,  NULL,    L"Key.JumpNextMonitor",   CConEmuCtrl::key_GuiMacro, false, L"WindowMode(10)"/*, DontHookJumps*/)
		->SetHotKey(VK_RIGHT,VK_LWIN,VK_SHIFT);
	Add(vkTileToLeft,     chk_User,  NULL,    L"Key.TileToLeft",        CConEmuCtrl::key_GuiMacro, false, L"WindowMode(6)"/*,  DontHookJumps*/)
		->SetHotKey(VK_LEFT,VK_LWIN);
	Add(vkTileToRight,    chk_User,  NULL,    L"Key.TileToRIght",       CConEmuCtrl::key_GuiMacro, false, L"WindowMode(7)"/*, DontHookJumps*/)
		->SetHotKey(VK_RIGHT,VK_LWIN);
		// GUI Macros
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
		// User (Modifiers)
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
	Add(vkWndDragKey,     chk_Modifier2,NULL, L"WndDragKey", CConEmuCtrl::key_WinDragStart) // модификатор таскания окна мышкой за любое место
		->SetHotKey(VK_LBUTTON,VK_CONTROL,VK_MENU);
		// System (predefined, fixed)
	Add(vkWinAltA,        chk_System, NULL, L"", CConEmuCtrl::key_GuiMacro, true/*OnKeyUp*/, L"About()") // Settings
		->SetHotKey('A',VK_LWIN,VK_MENU);
	Add(vkWinAltK,        chk_System, NULL, L"", CConEmuCtrl::key_GuiMacro, true/*OnKeyUp*/, L"Settings(171)") // Settings -> Keys&Macro
		->SetHotKey('K',VK_LWIN,VK_MENU);
	Add(vkWinAltP,        chk_System, NULL, L"", CConEmuCtrl::key_GuiMacro, true/*OnKeyUp*/, L"Settings()") // Settings
		->SetHotKey('P',VK_LWIN,VK_MENU);
	Add(vkWinAltSpace,    chk_System, NULL, L"", CConEmuCtrl::key_SystemMenu, true/*OnKeyUp*/) // System menu
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
		// Надо бы дать настроить модификатор, а сами кнопки - не трогать
	Add(vkWinLeft,    chk_ArrHost, ConEmuHotKey::UseWinArrows, L"", CConEmuCtrl::key_WinWidthDec)  // Decrease window width
		->SetVkMod(VK_LEFT|CEHOTKEY_ARRHOSTKEY);
	Add(vkWinRight,   chk_ArrHost, ConEmuHotKey::UseWinArrows, L"", CConEmuCtrl::key_WinWidthInc)  // Increase window width
		->SetVkMod(VK_RIGHT|CEHOTKEY_ARRHOSTKEY);
	Add(vkWinUp,      chk_ArrHost, ConEmuHotKey::UseWinArrows, L"", CConEmuCtrl::key_WinHeightDec) // Decrease window height
		->SetVkMod(VK_UP|CEHOTKEY_ARRHOSTKEY);
	Add(vkWinDown,    chk_ArrHost, ConEmuHotKey::UseWinArrows, L"", CConEmuCtrl::key_WinHeightInc) // Increase window height
		->SetVkMod(VK_DOWN|CEHOTKEY_ARRHOSTKEY);
		// Console activate by number
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

	UpdateNumberModifier();
	UpdateArrowModifier();

	// Чтобы не возникло проблем с инициализацией хуков (для обработки Win+<key>)
	int nHotKeyCount = this->size();
	_ASSERTE(nHotKeyCount<(HookedKeysMaxCount-1));

	return nHotKeyCount;
}
