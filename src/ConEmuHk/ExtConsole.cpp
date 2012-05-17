
/*
Copyright (c) 2012 Maximus5
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

#include <windows.h>
#include "defines.h"
#include "ConsoleAnnotation.h"
#include "ConEmuColors3.h"
#include "common.hpp"
#include "ConEmuCheck.h"
#include "UnicodeChars.h"
#include "version.h"
#include "ExtConsole.h"
#include "resource.h"

#define MSG_TITLE "ConEmu writer"
#define MSG_INVALID_CONEMU_VER "Unsupported ConEmu version detected!\nRequired version: " CONEMUVERS "\nConsole writer'll works in 4bit mode"
#define MSG_TRUEMOD_DISABLED   "«Colorer TrueMod support» is not checked in the ConEmu settings\nConsole writer'll works in 4bit mode"
#define MSG_NO_TRUEMOD_BUFFER  "TrueMod support not enabled in the ConEmu settings\nConsole writer'll works in 4bit mode"


static HWND    ghExtConEmuWndDC = NULL; // VirtualCon. инициализируется в CheckBuffers()


///* extern для MAssert, Здесь НЕ используется */
///* */       HWND ghConEmuWnd = NULL;      /* */
///* extern для MAssert, Здесь НЕ используется */

#ifdef USE_COMMIT_EVENT
HWND    ghRealConWnd = NULL;
bool    gbBatchStarted = false;
HANDLE  ghBatchEvent = NULL;
DWORD   gnBatchRegPID = 0;
#endif

CESERVER_CONSOLE_MAPPING_HDR SrvMapping = {};

AnnotationHeader* gpTrueColor = NULL;
HANDLE ghTrueColor = NULL;
BOOL CheckBuffers(bool abWrite = false);
void CloseBuffers();

bool gbInitialized = false;
bool gbFarBufferMode = false; // true, если Far запущен с ключом "/w"

struct ExtCurrentAttr
{
	bool  WasSet;
	bool  Reserved1;
	WORD  CONColor;
	
	#ifdef _WIN64
	DWORD Reserved2;
	#endif

	ConEmuColor CEColor;
	AnnotationInfo AIColor;
} gExtCurrentAttr;


bool isCharSpace(wchar_t inChar)
{
	// Сюда пихаем все символы, которые можно отрисовать пустым фоном (как обычный пробел)
	bool isSpace = (inChar == ucSpace || inChar == ucNoBreakSpace || inChar == 0 
		/*|| (inChar>=0x2000 && inChar<=0x200F)
		|| inChar == 0x2060 || inChar == 0x3000 || inChar == 0xFEFF*/);
	return isSpace;
}


BOOL GetBufferInfo(HANDLE &h, CONSOLE_SCREEN_BUFFER_INFO &csbi, SMALL_RECT &srWork)
{
	_ASSERTE(gbInitialized);
	
	h = GetStdHandle(STD_OUTPUT_HANDLE);
	if (!GetConsoleScreenBufferInfo(h, &csbi))
		return FALSE;
	
	if (gbFarBufferMode)
	{
		// Фар занимает нижнюю часть консоли. Прилеплен к левому краю
		SHORT nWidth  = csbi.srWindow.Right - csbi.srWindow.Left + 1;
		SHORT nHeight = csbi.srWindow.Bottom - csbi.srWindow.Top + 1;
		srWork.Left = 0;
		srWork.Right = nWidth - 1;
		srWork.Top = csbi.dwSize.Y - nHeight;
		srWork.Bottom = csbi.dwSize.Y - 1;
	}
	else
	{
		// Фар занимает все поле консоли
		srWork.Left = srWork.Top = 0;
		srWork.Right = csbi.dwSize.X - 1;
		srWork.Bottom = csbi.dwSize.Y - 1;
	}
	
	if (srWork.Left < 0 || srWork.Top < 0 || srWork.Left > srWork.Right || srWork.Top > srWork.Bottom)
	{
		_ASSERTE(srWork.Left >= 0 && srWork.Top >= 0 && srWork.Left <= srWork.Right && srWork.Top <= srWork.Bottom);
		return FALSE;
	}
	
	return TRUE;
}

