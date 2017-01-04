
/*
Copyright (c) 2014-2017 Maximus5
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

#include "../common/CEStr.h"
#include "DynDialog.h"
#include "LngRc.h"

wchar_t CDynDialog::Button[] = L"Button";
wchar_t CDynDialog::Edit[] = L"Edit";
wchar_t CDynDialog::Static[] = L"Static";
wchar_t CDynDialog::ListBox[] = L"ListBox";
wchar_t CDynDialog::ScrollBar[] = L"ScrollBar";
wchar_t CDynDialog::ComboBox[] = L"ComboBox";
wchar_t CDynDialog::Unknown[] = L"Unknown";

CDynDialog* CDynDialog::mp_Creating = NULL;
MMap<HWND,CDynDialog*>* CDynDialog::mp_DlgMap = NULL;

#ifdef _DEBUG
void CDynDialog::UnitTests()
{
	UINT nDlgID[] = {
		IDD_SETTINGS, IDD_SPG_FONTS,
		IDD_SPG_COLORS, IDD_SPG_INFO, IDD_SPG_FEATURES, IDD_SPG_DEBUG, IDD_SPG_FEATURE_FAR, IDD_SPG_TASKS,
		IDD_SPG_APPDISTINCT, IDD_SPG_COMSPEC, IDD_SPG_STARTUP, IDD_SPG_STATUSBAR, IDD_SPG_APPDISTINCT2, IDD_SPG_CURSOR,
		IDD_SPG_INTEGRATION, IDD_SPG_TRANSPARENT, IDD_SPG_SIZEPOS, IDD_SPG_MARKCOPY, IDD_SPG_TABS, IDD_SPG_VIEWS,
		IDD_SPG_KEYS, IDD_SPG_UPDATE, IDD_SPG_APPEAR, IDD_SPG_TASKBAR, IDD_SPG_DEFTERM, IDD_SPG_FARMACRO,
		IDD_SPG_KEYBOARD, IDD_SPG_MOUSE,
		IDD_SPG_HIGHLIGHT, IDD_SPG_PASTE, IDD_SPG_CONFIRM, IDD_SPG_HISTORY, IDD_SPG_BACKGR,
		IDD_MORE_CONFONT, IDD_MORE_DOSBOX, IDD_ATTACHDLG, IDD_FAST_CONFIG, IDD_FIND, IDD_ABOUT,
		IDD_RENAMETAB, IDD_CMDPROMPT, IDD_HELP, IDD_ACTION, IDD_HOTKEY,
		0
	};

	for (int i = 0; nDlgID[i]; i++)
	{
		CDynDialog dlg(nDlgID[i]);
		dlg.LoadTemplate();
	}
}
#endif

CDynDialog* CDynDialog::ShowDialog(UINT nDlgId, HWND hWndParent, DLGPROC lpDialogFunc, LPARAM dwInitParam)
{
	CDynDialog* pDlg = new CDynDialog(nDlgId);
	if (!pDlg->LoadTemplate())
	{
		SafeDelete(pDlg);
		return NULL;
	}

	// Correct font size?
	pDlg->FixupDialogSize();

	// Create modeless dialog
	pDlg->PrepareDlg(lpDialogFunc);
	pDlg->mh_Dlg = CreateDialogIndirectParam(g_hInstance, (LPCDLGTEMPLATEW)pDlg->mlp_Template, hWndParent, DynDialogBox, dwInitParam);
	_ASSERTE(mp_Creating == NULL);
	if (!pDlg->mh_Dlg)
	{
		DisplayLastError(L"CreateDialogIndirectParam failed");
		SafeDelete(pDlg);
	}

	return pDlg;
}

INT_PTR CDynDialog::ExecuteDialog(UINT nDlgId, HWND hWndParent, DLGPROC lpDialogFunc, LPARAM dwInitParam)
{
	CDynDialog* pDlg = new CDynDialog(nDlgId);
	if (!pDlg->LoadTemplate())
	{
		SafeDelete(pDlg);
		return -1;
	}

	// Correct font size?
	pDlg->FixupDialogSize();

	// Create modal dialog
	pDlg->PrepareDlg(lpDialogFunc);
	INT_PTR iRc = DialogBoxIndirectParam(g_hInstance, (LPCDLGTEMPLATEW)pDlg->mlp_Template, hWndParent, DynDialogBox, dwInitParam);
	_ASSERTE(mp_Creating == NULL);
	if (iRc == 0 || iRc == -1)
	{
		DisplayLastError(L"DialogBoxIndirectParam failed");
	}
	SafeDelete(pDlg);

	return iRc;
}

CDynDialog::CDynDialog(UINT nDlgId)
	: mh_Dlg(NULL)
	, mn_DlgId(nDlgId)
	, mp_DlgProc(NULL)
	, mlp_Template(NULL)
	, mp_pointsize(NULL)
	, mpsz_TypeFace(NULL)
	, mn_TemplateLength(0)
	, mpsz_Title(NULL)
{
}

CDynDialog::~CDynDialog()
{
	SafeFree(mlp_Template);

	if (mp_DlgMap && mh_Dlg)
	{
		mp_DlgMap->Del(mh_Dlg);
		if (!mp_DlgMap->GetNext(NULL, NULL, NULL))
		{
			MMap<HWND,CDynDialog*>* p = mp_DlgMap;
			mp_DlgMap = NULL;
			delete p;
		}
	}
}

void CDynDialog::PrepareDlg(DLGPROC lpDialogFunc)
{
	if (!mp_DlgMap)
	{
		mp_DlgMap = new MMap<HWND,CDynDialog*>();
		mp_DlgMap->Init(256, true);
	}
	mp_DlgProc = lpDialogFunc;
	mp_Creating = this;
}

CDynDialog* CDynDialog::GetDlgClass(HWND hwndDlg)
{
	CDynDialog* pDlg = NULL;
	if (mp_DlgMap)
		mp_DlgMap->Get(hwndDlg, &pDlg);
	return pDlg;
}

INT_PTR /*CALLBACK*/ CDynDialog::DynDialogBox(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	INT_PTR iRc = 0;
	CDynDialog* pDlg = NULL;

	if (uMsg == WM_INITDIALOG)
	{
		pDlg = mp_Creating;
		if (mp_DlgMap)
			mp_DlgMap->Set(hwndDlg, pDlg);
		_ASSERTE(pDlg->mh_Dlg == NULL || pDlg->mh_Dlg == hwndDlg);
		pDlg->mh_Dlg = hwndDlg;
		mp_Creating = NULL;
	}
	else
	{
		pDlg = GetDlgClass(hwndDlg);
	}

	if (pDlg && pDlg->mp_DlgProc)
	{
		iRc = pDlg->mp_DlgProc(hwndDlg, uMsg, wParam, lParam);
	}

	if (uMsg == WM_DESTROY)
	{
		if (mp_DlgMap)
		{
			mp_DlgMap->Del(hwndDlg);
			/*
			if (!mp_DlgMap->GetNext(NULL, NULL, NULL))
			{
				delete mp_DlgMap;
				mp_DlgMap = NULL;
			}
			*/
		}
	}

	return iRc;
}

