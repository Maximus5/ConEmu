
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
#include "Common.h"
#include "WConsoleInfo.h"

// !!! Only functions which do NOT call Console API !!!

void ChangeScreenBufferSize(CONSOLE_SCREEN_BUFFER_INFO& sbi, SHORT VisibleX, SHORT VisibleY, SHORT BufferX, SHORT BufferY)
{
	_ASSERTE(BufferX>=VisibleX && VisibleX && BufferY>=VisibleY && VisibleY);
	sbi.dwSize.X = BufferX;
	sbi.dwSize.Y = BufferY;
	sbi.srWindow.Right = min(BufferX,(sbi.srWindow.Left+VisibleX))-1;
	sbi.srWindow.Left = max(0,(sbi.srWindow.Right+1-VisibleX));
	sbi.srWindow.Bottom = min(BufferY,(sbi.srWindow.Top+VisibleY))-1;
	sbi.srWindow.Top = max(0,(sbi.srWindow.Bottom+1-VisibleY));
}

// sbi - откуда брать данные
// nCurWidth, nCurHeight - текущие (запомненные) высота и ширина рабочей области
// nCurScroll - текущие (запомненные) флаги
// pnNewWidth, pnNewHeight - результат
// pnScroll - флаги из RealBufferScroll
BOOL GetConWindowSize(const CONSOLE_SCREEN_BUFFER_INFO& sbi, int nCurWidth, int nCurHeight, DWORD nCurScroll, int* pnNewWidth, int* pnNewHeight, DWORD* pnScroll)
{
	DWORD nScroll = rbs_None; // enum RealBufferScroll
	int nNewWidth = 0, nNewHeight = 0;

	// Функция возвращает размер ОКНА (видимой области), то есть буфер может быть больше

	if (sbi.dwSize.X == nCurWidth)
	{
		nNewWidth = sbi.dwSize.X;
	}
	else
	{
		if (((nCurScroll & rbs_Horz) && (sbi.dwSize.X > nCurWidth))
			|| (sbi.dwSize.X > EvalBufferTurnOnSize(nCurWidth)))
		{
			nNewWidth = sbi.srWindow.Right - sbi.srWindow.Left + 1;
			_ASSERTE(nNewWidth <= sbi.dwSize.X);
		}
		else
		{
			nNewWidth = sbi.dwSize.X;
		}
	}
	// Флаги
	if (/*(sbi.dwSize.X > sbi.dwMaximumWindowSize.X) ||*/ (nNewWidth < sbi.dwSize.X))
	{
		// для проверки условий
		//_ASSERTE((sbi.dwSize.X > sbi.dwMaximumWindowSize.X) && (nNewWidth < sbi.dwSize.X));
		nScroll |= rbs_Horz;
	}


	if (sbi.dwSize.Y == nCurHeight)
	{
		nNewHeight = sbi.dwSize.Y;
	}
	else
	{
		if (((nCurScroll & rbs_Vert) && (sbi.dwSize.Y > nCurHeight))
			|| (sbi.dwSize.Y > EvalBufferTurnOnSize(nCurHeight)))
		{
			nNewHeight = sbi.srWindow.Bottom - sbi.srWindow.Top + 1;
		}
		else
		{
			nNewHeight = sbi.dwSize.Y;
		}
	}
	// Флаги
	if (/*(sbi.dwSize.Y > sbi.dwMaximumWindowSize.Y) ||*/ (nNewHeight < sbi.dwSize.Y))
	{
		// для проверки условий
		//_ASSERTE((sbi.dwSize.Y >= sbi.dwMaximumWindowSize.Y) && (nNewHeight < sbi.dwSize.Y));
		nScroll |= rbs_Vert;
	}

	// Validation
	if ((nNewWidth <= 0) || (nNewHeight <= 0))
	{
		_ASSERTE(nNewWidth>0 && nNewHeight>0);
		return FALSE;
	}

	// Result
	if (pnNewWidth)
		*pnNewWidth = nNewWidth;
	if (pnNewHeight)
		*pnNewHeight = nNewHeight;
	if (pnScroll)
		*pnScroll = nScroll;

	return TRUE;

	//BOOL lbBufferHeight = this->isScroll();

	//// Проверка режимов прокрутки
	//if (!lbBufferHeight)
	//{
	//	if (sbi.dwSize.Y > sbi.dwMaximumWindowSize.Y)
	//	{
	//		lbBufferHeight = TRUE; // однозначное включение прокрутки
	//	}
	//}

	//if (lbBufferHeight)
	//{
	//	if (sbi.srWindow.Top == 0
	//	        && sbi.dwSize.Y == (sbi.srWindow.Bottom + 1)
	//	  )
	//	{
	//		lbBufferHeight = FALSE; // однозначное вЫключение прокрутки
	//	}
	//}

	//// Теперь собственно размеры
	//if (!lbBufferHeight)
	//{
	//	nNewHeight =  sbi.dwSize.Y;
	//}
	//else
	//{
	//	// Это может прийти во время смены размера
	//	if ((sbi.srWindow.Bottom - sbi.srWindow.Top + 1) < MIN_CON_HEIGHT)
	//		nNewHeight = con.nTextHeight;
	//	else
	//		nNewHeight = sbi.srWindow.Bottom - sbi.srWindow.Top + 1;
	//}

	//WARNING("Здесь нужно выполнить коррекцию, если nNewHeight велико - включить режим BufferHeight");

	//if (pbBufferHeight)
	//	*pbBufferHeight = lbBufferHeight;

	//_ASSERTE(nNewWidth>=MIN_CON_WIDTH && nNewHeight>=MIN_CON_HEIGHT);

	//if (!nNewWidth || !nNewHeight)
	//{
	//	Assert(nNewWidth && nNewHeight);
	//	return FALSE;
	//}

	////if (nNewWidth < sbi.dwSize.X)
	////    nNewWidth = sbi.dwSize.X;
	//return TRUE;
}
