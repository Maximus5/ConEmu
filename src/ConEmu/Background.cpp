﻿
/*
Copyright (c) 2009-2013 Maximus5
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
#include "header.h"
#include "Options.h"
#include "Background.h"
#include "ConEmu.h"
#include "LoadImg.h"
#include "VirtualConsole.h"
#include "../common/CmdLine.h"
#include "../common/MSection.h"

CBackground::CBackground()
{
	bgSize = MakeCoord(-1,-1);
	hBgDc = NULL;
	hBgBitmap = NULL;
	hOldBitmap = NULL;
	//// Alpha blending
	//mh_MsImg32 = LoadLibrary(L"Msimg32.dll");
	//if (mh_MsImg32) {
	//	fAlphaBlend = (AlphaBlend_t)GetProcAddress(mh_MsImg32, "AlphaBlend");
	//} else {
	//	fAlphaBlend = NULL;
	//}
	#ifdef __GNUC__
	HMODULE hGdi32 = GetModuleHandle(L"gdi32.dll");
	GdiAlphaBlend = (AlphaBlend_t)(hGdi32 ? GetProcAddress(hGdi32, "GdiAlphaBlend") : NULL);
	#endif

	mb_NeedBgUpdate = false; mb_BgLastFade = false;
	mp_BkImgData = NULL; mn_BkImgDataMax = 0; mb_BkImgChanged = FALSE; mb_BkImgExist = /*mb_BkImgDelete =*/ FALSE;
	mp_BkEmfData = NULL; mn_BkEmfDataMax = 0; mb_BkEmfChanged = FALSE;
	mcs_BkImgData = NULL;
	mn_BkImgWidth = mn_BkImgHeight = 0;
}

CBackground::~CBackground()
{
	Destroy();
	//if (mh_MsImg32)
	//{
	//	FreeLibrary(mh_MsImg32);
	//	fAlphaBlend = NULL;
	//}

	MSectionLock SC;

	if (mcs_BkImgData)
		SC.Lock(mcs_BkImgData, TRUE);

	SafeFree(mp_BkImgData);
	mn_BkImgDataMax = 0;

	SafeFree(mp_BkEmfData);
	mn_BkImgDataMax = 0;

	if (mcs_BkImgData)
	{
		SC.Unlock();
		SafeDelete(mcs_BkImgData);
	}
}

void CBackground::Destroy()
{
	if (hBgDc && hOldBitmap)
	{
		SelectObject(hBgDc, hOldBitmap);
		hOldBitmap = NULL;
	}

	if (hBgBitmap)
	{
		DeleteObject(hBgBitmap);
		hBgBitmap = NULL;
	}

	if (hBgDc)
	{
		DeleteDC(hBgDc);
		hBgDc = NULL;
	}
}

bool CBackground::CreateField(int anWidth, int anHeight)
{
	if (!hBgDc || !hBgBitmap || anWidth != bgSize.X || anHeight != bgSize.Y)
	{
		Destroy();
	}
	else
	{
		return true; // уже создано
	}

	bool lbRc = false;
	// Создать MemoryDC
	const HDC hScreenDC = GetDC(ghWnd);
	hBgDc = CreateCompatibleDC(hScreenDC);

	if (hBgDc)
	{
		bgSize.X = klMin(32767,anWidth);
		bgSize.Y = klMin(32767,anHeight);
		hBgBitmap = CreateCompatibleBitmap(hScreenDC, bgSize.X, bgSize.Y);

		if (hBgBitmap)
		{
			// Выбрать в MemoryDC созданный Bitmap для установки его размера
			hOldBitmap = (HBITMAP)SelectObject(hBgDc, hBgBitmap);
			// И залить черным фоном
			RECT rcFull = MakeRect(bgSize.X, bgSize.Y);
			FillRect(hBgDc, &rcFull, (HBRUSH)GetStockObject(BLACK_BRUSH));
			// теперь - OK
			lbRc = true;
		}
	}

	ReleaseDC(ghWnd, hScreenDC);
	return lbRc;
}