#if 0
INT_PTR CDynDialog::ShowDialog(bool Modal, HWND hWndParent, DLGPROC lpDialogFunc, LPARAM dwInitParam)
{
	INT_PTR iRc = 0;

	if (!LoadTemplate())
	{
		LogString(L"Template loading failed, creating dialog by ID");

		if (Modal)
		{
			iRc = DialogBoxParam(g_hInstance, MAKEINTRESOURCE(mn_DlgId), hWndParent, lpDialogFunc, dwInitParam);
		}
		else
		{
			mh_Dlg = CreateDialogParam(g_hInstance, MAKEINTRESOURCE(mn_DlgId), hWndParent, lpDialogFunc, dwInitParam);
			iRc = (INT_PTR)mh_Dlg;
		}
	}
	else
	{
		if (Modal)
		{
			iRc = DialogBoxIndirectParam(g_hInstance, (LPCDLGTEMPLATEW)mlp_Template, hWndParent, lpDialogFunc, dwInitParam);
		}
		else
		{
			mh_Dlg = CreateDialogIndirectParam(g_hInstance, (LPCDLGTEMPLATEW)mlp_Template, hWndParent, lpDialogFunc, dwInitParam);
			iRc = (INT_PTR)mh_Dlg;
		}
	}

	return iRc;
}
#endif

