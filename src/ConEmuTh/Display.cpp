
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
#include "../common/RgnDetect.h"
#include "resource.h"
#include "ImgCache.h"


static ATOM hClass = NULL;
//LRESULT CALLBACK DisplayWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
//DWORD WINAPI DisplayThread(LPVOID lpvParam);
//void Paint(HWND hwnd, PAINTSTRUCT& ps, RECT& rc, CeFullPanelInfo* pi);
const wchar_t gsDisplayClassName[] = L"ConEmuPanelView";
HANDLE ghCreateEvent = NULL;
extern HICON ghUpIcon;
int gnCreateViewError = 0;
DWORD gnWin32Error = 0;
//ITEMIDLIST DesktopID = {{0}};
//IShellFolder *gpDesktopFolder = NULL;
BOOL gbCancelAll = FALSE;
extern COLORREF gcrActiveColors[16], gcrFadeColors[16], *gcrCurColors;
extern bool gbFadeColors;
UINT gnConEmuFadeMsg = 0, gnConEmuSettingsMsg = 0;
extern CRgnDetect *gpRgnDetect;
extern CEFAR_INFO gFarInfo;
extern DWORD gnRgnDetectFlags;

void ResetUngetBuffer();
int ShowLastError();

#define MSG_CREATE_VIEW (WM_USER+101)
#define CREATE_WND_TIMEOUT 5000


HWND CeFullPanelInfo::CreateView()
{
	gnCreateViewError = 0;
	gnWin32Error = 0;

	HWND hView = (this->bLeftPanel) ? ghLeftView : ghRightView;
	if (hView) {
		_ASSERTE(hView==NULL);
		if (IsWindow(hView)) {
			return hView;
		}
		hView = NULL;
	}
	this->hView = NULL;
	this->cbSize = sizeof(*this);
	
	if (!gnDisplayThreadId || !ghDisplayThread) {
		if (ghDisplayThread) {
			CloseHandle(ghDisplayThread); ghDisplayThread = NULL;
		}

		// Сначала нужно запустить нить (ее еще не создавали)
		HANDLE hReady = CreateEvent(NULL,FALSE,FALSE,NULL);
		ghDisplayThread = CreateThread(NULL,0,DisplayThread,(LPVOID)hReady,0,&gnDisplayThreadId);
		if (!ghDisplayThread) {
			gnWin32Error = GetLastError();
			gnCreateViewError = CECreateThreadFailed;
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
			gnWin32Error = GetLastError();
			gnCreateViewError = CEThreadActivationFailed;
			return NULL;
		}

		// И еще чуть подождать, а то возникает "Invalid thread identifier"
		Sleep(50);
	}
	
	if (GetCurrentThreadId() != gnDisplayThreadId) {
		ghCreateEvent = CreateEvent(NULL,FALSE,FALSE,NULL);
		BOOL lbRc = PostThreadMessage(gnDisplayThreadId, MSG_CREATE_VIEW, (WPARAM)ghCreateEvent, (LPARAM)this);
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
				// Нить завершилась
				gnCreateViewError = CEDisplayThreadTerminated;
				GetExitCodeThread(ghDisplayThread, &gnWin32Error);
			} else if (dwWait == (WAIT_OBJECT_0+1)) {
				hView = this->hView;
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
	
	this->cbSize = sizeof(*this);
	
	wchar_t szTitle[128];
	wsprintf(szTitle, L"ConEmu.%sPanelView.%i", (this->bLeftPanel) ? L"Left" : L"Right", gnSelfPID);
	hView = CreateWindow(gsDisplayClassName, szTitle, WS_CHILD|WS_CLIPSIBLINGS, 0,0,0,0, 
		ghConEmuRoot, NULL, (HINSTANCE)ghPluginModule, (LPVOID)this);
	this->hView = hView;
	if (!hView) {
		gnWin32Error = GetLastError();
		gnCreateViewError = CECreateWindowFailed;
	} else {
		if (this->bLeftPanel)
			ghLeftView = hView;
		else
			ghRightView = hView;
	}
	
	return hView;
}

LRESULT CALLBACK CeFullPanelInfo::DisplayWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg) {
	case WM_CREATE:
		{
			CREATESTRUCT* pcr = (CREATESTRUCT*)lParam;
			CeFullPanelInfo* pi = (CeFullPanelInfo*)pcr->lpCreateParams;
			_ASSERTE(pi && pi->cbSize==sizeof(CeFullPanelInfo));
			_ASSERTE(pi == (&pviLeft) || pi == (&pviRight));
			
			SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)pi);
			SetWindowLong(hwnd, 16*4, 1); // Fade == false
			
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
				pi->Paint(hwnd, ps, rc);
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
			COLORREF nBackColor = gcrCurColors[((nPanelColorIdx & 0xF0)>>4)]; //GetWindowLong(hwnd, 4*((nPanelColorIdx & 0xF0)>>4));
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
	case WM_DESTROY:
		{
			if (!ghLeftView && !ghRightView)
				PostThreadMessage(gnDisplayThreadId, WM_QUIT, 0, 0);
			return 0;
		}
	default:
		if (uMsg == gnConEmuFadeMsg) {
			SetWindowLong(hwnd, 16*4, (DWORD)lParam);
			if (gbFadeColors != (lParam == 2)) {
				gbFadeColors = (lParam == 2);
				gcrCurColors = gbFadeColors ? gcrFadeColors : gcrActiveColors;
				//InvalidateRect(hwnd, NULL, FALSE); -- не требуется
			}
		} else if (uMsg == gnConEmuSettingsMsg) {
			WARNING("gnConEmuSettingsMsg");
			MFileMapping<PanelViewSettings> ThSetMap;
			_ASSERTE(ghConEmuRoot!=NULL);

			DWORD nGuiPID;
			#ifdef _DEBUG
			GetWindowThreadProcessId(ghConEmuRoot, &nGuiPID);
			_ASSERTE(nGuiPID == wParam);
			#endif
			nGuiPID = wParam;

			ThSetMap.InitName(CECONVIEWSETNAME, nGuiPID);
			if (!ThSetMap.Open()) {
				MessageBox(NULL, ThSetMap.GetErrorText(), L"ConEmuTh", MB_ICONSTOP|MB_SETFOREGROUND|MB_SYSTEMMODAL);
			} else {
				ThSetMap.Get(&gThSet);
				ThSetMap.CloseMap();
				gpImgCache->Reset();
				// и обновить
				if (pviLeft.hView)
					pviLeft.OnSettingsChanged(TRUE);
				if (pviRight.hView)
					pviRight.OnSettingsChanged(TRUE);
			}
		}
	}

	return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

