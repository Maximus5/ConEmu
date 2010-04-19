
#include <windows.h>
#include <GdiPlus.h>
#include <crtdbg.h>
#include "../ThumbSDK.h"

//#include "../PVD2Helper.h"
//#include "../BltHelper.h"
//#include "../MStream.h"

typedef __int32 i32;
typedef __int64 i64;
typedef unsigned __int8 u8;
typedef unsigned __int16 u16;
typedef DWORD u32;

HMODULE ghModule;

//pvdInitPlugin2 ip = {0};

#define PGE_INVALID_FRAME        0x1001
#define PGE_ERROR_BASE           0x80000000
#define PGE_DLL_NOT_FOUND        0x80001001
#define PGE_EXCEPTION            0x80001002
#define PGE_FILE_NOT_FOUND       0x80001003
#define PGE_NOT_ENOUGH_MEMORY    0x80001004
#define PGE_INVALID_CONTEXT      0x80001005
#define PGE_FUNCTION_NOT_FOUND   0x80001006
#define PGE_WIN32_ERROR          0x80001007
#define PGE_UNKNOWN_COLORSPACE   0x80001008
#define PGE_UNSUPPORTED_PITCH    0x80001009
#define PGE_INVALID_PAGEDATA     0x8000100A
#define PGE_OLD_PICVIEW          0x8000100B
#define PGE_BITBLT_FAILED        0x8000100C


DWORD gnLastWin32Error = 0;

#ifdef _DEBUG
#define DebugString(x) OutputDebugString(x)
#define PaintDebugString(x) //OutputDebugString(x)
#else
#define DebugString(x)
#define PaintDebugString(x)
#endif



#define DllGetFunction(hModule, FunctionName) FunctionName = (FunctionName##_t)GetProcAddress(hModule, #FunctionName)
//#define MALLOC(n) HeapAlloc(GetProcessHeap(), 0, n)
#define CALLOC(n) HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, n)
#define FREE(p) HeapFree(GetProcessHeap(), 0, p)
#define STRING2(x) #x
#define STRING(x) STRING2(x)
#define FILE_LINE __FILE__ "(" STRING(__LINE__) "): "
#ifdef HIDE_TODO
#define TODO(s) 
#define WARNING(s) 
#else
#define TODO(s) __pragma(message (FILE_LINE "TODO: " s))
#define WARNING(s) __pragma(message (FILE_LINE "warning: " s))
#endif
#define PRAGMA_ERROR(s) __pragma(message (FILE_LINE "error: " s))

struct GDIPlusDecoder
{
	HMODULE hGDIPlus;
	ULONG_PTR gdiplusToken; bool bTokenInitialized;
	HRESULT nErrNumber, nLastError;
	//bool bUseICM, bForceSelfDisplay, bCMYK2RGB;
	BOOL bCoInitialized;
	//const wchar_t* pszPluginKey;
	//BOOL bAsDisplay;
	BOOL bCancelled;

