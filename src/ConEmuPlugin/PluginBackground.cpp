
/*
Copyright (c) 2009-2010 Maximus5
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

THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
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


#define SHOWDEBUGSTR
//#define MCHKHEAP
#define DEBUGSTRBG(s) //DEBUGSTR(s)

#define THSET_TIMEOUT 1000

#undef HIDE_TODO

//#include <stdio.h>
#include <windows.h>
//#include <windowsx.h>
//#include <string.h>
//#include <tchar.h>
#include "..\common\common.hpp"
#include "..\common\SetHook.h"
#include "..\common\pluginW1007.hpp"
#include "..\common\ConsoleAnnotation.h"
#include "..\common\WinObjects.h"
#include "PluginHeader.h"
#include "PluginBackground.h"

#include "../common/ConEmuCheck.h"


BOOL gbNeedBgActivate = FALSE; // требуется активация модуля в главной нити
BOOL gbBgPluginsAllowed = FALSE;
CPluginBackground *gpBgPlugin = NULL;

extern BOOL WINAPI IsConsoleActive();

CPluginBackground::CPluginBackground()
{
	mn_BgPluginsCount = 0;
	mn_BgPluginsMax = 16;
	mp_BgPlugins = (struct BackgroundInfo*)calloc(mn_BgPluginsMax,sizeof(struct BackgroundInfo));
	mn_ReqActions = 0;
	
	csBgPlugins = new MSection();
	
	m_LastThSetCheck = 0;
	
	memset(&m_ThSet, 0, sizeof(m_ThSet));
	
	memset(&m_Default, 0, sizeof(m_Default));
	//m_Default.pszLeftCurDir    = ms_LeftCurDir; ms_LeftCurDir[0] = 0;
	//m_Default.pszLeftFormat    = ms_LeftFormat; ms_LeftFormat[0] = 0;
	//m_Default.pszLeftHostFile  = ms_LeftHostFile; ms_LeftHostFile[0] = 0;
	//m_Default.pszRightCurDir   = ms_RightCurDir; ms_RightCurDir[0] = 0;
	//m_Default.pszRightFormat   = ms_RightFormat; ms_RightFormat[0] = 0;
	//m_Default.pszRightHostFile = ms_RightHostFile; ms_RightHostFile[0] = 0;
	
	if (ConEmuHwnd && IsWindow(ConEmuHwnd))
	{
		LoadThSet();
	}
}

CPluginBackground::~CPluginBackground()
{
	if (csBgPlugins)
	{
		delete csBgPlugins;
		csBgPlugins = NULL;
	}
}

int CPluginBackground::RegisterSubplugin(BackgroundInfo *pbk)
{
	if (!pbk)
	{
		_ASSERTE(pbk != NULL);
		return -1;
	}
	if (!gbBgPluginsAllowed)
	{
		_ASSERTE(gbBgPluginsAllowed == TRUE);
		return -2;
	}
	
	
	MSectionLock SC; SC.Lock(csBgPlugins, TRUE);
	
	// Выделение памяти под плагины
	if (mp_BgPlugins == NULL || (mn_BgPluginsCount == mn_BgPluginsMax))
		ReallocItems(16);
		
	// go
	BOOL lbNeedResort = FALSE;

	if (pbk->Cmd == rbc_Register)
	{
		// Память уже выделена
		int nFound = -1, nFirstEmpty = -1;
		for (int i = 0; i < mn_BgPluginsCount; i++)
		{
			if (mp_BgPlugins[i].Cmd == rbc_Register &&
				mp_BgPlugins[i].hPlugin == pbk->hPlugin &&
				mp_BgPlugins[i].UpdateConEmuBackground == pbk->UpdateConEmuBackground &&
				mp_BgPlugins[i].lParam == pbk->lParam)
			{
				nFound = i; break;
			}
			if (nFirstEmpty == -1 && 0 == (int)mp_BgPlugins[i].Cmd)
				nFirstEmpty = i;
		}
		if (nFound == -1)
		{
			if (nFirstEmpty > 0)
				nFound = nFirstEmpty;
			else
				nFound = mn_BgPluginsCount++;
			lbNeedResort = (mn_BgPluginsCount>1);
		}
		mp_BgPlugins[nFound] = *pbk;
	}
	else if (pbk->Cmd == rbc_Unregister)
	{
		for (int i = 0; i < mn_BgPluginsCount; i++)
		{
			if (mp_BgPlugins[i].Cmd == rbc_Register &&
				mp_BgPlugins[i].hPlugin == pbk->hPlugin &&
				((pbk->UpdateConEmuBackground == NULL) ||
				 (mp_BgPlugins[i].UpdateConEmuBackground == pbk->UpdateConEmuBackground
				  && mp_BgPlugins[i].lParam == pbk->lParam)))
			{
				memset(mp_BgPlugins+i, 0, sizeof(*mp_BgPlugins));
				lbNeedResort = TRUE;
			}
		}
		WARNING("Если количество зарегистрированных плагинов уменьшилось до 0");
		// Послать в GUI CECMD_SETBACKGROUND{bEnabled = FALSE}
		// Проверить, чтобы не было противного мелькания при закрытии FAR
	}
	else if (pbk->Cmd == rbc_Redraw)
	{
		// просто выставить gbNeedBgActivate
	}
	
	//TODO: Сортировка
	if (lbNeedResort)
	{
		WARNING("Сортировка зарегистрированных плагинов - CPluginBackground::RegisterBackground");
	}
	
	// Когда активируется MainThread - обновить background
	mn_ReqActions |= ra_UpdateBackground;
	gbNeedBgActivate = TRUE;
	// В фар2 сразу дернем Synchro
	if (IS_SYNCHRO_ALLOWED)
	{
		ExecuteSynchro();
	}
	return 0;
}

void CPluginBackground::ReallocItems(int nAddCount)
{
	_ASSERTE(nAddCount > 0);
	int nNewMax = mn_BgPluginsMax + nAddCount;
	_ASSERTE(nNewMax > 0);
	struct BackgroundInfo *pNew = (struct BackgroundInfo*)calloc(nNewMax,sizeof(struct BackgroundInfo));
	if (mp_BgPlugins)
	{
		if (mn_BgPluginsCount > 0)
			memmove(pNew, mp_BgPlugins, mn_BgPluginsCount*sizeof(struct BackgroundInfo));
		free(mp_BgPlugins);
	}
	mp_BgPlugins = pNew;
	mn_BgPluginsMax = nNewMax;
}

void CPluginBackground::OnMainThreadActivated(int anEditorEvent /*= -1*/, int anViewerEvent /*= -1*/)
{
	if ((mn_ReqActions & ra_CheckPanelFolders))
	{
		mn_ReqActions &= ~ra_CheckPanelFolders;
		CheckPanelFolders();
	}
	
	if ((mn_ReqActions & ra_UpdateBackground))
	{
		mn_ReqActions &= ~ra_UpdateBackground;
		UpdateBackground();
	}
}


