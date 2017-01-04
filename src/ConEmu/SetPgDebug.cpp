
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
#include "../common/execute.h"
#include "../common/WFiles.h"

#include "ConEmu.h"
#include "OptionsClass.h"
#include "SetPgDebug.h"

CSetPgDebug::CSetPgDebug()
{
	m_ActivityLoggingType = glt_None; mn_ActivityCmdStartTick = 0;
}

CSetPgDebug::~CSetPgDebug()
{
}

LRESULT CSetPgDebug::OnInitDialog(HWND hDlg, bool abInitial)
{
	if (abInitial)
	{
		checkDlgButton(hDlg, rbActivityDisabled, BST_CHECKED);

		HWND hList = GetDlgItem(hDlg, lbActivityLog);
		ListView_SetExtendedListViewStyleEx(hList,LVS_EX_FULLROWSELECT,LVS_EX_FULLROWSELECT);
		ListView_SetExtendedListViewStyleEx(hList,LVS_EX_LABELTIP|LVS_EX_INFOTIP,LVS_EX_LABELTIP|LVS_EX_INFOTIP);

		LVCOLUMN col = {
			LVCF_WIDTH|LVCF_TEXT|LVCF_FMT, LVCFMT_LEFT,
			gpSetCls->EvalSize(60, esf_Horizontal|esf_CanUseDpi)};
		wchar_t szTitle[4]; col.pszText = szTitle;
		wcscpy_c(szTitle, L" ");		ListView_InsertColumn(hList, 0, &col);

		HWND hTip = ListView_GetToolTips(hList);
		SetWindowPos(hTip, HWND_TOPMOST, 0,0,0,0, SWP_NOMOVE|SWP_NOSIZE|SWP_NOACTIVATE);
	}

	return 0;
}

GuiLoggingType CSetPgDebug::GetActivityLoggingType()
{
	CSetPgDebug* pDbgPg = (CSetPgDebug*)gpSetCls->GetPageObj(thi_Debug);
	if (!pDbgPg)
		return glt_None;
	return pDbgPg->m_ActivityLoggingType;
}