	typedef Gdiplus::Status (WINAPI *GdiplusStartup_t)(OUT ULONG_PTR *token, const Gdiplus::GdiplusStartupInput *input, OUT Gdiplus::GdiplusStartupOutput *output);
	typedef VOID (WINAPI *GdiplusShutdown_t)(ULONG_PTR token);
	typedef Gdiplus::GpStatus (WINGDIPAPI *GdipCreateBitmapFromFile_t)(GDIPCONST WCHAR* filename, Gdiplus::GpBitmap **bitmap);
	//typedef Gdiplus::GpStatus (WINGDIPAPI *GdipCreateBitmapFromStream_t)(IStream* stream, Gdiplus::GpBitmap **bitmap);
	//typedef Gdiplus::GpStatus (WINGDIPAPI *GdipCreateBitmapFromFileICM_t)(GDIPCONST WCHAR* filename, Gdiplus::GpBitmap **bitmap);
	//typedef Gdiplus::GpStatus (WINGDIPAPI *GdipCreateBitmapFromStreamICM_t)(IStream* stream, Gdiplus::GpBitmap **bitmap);
	typedef Gdiplus::GpStatus (WINGDIPAPI *GdipGetImageWidth_t)(Gdiplus::GpImage *image, UINT *width);
	typedef Gdiplus::GpStatus (WINGDIPAPI *GdipGetImageHeight_t)(Gdiplus::GpImage *image, UINT *height);
	typedef Gdiplus::GpStatus (WINGDIPAPI *GdipGetImagePixelFormat_t)(Gdiplus::GpImage *image, Gdiplus::PixelFormat *format);
	typedef Gdiplus::GpStatus (WINGDIPAPI *GdipBitmapLockBits_t)(Gdiplus::GpBitmap* bitmap, GDIPCONST Gdiplus::GpRect* rect, UINT flags, Gdiplus::PixelFormat format, Gdiplus::BitmapData* lockedBitmapData);
	typedef Gdiplus::GpStatus (WINGDIPAPI *GdipBitmapUnlockBits_t)(Gdiplus::GpBitmap* bitmap, Gdiplus::BitmapData* lockedBitmapData);
	typedef Gdiplus::GpStatus (WINGDIPAPI *GdipDisposeImage_t)(Gdiplus::GpImage *image);
	typedef Gdiplus::GpStatus (WINGDIPAPI *GdipImageGetFrameCount_t)(Gdiplus::GpImage *image, GDIPCONST GUID* dimensionID, UINT* count);
	typedef Gdiplus::GpStatus (WINGDIPAPI *GdipImageSelectActiveFrame_t)(Gdiplus::GpImage *image, GDIPCONST GUID* dimensionID, UINT frameIndex);
	typedef Gdiplus::GpStatus (WINGDIPAPI *GdipGetPropertyItemSize_t)(Gdiplus::GpImage *image, PROPID propId, UINT* size);
	typedef Gdiplus::GpStatus (WINGDIPAPI *GdipGetPropertyItem_t)(Gdiplus::GpImage *image, PROPID propId, UINT propSize, Gdiplus::PropertyItem* buffer);
	typedef Gdiplus::GpStatus (WINGDIPAPI *GdipGetImageRawFormat_t)(Gdiplus::GpImage *image, OUT GUID* format);
	typedef Gdiplus::GpStatus (WINGDIPAPI *GdipGetImageFlags_t)(Gdiplus::GpImage *image, UINT *flags);
	typedef Gdiplus::GpStatus (WINGDIPAPI *GdipGetImagePalette_t)(Gdiplus::GpImage *image, Gdiplus::ColorPalette *palette, INT size);
	typedef Gdiplus::GpStatus (WINGDIPAPI *GdipGetImagePaletteSize_t)(Gdiplus::GpImage *image, INT *size);
	typedef Gdiplus::GpStatus (WINGDIPAPI *GdipCreateFromHDC_t)(HDC hdc, Gdiplus::GpGraphics **graphics);
	typedef Gdiplus::GpStatus (WINGDIPAPI *GdipDeleteGraphics_t)(Gdiplus::GpGraphics *graphics);
	typedef Gdiplus::GpStatus (WINGDIPAPI *GdipDrawImageRectRectI_t)(Gdiplus::GpGraphics *graphics, Gdiplus::GpImage *image, INT dstx, INT dsty, INT dstwidth, INT dstheight, INT srcx, INT srcy, INT srcwidth, INT srcheight, Gdiplus::GpUnit srcUnit, const Gdiplus::GpImageAttributes* imageAttributes, Gdiplus::DrawImageAbort callback, VOID * callbackData);
	typedef Gdiplus::GpStatus (WINGDIPAPI *GdipCreateBitmapFromScan0_t)(INT width, INT height, INT stride, Gdiplus::PixelFormat format, BYTE* scan0, Gdiplus::GpBitmap** bitmap);
	typedef Gdiplus::GpStatus (WINGDIPAPI *GdipFillRectangleI_t)(Gdiplus::GpGraphics *graphics, Gdiplus::GpBrush *brush, INT x, INT y, INT width, INT height);
	typedef Gdiplus::GpStatus (WINGDIPAPI *GdipCreateSolidFill_t)(Gdiplus::ARGB color, Gdiplus::GpSolidFill **brush);
	typedef Gdiplus::GpStatus (WINGDIPAPI *GdipDeleteBrush_t)(Gdiplus::GpBrush *brush);
	//typedef Gdiplus::GpStatus (WINGDIPAPI *GdipCloneImage_t)(Gdiplus::GpImage *image, Gdiplus::GpImage **cloneImage);
	typedef Gdiplus::GpStatus (WINGDIPAPI *GdipCloneBitmapAreaI_t)(INT x, INT y, INT width, INT height, Gdiplus::PixelFormat format, Gdiplus::GpBitmap *srcBitmap, Gdiplus::GpBitmap **dstBitmap);
	typedef Gdiplus::GpStatus (WINGDIPAPI *GdipSetImagePalette_t)(Gdiplus::GpImage *image, GDIPCONST Gdiplus::ColorPalette *palette);



	GdiplusStartup_t GdiplusStartup;
	GdiplusShutdown_t GdiplusShutdown;
	GdipCreateBitmapFromFile_t GdipCreateBitmapFromFile;
	//GdipCreateBitmapFromStream_t GdipCreateBitmapFromStream;
	//GdipCreateBitmapFromFileICM_t GdipCreateBitmapFromFileICM;
	//GdipCreateBitmapFromStreamICM_t GdipCreateBitmapFromStreamICM;
	GdipGetImageWidth_t GdipGetImageWidth;
	GdipGetImageHeight_t GdipGetImageHeight;
	GdipGetImagePixelFormat_t GdipGetImagePixelFormat;
	GdipBitmapLockBits_t GdipBitmapLockBits;
	GdipBitmapUnlockBits_t GdipBitmapUnlockBits;
	GdipDisposeImage_t GdipDisposeImage;
	GdipImageGetFrameCount_t GdipImageGetFrameCount;
	GdipImageSelectActiveFrame_t GdipImageSelectActiveFrame;
	GdipGetPropertyItemSize_t GdipGetPropertyItemSize;
	GdipGetPropertyItem_t GdipGetPropertyItem;
	GdipGetImageRawFormat_t GdipGetImageRawFormat;
	GdipGetImageFlags_t GdipGetImageFlags;
	GdipGetImagePalette_t GdipGetImagePalette;
	GdipGetImagePaletteSize_t GdipGetImagePaletteSize;
	GdipCreateFromHDC_t GdipCreateFromHDC;
	GdipDeleteGraphics_t GdipDeleteGraphics;
	GdipDrawImageRectRectI_t GdipDrawImageRectRectI;
	GdipCreateBitmapFromScan0_t GdipCreateBitmapFromScan0;
	GdipFillRectangleI_t GdipFillRectangleI;
	GdipCreateSolidFill_t GdipCreateSolidFill;
	GdipDeleteBrush_t GdipDeleteBrush;
	//GdipCloneImage_t GdipCloneImage;
	GdipCloneBitmapAreaI_t GdipCloneBitmapAreaI;
	GdipSetImagePalette_t GdipSetImagePalette;
	
	GDIPlusDecoder() {
		hGDIPlus = NULL; gdiplusToken = NULL; bTokenInitialized = false;
		nErrNumber = 0; nLastError = 0; //bUseICM = false; bCoInitialized = FALSE; bCMYK2RGB = false;
		//pszPluginKey = NULL; 
		bCancelled = FALSE; 
		//bForceSelfDisplay = false;
	}

	//void ReloadConfig()
	//{
	//	PVDSettings set(pszPluginKey);
	//	
	//	bool bDefault = false;
	//	set.GetParam(L"UseICM", L"bool;Use ICM when possible",
	//		REG_BINARY, &bDefault, &bUseICM, sizeof(bUseICM));
	//		
	//	bDefault = false;
	//	set.GetParam(L"ForceSelfDisplay", L"bool;Forced self display",
	//		REG_BINARY, &bDefault, &bForceSelfDisplay, sizeof(bForceSelfDisplay));

