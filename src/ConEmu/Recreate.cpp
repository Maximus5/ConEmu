
/*
Copyright (c) 2009-2017 Maximus5
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
#include "../common/WFiles.h"
#include <lm.h>
#pragma warning(disable: 4091)
#include <ShlObj.h>
#pragma warning(default: 4091)
#include "ConEmu.h"
#include "DpiAware.h"
#include "DynDialog.h"
#include "LngRc.h"
#include "OptionsClass.h"
#include "RealConsole.h"
#include "Recreate.h"
#include "SetCmdTask.h"
#include "VConGroup.h"
#include "VirtualConsole.h"

CRecreateDlg::CRecreateDlg()
	: mh_Dlg(NULL)
	, mn_DlgRc(0)
	, mp_Args(NULL)
	, mh_Parent(NULL)
	, mb_DontAutoSelCmd(false)
	, mpsz_DefCmd(NULL)
	, mpsz_CurCmd(NULL)
	, mpsz_SysCmd(NULL)
	, mpsz_DefDir(NULL)
	, mp_DpiAware(NULL)
{
	ms_CurUser[0] = 0;
}

CRecreateDlg::~CRecreateDlg()
{
	_ASSERTE(!mh_Dlg || !IsWindow(mh_Dlg));
	Close();
}

HWND CRecreateDlg::GetHWND()
{
	if (!this)
	{
		_ASSERTE(this);
		return NULL;
	}
	return mh_Dlg;
}

// Открыть диалог с подтверждением параметров создания/закрытия/пересоздания консоли
int CRecreateDlg::RecreateDlg(RConStartArgs* apArgs, bool abDontAutoSelCmd /*= false*/)
{
	if (!this)
	{
		_ASSERTE(this);
		return IDCANCEL;
	}

	mb_DontAutoSelCmd = abDontAutoSelCmd;

	if (mh_Dlg && IsWindow(mh_Dlg))
	{
		DisplayLastError(L"Close previous 'Create dialog' first, please!", -1);
		return IDCANCEL;
	}

	DontEnable de;

	gpConEmu->SkipOneAppsRelease(true);

	//if (!gpConEmu->mh_RecreatePasswFont)
	//{
	//	gpConEmu->mh_RecreatePasswFont = CreateFont(

	//}

	mn_DlgRc = IDCANCEL;
	mp_Args = apArgs;

	mh_Parent = (apArgs->aRecreate == cra_EditTab && ghOpWnd) ? ghOpWnd : ghWnd;

	#ifdef _DEBUG
	if ((mh_Parent == ghWnd) && gpConEmu->isIconic())
	{
		_ASSERTE(FALSE && "Window must be shown before dialog!");
	}
	#endif

	InitVars();

	bool bPrev = gpConEmu->SetSkipOnFocus(true);
	if (!mp_DpiAware)
		mp_DpiAware = new CDpiForDialog();
	// Modal dialog (CreateDialog)
	int nRc = CDynDialog::ExecuteDialog(IDD_RESTART, mh_Parent, RecreateDlgProc, (LPARAM)this);
	UNREFERENCED_PARAMETER(nRc);
	gpConEmu->SetSkipOnFocus(bPrev);

	FreeVars();

	//if (gpConEmu->mh_RecreatePasswFont)
	//{
	//	DeleteObject(gpConEmu->mh_RecreatePasswFont);
	//	gpConEmu->mh_RecreatePasswFont = NULL;
	//}

	gpConEmu->SkipOneAppsRelease(false);

	return mn_DlgRc;
}

void CRecreateDlg::Close()
{
	if (mh_Dlg)
	{
		if (IsWindow(mh_Dlg))
		{
			mn_DlgRc = IDCANCEL;
			EndDialog(mh_Dlg, IDCANCEL);
		}
		mh_Dlg = NULL;
	}
	SafeDelete(mp_DpiAware);
}