void CSetPgDebug::SetLoggingType(GuiLoggingType aNewLogType)
{
	m_ActivityLoggingType = aNewLogType;

	HWND hList = GetDlgItem(mh_Dlg, lbActivityLog);

	ListView_DeleteAllItems(hList);

	for (int c = 0; (c <= 40) && ListView_DeleteColumn(hList, 0); c++)
		;

	//ListView_DeleteAllItems(hDetails);
	//for (int c = 0; (c <= 40) && ListView_DeleteColumn(hDetails, 0); c++);

	SetDlgItemText(mh_Dlg, ebActivityApp, L"");
	SetDlgItemText(mh_Dlg, ebActivityParm, L"");

	if ((m_ActivityLoggingType == glt_Processes)
		|| (m_ActivityLoggingType == glt_Shell))
	{
		LVCOLUMN col = {
			LVCF_WIDTH|LVCF_TEXT|LVCF_FMT, LVCFMT_LEFT,
			gpSetCls->EvalSize(80, esf_Horizontal|esf_CanUseDpi)};
		wchar_t szTitle[64]; col.pszText = szTitle;

		ListView_SetExtendedListViewStyleEx(hList,LVS_EX_FULLROWSELECT,LVS_EX_FULLROWSELECT);
		ListView_SetExtendedListViewStyleEx(hList,LVS_EX_LABELTIP|LVS_EX_INFOTIP,LVS_EX_LABELTIP|LVS_EX_INFOTIP);

		wcscpy_c(szTitle, L"Time");		ListView_InsertColumn(hList, lpc_Time, &col);
		col.cx = gpSetCls->EvalSize(60, esf_Horizontal|esf_CanUseDpi); col.fmt = LVCFMT_RIGHT;
		wcscpy_c(szTitle, L"PPID");		ListView_InsertColumn(hList, lpc_PPID, &col);
		col.cx = gpSetCls->EvalSize(60, esf_Horizontal|esf_CanUseDpi); col.fmt = LVCFMT_LEFT;
		wcscpy_c(szTitle, L"Func");		ListView_InsertColumn(hList, lpc_Func, &col);
		col.cx = gpSetCls->EvalSize(50, esf_Horizontal|esf_CanUseDpi);
		wcscpy_c(szTitle, L"Oper");		ListView_InsertColumn(hList, lpc_Oper, &col);
		col.cx = gpSetCls->EvalSize(40, esf_Horizontal|esf_CanUseDpi);
		wcscpy_c(szTitle, L"Bits");		ListView_InsertColumn(hList, lpc_Bits, &col);
		wcscpy_c(szTitle, L"Syst");		ListView_InsertColumn(hList, lpc_System, &col);
		col.cx = gpSetCls->EvalSize(120, esf_Horizontal|esf_CanUseDpi);
		wcscpy_c(szTitle, L"App");		ListView_InsertColumn(hList, lpc_App, &col);
		wcscpy_c(szTitle, L"Params");	ListView_InsertColumn(hList, lpc_Params, &col);
		//wcscpy_c(szTitle, L"CurDir");	ListView_InsertColumn(hList, 7, &col);
		col.cx = gpSetCls->EvalSize(120, esf_Horizontal|esf_CanUseDpi);
		wcscpy_c(szTitle, L"Flags");	ListView_InsertColumn(hList, lpc_Flags, &col);
		col.cx = gpSetCls->EvalSize(80, esf_Horizontal|esf_CanUseDpi);
		wcscpy_c(szTitle, L"StdIn");	ListView_InsertColumn(hList, lpc_StdIn, &col);
		wcscpy_c(szTitle, L"StdOut");	ListView_InsertColumn(hList, lpc_StdOut, &col);
		wcscpy_c(szTitle, L"StdErr");	ListView_InsertColumn(hList, lpc_StdErr, &col);

	}
	else if (m_ActivityLoggingType == glt_Input)
	{
		LVCOLUMN col = {
			LVCF_WIDTH|LVCF_TEXT|LVCF_FMT, LVCFMT_LEFT,
			gpSetCls->EvalSize(60, esf_Horizontal|esf_CanUseDpi)};
		wchar_t szTitle[64]; col.pszText = szTitle;

		ListView_SetExtendedListViewStyleEx(hList,LVS_EX_FULLROWSELECT,LVS_EX_FULLROWSELECT);
		ListView_SetExtendedListViewStyleEx(hList,LVS_EX_LABELTIP|LVS_EX_INFOTIP,LVS_EX_LABELTIP|LVS_EX_INFOTIP);

		wcscpy_c(szTitle, L"Time");		ListView_InsertColumn(hList, lic_Time, &col);
		col.cx = gpSetCls->EvalSize(50, esf_Horizontal|esf_CanUseDpi);
		wcscpy_c(szTitle, L"Type");		ListView_InsertColumn(hList, lic_Type, &col);
		col.cx = gpSetCls->EvalSize(50, esf_Horizontal|esf_CanUseDpi);
		wcscpy_c(szTitle, L"##");		ListView_InsertColumn(hList, lic_Dup, &col);
		col.cx = gpSetCls->EvalSize(300, esf_Horizontal|esf_CanUseDpi);
		wcscpy_c(szTitle, L"Event");	ListView_InsertColumn(hList, lic_Event, &col);

	}
	else if (m_ActivityLoggingType == glt_Commands)
	{
		mn_ActivityCmdStartTick = timeGetTime();

		LVCOLUMN col = {
			LVCF_WIDTH|LVCF_TEXT|LVCF_FMT, LVCFMT_LEFT,
			gpSetCls->EvalSize(60, esf_Horizontal|esf_CanUseDpi)};
		wchar_t szTitle[64]; col.pszText = szTitle;

		ListView_SetExtendedListViewStyleEx(hList,LVS_EX_FULLROWSELECT,LVS_EX_FULLROWSELECT);
		ListView_SetExtendedListViewStyleEx(hList,LVS_EX_LABELTIP|LVS_EX_INFOTIP,LVS_EX_LABELTIP|LVS_EX_INFOTIP);

		col.cx = gpSetCls->EvalSize(50, esf_Horizontal|esf_CanUseDpi);
		wcscpy_c(szTitle, L"In/Out");	ListView_InsertColumn(hList, lcc_InOut, &col);
		col.cx = gpSetCls->EvalSize(70, esf_Horizontal|esf_CanUseDpi);
		wcscpy_c(szTitle, L"Time");		ListView_InsertColumn(hList, lcc_Time, &col);
		col.cx = gpSetCls->EvalSize(60, esf_Horizontal|esf_CanUseDpi);
		wcscpy_c(szTitle, L"Duration");	ListView_InsertColumn(hList, lcc_Duration, &col);
		col.cx = gpSetCls->EvalSize(50, esf_Horizontal|esf_CanUseDpi);
		wcscpy_c(szTitle, L"Cmd");		ListView_InsertColumn(hList, lcc_Command, &col);
		wcscpy_c(szTitle, L"Size");		ListView_InsertColumn(hList, lcc_Size, &col);
		wcscpy_c(szTitle, L"PID");		ListView_InsertColumn(hList, lcc_PID, &col);
		col.cx = gpSetCls->EvalSize(300, esf_Horizontal|esf_CanUseDpi);
		wcscpy_c(szTitle, L"Pipe");		ListView_InsertColumn(hList, lcc_Pipe, &col);
		wcscpy_c(szTitle, L"Extra");	ListView_InsertColumn(hList, lcc_Extra, &col);

	}
	else
	{
		LVCOLUMN col = {
			LVCF_WIDTH|LVCF_TEXT|LVCF_FMT, LVCFMT_LEFT,
			gpSetCls->EvalSize(60, esf_Horizontal|esf_CanUseDpi)};
		wchar_t szTitle[4]; col.pszText = szTitle;
		wcscpy_c(szTitle, L" ");		ListView_InsertColumn(hList, 0, &col);
		//ListView_InsertColumn(hDetails, 0, &col);
	}
	ListView_DeleteAllItems(GetDlgItem(mh_Dlg, lbActivityLog));

	gpConEmu->OnGlobalSettingsChanged();
}

