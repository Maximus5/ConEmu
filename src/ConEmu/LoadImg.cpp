
/*
Copyright (c) 2011-2017 Maximus5
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

THIS SOFTWARE IS PROVIDED BY THE AUTHOR ''AS IS'' AND ANY EXPRESS OR
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

#define HIDE_USE_EXCEPTION_INFO
#include "Header.h"

#include "../common/GdiPlusLt.h"

typedef Gdiplus::Status (WINAPI *GdiplusStartup_t)(OUT ULONG_PTR *token, const Gdiplus::GdiplusStartupInput *input, void /*OUT Gdiplus::GdiplusStartupOutput*/ *output);
typedef VOID (WINAPI *GdiplusShutdown_t)(ULONG_PTR token);
typedef Gdiplus::Status (WINAPI *GdipCreateBitmapFromFile_t)(GDIPCONST WCHAR* filename, Gdiplus::GpBitmap **bitmap);
typedef Gdiplus::Status (WINAPI *GdipDisposeImage_t)(Gdiplus::GpImage *image);
typedef Gdiplus::Status (WINAPI *GdipGetImageWidth_t)(Gdiplus::GpImage *image, UINT *width);
typedef Gdiplus::Status (WINAPI *GdipGetImageHeight_t)(Gdiplus::GpImage *image, UINT *height);
typedef Gdiplus::Status (WINAPI *GdipCreateHBITMAPFromBitmap_t)(Gdiplus::GpBitmap* bitmap, HBITMAP* hbmReturn, Gdiplus::ARGB background);
typedef Gdiplus::Status (WINAPI *GdipGetImagePixelFormat_t)(Gdiplus::GpImage *image, Gdiplus::PixelFormat *format);
typedef Gdiplus::Status (WINAPI *GdipLoadImageFromStream_t)(IStream* stream, Gdiplus::GpImage **image);
typedef Gdiplus::Status (WINAPI *GdipSaveImageToFile_t)(Gdiplus::GpImage *image, LPCWSTR filename, const CLSID* clsidEncoder, const Gdiplus::EncoderParameters* encoderParams);
typedef Gdiplus::Status (WINAPI *GdipGetImageEncodersSize_t)(UINT *numEncoders, UINT *size);
typedef Gdiplus::Status (WINAPI *GdipGetImageEncoders_t)(UINT numEncoders, UINT size, Gdiplus::ImageCodecInfo *encoders);
typedef Gdiplus::Status (WINAPI *GdipCreateBitmapFromHBITMAP_t)(HBITMAP hbm, HPALETTE hpal, Gdiplus::GpImage** bitmap);


#include "LoadImg.h"

static HMODULE hGdi = NULL;
static ULONG_PTR gdiplusToken = 0;
static BOOL gdiplusInitialized = FALSE;
static GdiplusStartup_t GdiplusStartup = NULL;
static GdiplusShutdown_t GdiplusShutdown = NULL;
static GdipCreateBitmapFromFile_t GdipCreateBitmapFromFile = NULL;
static GdipDisposeImage_t GdipDisposeImage = NULL;
static GdipGetImageWidth_t GdipGetImageWidth = NULL;
static GdipGetImageHeight_t GdipGetImageHeight = NULL;
static GdipCreateHBITMAPFromBitmap_t GdipCreateHBITMAPFromBitmap = NULL;
static GdipGetImagePixelFormat_t GdipGetImagePixelFormat = NULL;
static GdipLoadImageFromStream_t GdipLoadImageFromStream = NULL;
static GdipSaveImageToFile_t GdipSaveImageToFile = NULL;
static GdipGetImageEncodersSize_t GdipGetImageEncodersSize = NULL;
static GdipGetImageEncoders_t GdipGetImageEncoders = NULL;
static GdipCreateBitmapFromHBITMAP_t GdipCreateBitmapFromHBITMAP = NULL;

#include "../common/MStream.h"