BOOL CheckBuffers(bool abWrite /*= false*/)
{
	if (!gbInitialized)
	{
		HANDLE h = GetStdHandle(STD_OUTPUT_HANDLE);
		CONSOLE_SCREEN_BUFFER_INFO csbi = {};
		if (GetConsoleScreenBufferInfo(h, &csbi))
		{
			SHORT nWidth  = csbi.srWindow.Right - csbi.srWindow.Left + 1;
			SHORT nHeight = csbi.srWindow.Bottom - csbi.srWindow.Top + 1;
			_ASSERTE((nWidth <= csbi.dwSize.X) && (nHeight <= csbi.dwSize.Y));
			gbFarBufferMode = (nWidth < csbi.dwSize.X) || (nHeight < csbi.dwSize.Y);
			gbInitialized = true;
		}
	}

	//TODO: Проверить, не изменился ли HWND консоли?
	//111101 - зовем именно GetConsoleWindow. Если она перехвачена - вернет как раз то окно, которое нам требуется
	HWND hCon = /*GetConsoleWindow(); */ GetConEmuHWND(0);
	if (!hCon)
	{
		CloseBuffers();
		SetLastError(E_HANDLE);
		return FALSE;
	}
	if (hCon != ghExtConEmuWndDC)
	{
		#ifdef _DEBUG
		// Функция GetConsoleWindow должна быть перехвачена в ConEmuHk, проверим
		ghExtConEmuWndDC = GetConsoleWindow();
		_ASSERTE(ghExtConEmuWndDC == hCon);
		#endif

		ghExtConEmuWndDC = hCon;
		CloseBuffers();

		#ifdef USE_COMMIT_EVENT
		ghRealConWnd = GetConEmuHWND(2);
		#endif
		
		//TODO: Пока работаем "по-старому", через буфер TrueColor. Переделать, он не оптимален
		wchar_t szMapName[128];
		wsprintf(szMapName, AnnotationShareName, (DWORD)sizeof(AnnotationInfo), (DWORD)hCon); //-V205
		ghTrueColor = OpenFileMapping(FILE_MAP_ALL_ACCESS, FALSE, szMapName);
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
		
		#ifdef USE_COMMIT_EVENT
		if (!LoadSrvMapping(ghRealConWnd, SrvMapping))
		{
			CloseBuffers();
			SetLastError(E_HANDLE);
			return FALSE;
		}
		#endif
	}
	if (gpTrueColor)
	{
		if (!gpTrueColor->locked)
		{
			//TODO: Сбросить флаги валидности ячеек?
			gpTrueColor->locked = TRUE;
		}
		
		#ifdef USE_COMMIT_EVENT
		if (!gbBatchStarted)
		{
			gbBatchStarted = true;
			
			if (!ghBatchEvent)
				ghBatchEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
				
			if (ghBatchEvent)
			{
				ResetEvent(ghBatchEvent);
				
				// Если еще не регистрировались...
				if (!gnBatchRegPID || gnBatchRegPID != SrvMapping.nServerPID)
				{
					gnBatchRegPID = SrvMapping.nServerPID;
					CESERVER_REQ* pIn = ExecuteNewCmd(CECMD_REGEXTCONSOLE, sizeof(CESERVER_REQ_HDR)+sizeof(CESERVER_REQ_REGEXTCON));
					if (pIn)
					{
						HANDLE hServer = OpenProcess(PROCESS_DUP_HANDLE, FALSE, gnBatchRegPID);
						if (hServer)
						{
							HANDLE hDupEvent = NULL;
							if (DuplicateHandle(GetCurrentProcess(), ghBatchEvent, hServer, &hDupEvent, 0, FALSE, DUPLICATE_SAME_ACCESS))
							{
								pIn->RegExtCon.hCommitEvent = hDupEvent;
								CESERVER_REQ* pOut = ExecuteSrvCmd(gnBatchRegPID, pIn, ghRealConWnd);
								if (pOut)
									ExecuteFreeResult(pOut);
							}
							CloseHandle(hServer);
						}
						ExecuteFreeResult(pIn);
					}
				}
				
			}
		}
		#endif
	}
	
	return (gpTrueColor!=NULL);
}