bool CBackground::FillBackground(
	const BITMAPFILEHEADER* apBkImgData, // Содержимое *.bmp файла
	LONG X, LONG Y, LONG Width, LONG Height, // Куда нужно положить картинку
	BackgroundOp Operation,              // {eUpLeft = 0, eStretch = 1, eTile = 2, eUpRight = 4, ...}
	bool abFade)                         // затемнение картинки, когда ConEmu НЕ в фокусе
{
	if (!hBgDc)
		return false;

	// Залить черным фоном
	RECT rcFull = MakeRect(X,Y,Width,Height);
	FillRect(hBgDc, &rcFull, (HBRUSH)GetStockObject(BLACK_BRUSH));

	if (apBkImgData == NULL ||
	        apBkImgData->bfType != 0x4D42/*BM*/ ||
	        IsBadReadPtr(apBkImgData, apBkImgData->bfSize))
	{
		return false;
	}

	bool lbRc = false;
	HDC         hLoadDC = NULL;
	HBITMAP     hLoadBmp = NULL;
	BITMAPINFO* pBmp  = (BITMAPINFO*)(apBkImgData+1);
	LPBYTE      pBits = ((LPBYTE)apBkImgData) + apBkImgData->bfOffBits;
	LPVOID      pDstBits = NULL;
	BITMAPINFOHEADER* pHdr = &pBmp->bmiHeader;

	if (pHdr->biPlanes != 1 || pHdr->biCompression != BI_RGB)  // BI_JPEG|BI_PNG
	{
		return false;
	}

	DWORD       nBitSize = apBkImgData->bfSize - apBkImgData->bfOffBits;
	TODO("Stride?");
	DWORD       nCalcSize = (pHdr->biWidth * pHdr->biHeight * pHdr->biBitCount) >> 3;

	if (nBitSize > nCalcSize)
		nBitSize = nCalcSize;

	if (!gpSet->isFadeInactive)
		abFade = false;

	// Создать MemoryDC
	const HDC hScreenDC = GetDC(ghWnd);

	if (hScreenDC)
	{
		hLoadDC = CreateCompatibleDC(hScreenDC);
		ReleaseDC(ghWnd, hScreenDC);

		if (hLoadDC)
		{
			hLoadBmp = CreateDIBSection(hLoadDC, pBmp, DIB_RGB_COLORS, &pDstBits, NULL, 0);

			if (hLoadBmp && pDstBits)
			{
				// Поместить биты из apBkImgData в hLoadDC
				HBITMAP hOldLoadBmp = (HBITMAP)SelectObject(hLoadDC, hLoadBmp);
				memmove(pDstBits, pBits, nBitSize);
				GdiFlush(); // Гарантировать commit битов
				// Теперь - скопировать биты из hLoadDC в hBgDc с учетом положения и Operation
				BLENDFUNCTION bf = {AC_SRC_OVER, 0, gpSet->bgImageDarker, 0};

				if (abFade)
				{
					// GetFadeColor возвращает ColorRef, поэтому при вызове для (0..255)
					// он должен вернуть "коэффициент" затемнения или осветления
					DWORD nHigh = (gpSet->GetFadeColor(255) & 0xFF);

					if (nHigh < 255)
					{
						// Затемнение фона
						bf.SourceConstantAlpha = nHigh * bf.SourceConstantAlpha / 255;
					}

					//// "коэффициент" вернется в виде RGB (R==G==B)
					//DWORD nLow = gpSet->GetFadeColor(0);
					//if (nLow > 0 && ((nLow & 0xFF) < nHigh))
					//{
					//	// Осветление фона
					//	RECT r = {X,Y,X+Width,Y+Height};
					//	HBRUSH h = CreateSolidBrush(nLow);
					//	FillRect(hBgDc, &r, h);
					//	DeleteObject(h);
					//	// еще нужно убедиться, что сама картинка будет немного прозрачной,
					//	// чтобы это осветление было заметно
					//	if ((nLow & 0xFF) < 200)
					//		bf.SourceConstantAlpha = klMin((int)bf.SourceConstantAlpha, (int)(255 - (nLow & 0xFF)));
					//	else if (bf.SourceConstantAlpha >= 240)
					//		bf.SourceConstantAlpha = 240;
					//}
				}

				if ((Operation == eUpLeft) || (Operation == eUpRight)
					|| (Operation == eDownLeft) || (Operation == eDownRight)
					|| (Operation == eCenter))
				{
					int W = klMin(Width,pHdr->biWidth); int H = klMin(Height,pHdr->biHeight);

					if (GdiAlphaBlend(hBgDc, X, Y, W, H, hLoadDC, 0, 0, W, H, bf))
						lbRc = true;
				}
				else if (Operation == eStretch || Operation == eFit)
				{
					if (GdiAlphaBlend(hBgDc, X, Y, Width, Height, hLoadDC, 0, 0, pHdr->biWidth, pHdr->biHeight, bf))
						lbRc = true;
				}
				else if (Operation == eFill)
				{
					int srcX = 0, srcY = 0, srcW = pHdr->biWidth, srcH = pHdr->biHeight;
					if (Width && Width > Height)
					{
						srcH = klMin((srcW * Height / Width), _abs(pHdr->biHeight));
						srcY = (pHdr->biHeight - srcH) / 2;
					}
					else if (Height)
					{
						srcW = klMin((srcH * Width / Height), pHdr->biWidth);
						srcX = (pHdr->biWidth - srcW) / 2;
					}

					if (GdiAlphaBlend(hBgDc, X, Y, Width, Height, hLoadDC, srcX, srcY, srcW, srcH, bf))
						lbRc = true;
				}
				else if (Operation == eTile)
				{
					for (int DY = Y; DY < (Y+Height); DY += pHdr->biHeight)
					{
						for (int DX = X; DX < (X+Width); DX += pHdr->biWidth)
						{
							int W = klMin((Width-DX),pHdr->biWidth);
							int H = klMin((Height-DY),pHdr->biHeight);

							if (GdiAlphaBlend(hBgDc, DX, DY, W, H, hLoadDC, 0, 0, W, H, bf))
								lbRc = true;
						}
					}
				}

				_ASSERTE(lbRc && "GdiAlphaBlend failed in background creation?");

				TODO("Осветление картинки в Fade, когда gpSet->mn_FadeLow>0");
				//if (abFade)
				//{
				//	// "коэффициент" вернется в виде RGB (R==G==B)
				//	DWORD nLow = gpSet->GetFadeColor(0);
				//	if (nLow)
				//	{
				//		// Осветление фона
				//		RECT r = {X,Y,X+Width,Y+Height};
				//		HBRUSH h = CreateSolidBrush(nLow);
				//		// Осветлить картинку
				//		//FillRect(hBgDc, &r, h);
				//		DeleteObject(h);
				//	}
				//}
				SelectObject(hLoadDC, hOldLoadBmp);
			}

			if (hLoadBmp)
			{
				DeleteObject(hLoadBmp);
				hLoadBmp = NULL;
			}

			DeleteDC(hLoadDC);
			hLoadDC = NULL;
		}
	}

	return lbRc;
}


