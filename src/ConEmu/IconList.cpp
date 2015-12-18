
/*
Copyright (c) 2009-2015 Maximus5
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

#define DEBUGSTRICON(s) //DEBUGSTR(s)

#include <windows.h>
#include <commctrl.h>
#include "header.h"
#include "ConEmu.h"
#include "IconList.h"
#include "Options.h"
#include "OptionsClass.h"
#include "ToolImg.h"
#include "../common/MSectionSimple.h"

#ifdef _DEBUG
#include "LoadImg.h"
#endif

CIconList::CIconList()
{
	mh_TabIcons = NULL;
	mn_AdminIcon = -1;
	mpcs = new MSectionSimple(true);
}

CIconList::~CIconList()
{
	for (INT_PTR i = 0; i < m_Icons.size(); i++)
	{
		SafeFree(m_Icons[i].pszIconDescr);
	}
	m_Icons.clear();

	for (INT_PTR i = 0; i < m_Dimensions.size(); i++)
	{
		HIMAGELIST h = m_Dimensions[i].hTabIcons;
		if (h) ImageList_Destroy(h);
	}
	m_Dimensions.clear();

	SafeDelete(mpcs);
}

bool CIconList::IsInitialized()
{
	return (m_Dimensions.size() != 0);
}

void CIconList::EvalDefaultTabIconSize(int& cx, int& cy)
{
	_ASSERTE((mn_SysCxIcon > 0) && (mn_SysCyIcon > 0));

	int iFontY = gpSetCls->EvalSize(gpSet->nTabFontHeight, esf_Vertical|esf_CanUseUnits|esf_CanUseDpi|esf_CanUseZoom);
	if (iFontY < 0)
		iFontY = gpSetCls->EvalFontHeight(gpSet->sTabFontFace, iFontY, gpSet->nTabFontHeight);

	cx = cy = 16;

	if (iFontY > 16)
	{
		EvalNearestIconSize(iFontY+1, cx, cy);
	}
}

void CIconList::EvalNearestIconSize(int nQueryY, int& cx, int& cy)
{
	_ASSERTE((mn_SysCxIcon > 0) && (mn_SysCyIcon > 0));

	#ifdef _DEBUG
	int iDpyY = gpSetCls->EvalSize(max(16,mn_SysCyIcon), esf_Vertical|esf_CanUseDpi);
	#endif

	cx = cy = 16;

	int iSizes[] = {20, 24, 28, 32, 40, 48, 60, 64};

	for (INT_PTR j = countof(iSizes)-1; j >= 0; j--)
	{
		if (nQueryY > iSizes[j])
		{
			cy = iSizes[j];
			cx = (mn_SysCxIcon * iSizes[j]) / mn_SysCyIcon;
			break;
		}
	}
}

bool CIconList::Initialize()
{
	bool lbOk = true;

	if (!IsInitialized())
	{
		mn_AdminIcon = -1;

		mn_SysCxIcon = GetSystemMetrics(SM_CXSMICON); mn_SysCyIcon = GetSystemMetrics(SM_CYSMICON);
		if (mn_SysCxIcon < 16 || mn_SysCyIcon < 16)
		{
			mn_SysCxIcon = mn_SysCyIcon = 16;
		}

		//EvalDefaultTabIconSize();

		lbOk = CreateDimension(mn_SysCxIcon, mn_SysCyIcon);
	}

	return lbOk;
}

bool CIconList::CreateDimension(int cx, int cy)
{
	bool lbOk = false;
	HIMAGELIST hTabIcons = NULL;

	wchar_t szLog[100];
	_wsprintf(szLog, SKIPCOUNT(szLog) L"Creating IconList for size {%ix%i} SysIcon size is {%ix%i}", mn_CxIcon, mn_CyIcon, mn_SysCxIcon, mn_SysCyIcon);
	gpConEmu->LogString(szLog);

	if ((hTabIcons = ImageList_Create(mn_CxIcon, mn_CyIcon, ILC_COLOR24|ILC_MASK, 0, 16)) != NULL)
	{
		CToolImg img;
		int nFirstAdd = -1;
		const int iShieldButtons = 4;
		if (img.Create(mn_CxIcon, mn_CyIcon, iShieldButtons, GetSysColor(COLOR_BTNFACE)))
		{
			if (img.AddButtons(g_hInstance, IDB_SHIELD16, iShieldButtons))
			{
				HBITMAP hShieldUser = img.GetBitmap();
				#ifdef _DEBUG
				BITMAP bmi = {};
				GetObject(hShieldUser, sizeof(bmi), &bmi);
				#ifdef _DEBUG
				//SaveImageEx(L"T:\\ShieldUser.png", hShieldUser);
				#endif
				#endif

				nFirstAdd = ImageList_AddMasked(hTabIcons, hShieldUser, RGB(128,0,0));
			}
		}

		#ifdef _DEBUG
		int iCount = ImageList_GetImageCount(hTabIcons);
		_ASSERTE(iCount==4);
		IMAGEINFO ii = {}; BOOL b;
		// An application should not call DeleteObject to destroy the bitmaps retrieved by ImageList_GetImageInfo
		b = ImageList_GetImageInfo(hTabIcons, 0, &ii);
		b = ImageList_GetImageInfo(hTabIcons, 1, &ii);
		#endif

		if (nFirstAdd >= 0)
		{
			lbOk = true;

			int nAdminIcon = nFirstAdd + ((gOSVer.dwMajorVersion >= 6) ? 0 : 1);
			if (mn_AdminIcon == -1)
			{
				mn_AdminIcon = nAdminIcon;
			}
			else
			{
				_ASSERTE(mn_AdminIcon == nAdminIcon);
			}

			Dimensions dim = {};
			push_back;
			if (pd) *pd = dim;
		}
	}

	return lbOk;
}

int CIconList::GetTabIcon(bool bAdmin)
{
	int iIconIdx = (bAdmin && gpSet->isAdminShield()) ? mn_AdminIcon : -1;
	return iIconIdx;
}

int CIconList::CreateTabIcon(LPCWSTR asIconDescr, bool bAdmin, LPCWSTR asWorkDir)
{
	if (bAdmin && !gpSet->isAdminShield())
		bAdmin = false;

	if (!asIconDescr || !*asIconDescr)
		return GetTabIcon(bAdmin);

	int iCreatedIcon = -1;

	MSectionLockSimple CS; CS.Lock(mpcs, 30000);

	for (INT_PTR i = 0; i < m_Icons.size(); i++)
	{
		const TabIconCache& icn = m_Icons[i];
		if ((icn.bAdmin!=FALSE) != bAdmin)
			continue;
		if (lstrcmpi(icn.pszIconDescr, asIconDescr) != 0)
			continue;

		// If tab was not created yet?
		if (icn.bWasNotLoaded && mh_TabIcons)
		{
			m_Icons.erase(i);
			break;
		}

		// Already was created!
		iCreatedIcon = icn.nIconIdx;
		goto wrap;
	}

	iCreatedIcon = mh_TabIcons ? CreateTabIconInt(asIconDescr, bAdmin, asWorkDir) : -1;

	if (iCreatedIcon == -1)
	{
		// To avoid numerous CreateTabIconInt calls - just remember "No icon" for that asIconDescr
		TabIconCache DummyIcon = {lstrdup(asIconDescr), -1, bAdmin, (mh_TabIcons==NULL)};
		m_Icons.push_back(DummyIcon);
	}
wrap:
	return iCreatedIcon;
}

int CIconList::CreateTabIconInt(LPCWSTR asIconDescr, bool bAdmin, LPCWSTR asWorkDir)
{
	wchar_t* pszExpanded = ExpandEnvStr(asIconDescr);

	// Need to be created!
	int iIconIdx = -1;
	HICON hFileIcon = NULL;
	CEStr szLoadFile;
	LPCWSTR lpszExt = NULL;
	LPCWSTR lpszIndex = NULL;
	int nIndex = 0;

	if (!szLoadFile.Set(pszExpanded ? pszExpanded : asIconDescr))
	{
		goto wrap;
	}

	lpszExt = PointToExt(szLoadFile);
	lpszIndex = wcsrchr(lpszExt ? lpszExt : szLoadFile, L',');
	if (lpszIndex)
	{
		szLoadFile.ms_Val[lpszIndex - szLoadFile.ms_Val] = 0;

		nIndex = wcstol(lpszIndex + 1, NULL, 10);
		if (nIndex == LONG_MAX || nIndex == LONG_MIN)
		{
			nIndex = 0;
		}
	}

	bool bDirChanged = false;
	if (asWorkDir && *asWorkDir)
	{
		// Executable (or icon) file may be not availbale by %PATH%, let "cd" to it...
		bDirChanged = gpConEmu->ChangeWorkDir(asWorkDir);
	}

	if (!lpszExt)
	{
		if (apiSearchPath(NULL, szLoadFile, L".exe", szLoadFile))
		{
			lpszExt = PointToExt(szLoadFile);
		}

		if (!lpszExt)
			goto wrap;
	}

	if (lstrcmpi(lpszExt, L".ico") == 0)
	{
		hFileIcon = (HICON)LoadImage(0, szLoadFile, IMAGE_ICON, mn_CxIcon, mn_CyIcon, LR_DEFAULTCOLOR|LR_LOADFROMFILE);
	}
	else if ((lstrcmpi(lpszExt, L".exe") == 0) || (lstrcmpi(lpszExt, L".dll") == 0))
	{
		HICON hIconLarge = NULL, hIconSmall = NULL;
		ExtractIconEx(szLoadFile, nIndex, &hIconLarge, &hIconSmall, 1);
		bool bUseLargeIcon = ((mn_CxIcon > 16) && (hIconLarge != NULL)) || (hIconSmall == NULL);
		HICON hDestroyIcon = bUseLargeIcon ? hIconSmall : hIconLarge;
		if (hDestroyIcon) DestroyIcon(hDestroyIcon);
		hFileIcon = bUseLargeIcon ? hIconLarge : hIconSmall;
	}
	else
	{
		//TODO: Shell icons for registered files (cmd, bat, sh, pl, py, ...)
	}

	if (hFileIcon)
	{
		wchar_t szIconInfo[80] = L"", szMergedInfo[80] = L"";
		GetIconInfoStr(hFileIcon, szIconInfo);

		if (gpSetCls->isAdvLogging)
		{
			CEStr lsLog(lstrmerge(L"Icon `", asIconDescr, L"` was loaded: ", szIconInfo));
			gpConEmu->LogString(lsLog);
		}

		int iIconIdxAdm = -1;
		iIconIdx = ImageList_ReplaceIcon(mh_TabIcons, -1, hFileIcon);

		TabIconCache NewIcon = {lstrdup(asIconDescr), iIconIdx, false};
		m_Icons.push_back(NewIcon);

		if (mn_AdminIcon >= 0)
		{
			HIMAGELIST hAdmList = ImageList_Merge(mh_TabIcons, iIconIdx, mh_TabIcons, mn_AdminIcon+2, 0,0);
			if (hAdmList)
			{
				HICON hNewIcon = ImageList_GetIcon(hAdmList, 0, ILD_TRANSPARENT);
				if (hNewIcon)
				{
					CEStr lsLog(lstrmerge(L"Admin icon `", asIconDescr, L"` was created: ", GetIconInfoStr(hNewIcon, szMergedInfo)));
					gpConEmu->LogString(lsLog);

					iIconIdxAdm = ImageList_ReplaceIcon(mh_TabIcons, -1, hNewIcon);
					DestroyIcon(hNewIcon);

					TabIconCache AdmIcon = {lstrdup(asIconDescr), iIconIdxAdm, true};
					m_Icons.push_back(AdmIcon);

					if (bAdmin && (iIconIdxAdm > 0))
					{
						iIconIdx = iIconIdxAdm;
					}
				}
				else
				{
					gpConEmu->LogString(L"GetIcon for admin icon was failed");
				}
				ImageList_Destroy(hAdmList);
			}
			else
			{
				gpConEmu->LogString(L"Admin icon merging was failed");
			}
		}

		DestroyIcon(hFileIcon);
	}

wrap:
	if (bDirChanged)
	{
		gpConEmu->ChangeWorkDir(NULL);
	}
	SafeFree(pszExpanded);
	if (gpSetCls->isAdvLogging && (iIconIdx < 0))
	{
		CEStr lsLog(lstrmerge(L"Icon `", asIconDescr, L"` loading was failed"));
		gpConEmu->LogString(lsLog);
	}
	return iIconIdx;
}

LPCWSTR CIconList::GetIconInfoStr(HICON h, wchar_t (&szInfo)[80])
{
	ICONINFO ii = {};
	GetIconInfo(h, &ii);
	BITMAP bi = {};
	GetObject(ii.hbmColor, sizeof(bi), &bi);
	_wsprintf(szInfo, SKIPCOUNT(szInfo) L"{%ix%i} planes=%u bpp=%u", bi.bmWidth, bi.bmHeight, bi.bmPlanes, bi.bmBitsPixel);
	SafeDeleteObject(ii.hbmColor);
	SafeDeleteObject(ii.hbmMask);
	return szInfo;
}

HICON CIconList::GetTabIconByIndex(int IconIndex)
{
	if (!this || !mh_TabIcons)
		return NULL;
	HICON hIcon = ImageList_GetIcon(mh_TabIcons, IconIndex, ILD_TRANSPARENT);
	return hIcon;
}

HIMAGELIST CIconList::GetImageList(int nMaxHeight)
{
	if (!this)
		return NULL;

	int cx = 16, cy = 16;
	Dimensions dim = {};
	wchar_t szLog[120];
	bool bFound = false;

	if ((nMaxHeight <= 0) && (m_Dimensions.size() > 0))
	{
		dim = m_Dimensions[0];
	}

	if (!bFound)
	{
		EvalNearestIconSize(nMaxHeight, cx, cy);
		for (INT_PTR i = 0; i < m_Dimensions; i++)
		{
			const Dimensions& d = m_Dimensions[i];
			if (d.cxIcon == cx && d.cyIcon == cy)
			{
				bFound = true;
				dim = d;
			}
		}
	}

	if (!bFound)
	{
		bFound = CreateDimension(cx, cy, &dim);
	}

	_wsprintf(szLog, SKIPCOUNT(szLog) L"IconList for size %i requested, get (%ix%i)", nMaxHeight, dim.cx, dim.cy);
	gpConEmu->LogString(szLog);
	return dim.hTabIcons;
}