void CloseBuffers()
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
BOOL WINAPI GetTextAttributes(ConEmuColor* Attributes)
{
	BOOL lbTrueColor = CheckBuffers();
	UNREFERENCED_PARAMETER(lbTrueColor);
	
	CONSOLE_SCREEN_BUFFER_INFO csbi = {};
	SMALL_RECT srWork = {};
	HANDLE h;
	if (!GetBufferInfo(h, csbi, srWork))
	{
		gExtCurrentAttr.WasSet = false;
		return FALSE;
	}

	// Что-то в некоторых случаях сбивается цвет вывода для printf
	//_ASSERTE("GetTextAttributes" && ((csbi.wAttributes & 0xFF) == 0x07));

	TODO("По хорошему, gExtCurrentAttr нада ветвить по разным h");
	if (gExtCurrentAttr.WasSet && (csbi.wAttributes == gExtCurrentAttr.CONColor))
	{
		*Attributes = gExtCurrentAttr.AIColor;
	}
	else
	{
		gExtCurrentAttr.WasSet = false;
		Attributes->Flags = FCF_FG_4BIT|FCF_BG_4BIT;
		Attributes->ForegroundColor = (csbi.wAttributes & 0xF);
		Attributes->BackgroundColor = (csbi.wAttributes & 0xF0) >> 4;
	}

	return TRUE;
	
	//WORD nDefReadBuf[1024];
	//WORD *pnReadAttr = NULL;
	//int nBufWidth  = csbi.srWindow.Right - csbi.srWindow.Left + 1;
	//int nBufHeight = csbi.srWindow.Bottom - csbi.srWindow.Top + 1;
	//int nBufCount  = nBufWidth*nBufHeight;
	//if (nBufWidth <= countof(nDefReadBuf))
	//	pnReadAttr = nDefReadBuf;
	//else
	//	pnReadAttr = (WORD*)calloc(nBufCount,sizeof(*pnReadAttr));
	//if (!pnReadAttr)
	//{
	//	SetLastError(E_OUTOFMEMORY);
	//	return FALSE;
	//}
	//
	//BOOL lbRc = TRUE;
	//COORD cr = {csbi.srWindow.Left, csbi.srWindow.Top};
	//DWORD nRead;
//
	//AnnotationInfo* pTrueColor = (AnnotationInfo*)(gpTrueColor ? (((LPBYTE)gpTrueColor) + gpTrueColor->struct_size) : NULL);
	//AnnotationInfo* pTrueColorEnd = pTrueColor ? (pTrueColor + gpTrueColor->bufferSize) : NULL;
//
	//for (;cr.Y <= csbi.srWindow.Bottom;cr.Y++)
	//{
	//	BOOL lbRead = ReadConsoleOutputAttribute(h, pnReadAttr, nBufWidth, cr, &nRead) && (nRead == nBufWidth);
//
	//	FarColor clr = {};
	//	WORD* pn = pnReadAttr;
	//	for (int X = 0; X < nBufWidth; X++, pn++)
	//	{
	//		clr.Flags = 0;
//
	//		if (pTrueColor && pTrueColor >= pTrueColorEnd)
	//		{
	//			_ASSERTE(pTrueColor && pTrueColor < pTrueColorEnd);
	//			pTrueColor = NULL; // Выделенный буфер оказался недостаточным
	//		}
	//		
	//		if (pTrueColor)
	//		{
	//			DWORD Style = pTrueColor->style;
	//			if (Style & AI_STYLE_BOLD)
	//				clr.Flags |= FCF_FG_BOLD;
	//			if (Style & AI_STYLE_ITALIC)
	//				clr.Flags |= FCF_FG_ITALIC;
	//			if (Style & AI_STYLE_UNDERLINE)
	//				clr.Flags |= FCF_FG_UNDERLINE;
	//		}
//
	//		if (pTrueColor && pTrueColor->fg_valid)
	//		{
	//			clr.ForegroundColor = pTrueColor->fg_color;
	//		}
	//		else
	//		{
	//			clr.Flags |= FCF_FG_4BIT;
	//			clr.ForegroundColor = lbRead ? ((*pn) & 0xF) : 7;
	//		}
//
	//		if (pTrueColor && pTrueColor->bk_valid)
	//		{
	//			clr.BackgroundColor = pTrueColor->bk_color;
	//		}
	//		else
	//		{
	//			clr.Flags |= FCF_BG_4BIT;
	//			clr.BackgroundColor = lbRead ? (((*pn) & 0xF0) >> 4) : 0;
	//		}
//
	//		//*(Attributes++) = clr; -- <G|S>etTextAttributes должны ожидать указатель на ОДИН FarColor.
	//		if (pTrueColor)
	//			pTrueColor++;
	//	}
	//}
	//
	//if (pnReadAttr != nDefReadBuf)
	//	free(pnReadAttr);
	
	//return lbRc;
}