// вызывается из SetBackgroundImageData
//   при получении нового Background (CECMD_SETBACKGROUND) из плагина
//   при очистке (закрытие/рестарт) консоли
UINT CBackground::IsBackgroundValid(const CESERVER_REQ_SETBACKGROUND* apImgData, bool* rpIsEmf)
{
	if (rpIsEmf)
		*rpIsEmf = false;

	if (!apImgData)
		return 0;

	//if (IsBadReadPtr(apImgData, sizeof(CESERVER_REQ_SETBACKGROUND)))
	//	return 0;
	if (!apImgData->bEnabled)
		return (UINT)sizeof(CESERVER_REQ_SETBACKGROUND);

	if (apImgData->bmp.bfType == 0x4D42/*BM*/ && apImgData->bmp.bfSize)
	{
		#ifdef _DEBUG
		if (IsBadReadPtr(&apImgData->bmp, apImgData->bmp.bfSize))
		{
			_ASSERTE(!IsBadReadPtr(&apImgData->bmp, apImgData->bmp.bfSize));
			return 0;
		}
		#endif

		LPBYTE pBuf = (LPBYTE)&apImgData->bmp;

		if (*(u32*)(pBuf + 0x0A) >= 0x36 && *(u32*)(pBuf + 0x0A) <= 0x436 && *(u32*)(pBuf + 0x0E) == 0x28 && !pBuf[0x1D] && !*(u32*)(pBuf + 0x1E))
		{
			//UINT nSize = (UINT)sizeof(CESERVER_REQ_SETBACKGROUND) - (UINT)sizeof(apImgData->bmp)
			//           + apImgData->bmp.bfSize;
			UINT nSize = apImgData->bmp.bfSize
			             + (((LPBYTE)(void*)&(apImgData->bmp)) - ((LPBYTE)apImgData));
			return nSize;
		}
	}
	else if (apImgData->bmp.bfType == 0x4645/*EF*/ && apImgData->bmp.bfSize)
	{
		#ifdef _DEBUG
		if (IsBadReadPtr(&apImgData->bmp, apImgData->bmp.bfSize))
		{
			_ASSERTE(!IsBadReadPtr(&apImgData->bmp, apImgData->bmp.bfSize));
			return 0;
		}
		#endif

		// Это EMF, но передан как BITMAPINFOHEADER
		if (rpIsEmf)
			*rpIsEmf = true;

		#ifdef _DEBUG
		LPBYTE pBuf = (LPBYTE)&apImgData->bmp;
		#endif

		UINT nSize = apImgData->bmp.bfSize
		             + (((LPBYTE)(void*)&(apImgData->bmp)) - ((LPBYTE)apImgData));
		return nSize;
	}

	return 0;
}