	//	bDefault = false;
	//	// Что-то в GDI+ CMYK изображения всегда загружаются сразу как PixelFormat24bppRGB
	//	//set.GetParam(L"CMYK2RGB", L"bool;Convert CMYK to RGB internally", REG_BINARY, &bDefault, &bCMYK2RGB, sizeof(bCMYK2RGB));
	//}

	bool Init(struct CET_Init* pInit)
	{
		bool result = false;
		nErrNumber = 0;

		//pszPluginKey = pInit->pRegKey;

		//ReloadConfig();

		HRESULT hrCoInitialized = CoInitialize(NULL);
		bCoInitialized = SUCCEEDED(hrCoInitialized);

		wchar_t FullPath[MAX_PATH*2+15]; FullPath[0] = 0;
		//if (ghModule)
		//{
		//	PVDSettings::FindFile(L"GdiPlus.dll", FullPath, sizeofarray(FullPath));
		//	hGDIPlus = LoadLibraryW(FullPath);
		//}
		//if (!hGDIPlus)
		hGDIPlus = LoadLibraryW(L"GdiPlus.dll");

		if (!hGDIPlus) {
			nErrNumber = PGE_DLL_NOT_FOUND;

		} else {
			DllGetFunction(hGDIPlus, GdiplusStartup);
			DllGetFunction(hGDIPlus, GdiplusShutdown);
			DllGetFunction(hGDIPlus, GdipCreateBitmapFromFile);
			//DllGetFunction(hGDIPlus, GdipCreateBitmapFromFileICM);
			//DllGetFunction(hGDIPlus, GdipCreateBitmapFromStream);
			//DllGetFunction(hGDIPlus, GdipCreateBitmapFromStreamICM);
			DllGetFunction(hGDIPlus, GdipGetImageWidth);
			DllGetFunction(hGDIPlus, GdipGetImageHeight);
			DllGetFunction(hGDIPlus, GdipGetImagePixelFormat);
			DllGetFunction(hGDIPlus, GdipBitmapLockBits);
			DllGetFunction(hGDIPlus, GdipBitmapUnlockBits);
			DllGetFunction(hGDIPlus, GdipDisposeImage);
			DllGetFunction(hGDIPlus, GdipImageGetFrameCount);
			DllGetFunction(hGDIPlus, GdipImageSelectActiveFrame);
			DllGetFunction(hGDIPlus, GdipGetPropertyItemSize);
			DllGetFunction(hGDIPlus, GdipGetPropertyItem);
			DllGetFunction(hGDIPlus, GdipGetImageRawFormat);
			DllGetFunction(hGDIPlus, GdipGetImageFlags);
			DllGetFunction(hGDIPlus, GdipGetImagePalette);
			DllGetFunction(hGDIPlus, GdipGetImagePaletteSize);
			DllGetFunction(hGDIPlus, GdipCreateFromHDC);
			DllGetFunction(hGDIPlus, GdipDeleteGraphics);
			DllGetFunction(hGDIPlus, GdipDrawImageRectRectI);
			DllGetFunction(hGDIPlus, GdipCreateBitmapFromScan0);
			DllGetFunction(hGDIPlus, GdipFillRectangleI);
			DllGetFunction(hGDIPlus, GdipCreateSolidFill);
			DllGetFunction(hGDIPlus, GdipDeleteBrush);
			DllGetFunction(hGDIPlus, GdipCloneBitmapAreaI);
			DllGetFunction(hGDIPlus, GdipSetImagePalette);

			//if (!GdipCreateBitmapFromFileICM && !GdipCreateBitmapFromStreamICM)
			//	bUseICM = false;

			if (GdiplusStartup && GdiplusShutdown && GdipCreateBitmapFromFile && GdipGetImageWidth && GdipGetImageHeight && GdipGetImagePixelFormat && GdipBitmapLockBits && GdipBitmapUnlockBits && GdipDisposeImage && GdipImageGetFrameCount && GdipImageSelectActiveFrame 
				&& GdipGetPropertyItemSize && GdipGetPropertyItem && GdipGetImageFlags
				&& GdipGetImagePalette && GdipGetImagePaletteSize && GdipCloneBitmapAreaI
				&& GdipCreateFromHDC && GdipDeleteGraphics && GdipDrawImageRectRectI
				&& GdipCreateBitmapFromScan0 && GdipFillRectangleI && GdipCreateSolidFill && GdipDeleteBrush
				&& GdipSetImagePalette)
			{
				__try{
					Gdiplus::GdiplusStartupInput gdiplusStartupInput;
					Gdiplus::Status lRc = GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL);
					result = (lRc == Gdiplus::Ok);
					if (!result) {
						nLastError = GetLastError();
						GdiplusShutdown(gdiplusToken); bTokenInitialized = false;
						nErrNumber = PGE_ERROR_BASE + (DWORD)lRc;
					} else {
						bTokenInitialized = true;
					}
				}__except( EXCEPTION_EXECUTE_HANDLER ){
					//if (ip.MessageLog) ip.MessageLog(ip.pCallbackContext, L"Exception in GdiplusStartup", 2);
					nErrNumber = PGE_EXCEPTION;
				}
			} else {
				nErrNumber = PGE_FUNCTION_NOT_FOUND;
			}
			if (!result)
				FreeLibrary(hGDIPlus);
		}
		if (result)
			pInit->lParam = (LPARAM)this;
		return result;
	};

	void Close()
	{
		__try{
			if (bTokenInitialized) {
				GdiplusShutdown(gdiplusToken);
				bTokenInitialized = false;
			}
		} __except( EXCEPTION_EXECUTE_HANDLER ) {
			//if (ip.MessageLog) ip.MessageLog(ip.pCallbackContext, L"Exception in GdiplusShutdown", 2);
			nErrNumber = PGE_EXCEPTION;
		}
		if (hGDIPlus) {
			FreeLibrary(hGDIPlus);
			hGDIPlus = NULL;
		}

		if (bCoInitialized) {
			bCoInitialized = FALSE;
			CoUninitialize();
		}
	};
};