INT_PTR CRecreateDlg::OnInitDialog(HWND hDlg, UINT messg, WPARAM wParam, LPARAM lParam)
{
	LRESULT lbRc = FALSE;

	gpConEmu->OnOurDialogOpened();

	CDynDialog::LocalizeDialog(hDlg);

	// Visual
	SendMessage(hDlg, WM_SETICON, ICON_BIG, (LPARAM)hClassIcon);
	SendMessage(hDlg, WM_SETICON, ICON_SMALL, (LPARAM)hClassIconSm);

	// Set password style (avoid "bars" on some OS)
	SendDlgItemMessage(hDlg, tRunAsPassword, WM_SETFONT, (LPARAM)(HFONT)GetStockObject(DEFAULT_GUI_FONT), 0);

	// Add menu items
	HMENU hSysMenu = GetSystemMenu(hDlg, FALSE);
	InsertMenu(hSysMenu, 0, MF_BYPOSITION | MF_SEPARATOR, 0, 0);
	InsertMenu(hSysMenu, 0, MF_BYPOSITION | MF_STRING | MF_ENABLED,
				ID_RESETCMDHISTORY, L"Clear history...");
	InsertMenu(hSysMenu, 0, MF_BYPOSITION | MF_STRING | MF_ENABLED
				| (gpSet->isSaveCmdHistory ? MF_CHECKED : 0),
				ID_STORECMDHISTORY, L"Store history");

	//#ifdef _DEBUG
	//SetWindowPos(ghOpWnd, HWND_NOTOPMOST, 0,0,0,0, SWP_NOSIZE|SWP_NOMOVE);
	//#endif

	RConStartArgs* pArgs = mp_Args;
	_ASSERTE(pArgs);

	// Fill command and task drop down
	SendMessage(hDlg, UM_FILL_CMDLIST, TRUE, 0);

	// Set text in command and folder fields
	{
	LPCWSTR pszSetCmd = mpsz_DefCmd ? mpsz_DefCmd : pArgs->pszSpecialCmd ? pArgs->pszSpecialCmd : L"";
	CEStr lsTempCmd, lsAppend;
	if (!mpsz_DefCmd && pArgs)
	{
		RConStartArgs tempArgs;
		tempArgs.AssignFrom(pArgs);
		tempArgs.CleanSecure();
		tempArgs.RunAsSystem = pArgs->RunAsSystem;
		tempArgs.eSplit = RConStartArgs::eSplitNone;
		SafeFree(tempArgs.pszSpecialCmd);
		SafeFree(tempArgs.pszStartupDir);
		tempArgs.NewConsole = pArgs->NewConsole;
		lsAppend = tempArgs.CreateCommandLine();
		if (!lsAppend.IsEmpty())
		{
			lsTempCmd = lstrmerge(pszSetCmd, ((lsAppend[0] == L' ') ? NULL : L" "), lsAppend);
			pszSetCmd = lsTempCmd.ms_Val;
		}
	}
	SetDlgItemText(hDlg, IDC_RESTART_CMD, pszSetCmd);
	// TODO: gh-959: enable autocorrection of cbRunAsAdmin by IDC_RESTART_CMD task contents (first line)
	}

	// "%CD%" was specified as startup dir? In Task parameters for example...
	CEStr lsStartDir;
	if (pArgs->pszStartupDir
		&& (lstrcmpi(pArgs->pszStartupDir, L"%CD%") == 0))
	{
		lsStartDir.Set(ms_RConCurDir);
	}
	// Suggest default ConEmu working directory otherwise (unless it's a cra_RecreateTab)
	if (lsStartDir.IsEmpty())
	{
		lsStartDir.Set(gpConEmu->WorkDir());
	}

	// Current directory, startup directory, ConEmu startup directory, and may be startup directory history in the future
	AddDirectoryList(mpsz_DefDir ? mpsz_DefDir : lsStartDir.ms_Val);
	AddDirectoryList(ms_RConCurDir);
	AddDirectoryList(ms_RConStartDir);
	AddDirectoryList(gpConEmu->WorkDir());
	LPCWSTR pszShowDir;
	if ((pArgs->aRecreate == cra_RecreateTab) && !ms_RConCurDir.IsEmpty())
		pszShowDir = ms_RConCurDir;
	else
		pszShowDir = mpsz_DefDir ? mpsz_DefDir : lsStartDir.ms_Val;
	SetDlgItemText(hDlg, IDC_STARTUP_DIR, pszShowDir);

	// Split controls
	if (pArgs->aRecreate == cra_RecreateTab)
	{
		// Hide Split's
		ShowWindow(GetDlgItem(hDlg, gbRecreateSplit), SW_HIDE);
		ShowWindow(GetDlgItem(hDlg, rbRecreateSplitNone), SW_HIDE);
		ShowWindow(GetDlgItem(hDlg, rbRecreateSplit2Right), SW_HIDE);
		ShowWindow(GetDlgItem(hDlg, rbRecreateSplit2Bottom), SW_HIDE);
		ShowWindow(GetDlgItem(hDlg, stRecreateSplit), SW_HIDE);
		ShowWindow(GetDlgItem(hDlg, tRecreateSplit), SW_HIDE);
	}
	else
	{
		// Fill splits
		SetDlgItemInt(hDlg, tRecreateSplit, (1000-pArgs->nSplitValue)/10, FALSE);
		CheckRadioButton(hDlg, rbRecreateSplitNone, rbRecreateSplit2Bottom, rbRecreateSplitNone+pArgs->eSplit);
		EnableWindow(GetDlgItem(hDlg, tRecreateSplit), (pArgs->eSplit != pArgs->eSplitNone));
		EnableWindow(GetDlgItem(hDlg, stRecreateSplit), (pArgs->eSplit != pArgs->eSplitNone));
	}

	// Спрятать флажок "New window"
	bool bRunInNewWindow_Hidden = (pArgs->aRecreate == cra_EditTab || pArgs->aRecreate == cra_RecreateTab);
	ShowWindow(GetDlgItem(hDlg, cbRunInNewWindow), bRunInNewWindow_Hidden ? SW_HIDE : SW_SHOWNORMAL);


	const wchar_t *pszUser = pArgs->pszUserName;
	const wchar_t *pszDomain = pArgs->pszDomain;
	bool bResticted = (pArgs->RunAsRestricted == crb_On);
	int nChecked = rbCurrentUser;
	int nNetOnly = cbRunAsNetOnly;
	DWORD nUserNameLen = countof(ms_CurUser);

	if (!GetUserName(ms_CurUser, &nUserNameLen))
		ms_CurUser[0] = 0;

	wchar_t szRbCaption[MAX_PATH*3];
	lstrcpy(szRbCaption, L"Run as current &user: "); lstrcat(szRbCaption, ms_CurUser);
	SetDlgItemText(hDlg, rbCurrentUser, szRbCaption);

	wchar_t szOtherUser[MAX_PATH*2+1]; szOtherUser[0] = 0;

	CVConGuard VCon;
	CVirtualConsole* pVCon = (gpConEmu->GetActiveVCon(&VCon) >= 0) ? VCon.VCon() : NULL;
	EnableWindow(GetDlgItem(hDlg, cbRunAsNetOnly), FALSE);
	if ((pArgs->pszUserName && *pArgs->pszUserName)
		|| ((pArgs->aRecreate == cra_RecreateTab) && pVCon && pVCon->RCon()->GetUserPwd(&pszUser, &pszDomain, &bResticted)))
	{
		nChecked = rbAnotherUser;
		CheckDlgButton(hDlg, cbRunAsNetOnly, pArgs->RunAsNetOnly ? BST_CHECKED : BST_UNCHECKED);
		if (bResticted)
		{
			CheckDlgButton(hDlg, cbRunAsRestricted, BST_CHECKED);
		}
		else
		{
			if (pArgs->pszUserName && *pArgs->pszUserName)
				lstrcpyn(szOtherUser, pArgs->pszUserName, countof(szOtherUser));
			else
				lstrcpyn(szOtherUser, ms_CurUser, countof(szOtherUser));

			if (pszDomain)
			{
				if (wcschr(pszDomain, L'.'))
				{
					// Если в имени домена есть точка - используем нотацию user@domain
					// По идее, мы сюда не попадаем, т.к. при вводе имени в таком формате
					// pszDomain не заполняется, и "UPN format" остается в pszUser
					lstrcpyn(szOtherUser, pszUser, MAX_PATH);
					wcscat_c(szOtherUser, L"@");
					lstrcpyn(szOtherUser+_tcslen(szOtherUser), pszDomain, MAX_PATH);
				}
				else
				{
					// "Старая" нотация domain\user
					lstrcpyn(szOtherUser, pszDomain, MAX_PATH);
					wcscat_c(szOtherUser, L"\\");
					lstrcpyn(szOtherUser+_tcslen(szOtherUser), pszUser, MAX_PATH);
				}
			}
			else
			{
				lstrcpyn(szOtherUser, pszUser, countof(szOtherUser));
			}

			SetDlgItemText(hDlg, tRunAsPassword, pArgs->szUserPassword);
			EnableWindow(GetDlgItem(hDlg, cbRunAsNetOnly), TRUE);
		}
	}

	SetDlgItemText(hDlg, tRunAsUser, (nChecked == rbAnotherUser) ? szOtherUser : L"");
	CheckRadioButton(hDlg, rbCurrentUser, rbAnotherUser, nChecked);
	RecreateDlgProc(hDlg, UM_USER_CONTROLS, 0, 0);

	if (gOSVer.dwMajorVersion < 6)
	{
		// В XP и ниже это просто RunAs - с возможностью ввода имени пользователя и пароля
		//apiShowWindow(GetDlgItem(hDlg, cbRunAsAdmin), SW_HIDE);
		SetDlgItemTextA(hDlg, cbRunAsAdmin, "&Run as..."); //GCC hack. иначе не собирается
		// И уменьшить длину
		RECT rcBox; GetWindowRect(GetDlgItem(hDlg, cbRunAsAdmin), &rcBox);
		SetWindowPos(GetDlgItem(hDlg, cbRunAsAdmin), NULL, 0, 0, (rcBox.right-rcBox.left)/2, rcBox.bottom-rcBox.top,
				        SWP_NOMOVE|SWP_NOZORDER);
	}
	else if (gpConEmu->mb_IsUacAdmin || (pArgs && (pArgs->RunAsAdministrator == crb_On)))
	{
		CheckDlgButton(hDlg, cbRunAsAdmin, BST_CHECKED);

		if (gpConEmu->mb_IsUacAdmin)  // Только в Vista+ если GUI уже запущен под админом
		{
			EnableWindow(GetDlgItem(hDlg, cbRunAsAdmin), FALSE);
		}
		else //if (gOSVer.dwMajorVersion < 6)
		{
			RecreateDlgProc(hDlg, WM_COMMAND, cbRunAsAdmin, 0);
		}
	}

	//}
	SetClassLongPtr(hDlg, GCLP_HICON, (LONG_PTR)hClassIcon);

	if (pArgs->aRecreate == cra_RecreateTab)
	{
		SetWindowText(hDlg, CLngRc::getRsrc(lng_DlgRestartConsole/*"Restart console"*/));
		SendDlgItemMessage(hDlg, IDC_RESTART_ICON, STM_SETICON, (WPARAM)LoadIcon(NULL,IDI_EXCLAMATION), 0);
		lbRc = TRUE;
	}
	else
	{
		SetWindowText(hDlg, CLngRc::getRsrc(lng_DlgCreateNewConsole/*"Create new console"*/));

		// If we disallowed to create "Multiple consoles in one window"
		// - Check & Disable "New window" checkbox
		bool bForceNewWindow = (!gpSetCls->IsMulti() && gpConEmu->isVConExists(0));
		CheckDlgButton(hDlg, cbRunInNewWindow, (pArgs->aRecreate == cra_CreateWindow || bForceNewWindow) ? BST_CHECKED : BST_UNCHECKED);
		EnableWindow(GetDlgItem(hDlg, cbRunInNewWindow), !bForceNewWindow);

		//
		SendDlgItemMessage(hDlg, IDC_RESTART_ICON, STM_SETICON, (WPARAM)LoadIcon(NULL,IDI_QUESTION), 0);
		POINT pt = {0,0};
		MapWindowPoints(GetDlgItem(hDlg, IDC_TERMINATE), hDlg, &pt, 1);
		DestroyWindow(GetDlgItem(hDlg, IDC_TERMINATE));
		SetWindowPos(GetDlgItem(hDlg, IDC_START), NULL, pt.x, pt.y, 0,0, SWP_NOSIZE|SWP_NOZORDER);
		SetDlgItemText(hDlg, IDC_START, (pArgs->aRecreate == cra_EditTab) ? L"&Save" : L"&Start");
		DestroyWindow(GetDlgItem(hDlg, IDC_WARNING));
	}

	// Align "New window" and "Run as administrator" checkboxes
	{
		RECT rcBox = {}; GetWindowRect(GetDlgItem(hDlg, cbRunAsAdmin), &rcBox);
		MapWindowPoints(NULL, hDlg, (LPPOINT)&rcBox, 2);
		const int chk_height = (rcBox.bottom - rcBox.top);

		POINT pt = {rcBox.left};
		if (!bRunInNewWindow_Hidden)
		{
			RECT rcIcoBox = {}; GetWindowRect(GetDlgItem(hDlg, IDC_RESTART_ICON), &rcIcoBox);
			MapWindowPoints(NULL, hDlg, (LPPOINT)&rcIcoBox, 2);
			const int ico_height = (rcIcoBox.bottom - rcIcoBox.top);
			const int h2 = (chk_height * 5) / 2;
			pt.y = rcIcoBox.top + (ico_height - h2)/2;
			SetWindowPos(GetDlgItem(hDlg, cbRunInNewWindow), NULL, pt.x, pt.y, 0,0, SWP_NOSIZE|SWP_NOZORDER);
			pt.y += (h2 - chk_height);
		}
		else
		{
			RECT rcBtnBox = {}; GetWindowRect(GetDlgItem(hDlg, IDC_START), &rcBtnBox);
			MapWindowPoints(NULL, hDlg, (LPPOINT)&rcBtnBox, 2);
			const int btn_height = (rcBtnBox.bottom - rcBtnBox.top);
			pt.y = rcBtnBox.top + (btn_height - chk_height)/2;
		}
		SetWindowPos(GetDlgItem(hDlg, cbRunAsAdmin), NULL, pt.x, pt.y, 0,0, SWP_NOSIZE|SWP_NOZORDER);
	}

	// Dpi aware processing at the end of sequence
	// because we done some manual control reposition
	if (mp_DpiAware)
	{
		mp_DpiAware->Attach(hDlg, ghWnd, CDynDialog::GetDlgClass(hDlg));
	}

	// Ensure, it will be "on screen"
	RECT rect; GetWindowRect(hDlg, &rect);
	RECT rcCenter = CenterInParent(rect, mh_Parent);
	MoveWindow(hDlg, rcCenter.left, rcCenter.top,
			    rect.right - rect.left, rect.bottom - rect.top, false);


	// Была отключена обработка CConEmuMain::OnFocus (лишние телодвижения)
	PostMessage(hDlg, (WM_APP+1), 0,0);


	// Default focus control
	if (pArgs->aRecreate == cra_RecreateTab)
		SetFocus(GetDlgItem(hDlg, IDC_START)); // Win+~ (Recreate tab), Focus on "Restart" button"
	else if ((pArgs->pszUserName && *pArgs->pszUserName) && !*pArgs->szUserPassword)
		SetFocus(GetDlgItem(hDlg, tRunAsPassword)); // We need password, all other fields are ready
	else
		SetFocus(GetDlgItem(hDlg, IDC_RESTART_CMD)); // Set focus in command-line field

	return lbRc;
}

