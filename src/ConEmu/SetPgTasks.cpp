
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

#pragma warning(disable: 4091)
#include <shlobj.h>
#pragma warning(default: 4091)
#ifdef __GNUC__
#include "ShObjIdl_Part.h"
#endif // __GNUC__

#include "ConEmu.h"
#include "ConEmuApp.h"
#include "ConfirmDlg.h"
#include "HotkeyDlg.h"
#include "OptionsClass.h"
#include "OptionsFast.h"
#include "Recreate.h"
#include "SetPgTasks.h"
#include "VConGroup.h"

CSetPgTasks::CSetPgTasks()
	: mb_IgnoreCmdGroupEdit(false)
	, mb_IgnoreCmdGroupList(false)
	, m_Tasks(-1, NULL)
{
}

CSetPgTasks::~CSetPgTasks()
{
	m_Tasks.Release();
}

CSetPgTasks::TaskVis::TaskVis(INT_PTR _id, LPCWSTR _label)
	: id(_id)
	, index(-1)
	, name(_label)
{
}

CSetPgTasks::TaskVis* CSetPgTasks::TaskVis::CreateChild(int cid, LPCWSTR label)
{
	TaskVis* p = new TaskVis{cid, label};
	children.push_back(p);
	return p;
}

CSetPgTasks::TaskVis* CSetPgTasks::TaskVis::CreateFolder(LPCWSTR folder)
{
	for (INT_PTR i = 0; i < children.size(); ++i)
	{
		TaskVis* p = children[i];
		if (p->id == -1 && lstrcmpi(folder, p->name) == 0)
			return p;
	}
	return CreateChild(-1, folder);
}