#define DEFINE_GUID_(name, l, w1, w2, b1, b2, b3, b4, b5, b6, b7, b8) \
	EXTERN_C const GUID DECLSPEC_SELECTANY name \
	= { l, w1, w2, { b1, b2,  b3,  b4,  b5,  b6,  b7,  b8 } }

DEFINE_GUID_(FrameDimensionTime, 0x6aedbd6d,0x3fb5,0x418a,0x83,0xa6,0x7f,0x45,0x22,0x9d,0xc8,0x72);
DEFINE_GUID_(FrameDimensionPage, 0x7462dc86,0x6180,0x4c7e,0x8e,0x3f,0xee,0x73,0x33,0xa7,0xa4,0x83);

struct GDIPlusImage;

struct GDIPlusPage
{
	struct GDIPlusImage *img;
	bool bBitmapData;
	UINT nActivePage;
	Gdiplus::BitmapData  bmd;
};

struct GDIPlusImage
{
#ifdef _DEBUG
	wchar_t szFileName[MAX_PATH];
#endif
	GDIPlusDecoder *gdi;
	Gdiplus::GpBitmap *img;
	//int nRefCount;
	//Gdiplus::PropertyItem *pFrameTimesProp;
	//Gdiplus::PropertyItem *pTransparent;
	//MStream *strm;
	HRESULT nErrNumber, nLastError;
	//
	UINT lWidth, lHeight, pf, nBPP, nPages, /*lFrameTime,*/ nActivePage, nTransparent, nImgFlags;
	bool Animation;
	wchar_t FormatName[0x80];

	//void AddRef() {
	//	_ASSERTE(nRefCount>=0);
	//	nRefCount ++;
	//};

	//struct GDIPlusImage* Clone()
	//{
	//	struct GDIPlusImage* newimg = (struct GDIPlusImage*)CALLOC(sizeof(struct GDIPlusImage));
	//	if (!newimg) {
	//		nErrNumber = PGE_NOT_ENOUGH_MEMORY;
	//		return NULL;
	//	}
//#ifdef _DEBUG
	//	lstrcpyn(newimg->szFileName, szFileName, MAX_PATH);
//#endif
//
	//	Gdiplus::Status lRc = gdi->GdipCloneBitmapAreaI(0,0,lWidth,lHeight, pf, img, &newimg->img);
	//	if (lRc) {
	//		nErrNumber = PGE_ERROR_BASE+(int)lRc;
	//		FREE(newimg);
	//		return NULL;
	//	}
	//
	//	
	//	newimg->gdi = gdi;
	//	//pFrameTimesProp;
	//	//pTransparent;
	//	newimg->lWidth = lWidth;
	//	newimg->lHeight = lHeight;
	//	newimg->pf = pf;
	//	newimg->nBPP = nBPP;
	//	newimg->nPages = 1;
	//	newimg->lFrameTime = lFrameTime;
	//	newimg->nActivePage = 0;
	//	newimg->nTransparent = nTransparent;
	//	newimg->nImgFlags = nImgFlags;
	//	newimg->Animation = Animation;
	//
	//	newimg->AddRef();
	//	return newimg;
	//};

	//Gdiplus::GpBitmap* OpenBitmapFromStream(MStream* strm)
	//{
	//	Gdiplus::Status lRc = Gdiplus::Ok;
	//	Gdiplus::GpBitmap *img = NULL;
	//	__try {
	//		if (gdi->bUseICM) {
	//			if (gdi->GdipCreateBitmapFromStreamICM)
	//				lRc = gdi->GdipCreateBitmapFromStreamICM(strm, &img);
	//			else
	//				gdi->bUseICM = false;
	//		}
	//		if (!gdi->bUseICM || !lRc || !img) {
	//			LARGE_INTEGER dlibMove; dlibMove.QuadPart = 0;
	//			strm->Seek(dlibMove, STREAM_SEEK_SET, NULL);
	//			lRc = gdi->GdipCreateBitmapFromStream(strm, &img);
	//		}
	//		if (!img) {
	//			nLastError = GetLastError();
	//			nErrNumber = PGE_ERROR_BASE + (DWORD)lRc;
	//		}
	//	} __except( EXCEPTION_EXECUTE_HANDLER ) {
	//		if (ip.MessageLog) ip.MessageLog(ip.pCallbackContext, L"Exception in GdipCreateBitmapFromStream", 2);
	//		nErrNumber = PGE_EXCEPTION;
	//	}
	//	return img;
	//}
	Gdiplus::GpBitmap* OpenBitmapFromFile(const wchar_t *pFileName)
	{
		Gdiplus::Status lRc = Gdiplus::Ok;
		Gdiplus::GpBitmap *img = NULL;
		__try {
			//if (gdi->bUseICM) {
			//	if (gdi->GdipCreateBitmapFromFileICM)
			//		lRc = gdi->GdipCreateBitmapFromFileICM(pFileName, &img);
			//	else
			//		gdi->bUseICM = false;
			//}
			//if (!gdi->bUseICM || !lRc || !img) {
			lRc = gdi->GdipCreateBitmapFromFile(pFileName, &img);
			//}
			if (!img) {
				nLastError = GetLastError();
				nErrNumber = PGE_ERROR_BASE + (DWORD)lRc;
			}
		} __except( EXCEPTION_EXECUTE_HANDLER ) {
			//if (ip.MessageLog) ip.MessageLog(ip.pCallbackContext, L"Exception in GdipCreateBitmapFromFile", 2);
			nErrNumber = PGE_EXCEPTION;
		}
		return img;
	}