// вызывается при получении нового Background (CECMD_SETBACKGROUND) из плагина
// и для очистки при закрытии (рестарте) консоли
SetBackgroundResult CBackground::SetPluginBackgroundImageData(CESERVER_REQ_SETBACKGROUND* apImgData, bool&/*OUT*/ bUpdate)
{
	if (!this) return esbr_Unexpected;

	//if (!isMainThread())
	//{

	//// При вызове из серверной нити (только что пришло из плагина)
	//if (mp_RCon->isConsoleClosing())
	//	return esbr_ConEmuInShutdown;

	bool bIsEmf = false;
	UINT nSize = IsBackgroundValid(apImgData, &bIsEmf);

	if (!nSize)
	{
		_ASSERTE(FALSE && "!IsBackgroundValid(apImgData, NULL)");
		return esbr_InvalidArg;
	}

	if (!apImgData->bEnabled)
	{
		//mb_BkImgDelete = TRUE;
		mb_BkImgExist = FALSE;
		NeedBackgroundUpdate();
		//Update(true/*bForce*/);
		bUpdate = true;
		return gpSet->isBgPluginAllowed ? esbr_OK : esbr_PluginForbidden;
	}

#ifdef _DEBUG

	if ((GetKeyState(VK_SCROLL) & 1))
	{
		static UINT nBackIdx = 0;
		wchar_t szFileName[32];
		_wsprintf(szFileName, SKIPLEN(countof(szFileName)) L"PluginBack_%04u.bmp", nBackIdx++);
		char szAdvInfo[512];
		BITMAPINFOHEADER* pBmp = (BITMAPINFOHEADER*)((&apImgData->bmp)+1);
		_wsprintfA(szAdvInfo, SKIPLEN(countof(szAdvInfo)) "\r\nnType=%i, bEnabled=%i,\r\nWidth=%i, Height=%i, Bits=%i, Encoding=%i\r\n",
		           apImgData->nType, apImgData->bEnabled,
		           pBmp->biWidth, pBmp->biHeight, pBmp->biBitCount, pBmp->biCompression);
		HANDLE hFile = CreateFile(szFileName, GENERIC_WRITE, FILE_SHARE_READ, 0, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);

		if (hFile != INVALID_HANDLE_VALUE)
		{
			DWORD cbWrite;
			WriteFile(hFile, &apImgData->bmp, apImgData->bmp.bfSize, &cbWrite, 0);
			WriteFile(hFile, szAdvInfo, lstrlenA(szAdvInfo), &cbWrite, 0);
			CloseHandle(hFile);
		}
	}

#endif

	//	// Поскольку вызов асинхронный (сразу возвращаем в плагин), то нужно сделать копию данных
	//	CESERVER_REQ_SETBACKGROUND* pCopy = (CESERVER_REQ_SETBACKGROUND*)malloc(nSize);
	//	if (!pCopy)
	//		return esbr_Unexpected;
	//	memmove(pCopy, apImgData, nSize);
	//	// Запомнить последний актуальный, и послать в главную нить
	//	mp_LastImgData = pCopy;
	//	mb_BkImgDelete = FALSE;
	//	gpConEmu->PostSetBackground(this, pCopy);
	//	return gpSet->isBgPluginAllowed ? esbr_OK : esbr_PluginForbidden;
	//}

	//// Если вызов пришел во время закрытия консоли - игнорировать
	//if (mp_RCon->isConsoleClosing())
	////// Этот apImgData уже не актуален. Во время обработки сообщения пришел новый Background.
	////	|| (mp_LastImgData && mp_LastImgData != apImgData))
	//{
	//	free(apImgData);
	//	return esbr_Unexpected;
	//}

	// Ссылку на актуальный - не сбрасываем. Она просто информационная, и есть возможность наколоться с многопоточностью
	//mp_LastImgData = NULL;

	//UINT nSize = IsBackgroundValid(apImgData);
	//if (!nSize)
	//{
	//	// Не допустимый apImgData. Вроде такого быть не должно - все уже проверено
	//	_ASSERTE(IsBackgroundValid(apImgData) != 0);
	//	//free(apImgData);
	//	return esbr_InvalidArg;
	//}

	//MSectionLock SBK; SBK.Lock(&csBkImgData);
	//_ASSERTE(isMainThread());

	if (!mcs_BkImgData)
		mcs_BkImgData = new MSection();

	MSectionLock SC;
	SC.Lock(mcs_BkImgData, TRUE);

	if (bIsEmf)
	{
		if (!mp_BkEmfData || mn_BkEmfDataMax < nSize)
		{
			if (mp_BkEmfData)
			{
				free(mp_BkEmfData); mp_BkEmfData = NULL;
				mb_BkImgChanged = mb_BkEmfChanged = TRUE;
				mb_BkImgExist = FALSE;
				mn_BkImgWidth = mn_BkImgHeight = 0;
			}

			mn_BkEmfDataMax = nSize+8192;
			mp_BkEmfData = (CESERVER_REQ_SETBACKGROUND*)malloc(mn_BkEmfDataMax);
		}
	}
	else
	{
		if (!mp_BkImgData || mn_BkImgDataMax < nSize)
		{
			if (mp_BkImgData)
			{
				free(mp_BkImgData); mp_BkImgData = NULL;
				mb_BkImgChanged = TRUE;
				mb_BkImgExist = FALSE;
				mn_BkImgWidth = mn_BkImgHeight = 0;
			}

			mp_BkImgData = (CESERVER_REQ_SETBACKGROUND*)malloc(nSize);
		}
	}

	SetBackgroundResult rc;

	if (!(bIsEmf ? mp_BkEmfData : mp_BkImgData))
	{
		_ASSERTE((bIsEmf ? mp_BkEmfData : mp_BkImgData)!=NULL);
		rc = esbr_Unexpected;
	}
	else
	{
		if (bIsEmf)
			memmove(mp_BkEmfData, apImgData, nSize);
		else
			memmove(mp_BkImgData, apImgData, nSize);
		mb_BkImgChanged = TRUE;
		mb_BkEmfChanged = bIsEmf;
		mb_BkImgExist = TRUE;
		BITMAPINFOHEADER* pBmp = bIsEmf ? (&mp_BkEmfData->bi) : (&mp_BkImgData->bi);
		mn_BkImgWidth = pBmp->biWidth;
		mn_BkImgHeight = pBmp->biHeight;
		NeedBackgroundUpdate();

		//// Это была копия данных - нужно освободить
		//free(apImgData); apImgData = NULL;

		if (/*gpConEmu->isVisible(this) &&*/ gpSet->isBgPluginAllowed)
		{
			//Update(true/*bForce*/);
			bUpdate = true;
		}

		rc = esbr_OK;
	}

	return rc;
}

void CBackground::NeedBackgroundUpdate()
{
	if (!this)
	{
		_ASSERTE(this!=NULL);
		return;
	}

	mb_NeedBgUpdate = true;
}