bool CDynDialog::LoadTemplate()
{
	if (mlp_Template)
	{
		// Template was already loaded
		return true;
	}

	bool bOk = false;
	HRSRC hsrc;
	HGLOBAL hrc;
	wchar_t szInfo[120];
	DLGTEMPLATEEX* ptr;
	DWORD_PTR data;
	DWORD nErr = 0;

	hsrc = FindResource(g_hInstance, MAKEINTRESOURCE(mn_DlgId), RT_DIALOG);
	if (!hsrc)
	{
		nErr = GetLastError();
		_wsprintf(szInfo, SKIPCOUNT(szInfo) L"FindResource failed, RT_DIALOG=%u", mn_DlgId);
		goto wrap;
	}
	hrc = LoadResource(g_hInstance, hsrc);
	if (!hrc)
	{
		nErr = GetLastError();
		_wsprintf(szInfo, SKIPCOUNT(szInfo) L"LoadResource failed, RT_DIALOG=%u", mn_DlgId);
		goto wrap;
	}
	mn_TemplateLength = SizeofResource(g_hInstance, hsrc);
	if (!mn_TemplateLength)
	{
		nErr = GetLastError();
		_wsprintf(szInfo, SKIPCOUNT(szInfo) L"SizeofResource failed, RT_DIALOG=%u", mn_DlgId);
		goto wrap;
	}
	ptr = (DLGTEMPLATEEX*)LockResource(hrc);
	if (!ptr)
	{
		nErr = GetLastError();
		_wsprintf(szInfo, SKIPCOUNT(szInfo) L"LockResource failed, RT_DIALOG=%u", mn_DlgId);
		goto wrap;
	}
	if ((ptr->signature == 0xFFFF) && (ptr->dlgVer != 1))
	{
		nErr = GetLastError();
		_wsprintf(szInfo, SKIPCOUNT(szInfo) L"Unsupported dialog resource type, RT_DIALOG=%u, ver=x%04X, sign=x%04X", mn_DlgId, ptr->dlgVer, ptr->signature);
		goto wrap;
	}

	mlp_Template = (DLGTEMPLATEEX*)calloc(mn_TemplateLength, 1);
	if (!mlp_Template)
	{
		nErr = GetLastError();
		_wsprintf(szInfo, SKIPCOUNT(szInfo) L"Resource data allocation failed, RT_DIALOG=%u, size=%u", mn_DlgId, mn_TemplateLength);
		goto wrap;
	}

	memmove(mlp_Template, ptr, mn_TemplateLength);

	data = (DWORD_PTR)mlp_Template;
	if ((ptr->signature == 0xFFFF) && (ptr->dlgVer == 1))
	{
		if (!ParseDialogEx(data))
		{
			size_t cchPos = data - (DWORD_PTR)mlp_Template;
			nErr = (DWORD)-1;
			_wsprintf(szInfo, SKIPCOUNT(szInfo) L"Parse DialogEx failed, RT_DIALOG=%u, pos=%u", mn_DlgId, (DWORD)cchPos);
			goto wrap;
		}
	}
	else
	{
		if (!ParseDialog(data))
		{
			size_t cchPos = data - (DWORD_PTR)mlp_Template;
			nErr = (DWORD)-1;
			_wsprintf(szInfo, SKIPCOUNT(szInfo) L"Parse Dialog failed, RT_DIALOG=%u, pos=%u", mn_DlgId, (DWORD)cchPos);
			goto wrap;
		}
	}

	bOk = true;
wrap:
	if (!bOk)
	{
		DisplayLastError(szInfo, nErr);
		SafeFree(mlp_Template);
	}
	return bOk;
}

