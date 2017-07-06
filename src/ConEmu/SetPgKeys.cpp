
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


#define HIDE_USE_EXCEPTION_INFO
#define SHOWDEBUGSTR

#include "Header.h"

#include "ConEmu.h"
#include "LngRc.h"
#include "OptionsClass.h"
#include "HotkeyDlg.h"
#include "HotkeyList.h"
#include "SearchCtrl.h"
#include "SetDlgLists.h"
#include "SetPgKeys.h"

CSetPgKeys::CSetPgKeys()
	: mp_ActiveHotKey(NULL)
	, mn_LastShowType(hkfv_All)
	, mb_LastHideEmpties(false)
{
}

CSetPgKeys::~CSetPgKeys()
{
}

LRESULT CSetPgKeys::OnInitDialog(HWND hDlg, bool abInitial)
{
	if (!abInitial)
	{
		FillHotKeysList(hDlg, abInitial);
		return 0;
	}

	EditIconHint_Set(ghOpWnd, GetDlgItem(hDlg, tHotkeysFilter), false,
		CLngRc::getRsrc(lng_HotkeysFilter/*"Filter (Alt+F)"*/), false,
		UM_HOTKEY_FILTER, bSaveSettings);

	BYTE b = mn_LastShowType;
	CSetDlgLists::FillListBoxItems(GetDlgItem(hDlg, lbHotKeyFilter), CSetDlgLists::eKeysFilter, b, false);

	for (INT_PTR i = gpHotKeys->size() - 1; i >= 0; i--)
	{
		ConEmuHotKey* p = &((*gpHotKeys)[i]);
		p->cchGuiMacroMax = p->GuiMacro ? (wcslen(p->GuiMacro)+1) : 0;
	}

	HWND hList = GetDlgItem(hDlg, lbConEmuHotKeys);
	mp_ActiveHotKey = NULL;

	HWND hTip = ListView_GetToolTips(hList);
	SetWindowPos(hTip, HWND_TOPMOST, 0,0,0,0, SWP_NOMOVE|SWP_NOSIZE|SWP_NOACTIVATE);

	// Убедиться, что поле клавиши идет поверх выпадающего списка
	//HWND hHk = GetDlgItem(hDlg, hkHotKeySelect);
	SendDlgItemMessage(hDlg, hkHotKeySelect, HKM_SETRULES, HKCOMB_A|HKCOMB_C|HKCOMB_CA|HKCOMB_S|HKCOMB_SA|HKCOMB_SC|HKCOMB_SCA, 0);

	// Создать колонки
	{
		LVCOLUMN col = {
			LVCF_WIDTH|LVCF_TEXT|LVCF_FMT, LVCFMT_LEFT,
			gpSetCls->EvalSize(60, esf_Horizontal|esf_CanUseDpi)};
		wchar_t szTitle[64]; col.pszText = szTitle;

		ListView_SetExtendedListViewStyleEx(hList,LVS_EX_FULLROWSELECT,LVS_EX_FULLROWSELECT);
		ListView_SetExtendedListViewStyleEx(hList,LVS_EX_LABELTIP|LVS_EX_INFOTIP,LVS_EX_LABELTIP|LVS_EX_INFOTIP);

		wcscpy_c(szTitle, L"Type");			ListView_InsertColumn(hList, klc_Type, &col);
		col.cx = gpSetCls->EvalSize(120, esf_Horizontal|esf_CanUseDpi);
		wcscpy_c(szTitle, L"Hotkey");		ListView_InsertColumn(hList, klc_Hotkey, &col);
		col.cx = gpSetCls->EvalSize(300, esf_Horizontal|esf_CanUseDpi);
		wcscpy_c(szTitle, L"Description");	ListView_InsertColumn(hList, klc_Desc, &col);
	}

	FillHotKeysList(hDlg, abInitial);

	for (int i = 0; i < 3; i++)
	{
		BYTE b = 0;
		CSetDlgLists::FillListBoxItems(GetDlgItem(hDlg, lbHotKeyMod1+i), CSetDlgLists::eModifiers, b, false);
	}
	//FillListBoxByte(hDlg, lbHotKeyMod1, SettingsNS::Modifiers, b);
	//FillListBoxByte(hDlg, lbHotKeyMod2, SettingsNS::Modifiers, b);
	//FillListBoxByte(hDlg, lbHotKeyMod3, SettingsNS::Modifiers, b);

	return 0;
}