	//Gdiplus::GpBitmap* OpenBitmap(const wchar_t *pFileName, i64 lFileSize, const u8 *pBuffer, u32 lBuffer)
	//{
	//	_ASSERTE(nRefCount>=0);
	//	Gdiplus::Status lRc = Gdiplus::Ok;
	//	Gdiplus::GpBitmap *img = NULL;
	//
	//	nErrNumber = PGE_FILE_NOT_FOUND;
	//
	//	#ifdef _DEBUG
	//	lstrcpyn(szFileName, pFileName, MAX_PATH);
	//	#endif
	//	
	//	if (gdi->bCoInitialized && pBuffer && lBuffer && (!lFileSize || (lFileSize == (i64)lBuffer))) {
	//		strm = new MStream();
	//		// Если нужно создать копию памяти - делать Write & Seek
	//		// Если выделять память не нужно (использовать ссылку на pBuffer) - SetData
	//		// Сейчас PicView не хранит открытый буфер
	//		nLastError = strm->Write(pBuffer, lBuffer, NULL);
	//		//strm->SetData(pBuffer, lBuffer);
	//		// Если вдруг памяти не хватило - попробуем
	//		if (nLastError == S_OK) {
	//			LARGE_INTEGER ll; ll.QuadPart = 0;
	//			strm->Seek(ll, STREAM_SEEK_SET, NULL);
	//			img = OpenBitmapFromStream(strm);
	//			if (img)
	//				return img;
	//		} else {
	//			nErrNumber = PGE_ERROR_BASE + (DWORD)Gdiplus::Win32Error;
	//		}
	//	}
	//	
	//	if (pFileName && *pFileName) {
	//		img = OpenBitmapFromFile(pFileName);
	//		TODO("Обязательно выставить флажок FILE_REQUIRED");
	//		return img;
	//	}
	//
	//	return NULL;
	//};