bool CDynDialog::ParseDialog(DWORD_PTR& data)
{
	DLGTEMPLATE* ptr = (DLGTEMPLATE*)data;
	_ASSERTE(ptr == mlp_Template);

	// First item - dialog descriptor itself
	ItemInfo i = {0, FALSE, &ptr->x, &ptr->y, &ptr->cx, &ptr->cy};
	m_Items.push_back(i);

	data += 18; // sizeof may be aligned

	// menu - sz_Or_Ord
	SkipSzOrOrd(data);

	// windowClass - sz_Or_Ord
	SkipSzOrOrd(data);

	//WCHAR     title[titleLen];
	mpsz_Title = SkipSz(data);

	if (ptr->style & (DS_SETFONT))
	{
		//WORD      pointsize;
		mp_pointsize = (LPWORD)data;
		data += sizeof(WORD);
		//WCHAR     typeface[stringLen];
		mpsz_TypeFace = SkipSz(data);
	}

	for (INT_PTR i = ptr->cdit; i--;)
	{
		if (!ParseItem(data))
			return false;
	}

	return true;
}

bool CDynDialog::ParseItem(DWORD_PTR& data)
{
	if (((data - (DWORD_PTR)mlp_Template) + sizeof(DLGITEMTEMPLATE)) > mn_TemplateLength)
	{
		return false;
	}

	// Each DLGITEMTEMPLATE structure in the template must be aligned on a DWORD boundary
	data = (((data + 3) >> 2) << 2);

	DLGITEMTEMPLATE* ptr = (DLGITEMTEMPLATE*)data;
	ItemInfo i = {ptr->id, FALSE, &ptr->x, &ptr->y, &ptr->cx, &ptr->cy};
	i.ptr = ptr;

	i.Style = ptr->style;
	i.pStyle = &ptr->style;

	data += 18; // sizeof may be aligned

	// windowClass - sz_Or_Atom
	i.ItemType = SkipSzOrAtom(data);

	// title - sz_Or_Ord
	SkipSzOrOrd(data);

	// extraCount
	WORD extraCount = *((LPWORD)data);
	data += sizeof(WORD)+extraCount;

	m_Items.push_back(i);
	return true;
}

bool CDynDialog::ParseDialogEx(DWORD_PTR& data)
{
	DLGTEMPLATEEX* ptr = (DLGTEMPLATEEX*)data;
	_ASSERTE(ptr == mlp_Template);

	if ((ptr->signature != 0xFFFF) || (ptr->dlgVer != 1))
	{
		// Must be checked in LoadTemplate()
		_ASSERTE((ptr->signature == 0xFFFF) && (ptr->dlgVer == 1));
		return false;
	}

	// First item - dialog descriptor itself
	ItemInfo i = {0, TRUE, &ptr->x, &ptr->y, &ptr->cx, &ptr->cy};
	m_Items.push_back(i);

	data += 26; // sizeof may be aligned

	// menu - sz_Or_Ord
	SkipSzOrOrd(data);

	// windowClass - sz_Or_Ord
	SkipSzOrOrd(data);

	//WCHAR     title[titleLen];
	mpsz_Title = SkipSz(data);

	if (ptr->style & (DS_SETFONT|DS_SHELLFONT))
	{
		//WORD      pointsize;
		mp_pointsize = (LPWORD)data;
		//WORD      weight;
		//BYTE      italic;
		//BYTE      charset;
		data += 6; // 2*WORD+2*BYTE
		//WCHAR     typeface[stringLen];
		mpsz_TypeFace = SkipSz(data);
	}

	for (INT_PTR i = ptr->cDlgItems; i--;)
	{
		if (!ParseItemEx(data))
			return false;
	}

	return true;
}

bool CDynDialog::ParseItemEx(DWORD_PTR& data)
{
	if (((data - (DWORD_PTR)mlp_Template) + sizeof(DLGITEMTEMPLATEEX)) > mn_TemplateLength)
	{
		return false;
	}

	// Each DLGITEMTEMPLATEEX structure in the template must be aligned on a DWORD boundary
	data = (((data + 3) >> 2) << 2);

	DLGITEMTEMPLATEEX* ptr = (DLGITEMTEMPLATEEX*)data;
	ItemInfo i = {ptr->id, TRUE, &ptr->x, &ptr->y, &ptr->cx, &ptr->cy};
	i.ptrEx = ptr;

	i.Style = ptr->style;
	i.pStyle = &ptr->style;

	data += 24; // sizeof may be aligned

	// windowClass - sz_Or_Atom
	i.ItemType = SkipSzOrAtom(data);

	// title - sz_Or_Ord
	SkipSzOrOrd(data);

	// extraCount
	WORD extraCount = *((LPWORD)data);
	data += sizeof(WORD)+extraCount;

	m_Items.push_back(i);
	return true;
}