//BOOL gbMStreamCoInitialized=FALSE;
//class MStream : public IStream
//{
//	private:
//		LONG mn_RefCount; BOOL mb_SelfAlloc; char* mp_Data; DWORD mn_DataSize; DWORD mn_DataLen; DWORD mn_DataPos;
//	public:
//		MStream(LPCVOID apData, DWORD anLen)
//		{
//			mn_RefCount = 1; mb_SelfAlloc=FALSE; mp_Data = (char*)apData; mn_DataSize = anLen;
//			mn_DataPos = 0; mn_DataLen = anLen;
//
//			if (!gbMStreamCoInitialized)
//			{
//				HRESULT hr = CoInitialize(NULL);
//				gbMStreamCoInitialized = TRUE;
//			}
//		};
//		MStream()
//		{
//			mn_RefCount = 1;
//			mn_DataSize = 4096*1024; mn_DataPos = 0; mn_DataLen = 0;
//			mp_Data = (char*)calloc(mn_DataSize,1);
//
//			if (mp_Data==NULL)
//			{
//				mn_DataSize = 0;
//			}
//
//			mb_SelfAlloc = TRUE;
//
//			if (!gbMStreamCoInitialized)
//			{
//				HRESULT hr = CoInitialize(NULL);
//				gbMStreamCoInitialized = TRUE;
//			}
//		};
//		HRESULT SaveAsData(
//		    void** rpData,
//		    long*  rnDataSize
//		)
//		{
//			if (mp_Data==NULL)
//			{
//				return E_POINTER;
//			}
//
//			if (mn_DataLen>0)
//			{
//				*rpData = LocalAlloc(LPTR, mn_DataLen);
//				memcpy(*rpData, mp_Data, mn_DataLen);
//			}
//
//			*rnDataSize = mn_DataLen;
//			return S_OK;
//		};
//	private:
//		~MStream()
//		{
//			if (mp_Data!=NULL)
//			{
//				if (mb_SelfAlloc) free(mp_Data);
//
//				mp_Data=NULL;
//				mn_DataSize = 0; mn_DataPos = 0; mn_DataLen = 0;
//			}
//		};
//	public:
//		virtual HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void __RPC_FAR *__RPC_FAR *ppvObject)
//		{
//			if (riid==IID_IStream || riid==IID_ISequentialStream || riid==IID_IUnknown)
//			{
//				*ppvObject = this;
//				return S_OK;
//			}
//
//			return E_NOINTERFACE;
//		}
//
//		virtual ULONG STDMETHODCALLTYPE AddRef(void)
//		{
//			if (this==NULL)
//				return 0;
//
//			return (++mn_RefCount);
//		}
//
//		virtual ULONG STDMETHODCALLTYPE Release(void)
//		{
//			if (this==NULL)
//				return 0;
//
//			mn_RefCount--;
//			int nCount = mn_RefCount;
//
//			if (nCount<=0)
//			{
//				delete this;
//			}
//
//			return nCount;
//		}
//	public:
//		/* ISequentialStream */
//		virtual /* [local] */ HRESULT STDMETHODCALLTYPE Read(
//		    /* [length_is][size_is][out] */ void *pv,
//		    /* [in] */ ULONG cb,
//		    /* [out] */ ULONG *pcbRead)
//		{
//			DWORD dwRead=0;
//
//			if (mp_Data)
//			{
//				dwRead = min(cb, max(0,(mn_DataLen-mn_DataPos)));
//
//				if (dwRead>0)
//				{
//					memmove(pv, mp_Data+mn_DataPos, dwRead);
//					mn_DataPos += dwRead;
//				}
//			}
//			else
//			{
//				return S_FALSE;
//			}
//
//			if (pcbRead) *pcbRead=dwRead;
//
//			return S_OK;
//		};
//
//		virtual /* [local] */ HRESULT STDMETHODCALLTYPE Write(
//		    /* [size_is][in] */ const void *pv,
//		    /* [in] */ ULONG cb,
//		    /* [out] */ ULONG *pcbWritten)
//		{
//			DWORD dwWritten=0;
//
//			if (!mp_Data)
//			{
//				ULARGE_INTEGER lNewSize; lNewSize.QuadPart = cb;
//				HRESULT hr;
//
//				if (FAILED(hr = SetSize(lNewSize)))
//					return hr;
//			}
//
//			if (mp_Data)
//			{
//				if ((mn_DataPos+cb)>mn_DataSize)
//				{
//					// Нужно увеличить буфер, но сохранить текущий размер
//					DWORD lLastLen = mn_DataLen;
//					ULARGE_INTEGER lNewSize; lNewSize.QuadPart = mn_DataSize + max((cb+1024), 256*1024);
//
//					if (lNewSize.HighPart!=0)
//						return S_FALSE;
//
//					if (FAILED(SetSize(lNewSize)))
//						return S_FALSE;
//
//					mn_DataLen = lLastLen; // Вернули текущий размер
//				}
//
//				// Теперь можно писать в буфер
//				memmove(mp_Data+mn_DataPos, pv, cb);
//				dwWritten = cb;
//				mn_DataPos += cb;
//
//				if (mn_DataPos>mn_DataLen) mn_DataLen=mn_DataPos;  //2008-03-18
//			}
//			else
//			{
//				return S_FALSE;
//			}
//
//			if (pcbWritten) *pcbWritten=dwWritten;
//
//			return S_OK;
//		};
//	public:
//		/* IStream */
//		virtual /* [local] */ HRESULT STDMETHODCALLTYPE Seek(
//		    /* [in] */ LARGE_INTEGER dlibMove,
//		    /* [in] */ DWORD dwOrigin,
//		    /* [out] */ ULARGE_INTEGER *plibNewPosition)
//		{
//			if (dwOrigin!=STREAM_SEEK_SET && dwOrigin!=STREAM_SEEK_CUR && dwOrigin!=STREAM_SEEK_END)
//				return STG_E_INVALIDFUNCTION;
//
//			if (mp_Data)
//			{
//				_ASSERTE(mn_DataSize);
//				LARGE_INTEGER lNew;
//
//				if (dwOrigin==STREAM_SEEK_SET)
//				{
//					lNew.QuadPart = dlibMove.QuadPart;
//				}
//				else if (dwOrigin==STREAM_SEEK_CUR)
//				{
//					lNew.QuadPart = mn_DataPos + dlibMove.QuadPart;
//				}
//				else if (dwOrigin==STREAM_SEEK_END)
//				{
//					lNew.QuadPart = mn_DataLen + dlibMove.QuadPart;
//				}
//
//				if (lNew.QuadPart<0)
//					return S_FALSE;
//
//				if (lNew.QuadPart>mn_DataSize)
//					return S_FALSE;
//
//				mn_DataPos = lNew.LowPart;
//
//				if (plibNewPosition)
//					plibNewPosition->QuadPart = mn_DataPos;
//			}
//			else
//			{
//				return S_FALSE;
//			}
//
//			return S_OK;
//		};
//
//		virtual HRESULT STDMETHODCALLTYPE SetSize(
//		    /* [in] */ ULARGE_INTEGER libNewSize)
//		{
//			HRESULT hr = STG_E_INVALIDFUNCTION;
//			ULARGE_INTEGER llTest; llTest.QuadPart = 0;
//			LARGE_INTEGER llShift; llShift.QuadPart = 0;
//
//			if (libNewSize.HighPart)
//				return E_OUTOFMEMORY;
//
//			if (mp_Data)
//			{
//				if (libNewSize.LowPart>mn_DataSize)
//				{
//					char* pNew = (char*)realloc(mp_Data, libNewSize.LowPart);
//
//					if (pNew==NULL)
//						return E_OUTOFMEMORY;
//
//					mp_Data = pNew;
//				}
//
//				mn_DataLen = libNewSize.LowPart;
//
//				if (mn_DataPos>mn_DataLen)  // Если размер уменьшили - проверить позицию
//					mn_DataPos = mn_DataLen;
//
//				hr = S_OK;
//			}
//			else
//			{
//				mp_Data = (char*)calloc(libNewSize.LowPart,1);
//
//				if (mp_Data==NULL)
//					return E_OUTOFMEMORY;
//
//				mn_DataSize = libNewSize.LowPart;
//				mn_DataLen = libNewSize.LowPart;
//				mn_DataPos = 0;
//				hr = S_OK;
//			}
//
//			return hr;
//		};
//
//		virtual /* [local] */ HRESULT STDMETHODCALLTYPE CopyTo(
//		    /* [unique][in] */ IStream *pstm,
//		    /* [in] */ ULARGE_INTEGER cb,
//		    /* [out] */ ULARGE_INTEGER *pcbRead,
//		    /* [out] */ ULARGE_INTEGER *pcbWritten)
//		{
//			return STG_E_INVALIDFUNCTION;
//		};
//
//		virtual HRESULT STDMETHODCALLTYPE Commit(
//		    /* [in] */ DWORD grfCommitFlags)
//		{
//			if (mp_Data)
//			{
//				//
//			}
//			else
//			{
//				return S_FALSE;
//			}
//
//			return S_OK;
//		};
//
//		virtual HRESULT STDMETHODCALLTYPE Revert(void)
//		{
//			return STG_E_INVALIDFUNCTION;
//		};
//
//		virtual HRESULT STDMETHODCALLTYPE LockRegion(
//		    /* [in] */ ULARGE_INTEGER libOffset,
//		    /* [in] */ ULARGE_INTEGER cb,
//		    /* [in] */ DWORD dwLockType)
//		{
//			return STG_E_INVALIDFUNCTION;
//		};
//
//		virtual HRESULT STDMETHODCALLTYPE UnlockRegion(
//		    /* [in] */ ULARGE_INTEGER libOffset,
//		    /* [in] */ ULARGE_INTEGER cb,
//		    /* [in] */ DWORD dwLockType)
//		{
//			return STG_E_INVALIDFUNCTION;
//		};
//
//		virtual HRESULT STDMETHODCALLTYPE Stat(
//		    /* [out] */ STATSTG *pstatstg,
//		    /* [in] */ DWORD grfStatFlag)
//		{
//			return STG_E_INVALIDFUNCTION;
//		};
//
//		virtual HRESULT STDMETHODCALLTYPE Clone(
//		    /* [out] */ IStream **ppstm)
//		{
//			return STG_E_INVALIDFUNCTION;
//		};
//};

