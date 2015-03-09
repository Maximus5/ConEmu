
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

CIconList::CIconList()
{
	mh_TabIcons = NULL;
	mn_AdminIcon = -1;
	mpcs = new MSectionSimple(true);
}

CIconList::~CIconList()
{
	if (mh_TabIcons)
	{
		ImageList_Destroy(mh_TabIcons);
		mh_TabIcons = NULL;
	}

	for (INT_PTR i = 0; i < m_Icons.size(); i++)
	{
		SafeFree(m_Icons[i].pszIconDescr);
	}
	m_Icons.clear();

	SafeDelete(mpcs);
}

bool CIconList::IsInitialized()
{
	return (mh_TabIcons != NULL);
}

bool CIconList::Initialize()
{
	bool lbOk = true;

	if (!mh_TabIcons)
	{
		int iSysX = GetSystemMetrics(SM_CXSMICON), iSysY = GetSystemMetrics(SM_CYSMICON);
		mn_CxIcon = gpSetCls->EvalSize(max(16,iSysX), esf_Horizontal|esf_CanUseDpi);
		mn_CyIcon = gpSetCls->EvalSize(max(16,iSysY), esf_Vertical|esf_CanUseDpi);

		wchar_t szLog[100];
		_wsprintf(szLog, SKIPCOUNT(szLog) L"Creating IconList for size {%ix%i} SysIcon size is {%ix%i}", mn_CxIcon, mn_CyIcon, iSysX, iSysY);
		gpConEmu->LogString(szLog);

		if ((mh_TabIcons = ImageList_Create(mn_CxIcon, mn_CyIcon, ILC_COLOR24|ILC_MASK, 0, 16)) != NULL)
		{
			CToolImg img;
			int nFirstAdd = -1;
			const int iShieldButtons = 4;
			if (img.Create(mn_CxIcon * iShieldButtons, mn_CyIcon, 1, GetSysColor(COLOR_BTNFACE)))
			{
				if (img.AddButtons(g_hInstance, IDB_SHIELD16, iShieldButtons))
				{
					HBITMAP hShieldUser = img.GetBitmap();
					#ifdef _DEBUG
					BITMAP bmi = {};
					GetObject(hShieldUser, sizeof(bmi), &bmi);
					#endif

					nFirstAdd = ImageList_AddMasked(mh_TabIcons, hShieldUser, RGB(128,0,0));
				}
			}

			#ifdef _DEBUG
			int iCount = ImageList_GetImageCount(mh_TabIcons);
			_ASSERTE(iCount==4);
			IMAGEINFO ii = {}; BOOL b;
			b = ImageList_GetImageInfo(mh_TabIcons, 0, &ii);
			b = ImageList_GetImageInfo(mh_TabIcons, 1, &ii);
			#endif

			if (nFirstAdd >= 0)
			{
				mn_AdminIcon = nFirstAdd + ((gOSVer.dwMajorVersion >= 6) ? 0 : 1);
			}
			else
			{
				lbOk = false;
			}
		}
		else
		{
			lbOk = false;
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
		// Already was created!
		iCreatedIcon = icn.nIconIdx;
		goto wrap;
	}

	iCreatedIcon = CreateTabIconInt(asIconDescr, bAdmin, asWorkDir);

	if (iCreatedIcon == -1)
	{
		// To avoid numerous CreateTabIconInt calls - just remember "No icon" for that asIconDescr
		TabIconCache DummyIcon = {lstrdup(asIconDescr), bAdmin, -1};
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
	wchar_t szTemp[MAX_PATH];
	LPCWSTR pszLoadFile = pszExpanded ? pszExpanded : asIconDescr;
	LPCWSTR lpszExt = (wchar_t*)PointToExt(pszLoadFile);

	bool bDirChanged = false;
	if (asWorkDir && *asWorkDir)
	{
		// Executable (or icon) file may be not availbale by %PATH%, let "cd" to it...
		bDirChanged = gpConEmu->ChangeWorkDir(asWorkDir);
	}

	if (!lpszExt)
	{
		LPWSTR pszFile = NULL;
		if (SearchPath(NULL, pszLoadFile, L".exe", countof(szTemp), szTemp, &pszFile))
		{
			pszLoadFile = szTemp;
			lpszExt = (wchar_t*)PointToExt(pszLoadFile);
		}

		if (!lpszExt)
			goto wrap;
	}

	if (lstrcmpi(lpszExt, L".ico") == 0)
	{
		hFileIcon = (HICON)LoadImage(0, pszLoadFile, IMAGE_ICON, mn_CxIcon, mn_CyIcon, LR_DEFAULTCOLOR|LR_LOADFROMFILE);
	}
	else if ((lstrcmpi(lpszExt, L".exe") == 0) || (lstrcmpi(lpszExt, L".dll") == 0))
	{
		//TODO: May be specified index of an icon in the file
		HICON hIconLarge = NULL, hIconSmall = NULL;
		ExtractIconEx(pszLoadFile, 0, &hIconLarge, &hIconSmall, 1);
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
		if (gpSetCls->isAdvLogging)
		{
			ICONINFO ii = {};
			GetIconInfo(hFileIcon, &ii);
			BITMAP bi = {};
			GetObject(ii.hbmColor, sizeof(bi), &bi);
			wchar_t szInfo[80];
			_wsprintf(szInfo, SKIPCOUNT(szInfo) L"{%ix%i} planes=%u bpp=%u", bi.bmWidth, bi.bmHeight, bi.bmPlanes, bi.bmBitsPixel);
			CEStr lsLog(lstrmerge(L"Icon `", asIconDescr, L"` was loaded: ", szInfo));
			gpConEmu->LogString(lsLog);
		}

		int iIconIdxAdm = -1;
		iIconIdx = ImageList_ReplaceIcon(mh_TabIcons, -1, hFileIcon);

		TabIconCache NewIcon = {lstrdup(asIconDescr), false, iIconIdx};
		m_Icons.push_back(NewIcon);

		if (mn_AdminIcon >= 0)
		{
			HIMAGELIST hAdmList = ImageList_Merge(mh_TabIcons, iIconIdx, mh_TabIcons, mn_AdminIcon+2, 0,0);
			if (hAdmList)
			{
				HICON hNewIcon = ImageList_GetIcon(hAdmList, 0, ILD_TRANSPARENT);
				if (hNewIcon)
				{
					iIconIdxAdm = ImageList_ReplaceIcon(mh_TabIcons, -1, hNewIcon);
					DestroyIcon(hNewIcon);

					TabIconCache AdmIcon = {lstrdup(asIconDescr), true, iIconIdxAdm};
					m_Icons.push_back(AdmIcon);

					if (bAdmin && (iIconIdxAdm > 0))
					{
						iIconIdx = iIconIdxAdm;
					}
				}
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

HICON CIconList::GetTabIconByIndex(int IconIndex)
{
	if (!this || !mh_TabIcons)
		return NULL;
	HICON hIcon = ImageList_GetIcon(mh_TabIcons, IconIndex, ILD_TRANSPARENT);
	return hIcon;
}
