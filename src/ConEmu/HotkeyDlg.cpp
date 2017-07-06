
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
#include "HotkeyDlg.h"
#include "DpiAware.h"
#include "DynDialog.h"
#include "LngRc.h"
#include "OptionsClass.h"
#include "SetDlgLists.h"

/* *********** Hotkey editor dialog *********** */
bool CHotKeyDialog::EditHotKey(HWND hParent, DWORD& VkMod)
{
	CHotKeyDialog Dlg(hParent, VkMod);
	// Modal dialog (CreateDialog)
	INT_PTR iRc = CDynDialog::ExecuteDialog(IDD_HOTKEY, hParent, hkDlgProc, (LPARAM)&Dlg);
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
	DWORD nCtrlValue = SendDlgItemMessage(hDlg, iEditCtrl, HKM_GETHOTKEY, 0, 0);
	DWORD nHotKey = LOBYTE(nCtrlValue);
	DWORD nCtrlMods = HIBYTE(LOWORD(nCtrlValue));

	bool bList = false;
	const ListBoxItem* pItems = NULL;
	uint nKeyCount = CSetDlgLists::GetListItems(CSetDlgLists::eKeysHot, pItems);
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

	if (nCtrlValue != nHotKey)
	{
		SetHotkeyField(GetDlgItem(hDlg, iEditCtrl), nHotKey);
		if (nCtrlMods & HOTKEYF_ALT)
			nHotKey |= cvk_Alt;
		if (nCtrlMods & HOTKEYF_CONTROL)
			nHotKey |= cvk_Ctrl;
		if (nCtrlMods & HOTKEYF_SHIFT)
			nHotKey |= cvk_Shift;
	}

	return nHotKey;
}

void CHotKeyDialog::SetHotkeyField(HWND hHk, BYTE vk)
{
	SendMessage(hHk, HKM_SETHOTKEY,
				vk|((vk==VK_DELETE||vk==VK_UP||vk==VK_DOWN||vk==VK_LEFT||vk==VK_RIGHT
				||vk==VK_PRIOR||vk==VK_NEXT||vk==VK_HOME||vk==VK_END
				||vk==VK_CANCEL // VK_CANCEL is "Pause/Break" key when it's pressed with Ctrl
				||vk==VK_INSERT) ? (HOTKEYF_EXT<<8) : 0), 0);
}

void CHotKeyDialog::FillModifierBoxes(const ConEmuHotKey& HK, HWND hDlg)
{
	BYTE Mods[3];
	HK.Key.GetModifiers(Mods);
	for (int n = 0; n < 3; n++)
	{
		CSetDlgLists::FillListBoxItems(GetDlgItem(hDlg, lbHotKeyMod1+n), CSetDlgLists::eModifiers, Mods[n], false);
	}
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
			// OnOurDialogOpened is not called because this dialog is intended
			// to be opened as modal dialog with settings dialog as a parent

			SendMessage(hDlg, WM_SETICON, ICON_BIG, (LPARAM)hClassIcon);
			SendMessage(hDlg, WM_SETICON, ICON_SMALL, (LPARAM)hClassIconSm);

			CDynDialog::LocalizeDialog(hDlg, lng_DlgHotkey);

			if (pDlg->mp_DpiAware)
			{
				pDlg->mp_DpiAware->Attach(hDlg, ghWnd, CDynDialog::GetDlgClass(hDlg));
			}

			// Ensure, it will be "on screen"
			RECT rect; GetWindowRect(hDlg, &rect);
			RECT rcCenter = CenterInParent(rect, pDlg->mh_Parent);
			MoveWindow(hDlg, rcCenter.left, rcCenter.top,
			           rect.right - rect.left, rect.bottom - rect.top, false);

			HWND hHk = GetDlgItem(hDlg, hkHotKeySelect);
			SendMessage(hHk, HKM_SETRULES, HKCOMB_A|HKCOMB_C|HKCOMB_CA|HKCOMB_S|HKCOMB_SA|HKCOMB_SC|HKCOMB_SCA, 0);

			BYTE vk = pDlg->m_HK.Key.Vk;
			SetHotkeyField(hHk, vk);

			// Warning! Если nVK не указан в SettingsNS::nKeysHot - nVK будет обнулен
			CSetDlgLists::FillListBoxItems(GetDlgItem(hDlg, lbHotKeyList), CSetDlgLists::eKeysHot, vk, false);

			FillModifierBoxes(pDlg->m_HK, hDlg);

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
					DWORD nHotKey = dlgGetHotkey(hDlg, hkHotKeySelect, lbHotKeyList);
					DWORD nMods = (nHotKey & cvk_ALLMASK) ? (nHotKey & cvk_ALLMASK) : pDlg->m_HK.Key.Mod;
					pDlg->m_HK.Key.Set(LOBYTE(nHotKey), nMods);
					FillModifierBoxes(pDlg->m_HK, hDlg);
					break;
				} // EN_CHANGE

				case CBN_SELCHANGE:
				{
					switch (LOWORD(wParam))
					{
						case lbHotKeyList:
						{
							BYTE vk = 0;
							CSetDlgLists::GetListBoxItem(hDlg, lbHotKeyList, CSetDlgLists::eKeysHot, vk);

							SetHotkeyField(GetDlgItem(hDlg, hkHotKeySelect), vk);

							if (pDlg->m_HK.Key.Mod == cvk_NULL)
							{
								// Если модификатора вообще не было - ставим Win
								BYTE b = VK_LWIN;
								CSetDlgLists::FillListBoxItems(GetDlgItem(hDlg, lbHotKeyMod1), CSetDlgLists::eModifiers, b, false);
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
								CSetDlgLists::GetListBoxItem(hDlg, lbHotKeyMod1+i, CSetDlgLists::eModifiers, vk);
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
