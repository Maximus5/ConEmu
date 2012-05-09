
/*
Copyright (c) 2011 Maximus5
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


BITMAPFILEHEADER* LoadImageGdip(LPCWSTR asImgPath)
{
	BITMAPFILEHEADER* pBkImgData = NULL;

	Gdiplus::GdiplusStartupInput gdiplusStartupInput;
	Gdiplus::Status lRc;
	
	Gdiplus::GpBitmap *bmp = NULL;
	UINT lWidth = 0, lHeight = 0;
	Gdiplus::PixelFormat format;
	
	
	HDC hdcSrc = NULL, hdcDst = NULL;
	HBITMAP hLoadBmp = NULL, hDestBmp = NULL, hOldSrc = NULL, hOldDst = NULL;
	
	if (!hGdi || !gdiplusInitialized)
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
		
		lRc = GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL);
		if (lRc != Gdiplus::Ok)
			goto wrap;

		gdiplusInitialized = TRUE;
	}
	
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
