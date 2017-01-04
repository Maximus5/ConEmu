
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
#include "../common/MSetter.h"

#include "SetPgTasks.h"

CSetPgTasks::CSetPgTasks()
	: mb_IgnoreCmdGroupEdit(false)
	, mb_IgnoreCmdGroupList(false)
{
}

CSetPgTasks::~CSetPgTasks()
{
}

LRESULT CSetPgTasks::OnInitDialog(HWND hDlg, bool abForceReload)
{
	mb_IgnoreCmdGroupEdit = true;

	wchar_t szKey[128] = L"";
	const ConEmuHotKey* pDefCmdKey = NULL;
	if (!gpSet->GetHotkeyById(vkMultiCmd, &pDefCmdKey) || !pDefCmdKey)
		wcscpy_c(szKey, gsNoHotkey);
	else
		pDefCmdKey->GetHotkeyName(szKey, true);
	CEStr lsLabel(L"Default shell (", szKey, L")");
	SetDlgItemText(hDlg, cbCmdGrpDefaultCmd, lsLabel);

	// Not implemented yet
	ShowWindow(GetDlgItem(hDlg, cbCmdGrpToolbar), SW_HIDE);

	if (abForceReload)
	{
		int nTab = 4*4; // represent the number of quarters of the average character width for the font
		SendDlgItemMessage(hDlg, lbCmdTasks, LB_SETTABSTOPS, 1, (LPARAM)&nTab);

		LONG_PTR nStyles = GetWindowLongPtr(GetDlgItem(hDlg, lbCmdTasks), GWL_STYLE);
		if (!(nStyles & LBS_NOTIFY))
			SetWindowLongPtr(GetDlgItem(hDlg, lbCmdTasks), GWL_STYLE, nStyles|LBS_NOTIFY);
	}

	// Сброс ранее загруженного списка (ListBox: lbCmdTasks)
	SendDlgItemMessage(hDlg, lbCmdTasks, LB_RESETCONTENT, 0,0);

	//if (abForceReload)
	//{
	//	// Обновить группы команд
	//	gpSet->LoadCmdTasks(NULL, true);
	//}

	int nGroup = 0;
	wchar_t szItem[1024];
	const CommandTasks* pGrp = NULL;
	while ((pGrp = gpSet->CmdTaskGet(nGroup)))
	{
		_wsprintf(szItem, SKIPLEN(countof(szItem)) L"%i\t", nGroup+1);
		int nPrefix = lstrlen(szItem);
		lstrcpyn(szItem+nPrefix, pGrp->pszName, countof(szItem)-nPrefix);

		INT_PTR iIndex = SendDlgItemMessage(hDlg, lbCmdTasks, LB_ADDSTRING, 0, (LPARAM)szItem);
		UNREFERENCED_PARAMETER(iIndex);
		//SendDlgItemMessage(hDlg, lbCmdTasks, LB_SETITEMDATA, iIndex, (LPARAM)pGrp);

		nGroup++;
	}

	OnComboBox(hDlg, lbCmdTasks, LBN_SELCHANGE);

	mb_IgnoreCmdGroupEdit = false;

	return 0;
}

LRESULT CSetPgTasks::OnEditChanged(HWND hDlg, WORD nCtrlId)
{
	switch (nCtrlId)
	{
	case tCmdGroupName:
	case tCmdGroupGuiArg:
	case tCmdGroupCommands:
		{
			if (mb_IgnoreCmdGroupEdit)
				break;

			int iCur = GetListboxCurSel(hDlg, lbCmdTasks, true);
			if (iCur < 0)
				break;

			HWND hName = GetDlgItem(hDlg, tCmdGroupName);
			HWND hGuiArg = GetDlgItem(hDlg, tCmdGroupGuiArg);
			HWND hCmds = GetDlgItem(hDlg, tCmdGroupCommands);
			size_t nNameLen = GetWindowTextLength(hName);
			wchar_t *pszName = (wchar_t*)calloc(nNameLen+1,sizeof(wchar_t));
			size_t nGuiLen = GetWindowTextLength(hGuiArg);
			wchar_t *pszGuiArgs = (wchar_t*)calloc(nGuiLen+1,sizeof(wchar_t));
			size_t nCmdsLen = GetWindowTextLength(hCmds);
			wchar_t *pszCmds = (wchar_t*)calloc(nCmdsLen+1,sizeof(wchar_t));

			if (pszName && pszCmds)
			{
				GetWindowText(hName, pszName, (int)(nNameLen+1));
				GetWindowText(hGuiArg, pszGuiArgs, (int)(nGuiLen+1));
				GetWindowText(hCmds, pszCmds, (int)(nCmdsLen+1));

				gpSet->CmdTaskSet(iCur, pszName, pszGuiArgs, pszCmds);

				mb_IgnoreCmdGroupList = true;
				OnInitDialog(hDlg, false);
				ListBoxMultiSel(hDlg, lbCmdTasks, iCur);
				mb_IgnoreCmdGroupList = false;
			}

			SafeFree(pszName);
			SafeFree(pszCmds);
			SafeFree(pszGuiArgs);
		}
		break;

	case tCmdGroupKey:
		break; // read-only

	#ifdef _DEBUG
	default:
		_ASSERTE(FALSE && "EditBox was not processed by CSetPgTasks::OnEditChanged");
	#endif
	}
	return 0;
}

