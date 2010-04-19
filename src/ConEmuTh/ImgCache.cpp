
#include <stdio.h>
#include <windows.h>
#include <crtdbg.h>
//#include <ole2.h>
//#include <ShlObj.h>
#include "ConEmuTh.h"
#include "resource.h"

#include "ImgCache.h"

//#pragma comment( lib, "ole32.lib" )


HICON ghUpIcon = NULL;
//ITEMIDLIST DesktopID = {{0}};
//IShellFolder *gpDesktopFolder = NULL;


template <typename T>
inline void SafeRelease(T *&p)
{
	if (NULL != p)
	{
		p->Release();
		p = NULL;
	}
}

#define SAFEDELETEOBJECT(o) if ((o)!=NULL) { DeleteObject(o); (o)=NULL; }
#define SAFEDELETEDC(d) if ((d)!=NULL) { DeleteDC(d); (d)=NULL; }

CImgCache::CImgCache(HMODULE hSelf)
{
	mb_Quit = FALSE;
	ms_CachePath[0] = ms_LastStoragePath[0] = 0;
	//nWidth = nHeight = 0;
	nPreviewSize = nXIcon = nYIcon = nXIconSpace = nYIconSpace = 0;
	hWhiteBrush = NULL;
	//nFieldX = nFieldY = 0;
	//memset(hField,0,sizeof(hField));
	//memset(hFieldBmp,0,sizeof(hFieldBmp));
	//memset(hOldBmp,0,sizeof(hOldBmp));
	memset(CacheInfo,0,sizeof(CacheInfo));
	mh_CompDC = NULL;
	mh_OldBmp = mh_DibSection = NULL;
	mp_DibBytes = NULL; mn_DibBytes = 0;
	memset(Modules,0,sizeof(Modules));
	mn_ModuleCount = 0;
	mpsz_ModuleSlash = ms_ModulePath;
	if (GetModuleFileName(hSelf, ms_ModulePath, sizeofarray(ms_ModulePath))) {
		wchar_t* pszSlash = wcsrchr(ms_ModulePath, L'\\');
		if (pszSlash) mpsz_ModuleSlash = pszSlash+1;
	}
	*mpsz_ModuleSlash = 0;

	// Prepare root storage file pathname
	SetCacheLocation(NULL); // По умолчанию - в %TEMP%

	// Initialize interfaces
	mp_RootStorage = mp_CurrentStorage = NULL;

	// Initialize Com
	CoInitialize(NULL);
}

CImgCache::~CImgCache(void)
{
	HRESULT hr;

	Reset();
	FreeModules();

	if (mp_CurrentStorage) {
		hr = mp_CurrentStorage->Commit(STGC_DEFAULT);
		_ASSERTE(SUCCEEDED(hr));
		SafeRelease(mp_CurrentStorage);
	}
	if (mp_RootStorage) {
		hr = mp_RootStorage->Commit(STGC_DEFAULT);
		_ASSERTE(SUCCEEDED(hr));
		SafeRelease(mp_RootStorage);
	}
	////
	//SafeRelease(gpDesktopFolder);
	
	// Done Com
	CoUninitialize();
}

