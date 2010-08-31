
#include "headers.h"
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
		TODO("Залить черным не нужно?");
		return true; // уже создано
	}
	
	bool lbRc = false;
	
	// Создать MemoryDC
    const HDC hScreenDC = GetDC(ghWnd);
    hBgDc = CreateCompatibleDC(hScreenDC);

    if (hBgDc)
    {
	    bgSize.X = Min(32767,anWidth);
	    bgSize.Y = Min(32767,anHeight);
		
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
    
    ReleaseDC(ghWnd, hScreenDC); hScreenDC = NULL;
    
    return lbRc;
}

bool CBackground::FillBackground(
	const BITMAPFILEHEADER* apBkImgData, // Содержимое *.bmp файла
	int X, int Y, int Width, int Height, // Куда нужно положить картинку
	BackgroundOp Operation)              // {eUpLeft = 0, eStretch = 1, eTile = 2}
{
	if (!hBgDc)
		return false;

	// Залить черным фоном
    RECT rcFull = MakeRect(X,Y,Width,Height);
    FillRect(hBgDc, &rcFull, (HBRUSH)GetStockObject(BLACK_BRUSH));
    
    if (apBkImgData == NULL ||
    	apBkImgData->fbType != 0x4D42/*BM*/ ||
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
    if (pHdr->biPlanes != 1 || pHdr->biCompression != BI_RGB) // BI_JPEG|BI_PNG
    {
    	return false;
    }
    
    DWORD       nBitSize = apBkImgData->bfSize - apBkImgData->bfOffBits;
    TODO("Stride?");
    DWORD       nCalcSize = (pHdr->biWidth * pHdr->biHeight * pHdr->biBitCount) << 3;
    if (nBitSize > nCalcSize)
    	nBitSize = nCalcSize;
    
    

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
		    	BLENDFUNCTION bf = {AC_SRC_OVER, 0, gSet.bgImageDarker, 0};
		    	if (Operation == eUpLeft)
		    	{
		    		int W = Min(Width,pHdr->biWidth); int H = Min(Height,pHdr->biHeight);
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
		    		for (int DY = Y; DY < (Y+Height); DY += pHdr->biHeight)
		    		{
			    		for (int DX = X; DX < (X+Width); DX += pHdr->biWidth)
			    		{
				    		int W = Min((Width-DX),pHdr->biWidth);
				    		int H = Min((Height-DY),pHdr->biHeight);
				    		if (GdiAlphaBlend(hBgDc, DX, DY, W, H, hLoadDC, 0, 0, W, H, bf))
				    			lbRc = true;
			    		}
		    		}
		    	}
		    	
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