INT_PTR CRecreateDlg::OnCtlColorStatic(HWND hDlg, UINT messg, WPARAM wParam, LPARAM lParam)
{
	if (GetDlgItem(hDlg, IDC_WARNING) == (HWND)lParam)
	{
		SetTextColor((HDC)wParam, 255);
		HBRUSH hBrush = GetSysColorBrush(COLOR_3DFACE);
		SetBkMode((HDC)wParam, TRANSPARENT);
		return (INT_PTR)hBrush;
	}

	return FALSE;
}

INT_PTR CRecreateDlg::OnFillCmdList(HWND hDlg, UINT messg, WPARAM wParam, LPARAM lParam)
{
	RConStartArgs* pArgs = mp_Args;
	_ASSERTE(pArgs);

	AddCommandList(pArgs->pszSpecialCmd);
	AddCommandList(mpsz_SysCmd, pArgs->pszSpecialCmd ? -1 : 0);
	AddCommandList(mpsz_CurCmd);
	AddCommandList(mpsz_DefCmd);

	// Может быть позван после очистки истории из меню, тогда нет смысла и дергаться
	if (wParam)
	{
		int index = 0;
		LPCWSTR pszHistory;
		while ((pszHistory = gpSet->HistoryGet(index++)) != NULL)
		{
			AddCommandList(pszHistory);
		}
	}

	// Tasks
	int nGroup = 0;
	const CommandTasks* pGrp = NULL;
	while ((pGrp = gpSet->CmdTaskGet(nGroup++)))
	{
		AddCommandList(pGrp->pszName);
	}

	return FALSE;
}

