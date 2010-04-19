
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
#define MCHKHEAP
#define DEBUGSTRMENU(s) DEBUGSTR(s)


#include <windows.h>
#include "ConEmuTh.h"
#include "../common/farcolor.hpp"
#include "resource.h"
#include "ImgCache.h"


static ATOM hClass = NULL;
LRESULT CALLBACK DisplayWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
DWORD WINAPI DisplayThread(LPVOID lpvParam);
void Paint(HWND hwnd, PAINTSTRUCT& ps, RECT& rc, CeFullPanelInfo* pi);
const wchar_t gsDisplayClassName[] = L"ConEmuPanelView";
HANDLE ghCreateEvent = NULL;
extern HICON ghUpIcon;
int gnCreateViewError = 0;
DWORD gnWin32Error = 0;
//ITEMIDLIST DesktopID = {{0}};
//IShellFolder *gpDesktopFolder = NULL;
BOOL gbCancelAll = FALSE;
//CThumbnails *gpImgCache = NULL;
extern CImgCache  *gpImgCache;
extern COLORREF gcrColors[16];

#define MSG_CREATE_VIEW (WM_USER+101)
#define CREATE_WND_TIMEOUT 5000


HWND CreateView(CeFullPanelInfo* pi)
{
	gnCreateViewError = 0;
	gnWin32Error = 0;

	HWND hView = (pi->bLeftPanel) ? ghLeftView : ghRightView;
	if (hView) {
		_ASSERTE(hView==NULL);
		if (IsWindow(hView)) {
			return hView;
		}
		hView = NULL;
	}
	pi->hView = NULL;
	pi->cbSize = sizeof(*pi);
	
	if (!gnDisplayThreadId || !ghDisplayThread) {
		if (ghDisplayThread) {
			CloseHandle(ghDisplayThread); ghDisplayThread = NULL;
		}

		// —начала нужно запустить нить (ее еще не создавали)
		HANDLE hReady = CreateEvent(NULL,FALSE,FALSE,NULL);
		ghDisplayThread = CreateThread(NULL,0,DisplayThread,(LPVOID)hReady,0,&gnDisplayThreadId);
		if (!ghDisplayThread) {
			gnWin32Error = GetLastError();
			gnCreateViewError = CECreateThreadFailed;
			return NULL;
		}

		// ƒождатьс€, пока нить оживет
		DWORD nTimeout = CREATE_WND_TIMEOUT;
		#ifdef _DEBUG
					if (IsDebuggerPresent())
						nTimeout = 60000;
		#endif
		HANDLE hEvents[2] = {ghDisplayThread,hReady};
		DWORD dwWait = WaitForMultipleObjects(2, hEvents, FALSE, nTimeout);
		CloseHandle(hReady); hReady = NULL;
		if (dwWait != (WAIT_OBJECT_0+1)) {
			gnWin32Error = GetLastError();
			gnCreateViewError = CEThreadActivationFailed;
			return NULL;
		}

		// » еще чуть подождать, а то возникает "Invalid thread identifier"
		Sleep(50);
	}
	
	if (GetCurrentThreadId() != gnDisplayThreadId) {
		ghCreateEvent = CreateEvent(NULL,FALSE,FALSE,NULL);
		BOOL lbRc = PostThreadMessage(gnDisplayThreadId, MSG_CREATE_VIEW, (WPARAM)ghCreateEvent, (LPARAM)pi);
		if (!lbRc) {
			gnWin32Error = GetLastError();
			gnCreateViewError = CEPostThreadMessageFailed;
			CloseHandle(ghCreateEvent); ghCreateEvent = NULL;
		} else {
			DWORD nTimeout = CREATE_WND_TIMEOUT;
			#ifdef _DEBUG
					if (IsDebuggerPresent())
						nTimeout = 60000;
			#endif
			HANDLE hEvents[2] = {ghDisplayThread,ghCreateEvent};
			DWORD dwWait = WaitForMultipleObjects(2, hEvents, FALSE, nTimeout);
			CloseHandle(ghCreateEvent); ghCreateEvent = NULL;
			if (dwWait == (WAIT_OBJECT_0)) {
				// Ќить завершилась
				gnCreateViewError = CEDisplayThreadTerminated;
				GetExitCodeThread(ghDisplayThread, &gnWin32Error);
			} else if (dwWait == (WAIT_OBJECT_0+1)) {
				hView = pi->hView;
			}
		}
		return hView;
	}
	
	if (!hClass) {
		WNDCLASS wc = {0};
		wc.style = CS_OWNDC|CS_DBLCLKS|CS_HREDRAW|CS_VREDRAW;
		wc.lpfnWndProc = DisplayWndProc;
		// 16 + <Disabled> + 3 зарезервировано
		// <Disabled> - это когда поверх панели расположен диалог (то есть не перехватывать клавиатуру)
		wc.cbWndExtra = 20*sizeof(DWORD);
		wc.hInstance = (HINSTANCE)ghPluginModule;
		wc.hCursor = LoadCursor(NULL, IDC_ARROW);
		wc.hbrBackground = (HBRUSH)GetStockObject(HOLLOW_BRUSH);
		wc.lpszClassName = gsDisplayClassName;
		hClass = RegisterClass(&wc);
		if (!hClass) {
			gnWin32Error = GetLastError();
			gnCreateViewError = CERegisterClassFailed;
			return NULL;
		}
	}
	
	pi->cbSize = sizeof(*pi);
	
	wchar_t szTitle[128];
	wsprintf(szTitle, L"ConEmu.%sPanelView.%i", (pi->bLeftPanel) ? L"Left" : L"Right", gnSelfPID);
	hView = CreateWindow(gsDisplayClassName, szTitle, WS_CHILD|WS_CLIPSIBLINGS, 0,0,0,0, 
		ghConEmuRoot, NULL, (HINSTANCE)ghPluginModule, (LPVOID)pi);
	pi->hView = hView;
	if (!hView) {
		gnWin32Error = GetLastError();
		gnCreateViewError = CECreateWindowFailed;
	} else {
		if (pi->bLeftPanel)
			ghLeftView = hView;
		else
			ghRightView = hView;
	}
	
	return hView;
}