void CSetPgKeys::FillHotKeysList(HWND hDlg, bool abInitial)
{
	HWND hList = GetDlgItem(hDlg, lbConEmuHotKeys);
	SendMessage(hList, WM_SETREDRAW, FALSE, 0);

	BYTE b = 0;
	CSetDlgLists::GetListBoxItem(hDlg, lbHotKeyFilter, CSetDlgLists::eKeysFilter, b);
	KeysFilterValues nShowType = (KeysFilterValues)b;

	bool bHideEmpties = (isChecked(hDlg, cbHotkeysAssignedOnly) == BST_CHECKED);

	CEStr lsFilter;
	GetString(hDlg, tHotkeysFilter, &lsFilter.ms_Val);

	// Населить список всеми хоткеями
	if (abInitial
		|| (mn_LastShowType != nShowType)
		|| (mb_LastHideEmpties != bHideEmpties)
		)
	{
		ListView_DeleteAllItems(hList);
		abInitial = true;
	}
	mn_LastShowType = nShowType;
	mb_LastHideEmpties = bHideEmpties;
	ms_LastFilter.Set(lsFilter);


	wchar_t szName[128], szDescr[512];
	//HWND hDetails = GetDlgItem(hDlg, lbActivityDetails);
	LVITEM lvi = {LVIF_TEXT|LVIF_STATE|LVIF_PARAM};
	lvi.state = 0;
	lvi.stateMask = LVIS_SELECTED|LVIS_FOCUSED;
	lvi.pszText = szName;
	const ConEmuHotKey *ppHK = NULL;
	int ItemsCount = (int)ListView_GetItemCount(hList);
	int nItem = -1; // если -1 то будет добавлен новый

	// Если выбран режим "Показать только макросы"
	// то сначала отобразить пользовательские "Macro 00"
	// а после них все системные, которые используют Macro (для справки)
	size_t stepMax = (nShowType == hkfv_Macros) ? 1 : 0;
	for (size_t step = 0; step <= stepMax; step++)
	{
		int iHkIdx = 0;

		for (;;)
		{
			if (abInitial)
			{
				ppHK = gpHotKeys->GetHotKeyPtr(iHkIdx++);
				if (!ppHK)
					break; // кончились
				nItem = -1; // если -1 то будет добавлен новый

				switch (nShowType)
				{
				case hkfv_User:
					if ((ppHK->HkType != chk_User) && (ppHK->HkType != chk_Global) && (ppHK->HkType != chk_Local)
						&& (ppHK->HkType != chk_Modifier) && (ppHK->HkType != chk_Modifier2) && (ppHK->HkType != chk_Task))
						continue;
					break;
				case hkfv_Macros:
					if (((step == 0) && (ppHK->HkType != chk_Macro))
						|| ((step > 0) && ((ppHK->HkType == chk_Macro) || !ppHK->GuiMacro)))
						continue;
					break;
				case hkfv_System:
					if ((ppHK->HkType != chk_System) && (ppHK->HkType != chk_ArrHost) && (ppHK->HkType != chk_NumHost))
						continue;
					break;
				default:
					; // OK
				}

				if (bHideEmpties)
				{
					if (ppHK->Key.IsEmpty())
						continue;
				}

			}
			else
			{
				nItem++; // на старте было "-1"
				if (nItem >= ItemsCount)
					break; // кончились
				LVITEM lvf = {LVIF_PARAM, nItem};
				if (!ListView_GetItem(hList, &lvf))
				{
					_ASSERTE(ListView_GetItem(hList, &lvf));
					break;
				}
				ppHK = (const ConEmuHotKey*)lvf.lParam;
				if (!ppHK || !ppHK->DescrLangID)
				{
					_ASSERTE(ppHK && ppHK->DescrLangID);
					break;
				}
			}

			if (!lsFilter.IsEmpty())
			{
				bool bMatch = false;
				ppHK->GetDescription(szDescr, countof(szDescr));
				ppHK->GetHotkeyName(szName);
				if (StrStrI(szDescr, lsFilter))
					bMatch = true;
				else if (StrStrI(szName, lsFilter))
					bMatch = true;
				// Match?
				if (!bMatch)
				{
					if (!abInitial)
					{
						ListView_DeleteItem(hList, nItem);
						nItem--; ItemsCount--;
					}
					continue;
				}
			}

			switch (ppHK->HkType)
			{
			case chk_Global:
				wcscpy_c(szName, L"Global"); break;
			case chk_Local:
				wcscpy_c(szName, L"Local"); break;
			case chk_User:
				wcscpy_c(szName, L"User"); break;
			case chk_Macro:
				_wsprintf(szName, SKIPLEN(countof(szName)) L"Macro %02i", ppHK->DescrLangID-vkGuiMacro01+1); break;
			case chk_Modifier:
			case chk_Modifier2:
				wcscpy_c(szName, L"Modifier"); break;
			case chk_NumHost:
			case chk_ArrHost:
				wcscpy_c(szName, L"System"); break;
			case chk_System:
				wcscpy_c(szName, L"System"); break;
			case chk_Task:
				wcscpy_c(szName, L"Task"); break;
			default:
				// Неизвестный тип!
				_ASSERTE(FALSE && "Unknown ppHK->HkType");
				//wcscpy_c(szName, L"???");
				continue;
			}

			if (nItem == -1)
			{
				lvi.iItem = ItemsCount + 1; // в конец
				lvi.lParam = (LPARAM)ppHK;
				nItem = ListView_InsertItem(hList, &lvi);
				//_ASSERTE(nItem==ItemsCount && nItem>=0);
				ItemsCount++;
			}
			if (abInitial)
			{
				ListView_SetItemState(hList, nItem, 0, LVIS_SELECTED|LVIS_FOCUSED);
			}

			ppHK->GetHotkeyName(szName);

			ListView_SetItemText(hList, nItem, klc_Hotkey, szName);

			if (ppHK->HkType == chk_Macro)
			{
				//wchar_t* pszBuf = EscapeString(true, ppHK->GuiMacro);
				//LPCWSTR pszMacro = pszBuf;
				LPCWSTR pszMacro = ppHK->GuiMacro;
				if (!pszMacro || !*pszMacro)
					pszMacro = L"<Not set>";
				ListView_SetItemText(hList, nItem, klc_Desc, (wchar_t*)pszMacro);
				//SafeFree(pszBuf);
			}
			else
			{
				ppHK->GetDescription(szDescr, countof(szDescr));
				ListView_SetItemText(hList, nItem, klc_Desc, szDescr);
			}
		}
	}

	SendMessage(hList, WM_SETREDRAW, TRUE, 0);

	//ListView_SetSelectionMark(hList, -1);
	gpSet->CheckHotkeyUnique();
}

