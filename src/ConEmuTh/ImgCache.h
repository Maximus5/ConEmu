
#pragma once

//#include <Objbase.h>
//#include <Objidl.h>
#include "Modules/ThumbSDK.h"
#include "QueueProcessor.h"

#define ImgCacheFileName L"ConEmuTh.cache"
#define ImgCacheListName L"#LFN"

extern HICON ghUpIcon;

struct CePluginPanelItem;

//typedef BOOL (WINAPI* AlphaBlend_t)(HDC hdcDest, int xoriginDest, int yoriginDest, int wDest, int hDest, HDC hdcSrc, int xoriginSrc, int yoriginSrc, int wSrc, int hSrc, BLENDFUNCTION ftn);

typedef UINT ImgLoadType;
static const ImgLoadType
	ilt_ShellSmall      = 1, // ShellIcon (16x16)
	ilt_ShellLarge      = 2, // ShellIcon (Large)
	ilt_ShellMask       = (ilt_ShellSmall|ilt_ShellLarge),
	ilt_Thumbnail       = 4, // Thumbnail/preview
	ilt_TypeMask        = (ilt_ShellMask|ilt_Thumbnail),
	ilt_ThumbnailLoaded = 8, // Thumbnail был реально загружен, а не только извлечена информация
	ilt_ThumbnailMask   = (ilt_Thumbnail|ilt_ThumbnailLoaded),
	ilt_None            = 0
;

struct IMAGE_CACHE_INFO
{
	DWORD nAccessTick;
	union
	{
		struct
		{
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
	ImgLoadType PreviewLoaded;  // пытались ли уже загружать превьюшку, и что удалось загрузить
	//BOOL bPreviewExists; // и получилось ли ее загрузить реально, или в кеше только ShellIcon?
	BOOL bIgnoreFileDescription; // ImpEx показывает в описании размер изображения, получается некрасивое дублирование
	//int N,X,Y;
	struct ImageBits
	{
		COORD crSize; // Предпочтительно, должен совпадать с crLoadSize
		DWORD cbStride; // Bytes per line
		DWORD nBits; // 32 bit required!
		// [Out] Next fields MUST be LocalAlloc(LPTR)
		DWORD ColorModel; // One of CET_CM_xxx
		LPDWORD Pixels; // Alpha channel (highest byte) allowed.
		DWORD cbPixelsSize; // size in BYTES
	} Icon, Preview;
	wchar_t *pszComments; // This may be "512 x 232 x 32bpp"
	DWORD wcCommentsSize; // size in WORDS
	//// Module can place here information about original image (dimension, format, etc.)
	//// This must be double zero terminated string
	//wchar_t *pszInfo;
	//DWORD wcInfoSize; // size in WORDS
};

class CImgLoader;

class CImgCache
{
	protected:
		wchar_t ms_CachePath[MAX_PATH] = L"";
		wchar_t ms_LastStoragePath[MAX_WIDE_PATH_LENGTH] = L"";
		int nPreviewSize = 0; // 96x96
		//int nXIcon, nYIcon, nXIconSpace, nYIconSpace;
		COLORREF crBackground;
		HBRUSH hbrBack = NULL;
		// Теперь - собственно поле кеша
#define FIELD_MAX_COUNT 1000
		//#define ITEMS_IN_FIELD 10 // количество в "строке"
		//int nFieldX, nFieldY; // реальное количество в "строке"/"столбце" (не больше ITEMS_IN_FIELD)
		//HDC hField[FIELD_MAX_COUNT]; HBITMAP hFieldBmp[FIELD_MAX_COUNT], hOldBmp[FIELD_MAX_COUNT];
		IMAGE_CACHE_INFO CacheInfo[FIELD_MAX_COUNT] = {};
		HDC mh_LoadDC = NULL, mh_DrawDC = NULL;
		HBITMAP mh_OldLoadBmp = NULL, mh_OldDrawBmp = NULL, mh_LoadDib = NULL, mh_DrawDib = NULL;
		COORD mcr_LoadDibSize, mcr_DrawDibSize;
		LPBYTE  mp_LoadDibBytes = NULL; DWORD mn_LoadDibBytes = 0;
		LPBYTE  mp_DrawDibBytes = NULL; DWORD mn_DrawDibBytes = 0;
		BOOL CheckLoadDibCreated();
		BOOL CheckDrawDibCreated();
		//void UpdateCell(struct IMAGE_CACHE_INFO* pInfo, BOOL abLoadPreview);
		BOOL FindInCache(CePluginPanelItem* pItem, int* pnIndex, ImgLoadType aLoadType);
		void CopyBits(COORD crSrcSize, LPBYTE lpSrc, DWORD nSrcStride, COORD crDstSize, LPBYTE lpDst);

