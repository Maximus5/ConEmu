
/*
Copyright (c) 2012-2017 Maximus5
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

#include <windows.h>
#include "../common/defines.h"
#include "../common/Common.h"
#include "../common/ConEmuCheck.h"
#include "../common/MRect.h"
#include "../common/MStrDup.h"
#include "../common/UnicodeChars.h"
#include "../ConEmu/version.h"
#include "ExtConsole.h"
#include "../common/ConEmuColors3.h"
#include "../common/WObjects.h"

#include "SetHook.h"
#include "hkConsoleOutput.h"
#include "hkWindow.h"
#include "hlpConsole.h"

#define MSG_TITLE "ConEmu writer"
#define MSG_INVALID_CONEMU_VER "Unsupported ConEmu version detected!\nRequired version: " CONEMUVERS "\nConsole writer'll works in 4bit mode"
#define MSG_TRUEMOD_DISABLED   "«Colorer TrueMod support» is not checked in the ConEmu settings\nConsole writer'll works in 4bit mode"
#define MSG_NO_TRUEMOD_BUFFER  "TrueMod support not enabled in the ConEmu settings\nConsole writer'll works in 4bit mode"

TODO("Отвязаться от координат видимой области");

extern HMODULE ghOurModule; // Must be defined and initialized in DllMain

HWND ghExtConEmuWndDC = NULL; // VirtualCon. инициализируется в ExtCheckBuffers()

extern HWND ghConWnd;      // Console window
extern HWND ghConEmuWnd;   // Root! window
extern HWND ghConEmuWndDC; // ConEmu DC window

CESERVER_CONSOLE_MAPPING_HDR SrvMapping = {};

AnnotationHeader* gpTrueColor = NULL;
HANDLE ghTrueColor = NULL;

bool gbInitialized = false;
DWORD gnInitializeErrCode = 0;
//bool gbFarBufferMode = false; // true, если Far запущен с ключом "/w"

WORD defConForeIdx = 7;
WORD defConBackIdx = 0;

typedef BOOL (WINAPI* WriteConsoleW_t)(HANDLE hConsoleOutput, const VOID *lpBuffer, DWORD nNumberOfCharsToWrite, LPDWORD lpNumberOfCharsWritten, LPVOID lpReserved);

struct ExtCurrentAttr
{
	bool  WasSet;
	bool  Reserved1;

	WORD  CONColor;
	WORD  CONForeIdx;
	WORD  CONBackIdx;

	ConEmuColor CEColor;
	AnnotationInfo AIColor;
} gExtCurrentAttr;


#if 0
static bool isCharSpace(wchar_t inChar)
{
	// Сюда пихаем все символы, которые можно отрисовать пустым фоном (как обычный пробел)
	bool isSpace = (inChar == ucSpace || inChar == ucNoBreakSpace || inChar == 0
		/*|| (inChar>=0x2000 && inChar<=0x200F)
		|| inChar == 0x2060 || inChar == 0x3000 || inChar == 0xFEFF*/);
	return isSpace;
}
#endif


static BOOL ExtGetBufferInfo(HANDLE &h, CONSOLE_SCREEN_BUFFER_INFO &csbi, SMALL_RECT &srWork)
{
	_ASSERTE(gbInitialized);

	if (!h)
	{
		_ASSERTE(h!=NULL);
		h = GetStdHandle(STD_OUTPUT_HANDLE);
	}

	if (!GetConsoleScreenBufferInfoCached(h, &csbi))
	{
		_ASSERTE(FALSE && "handle is not supported by ConAPI");
		return FALSE;
	}

	//if (gbFarBufferMode)
	{
		// Фар занимает нижнюю часть консоли. Прилеплен к левому краю
		SHORT nWidth  = csbi.srWindow.Right - csbi.srWindow.Left + 1;
		SHORT nHeight = csbi.srWindow.Bottom - csbi.srWindow.Top + 1;
		srWork.Left = 0;
		srWork.Right = nWidth - 1;
		srWork.Top = csbi.dwSize.Y - nHeight;
		srWork.Bottom = csbi.dwSize.Y - 1;
	}
	/*else
	{
		// Фар занимает все поле консоли
		srWork.Left = srWork.Top = 0;
		srWork.Right = csbi.dwSize.X - 1;
		srWork.Bottom = csbi.dwSize.Y - 1;
	}*/

	if (srWork.Left < 0 || srWork.Top < 0 || srWork.Left > srWork.Right || srWork.Top > srWork.Bottom)
	{
		_ASSERTE(srWork.Left >= 0 && srWork.Top >= 0 && srWork.Left <= srWork.Right && srWork.Top <= srWork.Bottom);
		return FALSE;
	}

	return TRUE;
}

static BOOL ExtCheckBuffers(HANDLE h)
{
	if (!h)
	{
		_ASSERTE(h!=NULL);
		h = GetStdHandle(STD_OUTPUT_HANDLE);
	}

	if (!gbInitialized)
	{
		CONSOLE_SCREEN_BUFFER_INFO csbi = {};
		if (!GetConsoleScreenBufferInfoCached(h, &csbi, TRUE))
		{
			gnInitializeErrCode = GetLastError();
			_ASSERTE(FALSE && "handle is not supported by ConAPI");
		}
		else
		{
			#ifdef _DEBUG
			SHORT nWidth  = csbi.srWindow.Right - csbi.srWindow.Left + 1;
			SHORT nHeight = csbi.srWindow.Bottom - csbi.srWindow.Top + 1;
			_ASSERTE((nWidth <= csbi.dwSize.X) && (nHeight <= csbi.dwSize.Y));
			//gbFarBufferMode = (nWidth < csbi.dwSize.X) || (nHeight < csbi.dwSize.Y);
			#endif
			gbInitialized = true;
		}
	}

	if (!ghConEmuWndDC)
	{
		ExtCloseBuffers();
		SetLastError(E_HANDLE);
		return FALSE;
	}

	if (ghConEmuWndDC != ghExtConEmuWndDC)
	{
		ghExtConEmuWndDC = ghConEmuWndDC;
		ExtCloseBuffers();

		typedef HANDLE(WINAPI* OnOpenFileMappingW_t)(DWORD dwDesiredAccess, BOOL bInheritHandle, LPCWSTR lpName);
		extern HANDLE WINAPI OnOpenFileMappingW(DWORD dwDesiredAccess, BOOL bInheritHandle, LPCWSTR lpName);
		ORIGINAL_KRNL(OpenFileMappingW);

		//TODO: Пока работаем "по-старому", через буфер TrueColor. Переделать, он не оптимален
		wchar_t szMapName[128];
		_ASSERTE(sizeof(AnnotationInfo) == 8*sizeof(int)/*sizeof(AnnotationInfo::raw)*/);
		msprintf(szMapName, countof(szMapName), AnnotationShareName, (DWORD)sizeof(AnnotationInfo), LODWORD(ghExtConEmuWndDC)); //-V205
		ghTrueColor = F(OpenFileMappingW)(FILE_MAP_ALL_ACCESS, FALSE, szMapName);
		if (!ghTrueColor)
		{
			return FALSE;
		}
		gpTrueColor = (AnnotationHeader*)MapViewOfFile(ghTrueColor, FILE_MAP_ALL_ACCESS,0,0,0);
		if (!gpTrueColor)
		{
			CloseHandle(ghTrueColor);
			ghTrueColor = NULL;
			return FALSE;
		}
	}
	if (gpTrueColor)
	{
		if (!gpTrueColor->locked)
		{
			gpTrueColor->locked = TRUE;
		}
	}

	return (gpTrueColor!=NULL);
}

void ExtCloseBuffers()
{
	if (gpTrueColor)
	{
		UnmapViewOfFile(gpTrueColor);
		gpTrueColor = NULL;
	}
	if (ghTrueColor)
	{
		CloseHandle(ghTrueColor);
		ghTrueColor = NULL;
	}
}

// Это это "цвет по умолчанию".
// То, что соответствует "Screen Text" и "Screen Background" в свойствах консоли.
// То, что задаётся командой color в cmd.exe.
// То, что будет использовано если в консоль просто писать
// по printf/std::cout/WriteConsole, без явного указания цвета.
BOOL ExtGetAttributes(ExtAttributesParm* Info)
{
	if (!Info)
	{
		_ASSERTE(Info!=NULL);
		SetLastError(E_INVALIDARG);
		return FALSE;
	}

	BOOL lbTrueColor = ExtCheckBuffers(Info->ConsoleOutput);
	UNREFERENCED_PARAMETER(lbTrueColor);

	HANDLE h = Info->ConsoleOutput;
	CONSOLE_SCREEN_BUFFER_INFO csbi = {};
	SMALL_RECT srWork = {};
	if (!ExtGetBufferInfo(h, csbi, srWork))
	{
		_ASSERTE(FALSE && "ExtGetBufferInfo failed in ExtGetAttributes");
		gExtCurrentAttr.WasSet = false;
		gExtCurrentAttr.CEColor.Flags = CECF_NONE;
		gExtCurrentAttr.CEColor.ForegroundColor = defConForeIdx;
		gExtCurrentAttr.CEColor.BackgroundColor = defConBackIdx;
		memset(&gExtCurrentAttr.AIColor, 0, sizeof(gExtCurrentAttr.AIColor));
		return FALSE;
	}

	// Что-то в некоторых случаях сбивается цвет вывода для printf
	//_ASSERTE("GetTextAttributes" && ((csbi.wAttributes & 0xFF) == 0x07));

	TODO("По хорошему, gExtCurrentAttr нада ветвить по разным h");
	if (gExtCurrentAttr.WasSet && (csbi.wAttributes == gExtCurrentAttr.CONColor))
	{
		Info->Attributes = gExtCurrentAttr.CEColor;
	}
	else
	{
		gExtCurrentAttr.WasSet = false;
		memset(&gExtCurrentAttr.CEColor, 0, sizeof(gExtCurrentAttr.CEColor));
		memset(&gExtCurrentAttr.AIColor, 0, sizeof(gExtCurrentAttr.AIColor));

		Info->Attributes.Flags = CECF_NONE;
		Info->Attributes.ForegroundColor = CONFORECOLOR(csbi.wAttributes);
		Info->Attributes.BackgroundColor = CONBACKCOLOR(csbi.wAttributes);
	}

	return TRUE;
}