INT_PTR CSetPgTasks::OnComboBox(HWND hWnd2, WORD nCtrlId, WORD code)
{
	// Other codes are not interesting yet
	if (code != CBN_SELCHANGE)
	{
		return 0;
	}

	switch (nCtrlId)
	{
	case lbCmdTasks:
		{
			if (mb_IgnoreCmdGroupList)
				break;
			MSetter lIgnoreEdit(&mb_IgnoreCmdGroupEdit);

			const CommandTasks* pCmd = NULL;
			int* Items = NULL;
			int iSelCount = GetListboxSelection(hWnd2, lbCmdTasks, Items);
			int iCur = (iSelCount == 1) ? Items[0] : -1;
			if (iCur >= 0)
				pCmd = gpSet->CmdTaskGet(iCur);
			bool lbEnable = false;
			if (pCmd)
			{
				_ASSERTE(pCmd->pszName);
				wchar_t* pszNoBrk = lstrdup(!pCmd->pszName ? L"" : (pCmd->pszName[0] != TaskBracketLeft) ? pCmd->pszName : (pCmd->pszName+1));
				if (*pszNoBrk && (pszNoBrk[_tcslen(pszNoBrk)-1] == TaskBracketRight))
					pszNoBrk[_tcslen(pszNoBrk)-1] = 0;
				SetDlgItemText(hWnd2, tCmdGroupName, pszNoBrk);
				SafeFree(pszNoBrk);

				wchar_t szKey[128] = L"";
				SetDlgItemText(hWnd2, tCmdGroupKey, pCmd->HotKey.GetHotkeyName(szKey));

				SetDlgItemText(hWnd2, tCmdGroupGuiArg, pCmd->pszGuiArgs ? pCmd->pszGuiArgs : L"");
				SetDlgItemText(hWnd2, tCmdGroupCommands, pCmd->pszCommands ? pCmd->pszCommands : L"");

				checkDlgButton(hWnd2, cbCmdGrpDefaultNew, (pCmd->Flags & CETF_NEW_DEFAULT) ? BST_CHECKED : BST_UNCHECKED);
				checkDlgButton(hWnd2, cbCmdGrpDefaultCmd, (pCmd->Flags & CETF_CMD_DEFAULT) ? BST_CHECKED : BST_UNCHECKED);
				checkDlgButton(hWnd2, cbCmdGrpTaskbar, ((pCmd->Flags & CETF_NO_TASKBAR) == CETF_NONE) ? BST_CHECKED : BST_UNCHECKED);
				checkDlgButton(hWnd2, cbCmdGrpToolbar, (pCmd->Flags & CETF_ADD_TOOLBAR) ? BST_CHECKED : BST_UNCHECKED);

				lbEnable = true;
			}
			else
			{
				SetDlgItemText(hWnd2, tCmdGroupName, L"");
				SetDlgItemText(hWnd2, tCmdGroupGuiArg, L"");
				SetDlgItemText(hWnd2, tCmdGroupCommands, L"");
			}
			//for (size_t i = 0; i < countof(SettingsNS::nTaskCtrlId); i++)
			//	EnableWindow(GetDlgItem(hWnd2, SettingsNS::nTaskCtrlId[i]), lbEnable);
			EnableDlgItems(hWnd2, eTaskCtrlId, lbEnable);

			delete[] Items;

			break;
		} // lbCmdTasks:

	#ifdef _DEBUG
	default:
		_ASSERTE(FALSE && "ComboBox was not processed by CSetPgTasks::OnComboBox");
	#endif
	}
	return 0;
}

bool CSetPgTasks::SelectNextItem(bool bNext, bool bProcess)
{
	if (bProcess)
	{
		int iCur = CSetDlgLists::GetListboxCurSel(mh_Dlg, lbCmdTasks, true);
		if (iCur >= 0)
		{
			if (bNext)
			{
				iCur++;
				if (iCur >= (int)SendDlgItemMessage(mh_Dlg, lbCmdTasks, LB_GETCOUNT, 0,0))
					goto wrap;
			}
			else
			{
				iCur--;
				if (iCur < 0)
					goto wrap;
			}

			CSetDlgLists::ListBoxMultiSel(mh_Dlg, lbCmdTasks, iCur);

			OnComboBox(mh_Dlg, lbCmdTasks, LBN_SELCHANGE);
		}
	}

wrap:
	return true;
}