INT_PTR CRecreateDlg::OnUserControls(HWND hDlg, UINT messg, WPARAM wParam, LPARAM lParam)
{
	if (SendDlgItemMessage(hDlg, rbCurrentUser, BM_GETCHECK, 0, 0))
	{
		EnableWindow(GetDlgItem(hDlg, cbRunAsRestricted), TRUE);
		//BOOL lbText = SendDlgItemMessage(hDlg, cbRunAsRestricted, BM_GETCHECK, 0, 0) == 0;
		EnableWindow(GetDlgItem(hDlg, tRunAsUser), FALSE);
		EnableWindow(GetDlgItem(hDlg, tRunAsPassword), FALSE);
		EnableWindow(GetDlgItem(hDlg, cbRunAsNetOnly), FALSE);
	}
	else
	{
		EnableWindow(GetDlgItem(hDlg, cbRunAsNetOnly), TRUE);
		if (SendDlgItemMessage(hDlg, tRunAsUser, CB_GETCOUNT, 0, 0) == 0)
		{
			DWORD dwLevel = 3, dwEntriesRead = 0, dwTotalEntries = 0, dwResumeHandle = 0;
			NET_API_STATUS nStatus;
			USER_INFO_3 *info = NULL;
			nStatus = ::NetUserEnum(NULL, dwLevel, FILTER_NORMAL_ACCOUNT, (PBYTE*) & info,
					                MAX_PREFERRED_LENGTH, &dwEntriesRead, &dwTotalEntries, &dwResumeHandle);

			if (nStatus == NERR_Success)
			{
				wchar_t *pszAdmin = NULL, *pszLikeAdmin = NULL, *pszOtherUser = NULL;

				for (DWORD i = 0; i < dwEntriesRead; ++i)
				{
					// usri3_logon_server	"\\*"	wchar_t *
					if (!(info[i].usri3_flags & UF_ACCOUNTDISABLE) && info[i].usri3_name && *info[i].usri3_name)
					{
						SendDlgItemMessage(hDlg, tRunAsUser, CB_ADDSTRING, 0, (LPARAM)info[i].usri3_name);

						if (info[i].usri3_priv == 2/*USER_PRIV_ADMIN*/)
						{
							if (!pszAdmin && (info[i].usri3_user_id == 500))
								pszAdmin = lstrdup(info[i].usri3_name);
							else if (!pszLikeAdmin && (lstrcmpi(ms_CurUser, info[i].usri3_name) != 0))
								pszLikeAdmin = lstrdup(info[i].usri3_name);
						}
						else if (!pszOtherUser
							&& (info[i].usri3_priv == 1/*USER_PRIV_USER*/)
							&& (lstrcmpi(ms_CurUser, info[i].usri3_name) != 0))
						{
							pszOtherUser = lstrdup(info[i].usri3_name);
						}
					}
				}

				if (GetWindowTextLength(GetDlgItem(hDlg, tRunAsUser)) == 0)
				{
					// Try to suggest "Administrator" account
					SetDlgItemText(hDlg, tRunAsUser, pszAdmin ? pszAdmin : pszLikeAdmin ? pszLikeAdmin : pszOtherUser ? pszOtherUser : ms_CurUser);
				}

				::NetApiBufferFree(info);
				SafeFree(pszAdmin);
				SafeFree(pszLikeAdmin);
			}
			else
			{
				// Добавить хотя бы текущего
				SendDlgItemMessage(hDlg, tRunAsUser, CB_ADDSTRING, 0, (LPARAM)ms_CurUser);
			}
		}

		EnableWindow(GetDlgItem(hDlg, cbRunAsRestricted), FALSE);
		EnableWindow(GetDlgItem(hDlg, tRunAsUser), TRUE);
		EnableWindow(GetDlgItem(hDlg, tRunAsPassword), TRUE);
	}

	if (wParam == rbAnotherUser)
		SetFocus(GetDlgItem(hDlg, tRunAsUser));

	return FALSE;
}