static void ExtPrepareColor(const ConEmuColor& Attributes, AnnotationInfo& t, WORD& n)
{
	//-- zeroing must be done by calling function
	//memset(&t, 0, sizeof(t)); n = 0;

	WORD f = 0;
	CECOLORFLAGS Flags = Attributes.Flags;

	if (Flags & CECF_FG_BOLD)
		f |= AI_STYLE_BOLD;
	if (Flags & CECF_FG_ITALIC)
		f |= AI_STYLE_ITALIC;
	if (Flags & CECF_FG_UNDERLINE)
		f |= AI_STYLE_UNDERLINE;
	t.style = f;

	DWORD nForeColor, nBackColor;
	if (Flags & CECF_FG_24BIT)
	{
		//n |= 0x07;
		nForeColor = Attributes.ForegroundColor & 0x00FFFFFF;
		Far3Color::Color2FgIndex(nForeColor, n);
		t.fg_color = nForeColor;
		t.fg_valid = TRUE;
	}
	else
	{
		nForeColor = -1;
		n |= (WORD)CONCOLORINDEX(Attributes.ForegroundColor);
		t.fg_valid = FALSE;
	}

	if (Flags & CECF_BG_24BIT)
	{
		nBackColor = Attributes.BackgroundColor & 0x00FFFFFF;
		Far3Color::Color2BgIndex(nBackColor, nBackColor==nForeColor, n);
		t.bk_color = nBackColor;
		t.bk_valid = TRUE;
	}
	else
	{
		n |= (WORD)(CONCOLORINDEX(Attributes.BackgroundColor)<<4);
		t.bk_valid = FALSE;
	}
}

// Это это "цвет по умолчанию".
// То, что соответствует "Screen Text" и "Screen Background" в свойствах консоли.
// То, что задаётся командой color в cmd.exe.
// То, что будет использовано если в консоль просто писать
// по printf/std::cout/WriteConsole, без явного указания цвета.
BOOL ExtSetAttributes(const ExtAttributesParm* Info)
{
	if (!Info)
	{
		// ConEmu internals: сбросить запомненный "атрибут"
		gExtCurrentAttr.WasSet = false;
		return TRUE;
	}

	BOOL lbTrueColor = ExtCheckBuffers(Info->ConsoleOutput);
	UNREFERENCED_PARAMETER(lbTrueColor);

	HANDLE h = Info->ConsoleOutput;
	CONSOLE_SCREEN_BUFFER_INFO csbi = {};
	SMALL_RECT srWork = {};
	if (!ExtGetBufferInfo(h, csbi, srWork))
		return FALSE;

	BOOL lbRc = TRUE;

	// <G|S>etTextAttributes должны ожидать указатель на ОДИН ConEmuColor.
	AnnotationInfo t = {};
	WORD n = 0;
	ExtPrepareColor(Info->Attributes, t, n);

	SetConsoleTextAttribute(h, n);

	TODO("По хорошему, gExtCurrentAttr нада ветвить по разным h");
	// запомнить, что WriteConsole должен писать атрибутом "t"
	gExtCurrentAttr.WasSet = true;
	gExtCurrentAttr.CONColor = n;
	gExtCurrentAttr.CONForeIdx = CONFORECOLOR(n);
	gExtCurrentAttr.CONBackIdx = CONBACKCOLOR(n);
	gExtCurrentAttr.CEColor = Info->Attributes;
	gExtCurrentAttr.AIColor = t;

	return lbRc;
}

void ConEmuCharBuffer::Inc(size_t Cells /*= 1*/)
{
	switch (BufferType)
	{
	case (int)ewtf_FarClr:
		FARBuffer += Cells;
		break;
	case (int)ewtf_CeClr:
		CEBuffer += Cells;
		CEColor += Cells;
		break;
	case (int)ewtf_AiClr:
		CEBuffer += Cells;
		AIColor += Cells;
		break;

	#ifdef _DEBUG
	default:
		_ASSERTE(BufferType==(int)ewtf_FarClr && "Invalid type specified!");
	#endif
	};
};


