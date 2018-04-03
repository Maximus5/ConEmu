
/*
Copyright (c) 2010-present Maximus5
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

#pragma once

class MSection;

struct CeFullPanelInfo
{
	DWORD cbSize;
	HWND  hView;
	//HANDLE hPanel;
	PanelViewMode PVM;
	ThumbSizes Spaces;
	wchar_t sFontName[32]; // Tahoma
	int nFontHeight; // 14
	//
	int nWholeW, nWholeH; // инициализируется при первом Paint
	int nXCountFull; // тут четко, кусок иконки не допускается
	int nXCount; // а тут допускается отображение левой части иконки
	int nYCountFull; // тут четко, кусок иконки не допускается
	int nYCount; // а тут допускается отображение верхней части иконки
	//
	CEFarInterfaceSettings FarInterfaceSettings;
	CEFarPanelSettings FarPanelSettings;
	BOOL  bLeftPanel, bPlugin;
	BOOL IsFilePanel;
	int PanelMode; // 0..9 - текущий режим панели.
	BOOL Visible;
	BOOL ShortNames;
	BOOL Focus;
	RECT  PanelRect;
	RECT  WorkRect; // "рабочий" прямоугольник. где собственно файлы лежат
	INT_PTR ItemsNumber;
	//
	INT_PTR CurrentItem;
	INT_PTR TopPanelItem;
	// Это мы хотим выставить при следующем Synchro
	bool bRequestItemSet;
	INT_PTR ReqCurrentItem;
	INT_PTR ReqTopPanelItem;
	//
	INT_PTR OurTopPanelItem; // он может НЕ совпадать с фаровским, чтобы CurrentItem был таки видим
	unsigned __int64 Flags; // CEPANELINFOFLAGS
	// ************************
	//int nMaxFarColors;
	BYTE nFarColors[col_LastIndex]; // Массив цветов фара
	COLORREF crLastBackBrush;
	HBRUSH hLastBackBrush;
	// ************************
	size_t nMaxPanelDir;
	wchar_t* pszPanelDir;
	size_t nMaxPanelGetDir;
	void* pFarPanelDirectory;
	// ************************
	INT_PTR nMaxItemsNumber;
	CePluginPanelItem** ppItems;
	CePluginPanelItemColor* pItemColors;
	MSection* pSection;
	// ************************
	size_t nFarTmpBuf;    // Временный буфер для получения
	LPVOID pFarTmpBuf; // информации об элементе панели


	int RegisterPanelView();
	int UnregisterPanelView(bool abHideOnly=false);
	void Close();
	HWND CreateView();
	BOOL ReallocItems(INT_PTR anCount);
	void FinalRelease();

	// Эта "дисплейная" функция вызывается из основной нити, там можно дергать FAR Api
	void DisplayReloadPanel();

	// Эта функция Safe-thread - ее можно дергать из любой нити
	void RequestSetPos(INT_PTR anCurrentItem, INT_PTR anTopItem, BOOL abSetFocus = FALSE);

	static LRESULT CALLBACK DisplayWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
	static DWORD WINAPI DisplayThread(LPVOID lpvParam);
	INT_PTR CalcTopPanelItem(INT_PTR anCurrentItem, INT_PTR anTopItem);
	void Invalidate();
	static void InvalidateAll();
	void Paint(HWND hwnd, PAINTSTRUCT& ps, RECT& rc);
	BOOL PaintItem(HDC hdc, INT_PTR nIndex, int x, int y, CePluginPanelItem* pItem, CePluginPanelItemColor* pItemColor,
	               BOOL abCurrentItem, BOOL abSelectedItem,
	               /*COLORREF *nBackColor, COLORREF *nForeColor, HBRUSH *hBack,*/
	               /*BOOL abAllowPreview,*/ HBRUSH hBackBrush, HBRUSH hPanelBrush, COLORREF crPanelColor);
	int DrawItemText(HDC hdc, LPRECT prcText, LPRECT prcMaxText, CePluginPanelItem* pItem, LPCWSTR pszComments, HBRUSH hBr, BOOL bIgnoreFileDescription);
	BOOL OnSettingsChanged(BOOL bInvalidate);
	BOOL GetIndexFromWndCoord(int x, int y, INT_PTR &rnIndex);
	BOOL GetConCoordFromIndex(INT_PTR nIndex, COORD& rCoord);
	HBRUSH GetItemColors(INT_PTR nIndex, CePluginPanelItem* pItem, CePluginPanelItemColor* pItemColor, BOOL abCurrentItem, COLORREF &crFore, COLORREF &crBack);
	void LoadItemColors(INT_PTR nIndex, CePluginPanelItem* pItem, CePluginPanelItemColor* pItemColor, BOOL abCurrentItem, BOOL abStrictConsole);
	// Conversions
	BOOL FarItem2CeItem(INT_PTR anIndex,
	                    const wchar_t*   asName,
	                    const wchar_t*   asDesc,
	                    DWORD            dwFileAttributes,
	                    FILETIME         ftLastWriteTime,
	                    unsigned __int64 anFileSize,
	                    BOOL             abVirtualItem,
						HANDLE           ahPlugin,
	                    DWORD_PTR        apUserData,
						FARPROC          afFreeUserData,
	                    unsigned __int64 anFlags,
	                    DWORD            anNumberOfLinks);
	BOOL BisItem2CeItem(INT_PTR anIndex,
	                    BOOL abLoaded,
	                    unsigned __int64 anFlags = 0,
	                    COLORREF acrForegroundColor = 0,
	                    COLORREF acrBackgroundColor = 0,
	                    int anPosX = 0, int anPosY = 0); // 1-based, relative to Far workspace
						

	/*{
	if (ppItems) {
	for (int i=0; i<ItemsNumber; i++)
	if (ppItems[i]) free(ppItems[i]);
	free(ppItems);
	ppItems = NULL;
	}
	nMaxItemsNumber = 0;
	if (pszPanelDir) {
	free(pszPanelDir);
	pszPanelDir = NULL;
	}
	nMaxPanelDir = 0;
	if (nFarColors) {
	free(nFarColors);
	nFarColors = NULL;
	}
	nMaxFarColors = 0;
	if (pFarTmpBuf) {
	free(pFarTmpBuf);
	pFarTmpBuf = NULL;
	}
	nFarTmpBuf = 0;
	}*/
};
