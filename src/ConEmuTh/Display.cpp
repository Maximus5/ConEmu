
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
#include <ShlObj.h>
#include "ConEmuTh.h"
#include "../common/farcolor.hpp"
#include "resource.h"

template <typename T>
inline void SafeRelease(T *&p)
{
	if (NULL != p)
	{
		p->Release();
		p = NULL;
	}
}

static ATOM hClass = NULL;
LRESULT CALLBACK DisplayWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
DWORD WINAPI DisplayThread(LPVOID lpvParam);
void Paint(HWND hwnd, PAINTSTRUCT& ps, RECT& rc, CeFullPanelInfo* pi);
const wchar_t gsDisplayClassName[] = L"ConEmuPanelView";
HANDLE ghCreateEvent = NULL;
HICON ghUpIcon = NULL;
int gnCreateViewError = 0;
DWORD gnWin32Error = 0;
ITEMIDLIST DesktopID = {{0}};
IShellFolder *gpDesktopFolder = NULL;
BOOL gbCancelAll = FALSE;
CThumbnails *gpThumbnails = NULL;

#define MSG_CREATE_VIEW (WM_USER+101)
#define CREATE_WND_TIMEOUT 5000

CThumbnails::CThumbnails()
{
	nWidth = nHeight = 0;
	nXIcon = nYIcon = nXIconSpace = nYIconSpace = 0;
	hWhiteBrush = NULL;
	nFieldX = nFieldY = 0;
	memset(hField,0,sizeof(hField));
	memset(hFieldBmp,0,sizeof(hFieldBmp));
	memset(hOldBmp,0,sizeof(hOldBmp));
	memset(CacheInfo,0,sizeof(CacheInfo));
};
CThumbnails::~CThumbnails()
{
	Reset();
};
void CThumbnails::Reset()
{
	int i;
	for (i=0; i<FIELD_MAX_COUNT; i++) {
		if (hField[i]) {
			if (hOldBmp[i]) { SelectObject(hField[i], hOldBmp[i]); hOldBmp[i] = NULL; }
			if (hFieldBmp[i]) { DeleteObject(hFieldBmp[i]); hFieldBmp[i] = NULL; }
			DeleteDC(hField[i]); hField[i] = NULL;
		}
	}
	for (i=0; i<sizeofarray(CacheInfo); i++) {
		if (CacheInfo[i].lpwszFileName) {
			free(CacheInfo[i].lpwszFileName);
			CacheInfo[i].lpwszFileName = NULL;
		}
	}
};
void CThumbnails::Init(HBRUSH ahWhiteBrush)
{
	// Инициализация (или сброс если изменились размеры превьюшек)
	hWhiteBrush = ahWhiteBrush; 
	_ASSERTE(gThSet.nWidth>=16 && gThSet.nWidth<=256 && gThSet.nHeight>=16 && gThSet.nHeight<=256);
	int nW = (gThSet.nWidth-gThSet.nThumbFrame*2);
	int nH = (gThSet.nHeight-gThSet.nThumbFrame*2);
	if (nWidth != nW || nHeight != nH) {
		Reset();
		nWidth = nW; nHeight = nH;
		// Размеры полей
		nFieldX = min(ITEMS_IN_FIELD,((int)(2048/nWidth)));
		nFieldY = min(ITEMS_IN_FIELD,((int)(2048/nHeight)));
	}
	// Как центрируется иконка
	nXIcon = GetSystemMetrics(SM_CXICON);
	nYIcon = GetSystemMetrics(SM_CYICON);
	nXIconSpace = (nWidth - nXIcon) >> 1;
	nYIconSpace = (nHeight - nYIcon) >> 1;
};
BOOL CThumbnails::FindInCache(CePluginPanelItem* pItem, int* pnIndex)
{
	_ASSERTE(pItem && pnIndex);
	BOOL lbReady = FALSE, lbFound = FALSE;
	DWORD nCurTick = GetTickCount();
	DWORD nMaxDelta = 0, nDelta;
	int nFree = -1, nOldest = -1, i;
	const wchar_t *pszName = pItem->pszFullName;
	if ((pItem->FindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
		&& pItem->FindData.lpwszFileNamePart[0] == L'.'
		&& pItem->FindData.lpwszFileNamePart[1] == L'.'
		&& pItem->FindData.lpwszFileNamePart[2] == 0)
		pszName = pItem->FindData.lpwszFileNamePart;

	for (i=0; i<sizeofarray(CacheInfo); i++) {
		if (!CacheInfo[i].lpwszFileName) {
			if (nFree == -1) nFree = i;
			continue;
		}

		if (lstrcmpi(CacheInfo[i].lpwszFileName, pszName) == 0) {
			// Наш.
			*pnIndex = i; lbFound = TRUE;
			if (CacheInfo[i].nFileSize == pItem->FindData.nFileSize
				&& CacheInfo[i].ftLastWriteTime.dwHighDateTime == pItem->FindData.ftLastWriteTime.dwHighDateTime
				&& CacheInfo[i].ftLastWriteTime.dwLowDateTime == pItem->FindData.ftLastWriteTime.dwLowDateTime)
			{
				lbReady = TRUE;
				CacheInfo[i].dwFileAttributes = pItem->FindData.dwFileAttributes;
			} // иначе потребуется обновление превьюшки (файл изменился)
			break;
		}

		nDelta = nCurTick - CacheInfo[i].nAccessTick;
		if (nDelta > nMaxDelta) {
			nOldest = i; nMaxDelta = nDelta;
		}
	}
	if (!lbFound) {
		if (nFree != -1) {
			*pnIndex = nFree;
		} else {
			_ASSERTE(nOldest!=-1);
			if (nOldest == -1) nOldest = 0;
			if (CacheInfo[nOldest].lpwszFileName) { free(CacheInfo[nOldest].lpwszFileName); CacheInfo[nOldest].lpwszFileName = NULL; }
			*pnIndex = nOldest;
		}
	}
	i = *pnIndex;
	if (CacheInfo[i].lpwszFileName == NULL) {
		CacheInfo[i].lpwszFileName = _wcsdup(pszName);
		lbReady = FALSE;
	}
	if (!lbReady) {
		CacheInfo[i].bPreviewLoaded = FALSE;
		CacheInfo[i].nAccessTick = GetTickCount();
		CacheInfo[i].dwFileAttributes = pItem->FindData.dwFileAttributes;
		CacheInfo[i].nFileSize = pItem->FindData.nFileSize;
		CacheInfo[i].ftLastWriteTime = pItem->FindData.ftLastWriteTime;
	}
	return lbReady;
};
BOOL CThumbnails::PaintItem(HDC hdc, int x, int y, CePluginPanelItem* pItem, BOOL abLoadPreview)
{
	int nIndex = -1;
	BOOL lbWasDraw = FALSE;
	BOOL lbWasPreview = FALSE;
	BOOL lbReady = FindInCache(pItem, &nIndex);
	if ((nFieldX*nFieldY) == 0) {
		_ASSERTE((nFieldX*nFieldY)!=0);
		return FALSE;
	}
	int iField = nIndex / (nFieldX*nFieldY);
	int ii = nIndex - (iField*nFieldX*nFieldY);
	int iRow = ii / nFieldX;
	int iCol = ii - iRow*nFieldX;

	if (iField<0 || iField>=FIELD_MAX_COUNT) {
		_ASSERTE(iField>=0 && iField<FIELD_MAX_COUNT);
		return FALSE;
	}
	if (hField[iField] == NULL) {
		DWORD dwErr = 0;
		HDC hScreen = GetDC(NULL);
		hField[iField] = CreateCompatibleDC(hScreen);
		hFieldBmp[iField] = CreateCompatibleBitmap(hScreen, nWidth*nFieldX, nHeight*nFieldY);
		dwErr = GetLastError();
		ReleaseDC(NULL, hScreen);
		if (hFieldBmp[iField] == NULL) {
			wchar_t szErr[255];
			wsprintf(szErr, L"Can't create compatible bitmap (%ix%i)\nErrCode=0x%08X", nWidth*nFieldX, nHeight*nFieldY, dwErr);
			MessageBox(NULL, szErr, L"ConEmu Thumbnails", MB_OK|MB_SYSTEMMODAL|MB_ICONERROR);
		}
		hOldBmp[iField] = (HBITMAP)SelectObject(hField[iField], hFieldBmp[iField]);
	}

	RECT rc = {iCol*nWidth, iRow*nHeight, (iCol+1)*nWidth, (iRow+1)*nHeight};

	// Если в прошлый раз загрузили только ShellIcon и сейчас просят Preview
	if (lbReady && abLoadPreview && CacheInfo[nIndex].bPreviewLoaded == FALSE)
		lbReady = FALSE;

	// Если нужно загрузить иконку или превьюшку (или обновить их)
	if (!lbReady) {
		UpdateCell(hField[iField], CacheInfo+nIndex, rc.left, rc.top, abLoadPreview);
	}

	// А теперь - собственно отрисовка куда просили
	BitBlt(hdc, x,y,x+nWidth-2,y+nHeight-2, hField[iField], rc.left,rc.top, SRCCOPY);

	return lbWasDraw;
}
void CThumbnails::UpdateCell(HDC hdc, struct tag_CacheInfo* pInfo, int x, int y, BOOL abLoadPreview)
{
	HICON hIcon = NULL;
	HBITMAP hBmp = NULL;
	BOOL lbPreviewWasLoaded = FALSE;
	const wchar_t* pszName = pInfo->lpwszFileName;
	int nDrawRC = -1;
	SHFILEINFO sfi = {NULL};
	UINT cbSize = sizeof(sfi);
	DWORD_PTR shRc = 0;
	RECT rc = {x,y,x+nWidth,y+nHeight};

	// Если хотят превьюшку - сразу поставим флажок, что пытались
	lbPreviewWasLoaded = pInfo->bPreviewLoaded;
	if (abLoadPreview) pInfo->bPreviewLoaded = TRUE;

	// Очистить
	FillRect(hdc, &rc, hWhiteBrush);

	if (pszName[0] == L'.' && pszName[1] == L'.' && pszName[2] == 0) {
		if (!ghUpIcon)
			ghUpIcon = LoadIcon(ghPluginModule, MAKEINTRESOURCE(IDI_UP));
		if (ghUpIcon) {
			hIcon = ghUpIcon;
		}
		goto DoDraw;
	}

	// Попробовать извлечь превьюшку
	if (abLoadPreview) {
		if (pInfo->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
			if (!gThSet.bLoadFolders)
				return;
		} else {
			if (!gThSet.bLoadPreviews)
				return;
		}

		__try {
			hBmp = LoadThumbnail(pInfo);
		}__except(EXCEPTION_EXECUTE_HANDLER){
			hBmp = NULL;
		}

		if (!hBmp)
			return;
	}

	// Если не получилось - то ассоциированную иконку
	if (!hBmp)
	{
		shRc = SHGetFileInfo ( pInfo->lpwszFileName, pInfo->dwFileAttributes, &sfi, cbSize, 
			SHGFI_ICON|SHGFI_LARGEICON|SHGFI_USEFILEATTRIBUTES);
		if (shRc && sfi.hIcon)
			hIcon = sfi.hIcon;
	}

DoDraw:

	if (hBmp) {
		HDC hCompDC = CreateCompatibleDC(hdc);
		HBITMAP hOldBmp = (HBITMAP)SelectObject(hCompDC, hBmp);
		BitBlt(hdc, x, y, nWidth, nHeight, hCompDC, 0,0, SRCCOPY);
		SelectObject(hCompDC, hOldBmp);
		DeleteDC(hCompDC);
		DeleteObject(hBmp);

	} else if (hIcon) {
		nDrawRC = DrawIconEx(hdc,
			x+nXIconSpace, y+nYIconSpace,
			hIcon, nXIcon, nYIcon, 0, NULL, DI_NORMAL);
	} else {
		// Нарисовать стандартную иконку?
	}
	if (sfi.hIcon) {
		DestroyIcon(sfi.hIcon);
		sfi.hIcon = NULL;
	}
}
HBITMAP CThumbnails::LoadThumbnail(struct tag_CacheInfo* pItem)
{
	if (!gpDesktopFolder)
		return NULL;

	const wchar_t* pFileName = pItem->lpwszFileName;

	// Пока UNC не обрабатываем
	if (pFileName[0] == L'\\' && pFileName[1] == L'\\' && (pFileName[2] == L'.' || pFileName[2] == L'?') && pFileName[3] == L'\\') {
		pFileName += 4;
		if (pFileName[0] == L'U' && pFileName[1] == L'N' && pFileName[2] == L'C' && pFileName[3] == L'\\') {
			return NULL;
		}
	}

	int nLen = lstrlen(pFileName);
	if (nLen > 2*MAX_PATH)
		return NULL; // Шелл такой путь не обработает

	wchar_t *pszSourceFile = /*_wcsdup(pFileName)*/ (wchar_t*)calloc((nLen+1),2), *pszSlash = NULL;
	if (!pszSourceFile) {
		_ASSERTE(pszSourceFile!=NULL);
		return NULL;
	}
	lstrcpy(pszSourceFile, pFileName);
	pszSlash = wcsrchr(pszSourceFile, '\\');
	if (!pszSlash || ((pszSlash - pszSourceFile) < 2)) {
		free(pszSourceFile);
		return NULL;
	}


	IShellFolder *pFile=NULL;
	IExtractImage *pEI=NULL;
	LPITEMIDLIST pIdl = NULL;
	wchar_t wchPrev = L'\\';
	ULONG nEaten = 0;
	DWORD dwAttr = 0;
	HRESULT hr = S_OK;
	HBITMAP hbmp = NULL;
	BOOL lbRc = FALSE;
	wchar_t wsPathBuffer[MAX_PATH*2+32]; //, *pszThumbs = NULL;
	SIZE size;
	DWORD dwPrior = 0, dwFlags = 0;
	DWORD nBitDepth = 32;
	//HBITMAP hBmp = NULL, hOldBmp = NULL;

	//// Подготовить путь к шелловскому кешу превьюшек
	//lstrcpy(wsPathBuffer, pFileName);
	//if (abFolder) {
	//	pszThumbs = wsPathBuffer+wcslen(wsPathBuffer)-1;
	//	if (*pszThumbs != L'\\') {
	//		pszThumbs++;
	//		pszThumbs[0] = L'\\';
	//		pszThumbs[1] = 0;
	//	}
	//	pszThumbs++;
	//} else {
	//	pszThumbs = wcsrchr(wsPathBuffer, L'\\');
	//	if (pszThumbs) pszThumbs++;
	//}
	//if (pszThumbs) {
	//	lstrcpy(pszThumbs, L"Thumbs.db");
	//} else {
	//	wsPathBuffer[0] = 0;
	//}


	__try
	{
		if (SUCCEEDED(hr)) {
			if (*(pszSlash-1) == L':') pszSlash++; // для корня диска нужно слэш оставить
			wchPrev = *pszSlash; *pszSlash = 0;

			hr = gpDesktopFolder->ParseDisplayName(NULL, NULL, pszSourceFile, &nEaten, &pIdl, &dwAttr);

			*pszSlash = wchPrev;
		}

		if (SUCCEEDED(hr)) {
			hr = gpDesktopFolder->BindToObject(pIdl, NULL, IID_IShellFolder, (void**)&pFile);
			if (pIdl) { CoTaskMemFree(pIdl); pIdl = NULL; }
		}

		if (SUCCEEDED(hr)) {
			if (wchPrev=='\\') pszSlash ++;
			hr = pFile->ParseDisplayName(NULL, NULL, pszSlash, &nEaten, &pIdl, &dwAttr);
		}	    

		if (SUCCEEDED(hr)) {
			hr = pFile->GetUIObjectOf(NULL, 1, (LPCITEMIDLIST*)&pIdl, IID_IExtractImage, NULL, (void**)&pEI);
			// Если возвращает "Файл не найден" - значит файл не содержит превьюшки!
			if (pIdl) { CoTaskMemFree(pIdl); pIdl = NULL; }
		}

		if (SUCCEEDED(hr)) {
			// Пытаемся дернуть картинку сразу. Большое разрешение все-равно получить
			// не удастся, а иногда Shell дает IID_IExtractImage, но отказывается отдать Bitmap.



			wsPathBuffer[0] = 0;
			size.cx = nWidth;
			size.cy = nHeight;


			dwFlags = IEIFLAG_SCREEN|IEIFLAG_ASYNC; //|IEIFLAG_QUALITY; // IEIFLAG_ASPECT
			hr = pEI->GetLocation(wsPathBuffer, 512, &dwPrior, &size, nBitDepth, &dwFlags);

			// Ошибка 0x8000000a (E_PENDING) теоретически может возвращаться, если pEI запустил извлечение превьюшки
			// в Background thread. И теоретически, он должен поддерживать интерфейс IID_IRunnableTask для его 
			// остановки и проверки статуса.
			// Эту ошибку могут возвращать Adobe, SolidEdge (jt), может еще кто...

			// На путь (wsPathBuffer) ориентироваться нельзя (в SE его нет). Вообще непонятно зачем он нужен...
			if (hr==E_PENDING) {
				IRunnableTask* pRun = NULL;
				ULONG lRunState = 0;
				hr = pEI->QueryInterface(IID_IRunnableTask, (void**)&pRun);
				// А вот не экспортит SE этот интерфейс
				if (SUCCEEDED(hr) && pRun) {
					hr = pRun->Run();
					Sleep(10);
					while (!gbCancelAll) {
						lRunState = pRun->IsRunning();
						if (lRunState == IRTIR_TASK_FINISHED || lRunState == IRTIR_TASK_NOT_RUNNING)
							break;
						Sleep(10);
					}
					if (gbCancelAll)
						pRun->Kill(0);

					pRun->Release();
					pRun = NULL;
				}
				hr = S_OK;
			}
		}

		if (SUCCEEDED(hr)) {
			hr = pEI->Extract(&hbmp);
		}

	}
	__except(EXCEPTION_EXECUTE_HANDLER)
	{
		hbmp = NULL;
	}

	SafeRelease(pFile);
	if (pIdl) { CoTaskMemFree(pIdl); pIdl = NULL; }
	if (pszSourceFile) { free(pszSourceFile); pszSourceFile = NULL; }
	SafeRelease(pEI);

	return hbmp;
}


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
				// Нить завершилась
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
			
			BYTE nPanelColorIdx = pi->nFarColors[COL_PANELTEXT];
			COLORREF nBackColor = GetWindowLong(hwnd, 4*((nPanelColorIdx & 0xF0)>>4));
			#ifdef _DEBUG
			COLORREF nForeColor = GetWindowLong(hwnd, 4*(nPanelColorIdx & 0xF));
			#endif
			
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

	gpThumbnails = new CThumbnails();
	_ASSERTE(gpThumbnails);



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

	delete gpThumbnails;
	gpThumbnails = NULL;

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

	gpThumbnails->PaintItem(hdc, x+gThSet.nThumbFrame+gThSet.nHPadding, y+gThSet.nThumbFrame+gThSet.nVPadding,
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
	HBRUSH hWhiteBrush = CreateSolidBrush(crWhite);
	HFONT hFont = CreateFont(gThSet.nFontHeight,0,0,0,400,0,0,0,ANSI_CHARSET,OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,
		NONANTIALIASED_QUALITY,DEFAULT_PITCH,gThSet.sFontName);

	// Передернуть класс на предмет смены/инициализации настроек
	gpThumbnails->Init(hWhiteBrush);

	
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


	if (gpDesktopFolder == NULL) {
		HRESULT hr = SHGetDesktopFolder(&gpDesktopFolder);
		if (FAILED(hr)) {
			SafeRelease(gpDesktopFolder);
		}
	} else {
		_ASSERTE(gpDesktopFolder == NULL);
	}


	for (int nStep = 0; !gbCancelAll && nStep <= 1; nStep++)
	{
		int nItem = nTopItem;
		int nYCoord = 0;

		for (int Y = 0; !gbCancelAll && Y < nYCount && nItem < nItemCount; Y++) {
			int nXCoord = 0;
			for (int X = 0; !gbCancelAll && X < nXCount && nItem < nItemCount; X++, nItem++, ppItems) {
				const wchar_t* pszName = ppItems[nItem]->FindData.lpwszFileName;
				if (wcschr(pszName, L'\\')) {
					// Это уже может быть полный путь (TempPanel?)
					ppItems[nItem]->pszFullName = pszName;
				} else {
					// Полный путь нужно сформировать
					lstrcpyn(pszNamePtr, pszName, MAX_PATH+1);
					ppItems[nItem]->pszFullName = pszFull;
				}

				if (PaintItem(hCompDC, 0, 0, ppItems[nItem], (nItem==nCurrentItem), 
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

	SafeRelease(gpDesktopFolder);

	SelectObject(hCompDC, hOldPen);     DeleteObject(hPen);
	SelectObject(hCompDC, hOldBr);      DeleteObject(hWhiteBrush);
	SelectObject(hCompDC, hOldFont);    DeleteObject(hFont);
	SelectObject(hCompDC, hOldCompBmp); DeleteObject(hCompBmp);
	DeleteDC(hCompDC);

	
	for (i = 0; i < 4; i++) DeleteObject(hBack[i]);

//#ifdef _DEBUG
//	BitBlt(hdc, 0,0,rc.right,rc.bottom, gpThumbnails->hField[0], 0,0, SRCCOPY);
//#endif
}
