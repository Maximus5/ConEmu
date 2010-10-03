
#pragma once

//#include <Objbase.h>
//#include <Objidl.h>
#include "Modules/ThumbSDK.h"
#include "QueueProcessor.h"

#define ImgCacheFileName L"ConEmuTh.cache"
#define ImgCacheListName L"#LFN"

extern HICON ghUpIcon;

typedef struct tag_CePluginPanelItem CePluginPanelItem;

typedef BOOL (WINAPI* AlphaBlend_t)(HDC hdcDest, int xoriginDest, int yoriginDest, int wDest, int hDest, HDC hdcSrc, int xoriginSrc, int yoriginSrc, int wSrc, int hSrc, BLENDFUNCTION ftn);

struct IMAGE_CACHE_INFO
{
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
	DWORD_PTR UserData;
	BOOL bPreviewLoaded; // пытались ли уже загружать превьюшку
	BOOL bPreviewExists; // и получилось ли ее загрузить реально, или в кеше только ShellIcon?
	BOOL bIgnoreFileDescription; // ImpEx показывает в описании размер изображени€, получаетс€ некрасивое дублирование
	//int N,X,Y;
	COORD crSize; // ѕредпочтительно, должен совпадать с crLoadSize
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
};

class CImgLoader : public CQueueProcessor<IMAGE_CACHE_INFO>
{
};

class CImgCache
{
protected:
	wchar_t ms_CachePath[MAX_PATH];
	wchar_t ms_LastStoragePath[32768];
	int nPreviewSize; // 96x96
	//int nXIcon, nYIcon, nXIconSpace, nYIconSpace;
	COLORREF crBackground;
	HBRUSH hbrBack;
	// “еперь - собственно поле кеша
	#define FIELD_MAX_COUNT 1000
	//#define ITEMS_IN_FIELD 10 // количество в "строке"
	//int nFieldX, nFieldY; // реальное количество в "строке"/"столбце" (не больше ITEMS_IN_FIELD)
	//HDC hField[FIELD_MAX_COUNT]; HBITMAP hFieldBmp[FIELD_MAX_COUNT], hOldBmp[FIELD_MAX_COUNT];
	IMAGE_CACHE_INFO CacheInfo[FIELD_MAX_COUNT];
	HDC mh_CompDC;
	HBITMAP mh_OldBmp, mh_DibSection;
	COORD mcr_DibSize;
	LPBYTE  mp_DibBytes; DWORD mn_DibBytes;
	BOOL CheckDibCreated();
	void UpdateCell(struct IMAGE_CACHE_INFO* pInfo, BOOL abLoadPreview);
	BOOL LoadThumbnail(struct IMAGE_CACHE_INFO* pItem);
	BOOL LoadShellIcon(struct IMAGE_CACHE_INFO* pItem);
	BOOL FindInCache(CePluginPanelItem* pItem, int* pnIndex);
	void CopyBits(COORD crSrcSize, LPBYTE lpSrc, DWORD nSrcStride, COORD crDstSize, LPBYTE lpDst);

	CImgLoader m_ShellLoader;

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
	//// Alpha blending
	//HMODULE mh_MsImg32;
	//AlphaBlend_t fAlphaBlend;

public:
	CImgCache(HMODULE hSelf);
	~CImgCache(void);
	void SetCacheLocation(LPCWSTR asCachePath);
	void Reset();
	void Init(COLORREF acrBack);
	BOOL PaintItem(HDC hdc, int x, int y, int nImgSize, CePluginPanelItem* pItem, BOOL abLoadPreview, LPCWSTR* ppszComments, BOOL* pbIgnoreFileDescription);

protected:
	BOOL mb_Quit;
	static DWORD WINAPI ShellExtractionThread(LPVOID apArg);
	IStorage *mp_RootStorage, *mp_CurrentStorage;
	BOOL LoadPreview();
};

extern CImgCache *gpImgCache;
