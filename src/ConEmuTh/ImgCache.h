
#pragma once

//#include <Objbase.h>
//#include <Objidl.h>
#include "Modules/ThumbSDK.h"

#define ImgCacheFileName L"ConEmuTh.cache"
#define ImgCacheListName L"#LFN"

struct CePluginPanelItem;

class CImgCache
{
protected:
	wchar_t ms_CachePath[MAX_PATH];
	wchar_t ms_LastStoragePath[32768];
	int nPreviewSize; // 96x96
	//int nXIcon, nYIcon, nXIconSpace, nYIconSpace;
	COLORREF crBackground;
	HBRUSH hbrBack;
	// Теперь - собственно поле кеша
	#define FIELD_MAX_COUNT 1000
	//#define ITEMS_IN_FIELD 10 // количество в "строке"
	//int nFieldX, nFieldY; // реальное количество в "строке"/"столбце" (не больше ITEMS_IN_FIELD)
	//HDC hField[FIELD_MAX_COUNT]; HBITMAP hFieldBmp[FIELD_MAX_COUNT], hOldBmp[FIELD_MAX_COUNT];
	struct tag_CacheInfo {
		DWORD nAccessTick;
		union {
			struct {
				DWORD nFileSizeHigh;
				DWORD nFileSizeLow;
			};
			unsigned __int64 nFileSize;
		};
		FILETIME ftLastWriteTime;
		DWORD dwFileAttributes;
		wchar_t *lpwszFileName;
		BOOL bVirtualItem;
		BOOL bPreviewLoaded;
		//int N,X,Y;
		COORD crSize; // Предпочтительно, должен совпадать с crLoadSize
		DWORD cbStride; // Bytes per line
		DWORD nBits; // 32 bit required!
		// [Out] Next fields MUST be LocalAlloc(LPTR)
		DWORD ColorModel; // One of CET_CM_xxx
		LPDWORD Pixels; // Alpha channel (highest byte) allowed.
		DWORD cbPixelsSize; // size in BYTES
		wchar_t *pszComments; // This may be "512 x 232 x 32bpp"
		DWORD wcCommentsSize; // size in WORDS
		// Module can place here information about original image (dimension, format, etc.)
		// This must be double zero terminated string
		wchar_t *pszInfo;
		DWORD wcInfoSize; // size in WORDS
	} CacheInfo[FIELD_MAX_COUNT];
	HDC mh_CompDC;
	HBITMAP mh_OldBmp, mh_DibSection;
	COORD mcr_DibSize;
	LPBYTE  mp_DibBytes; DWORD mn_DibBytes;
	BOOL CheckDibCreated();
	void UpdateCell(struct tag_CacheInfo* pInfo, BOOL abLoadPreview);
	BOOL LoadThumbnail(struct tag_CacheInfo* pItem);
	BOOL LoadShellIcon(struct tag_CacheInfo* pItem);
	BOOL FindInCache(CePluginPanelItem* pItem, int* pnIndex);
	void CopyBits(COORD crSrcSize, LPBYTE lpSrc, DWORD nSrcStride, COORD crDstSize, LPBYTE lpDst);

	#define MAX_MODULES 20
	struct tag_Module {
		HMODULE hModule;
		CET_Init_t Init;
		CET_Done_t Done;
		CET_Load_t LoadInfo;
		CET_Free_t FreeInfo;
		CET_Cancel_t Cancel;
		LPVOID pContext;
	} Modules[MAX_MODULES];
	int mn_ModuleCount;
	void LoadModules();
	void FreeModules();
	wchar_t ms_ModulePath[MAX_PATH], *mpsz_ModuleSlash;

public:
	CImgCache(HMODULE hSelf);
	~CImgCache(void);
	void SetCacheLocation(LPCWSTR asCachePath);
	void Reset();
	void Init(COLORREF acrBack);
	BOOL PaintItem(HDC hdc, int x, int y, int nImgSize, CePluginPanelItem* pItem, BOOL abLoadPreview, LPCWSTR* ppszComments);

protected:
	BOOL mb_Quit;
	static DWORD WINAPI ShellExtractionThread(LPVOID apArg);
	IStorage *mp_RootStorage, *mp_CurrentStorage;
	BOOL LoadPreview();
};

extern CImgCache *gpImgCache;