void CSetPgDebug::debugLogShell(DWORD nParentPID, CESERVER_REQ_ONCREATEPROCESS* pInfo)
{
	CSetPgDebug* pDbgPg = (CSetPgDebug*)gpSetCls->GetPageObj(thi_Debug);
	if (!pDbgPg)
		return;
	if ((pDbgPg->GetActivityLoggingType() != glt_Processes)
		&& (pDbgPg->GetActivityLoggingType() != glt_Shell))
		return;

	LPCWSTR pszFile = pInfo->wsValue + pInfo->nActionLen;
	LPCWSTR pszParam = pInfo->wsValue + pInfo->nActionLen+pInfo->nFileLen;
	DebugLogShellActivity *shl = (DebugLogShellActivity*)calloc(sizeof(DebugLogShellActivity),1);
	shl->nParentPID = nParentPID;
	shl->nParentBits = pInfo->nSourceBits;
	wcscpy_c(shl->szFunction, pInfo->sFunction);
	shl->pszAction = lstrdup(pInfo->wsValue);
	shl->pszFile   = lstrdup(pszFile);
	shl->pszParam  = lstrdup(pszParam);
	shl->bDos = (pInfo->nImageBits == 16) && (pInfo->nImageSubsystem == IMAGE_SUBSYSTEM_DOS_EXECUTABLE);
	shl->nImageBits = pInfo->nImageBits;
	shl->nImageSubsystem = pInfo->nImageSubsystem;
	shl->nShellFlags = pInfo->nShellFlags;
	shl->nCreateFlags = pInfo->nCreateFlags;
	shl->nStartFlags = pInfo->nStartFlags;
	shl->nShowCmd = pInfo->nShowCmd;
	shl->hStdIn = (DWORD)pInfo->hStdIn;
	shl->hStdOut = (DWORD)pInfo->hStdOut;
	shl->hStdErr = (DWORD)pInfo->hStdErr;

	// Append directory and bat/tmp files contents to pszParam
	{
		LPCWSTR pszDir = (pInfo->wsValue+pInfo->nActionLen+pInfo->nFileLen+pInfo->nParamLen);
		LPCWSTR pszAppFile = NULL;
		wchar_t*& pszParamEx = shl->pszParam;

		if (pszDir && *pszDir)
		{
			CEStr lsDir((pszParamEx && *pszParamEx) ? L"\r\n\r\n" : NULL, L"CD: \"", pszDir, L"\"");
			lstrmerge(&pszParamEx, lsDir);
		}

		if (shl->pszFile)
		{
			LPCWSTR pszExt = PointToExt(shl->pszFile);
			if (pszExt && (!lstrcmpi(pszExt, L".bat") || !lstrcmpi(pszExt, L".cmd")))
				debugLogShellText(pszParamEx, (pszAppFile = shl->pszFile));
		}

		if (pszParam && *pszParam)
		{
			LPCWSTR pszNext = pszParam;
			CEStr szArg;
			while (0 == NextArg(&pszNext, szArg))
			{
				if (!*szArg || (*szArg == L'-') || (*szArg == L'/'))
					continue;
				LPCWSTR pszExt = PointToExt(szArg);

				// Perhaps process *.tmp files too? (used with VC RC compilation?)
				if (pszExt && (!lstrcmpi(pszExt, L".bat") || !lstrcmpi(pszExt, L".cmd") /*|| !lstrcmpi(pszExt, L".tmp")*/)
					&& (!pszAppFile || (lstrcmpi(szArg, pszAppFile) != 0)))
				{
					debugLogShellText(pszParamEx, szArg);
				}
				else if (szArg[0] == L'@')
				{
					CEStr lsPath;

					if (IsFilePath(szArg.Mid(1), true))
					{
						// Full path to "arguments file"
						lsPath.Set(szArg.Mid(1));
					}
					else if ((szArg[1] != L'\\' && szArg[1] != L'/')
						&& (pszDir && *pszDir))
					{
						// Relative path to "arguments file"
						lsPath = JoinPath(pszDir, szArg.Mid(1));
					}

					if (!lsPath.IsEmpty())
					{
						debugLogShellText(pszParamEx, lsPath);
					}
				}
			}
		}
	}
	// end of "Append directory and bat/tmp files contents to pszParam"

	PostMessage(pDbgPg->Dlg(), DBGMSG_LOG_ID, DBGMSG_LOG_SHELL_MAGIC, (LPARAM)shl);
}

