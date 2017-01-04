
/*
Copyright (c) 2009-2017 Maximus5
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

//#ifdef _DEBUG
#define PNGDUMP
//#endif

#include <windows.h>
//#ifdef PNGDUMP
//#include <Gdiplus.h>
//#endif
#include "Header.h"
#include "LoadImg.h"
#include "ScreenDump.h"

//WARNING("GdiPlus is XP Only");

BOOL LoadScreen(HDC hScreen, int anX, int anY, int anWidth, int anHeight, LPBYTE* pScreen, DWORD *dwSize, bool PreserveTransparency=true)
{
	*pScreen = NULL;
	*dwSize = 0;
	int iBitPerPixel = PreserveTransparency ? 32 : 24;
	int iHdrSize = sizeof(BITMAPINFOHEADER);
	int iScrWidth = (anWidth+3)>>2<<2;
	int iScrHeight= anHeight;
	int iWidth = iScrWidth;
	int iHeight= iScrHeight;
	int iMemSize = iWidth*iHeight*iBitPerPixel/8;
	BITMAPINFO *bi = (BITMAPINFO *) calloc(iHdrSize,1);
	BITMAPINFOHEADER *bih = &(bi->bmiHeader);
	bih->biSize = sizeof(BITMAPINFOHEADER);
	bih->biWidth  = iWidth;
	bih->biHeight = iHeight;
	bih->biPlanes = 1;
	bih->biBitCount = iBitPerPixel;
	bih->biCompression = BI_RGB;
	bih->biXPelsPerMeter = 96;
	bih->biYPelsPerMeter = 96;
	bih->biClrImportant = 0;
	char * pData = (char*)calloc(iMemSize,1);
	//char * pDataEnd = pData + iMemSize;

	if (pData==NULL || bi==NULL)
	{
		if (bi) free(bi);

		if (pData) free(pData);

		return FALSE;
	}

	//char *pDataCur = pData;
	HDC hMem = CreateCompatibleDC(hScreen);
	char* ppvBits = NULL;
	HBITMAP hScreenBmp = CreateDIBSection(hMem, (const BITMAPINFO*) bi, DIB_RGB_COLORS, (LPVOID*)&ppvBits, NULL, 0);
	HBITMAP hOldBmp = (HBITMAP) SelectObject(hMem, hScreenBmp);
	BOOL bRc = FALSE;

	bool lbHasAlpha = true;
	#ifdef __GNUC__
	static AlphaBlend_t GdiAlphaBlend = NULL;
	if (!GdiAlphaBlend)
	{
		HMODULE hGdi32 = GetModuleHandle(L"gdi32.dll");
		GdiAlphaBlend = (AlphaBlend_t)(hGdi32 ? GetProcAddress(hGdi32, "GdiAlphaBlend") : NULL);
	}
	lbHasAlpha = (GdiAlphaBlend != NULL);
	#endif

	if (lbHasAlpha)
	{
		BLENDFUNCTION bf = {AC_SRC_OVER, 0, 255, AC_SRC_ALPHA};
		bRc = GdiAlphaBlend(hMem, 0,0,iWidth,iHeight, hScreen, anX, anY, iWidth,iHeight, bf);
	}
	else
	{
		bRc = BitBlt(hMem, 0,0,iWidth,iHeight, hScreen, anX, anY, SRCCOPY);
	}

	UNREFERENCED_PARAMETER(bRc);

	BITMAPFILEHEADER bfh;
	bfh.bfType = 0x4D42; // 'MB';
	bfh.bfSize = sizeof(BITMAPFILEHEADER)+iHdrSize+iMemSize;
	bfh.bfReserved1=0;
	bfh.bfReserved2=0;
	bfh.bfOffBits=sizeof(BITMAPFILEHEADER)+iHdrSize;
	*dwSize = sizeof(bfh)+iHdrSize+iMemSize;
	*pScreen = (LPBYTE)calloc(*dwSize,1);
	BOOL lbRc = FALSE;

	if (*pScreen)
	{
		LPBYTE pPtr = *pScreen;
		memcpy(pPtr,&bfh,sizeof(bfh));
		pPtr += sizeof(bfh);
		memcpy(pPtr,bi,iHdrSize);
		pPtr += iHdrSize;
		memcpy(pPtr,ppvBits,iMemSize);
		pPtr += iMemSize;
		lbRc = TRUE;
	}
	else
	{
		*dwSize = 0;
	}

	SelectObject(hMem, hOldBmp);
	DeleteDC(hMem);
	DeleteObject(hScreenBmp);
	free(pData);
	free(bi);
	return lbRc;
}

#ifdef PNGDUMP
//int GetCodecClsid(const WCHAR* format, CLSID* pClsid)
//{
//	UINT  num = 0;          // number of image encoders
//	UINT  size = 0;         // size of the image encoder array in bytes
//	Gdiplus::ImageCodecInfo* pImageCodecInfo = NULL;
//	Gdiplus::GetImageEncodersSize(&num, &size);
//
//	if (size == 0)
//		return -1;  // Failure
//
//	pImageCodecInfo = (Gdiplus::ImageCodecInfo*)(malloc(size));
//
//	if (pImageCodecInfo == NULL)
//		return -1;  // Failure
//
//	Gdiplus::GetImageEncoders(num, size, pImageCodecInfo);
//
//	for(int j = 0; j < (int)num; ++j)
//	{
//		if (wcscmp(pImageCodecInfo[j].MimeType, format) == 0)
//		{
//			*pClsid = pImageCodecInfo[j].Clsid;
//			return j;  // Success
//		}
//	} // for
//
//	return -1;  // Failure
//} // GetCodecClsid


#endif



BOOL DumpImage(HDC hScreen, HBITMAP hBitmap, int anX, int anY, int anWidth, int anHeight, LPCWSTR pszFile, bool PreserveTransparency/*=true*/)
{
	BOOL lbRc = FALSE;
	LPBYTE pScreen = NULL;
	DWORD cbOut = 0;

	wchar_t* pszDot = NULL;
	wchar_t szFile[MAX_PATH+1] = {};

	if (pszFile)
	{
		lstrcpynW(szFile, pszFile, MAX_PATH);
		pszDot = (wchar_t*)PointToExt(szFile);
	}


	#ifdef PNGDUMP
	// Если передали готовый hBitmap - сохранить сразу
	if (pszDot)
		lstrcpyW(pszDot, L".png");

	if (hBitmap && SaveImageEx(szFile, hBitmap))
	{
		return TRUE;
	}
	#endif


	// иначе - снять копию с hScreen
	if (!LoadScreen(hScreen, anX, anY, anWidth, anHeight, &pScreen, &cbOut, PreserveTransparency))
		return FALSE;

	if (!*szFile)
	{
		//static wchar_t szLastDumpFile[MAX_PATH];
		SYSTEMTIME st; GetLocalTime(&st);
		_wsprintf(szFile, SKIPLEN(countof(szFile)) L"%02u%02u%02u%02u%02u%02u",
			st.wYear%100, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond);

		OPENFILENAME ofn; memset(&ofn,0,sizeof(ofn));
		ofn.lStructSize=sizeof(ofn);
		HWND h = GetForegroundWindow();
		DWORD nPID = 0;
		if (h)
			GetWindowThreadProcessId(h, &nPID);
		ofn.hwndOwner = (nPID == GetCurrentProcessId()) ? h : ghWnd;
		#ifdef PNGDUMP
		ofn.lpstrFilter = L"PNG (*.png)\0*.png\0JPEG (*.jpg)\0*.jpg\0BMP (*.bmp)\0*.bmp\0\0";
		ofn.lpstrDefExt = L"png";
		#else
		ofn.lpstrFilter = L"BMP (*.bmp)\0*.bmp\0\0";
		ofn.lpstrDefExt = L"bmp";
		#endif
		ofn.nFilterIndex = 1;
		ofn.lpstrFile = szFile;
		ofn.nMaxFile = countof(szFile);
		ofn.lpstrTitle = L"Save screenshot";
		ofn.Flags = OFN_ENABLESIZING|OFN_NOCHANGEDIR
		        | OFN_PATHMUSTEXIST|OFN_EXPLORER|OFN_HIDEREADONLY|OFN_OVERWRITEPROMPT;
		if (!GetSaveFileName(&ofn) || ((pszDot = (wchar_t*)PointToExt(szFile)) == NULL))
		{
			lbRc = TRUE; // чтобы не ругалось...
			goto wrap;
		}
		//wcscpy_c(szFile, szLastDumpFile);
	}

	#ifdef PNGDUMP
	if (SaveImageEx(szFile, pScreen, cbOut))
	{
		lbRc = TRUE;
		goto wrap;
	}
	#endif


	if (pScreen && cbOut)
	{
		if (pszDot)
		{
			#ifdef PNGDUMP
			lstrcpyW(pszDot, L".png");
			#else
			lstrcpyW(pszDot, L".bmp");
			#endif

			HANDLE hFile = CreateFile(szFile, GENERIC_WRITE, FILE_SHARE_READ, 0, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);

			if (hFile != INVALID_HANDLE_VALUE)
			{
				DWORD dwWritten = 0;
				lbRc = WriteFile(hFile, pScreen, cbOut, &dwWritten, 0);
				CloseHandle(hFile);
			}
		}
	}

wrap:
	SafeFree(pScreen);
	return lbRc;
}

