
/*
Copyright (c) 2009-2012 Maximus5
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
    BackgroundOp Operation,              // {eUpLeft = 0, eStretch = 1, eTile = 2, eUpRight = 4}
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
					|| (Operation == eDownLeft) || (Operation == eDownRight))
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








/* ********************************************************************* */
/* ********************************************************************* */
/* ********************************************************************* */
/* ********************************************************************* */
/* ********************************************************************* */

#ifdef APPDISTINCTBACKGROUND

PRAGMA_ERROR("Заменить gpSet на pApp и this");

CBackgroundFile::CBackgroundFile()
{
	sBgImage[0] = 0;
	ZeroStruct(ftBgModified);
	nBgModifiedTick = 0;
	mb_IsFade = false;
	mb_NeedBgUpdate = false;
}

CBackgroundFile::~CBackgroundFile()
{
}

const wchar_t* CBackgroundFile::BgImage()
{
	return sBgImage;
}

bool CBackgroundFile::IsFade()
{
	return mb_IsFade;
}

void CBackgroundFile::NeedBackgroundUpdate()
{
	mb_NeedBgUpdate = true;
}

bool CBackgroundFile::PollBackgroundFile()
{
	bool lbChanged = false;

	if (gpSet->isShowBgImage && sBgImage[0] && ((GetTickCount() - nBgModifiedTick) > BACKGROUND_FILE_POLL))
	{
		WIN32_FIND_DATA fnd = {0};
		HANDLE hFind = FindFirstFile(gpSet->sBgImage, &fnd);

		if (hFind != INVALID_HANDLE_VALUE)
		{
			if (fnd.ftLastWriteTime.dwHighDateTime != ftBgModified.dwHighDateTime
			        || fnd.ftLastWriteTime.dwLowDateTime != ftBgModified.dwLowDateTime)
			{
				//NeedBackgroundUpdate(); -- поставит LoadBackgroundFile, если у него получится файл открыть
				lbChanged = LoadBackgroundFile(gpSet->sBgImage, false);
				nBgModifiedTick = GetTickCount();
			}

			FindClose(hFind);
		}
	}

	return lbChanged;
}

bool CBackgroundFile::LoadBackgroundFile(const wchar_t* inPath, bool abShowErrors)
{
	//_ASSERTE(gpConEmu->isMainThread());
	if (!inPath || _tcslen(inPath)>=MAX_PATH)
	{
		if (abShowErrors)
			MBoxA(L"Invalid 'inPath' in Settings::LoadImageFrom");

		return false;
	}

	TCHAR exPath[MAX_PATH + 2];

	if (!ExpandEnvironmentStrings(inPath, exPath, MAX_PATH))
	{
		if (abShowErrors)
		{
			wchar_t szError[MAX_PATH*2];
			DWORD dwErr = GetLastError();
			_wsprintf(szError, SKIPLEN(countof(szError)) L"Can't expand environment strings:\r\n%s\r\nError code=0x%08X\r\nImage loading failed",
			          inPath, dwErr);
			MBoxA(szError);
		}

		return false;
	}

	_ASSERTE(gpConEmu->isMainThread());
	bool lRes = false;
	BY_HANDLE_FILE_INFORMATION inf = {0};
	BITMAPFILEHEADER* pBkImgData = LoadImageEx(exPath, inf);
	if (pBkImgData)
	{
		ftBgModified = inf.ftLastWriteTime;
		nBgModifiedTick = GetTickCount();
		NeedBackgroundUpdate();
		//MSectionLock SBG; SBG.Lock(&mcs_BgImgData);
		SafeFree(mp_BgImgData);
		isBackgroundImageValid = true;
		mp_BgImgData = pBkImgData;
		lRes = true;
	}
	
	return lRes;
}

