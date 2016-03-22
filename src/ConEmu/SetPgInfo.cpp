
/*
Copyright (c) 2016 Maximus5
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
#include "OptionsClass.h"
#include "SetPgInfo.h"
#include "VConGroup.h"
#include "VirtualConsole.h"

CSetPgInfo::CSetPgInfo()
{
}

CSetPgInfo::~CSetPgInfo()
{
}

LRESULT CSetPgInfo::OnInitDialog(HWND hDlg, bool abInitial)
{
	CVirtualConsole* pVCon = NULL;
	CVConGuard VCon;
	if (CVConGroup::GetActiveVCon(&VCon) >= 0)
		pVCon = VCon.VCon();

	SetDlgItemText(hDlg, tCurCmdLine, GetCommandLine());
	// Performance
	gpSetCls->Performance(gbPerformance, TRUE);
	gpConEmu->UpdateProcessDisplay(TRUE);
	gpConEmu->UpdateSizes();
	if (pVCon)
	{
		ConsoleInfoArg cursorInfo = {};
		pVCon->RCon()->GetConsoleInfo(&cursorInfo);
		FillCursorInfo(hDlg, &cursorInfo);
	}
	FillFontInfo(hDlg);
	if (pVCon)
	{
		FillConsoleMode(hDlg, pVCon->RCon());
	}

	return 0;
}

void CSetPgInfo::FillFontInfo(HWND hDlg)
{
	wchar_t szTemp[32];
	_wsprintf(szTemp, SKIPLEN(countof(szTemp)) L"%ix%ix%i", gpFontMgr->LogFont.lfHeight, gpFontMgr->LogFont.lfWidth, gpFontMgr->m_tm->tmAveCharWidth);
	SetDlgItemText(hDlg, tRealFontMain, szTemp);
	_wsprintf(szTemp, SKIPLEN(countof(szTemp)) L"%ix%i", gpFontMgr->LogFont2.lfHeight, gpFontMgr->LogFont2.lfWidth);
	SetDlgItemText(hDlg, tRealFontBorders, szTemp);
}

void CSetPgInfo::FillConsoleMode(HWND hDlg, CRealConsole* pRCon)
{
	WORD nConInMode = 0, nConOutMode = 0;
	TermEmulationType Term = te_win32;
	BOOL bBracketedPaste = FALSE;
	pRCon->GetConsoleModes(nConInMode, nConOutMode, Term, bBracketedPaste);
	CEActiveAppFlags appFlags = pRCon->GetActiveAppFlags();

	wchar_t szFlags[128] = L"";
	switch (Term)
	{
	case te_win32:
		wcscpy_c(szFlags, L"win32"); break;
	case te_xterm:
		wcscpy_c(szFlags, L"xterm"); break;
	default:
		msprintf(szFlags, countof(szFlags), L"term=%u", Term);
	}
	if (bBracketedPaste)
		wcscat_c(szFlags, L"|BrPaste");
	if (appFlags & caf_Cygwin1)
		wcscat_c(szFlags, L"|cygwin");
	if (appFlags & caf_Msys1)
		wcscat_c(szFlags, L"|msys");
	if (appFlags & caf_Msys2)
		wcscat_c(szFlags, L"|msys2");
	if (appFlags & caf_Clink)
		wcscat_c(szFlags, L"|clink");

	wchar_t szInfo[255];
	_wsprintf(szInfo, SKIPLEN(countof(szInfo))
		L"Console states (In=x%02X, Out=x%02X, %s)",
		nConInMode, nConOutMode, szFlags);
	SetDlgItemText(hDlg, IDC_CONSOLE_STATES, szInfo);
}

void CSetPgInfo::FillCursorInfo(HWND hDlg, const ConsoleInfoArg* pInfo)
{
	wchar_t szCursor[64];
	_wsprintf(szCursor, SKIPLEN(countof(szCursor)) L"%ix%i, %i %s",
		(int)pInfo->crCursor.X, (int)pInfo->crCursor.Y,
		pInfo->cInfo.dwSize, pInfo->cInfo.bVisible ? L"vis" : L"hid");
	SetDlgItemText(hDlg, tCursorPos, szCursor);
}