static bool InitializeGdiPlus()
{
	bool lbRc = false;
	Gdiplus::GdiplusStartupInput gdiplusStartupInput;
	Gdiplus::Status lRc;

	if (hGdi && gdiplusInitialized)
	{
		lbRc = true;
	}
	else
	{
		hGdi = LoadLibrary(L"GdiPlus.dll");
		if (!hGdi || hGdi == INVALID_HANDLE_VALUE)
			goto wrap;
		if (!(GdiplusStartup = (GdiplusStartup_t)GetProcAddress(hGdi, "GdiplusStartup")))
			goto wrap;
		if (!(GdiplusShutdown = (GdiplusShutdown_t)GetProcAddress(hGdi, "GdiplusShutdown")))
			goto wrap;
		if (!(GdipCreateBitmapFromFile = (GdipCreateBitmapFromFile_t)GetProcAddress(hGdi, "GdipCreateBitmapFromFile")))
			goto wrap;
		if (!(GdipDisposeImage = (GdipDisposeImage_t)GetProcAddress(hGdi, "GdipDisposeImage")))
			goto wrap;
		if (!(GdipGetImageWidth = (GdipGetImageWidth_t)GetProcAddress(hGdi, "GdipGetImageWidth")))
			goto wrap;
		if (!(GdipGetImageHeight = (GdipGetImageHeight_t)GetProcAddress(hGdi, "GdipGetImageHeight")))
			goto wrap;
		if (!(GdipCreateHBITMAPFromBitmap = (GdipCreateHBITMAPFromBitmap_t)GetProcAddress(hGdi, "GdipCreateHBITMAPFromBitmap")))
			goto wrap;
		if (!(GdipGetImagePixelFormat = (GdipGetImagePixelFormat_t)GetProcAddress(hGdi, "GdipGetImagePixelFormat")))
			goto wrap;
		if (!(GdipLoadImageFromStream = (GdipLoadImageFromStream_t)GetProcAddress(hGdi, "GdipLoadImageFromStream")))
			goto wrap;
		if (!(GdipSaveImageToFile = (GdipSaveImageToFile_t)GetProcAddress(hGdi, "GdipSaveImageToFile")))
			goto wrap;
		if (!(GdipGetImageEncodersSize = (GdipGetImageEncodersSize_t)GetProcAddress(hGdi, "GdipGetImageEncodersSize")))
			goto wrap;
		if (!(GdipGetImageEncoders = (GdipGetImageEncoders_t)GetProcAddress(hGdi, "GdipGetImageEncoders")))
			goto wrap;
		if (!(GdipCreateBitmapFromHBITMAP = (GdipCreateBitmapFromHBITMAP_t)GetProcAddress(hGdi, "GdipCreateBitmapFromHBITMAP")))
			goto wrap;

		lRc = GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL);
		if (lRc != Gdiplus::Ok)
			goto wrap;

		gdiplusInitialized = TRUE;
		lbRc = true;
	}