#if 0
// ConEmuCharBuffer Buffer, BufferSize, BufferCoord, Region
BOOL WINAPI ExtReadOutput(ExtReadWriteOutputParm* Info)
{
	if (!Info || (Info->StructSize < sizeof(*Info)))
	{
		_ASSERTE(Info!=NULL && (Info->StructSize >= sizeof(*Info)));
		SetLastError(E_INVALIDARG);
		return FALSE;
	}

	BOOL lbTrueColor = ExtCheckBuffers();
	UNREFERENCED_PARAMETER(lbTrueColor);

	HANDLE h = Info->ConsoleOutput;
	CONSOLE_SCREEN_BUFFER_INFO csbi = {};
	SMALL_RECT srWork = {};
	if (!ExtGetBufferInfo(h, csbi, srWork))
		return FALSE;

	CHAR_INFO cDefReadBuf[1024];
	CHAR_INFO *pcReadBuf = NULL;
	int nWindowWidth  = srWork.Right - srWork.Left + 1;
	if (Info->BufferSize.X <= (int)countof(cDefReadBuf))
		pcReadBuf = cDefReadBuf;
	else
		pcReadBuf = (CHAR_INFO*)calloc(Info->BufferSize.X,sizeof(*pcReadBuf));
	if (!pcReadBuf)
	{
		SetLastError(E_OUTOFMEMORY);
		return FALSE;
	}

	BOOL lbRc = TRUE;

	AnnotationInfo* pTrueColorStart = (AnnotationInfo*)(gpTrueColor ? (((LPBYTE)gpTrueColor) + gpTrueColor->struct_size) : NULL);
	AnnotationInfo* pTrueColorEnd = pTrueColorStart ? (pTrueColorStart + gpTrueColor->bufferSize) : NULL;
	PRAGMA _ERROR("Заменить на INT_PTR nLinePosition");
	//AnnotationInfo* pTrueColorLine = (AnnotationInfo*)(pTrueColorStart ? (pTrueColorStart + nWindowWidth * (Info->Region.Top /*- srWork.Top*/)) : NULL);

	int nBufferType = (int)(Info->Flags & ewtf_Mask);

	ConEmuCharBuffer BufEnd = {};
	BufEnd.BufferType = nBufferType;
	switch (nBufferType)
	{
	case (int)ewtf_FarClr:
		BufEnd.FARBuffer = Info->Buffer.FARBuffer + (Info->BufferSize.X * Info->BufferSize.Y); //-V104;
		break;
	case (int)ewtf_CeClr:
	case (int)ewtf_AiClr:
		BufEnd.CEBuffer = BufEnd.CEBuffer = Info->Buffer.CEBuffer + (Info->BufferSize.X * Info->BufferSize.Y); //-V104;
		break;
	default:
		_ASSERTE(nBufferType==(int)ewtf_FarClr && "Invalid type specified!");
		SetLastError(E_INVALIDARG);
		return FALSE;
	};


	SMALL_RECT rcRead = Info->Region;
	COORD MyBufferSize = {Info->BufferSize.X, 1};
	COORD MyBufferCoord = {Info->BufferCoord.X, 0};
	SHORT YShift = /*gbFarBufferMode ?*/ (csbi.dwSize.Y - (srWork.Bottom - srWork.Top + 1)) /*: 0*/;
	SHORT Y1 = Info->Region.Top + YShift;
	SHORT Y2 = Info->Region.Bottom + YShift;
	SHORT BufferShift = Info->Region.Top + YShift;

	if (Y2 >= csbi.dwSize.Y)
	{
		_ASSERTE((Y2 >= 0) && (Y2 < csbi.dwSize.Y));
		Y2 = csbi.dwSize.Y - 1;
		lbRc = FALSE; // но продолжим, запишем, сколько сможем
	}

	if ((Y1 < 0) || (Y1 >= csbi.dwSize.Y))
	{
		_ASSERTE((Y1 >= 0) && (Y1 < csbi.dwSize.Y));
		if (Y1 >= csbi.dwSize.Y)
			Y1 = csbi.dwSize.Y - 1;
		if (Y1 < 0)
			Y1 = 0;
		lbRc = FALSE;
	}

	for (rcRead.Top = Y1; rcRead.Top <= Y2; rcRead.Top++)
	{
		rcRead.Bottom = rcRead.Top;
		BOOL lbRead = MyReadConsoleOutputW(h, pcReadBuf, MyBufferSize, MyBufferCoord, &rcRead);

		CHAR_INFO* pc = pcReadBuf + Info->BufferCoord.X;
		ConEmuCharBuffer BufPtr = Info->Buffer;
		BufPtr.Inc((rcRead.Top - BufferShift + Info->BufferCoord.Y)*Info->BufferSize.X + Info->BufferCoord.X); //-V104
		//FAR_CHAR_INFO* pFar = Buffer + (rcRead.Top - BufferShift + Info->BufferCoord.Y)*Info->BufferSize.X + Info->BufferCoord.X;
		AnnotationInfo* pTrueColor = (pTrueColorLine && (pTrueColorLine >= pTrueColorStart)) ? (pTrueColorLine + Info->Region.Left) : NULL;

		for (int X = rcRead.Left; X <= rcRead.Right; X++, pc++, BufPtr.Inc())
		{
			if (BufPtr.RAW >= BufEnd.RAW)
			{
				_ASSERTE(BufPtr.RAW < BufEnd.RAW);
				break;
			}

			if (pTrueColor && pTrueColor >= pTrueColorEnd)
			{
				_ASSERTE(pTrueColor && pTrueColor < pTrueColorEnd);
				pTrueColor = NULL; // Выделенный буфер оказался недостаточным
			}

			switch (nBufferType)
			{
			case (int)ewtf_FarClr:
				{
					FAR_CHAR_INFO chr = {lbRead ? pc->Char.UnicodeChar : L' '};

					if (pTrueColor)
					{
						DWORD Style = pTrueColor->style;
						if (Style & AI_STYLE_BOLD)
							chr.Attributes.Flags |= FCF_FG_BOLD;
						if (Style & AI_STYLE_ITALIC)
							chr.Attributes.Flags |= FCF_FG_ITALIC;
						if (Style & AI_STYLE_UNDERLINE)
							chr.Attributes.Flags |= FCF_FG_UNDERLINE;
					}

					if (pTrueColor && pTrueColor->fg_valid)
					{
						chr.Attributes.ForegroundColor = pTrueColor->fg_color;
					}
					else
					{
						chr.Attributes.Flags |= FCF_FG_4BIT;
						chr.Attributes.ForegroundColor = lbRead ? CONFORECOLOR(pc->Attributes) : defConForeIdx;
					}

					if (pTrueColor && pTrueColor->bk_valid)
					{
						chr.Attributes.BackgroundColor = pTrueColor->bk_color;
					}
					else
					{
						chr.Attributes.Flags |= FCF_BG_4BIT;
						chr.Attributes.BackgroundColor = lbRead ? CONBACKCOLOR(pc->Attributes) : defConBackIdx;
					}

					*BufPtr.FARBuffer = chr;

				} // ewtf_FarClr
				break;

			case (int)ewtf_CeClr:
				{
					*BufPtr.CEBuffer = *pc;

					ConEmuColor clr = {};

					if (pTrueColor)
					{
						DWORD Style = pTrueColor->style;
						if (Style & AI_STYLE_BOLD)
							clr.Flags |= CECF_FG_BOLD;
						if (Style & AI_STYLE_ITALIC)
							clr.Flags |= CECF_FG_ITALIC;
						if (Style & AI_STYLE_UNDERLINE)
							clr.Flags |= CECF_FG_UNDERLINE;
					}

					if (pTrueColor && pTrueColor->fg_valid)
					{
						clr.Flags |= CECF_FG_24BIT;
						clr.ForegroundColor = pTrueColor->fg_color;
					}
					else
					{
						clr.ForegroundColor = lbRead ? CONFORECOLOR(pc->Attributes) : defConForeIdx;
					}

					if (pTrueColor && pTrueColor->bk_valid)
					{
						clr.Flags |= CECF_BG_24BIT;
						clr.BackgroundColor = pTrueColor->bk_color;
					}
					else
					{
						clr.BackgroundColor = lbRead ? CONBACKCOLOR(pc->Attributes) : defConBackIdx;
					}

					*BufPtr.CEColor = clr;

				} // ewtf_CeClr
				break;

			case (int)ewtf_AiClr:
				{
					*BufPtr.CEBuffer = *pc;

					if (pTrueColor)
						*BufPtr.AIColor = *pTrueColor;
					else
						memset(BufPtr.AIColor, 0, sizeof(*BufPtr.AIColor));

				} // ewtf_AiClr
				break;
			};

			if (pTrueColor)
				pTrueColor++;
		}

		if (pTrueColorLine)
			pTrueColorLine += nWindowWidth; //-V102
	}

	if (pcReadBuf != cDefReadBuf)
		free(pcReadBuf);

	return lbRc;
}