void CPluginBackground::CheckPanelFolders()
{
	if (gFarVersion.dwVerMajor==1 || gFarVersion.dwBuild < 1348)
		FillUpdateBackgroundA(&m_Default);
	else if (gFarVersion.dwBuild>=FAR_Y_VER)
		FUNC_Y(FillUpdateBackground)(&m_Default);
	else
		FUNC_X(FillUpdateBackground)(&m_Default);
}

void CPluginBackground::SetDcPanelRect(RECT *rcDc, UpdateBackgroundArg::BkPanelInfo *Panel, UpdateBackgroundArg *Arg)
{
	if (!Panel->bVisible)
	{
		rcDc->left = rcDc->top = rcDc->right = rcDc->bottom = -1;
	}
	else
	{
		int nConW = Arg->rcConWorkspace.right - Arg->rcConWorkspace.left + 1;
		int nConH = Arg->rcConWorkspace.bottom - Arg->rcConWorkspace.top + 1;
		if (nConW < 1)
		{
			_ASSERTE(nConW>0);
			nConW = 1;
		}
		if (nConH < 1)
		{
			_ASSERTE(nConH>0);
			nConH = 1;
		}
		rcDc->left = (Panel->rcPanelRect.left - Arg->rcConWorkspace.left) * Arg->dcSizeX / nConW;
		rcDc->top = (Panel->rcPanelRect.top - Arg->rcConWorkspace.top) * Arg->dcSizeY / nConH;
		rcDc->right = (Panel->rcPanelRect.right - Arg->rcConWorkspace.left + 1) * Arg->dcSizeX / nConW;
		rcDc->bottom = (Panel->rcPanelRect.bottom - Arg->rcConWorkspace.left + 1) * Arg->dcSizeY / nConH;
	}
}