wrap:
	return lbRc;
}

BITMAPFILEHEADER* LoadImageGdip(LPCWSTR asImgPath)
{
	BITMAPFILEHEADER* pBkImgData = NULL;

	Gdiplus::Status lRc;

	Gdiplus::GpBitmap *bmp = NULL;
	UINT lWidth = 0, lHeight = 0;
	Gdiplus::PixelFormat format;


	HDC hdcSrc = NULL, hdcDst = NULL;
	HBITMAP hLoadBmp = NULL, hDestBmp = NULL, hOldSrc = NULL, hOldDst = NULL;

	if (!InitializeGdiPlus())
		goto wrap;

	lRc = GdipCreateBitmapFromFile(asImgPath, &bmp);
	if (lRc != Gdiplus::Ok)
		goto wrap;

	lRc = GdipGetImageWidth(bmp, &lWidth);
	if (lRc != Gdiplus::Ok || !lWidth)
		goto wrap;
	lRc = GdipGetImageHeight(bmp, &lHeight);
	if (lRc != Gdiplus::Ok || !lHeight)
		goto wrap;

	lRc = GdipGetImagePixelFormat(bmp, &format);
	if (lRc != Gdiplus::Ok)
		goto wrap;

	lRc = GdipCreateHBITMAPFromBitmap(bmp, &hLoadBmp, 0);
	if (lRc != Gdiplus::Ok || !hLoadBmp)
		goto wrap;

	hdcSrc = CreateCompatibleDC(0);
	if (!hdcSrc)
		goto wrap;
	hOldSrc = (HBITMAP)SelectObject(hdcSrc, hLoadBmp);

	hdcDst = CreateCompatibleDC(0);
	if (!hdcDst)
		goto wrap;

	{
		UINT lWidth0 = lWidth; //((lWidth + 7) >> 3) << 3;
		UINT lStride = lWidth0*3;
		size_t nAllSize = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFO) + lHeight*lStride; //(bmd.Stride>0?bmd.Stride:-bmd.Stride);

		pBkImgData = (BITMAPFILEHEADER*)malloc(nAllSize);
		if (pBkImgData)
		{
			BITMAPINFOHEADER* pBmp = (BITMAPINFOHEADER*)(pBkImgData+1);
			// Заполняем данными
			pBkImgData->bfType = 0x4D42/*BM*/;
			pBkImgData->bfSize = (DWORD)nAllSize;
			pBkImgData->bfReserved1 = pBkImgData->bfReserved2 = 0;
			pBkImgData->bfOffBits = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFO);
			pBmp->biSize = sizeof(BITMAPINFOHEADER);
			pBmp->biWidth = lWidth0;
			pBmp->biHeight = lHeight;
			pBmp->biPlanes = 1;
			pBmp->biBitCount = 24;
			pBmp->biCompression = BI_RGB;
			pBmp->biSizeImage = 0;
			pBmp->biXPelsPerMeter = 72;
			pBmp->biYPelsPerMeter = 72;
			pBmp->biClrUsed = 0;
			pBmp->biClrImportant = 0;
			LPBYTE pDst = ((LPBYTE)(pBkImgData)) + pBkImgData->bfOffBits;

			LPBYTE pDstBits = NULL;
			hDestBmp = CreateDIBSection(hdcDst, (BITMAPINFO*)pBmp, DIB_RGB_COLORS, (void**)&pDstBits, NULL, 0);
			if (!hDestBmp || !pDstBits)
			{
				free(pBkImgData);
				pBkImgData = NULL;
				goto wrap;
			}
			hOldDst = (HBITMAP)SelectObject(hdcDst, hDestBmp);

			BitBlt(hdcDst, 0,0,lWidth,lHeight, hdcSrc, 0,0, SRCCOPY);
			GdiFlush();

			memmove(pDst, pDstBits, pBmp->biWidth*lHeight*3);
			pBmp->biWidth = lWidth;
		}
	}