// Создает (или возвращает уже созданный) HDC (CompatibleDC) для mp_BkImgData
bool CBackground::PutPluginBackgroundImage(/*CBackground* pBack,*/ LONG X, LONG Y, LONG Width, LONG Height)
{
	if (!this) return NULL;

	_ASSERTE(isMainThread());

	// Сразу
	mb_BkImgChanged = FALSE;

	/*if (mb_BkImgDelete && mp_BkImgData)
	{
		free(mp_BkImgData); mp_BkImgData = NULL;
		mb_BkImgExist = FALSE;
		return false;
	}*/
	if (!mb_BkImgExist)
		return false;

	MSectionLock SC;
	SC.Lock(mcs_BkImgData, FALSE);

	if (mb_BkEmfChanged)
	{
		// Сразу сброс
		mb_BkEmfChanged = FALSE;

		if (!mp_BkEmfData)
		{
			_ASSERTE(mp_BkEmfData!=NULL);
			return false;
		}

		// Нужно перекинуть EMF в mp_BkImgData
		BITMAPINFOHEADER bi = mp_BkEmfData->bi;
		size_t nBitSize = bi.biWidth*bi.biHeight*sizeof(COLORREF);
		size_t nWholeSize = sizeof(CESERVER_REQ_SETBACKGROUND)+nBitSize; //-V103 //-V119
		if (!mp_BkImgData || (mn_BkImgDataMax < nWholeSize))
		{
			if (mp_BkImgData)
				free(mp_BkImgData);
			mp_BkImgData = (CESERVER_REQ_SETBACKGROUND*)malloc(nWholeSize);
			if (!mp_BkImgData)
			{
				_ASSERTE(mp_BkImgData!=NULL);
				return false;
			}
		}

		*mp_BkImgData = *mp_BkEmfData;
		mp_BkImgData->bmp.bfType = 0x4D42/*BM*/;
		mp_BkImgData->bmp.bfSize = nBitSize+sizeof(BITMAPFILEHEADER)+sizeof(BITMAPINFOHEADER); //-V119

		// Теперь нужно сформировать DIB и нарисовать в нем EMF
		HDC hScreen = GetDC(NULL);
		//RECT rcMeta = {0,0, mn_BkImgWidth, mn_BkImgHeight}; // (in pixels)
		//RECT rcMetaMM = {0,0, mn_BkImgWidth*10, mn_BkImgHeight*10}; // (in .01-millimeter units)
		//HDC hdcEmf = CreateEnhMetaFile(NULL, NULL, &rcMetaMM, L"ConEmu\0Far Background\0\0");
		//if (!hdcEmf)
		//{
		//	_ASSERTE(hdcEmf!=NULL);
		//	return;
		//}

		HDC hdcDib = CreateCompatibleDC(hScreen);
		if (!hdcDib)
		{
			_ASSERTE(hdcDib!=NULL);
			//DeleteEnhMetaFile(hdcEmf);
			return false;
		}
		COLORREF* pBits = NULL;
		HBITMAP hDib = CreateDIBSection(hScreen, (BITMAPINFO*)&bi, DIB_RGB_COLORS, (void**)&pBits, NULL, 0);
		ReleaseDC(NULL, hScreen); hScreen = NULL;
		if (!hDib || !pBits)
		{
			_ASSERTE(hDib && pBits);
			return false;
		}

		HBITMAP hOld = (HBITMAP)SelectObject(hdcDib, hDib);

		size_t nBitsSize = bi.biWidth*bi.biHeight*sizeof(COLORREF);
		// Залить черным - по умолчанию
		#ifdef _DEBUG
			memset(pBits, 128, nBitSize);
		#else
			memset(pBits, 0, nBitsSize);
		#endif

		DWORD nEmfBits = mp_BkEmfData->bmp.bfSize - sizeof(BITMAPFILEHEADER) - sizeof(BITMAPINFOHEADER);
		LPBYTE pEmfBits = (LPBYTE)(mp_BkEmfData+1);
		HENHMETAFILE hdcEmf = SetEnhMetaFileBits(nEmfBits, pEmfBits);
		RECT rcPlay = {0,0, bi.biWidth, bi.biHeight};
		DWORD nPlayErr = 0;
		if (hdcEmf)
		{
			#ifdef _DEBUG
			ENHMETAHEADER	emh = {0};
			DWORD			PixelsX, PixelsY, MMX, MMY, cx, cy;
			emh.nSize = sizeof(ENHMETAHEADER);
			if( GetEnhMetaFileHeader( hdcEmf, sizeof( ENHMETAHEADER ), &emh ) )
			{
				// Get the characteristics of the output device
				HDC hDC = GetDC(NULL);
				PixelsX = GetDeviceCaps( hDC, HORZRES );
				PixelsY = GetDeviceCaps( hDC, VERTRES );
				MMX = GetDeviceCaps( hDC, HORZSIZE ) * 100;
				MMY = GetDeviceCaps( hDC, VERTSIZE ) * 100;
				ReleaseDC(NULL, hDC);

				// Calculate the rect in which to draw the metafile based on the
				// intended size and the current output device resolution
				// Remember that the intended size is given in 0.01mm units, so
				// convert those to device units on the target device
				cx = (int)((float)(emh.rclFrame.right - emh.rclFrame.left) * PixelsX / (MMX));
				cy = (int)((float)(emh.rclFrame.bottom - emh.rclFrame.top) * PixelsY / (MMY));
				//pw->PreferredSize.cx = ip.MulDivI32((emh.rclFrame.right - emh.rclFrame.left), PixelsX, MMX);
				//pw->PreferredSize.cy = ip.MulDivI32((emh.rclFrame.bottom - emh.rclFrame.top), PixelsY, MMY);
				_ASSERTE(cx>0 && cy>0);
				//if (pw->PreferredSize.cx < 0) pw->PreferredSize.cx = -pw->PreferredSize.cx;
				//if (pw->PreferredSize.cy < 0) pw->PreferredSize.cy = -pw->PreferredSize.cy;
				//rcPlay = MakeRect(emh.rclBounds.left,emh.rclBounds.top,emh.rclBounds.right,emh.rclBounds.bottom);
			}
			#endif

			if (!PlayEnhMetaFile(hdcDib, hdcEmf, &rcPlay))
			{
				nPlayErr = GetLastError();
				_ASSERTE(FALSE && (nPlayErr == 0));
			}

			GdiFlush();
			memmove(mp_BkImgData+1, pBits, nBitSize);
		}
		UNREFERENCED_PARAMETER(nPlayErr);

		SelectObject(hdcDib, hOld);
		DeleteObject(hDib);
		DeleteDC(hdcDib);
		if (hdcEmf)
		{
			DeleteEnhMetaFile(hdcEmf);
		}
		else
		{
			return false;
		}
	}

	if (!mp_BkImgData)
	{
		// Нужен ли тут? Или допустимая ситуация?
		_ASSERTE(mp_BkImgData!=NULL);
		return false;
	}

	bool lbFade = false;

	if (gpSet->isFadeInactive && !gpConEmu->isMeForeground(false))
		lbFade = true;

	bool lbRc = FillBackground(&mp_BkImgData->bmp, X, Y, Width, Height, eUpLeft, lbFade);

	//mb_BkImgChanged = FALSE;
	return lbRc;
}