// ConEmuCharBuffer Buffer, BufferSize, BufferCoord, Region
BOOL WINAPI ExtWriteOutput(const ExtReadWriteOutputParm* Info)
{
	if (!Info || (Info->StructSize < sizeof(*Info)))
	{
		_ASSERTE(Info!=NULL && (Info->StructSize >= sizeof(*Info)));
		SetLastError(E_INVALIDARG);
		return FALSE;
	}

	BOOL lbTrueColor = ExtCheckBuffers();
	UNREFERENCED_PARAMETER(lbTrueColor);

	HANDLE h = Info->ConsoleOutput;
	CONSOLE_SCREEN_BUFFER_INFO csbi = {};
	SMALL_RECT srWork = {};
	if (!ExtGetBufferInfo(h, csbi, srWork))
		return FALSE;

	CHAR_INFO cDefWriteBuf[1024];
	CHAR_INFO *pcWriteBuf = NULL;
	int nWindowWidth  = srWork.Right - srWork.Left + 1;
	if (Info->BufferSize.X <= (int)countof(cDefWriteBuf))
		pcWriteBuf = cDefWriteBuf;
	else
		pcWriteBuf = (CHAR_INFO*)calloc(Info->BufferSize.X,sizeof(*pcWriteBuf));
	if (!pcWriteBuf)
	{
		SetLastError(E_OUTOFMEMORY);
		return FALSE;
	}

	BOOL lbRc = TRUE;

	AnnotationInfo* pTrueColorStart = (AnnotationInfo*)(gpTrueColor ? (((LPBYTE)gpTrueColor) + gpTrueColor->struct_size) : NULL); //-V104
	AnnotationInfo* pTrueColorEnd = pTrueColorStart ? (pTrueColorStart + gpTrueColor->bufferSize) : NULL; //-V104
	PRAGMA _ERROR("Заменить на INT_PTR nLinePosition");
	//AnnotationInfo* pTrueColorLine = (AnnotationInfo*)(pTrueColorStart ? (pTrueColorStart + nWindowWidth * (Info->Region.Top /*- srWork.Top*/)) : NULL); //-V104

	int nBufferType = (int)(Info->Flags & ewtf_Mask);

	SMALL_RECT rcWrite = Info->Region;
	COORD MyBufferSize = {Info->BufferSize.X, 1};
	COORD MyBufferCoord = {Info->BufferCoord.X, 0};
	SHORT YShift = /*gbFarBufferMode ?*/ (csbi.dwSize.Y - (srWork.Bottom - srWork.Top + 1)) /*: 0*/;
	SHORT Y1 = Info->Region.Top + YShift;
	SHORT Y2 = Info->Region.Bottom + YShift;
	SHORT BufferShift = Info->Region.Top + YShift;

	if (Y2 >= csbi.dwSize.Y)
	{
		_ASSERTE((Y2 >= 0) && (Y2 < csbi.dwSize.Y));
		Y2 = csbi.dwSize.Y - 1;
		lbRc = FALSE; // но продолжим, запишем, сколько сможем
	}

	if ((Y1 < 0) || (Y1 >= csbi.dwSize.Y))
	{
		_ASSERTE((Y1 >= 0) && (Y1 < csbi.dwSize.Y));
		if (Y1 >= csbi.dwSize.Y)
			Y1 = csbi.dwSize.Y - 1;
		if (Y1 < 0)
			Y1 = 0;
		lbRc = FALSE;
	}

	for (rcWrite.Top = Y1; rcWrite.Top <= Y2; rcWrite.Top++)
	{
		rcWrite.Bottom = rcWrite.Top;

		CHAR_INFO* pc = pcWriteBuf + Info->BufferCoord.X;
		ConEmuCharBuffer BufPtr = Info->Buffer;
		BufPtr.Inc((rcWrite.Top - BufferShift + Info->BufferCoord.Y)*Info->BufferSize.X + Info->BufferCoord.X); //-V104
		//const FAR_CHAR_INFO* pFar = Buffer + (rcWrite.Top - BufferShift + Info->BufferCoord.Y)*Info->BufferSize.X + Info->BufferCoord.X; //-V104
		AnnotationInfo* pTrueColor = (pTrueColorLine && (pTrueColorLine >= pTrueColorStart)) ? (pTrueColorLine + Info->Region.Left) : NULL;


		for (int X = rcWrite.Left; X <= rcWrite.Right; X++, pc++, BufPtr.Inc())
		{
			if (pTrueColor && pTrueColor >= pTrueColorEnd)
			{
				#ifdef _DEBUG
				static bool bBufferAsserted = false;
				if (!bBufferAsserted)
				{
					bBufferAsserted = true;
					_ASSERTE(pTrueColor && pTrueColor < pTrueColorEnd && "Allocated buffer was not enough");
				}
				#endif
				pTrueColor = NULL; // Выделенный буфер оказался недостаточным
			}

			bool Bold = false, Italic = false, Underline = false;
			bool Fore24bit = false, Back24bit = false;
			DWORD ForegroundColor = 0, BackgroundColor = 0;

			// go
			switch (nBufferType)
			{
			case (int)ewtf_FarClr:
				{
					pc->Char.UnicodeChar = BufPtr.FARBuffer->Char;

					unsigned __int64 Flags = BufPtr.FARBuffer->Attributes.Flags;

					Bold = (Flags & FCF_FG_BOLD) != 0;
					Italic = (Flags & FCF_FG_ITALIC) != 0;
					Underline = (Flags & FCF_FG_UNDERLINE) != 0;

					Fore24bit = (Flags & FCF_FG_4BIT) == 0;
					Back24bit = (Flags & FCF_BG_4BIT) == 0;

					ForegroundColor = BufPtr.FARBuffer->Attributes.ForegroundColor & (Fore24bit ? COLORMASK : INDEXMASK);
					BackgroundColor = BufPtr.FARBuffer->Attributes.BackgroundColor & (Back24bit ? COLORMASK : INDEXMASK);
				}
				break;

			case (int)ewtf_CeClr:
				{
					pc->Char.UnicodeChar = BufPtr.CEBuffer->Char.UnicodeChar;

					unsigned __int64 Flags = BufPtr.CEColor->Flags;

					Bold = (Flags & CECF_FG_BOLD) != 0;
					Italic = (Flags & CECF_FG_ITALIC) != 0;
					Underline = (Flags & CECF_FG_UNDERLINE) != 0;

					Fore24bit = (Flags & CECF_FG_24BIT) != 0;
					Back24bit = (Flags & CECF_BG_24BIT) != 0;

					ForegroundColor = BufPtr.CEColor->ForegroundColor & (Fore24bit ? COLORMASK : INDEXMASK);
					BackgroundColor = BufPtr.CEColor->BackgroundColor & (Back24bit ? COLORMASK : INDEXMASK);
				}
				break;

			case (int)ewtf_AiClr:
				{
					pc->Char.UnicodeChar = BufPtr.CEBuffer->Char.UnicodeChar;

					unsigned int Flags = BufPtr.AIColor->style;

					Bold = (Flags & AI_STYLE_BOLD) != 0;
					Italic = (Flags & AI_STYLE_ITALIC) != 0;
					Underline = (Flags & AI_STYLE_UNDERLINE) != 0;

					Fore24bit = BufPtr.AIColor->fg_valid != 0;
					Back24bit = BufPtr.AIColor->bk_valid != 0;

					ForegroundColor = Fore24bit ? BufPtr.AIColor->fg_color : CONFORECOLOR(BufPtr.CEBuffer->Attributes);
					BackgroundColor = Back24bit ? BufPtr.AIColor->bk_color : CONBACKCOLOR(BufPtr.CEBuffer->Attributes);
				}
				break;
			}

			WORD n = 0, f = 0;

			if (pTrueColor)
			{
				if (Bold)
					f |= AI_STYLE_BOLD;
				if (Italic)
					f |= AI_STYLE_ITALIC;
				if (Underline)
					f |= AI_STYLE_UNDERLINE;
				pTrueColor->style = f;
			}

			DWORD nForeColor, nBackColor;
			if (!Fore24bit)
			{
				nForeColor = -1;
				_ASSERTE(ForegroundColor<=INDEXMASK);
				n |= (WORD)ForegroundColor;
				if (pTrueColor)
				{
					pTrueColor->fg_valid = FALSE;
				}
			}
			else
			{
				//n |= 0x07;
				nForeColor = ForegroundColor;
				Far3Color::Color2FgIndex(nForeColor, n);
				if (pTrueColor)
				{
					pTrueColor->fg_color = nForeColor;
					pTrueColor->fg_valid = TRUE;
				}
			}

			if (!Back24bit)
			{
				nBackColor = -1;
				_ASSERTE(BackgroundColor<=INDEXMASK);
				WORD bk = (WORD)BackgroundColor;
				// Коррекция яркости, если подобранные индексы совпали
				if (n == bk && Fore24bit && !isCharSpace(BufPtr.FARBuffer->Char))
				{
					if (n & 8)
						bk ^= 8;
					else
						n |= 8;
				}
				n |= bk<<4;

				if (pTrueColor)
				{
					pTrueColor->bk_valid = FALSE;
				}
			}
			else
			{
				nBackColor = BackgroundColor;
				Far3Color::Color2BgIndex(nBackColor, nForeColor==nBackColor, n);
				if (pTrueColor)
				{
					pTrueColor->bk_color = nBackColor;
					pTrueColor->bk_valid = TRUE;
				}
			}

			// Apply color (console, 4bit)
			pc->Attributes = n;

			// Done
			if (pTrueColor)
				pTrueColor++;
		}

		if (!WriteConsoleOutputW(h, pcWriteBuf, MyBufferSize, MyBufferCoord, &rcWrite))
		{
			lbRc = FALSE;
		}

		if (pTrueColorLine)
			pTrueColorLine += nWindowWidth; //-V102
	}

	PRAGMA _ERROR("Не забыть накрутить индекс в AnnotationInfo");

	// Commit по флагу
	if (Info->Flags & ewtf_Commit)
	{
		ExtCommitParm c = {sizeof(c), Info->ConsoleOutput};
		ExtCommit(&c);
	}

	if (pcWriteBuf != cDefWriteBuf)
		free(pcWriteBuf);

	return lbRc;
}
#endif

BOOL WINAPI apiWriteConsoleW(HANDLE hConsoleOutput, const VOID *lpBuffer, DWORD nNumberOfCharsToWrite, LPDWORD lpNumberOfCharsWritten, LPVOID lpReserved, WriteConsoleW_t lpfn = NULL)
{
	BOOL lbRc = FALSE;

	// Don't use static var, it may be changed in runtime
	WriteConsoleW_t fnWriteConsoleW = lpfn;

	if (!fnWriteConsoleW)
	{
		_ASSERTE(FALSE && "Is not supposed to be used out of ConEmuHk itself, therefore lpfn must be passed into");
		{
			HMODULE hHooks = GetModuleHandle(WIN3264TEST(L"ConEmuHk.dll",L"ConEmuHk64.dll"));
			if (hHooks && (hHooks != ghOurModule))
			{
				typedef FARPROC (WINAPI* GetWriteConsoleW_t)();
				GetWriteConsoleW_t getf = (GetWriteConsoleW_t)GetProcAddress(hHooks, "GetWriteConsoleW");
				if (!getf)
				{
					_ASSERTE(getf!=NULL);
					return FALSE;
				}

				fnWriteConsoleW = (WriteConsoleW_t)getf();
				if (!fnWriteConsoleW)
				{
					_ASSERTE(fnWriteConsoleW!=NULL);
					return FALSE;
				}
			}

			if (!fnWriteConsoleW)
			{
				fnWriteConsoleW = WriteConsoleW;
			}
		}
	}

	lbRc = fnWriteConsoleW(hConsoleOutput, lpBuffer, nNumberOfCharsToWrite, lpNumberOfCharsWritten, lpReserved);

	return lbRc;
}


static BOOL IntWriteText(HANDLE h, SHORT x, SHORT ForceDumpX,
						 const wchar_t *pFrom, DWORD cchWrite,
						 AnnotationInfo* pTrueColorLine, AnnotationInfo* pTrueColorEnd,
						 const AnnotationInfo& AIColor,
						 const CONSOLE_SCREEN_BUFFER_INFO& csbi,
						 WriteConsoleW_t lpfn = NULL)
{
	SHORT nWindowWidth = csbi.srWindow.Right - csbi.srWindow.Left + 1;
	_ASSERTEX(nWindowWidth > 0);

	// Write extended attributes if required
	if (pTrueColorLine && (nWindowWidth > 0))
	{
		AnnotationInfo* pTrueColor = pTrueColorLine + x;
		const wchar_t *pch = pFrom;

		for (SHORT x1 = x; x1 <= ForceDumpX; x1++, pch++)
		{
			// This control chars do not have glyphs unless ENABLE_PROCESSED_OUTPUT is cleared!
			switch (*pch)
			{
			case L'\r':
				pTrueColor = pTrueColorLine;
				continue;
			case L'\n':
				pTrueColor = pTrueColorLine = (pTrueColorLine + nWindowWidth);
				continue;
			case 7: // bell
				continue; // non-spacing
			case 8: // bs
				pTrueColor = max(pTrueColorLine, pTrueColor-1);
				continue;
			case 9: // tab
				TODO("Fill and mark affected console cells?");
				break;
			}

			if (pTrueColor >= pTrueColorEnd)
			{
				#ifdef _DEBUG
				static bool bBufferAsserted = false;
				if (!bBufferAsserted)
				{
					bBufferAsserted = true;
					_ASSERTE(pTrueColor && pTrueColor < pTrueColorEnd && "Allocated buffer was not enough");
				}
				#endif
				pTrueColor = NULL; // Выделенный буфер оказался недостаточным
				break;
			}

			*(pTrueColor++) = AIColor;
		}
	}

	// Вывод в консоль "текста"
	DWORD nWritten = 0;
	BOOL lbRc = apiWriteConsoleW(h, pFrom, cchWrite, &nWritten, NULL, lpfn);

	DEBUGTEST(DWORD nWriteErr = GetLastError());

	return lbRc;
}