LRESULT CALLBACK DisplayWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg) {
	case WM_CREATE:
		{
			CREATESTRUCT* pcr = (CREATESTRUCT*)lParam;
			CeFullPanelInfo* pi = (CeFullPanelInfo*)pcr->lpCreateParams;
			_ASSERTE(pi && pi->cbSize==sizeof(CeFullPanelInfo));
			_ASSERTE(pi == (&pviLeft) || pi == (&pviRight));
			
			SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)pi);
			
			return 0; //continue creation
		}
	case WM_PAINT:
		{
			CeFullPanelInfo* pi = (CeFullPanelInfo*)GetWindowLongPtr(hwnd, GWLP_USERDATA);
			_ASSERTE(pi && pi->cbSize==sizeof(CeFullPanelInfo));
			_ASSERTE(pi == (&pviLeft) || pi == (&pviRight));
			
			//BYTE nPanelColorIdx = pi->nFarColors[COL_PANELTEXT];
			//COLORREF nBackColor = GetWindowLong(hwnd, 4*((nPanelColorIdx & 0xF0)>>4));
			//#ifdef _DEBUG
			//COLORREF nForeColor = GetWindowLong(hwnd, 4*(nPanelColorIdx & 0xF));
			//#endif
			
			PAINTSTRUCT ps = {NULL};
			HDC hdc = BeginPaint(hwnd, &ps);
			if (hdc) {
				RECT rc; GetClientRect(hwnd, &rc);
				Paint(hwnd, ps, rc, pi);
				EndPaint(hwnd, &ps);
			}
			
			return 0;
		}
	case WM_ERASEBKGND:
		{
			CeFullPanelInfo* pi = (CeFullPanelInfo*)GetWindowLongPtr(hwnd, GWLP_USERDATA);
			_ASSERTE(pi && pi->cbSize==sizeof(CeFullPanelInfo));
			_ASSERTE(pi == (&pviLeft) || pi == (&pviRight));
			BYTE nPanelColorIdx = pi->nFarColors[COL_PANELTEXT];
			COLORREF nBackColor = GetWindowLong(hwnd, 4*((nPanelColorIdx & 0xF0)>>4));
			RECT rc; GetClientRect(hwnd, &rc);
			HBRUSH hbr = CreateSolidBrush(nBackColor);
			_ASSERTE(wParam!=0);
			FillRect((HDC)wParam, &rc, hbr);
			DeleteObject(hbr);
			return TRUE;
		}
	case WM_CLOSE:
		{
			if (ghLeftView == hwnd)
				ghLeftView = NULL;
			else if (ghRightView == hwnd)
				ghRightView = NULL;

			DestroyWindow(hwnd);

			if (!ghLeftView && !ghRightView)
				PostThreadMessage(gnDisplayThreadId, WM_QUIT, 0, 0);
			return 0;
		}
	}

	return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