// Вызывается ТОЛЬКО в главной нити!
void CPluginBackground::UpdateBackground()
{
	if (!ConEmuHwnd || !IsWindow(ConEmuHwnd))
		return;

	RECT rcClient; GetClientRect(ConEmuHwnd, &rcClient);

	struct UpdateBackgroundArg Arg = m_Default;
	Arg.cbSize = sizeof(struct UpdateBackgroundArg);
	m_Default.dcSizeX = Arg.dcSizeX = rcClient.right+1;
	m_Default.dcSizeY = Arg.dcSizeY = rcClient.bottom+1;
	SetDcPanelRect(&Arg.rcDcLeft, &Arg.LeftPanel, &Arg);
	SetDcPanelRect(&Arg.rcDcRight, &Arg.RightPanel, &Arg);
	if (gpTabs->Tabs.CurrentType == WTYPE_EDITOR)
		Arg.Place = bup_Editor;
	else if (gpTabs->Tabs.CurrentType == WTYPE_VIEWER)
		Arg.Place = bup_Viewer;
	else if (Arg.LeftPanel.bVisible || Arg.RightPanel.bVisible)
		Arg.Place = bup_Panels;

	//_ASSERTE(Arg.LeftPanel.bVisible || Arg.RightPanel.bVisible);

	HDC hScreen = GetDC(NULL);
	HDC hdc = CreateCompatibleDC(hScreen);
	Arg.hdc = hdc;
	BITMAPINFOHEADER bi = {sizeof(BITMAPINFOHEADER)};
	bi.biWidth = Arg.dcSizeX;
	bi.biHeight = Arg.dcSizeY;
	bi.biPlanes = 1;
	bi.biBitCount = 32;
	bi.biCompression = BI_RGB;
	COLORREF* pBits = NULL;
	HBITMAP hDib = CreateDIBSection(hScreen, (BITMAPINFO*)&bi, DIB_RGB_COLORS, (void**)&pBits, NULL, 0);

	ReleaseDC(NULL, hScreen); hScreen = NULL;

	if (!hDib || !pBits)
	{
		_ASSERTE(hDib && pBits);
		return;
	}
	HBITMAP hOld = (HBITMAP)SelectObject(hdc, hDib);

	// Залить черным - по умолчанию
#ifdef _DEBUG
	memset(pBits, 128, bi.biWidth*bi.biHeight*4);
#else
	memset(pBits, 0, bi.biWidth*bi.biHeight*4);
#endif

	// Painting!
	int nProcessed = 0;
	MSectionLock SC; SC.Lock(csBgPlugins, TRUE);
	for (int i = 0; i < mn_BgPluginsCount; i++)
	{
		if (mp_BgPlugins[i].Cmd != rbc_Register ||
			!(mp_BgPlugins[i].nPlaces & Arg.Place) ||
			!(mp_BgPlugins[i].UpdateConEmuBackground))
			continue; // пустая, неактивная в данном контексте, или некорректная ячейка

		Arg.nLevel = i;
		Arg.lParam = mp_BgPlugins[i].lParam;
		mp_BgPlugins[i].UpdateConEmuBackground(&Arg);
		nProcessed++;
	}
	SC.Unlock();
	// Sending background to GUI!

	if (nProcessed < 1)
	{
		// Ситуация возникает при старте ConEmu, когда панелей "еще нет"
		//_ASSERTE(nProcessed >= 1);
	}
	else
	{
		GdiFlush();

		DWORD nBitSize = bi.biWidth*bi.biHeight*4;
		DWORD nWholeSize = sizeof(CESERVER_REQ_HDR)+sizeof(CESERVER_REQ_SETBACKGROUND)+nBitSize;

		CESERVER_REQ *pIn = ExecuteNewCmd(CECMD_SETBACKGROUND, nWholeSize);
		if (!pIn)
		{
			_ASSERTE(pIn);
		}
		else
		{
			pIn->Background.nType = 1;
			pIn->Background.bEnabled = TRUE;
			pIn->Background.bmp.bfType = 0x4D42/*BM*/;
			pIn->Background.bmp.bfSize = nBitSize+sizeof(BITMAPFILEHEADER)+sizeof(BITMAPINFOHEADER);
			pIn->Background.bmp.bfOffBits = sizeof(BITMAPFILEHEADER)+sizeof(BITMAPINFOHEADER);
			pIn->Background.bi = bi;
			memmove(((LPBYTE)&pIn->Background.bi)+sizeof(pIn->Background.bi), pBits, bi.biWidth*bi.biHeight*4);

			CESERVER_REQ *pOut = ExecuteGuiCmd(FarHwnd, pIn, FarHwnd);
			if (pOut)
			{
				TODO("Показать ошибку, если есть");
				ExecuteFreeResult(pOut);
			}
			ExecuteFreeResult(pIn);
		}
	}

	SelectObject(hdc, hOld);
	DeleteObject(hDib);
	DeleteDC(hdc);
}