// Должна вернуть true, если файл изменился
// Работает ТОЛЬКО с файлом. Данные плагинов обрабатываются в самом CVirtualConsole!
bool CBackgroundFile::PrepareBackground(HDC* phBgDc, COORD* pbgBmpSize)
{
	bool lbForceUpdate = false;
	LONG lMaxBgWidth = 0, lMaxBgHeight = 0;
	bool bIsForeground = gpConEmu->isMeForeground(false);

	if (!mb_NeedBgUpdate)
	{
		if ((mb_BgLastFade == bIsForeground && gpSet->isFadeInactive)
		        || (!gpSet->isFadeInactive && mb_BgLastFade))
		{
			NeedBackgroundUpdate();
		}
	}

	PollBackgroundFile();

	if (mp_Bg == NULL)
	{
		NeedBackgroundUpdate();
	}

	// Если это НЕ плагиновая подложка - необходимо проверить размер требуемой картинки
	// -- здесь - всегда только файловая подложка
	//if (!mb_WasVConBgImage)
	{
		if ((gpSet->bgOperation == eUpLeft) || (gpSet->bgOperation == eUpRight)
			|| (gpSet->bgOperation == eDownLeft) || (gpSet->bgOperation == eDownRight))
		{
			// MemoryDC создается всегда по размеру картинки, т.е. изменение размера окна - игнорируется
		}
		else
		{
			RECT rcWnd, rcWork; GetClientRect(ghWnd, &rcWnd);
			WARNING("DoubleView: тут непонятно, какой и чей размер, видимо, нужно ветвиться, и хранить Background в самих VCon");
			rcWork = gpConEmu->CalcRect(CER_BACK, rcWnd, CER_MAINCLIENT);

			// Смотрим дальше
			if (gpSet->bgOperation == eStretch)
			{
				// Строго по размеру клиентской (точнее Workspace) области окна
				lMaxBgWidth = rcWork.right - rcWork.left;
				lMaxBgHeight = rcWork.bottom - rcWork.top;
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

			if (mp_Bg)
			{
				if (mp_Bg->bgSize.X != lMaxBgWidth || mp_Bg->bgSize.Y != lMaxBgHeight)
					NeedBackgroundUpdate();
			}
			else
			{
				NeedBackgroundUpdate();
			}
		}
	}

	if (mb_NeedBgUpdate)
	{
		mb_NeedBgUpdate = FALSE;
		lbForceUpdate = true;
		_ASSERTE(gpConEmu->isMainThread());
		//MSectionLock SBG; SBG.Lock(&mcs_BgImgData);
		//BITMAPFILEHEADER* pImgData = mp_BgImgData;
		BackgroundOp op = (BackgroundOp)gpSet->bgOperation;
		BOOL lbImageExist = (mp_BgImgData != NULL);
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
			if (!mp_Bg)
				mp_Bg = new CBackground;

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
				BITMAPINFOHEADER* pBmp = (BITMAPINFOHEADER*)(mp_BgImgData+1);

				if (!lMaxBgWidth || !lMaxBgHeight)
				{
					// Сюда мы можем попасть только в случае eUpLeft/eUpRight/eDownLeft/eDownRight
					lMaxBgWidth = pBmp->biWidth;
					lMaxBgHeight = pBmp->biHeight;
				}

				if (!mp_Bg->CreateField(lMaxBgWidth, lMaxBgHeight) ||
				        !mp_Bg->FillBackground(mp_BgImgData, 0,0, lMaxBgWidth, lMaxBgHeight, op, mb_BgLastFade))
				{
					delete mp_Bg;
					mp_Bg = NULL;
				}
			}
		}
		else
		{
			delete mp_Bg;
			mp_Bg = NULL;
		}
	}

	if (mp_Bg)
	{
		*phBgDc = mp_Bg->hBgDc;
		*pbgBmpSize = mp_Bg->bgSize;
	}
	else
	{
		*phBgDc = NULL;
		*pbgBmpSize = MakeCoord(0,0);
	}

	return lbForceUpdate;
}

#endif