DWORD WINAPI DisplayThread(LPVOID lpvParam)
{
	MSG msg;
	HANDLE hReady = (HANDLE)lpvParam;

	CoInitialize(NULL);

	_ASSERTE(gpImgCache);



	// ¬ыставл€ем событие, что нить готова
	if (hReady) {
		SetEvent(hReady);
	} else {
		_ASSERTE(hReady!=NULL);
	}

	while (GetMessage(&msg,0,0,0)) {

		if (msg.message == MSG_CREATE_VIEW) {
			HANDLE hEvent = (HANDLE)msg.wParam;
			CeFullPanelInfo* pi = (CeFullPanelInfo*)msg.lParam;
			if (hEvent != ghCreateEvent) {
				_ASSERTE(hEvent == ghCreateEvent);
			} else {
				HWND hView = CreateView(pi);
				_ASSERTE(pi->hView == hView);
				pi->hView = hView;
				SetEvent(hEvent);
			}
			continue;
		}

		// default
		DispatchMessage(&msg);
	}

	if (hClass) {
		UnregisterClass(gsDisplayClassName, (HINSTANCE)ghPluginModule);
		hClass = NULL;
	}

	//CloseHandle(ghDisplayThread); ghDisplayThread = NULL;
	gnDisplayThreadId = 0;

	delete gpImgCache;
	gpImgCache = NULL;

	// ќсвободить пам€ть
	pviLeft.FreeInfo();
	pviRight.FreeInfo();

	UpdateEnvVar(FALSE);
	
	CoUninitialize();
	
	return 0;
}