wrap:
	if (hdcSrc)
	{
		if (hOldSrc)
			SelectObject(hdcSrc, hOldSrc);
		DeleteDC(hdcSrc);
	}
	if (hdcDst)
	{
		if (hOldDst)
			SelectObject(hdcDst, hOldDst);
		DeleteDC(hdcDst);
	}
	if (hLoadBmp)
		DeleteObject(hLoadBmp);
	if (hDestBmp)
		DeleteObject(hDestBmp);
	if (bmp)
		GdipDisposeImage(bmp);
	if (!gdiplusInitialized)
		LoadImageFinalize();
	return pBkImgData;
}

BITMAPFILEHEADER* CreateSolidImage(COLORREF clr, UINT lWidth, UINT lHeight)
{
	BITMAPFILEHEADER* pBkImgData = NULL;

	{
		_ASSERTE(lWidth == 128); // Some predefined size...
		UINT lWidth0 = lWidth; //((lWidth + 7) >> 3) << 3;
		UINT lStride = lWidth0*3;
		size_t nAllSize = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFO) + lHeight*lStride; //(bmd.Stride>0?bmd.Stride:-bmd.Stride);

		pBkImgData = (BITMAPFILEHEADER*)malloc(nAllSize);
		if (pBkImgData)
		{
			BITMAPINFOHEADER* pBmp = (BITMAPINFOHEADER*)(pBkImgData+1);
			// Заполняем данными
			pBkImgData->bfType = 0x4D42/*BM*/;
			pBkImgData->bfSize = (DWORD)nAllSize;
			pBkImgData->bfReserved1 = pBkImgData->bfReserved2 = 0;
			pBkImgData->bfOffBits = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFO);
			pBmp->biSize = sizeof(BITMAPINFOHEADER);
			pBmp->biWidth = lWidth0;
			pBmp->biHeight = lHeight;
			pBmp->biPlanes = 1;
			pBmp->biBitCount = 24;
			pBmp->biCompression = BI_RGB;
			pBmp->biSizeImage = 0;
			pBmp->biXPelsPerMeter = 72;
			pBmp->biYPelsPerMeter = 72;
			pBmp->biClrUsed = 0;
			pBmp->biClrImportant = 0;

			//DWORD  n3 = (clr & 0xFFFFFF);
			//DWORD  nColors[3] = { (n3 | ((n3 & 0xFF) << 24)), ((n3 >> 8) | ((n3 & 0xFFFF) << 16)) , ((n3 >> 16) | (n3 << 8)) };
			DWORD  n1 = (clr & 0x0000FF);
			DWORD  n2 = (clr & 0x00FF00)>>8;
			DWORD  n3 = (clr & 0xFF0000)>>16;
			DWORD  nColors[3] = { (n3)|(n2<<8)|(n1<<16)|(n3<<24), (n2)|(n1<<8)|(n3<<16)|(n2<<24), (n1)|(n3<<8)|(n2<<16)|(n1<<24) };

			LPDWORD pDst = (LPDWORD)(((LPBYTE)(pBkImgData)) + pBkImgData->bfOffBits);
			size_t nMax = (lHeight*lStride) >> 2; // Size of data in DWORDs
			size_t i = 0;
			while (i < nMax)
			{
				*(pDst++) = nColors[0];
				*(pDst++) = nColors[1];
				*(pDst++) = nColors[2];
				i += 3;
			}

			pBmp->biWidth = lWidth;
		}
	}

	return pBkImgData;
}