LPWSTR CDynDialog::SkipSz(DWORD_PTR& data)
{
	wchar_t* psz = (wchar_t*)data;
	size_t len = wcslen(psz);
	data += (len+1)*sizeof(WORD);
	return psz;
}

LPWSTR CDynDialog::SkipSzOrOrd(DWORD_PTR& data)
{
	LPWSTR pszTitle;

	if (*((LPWORD)data) == 0xFFFF)
	{
		data += sizeof(WORD);
		pszTitle = (LPWSTR)MAKEINTRESOURCE(*((LPWORD)data));
		data += sizeof(WORD);
	}
	else
	{
		pszTitle = SkipSz(data);
	}

	return pszTitle;
}

LPWSTR CDynDialog::SkipSzOrAtom(DWORD_PTR& data)
{
	LPWSTR pszItemType;

	if (*((LPWORD)data) != 0xFFFF)
	{
		pszItemType = SkipSz(data);
	}
	else
	{
		data += sizeof(WORD);

		switch (*((LPWORD)data))
		{
		case 0x0080: pszItemType = CDynDialog::Button; break;
		case 0x0081: pszItemType = CDynDialog::Edit; break;
		case 0x0082: pszItemType = CDynDialog::Static; break;
		case 0x0083: pszItemType = CDynDialog::ListBox; break;
		case 0x0084: pszItemType = CDynDialog::ScrollBar; break;
		case 0x0085: pszItemType = CDynDialog::ComboBox; break;
		default:
			_ASSERTE(FALSE && "Unknown control type?");
			pszItemType = CDynDialog::Unknown;
		}

		data += sizeof(WORD);
	}

	return pszItemType;
}

UINT CDynDialog::GetFontPointSize()
{
	return (mp_pointsize && *mp_pointsize) ? *mp_pointsize : 8;
}

void CDynDialog::FixupDialogSize()
{
	#if 0
	//if (mp_pointsize)
	//	*mp_pointsize = 4;
	for (INT_PTR i = m_Items.size()-1; i >= 0; i--)
	{
		ItemInfo& ii = m_Items[i];
		if (ii.ItemType == CDynDialog::Button)
		{
			// Just use bitfields to check for Radio/Check
			if ((ii.Style & (BS_CHECKBOX|BS_RADIOBUTTON|BS_USERBUTTON)))
			{
				*ii.pStyle = ((ii.Style & ~BS_TYPEMASK) | BS_OWNERDRAW);
			}
		}
	}
	#endif
}

bool CDynDialog::DrawButton(WPARAM wID, DRAWITEMSTRUCT* pDraw)
{
	if (!pDraw || (pDraw->CtlType != ODT_BUTTON))
		return false;

	CDynDialog* pDlg = NULL;
	if (!mp_DlgMap || !mp_DlgMap->Get(GetParent(pDraw->hwndItem), &pDlg))
		return false;

	ItemInfo* p = NULL;
	for (INT_PTR i = pDlg->m_Items.size()-1; i >= 0; i--)
	{
		p = &(pDlg->m_Items[i]);
		if ((p->ItemType == Button)
			&& (p->ID == pDraw->CtlID))
		{
			DWORD style = (p->Style & BS_TYPEMASK);
			switch (style)
			{
			case BS_CHECKBOX: case BS_AUTOCHECKBOX:
			case BS_RADIOBUTTON: case BS_AUTORADIOBUTTON:
			case BS_3STATE:
				pDlg->DrawButton(p, pDraw);
				return true;
			}
		}
	}

	return false;
}