LRESULT CSetPgKeys::OnHotkeysNotify(HWND hDlg, WPARAM wParam, LPARAM lParam)
{
	static bool bChangePosted = false;

	if (!lParam)
	{
		_ASSERTE(HIWORD(wParam) == 0xFFFF && LOWORD(wParam) == lbConEmuHotKeys);
		bChangePosted = false;

		HWND hHk = GetDlgItem(hDlg, hkHotKeySelect);
		BOOL bHotKeyEnabled = FALSE, bKeyListEnabled = FALSE, bModifiersEnabled = FALSE, bMacroEnabled = FALSE;
		LPCWSTR pszLabel = L"Choose hotkey:";
		LPCWSTR pszDescription = L"";
		wchar_t szDescTemp[512];
		DWORD VkMod = 0;

		int iItem = (int)SendDlgItemMessage(hDlg, lbConEmuHotKeys, LVM_GETNEXTITEM, -1, LVNI_SELECTED);

		if (iItem >= 0)
		{
			HWND hList = GetDlgItem(hDlg, lbConEmuHotKeys);
			LVITEM lvi = {LVIF_PARAM, iItem};
			ConEmuHotKey* pk = NULL;
			if (ListView_GetItem(hList, &lvi))
				pk = (ConEmuHotKey*)lvi.lParam;
			if (pk && !(pk->DescrLangID /*&& (pk->VkMod || pk->HkType == chk_Macro)*/))
			{
				//_ASSERTE(pk->DescrLangID && (pk->VkMod || pk->HkType == chk_Macro));
				_ASSERTE(pk->DescrLangID);
				pk = NULL;
			}
			mp_ActiveHotKey = pk;

			if (pk)
			{
				//SetDlgItemText(hDlg, stHotKeySelect, (pk->Type == 0) ? L"Choose hotkey:" : (pk->Type == 1) ?  L"Choose modifier:" : L"Choose modifiers:");
				switch (pk->HkType)
				{
				case chk_User:
				case chk_Global:
				case chk_Local:
				case chk_Task:
					pszLabel = L"Choose hotkey:";
					VkMod = pk->GetVkMod();
					bHotKeyEnabled = bKeyListEnabled = bModifiersEnabled = TRUE;
					break;
				case chk_Macro:
					pszLabel = L"Choose hotkey:";
					VkMod = pk->GetVkMod();
					bHotKeyEnabled = bKeyListEnabled = bModifiersEnabled = bMacroEnabled = TRUE;
					break;
				case chk_Modifier:
					pszLabel = L"Choose modifier:";
					VkMod = pk->GetVkMod();
					bKeyListEnabled = TRUE;
					break;
				case chk_Modifier2:
					pszLabel = L"Choose modifier:";
					VkMod = pk->GetVkMod();
					bModifiersEnabled = TRUE;
					break;
				case chk_NumHost:
				case chk_ArrHost:
					pszLabel = L"Choose modifiers:";
					VkMod = pk->GetVkMod();
					_ASSERTE(VkMod);
					bModifiersEnabled = TRUE;
					break;
				case chk_System:
					pszLabel = L"Predefined:";
					VkMod = pk->GetVkMod();
					bHotKeyEnabled = bKeyListEnabled = bModifiersEnabled = 2;
					break;
				default:
					_ASSERTE(0 && "Unknown HkType");
					pszLabel = L"Unknown:";
				}
				//SetDlgItemText(hDlg, stHotKeySelect, psz);

				//EnableWindow(GetDlgItem(hDlg, stHotKeySelect), TRUE);
				//EnableWindow(GetDlgItem(hDlg, lbHotKeyList), TRUE);
				//EnableWindow(hHk, (pk->Type == 0));

				//if (bMacroEnabled)
				//{
				//	pszDescription = pk->GuiMacro;
				//}
				//else
				//{
				//	if (!CLngRc::getHint(pk->DescrLangID, szDescTemp, countof(szDescTemp)))
				//		szDescTemp[0] = 0;
				//	pszDescription = szDescTemp;
				//}
				// -- use function
				pszDescription = pk->GetDescription(szDescTemp, countof(szDescTemp));

				//nVK = pk ? *pk->VkPtr : 0;
				//if ((pk->Type == 0) || (pk->Type == 2))
				BYTE vk = ConEmuHotKey::GetHotkey(VkMod);
				if (bHotKeyEnabled)
				{
					CHotKeyDialog::SetHotkeyField(hHk, vk);
					//SendMessage(hHk, HKM_SETHOTKEY,
					//	vk|((vk==VK_DELETE||vk==VK_UP||vk==VK_DOWN||vk==VK_LEFT||vk==VK_RIGHT
					//	||vk==VK_PRIOR||vk==VK_NEXT||vk==VK_HOME||vk==VK_END
					//	||vk==VK_INSERT) ? (HOTKEYF_EXT<<8) : 0), 0);

					// Warning! Если nVK не указан в SettingsNS::nKeysHot - nVK будет обнулен
					CSetDlgLists::FillListBoxItems(GetDlgItem(hDlg, lbHotKeyList), CSetDlgLists::eKeysHot, vk, false);
				}
				else if (bKeyListEnabled)
				{
					CHotKeyDialog::SetHotkeyField(hHk, 0);
					CSetDlgLists::FillListBoxItems(GetDlgItem(hDlg, lbHotKeyList), CSetDlgLists::eKeys, vk, false);
				}
			}
		}
		else
		{
			mp_ActiveHotKey = NULL;
		}

		//if (!mp_ActiveHotKey)
		//{
		SetDlgItemText(hDlg, stHotKeySelect, pszLabel);
		EnableWindow(GetDlgItem(hDlg, stHotKeySelect), (bHotKeyEnabled || bKeyListEnabled || bModifiersEnabled));
		EnableWindow(GetDlgItem(hDlg, lbHotKeyList), (bKeyListEnabled==TRUE));
		EnableWindow(hHk, (bHotKeyEnabled==TRUE));
		EnableWindow(GetDlgItem(hDlg, stGuiMacro), (bMacroEnabled==TRUE));
		SetDlgItemText(hDlg, stGuiMacro, bMacroEnabled ? L"GUI Macro:" : L"Description:");
		HWND hMacro = GetDlgItem(hDlg, tGuiMacro);
		EnableWindow(hMacro, (mp_ActiveHotKey!=NULL));
		SendMessage(hMacro, EM_SETREADONLY, !bMacroEnabled, 0);
		MySetDlgItemText(hDlg, tGuiMacro, pszDescription/*, bMacroEnabled*/);
		EnableWindow(GetDlgItem(hDlg, cbGuiMacroHelp), (mp_ActiveHotKey!=NULL) && (bMacroEnabled || mp_ActiveHotKey->GuiMacro));
		if (!bHotKeyEnabled)
			CHotKeyDialog::SetHotkeyField(hHk, 0);
			//SendMessage(hHk, HKM_SETHOTKEY, 0, 0);
		if (!bKeyListEnabled)
			SendDlgItemMessage(hDlg, lbHotKeyList, CB_RESETCONTENT, 0, 0);
		// Modifiers
		BOOL bEnabled = (mp_ActiveHotKey && (bModifiersEnabled==TRUE));
		BOOL bShow = (mp_ActiveHotKey && (mp_ActiveHotKey->HkType != chk_Modifier));
		for (int n = 0; n < 3; n++)
		{
			BYTE b = (bShow && VkMod) ? ConEmuHotKey::GetModifier(VkMod,n+1) : 0;
			//FillListBoxByte(hDlg, lbHotKeyMod1+n, SettingsNS::Modifiers, b);
			CSetDlgLists::FillListBoxItems(GetDlgItem(hDlg, lbHotKeyMod1+n), CSetDlgLists::eModifiers, b, false);
			EnableWindow(GetDlgItem(hDlg, lbHotKeyMod1+n), bEnabled);
		}
		//for (size_t n = 0; n < countof(HostkeyCtrlIds); n++)
		//{
		//	BOOL bEnabled = (mp_ActiveHotKey && bModifiersEnabled);
		//	BOOL bChecked = bEnabled ? gpSet->HasModifier(VkMod, HostkeyVkIds[n]) : false;
		//	checkDlgButton(hDlg, HostkeyCtrlIds[n], bChecked);
		//	EnableWindow(GetDlgItem(hDlg, HostkeyCtrlIds[n]), bEnabled);
		//}
		//}
	}
	else switch (((NMHDR*)lParam)->code)
	{
	case LVN_ITEMCHANGED:
		{
			#ifdef _DEBUG
			LPNMLISTVIEW p = (LPNMLISTVIEW)lParam; UNREFERENCED_PARAMETER(p);
			#endif

			if (!bChangePosted)
			{
				bChangePosted = true;
				PostMessage(hDlg, WM_COMMAND, MAKELONG(lbConEmuHotKeys,0xFFFF), 0);
			}
		} //LVN_ITEMCHANGED
		break;

	case LVN_COLUMNCLICK:
		{
			LPNMLISTVIEW pnmv = (LPNMLISTVIEW)lParam;
			ListView_SortItems(GetDlgItem(hDlg, lbConEmuHotKeys), HotkeysCompare, pnmv->iSubItem);
		} // LVN_COLUMNCLICK
		break;
	}
	return 0;
}