// Должна вернуть true, если данные изменились (то есть будет isForce при полной перерисовке)
bool CBackground::PrepareBackground(CVirtualConsole* pVCon, HDC&/*OUT*/ phBgDc, COORD&/*OUT*/ pbgBmpSize)
{
	if (!this)
	{
		_ASSERTE(this!=NULL);
		return false;
	}

	_ASSERTE(isMainThread() && "Must be executed in main thread");

	bool bSucceeded = true;
	bool lbForceUpdate = false;
	LONG lBgWidth = 0, lBgHeight = 0;
	BOOL lbVConImage = FALSE;

	if (gpSet->isBgPluginAllowed)
	{
		if (HasPluginBackgroundImage(&lBgWidth, &lBgHeight)
			&& lBgWidth && lBgHeight)
		{
			lbVConImage = TRUE;
		}
	}

	LONG lMaxBgWidth = 0, lMaxBgHeight = 0;
	bool bIsForeground = gpConEmu->isMeForeground(true);


	// Если плагин свой фон не подсунул
	if (!lbVConImage)
	{
		//if (mp_PluginBg)
		//{
		//	delete mp_PluginBg;
		//	mp_PluginBg = NULL;
		//}

		#ifndef APPDISTINCTBACKGROUND
		// То работаем на общих основаниях, через настройки (или AppDistinct)
		return gpSetCls->PrepareBackground(pVCon, &phBgDc, &pbgBmpSize);
		#else

		CBackgroundInfo* pBgFile = pVCon->GetBackgroundObject();
		if (!pBgFile)
		{
			_ASSERTE(FALSE && "Background object must be created in VCon");
			return false;
		}

		if (!mb_NeedBgUpdate)
		{
			if ((hBgDc == NULL)
				|| (mb_BgLastFade == bIsForeground && gpSet->isFadeInactive)
				|| (!gpSet->isFadeInactive && mb_BgLastFade))
			{
				NeedBackgroundUpdate();
			}
		}

		pBgFile->PollBackgroundFile();

		RECT rcWork = {0,0,(int)pVCon->Width,(int)pVCon->Height};

		// необходимо проверить размер требуемой картинки
		// -- здесь - всегда только файловая подложка
		if ((gpSet->bgOperation == eUpLeft) || (gpSet->bgOperation == eUpRight)
			|| (gpSet->bgOperation == eDownLeft) || (gpSet->bgOperation == eDownRight)
			|| (gpSet->bgOperation == eCenter))
		{
			// MemoryDC создается всегда по размеру картинки, т.е. изменение размера окна - игнорируется
		}
		else
		{
			// Смотрим дальше
			if (gpSet->bgOperation == eStretch)
			{
				// Строго по размеру клиентской (точнее Workspace) области окна
				lMaxBgWidth = rcWork.right - rcWork.left;
				lMaxBgHeight = rcWork.bottom - rcWork.top;
			}
			else if (gpSet->bgOperation == eFit || gpSet->bgOperation == eFill)
			{
				lMaxBgWidth = rcWork.right - rcWork.left;
				lMaxBgHeight = rcWork.bottom - rcWork.top;
				// Correct aspect ratio
				const BITMAPFILEHEADER* pBgImgData = pBgFile->GetBgImgData();
				const BITMAPINFOHEADER* pBmp = pBgImgData ? (const BITMAPINFOHEADER*)(pBgImgData+1) : NULL;
				if (pBmp
					&& (rcWork.bottom - rcWork.top) > 0 && (rcWork.right - rcWork.left) > 0)
				{
					double ldVCon = (rcWork.right - rcWork.left) / (double)(rcWork.bottom - rcWork.top);
					double ldImg = pBmp->biWidth / (double)pBmp->biHeight;
					if (ldVCon > ldImg)
					{
						if (gpSet->bgOperation == eFit)
							lMaxBgWidth = lMaxBgHeight * ldImg;
						else
							lMaxBgHeight = lMaxBgWidth / ldImg;
					}
					else
					{
						if (gpSet->bgOperation == eFill)
							lMaxBgWidth = lMaxBgHeight * ldImg;
						else
							lMaxBgHeight = lMaxBgWidth / ldImg;
					}
				}
			}
			else if (gpSet->bgOperation == eTile)
			{
				// Max между клиентской (точнее Workspace) областью окна и размером текущего монитора
				// Окно может быть растянуто на несколько мониторов, т.е. размер клиентской области может быть больше
				HMONITOR hMon = MonitorFromWindow(ghWnd, MONITOR_DEFAULTTONEAREST);
				MONITORINFO mon = {sizeof(MONITORINFO)};
				GetMonitorInfo(hMon, &mon);
				//
				lMaxBgWidth = klMax(rcWork.right - rcWork.left,mon.rcMonitor.right - mon.rcMonitor.left);
				lMaxBgHeight = klMax(rcWork.bottom - rcWork.top,mon.rcMonitor.bottom - mon.rcMonitor.top);
			}

			if (bgSize.X != lMaxBgWidth || bgSize.Y != lMaxBgHeight)
				NeedBackgroundUpdate();
		}

		if (mb_NeedBgUpdate)
		{
			mb_NeedBgUpdate = false;
			lbForceUpdate = true;
			_ASSERTE(isMainThread());
			//MSectionLock SBG; SBG.Lock(&mcs_BgImgData);
			//BITMAPFILEHEADER* pImgData = mp_BgImgData;
			BackgroundOp op = (BackgroundOp)gpSet->bgOperation;
			const BITMAPFILEHEADER* pBgImgData = pBgFile->GetBgImgData();
			BOOL lbImageExist = (pBgImgData != NULL);
			//BOOL lbVConImage = FALSE;
			//LONG lBgWidth = 0, lBgHeight = 0;
			//CVirtualConsole* pVCon = gpConEmu->ActiveCon();

			////MSectionLock SBK;
			//if (apVCon && gpSet->isBgPluginAllowed)
			//{
			//	//SBK.Lock(&apVCon->csBkImgData);
			//	if (apVCon->HasBackgroundImage(&lBgWidth, &lBgHeight)
			//	        && lBgWidth && lBgHeight)
			//	{
			//		lbVConImage = lbImageExist = TRUE;
			//	}
			//}

			//mb_WasVConBgImage = lbVConImage;

			if (lbImageExist)
			{
				mb_BgLastFade = (!bIsForeground && gpSet->isFadeInactive);
				TODO("Переделать, ориентироваться только на размер картинки - неправильно");
				TODO("DoubleView - скорректировать X,Y");

				//if (lbVConImage)
				//{
				//	if (lMaxBgWidth && lMaxBgHeight)
				//	{
				//		lBgWidth = lMaxBgWidth;
				//		lBgHeight = lMaxBgHeight;
				//	}

				//	if (!mp_Bg->CreateField(lBgWidth, lBgHeight) ||
				//	        !apVCon->PutBackgroundImage(mp_Bg, 0,0, lBgWidth, lBgHeight))
				//	{
				//		delete mp_Bg;
				//		mp_Bg = NULL;
				//	}
				//}
				//else
				{
					const BITMAPINFOHEADER* pBmp = (const BITMAPINFOHEADER*)(pBgImgData+1);

					if (!lMaxBgWidth || !lMaxBgHeight)
					{
						// Сюда мы можем попасть только в случае eUpLeft/eUpRight/eDownLeft/eDownRight
						lMaxBgWidth = pBmp->biWidth;
						lMaxBgHeight = pBmp->biHeight;
					}

					//LONG lImgX = 0, lImgY = 0, lImgW = lMaxBgWidth, lImgH = lMaxBgHeight;

					//if ((gpSet->bgOperation == eFit || gpSet->bgOperation == eFill)
					//	&& (rcWork.bottom - rcWork.top) > 0 && (rcWork.right - rcWork.left) > 0)
					//{
					//	double ldVCon = (rcWork.right - rcWork.left) / (double)(rcWork.bottom - rcWork.top);
					//	double ldImg = pBmp->biWidth / (double)pBmp->biHeight;
					//	if (ldVCon > ldImg)
					//	{
					//		if (gpSet->bgOperation == eFit)
					//			lMaxBgWidth = lMaxBgHeight * ldImg;
					//		else
					//			lMaxBgHeight = lMaxBgWidth / ldImg;
					//	}
					//	else
					//	{
					//		if (gpSet->bgOperation == eFill)
					//			lMaxBgWidth = lMaxBgHeight * ldImg;
					//		else
					//			lMaxBgHeight = lMaxBgWidth / ldImg;
					//	}
					//}

					if (!CreateField(lMaxBgWidth, lMaxBgHeight) ||
						!FillBackground(pBgImgData, 0,0,lMaxBgWidth,lMaxBgHeight, op, mb_BgLastFade))
					{
						bSucceeded = false;
					}
				}
			}
			else
			{
				bSucceeded = false;
			}
		}

		pBgFile->Release();

		#endif
	}
	else
	{
		if (!mb_NeedBgUpdate)
		{
			if ((mb_BgLastFade == bIsForeground && gpSet->isFadeInactive)
				|| (!gpSet->isFadeInactive && mb_BgLastFade))
			{
				NeedBackgroundUpdate();
			}
		}

		//if (mp_PluginBg == NULL)
		//{
		//	NeedBackgroundUpdate();
		//}

		if (mb_NeedBgUpdate)
		{
			mb_NeedBgUpdate = false;
			lbForceUpdate = true;

			//if (!mp_PluginBg)
			//	mp_PluginBg = new CBackground;

			mb_BgLastFade = (!bIsForeground && gpSet->isFadeInactive);
			TODO("Переделать, ориентироваться только на размер картинки - неправильно");
			TODO("DoubleView - скорректировать X,Y");

			if (lMaxBgWidth && lMaxBgHeight)
			{
				lBgWidth = lMaxBgWidth;
				lBgHeight = lMaxBgHeight;
			}

			if (!CreateField(lBgWidth, lBgHeight) ||
				!PutPluginBackgroundImage(0,0, lBgWidth, lBgHeight))
			{
				//delete mp_PluginBg;
				//mp_PluginBg = NULL;
				bSucceeded = false;
			}
		}
	}

	if (bSucceeded)
	{
		phBgDc = hBgDc;
		pbgBmpSize = bgSize;
	}
	else
	{
		phBgDc = NULL;
		pbgBmpSize = MakeCoord(0,0);
	}

	return lbForceUpdate;
}

