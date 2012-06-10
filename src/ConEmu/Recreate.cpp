
/*
Copyright (c) 2009-2012 Maximus5
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
#include <lm.h>
#include <ShlObj.h>
#include "ConEmu.h"
#include "Recreate.h"
#include "VirtualConsole.h"
#include "RealConsole.h"
#include "../common/WinObjects.h"

CRecreateDlg::CRecreateDlg()
	: mh_Dlg(NULL)
	, mn_DlgRc(0)
	, mp_Args(NULL)
{
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
int CRecreateDlg::RecreateDlg(RConStartArgs* apArgs)
{
	if (!this)
	{
		_ASSERTE(this);
		return IDCANCEL;
	}

	if (mh_Dlg && IsWindow(mh_Dlg))
	{
		_ASSERTE(mh_Dlg == NULL);
		return IDCANCEL;
	}
	
	BOOL b = gbDontEnable;
	gbDontEnable = TRUE;

	gpConEmu->SkipOneAppsRelease(true);

	//if (!gpConEmu->mh_RecreatePasswFont)
	//{
	//	gpConEmu->mh_RecreatePasswFont = CreateFont(

	//}

	mn_DlgRc = IDCANCEL;
	mp_Args = apArgs;

	gpConEmu->SetSkipOnFocus(TRUE);
	int nRc = DialogBoxParam(g_hInstance, MAKEINTRESOURCE(IDD_RESTART), ghWnd, RecreateDlgProc, (LPARAM)this);
	UNREFERENCED_PARAMETER(nRc);
	gpConEmu->SetSkipOnFocus(FALSE);

	//if (gpConEmu->mh_RecreatePasswFont)
	//{
	//	DeleteObject(gpConEmu->mh_RecreatePasswFont);
	//	gpConEmu->mh_RecreatePasswFont = NULL;
	//}

	gpConEmu->SkipOneAppsRelease(false);

	gbDontEnable = b;
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
}

INT_PTR CRecreateDlg::RecreateDlgProc(HWND hDlg, UINT messg, WPARAM wParam, LPARAM lParam)
{
#define UM_USER_CONTROLS (WM_USER+121)
#define UM_FILL_CMDLIST (WM_USER+122)

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

	switch (messg)
	{
		case WM_INITDIALOG:
		{
			LRESULT lbRc = FALSE;
			

			SendMessage(hDlg, WM_SETICON, ICON_BIG, (LPARAM)hClassIcon);
			SendMessage(hDlg, WM_SETICON, ICON_SMALL, (LPARAM)hClassIconSm);

			HMENU hSysMenu = GetSystemMenu(hDlg, FALSE);
			InsertMenu(hSysMenu, 0, MF_BYPOSITION, MF_SEPARATOR, 0);
			InsertMenu(hSysMenu, 0, MF_BYPOSITION | MF_STRING | MF_ENABLED
					   | ((GetWindowLong(ghOpWnd,GWL_EXSTYLE)&WS_EX_TOPMOST) ? MF_CHECKED : 0),
					   ID_RESETCMDHISTORY, _T("Reset command history..."));


			SendDlgItemMessage(hDlg, tRunAsPassword, WM_SETFONT, (LPARAM)(HFONT)GetStockObject(DEFAULT_GUI_FONT), 0);

			//#ifdef _DEBUG
			//SetWindowPos(ghOpWnd, HWND_NOTOPMOST, 0,0,0,0, SWP_NOSIZE|SWP_NOMOVE);
			//#endif

			SendMessage(hDlg, UM_FILL_CMDLIST, TRUE, 0);
			
			RConStartArgs* pArgs = pDlg->mp_Args;
			_ASSERTE(pArgs);
			LPCWSTR pszCmd = pArgs->pszSpecialCmd
			                 ? pArgs->pszSpecialCmd
			                 : gpConEmu->ActiveCon()->RCon()->GetCmd();
			//int nId = SendDlgItemMessage(hDlg, IDC_RESTART_CMD, CB_FINDSTRINGEXACT, -1, (LPARAM)pszCmd);
			//if (nId < 0) SendDlgItemMessage(hDlg, IDC_RESTART_CMD, CB_INSERTSTRING, 0, (LPARAM)pszCmd);
			LPCWSTR pszSystem = gpSet->GetCmd();
			//if (pszSystem != pszCmd && (pszSystem && pszCmd && (lstrcmpi(pszSystem, pszCmd) != 0)))
			//{
			//	nId = SendDlgItemMessage(hDlg, IDC_RESTART_CMD, CB_FINDSTRINGEXACT, -1, (LPARAM)pszSystem);
			//	if (nId < 0) SendDlgItemMessage(hDlg, IDC_RESTART_CMD, CB_INSERTSTRING, pArgs->pszSpecialCmd ? -1 : 0, (LPARAM)pszSystem);
			//}
			//LPCWSTR pszHistory = gpSet->HistoryGet();
			//if (pszHistory)
			//{
			//	while (*pszHistory)
			//	{
			//		nId = SendDlgItemMessage(hDlg, IDC_RESTART_CMD, CB_FINDSTRINGEXACT, -1, (LPARAM)pszHistory);
			//		if (nId < 0)
			//			SendDlgItemMessage(hDlg, IDC_RESTART_CMD, CB_INSERTSTRING, -1, (LPARAM)pszHistory);
			//		pszHistory += _tcslen(pszHistory)+1;
			//	}
			//}
			////// Обновить группы команд
			////gpSet->LoadCmdTasks(NULL);
			//int nGroup = 0;
			//const Settings::CommandTasks* pGrp = NULL;
			//while ((pGrp = gpSet->CmdTaskGet(nGroup++)))
			//{
			//	nId = SendDlgItemMessage(hDlg, IDC_RESTART_CMD, CB_FINDSTRINGEXACT, -1, (LPARAM)pGrp->pszName);
			//	if (nId < 0)
			//		SendDlgItemMessage(hDlg, IDC_RESTART_CMD, CB_INSERTSTRING, -1, (LPARAM)pGrp->pszName);
			//}

			if (pArgs->aRecreate == cra_RecreateTab)
			{
				SetDlgItemText(hDlg, IDC_RESTART_CMD, pszCmd);
				SetDlgItemText(hDlg, IDC_STARTUP_DIR, gpConEmu->ActiveCon()->RCon()->GetDir());
			}
			else
			{
				SetDlgItemText(hDlg, IDC_RESTART_CMD, pArgs->pszSpecialCmd ? pArgs->pszSpecialCmd : pszSystem);
				SetDlgItemText(hDlg, IDC_STARTUP_DIR, pArgs->pszStartupDir ? pArgs->pszStartupDir : L"");
			}
			//EnableWindow(GetDlgItem(hDlg, IDC_STARTUP_DIR), FALSE);
			//#ifndef _DEBUG
			//EnableWindow(GetDlgItem(hDlg, IDC_CHOOSE_DIR), FALSE);
			//#endif
			const wchar_t *pszUser, *pszDomain; BOOL bResticted;
			int nChecked = rbCurrentUser;
			wchar_t szCurUser[MAX_PATH*2+1]; DWORD nUserNameLen = countof(szCurUser);

			if (!GetUserName(szCurUser, &nUserNameLen)) szCurUser[0] = 0;

			wchar_t szRbCaption[MAX_PATH*3];
			lstrcpy(szRbCaption, L"Run as current &user: "); lstrcat(szRbCaption, szCurUser);
			SetDlgItemText(hDlg, rbCurrentUser, szRbCaption);

			if ((pArgs->aRecreate == cra_RecreateTab) && gpConEmu->ActiveCon()->RCon()->GetUserPwd(&pszUser, &pszDomain, &bResticted))
			{
				nChecked = rbAnotherUser;

				if (bResticted)
				{
					CheckDlgButton(hDlg, cbRunAsRestricted, BST_CHECKED);
				}
				else
				{
					if (pszDomain)
					{
						if (wcschr(pszDomain, L'.'))
						{
							// Если в имени домена есть точка - используем нотацию user@domain
							// По идее, мы сюда не попадаем, т.к. при вводе имени в таком формате
							// pszDomain не заполняется, и "UPN format" остается в pszUser
							lstrcpyn(szCurUser, pszUser, MAX_PATH);
							wcscat_c(szCurUser, L"@");
							lstrcpyn(szCurUser+_tcslen(szCurUser), pszDomain, MAX_PATH);
						}
						else
						{
							// "Старая" нотация domain\user
							lstrcpyn(szCurUser, pszDomain, MAX_PATH);
							wcscat_c(szCurUser, L"\\");
							lstrcpyn(szCurUser+_tcslen(szCurUser), pszUser, MAX_PATH);
						}
					}
					else
					{
						lstrcpyn(szCurUser, pszUser, countof(szCurUser));
					}
					SetDlgItemText(hDlg, tRunAsPassword, L"");
				}
			}

			SetDlgItemText(hDlg, tRunAsUser, szCurUser);
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
			else if (gpConEmu->mb_IsUacAdmin || (pArgs && pArgs->bRunAsAdministrator))
			{
				CheckDlgButton(hDlg, cbRunAsAdmin, BST_CHECKED);

				if (gpConEmu->mb_IsUacAdmin)  // Только в Vista+ если GUI уже запущен под админом
				{
					EnableWindow(GetDlgItem(hDlg, cbRunAsAdmin), FALSE);
				}
				else if (gOSVer.dwMajorVersion < 6)
				{
					RecreateDlgProc(hDlg, WM_COMMAND, cbRunAsAdmin, 0);
				}
			}

			//}
			SetClassLongPtr(hDlg, GCLP_HICON, (LONG_PTR)hClassIcon);

			RECT rcBtnBox = {0};
			if (pArgs->aRecreate == cra_RecreateTab)
			{
				//GCC hack. иначе не собирается
				SetDlgItemTextA(hDlg, IDC_RESTART_MSG, "About to recreate console");
				SendDlgItemMessage(hDlg, IDC_RESTART_ICON, STM_SETICON, (WPARAM)LoadIcon(NULL,IDI_EXCLAMATION), 0);
				// Выровнять флажок по кнопке
				GetWindowRect(GetDlgItem(hDlg, IDC_START), &rcBtnBox);
				// Спрятать флажок "New window"
				ShowWindow(GetDlgItem(hDlg, cbRunInNewWindow), SW_HIDE);
				lbRc = TRUE;
			}
			else
			{
				//GCC hack. иначе не собирается
				SetDlgItemTextA(hDlg, IDC_RESTART_MSG,  "Create new console");

				CheckDlgButton(hDlg, cbRunInNewWindow, (pArgs->aRecreate == cra_CreateWindow) ? BST_CHECKED : BST_UNCHECKED);
				//if (pArgs->aRecreate == cra_CreateWindow)
				//{
				//	SetWindowText(hDlg, L"ConEmu - Create new window");
				//	//GCC hack. иначе не собирается
				//	SetDlgItemTextA(hDlg, IDC_RESTART_MSG,  "Create new window");
				//}
				//else
				//{
				//	//GCC hack. иначе не собирается
				//	SetDlgItemTextA(hDlg, IDC_RESTART_MSG,  "Create new console");
				//}

				SendDlgItemMessage(hDlg, IDC_RESTART_ICON, STM_SETICON, (WPARAM)LoadIcon(NULL,IDI_QUESTION), 0);
				POINT pt = {0,0};
				MapWindowPoints(GetDlgItem(hDlg, IDC_TERMINATE), hDlg, &pt, 1);
				DestroyWindow(GetDlgItem(hDlg, IDC_TERMINATE));
				SetWindowPos(GetDlgItem(hDlg, IDC_START), NULL, pt.x, pt.y, 0,0, SWP_NOSIZE|SWP_NOZORDER);
				SetDlgItemText(hDlg, IDC_START, L"&Start");
				DestroyWindow(GetDlgItem(hDlg, IDC_WARNING));
				// Выровнять флажок по кнопке
				GetWindowRect(GetDlgItem(hDlg, IDC_START), &rcBtnBox);
				SetFocus(GetDlgItem(hDlg, IDC_RESTART_CMD));
			}

			if (rcBtnBox.left)
			{
				// Выровнять флажок по кнопке
				MapWindowPoints(NULL, hDlg, (LPPOINT)&rcBtnBox, 2);
				RECT rcBox; GetWindowRect(GetDlgItem(hDlg, cbRunAsAdmin), &rcBox);
				POINT pt;
				pt.x = rcBtnBox.left - (rcBox.right - rcBox.left) - 5;
				pt.y = rcBtnBox.top + ((rcBtnBox.bottom-rcBtnBox.top) - (rcBox.bottom-rcBox.top))/2;
				SetWindowPos(GetDlgItem(hDlg, cbRunAsAdmin), NULL, pt.x, pt.y, 0,0, SWP_NOSIZE|SWP_NOZORDER);
				SetFocus(GetDlgItem(hDlg, IDC_RESTART_CMD));
			}

			if (pArgs->aRecreate != cra_RecreateTab)
			{
				POINT pt = {};
				MapWindowPoints(GetDlgItem(hDlg, cbRunAsAdmin), hDlg, &pt, 1);
				RECT rcBox2; GetWindowRect(GetDlgItem(hDlg, cbRunInNewWindow), &rcBox2);
				SetWindowPos(GetDlgItem(hDlg, cbRunInNewWindow), NULL,
					pt.x-(rcBox2.right-rcBox2.left), pt.y, 0,0, SWP_NOSIZE);
			}

			RECT rect;
			GetWindowRect(hDlg, &rect);
			RECT rcParent;
			GetWindowRect(ghWnd, &rcParent);
			MoveWindow(hDlg,
			           (rcParent.left+rcParent.right-rect.right+rect.left)/2,
			           (rcParent.top+rcParent.bottom-rect.bottom+rect.top)/2,
			           rect.right - rect.left, rect.bottom - rect.top, false);

			PostMessage(hDlg, (WM_APP+1), 0,0);
			return lbRc;
		}
		case (WM_APP+1):
			gpConEmu->SetSkipOnFocus(FALSE);
			return FALSE;
		case WM_CTLCOLORSTATIC:

			if (GetDlgItem(hDlg, IDC_WARNING) == (HWND)lParam)
			{
				SetTextColor((HDC)wParam, 255);
				HBRUSH hBrush = GetSysColorBrush(COLOR_3DFACE);
				SetBkMode((HDC)wParam, TRANSPARENT);
				return (INT_PTR)hBrush;
			}

			break;
		//case WM_GETICON:

		//	if (wParam==ICON_BIG)
		//	{
		//		/*SetWindowLong(hWnd2, DWL_MSGRESULT, (LRESULT)hClassIcon);
		//		return 1;*/
		//	}
		//	else
		//	{
		//		SetWindowLongPtr(hDlg, DWLP_MSGRESULT, (LRESULT)hClassIconSm);
		//		return 1;
		//	}

		//	return 0;

		case UM_FILL_CMDLIST:
		{
			RConStartArgs* pArgs = pDlg->mp_Args;
			_ASSERTE(pArgs);

			LPCWSTR pszCmd = pArgs->pszSpecialCmd
			                 ? pArgs->pszSpecialCmd
			                 : gpConEmu->ActiveCon()->RCon()->GetCmd();
			int nId = SendDlgItemMessage(hDlg, IDC_RESTART_CMD, CB_FINDSTRINGEXACT, -1, (LPARAM)pszCmd);

			if (nId < 0) SendDlgItemMessage(hDlg, IDC_RESTART_CMD, CB_INSERTSTRING, 0, (LPARAM)pszCmd);

			LPCWSTR pszSystem = gpSet->GetCmd();

			if (pszSystem != pszCmd && (pszSystem && pszCmd && (lstrcmpi(pszSystem, pszCmd) != 0)))
			{
				nId = SendDlgItemMessage(hDlg, IDC_RESTART_CMD, CB_FINDSTRINGEXACT, -1, (LPARAM)pszSystem);

				if (nId < 0) SendDlgItemMessage(hDlg, IDC_RESTART_CMD, CB_INSERTSTRING, pArgs->pszSpecialCmd ? -1 : 0, (LPARAM)pszSystem);
			}

			if (wParam)
			{
				LPCWSTR pszHistory = gpSet->HistoryGet();

				if (pszHistory)
				{
					while (*pszHistory)
					{
						nId = SendDlgItemMessage(hDlg, IDC_RESTART_CMD, CB_FINDSTRINGEXACT, -1, (LPARAM)pszHistory);
						if (nId < 0)
							SendDlgItemMessage(hDlg, IDC_RESTART_CMD, CB_INSERTSTRING, -1, (LPARAM)pszHistory);

						pszHistory += _tcslen(pszHistory)+1;
					}
				}
			}

			//// Обновить группы команд
			//gpSet->LoadCmdTasks(NULL);

			int nGroup = 0;
			const Settings::CommandTasks* pGrp = NULL;
			while ((pGrp = gpSet->CmdTaskGet(nGroup++)))
			{
				nId = SendDlgItemMessage(hDlg, IDC_RESTART_CMD, CB_FINDSTRINGEXACT, -1, (LPARAM)pGrp->pszName);
				if (nId < 0)
					SendDlgItemMessage(hDlg, IDC_RESTART_CMD, CB_INSERTSTRING, -1, (LPARAM)pGrp->pszName);
			}
		}
		return 0;

		case UM_USER_CONTROLS:
		{
			if (SendDlgItemMessage(hDlg, rbCurrentUser, BM_GETCHECK, 0, 0))
			{
				EnableWindow(GetDlgItem(hDlg, cbRunAsRestricted), TRUE);
				//BOOL lbText = SendDlgItemMessage(hDlg, cbRunAsRestricted, BM_GETCHECK, 0, 0) == 0;
				EnableWindow(GetDlgItem(hDlg, tRunAsUser), FALSE);
				EnableWindow(GetDlgItem(hDlg, tRunAsPassword), FALSE);
			}
			else
			{
				if (SendDlgItemMessage(hDlg, tRunAsUser, CB_GETCOUNT, 0, 0) == 0)
				{
					DWORD dwLevel = 3, dwEntriesRead = 0, dwTotalEntries = 0, dwResumeHandle = 0;
					NET_API_STATUS nStatus;
					USER_INFO_3 *info = NULL;
					nStatus = ::NetUserEnum(NULL, dwLevel, FILTER_NORMAL_ACCOUNT, (PBYTE*) & info,
					                        MAX_PREFERRED_LENGTH, &dwEntriesRead, &dwTotalEntries, &dwResumeHandle);

					if (nStatus == NERR_Success)
					{
						for(DWORD i = 0; i < dwEntriesRead; ++i)
						{
							// usri3_logon_server	"\\*"	wchar_t *
							if ((info[i].usri3_flags & UF_ACCOUNTDISABLE) == 0)
								SendDlgItemMessage(hDlg, tRunAsUser, CB_ADDSTRING, 0, (LPARAM)info[i].usri3_name);
						}

						::NetApiBufferFree(info);
					}
					else
					{
						// Добавить хотя бы текущего
						wchar_t szCurUser[MAX_PATH];

						if (GetWindowText(GetDlgItem(hDlg, tRunAsUser), szCurUser, countof(szCurUser)))
							SendDlgItemMessage(hDlg, tRunAsUser, CB_ADDSTRING, 0, (LPARAM)szCurUser);
					}
				}

				EnableWindow(GetDlgItem(hDlg, cbRunAsRestricted), FALSE);
				EnableWindow(GetDlgItem(hDlg, tRunAsUser), TRUE);
				EnableWindow(GetDlgItem(hDlg, tRunAsPassword), TRUE);
			}

			if (wParam == rbAnotherUser)
				SetFocus(GetDlgItem(hDlg, tRunAsUser));
		}
		return 0;

		case WM_SYSCOMMAND:
			if (LOWORD(wParam) == ID_RESETCMDHISTORY)
			{
				if (IDYES == MessageBox(hDlg, L"Clear command history?", gpConEmu->GetDefaultTitle(), MB_ICONEXCLAMATION|MB_YESNO|MB_DEFBUTTON2))
				{
                	gpSet->HistoryReset();
                	wchar_t* pszCmd = GetDlgItemText(hDlg, IDC_RESTART_CMD);
                	SendDlgItemMessage(hDlg, IDC_RESTART_CMD, CB_RESETCONTENT, 0,0);
                	SendMessage(hDlg, UM_FILL_CMDLIST, FALSE, 0);
                	if (pszCmd)
                	{
                		SetDlgItemText(hDlg, IDC_RESTART_CMD, pszCmd);
                		free(pszCmd);
                	}
				}
				SetWindowLongPtr(hDlg, DWLP_MSGRESULT, 0);
				return 1;
			}
			break;

		case WM_COMMAND:

			if (HIWORD(wParam) == BN_CLICKED)
			{
				switch(LOWORD(wParam))
				{
					case IDC_CHOOSE:
					{
						wchar_t *pszFilePath = NULL;
						int nLen = MAX_PATH*2;
						pszFilePath = (wchar_t*)calloc(nLen+3,2); // +2*'"'+\0

						if (!pszFilePath) return 1;

						OPENFILENAME ofn; memset(&ofn,0,sizeof(ofn));
						ofn.lStructSize=sizeof(ofn);
						ofn.hwndOwner = hDlg;
						ofn.lpstrFilter = _T("Executables (*.exe)\0*.exe\0\0");
						ofn.nFilterIndex = 1;
						ofn.lpstrFile = pszFilePath+1;
						ofn.nMaxFile = nLen;
						ofn.lpstrTitle = _T("Choose program to run");
						ofn.Flags = OFN_ENABLESIZING|OFN_NOCHANGEDIR
						            | OFN_PATHMUSTEXIST|OFN_EXPLORER|OFN_HIDEREADONLY|OFN_FILEMUSTEXIST;

						if (GetOpenFileName(&ofn))
						{
							LPCWSTR pszNewText = pszFilePath + 1;
							if (wcschr(pszFilePath, L' '))
							{
								pszFilePath[0] = L'"'; _wcscat_c(pszFilePath, nLen+3, L"\"");
								pszNewText = pszFilePath;
							}
							SetDlgItemText(hDlg, IDC_RESTART_CMD, pszNewText);
						}

						SafeFree(pszFilePath);
						return 1;
					}
					case IDC_CHOOSE_DIR:
					{
						BROWSEINFO bi = {ghWnd};
						wchar_t szFolder[MAX_PATH+1] = {0};
						GetDlgItemText(hDlg, IDC_STARTUP_DIR, szFolder, countof(szFolder));
						bi.pszDisplayName = szFolder;
						wchar_t szTitle[100];
						bi.lpszTitle = wcscpy(szTitle, L"Choose startup directory");
						bi.ulFlags = BIF_EDITBOX | BIF_RETURNONLYFSDIRS | BIF_VALIDATE;
						bi.lpfn = BrowseCallbackProc;
						bi.lParam = (LPARAM)szFolder;
						LPITEMIDLIST pRc = SHBrowseForFolder(&bi);

						if (pRc)
						{
							if (SHGetPathFromIDList(pRc, szFolder))
							{
								SetDlgItemText(hDlg, IDC_STARTUP_DIR, szFolder);
							}

							CoTaskMemFree(pRc);
						}

						return 1;
					}
					case cbRunAsAdmin:
					{
						// BCM_SETSHIELD = 5644
						BOOL bRunAs = SendDlgItemMessage(hDlg, cbRunAsAdmin, BM_GETCHECK, 0, 0);

						if (gOSVer.dwMajorVersion >= 6)
						{
							SendDlgItemMessage(hDlg, IDC_START, 5644/*BCM_SETSHIELD*/, 0, bRunAs);
						}

						if (bRunAs)
						{
							CheckRadioButton(hDlg, rbCurrentUser, rbAnotherUser, rbCurrentUser);
							CheckDlgButton(hDlg, cbRunAsRestricted, BST_UNCHECKED);
							RecreateDlgProc(hDlg, UM_USER_CONTROLS, 0, 0);
						}

						return 1;
					}
					case rbCurrentUser:
					case rbAnotherUser:
					case cbRunAsRestricted:
					{
						RecreateDlgProc(hDlg, UM_USER_CONTROLS, LOWORD(wParam), 0);
						return 1;
					}
					case IDC_START:
					{
						RConStartArgs* pArgs = pDlg->mp_Args;
						_ASSERTE(pArgs);
						SafeFree(pArgs->pszUserName);
						SafeFree(pArgs->pszDomain);

						//SafeFree(pArgs->pszUserPassword);
						if (SendDlgItemMessage(hDlg, rbAnotherUser, BM_GETCHECK, 0, 0))
						{
							pArgs->bRunAsRestricted = FALSE;
							pArgs->pszUserName = GetDlgItemText(hDlg, tRunAsUser);

							if (pArgs->pszUserName)
							{
								//pArgs->pszUserPassword = GetDlgItemText(hDlg, tRunAsPassword);
								// Попытаться проверить правильность введенного пароля и возможность запуска
								if (!pArgs->CheckUserToken(GetDlgItem(hDlg, tRunAsPassword)))
									return 1;
							}
						}
						else
						{
							pArgs->bRunAsRestricted = SendDlgItemMessage(hDlg, cbRunAsRestricted, BM_GETCHECK, 0, 0);
						}

						// Command
						// pszSpecialCmd мог быть передан аргументом - умолчание для строки ввода
						SafeFree(pArgs->pszSpecialCmd);

						// GetDlgItemText выделяет память через calloc
						pArgs->pszSpecialCmd = GetDlgItemText(hDlg, IDC_RESTART_CMD);

						if (pArgs->pszSpecialCmd)
							gpSet->HistoryAdd(pArgs->pszSpecialCmd);

						// StartupDir (может быть передан аргументом)
						SafeFree(pArgs->pszStartupDir);
						pArgs->pszStartupDir = GetDlgItemText(hDlg, IDC_STARTUP_DIR);
						// Vista+ (As Admin...)
						pArgs->bRunAsAdministrator = SendDlgItemMessage(hDlg, cbRunAsAdmin, BM_GETCHECK, 0, 0);
						if (pArgs->aRecreate != cra_RecreateTab)
						{
							if (SendDlgItemMessage(hDlg, cbRunInNewWindow, BM_GETCHECK, 0, 0))
								pArgs->aRecreate = cra_CreateWindow;
							else
								pArgs->aRecreate = cra_CreateTab;
						}
						pDlg->mn_DlgRc = IDC_START;
						EndDialog(hDlg, IDC_START);
						return 1;
					}
					case IDC_TERMINATE:
						pDlg->mn_DlgRc = IDC_TERMINATE;
						EndDialog(hDlg, IDC_TERMINATE);
						return 1;
					case IDCANCEL:
						pDlg->mn_DlgRc = IDCANCEL;
						EndDialog(hDlg, IDCANCEL);
						return 1;
				}
			}

			break;
		default:
			return 0;
	}

	return 0;
}

int CRecreateDlg::BrowseCallbackProc(HWND hwnd, UINT uMsg, LPARAM lParam, LPARAM lpData)
{
	if (uMsg==BFFM_INITIALIZED)
	{
		SendMessage(hwnd, BFFM_SETSELECTION, TRUE, lpData);
	}

	return 0;
}