void CSetPgDebug::debugLogShell(DebugLogShellActivity *pShl)
{
	if (m_ActivityLoggingType != glt_Processes)
	{
		_ASSERTE(m_ActivityLoggingType == glt_Processes);
		return;
	}

	SYSTEMTIME st; GetLocalTime(&st);
	wchar_t szTime[128]; _wsprintf(szTime, SKIPLEN(countof(szTime)) L"%02i:%02i:%02i.%03i", st.wHour, st.wMinute, st.wSecond, st.wMilliseconds);
	HWND hList = GetDlgItem(mh_Dlg, lbActivityLog);
	LVITEM lvi = {LVIF_TEXT|LVIF_STATE};
	lvi.state = lvi.stateMask = LVIS_SELECTED|LVIS_FOCUSED;
	lvi.pszText = szTime;
	int nItem = ListView_InsertItem(hList, &lvi);
	//
	ListView_SetItemText(hList, nItem, lpc_Func, (wchar_t*)pShl->szFunction);
	_wsprintf(szTime, SKIPLEN(countof(szTime)) L"%u:%u", pShl->nParentPID, pShl->nParentBits);
	ListView_SetItemText(hList, nItem, lpc_PPID, szTime);
	if (pShl->pszAction)
		ListView_SetItemText(hList, nItem, lpc_Oper, (wchar_t*)pShl->pszAction);
	if (pShl->nImageBits)
	{
		_wsprintf(szTime, SKIPLEN(countof(szTime)) L"%u", pShl->nImageBits);
		ListView_SetItemText(hList, nItem, lpc_Bits, szTime);
	}
	if (pShl->nImageSubsystem)
	{
		_wsprintf(szTime, SKIPLEN(countof(szTime)) L"%u", pShl->nImageSubsystem);
		ListView_SetItemText(hList, nItem, lpc_System, szTime);
	}

	if (pShl->pszFile)
		ListView_SetItemText(hList, nItem, lpc_App, pShl->pszFile);
	SetDlgItemText(mh_Dlg, ebActivityApp, (pShl->pszFile ? pShl->pszFile : L""));

	if (pShl->pszParam)
		ListView_SetItemText(hList, nItem, lpc_Params, pShl->pszParam);
	SetDlgItemText(mh_Dlg, ebActivityParm, (pShl->pszParam ? pShl->pszParam : L""));

	//TODO: CurDir?

	szTime[0] = 0;
	if (pShl->nShellFlags)
		_wsprintf(szTime+_tcslen(szTime), SKIPLEN(32) L"Sh:0x%04X ", pShl->nShellFlags); //-V112
	if (pShl->nCreateFlags)
		_wsprintf(szTime+_tcslen(szTime), SKIPLEN(32) L"Cr:0x%04X ", pShl->nCreateFlags); //-V112
	if (pShl->nStartFlags)
		_wsprintf(szTime+_tcslen(szTime), SKIPLEN(32) L"St:0x%04X ", pShl->nStartFlags); //-V112
	if (pShl->nShowCmd)
		_wsprintf(szTime+_tcslen(szTime), SKIPLEN(32) L"Sw:%u ", pShl->nShowCmd); //-V112
	ListView_SetItemText(hList, nItem, lpc_Flags, szTime);

	if (pShl->hStdIn)
	{
		_wsprintf(szTime, SKIPLEN(countof(szTime)) L"0x%08X", pShl->hStdIn);
		ListView_SetItemText(hList, nItem, lpc_StdIn, szTime);
	}
	if (pShl->hStdOut)
	{
		_wsprintf(szTime, SKIPLEN(countof(szTime)) L"0x%08X", pShl->hStdOut);
		ListView_SetItemText(hList, nItem, lpc_StdOut, szTime);
	}
	if (pShl->hStdErr)
	{
		_wsprintf(szTime, SKIPLEN(countof(szTime)) L"0x%08X", pShl->hStdErr);
		ListView_SetItemText(hList, nItem, lpc_StdErr, szTime);
	}
	if (pShl->pszAction)
		free(pShl->pszAction);
	if (pShl->pszFile)
		free(pShl->pszFile);
	if (pShl->pszParam)
		free(pShl->pszParam);
	free(pShl);
}

void CSetPgDebug::debugLogShellText(wchar_t* &pszParamEx, LPCWSTR asFile)
{
	_ASSERTE(pszParamEx!=NULL && asFile && *asFile);

	CEStr szBuf;
	DWORD cchMax = 32*1024/*32 KB*/;
	DWORD nRead = 0, nErrCode = (DWORD)-1;

	#ifdef _DEBUG
	LPCWSTR pszExt = PointToExt(asFile);
	bool bCmdBatch = ((lstrcmpi(pszExt, L".bat") == 0) && (lstrcmpi(pszExt, L".cmd") == 0));
	#endif

	// ReadTextFile will use BOM if it exists
	UINT nDefCP = CP_OEMCP;

	if (0 == ReadTextFile(asFile, cchMax, szBuf.ms_Val, nRead, nErrCode, nDefCP))
	{
		size_t nAll = 0;
		wchar_t* pszNew = NULL;

		nAll = (lstrlen(pszParamEx)+20) + nRead + 1 + 2*lstrlen(asFile);
		pszNew = (wchar_t*)realloc(pszParamEx, nAll*sizeof(wchar_t));
		if (pszNew)
		{
			_wcscat_c(pszNew, nAll, L"\r\n\r\n>>>");
			_wcscat_c(pszNew, nAll, asFile);
			_wcscat_c(pszNew, nAll, L"\r\n");
			_wcscat_c(pszNew, nAll, szBuf);
			_wcscat_c(pszNew, nAll, L"\r\n<<<");
			_wcscat_c(pszNew, nAll, asFile);
			pszParamEx = pszNew;
		}
	}
}