INT_PTR CRecreateDlg::OnSysCommand(HWND hDlg, UINT messg, WPARAM wParam, LPARAM lParam)
{
	switch (LOWORD(wParam))
	{
	case ID_RESETCMDHISTORY:
		// Подтверждение спросит ResetCmdHistory
		if (gpSetCls->ResetCmdHistory(hDlg))
		{
			wchar_t* pszCmd = GetDlgItemTextPtr(hDlg, IDC_RESTART_CMD);
			SendDlgItemMessage(hDlg, IDC_RESTART_CMD, CB_RESETCONTENT, 0,0);
			SendMessage(hDlg, UM_FILL_CMDLIST, FALSE, 0);
			if (pszCmd)
			{
				SetDlgItemText(hDlg, IDC_RESTART_CMD, pszCmd);
				free(pszCmd);
			}
		}
		SetWindowLongPtr(hDlg, DWLP_MSGRESULT, 0);
		return TRUE;

	case ID_STORECMDHISTORY:
		if (MsgBox(gpSet->isSaveCmdHistory ? L"Do you want to disable history?" : L"Do you want to enable history?", MB_YESNO|MB_ICONQUESTION, NULL, hDlg) == IDYES)
		{
			gpSetCls->SetSaveCmdHistory(!gpSet->isSaveCmdHistory);
			HMENU hSysMenu = GetSystemMenu(hDlg, FALSE);
			CheckMenuItem(hSysMenu, ID_STORECMDHISTORY, MF_BYCOMMAND|(gpSet->isSaveCmdHistory ? MF_CHECKED : 0));
		}
		SetWindowLongPtr(hDlg, DWLP_MSGRESULT, 0);
		return TRUE;
	}

	return FALSE;
}