void LoadImageFinalize()
{
	if (hGdi)
	{
		if (gdiplusInitialized)
		{
			GdiplusShutdown(gdiplusToken);
			gdiplusInitialized = FALSE;
		}
		FreeLibrary(hGdi);
		hGdi = NULL;
		GdiplusShutdown = NULL;
	}
}

BITMAPFILEHEADER* LoadImageEx(LPCWSTR asImgPath, BY_HANDLE_FILE_INFORMATION& inf)
{
	BITMAPFILEHEADER* pBkImgData = NULL;
	HANDLE hFile = CreateFile(asImgPath, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, 0, NULL);

	if (hFile != INVALID_HANDLE_VALUE)
	{
		BOOL lbIsBmpFile = FALSE, lbFileIsValid = FALSE;

		if (GetFileInformationByHandle(hFile, &inf) && inf.nFileSizeHigh == 0
		        && inf.nFileSizeLow >= (sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFO))) //-V119
		{
			lbFileIsValid = TRUE;

			BYTE bm[2];
			DWORD nRead;
			if (ReadFile(hFile, bm, sizeof(bm), &nRead, NULL) && nRead == 2 && (bm[0] == 'B' && bm[1] == 'M'))
			{
				lbIsBmpFile = TRUE;

				pBkImgData = (BITMAPFILEHEADER*)malloc(inf.nFileSizeLow);
				if (!pBkImgData)
				{
					_ASSERTE(pBkImgData!=NULL);
					CloseHandle(hFile);
					return NULL;
				}
				BYTE *pBuf = (BYTE*)pBkImgData;
				memmove(pBkImgData, bm, sizeof(bm));

				if (pBkImgData && ReadFile(hFile, pBuf+sizeof(bm), inf.nFileSizeLow-sizeof(bm), &inf.nFileSizeLow, NULL))
				{
					inf.nFileSizeLow += sizeof(bm); // BM

					if (pBuf[0] == 'B' && pBuf[1] == 'M' && *(u32*)(pBuf + 0x0A) >= 0x36 && *(u32*)(pBuf + 0x0A) <= 0x436 && *(u32*)(pBuf + 0x0E) == 0x28 && !pBuf[0x1D] && !*(u32*)(pBuf + 0x1E))
					{
						// OK
					}
					else
					{
						free(pBkImgData);
						pBkImgData = NULL;
					}
				}
			}
		}
		CloseHandle(hFile);

		if (!pBkImgData && lbFileIsValid && !lbIsBmpFile)
		{
			// Если это не BMP - попытаться через GDI+
			pBkImgData = LoadImageGdip(asImgPath);
		}
	}

	return pBkImgData;
}