BOOL CPluginBackground::LoadThSet()
{
	if (!ConEmuHwnd || !IsWindow(ConEmuHwnd))
	{
		_ASSERTE(ConEmuHwnd!=NULL);
		return FALSE;
	}

	BOOL lbRc = FALSE;
	MFileMapping<PanelViewSettings> ThSetMap;

	DWORD nGuiPID = 0;
	GetWindowThreadProcessId(ConEmuHwnd, &nGuiPID);

	ThSetMap.InitName(CECONVIEWSETNAME, nGuiPID);
	if (ThSetMap.Open())
	{
		ThSetMap.GetTo(&m_ThSet);
		ThSetMap.CloseMap();
		lbRc = TRUE;
	}

	return lbRc;
}

void CPluginBackground::MonitorBackground()
{
	if (mn_BgPluginsCount == 0 || !ConEmuHwnd || !IsWindow(ConEmuHwnd))
		return;

	BOOL lbUpdateRequired = FALSE;

	DWORD nDelta = (GetTickCount() - m_LastThSetCheck);
	if (nDelta > THSET_TIMEOUT)
	{
		//BYTE nFarColors[0x100]; // Массив цветов фара
		
		if (LoadThSet())
		{
			// Если изменились визуальные параметры CE - перерисоваться
			if (memcmp(&m_Default.MainFont, &m_ThSet.MainFont, sizeof(m_ThSet.MainFont)))
			{
				lbUpdateRequired = TRUE;
				m_Default.MainFont = m_ThSet.MainFont;
			}
			if (memcmp(m_Default.crPalette, m_ThSet.crPalette, sizeof(m_ThSet.crPalette)))
			{
				lbUpdateRequired = TRUE;
				memmove(m_Default.crPalette, m_ThSet.crPalette, sizeof(m_ThSet.crPalette));
			}
		}

		if (!lbUpdateRequired)
		{
			// Проверим, а не изменился ли размер окна?
			if (IsConsoleActive())
			{
				RECT rcClient; GetClientRect(ConEmuHwnd, &rcClient);
				if ((m_Default.dcSizeX != (rcClient.right+1))
					|| (m_Default.dcSizeY != (rcClient.bottom+1)))
				{
					lbUpdateRequired = TRUE;
				}
			}
			if (!lbUpdateRequired)
			{
				TODO("Проверить, не изменился ли размер консольного окна?");
			}
		}
		
		if (lbUpdateRequired)
		{
			// Когда активируется MainThread - обновить background
			mn_ReqActions |= ra_UpdateBackground;
		}
		
		// Но проверить все параметры фара тоже нужно
		mn_ReqActions |= ra_CheckPanelFolders;
		gbNeedBgActivate = TRUE;
	}
}