INT_PTR CRecreateDlg::OnButtonClicked(HWND hDlg, UINT messg, WPARAM wParam, LPARAM lParam)
{
	switch (LOWORD(wParam))
	{
	case IDC_CHOOSE:
	{
		wchar_t *pszFilePath = SelectFile(L"Choose program to run", NULL, NULL, hDlg, L"Executables (*.exe)\0*.exe\0All files (*.*)\0*.*\0\0", sff_AutoQuote);
		if (pszFilePath)
		{
			SetDlgItemText(hDlg, IDC_RESTART_CMD, pszFilePath);
			SafeFree(pszFilePath);
		}
		return TRUE;
	} // case IDC_CHOOSE:

	case IDC_CHOOSE_DIR:
	{
		wchar_t* pszDefFolder = GetDlgItemTextPtr(hDlg, IDC_STARTUP_DIR);
		wchar_t* pszFolder = SelectFolder(L"Choose startup directory", pszDefFolder, hDlg, sff_Default);
		if (pszFolder)
		{
			SetDlgItemText(hDlg, IDC_STARTUP_DIR, pszFolder);
			SafeFree(pszFolder);
		}
		SafeFree(pszDefFolder);
		return TRUE;
	} // case IDC_CHOOSE_DIR:

	case cbRunAsAdmin:
	{
		// BCM_SETSHIELD = 5644
		BOOL bRunAs = SendDlgItemMessage(hDlg, cbRunAsAdmin, BM_GETCHECK, 0, 0);

		if (gOSVer.dwMajorVersion >= 6)
		{
			SendDlgItemMessage(hDlg, IDC_START, 5644/*BCM_SETSHIELD*/, 0, bRunAs && (mp_Args->aRecreate != cra_EditTab));
		}

		if (bRunAs)
		{
			CheckRadioButton(hDlg, rbCurrentUser, rbAnotherUser, rbCurrentUser);
			CheckDlgButton(hDlg, cbRunAsRestricted, BST_UNCHECKED);
			RecreateDlgProc(hDlg, UM_USER_CONTROLS, 0, 0);
		}

		return TRUE;
	} // case cbRunAsAdmin:

	case rbCurrentUser:
	case rbAnotherUser:
	case cbRunAsNetOnly:
	case cbRunAsRestricted:
	{
		RecreateDlgProc(hDlg, UM_USER_CONTROLS, LOWORD(wParam), 0);
		return TRUE;
	}

	case rbRecreateSplitNone:
	case rbRecreateSplit2Right:
	case rbRecreateSplit2Bottom:
	{
		RConStartArgs* pArgs = mp_Args;
		switch (LOWORD(wParam))
		{
		case rbRecreateSplitNone:
			pArgs->eSplit = RConStartArgs::eSplitNone; break;
		case rbRecreateSplit2Right:
			pArgs->eSplit = RConStartArgs::eSplitHorz; break;
		case rbRecreateSplit2Bottom:
			pArgs->eSplit = RConStartArgs::eSplitVert; break;
		}
		EnableWindow(GetDlgItem(hDlg, tRecreateSplit), (pArgs->eSplit != pArgs->eSplitNone));
		EnableWindow(GetDlgItem(hDlg, stRecreateSplit), (pArgs->eSplit != pArgs->eSplitNone));
		if (pArgs->eSplit != pArgs->eSplitNone)
			SetFocus(GetDlgItem(hDlg, tRecreateSplit));
		return TRUE;
	} // case rbRecreateSplitXXX

	case IDC_START:
	{
		RConStartArgs* pArgs = mp_Args;
		_ASSERTE(pArgs);
		SafeFree(pArgs->pszUserName);
		SafeFree(pArgs->pszDomain);

		//SafeFree(pArgs->pszUserPassword);
		if (SendDlgItemMessage(hDlg, rbAnotherUser, BM_GETCHECK, 0, 0))
		{
			pArgs->RunAsRestricted = crb_Off;
			pArgs->pszUserName = GetDlgItemTextPtr(hDlg, tRunAsUser);
			pArgs->RunAsNetOnly = SendDlgItemMessage(hDlg, cbRunAsNetOnly, BM_GETCHECK, 0, 0) ? crb_On : crb_Off;

			if (pArgs->pszUserName)
			{
				//pArgs->pszUserPassword = GetDlgItemText(hDlg, tRunAsPassword);
				// Попытаться проверить правильность введенного пароля и возможность запуска
				bool bCheckPwd = pArgs->CheckUserToken(GetDlgItem(hDlg, tRunAsPassword));
				DWORD nErr = bCheckPwd ? 0 : GetLastError();
				if (!bCheckPwd)
				{
					DisplayLastError(L"Invalid user name or password was specified!", nErr, MB_ICONSTOP, NULL, hDlg);
					return 1;
				}
			}
		}
		else
		{
			pArgs->RunAsRestricted = SendDlgItemMessage(hDlg, cbRunAsRestricted, BM_GETCHECK, 0, 0) ? crb_On : crb_Off;
			pArgs->RunAsNetOnly = crb_Off;
		}

		// Vista+ (As Admin...)
		pArgs->RunAsAdministrator = SendDlgItemMessage(hDlg, cbRunAsAdmin, BM_GETCHECK, 0, 0) ? crb_On : crb_Off;

		// StartupDir (may be specified as argument)
		wchar_t* pszDir = GetDlgItemTextPtr(hDlg, IDC_STARTUP_DIR);
		wchar_t* pszExpand = (pszDir && wcschr(pszDir, L'%')) ? ExpandEnvStr(pszDir) : NULL;
		LPCWSTR pszDirResult = pszExpand ? pszExpand : pszDir;
		// Another user? We may fail with access denied. Check only for "current user" account
		if (!pArgs->pszUserName && pszDirResult && *pszDirResult && !DirectoryExists(pszDirResult))
		{
			wchar_t* pszErrInfo = lstrmerge(L"Specified directory does not exists!\n", pszDirResult, L"\n" L"Do you want to choose another directory?\n\n");
			DWORD nErr = GetLastError();
			int iDirBtn = DisplayLastError(pszErrInfo, nErr, MB_ICONEXCLAMATION|MB_YESNO, NULL, hDlg);
			if (iDirBtn == IDYES)
			{
				SafeFree(pszDir);
				SafeFree(pszExpand);
				SafeFree(pszErrInfo);
				return 1;
			}
			// User want to run "as is". Most likely it will fail, but who knows...
		}
		SafeFree(pArgs->pszStartupDir);
		pArgs->pszStartupDir = pszExpand ? pszExpand : pszDir;
		if (pszExpand)
		{
			SafeFree(pszDir)
		}

		// Command
		// pszSpecialCmd мог быть передан аргументом - умолчание для строки ввода
		SafeFree(pArgs->pszSpecialCmd);

		// GetDlgItemText выделяет память через calloc
		pArgs->pszSpecialCmd = GetDlgItemTextPtr(hDlg, IDC_RESTART_CMD);

		if (pArgs->pszSpecialCmd)
			gpSet->HistoryAdd(pArgs->pszSpecialCmd);

		// Especially to reset properly RunAsSystem in active console
		pArgs->ProcessNewConArg();
		if (pArgs->RunAsSystem == crb_Undefined)
			pArgs->RunAsSystem = crb_Off;

		if ((pArgs->aRecreate != cra_RecreateTab) && (pArgs->aRecreate != cra_EditTab))
		{
			if (SendDlgItemMessage(hDlg, cbRunInNewWindow, BM_GETCHECK, 0, 0))
				pArgs->aRecreate = cra_CreateWindow;
			else
				pArgs->aRecreate = cra_CreateTab;
		}
		if (((pArgs->aRecreate == cra_CreateTab) || (pArgs->aRecreate == cra_EditTab))
			&& (pArgs->eSplit != RConStartArgs::eSplitNone))
		{
			BOOL bOk = FALSE;
			int nPercent = GetDlgItemInt(hDlg, tRecreateSplit, &bOk, FALSE);
			if (bOk && (nPercent >= 1) && (nPercent <= 99))
			{
				pArgs->nSplitValue = (100-nPercent) * 10;
			}
			//pArgs->nSplitPane = 0; Сбрасывать не будем?
		}
		mn_DlgRc = IDC_START;
		EndDialog(hDlg, IDC_START);
		return TRUE;
	} // case IDC_START:

	case IDC_TERMINATE:
		mn_DlgRc = IDC_TERMINATE;
		EndDialog(hDlg, IDC_TERMINATE);
		return TRUE;

	case IDCANCEL:
		mn_DlgRc = IDCANCEL;
		EndDialog(hDlg, IDCANCEL);
		return TRUE;
	}

	return FALSE;
}