	bool Open(const wchar_t *pFileName)
	{
		//_ASSERTE(nRefCount>=0);
		_ASSERTE(img == NULL);
		_ASSERTE(gdi != NULL);

		bool result = false;
		Gdiplus::Status lRc;

		nActivePage = -1; nTransparent = -1; nImgFlags = 0;
		img = OpenBitmapFromFile(pFileName);

		if (!img) {
			//nErrNumber = gdi->nErrNumber; -- ошибка УЖЕ в nErrNumber

		} else {
			lRc = gdi->GdipGetImageWidth(img, &lWidth);
			lRc = gdi->GdipGetImageHeight(img, &lHeight);
			lRc = gdi->GdipGetImagePixelFormat(img, (Gdiplus::PixelFormat*)&pf);
			nBPP = pf >> 8 & 0xFF;

			lRc = gdi->GdipGetImageFlags(img, &nImgFlags);
			
			//if ((pf & PixelFormatIndexed) == PixelFormatIndexed) {
			//	//nTransparent
			//	UINT lPropSize;
			//	if (!(lRc = gdi->GdipGetPropertyItemSize(img, 0x5104/*PropertyTagIndexTransparent*/, &lPropSize)))
			//	{
			//		pTransparent = (Gdiplus::PropertyItem*)CALLOC(lPropSize);
			//		if ((lRc = gdi->GdipGetPropertyItem(img, 0x5104/*PropertyTagIndexTransparent*/, lPropSize, pTransparent)))
			//		{
			//			FREE(pTransparent);
			//			pTransparent = NULL;
			//		}
			//	}
			//}

			Animation = false; nPages = 1;
			if (!(lRc = gdi->GdipImageGetFrameCount(img, &FrameDimensionTime, &nPages)))
				Animation = nPages > 1;
			else
				if ((lRc = gdi->GdipImageGetFrameCount(img, &FrameDimensionPage, &nPages)))
					nPages = 1;

			//pFrameTimesProp = NULL;
			//if (Animation)
			//{
			//	UINT lPropSize;
			//	if (!(lRc = gdi->GdipGetPropertyItemSize(img, PropertyTagFrameDelay, &lPropSize)))
			//		if (pFrameTimesProp = (Gdiplus::PropertyItem*)CALLOC(lPropSize))
			//			if ((lRc = gdi->GdipGetPropertyItem(img, PropertyTagFrameDelay, lPropSize, pFrameTimesProp)))
			//			{
			//				FREE(pFrameTimesProp);
			//				pFrameTimesProp = NULL;
			//			}
			//}

			FormatName[0] = 0;
			if (gdi->GdipGetImageRawFormat)
			{
				const wchar_t Format[][5] = {L"BMP", L"EMF", L"WMF", L"JPEG", L"PNG", L"GIF", L"TIFF", L"EXIF", L"", L"", L"ICO"};
				GUID gformat;
				if (!(lRc = gdi->GdipGetImageRawFormat(img, &gformat))) {
					// DEFINE_GUID(ImageFormatUndefined, 0xb96b3ca9,0x0728,0x11d3,0x9d,0x7b,0x00,0x00,0xf8,0x1e,0xf3,0x2e);
					// DEFINE_GUID(ImageFormatMemoryBMP, 0xb96b3caa,0x0728,0x11d3,0x9d,0x7b,0x00,0x00,0xf8,0x1e,0xf3,0x2e);

					if (gformat.Data1 >= 0xB96B3CAB && gformat.Data1 <= 0xB96B3CB5) {
						lstrcpy(FormatName, Format[gformat.Data1 - 0xB96B3CAB]);
					}
				}
			}

			result = SelectPage(0);
		}
		return result;
	};
	bool SelectPage(UINT iPage)
	{
		/*if (iPage == nActivePage)
			return true;
		if ((int)iPage < 0 || iPage >= nPages)
			return false;*/

		bool result = false;
		Gdiplus::Status lRc;
		__try{
			if ((lRc = gdi->GdipImageGetFrameCount(img, &FrameDimensionTime, &pf)))
				lRc = gdi->GdipImageSelectActiveFrame(img, &FrameDimensionPage, iPage);
			else
				lRc = gdi->GdipImageSelectActiveFrame(img, &FrameDimensionTime, iPage);
			lRc = gdi->GdipGetImageWidth(img, &lWidth);
			lRc = gdi->GdipGetImageHeight(img, &lHeight);
			lRc = gdi->GdipGetImagePixelFormat(img, (Gdiplus::PixelFormat*)&pf);

			nBPP = pf >> 8 & 0xFF;

			////BUGBUG
			//lFrameTime = 0;
			//if (pFrameTimesProp) {
			//	lFrameTime = (pFrameTimesProp->length >= (iPage + 1) * 4)
			//		? (((u32*)pFrameTimesProp->value)[iPage] * 10) : 100;
			//	if (lFrameTime == 0)
			//		lFrameTime = 100;
			//}

			nActivePage = iPage;
			result = true;
		} __except( EXCEPTION_EXECUTE_HANDLER ) {
			//if (ip.MessageLog) ip.MessageLog(ip.pCallbackContext, L"Exception in GdipImageGetFrameCount...", 2);
			nErrNumber = PGE_EXCEPTION;
		}

		return result;
	};
	void Close()
	{
		if (!gdi) return;

		Gdiplus::Status lRc;
		__try{
			//if (pFrameTimesProp) {
			//	//delete[] (u8*)pFrameTimesProp;
			//	FREE(pFrameTimesProp);
			//	pFrameTimesProp = NULL;
			//}
			if (img) {
				lRc = gdi->GdipDisposeImage(img);
				img = NULL;
			}
			//if (strm) {
			//	delete strm;
			//	strm = NULL;
			//}
		} __except( EXCEPTION_EXECUTE_HANDLER ) {
			//if (ip.MessageLog) ip.MessageLog(ip.pCallbackContext, L"Exception in GdipDisposeImage", 2);
			nErrNumber = PGE_EXCEPTION;
		}

		__try {
			FREE(this);
		} __except( EXCEPTION_EXECUTE_HANDLER ) {
			//if (ip.MessageLog) ip.MessageLog(ip.pCallbackContext, L"Exception in FREE(this)", 2);
			nErrNumber = PGE_EXCEPTION;
		}
	};
	static BOOL CALLBACK DrawImageAbortCallback(GDIPlusDecoder *pGDI)
	{
		if (pGDI)
			return pGDI->bCancelled;
		return FALSE;
	};
	bool GetPageBits(CET_LoadPreview *pDecodeInfo)
	{
		bool result = false;
		
		__try{
			//HDC hCompDC = CreateCompatibleDC(p->pstruct.hdc);
			//_ASSERTE(hCompDC);
			//if (hCompDC)
			{
				int nCanvasWidth  = pDecodeInfo->crLoadSize.X;
				int nCanvasWidthS = nCanvasWidth; //((nCanvasWidth+7) >> 3) << 3; // try to align x8 pixels
				int nCanvasHeight = pDecodeInfo->crLoadSize.Y;
				int nShowWidth, nShowHeight;
				float fAspectX = (float)lWidth / (float)nCanvasWidth;
				float fAspectY = (float)lHeight / (float)nCanvasHeight;
				if (fAspectX > fAspectY) {
					nShowWidth = nCanvasWidth;
					nShowHeight = (int)(lHeight / fAspectX);
				} else {
					nShowWidth = (int)(lWidth / fAspectY);
					nShowHeight = nCanvasHeight;
				}
				//HBITMAP hCompBMP = (HBITMAP)CreateCompatibleBitmap(p->pstruct.hdc, nShowWidth, nShowHeight);
				//HBITMAP hCompBMP = CreateDIBSection(pDecodeInfo->crLoadSize.X, pDecodeInfo->crLoadSize.Y);
				//HBITMAP hOldBMP = (HBITMAP)SelectObject(hCompDC, hCompBMP);
				
				HDC hCompDc1 = CreateCompatibleDC(NULL);

				BITMAPINFOHEADER bmi = {sizeof(BITMAPINFOHEADER)};
				bmi.biWidth = nCanvasWidthS;
				bmi.biHeight = nCanvasHeight;
				bmi.biPlanes = 1;
				bmi.biBitCount = 32;
				bmi.biCompression = BI_RGB;

				LPBYTE pBits = NULL;
				HBITMAP hDIB = CreateDIBSection(hCompDc1, (BITMAPINFO*)&bmi, DIB_RGB_COLORS, (void**)&pBits, NULL, 0);
				_ASSERTE(hDIB);

				HBITMAP hOld1 = (HBITMAP)SelectObject(hCompDc1, hDIB);
				
				RECT rcFull = {0,0,nCanvasWidthS, nCanvasHeight};
				HBRUSH hBr = CreateSolidBrush(pDecodeInfo->crBackground);
				FillRect(hCompDc1, &rcFull, hBr);

				Gdiplus::GpGraphics *pGr = NULL;
				Gdiplus::Status stat = gdi->GdipCreateFromHDC(hCompDc1, &pGr);
				if (!stat) {
					stat = gdi->GdipDrawImageRectRectI(
						pGr, img,
						nCanvasWidth-nShowWidth, nCanvasHeight-nShowHeight, nShowWidth, nShowHeight,
						0,0,lWidth,lHeight,
						Gdiplus::UnitPixel, NULL, //NULL, NULL);
						(Gdiplus::DrawImageAbort)DrawImageAbortCallback, gdi);
					gdi->GdipDeleteGraphics(pGr);
				}
		
				if (stat) {
					pDecodeInfo->nErrNumber = PGE_BITBLT_FAILED;
				} else {
					pDecodeInfo->crSize.X = nCanvasWidth; pDecodeInfo->crSize.Y = nCanvasHeight;
					pDecodeInfo->cbStride = nCanvasWidthS * 4;
					pDecodeInfo->nBits = 32;
					pDecodeInfo->ColorModel = CET_CM_BGR;
					DWORD nMemSize = pDecodeInfo->cbStride * nCanvasHeight;
					pDecodeInfo->Pixels = (LPDWORD)LocalAlloc(LMEM_FIXED, nMemSize);
					memmove(pDecodeInfo->Pixels, pBits, nMemSize);
				}

				if (hCompDc1 && hOld1)
					SelectObject(hCompDc1, hOld1);
				DeleteObject(hDIB);
				DeleteDC(hCompDc1);
			}
		
			//Gdiplus::Status lRc;
			//Gdiplus::GpRect rect(0, 0, lWidth, lHeight);
			//
			//GDIPlusPage *pPage = (GDIPlusPage*)CALLOC(sizeof(GDIPlusPage));
			//pPage->img = this;
			//pPage->nActivePage = nActivePage;
			//
			//Gdiplus::BitmapData *bmd = pPage ? &(pPage->bmd) : NULL;
			//
			//
			//if (!bmd) {
			//	nErrNumber = PGE_NOT_ENOUGH_MEMORY;
			//	
			//} else if (SelectPage(nActivePage)) {
			//	//TODO("прозрачные 8-битные GIF отрисовывются на белом фоне");
			//	//BOOL lbHasAlpha = (nImgFlags & Gdiplus::ImageFlagsHasAlpha) == Gdiplus::ImageFlagsHasAlpha;
			//	//pDecodeInfo->pPalette = NULL;
			//	//pDecodeInfo->Flags = PVD_IDF_READONLY;
			//	pDecodeInfo->ColorModel = CET_CM_BGR;
			//	
			//	// К сожалению, GDI+ декодирует CMYK СРАЗУ как YCbCrK, что не дает возможности отдать данные в CMYK
			//	if (pf == PixelFormat1bppIndexed || pf == PixelFormat4bppIndexed || pf == PixelFormat8bppIndexed)
			//	{
			//		INT cbPalette = 0;
			//		lRc = gdi->GdipGetImagePaletteSize(img, (INT*)&cbPalette);
			//		if (lRc == Gdiplus::Ok) {
			//			Gdiplus::ColorPalette* pPalette = (Gdiplus::ColorPalette*)CALLOC(cbPalette);
			//			lRc = gdi->GdipGetImagePalette(img, pPalette, cbPalette);
			//			if (lRc == Gdiplus::Ok) {
			//				_ASSERTE(pPalette->Count);
			//				pDecodeInfo->pPalette = (UINT32*)CALLOC(pPalette->Count*sizeof(UINT32));
			//				lbHasAlpha = (pPalette->Flags & Gdiplus::PaletteFlagsHasAlpha) == Gdiplus::PaletteFlagsHasAlpha;
			//				_ASSERTE(cbPalette > (INT)(pPalette->Count*sizeof(UINT32)));
			//				memmove(pDecodeInfo->pPalette, pPalette->Entries, pPalette->Count*sizeof(UINT32));
			//				if (lbHasAlpha) {
			//					UINT32 n, nIdx = 0, nCount = 0;
			//					// Найти в палитре прозрачный цвет
			//					for (n = 0; n < pPalette->Count; n++) {
			//						if ((pPalette->Entries[n] & 0xFF000000) != 0xFF000000) {
			//							if (nCount == 0) nIdx = n;
			//							nCount ++;
			//						}
			//					}
			//					if (nCount == 0) {
			//						lbHasAlpha = FALSE;
			//					} else if (nCount == 1 && (pPalette->Entries[nIdx] & 0xFF000000) == 0) {
			//						lbHasAlpha = FALSE; pDecodeInfo->Flags |= PVD_IDF_TRANSPARENT_INDEX;
			//						pDecodeInfo->nTransparentColor = nIdx;
			//					} else if (nCount > 0) {
			//						lbHasAlpha = TRUE;
			//					}
			//				}
			//				FREE(pPalette);
			//			}
			//		}
			//
			//		lRc = gdi->GdipBitmapLockBits(img, &rect, Gdiplus::ImageLockModeRead, pf, bmd);
			//		if (pf == PixelFormat1bppIndexed)
			//			pDecodeInfo->nBPP = 1;
			//		else if (pf == PixelFormat4bppIndexed)
			//			pDecodeInfo->nBPP = 4;
			//		else if (pf == PixelFormat8bppIndexed)
			//			pDecodeInfo->nBPP = 8;
			//
			//	} else {
			//		lRc = gdi->GdipBitmapLockBits(img, &rect, Gdiplus::ImageLockModeRead,
			//			lbHasAlpha ? PixelFormat32bppARGB : PixelFormat32bppRGB, bmd);
			//		if (lRc == 0) {
			//			pDecodeInfo->nBPP = 32;
			//		} else
			//		if (lRc /* == NotEnoughMemory */ && pf == PixelFormat24bppRGB) {
			//			lRc = gdi->GdipBitmapLockBits(img, &rect, Gdiplus::ImageLockModeRead,
			//				PixelFormat24bppRGB, bmd);
			//			pDecodeInfo->nBPP = 24;
			//		}
			//	}
			//	if (lRc != Gdiplus::Ok) {
			//		nLastError = GetLastError();
			//		nErrNumber = PGE_ERROR_BASE + (DWORD)lRc;
			//	} else {
			//		result = true;
			//		pPage->bBitmapData = true;
			//		if (lbHasAlpha) {
			//			pDecodeInfo->Flags |= PVD_IDF_ALPHA;
			//			pDecodeInfo->ColorModel = PVD_CM_BGRA;
			//		}
			//		pDecodeInfo->lWidth = lWidth;
			//		pDecodeInfo->lHeight = lHeight;
			//		pDecodeInfo->lImagePitch = bmd->Stride;
			//		pDecodeInfo->pImage = (LPBYTE)bmd->Scan0;
			//		pDecodeInfo->lParam = (LPARAM)pPage;
			//	}
			//}
			
		} __except( EXCEPTION_EXECUTE_HANDLER ) {
			//if (ip.MessageLog) ip.MessageLog(ip.pCallbackContext, L"Exception in GetPageBits", 2);
			nErrNumber = PGE_EXCEPTION;
		}

		return result;
	};
	//void FreePageBits(pvdInfoDecode2 *pDecodeInfo)
	//{
	//	if (!pDecodeInfo || !pDecodeInfo->lParam) return;
	//	__try{
	//		GDIPlusPage *pPage = (GDIPlusPage*)pDecodeInfo->lParam;
	//		if (pPage->bBitmapData)
	//			gdi->GdipBitmapUnlockBits(img, &(pPage->bmd));
	//
	//		FREE(pPage);
	//
	//		if (pDecodeInfo->pPalette)
	//			FREE(pDecodeInfo->pPalette);
	//	} __except( EXCEPTION_EXECUTE_HANDLER ) {
	//		if (ip.MessageLog) ip.MessageLog(ip.pCallbackContext, L"Exception in FreePageBits", 2);
	//		nErrNumber = PGE_EXCEPTION;
	//	}
	//}
};





