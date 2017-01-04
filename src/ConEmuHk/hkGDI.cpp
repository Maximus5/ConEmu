
/*
Copyright (c) 2015-2017 Maximus5
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

#ifdef _DEBUG
	#define DebugString(x) //OutputDebugString(x)
#else
	#define DebugString(x) //OutputDebugString(x)
#endif

#include "../common/Common.h"

#include "hkGDI.h"
#include "MainThread.h"

/* **************** */

static HDC ghTempHDC = NULL;
static RECT StretchBltBatch = {}; // Support batches from GdipDrawImageRectRectI

/* **************** */

HDC WINAPI OnGetDC(HWND hWnd)
{
	//typedef HDC (WINAPI* OnGetDC_t)(HWND hWnd);
	ORIGINAL_EX(GetDC);
	HDC hDC = NULL;

	if (F(GetDC))
		hDC = F(GetDC)(hWnd);

	if (hDC && ghConEmuWndDC && hWnd == ghConEmuWndDC)
		ghTempHDC = hDC;

	return hDC;
}


HDC WINAPI OnGetDCEx(HWND hWnd, HRGN hrgnClip, DWORD flags)
{
	//typedef HDC (WINAPI* OnGetDCEx_t)(HWND hWnd, HRGN hrgnClip, DWORD flags);
	ORIGINAL_EX(GetDCEx);
	HDC hDC = NULL;

	if (F(GetDCEx))
		hDC = F(GetDCEx)(hWnd, hrgnClip, flags);

	if (hDC && ghConEmuWndDC && hWnd == ghConEmuWndDC)
		ghTempHDC = hDC;

	return hDC;
}


int WINAPI OnReleaseDC(HWND hWnd, HDC hDC)
{
	//typedef int (WINAPI* OnReleaseDC_t)(HWND hWnd, HDC hDC);
	ORIGINAL_EX(ReleaseDC);
	int iRc = 0;

	if (F(ReleaseDC))
		iRc = F(ReleaseDC)(hWnd, hDC);

	if (ghTempHDC == hDC)
		ghTempHDC = NULL;

	return iRc;
}


int WINAPI OnStretchDIBits(HDC hdc, int XDest, int YDest, int nDestWidth, int nDestHeight, int XSrc, int YSrc, int nSrcWidth, int nSrcHeight, const VOID *lpBits, const BITMAPINFO *lpBitsInfo, UINT iUsage, DWORD dwRop)
{
	//typedef int (WINAPI* OnStretchDIBits_t)(HDC hdc, int XDest, int YDest, int nDestWidth, int nDestHeight, int XSrc, int YSrc, int nSrcWidth, int nSrcHeight, const VOID *lpBits, const BITMAPINFO *lpBitsInfo, UINT iUsage, DWORD dwRop);
	ORIGINAL_EX(StretchDIBits);
	int iRc = 0;

	if (F(StretchDIBits))
		iRc = F(StretchDIBits)(hdc, XDest, YDest, nDestWidth, nDestHeight, XSrc, YSrc, nSrcWidth, nSrcHeight, lpBits, lpBitsInfo, iUsage, dwRop);

	// Если рисуют _прямо_ на канвасе ConEmu
	if (iRc != (int)GDI_ERROR && hdc && hdc == ghTempHDC)
	{
		// Уведомить GUI, что у него прямо на канвасе кто-то что-то нарисовал :)
		CESERVER_REQ* pIn = ExecuteNewCmd(CECMD_LOCKDC, sizeof(CESERVER_REQ_HDR)+sizeof(CESERVER_REQ_LOCKDC));
		if (pIn)
		{
			pIn->LockDc.hDcWnd = ghConEmuWndDC; // На всякий случай
			pIn->LockDc.bLock = TRUE;
			pIn->LockDc.Rect.left = XDest;
			pIn->LockDc.Rect.top = YDest;
			pIn->LockDc.Rect.right = XDest+nDestWidth-1;
			pIn->LockDc.Rect.bottom = YDest+nDestHeight-1;

			CESERVER_REQ* pOut = ExecuteGuiCmd(ghConWnd, pIn, ghConWnd);

			if (pOut)
				ExecuteFreeResult(pOut);
			ExecuteFreeResult(pIn);
		}
	}

	return iRc;
}