INT_PTR CRecreateDlg::OnEditSetFocus(HWND hDlg, UINT messg, WPARAM wParam, LPARAM lParam)
{
	switch (LOWORD(wParam))
	{
	case tRecreateSplit:
	case tRunAsPassword:
		PostMessage((HWND)lParam, EM_SETSEL, 0, SendMessage((HWND)lParam, WM_GETTEXTLENGTH, 0,0));
		break;
	}

	return FALSE;
}

INT_PTR CRecreateDlg::OnComboSetFocus(HWND hDlg, UINT messg, WPARAM wParam, LPARAM lParam)
{
	switch (LOWORD(wParam))
	{
	case IDC_RESTART_CMD:
		if (mb_DontAutoSelCmd)
		{
			PostMessage((HWND)lParam, CB_SETEDITSEL, 0, MAKELPARAM(-1,0));
			return TRUE;
		}
		break;
	}

	return FALSE;
}

INT_PTR CRecreateDlg::RecreateDlgProc(HWND hDlg, UINT messg, WPARAM wParam, LPARAM lParam)
{
	CRecreateDlg* pDlg = NULL;
	if (messg == WM_INITDIALOG)
	{
		pDlg = (CRecreateDlg*)lParam;
		pDlg->mh_Dlg = hDlg;
		SetWindowLongPtr(hDlg, DWLP_USER, lParam);
	}
	else
	{
		pDlg = (CRecreateDlg*)GetWindowLongPtr(hDlg, DWLP_USER);
	}
	if (!pDlg)
	{
		return FALSE;
	}

	PatchMsgBoxIcon(hDlg, messg, wParam, lParam);

	switch (messg)
	{
		case WM_INITDIALOG:
			return pDlg->OnInitDialog(hDlg, messg, wParam, lParam);

		case (WM_APP+1):
			//TODO: Не совсем корректно, не учитывается предыдущее значение флажка
			gpConEmu->SetSkipOnFocus(false);
			return FALSE;

		case WM_CTLCOLORSTATIC:
			return pDlg->OnCtlColorStatic(hDlg, messg, wParam, lParam);

		case UM_FILL_CMDLIST:
			return pDlg->OnFillCmdList(hDlg, messg, wParam, lParam);

		case UM_USER_CONTROLS:
			return pDlg->OnUserControls(hDlg, messg, wParam, lParam);

		case WM_SYSCOMMAND:
			return pDlg->OnSysCommand(hDlg, messg, wParam, lParam);

		case WM_COMMAND:
			switch (HIWORD(wParam))
			{
			case BN_CLICKED:
				return pDlg->OnButtonClicked(hDlg, messg, wParam, lParam);

			case EN_SETFOCUS:
				if (lParam)
					return pDlg->OnEditSetFocus(hDlg, messg, wParam, lParam);
				break;

			case CBN_SETFOCUS:
				if (lParam)
					return pDlg->OnComboSetFocus(hDlg, messg, wParam, lParam);
				break;

			// TODO: gh-959: enable autocorrection of cbRunAsAdmin by IDC_RESTART_CMD task contents (first line)

			} // switch (HIWORD(wParam))
			break;

		case WM_CLOSE:
			gpConEmu->OnOurDialogClosed();
			break;

		case WM_DESTROY:
			if (pDlg->mp_DpiAware)
				pDlg->mp_DpiAware->Detach();
			break;

		default:
			if (pDlg->mp_DpiAware && pDlg->mp_DpiAware->ProcessDpiMessages(hDlg, messg, wParam, lParam))
			{
				return TRUE;
			}
	}

	return 0;
}

int CRecreateDlg::BrowseCallbackProc(HWND hwnd, UINT uMsg, LPARAM lParam, LPARAM lpData)
{
	if (uMsg==BFFM_INITIALIZED)
	{
		if (lpData && *(LPCWSTR)lpData)
		{
			SendMessage(hwnd, BFFM_SETSELECTION, TRUE, lpData);
		}
	}

	return 0;
}