void CSetPgDebug::debugLogInfo(CESERVER_REQ_PEEKREADINFO* pInfo)
{
	CSetPgDebug* pDbgPg = (CSetPgDebug*)gpSetCls->GetPageObj(thi_Debug);
	if (!pDbgPg)
		return;
	if (pDbgPg->GetActivityLoggingType() != glt_Input)
		return;

	for (UINT nIdx = 0; nIdx < pInfo->nCount; nIdx++)
	{
		const INPUT_RECORD *pr = pInfo->Buffer+nIdx;
		SYSTEMTIME st; GetLocalTime(&st);
		wchar_t szTime[255]; _wsprintf(szTime, SKIPLEN(countof(szTime)) L"%02i:%02i:%02i.%03i", st.wHour, st.wMinute, st.wSecond, st.wMilliseconds);
		HWND hList = GetDlgItem(pDbgPg->Dlg(), lbActivityLog);
		LVITEM lvi = {LVIF_TEXT|LVIF_STATE};
		lvi.state = lvi.stateMask = LVIS_SELECTED|LVIS_FOCUSED;
		lvi.pszText = szTime;
		static INPUT_RECORD LastLogEvent1; static char LastLogEventType1; static UINT LastLogEventDup1;
		static INPUT_RECORD LastLogEvent2; static char LastLogEventType2; static UINT LastLogEventDup2;
		if (LastLogEventType1 == pInfo->cPeekRead &&
			memcmp(&LastLogEvent1, pr, sizeof(LastLogEvent1)) == 0)
		{
			LastLogEventDup1 ++;
			_wsprintf(szTime, SKIPLEN(countof(szTime)) L"%u", LastLogEventDup1);
			ListView_SetItemText(hList, 0, lic_Dup, szTime); // верхний
			//free(pr);
			continue; // дубли - не показывать? только если прошло время?
		}
		if (LastLogEventType2 == pInfo->cPeekRead &&
			memcmp(&LastLogEvent2, pr, sizeof(LastLogEvent2)) == 0)
		{
			LastLogEventDup2 ++;
			_wsprintf(szTime, SKIPLEN(countof(szTime)) L"%u", LastLogEventDup2);
			ListView_SetItemText(hList, 1, lic_Dup, szTime); // верхний
			//free(pr);
			continue; // дубли - не показывать? только если прошло время?
		}
		int nItem = ListView_InsertItem(hList, &lvi);
		if (LastLogEventType1 && LastLogEventType1 != pInfo->cPeekRead)
		{
			LastLogEvent2 = LastLogEvent1; LastLogEventType2 = LastLogEventType1; LastLogEventDup2 = LastLogEventDup1;
		}
		LastLogEventType1 = pInfo->cPeekRead;
		memmove(&LastLogEvent1, pr, sizeof(LastLogEvent1));
		LastLogEventDup1 = 1;

		_wcscpy_c(szTime, countof(szTime), L"1");
		ListView_SetItemText(hList, nItem, lic_Dup, szTime);
		//
		szTime[0] = (wchar_t)pInfo->cPeekRead; szTime[1] = L'.'; szTime[2] = 0;
		if (pr->EventType == MOUSE_EVENT)
		{
			wcscat_c(szTime, L"Mouse");
			ListView_SetItemText(hList, nItem, lic_Type, szTime);
			const MOUSE_EVENT_RECORD *rec = &pr->Event.MouseEvent;
			_wsprintf(szTime, SKIPLEN(countof(szTime))
				L"[%d,%d], Btn=0x%08X (%c%c%c%c%c), Ctrl=0x%08X (%c%c%c%c%c - %c%c%c%c), Flgs=0x%08X (%s)",
				rec->dwMousePosition.X,
				rec->dwMousePosition.Y,
				rec->dwButtonState,
				(rec->dwButtonState&FROM_LEFT_1ST_BUTTON_PRESSED?L'L':L'l'),
				(rec->dwButtonState&RIGHTMOST_BUTTON_PRESSED?L'R':L'r'),
				(rec->dwButtonState&FROM_LEFT_2ND_BUTTON_PRESSED?L'2':L' '),
				(rec->dwButtonState&FROM_LEFT_3RD_BUTTON_PRESSED?L'3':L' '),
				(rec->dwButtonState&FROM_LEFT_4TH_BUTTON_PRESSED?L'4':L' '),
				rec->dwControlKeyState,
				(rec->dwControlKeyState&LEFT_CTRL_PRESSED?L'C':L'c'),
				(rec->dwControlKeyState&LEFT_ALT_PRESSED?L'A':L'a'),
				(rec->dwControlKeyState&SHIFT_PRESSED?L'S':L's'),
				(rec->dwControlKeyState&RIGHT_ALT_PRESSED?L'A':L'a'),
				(rec->dwControlKeyState&RIGHT_CTRL_PRESSED?L'C':L'c'),
				(rec->dwControlKeyState&ENHANCED_KEY?L'E':L'e'),
				(rec->dwControlKeyState&CAPSLOCK_ON?L'C':L'c'),
				(rec->dwControlKeyState&NUMLOCK_ON?L'N':L'n'),
				(rec->dwControlKeyState&SCROLLLOCK_ON?L'S':L's'),
				rec->dwEventFlags,
				(rec->dwEventFlags==0?L"(Click)":
				(rec->dwEventFlags==DOUBLE_CLICK?L"(DblClick)":
				(rec->dwEventFlags==MOUSE_MOVED?L"(Moved)":
				(rec->dwEventFlags==MOUSE_WHEELED?L"(Wheel)":
				(rec->dwEventFlags==0x0008/*MOUSE_HWHEELED*/?L"(HWheel)":L"")))))
				);
			if (rec->dwEventFlags==MOUSE_WHEELED  || rec->dwEventFlags==0x0008/*MOUSE_HWHEELED*/)
			{
				int nLen = _tcslen(szTime);
				_wsprintf(szTime+nLen, SKIPLEN(countof(szTime)-nLen)
					L" (Delta=%d)",HIWORD(rec->dwButtonState));
			}
			ListView_SetItemText(hList, nItem, lic_Event, szTime);
		}
		else if (pr->EventType == KEY_EVENT)
		{
			wcscat_c(szTime, L"Key");
			ListView_SetItemText(hList, nItem, lic_Type, szTime);
			_wsprintf(szTime, SKIPLEN(countof(szTime)) L"%c(%i) VK=%i, SC=%i, U=%c(x%04X), ST=x%08X",
				pr->Event.KeyEvent.bKeyDown ? L'D' : L'U',
				pr->Event.KeyEvent.wRepeatCount,
				pr->Event.KeyEvent.wVirtualKeyCode, pr->Event.KeyEvent.wVirtualScanCode,
				pr->Event.KeyEvent.uChar.UnicodeChar ? pr->Event.KeyEvent.uChar.UnicodeChar : L' ',
				pr->Event.KeyEvent.uChar.UnicodeChar,
				pr->Event.KeyEvent.dwControlKeyState);
			ListView_SetItemText(hList, nItem, lic_Event, szTime);
		}
		else if (pr->EventType == FOCUS_EVENT)
		{
			wcscat_c(szTime, L"Focus");
			ListView_SetItemText(hList, nItem, lic_Type, szTime);
			_wsprintf(szTime, SKIPLEN(countof(szTime)) L"%u", (DWORD)pr->Event.FocusEvent.bSetFocus);
			ListView_SetItemText(hList, nItem, lic_Event, szTime);
		}
		else if (pr->EventType == WINDOW_BUFFER_SIZE_EVENT)
		{
			wcscat_c(szTime, L"Buffer");
			ListView_SetItemText(hList, nItem, lic_Type, szTime);
			_wsprintf(szTime, SKIPLEN(countof(szTime)) L"%ix%i", (int)pr->Event.WindowBufferSizeEvent.dwSize.X, (int)pr->Event.WindowBufferSizeEvent.dwSize.Y);
			ListView_SetItemText(hList, nItem, lic_Event, szTime);
		}
		else if (pr->EventType == MENU_EVENT)
		{
			wcscat_c(szTime, L"Menu");
			ListView_SetItemText(hList, nItem, lic_Type, szTime);
			_wsprintf(szTime, SKIPLEN(countof(szTime)) L"%u", (DWORD)pr->Event.MenuEvent.dwCommandId);
			ListView_SetItemText(hList, nItem, lic_Event, szTime);
		}
		else
		{
			_wsprintf(szTime+2, SKIPLEN(countof(szTime)-2) L"%u", (DWORD)pr->EventType);
			ListView_SetItemText(hList, nItem, lic_Type, szTime);
		}
	}
	free(pInfo);
}