void CImgCache::LoadModules()
{
	if (mn_ModuleCount) return; // Уже загружали!

	_ASSERTE(mpsz_ModuleSlash);
	#ifdef WIN64
	lstrcpy(mpsz_ModuleSlash, L"*.t64");
	#else
	lstrcat(mpsz_ModuleSlash, L"*.t32");
	#endif

	WIN32_FIND_DATA fnd;
	HANDLE hFind = FindFirstFile(ms_ModulePath, &fnd);
	if (hFind == INVALID_HANDLE_VALUE)
		return; // ни одного нету

	do {
		if ((fnd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
			continue;

		lstrcpy(mpsz_ModuleSlash, fnd.cFileName);
		HMODULE hLib = LoadLibrary(ms_ModulePath);
		if (hLib) {
			CET_Init_t Init = (CET_Init_t)GetProcAddress(hLib, "CET_Init");
			CET_Done_t Done = (CET_Done_t)GetProcAddress(hLib, "CET_Done");
			CET_Load_t Load = (CET_Load_t)GetProcAddress(hLib, "CET_Load");

			if (!Init || !Done || !Load) {
				FreeLibrary(hLib);
				continue;
			}

			struct CET_Init InitArg = {sizeof(struct CET_Init)};
			InitArg.hModule = hLib;

			if (!Init(&InitArg)) {
				FreeLibrary(hLib);
				continue;
			}
			
			Modules[mn_ModuleCount].hModule = hLib;
			Modules[mn_ModuleCount].Init = Init;
			Modules[mn_ModuleCount].Done = Done;
			Modules[mn_ModuleCount].Load = Load;
			Modules[mn_ModuleCount].lParam = InitArg.lParam;

			mn_ModuleCount++;
		}
	} while (mn_ModuleCount<MAX_MODULES && FindNextFile(hFind, &fnd));

	FindClose(hFind);
}

void CImgCache::FreeModules()
{
	for (int i = 0; i < MAX_MODULES; i++) {
		if (Modules[i].hModule) {
			struct CET_Init InitArg = {sizeof(struct CET_Init)};
			InitArg.hModule = Modules[i].hModule;
			InitArg.lParam = Modules[i].lParam;
			Modules[i].Done(&InitArg);
			FreeLibrary(Modules[i].hModule);
			Modules[i].hModule = NULL;
		}
	}
}

void CImgCache::SetCacheLocation(LPCWSTR asCachePath)
{
	// Prepare root storage file pathname
	if (asCachePath) {
		lstrcpyn(ms_CachePath, asCachePath, MAX_PATH-32);
	} else {
		GetTempPath(MAX_PATH-32, ms_CachePath);
	}
	// add end slash
	int nLen = lstrlen(ms_CachePath);
	if (nLen && ms_CachePath[nLen-1] != L'\\') {
		ms_CachePath[nLen++] = L'\\'; ms_CachePath[nLen] = 0;
	}
	// add our storage file name
	lstrcpy(ms_CachePath+nLen, ImgCacheFileName);
}

DWORD CImgCache::ShellExtractionThread(LPVOID apArg)
{
	CImgCache *pImgCache = (CImgCache*)apArg;
	// Initialize Com
	CoInitialize(NULL);

	// Loop

	// Done Com
	CoUninitialize();
	return 0;
}

BOOL CImgCache::LoadPreview()
{
	return TRUE;
}

BOOL CImgCache::CheckDibCreated()
{
	if (!nPreviewSize) {
		_ASSERTE(nPreviewSize != 0);
		return FALSE;
	}
	if (mh_DibSection) {
		return TRUE;
	}
	
	_ASSERTE(mh_CompDC==NULL);
	if (!mh_CompDC) {
		mh_CompDC = CreateCompatibleDC(NULL);
	}
	
	BITMAPINFOHEADER bmi = {sizeof(BITMAPINFOHEADER)};
	bmi.biWidth = bmi.biHeight = nPreviewSize;
	bmi.biPlanes = 1;
	bmi.biBitCount = 32;
	bmi.biCompression = BI_RGB;

	_ASSERTE(mp_DibBytes==NULL);
	mp_DibBytes = NULL; mn_DibBytes = 0;
	HBITMAP mh_DibSection = CreateDIBSection(mh_CompDC, (BITMAPINFO*)&bmi, DIB_RGB_COLORS, (void**)&mp_DibBytes, NULL, 0);
	if (!mh_DibSection) {
		_ASSERTE(mh_DibSection);
		mp_DibBytes = NULL;
		DeleteDC(mh_CompDC); mh_CompDC = NULL;
		return FALSE;
	}
	_ASSERTE(mp_DibBytes);
	mn_DibBytes = bmi.biWidth * 4 * bmi.biHeight;

	//	OK
	mh_OldBmp = (HBITMAP)SelectObject(mh_CompDC, mh_DibSection);
	return TRUE;
}

void CImgCache::Reset()
{
	int i;
	//for (i=0; i<FIELD_MAX_COUNT; i++) {
	//	if (hField[i]) {
	//		if (hOldBmp[i]) { SelectObject(hField[i], hOldBmp[i]); hOldBmp[i] = NULL; }
	//		if (hFieldBmp[i]) { DeleteObject(hFieldBmp[i]); hFieldBmp[i] = NULL; }
	//		DeleteDC(hField[i]); hField[i] = NULL;
	//	}
	//}
	for (i=0; i<sizeofarray(CacheInfo); i++) {
		if (CacheInfo[i].lpwszFileName) {
			free(CacheInfo[i].lpwszFileName);
			CacheInfo[i].lpwszFileName = NULL;
		}
		if (CacheInfo[i].Pixels) {
			LocalFree(CacheInfo[i].Pixels);
			CacheInfo[i].Pixels = NULL;
		}
		if (CacheInfo[i].pszInfo) {
			LocalFree(CacheInfo[i].pszInfo);
			CacheInfo[i].pszInfo = NULL;
		}
	}
	//
	if (mh_CompDC && mh_OldBmp)
		SelectObject(mh_CompDC, mh_OldBmp); mh_OldBmp = NULL;
	DeleteObject(mh_DibSection); mp_DibBytes = NULL; mh_DibSection = NULL;
	DeleteDC(mh_CompDC); mh_CompDC = NULL;
};
void CImgCache::Init(HBRUSH ahWhiteBrush)
{
	// Инициализация (или сброс если изменились размеры превьюшек)
	hWhiteBrush = ahWhiteBrush; 
	_ASSERTE(gThSet.nThumbSize>=16);

	if (nPreviewSize != gThSet.nThumbSize || crBackground != gThSet.crBackground) {
		Reset();
		nPreviewSize = gThSet.nThumbSize;
		crBackground = gThSet.crBackground;
	}

	// Как центрируется иконка
	nXIcon = GetSystemMetrics(SM_CXICON);
	nYIcon = GetSystemMetrics(SM_CYICON);
	nXIconSpace = (nPreviewSize - nXIcon) >> 1;
	nYIconSpace = (nPreviewSize - nYIcon) >> 1;
};
BOOL CImgCache::FindInCache(CePluginPanelItem* pItem, int* pnIndex)
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
BOOL CImgCache::PaintItem(HDC hdc, int x, int y, CePluginPanelItem* pItem, BOOL abLoadPreview)
{
	int nIndex = -1;
	if (!CheckDibCreated())
		return FALSE;
	
	BOOL lbWasDraw = FALSE;
	//BOOL lbWasPreview = FALSE;
	BOOL lbReady = FindInCache(pItem, &nIndex);
	//if ((nFieldX*nFieldY) == 0) {
	//	_ASSERTE((nFieldX*nFieldY)!=0);
	//	return FALSE;
	//}
	//int iField = nIndex / (nFieldX*nFieldY);
	//int ii = nIndex - (iField*nFieldX*nFieldY);
	//int iRow = ii / nFieldX;
	//int iCol = ii - iRow*nFieldX;

	//if (iField<0 || iField>=FIELD_MAX_COUNT) {
	//	_ASSERTE(iField>=0 && iField<FIELD_MAX_COUNT);
	//	return FALSE;
	//}
	//if (hField[iField] == NULL) {
	//	DWORD dwErr = 0;
	//	HDC hScreen = GetDC(NULL);
	//	hField[iField] = CreateCompatibleDC(hScreen);
	//	hFieldBmp[iField] = CreateCompatibleBitmap(hScreen, nWidth*nFieldX, nHeight*nFieldY);
	//	dwErr = GetLastError();
	//	ReleaseDC(NULL, hScreen);
	//	if (hFieldBmp[iField] == NULL) {
	//		wchar_t szErr[255];
	//		wsprintf(szErr, L"Can't create compatible bitmap (%ix%i)\nErrCode=0x%08X", nWidth*nFieldX, nHeight*nFieldY, dwErr);
	//		MessageBox(NULL, szErr, L"ConEmu Thumbnails", MB_OK|MB_SYSTEMMODAL|MB_ICONERROR);
	//	}
	//	hOldBmp[iField] = (HBITMAP)SelectObject(hField[iField], hFieldBmp[iField]);
	//}
	//
	//RECT rc = {iCol*nWidth, iRow*nHeight, (iCol+1)*nWidth, (iRow+1)*nHeight};

	// Если в прошлый раз загрузили только ShellIcon и сейчас просят Preview
	if (lbReady && abLoadPreview && CacheInfo[nIndex].bPreviewLoaded == FALSE)
		lbReady = FALSE;

	// Если нужно загрузить иконку или превьюшку (или обновить их)
	if (!lbReady) {
		UpdateCell(CacheInfo+nIndex, abLoadPreview);
	}
	
	// Скинуть биты в MemDC
	CopyBits(CacheInfo[nIndex].crSize, (LPBYTE)(CacheInfo[nIndex].Pixels), CacheInfo[nIndex].cbStride,
		CacheInfo[nIndex].crSize, mp_DibBytes);
	//int nMaxY = min(nPreviewSize,CacheInfo[nIndex].crSize.Y);
	//LPBYTE lpSrc = (LPBYTE)(CacheInfo[nIndex].Pixels);
	//_ASSERTE(lpSrc);
	//LPBYTE lpDst = mp_DibBytes;
	//_ASSERTE(lpDst);
	//DWORD nSrcStride = CacheInfo[nIndex].cbStride;
	//_ASSERTE(nSrcStride);
	//DWORD nDstStride = nPreviewSize*4;
	//for (int y = 0; y < nMaxY; y++) {
	//	memmove(lpDst, lpSrc, nSrcStride);
	//	lpSrc += nSrcStride;
	//	lpDst += nDstStride;
	//}
	

	// А теперь - собственно отрисовка куда просили

	if (nPreviewSize > CacheInfo[nIndex].crSize.X || nPreviewSize > CacheInfo[nIndex].crSize.Y) {
		// Очистить
		RECT rc = {x,y,x+nPreviewSize,y+nPreviewSize};
		FillRect(hdc, &rc, hWhiteBrush);
		// Со сдвигом
		lbWasDraw = BitBlt(hdc, x+nXIconSpace,y+nXIconSpace,
			x+CacheInfo[nIndex].crSize.X,y+CacheInfo[nIndex].crSize.Y, mh_CompDC,
			CacheInfo[nIndex].crSize.X,CacheInfo[nIndex].crSize.X, SRCCOPY);
	} else {
		lbWasDraw = BitBlt(hdc, x,y,x+nPreviewSize,y+nPreviewSize, mh_CompDC,
			CacheInfo[nIndex].crSize.X,CacheInfo[nIndex].crSize.X, SRCCOPY);
	}
	//
	//	if (hBmp) {
	//		HDC hCompDC = CreateCompatibleDC(hdc);
	//		HBITMAP hOldBmp = (HBITMAP)SelectObject(hCompDC, hBmp);
	//		BitBlt(hdc, x, y, nWidth, nHeight, hCompDC, 0,0, SRCCOPY);
	//		SelectObject(hCompDC, hOldBmp);
	//		DeleteDC(hCompDC);
	//		DeleteObject(hBmp);
	//
	//	} else if (hIcon) {
	//		nDrawRC = DrawIconEx(hdc,
	//			x+nXIconSpace, y+nYIconSpace,
	//			hIcon, nXIcon, nYIcon, 0, NULL, DI_NORMAL);

	

	
	return lbWasDraw;
}
void CImgCache::CopyBits(COORD crSrcSize, LPBYTE lpSrc, DWORD nSrcStride, COORD crDstSize, LPBYTE lpDst)
{
	int nMaxY = min(crDstSize.Y,crSrcSize.Y);
	_ASSERTE(lpSrc);
	_ASSERTE(lpDst);
	_ASSERTE(nSrcStride);
	DWORD nDstStride = crDstSize.X*4;
	DWORD nStride = min(nDstStride,nSrcStride);
	_ASSERTE(nStride);
	for (int y = 0; y < nMaxY; y++) {
		memmove(lpDst, lpSrc, nStride);
		lpSrc += nSrcStride;
		lpDst += nDstStride;
	}
}
void CImgCache::UpdateCell(struct tag_CacheInfo* pInfo, BOOL abLoadPreview)
{
	if (pInfo->bPreviewLoaded)
		return; // Уже

	if (abLoadPreview) {
		// Если хотят превьюшку - сразу поставим флажок, что пытались
		pInfo->bPreviewLoaded = TRUE;

		// Для папок - самостоятельно
		if (pInfo->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
			return;
		} else {
			LoadThumbnail(pInfo);
		}
	} else {
		LoadShellIcon(pInfo);
	}

//	HICON hIcon = NULL;
//	HBITMAP hBmp = NULL;
//	BOOL lbPreviewWasLoaded = FALSE;
//	const wchar_t* pszName = pInfo->lpwszFileName;
//	int nDrawRC = -1;
//	SHFILEINFO sfi = {NULL};
//	UINT cbSize = sizeof(sfi);
//	DWORD_PTR shRc = 0;
//	RECT rc = {x,y,x+nWidth,y+nHeight};
//
//	
//	}
//	lbPreviewWasLoaded = pInfo->bPreviewLoaded;
//	if (abLoadPreview) pInfo->bPreviewLoaded = TRUE;
//
//	if (pszName[0] == L'.' && pszName[1] == L'.' && pszName[2] == 0) {
//		if (!ghUpIcon)
//			ghUpIcon = LoadIcon(ghPluginModule, MAKEINTRESOURCE(IDI_UP));
//		if (ghUpIcon) {
//			hIcon = ghUpIcon;
//		}
//		goto DoDraw;
//	}
//
//	// Попробовать извлечь превьюшку
//	if (abLoadPreview) {
//		if (pInfo->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
//			if (!gThSet.bLoadFolders)
//				return;
//		} else {
//			if (!gThSet.bLoadPreviews)
//				return;
//		}
//
//		__try {
//			hBmp = LoadThumbnail(pInfo);
//		}__except(EXCEPTION_EXECUTE_HANDLER){
//			hBmp = NULL;
//		}
//
//		if (!hBmp)
//			return;
//	}
//
//	// Если не получилось - то ассоциированную иконку
//	if (!hBmp)
//	{
//		shRc = SHGetFileInfo ( pInfo->lpwszFileName, pInfo->dwFileAttributes, &sfi, cbSize, 
//			SHGFI_ICON|SHGFI_LARGEICON|SHGFI_USEFILEATTRIBUTES);
//		if (shRc && sfi.hIcon)
//			hIcon = sfi.hIcon;
//	}
//
//DoDraw:
//
//	// Очистить
//	FillRect(hdc, &rc, hWhiteBrush);
//
//	if (hBmp) {
//		HDC hCompDC = CreateCompatibleDC(hdc);
//		HBITMAP hOldBmp = (HBITMAP)SelectObject(hCompDC, hBmp);
//		BitBlt(hdc, x, y, nWidth, nHeight, hCompDC, 0,0, SRCCOPY);
//		SelectObject(hCompDC, hOldBmp);
//		DeleteDC(hCompDC);
//		DeleteObject(hBmp);
//
//	} else if (hIcon) {
//		nDrawRC = DrawIconEx(hdc,
//			x+nXIconSpace, y+nYIconSpace,
//			hIcon, nXIcon, nYIcon, 0, NULL, DI_NORMAL);
//	} else {
//		// Нарисовать стандартную иконку?
//	}
//	if (sfi.hIcon) {
//		DestroyIcon(sfi.hIcon);
//		sfi.hIcon = NULL;
//	}
}
BOOL CImgCache::LoadShellIcon(struct tag_CacheInfo* pItem)
{
	HICON hIcon = NULL;
	HBITMAP hBmp = NULL;
	BOOL lbPreviewWasLoaded = FALSE;
	const wchar_t* pszName = pItem->lpwszFileName;
	int nDrawRC = -1;
	SHFILEINFO sfi = {NULL};
	UINT cbSize = sizeof(sfi);
	DWORD_PTR shRc = 0;
	RECT rc = {0,0,nXIcon,nYIcon};

	if (!CheckDibCreated())
		return FALSE;

	if (pItem->Pixels) {
		LocalFree(pItem->Pixels); pItem->Pixels = NULL;
	}
	if (pItem->pszInfo) {
		LocalFree(pItem->pszInfo); pItem->pszInfo = NULL;
	}


	if (pszName[0] == L'.' && pszName[1] == L'.' && pszName[2] == 0) {
		if (!ghUpIcon)
			ghUpIcon = LoadIcon(ghPluginModule, MAKEINTRESOURCE(IDI_UP));
		if (ghUpIcon) {
			hIcon = ghUpIcon;
		}
	} else {
		// ассоциированная иконка
		shRc = SHGetFileInfo ( pItem->lpwszFileName, pItem->dwFileAttributes, &sfi, cbSize, 
			SHGFI_ICON|SHGFI_LARGEICON|SHGFI_USEFILEATTRIBUTES);
		if (shRc && sfi.hIcon)
			hIcon = sfi.hIcon;
	}


	// Очистить
	FillRect(mh_CompDC, &rc, hWhiteBrush);

	if (hIcon) {
		nDrawRC = DrawIconEx(mh_CompDC, 0, 0, hIcon, nXIcon, nYIcon, 0, NULL, DI_NORMAL);
		if (sfi.hIcon) {
			DestroyIcon(sfi.hIcon);
		}
	} else {
		// Нарисовать стандартную иконку?
	}


	pItem->crSize.X = nXIcon; pItem->crSize.Y = nYIcon;
	pItem->cbStride = nXIcon*4;
	pItem->nBits = 32;
	pItem->ColorModel = CET_CM_BGR;
	pItem->Pixels = (LPDWORD)LocalAlloc(LMEM_FIXED, pItem->cbStride * pItem->crSize.Y);
	if (!pItem->Pixels) {
		_ASSERTE(pItem->Pixels);
		return FALSE;
	}

	CopyBits(pItem->crSize, mp_DibBytes, nPreviewSize*4, pItem->crSize, (LPBYTE)pItem->Pixels);

	return TRUE;
}
BOOL CImgCache::LoadThumbnail(struct tag_CacheInfo* pItem)
{
	if (!CheckDibCreated())
		return FALSE;

	struct CET_LoadPreview PV = {sizeof(struct CET_LoadPreview)};

	PV.sFileName = pItem->lpwszFileName;
	PV.ftModified = PV.ftModified;
	PV.nFileSize = PV.nFileSize;
	PV.crLoadSize.X = PV.crLoadSize.Y = nPreviewSize;
	PV.bTilesMode = FALSE; // TRUE, when pszInfo acquired.
	PV.crBackground = crBackground; // Must be used to fill background.

	for (int i = 0; i < MAX_MODULES; i++) {
		if (Modules[i].hModule == NULL)
			continue;

		PV.lParam = Modules[i].lParam;
		if (!Modules[i].Load(&PV))
			continue;

		if (pItem->Pixels)
			LocalFree(pItem->Pixels);
		pItem->Pixels = PV.Pixels;
		if (pItem->pszInfo)
			LocalFree(pItem->pszInfo);
		pItem->pszInfo = PV.pszInfo;

		pItem->crSize = PV.crSize; // Предпочтительно, должен совпадать с crLoadSize
		pItem->cbStride = PV.cbStride; // Bytes per line
		pItem->nBits = PV.nBits; // 32 bit required!
		pItem->ColorModel = PV.ColorModel; // One of CET_CM_xxx
		return TRUE;
	}

	return FALSE;

	//if (gpDesktopFolder == NULL) {
	//	HRESULT hr = SHGetDesktopFolder(&gpDesktopFolder);
	//	if (FAILED(hr)) {
	//		SafeRelease(gpDesktopFolder);
	//	}
	//}
	//if (!gpDesktopFolder)
	//	return NULL;

	//const wchar_t* pFileName = pItem->lpwszFileName;

	//// Пока UNC не обрабатываем
	//if (pFileName[0] == L'\\' && pFileName[1] == L'\\' && (pFileName[2] == L'.' || pFileName[2] == L'?') && pFileName[3] == L'\\') {
	//	pFileName += 4;
	//	if (pFileName[0] == L'U' && pFileName[1] == L'N' && pFileName[2] == L'C' && pFileName[3] == L'\\') {
	//		return NULL;
	//	}
	//}

	//int nLen = lstrlen(pFileName);
	//if (nLen > 2*MAX_PATH)
	//	return NULL; // Шелл такой путь не обработает

	//wchar_t *pszSourceFile = /*_wcsdup(pFileName)*/ (wchar_t*)calloc((nLen+1),2), *pszSlash = NULL;
	//if (!pszSourceFile) {
	//	_ASSERTE(pszSourceFile!=NULL);
	//	return NULL;
	//}
	//lstrcpy(pszSourceFile, pFileName);
	//pszSlash = wcsrchr(pszSourceFile, '\\');
	//if (!pszSlash || ((pszSlash - pszSourceFile) < 2)) {
	//	free(pszSourceFile);
	//	return NULL;
	//}


	//IShellFolder *pFile=NULL;
	//IExtractImage *pEI=NULL;
	//LPITEMIDLIST pIdl = NULL;
	//wchar_t wchPrev = L'\\';
	//ULONG nEaten = 0;
	//DWORD dwAttr = 0;
	//HRESULT hr = S_OK;
	//HBITMAP hbmp = NULL;
	//BOOL lbRc = FALSE;
	//wchar_t wsPathBuffer[MAX_PATH*2+32]; //, *pszThumbs = NULL;
	//SIZE size;
	//DWORD dwPrior = 0, dwFlags = 0;
	//DWORD nBitDepth = 32;
	////HBITMAP hBmp = NULL, hOldBmp = NULL;

	////// Подготовить путь к шелловскому кешу превьюшек
	////lstrcpy(wsPathBuffer, pFileName);
	////if (abFolder) {
	////	pszThumbs = wsPathBuffer+wcslen(wsPathBuffer)-1;
	////	if (*pszThumbs != L'\\') {
	////		pszThumbs++;
	////		pszThumbs[0] = L'\\';
	////		pszThumbs[1] = 0;
	////	}
	////	pszThumbs++;
	////} else {
	////	pszThumbs = wcsrchr(wsPathBuffer, L'\\');
	////	if (pszThumbs) pszThumbs++;
	////}
	////if (pszThumbs) {
	////	lstrcpy(pszThumbs, L"Thumbs.db");
	////} else {
	////	wsPathBuffer[0] = 0;
	////}


	//__try
	//{
	//	if (SUCCEEDED(hr)) {
	//		if (*(pszSlash-1) == L':') pszSlash++; // для корня диска нужно слэш оставить
	//		wchPrev = *pszSlash; *pszSlash = 0;

	//		hr = gpDesktopFolder->ParseDisplayName(NULL, NULL, pszSourceFile, &nEaten, &pIdl, &dwAttr);

	//		*pszSlash = wchPrev;
	//	}

	//	if (SUCCEEDED(hr)) {
	//		hr = gpDesktopFolder->BindToObject(pIdl, NULL, IID_IShellFolder, (void**)&pFile);
	//		if (pIdl) { CoTaskMemFree(pIdl); pIdl = NULL; }
	//	}

	//	if (SUCCEEDED(hr)) {
	//		if (wchPrev=='\\') pszSlash ++;
	//		hr = pFile->ParseDisplayName(NULL, NULL, pszSlash, &nEaten, &pIdl, &dwAttr);
	//	}	    

	//	if (SUCCEEDED(hr)) {
	//		hr = pFile->GetUIObjectOf(NULL, 1, (LPCITEMIDLIST*)&pIdl, IID_IExtractImage, NULL, (void**)&pEI);
	//		// Если возвращает "Файл не найден" - значит файл не содержит превьюшки!
	//		if (pIdl) { CoTaskMemFree(pIdl); pIdl = NULL; }
	//	}

	//	if (SUCCEEDED(hr)) {
	//		// Пытаемся дернуть картинку сразу. Большое разрешение все-равно получить
	//		// не удастся, а иногда Shell дает IID_IExtractImage, но отказывается отдать Bitmap.



	//		wsPathBuffer[0] = 0;
	//		size.cx = nWidth;
	//		size.cy = nHeight;


	//		dwFlags = IEIFLAG_SCREEN|IEIFLAG_ASYNC; //|IEIFLAG_QUALITY; // IEIFLAG_ASPECT
	//		wsPathBuffer[0] = 0;
	//		hr = pEI->GetLocation(wsPathBuffer, 512, &dwPrior, &size, nBitDepth, &dwFlags);

	//		// Ошибка 0x8000000a (E_PENDING) теоретически может возвращаться, если pEI запустил извлечение превьюшки
	//		// в Background thread. И теоретически, он должен поддерживать интерфейс IID_IRunnableTask для его 
	//		// остановки и проверки статуса.
	//		// Эту ошибку могут возвращать Adobe, SolidEdge (jt), может еще кто...

	//		// На путь (wsPathBuffer) ориентироваться нельзя (в SE его нет). Вообще непонятно зачем он нужен...
	//		if (hr==E_PENDING) {
	//			IRunnableTask* pRun = NULL;
	//			ULONG lRunState = 0;
	//			hr = pEI->QueryInterface(IID_IRunnableTask, (void**)&pRun);
	//			// А вот не экспортит SE этот интерфейс
	//			if (SUCCEEDED(hr) && pRun) {
	//				hr = pRun->Run();
	//				Sleep(10);
	//				while (!gbCancelAll) {
	//					lRunState = pRun->IsRunning();
	//					if (lRunState == IRTIR_TASK_FINISHED || lRunState == IRTIR_TASK_NOT_RUNNING)
	//						break;
	//					Sleep(10);
	//				}
	//				if (gbCancelAll)
	//					pRun->Kill(0);

	//				pRun->Release();
	//				pRun = NULL;
	//			}
	//			hr = S_OK;
	//		}
	//	}

	//	if (SUCCEEDED(hr)) {
	//		hr = pEI->Extract(&hbmp);
	//	}

	//}
	//__except(EXCEPTION_EXECUTE_HANDLER)
	//{
	//	hbmp = NULL;
	//}

	//SafeRelease(pFile);
	//if (pIdl) { CoTaskMemFree(pIdl); pIdl = NULL; }
	//if (pszSourceFile) { free(pszSourceFile); pszSourceFile = NULL; }
	//SafeRelease(pEI);

	//return hbmp;
}

BOOL CImgCache::GetBits(HBITMAP hBmp)
{
	//if (SUCCEEDED(ghLastWin32Error) && hBmp) {
	//	BITMAPCOREHEADER *bch = (BITMAPCOREHEADER*)GlobalLock((HGLOBAL)hBmp);
	//	if (bch) GlobalUnlock((HGLOBAL)hBmp);


	//	HDC hScreenDC = GetDC(NULL);
	//	_ASSERTE(hScreenDC);
	//	_ASSERTE(pDecodeInfo->pImage == NULL);
	//	pDecodeInfo->pImage = NULL;

	//	HDC hCompDc2 = CreateCompatibleDC(hScreenDC);
	//	HBITMAP hOld2 = (HBITMAP)SelectObject(hCompDc2, hBmp);

	//	BOOL lbCanGetBits = FALSE, lbHasAlpha = FALSE, lbHasNoAlpha = FALSE;
	//	BITMAPINFO* pbi = (BITMAPINFO*)CALLOC(sizeof(BITMAPINFO)+255*sizeof(RGBQUAD));
	//	pbi->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	//	int iDibRc = GetDIBits(hCompDc2, hBmp, 0, 0, NULL, pbi, DIB_RGB_COLORS);
	//	if (iDibRc && pbi->bmiHeader.biWidth && pbi->bmiHeader.biHeight) {
	//		size.cx = pbi->bmiHeader.biWidth;
	//		size.cy = pbi->bmiHeader.biHeight;
	//		lbCanGetBits = (pbi->bmiHeader.biBitCount == 32);
	//		_ASSERTE(nBitDepth==32);
	//	}

	//	pDecodeInfo->lWidth = pDecodeInfo->lSrcWidth = size.cx;
	//	pDecodeInfo->lHeight = pDecodeInfo->lSrcHeight = size.cy;
	//	pDecodeInfo->nBPP = nBitDepth;

	//	// ПОКА с выравниванием не заморачиваемся, т.к. размер фиксирован 400x400
	//	pDecodeInfo->lImagePitch = pDecodeInfo->lWidth * 4;
	//	pDecodeInfo->pImage = (LPBYTE)CALLOC(pDecodeInfo->lImagePitch * pDecodeInfo->lHeight);
	//	if (!pDecodeInfo->pImage) {
	//		pDecodeInfo->nErrNumber = PSE_NOT_ENOUGH_MEMORY;

	//	} else {

	//		if (lbCanGetBits) {
	//			// Чтобы вернуть прозрачность - сначала пробуем напролом получить биты
	//			iDibRc = GetDIBits(hCompDc2, hBmp, 0, size.cy, pDecodeInfo->pImage, pbi, DIB_RGB_COLORS);
	//			lbRc = iDibRc!=0;
	//			if (!lbRc) {
	//				pDecodeInfo->nErrNumber = PSE_GETDIBITS_FAILED;
	//			} else if (gbAutoDetectAlphaChannel) {
	//				//PRAGMA_ERROR("нужно просматривать все полученные биты на предмет наличия в них альфа канала и ставить PVD_IDF_ALPHA");
	//				LPBYTE pSrc = pDecodeInfo->pImage;
	//				DWORD lAbsSrcPitch = pDecodeInfo->lImagePitch, A;
	//				for (DWORD y = size.cy; y-- && !(lbHasAlpha && lbHasNoAlpha); pSrc += lAbsSrcPitch) {
	//					for (int x = 0; x<size.cx; x++) {
	//						A = (((DWORD*)pSrc)[x]) & 0xFF000000;
	//						if (A) lbHasAlpha = TRUE; else lbHasNoAlpha = TRUE;
	//					}
	//				}
	//			}
	//		} else {
	//			HDC hCompDc1 = CreateCompatibleDC(hScreenDC);

	//			BITMAPINFOHEADER bmi = {sizeof(BITMAPINFOHEADER)};
	//			bmi.biWidth = size.cx;
	//			bmi.biHeight = size.cy;
	//			bmi.biPlanes = 1;
	//			bmi.biBitCount = 32;
	//			bmi.biCompression = BI_RGB;

	//			LPBYTE pBits = NULL;
	//			HBITMAP hDIB = CreateDIBSection(hScreenDC, (BITMAPINFO*)&bmi, DIB_RGB_COLORS, (void**)&pBits, NULL, 0);
	//			_ASSERTE(hDIB);

	//			HBITMAP hOld1 = (HBITMAP)SelectObject(hCompDc1, hDIB);

	//			//if (pDecodeInfo->cbSize >= sizeof(pvdInfoDecode2)) {
	//			//	HBRUSH hBackBr = CreateSolidBrush(pDecodeInfo->nBackgroundColor);
	//			//	RECT rc = {0,0,size.cx,size.cy};
	//			//	FillRect(hCompDc1, &rc, hBackBr);
	//			//	DeleteObject(hBackBr);
	//			//}

	//			// Не прокатывает. PNG и GIF "прозрачными" не рендерятся. Получается белый фон. Ну и ладно
	//			//BLENDFUNCTION bf = {AC_SRC_OVER,0,255,AC_SRC_ALPHA};
	//			//lbRc = AlphaBlend(hCompDc1, 0,0,size.cx,size.cy, hCompDc2, 0,0, size.cx,size.cy, bf);
	//			//if (!lbRc)
	//			lbRc = BitBlt(hCompDc1, 0,0,size.cx,size.cy, hCompDc2, 0,0, SRCCOPY);

	//			if (!lbRc) {
	//				pDecodeInfo->nErrNumber = PSE_BITBLT_FAILED;
	//			} else {
	//				memmove(pDecodeInfo->pImage, pBits, pDecodeInfo->lImagePitch * pDecodeInfo->lHeight);
	//			}

	//			if (hCompDc1 && hOld1)
	//				SelectObject(hCompDc1, hOld1);
	//			SAFEDELETEOBJECT(hDIB);
	//			SAFEDELETEDC(hCompDc1);
	//		}

	//		if (lbRc) {
	//			// В DIB строки идут снизу вверх
	//			pDecodeInfo->lImagePitch = - pDecodeInfo->lImagePitch;

	//			pDecodeInfo->ColorModel = (lbHasAlpha && lbHasNoAlpha) ? PVD_CM_BGRA : PVD_CM_BGR;
	//			pDecodeInfo->Flags = (lbHasAlpha && lbHasNoAlpha) ? PVD_IDF_ALPHA : 0;
	//		}
	//	}

	//	if (hCompDc2 && hOld2)
	//		SelectObject(hCompDc2, hOld2);
	//	SAFEDELETEDC(hCompDc2);
	//	SAFEDELETEOBJECT(hBmp);
	//	SAFERELEASEDC(NULL, hScreenDC);
	//}
	return FALSE;
}