BOOL WINAPI OnBitBlt(HDC hdcDest, int nXDest, int nYDest, int nWidth, int nHeight, HDC hdcSrc, int nXSrc, int nYSrc, DWORD dwRop)
{
	//typedef int (WINAPI* OnBitBlt_t)(HDC hdcDest, int nXDest, int nYDest, int nWidth, int nHeight, HDC hdcSrc, int nXSrc, int nYSrc, DWORD dwRop);
	ORIGINAL_EX(BitBlt);
	BOOL bRc = FALSE;

	if (F(BitBlt))
		bRc = F(BitBlt)(hdcDest, nXDest, nYDest, nWidth, nHeight, hdcSrc, nXSrc, nYSrc, dwRop);

	// Если рисуют _прямо_ на канвасе ConEmu
	if (bRc && hdcDest && hdcDest == ghTempHDC)
	{
		// Уведомить GUI, что у него прямо на канвасе кто-то что-то нарисовал :)
		CESERVER_REQ* pIn = ExecuteNewCmd(CECMD_LOCKDC, sizeof(CESERVER_REQ_HDR)+sizeof(CESERVER_REQ_LOCKDC));
		if (pIn)
		{
			pIn->LockDc.hDcWnd = ghConEmuWndDC; // На всякий случай
			pIn->LockDc.bLock = TRUE;
			pIn->LockDc.Rect.left = nXDest;
			pIn->LockDc.Rect.top = nYDest;
			pIn->LockDc.Rect.right = nXDest+nWidth-1;
			pIn->LockDc.Rect.bottom = nYDest+nHeight-1;

			CESERVER_REQ* pOut = ExecuteGuiCmd(ghConWnd, pIn, ghConWnd);

			if (pOut)
				ExecuteFreeResult(pOut);
			ExecuteFreeResult(pIn);
		}
	}

	return bRc;
}


BOOL WINAPI OnStretchBlt(HDC hdcDest, int nXOriginDest, int nYOriginDest, int nWidthDest, int nHeightDest, HDC hdcSrc, int nXOriginSrc, int nYOriginSrc, int nWidthSrc, int nHeightSrc, DWORD dwRop)
{
	//typedef int (WINAPI* OnStretchBlt_t)(HDC hdcDest, int nXOriginDest, int nYOriginDest, int nWidthDest, int nHeightDest, HDC hdcSrc, int nXOriginSrc, int nYOriginSrc, int nWidthSrc, int nHeightSrc, DWORD dwRop);
	ORIGINAL_EX(StretchBlt);
	BOOL bRc = FALSE;

	//#ifdef _DEBUG
	//HWND h = WindowFromDC(hdcDest);
	//#endif

	// Если рисуют _прямо_ на канвасе ConEmu
	if (/*bRc &&*/ hdcDest && hdcDest == ghTempHDC)
	{
		if (
			(!StretchBltBatch.bottom && !StretchBltBatch.top)
			|| (nYOriginDest <= StretchBltBatch.top)
			|| (nXOriginDest != StretchBltBatch.left)
			|| (StretchBltBatch.right != (nXOriginDest+nWidthDest-1))
			|| (StretchBltBatch.bottom != (nYOriginDest-1))
			)
		{
			// Сброс батча
			StretchBltBatch.left = nXOriginDest;
			StretchBltBatch.top = nYOriginDest;
			StretchBltBatch.right = nXOriginDest+nWidthDest-1;
			StretchBltBatch.bottom = nYOriginDest+nHeightDest-1;
		}
		else
		{
			StretchBltBatch.bottom = nYOriginDest+nHeightDest-1;
		}

		// Уведомить GUI, что у него прямо на канвасе кто-то что-то нарисовал :)
		CESERVER_REQ* pIn = ExecuteNewCmd(CECMD_LOCKDC, sizeof(CESERVER_REQ_HDR)+sizeof(CESERVER_REQ_LOCKDC));
		if (pIn)
		{
			pIn->LockDc.hDcWnd = ghConEmuWndDC; // На всякий случай
			pIn->LockDc.bLock = TRUE;
			pIn->LockDc.Rect = StretchBltBatch;

			CESERVER_REQ* pOut = ExecuteGuiCmd(ghConWnd, pIn, ghConWnd);

			if (pOut)
				ExecuteFreeResult(pOut);
			ExecuteFreeResult(pIn);
		}
	}

	if (F(StretchBlt))
		bRc = F(StretchBlt)(hdcDest, nXOriginDest, nYOriginDest, nWidthDest, nHeightDest, hdcSrc, nXOriginSrc, nYOriginSrc, nWidthSrc, nHeightSrc, dwRop);

	return bRc;
}