bool CDynDialog::DrawButton(ItemInfo* pItem, DRAWITEMSTRUCT* pDraw)
{
	int ih = pDraw->rcItem.bottom-pDraw->rcItem.top-1;
	RECT rc = {pDraw->rcItem.left+1, pDraw->rcItem.top+1, pDraw->rcItem.left+ih, pDraw->rcItem.top+ih};
	UINT uType = DFC_BUTTON;
	UINT uState = 0;

	DWORD style = (pItem->Style & BS_TYPEMASK);
	switch (style)
	{
	case BS_CHECKBOX: case BS_AUTOCHECKBOX:
		uState = DFCS_BUTTONCHECK|DFCS_FLAT
			| ((pDraw->itemState & ODS_CHECKED) ? DFCS_CHECKED : 0)
			| ((pDraw->itemState & ODS_DISABLED) ? DFCS_INACTIVE : 0)
			;
		break;
	case BS_RADIOBUTTON: case BS_AUTORADIOBUTTON:
		uState = DFCS_BUTTONRADIO/*|DFCS_FLAT*/
			| ((pDraw->itemState & ODS_CHECKED) ? DFCS_CHECKED : 0)
			| ((pDraw->itemState & ODS_DISABLED) ? DFCS_INACTIVE : 0)
			;
		break;
	case BS_3STATE:
		_ASSERTE(FALSE && "BS_3STATE");
		return false;
	}

	DrawFrameControl(pDraw->hDC, &rc, uType, uState);
	return true;

	//HBRUSH hbr = (HBRUSH)GetStockObject(BLACK_BRUSH);
	//FillRect(pDraw->hDC, &pDraw->rcItem, hbr);
	//return true;
}

// static
void CDynDialog::LocalizeDialog(HWND hDlg, UINT nTitleRsrcId /*= 0*/)
{
	#if defined(_DEBUG)
	static HWND hLastDlg = NULL;
	if (hLastDlg != hDlg)
		hLastDlg = hDlg;
	else
		_ASSERTE(hLastDlg != hDlg); // avoid re-localization
	#endif

	// Call it before CLngRc::isLocalized, title may be changed via LngDataRsrcs.h
	if (nTitleRsrcId != 0)
	{
		LPCWSTR pszTitle = CLngRc::getRsrc(nTitleRsrcId);
		if (pszTitle && *pszTitle)
			SetWindowText(hDlg, pszTitle);
	}

	// Control text may be overriden only by ConEmu.l10n
	if (!CLngRc::isLocalized())
		return;
	EnumChildWindows(hDlg, CDynDialog::LocalizeControl, (LPARAM)hDlg);
}

// static, CALLBACK
// Intended to be called from EnumChildWindows
BOOL CDynDialog::LocalizeControl(HWND hChild, LPARAM lParam)
{
	// Nothing to do
	if (!CLngRc::isLocalized())
		return TRUE;

	_ASSERTE(lParam!=NULL);
	_ASSERTE(IsWindow((HWND)lParam));

	// Only first level
	HWND hParent = GetParent(hChild);
	if (hParent != (HWND)lParam)
	{
		return TRUE; // continue enum
	}

	LONG wID = GetWindowLong(hChild, GWL_ID);

	#if 0
	if (wID == stCmdGroupCommands)
		wID = wID; // for debugger breakpoint
	#endif

	_ASSERTE(IDC_STATIC == -1);
	if ((wID != 0)              // Unknown control?
		&& (wID != IDC_STATIC)) // IDC_STATIC
	{
		CEStr lsLoc;
		wchar_t szClass[64];

		if (CLngRc::getControl(wID, lsLoc)
			&& !lsLoc.IsEmpty())
		{
			// BUTTON, STATICTEXT, ...?
			if (GetClassName(hChild, szClass, countof(szClass)))
			{
				if ((lstrcmpi(szClass, L"Button") == 0)
					|| (lstrcmpi(szClass, L"Static") == 0))
				{
					SetWindowText(hChild, lsLoc.ms_Val);
				}
				#ifdef _DEBUG
				else
				{
					int w = wID; // control class is not available for l10n
				}
				#endif
			}
		}
	}

	return TRUE; // continue enumeration
}