void CSetPgDebug::debugLogCommand(CESERVER_REQ* pInfo, BOOL abInput, DWORD anTick, DWORD anDur, LPCWSTR asPipe, CESERVER_REQ* pResult/*=NULL*/)
{
	CSetPgDebug* pDbgPg = (CSetPgDebug*)gpSetCls->GetPageObj(thi_Debug);
	if (!pDbgPg)
		return;
	if (pDbgPg->GetActivityLoggingType() != glt_Commands)
		return;

	_ASSERTE(abInput==TRUE || pResult!=NULL || (pInfo->hdr.nCmd==CECMD_LANGCHANGE || pInfo->hdr.nCmd==CECMD_GUICHANGED || pInfo->hdr.nCmd==CMD_FARSETCHANGED || pInfo->hdr.nCmd==CECMD_ONACTIVATION));

	LogCommandsData* pData = (LogCommandsData*)calloc(1,sizeof(LogCommandsData));

	if (!pData)
		return;

	pData->bInput = abInput;
	pData->bMainThread = (abInput == FALSE) && isMainThread();
	pData->nTick = anTick - pDbgPg->mn_ActivityCmdStartTick;
	pData->nDur = anDur;
	pData->nCmd = pInfo->hdr.nCmd;
	pData->nSize = pInfo->hdr.cbSize;
	pData->nPID = abInput ? pInfo->hdr.nSrcPID : pResult ? pResult->hdr.nSrcPID : 0;
	LPCWSTR pszName = asPipe ? PointToName(asPipe) : NULL;
	lstrcpyn(pData->szPipe, pszName ? pszName : L"", countof(pData->szPipe));
	switch (pInfo->hdr.nCmd)
	{
	case CECMD_POSTCONMSG:
		_wsprintf(pData->szExtra, SKIPLEN(countof(pData->szExtra))
			L"HWND=x%08X, Msg=%u, wParam=" WIN3264TEST(L"x%08X",L"x%08X%08X") L", lParam=" WIN3264TEST(L"x%08X",L"x%08X%08X") L": ",
			pInfo->Msg.hWnd, pInfo->Msg.nMsg, WIN3264WSPRINT(pInfo->Msg.wParam), WIN3264WSPRINT(pInfo->Msg.lParam));
		GetClassName(pInfo->Msg.hWnd, pData->szExtra+lstrlen(pData->szExtra), countof(pData->szExtra)-lstrlen(pData->szExtra));
		break;
	case CECMD_NEWCMD:
		lstrcpyn(pData->szExtra, pInfo->NewCmd.GetCommand(), countof(pData->szExtra));
		break;
	case CECMD_GUIMACRO:
		lstrcpyn(pData->szExtra, pInfo->GuiMacro.sMacro, countof(pData->szExtra));
		break;
	case CMD_POSTMACRO:
		lstrcpyn(pData->szExtra, (LPCWSTR)pInfo->wData, countof(pData->szExtra));
		break;
	}

	PostMessage(pDbgPg->Dlg(), DBGMSG_LOG_ID, DBGMSG_LOG_CMD_MAGIC, (LPARAM)pData);
}