int CSetPgKeys::HotkeysCompare(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort)
{
	int nCmp = 0;
	ConEmuHotKey* pHk1 = (ConEmuHotKey*)lParam1;
	ConEmuHotKey* pHk2 = (ConEmuHotKey*)lParam2;

	if (pHk1 && pHk1->DescrLangID && pHk2 && pHk2->DescrLangID)
	{
		switch (lParamSort)
		{
		case 0:
			// Type
			nCmp =
				(pHk1->HkType < pHk2->HkType) ? -1 :
				(pHk1->HkType > pHk2->HkType) ? 1 :
				(pHk1 < pHk2) ? -1 :
				(pHk1 > pHk2) ? 1 :
				0;
			break;

		case 1:
			// Hotkey
			{
				wchar_t szFull1[128], szFull2[128]; ;
				nCmp = lstrcmp(pHk1->GetHotkeyName(szFull1), pHk2->GetHotkeyName(szFull2));
				if (nCmp == 0)
					nCmp = (pHk1 < pHk2) ? -1 : (pHk1 > pHk2) ? 1 : 0;
			}
			break;

		case 2:
			// Description
			{
				LPCWSTR pszDescr1, pszDescr2;
				wchar_t szBuf1[512], szBuf2[512];

				if (pHk1->HkType == chk_Macro)
					pszDescr1 = (pHk1->GuiMacro && *pHk1->GuiMacro) ? pHk1->GuiMacro : L"<Not set>";
				else
					pszDescr1 = pHk1->GetDescription(szBuf1, countof(szBuf1));

				if (pHk2->HkType == chk_Macro)
					pszDescr2 = (pHk2->GuiMacro && *pHk2->GuiMacro) ? pHk2->GuiMacro : L"<Not set>";
				else
					pszDescr2 = pHk2->GetDescription(szBuf2, countof(szBuf2));

				nCmp = lstrcmpi(pszDescr1, pszDescr2);
				if (nCmp == 0)
					nCmp = (pHk1 < pHk2) ? -1 : (pHk1 > pHk2) ? 1 : 0;
			}
			break;
		default:
			nCmp = (pHk1 < pHk2) ? -1 : (pHk1 > pHk2) ? 1 : 0;
		}
	}

	return nCmp;
}