bool SaveImageEx(LPCWSTR asImgPath, HBITMAP hBitmap)
{
	if (!InitializeGdiPlus())
		return false;

	CLSID codecClsid = {};
	if (GetCodecClsid(L"image/png", &codecClsid) < 0)
		return false;

	bool lbRc = false;
	Gdiplus::Status lRc;
	GUID EncoderQuality = {0x1d5be4b5, 0xfa4a, 0x452d, {0x9c,0xdd,0x5d,0xb3,0x51,0x05,0xe7,0xeb}};

	Gdiplus::EncoderParameters encoderParameters;
	Gdiplus::GpImage *pImg = NULL;

	#ifdef _DEBUG
	DWORD nErrCode = 0;
	BITMAP bmpInfo = {};
	nErrCode = GetObject(hBitmap, sizeof(bmpInfo), &bmpInfo);
	#endif

	lRc = GdipCreateBitmapFromHBITMAP(hBitmap, NULL, &pImg);

	if ((lRc == Gdiplus::Ok) && pImg)
	{
		encoderParameters.Count = 1;
		encoderParameters.Parameter[0].Guid = EncoderQuality;
		encoderParameters.Parameter[0].Type = Gdiplus::EncoderParameterValueTypeLong;
		encoderParameters.Parameter[0].NumberOfValues = 1;
		long quality = 75;
		encoderParameters.Parameter[0].Value = &quality;

		lRc = GdipSaveImageToFile(pImg, asImgPath, &codecClsid, &encoderParameters);

		if (lRc == Gdiplus::Ok)
			lbRc = true;

		GdipDisposeImage(pImg);
	}
	else if (lRc == Gdiplus::Win32Error)
	{
		// Возможно, это проблемы с палитрой в WinXP
		#ifdef _DEBUG
		nErrCode = GetLastError();
		#endif
		_ASSERTE(lRc == Gdiplus::Ok);
	}
	else
	{
		_ASSERTE((lRc == Gdiplus::Ok) && pImg);
	}

	return lbRc;
}

