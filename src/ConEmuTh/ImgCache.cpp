
#define SHOWDEBUGSTR

#include "../common/Common.h"
#include "../common/CmdLine.h"
#include "../common/MStrDup.h"
#include "ConEmuTh.h"
#include "resource.h"

#include "ImgCache.h"

#ifdef CONEMU_LOG_FILES
	#define DEBUGSTRLOAD(s) if (gpLogLoad) {gpLogLoad->LogString(s);} // DEBUGSTR(s)
	#define DEBUGSTRLOAD2(s) if (gpLogLoad) {gpLogLoad->LogString(s);} // OutputDebugStringW(s)
	#define DEBUGSTRPAINT(s) if (gpLogPaint) {gpLogPaint->LogString(s);} // DEBUGSTR(s)
#else
	#define DEBUGSTRLOAD(s)
	#define DEBUGSTRLOAD2(s)
	#define DEBUGSTRPAINT(s)
#endif

//#pragma comment( lib, "ole32.lib" )


HICON ghUpIcon = NULL;
//ITEMIDLIST DesktopID = {{0}};
//IShellFolder *gpDesktopFolder = NULL;
class MFileLog;
extern MFileLog* gpLogLoad;
extern MFileLog* gpLogPaint;

#ifdef _DEBUG
#undef SafeRelease
template <typename T>
inline void SafeRelease(T *&p)
{
	if (NULL != p)
	{
		p->Release();
		p = NULL;
	}
}
#endif

#define SAFEDELETEOBJECT(o) if ((o)!=NULL) { DeleteObject(o); (o)=NULL; }
#define SAFEDELETEDC(d) if ((d)!=NULL) { DeleteDC(d); (d)=NULL; }