void CRecreateDlg::InitVars()
{
	if (mpsz_DefCmd || mpsz_CurCmd || mpsz_SysCmd || mpsz_DefDir || !ms_RConStartDir.IsEmpty() || !ms_RConCurDir.IsEmpty())
	{
		_ASSERTE(!(mpsz_DefCmd || mpsz_CurCmd || mpsz_SysCmd || mpsz_DefDir || !ms_RConStartDir.IsEmpty() || !ms_RConCurDir.IsEmpty()));
		FreeVars();
	}

	CVConGuard VCon;
	CVirtualConsole* pVCon = (gpConEmu->GetActiveVCon(&VCon) >= 0) ? VCon.VCon() : NULL;
	CRealConsole* pRCon = pVCon ? pVCon->RCon() : NULL;

	if (pRCon)
	{
		ms_RConStartDir.Set(pRCon->GetStartupDir());
		pRCon->GetConsoleCurDir(ms_RConCurDir);
	}

	// Если уже передана команда через параметры - из текущей консоли не извлекать
	if (!mp_Args || mp_Args->pszSpecialCmd)
	{
		_ASSERTE(mp_Args!=NULL);
		return;
	}

	// AutoStartTaskName - не возвращаем никогда.
	//   Если он выбран при старте - то либо текущая консоль, либо первая команда из AutoStartTaskName, либо команда по умолчанию
	// Диалог может быть вызван в следующих случаях
	// * Recreate
	// * Свободный выбор = cra_EditTab (добавление новой команды в task или выбор шелла при обломе на старте)
	// * Ни одной консоли нет (предложить то что запускается при старте - Task, команда)
	// * Консоли есть (пусть наверное будет то что запускается при старте - Task, команда)

	wchar_t* pszBuf = NULL;

	_ASSERTE(pRCon || (mp_Args->aRecreate == cra_CreateTab || mp_Args->aRecreate == cra_EditTab));

	LPCWSTR pszCmd = pRCon ? pRCon->GetCmd() : NULL;
	if (pszCmd && *pszCmd)
		mpsz_CurCmd = lstrdup(pszCmd);

	LPCWSTR pszSystem = gpConEmu->GetCmd();
	if (pszSystem && *pszSystem && (lstrcmpi(pszSystem, AutoStartTaskName) != 0))
		mpsz_SysCmd = lstrdup(pszSystem);

	bool bDirFromRCon = false;

	if (mp_Args->aRecreate == cra_RecreateTab)
	{
		// When recreate - always show active console command line
		_ASSERTE(pRCon);
		mpsz_DefCmd = lstrdup(pszCmd);
		bDirFromRCon = true;
	}
	else
	{
		// В диалоге запуска новой консоли - нечего делать автостартующему таску?
		if (pszSystem && lstrcmpi(pszSystem, AutoStartTaskName) == 0)
		{
			// Раз активной консоли нет - попробовать взять первую команду из AutoStartTaskName
			if (!pszCmd)
			{
				pszBuf = gpConEmu->LoadConsoleBatch(AutoStartTaskName);
				wchar_t* pszLine = wcschr(pszBuf, L'\n');
				if (pszLine > pszBuf && *(pszLine-1) == L'\r')
					pszLine--;
				if (pszLine)
					*pszLine = 0;

				pszCmd = gpConEmu->ParseScriptLineOptions(pszBuf, NULL, mp_Args);
			}
			else
			{
				bDirFromRCon = true;
			}
		}
		else if (pszSystem && *pszSystem)
		{
			pszCmd = pszSystem;
		}
		else
		{
			bDirFromRCon = true;
		}

		mpsz_DefCmd = lstrdup(pszCmd);
		//mpsz_DefDir = lstrdup(mp_Args->pszStartupDir ? mp_Args->pszStartupDir : L"");
	}

	if (bDirFromRCon)
	{
		TODO("May be try to retrieve current directory of the shell?");
		mpsz_DefDir = lstrdup(ms_RConCurDir);
	}

	SafeFree(pszBuf);
}

void CRecreateDlg::FreeVars()
{
	SafeFree(mpsz_DefCmd);
	SafeFree(mpsz_CurCmd);
	SafeFree(mpsz_SysCmd);
	SafeFree(mpsz_DefDir);
	ms_RConStartDir.Empty();
	ms_RConCurDir.Empty();
}

void CRecreateDlg::AddCommandList(LPCWSTR asCommand, INT_PTR iAfter /*= -1*/)
{
	if (!asCommand || !*asCommand)
		return;
	if (lstrcmpi(asCommand, AutoStartTaskName) == 0)
		return;

	//Already exist?
	INT_PTR nId = SendDlgItemMessage(mh_Dlg, IDC_RESTART_CMD, CB_FINDSTRINGEXACT, -1, (LPARAM)asCommand);

	if (nId < 0)
	{
		SendDlgItemMessage(mh_Dlg, IDC_RESTART_CMD, CB_INSERTSTRING, iAfter, (LPARAM)asCommand);
	}
}

void CRecreateDlg::AddDirectoryList(LPCWSTR asDirectory, INT_PTR iAfter /*= -1*/)
{
	if (!asDirectory || !*asDirectory)
		return;

	//Already exist?
	INT_PTR nId = SendDlgItemMessage(mh_Dlg, IDC_STARTUP_DIR, CB_FINDSTRINGEXACT, -1, (LPARAM)asDirectory);

	if (nId < 0)
	{
		SendDlgItemMessage(mh_Dlg, IDC_STARTUP_DIR, CB_INSERTSTRING, iAfter, (LPARAM)asDirectory);
	}
}