DWORD WINAPI CeFullPanelInfo::DisplayThread(LPVOID lpvParam)
{
	MSG msg;
	HANDLE hReady = (HANDLE)lpvParam;

	CoInitialize(NULL);

	_ASSERTE(gpImgCache);

	gnConEmuFadeMsg = RegisterWindowMessage(CONEMUMSG_PNLVIEWFADE);
	gnConEmuSettingsMsg = RegisterWindowMessage(CONEMUMSG_PNLVIEWSETTINGS);


	// Выставляем событие, что нить готова
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
				HWND hView = pi->CreateView();
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

	// 26.04.2010 Maks - delete нельзя! не будем выгружать gdiplus.dll до выхода из фара. валится...
	//delete gpImgCache;
	//gpImgCache = NULL;
	gpImgCache->Reset(); // только Reset.

	// Освободить память
	pviLeft.FreeInfo();
	pviRight.FreeInfo();

	UpdateEnvVar(FALSE);
	
	CoUninitialize();
	
	return 0;
}


BOOL CeFullPanelInfo::PaintItem(HDC hdc, int x, int y, CePluginPanelItem* pItem, BOOL abCurrentItem, 
			   COLORREF *nBackColor, COLORREF *nForeColor, HBRUSH *hBack,
			   //int nXIcon, int nYIcon, int nXIconSpace, int nYIconSpace,
			   BOOL abAllowPreview, HBRUSH hBackBrush)
{
	//const wchar_t* pszName = pItem->FindData.lpwszFileNamePart;
	//int nLen = lstrlen(pszName);
	//int nDrawRC = -1;
	//SHFILEINFO sfi = {NULL};
	//UINT cbSize = sizeof(sfi);
	//DWORD_PTR shRc = 0;
	//HICON hIcon = NULL;
	//HBITMAP hBmp = NULL;

	if (gbCancelAll)
		return FALSE;

	RECT rcFull = {x, y, x+nWholeW, y+nWholeH};
	FillRect(hdc, &rcFull, hBack[0]);

	if (gThSet.nPreviewFrame == 1) {
		Rectangle(hdc,
			x+Spaces.nSpaceX1, y+Spaces.nSpaceY1,
			x+Spaces.nSpaceX1+(Spaces.nImgSize+2*gThSet.nPreviewFrame), y+Spaces.nSpaceY1+(Spaces.nImgSize+2*gThSet.nPreviewFrame));
		/*
		Rectangle(hdc,
			x+gThSet.nHPadding, y+gThSet.nVPadding,
			x+gThSet.nHPadding+gThSet.nWidth, y+gThSet.nVPadding+gThSet.nHeight);
		*/
	} else if (gThSet.nPreviewFrame == 0) {
		RECT rcTmp = {x+Spaces.nSpaceX1, y+Spaces.nSpaceY1,
			x+Spaces.nSpaceX1+(Spaces.nImgSize+2*gThSet.nPreviewFrame), y+Spaces.nSpaceY1+(Spaces.nImgSize+2*gThSet.nPreviewFrame)};
		/*
		RECT rcTmp = {x+gThSet.nHPadding, y+gThSet.nVPadding,
			x+gThSet.nHPadding+gThSet.nWidth, y+gThSet.nVPadding+gThSet.nHeight};
		*/
		FillRect(hdc, &rcTmp, hBackBrush);
	} else {
		_ASSERTE(gThSet.nPreviewFrame==0 || gThSet.nPreviewFrame==1);
		return FALSE;
	}

	const wchar_t* pszComments = NULL;
	gpImgCache->PaintItem(hdc, x+gThSet.nPreviewFrame+Spaces.nSpaceX1, y+gThSet.nPreviewFrame+Spaces.nSpaceY1,
		Spaces.nImgSize, pItem, abAllowPreview, &pszComments);

	RECT rcClip = {0}, rcMaxText;
	COLORREF crBack = 0, crFore = 0;
	
	if (PVM == pvm_Thumbnails) {
		rcClip.left   = x+Spaces.nSpaceX1;
		rcClip.top    = y+Spaces.nSpaceY1+(Spaces.nImgSize+2*gThSet.nPreviewFrame)+Spaces.nLabelSpacing;
		rcClip.right  = x+Spaces.nSpaceX1+(Spaces.nImgSize+2*gThSet.nPreviewFrame);
		rcClip.bottom = y+Spaces.nSpaceY1+(Spaces.nImgSize+2*gThSet.nPreviewFrame)+nFontHeight+2;
		//
		rcMaxText = rcClip;
	} else if (PVM == pvm_Tiles) {
		rcClip.left   = x+Spaces.nSpaceX1+(Spaces.nImgSize+2*gThSet.nPreviewFrame)+Spaces.nLabelSpacing;
		rcClip.top    = y+Spaces.nSpaceY1;
		rcClip.right  = x+Spaces.nSpaceX1+(Spaces.nImgSize+2*gThSet.nPreviewFrame)+Spaces.nSpaceX2;
		rcClip.bottom = y+Spaces.nSpaceY1+(Spaces.nImgSize+2*gThSet.nPreviewFrame);
		//
		rcMaxText = rcClip;
		rcMaxText.top = y;
		rcMaxText.bottom = y+2*Spaces.nSpaceY1+(Spaces.nImgSize+2*gThSet.nPreviewFrame);
	}

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
	if (Spaces.nLabelPadding && Spaces.nLabelPadding < ((rcClip.right-rcClip.left)/3)) {
		rcClip.left += Spaces.nLabelPadding; rcClip.right -= Spaces.nLabelPadding;
	}
	//DrawText(hdc, pszName, nLen, &rcClip, DT_END_ELLIPSIS|DT_NOPREFIX|DT_SINGLELINE|DT_VCENTER);
	DrawItemText(hdc, &rcClip, &rcMaxText, pItem, pszComments, hBack[nIdx]);

	return TRUE;
}