// wchar_t* Buffer, NumberOfCharsToWrite
BOOL ExtWriteText(ExtWriteTextParm* Info)
{
	if (!Info || (Info->StructSize < sizeof(*Info)))
	{
		_ASSERTE(Info!=NULL && (Info->StructSize >= sizeof(*Info)));
		SetLastError(E_INVALIDARG);
		return FALSE;
	}

	#ifdef _DEBUG
	extern FARPROC CallWriteConsoleW;
	_ASSERTE((CallWriteConsoleW!=NULL) || !HooksWereSet);
	_ASSERTE((Info->Private==NULL) || (Info->Private==(void*)CallWriteConsoleW) || (!HooksWereSet && (Info->Private==(void*)WriteConsoleW)));
	#endif

	// Проверка аргументов
	if (!Info->ConsoleOutput || !Info->Buffer || !Info->NumberOfCharsToWrite)
	{
		_ASSERTE(Info->ConsoleOutput && Info->Buffer && Info->NumberOfCharsToWrite);
		SetLastError(E_INVALIDARG);
		return FALSE;
	}

	BOOL lbTrueColor = ExtCheckBuffers(Info->ConsoleOutput);
	UNREFERENCED_PARAMETER(lbTrueColor);

	HANDLE h = Info->ConsoleOutput;
	CONSOLE_SCREEN_BUFFER_INFO csbi = {};
	SMALL_RECT srWork = {};
	if (!ExtGetBufferInfo(h, csbi, srWork))
	{
		return FALSE;
	}

	bool  bWrap = true;
	bool  bVirtualWrap = false;
	bool  bRevertMode = false;
	SHORT WrapAtCol = csbi.dwSize.X; // 1-based
	DWORD Mode = 0;

	// "Working lines" may be defined (Vim and others)
	LONG  ScrollTop, ScrollBottom;
	bool  bScrollRegion = ((Info->Flags & ewtf_Region) != 0);
	RECT  rcScrollRegion = Info->Region;
	COORD crScrollCursor;
	if (bScrollRegion)
	{
		_ASSERTEX(Info->Region.left==-1 && Info->Region.right==-1); // Not used yet
		rcScrollRegion.left = 0; rcScrollRegion.right = (csbi.dwSize.X - 1);
		_ASSERTEX(Info->Region.top>=0 && Info->Region.bottom>=Info->Region.top);
		ScrollTop = Info->Region.top;
		ScrollBottom = Info->Region.bottom+1;
	}
	else
	{
		ScrollTop = 0;
		ScrollBottom = csbi.dwSize.Y;
	}

	GetConsoleModeCached(h, &Mode);

	if ((Info->Flags & ewtf_WrapAt))
	{
		if ((Info->WrapAtCol > 0) && (Info->WrapAtCol < WrapAtCol))
		{
			WrapAtCol = Info->WrapAtCol;
			bVirtualWrap = true;
		}
	}
	else
	{
		bWrap = (Mode & ENABLE_WRAP_AT_EOL_OUTPUT) != 0;
		//_ASSERTE((Mode & ENABLE_PROCESSED_OUTPUT) != 0);
	}

	_ASSERTE(srWork.Bottom == (csbi.dwSize.Y - 1));
	// Размер TrueColor буфера
	SHORT nWindowWidth  = srWork.Right - srWork.Left + 1;
	DEBUGTEST(SHORT nWindowHeight = srWork.Bottom - srWork.Top + 1);
	SHORT YShift = csbi.dwCursorPosition.Y - srWork.Top;

	BOOL lbRc = TRUE;


	AnnotationInfo* pTrueColorStart = (AnnotationInfo*)(gpTrueColor ? (((LPBYTE)gpTrueColor) + gpTrueColor->struct_size) : NULL); //-V104
	AnnotationInfo* pTrueColorEnd = pTrueColorStart ? (pTrueColorStart + gpTrueColor->bufferSize) : NULL; //-V104
	//AnnotationInfo* pTrueColorLine = (AnnotationInfo*)(pTrueColorStart ? (pTrueColorStart + nWindowWidth * YShift) : NULL); //-V104
	INT_PTR nLinePosition = nWindowWidth * YShift;

	// пока только "текущим" выбранным цветом
	_ASSERTE((Info->Flags & ewtf_Current) == ewtf_Current);

	// Current attributes
	AnnotationInfo AIColor = {};
	ExtAttributesParm DefClr = {sizeof(DefClr), h};
	ExtGetAttributes(&DefClr);
	WORD DefConColor = 0;
	ExtPrepareColor(DefClr.Attributes, AIColor, DefConColor);

	// go
	const wchar_t *pFrom = Info->Buffer;
	const wchar_t *pEnd = pFrom + Info->NumberOfCharsToWrite;

	// tmux, status line
	// top (from ssh-ed UNIX) - writes all lines using full console width and thereafter some ANSI-s and "\r\n"
	if ((Info->Flags & ewtf_DontWrap))
	{
		//TODO: These are assumptions, it'll be better to reimplement console interface from scratch...
		if (bWrap
			//&& (csbi.dwCursorPosition.Y == csbi.srWindow.Bottom)
			&& ((csbi.dwCursorPosition.X + Info->NumberOfCharsToWrite) == csbi.dwSize.X))
		{
			bRevertMode = true;
			// If app was asked to write carriage return - don't enable non-wrap mode
			for (const wchar_t* p = pFrom; p < pEnd; p++)
			{
				if ((*p == L'\n') || (*p == L'\r'))
				{
					bRevertMode = false; break;
				}
			}
			// Don't let windows console to move cursor to the next line and **scroll** contents
			if (bRevertMode)
			{
				SetConsoleMode(h, Mode & ~ENABLE_WRAP_AT_EOL_OUTPUT);
			}
		}
	}
	#ifdef _DEBUG
	else
	{
		int iDbg = 0; // ewtf_DontWrap was not set
	}
	#endif

	bool bAutoLfNl = ((Info->Flags & ewtf_NoLfNl) == 0);
	bool bNonAutoLfNl = false;

	const wchar_t *pCur = pFrom;
	SHORT x = csbi.dwCursorPosition.X, y = csbi.dwCursorPosition.Y; // 0-based
	SHORT x2 = x, y2 = y;

	Info->ScrolledRowsUp = 0;

	TODO("Тут может быть засада - если другие приложения в это же время выводят в консоль - будет драка...");
	for (; pCur < pEnd; pCur++)
	{
		// Учитывать
		// bWrap - при выводе в последнюю колонку консоли - переводить курсор, если последняя строка - делать скролл

		/*
		if (csbi.dwCursorPosition.X > nMaxCursorPos)
		{
			// т.к. тут текущую позицию курсора затирать не нужно (наверное?)
			// это может быть только в том случае, если курсор попал "за правую границу"
			// например, сначала вывели длинную строку, а потом ограничили правый край
			csbi.dwCursorPosition.X++;
			PadAndScroll(hConsoleOutput, csbi);
		}
		*/

		SHORT ForceDumpX = 0;
		bool BSRN = false;
		bool bForceDumpScroll = false;
		bool bSkipBELL = false;

		WARNING("Refactoring required: use an object to cache symbols and write them on request");

		// Обработка символов, которые двигают курсор
		switch (*pCur)
		{
		case L'\t':
			x2 = ((x2 + 7) >> 3) << 3;
			// like normal char...
			if (x2 >= WrapAtCol)
			{
				ForceDumpX = min(x2, WrapAtCol)-1;
			}
			break;
		case L'\r':
			if (x2 > 0)
				ForceDumpX = x2-1;
			x2 = 0;
			bNonAutoLfNl = false;
			BSRN = true;
			// "\r\n"? Do not break in two physical writes
			if (((pCur+1) < pEnd) && (*(pCur+1) == L'\n'))
			{
				pCur++; y2++;
				_ASSERTE(bWrap);
			}
			break;
		case L'\n':
			if (x2 > 0)
				ForceDumpX = x2-1;
			if (bAutoLfNl)
				x2 = 0;
			else if (x2)
				bNonAutoLfNl = true;
			y2++;
			if (y2 >= ScrollBottom)
				bForceDumpScroll = true;
			_ASSERTE(bWrap);
			BSRN = true;
			break;
		case 7:
			//Beep (no spacing)
			if ((Info->Flags & ewtf_NoBells))
			{
				LogBeepSkip(L"--- Skipped ExtWriteText(7)\n");
				bForceDumpScroll = bSkipBELL = true;
				pCur--;
			}
			else
			{
				LogBeepSkip(L"--- ExtWriteText(7)\n");
			}
			break;
		case 8:
			//BS
			if (x2>x)
				ForceDumpX = x2;
			if (x2>0)
				x2--;
			BSRN = true;
			break;
		default:
			// Просто буква.
			// В real-консоли вроде бы не может быть non-spacing символов, кроме выше-обработанных
			x2++;
			// like '\t'
			if ((x2 >= WrapAtCol)
				&& (!bRevertMode || ((pCur+1) < pEnd)))
			{
				ForceDumpX = min(x2, WrapAtCol)-1;
				x2 = 0; y2++;
			}
		}


		if (ForceDumpX || bForceDumpScroll)
		{
			// Printing (including pCur) using extended attributes
			if (pCur >= pFrom)
				lbRc = IntWriteText(h, x, ForceDumpX, pFrom, (DWORD)(pCur - pFrom + 1),
						(pTrueColorStart && (nLinePosition >= 0)) ? (pTrueColorStart + nLinePosition) : NULL,
						(pTrueColorEnd), AIColor, csbi, (WriteConsoleW_t)Info->Private);

			if (bSkipBELL)
			{
				pCur++;
				// User may disable flashing in ConEmu settings
				GuiFlashWindow(eFlashBeep, ghConWnd, FALSE, FLASHW_ALL, 1, 0);
			}

			// Update processed pos
			pFrom = pCur + 1;
		}
		else
		{
			_ASSERTE(!bSkipBELL);
		}

		// После BS - сменить "начальную" координату
		if (BSRN)
			x = x2;

		// При смене строки
		if (y2 > y)
		{
			//_ASSERTE(bWrap && L"для !Wrap - доделать");
			if (y2 >= ScrollBottom/*csbi.dwSize.Y*/)
			{
				// Экран прокрутился на одну строку вверх

				// расширенный буфер - прокрутить
				_ASSERTE((y-y2) == -1); // Должен быть сдвиг на одну строку
				ExtScrollScreenParm Shift = {sizeof(Shift), essf_None, h, y-y2, DefClr.Attributes, L' '};
				if (bScrollRegion)
				{
					Shift.Flags |= essf_Region;
					Shift.Region = rcScrollRegion;
					//Shift.Region.top += (y2-y);
					if (ScrollBottom >= csbi.dwSize.Y)
						Shift.Flags |= essf_ExtOnly;
				}
				else
				{
					Shift.Flags |= essf_ExtOnly;
				}
				ExtScrollScreen(&Shift);

				Info->ScrolledRowsUp++;

				// координату - "отмотать" (она как бы не изменилась)
				if (bScrollRegion && (ScrollBottom < csbi.dwSize.Y))
				{
					y2 = LOSHORT(ScrollBottom) - 1;
					_ASSERTEX((int)y2 == (int)(ScrollBottom - 1));

					crScrollCursor.X = x2;
					crScrollCursor.Y = y2;

					SetConsoleCursorPosition(Info->ConsoleOutput, crScrollCursor);

					bNonAutoLfNl = false; // already processed
				}
				else
				{
					y2 = csbi.dwSize.Y - 1;
				}
			}
			else //if (pTrueColorLine)
			{
				// Перейти на следующую строку в расширенном буфере
				nLinePosition += nWindowWidth;
				//pTrueColorLine += nWindowWidth; //-V102
			}
			y = y2;
		}

		// xterm-compatible '\n': just move cursor downward, don't do CR
		if (bNonAutoLfNl)
		{
			_ASSERTE(x2>0);
			bNonAutoLfNl = false;
			crScrollCursor.X = x2;
			crScrollCursor.Y = y2;
			SetConsoleCursorPosition(Info->ConsoleOutput, crScrollCursor);
		}
	}

	if (pCur > pFrom)
	{
		_ASSERTE(!bNonAutoLfNl && "bNonAutoLfNl must be processed already");

		// Printing (NOT including pCur) using extended attributes
		SHORT ForceDumpX = (x2 > x) ? (min(x2, WrapAtCol)-1) : -1;
		DWORD nSplit1 = 0;
		DWORD toWrite = (DWORD)(pCur - pFrom);
		if (bRevertMode && (toWrite > 1))
		{
			// Win-8.1 bug: When cursor is on cell 1 (0-based) and we write {Width-1} spaces
			//              the cursor after write operation goes to wrong position.
			//              For example, when Width=110, CursorX becomes 101.
			if ((toWrite + x) == (DWORD)csbi.dwSize.X)
			{
				toWrite--; nSplit1++;
			}
		}
		lbRc = IntWriteText(h, x, ForceDumpX, pFrom, toWrite,
					 (pTrueColorStart && (nLinePosition >= 0)) ? (pTrueColorStart + nLinePosition) : NULL,
					 (pTrueColorEnd), AIColor, csbi, (WriteConsoleW_t)Info->Private);
		if (nSplit1)
		{

			lbRc |= IntWriteText(h, x+toWrite, ForceDumpX, pFrom+toWrite, nSplit1,
					 (pTrueColorStart && (nLinePosition >= 0)) ? (pTrueColorStart + nLinePosition) : NULL,
					 (pTrueColorEnd), AIColor, csbi, (WriteConsoleW_t)Info->Private);
		}
	}

	if (bRevertMode)
	{
		SetConsoleMode(h, Mode);
	}

	// накрутить индекс в AnnotationInfo
	if (gpTrueColor)
	{
		gpTrueColor->flushCounter++;
	}

	// Commit по флагу
	if (Info->Flags & ewtf_Commit)
	{
		ExtCommitParm c = {sizeof(c), Info->ConsoleOutput};
		ExtCommit(&c);
	}

	if (lbRc)
		Info->NumberOfCharsWritten = Info->NumberOfCharsToWrite;

	return lbRc;
}