BOOL WINAPI CET_Init(struct CET_Init* pInit)
{
	_ASSERTE(pInit->cbSize >= sizeof(struct CET_Init));
	if (pInit->cbSize < sizeof(struct CET_Init)) {
		pInit->nErrNumber = PGE_OLD_PICVIEW;
		return FALSE;
	}

	ghModule = pInit->hModule;

	GDIPlusDecoder *pDecoder = (GDIPlusDecoder*) CALLOC(sizeof(GDIPlusDecoder));
	if (!pDecoder) {
		pInit->nErrNumber = PGE_NOT_ENOUGH_MEMORY;
		return FALSE;
	}
	if (!pDecoder->Init(pInit)) {
		pInit->nErrNumber = pDecoder->nErrNumber;
		pDecoder->Close();
		FREE(pDecoder);
		return FALSE;
	}

	_ASSERTE(pInit->lParam == (LPARAM)pDecoder); // Уже вернул pDecoder->Init
	return TRUE;
}

VOID WINAPI CET_Done(struct CET_Init* pInit)
{
	if (pInit) {
		GDIPlusDecoder *pDecoder = (GDIPlusDecoder*)pInit->lParam;
		if (pDecoder) {
			pDecoder->Close();
			FREE(pDecoder);
		}
	}
}