int CeFullPanelInfo::DrawItemText(HDC hdc, LPRECT prcText, LPRECT prcMaxText, CePluginPanelItem* pItem, LPCWSTR pszComments, HBRUSH hBr)
{
	int iRc = 0;
	const wchar_t* pszName = pItem->FindData.lpwszFileNamePart;
	int nLen = lstrlen(pszName);
	RECT rcClip = *prcText;
	
	if (PVM == pvm_Thumbnails) {
		const wchar_t* pszExt = NULL;
		
		// только для файлов, для папок "расширение" не ровняем, ибо это не расширение.
		if ((pItem->FindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0) {
			pszExt = wcsrchr(pszName, L'.');
			if (pszExt == pszName) pszExt = NULL; // если точка одна, и имя начинается с нее
		}
		
		if (pszExt) {
			RECT rcExt = rcClip;
			RECT rcName = rcClip;
			int nExtLen = lstrlen(pszExt);
			int nNameLen = pszExt - pszName;
			
			iRc = DrawText(hdc, pszExt, nExtLen, &rcExt, DT_CALCRECT|DT_NOPREFIX|DT_SINGLELINE|DT_TOP|DT_LEFT);
			iRc = DrawText(hdc, pszName, nNameLen, &rcName, DT_CALCRECT|DT_NOPREFIX|DT_SINGLELINE|DT_TOP|DT_LEFT);
			
			// Рисуем расширение отдельно только если длина расширения в пикселах меньше половины от выделенного prcText
			if (iRc && (rcExt.right-rcExt.left) <= ((rcClip.right-rcClip.left)-(rcName.right-rcName.left))) {
				rcExt.left = rcClip.right-(rcExt.right-rcExt.left);
				rcExt.right = rcClip.right; rcExt.top = rcClip.top; rcExt.bottom = rcClip.bottom;
				iRc = DrawText(hdc, pszExt, nExtLen, &rcExt, DT_NOPREFIX|DT_SINGLELINE|DT_VCENTER);
				nLen = nNameLen;
				rcClip.right = rcExt.left;
			}
		}
		iRc = DrawText(hdc, pszName, nLen, &rcClip, DT_END_ELLIPSIS|DT_NOPREFIX|DT_SINGLELINE|DT_VCENTER);
	} else {
		TODO("Обработка pItem->pszInfo");
		wchar_t szFullInfo[MAX_PATH*4]; wchar_t* psz = szFullInfo;
		// формируем
		lstrcpyn(psz, pszName, MAX_PATH); psz += lstrlen(psz);
		
		if (!(pszName[0] == L'.' && (pszName[1] == 0 || (pszName[1] == L'.' && pszName[2] == 0)))) {
			*(psz++) = L'\n'; *psz = 0;

			if ((pItem->FindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0) {
				if ((pItem->NumberOfLinks > 1) || (pItem->FindData.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT)) {
					*(psz++) = L'<'; *psz = 0;
					lstrcpy(psz, gsSymLink);
					//gsJunction, gsSymLink, gsFolder, gsHardLink
					//else if (pItem->FindData.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT)
					//	lstrcpy(psz, gsSymLink);
					//else
					//	lstrcpy(psz, gsFolder);
					psz += lstrlen(psz);
					*(psz++) = L'>'; *(psz++) = L' '; *psz = 0;
				}
			}

			SYSTEMTIME st; FileTimeToSystemTime(&pItem->FindData.ftLastWriteTime, &st);
			wsprintf(psz, L"%02i.%02i.%02i %i:%02i", st.wDay, st.wMonth, st.wYear % 100, st.wHour, st.wMinute);
			psz += lstrlen(psz);

			if (pItem->FindData.nFileSize
				|| ((pItem->FindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0))
			{
				*(psz++) = L' '; *(psz++) = L' '; *psz = 0;
				__int64 nSize = pItem->FindData.nFileSize;
				wchar_t cType = L'B';
				int n = 0;
				const wchar_t cTypeList[] = L"BKMGTP";
				int nMax = sizeofarray(cTypeList);
				while (nSize > 9999) {
					nSize = nSize >> 10; n++;
				}
				if (n < nMax) cType = cTypeList[n]; else cType = L'?';
				if (nSize > 999) {
					int nAll = (int)nSize;
					int nL = nAll % 1000;
					int nH = nAll / 1000;
					wsprintfW(psz, L"%i,%i %c", nH, nL, cType);
				} else {
					wsprintfW(psz, L"%i %c", (int)nSize, cType);
				}

				psz += lstrlen(psz);
			}
			*psz = 0; // \0 ended
		}
		
		if (pszComments && *pszComments) {
			*(psz++) = L'\n'; *psz = 0;
			lstrcpyn(psz, pszComments, MAX_PATH); psz += lstrlen(psz);
		}
		
		if (pItem->pszDescription && *pItem->pszDescription) {
			*(psz++) = L'\n'; *psz = 0;
			lstrcpyn(psz, pItem->pszDescription, MAX_PATH); psz += lstrlen(psz);
		}

		*psz = 0; // \0 ended
		nLen = lstrlen(szFullInfo);
		DWORD nFlags = DT_WORD_ELLIPSIS|DT_NOPREFIX|DT_TOP|DT_LEFT;
		RECT rcText = rcClip; rcText.bottom = nFontHeight;
		// если все строки не помещаются - нужно прижать к верхнему краю
		iRc = DrawText(hdc, szFullInfo, nLen, &rcText, DT_CALCRECT|nFlags);
		int nCellHeight = (rcClip.bottom-rcClip.top);
		int nTextHeight = (rcText.bottom-rcText.top);
		if (nTextHeight < nCellHeight) {
			rcClip.top = rcClip.top + ((nCellHeight - nTextHeight) >> 1);
		} else if (nTextHeight > nCellHeight) {
			RECT rcFull = *prcMaxText;
			int nFullHeight = rcFull.bottom-rcFull.top;
			if (nFullHeight > nCellHeight) {
				if (nTextHeight < nFullHeight) {
					rcClip.top = rcFull.top = rcFull.top + ((nFullHeight - nTextHeight) >> 1);
					rcClip.bottom = rcFull.bottom = rcFull.top + nTextHeight;
				} else {
					rcClip.top = rcFull.top;
					rcClip.bottom = rcFull.bottom;
				}
				FillRect(hdc, &rcFull, hBr);
			}
		}
		// теперь - рисуем
		iRc = DrawText(hdc, szFullInfo, nLen, &rcClip, nFlags);
	}
	
	return iRc;
}

void CeFullPanelInfo::Paint(HWND hwnd, PAINTSTRUCT& ps, RECT& rc)
{
	gbCancelAll = FALSE;

	HDC hdc = ps.hdc;

	//for (int i=0; i<16; i++)
	//	gcrColors[i] = GetWindowLong(hwnd, 4*i);

	BYTE nIndexes[4] = {
		this->nFarColors[COL_PANELTEXT],
		this->nFarColors[COL_PANELSELECTEDTEXT],
		this->nFarColors[COL_PANELCURSOR],
		this->nFarColors[COL_PANELSELECTEDCURSOR]
	};
	COLORREF nBackColor[4], nForeColor[4];
	HBRUSH hBack[4];
	int i;
	for (i = 0; i < 4; i++) {
		nBackColor[i] = gcrCurColors[((nIndexes[i] & 0xF0)>>4)];
		nForeColor[i] = gcrCurColors[(nIndexes[i] & 0xF)];
		hBack[i] = CreateSolidBrush(nBackColor[i]);
	}
	COLORREF crGray = gcrCurColors[8];
	COLORREF crBack = nBackColor[0]; //gcrColors[15];

	if (gThSet.crBackground.UseIndex) {
		if (gThSet.crBackground.ColorIdx <= 15)
			crBack = gcrCurColors[gThSet.crBackground.ColorIdx];
	} else {
		crBack = gThSet.crBackground.ColorRGB;
	}

	if (gThSet.crPreviewFrame.UseIndex) {
		if (gThSet.crPreviewFrame.ColorIdx <= 15)
			crGray = gcrCurColors[gThSet.crPreviewFrame.ColorIdx];
	} else {
		crGray = gThSet.crPreviewFrame.ColorRGB;
	}
		
	
	HPEN hPen = CreatePen(PS_SOLID, 1, crGray);
	HBRUSH hBackBrush = CreateSolidBrush(crBack);
	HFONT hFont = CreateFont(nFontHeight,0,0,0,400,0,0,0,ANSI_CHARSET,OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,
		NONANTIALIASED_QUALITY,DEFAULT_PITCH,sFontName);

	// Передернуть класс на предмет смены/инициализации настроек
	gpImgCache->Init(crBack);

	
	nWholeW = (Spaces.nImgSize+2*gThSet.nPreviewFrame)  + Spaces.nSpaceX2 + Spaces.nSpaceX1*2;
	nWholeH = (Spaces.nImgSize+2*gThSet.nPreviewFrame) + Spaces.nSpaceY2 + Spaces.nSpaceY1*2;


	nXCount = (rc.right+Spaces.nSpaceX2) / nWholeW; // тут четко, кусок иконки не допускается
	if (nXCount < 1) nXCount = 1;
	nYCountFull = (rc.bottom+Spaces.nSpaceY2) / nWholeH; // тут четко, кусок иконки не допускается
	if (nYCountFull < 1) nYCountFull = 1;
	if (PVM == pvm_Thumbnails)
		nYCount = (rc.bottom+(Spaces.nImgSize+2*gThSet.nPreviewFrame)+Spaces.nSpaceY2) / nWholeH; // а тут допускается отображение верхней части иконки
	else
		nYCount = nYCountFull; // Во всех остальных режимах - часть по низу не допускается.
	if (nYCount < 1) nYCount = 1;
	this->nXCount = nXCount; this->nYCountFull = nYCountFull; this->nYCount = nYCount;
	
	int nTopItem = this->TopPanelItem;
	int nItemCount = this->ItemsNumber;
	int nCurrentItem = this->CurrentItem;
	//CePluginPanelItem** ppItems = this->ppItems;

	if ((nTopItem + nXCount*nYCountFull) <= nCurrentItem) {
		TODO("Выравнивание на границу nXCount");
		nTopItem = nCurrentItem - (nXCount*(nYCountFull-1));
	}
	if (PVM == pvm_Thumbnails)
	{
		int nMod = nTopItem % nXCount;
		if (nMod) {
			nTopItem = max(0,nTopItem-nMod);
			//if (nTopItem > nCurrentItem)
			//	nTopItem = max(nCurrentItem,(nTopItem-nXCount));
		}
	}
	this->OurTopPanelItem = nTopItem;

	
	int nMaxLen = (this->pszPanelDir ? lstrlen(this->pszPanelDir) : 0) + MAX_PATH+3;
	wchar_t* pszFull = (wchar_t*)malloc(nMaxLen*2);
	wchar_t* pszNamePtr = NULL;
	if (this->pszPanelDir && *this->pszPanelDir) {
		lstrcpy(pszFull, this->pszPanelDir);
		pszNamePtr = pszFull+lstrlen(pszFull);
		if (*(pszNamePtr-1) != L'\\') {
			*(pszNamePtr++) = L'\\';
			*pszNamePtr = 0;
		}
	} else {
		pszNamePtr = pszFull;
	}
	
	//int nXIcon = GetSystemMetrics(SM_CXICON);
	//int nYIcon = GetSystemMetrics(SM_CYICON);
	//int nXIconSpace = ((Spaces.nImgSize+2*gThSet.nPreviewFrame) - nXIcon) >> 1;
	//int nYIconSpace = ((Spaces.nImgSize+2*gThSet.nPreviewFrame) - nYIcon) >> 1;

	HDC hCompDC = CreateCompatibleDC(hdc);
	HBITMAP hCompBmp = CreateCompatibleBitmap(hdc, nWholeW, nWholeH);
	HBITMAP hOldCompBmp = (HBITMAP)SelectObject(hCompDC, hCompBmp);
	{
		RECT rcComp = {0,0,nWholeW,nWholeH};
		FillRect(hCompDC, &rcComp, hBack[0]);
	}
	HPEN hOldPen = (HPEN)SelectObject(hCompDC, hPen);
	HBRUSH hOldBr = (HBRUSH)SelectObject(hCompDC, hBackBrush);
	HFONT hOldFont = (HFONT)SelectObject(hCompDC,hFont);


	if (!this->Focus)
		nCurrentItem = -1;


	for (int nStep = 0; !gbCancelAll && nStep <= 1; nStep++)
	{
		if (nStep) {
			if ((gThSet.bLoadPreviews & PVM) == 0)
				continue; // для этого режима превьюшки не просили
		}
		
		int nItem = nTopItem;
		int nYCoord = 0, nXCoord = 0;
		int X = -1, Y = -1;
		int iCount = (PVM == pvm_Thumbnails) ? nYCount : nXCount;
		int jCount = (PVM == pvm_Thumbnails) ? nXCount : nYCount;


		//for (int Y = 0; !gbCancelAll && Y < nYCount && nItem < nItemCount; Y++) {
		for (int i = 0; !gbCancelAll && i < iCount && nItem < nItemCount; i++) {
			if (PVM == pvm_Thumbnails) {
				nXCoord = 0; Y++; X = -1;
			} else {
				nYCoord = 0; X++; Y = -1;
			}
				
			//for (int X = 0; !gbCancelAll && X < nXCount && nItem < nItemCount; X++, nItem++) {
			for (int j = 0; !gbCancelAll && j < jCount && nItem < nItemCount; j++, nItem++) {
				if (PVM == pvm_Thumbnails) {
					X++;
				} else {
					Y++;
				}

				CePluginPanelItem* pItem = this->ppItems[nItem];
				if (!pItem) {
					_ASSERTE(this->ppItems[nItem]!=NULL);
					continue; // Ошибка?
				}
				if (!nStep || !pItem->bPreviewLoaded) {
					// поехали
					const wchar_t* pszName = pItem->FindData.lpwszFileName;
					if (wcschr(pszName, L'\\')) {
						// Это уже может быть полный путь (TempPanel?)
						pItem->pszFullName = pszName;
					} else {
						// Полный путь нужно сформировать
						lstrcpyn(pszNamePtr, pszName, MAX_PATH+1);
						pItem->pszFullName = pszFull;
					}

					if (PaintItem(hCompDC, 0, 0, pItem, (nItem==nCurrentItem), 
						nBackColor, nForeColor, hBack,
						//nXIcon, nYIcon, nXIconSpace, nYIconSpace, 
						(nStep == 1), hBackBrush))
					{
						BitBlt(hdc, nXCoord, nYCoord, nWholeW, nWholeH, hCompDC, 0,0, SRCCOPY);
						#ifdef _DEBUG
						GdiFlush();
						#endif
					}
				}

				if (PVM == pvm_Thumbnails) {
					nXCoord += nWholeW;
				} else {
					nYCoord += nWholeH;
				}
			}


			if (PVM == pvm_Thumbnails) {
				if (!nStep && nXCoord < rc.right) {
					RECT rcComp = {nXCoord,nYCoord,rc.right,nYCoord+nWholeH};
					FillRect(hdc, &rcComp, hBack[0]);
				}

				nYCoord += nWholeH;
			} else {
				if (!nStep && nYCoord < rc.bottom) {
					RECT rcComp = {nXCoord,nYCoord,rc.right,rc.bottom};
					FillRect(hdc, &rcComp, hBack[0]);
				}

				nXCoord += nWholeW;
			}
		}


		if (PVM == pvm_Thumbnails) {
			if (!nStep && nYCoord < rc.bottom) {
				RECT rcComp = {0,nYCoord,rc.right,rc.bottom};
				FillRect(hdc, &rcComp, hBack[0]);
			}
		} else {
			if (!nStep && nXCoord < rc.right) {
				RECT rcComp = {nXCoord,0,rc.right,rc.bottom};
				FillRect(hdc, &rcComp, hBack[0]);
			}
		}
	}

	free(pszFull);

	//SafeRelease(gpDesktopFolder);

	SelectObject(hCompDC, hOldPen);     DeleteObject(hPen);
	SelectObject(hCompDC, hOldBr);      DeleteObject(hBackBrush);
	SelectObject(hCompDC, hOldFont);    DeleteObject(hFont);
	SelectObject(hCompDC, hOldCompBmp); DeleteObject(hCompBmp);
	DeleteDC(hCompDC);

	
	for (i = 0; i < 4; i++) DeleteObject(hBack[i]);

//#ifdef _DEBUG
//	BitBlt(hdc, 0,0,rc.right,rc.bottom, gpImgCache->hField[0], 0,0, SRCCOPY);
//#endif
}


// Эта "дисплейная" функция вызывается из основной нити, там можно дергать FAR Api
void CeFullPanelInfo::DisplayReloadPanel()
{
	TODO("Определить повторно какие элементы видимы, и перечитать только их");

	for (int nItem = 0; nItem < this->ItemsNumber; nItem++)
	{
		// Обновить информацию об элементе (имя, веделенность, и т.п.)
		LoadPanelItemInfo(this, nItem);
	}
}




// Эта "дисплейная" функция вызывается из основной нити, там можно дергать FAR Api
int CeFullPanelInfo::RegisterPanelView()
{
	// Страховка от того, что conemu.dll могли выгрузить (unload:...)
	if (!CheckConEmu() || !gfRegisterPanelView)
		return -1;

	//CeFullPanelInfo* pi = abLeft ? &pviLeft : &pviRight;

	PanelViewInit pvi = {sizeof(PanelViewInit)};
	pvi.bLeftPanel = this->bLeftPanel;
	pvi.bPanelFullCover = FALSE; // только рабочая область
	pvi.nFarInterfaceSettings = this->nFarInterfaceSettings;
	pvi.nFarPanelSettings = this->nFarPanelSettings;
	pvi.PanelRect = this->PanelRect;
	pvi.pfnPeekPreCall = OnPrePeekConsole;
	pvi.pfnPeekPostCall = OnPostPeekConsole;
	pvi.pfnReadPreCall = OnPreReadConsole;
	pvi.pfnReadPostCall = OnPostReadConsole;
	pvi.pfnWriteCall = OnPreWriteConsoleOutput;

	// Зарегистрироваться (или обновить положение)
	pvi.bRegister = TRUE;
	pvi.hWnd = this->hView;

	BOOL lbRc = FALSE;
	DWORD dwErr = 0;
	int nRc = gfRegisterPanelView(&pvi);
	
	gnCreateViewError = 0;

	// Если пока все ОК
	if (nRc == 0) {
		// Настройки отображения
		memset(&gThSet, 0, sizeof(gThSet));
		if (!pvi.ThSet.cbSize || !pvi.ThSet.Thumbs.nImgSize || !pvi.ThSet.Tiles.nImgSize) {
			gnCreateViewError = CEInvalidSettingValues;
			nRc = -1000;
		} else {
			_ASSERTE(pvi.ThSet.cbSize == sizeof(gThSet));
			memmove(&gThSet, &pvi.ThSet, min(pvi.ThSet.cbSize,sizeof(gThSet)));
			// Цвета "консоли"
			for (int i=0; i<16; i++) {
				gcrActiveColors[i] = pvi.crPalette[i];
				gcrFadeColors[i] = pvi.crFadePalette[i];
			}
			// Мы активны? По идее должны, при активации плагина-то
			gbFadeColors = (pvi.bFadeColors!=FALSE);
			gcrCurColors = gbFadeColors ? gcrFadeColors : gcrActiveColors;
			SetWindowLong(this->hView, 16*4, gbFadeColors ? 2 : 1);
			// Подготовить детектор диалогов
			_ASSERTE(gpRgnDetect!=NULL);
			if (gpRgnDetect->InitializeSBI(gcrCurColors)) {
				gpRgnDetect->PrepareTransparent(&gFarInfo, gcrCurColors);
			}
			
			// Сразу скопировать параметры соответствующего режима к себе
			if (!OnSettingsChanged(FALSE))
				nRc = -1000;
			//if (PVM == pvm_Thumbnails) {
			//	Spaces = gThSet.Thumbs;
			//	lstrcpy(sFontName, gThSet.Thumbs.sFontName);
			//	nFontHeight = gThSet.Thumbs.nFontHeight;
			//} else if (PVM == pvm_Tiles) {
			//	Spaces = gThSet.Tiles;
			//	lstrcpy(sFontName, gThSet.Tiles.sFontName);
			//	nFontHeight = gThSet.Tiles.nFontHeight;
			//} else {
			//	_ASSERTE(PVM==pvm_Thumbnails || PVM==pvm_Tiles);
			//	gnCreateViewError = CEUnknownPanelMode;
			//	nRc = -1000;
			//}
		}
	}

	// Если можно продолжать - продолжаем
	if (nRc == 0) {
		gnRgnDetectFlags = gpRgnDetect->GetFlags();
		//gbLastCheckWindow = true;

		if (!IsWindowVisible(this->hView)) {
			lbRc = ShowWindow(this->hView, SW_SHOWNORMAL);
			if (!lbRc)
				dwErr = GetLastError();
		}
		_ASSERTE(lbRc || IsWindowVisible(this->hView));
		InvalidateRect(this->hView, NULL, FALSE);
		RedrawWindow(this->hView, NULL, NULL, RDW_INTERNALPAINT|RDW_UPDATENOW);

		UpdateEnvVar(FALSE);
	}

	
	// Если GUI отказался от панели (или уже здесь произошла ошибка) - нужно ее закрыть
	if (nRc != 0) {
		// Закрыть окно (по WM_CLOSE окно само должно послать WM_QUIT если оно единственное)
		_ASSERTE(nRc == 0);

		// Если эта панель единственная - сразу сбрасываем переменные
		if (!pviLeft.hView || !pviRight.hView) {
			ResetUngetBuffer();
			SetEnvironmentVariable(TH_ENVVAR_NAME, NULL);
		}

		PostMessage(this->hView, WM_CLOSE, 0, 0);
		if (gnCreateViewError == 0) // если ошибку еще не установили - жалуемся на GUI
			gnCreateViewError = CEGuiDontAcceptPanel;
		gnWin32Error = nRc;

		if (GetCurrentThreadId() == gnMainThreadId) {
			ShowLastError();
		}
	}	

	return nRc;
}


// Эта "дисплейная" функция вызывается из основной нити, там можно дергать FAR Api
int CeFullPanelInfo::UnregisterPanelView()
{
	// Страховка от того, что conemu.dll могли выгрузить (unload:...)
	if (!CheckConEmu() || !gfRegisterPanelView)
		return -1;

	// Если эта панель единственная - сразу сбрасываем переменные
	if (!pviLeft.hView || !pviRight.hView) {
		ResetUngetBuffer();
		SetEnvironmentVariable(TH_ENVVAR_NAME, NULL);
	}

	//CeFullPanelInfo* pi = abLeft ? &pviLeft : &pviRight;

	PanelViewInit pvi = {sizeof(PanelViewInit)};
	pvi.bLeftPanel = this->bLeftPanel;
	pvi.nFarInterfaceSettings = this->nFarInterfaceSettings;
	pvi.nFarPanelSettings = this->nFarPanelSettings;
	pvi.PanelRect = this->PanelRect;

	// Отрегистрироваться
	pvi.bRegister = FALSE;
	pvi.hWnd = this->hView;

	int nRc = gfRegisterPanelView(&pvi);

	// Закрыть окно (по WM_CLOSE окно само должно послать WM_QUIT если оно единственное)
	PostMessage(this->hView, WM_CLOSE, 0, 0);

	return nRc;
}

BOOL CeFullPanelInfo::OnSettingsChanged(BOOL bInvalidate)
{
	BOOL lbRc = TRUE;
	// Сразу скопировать параметры соответствующего режима к себе
	if (PVM == pvm_Thumbnails) {
		Spaces = gThSet.Thumbs;
		lstrcpy(sFontName, gThSet.Thumbs.sFontName);
		nFontHeight = gThSet.Thumbs.nFontHeight;
	} else if (PVM == pvm_Tiles) {
		Spaces = gThSet.Tiles;
		lstrcpy(sFontName, gThSet.Tiles.sFontName);
		nFontHeight = gThSet.Tiles.nFontHeight;
	} else {
		_ASSERTE(PVM==pvm_Thumbnails || PVM==pvm_Tiles);
		gnCreateViewError = CEUnknownPanelMode;
		//nRc = -1000;
		lbRc = FALSE;
	}
	if (bInvalidate && IsWindowVisible(hView))
		InvalidateRect(hView, NULL, FALSE);
	return lbRc;
}