BOOL ExtFillOutput(ExtFillOutputParm* Info)
{
	if (!Info || !Info->Flags || !Info->Count)
	{
		_ASSERTE(Info && Info->Flags && Info->Count);
		return FALSE;
	}

	BOOL lbRc = TRUE, b;
	DWORD nWritten;

	BOOL lbTrueColor = ExtCheckBuffers(Info->ConsoleOutput);
	UNREFERENCED_PARAMETER(lbTrueColor);

	HANDLE h = Info->ConsoleOutput;
	CONSOLE_SCREEN_BUFFER_INFO csbi = {};
	SMALL_RECT srWork = {};
	if (!ExtGetBufferInfo(h, csbi, srWork))
		return FALSE;

	_ASSERTE(srWork.Bottom == (csbi.dwSize.Y - 1));
	// Размер TrueColor буфера
	SHORT nWindowWidth  = srWork.Right - srWork.Left + 1;
	SHORT nWindowHeight = srWork.Bottom - srWork.Top + 1; UNREFERENCED_PARAMETER(nWindowHeight);
	//SHORT YShift = csbi.dwCursorPosition.Y - srWork.Top;

	AnnotationInfo* pTrueColorStart = (AnnotationInfo*)(gpTrueColor ? (((LPBYTE)gpTrueColor) + gpTrueColor->struct_size) : NULL); //-V104
	AnnotationInfo* pTrueColorEnd = pTrueColorStart ? (pTrueColorStart + gpTrueColor->bufferSize) : NULL; //-V104
	//AnnotationInfo* pTrueColorLine = (AnnotationInfo*)(pTrueColorStart ? (pTrueColorStart + nWindowWidth * YShift) : NULL); //-V104
	UNREFERENCED_PARAMETER(pTrueColorEnd);


	if (Info->Flags & efof_Attribute)
	{
		if (Info->Flags & efof_Current)
		{
			ExtAttributesParm DefClr = {sizeof(DefClr), h};
			ExtGetAttributes(&DefClr);
			Info->FillAttr = DefClr.Attributes;
		}
		else if (Info->Flags & efof_ResetExt)
		{
			Info->FillAttr.Flags = CECF_NONE;
			// Цвет - без разницы. Будут сброшены только расширенные атрибуты,
			// реальный цвет в консоли оставляем без изменений
			Info->FillAttr.ForegroundColor = 7;
			Info->FillAttr.BackgroundColor = 0;
		}

		AnnotationInfo t = {};
		WORD n = 0;
		ExtPrepareColor(Info->FillAttr, t, n);

		if (gpTrueColor)
		{
			SHORT Y1 = Info->Coord.Y;
			DWORD nEndCell = Info->Coord.X + (Info->Coord.Y * csbi.dwSize.X) + Info->Count;
			SHORT Y2 = (SHORT)(nEndCell / csbi.dwSize.X);
			SHORT X2 = (SHORT)(nEndCell % csbi.dwSize.X);
			TODO("Проверить конец буфера");
			DWORD nMaxLines = gpTrueColor->bufferSize / nWindowWidth;
			if (Y2 >= (int)(srWork.Top + nMaxLines))
			{
				Y2 = (SHORT)(srWork.Top + nMaxLines - 1);
				X2 = nWindowWidth;
			}

			for (SHORT y = max(Y1, srWork.Top); y <= Y2; y++)
			{
				SHORT xMax = (y == Y2) ? X2 : nWindowWidth;
				SHORT xMin = (y == Y1) ? Info->Coord.X : 0;
				TODO("Адресация полной консоли, а не только видимой части...");
				INT_PTR nShift = nWindowWidth * (y - srWork.Top) + xMin;
				if (nShift < 0)
				{
					_ASSERTE((nWindowWidth * (y - srWork.Top) + xMin) > 0);
					continue;
				}
				AnnotationInfo* pTrueColor = pTrueColorStart + nShift;
				for (SHORT x = xMin; x < xMax; x++)
				{
					*(pTrueColor++) = t;
				}
				_ASSERTE(pTrueColor <= pTrueColorEnd);
			}
		}

		if (!(Info->Flags & efof_ResetExt))
		{
			b = FillConsoleOutputAttribute(h, n, Info->Count, Info->Coord, &nWritten);
			lbRc &= b;
		}
	}

	if (Info->Flags & efof_Character)
	{
		b = FillConsoleOutputCharacter(h, Info->FillChar ? Info->FillChar : L' ', Info->Count, Info->Coord, &nWritten);
		lbRc &= b;
	}

	// Commit по флагу
	if (Info->Flags & ewtf_Commit)
	{
		ExtCommitParm c = {sizeof(c), h};
		ExtCommit(&c);
	}

	return lbRc;
}