bool SaveImageEx(LPCWSTR asImgPath, LPBYTE pBmpData, DWORD cbBmpDataSize)
{
	if (!InitializeGdiPlus())
		return false;

	CLSID codecClsid = {};
	if (GetCodecClsid(L"image/png", &codecClsid) < 0)
		return false;

	bool lbRc = false;
	Gdiplus::Status lRc;
	MStream *pStream = new MStream();

	if (pStream)
	{
		pStream->SetData(pBmpData, cbBmpDataSize);
		GUID EncoderQuality = {0x1d5be4b5, 0xfa4a, 0x452d, {0x9c,0xdd,0x5d,0xb3,0x51,0x05,0xe7,0xeb}};

		Gdiplus::EncoderParameters encoderParameters;
		Gdiplus::GpImage *pImg = NULL;

		lRc = GdipLoadImageFromStream((IStream*)pStream, &pImg);

		if ((lRc == Gdiplus::Ok) && pImg)
		{
			encoderParameters.Count = 1;
			encoderParameters.Parameter[0].Guid = EncoderQuality;
			encoderParameters.Parameter[0].Type = Gdiplus::EncoderParameterValueTypeLong;
			encoderParameters.Parameter[0].NumberOfValues = 1;
			long quality = 75;
			encoderParameters.Parameter[0].Value = &quality;

			lRc = GdipSaveImageToFile(pImg, asImgPath, &codecClsid, &encoderParameters);

			if (lRc == Gdiplus::Ok)
				lbRc = true;

			GdipDisposeImage(pImg);
		}

		pStream->Release(); pStream=NULL;
	}

	return lbRc;
}

int GetCodecClsid(const WCHAR* format, CLSID* pClsid)
{
	UINT  num = 0;          // number of image encoders
	UINT  size = 0;         // size of the image encoder array in bytes
	Gdiplus::ImageCodecInfo* pImageCodecInfo = NULL;

	GdipGetImageEncodersSize(&num, &size);

	if (size == 0)
		return -1;  // Failure

	pImageCodecInfo = (Gdiplus::ImageCodecInfo*)(calloc(size,1));

	if (pImageCodecInfo == NULL)
		return -1;  // Failure

	int iRc = -1;

	GdipGetImageEncoders(num, size, pImageCodecInfo);
	for (int j = 0; j < (int)num; ++j)
	{
		if (lstrcmpi(pImageCodecInfo[j].MimeType, format) == 0)
		{
			*pClsid = pImageCodecInfo[j].Clsid;
			iRc = j; // Success
			break;
		}
	} // for

	free(pImageCodecInfo);

	return iRc;
} // GetCodecClsid
