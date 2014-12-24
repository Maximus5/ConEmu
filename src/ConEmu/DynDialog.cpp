
/*
Copyright (c) 2014 Maximus5
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

#include "DynDialog.h"

#ifdef _DEBUG
void CDynDialog::UnitTests()
{
	UINT nDlgID[] = {
		IDD_SETTINGS, IDD_SPG_MAIN,
		IDD_SPG_COLORS, IDD_SPG_INFO, IDD_SPG_FEATURE, IDD_SPG_DEBUG, IDD_SPG_FEATURE_FAR, IDD_SPG_CMDTASKS,
		IDD_SPG_APPDISTINCT, IDD_SPG_COMSPEC, IDD_SPG_STARTUP, IDD_SPG_STATUSBAR, IDD_SPG_APPDISTINCT2, IDD_SPG_CURSOR,
		IDD_SPG_INTEGRATION, IDD_SPG_TRANSPARENT, IDD_SPG_WNDSIZEPOS, IDD_SPG_MARKCOPY, IDD_SPG_TABS, IDD_SPG_VIEWS,
		IDD_SPG_KEYS, IDD_SPG_UPDATE, IDD_SPG_CONTROL, IDD_SPG_SHOW, IDD_SPG_TASKBAR, IDD_SPG_DEFTERM, IDD_SPG_FARMACRO,
		IDD_SPG_HIGHLIGHT, IDD_SPG_PASTE, IDD_SPG_CONFIRM, IDD_SPG_HISTORY, IDD_SPG_BACK,
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

CDynDialog::CDynDialog(UINT nDlgId)
	: mh_Dlg(NULL)
	, mn_DlgId(nDlgId)
	, mlp_Template(NULL)
	, mp_pointsize(0)
	, mn_TemplateLength(0)
{
}

CDynDialog::~CDynDialog()
{
	SafeFree(mlp_Template);
}

INT_PTR CDynDialog::ShowDialog(bool Modal, HWND hWndParent, DLGPROC lpDialogFunc, LPARAM dwInitParam)
{
	INT_PTR iRc = 0;

	if (!LoadTemplate())
	{
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

bool CDynDialog::LoadTemplate()
{
	if (mlp_Template)
	{
		_ASSERTE(FALSE && "Template was already loaded!");
		return true;
	}

	bool bOk = false;
	HRSRC hsrc;
	HGLOBAL hrc;
	wchar_t szInfo[80];
	DLGTEMPLATEEX* ptr;
	DWORD_PTR data;
	DWORD nErr = 0;

	hsrc = FindResource(g_hInstance, MAKEINTRESOURCE(mn_DlgId), MAKEINTRESOURCE(RT_DIALOG));
	if (!hsrc)
	{
		nErr = GetLastError();
		_wsprintf(szInfo, SKIPCOUNT(szInfo) L"FindResource failed, ID=%u", mn_DlgId);
		goto wrap;
	}
	hrc = LoadResource(g_hInstance, hsrc);
	if (!hrc)
	{
		nErr = GetLastError();
		_wsprintf(szInfo, SKIPCOUNT(szInfo) L"LoadResource failed, ID=%u", mn_DlgId);
		goto wrap;
	}
	mn_TemplateLength = SizeofResource(g_hInstance, hsrc);
	if (!mn_TemplateLength)
	{
		nErr = GetLastError();
		_wsprintf(szInfo, SKIPCOUNT(szInfo) L"SizeofResource failed, ID=%u", mn_DlgId);
		goto wrap;
	}
	ptr = (DLGTEMPLATEEX*)LockResource(hrc);
	if (!ptr)
	{
		nErr = GetLastError();
		_wsprintf(szInfo, SKIPCOUNT(szInfo) L"LockResource failed, ID=%u", mn_DlgId);
		goto wrap;
	}
	if ((ptr->signature == 0xFFFF) && (ptr->dlgVer != 1))
	{
		nErr = GetLastError();
		_wsprintf(szInfo, SKIPCOUNT(szInfo) L"Unsupported dialog resource type, ID=%u, ver=x%04X, sign=x%04X", mn_DlgId, ptr->dlgVer, ptr->signature);
		goto wrap;
	}

	mlp_Template = (DLGTEMPLATEEX*)calloc(mn_TemplateLength, 1);
	if (!mlp_Template)
	{
		nErr = GetLastError();
		_wsprintf(szInfo, SKIPCOUNT(szInfo) L"Resource data allocation failed, ID=%u, size=%u", mn_DlgId, mn_TemplateLength);
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
			_wsprintf(szInfo, SKIPCOUNT(szInfo) L"Parse DialogEx failed, ID=%u, pos=%u", mn_DlgId, (DWORD)cchPos);
			goto wrap;
		}
	}
	else
	{
		if (!ParseDialog(data))
		{
			size_t cchPos = data - (DWORD_PTR)mlp_Template;
			nErr = (DWORD)-1;
			_wsprintf(szInfo, SKIPCOUNT(szInfo) L"Parse Dialog failed, ID=%u, pos=%u", mn_DlgId, (DWORD)cchPos);
			goto wrap;
		}
	}

	bOk = true;
wrap:
	if (!bOk)
	{
		DisplayLastError(szInfo, nErr);
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

		static wchar_t Button[] = L"Button";
		static wchar_t Edit[] = L"Edit";
		static wchar_t Static[] = L"Static";
		static wchar_t ListBox[] = L"ListBox";
		static wchar_t ScrollBar[] = L"ScrollBar";
		static wchar_t ComboBox[] = L"ComboBox";
		static wchar_t Unknown[] = L"Unknown";

		switch (*((LPWORD)data))
		{
		case 0x0080: pszItemType = Button; break;
		case 0x0081: pszItemType = Edit; break;
		case 0x0082: pszItemType = Static; break;
		case 0x0083: pszItemType = ListBox; break;
		case 0x0084: pszItemType = ScrollBar; break;
		case 0x0085: pszItemType = ComboBox; break;
		default:
			_ASSERTE(FALSE && "Unknown control type?");
			pszItemType = Unknown;
		}

		data += sizeof(WORD);
	}

	return pszItemType;
}