BOOL DumpImage(HDC hScreen, HBITMAP hBitmap, int anWidth, int anHeight, LPCWSTR pszFile)
{
	return DumpImage(hScreen, hBitmap, 0, 0, anWidth, anHeight, pszFile);
}

BOOL DumpImage(BITMAPINFOHEADER* pHdr, LPVOID pBits, LPCWSTR pszFile)
{
	if (!pHdr || (pHdr->biBitCount != 32))
		return FALSE;

	BOOL lbRc = FALSE;

	int iMemSize = pHdr->biWidth * pHdr->biHeight * pHdr->biBitCount / 8;
	if (iMemSize < 0)
		return FALSE;

	HANDLE hFile = CreateFile(pszFile, GENERIC_WRITE, FILE_SHARE_READ, 0, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);

	if (hFile != INVALID_HANDLE_VALUE)
	{
		DWORD dwWritten = 0;

		BITMAPFILEHEADER bfh;
		bfh.bfType = 0x4D42; // 'MB';
		bfh.bfSize = sizeof(BITMAPFILEHEADER)+sizeof(BITMAPINFOHEADER)+iMemSize;
		bfh.bfReserved1=0;
		bfh.bfReserved2=0;
		bfh.bfOffBits=sizeof(BITMAPFILEHEADER)+sizeof(BITMAPINFOHEADER);

		lbRc =	WriteFile(hFile, &bfh, sizeof(bfh), &dwWritten, 0)
				&& WriteFile(hFile, pHdr, sizeof(*pHdr), &dwWritten, 0)
				&& WriteFile(hFile, pBits, iMemSize, &dwWritten, 0);

		CloseHandle(hFile);
	}

	return lbRc;
}