BOOL PaintItem(HDC hdc, int x, int y, CePluginPanelItem* pItem, BOOL abCurrentItem, 
			   COLORREF *nBackColor, COLORREF *nForeColor, HBRUSH *hBack,
			   int nXIcon, int nYIcon, int nXIconSpace, int nYIconSpace,
			   BOOL abAllowPreview, HBRUSH hWhiteBrush)
{
	const wchar_t* pszName = pItem->FindData.lpwszFileNamePart;
	int nLen = lstrlen(pszName);
	//int nDrawRC = -1;
	//SHFILEINFO sfi = {NULL};
	//UINT cbSize = sizeof(sfi);
	//DWORD_PTR shRc = 0;
	//HICON hIcon = NULL;
	//HBITMAP hBmp = NULL;

	if (gbCancelAll)
		return FALSE;


	if (gThSet.nThumbFrame == 1) {
		Rectangle(hdc,
			x+gThSet.nHPadding, y+gThSet.nVPadding,
			x+gThSet.nHPadding+gThSet.nWidth, y+gThSet.nVPadding+gThSet.nHeight);
	} else if (gThSet.nThumbFrame == 0) {
		RECT rcTmp = {x+gThSet.nHPadding, y+gThSet.nVPadding,
			x+gThSet.nHPadding+gThSet.nWidth, y+gThSet.nVPadding+gThSet.nHeight};
		FillRect(hdc, &rcTmp, hWhiteBrush);
	} else {
		_ASSERTE(gThSet.nThumbFrame==0 || gThSet.nThumbFrame==1);
	}

	gpImgCache->PaintItem(hdc, x+gThSet.nThumbFrame+gThSet.nHPadding, y+gThSet.nThumbFrame+gThSet.nVPadding,
		pItem, abAllowPreview);

	RECT rcClip = {
		x+gThSet.nHPadding, 
		y+gThSet.nVPadding+gThSet.nHeight,
		x+gThSet.nHPadding+gThSet.nWidth,
		y+gThSet.nVPadding+gThSet.nHeight+gThSet.nFontHeight+2
	};
	COLORREF crBack = 0, crFore = 0;

	int nIdx = ((pItem->Flags & (0x40000000/*PPIF_SELECTED*/)) ? 
		((abCurrentItem/*nItem==nCurrentItem*/) ? 3 : 1) :
		((abCurrentItem/*nItem==nCurrentItem*/) ? 2 : 0));
	crBack = nBackColor[nIdx];
	crFore = nForeColor[nIdx];
	if ((pItem->FindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
		&& ((pItem->Flags & (0x40000000/*PPIF_SELECTED*/)) == 0))
		crFore = 0xFFFFFF;

	FillRect(hdc, &rcClip, hBack[nIdx]);

	SetTextColor(hdc, crFore);
	SetBkColor(hdc, crBack);
	//ExtTextOut(hdc, rcClip.left,rcClip.top, ETO_CLIPPED, &rcClip, pszName, nLen, NULL);
	DrawText(hdc, pszName, nLen, &rcClip, DT_END_ELLIPSIS|DT_NOPREFIX|DT_SINGLELINE);

	return TRUE;
}

void Paint(HWND hwnd, PAINTSTRUCT& ps, RECT& rc, CeFullPanelInfo* pi)
{
	gbCancelAll = FALSE;

	for (int i=0; i<16; i++)
		gcrColors[i] = GetWindowLong(hwnd, 4*i);

	BYTE nIndexes[4] = {
		pi->nFarColors[COL_PANELTEXT],
		pi->nFarColors[COL_PANELSELECTEDTEXT],
		pi->nFarColors[COL_PANELCURSOR],
		pi->nFarColors[COL_PANELSELECTEDCURSOR]
	};
	COLORREF nBackColor[4], nForeColor[4];
	HBRUSH hBack[4];
	int i;
	for (i = 0; i < 4; i++) {
		nBackColor[i] = gcrColors[((nIndexes[i] & 0xF0)>>4)];
		nForeColor[i] = gcrColors[(nIndexes[i] & 0xF)];
		hBack[i] = CreateSolidBrush(nBackColor[i]);
	}
	COLORREF crGray = gcrColors[8];
	COLORREF crWhite = nBackColor[0]; //gcrColors[15];
		
	HDC hdc = ps.hdc;

	
	
	HPEN hPen = CreatePen(PS_SOLID, 1, crGray);
	HBRUSH hWhiteBrush = CreateSolidBrush(crWhite);
	HFONT hFont = CreateFont(gThSet.nFontHeight,0,0,0,400,0,0,0,ANSI_CHARSET,OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,
		NONANTIALIASED_QUALITY,DEFAULT_PITCH,gThSet.sFontName);

	// ѕередернуть класс на предмет смены/инициализации настроек
	gpImgCache->Init(hWhiteBrush);

	
	int nWholeW = gThSet.nWidth  + gThSet.nHSpacing + gThSet.nHPadding*2;
	int nWholeH = gThSet.nHeight + gThSet.nVSpacing + gThSet.nVPadding*2;
	int nXCount = (rc.right+gThSet.nHSpacing) / nWholeW; // тут четко, кусок иконки не допускаетс€
	if (nXCount < 1) nXCount = 1;
	int nYCountFull = (rc.bottom+gThSet.nVSpacing) / nWholeH; // тут четко, кусок иконки не допускаетс€
	if (nYCountFull < 1) nYCountFull = 1;
	int nYCount = (rc.bottom+gThSet.nHeight+gThSet.nVSpacing) / nWholeH; // а тут допускаетс€ отображение верхней части иконки
	if (nYCount < 1) nYCount = 1;
	pi->nXCount = nXCount; pi->nYCountFull = nYCountFull; pi->nYCount = nYCount;
	
	int nTopItem = pi->TopPanelItem;
	int nItemCount = pi->ItemsNumber;
	int nCurrentItem = pi->CurrentItem;
	//CePluginPanelItem** ppItems = pi->ppItems;

	if ((nTopItem + nXCount*nYCountFull) <= nCurrentItem) {
		TODO("¬ыравнивание на границу nXCount");
		nTopItem = nCurrentItem - (nXCount*(nYCountFull-1));
	}
	int nMod = nTopItem % nXCount;
	if (nMod) {
		nTopItem = max(0,nTopItem-nMod);
		//if (nTopItem > nCurrentItem)
		//	nTopItem = max(nCurrentItem,(nTopItem-nXCount));
	}
	pi->OurTopPanelItem = nTopItem;

	
	int nMaxLen = (pi->pszPanelDir ? lstrlen(pi->pszPanelDir) : 0) + MAX_PATH+3;
	wchar_t* pszFull = (wchar_t*)malloc(nMaxLen*2);
	wchar_t* pszNamePtr = NULL;
	if (pi->pszPanelDir && *pi->pszPanelDir) {
		lstrcpy(pszFull, pi->pszPanelDir);
		pszNamePtr = pszFull+lstrlen(pszFull);
		if (*(pszNamePtr-1) != L'\\') {
			*(pszNamePtr++) = L'\\';
			*pszNamePtr = 0;
		}
	} else {
		pszNamePtr = pszFull;
	}
	
	int nXIcon = GetSystemMetrics(SM_CXICON);
	int nYIcon = GetSystemMetrics(SM_CYICON);
	int nXIconSpace = (gThSet.nWidth - nXIcon) >> 1;
	int nYIconSpace = (gThSet.nHeight - nYIcon) >> 1;

	HDC hCompDC = CreateCompatibleDC(hdc);
	HBITMAP hCompBmp = CreateCompatibleBitmap(hdc, nWholeW, nWholeH);
	HBITMAP hOldCompBmp = (HBITMAP)SelectObject(hCompDC, hCompBmp);
	{
		RECT rcComp = {0,0,nWholeW,nWholeH};
		FillRect(hCompDC, &rcComp, hBack[0]);
	}
	HPEN hOldPen = (HPEN)SelectObject(hCompDC, hPen);
	HBRUSH hOldBr = (HBRUSH)SelectObject(hCompDC, hWhiteBrush);
	HFONT hOldFont = (HFONT)SelectObject(hCompDC,hFont);


	if (!pi->Focus)
		nCurrentItem = -1;


	for (int nStep = 0; !gbCancelAll && nStep <= 1; nStep++)
	{
		int nItem = nTopItem;
		int nYCoord = 0;

		for (int Y = 0; !gbCancelAll && Y < nYCount && nItem < nItemCount; Y++) {
			int nXCoord = 0;
			for (int X = 0; !gbCancelAll && X < nXCount && nItem < nItemCount; X++, nItem++) {
				// ќбновить информацию об элементе (им€, веделенность, и т.п.)
				if (nStep == 0 || pi->ppItems[nItem] == NULL) {
					LoadPanelItemInfo(pi, nItem);
				}

				CePluginPanelItem* pItem = pi->ppItems[nItem];
				if (!pItem) {
					continue; // ќшибка?
				}
			
				// поехали
				const wchar_t* pszName = pItem->FindData.lpwszFileName;
				if (wcschr(pszName, L'\\')) {
					// Ёто уже может быть полный путь (TempPanel?)
					pItem->pszFullName = pszName;
				} else {
					// ѕолный путь нужно сформировать
					lstrcpyn(pszNamePtr, pszName, MAX_PATH+1);
					pItem->pszFullName = pszFull;
				}

				if (PaintItem(hCompDC, 0, 0, pItem, (nItem==nCurrentItem), 
					nBackColor, nForeColor, hBack,
					nXIcon, nYIcon, nXIconSpace, nYIconSpace, 
					(nStep == 1), hWhiteBrush))
				{
					BitBlt(hdc, nXCoord, nYCoord, nWholeW, nWholeH, hCompDC, 0,0, SRCCOPY);
				}

				nXCoord += nWholeW;
			}

			if (!nStep && nXCoord < rc.right) {
				RECT rcComp = {nXCoord,nYCoord,rc.right,nYCoord+nWholeH};
				FillRect(hdc, &rcComp, hBack[0]);
			}

			nYCoord += nWholeH;
		}
		if (!nStep && nYCoord < rc.bottom) {
			RECT rcComp = {0,nYCoord,rc.right,rc.bottom};
			FillRect(hdc, &rcComp, hBack[0]);
		}
	}

	free(pszFull);

	//SafeRelease(gpDesktopFolder);

	SelectObject(hCompDC, hOldPen);     DeleteObject(hPen);
	SelectObject(hCompDC, hOldBr);      DeleteObject(hWhiteBrush);
	SelectObject(hCompDC, hOldFont);    DeleteObject(hFont);
	SelectObject(hCompDC, hOldCompBmp); DeleteObject(hCompBmp);
	DeleteDC(hCompDC);

	
	for (i = 0; i < 4; i++) DeleteObject(hBack[i]);

//#ifdef _DEBUG
//	BitBlt(hdc, 0,0,rc.right,rc.bottom, gpImgCache->hField[0], 0,0, SRCCOPY);
//#endif
}