void CSetPgDebug::debugLogCommand(LogCommandsData* apData)
{
	if (!apData)
		return;

	/*
		struct LogCommandsData
		{
			BOOL  bInput, bMainThread;
			DWORD nTick, nDur, nCmd, nSize, nPID;
			wchar_t szPipe[64];
		};
	*/

	wchar_t szText[128];
	HWND hList = GetDlgItem(mh_Dlg, lbActivityLog);

	wcscpy_c(szText, apData->bInput ? L"In" : L"Out");

	LVITEM lvi = {LVIF_TEXT|LVIF_STATE};
	lvi.state = lvi.stateMask = LVIS_SELECTED|LVIS_FOCUSED;
	lvi.pszText = szText;
	int nItem = ListView_InsertItem(hList, &lvi);

	int nMin = apData->nTick / 60000; apData->nTick -= nMin*60000;
	int nSec = apData->nTick / 1000;
	int nMS = apData->nTick % 1000;
	_wsprintf(szText, SKIPLEN(countof(szText)) L"%02i:%02i:%03i", nMin, nSec, nMS);
	ListView_SetItemText(hList, nItem, lcc_Time, szText);

	_wsprintf(szText, SKIPLEN(countof(szText)) apData->bInput ? L"" : L"%u", apData->nDur);
	ListView_SetItemText(hList, nItem, lcc_Duration, szText);

	_wsprintf(szText, SKIPLEN(countof(szText)) L"%u", apData->nCmd);
	ListView_SetItemText(hList, nItem, lcc_Command, szText);

	_wsprintf(szText, SKIPLEN(countof(szText)) L"%u", apData->nSize);
	ListView_SetItemText(hList, nItem, lcc_Size, szText);

	_wsprintf(szText, SKIPLEN(countof(szText)) apData->nPID ? L"%u" : L"", apData->nPID);
	ListView_SetItemText(hList, nItem, lcc_PID, szText);

	ListView_SetItemText(hList, nItem, lcc_Pipe, apData->szPipe);

	free(apData);
}