CImgCache::CImgCache(HMODULE hSelf)
{
	mb_Quit = FALSE;
	mp_ShellLoader = NULL;
	ms_CachePath[0] = ms_LastStoragePath[0] = 0;
	//nWidth = nHeight = 0;
	nPreviewSize = 0; //nXIcon = nYIcon = nXIconSpace = nYIconSpace = 0;
	hbrBack = NULL;
	//nFieldX = nFieldY = 0;
	//memset(hField,0,sizeof(hField));
	//memset(hFieldBmp,0,sizeof(hFieldBmp));
	//memset(hOldBmp,0,sizeof(hOldBmp));
	memset(CacheInfo,0,sizeof(CacheInfo));
	mh_LoadDC = mh_DrawDC = NULL;
	mh_OldLoadBmp = mh_OldDrawBmp = mh_LoadDib = mh_DrawDib = NULL;
	mp_LoadDibBytes = NULL; mn_LoadDibBytes = 0;
	mp_DrawDibBytes = NULL; mn_DrawDibBytes = 0;
	memset(Modules,0,sizeof(Modules));
	mn_ModuleCount = 0;
	mpsz_ModuleSlash = ms_ModulePath;

	if (GetModuleFileName(hSelf, ms_ModulePath, countof(ms_ModulePath)))
	{
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
	//// Alpha blending
	//mh_MsImg32 = LoadLibrary(L"Msimg32.dll");
	//if (mh_MsImg32) {
	//	fAlphaBlend = (AlphaBlend_t)GetProcAddress(mh_MsImg32, "AlphaBlend");
	//} else {
	//	fAlphaBlend = NULL;
	//}
}

CImgCache::~CImgCache(void)
{
	HRESULT hr;
	Reset();
	FreeModules();

	if (hbrBack)
	{
		DeleteObject(hbrBack); hbrBack = NULL;
	}

	if (mp_CurrentStorage)
	{
		hr = mp_CurrentStorage->Commit(STGC_DEFAULT);
		_ASSERTE(SUCCEEDED(hr));
		SafeRelease(mp_CurrentStorage);
	}

	if (mp_RootStorage)
	{
		hr = mp_RootStorage->Commit(STGC_DEFAULT);
		_ASSERTE(SUCCEEDED(hr));
		SafeRelease(mp_RootStorage);
	}

	////
	//SafeRelease(gpDesktopFolder);

	//if (mh_MsImg32) {
	//	fAlphaBlend = NULL;
	//	FreeLibrary(mh_MsImg32);
	//}

	if (mp_ShellLoader)
	{
		delete mp_ShellLoader;
		mp_ShellLoader = NULL;
	}

	// Done Com
	CoUninitialize();
}

void CImgCache::LoadModules()
{
	if (mn_ModuleCount) return;  // Уже загружали!

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

	do
	{
		if ((fnd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
			continue;

		lstrcpy(mpsz_ModuleSlash, fnd.cFileName);
		HMODULE hLib = NULL;
		SAFETRY
		{
			hLib = LoadLibrary(ms_ModulePath);
		} SAFECATCH
		{
			hLib = NULL;
		}

		if (hLib)
		{
			CET_Init_t Init = (CET_Init_t)GetProcAddress(hLib, "CET_Init");
			CET_Done_t Done = (CET_Done_t)GetProcAddress(hLib, "CET_Done");
			CET_Load_t LoadInfo = (CET_Load_t)GetProcAddress(hLib, "CET_Load");
			CET_Free_t FreeInfo = (CET_Free_t)GetProcAddress(hLib, "CET_Free");
			CET_Cancel_t Cancel = (CET_Cancel_t)GetProcAddress(hLib, "CET_Cancel");

			if (!Init || !Done || !LoadInfo || !FreeInfo)
			{
				SAFETRY
				{
					FreeLibrary(hLib);
				} SAFECATCH
				{
				}
				continue;
			}

			struct CET_Init InitArg = {sizeof(struct CET_Init)};

			InitArg.hModule = hLib;

			BOOL lbInitRc;

			SAFETRY
			{
				lbInitRc = Init(&InitArg);
			} SAFECATCH
			{
				lbInitRc = FALSE;
			}

			if (!lbInitRc)
			{
				SAFETRY
				{
					FreeLibrary(hLib);
				} SAFECATCH
				{
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
	}
	while(mn_ModuleCount<MAX_MODULES && FindNextFile(hFind, &fnd));

	FindClose(hFind);
}

void CImgCache::FreeModules()
{
	for(int i = 0; i < MAX_MODULES; i++)
	{
		if (Modules[i].hModule)
		{
			struct CET_Init InitArg = {sizeof(struct CET_Init)};
			InitArg.hModule = Modules[i].hModule;
			InitArg.pContext = Modules[i].pContext;
			SAFETRY
			{
				Modules[i].Done(&InitArg);
			} SAFECATCH
			{
			}
			SAFETRY
			{
				FreeLibrary(Modules[i].hModule);
			} SAFECATCH
			{
			}
			Modules[i].hModule = NULL;
		}
	}
}

void CImgCache::SetCacheLocation(LPCWSTR asCachePath)
{
	// Prepare root storage file pathname
	if (asCachePath)
	{
		lstrcpyn(ms_CachePath, asCachePath, MAX_PATH-32); //-V112
	}
	else
	{
		GetTempPath(MAX_PATH-32, ms_CachePath); //-V112
	}

	// add end slash
	int nLen = lstrlen(ms_CachePath);

	if (nLen && ms_CachePath[nLen-1] != L'\\')
	{
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

BOOL CImgCache::CheckLoadDibCreated()
{
	if (!nPreviewSize)
	{
		_ASSERTE(nPreviewSize != 0);
		return FALSE;
	}

	if (mh_LoadDib)
	{
		return TRUE;
	}

	HDC hScreen = GetDC(NULL);

	if (mh_LoadDC != NULL)
	{
		_ASSERTE(mh_LoadDC==NULL);
	}
	else
	{
		mh_LoadDC = CreateCompatibleDC(hScreen);
	}

	BITMAPINFOHEADER bmi = {sizeof(BITMAPINFOHEADER)};
	bmi.biWidth = nPreviewSize;
	bmi.biHeight = -nPreviewSize; // Top-Down DIB
	bmi.biPlanes = 1;
	bmi.biBitCount = 32; //-V112
	bmi.biCompression = BI_RGB;
	mcr_LoadDibSize.X = mcr_LoadDibSize.Y = nPreviewSize;
	_ASSERTE(mp_LoadDibBytes==NULL);
	mp_LoadDibBytes = NULL; mn_LoadDibBytes = 0;
	mh_LoadDib = CreateDIBSection(hScreen, (BITMAPINFO*)&bmi, DIB_RGB_COLORS, (void**)&mp_LoadDibBytes, NULL, 0);
	ReleaseDC(NULL, hScreen); hScreen = NULL;

	if (!mh_LoadDib)
	{
		_ASSERTE(mh_LoadDib);
		mp_LoadDibBytes = NULL;
		DeleteDC(mh_LoadDC); mh_LoadDC = NULL;
		return FALSE;
	}

	_ASSERTE(mp_LoadDibBytes);
	mn_LoadDibBytes = bmi.biWidth * bmi.biHeight * sizeof(COLORREF);
	//	OK
	mh_OldLoadBmp = (HBITMAP)SelectObject(mh_LoadDC, mh_LoadDib);
	return TRUE;
}

BOOL CImgCache::CheckDrawDibCreated()
{
	if (!nPreviewSize)
	{
		_ASSERTE(nPreviewSize != 0);
		return FALSE;
	}

	if (mh_DrawDib)
	{
		return TRUE;
	}

	HDC hScreen = GetDC(NULL);

	if (mh_DrawDC != NULL)
	{
		_ASSERTE(mh_DrawDC==NULL);
	}
	else
	{
		mh_DrawDC = CreateCompatibleDC(hScreen);
	}

	BITMAPINFOHEADER bmi = {sizeof(BITMAPINFOHEADER)};
	bmi.biWidth = nPreviewSize;
	bmi.biHeight = -nPreviewSize; // Top-Down DIB
	bmi.biPlanes = 1;
	bmi.biBitCount = 32; //-V112
	bmi.biCompression = BI_RGB;
	mcr_DrawDibSize.X = mcr_DrawDibSize.Y = nPreviewSize;
	_ASSERTE(mp_DrawDibBytes==NULL);
	mp_DrawDibBytes = NULL; mn_DrawDibBytes = 0;
	mh_DrawDib = CreateDIBSection(hScreen, (BITMAPINFO*)&bmi, DIB_RGB_COLORS, (void**)&mp_DrawDibBytes, NULL, 0);
	ReleaseDC(NULL, hScreen); hScreen = NULL;

	if (!mh_DrawDib)
	{
		_ASSERTE(mh_DrawDib);
		mp_DrawDibBytes = NULL;
		DeleteDC(mh_DrawDC); mh_DrawDC = NULL;
		return FALSE;
	}

	_ASSERTE(mp_DrawDibBytes);
	mn_DrawDibBytes = bmi.biWidth * bmi.biHeight * sizeof(COLORREF);
	//	OK
	mh_OldDrawBmp = (HBITMAP)SelectObject(mh_DrawDC, mh_DrawDib);
	return TRUE;
}

void CImgCache::Reset()
{
	// Сначала остановим фоновые декодеры
	if (mp_ShellLoader)
		mp_ShellLoader->Terminate();

	int i;

	//for (i=0; i<FIELD_MAX_COUNT; i++) {
	//	if (hField[i]) {
	//		if (hOldBmp[i]) { SelectObject(hField[i], hOldBmp[i]); hOldBmp[i] = NULL; }
	//		if (hFieldBmp[i]) { DeleteObject(hFieldBmp[i]); hFieldBmp[i] = NULL; }
	//		DeleteDC(hField[i]); hField[i] = NULL;
	//	}
	//}
	for(i=0; i<countof(CacheInfo); i++)
	{
		if (CacheInfo[i].lpwszFileName)
		{
			free(CacheInfo[i].lpwszFileName);
			CacheInfo[i].lpwszFileName = NULL;
		}

		if (CacheInfo[i].Icon.Pixels)
		{
			free(CacheInfo[i].Icon.Pixels);
			CacheInfo[i].Icon.Pixels = NULL;
			CacheInfo[i].Icon.cbPixelsSize = 0;
		}

		if (CacheInfo[i].Preview.Pixels)
		{
			free(CacheInfo[i].Preview.Pixels);
			CacheInfo[i].Preview.Pixels = NULL;
			CacheInfo[i].Preview.cbPixelsSize = 0;
		}

		if (CacheInfo[i].pszComments)
		{
			free(CacheInfo[i].pszComments);
			CacheInfo[i].pszComments = NULL;
			CacheInfo[i].wcCommentsSize = 0;
		}

		//if (CacheInfo[i].pszInfo)
		//{
		//	free(CacheInfo[i].pszInfo);
		//	CacheInfo[i].pszInfo = NULL;
		//	CacheInfo[i].wcInfoSize = 0;
		//}
	}

	memset(CacheInfo, 0, sizeof(CacheInfo));

	//
	if (mh_LoadDC || mh_OldLoadBmp)
	{
		if (mh_LoadDC) SelectObject(mh_LoadDC, mh_OldLoadBmp);

		mh_OldLoadBmp = NULL;
	}

	if (mh_DrawDC || mh_OldDrawBmp)
	{
		if (mh_DrawDC) SelectObject(mh_DrawDC, mh_OldDrawBmp);

		mh_OldDrawBmp = NULL;
	}

	if (mh_LoadDC)
	{
		DeleteDC(mh_LoadDC); mh_LoadDC = NULL;
	}

	if (mh_DrawDC)
	{
		DeleteDC(mh_DrawDC); mh_DrawDC = NULL;
	}

	if (mh_LoadDib)
	{
		DeleteObject(mh_LoadDib); mh_LoadDib = NULL;
	}

	mp_LoadDibBytes = NULL;

	if (mh_DrawDib)
	{
		DeleteObject(mh_DrawDib); mh_DrawDib = NULL;
	}

	mp_DrawDibBytes = NULL;
	nPreviewSize = 0;
};
void CImgCache::Init(COLORREF acrBack)
{
	// Инициализация (или сброс если изменились размеры превьюшек)
	//hWhiteBrush = ahWhiteBrush;
	_ASSERTE(gThSet.Thumbs.nImgSize>=16 && gThSet.Tiles.nImgSize>=16);
	WARNING("Не предусмотрен размер gThSet.Tiles.nImgSize?");
	int nMaxSize = std::max(gThSet.Thumbs.nImgSize,gThSet.Tiles.nImgSize);

	// Не будем при смене фона дергаться, а то на Fade проблемы...
	if (nPreviewSize != nMaxSize /*|| crBackground != gThSet.crBackground*/)
	{
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
BOOL CImgCache::FindInCache(CePluginPanelItem* pItem, int* pnIndex, ImgLoadType aLoadType)
{
	_ASSERTE(pItem && pnIndex);
	BOOL lbReady = FALSE, lbFound = FALSE;
	DWORD nCurTick = GetTickCount();
	DWORD nMaxDelta = 0, nDelta;
	int nFree = -1, nOldest = -1, i, nIndex = -1;
	const wchar_t *pszName = pItem->pszFullName;

	if ((pItem->FindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
	        && pItem->FindData.lpwszFileNamePart[0] == L'.'
	        && pItem->FindData.lpwszFileNamePart[1] == L'.'
	        && pItem->FindData.lpwszFileNamePart[2] == 0)
	{
		// pszFullName содержит и полный путь к папке, а для удобства оставить только ".."
		pszName = L".."; //pItem->FindData.lpwszFileNamePart;
	}

	for(i=0; i<countof(CacheInfo); i++)
	{
		if (!CacheInfo[i].lpwszFileName)
		{
			if (nFree == -1) nFree = i;

			continue;
		}

		// Некоторые плагины по ShiftF7 переключают режим папка-файл (Reg, Torrent,...)
		if ((pItem->FindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
		        == (CacheInfo[i].dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
		        && lstrcmpi(CacheInfo[i].lpwszFileName, pszName) == 0)
		{
			// Наш.
			*pnIndex = nIndex = i; lbFound = TRUE;
			TODO("Пока будем игнорировать изменение размера/даты - сильное мелькание для изменяющихся файлов(.log)");
			//т.е. если на панели открыта папка, содержащая пишущийся log-файл - иконка
			//этого файла дико моргает - постоянно сбрасывается и обновляется заново
			lbReady = TRUE;
			//if (CacheInfo[i].nFileSize == pItem->FindData.nFileSize
			//	&& CacheInfo[i].ftLastWriteTime.dwHighDateTime == pItem->FindData.ftLastWriteTime.dwHighDateTime
			//	&& CacheInfo[i].ftLastWriteTime.dwLowDateTime == pItem->FindData.ftLastWriteTime.dwLowDateTime)
			//{
			//	lbReady = TRUE;
			//	CacheInfo[i].dwFileAttributes = pItem->FindData.dwFileAttributes;
			//} // иначе потребуется обновление превьюшки (файл изменился)
			break;
		}

		nDelta = nCurTick - CacheInfo[i].nAccessTick;

		if (nDelta > nMaxDelta)
		{
			nOldest = i; nMaxDelta = nDelta;
		}
	}

	if (!lbFound)
	{
		if (nFree != -1)
		{
			*pnIndex = nIndex = nFree;
		}
		else
		{
			_ASSERTE(nOldest!=-1);

			if (nOldest == -1) nOldest = 0;

			if (CacheInfo[nOldest].lpwszFileName)
			{
				WARNING("В этом месте мы можем теоретически подраться с mp_ShellLoader");
				free(CacheInfo[nOldest].lpwszFileName);
				CacheInfo[nOldest].lpwszFileName = NULL;
			}

			*pnIndex = nIndex = nOldest;
		}
	}

	i = nIndex;

	if (CacheInfo[i].lpwszFileName == NULL)
	{
		CacheInfo[i].lpwszFileName = lstrdup(pszName);
		lbReady = FALSE;
	}

	if (lbReady)
	{
		if (aLoadType & ilt_Thumbnail)
		{
			if (!(CacheInfo[i].PreviewLoaded & ilt_Thumbnail))
				lbReady = FALSE;
		}
		else
		{
			if (CacheInfo[i].PreviewLoaded == ilt_None)
				lbReady = FALSE;
		}
	}

	if (!lbReady)
	{
		if (!lbFound)
		{
			TODO("Если потребуется обновление превьюшки/иконки - нужно таки сбрасывать соответствующие биты");
			CacheInfo[i].PreviewLoaded = ilt_None;
		}

		//CacheInfo[i].bPreviewExists = FALSE;
		CacheInfo[i].nAccessTick = GetTickCount();
		CacheInfo[i].dwFileAttributes = pItem->FindData.dwFileAttributes;
		CacheInfo[i].nFileSize = pItem->FindData.nFileSize;
		CacheInfo[i].ftLastWriteTime = pItem->FindData.ftLastWriteTime;
		CacheInfo[i].bVirtualItem = pItem->bVirtualItem;
		CacheInfo[i].UserData = pItem->UserData;
	}

	return lbReady;
};
BOOL CImgCache::RequestItem(CePluginPanelItem* pItem, ImgLoadType aLoadType)
{
	int nIndex = -1;
	//if (!CheckDibCreated())
	//	return FALSE;

	if (!pItem || !pItem->pszFullName)
		return FALSE;

	// Если отсутствует - добавляет
	// Если в прошлый раз загрузили только ShellIcon и сейчас просят Preview - вернет FALSE
	// Возвращает TRUE только если картинка уже готова
	BOOL lbReady = FindInCache(pItem, &nIndex, aLoadType);
	_ASSERTE(nIndex>=0 && nIndex<countof(CacheInfo));

	// Если нужно загрузить иконку или превьюшку (или обновить их)
	if (!lbReady && (nIndex  >= 0))
	{
		//UpdateCell(CacheInfo+nIndex, aLoadType);
		if (!mp_ShellLoader)
			mp_ShellLoader = new CImgLoader;

#ifdef _DEBUG
		wchar_t szDbg[MAX_PATH+32];
		lstrcpy(szDbg, (aLoadType&ilt_Thumbnail) ? L"ReqThumb: " : L"ReqShell: ");
		lstrcpyn(szDbg+10, pItem->FindData.lpwszFileName, MAX_PATH);
		DEBUGSTRPAINT(szDbg);
#endif
		mp_ShellLoader->RequestItem(false,
			(aLoadType&ilt_Thumbnail) ? ePriorityNormal : ePriorityAboveNormal,
			CacheInfo+nIndex,
			(aLoadType & ilt_TypeMask));
	}

	return lbReady;
}
BOOL CImgCache::PaintItem(HDC hdc, int x, int y, int nImgSize, CePluginPanelItem* pItem, /*BOOL abLoadPreview,*/ LPCWSTR* ppszComments, BOOL* pbIgnoreFileDescription)
{
	int nIndex = -1;

	if (!CheckDrawDibCreated())
		return FALSE;

	if (!pItem || !pItem->pszFullName)
	{
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
	BOOL lbReady = FindInCache(pItem, &nIndex, FALSE/*вернуть что есть*/);
	// -- // Если нужно загрузить иконку или превьюшку (или обновить их)
	//if (!lbReady)
	//{
	//	UpdateCell(CacheInfo+nIndex, abLoadPreview);
	//}
	ImgLoadType PreviewLoaded = CacheInfo[nIndex].PreviewLoaded;
	pItem->PreviewLoaded = PreviewLoaded;
	IMAGE_CACHE_INFO::ImageBits *pImg = NULL;

	if ((PreviewLoaded & ilt_ThumbnailMask) == ilt_ThumbnailMask)
	{
		pImg = &(CacheInfo[nIndex].Preview);

		if (pImg->cbStride == 0 || pImg->Pixels == NULL)
		{
			_ASSERTE(pImg->cbStride && pImg->Pixels);
			pImg = NULL;
		}
	}

	if (!pImg && (PreviewLoaded & ilt_ShellMask))
	{
		pImg = &(CacheInfo[nIndex].Icon);
	}

	if (ppszComments)
		*ppszComments = CacheInfo[nIndex].pszComments;

	if (pbIgnoreFileDescription)
		*pbIgnoreFileDescription = CacheInfo[nIndex].bIgnoreFileDescription;

	// Скинуть биты в MemDC
	if (pImg && pImg->cbStride && pImg->Pixels)
	{
		CopyBits(pImg->crSize, (LPBYTE)(pImg->Pixels), pImg->cbStride,
		         mcr_DrawDibSize, mp_DrawDibBytes);
	}
	else
	{
		//_ASSERTE(CacheInfo[nIndex].cbStride!=0); -- значит еще загрузить не успели
		// Просто очистка на всякий случай
		RECT rc = {x,y,x+nImgSize,y+nImgSize};
		FillRect(hdc, &rc, hbrBack);
		return FALSE;
	}

	/* *** А теперь - собственно отрисовка куда просили *** */

	// nImgSize - размер поля отрисовки (по умолчанию 96 для thumbs, 46 для tiles)

	// Если загруженная превьюшка БОЛЬШЕ поля отрисовки
	if (nImgSize < pImg->crSize.X || nImgSize < pImg->crSize.Y)
	{
		// Очистить
		RECT rc = {x,y,x+nImgSize,y+nImgSize};
		FillRect(hdc, &rc, hbrBack);
		// Посчитать
		int lWidth = pImg->crSize.X;
		int lHeight = pImg->crSize.Y;
		int nCanvasWidth  = nImgSize;
		int nCanvasHeight = nImgSize;
		int nShowWidth, nShowHeight;
		__int64 aSrc = (100 * (__int64) lWidth) / lHeight;
		__int64 aCvs = (100 * (__int64) nCanvasWidth) / nCanvasHeight;

		if (aSrc > aCvs)
		{
			_ASSERTE(lWidth >= (int)nCanvasWidth);
			nShowWidth = nCanvasWidth;
			nShowHeight = (int)((((__int64)lHeight) * nCanvasWidth) / lWidth); //-V537
		}
		else
		{
			_ASSERTE(lHeight >= (int)nCanvasHeight);
			nShowWidth = (int)((((__int64)lWidth) * nCanvasHeight) / lHeight); //-V537
			nShowHeight = nCanvasHeight;
		}

		// Со сдвигом
		int nXSpace = (nImgSize - nShowWidth) >> 1;
		int nYSpace = (nImgSize - nShowHeight) >> 1;
		WARNING("В MSDN какие-то предупреждения про MultiMonitor... возможно стоит StretchDIBits");
		SetStretchBltMode(hdc, COLORONCOLOR);

		if (pImg->ColorModel == CET_CM_BGRA /*&& fAlphaBlend*/)
		{
			BLENDFUNCTION bf = {AC_SRC_OVER, 0, 255, AC_SRC_ALPHA};
			//lbWasDraw = fAlphaBlend(
			lbWasDraw = GdiAlphaBlend(
			                hdc,
			                x+nXSpace,y+nYSpace,nShowWidth,nShowHeight,
			                mh_DrawDC,
			                0,0,pImg->crSize.X,pImg->crSize.Y,
			                bf);
		}
		else
		{
			lbWasDraw = StretchBlt(
			                hdc,
			                x+nXSpace,y+nYSpace,nShowWidth,nShowHeight,
			                mh_DrawDC,
			                0,0,pImg->crSize.X,pImg->crSize.Y,
			                SRCCOPY);
		}
	}
	// Если загруженная превьюшка МЕНЬШЕ поля отрисовки И юзер включил увеличение превьюшки
	// и только если это действительно превьшка (ilt_ThumbnailMask), а не ShellIcon+информация о файле
	else if (((CacheInfo[nIndex].PreviewLoaded & ilt_ThumbnailMask) == ilt_ThumbnailMask) && gThSet.nMaxZoom>100
	        && (nImgSize >= 2*pImg->crSize.X && nImgSize >= 2*pImg->crSize.Y))
	{
		// Очистить
		RECT rc = {x,y,x+nImgSize,y+nImgSize};
		FillRect(hdc, &rc, hbrBack);
		// Максимальное увеличение, пока картинка вписывается в область
		int nZ = 2;

		for(UINT i = 300; i <= gThSet.nMaxZoom; i+=100)
		{
			if (nImgSize >= (nZ+1)*pImg->crSize.X && nImgSize >= (nZ+1)*pImg->crSize.Y)
				nZ++;
			else
				break;
		}

		int nShowWidth  = nZ*pImg->crSize.X;
		int nShowHeight = nZ*pImg->crSize.Y;
		// Со сдвигом
		int nXSpace = (nImgSize - nShowWidth) >> 1;
		int nYSpace = (nImgSize - nShowHeight) >> 1;
		WARNING("В MSDN какие-то предупреждения про MultiMonitor... возможно стоит StretchDIBits");
		SetStretchBltMode(hdc, COLORONCOLOR);

		if (pImg->ColorModel == CET_CM_BGRA /*&& fAlphaBlend*/)
		{
			BLENDFUNCTION bf = {AC_SRC_OVER, 0, 255, AC_SRC_ALPHA};
			//lbWasDraw = fAlphaBlend(
			lbWasDraw = GdiAlphaBlend(
			                hdc,
			                x+nXSpace,y+nYSpace,nShowWidth,nShowHeight,
			                mh_DrawDC,
			                0,0,pImg->crSize.X,pImg->crSize.Y,
			                bf);
		}
		else
		{
			lbWasDraw = StretchBlt(
			                hdc,
			                x+nXSpace,y+nYSpace,nShowWidth,nShowHeight,
			                mh_DrawDC,
			                0,0,pImg->crSize.X,pImg->crSize.Y,
			                SRCCOPY);
		}
	}
	// Превьюшка (или иконка) меньше поля отрисовки
	else if (nImgSize > pImg->crSize.X || nImgSize > pImg->crSize.Y)
	{
		// Очистить
		RECT rc = {x,y,x+nImgSize,y+nImgSize};
		FillRect(hdc, &rc, hbrBack);
		// Со сдвигом
		int nXSpace = (nImgSize - pImg->crSize.X) >> 1;
		int nYSpace = (nImgSize - pImg->crSize.Y) >> 1;
		lbWasDraw = BitBlt(hdc, x+nXSpace,y+nYSpace,
		                   pImg->crSize.X,pImg->crSize.Y, mh_DrawDC,
		                   0,0, SRCCOPY);
	}
	else // размеры совпадают?
	{
		lbWasDraw = BitBlt(hdc, x,y,nImgSize,nImgSize, mh_DrawDC,
		                   0,0, SRCCOPY);
	}

	return lbWasDraw;
}
void CImgCache::CopyBits(COORD crSrcSize, LPBYTE lpSrc, DWORD nSrcStride, COORD crDstSize, LPBYTE lpDst)
{
	int nMaxY = std::min(crDstSize.Y,crSrcSize.Y);
	_ASSERTE(lpSrc);
	_ASSERTE(lpDst);
	_ASSERTE(nSrcStride);
	DWORD nDstStride = crDstSize.X*4;
	DWORD nStride = std::min(nDstStride,nSrcStride);
	_ASSERTE(nStride);

	for(int y = 0; y < nMaxY; y++)
	{
		memmove(lpDst, lpSrc, nStride);
		lpSrc += nSrcStride; //-V102
		lpDst += nDstStride; //-V102
	}
}
//void CImgCache::UpdateCell(struct IMAGE_CACHE_INFO* pInfo, BOOL abLoadPreview)
//{
//	if (pInfo->PreviewLoaded)
//		return; // Уже
//
//	if (abLoadPreview)
//	{
//		// Если хотят превьюшку - сразу поставим флажок, что пытались
//		pInfo->bPreviewLoaded = TRUE;
//
//		// Для папок - самостоятельно
//		if (pInfo->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
//		{
//			return;
//		}
//		else
//		{
//			LoadThumbnail(pInfo);
//		}
//	}
//	else
//	{
//		LoadShellIcon(pInfo);
//	}
//}
BOOL CImgCache::LoadShellIcon(struct IMAGE_CACHE_INFO* pItem, BOOL bLargeIcon /*= TRUE*/)
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

	if (!CheckLoadDibCreated())
		return FALSE;

	if (!pItem->lpwszFileName)
	{
		_ASSERTE(pItem->lpwszFileName);
		return FALSE;
	}

#ifdef _DEBUG
	wchar_t szDbg[MAX_PATH+32];
	lstrcpy(szDbg, L"LoadShell: "); lstrcpyn(szDbg+11, pItem->lpwszFileName, MAX_PATH);
	DEBUGSTRLOAD(szDbg);
#endif

	if (pItem->Icon.Pixels && pItem->Icon.cbPixelsSize)
	{
		memset(pItem->Icon.Pixels, 0, pItem->Icon.cbPixelsSize);
	}

	if (pItem->pszComments)
	{
		pItem->pszComments[0] = 0;
	}

	if (pszName[0] == L'.' && pszName[1] == L'.' && pszName[2] == 0)
	{
		if (!ghUpIcon)
			ghUpIcon = LoadIcon(ghPluginModule, MAKEINTRESOURCE(IDI_UP));

		if (ghUpIcon)
		{
			hIcon = ghUpIcon;
		}
	}
	else
	{
		LPCWSTR pszFileName = pItem->lpwszFileName;
		DWORD uFlags = SHGFI_ICON|(bLargeIcon?(SHGFI_LARGEICON|SHGFI_SHELLICONSIZE):SHGFI_SMALLICON)|SHGFI_USEFILEATTRIBUTES;

		if (pszFileName && pItem->bVirtualItem)
		{
			pszFileName = PointToName(pszFileName);
			if (!pszFileName || !*pszFileName)
				pszFileName = L"File";

			//LPCWSTR pszSlash = wcsrchr(pszFileName, L'\\');
			//if (pszSlash)
			//	pszFileName = pszSlash+1;
			//else
			//	pszFileName = L"";
		}
		else
		{
			uFlags |= SHGFI_ADDOVERLAYS|SHGFI_ATTRIBUTES;
		}

		// ассоциированная иконка
		shRc = SHGetFileInfo(pszFileName, pItem->dwFileAttributes, &sfi, cbSize, uFlags);

		if (shRc && sfi.hIcon)
		{
			hIcon = sfi.hIcon;
		}

		TODO("Обработать (sfi.dwAttributes & SFGAO_SHARE (0x00020000)) - The specified objects are shared.");
	}

	if (hIcon)
	{
		ICONINFO ii = {0};

		if (GetIconInfo(hIcon, &ii))
		{
			BITMAP bi;

			if (ii.hbmColor)
			{
				if (GetObject(ii.hbmColor, sizeof(bi), &bi))
				{
					rc.right = bi.bmWidth;
					rc.bottom = bi.bmHeight ? bi.bmHeight : -bi.bmHeight;
				}
			}
			else if (ii.hbmMask)
			{
				if (GetObject(ii.hbmMask, sizeof(bi), &bi))
				{
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
	FillRect(mh_LoadDC, &rc, hbrBack);

	if (hIcon)
	{
		nDrawRC = DrawIconEx(mh_LoadDC, 0, 0, hIcon, rc.right, rc.left, 0, NULL, DI_NORMAL);

		if (sfi.hIcon)
		{
			DestroyIcon(sfi.hIcon);
		}
	}
	else
	{
		// Нарисовать стандартную иконку?
	}

	// Commit changes to MemDC (но это может затронуть и ScreenDC?)
	GdiFlush();
	pItem->Icon.crSize.X = (SHORT)rc.right; pItem->Icon.crSize.Y = (SHORT)rc.bottom;
	pItem->Icon.cbStride = ((SHORT)rc.right)*4;
	pItem->Icon.nBits = 32; //-V112
	pItem->Icon.ColorModel = CET_CM_BGR;
	pItem->Icon.cbPixelsSize = pItem->Icon.cbStride * pItem->Icon.crSize.Y;
	pItem->Icon.Pixels = (LPDWORD)malloc(pItem->Icon.cbPixelsSize);

	if (!pItem->Icon.Pixels)
	{
		_ASSERTE(pItem->Icon.Pixels);
		DEBUGSTRLOAD2(L"result=FAILED\n");
		return FALSE;
	}

	_ASSERTE(nPreviewSize == mcr_LoadDibSize.X);
	COORD crSize = {nPreviewSize,pItem->Icon.crSize.Y};
	CopyBits(crSize, mp_LoadDibBytes, nPreviewSize*4, pItem->Icon.crSize, (LPBYTE)pItem->Icon.Pixels);
	// ShellIcon загрузили
	pItem->PreviewLoaded |= (bLargeIcon ? ilt_ShellLarge : ilt_ShellSmall);
	TODO("Хорошо бы инвалидатить только соответствующий прямоугольник?");
	CeFullPanelInfo::InvalidateAll();
	DEBUGSTRLOAD2(L"result=OK\n");
	return TRUE;
}
BOOL CImgCache::LoadThumbnail(struct IMAGE_CACHE_INFO* pItem)
{
	if (!CheckLoadDibCreated())
		return FALSE;

	if (!pItem->lpwszFileName)
	{
		_ASSERTE(pItem->lpwszFileName);
		return FALSE;
	}

#ifdef _DEBUG
	wchar_t szDbg[MAX_PATH+32];
	lstrcpy(szDbg, L"LoadThumb: "); lstrcpyn(szDbg+11, pItem->lpwszFileName, MAX_PATH);
	DEBUGSTRLOAD(szDbg);
#endif
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
	LPVOID lptrLocal = NULL;
	BOOL lbIgnoreFileDescription = FALSE, lbIgnoreComments = FALSE;
	HANDLE hFile = INVALID_HANDLE_VALUE, hMapping = NULL;
	LPVOID pFileMap = NULL;

	if (!PV.bVirtualItem)
	{
		WARNING("Переделать на UNC!"); // Не работает на "PictureView.img\Bad Names"
		hFile = CreateFileW(PV.sFileName, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);

		if (hFile == INVALID_HANDLE_VALUE)
		{
			DWORD dwErr = GetLastError();

			if (dwErr == ERROR_SHARING_VIOLATION)
			{
				hFile = CreateFileW(PV.sFileName, GENERIC_READ, FILE_SHARE_READ|FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);
			}
		}

		if (hFile != INVALID_HANDLE_VALUE)
		{
			hMapping = CreateFileMapping(hFile, NULL, PAGE_READONLY, 0, 0, NULL);

			if (hMapping)
			{
				pFileMap = MapViewOfFile(hMapping, FILE_MAP_READ, 0,0,0);
				PV.pFileData = (const BYTE*)pFileMap;
			}
		}
	}
	else if (pItem->UserData)
	{
		// Из некоторых плагином данные можно извлечь...
		// Хм... в фаре IsBadReadPtr иногда зависал. А тут?
		if (!IsBadReadPtr((void*)pItem->UserData, sizeof(ImpExPanelItem)))
		{
			ImpExPanelItem *pImpEx = (ImpExPanelItem*)pItem->UserData;

			if (pImpEx->nMagic == IMPEX_MAGIC && pImpEx->cbSizeOfStruct >= 1024
			        && pImpEx->nBinarySize > 16 && pImpEx->nBinarySize <= 32*1024*1024 //-V112
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

		// PBFar
		//int _GetFileData(CPBPlugin* pPlugin, LPCWSTR asFile, LPVOID* ppData, DWORD* pDataSize)
		if (!PV.pFileData && !IsBadReadPtr((void*)pItem->UserData, sizeof(PbFarPanelItem)))
		{
			PbFarPanelItem *pPbFar = (PbFarPanelItem*)pItem->UserData;

			if (pPbFar->nMagic == PBFAR_MAGIC && pPbFar->cbSizeOfStruct == sizeof(PbFarPanelItem)
			        && pPbFar->pPlugin)
			{
				const wchar_t* pszExt = wcsrchr(pItem->lpwszFileName, L'.');

				if (pszExt)
				{
					if (!lstrcmpi(pszExt, L".bmp") || !lstrcmpi(pszExt, L".gif") || !lstrcmpi(pszExt, L".ico") || !lstrcmpi(pszExt, L".png") || !lstrcmpi(pszExt, L".wmf"))
					{
						HMODULE hPbFar = GetModuleHandle(L"PBFar.dll");

						if (hPbFar)
						{
							GetPbFarFileData_t GetPbFarFileData = (GetPbFarFileData_t)GetProcAddress(hPbFar, "_GetFileData");

							if (GetPbFarFileData)
							{
								DWORD nSize = 0; LPVOID lptr = NULL;

								if (GetPbFarFileData(pPbFar->pPlugin, pItem->lpwszFileName, &lptr, &nSize) && lptr)
								{
									lptrLocal = lptr;
									PV.nFileSize = nSize;
									PV.pFileData = (const BYTE*)lptr;
									lbIgnoreComments = TRUE;
								}
							}
						}
					}
				}
			}
		}
	}

	if (PV.pFileData)
	{
		for(int i = 0; i < MAX_MODULES; i++)
		{
			if (Modules[i].hModule == NULL)
				continue;

			PV.pContext = Modules[i].pContext;
			BOOL lbException = FALSE;
			SAFETRY
			{
				lbLoadRc = Modules[i].LoadInfo(&PV);
			} SAFECATCH
			{
				lbLoadRc = FALSE; lbException = TRUE;
			}

			if (lbException)
			{
				if (PV.pFileContext)
				{
					SAFETRY
					{
						Modules[i].FreeInfo(&PV);
					} SAFECATCH
					{
					}
				}

				continue;
			}

			// Здесь может быть не только Pixels, например только информация о версии dll (в pszComments)
			if (!lbLoadRc || ((!PV.Pixels || !PV.cbPixelsSize) && !PV.pszComments))
			{
				if (PV.pFileContext)
				{
					SAFETRY
					{
						Modules[i].FreeInfo(&PV);
					} SAFECATCH
					{
					}
				}

				continue;
			}

			// Если принятый размер данных (битмап) больше ранее созданного в pItem
			if (PV.Pixels && pItem->Preview.Pixels && pItem->Preview.cbPixelsSize < PV.cbPixelsSize)
			{
				// Освободить и пересоздать
				free(pItem->Preview.Pixels); pItem->Preview.Pixels = NULL;
			}

			if (PV.Pixels)
			{
				if (!pItem->Preview.Pixels)
				{
					pItem->Preview.Pixels = (LPDWORD)malloc(PV.cbPixelsSize);

					if (!pItem->Preview.Pixels)
					{
						_ASSERTE(pItem->Preview.Pixels);
						SAFETRY
						{
							Modules[i].FreeInfo(&PV);
						} SAFECATCH
						{
						}
						continue;
					}

					pItem->Preview.cbPixelsSize = PV.cbPixelsSize;
				}

				_ASSERTE(pItem->Preview.cbPixelsSize >= PV.cbPixelsSize);
				memmove(pItem->Preview.Pixels, PV.Pixels, PV.cbPixelsSize);
				pItem->Preview.crSize = PV.crSize; // Предпочтительно, должен совпадать с crLoadSize
				pItem->Preview.cbStride = PV.cbStride; // Bytes per line
				pItem->Preview.nBits = PV.nBits; // 32 bit required!
				pItem->Preview.ColorModel = PV.ColorModel; // One of CET_CM_xxx
				// Запомнить, что превьюшка действительно есть
				pItem->PreviewLoaded |= ilt_ThumbnailLoaded; //-V112
			}

			//pItem->bPreviewExists = (PV.Pixels!=NULL);

			if (PV.pszComments && !lbIgnoreComments)
			{
				DWORD nLen = (DWORD)std::max(255,lstrlen(PV.pszComments));

				if (pItem->pszComments && pItem->wcCommentsSize <= nLen)
				{
					free(pItem->pszComments); pItem->pszComments = 0;
				}

				if (!pItem->pszComments)
				{
					pItem->wcCommentsSize = nLen+1;
					pItem->pszComments = (wchar_t*)malloc(pItem->wcCommentsSize*2);
				}

				if (pItem->pszComments)
				{
					lstrcpyn(pItem->pszComments, PV.pszComments, nLen);
				}
			}
			else if (pItem->pszComments)
			{
				free(pItem->pszComments); pItem->pszComments = NULL; pItem->wcCommentsSize = 0;
			}

			pItem->bIgnoreFileDescription = lbIgnoreFileDescription && (pItem->pszComments != NULL);
			//if (pItem->pszInfo)
			//	LocalFree(pItem->pszInfo);
			//pItem->pszInfo = PV.pszInfo;
			SAFETRY
			{
				Modules[i].FreeInfo(&PV);
			} SAFECATCH
			{
			}
			lbThumbRc = TRUE;
			break;
		}
	}

	// Поставим флаг того, что превьюшку загружать ПЫТАЛИСЬ
	pItem->PreviewLoaded |= ilt_Thumbnail;
#ifdef _DEBUG
	swprintf_c(szDbg, L"result=%s, preview=%s\n", lbThumbRc ? L"OK" : L"FAILED", (pItem->PreviewLoaded & ilt_ThumbnailLoaded) ? L"Loaded" : L"NOT loaded"); //-V112
	DEBUGSTRLOAD2(szDbg);
#endif

	if (lptrLocal)
	{
		LocalFree(lptrLocal); // PBFAR
	}

	if (hFile != INVALID_HANDLE_VALUE)
	{
		if (hMapping)
		{
			CloseHandle(hMapping);
		}

		if (pFileMap)
		{
			UnmapViewOfFile(pFileMap);
		}

		CloseHandle(hFile);
	}

	if (lbThumbRc)
	{
		TODO("Хорошо бы инвалидатить только соответствующий прямоугольник?");
		CeFullPanelInfo::InvalidateAll();
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
	//wchar_t *pszSourceFile = /*lstrdup(pFileName)*/ (wchar_t*)calloc((nLen+1),2), *pszSlash = NULL;
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
	////	pszThumbs = wsPathBuffer+lstrlen(wsPathBuffer)-1;
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