#define SETERROR(n) if (pLoadPreview) pLoadPreview->nErrNumber = n; //else pLoadInfo->nErrNumber = n;


BOOL WINAPI CET_Load(LPVOID pArgs)
{
	_ASSERTE(*((LPDWORD)pArgs) == sizeof(struct CET_LoadPreview)); // || *((LPDWORD)pArgs) == sizeof(CET_LoadInfo));
	
	//void *pContext = NULL;
	//const wchar_t *pFileName = NULL;
	//INT64 lFileSize = 0;
	//const BYTE *pBuf = NULL;
	//UINT32 lBuf = 0;
	////pvdInfoImage2 *pImageInfo = NULL;
	//FILETIME ftModified = {0,0};
	//COORD crLoadSize = {96,96};
	//BOOL bTilesMode = FALSE;
	struct CET_LoadPreview *pLoadPreview = NULL;
	//CET_LoadInfo *pLoadInfo = NULL;
	
	if (*((LPDWORD)pArgs) == sizeof(struct CET_LoadPreview)) {
		pLoadPreview = (struct CET_LoadPreview*)pArgs;
		//pContext = (void*)pLoadPreview->lParam;
		//pFileName = pLoadPreview->sFileName;
		//lFileSize = pLoadPreview->nFileSize;
		//ftModified = pLoadPreview->ftModified;
		//crLoadSize = pLoadPreview->crLoadSize;
		//bTilesMode = pLoadPreview->bTilesMode;
	//} else if (*((LPDWORD)pArgs) == sizeof(CET_LoadInfo)) {
	//	pLoadInfo = (CET_LoadInfo*)pArgs;
	//	pContext = (void*)pLoadInfo->lParam;
	//	pFileName = pLoadInfo->sFileName;
	//	lFileSize = pLoadInfo->nFileSize;
	//	ftModified = pLoadInfo->ftModified;
	//	bTilesMode = TRUE;
	} else {
		return NULL;
	}
	
	if (!pLoadPreview->lParam) {
		SETERROR(PGE_INVALID_CONTEXT);
		return FALSE;
	}

	
	GDIPlusImage *pImage = (GDIPlusImage*)CALLOC(sizeof(GDIPlusImage));
	if (!pImage) {
		SETERROR(PGE_NOT_ENOUGH_MEMORY);
		return FALSE;
	}

	pImage->gdi = (GDIPlusDecoder*)pLoadPreview->lParam;

	if (!pImage->Open(pLoadPreview->sFileName)) {
		SETERROR(pImage->nErrNumber);
		pImage->Close();
		return FALSE;
	}

	//pImage->AddRef();

	//pImageInfo->pImageContext = pImage;
	//pImageInfo->nPages = pImage->nPages;
	//pImageInfo->Flags = (pImage->Animation ? PVD_IIF_ANIMATED : 0);
	//pImageInfo->pFormatName = pImage->FormatName;
	//pImageInfo->pCompression = NULL;
	//pImageInfo->pComments = NULL;
	//pPageInfo->lWidth = pImage->lWidth;
	//_ASSERTE(pImage->lHeight>0);
	//pPageInfo->lHeight = pImage->lHeight;
	//pPageInfo->nBPP = pImage->nBPP;
	
	if (pLoadPreview) {
		if (!pImage->GetPageBits(pLoadPreview)) {
			SETERROR(pImage->nErrNumber);
			pImage->Close();
			return FALSE;
		}
	}
	
	pImage->Close();
	return TRUE;
}