LRESULT CSetPgDebug::OnActivityLogNotify(WPARAM wParam, LPARAM lParam)
{
	switch (((NMHDR*)lParam)->code)
	{
	case LVN_ITEMCHANGED:
		{
			LPNMLISTVIEW p = (LPNMLISTVIEW)lParam;
			if ((p->uNewState & LVIS_SELECTED) && (p->iItem >= 0))
			{
				HWND hList = GetDlgItem(mh_Dlg, lbActivityLog);
				size_t cchText = 65535;
				wchar_t *szText = (wchar_t*)malloc(cchText*sizeof(*szText));
				if (!szText)
					return 0;
				szText[0] = 0;
				if (m_ActivityLoggingType == glt_Processes)
				{
					ListView_GetItemText(hList, p->iItem, lpc_App, szText, cchText);
					SetDlgItemText(mh_Dlg, ebActivityApp, szText);
					ListView_GetItemText(hList, p->iItem, lpc_Params, szText, cchText);
					SetDlgItemText(mh_Dlg, ebActivityParm, szText);
				}
				else if (m_ActivityLoggingType == glt_Input)
				{
					ListView_GetItemText(hList, p->iItem, lic_Type, szText, cchText);
					_wcscat_c(szText, cchText, L" - ");
					int nLen = _tcslen(szText);
					ListView_GetItemText(hList, p->iItem, lic_Dup, szText+nLen, cchText-nLen);
					SetDlgItemText(mh_Dlg, ebActivityApp, szText);
					ListView_GetItemText(hList, p->iItem, lic_Event, szText, cchText);
					SetDlgItemText(mh_Dlg, ebActivityParm, szText);
				}
				else if (m_ActivityLoggingType == glt_Commands)
				{
					ListView_GetItemText(hList, p->iItem, lcc_Pipe, szText, cchText);
					SetDlgItemText(mh_Dlg, ebActivityApp, szText);
					SetDlgItemText(mh_Dlg, ebActivityParm, L"");
				}
				else
				{
					SetDlgItemText(mh_Dlg, ebActivityApp, L"");
					SetDlgItemText(mh_Dlg, ebActivityParm, L"");
				}
				free(szText);
			}
		} //LVN_ITEMCHANGED
		break;
	}
	return 0;
}

void CSetPgDebug::OnSaveActivityLogFile()
{
	HWND hListView = GetDlgItem(mh_Dlg, lbActivityLog);

	wchar_t szLogFile[MAX_PATH];
	OPENFILENAME ofn; memset(&ofn,0,sizeof(ofn));
	ofn.lStructSize=sizeof(ofn);
	ofn.hwndOwner = ghOpWnd;
	ofn.lpstrFilter = _T("Log files (*.csv)\0*.csv\0\0");
	ofn.nFilterIndex = 1;

	ofn.lpstrFile = szLogFile; szLogFile[0] = 0;
	ofn.nMaxFile = countof(szLogFile);
	ofn.lpstrTitle = L"Save shell and processes log...";
	ofn.lpstrDefExt = L"csv";
	ofn.Flags = OFN_ENABLESIZING|OFN_NOCHANGEDIR
	            | OFN_PATHMUSTEXIST|OFN_EXPLORER|OFN_HIDEREADONLY|OFN_OVERWRITEPROMPT;

	if (!GetSaveFileName(&ofn))
		return;

	HANDLE hFile = CreateFile(szLogFile, GENERIC_WRITE, FILE_SHARE_READ, NULL, OPEN_ALWAYS, 0, NULL);
	if (!hFile || hFile == INVALID_HANDLE_VALUE)
	{
		DisplayLastError(L"Create log file failed!");
		return;
	}

	int nMaxText = 262144;
	wchar_t* pszText = (wchar_t*)malloc(nMaxText*sizeof(wchar_t));
	LVCOLUMN lvc = {LVCF_TEXT};
	//LVITEM lvi = {0};
	int nColumnCount = 0;
	DWORD nWritten = 0, nLen;

	for (;;nColumnCount++)
	{
		if (nColumnCount)
			WriteFile(hFile, L";", 2, &nWritten, NULL);

		lvc.pszText = pszText;
		lvc.cchTextMax = nMaxText;
		if (!ListView_GetColumn(hListView, nColumnCount, &lvc))
			break;

		nLen = _tcslen(pszText)*2;
		if (nLen)
			WriteFile(hFile, pszText, nLen, &nWritten, NULL);
	}
	WriteFile(hFile, L"\r\n", 2*sizeof(wchar_t), &nWritten, NULL); //-V112

	if (nColumnCount > 0)
	{
		INT_PTR nItems = ListView_GetItemCount(hListView); //-V220
		for (INT_PTR i = 0; i < nItems; i++)
		{
			for (int c = 0; c < nColumnCount; c++)
			{
				if (c)
					WriteFile(hFile, L";", 2, &nWritten, NULL);
				pszText[0] = 0;
				ListView_GetItemText(hListView, i, c, pszText, nMaxText);
				nLen = _tcslen(pszText)*2;
				if (nLen)
					WriteFile(hFile, pszText, nLen, &nWritten, NULL);
			}
			WriteFile(hFile, L"\r\n", 2*sizeof(wchar_t), &nWritten, NULL); //-V112
		}
	}

	free(pszText);
	CloseHandle(hFile);
}