void CSetPgKeys::RefilterHotkeys(bool bReset /*= true*/)
{
	FillHotKeysList(mh_Dlg, bReset);
	OnHotkeysNotify(mh_Dlg, MAKELONG(lbConEmuHotKeys,0xFFFF), 0);
}

INT_PTR CSetPgKeys::PageDlgProc(HWND hDlg, UINT messg, WPARAM wParam, LPARAM lParam)
{
	switch (messg)
	{
	case UM_HOTKEY_FILTER:
		{
			if (IsWindowVisible(mh_Dlg))
			{
				CEStr lsStr;
				GetString(mh_Dlg, tHotkeysFilter, &lsStr.ms_Val);
				bool bReset = true;
				if (!lsStr.IsEmpty()
					&& (lsStr.GetLen() > ms_LastFilter.GetLen())
					&& (ms_LastFilter.IsEmpty() || StrStrI(lsStr, ms_LastFilter))
					)
				{
					// We may just delete some already populated hotkeys,
					// because new filter is stronger and including previous one
					bReset = false;
				}

				RefilterHotkeys(bReset);
			}
		}
		break;
	}

	return 0;
}

void CSetPgKeys::ReInitHotkeys()
{
	gpHotKeys->ReleaseHotkeys();

	// Горячие клавиши (умолчания)
	gpHotKeys->AllocateHotkeys();

	CSetPgKeys* pPage;
	if (gpSetCls->GetPageObj(pPage))
		pPage->mp_ActiveHotKey = NULL;
}