// Это это "цвет по умолчанию".
// То, что соответствует "Screen Text" и "Screen Background" в свойствах консоли.
// То, что задаётся командой color в cmd.exe.
// То, что будет использовано если в консоль просто писать 
// по printf/std::cout/WriteConsole, без явного указания цвета.
BOOL WINAPI SetTextAttributes(const FarColor* Attributes)
{
	if (!Attributes)
	{
		// ConEmu internals: сбросить запомненный "атрибут"
		gExtCurrentAttr.WasSet = false;
		return TRUE;
	}

	BOOL lbTrueColor = CheckBuffers();
	UNREFERENCED_PARAMETER(lbTrueColor);

	CONSOLE_SCREEN_BUFFER_INFO csbi = {};
	SMALL_RECT srWork = {};
	HANDLE h;
	if (!GetBufferInfo(h, csbi, srWork))
		return FALSE;
	
	BOOL lbRc = TRUE;
	
	#ifdef _DEBUG
	AnnotationInfo* pTrueColor = (AnnotationInfo*)(gpTrueColor ? (((LPBYTE)gpTrueColor) + gpTrueColor->struct_size) : NULL);
	AnnotationInfo* pTrueColorEnd = pTrueColor ? (pTrueColor + gpTrueColor->bufferSize) : NULL;
	#endif

	// <G|S>etTextAttributes должны ожидать указатель на ОДИН FarColor.
	AnnotationInfo t = {};
	WORD n = 0, f = 0;
	unsigned __int64 Flags = Attributes->Flags;

	if (Flags & FCF_FG_BOLD)
		f |= AI_STYLE_BOLD;
	if (Flags & FCF_FG_ITALIC)
		f |= AI_STYLE_ITALIC;
	if (Flags & FCF_FG_UNDERLINE)
		f |= AI_STYLE_UNDERLINE;
	t.style = f;

	DWORD nForeColor, nBackColor;
	if (Flags & FCF_FG_4BIT)
	{
		nForeColor = -1;
		n |= (WORD)(Attributes->ForegroundColor & 0xF);
		t.fg_valid = FALSE;
	}
	else
	{
		//n |= 0x07;
		nForeColor = Attributes->ForegroundColor & 0x00FFFFFF;
		Far3Color::Color2FgIndex(nForeColor, n);
		t.fg_color = nForeColor;
		t.fg_valid = TRUE;
	}

	if (Flags & FCF_BG_4BIT)
	{
		n |= (WORD)(Attributes->BackgroundColor & 0xF)<<4;
		t.bk_valid = FALSE;
	}
	else
	{
		nBackColor = Attributes->BackgroundColor & 0x00FFFFFF;
		Far3Color::Color2BgIndex(nBackColor, nBackColor==nForeColor, n);
		t.bk_color = nBackColor;
		t.bk_valid = TRUE;
	}
	
	SetConsoleTextAttribute(h, n);
	
	TODO("По хорошему, gExtCurrentAttr нада ветвить по разным h");
	// запомнить, что WriteConsole должен писать атрибутом "t"
	gExtCurrentAttr.WasSet = true;
	gExtCurrentAttr.CONColor = n;
	gExtCurrentAttr.CEColor = *Attributes;
	gExtCurrentAttr.AIColor = t;
	
	return lbRc;
}

