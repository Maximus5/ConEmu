
#include <stdio.h>
#include <windows.h>
//#include <crtdbg.h>
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
	nPreviewSize = 0; //nXIcon = nYIcon = nXIconSpace = nYIconSpace = 0;
	hbrBack = NULL;
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
	if (GetModuleFileName(hSelf, ms_ModulePath, countof(ms_ModulePath))) {
		wchar_t* pszSlash = wcsrchr(ms_ModulePath, L'\\');
		if (pszSlash) mpsz_ModuleSlash = pszSlash+1;
	}
	*mpsz_ModuleSlash = 0;

	// Prepare root storage file pathname
	SetCacheLocation(NULL); // По умолчанию - в %TEMP%

	// Загрузить "модули"
	LoadModules();

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

	if (hbrBack) {
		DeleteObject(hbrBack); hbrBack = NULL;
	}

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
		HMODULE hLib = NULL;
		
		SAFETRY {
			hLib = LoadLibrary(ms_ModulePath);
		} SAFECATCH  {
			hLib = NULL;
		}
		
		if (hLib) {
			CET_Init_t Init = (CET_Init_t)GetProcAddress(hLib, "CET_Init");
			CET_Done_t Done = (CET_Done_t)GetProcAddress(hLib, "CET_Done");
			CET_Load_t LoadInfo = (CET_Load_t)GetProcAddress(hLib, "CET_Load");
			CET_Free_t FreeInfo = (CET_Free_t)GetProcAddress(hLib, "CET_Free");
			CET_Cancel_t Cancel = (CET_Cancel_t)GetProcAddress(hLib, "CET_Cancel");

			if (!Init || !Done || !LoadInfo || !FreeInfo) {
				 SAFETRY  {
					FreeLibrary(hLib);
				} SAFECATCH  {
				}
				continue;
			}

			struct CET_Init InitArg = {sizeof(struct CET_Init)};
			InitArg.hModule = hLib;
			
			BOOL lbInitRc;
			
			 SAFETRY  {
				lbInitRc = Init(&InitArg);
			} SAFECATCH  {
				lbInitRc = FALSE;
			}

			if (!lbInitRc) {
				 SAFETRY  {
					FreeLibrary(hLib);
				} SAFECATCH  {
				}
				continue;
			}
			
			Modules[mn_ModuleCount].hModule = hLib;
			Modules[mn_ModuleCount].Init = Init;
			Modules[mn_ModuleCount].Done = Done;
			Modules[mn_ModuleCount].LoadInfo = LoadInfo;
			Modules[mn_ModuleCount].FreeInfo = FreeInfo;
			Modules[mn_ModuleCount].Cancel = Cancel;
			Modules[mn_ModuleCount].pContext = InitArg.pContext;

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
			InitArg.pContext = Modules[i].pContext;
			SAFETRY  {
				Modules[i].Done(&InitArg);
			} SAFECATCH  {
			}
			 SAFETRY  {
				FreeLibrary(Modules[i].hModule);
			} SAFECATCH  {
			}
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
	
	if (mh_CompDC != NULL) {
		_ASSERTE(mh_CompDC==NULL);
	} else {
		mh_CompDC = CreateCompatibleDC(NULL);
	}
	
	BITMAPINFOHEADER bmi = {sizeof(BITMAPINFOHEADER)};
	bmi.biWidth = nPreviewSize;
	bmi.biHeight = -nPreviewSize; // Top-Down DIB
	bmi.biPlanes = 1;
	bmi.biBitCount = 32;
	bmi.biCompression = BI_RGB;

	mcr_DibSize.X = mcr_DibSize.Y = nPreviewSize;

	_ASSERTE(mp_DibBytes==NULL);
	mp_DibBytes = NULL; mn_DibBytes = 0;
	mh_DibSection = CreateDIBSection(mh_CompDC, (BITMAPINFO*)&bmi, DIB_RGB_COLORS, (void**)&mp_DibBytes, NULL, 0);
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
	for (i=0; i<countof(CacheInfo); i++) {
		if (CacheInfo[i].lpwszFileName) {
			free(CacheInfo[i].lpwszFileName);
			CacheInfo[i].lpwszFileName = NULL;
		}
		if (CacheInfo[i].Pixels) {
			free(CacheInfo[i].Pixels);
			CacheInfo[i].Pixels = NULL;
			CacheInfo[i].cbPixelsSize = 0;
		}
		if (CacheInfo[i].pszComments) {
			free(CacheInfo[i].pszComments);
			CacheInfo[i].pszComments = NULL;
			CacheInfo[i].wcCommentsSize = 0;
		}
		if (CacheInfo[i].pszInfo) {
			free(CacheInfo[i].pszInfo);
			CacheInfo[i].pszInfo = NULL;
			CacheInfo[i].wcInfoSize = 0;
		}
	}
	memset(CacheInfo, 0, sizeof(CacheInfo));
	//
	if (mh_CompDC || mh_OldBmp) {
		if (mh_CompDC) SelectObject(mh_CompDC, mh_OldBmp);
		mh_OldBmp = NULL;
	}
	if (mh_DibSection) {
		DeleteObject(mh_DibSection); mh_DibSection = NULL;
	}
	mp_DibBytes = NULL;
	if (mh_CompDC) {
		DeleteDC(mh_CompDC); mh_CompDC = NULL;
	}
	nPreviewSize = 0;
};
void CImgCache::Init(COLORREF acrBack)
{
	// Инициализация (или сброс если изменились размеры превьюшек)
	//hWhiteBrush = ahWhiteBrush; 
	_ASSERTE(gThSet.Thumbs.nImgSize>=16 && gThSet.Tiles.nImgSize>=16);

	WARNING("Не предусмотрен размер gThSet.Tiles.nImgSize?");

	int nMaxSize = max(gThSet.Thumbs.nImgSize,gThSet.Tiles.nImgSize);

	// Не будем при смене фона дергаться, а то на Fade проблемы...
	if (nPreviewSize != nMaxSize /*|| crBackground != gThSet.crBackground*/) {
		Reset();
		nPreviewSize = nMaxSize;
		crBackground = acrBack; //gThSet.crBackground; -- gThSet.crBackground пока не инициализируется
		hbrBack = CreateSolidBrush(acrBack);
	}

	//// Как центрируется иконка
	//nXIcon = GetSystemMetrics(SM_CXICON);
	//nYIcon = GetSystemMetrics(SM_CYICON);
	//nXIconSpace = (nPreviewSize - nXIcon) >> 1;
	//nYIconSpace = (nPreviewSize - nYIcon) >> 1;
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
		pszName = L".."; //pItem->FindData.lpwszFileNamePart;

	for (i=0; i<countof(CacheInfo); i++) {
		if (!CacheInfo[i].lpwszFileName) {
			if (nFree == -1) nFree = i;
			continue;
		}

		// Некторые плагины по ShiftF7 переключают режим папка-файл (Reg, Torrent,...)
		if ((pItem->FindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
		     == (CacheInfo[i].dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
			&& lstrcmpi(CacheInfo[i].lpwszFileName, pszName) == 0)
		{
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
		CacheInfo[i].bVirtualItem = pItem->bVirtualItem;
		CacheInfo[i].UserData = pItem->UserData;
	}
	return lbReady;
};
BOOL CImgCache::PaintItem(HDC hdc, int x, int y, int nImgSize, CePluginPanelItem* pItem, BOOL abLoadPreview, LPCWSTR* ppszComments, BOOL* pbIgnoreFileDescription)
{
	int nIndex = -1;
	if (!CheckDibCreated())
		return FALSE;

	if (!pItem || !pItem->pszFullName) {
		#ifdef _DEBUG
		_ASSERTE(pItem!=NULL);
		if (pItem) _ASSERTE(pItem->pszFullName != NULL);
		#endif

		// Очистить
		RECT rc = {x,y,x+nImgSize,y+nImgSize};
		FillRect(hdc, &rc, hbrBack);

		return TRUE;
	}
	
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
	pItem->bPreviewLoaded = CacheInfo[nIndex].bPreviewLoaded;
	
	if (ppszComments)
		*ppszComments = CacheInfo[nIndex].pszComments;
	if (pbIgnoreFileDescription)
		*pbIgnoreFileDescription = CacheInfo[nIndex].bIgnoreFileDescription;
	
	// Скинуть биты в MemDC
	if (CacheInfo[nIndex].cbStride) {
		CopyBits(CacheInfo[nIndex].crSize, (LPBYTE)(CacheInfo[nIndex].Pixels), CacheInfo[nIndex].cbStride,
			mcr_DibSize, mp_DibBytes);
	} else {
		_ASSERTE(CacheInfo[nIndex].cbStride!=0);
	}
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

	if (nImgSize < CacheInfo[nIndex].crSize.X || nImgSize < CacheInfo[nIndex].crSize.Y)
	{
		// Очистить
		RECT rc = {x,y,x+nImgSize,y+nImgSize};
		FillRect(hdc, &rc, hbrBack);

		// Посчитать
		int lWidth = CacheInfo[nIndex].crSize.X;
		int lHeight = CacheInfo[nIndex].crSize.Y;	
		int nCanvasWidth  = nImgSize;
		int nCanvasHeight = nImgSize;
		int nShowWidth, nShowHeight;
		__int64 aSrc = (100 * (__int64) lWidth) / lHeight;
		__int64 aCvs = (100 * (__int64) nCanvasWidth) / nCanvasHeight;
		if (aSrc > aCvs)
		{
			_ASSERTE(lWidth >= (int)nCanvasWidth);
			nShowWidth = nCanvasWidth;
			nShowHeight = (int)((((__int64)lHeight) * nCanvasWidth) / lWidth);
		} else {
			_ASSERTE(lHeight >= (int)nCanvasHeight);
			nShowWidth = (int)((((__int64)lWidth) * nCanvasHeight) / lHeight);
			nShowHeight = nCanvasHeight;
		}
		
		// Со сдвигом
		int nXSpace = (nImgSize - nShowWidth) >> 1;
		int nYSpace = (nImgSize - nShowHeight) >> 1;
		
		WARNING("В MSDN какие-то предупреждения про MultiMonitor... возможно стоит StretchDIBits");
		SetStretchBltMode(hdc, COLORONCOLOR);
		lbWasDraw = StretchBlt(
			hdc, 
			x+nXSpace,y+nYSpace,nShowWidth,nShowHeight,
			mh_CompDC,
			0,0,CacheInfo[nIndex].crSize.X,CacheInfo[nIndex].crSize.Y,
			SRCCOPY);
	}
	else if (CacheInfo[nIndex].bPreviewExists && gThSet.nMaxZoom>100 
		&& (nImgSize >= 2*CacheInfo[nIndex].crSize.X && nImgSize >= 2*CacheInfo[nIndex].crSize.Y))
	{
		// Очистить
		RECT rc = {x,y,x+nImgSize,y+nImgSize};
		FillRect(hdc, &rc, hbrBack);

		// Максимальное увеличение, пока картинка вписывается в область
		int nZ = 2;
		for (UINT i = 300; i <= gThSet.nMaxZoom; i+=100) {
			if (nImgSize >= (nZ+1)*CacheInfo[nIndex].crSize.X && nImgSize >= (nZ+1)*CacheInfo[nIndex].crSize.Y)
				nZ++;
			else
				break;
		}
		int nShowWidth  = nZ*CacheInfo[nIndex].crSize.X;
		int nShowHeight = nZ*CacheInfo[nIndex].crSize.Y;

		// Со сдвигом
		int nXSpace = (nImgSize - nShowWidth) >> 1;
		int nYSpace = (nImgSize - nShowHeight) >> 1;

		WARNING("В MSDN какие-то предупреждения про MultiMonitor... возможно стоит StretchDIBits");
		SetStretchBltMode(hdc, COLORONCOLOR);
		lbWasDraw = StretchBlt(
			hdc, 
			x+nXSpace,y+nYSpace,nShowWidth,nShowHeight,
			mh_CompDC,
			0,0,CacheInfo[nIndex].crSize.X,CacheInfo[nIndex].crSize.Y,
			SRCCOPY);

	}
	else if (nImgSize > CacheInfo[nIndex].crSize.X || nImgSize > CacheInfo[nIndex].crSize.Y)
	{
		// Очистить
		RECT rc = {x,y,x+nImgSize,y+nImgSize};
		FillRect(hdc, &rc, hbrBack);
		// Со сдвигом
		int nXSpace = (nImgSize - CacheInfo[nIndex].crSize.X) >> 1;
		int nYSpace = (nImgSize - CacheInfo[nIndex].crSize.Y) >> 1;
		lbWasDraw = BitBlt(hdc, x+nXSpace,y+nYSpace,
			CacheInfo[nIndex].crSize.X,CacheInfo[nIndex].crSize.Y, mh_CompDC,
			0,0, SRCCOPY);
	}
	else
	{
		lbWasDraw = BitBlt(hdc, x,y,nImgSize,nImgSize, mh_CompDC,
			0,0, SRCCOPY);
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
//		 SAFETRY  {
//			hBmp = LoadThumbnail(pInfo);
//		} SAFECATCH {
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
	RECT rc = {0,0,32,32}; // если иконку удастся загрузить - будет перебито

	if (!CheckDibCreated())
		return FALSE;

	if (!pItem->lpwszFileName) {
		_ASSERTE(pItem->lpwszFileName);
		return FALSE;
	}

	if (pItem->Pixels && pItem->cbPixelsSize) {
		memset(pItem->Pixels, 0, pItem->cbPixelsSize);
	}
	if (pItem->pszComments) {
		pItem->pszComments[0] = 0;
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
			SHGFI_ICON|SHGFI_SHELLICONSIZE|SHGFI_LARGEICON|SHGFI_USEFILEATTRIBUTES|SHGFI_ADDOVERLAYS|SHGFI_ATTRIBUTES);
		if (shRc && sfi.hIcon) {
			hIcon = sfi.hIcon;
		}
		TODO("Обработать (sfi.dwAttributes & SFGAO_SHARE (0x00020000)) - The specified objects are shared.");
	}

	if (hIcon) {
		ICONINFO ii = {0};
		if (GetIconInfo(hIcon, &ii)) {
			BITMAP bi;
			if (ii.hbmColor) {
				if (GetObject(ii.hbmColor, sizeof(bi), &bi)) {
					rc.right = bi.bmWidth;
					rc.bottom = bi.bmHeight ? bi.bmHeight : -bi.bmHeight;
				}
			} else if (ii.hbmMask) {
				if (GetObject(ii.hbmMask, sizeof(bi), &bi)) {
					rc.right = bi.bmWidth;
					rc.bottom = bi.bmHeight ? bi.bmHeight : -bi.bmHeight;
				}
			}
			if (ii.hbmColor) DeleteObject(ii.hbmColor);
			if (ii.hbmMask) DeleteObject(ii.hbmMask);
			if (rc.right > nPreviewSize) rc.right = nPreviewSize;
			if (rc.bottom > nPreviewSize) rc.bottom = nPreviewSize;
		}
	}

	// Очистить
	FillRect(mh_CompDC, &rc, hbrBack);

	if (hIcon) {
		nDrawRC = DrawIconEx(mh_CompDC, 0, 0, hIcon, rc.right, rc.left, 0, NULL, DI_NORMAL);
		if (sfi.hIcon) {
			DestroyIcon(sfi.hIcon);
		}
	} else {
		// Нарисовать стандартную иконку?
	}
	// Commit changes to MemDC (но это может затронуть и ScreenDC?)
	GdiFlush();

	pItem->crSize.X = (SHORT)rc.right; pItem->crSize.Y = (SHORT)rc.bottom;
	pItem->cbStride = ((SHORT)rc.right)*4;
	pItem->nBits = 32;
	pItem->ColorModel = CET_CM_BGR;
	pItem->cbPixelsSize = pItem->cbStride * pItem->crSize.Y;
	pItem->Pixels = (LPDWORD)malloc(pItem->cbPixelsSize);
	if (!pItem->Pixels) {
		_ASSERTE(pItem->Pixels);
		return FALSE;
	}

	_ASSERTE(nPreviewSize == mcr_DibSize.X);
	COORD crSize = {nPreviewSize,pItem->crSize.Y};
	CopyBits(crSize, mp_DibBytes, nPreviewSize*4, pItem->crSize, (LPBYTE)pItem->Pixels);

	return TRUE;
}
BOOL CImgCache::LoadThumbnail(struct tag_CacheInfo* pItem)
{
	if (!CheckDibCreated())
		return FALSE;

	if (!pItem->lpwszFileName) {
		_ASSERTE(pItem->lpwszFileName);
		return FALSE;
	}

	BOOL lbThumbRc = FALSE;
	struct CET_LoadInfo PV = {sizeof(struct CET_LoadInfo)};

	PV.sFileName = pItem->lpwszFileName;
	PV.bVirtualItem = pItem->bVirtualItem;
	PV.ftModified = pItem->ftLastWriteTime;
	PV.nFileSize = pItem->nFileSize;
	PV.crLoadSize.X = PV.crLoadSize.Y = nPreviewSize;
	PV.bTilesMode = FALSE; // TRUE, when pszInfo acquired.
	PV.crBackground = crBackground; // Must be used to fill background.
	_ASSERTE(gThSet.nMaxZoom>0 && gThSet.nMaxZoom < 1000);
	//PV.nMaxZoom = gThSet.nMaxZoom;
	BOOL lbLoadRc;
	
	BOOL lbIgnoreFileDescription = FALSE, lbIgnoreComments = FALSE;
	HANDLE hFile = INVALID_HANDLE_VALUE, hMapping = NULL;
	LPVOID pFileMap = NULL;
	if (!PV.bVirtualItem) {
		hFile = CreateFileW(PV.sFileName, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
		if (hFile != INVALID_HANDLE_VALUE) {
			hMapping = CreateFileMapping(hFile, NULL, PAGE_READONLY, 0, 0, NULL);
			if (hMapping) {
				pFileMap = MapViewOfFile(hMapping, FILE_MAP_READ, 0,0,0);
				PV.pFileData = (const BYTE*)pFileMap;
			}
		}
	} else if (pItem->UserData) {
		// Из некоторых плагином данные можно извлечь...
		if (!IsBadReadPtr((void*)pItem->UserData, sizeof(ImpExPanelItem))) {
			ImpExPanelItem *pImpEx = (ImpExPanelItem*)pItem->UserData;
			if (pImpEx->nMagic == IMPEX_MAGIC && pImpEx->cbSizeOfStruct >= 1024
				&& pImpEx->nBinarySize > 16 && pImpEx->nBinarySize <= 32*1024*1024
				&& pImpEx->pBinaryData)
			{
				if (!IsBadReadPtr(pImpEx->pBinaryData, pImpEx->nBinarySize))
				{
					PV.nFileSize = pImpEx->nBinarySize;
					PV.pFileData = (const BYTE*)pImpEx->pBinaryData;
					// ImpEx показывает в описании размер изображения, получается некрасивое дублирование
					//lbIgnoreFileDescription = TRUE;
					lbIgnoreComments = TRUE;
				}
			}
		}
	}

	if (PV.pFileData)
	{
		for (int i = 0; i < MAX_MODULES; i++) {
			if (Modules[i].hModule == NULL)
				continue;

			PV.pContext = Modules[i].pContext;
			BOOL lbException = FALSE;
			 SAFETRY  {
				lbLoadRc = Modules[i].LoadInfo(&PV);
			} SAFECATCH  {
				lbLoadRc = FALSE; lbException = TRUE;
			}
			if (lbException) {
				if (PV.pFileContext) {
					 SAFETRY  {
						Modules[i].FreeInfo(&PV);
					} SAFECATCH  {
					}
				}
				continue;
			}

			TODO("Потом может быть не только Pixels, например только информация о версии dll");	
			if (!lbLoadRc || ((!PV.Pixels || !PV.cbPixelsSize) && !PV.pszComments)) {
				if (PV.pFileContext) {
					 SAFETRY  {
						Modules[i].FreeInfo(&PV);
					} SAFECATCH  {
					}
				}
				continue;
			}

			if (PV.Pixels && pItem->Pixels && pItem->cbPixelsSize < PV.cbPixelsSize) {
				free(pItem->Pixels); pItem->Pixels = NULL;
			}
			if (PV.Pixels)
			{
				if (!pItem->Pixels)
				{
					pItem->Pixels = (LPDWORD)malloc(PV.cbPixelsSize);
					if (!pItem->Pixels) {
						_ASSERTE(pItem->Pixels);
						 SAFETRY  {
							Modules[i].FreeInfo(&PV);
						} SAFECATCH  {
						}
						continue;
					}
					pItem->cbPixelsSize = PV.cbPixelsSize;
				}
				memmove(pItem->Pixels, PV.Pixels, PV.cbPixelsSize);

				pItem->crSize = PV.crSize; // Предпочтительно, должен совпадать с crLoadSize
				pItem->cbStride = PV.cbStride; // Bytes per line
				pItem->nBits = PV.nBits; // 32 bit required!
				pItem->ColorModel = PV.ColorModel; // One of CET_CM_xxx
			}
			pItem->bPreviewExists = (PV.Pixels!=NULL);
			
			if (PV.pszComments && !lbIgnoreComments) {
				DWORD nLen = (DWORD)max(255,wcslen(PV.pszComments));
				if (pItem->pszComments && pItem->wcCommentsSize <= nLen) {
					free(pItem->pszComments); pItem->pszComments = 0;
				}
				if (!pItem->pszComments) {
					pItem->wcCommentsSize = nLen+1;
					pItem->pszComments = (wchar_t*)malloc(pItem->wcCommentsSize*2);
				}
				if (pItem->pszComments) {
					lstrcpyn(pItem->pszComments, PV.pszComments, nLen);
				}
			} else if (pItem->pszComments) {
				free(pItem->pszComments); pItem->pszComments = NULL; pItem->wcCommentsSize = 0;
			}
			
			pItem->bIgnoreFileDescription = lbIgnoreFileDescription && (pItem->pszComments != NULL);
						
			//if (pItem->pszInfo)
			//	LocalFree(pItem->pszInfo);
			//pItem->pszInfo = PV.pszInfo;

			
			 SAFETRY  {
				Modules[i].FreeInfo(&PV);
			} SAFECATCH  {
			}
			
			lbThumbRc = TRUE;
			break;
		}
	}
	
	if (hFile != INVALID_HANDLE_VALUE) {
		if (hMapping) {
			CloseHandle(hMapping);
		}
		if (pFileMap) {
			UnmapViewOfFile(pFileMap);
		}
		CloseHandle(hFile);
	}

	return lbThumbRc;

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


	// SAFETRY 
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
	// SAFECATCH 
	//{
	//	hbmp = NULL;
	//}

	//SafeRelease(pFile);
	//if (pIdl) { CoTaskMemFree(pIdl); pIdl = NULL; }
	//if (pszSourceFile) { free(pszSourceFile); pszSourceFile = NULL; }
	//SafeRelease(pEI);

	//return hbmp;
}