bool CSetPgKeys::QueryDialogCancel()
{
	HWND hCtrl = GetFocus();
	if (hCtrl)
	{
		switch (GetDlgCtrlID(hCtrl))
		{
		case tHotkeysFilter:
			if (GetWindowTextLength(hCtrl) > 0)
				SetWindowText(hCtrl, L"");
			else
				SetFocus(GetDlgItem(mh_Dlg, lbConEmuHotKeys));
			break;
		}
	}

	return true; // Allow dialog close
}

LRESULT CSetPgKeys::OnEditChanged(HWND hDlg, WORD nCtrlId)
{
	switch (nCtrlId)
	{
	case hkHotKeySelect:
		{
			DWORD hHotKeyMods = CHotKeyDialog::dlgGetHotkey(hDlg, hkHotKeySelect, lbHotKeyList);
			DWORD nHotKey = LOBYTE(hHotKeyMods);

			if (mp_ActiveHotKey && mp_ActiveHotKey->CanChangeVK())
			{
				if (!(hHotKeyMods & cvk_ALLMASK))
				{
					DWORD nCurMods = (CEHOTKEY_MODMASK & mp_ActiveHotKey->GetVkMod());
					if (!nCurMods)
						nCurMods = CEHOTKEY_NOMOD;

					SetHotkeyVkMod(mp_ActiveHotKey, nHotKey | nCurMods);
				}
				else
				{
					mp_ActiveHotKey->Key.Set(LOBYTE(hHotKeyMods), (hHotKeyMods & cvk_ALLMASK));
				}

				if (hHotKeyMods & CEHOTKEY_MODMASK)
				{
					CHotKeyDialog::FillModifierBoxes(*mp_ActiveHotKey, hDlg);
				}

				FillHotKeysList(hDlg, false);
			}
			break;
		} // case hkHotKeySelect:

	case tGuiMacro:
		if (mp_ActiveHotKey && (mp_ActiveHotKey->HkType == chk_Macro))
		{
			MyGetDlgItemText(hDlg, tGuiMacro, mp_ActiveHotKey->cchGuiMacroMax, mp_ActiveHotKey->GuiMacro);
			FillHotKeysList(hDlg, false);
		}
		break;

	case tHotkeysFilter:
		{
			EditIconHint_SetTimer(GetDlgItem(hDlg, nCtrlId));
		}
		break;

	}
	return 0;
}

