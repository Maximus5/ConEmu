
/*
Copyright (c) 2009-2011 Maximus5
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
#include <Tlhelp32.h>
#include "ConEmu.h"
#include "Attach.h"
#include "../common/WinObjects.h"
#include "../common/ProcList.h"

enum AttachListColumns
{
	alc_PID = 0,
	alc_Type,
	alc_Title,
	alc_File, // executable name
	alc_Path, // executable full path
	alc_HWND,
	alc_Class,
};

CAttachDlg::CAttachDlg()
	: mh_Dlg(NULL)
	, mh_List(NULL)
	, mn_AttachType(0)
	, mn_AttachPID(0)
	, mh_AttachHWND(NULL)
	, mp_ProcessData(NULL)
{
	mb_IsWin64 = IsWindows64(NULL);
}

CAttachDlg::~CAttachDlg()
{
	_ASSERTE(!mh_Dlg || !IsWindow(mh_Dlg));
	Close();
}

HWND CAttachDlg::GetHWND()
{
	if (!this)
	{
		_ASSERTE(this);
		return NULL;
	}
	return mh_Dlg;
}

// Открыть диалог со списком окон/процессов, к которым мы можем подцепиться
void CAttachDlg::AttachDlg()
{
	if (!this)
	{
		_ASSERTE(this);
		return;
	}

	if (mh_Dlg && IsWindow(mh_Dlg))
	{
		ShowWindow(mh_Dlg, SW_SHOWNORMAL);
		SetForegroundWindow(mh_Dlg);
		return;
	}
	
	mh_Dlg = CreateDialogParam(g_hInstance, MAKEINTRESOURCE(IDD_ATTACHDLG), ghWnd, AttachDlgProc, (LPARAM)this);

	//DWORD_PTR nAttachParm[3] = {};
	//int nRc = DialogBoxParam(nAttachParm);
	//switch (nRc)
	//{
	//case IDC_NEWCONSOLE:
	//	Recreate(FALSE, gpSet->isMultiNewConfirm);
	//	break;
	//case IDOK:
	//	// [0] Type: 1 - console, 2 - GUI
	//	// [1] PID
	//	// [2] HWND (для GUI)
	//	_ASSERTE((nAttachParm[0] == 1 || nAttachParm[0] == 2) && (nAttachParm[1]) && (nAttachParm[2] || nAttachParm[0] == 1));
	//	TODO("Ну и сам аттач, собственно");
	//	break;
	//}
}

void CAttachDlg::Close()
{
	if (mh_Dlg)
	{
		if (IsWindow(mh_Dlg))
			DestroyWindow(mh_Dlg);
		mh_Dlg = NULL;
	}
	
	if (mp_ProcessData)
	{
		_ASSERTE(mp_ProcessData==NULL);
		delete mp_ProcessData;
		mp_ProcessData = NULL;
	}
}

bool CAttachDlg::StartAttach()
{
	// Тут нужно получить инфу из списка и дернуть собственно аттач
	_ASSERTE(FALSE);
	return false;
}

BOOL CAttachDlg::AttachDlgEnumWin(HWND hFind, LPARAM lParam)
{
	if (IsWindowVisible(hFind))
	{
		// Условия?
		DWORD_PTR nStyle = GetWindowLongPtr(hFind, GWL_STYLE);
		DWORD_PTR nStyleEx = GetWindowLongPtr(hFind, GWL_EXSTYLE);
		DWORD nPID;
		if (!GetWindowThreadProcessId(hFind, &nPID))
			nPID = 0;

		bool lbCan = ((nStyle & (WS_VISIBLE|WS_CAPTION|WS_MAXIMIZEBOX)) == (WS_VISIBLE|WS_CAPTION|WS_MAXIMIZEBOX));
		if (lbCan && nPID == GetCurrentProcessId())
			lbCan = false;
		if (lbCan && (nStyle & WS_CHILD))
			lbCan = false;
		if (lbCan && (nStyleEx & WS_EX_TOOLWINDOW))
			lbCan = false;
		if (lbCan && gpConEmu->IsOurConsoleWindow(hFind))
			lbCan = false;

		if (lbCan)
		{
			wchar_t szClass[MAX_PATH], szTitle[4096], szTemp[32], szType[16]; szClass[0] = szTitle[0] = szTemp[0] = szType[0] = 0;
			GetClassName(hFind, szClass, countof(szClass));
			GetWindowText(hFind, szTitle, countof(szTitle));

			CAttachDlg* pDlg = (CAttachDlg*)lParam;
			HWND hList = pDlg->mh_List;
			DWORD nPID = 0;
			if (GetWindowThreadProcessId(hFind, &nPID))
				_wsprintf(szTemp, SKIPLEN(countof(szTemp)) L"%u", nPID);
			else
				wcscpy_c(szTemp, L"????");

			LVITEM lvi = {LVIF_TEXT|LVIF_STATE};
			lvi.state = 0;
			lvi.stateMask = LVIS_SELECTED|LVIS_FOCUSED;
			lvi.pszText = szTemp;
			lvi.iItem = ListView_GetItemCount(hList); // в конец
			int nItem = ListView_InsertItem(hList, &lvi);
			ListView_SetItemState(hList, nItem, 0, LVIS_SELECTED|LVIS_FOCUSED);

			ListView_SetItemText(hList, nItem, alc_Title, szTitle);
			ListView_SetItemText(hList, nItem, alc_Class, szClass);
			
			_wsprintf(szTemp, SKIPLEN(countof(szTemp)) L"0x%08X", (DWORD)(((DWORD_PTR)hFind) & (DWORD)-1));
			ListView_SetItemText(hList, nItem, alc_HWND, szTemp);

			HANDLE h;
			bool lbExeFound = false;
			int nImageBits = 32;

			wcscpy_c(szType, (lstrcmp(szClass, L"ConsoleWindowClass") == 0) ? L"CON" : L"GUI");
			
			if (pDlg->mp_ProcessData)
			{
				lbExeFound = pDlg->mp_ProcessData->GetProcessName(nPID, szClass, countof(szClass), szTitle, countof(szTitle), &nImageBits);
				if (lbExeFound)
				{
					ListView_SetItemText(hList, nItem, alc_File, szClass);
					ListView_SetItemText(hList, nItem, alc_Path, szTitle);
					if (pDlg->mb_IsWin64)
					{
						wcscat_c(szType, (nImageBits == 64) ? L" *64" : L" *32");
					}
				}
			}

			if (!lbExeFound)
			{
				h = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, nPID);
				if (h && h != INVALID_HANDLE_VALUE)
				{
					MODULEENTRY32 mi = {sizeof(mi)};
					if (Module32First(h, &mi))
					{
						ListView_SetItemText(hList, nItem, alc_File, *mi.szModule ? mi.szModule : (wchar_t*)PointToName(mi.szExePath));
						ListView_SetItemText(hList, nItem, alc_Path, mi.szExePath);
						if (pDlg->mb_IsWin64)
						{
							wcscat_c(szType, WIN3264TEST(L" *32",L" *64"));
						}
						lbExeFound = true;
					}
					else
					{
						if (pDlg->mb_IsWin64)
						{
							wcscat_c(szType, L" *64");
						}
					}
					CloseHandle(h);
				}
				
				#ifdef _WIN64
				if (!lbExeFound)
				{
					h = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE|TH32CS_SNAPMODULE32, nPID);
					if (h && h != INVALID_HANDLE_VALUE)
					{
						MODULEENTRY32 mi = {sizeof(mi)};
						if (Module32First(h, &mi))
						{
							ListView_SetItemText(hList, nItem, alc_File, *mi.szModule ? mi.szModule : (wchar_t*)PointToName(mi.szExePath));
							ListView_SetItemText(hList, nItem, alc_Path, mi.szExePath);
							wcscat_c(szType, L" *32");
						}
						CloseHandle(h);
					}
				}
				#endif
			}
			
			ListView_SetItemText(hList, nItem, alc_Type, szType);
		}
	}
	return TRUE; // Продолжить
}

INT_PTR CAttachDlg::AttachDlgProc(HWND hDlg, UINT messg, WPARAM wParam, LPARAM lParam)
{
	CAttachDlg* pDlg = NULL;
	if (messg == WM_INITDIALOG)
	{
		SetWindowLongPtr(hDlg, DWLP_USER, (LONG_PTR)lParam);
		pDlg = (CAttachDlg*)lParam;
		pDlg->mh_Dlg = hDlg;
	}
	else
	{
		pDlg = (CAttachDlg*)GetWindowLongPtr(hDlg, DWLP_USER);
	}
	
	if (!pDlg)
	{
		//_ASSERTE(pDlg!=NULL);
		return 0;
	}
	_ASSERTE(pDlg->mh_Dlg!=NULL);

	switch (messg)
	{
		case WM_INITDIALOG:
		{
			LRESULT lbRc = TRUE;

			// Если ConEmu - AlwaysOnTop, то и диалогу нужно выставит этот флаг
			if (GetWindowLongPtr(ghWnd, GWL_EXSTYLE) & WS_EX_TOPMOST)
				SetWindowPos(hDlg, HWND_TOPMOST, 0,0,0,0, SWP_NOMOVE|SWP_NOSIZE);

			SendMessage(hDlg, WM_SETICON, ICON_BIG, (LPARAM)hClassIcon);
			SendMessage(hDlg, WM_SETICON, ICON_SMALL, (LPARAM)hClassIconSm);
			SetClassLongPtr(hDlg, GCLP_HICON, (LONG_PTR)hClassIcon);
			
			EnableWindow(GetDlgItem(hDlg,IDOK), FALSE);

			HWND hList = GetDlgItem(hDlg, IDC_ATTACHLIST);
			pDlg->mh_List = hList;
			{
				LVCOLUMN col ={LVCF_WIDTH|LVCF_TEXT|LVCF_FMT, LVCFMT_LEFT, 60};
				wchar_t szTitle[64]; col.pszText = szTitle;

				ListView_SetExtendedListViewStyleEx(hList,LVS_EX_FULLROWSELECT,LVS_EX_FULLROWSELECT);
				ListView_SetExtendedListViewStyleEx(hList,LVS_EX_LABELTIP|LVS_EX_INFOTIP,LVS_EX_LABELTIP|LVS_EX_INFOTIP);
				
				wcscpy_c(szTitle, L"PID");			ListView_InsertColumn(hList, alc_PID, &col);
				col.cx = pDlg->mb_IsWin64 ? 60 : 45;
				wcscpy_c(szTitle, L"Type");			ListView_InsertColumn(hList, alc_Type, &col);
				col.cx = 200;
				wcscpy_c(szTitle, L"Title");		ListView_InsertColumn(hList, alc_Title, &col);
				col.cx = 120;
				wcscpy_c(szTitle, L"Image name");	ListView_InsertColumn(hList, alc_File, &col);
				col.cx = 200;
				wcscpy_c(szTitle, L"Image path");	ListView_InsertColumn(hList, alc_Path, &col);
				col.cx = 90;
				wcscpy_c(szTitle, L"HWND");			ListView_InsertColumn(hList, alc_HWND, &col);
				col.cx = 200;
				wcscpy_c(szTitle, L"Class");		ListView_InsertColumn(hList, alc_Class, &col);
			}

			pDlg->mp_ProcessData = new CProcessData;

			EnumWindows(AttachDlgEnumWin, (LPARAM)pDlg);

			delete pDlg->mp_ProcessData;
			pDlg->mp_ProcessData = NULL;

			AttachDlgProc(hDlg, WM_SIZE, 0, 0);

			RECT rect;
			GetWindowRect(hDlg, &rect);
			RECT rcParent;
			GetWindowRect(ghWnd, &rcParent);
			MoveWindow(hDlg,
			           (rcParent.left+rcParent.right-rect.right+rect.left)/2,
			           (rcParent.top+rcParent.bottom-rect.bottom+rect.top)/2,
			           rect.right - rect.left, rect.bottom - rect.top, false);
			
			ShowWindow(hDlg, SW_SHOWNORMAL);

			return lbRc;
		}

		case WM_SIZE:
		{
			RECT rcDlg, rcBtn, rcList;
			GetClientRect(hDlg, &rcDlg);
			HWND h = GetDlgItem(hDlg, IDC_ATTACHLIST);
			GetWindowRect(h, &rcList); MapWindowPoints(NULL, hDlg, (LPPOINT)&rcList, 2);
			HWND hb = GetDlgItem(hDlg, IDOK);
			GetWindowRect(hb, &rcBtn); MapWindowPoints(NULL, hb, (LPPOINT)&rcBtn, 2);
			MoveWindow(h,
				rcList.left, rcList.top, rcDlg.right - 2*rcList.left, rcDlg.bottom - 3*rcList.top - rcBtn.bottom, true);
			MoveWindow(GetDlgItem(hDlg, IDC_NEWCONSOLE),
				rcList.left, rcDlg.bottom - rcList.top - rcBtn.bottom, rcBtn.right, rcBtn.bottom, true);
			MoveWindow(GetDlgItem(hDlg, IDC_REFRESH),
				rcList.left*2 + rcBtn.right, rcDlg.bottom - rcList.top - rcBtn.bottom, rcBtn.right, rcBtn.bottom, true);
			MoveWindow(GetDlgItem(hDlg, IDCANCEL),
				rcDlg.right - rcList.left - rcBtn.right, rcDlg.bottom - rcList.top - rcBtn.bottom, rcBtn.right, rcBtn.bottom, true);
			MoveWindow(GetDlgItem(hDlg, IDOK),
				rcDlg.right - 2*rcList.left - 2*rcBtn.right, rcDlg.bottom - rcList.top - rcBtn.bottom, rcBtn.right, rcBtn.bottom, true);
			break;
		}

		case WM_NOTIFY:
		{
			LPNMITEMACTIVATE lpnmitem = (LPNMITEMACTIVATE)lParam;
			// По LMouseDblClick - выполнить аттач
			if (lpnmitem && (lpnmitem->hdr.idFrom == IDC_ATTACHLIST))
			{
				if (lpnmitem->hdr.code == NM_DBLCLK)
				{
					PostMessage(hDlg, WM_COMMAND, IDOK, 0);
				}
			}
			break;
		}

		//case WM_TIMER:
		//	// Автообновление?
		//	break;

		case WM_CLOSE:
			pDlg->Close();
			break;
		case WM_DESTROY:
			break;
		
		case WM_COMMAND:
			if (HIWORD(wParam) == BN_CLICKED)
			{
				DWORD_PTR *pAttachParm = (DWORD_PTR*)GetWindowLongPtr(hDlg, DWLP_USER);
				switch (LOWORD(wParam))
				{
					case IDOK:
						// Тут нужно получить инфу из списка и дернуть собственно аттач
						if (pDlg->StartAttach())
							pDlg->Close();
						return 1;
					case IDCANCEL:
					case IDCLOSE:
						// Просто закрыть
						pDlg->Close();
						return 1;
					case IDC_NEWCONSOLE:
						// Запрос на создание новой консоли
						PostMessage(ghWnd, WM_SYSCOMMAND, ID_NEWCONSOLE, 0);
						pDlg->Close();
						return 1;
					case IDC_REFRESH:
						// Перечитать список доступных окон/процессов
						{
							ListView_DeleteAllItems(pDlg->mh_List);
							pDlg->mp_ProcessData = new CProcessData;
							EnumWindows(AttachDlgEnumWin, (LPARAM)pDlg);
							delete pDlg->mp_ProcessData;
							pDlg->mp_ProcessData = NULL;
						}
						return 1;
				}
			}

			break;
		default:
			return 0;
	}

	return 0;
}