BOOL WINAPI WriteOutput(const FAR_CHAR_INFO* Buffer, COORD BufferSize, COORD BufferCoord, SMALL_RECT* WriteRegion)
{
	BOOL lbTrueColor = CheckBuffers(true);
	UNREFERENCED_PARAMETER(lbTrueColor);
	/*
	struct FAR_CHAR_INFO
	{
		WCHAR Char;
		struct FarColor Attributes;
	};
	typedef struct _CHAR_INFO {
	  union {
	    WCHAR UnicodeChar;
	    CHAR AsciiChar;
	  } Char;
	  WORD  Attributes;
	}CHAR_INFO, *PCHAR_INFO;
	BOOL WINAPI WriteConsoleOutput(
	  __in     HANDLE hConsoleOutput,
	  __in     const CHAR_INFO *lpBuffer,
	  __in     COORD dwBufferSize,
	  __in     COORD dwBufferCoord,
	  __inout  PSMALL_RECT lpWriteRegion
	);
	*/

	CONSOLE_SCREEN_BUFFER_INFO csbi = {};
	SMALL_RECT srWork = {};
	HANDLE h;
	if (!GetBufferInfo(h, csbi, srWork))
		return FALSE;

	CHAR_INFO cDefWriteBuf[1024];
	CHAR_INFO *pcWriteBuf = NULL;
	int nWindowWidth  = srWork.Right - srWork.Left + 1;
	if (BufferSize.X <= (int)countof(cDefWriteBuf))
		pcWriteBuf = cDefWriteBuf;
	else
		pcWriteBuf = (CHAR_INFO*)calloc(BufferSize.X,sizeof(*pcWriteBuf));
	if (!pcWriteBuf)
	{
		SetLastError(E_OUTOFMEMORY);
		return FALSE;
	}

	BOOL lbRc = TRUE;

	AnnotationInfo* pTrueColorStart = (AnnotationInfo*)(gpTrueColor ? (((LPBYTE)gpTrueColor) + gpTrueColor->struct_size) : NULL); //-V104
	AnnotationInfo* pTrueColorEnd = pTrueColorStart ? (pTrueColorStart + gpTrueColor->bufferSize) : NULL; //-V104
	AnnotationInfo* pTrueColorLine = (AnnotationInfo*)(pTrueColorStart ? (pTrueColorStart + nWindowWidth * (WriteRegion->Top /*- srWork.Top*/)) : NULL); //-V104

	SMALL_RECT rcWrite = *WriteRegion;
	COORD MyBufferSize = {BufferSize.X, 1};
	COORD MyBufferCoord = {BufferCoord.X, 0};
	SHORT YShift = gbFarBufferMode ? (csbi.dwSize.Y - (srWork.Bottom - srWork.Top + 1)) : 0;
	SHORT Y1 = WriteRegion->Top + YShift;
	SHORT Y2 = WriteRegion->Bottom + YShift;
	SHORT BufferShift = WriteRegion->Top + YShift;

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

		CHAR_INFO* pc = pcWriteBuf + BufferCoord.X;
		const FAR_CHAR_INFO* pFar = Buffer + (rcWrite.Top - BufferShift + BufferCoord.Y)*BufferSize.X + BufferCoord.X; //-V104
		AnnotationInfo* pTrueColor = (pTrueColorLine && (pTrueColorLine >= pTrueColorStart)) ? (pTrueColorLine + WriteRegion->Left) : NULL;

		for (int X = rcWrite.Left; X <= rcWrite.Right; X++, pc++, pFar++)
		{
			pc->Char.UnicodeChar = pFar->Char;

			if (pTrueColor && pTrueColor >= pTrueColorEnd)
			{
				#ifdef _DEBUG
				static bool bBufferAsserted = false;
				if (!bBufferAsserted)
				{
					bBufferAsserted = true;
					_ASSERTE(pTrueColor && pTrueColor < pTrueColorEnd);
				}
				#endif
				pTrueColor = NULL; // Выделенный буфер оказался недостаточным
			}
			
			WORD n = 0, f = 0;
			unsigned __int64 Flags = pFar->Attributes.Flags;
			BOOL Fore4bit = (Flags & FCF_FG_4BIT);
			
			if (pTrueColor)
			{
				if (Flags & FCF_FG_BOLD)
					f |= AI_STYLE_BOLD;
				if (Flags & FCF_FG_ITALIC)
					f |= AI_STYLE_ITALIC;
				if (Flags & FCF_FG_UNDERLINE)
					f |= AI_STYLE_UNDERLINE;
				pTrueColor->style = f;
			}

			DWORD nForeColor, nBackColor;
			if (Fore4bit)
			{
				nForeColor = -1;
				n |= (WORD)(pFar->Attributes.ForegroundColor & 0xF);
				if (pTrueColor)
				{
					pTrueColor->fg_valid = FALSE;
				}
			}
			else
			{
				//n |= 0x07;
				nForeColor = pFar->Attributes.ForegroundColor & 0x00FFFFFF;
				Far3Color::Color2FgIndex(nForeColor, n);
				if (pTrueColor)
				{
					pTrueColor->fg_color = nForeColor;
					pTrueColor->fg_valid = TRUE;
				}
			}
			
			if (Flags & FCF_BG_4BIT)
			{
				nBackColor = -1;
				WORD bk = (WORD)(pFar->Attributes.BackgroundColor & 0xF);
				// Коррекция яркости, если подобранные индексы совпали
				if (n == bk && !Fore4bit && !isCharSpace(pFar->Char))
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
				nBackColor = pFar->Attributes.BackgroundColor & 0x00FFFFFF;
				Far3Color::Color2BgIndex(nBackColor, nForeColor==nBackColor, n);
				if (pTrueColor)
				{
					pTrueColor->bk_color = nBackColor;
					pTrueColor->bk_valid = TRUE;
				}
			}
			
			
			
			pc->Attributes = n;

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

	if (pcWriteBuf != cDefWriteBuf)
		free(pcWriteBuf);

	return lbRc;
}

BOOL WINAPI WriteText(HANDLE hConsoleOutput, const AnnotationInfo* Attributes, const wchar_t* Buffer, DWORD nNumberOfCharsToWrite, LPDWORD lpNumberOfCharsWritten)
{
	// Ограничение Short - на функции WriteConsoleOutput
	if (!Buffer || !nNumberOfCharsToWrite || (nNumberOfCharsToWrite >= 0x8000))
		return FALSE;

	BOOL lbRc = FALSE;

	TODO("Отвязаться от координат видимой области");

	FarColor FarAttributes = {};
	GetTextAttributes(&FarAttributes);

	if (Attributes)
	{
		if (Attributes->bk_valid)
		{
			FarAttributes.Flags &= ~FCF_BG_4BIT;
			FarAttributes.BackgroundColor = Attributes->bk_color;
		}

		if (Attributes->fg_valid)
		{
			FarAttributes.Flags &= ~FCF_FG_4BIT;
			FarAttributes.ForegroundColor = Attributes->fg_color;
		}

		// Bold
		if (Attributes->style & AI_STYLE_BOLD)
			FarAttributes.Flags |= FCF_FG_BOLD;
		else
			FarAttributes.Flags &= ~FCF_FG_BOLD;

		// Italic
		if (Attributes->style & AI_STYLE_ITALIC)
			FarAttributes.Flags |= FCF_FG_ITALIC;
		else
			FarAttributes.Flags &= ~FCF_FG_ITALIC;

		// Underline
		if (Attributes->style & AI_STYLE_UNDERLINE)
			FarAttributes.Flags |= FCF_FG_UNDERLINE;
		else
			FarAttributes.Flags &= ~FCF_FG_UNDERLINE;
	}

	CONSOLE_SCREEN_BUFFER_INFO csbi = {};
	SMALL_RECT srWork = {};
	HANDLE h;
	if (!GetBufferInfo(h, csbi, srWork))
		return FALSE;

	FAR_CHAR_INFO* lpFarBuffer = (FAR_CHAR_INFO*)malloc(nNumberOfCharsToWrite * sizeof(*lpFarBuffer));
	if (!lpFarBuffer)
		return FALSE;

	TODO("Обработка символов \t\n\r?");

	FAR_CHAR_INFO* lp = lpFarBuffer;
	for (DWORD i = 0; i < nNumberOfCharsToWrite; ++i, ++Buffer, ++lp)
	{
		lp->Char = *Buffer;
		lp->Attributes = FarAttributes;
	}

	//const FAR_CHAR_INFO* Buffer, COORD BufferSize, COORD BufferCoord, SMALL_RECT* WriteRegion
	COORD BufferSize = {(SHORT)nNumberOfCharsToWrite, 1};
	COORD BufferCoord = {0,0};
	SMALL_RECT WriteRegion = {csbi.dwCursorPosition.X, csbi.dwCursorPosition.Y, csbi.dwCursorPosition.X+(SHORT)nNumberOfCharsToWrite-1, csbi.dwCursorPosition.Y};
	if (WriteRegion.Right >= csbi.dwSize.X)
	{
		_ASSERTE(WriteRegion.Right < csbi.dwSize.X);
		WriteRegion.Right = csbi.dwSize.X - 1;
	}
	if (WriteRegion.Right > WriteRegion.Left)
	{
		return FALSE;
	}
	lbRc = WriteOutput(lpFarBuffer, BufferSize, BufferCoord, &WriteRegion);

	free(lpFarBuffer);

	// Обновить позицию курсора
	_ASSERTE((csbi.dwCursorPosition.X+(int)nNumberOfCharsToWrite-1) < csbi.dwSize.X);
	csbi.dwCursorPosition.X = (SHORT)max((csbi.dwSize.X-1),(csbi.dwCursorPosition.X+(int)nNumberOfCharsToWrite-1));
	SetConsoleCursorPosition(h, csbi.dwCursorPosition);

	#if 0
	typedef BOOL (WINAPI* WriteConsoleW_t)(HANDLE hConsoleOutput, const VOID *lpBuffer, DWORD nNumberOfCharsToWrite, LPDWORD lpNumberOfCharsWritten, LPVOID lpReserved);
	static WriteConsoleW_t fnWriteConsoleW = NULL;
	typedef FARPROC (WINAPI* GetWriteConsoleW_t)();

	if (!fnWriteConsoleW)
	{
		HANDLE hHooks = GetModuleHandle(WIN3264TEST(L"ConEmuHk.dll",L"ConEmuHk64.dll"));
		if (hHooks)
		{
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
			fnWriteConsoleW = WriteConsole;
		}
	}
	
	lbRc = fnWriteConsoleW(hConsoleOutput, Buffer, nNumberOfCharsToWrite, lpNumberOfCharsWritten,NULL);
	#endif

	return lbRc;
}


BOOL WINAPI Commit()
{
	// Если буфер не был создан - то и передергивать нечего
	if (gpTrueColor != NULL)
	{
		if (gpTrueColor->locked)
		{
			gpTrueColor->flushCounter++;
			gpTrueColor->locked = FALSE;
		}
	}
	
	#ifdef USE_COMMIT_EVENT
	// "Отпустить" сервер
	if (ghBatchEvent)
		SetEvent(ghBatchEvent);
	gbBatchStarted = false;
	#endif
	
	return TRUE; //TODO: А чего возвращать-то?
}