INT_PTR CSetPgKeys::OnButtonClicked(HWND hDlg, HWND hBtn, WORD nCtrlId)
{
	switch (nCtrlId)
	{
	case cbHotkeysAssignedOnly:
		{
			RefilterHotkeys();
			return 0;
		} // lbHotKeyFilter

	default:
		return CSetPgBase::OnButtonClicked(hDlg, hBtn, nCtrlId);
	}
}

INT_PTR CSetPgKeys::OnComboBox(HWND hDlg, WORD nCtrlId, WORD code)
{
	switch (nCtrlId)
	{
	case lbHotKeyFilter:
		{
			BYTE f = 0;
			CSetDlgLists::GetListBoxItem(hDlg, lbHotKeyFilter, CSetDlgLists::eKeysFilter, f);
			mn_LastShowType = (KeysFilterValues)f;
			RefilterHotkeys();
			break;
		} // lbHotKeyFilter

	case lbHotKeyList:
		{
			if (!mp_ActiveHotKey)
				break;

			BYTE vk = 0;

			//if (mp_ActiveHotKey && (mp_ActiveHotKey->Type == 0))
			//{
			//}
			//else
			//{
			//}

			//if (mp_ActiveHotKey && (mp_ActiveHotKey->Type == 0 || mp_ActiveHotKey->Type == 1) && mp_ActiveHotKey->VkPtr)
			if (mp_ActiveHotKey)
			{
				if (mp_ActiveHotKey->CanChangeVK())
				{
					//GetListBoxByte(hDlg, wId, SettingsNS::KeysHot, vk);
					CSetDlgLists::GetListBoxItem(hDlg, nCtrlId, CSetDlgLists::eKeysHot, vk);

					CHotKeyDialog::SetHotkeyField(GetDlgItem(hDlg, hkHotKeySelect), vk);
					//SendDlgItemMessage(hDlg, hkHotKeySelect, HKM_SETHOTKEY, vk|(vk==VK_DELETE ? (HOTKEYF_EXT<<8) : 0), 0);

					DWORD nMod = (CEHOTKEY_MODMASK & mp_ActiveHotKey->GetVkMod());
					if (nMod == 0)
					{
						// Если модификатора вообще не было - ставим Win
						BYTE b = VK_LWIN;
						CSetDlgLists::FillListBoxItems(GetDlgItem(hDlg, lbHotKeyMod1), CSetDlgLists::eModifiers, b, false);
						nMod = (VK_LWIN << 8);
					}
					SetHotkeyVkMod(mp_ActiveHotKey, ((DWORD)vk) | nMod);
				}
				else if (mp_ActiveHotKey->HkType == chk_Modifier)
				{
					CSetDlgLists::GetListBoxItem(hDlg, nCtrlId, CSetDlgLists::eKeys, vk);
					CHotKeyDialog::SetHotkeyField(GetDlgItem(hDlg, hkHotKeySelect), 0);
					SetHotkeyVkMod(mp_ActiveHotKey, vk);
				}
				FillHotKeysList(hDlg, false);
			}

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
				BYTE vkChange = vk;

				// Некоторые модификаторы НЕ допустимы при регистрации глобальных хоткеев (ограничения WinAPI)
				if (mp_ActiveHotKey && (mp_ActiveHotKey->HkType == chk_Global || mp_ActiveHotKey->HkType == chk_Local))
				{
					switch (vk)
					{
					case VK_APPS:
						vkChange = 0; break;
					case VK_LMENU: case VK_RMENU:
						vkChange = VK_MENU; break;
					case VK_LCONTROL: case VK_RCONTROL:
						vkChange = VK_CONTROL; break;
					case VK_LSHIFT: case VK_RSHIFT:
						vkChange = VK_SHIFT; break;
					}
				}

				if (vkChange != vk)
				{
					vk = vkChange;
					CSetDlgLists::FillListBoxItems(GetDlgItem(hDlg, lbHotKeyMod1+i), CSetDlgLists::eModifiers, vkChange, false);
				}

				if (vk)
					nModifers = ConEmuHotKey::SetModifier(nModifers, vk, false);
			}

			_ASSERTE((nModifers & 0xFF) == 0); // Модификаторы должны быть строго в старших 3-х байтах

			if (mp_ActiveHotKey && (mp_ActiveHotKey->HkType != chk_Modifier) && (mp_ActiveHotKey->HkType != chk_System))
			{
				//if (mp_ActiveHotKey->VkModPtr)
				//{
				//	*mp_ActiveHotKey->VkModPtr = (cvk_VK_MASK & *mp_ActiveHotKey->VkModPtr) | nModifers;
				//}
				//else
				if (mp_ActiveHotKey->HkType == chk_NumHost)
				{
					if (!nModifers)
						nModifers = (VK_LWIN << 8);
					// Для этой группы - модификаторы назначаются "чохом"
					_ASSERTE((nModifers & 0xFF) == 0); // тут данные в старших трех байтах
					gpSet->nHostkeyNumberModifier = (nModifers >> 8); // а тут в младших трех
					gpHotKeys->UpdateNumberModifier();
				}
				else if (mp_ActiveHotKey->HkType == chk_ArrHost)
				{
					if (!nModifers)
						nModifers = (VK_LWIN << 8);
					// Для этой группы - модификаторы назначаются "чохом"
					_ASSERTE((nModifers & 0xFF) == 0); // тут данные в старших трех байтах
					gpSet->nHostkeyArrowModifier = (nModifers >> 8); // а тут в младших трех
					gpHotKeys->UpdateArrowModifier();
				}
				else //if (mp_ActiveHotKey->VkMod)
				{
					if (!nModifers)
						nModifers = CEHOTKEY_NOMOD;
					SetHotkeyVkMod(mp_ActiveHotKey, (cvk_VK_MASK & mp_ActiveHotKey->GetVkMod()) | nModifers);
				}
			}

			//if (mp_ActiveHotKey->HkType == chk_Hostkey)
			//{
			//	gpSet->nHostkeyModifier = (nModifers >> 8); // а тут они хранятся в младших (так сложилось)
			//}
			//else if (mp_ActiveHotKey->HkType == chk_Hostkey)
			//{
			//	gpSet->nHostkeyModifier = (nModifers >> 8); // а тут они хранятся в младших (так сложилось)
			//}

			FillHotKeysList(hDlg, false);

			break;
		} // lbHotKeyMod1, lbHotKeyMod2, lbHotKeyMod3
	}
	return 0;
}

// Called when user change hotkey or modifiers in Settings dialog
void CSetPgKeys::SetHotkeyVkMod(ConEmuHotKey *pHK, DWORD VkMod)
{
	if (!pHK)
	{
		_ASSERTE(pHK!=NULL);
		return;
	}

	// Usually, this is equal to "mp_ActiveHotKey->VkMod = VkMod"
	pHK->SetVkMod(VkMod);

	// Global? Need to re-register?
	if (pHK->HkType == chk_Local)
	{
		gpConEmu->GlobalHotKeyChanged();
	}
}
