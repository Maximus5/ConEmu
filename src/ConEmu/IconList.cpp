
/*
Copyright (c) 2009-2013 Maximus5
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
#include "IconList.h"
#include "Options.h"

CIconList::CIconList()
{
	mh_TabIcons = NULL;
	mn_AdminIcon = 0;
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
		//mh_TabIcons = ImageList_LoadImage(g_hInstance, MAKEINTRESOURCE(IDB_SHIELD), 14, 1, RGB(128,0,0), IMAGE_BITMAP, LR_CREATEDIBSECTION);
		mh_TabIcons = ImageList_Create(16, 16, ILC_COLOR24|ILC_MASK, 0, 16);
		if (mh_TabIcons)
		{
			HBITMAP hShieldUser = LoadBitmap(g_hInstance, MAKEINTRESOURCE(IDB_SHIELD16));
			int nFirstAdd = ImageList_AddMasked(mh_TabIcons, hShieldUser, RGB(128,0,0));
			DeleteObject(hShieldUser);
			
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
	int iIconIdx = (bAdmin && gpSet->bAdminShield) ? mn_AdminIcon : -1;
	return iIconIdx;
}

int CIconList::CreateTabIcon(LPCWSTR asIconDescr, bool bAdmin)
{
	if (!asIconDescr || !*asIconDescr)
		return GetTabIcon(bAdmin);

	for (INT_PTR i = 0; i < m_Icons.size(); i++)
	{
		const TabIconCache& icn = m_Icons[i];
		if ((icn.bAdmin!=FALSE) != bAdmin)
			continue;
		if (lstrcmpi(icn.pszIconDescr, asIconDescr) != 0)
			continue;
		// Already was created!
		return icn.nIconIdx;
	}

	wchar_t* pszExpanded = ExpandEnvStr(asIconDescr);

	// Need to be created!
	int iIconIdx = -1;
	HICON hFileIcon = NULL;
	wchar_t szTemp[MAX_PATH];
	LPCWSTR pszLoadFile = pszExpanded ? pszExpanded : asIconDescr;
	LPCWSTR lpszExt = (wchar_t*)PointToExt(pszLoadFile);

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
		hFileIcon = (HICON)LoadImage(0, pszLoadFile, IMAGE_ICON,
			                            GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON), LR_DEFAULTCOLOR|LR_LOADFROMFILE);
    }
    else if ((lstrcmpi(lpszExt, L".exe") == 0) || (lstrcmpi(lpszExt, L".dll") == 0))
    {
		//TODO: May be specified index of an icon in the file
		HICON hIconLarge = NULL;
        ExtractIconEx(pszLoadFile, 0, &hIconLarge, &hFileIcon, 1);
		if (hIconLarge) DestroyIcon(hIconLarge);
    }
	else
	{
		//TODO: Shell icons for registered files (cmd, bat, sh, pl, py, ...)
	}

	if (hFileIcon)
	{
		int iIconIdxAdm = -1;
		iIconIdx = ImageList_ReplaceIcon(mh_TabIcons, -1, hFileIcon);

		TabIconCache NewIcon = {lstrdup(asIconDescr), false, iIconIdx};
		m_Icons.push_back(NewIcon);

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

		//TODO: bAdmin!!!

		DestroyIcon(hFileIcon);
	}

wrap:
	SafeFree(pszExpanded);
	return iIconIdx;
}