bool CBackground::HasPluginBackgroundImage(LONG* pnBgWidth, LONG* pnBgHeight)
{
	if (!this) return false;

	//if (!mp_RCon || !mp_RCon->isFar()) return false;

	if (!mb_BkImgExist || !(mp_BkImgData || (mp_BkEmfData && mb_BkEmfChanged))) return false;

	// Возвращаем mn_BkImgXXX чтобы не беспокоиться об указателе mp_BkImgData

	if (pnBgWidth)
		*pnBgWidth = mn_BkImgWidth;

	if (pnBgHeight)
		*pnBgHeight = mn_BkImgHeight;

	return (mn_BkImgWidth != 0 && mn_BkImgHeight != 0);
	//MSectionLock SBK; SBK.Lock(&csBkImgData);
	//if (mp_BkImgData)
	//{
	//	BITMAPINFOHEADER* pBmp = (BITMAPINFOHEADER*)(mp_BkImgData+1);
	//	if (pnBgWidth)
	//		*pnBgWidth = pBmp->biWidth;
	//	if (pnBgHeight)
	//		*pnBgHeight = pBmp->biHeight;
	//}
	//return mp_BkImgData;
}






/* ********************************************************************* */
/* ********************************************************************* */
/* ********************************************************************* */
/* ********************************************************************* */
/* ********************************************************************* */

#ifdef APPDISTINCTBACKGROUND

MArray<CBackgroundInfo*> CBackgroundInfo::g_Backgrounds;