// From the cursor position!
BOOL ExtScrollLine(ExtScrollScreenParm* Info)
{
	ORIGINALCALLCOUNT_(ExtScrollLine,-1);

	int nDir = (int)Info->Dir;
	if (nDir == 0)
	{
		_ASSERTE(FALSE && "Nothing to do");
		return FALSE;
	}

	HANDLE h = Info->ConsoleOutput;
	BOOL lbTrueColor = ExtCheckBuffers(h);
	UNREFERENCED_PARAMETER(lbTrueColor);

	CONSOLE_SCREEN_BUFFER_INFO csbi = {};
	SMALL_RECT srWork = {};
	if (!ExtGetBufferInfo(h, csbi, srWork))
		return FALSE;


	SHORT nWindowWidth  = srWork.Right - srWork.Left + 1;
	SHORT nWindowHeight = srWork.Bottom - srWork.Top + 1;

	int nMaxCell = min(nWindowWidth * nWindowHeight, gpTrueColor->bufferSize);
	int nMaxRows = nMaxCell / nWindowWidth;
	int nY1 = (csbi.dwCursorPosition.Y - srWork.Top);

	AnnotationInfo* pTrueColorStart = (AnnotationInfo*)(gpTrueColor ? (((LPBYTE)gpTrueColor) + gpTrueColor->struct_size) : NULL); //-V104
	AnnotationInfo* pTrueColorEnd = pTrueColorStart ? (pTrueColorStart + gpTrueColor->bufferSize) : NULL; //-V104
	AnnotationInfo* pTrueColorFrom;
	AnnotationInfo* pTrueColorDest;

	ORIGINAL_KRNL(ScrollConsoleScreenBufferW);

	if (Info->Flags & essf_Current)
	{
		ExtAttributesParm DefClr = {sizeof(DefClr), h};
		ExtGetAttributes(&DefClr);
		Info->FillAttr = DefClr.Attributes;
	}

	if (csbi.dwCursorPosition.X >= (csbi.dwSize.X-1))
	{
		_ASSERTE((csbi.dwCursorPosition.X >= (csbi.dwSize.X - 1)) && "Nothing to scroll?")
	}
	else if (_abs(nDir) >= (csbi.dwSize.X - csbi.dwCursorPosition.X))
	{
		COORD crf = csbi.dwCursorPosition;
		ExtFillOutputParm f0 = {sizeof(f0), efof_Attribute|efof_Character, h,
			Info->FillAttr, Info->FillChar, crf, (csbi.dwSize.X - csbi.dwCursorPosition.X)};
		ExtFillOutput(&f0);

	}
	else
	{
		INT_PTR ccCellsTotal = (csbi.dwSize.X - csbi.dwCursorPosition.X - 1);
		INT_PTR ccCellsMove = ccCellsTotal - _abs(nDir);
		INT_PTR ccCellsClear = _abs(nDir);
		COORD crf = csbi.dwCursorPosition;
		if (nDir < 0)
			crf.X = csbi.dwSize.X + nDir;

		if ((nY1 >= 0) && (nY1 < nMaxRows))
		{
			_ASSERTE((ccCellsTotal > 0) && (ccCellsTotal < 0xFFFF));
			_ASSERTE((ccCellsMove > 0) && (ccCellsClear > 0));

			pTrueColorFrom = pTrueColorStart + (nY1 * nWindowWidth + csbi.dwCursorPosition.X);
			if (nDir < 0)
			{
				pTrueColorDest = pTrueColorFrom;
				pTrueColorFrom += -nDir;
			}
			else // (nDir > 0)
			{
				pTrueColorDest = pTrueColorFrom + nDir;
			}

			// ccCellsMove
			{
				_ASSERTEX((pTrueColorDest+ ccCellsMove)<pTrueColorEnd && pTrueColorDest>=pTrueColorStart);
				_ASSERTEX((pTrueColorFrom+ ccCellsMove)<pTrueColorEnd && pTrueColorFrom>=pTrueColorStart);

				WARNING("OPTIMIZE: Не нужно двигать заполненную нулями память. Нет атрибутов в строке - и не дергаться?");
				memmove(pTrueColorDest, pTrueColorFrom, ccCellsMove * sizeof(*pTrueColorDest));
			}
		}

		// And the real console
		if (!(Info->Flags & essf_ExtOnly))
		{
			SMALL_RECT rcSrc = {csbi.dwCursorPosition.X, csbi.dwCursorPosition.Y, csbi.dwSize.X-1, csbi.dwCursorPosition.Y};
			SMALL_RECT rcClip = rcSrc;
			COORD crDst = MakeCoord(csbi.dwCursorPosition.X + nDir, csbi.dwCursorPosition.Y);
			/*
			if (nDir < 0)
			{
				rcSrc.Left -= nDir; // increase
			}
			else // (nDir > 0)
			{
				rcSrc.Right -= nDir; // decrease
				crDst.X += nDir; // increase
			}
			*/

			AnnotationInfo t = {};
			CHAR_INFO cFill = {{Info->FillChar}};
			ExtPrepareColor(Info->FillAttr, t, cFill.Attributes);

			F(ScrollConsoleScreenBufferW)(h, &rcSrc, &rcClip, crDst, &cFill);

			DEBUGTEST(ZeroStruct(cFill)); // Just for debugger breakpoint
		}

		if (ccCellsClear > 0)
		{
			ExtFillOutputParm f0 = { sizeof(f0), efof_Attribute | efof_Character, h,
				Info->FillAttr, Info->FillChar, crf, ccCellsClear };
			ExtFillOutput(&f0);
		}
	}

	return TRUE;
}

