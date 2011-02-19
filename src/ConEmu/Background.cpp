
/*
Copyright (c) 2009-2010 Maximus5
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


#include "header.h"
#include "Options.h"
#include "Background.h"

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
}

CBackground::~CBackground()
{
	Destroy();
	//if (mh_MsImg32)
	//{
	//	FreeLibrary(mh_MsImg32);
	//	fAlphaBlend = NULL;
	//}
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
    BackgroundOp Operation,              // {eUpLeft = 0, eStretch = 1, eTile = 2}
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

				if (Operation == eUpLeft)
				{
					int W = klMin(Width,pHdr->biWidth); int H = klMin(Height,pHdr->biHeight);

					if (GdiAlphaBlend(hBgDc, X, Y, W, H, hLoadDC, 0, 0, W, H, bf))
						lbRc = true;
				}
				else if (Operation == eStretch)
				{
					if (GdiAlphaBlend(hBgDc, X, Y, Width, Height, hLoadDC, 0, 0, pHdr->biWidth, pHdr->biHeight, bf))
						lbRc = true;
				}
				else if (Operation == eTile)
				{
					for(int DY = Y; DY < (Y+Height); DY += pHdr->biHeight)
					{
						for(int DX = X; DX < (X+Width); DX += pHdr->biWidth)
						{
							int W = klMin((Width-DX),pHdr->biWidth);
							int H = klMin((Height-DY),pHdr->biHeight);

							if (GdiAlphaBlend(hBgDc, DX, DY, W, H, hLoadDC, 0, 0, W, H, bf))
								lbRc = true;
						}
					}
				}

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
