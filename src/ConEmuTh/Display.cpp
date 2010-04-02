
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

static ATOM hClass = NULL;
LRESULT CALLBACK DisplayWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
DWORD WINAPI DisplayThread(LPVOID lpvParam);
void Paint(HWND hwnd, PAINTSTRUCT& ps, RECT& rc, CeFullPanelInfo* pi);
const wchar_t gsDisplayClassName[] = L"ConEmuPanelView";
HANDLE ghCreateEvent = NULL;
HICON ghUpIcon = NULL;


#define MSG_CREATE_VIEW (WM_USER+101)
#define CREATE_WND_TIMEOUT 5000


HWND CreateView(CeFullPanelInfo* pi)
{
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
	
	if (!ghDisplayThread) {
		// Сначала нужно запустить нить (ее еще не создавали)
		HANDLE hReady = CreateEvent(NULL,FALSE,FALSE,NULL);
		ghDisplayThread = CreateThread(NULL,0,DisplayThread,(LPVOID)hReady,0,&gnDisplayThreadId);
		if (!ghDisplayThread) {
			TODO("Показать ошибку");
			return NULL;
		}

		// Дождаться, пока нить оживет
		DWORD nTimeout = CREATE_WND_TIMEOUT;
		#ifdef _DEBUG
					if (IsDebuggerPresent())
						nTimeout = 60000;
		#endif
		HANDLE hEvents[2] = {ghDisplayThread,hReady};
		DWORD dwWait = WaitForMultipleObjects(2, hEvents, FALSE, nTimeout);
		CloseHandle(hReady); hReady = NULL;
		if (dwWait != (WAIT_OBJECT_0+1)) {
			TODO("Показать ошибку");
			return NULL;
		}
	}
	
	if (GetCurrentThreadId() != gnDisplayThreadId) {
		ghCreateEvent = CreateEvent(NULL,FALSE,FALSE,NULL);
		DWORD dwErr = 0;
		BOOL lbRc = PostThreadMessage(gnDisplayThreadId, MSG_CREATE_VIEW, (WPARAM)ghCreateEvent, (LPARAM)pi);
		if (!lbRc) {
			dwErr = GetLastError();
			CloseHandle(ghCreateEvent); ghCreateEvent = NULL;
			TODO("Показать ошибку");
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
				// Нить завершилась
				TODO("Показать ошибку");
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
			TODO("Показать ошибку");
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
		TODO("Показать ошибку");
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
			
			BYTE nPanelColorIdx = pi->nFarColors[COL_PANELTEXT];
			COLORREF nBackColor = GetWindowLong(hwnd, 4*((nPanelColorIdx & 0xF0)>>4));
			#ifdef _DEBUG
			COLORREF nForeColor = GetWindowLong(hwnd, 4*(nPanelColorIdx & 0xF));
			#endif
			
			PAINTSTRUCT ps = {NULL};
			HDC hdc = BeginPaint(hwnd, &ps);
			if (hdc) {
				RECT rc; GetClientRect(hwnd, &rc);
				HBRUSH hbr = CreateSolidBrush(nBackColor);
				FillRect(hdc, &rc, hbr);
				DeleteObject(hbr);
				Paint(hwnd, ps, rc, pi);
				EndPaint(hwnd, &ps);
			}
			
			return 0;
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

	CloseHandle(ghDisplayThread); ghDisplayThread = NULL;
	gnDisplayThreadId = 0;
	
	return 0;
}

void Paint(HWND hwnd, PAINTSTRUCT& ps, RECT& rc, CeFullPanelInfo* pi)
{
	HDC hdc = ps.hdc;
	COLORREF crGray = GetWindowLong(hwnd, 4*8);
	COLORREF crWhite = GetWindowLong(hwnd, 4*15);

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
		nBackColor[i] = GetWindowLong(hwnd, 4*((nIndexes[i] & 0xF0)>>4));
		nForeColor[i] = GetWindowLong(hwnd, 4*(nIndexes[i] & 0xF));
		hBack[i] = CreateSolidBrush(nBackColor[i]);
	}
	
	
	HPEN hPen = CreatePen(PS_SOLID, 1, crGray);
	HPEN hOldPen = (HPEN)SelectObject(hdc, hPen);
	
	HBRUSH hBr = CreateSolidBrush(crWhite);
	HBRUSH hOldBr = (HBRUSH)SelectObject(hdc, hBr);
	
	HFONT hFont = CreateFont(14,0,0,0,400,0,0,0,ANSI_CHARSET,OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,
		NONANTIALIASED_QUALITY,DEFAULT_PITCH,L"Tahoma");
	HFONT hOldFont = (HFONT)SelectObject(hdc,hFont);

	
	int nWholeW = gThSet.nWidth  + gThSet.nHSpacing + gThSet.nHPadding*2;
	int nWholeH = gThSet.nHeight + gThSet.nVSpacing + gThSet.nVPadding*2;
	int nXCount = (rc.right+gThSet.nHSpacing) / nWholeW; // тут четко, кусок иконки не допускается
	int nYCountFull = (rc.bottom+gThSet.nVSpacing) / nWholeH; // тут четко, кусок иконки не допускается
	int nYCount = (rc.bottom+gThSet.nHeight+gThSet.nVSpacing) / nWholeH; // а тут допускается отображение верхней части иконки
	pi->nXCount = nXCount; pi->nYCountFull = nYCountFull; pi->nYCount = nYCount;
	
	int nTopItem = pi->TopPanelItem;
	int nItemCount = pi->ItemsNumber;
	int nCurrentItem = pi->CurrentItem;
	CePluginPanelItem** ppItems = pi->ppItems;

	if ((nTopItem + nXCount*nYCountFull) < nCurrentItem) {
		TODO("Выравнивание на границу nXCount");
		nTopItem = nCurrentItem - (nXCount*(nYCountFull-1));
	}

	int nItem = nTopItem;
	int nYCoord = 0;
	
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

	for (int Y = 0; Y < nYCount && nItem < nItemCount; Y++) {
		int nXCoord = 0;
		for (int X = 0; X < nXCount && nItem < nItemCount; X++, nItem++, ppItems) {
			Rectangle(hdc,
				nXCoord+gThSet.nHPadding, nYCoord+gThSet.nVPadding,
				nXCoord+gThSet.nHPadding+gThSet.nWidth, nYCoord+gThSet.nVPadding+gThSet.nHeight);

			const wchar_t* pszName = ppItems[nItem]->FindData.lpwszFileName;
			int nLen = lstrlen(pszName);
			
			lstrcpyn(pszNamePtr, pszName, MAX_PATH+1);

			int nDrawRC = -1;
			SHFILEINFO sfi = {NULL};
			UINT cbSize = sizeof(sfi);
			DWORD_PTR shRc = 0;
			HICON hIcon = NULL;

			if (pszName[0] == L'.' && pszName[1] == L'.' && pszName[2] == 0) {
				if (!ghUpIcon)
					ghUpIcon = LoadIcon(ghPluginModule, MAKEINTRESOURCE(IDI_UP));
				if (ghUpIcon) {
					hIcon = ghUpIcon;
				}
			}
			if (!hIcon)
			{
				shRc = SHGetFileInfo ( pszNamePtr, ppItems[nItem]->FindData.dwFileAttributes, &sfi, cbSize, 
					SHGFI_ICON|SHGFI_LARGEICON|SHGFI_USEFILEATTRIBUTES);
				if (shRc && sfi.hIcon)
					hIcon = sfi.hIcon;
			}

			if (hIcon) {
				nDrawRC = DrawIconEx(hdc,
					nXCoord+gThSet.nHPadding+nXIconSpace,
					nYCoord+gThSet.nVPadding+nYIconSpace,
					hIcon, nXIcon, nYIcon, 0, NULL, DI_NORMAL);
			} else {
				// Нарисовать стандартную иконку?
			}
			if (sfi.hIcon) {
				DestroyIcon(sfi.hIcon);
				sfi.hIcon = NULL;
			}
				
			
			RECT rcClip = {
				nXCoord+gThSet.nHPadding, 
					nYCoord+gThSet.nVPadding+gThSet.nHeight,
				nXCoord+gThSet.nHPadding+gThSet.nWidth,
					nYCoord+gThSet.nVPadding+gThSet.nHeight+gThSet.nVSpacing
				};
			COLORREF crBack = 0, crFore = 0;
			
			int nIdx = ((ppItems[nItem]->Flags & (0x40000000/*PPIF_SELECTED*/)) ? 
					((nItem==nCurrentItem) ? 3 : 1) :
					((nItem==nCurrentItem) ? 2 : 0));
			crBack = nBackColor[nIdx];
			crFore = nForeColor[nIdx];
			
			FillRect(hdc, &rcClip, hBack[nIdx]);
			
			SetTextColor(hdc, crFore);
			SetBkColor(hdc, crBack);
			//ExtTextOut(hdc, rcClip.left,rcClip.top, ETO_CLIPPED, &rcClip, pszName, nLen, NULL);
			DrawText(hdc, pszName, nLen, &rcClip, DT_END_ELLIPSIS|DT_NOPREFIX|DT_SINGLELINE);
		
			nXCoord += nWholeW;
		}
		nYCoord += nWholeH;
	}
	
	free(pszFull);
	
	SelectObject(hdc, hOldPen); DeleteObject(hPen);
	SelectObject(hdc, hOldBr);  DeleteObject(hBr);
	SelectObject(hdc, hOldFont);DeleteObject(hFont);
	for (i = 0; i < 4; i++) DeleteObject(hBack[i]);
}
