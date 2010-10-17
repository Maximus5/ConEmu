
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
	m_Default.pszLeftCurDir    = ms_LeftCurDir; ms_LeftCurDir[0] = 0;
	m_Default.pszLeftFormat    = ms_LeftFormat; ms_LeftFormat[0] = 0;
	m_Default.pszLeftHostFile  = ms_LeftHostFile; ms_LeftHostFile[0] = 0;
	m_Default.pszRightCurDir   = ms_RightCurDir; ms_RightCurDir[0] = 0;
	m_Default.pszRightFormat   = ms_RightFormat; ms_RightFormat[0] = 0;
	m_Default.pszRightHostFile = ms_RightHostFile; ms_RightHostFile[0] = 0;
	
	LoadThSet();
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
}

void CPluginBackground::UpdateBackground()
{
	if (!ConEmuHwnd || !IsWindow(ConEmuHwnd))
		return;

	struct UpdateBackgroundArg def = {sizeof(struct UpdateBackgroundArg)};
	
	RECT rcClient; GetClientRect(ConEmuHwnd, &rcClient);
	
	//struct UpdateBackgroundArg
	//{
	//	DWORD cbSize;
	//
	//	LPARAM lParam; // указан при вызове RegisterBackground(rbc_Register)
	//	enum BackgroundUpdatePlace Place; // панели/редактор/вьювер
	//
	//	HDC hdc; // DC для отрисовки фона. Изначально (для nLevel==0) DC залит цветом фона crPalette[0]
	//	int dcSizeX, dcSizeY; // размер DC в пикселях
	//	// Далее идет информация о консоли. Для удобства и исключения
	//	// ветвления - координаты приведены к 0x0 (т.е. буфер /w можно игнорировать)
	//	int conSizeX, conSizeY; // размер консоли в ячейках (символах)
	//	BOOL bLeft, bRight; // Наличие левой/правой панелей
	//	RECT rcConLeft, rcConRight; // Консольные кооринаты панелей, приведенные к 0x0
	//	RECT rcDcLeft, rcDcRight; // Координаты панелей в DC
	//
	//	// Слой, в котором вызван плагин. По сути, это порядковый номер, 0-based
	//	// если (nLevel > 0) плагин НЕ ДОЛЖЕН затирать фон целиком.
	//	int nLevel;
	//
	//	DWORD dwEventFlags; // комбинация из enum BackgroundUpdateEvent
	//
	//	COLORREF crPalette[16]; // Палитра в ConEmu
	//	BYTE nFarColors[0x100]; // Массив цветов фара
	//
	//    /* Основной шрифт в GUI */
	//    struct {
	//    	wchar_t sFontName[32];
	//    	DWORD nFontHeight, nFontWidth, nFontCellWidth;
	//    	DWORD nFontQuality, nFontCharSet;
	//    	BOOL Bold, Italic;
	//    	wchar_t sBorderFontName[32];
	//        DWORD nBorderFontWidth;
	//    } MainFont;
	//};
}

BOOL CPluginBackground::LoadThSet()
{
	BOOL lbRc = FALSE;
	MFileMapping<PanelViewSettings> ThSetMap;
	_ASSERTE(ConEmuHwnd!=NULL);

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
	if (!ConEmuHwnd || !IsWindow(ConEmuHwnd))
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