		CImgLoader *mp_ShellLoader = NULL;

#define MAX_MODULES 20
		struct tag_Module
		{
			HMODULE hModule;
			CET_Init_t Init;
			CET_Done_t Done;
			CET_Load_t LoadInfo;
			CET_Free_t FreeInfo;
			CET_Cancel_t Cancel;
			LPVOID pContext;
		} Modules[MAX_MODULES] = {};
		int mn_ModuleCount = 0;
		void LoadModules();
		void FreeModules();
		wchar_t ms_ModulePath[MAX_PATH] = L"", *mpsz_ModuleSlash;
		//// Alpha blending
		//HMODULE mh_MsImg32;
		//AlphaBlend_t fAlphaBlend;

	public:
		CImgCache(HMODULE hSelf);
		~CImgCache(void);
		void SetCacheLocation(LPCWSTR asCachePath);
		void Reset();
		void Init(COLORREF acrBack);
		BOOL RequestItem(CePluginPanelItem* pItem, ImgLoadType aLoadType);
		BOOL PaintItem(HDC hdc, int x, int y, int nImgSize, CePluginPanelItem* pItem, /*BOOL abLoadPreview,*/ LPCWSTR* ppszComments, BOOL* pbIgnoreFileDescription);

	public:
		BOOL LoadThumbnail(struct IMAGE_CACHE_INFO* pItem);
		BOOL LoadShellIcon(struct IMAGE_CACHE_INFO* pItem, BOOL bLargeIcon = TRUE);

	protected:
		bool mb_comInitialized = false;
		IStorage *mp_RootStorage = nullptr, *mp_CurrentStorage = nullptr;
		BOOL LoadPreview();
};

extern CImgCache *gpImgCache;

class CImgLoader : public CQueueProcessor<IMAGE_CACHE_INFO*>
{
	public:
		// Обработка элемента. Функция должна возвращать:
		// S_OK    - Элемент успешно обработан, будет установлен статус eItemReady
		// S_FALSE - ошибка обработки, будет установлен статус eItemFailed
		// FAILED()- статус eItemFailed И нить обработчика будет ЗАВЕРШЕНА
		HRESULT ProcessItem(IMAGE_CACHE_INFO*& pItem, LONG_PTR lParam) override
		{
			if (!gpImgCache)
			{
				return S_FALSE;
			}

			if (lParam == ilt_ShellSmall)
			{
				return gpImgCache->LoadShellIcon(pItem, FALSE) ? S_OK : S_FALSE;
			}
			else if (lParam == ilt_ShellLarge)
			{
				return gpImgCache->LoadShellIcon(pItem, TRUE) ? S_OK : S_FALSE;
			}
			else if (lParam == ilt_Thumbnail)
			{
				return gpImgCache->LoadThumbnail(pItem) ? S_OK : S_FALSE;
			}

			return S_FALSE;
		};

		//// Вызывается при успешном завершении обработки элемента при асинхронной обработке.
		//// Если элемент обработан успешно (Status == eItemReady), вызывается OnItemReady
		//virtual void OnItemReady(IMAGE_CACHE_INFO*& pItem, LONG_PTR lParam)
		//{
		//	return;
		//};
		//// Иначе (Status == eItemFailed) - OnItemFailed
		//virtual void OnItemFailed(IMAGE_CACHE_INFO*& pItem, LONG_PTR lParam)
		//{
		//	return;
		//};
		//// После завершения этих функций ячейка стирается!

		// Если требуется останов всех запросов и выход из нити обработчика
		bool IsTerminationRequested() override
		{
			TODO("Вернуть TRUE при ExitFar");
			return CQueueProcessor<IMAGE_CACHE_INFO*>::IsTerminationRequested();
		};
		// Здесь потомок может выполнить CoInitialize например
		HRESULT OnThreadStarted() override
		{
			if (!mb_comInitialized)
			{
				mb_comInitialized = SUCCEEDED(CoInitializeEx(nullptr, COINIT_MULTITHREADED));
			}
			return S_OK;
		}
		// Здесь потомок может выполнить CoUninitialize например
		void OnThreadStopped() override
		{
			if (mb_comInitialized)
			{
				mb_comInitialized = false;
				CoUninitialize();
			}
			return;
		};
		// Можно переопределить для изменения логики сравнения (используется при поиске)
		bool IsEqual(IMAGE_CACHE_INFO* const& pItem1, LONG_PTR lParam1, IMAGE_CACHE_INFO* const& pItem2, LONG_PTR lParam2) override
		{
			return (pItem1 == pItem2) && (lParam1 == lParam2);
		};
		// Если элемент потерял актуальность - стал НЕ высокоприоритетным
		bool CheckHighPriority(IMAGE_CACHE_INFO* const& pItem) override
		{
			// Перекрыть в потомке и вернуть false, если, например, был запрос
			// для текущей картинки, но пользователь уже улистал с нее на другую
			return true;
		};

	private:
		bool mb_comInitialized = false;
};