BOOL ExtScrollScreen(ExtScrollScreenParm* Info)
{
	ORIGINALCALLCOUNT_(ExtScrollScreen,-1);
	TODO("!!!scrolling region!!!");

	BOOL lbTrueColor = ExtCheckBuffers(Info->ConsoleOutput);
	UNREFERENCED_PARAMETER(lbTrueColor);

	ORIGINAL_KRNL(ScrollConsoleScreenBufferW);

	HANDLE h = Info->ConsoleOutput;
	CONSOLE_SCREEN_BUFFER_INFO csbi = {};
	SMALL_RECT srWork = {};
	if (!ExtGetBufferInfo(h, csbi, srWork))
		return FALSE;

	BOOL lbRc = TRUE;
	COORD crSize = {csbi.dwSize.X,1};
	COORD cr0 = {};
	//RECT rcRgn = Info->Region;
	int SrcLineTop = 0, SrcLineBottom = 0, DstLineTop = 0;
	int nDir = (int)Info->Dir;

	if (Info->Flags & essf_Global)
	{
		if (Info->Flags & essf_Region)
		{
			RECT rcDest = {};
			if (!IntersectSmallRect(Info->Region, srWork, &rcDest))
				return FALSE; // Nothing to scroll
			// We need relative "working" coordinates here
			Info->Region = MakeRect(rcDest.left, rcDest.top - srWork.Top, rcDest.right, rcDest.bottom - srWork.Top);
		}
	}

	if (Info->Flags & essf_Region)
	{
		_ASSERTEX(Info->Region.left==0 && (Info->Region.right==0 || Info->Region.right==(csbi.dwSize.X-1))); // Not used yet!
		if ((Info->Region.top < 0) || (Info->Region.bottom < Info->Region.top))
		{
			_ASSERTEX(Info->Region.top>=0 && Info->Region.bottom>=Info->Region.top); // Invalid scroll region was requested?
			SetLastError(E_INVALIDARG);
			return FALSE;
		}

		WARNING("ANSI: +srWork.Top ???");

		if (nDir < 0)
		{
			DstLineTop = Info->Region.top;
			SrcLineTop = Info->Region.top + (-nDir);
			SrcLineBottom = Info->Region.bottom;
		}
		else if (nDir > 0)
		{
			DstLineTop = Info->Region.top + nDir;
			SrcLineTop = Info->Region.top;
			SrcLineBottom = Info->Region.bottom - nDir;
		}
	}
	else
	{
		if (nDir < 0)
		{
			DstLineTop = srWork.Top;
			SrcLineTop = srWork.Top + (-nDir);
			SrcLineBottom = csbi.dwSize.Y - 1;
		}
		else if (nDir > 0)
		{
			DstLineTop = srWork.Top + nDir;
			SrcLineTop = srWork.Top;
			SrcLineBottom = (csbi.dwSize.Y - 1) - nDir;
		}
	}

	if (SrcLineBottom < SrcLineTop)
	{
		// We get here if cmd "cls" is called
		int nLines = csbi.dwSize.Y - cr0.Y;
		ExtFillOutputParm f = {sizeof(f), efof_Attribute|efof_Character, Info->ConsoleOutput,
			Info->FillAttr, Info->FillChar, cr0, csbi.dwSize.X * nLines};
		ExtFillOutput(&f);
		return FALSE;
	}


	// Размер TrueColor буфера
	SHORT nWindowWidth  = srWork.Right - srWork.Left + 1;
	SHORT nWindowHeight = srWork.Bottom - srWork.Top + 1;

	AnnotationInfo* pTrueColorStart = (AnnotationInfo*)(gpTrueColor ? (((LPBYTE)gpTrueColor) + gpTrueColor->struct_size) : NULL); //-V104
	DEBUGTEST(AnnotationInfo* pTrueColorEnd = pTrueColorStart ? (pTrueColorStart + gpTrueColor->bufferSize) : NULL); //-V104
	AnnotationInfo* pTrueColorFrom;
	AnnotationInfo* pTrueColorDest;

	if (Info->Flags & essf_Current)
	{
		ExtAttributesParm DefClr = {sizeof(DefClr), h};
		ExtGetAttributes(&DefClr);
		Info->FillAttr = DefClr.Attributes;
	}

	if (Info->Flags & essf_Pad)
	{
		_ASSERTE(Info->Dir == -1);
		nDir = -1;

		// Пробелами добивать? Хотя вроде и не нужно. Это при "виртуальном" врапе на заданную границу зовется.

		/*
		COORD crFrom = {csbi.dwCursorPosition.X, csbi.dwCursorPosition.Y};
		nCount = csbi.dwSize.X - csbi.dwCursorPosition.X;

		ExtFillOutputParm f = {sizeof(f), efof_Attribute|efof_Character, Info->ConsoleOutput,
			Info->FillAttr, Info->FillChar, crFrom, nCount};
		lbRc = ExtFillOutput(&f);

		if (lbRc)
			nDir = -1;
		*/
	}


	WARNING("+ регионы нужно по разному считать:");
	WARNING("  если оно пришло из ANSI - то видимый участок");
	WARNING("  если оно пришло из WinAPI - то консольный буфер полностью (точнее то, что указано)");


	// Go
	if (nDir < 0)
	{
		// Scroll whole page up by n (default 1) lines. New lines are added at the bottom.
		if (gpTrueColor)
		{
			int nMaxCell = min(nWindowWidth * nWindowHeight,gpTrueColor->bufferSize);
			int nMaxRows = nMaxCell / nWindowWidth;
			//_ASSERTEX(SrcLineTop >= srWork.Top); That will be if visible region is ABOVE our TrueColor region
			int nShiftRows = -nDir; // 1
			int nY1 = min(max((SrcLineTop - srWork.Top),nShiftRows),nMaxRows);    // 2
			int nY2 = min(max((SrcLineBottom - srWork.Top),nShiftRows),nMaxRows); // 6
			int nRows = nY2 - nY1 + 1; // 5

			if (nRows > 0)
			{
				//if (nRows > -nDir)
				{
					pTrueColorDest = pTrueColorStart + ((nY1-nShiftRows) * nWindowWidth);
					pTrueColorFrom = pTrueColorStart + (nY1 * nWindowWidth);

					size_t ccCells = nRows * nWindowWidth;
					_ASSERTEX((pTrueColorDest+ccCells)<pTrueColorEnd && pTrueColorDest>=pTrueColorStart);
					_ASSERTEX((pTrueColorFrom+ccCells)<pTrueColorEnd && pTrueColorFrom>=pTrueColorStart);
					_ASSERTEX(pTrueColorDest < pTrueColorFrom);

					WARNING("OPTIMIZE: Не нужно двигать заполненную нулями память. Нет атрибутов в строке - и не дергаться?");
					memmove(pTrueColorDest, pTrueColorFrom, ccCells * sizeof(*pTrueColorDest));
				}

				if (Info->Flags & essf_ExtOnly)
				{
					#if 1
					AnnotationInfo AIInfo = {};
					for (int i = (nWindowHeight-nShiftRows)*nWindowWidth; i < nMaxCell; i++)
					{
						pTrueColorStart[i] = AIInfo;
					}
					#else
					_ASSERTE(FALSE && "Continue to fill");
					cr0.Y = max(0,(nWindowHeight+nDir));
					int nLines = (-nDir);
					ExtFillOutputParm f = {sizeof(f), efof_Attribute|efof_Character, Info->ConsoleOutput,
						Info->FillAttr, Info->FillChar, cr0, csbi.dwSize.X * nLines};
					ExtFillOutput(&f);
					#endif
				}
			}
			else
			{
				_ASSERTEX(nRows > 0);
			}
		}

		if (!(Info->Flags & essf_ExtOnly))
		{
			SMALL_RECT rcSrc = MakeSmallRect(0, SrcLineTop, csbi.dwSize.X-1, SrcLineBottom);
			COORD crDst = MakeCoord(0, SrcLineTop + nDir/*<0*/);
			AnnotationInfo t = {};
			CHAR_INFO cFill = {{Info->FillChar}};
			ExtPrepareColor(Info->FillAttr, t, cFill.Attributes);

			F(ScrollConsoleScreenBufferW)(Info->ConsoleOutput, &rcSrc, NULL, crDst, &cFill);

			if (nDir < 0)
			{
				cr0.Y = max(0,(SrcLineBottom + nDir + 1));
				int nLines = SrcLineBottom - cr0.Y + 1;
				ExtFillOutputParm f = {sizeof(f), efof_Attribute|efof_Character, Info->ConsoleOutput,
					Info->FillAttr, Info->FillChar, cr0, csbi.dwSize.X * nLines};
				ExtFillOutput(&f);
			}
		}
	}
	else if (nDir > 0)
	{
		// Scroll whole page down by n (default 1) lines. New lines are added at the top.
		if (gpTrueColor)
		{
			int nMaxCell = min(nWindowWidth * nWindowHeight,gpTrueColor->bufferSize);
			int nMaxRows = nMaxCell / nWindowWidth;
			//_ASSERTEX(SrcLineTop >= srWork.Top); That will be if visible region is ABOVE our TrueColor region
			int nShiftRows = nDir;
			int nMaxRowsShift = nMaxRows - nShiftRows;
			int nY1 = min(max((SrcLineTop - srWork.Top),0),nMaxRowsShift);
			int nY2 = min(max((SrcLineBottom - srWork.Top),0),nMaxRowsShift);
			int nRows = nY2 - nY1 + 1;

			if (nRows > 0)
			{
				//if (nRows >= nDir)
				{
					pTrueColorDest = pTrueColorStart+((nY1 + nDir) * nWindowWidth);
					pTrueColorFrom = pTrueColorStart + (nY1 * nWindowWidth);

					size_t ccCells = nRows * nWindowWidth;
					_ASSERTEX((pTrueColorDest+ccCells)<pTrueColorEnd && pTrueColorDest>=pTrueColorStart);
					_ASSERTEX((pTrueColorFrom+ccCells)<pTrueColorEnd && pTrueColorFrom>=pTrueColorStart);
					_ASSERTEX(pTrueColorDest > pTrueColorFrom);

					WARNING("OPTIMIZE: Не нужно двигать заполненную нулями память. Нет атрибутов в строке - и не дергаться.");
					memmove(pTrueColorDest, pTrueColorFrom, ccCells * sizeof(*pTrueColorStart));
				}

				if (Info->Flags & essf_ExtOnly)
				{
					TODO("Чем заливать");
					AnnotationInfo AIInfo = {};
					for (int i = (nDir*nWindowWidth)-1; i > 0; i--)
					{
						pTrueColorStart[i] = AIInfo;
					}
				}
			}
		}

		if (!(Info->Flags & essf_ExtOnly))
		{
			SMALL_RECT rcSrc = MakeSmallRect(0, SrcLineTop, csbi.dwSize.X-1, SrcLineBottom);
			COORD crDst = MakeCoord(0, SrcLineTop + nDir/*>0*/);
			AnnotationInfo t = {};
			CHAR_INFO cFill = {{Info->FillChar}};
			ExtPrepareColor(Info->FillAttr, t, cFill.Attributes);

			F(ScrollConsoleScreenBufferW)(Info->ConsoleOutput, &rcSrc, NULL, crDst, &cFill);

			if (nDir > 0)
			{
				cr0.Y = SrcLineTop;
				ExtFillOutputParm f = {sizeof(f), efof_Attribute|efof_Character, Info->ConsoleOutput,
					Info->FillAttr, Info->FillChar, cr0, csbi.dwSize.X * nDir};
				ExtFillOutput(&f);
			}
		}
	}

	// накрутить индекс в AnnotationInfo
	if (gpTrueColor)
	{
		gpTrueColor->flushCounter++;
	}

	// Commit по флагу
	if (Info->Flags & essf_Commit)
	{
		ExtCommitParm c = {sizeof(c), Info->ConsoleOutput};
		ExtCommit(&c);
	}

	return lbRc;
}



BOOL ExtCommit(ExtCommitParm* Info)
{
	if (!Info)
	{
		_ASSERTE(Info!=NULL);
		SetLastError(E_INVALIDARG);
		return FALSE;
	}

	TODO("ExtCommit: Info->ConsoleOutput");

	// Если буфер не был создан - то и передергивать нечего
	if (gpTrueColor != NULL)
	{
		if (gpTrueColor->locked)
		{
			gpTrueColor->flushCounter++;
			gpTrueColor->locked = FALSE;
		}
	}

	return TRUE;
}