void CSetPgTasks::TaskVis::Release()
{
	for (INT_PTR i = 0; i < children.size(); ++i)
	{
		TaskVis* p = NULL;
		klSwap(p, children[i]);
		if (p)
			p->Release();
		delete p;
	}
	children.clear();
	name.Clear();
	id = -1;
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
		int nTab = 3*4; // represent the number of quarters of the average character width for the font
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


	m_TasksPtr.clear();
	m_Tasks.Release();
	int nGroup = 0;
	const CommandTasks* pGrp = NULL;
	while ((pGrp = gpSet->CmdTaskGet(nGroup)))
	{
		LPCWSTR pszColon = wcsstr(pGrp->pszName, L"::");
		if (pszColon)
		{
			CEStr lsFolder((LPCWSTR)pGrp->pszName);
			wchar_t* pch = wcsstr(lsFolder.ms_Val, L"::");
			*(pch+2) = 0;
			TaskVis* parent = m_Tasks.CreateFolder(lsFolder.ms_Val);
			parent->CreateChild(nGroup, pszColon+2);
		}
		else
		{
			m_Tasks.CreateChild(nGroup, pGrp->pszName);
		}
		nGroup++;
	}
	for (INT_PTR i1 = 0; i1 < m_Tasks.children.size(); ++i1)
	{
		TaskVis* p1 = m_Tasks.children[i1];
		INT_PTR iIndex = SendDlgItemMessage(hDlg, lbCmdTasks, LB_ADDSTRING, 0, (LPARAM)p1->name.ms_Val);
		if (iIndex < 0)
		{
			_ASSERTEX(iIndex >= 0);
			break;
		}
		_ASSERTEX(iIndex == m_TasksPtr.size());
		p1->index = iIndex;
		m_TasksPtr.push_back(p1);
		//SendDlgItemMessage(hDlg, lbCmdTasks, LB_SETITEMDATA, iIndex, (LPARAM)p1->id);

		for (INT_PTR i2 = 0; i2 < p1->children.size(); ++i2)
		{
			TaskVis* p2 = p1->children[i2];
			CEStr lsLabel(L"\t", p2->name);
			INT_PTR iIndex = SendDlgItemMessage(hDlg, lbCmdTasks, LB_ADDSTRING, 0, (LPARAM)lsLabel.ms_Val);
			if (iIndex < 0)
			{
				_ASSERTEX(iIndex >= 0);
				break;
			}
			_ASSERTEX(iIndex == m_TasksPtr.size());
			p2->index = iIndex;
			m_TasksPtr.push_back(p2);
			//SendDlgItemMessage(hDlg, lbCmdTasks, LB_SETITEMDATA, iIndex, (LPARAM)p2->id);
		}
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

// cbCmdGrpDefaultNew, cbCmdGrpDefaultCmd, cbCmdGrpTaskbar, cbCmdGrpToolbar
void CSetPgTasks::OnTaskFlags(WORD CB)
{
	int iCur = CSetDlgLists::GetListboxCurSel(hDlg, lbCmdTasks, true);
	if (iCur < 0)
		return;

	CommandTasks* p = (CommandTasks*)gpSet->CmdTaskGet(iCur);
	if (!p)
		return;

	bool bDistinct = false;
	bool bUpdate = false;
	CETASKFLAGS flag = CETF_NONE;

	switch (CB)
	{
	case cbCmdGrpDefaultNew:
		flag = CETF_NEW_DEFAULT;
		if (isChecked(hDlg, CB))
		{
			bDistinct = true;
			p->Flags |= flag;
		}
		else
		{
			p->Flags &= ~flag;
		}
		break;
	case cbCmdGrpDefaultCmd:
		flag = CETF_CMD_DEFAULT;
		if (isChecked(hDlg, CB))
		{
			bDistinct = true;
			p->Flags |= flag;
		}
		else
		{
			p->Flags &= ~flag;
		}
		break;
	case cbCmdGrpTaskbar:
		if (isChecked(hDlg, CB))
			p->Flags &= ~CETF_NO_TASKBAR;
		else
			p->Flags |= CETF_NO_TASKBAR;
		bUpdate = true;
		break;
	case cbCmdGrpToolbar:
		if (isChecked(hDlg, CB))
			p->Flags |= CETF_ADD_TOOLBAR;
		else
			p->Flags &= ~CETF_ADD_TOOLBAR;
		break;
	}

	if (bDistinct)
	{
		int nGroup = 0;
		CommandTasks* pGrp = NULL;
		while ((pGrp = (CommandTasks*)gpSet->CmdTaskGet(nGroup++)))
		{
			if (pGrp != p)
				pGrp->Flags &= ~flag;
		}
	}

	if (bUpdate)
	{
		if (gpSet->isJumpListAutoUpdate)
		{
			UpdateWin7TaskList(true/*bForce*/, true/*bNoSuccMsg*/);
		}
		else
		{
			TODO("Show hint to press ‘Update now’ button");
		}
	}

} // cbCmdGrpDefaultNew, cbCmdGrpDefaultCmd, cbCmdGrpTaskbar, cbCmdGrpToolbar


// cbCmdTasksAdd
void CSetPgTasks::OnTaskAdd()
{
	_ASSERTE(CB==cbCmdTasksAdd);

	int iCount = (int)SendDlgItemMessage(hDlg, lbCmdTasks, LB_GETCOUNT, 0,0);
	if (iCount < 0)
		return;
	if (!gpSet->CmdTaskGet(iCount))
		gpSet->CmdTaskSet(iCount, L"", L"", L"");

	OnInitDialog(mh_Dlg, false);

	CSetDlgLists::ListBoxMultiSel(hDlg, lbCmdTasks, iCount);

	if (pTasksPg)
		pTasksPg->OnComboBox(hDlg, lbCmdTasks, LBN_SELCHANGE);

} // cbCmdTasksAdd


// cbCmdTasksDup
void CSetPgTasks::OnTaskDup()
{
	_ASSERTE(CB==cbCmdTasksDup);

	// Get the one selected task
	int *Selected = NULL, iCount = CSetDlgLists::GetListboxSelection(hDlg, lbCmdTasks, Selected);
	if (iCount != 1 || Selected[0] < 0)
		return;
	int iSelected = Selected[0];
	delete[] Selected;
	const CommandTasks* pSrc = gpSet->CmdTaskGet(iSelected);
	if (!pSrc)
		return;

	iCount = (int)SendDlgItemMessage(hDlg, lbCmdTasks, LB_GETCOUNT, 0,0);
	if (iCount < 0)
		return;
	// ensure the cell is empty and create new task with the same GuiArgs and Commands
	if (gpSet->CmdTaskGet(iCount))
		return;
	gpSet->CmdTaskSet(iCount, L"", pSrc->pszGuiArgs, pSrc->pszCommands);
	while (iSelected < (iCount - 1))
	{
		gpSet->CmdTaskXch(iCount, iCount - 1);
		--iCount;
	}

	OnInitDialog(mh_Dlg, false);

	CSetDlgLists::ListBoxMultiSel(hDlg, lbCmdTasks, iCount);

	if (pTasksPg)
		pTasksPg->OnComboBox(hDlg, lbCmdTasks, LBN_SELCHANGE);

} // cbCmdTasksDup


// cbCmdTasksDel
void CSetPgTasks::OnTaskDel()
{
	_ASSERTE(CB==cbCmdTasksDel);

	int *Selected = NULL, iCount = CSetDlgLists::GetListboxSelection(hDlg, lbCmdTasks, Selected);
	if (iCount <= 0)
		return;

	const CommandTasks* p = NULL;
	bool bIsStartup = false;

	for (INT_PTR i = iCount-1; i >= 0; i--)
	{
		p = gpSet->CmdTaskGet(Selected[iCount-1]);
		if (!p)
		{
			_ASSERTE(FALSE && "Failed to get selected task");
			delete[] Selected;
			return;
		}

		if (gpSet->psStartTasksName && p->pszName && (lstrcmpi(gpSet->psStartTasksName, p->pszName) == 0))
			bIsStartup = true;
	}

	size_t cchMax = (p->pszName ? _tcslen(p->pszName) : 0) + 200;
	wchar_t* pszMsg = (wchar_t*)malloc(cchMax*sizeof(*pszMsg));
	if (!pszMsg)
	{
		delete[] Selected;
		return;
	}

	wchar_t szOthers[64] = L"";
	if (iCount > 1)
		_wsprintf(szOthers, SKIPCOUNT(szOthers) L"\n" L"and %i other task(s)", (iCount-1));

	_wsprintf(pszMsg, SKIPLEN(cchMax) L"%sDelete Task\n%s%s?",
		bIsStartup ? L"Warning! You about to delete startup task!\n\n" : L"",
		p->pszName ? p->pszName : L"{???}",
		szOthers);

	int nBtn = MsgBox(pszMsg, MB_YESNO|(bIsStartup ? MB_ICONEXCLAMATION : MB_ICONQUESTION), NULL, ghOpWnd);
	SafeFree(pszMsg);

	if (nBtn != IDYES)
	{
		delete[] Selected;
		return;
	}

	for (INT_PTR i = iCount-1; i >= 0; i--)
	{
		gpSet->CmdTaskSet(Selected[i], NULL, NULL, NULL);

		if (bIsStartup && gpSet->psStartTasksName)
			*gpSet->psStartTasksName = 0;
	}

	OnInitDialog(mh_Dlg, false);

	iCount = (int)SendDlgItemMessage(hDlg, lbCmdTasks, LB_GETCOUNT, 0,0);
	CSetDlgLists::ListBoxMultiSel(hDlg, lbCmdTasks, min(Selected[0],(iCount-1)));

	if (pTasksPg)
		pTasksPg->OnComboBox(hDlg, lbCmdTasks, LBN_SELCHANGE);

	delete[] Selected;

} // cbCmdTasksDel


// cbCmdTasksUp || cbCmdTasksDown
void CSetPgTasks::OnTaskUpDown(WORD CB)
{
	_ASSERTE(CB==cbCmdTasksUp || CB==cbCmdTasksDown);

	int iCur, iChg;
	iCur = CSetDlgLists::GetListboxCurSel(hDlg, lbCmdTasks, true);
	if (iCur < 0)
		return;
	if (CB == cbCmdTasksUp)
	{
		if (!iCur)
			return;
		iChg = iCur - 1;
	}
	else
	{
		iChg = iCur + 1;
		if (iChg >= (int)SendDlgItemMessage(hDlg, lbCmdTasks, LB_GETCOUNT, 0,0))
			return;
	}

	if (!gpSet->CmdTaskXch(iCur, iChg))
		return;

	OnInitDialog(mh_Dlg, false);

	CSetDlgLists::ListBoxMultiSel(hDlg, lbCmdTasks, iChg);

	if (pTasksPg)
		pTasksPg->OnComboBox(hDlg, lbCmdTasks, LBN_SELCHANGE);

} // cbCmdTasksUp || cbCmdTasksDown


// cbCmdGroupKey
void CSetPgTasks::OnTaskHotKey()
{
	_ASSERTE(CB==cbCmdGroupKey);

	int iCur = CSetDlgLists::GetListboxCurSel(hDlg, lbCmdTasks, true);
	if (iCur < 0)
		return;
	const CommandTasks* pCmd = gpSet->CmdTaskGet(iCur);
	if (!pCmd)
		return;

	DWORD VkMod = pCmd->HotKey.GetVkMod();
	if (CHotKeyDialog::EditHotKey(ghOpWnd, VkMod))
	{
		gpSet->CmdTaskSetVkMod(iCur, VkMod);
		wchar_t szKey[128] = L"";
		SetDlgItemText(hDlg, tCmdGroupKey, ConEmuHotKey::GetHotkeyName(pCmd->HotKey.GetVkMod(), szKey));
	}
} // cbCmdGroupKey


// cbCmdGroupApp
void CSetPgTasks::OnCmdAddTab()
{
	_ASSERTE(CB==cbCmdGroupApp);

	// Добавить команду в группу
	RConStartArgsEx args;
	args.aRecreate = cra_EditTab;
	int nDlgRc = gpConEmu->RecreateDlg(&args);

	if (nDlgRc == IDC_START)
	{
		wchar_t* pszCmd = args.CreateCommandLine();
		if (!pszCmd || !*pszCmd)
		{
			DisplayLastError(L"Can't compile command line for new tab\nAll fields are empty?", -1);
		}
		else
		{
			//SendDlgItemMessage(hDlg, tCmdGroupCommands, EM_REPLACESEL, TRUE, (LPARAM)pszName);
			wchar_t* pszFull = GetDlgItemTextPtr(hDlg, tCmdGroupCommands);
			if (!pszFull || !*pszFull)
			{
				SafeFree(pszFull);
				pszFull = pszCmd;
			}
			else
			{
				size_t cchLen = _tcslen(pszFull);
				size_t cchMax = cchLen + 7 + _tcslen(pszCmd);
				pszFull = (wchar_t*)realloc(pszFull, cchMax*sizeof(*pszFull));

				int nRN = 0;
				if (cchLen >= 2)
				{
					if (pszFull[cchLen-2] == L'\r' && pszFull[cchLen-1] == L'\n')
					{
						nRN++;
						if (cchLen >= 4)
						{
							if (pszFull[cchLen-4] == L'\r' && pszFull[cchLen-3] == L'\n')
							{
								nRN++;
							}
						}
					}
				}

				if (nRN < 2)
					_wcscat_c(pszFull, cchMax, nRN ? L"\r\n" : L"\r\n\r\n");

				_wcscat_c(pszFull, cchMax, pszCmd);
			}

			if (pszFull)
			{
				SetDlgItemText(hDlg, tCmdGroupCommands, pszFull);
				pTasksPg->OnEditChanged(mh_Dlg, tCmdGroupCommands);
			}
			else
			{
				_ASSERTE(pszFull);
			}
			if (pszCmd == pszFull)
				pszCmd = NULL;
			SafeFree(pszFull);
		}
		SafeFree(pszCmd);
	}
} // cbCmdGroupApp


// cbCmdTasksParm - Show "Choose file" dialog and paste into the Task contents the pathname
void CSetPgTasks::OnCmdAddFile()
{
	_ASSERTE(CB==cbCmdTasksParm);

	// Добавить файл
	wchar_t temp[MAX_PATH+10] = {};
	OPENFILENAME ofn = {sizeof(ofn)};
	ofn.hwndOwner = ghOpWnd;
	ofn.lpstrFilter = L"All files (*.*)\0*.*\0Text files (*.txt,*.ini,*.log)\0*.txt;*.ini;*.log\0Executables (*.exe,*.com,*.bat,*.cmd)\0*.exe;*.com;*.bat;*.cmd\0Scripts (*.vbs,*.vbe,*.js,*.jse)\0*.vbs;*.vbe;*.js;*.jse\0\0";
	//ofn.lpstrFilter = L"All files (*.*)\0*.*\0\0";
	ofn.lpstrFile = temp+1;
	ofn.nMaxFile = countof(temp)-10;
	ofn.lpstrTitle = L"Choose file";
	ofn.Flags = OFN_ENABLESIZING|OFN_NOCHANGEDIR
	            | OFN_PATHMUSTEXIST|OFN_EXPLORER|OFN_HIDEREADONLY|OFN_FILEMUSTEXIST;

	if (GetOpenFileName(&ofn))
	{
		LPCWSTR pszName = temp+1;
		if (wcschr(pszName, L' '))
		{
			temp[0] = L'"';
			wcscat_c(temp, L"\"");
			pszName = temp;
		}
		else
		{
			temp[0] = L' ';
		}
		//wcscat_c(temp, L"\r\n\r\n");

		SendDlgItemMessage(hDlg, tCmdGroupCommands, EM_REPLACESEL, TRUE, (LPARAM)pszName);
	}
} // cbCmdTasksParm


// cbCmdTasksDir - Choose and paste tab startup directory
void CSetPgTasks::OnCmdAddDir()
{
	_ASSERTE(CB==cbCmdTasksDir);

	TODO("Извлечь текущий каталог запуска");

	BROWSEINFO bi = {ghOpWnd};
	wchar_t szFolder[MAX_PATH+1] = {0};
	TODO("Извлечь текущий каталог запуска");
	bi.pszDisplayName = szFolder;
	wchar_t szTitle[100];
	bi.lpszTitle = lstrcpyn(szTitle, L"Choose tab startup directory", countof(szTitle));
	bi.ulFlags = BIF_EDITBOX | BIF_RETURNONLYFSDIRS | BIF_VALIDATE;
	bi.lpfn = CRecreateDlg::BrowseCallbackProc;
	bi.lParam = (LPARAM)szFolder;
	LPITEMIDLIST pRc = SHBrowseForFolder(&bi);

	if (pRc)
	{
		if (SHGetPathFromIDList(pRc, szFolder))
		{
			bool bQuot = IsQuotationNeeded(szFolder);
			CEStr lsFull(L" -new_console:d:", bQuot ? L"\"" : NULL, szFolder, bQuot ? L"\" " : L" ");

			SendDlgItemMessage(hDlg, tCmdGroupCommands, EM_REPLACESEL, TRUE, (LPARAM)lsFull.ms_Val);
		}

		CoTaskMemFree(pRc);
	}
} // cbCmdTasksDir

// cbCmdTasksActive - Paste all open tabs into the Task
void CSetPgTasks::OnCmdActiveTabs()
{
	_ASSERTE(CB==cbCmdTasksActive);

	wchar_t* pszTasks = CVConGroup::GetTasks(); // return all opened tabs
	if (pszTasks)
	{
		SendDlgItemMessage(hDlg, tCmdGroupCommands, EM_REPLACESEL, TRUE, (LPARAM)pszTasks);
		SafeFree(pszTasks);
	}
} // cbCmdTasksActive


// cbAddDefaults
void CSetPgTasks::OnTasksAddRefresh()
{
	_ASSERTE(CB==cbAddDefaults);

	int iBtn = ConfirmDialog(L"Do you want to add new default tasks in your task list?",
		L"Default tasks", gpConEmu->GetDefaultTitle(),
		CEWIKIBASE L"Tasks.html#add-default-tasks",
		MB_YESNOCANCEL|MB_ICONEXCLAMATION, ghOpWnd,
		L"Add new tasks", L"Append absent tasks for newly installed shells",
		L"Refresh default tasks", L"Add new and REWRITE EXISTING tasks with defaults",
		L"Cancel");
	if (iBtn == IDCANCEL)
		return;

	// Append or rewrite default tasks
	CreateDefaultTasks(slf_AppendTasks|((iBtn == IDNO) ? slf_RewriteExisting : slf_None));

	// Refresh onscreen contents
	OnInitDialog(mh_Dlg, true);

} // cbAddDefaults

// cbCmdTasksReload
void CSetPgTasks::OnTasksReload()
{
	_ASSERTE(CB==cbCmdTasksReload);

	int iBtn = ConfirmDialog(L"All unsaved changes will be lost!\n\nReload Tasks from settings?",
		L"Warning!", gpConEmu->GetDefaultTitle(),
		CEWIKIBASE L"Tasks.html",
		MB_YESNO|MB_ICONEXCLAMATION, ghOpWnd,
		L"Reload Tasks", NULL,
		L"Cancel", NULL);
	if (iBtn != IDYES)
		return;

	// Reload Tasks
	gpSet->LoadCmdTasks(NULL, true);

	// Refresh onscreen contents
	OnInitDialog(mh_Dlg, true);

} // cbCmdTasksReload
