
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

#include "OptionsClass.h"
#include "SetDlgLists.h"
#include "SetPgStartup.h"

CSetPgStartup::CSetPgStartup()
{
}

CSetPgStartup::~CSetPgStartup()
{
}

LRESULT CSetPgStartup::OnInitDialog(HWND hDlg, bool abInitial)
{
	PageDlgProc(hDlg, WM_INITDIALOG, 0, abInitial);

	return 0;
}

INT_PTR CSetPgStartup::PageDlgProc(HWND hDlg, UINT messg, WPARAM wParam, LPARAM lParam)
{
	INT_PTR iRc = 0;
	const wchar_t* csNoTask = L"<None>";
	#define MSG_SHOWTASKCONTENTS (WM_USER+64)

	switch (messg)
	{
	case WM_NOTIFY:
		{
			LPNMHDR phdr = (LPNMHDR)lParam;

			if (phdr->code == TTN_GETDISPINFO)
			{
				return gpSetCls->ProcessTipHelp(hDlg, messg, wParam, lParam);
			}

			break;
		}
	case WM_INITDIALOG:
		{
			BOOL bInitial = (lParam != 0); UNREFERENCED_PARAMETER(bInitial);

			checkRadioButton(hDlg, rbStartSingleApp, rbStartLastTabs, rbStartSingleApp+gpSet->nStartType);

			SetDlgItemText(hDlg, tCmdLine, gpSet->psStartSingleApp ? gpSet->psStartSingleApp : L"");

			// Признак "командного файла" - лидирующая @, в диалоге - не показываем
			SetDlgItemText(hDlg, tStartTasksFile, gpSet->psStartTasksFile ? (*gpSet->psStartTasksFile == CmdFilePrefix ? (gpSet->psStartTasksFile+1) : gpSet->psStartTasksFile) : L"");

			int nGroup = 0;
			const CommandTasks* pGrp = NULL;
			SendDlgItemMessage(hDlg, lbStartNamedTask, CB_RESETCONTENT, 0,0);
			SendDlgItemMessage(hDlg, lbStartNamedTask, CB_ADDSTRING, 0, (LPARAM)csNoTask);
			// Fill tasks from settings
			while ((pGrp = gpSet->CmdTaskGet(nGroup++)))
				SendDlgItemMessage(hDlg, lbStartNamedTask, CB_ADDSTRING, 0, (LPARAM)pGrp->pszName);
			// Select active task
			pGrp = gpSet->psStartTasksName ? gpSet->CmdTaskGetByName(gpSet->psStartTasksName) : NULL;
			if (!pGrp || !pGrp->pszName
				|| (CSetDlgLists::SelectStringExact(hDlg, lbStartNamedTask, pGrp->pszName) <= 0))
			{
				if (gpSet->nStartType == (rbStartNamedTask - rbStartSingleApp))
				{
					// 0 -- csNoTask
					// Задачи с таким именем больше нет - прыгаем на "Командную строку"
					gpSet->nStartType = 0;
					checkRadioButton(hDlg, rbStartSingleApp, rbStartLastTabs, rbStartSingleApp);
				}
			}

			SetDlgItemInt(hDlg, tStartCreateDelay, gpSet->nStartCreateDelay, FALSE);

			PageDlgProc(hDlg, WM_COMMAND, rbStartSingleApp+gpSet->nStartType, 0);
		}
		break;
	case WM_COMMAND:
		{
			switch (HIWORD(wParam))
			{
			case BN_CLICKED:
				{
					WORD CB = LOWORD(wParam);
					switch (CB)
					{
					case rbStartSingleApp:
					case rbStartTasksFile:
					case rbStartNamedTask:
					case rbStartLastTabs:
						gpSet->nStartType = (CB - rbStartSingleApp);
						//
						EnableWindow(GetDlgItem(hDlg, tCmdLine), (CB == rbStartSingleApp));
						EnableWindow(GetDlgItem(hDlg, cbCmdLine), (CB == rbStartSingleApp));
						//
						EnableWindow(GetDlgItem(hDlg, tStartTasksFile), (CB == rbStartTasksFile));
						EnableWindow(GetDlgItem(hDlg, cbStartTasksFile), (CB == rbStartTasksFile));
						//
						EnableWindow(GetDlgItem(hDlg, lbStartNamedTask), (CB == rbStartNamedTask));
						// -- not supported yet
						ShowWindow(GetDlgItem(hDlg, cbStartFarRestoreFolders), SW_HIDE);
						ShowWindow(GetDlgItem(hDlg, cbStartFarRestoreEditors), SW_HIDE);
						//
						EnableWindow(GetDlgItem(hDlg, tStartGroupCommands), (CB == rbStartNamedTask) || (CB == rbStartLastTabs));
						// Task source
						PageDlgProc(hDlg, MSG_SHOWTASKCONTENTS, CB, 0);
						break;
					case cbCmdLine:
					case cbStartTasksFile:
						{
							wchar_t temp[MAX_PATH+1], edt[MAX_PATH];
							if (!GetDlgItemText(hDlg, (CB==cbCmdLine)?tCmdLine:tStartTasksFile, edt, countof(edt)))
								edt[0] = 0;
							ExpandEnvironmentStrings(edt, temp, countof(temp));

							LPCWSTR pszFilter, pszTitle;
							if (CB==cbCmdLine)
							{
								pszFilter = L"Executables (*.exe)\0*.exe\0All files (*.*)\0*.*\0\0";
								pszTitle = L"Choose application";
							}
							else
							{
								pszFilter = L"Text files (*.txt)\0*.txt\0All files (*.*)\0*.*\0\0";
								pszTitle = L"Choose command file";
							}

							wchar_t* pszRet = SelectFile(pszTitle, temp, NULL, ghOpWnd, pszFilter, (CB==cbCmdLine)?sff_AutoQuote:sff_Default);

							if (pszRet)
							{
								SetDlgItemText(hDlg, (CB==cbCmdLine)?tCmdLine:tStartTasksFile, pszRet);
								SafeFree(pszRet);
							}
						}
						break;
					}
				} // BN_CLICKED
				break;
			case EN_CHANGE:
				{
					switch (LOWORD(wParam))
					{
					case tCmdLine:
						GetString(hDlg, tCmdLine, &gpSet->psStartSingleApp);
						break;
					case tStartTasksFile:
						{
							wchar_t* psz = NULL;
							INT_PTR nLen = GetString(hDlg, tStartTasksFile, &psz);
							if ((nLen <= 0) || !psz || !*psz)
							{
								SafeFree(gpSet->psStartTasksFile);
							}
							else
							{
								LPCWSTR pszName = (*psz == CmdFilePrefix) ? (psz+1) : psz;
								SafeFree(gpSet->psStartTasksFile);
								gpSet->psStartTasksFile = (wchar_t*)calloc(nLen+2,sizeof(*gpSet->psStartTasksFile));
								*gpSet->psStartTasksFile = CmdFilePrefix;
								_wcscpy_c(gpSet->psStartTasksFile+1, nLen+1, pszName);
							}
							SafeFree(psz);
						} // tStartTasksFile
						break;
					case tStartCreateDelay:
						{
							BOOL bDelayOk = FALSE;
							UINT nNewDelay = GetDlgItemInt(hDlg, tStartCreateDelay, &bDelayOk, FALSE);
							if (bDelayOk)
								gpSet->nStartCreateDelay = GetMinMax(nNewDelay, RUNQUEUE_CREATE_LAG_MIN, RUNQUEUE_CREATE_LAG_MAX);
							else
								gpSet->nStartCreateDelay = RUNQUEUE_CREATE_LAG_DEF;
						}
						break;
					}
				} // EN_CHANGE
				break;
			case CBN_SELCHANGE:
				{
					switch (LOWORD(wParam))
					{
					case lbStartNamedTask:
						{
							wchar_t* pszName = NULL;
							CSetDlgLists::GetSelectedString(hDlg, lbStartNamedTask, &pszName);
							if (pszName)
							{
								if (lstrcmp(pszName, csNoTask) != 0)
								{
									SafeFree(gpSet->psStartTasksName);
									gpSet->psStartTasksName = pszName;
								}
							}
							else
							{
								SafeFree(pszName);
								// If the Task does not exist - force to simple "Command line"
								gpSet->nStartType = 0;
								checkRadioButton(hDlg, rbStartSingleApp, rbStartLastTabs, rbStartSingleApp);
								PageDlgProc(hDlg, WM_COMMAND, rbStartSingleApp+gpSet->nStartType, 0);
							}

							// Show contents of the Task
							PageDlgProc(hDlg, MSG_SHOWTASKCONTENTS, gpSet->nStartType+rbStartSingleApp, 0);
						}
						break;
					}
				}
				break;
			}
		} // WM_COMMAND
		break;
	case MSG_SHOWTASKCONTENTS:
		if ((wParam == rbStartLastTabs) || (wParam == rbStartNamedTask))
		{
			if ((gpSet->nStartType == (rbStartLastTabs-rbStartSingleApp)) || (gpSet->nStartType == (rbStartNamedTask-rbStartSingleApp)))
			{
				int nIdx = -2;
				if (gpSet->nStartType == (rbStartLastTabs-rbStartSingleApp))
				{
					nIdx = -1;
				}
				else
				{
					nIdx = SendDlgItemMessage(hDlg, lbStartNamedTask, CB_GETCURSEL, 0, 0) - 1;
					if (nIdx == -1)
						nIdx = -2;
				}
				const CommandTasks* pTask = (nIdx >= -1) ? gpSet->CmdTaskGet(nIdx) : NULL;
				SetDlgItemText(hDlg, tStartGroupCommands, pTask ? pTask->pszCommands : L"");
			}
			else
			{
				SetDlgItemText(hDlg, tStartGroupCommands, L"");
			}
		}
		break;
	}

	return iRc;
}