CBackgroundInfo* CBackgroundInfo::CreateBackgroundObject(LPCWSTR inPath, bool abShowErrors)
{
	if (inPath && _tcslen(inPath)>=MAX_PATH)
	{
		if (abShowErrors)
			MBoxA(L"Invalid 'BgImagePath' in CBackgroundInfo::CreateBackgroundObject");
		return NULL;
	}

	// Допускается и пустой путь!
	inPath = SkipNonPrintable(inPath);
	if (!inPath)
		inPath = L"";

	CBackgroundInfo* p = NULL;

	for (INT_PTR i = 0; i < g_Backgrounds.size(); i++)
	{
		if (!g_Backgrounds[i])
		{
			_ASSERTE(g_Backgrounds[i]!=NULL);
			continue;
		}
		if (lstrcmpi(g_Backgrounds[i]->BgImage(), inPath) == 0)
		{
			p = g_Backgrounds[i];
			break;
		}
	}

	if (p)
	{
		p->AddRef();
	}
	else
	{
		p = new CBackgroundInfo(inPath);
		if (inPath && *inPath && !p->LoadBackgroundFile(abShowErrors))
		{
			SafeRelease(p);
		}
	}

	return p;
}

CBackgroundInfo::CBackgroundInfo(LPCWSTR inPath)
{
	wcscpy_c(ms_BgImage, inPath ? inPath : L"");

	ZeroStruct(ftBgModified);
	nBgModifiedTick = 0;
	//mb_IsFade = bFade;
	//mb_NeedBgUpdate = false;
	mb_IsBackgroundImageValid = false;
	mp_BgImgData = NULL;

	CBackgroundInfo* p = this;
	g_Backgrounds.push_back(p);
}

void CBackgroundInfo::FinalRelease()
{
	CBackgroundInfo* p = this;
	delete p;
}

CBackgroundInfo::~CBackgroundInfo()
{
	SafeFree(mp_BgImgData);

	CBackgroundInfo* p = this;
	for (INT_PTR i = 0; i < g_Backgrounds.size(); i++)
	{
		if (g_Backgrounds[i] == p)
		{
			g_Backgrounds.erase(i);
			break;
		}
	}
}

const wchar_t* CBackgroundInfo::BgImage()
{
	if (!this) return L"";
	return ms_BgImage;
}

//bool CBackgroundInfo::IsImageExist()
//{
//	if (!this) return false;
//	return (mp_BgImgData != NULL);
//}

const BITMAPFILEHEADER* CBackgroundInfo::GetBgImgData()
{
	if (!this) return NULL;
	return mp_BgImgData;
}

//bool CBackgroundInfo::IsFade()
//{
//	return mb_IsFade;
//}

//void CBackgroundInfo::NeedBackgroundUpdate()
//{
//	mb_NeedBgUpdate = true;
//}

bool CBackgroundInfo::PollBackgroundFile()
{
	bool lbChanged = false;

	if (gpSet->isShowBgImage && ms_BgImage[0] && ((GetTickCount() - nBgModifiedTick) > BACKGROUND_FILE_POLL)
		&& wcspbrk(ms_BgImage, L"%\\.")) // только для файлов!
	{
		WIN32_FIND_DATA fnd = {0};
		HANDLE hFind = FindFirstFile(gpSet->sBgImage, &fnd);

		if (hFind != INVALID_HANDLE_VALUE)
		{
			if (fnd.ftLastWriteTime.dwHighDateTime != ftBgModified.dwHighDateTime
			        || fnd.ftLastWriteTime.dwLowDateTime != ftBgModified.dwLowDateTime)
			{
				//NeedBackgroundUpdate(); -- поставит LoadBackgroundFile, если у него получится файл открыть
				lbChanged = LoadBackgroundFile(false);
				nBgModifiedTick = GetTickCount();
			}

			FindClose(hFind);
		}
	}

	return lbChanged;
}

bool CBackgroundInfo::LoadBackgroundFile(bool abShowErrors)
{
	// Пустой путь - значит БЕЗ обоев
	if (!*ms_BgImage)
	{
		return true;
	}

	//_ASSERTE(isMainThread());

	_ASSERTE(isMainThread());
	bool lRes = false;
	BY_HANDLE_FILE_INFORMATION inf = {0};
	BITMAPFILEHEADER* pBkImgData = NULL;

	if (wcspbrk(ms_BgImage, L"%\\.") == NULL)
	{
		// May be "Solid color"
		COLORREF clr = (COLORREF)-1;
		if (GetColorRef(ms_BgImage, &clr))
		{
			pBkImgData = CreateSolidImage(clr, 128, 128);
		}
	}

	if (!pBkImgData)
	{
		wchar_t* exPath = ExpandEnvStr(ms_BgImage);

		if (!exPath || !*exPath)
		{
			if (abShowErrors)
			{
				wchar_t szError[MAX_PATH*2];
				DWORD dwErr = GetLastError();
				_wsprintf(szError, SKIPLEN(countof(szError)) L"Can't expand environment strings:\r\n%s\r\nError code=0x%08X\r\nImage loading failed",
				          ms_BgImage, dwErr);
				MBoxA(szError);
			}

			SafeFree(exPath);
			return false;
		}

		pBkImgData = LoadImageEx(exPath, inf);
		SafeFree(exPath);
	}

	if (pBkImgData)
	{
		ftBgModified = inf.ftLastWriteTime;
		nBgModifiedTick = GetTickCount();
		//NeedBackgroundUpdate();
		//MSectionLock SBG; SBG.Lock(&mcs_BgImgData);
		SafeFree(mp_BgImgData);
		mb_IsBackgroundImageValid = true;
		mp_BgImgData = pBkImgData;
		lRes = true;
	}

	return lRes;
}
#endif
