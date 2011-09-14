
#include <windows.h>
#include <crtdbg.h>

#pragma comment(lib, "Comdlg32.lib")

#define MASSERT_HEADER_DEFINED
#define MEMORY_HEADER_DEFINED

#include "../common/pluginW1900.hpp"
#include "../common/ConsoleAnnotation.h"
#include "../common/ConEmuColors3.h"
#include "../common/common.hpp"
#include "../common/UnicodeChars.h"
#include "../ConEmu/version.h"
#include "ConEmuDw.h"
#include "resource.h"

#define MSG_TITLE "ConEmu writer"
#define MSG_INVALID_CONEMU_VER "Unsupported ConEmu version detected!\nRequired version: " CONEMUVERS "\nConsole writer'll works in 4bit mode"
#define MSG_TRUEMOD_DISABLED   "«Colorer TrueMod support» is not checked in the ConEmu settings\nConsole writer'll works in 4bit mode"
#define MSG_NO_TRUEMOD_BUFFER  "TrueMod support not enabled in the ConEmu settings\nConsole writer'll works in 4bit mode"

// "Прозрачность" в фаре хранится в старшем байте,
// используется (пока) только при обработке раскраски файлов
#define IsTransparent(C) (((C) & 0xFF000000) == 0)
#define SetTransparent(T) ((T) ? 0 : 0xFF000000)


#define MAX_READ_BUF 16384

#if defined(__GNUC__)
extern "C" {
	BOOL WINAPI DllMain(HANDLE hModule, DWORD  ul_reason_for_call, LPVOID lpReserved);
};
#endif


HMODULE ghOurModule = NULL; // ConEmuDw.dll
HWND    ghConWnd = NULL; // инициализируется в CheckBuffers()

AnnotationHeader* gpTrueColor = NULL;
HANDLE ghTrueColor = NULL;
BOOL CheckBuffers();
void CloseBuffers();


BOOL WINAPI DllMain(HANDLE hModule, DWORD  ul_reason_for_call, LPVOID lpReserved)
{
	switch(ul_reason_for_call)
	{
		case DLL_PROCESS_ATTACH:
			{
				//HeapInitialize();
				
				ghOurModule = (HMODULE)hModule;
				
			}
			break;
		
		case DLL_THREAD_ATTACH:
			break;
		case DLL_THREAD_DETACH:
			break;
		
		case DLL_PROCESS_DETACH:
			{
				CloseBuffers();
			}
			break;
	}

	return TRUE;
}

#if defined(CRTSTARTUP)
extern "C" {
	BOOL WINAPI _DllMainCRTStartup(HANDLE hDll,DWORD dwReason,LPVOID lpReserved);
};

BOOL WINAPI _DllMainCRTStartup(HANDLE hDll,DWORD dwReason,LPVOID lpReserved)
{
	DllMain(hDll, dwReason, lpReserved);
	return TRUE;
}
#endif




BOOL CheckBuffers()
{
	//TODO: Проверить, не изменился ли HWND консоли?
	HWND hCon = GetConsoleWindow();
	if (!hCon)
	{
		CloseBuffers();
		SetLastError(E_HANDLE);
		return FALSE;
	}
	if (hCon != ghConWnd)
	{
		ghConWnd = hCon;
		CloseBuffers();
		
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
	}
	if (gpTrueColor && !gpTrueColor->locked)
	{
		//TODO: Сбросить флаги валидности ячеек?
		gpTrueColor->locked = TRUE;
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

//__inline int Max(int i1, int i2)
//{
//	return (i1 > i2) ? i1 : i2;
//}
//
//void Color2FgIndex(COLORREF Color, WORD& Con)
//{
//	int Index;
//	static int LastColor, LastIndex;
//	if (LastColor == Color)
//	{
//		Index = LastIndex;
//	}
//	else
//	{
//		int B = (Color & 0xFF0000) >> 16;
//		int G = (Color & 0xFF00) >> 8;
//		int R = (Color & 0xFF);
//		int nMax = Max(B,Max(R,G));
//		
//		Index =
//			(((B+32) > nMax) ? 1 : 0) |
//			(((G+32) > nMax) ? 2 : 0) |
//			(((R+32) > nMax) ? 4 : 0);
//
//		if (Index == 7)
//		{
//			if (nMax < 32)
//				Index = 0;
//			else if (nMax < 160)
//				Index = 8;
//			else if (nMax > 200)
//				Index = 15;
//		}
//		else if (nMax > 220)
//		{
//			Index |= 8;
//		}
//
//		LastColor = Color;
//		LastIndex = Index;
//	}
//
//	Con |= Index;
//}
//
//void Color2BgIndex(COLORREF Color, BOOL Equal, WORD& Con)
//{
//	int Index;
//	static int LastColor, LastIndex;
//	if (LastColor == Color)
//	{
//		Index = LastIndex;
//	}
//	else
//	{
//		int B = (Color & 0xFF0000) >> 16;
//		int G = (Color & 0xFF00) >> 8;
//		int R = (Color & 0xFF);
//		int nMax = Max(B,Max(R,G));
//
//		Index =
//			(((B+32) > nMax) ? 1 : 0) |
//			(((G+32) > nMax) ? 2 : 0) |
//			(((R+32) > nMax) ? 4 : 0);
//
//		if (Index == 7)
//		{
//			if (nMax < 32)
//				Index = 0;
//			else if (nMax < 160)
//				Index = 8;
//			else if (nMax > 200)
//				Index = 15;
//		}
//		else if (nMax > 220)
//		{
//			Index |= 8;
//		}
//
//		LastColor = Color;
//		LastIndex = Index;
//	}
//
//	if (Index == Con)
//	{
//		if (Con & 8)
//			Index ^= 8;
//		else
//			Con |= 8;
//	}
//	
//	Con |= (Index<<4);
//}

//TODO: Юзкейс понят неправильно.
//TODO: Это не "всего видимого экрана", это "цвет по умолчанию". То, что соответствует 
//TODO: "Screen Text" и "Screen Background" в свойствах консоли. То, что задаётся командой
//TODO: color в cmd.exe. То, что будет использовано если в консоль просто писать 
//TODO: по printf/std::cout/WriteConsole, без явного указания цвета.
BOOL WINAPI GetTextAttributes(FarColor* Attributes)
{
	BOOL lbTrueColor = CheckBuffers();
	
	CONSOLE_SCREEN_BUFFER_INFO csbi = {};
	HANDLE h = GetStdHandle(STD_OUTPUT_HANDLE);
	if (!GetConsoleScreenBufferInfo(h, &csbi))
		return FALSE;

	// Что-то в некоторых случаях сбивается цвет вывода для printf
	_ASSERTE("GetTextAttributes" && ((csbi.wAttributes & 0xFF) == 0x07));
		
	//TODO: Вообще-то нужно бы возвращать то, что задано в SetTextAttributes?
	Attributes->Flags = FCF_FG_4BIT|FCF_BG_4BIT;
	Attributes->ForegroundColor = (csbi.wAttributes & 0xF);
	Attributes->BackgroundColor = (csbi.wAttributes & 0xF0) >> 4;
	
	return TRUE;
	
	//WORD nDefReadBuf[1024];
	//WORD *pnReadAttr = NULL;
	//int nBufWidth  = csbi.srWindow.Right - csbi.srWindow.Left + 1;
	//int nBufHeight = csbi.srWindow.Bottom - csbi.srWindow.Top + 1;
	//int nBufCount  = nBufWidth*nBufHeight;
	//if (nBufWidth <= ARRAYSIZE(nDefReadBuf))
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

//TODO: Юзкейс понят неправильно.
//TODO: Это не "всего видимого экрана", это "цвет по умолчанию". То, что соответствует 
//TODO: "Screen Text" и "Screen Background" в свойствах консоли. То, что задаётся командой
//TODO: color в cmd.exe. То, что будет использовано если в консоль просто писать 
//TODO: по printf/std::cout/WriteConsole, без явного указания цвета.
BOOL WINAPI SetTextAttributes(const FarColor* Attributes)
{
	BOOL lbTrueColor = CheckBuffers();

	CONSOLE_SCREEN_BUFFER_INFO csbi = {};
	HANDLE h = GetStdHandle(STD_OUTPUT_HANDLE);
	if (!GetConsoleScreenBufferInfo(h, &csbi))
		return FALSE;
	
	WORD nDefWriteBuf[1024];
	WORD *pnWriteAttr = NULL;
	int nBufWidth  = csbi.srWindow.Right - csbi.srWindow.Left + 1;
	int nBufHeight = csbi.srWindow.Bottom - csbi.srWindow.Top + 1;
	int nBufCount  = nBufWidth*nBufHeight;
	if (nBufWidth <= ARRAYSIZE(nDefWriteBuf))
		pnWriteAttr = nDefWriteBuf;
	else
		pnWriteAttr = (WORD*)calloc(nBufCount,sizeof(*pnWriteAttr));
	if (!pnWriteAttr)
	{
		SetLastError(E_OUTOFMEMORY);
		return FALSE;
	}
	
	BOOL lbRc = TRUE;
	COORD cr = {csbi.srWindow.Left, csbi.srWindow.Top};
	//DWORD nWritten;
	
	AnnotationInfo* pTrueColor = (AnnotationInfo*)(gpTrueColor ? (((LPBYTE)gpTrueColor) + gpTrueColor->struct_size) : NULL);
	AnnotationInfo* pTrueColorEnd = pTrueColor ? (pTrueColor + gpTrueColor->bufferSize) : NULL;

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


	//for (;cr.Y <= csbi.srWindow.Bottom;cr.Y++)
	//{
	//	WORD* pn = pnWriteAttr;
	//	for (int X = 0; X < nBufWidth; X++, pn++)
	//	{
	//		if (pTrueColor && pTrueColor >= pTrueColorEnd)
	//		{
	//			_ASSERTE(pTrueColor && pTrueColor < pTrueColorEnd);
	//			pTrueColor = NULL; // Выделенный буфер оказался недостаточным
	//		}

	//		// цвет в RealConsole
	//		*pn = n;

	//		// цвет в ConEmu (GUI)
	//		if (pTrueColor)
	//		{
	//			*(pTrueColor++) = t;
	//		}
	//	}
	//
	//	if (!WriteConsoleOutputAttribute(h, pnWriteAttr, nBufWidth, cr, &nWritten) || (nWritten != nBufWidth))
	//		lbRc = FALSE;
	//}
	
	SetConsoleTextAttribute(h, n);
	
	if (pnWriteAttr != nDefWriteBuf)
		free(pnWriteAttr);
	
	return lbRc;
}

BOOL WINAPI ClearExtraRegions(const FarColor* Color)
{
	//TODO: Пока работаем через старый Annotation buffer (переделать нужно)
	//TODO: который по определению соответствует видимой области экрана
	return SetTextAttributes(Color);
}

BOOL WINAPI ReadOutput(FAR_CHAR_INFO* Buffer, COORD BufferSize, COORD BufferCoord, SMALL_RECT* ReadRegion)
{
	BOOL lbTrueColor = CheckBuffers();
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
	BOOL WINAPI ReadConsoleOutput(
	  __in     HANDLE hConsoleOutput,
	  __out    PCHAR_INFO lpBuffer,
	  __in     COORD dwBufferSize,
	  __in     COORD dwBufferCoord,
	  __inout  PSMALL_RECT lpReadRegion
	);
	*/

	CONSOLE_SCREEN_BUFFER_INFO csbi = {};
	HANDLE h = GetStdHandle(STD_OUTPUT_HANDLE);
	if (!GetConsoleScreenBufferInfo(h, &csbi))
		return FALSE;

	CHAR_INFO cDefReadBuf[1024];
	CHAR_INFO *pcReadBuf = NULL;
	int nWindowWidth  = csbi.srWindow.Right - csbi.srWindow.Left + 1;
	if (BufferSize.X <= ARRAYSIZE(cDefReadBuf))
		pcReadBuf = cDefReadBuf;
	else
		pcReadBuf = (CHAR_INFO*)calloc(BufferSize.X,sizeof(*pcReadBuf));
	if (!pcReadBuf)
	{
		SetLastError(E_OUTOFMEMORY);
		return FALSE;
	}

	BOOL lbRc = TRUE;

	AnnotationInfo* pTrueColorStart = (AnnotationInfo*)(gpTrueColor ? (((LPBYTE)gpTrueColor) + gpTrueColor->struct_size) : NULL);
	AnnotationInfo* pTrueColorEnd = pTrueColorStart ? (pTrueColorStart + gpTrueColor->bufferSize) : NULL;
	AnnotationInfo* pTrueColorLine = (AnnotationInfo*)(pTrueColorStart ? (pTrueColorStart + nWindowWidth * (ReadRegion->Top /*- csbi.srWindow.Top*/)) : NULL);

	FAR_CHAR_INFO* pFarEnd = Buffer + BufferSize.X*BufferSize.Y; //-V104

	SMALL_RECT rcRead = *ReadRegion;
	COORD MyBufferSize = {BufferSize.X, 1};
	COORD MyBufferCoord = {BufferCoord.X, 0};
	SHORT YShift = csbi.dwSize.Y - (csbi.srWindow.Bottom - csbi.srWindow.Top + 1);
	SHORT Y1 = ReadRegion->Top + YShift;
	SHORT Y2 = ReadRegion->Bottom + YShift;
	SHORT BufferShift = ReadRegion->Top + YShift;
	for (rcRead.Top = Y1; rcRead.Top <= Y2; rcRead.Top++)
	{
		rcRead.Bottom = rcRead.Top;
		BOOL lbRead = ReadConsoleOutputW(h, pcReadBuf, MyBufferSize, MyBufferCoord, &rcRead);

		CHAR_INFO* pc = pcReadBuf + BufferCoord.X;
		FAR_CHAR_INFO* pFar = Buffer + (rcRead.Top - BufferShift + BufferCoord.Y)*BufferSize.X + BufferCoord.X; //-V104
		AnnotationInfo* pTrueColor = (pTrueColorLine && (pTrueColorLine >= pTrueColorStart)) ? (pTrueColorLine + ReadRegion->Left) : NULL;

		for (int X = rcRead.Left; X <= rcRead.Right; X++, pc++, pFar++)
		{
			if (pFar >= pFarEnd)
			{
				_ASSERTE(pFar < pFarEnd);
				break;
			}

			FAR_CHAR_INFO chr = {lbRead ? pc->Char.UnicodeChar : L' '};

			if (pTrueColor && pTrueColor >= pTrueColorEnd)
			{
				_ASSERTE(pTrueColor && pTrueColor < pTrueColorEnd);
				pTrueColor = NULL; // Выделенный буфер оказался недостаточным
			}

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
				chr.Attributes.ForegroundColor = lbRead ? (pc->Attributes & 0xF) : 7;
			}

			if (pTrueColor && pTrueColor->bk_valid)
			{
				chr.Attributes.BackgroundColor = pTrueColor->bk_color;
			}
			else
			{
				chr.Attributes.Flags |= FCF_BG_4BIT;
				chr.Attributes.BackgroundColor = lbRead ? ((pc->Attributes & 0xF0)>>4) : 0;
			}

			*pFar = chr;

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

BOOL WINAPI WriteOutput(const FAR_CHAR_INFO* Buffer, COORD BufferSize, COORD BufferCoord, SMALL_RECT* WriteRegion)
{
	BOOL lbTrueColor = CheckBuffers();
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
	HANDLE h = GetStdHandle(STD_OUTPUT_HANDLE);
	if (!GetConsoleScreenBufferInfo(h, &csbi))
		return FALSE;

	CHAR_INFO cDefWriteBuf[1024];
	CHAR_INFO *pcWriteBuf = NULL;
	int nWindowWidth  = csbi.srWindow.Right - csbi.srWindow.Left + 1;
	if (BufferSize.X <= ARRAYSIZE(cDefWriteBuf))
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
	AnnotationInfo* pTrueColorLine = (AnnotationInfo*)(pTrueColorStart ? (pTrueColorStart + nWindowWidth * (WriteRegion->Top /*- csbi.srWindow.Top*/)) : NULL); //-V104

	SMALL_RECT rcWrite = *WriteRegion;
	COORD MyBufferSize = {BufferSize.X, 1};
	COORD MyBufferCoord = {BufferCoord.X, 0};
	SHORT YShift = csbi.dwSize.Y - (csbi.srWindow.Bottom - csbi.srWindow.Top + 1);
	SHORT Y1 = WriteRegion->Top + YShift;
	SHORT Y2 = WriteRegion->Bottom + YShift;
	SHORT BufferShift = WriteRegion->Top + YShift;
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
				_ASSERTE(pTrueColor && pTrueColor < pTrueColorEnd);
				pTrueColor = NULL; // Выделенный буфер оказался недостаточным
			}
			
			WORD n = 0, f = 0;
			unsigned __int64 Flags = pFar->Attributes.Flags;
			
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
			if (Flags & FCF_FG_4BIT)
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
				if (n == bk)
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
	return TRUE; //TODO: А чего возвращать-то?
}

// Изначально - здесь будет консольная палитра,
// но юзеру дать возможность ее менять, для удобства
// назначения одинаковых расширенных цветов
// в разных местах (в одном сеансе)
COLORREF gcrCustomColors[16] = {
	0x00000000, 0x00800000, 0x00008000, 0x00808000, 0x00000080, 0x00800080, 0x00008080, 0x00c0c0c0,
	0x00808080, 0x00ff0000, 0x0000ff00, 0x00ffff00, 0x000000ff, 0x00ff00ff, 0x0000ffff, 0x00ffffff
	};

struct ColorParam
{
	FarColor Color;
	BOOL bTrueColorEnabled;
	BOOL b4bitfore;
	BOOL b4bitback;
	BOOL bCentered;
	BOOL bAddTransparent;
	//COLORREF crCustom[16];
	HWND hConsole, hGUI, hGUIRoot;
	RECT rcParent;
	SMALL_RECT rcBuffer; // видимый буфер в консоли
	HWND hDialog;

	BOOL bBold, bItalic, bUnderline;
	BOOL bBackTransparent, bForeTransparent;
	COLORREF crBackColor, crForeColor;
	HBRUSH hbrBackground;
	
	LOGFONT lf;
	HFONT hFont;
	
	void CreateBrush()
	{
		if (hbrBackground)
			DeleteObject(hbrBackground);
		//hbrBackground = CreateSolidBrush(bBackTransparent ? GetSysColor(COLOR_BTNFACE) : crBackColor);
		hbrBackground = CreateSolidBrush(crBackColor);
	}
	
	void RecreateFont(HWND hStatic)
	{
		if (hFont)
			DeleteObject(hFont);
		lf.lfWeight = bBold ? FW_BOLD : FW_NORMAL;
		lf.lfItalic = bItalic;
		lf.lfUnderline = bUnderline;
		hFont = ::CreateFontIndirect(&lf);
		SendMessage(hStatic, WM_SETFONT, (WPARAM)hFont, TRUE);
	}

	void Far2Ref(const FarColor* p, BOOL Foreground, COLORREF* cr, BOOL* Transparent)
	{
		*cr = 0; *Transparent = FALSE;
		if (Foreground)
		{
			*Transparent = IsTransparent(p->ForegroundColor);
			UINT nClr = (p->ForegroundColor & 0x00FFFFFF);
			if (p->Flags & FCF_FG_4BIT)
			{
				if (nClr < 16)
				{
					*cr = Far3Color::GetStdColor(nClr);
				}
			}
			else
			{
				*cr = nClr;
			}
			bBold = (p->Flags & FCF_FG_BOLD) == FCF_FG_BOLD;
			bItalic = (p->Flags & FCF_FG_ITALIC) == FCF_FG_ITALIC;
			bUnderline = (p->Flags & FCF_FG_UNDERLINE) == FCF_FG_UNDERLINE;
		}
		else // Background
		{
			*Transparent = IsTransparent(p->BackgroundColor);
			UINT nClr = (p->BackgroundColor & 0x00FFFFFF);
			if (p->Flags & FCF_BG_4BIT)
			{
				if (nClr < 16)
				{
					*cr = Far3Color::GetStdColor(nClr);
				}
			}
			else
			{
				*cr = nClr;
			}
		}
	};
	void Ref2Far(BOOL Transparent, COLORREF cr, BOOL Foreground, FarColor* p)
	{
		int Color = (cr & 0x00FFFFFF);
		
		if (Foreground ? b4bitfore : b4bitback)
		{
			//int Change = -1;
			//for (int i = 0; i < ARRAYSIZE(crCustom); i++)
			//{
			//	if (crCustom[i] == Color)
			//	{
			//		Change = i;
			//		break;
			//	}
			//}
			//if (Change == -1)
			{
				WORD nIndex = 0;
				// Используем "Fg" и для Background, т.к. Color2BgIndex делает
				// дополнительную корректцию, чтобы "текст был читаем", а это
				// здесь не нужно
				Far3Color::Color2FgIndex(cr, nIndex);
				Color = nIndex;
			}
			//else
			//{
			//	Color = Change;
			//}
		}
		
		if (Foreground)
		{
			if (Foreground ? b4bitfore : b4bitback)
				p->Flags |= FCF_FG_4BIT;
			else
				p->Flags &= ~FCF_FG_4BIT; //TODO: Остальные флаги?
			p->ForegroundColor = Color | SetTransparent(Transparent);

			if (bBold)
				p->Flags |= FCF_FG_BOLD;
			else
				p->Flags &= ~FCF_FG_BOLD;
			if (bItalic)
				p->Flags |= FCF_FG_ITALIC;
			else
				p->Flags &= ~FCF_FG_ITALIC;
			if (bUnderline)
				p->Flags |= FCF_FG_UNDERLINE;
			else
				p->Flags &= ~FCF_FG_UNDERLINE;
		}
		else // Background
		{
			if (Foreground ? b4bitfore : b4bitback)
				p->Flags |= FCF_BG_4BIT;
			else
				p->Flags &= ~FCF_BG_4BIT; //TODO: Остальные флаги?
			p->BackgroundColor = Color | SetTransparent(Transparent);
		}
	};
};



INT_PTR CALLBACK ColorDialogProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	ColorParam* P = NULL;

	if (uMsg == WM_INITDIALOG)
	{
		P = (ColorParam*)lParam;
		SetWindowLongPtr(hwndDlg, GWLP_USERDATA, lParam); //-V107
		// 4bit
		CheckDlgButton(hwndDlg, IDC_FORE_4BIT, P->b4bitfore ? BST_CHECKED : BST_UNCHECKED);
		CheckDlgButton(hwndDlg, IDC_BACK_4BIT, P->b4bitback ? BST_CHECKED : BST_UNCHECKED);
		// Transparent
		CheckDlgButton(hwndDlg, IDC_FORE_TRANS, (P->bForeTransparent && P->bAddTransparent) ? BST_CHECKED : BST_UNCHECKED);
		CheckDlgButton(hwndDlg, IDC_BACK_TRANS, (P->bBackTransparent && P->bAddTransparent) ? BST_CHECKED : BST_UNCHECKED);
		// Bold/Italic/Underline
		CheckDlgButton(hwndDlg, IDC_BOLD, P->bBold ? BST_CHECKED : BST_UNCHECKED);
		CheckDlgButton(hwndDlg, IDC_ITALIC, P->bItalic ? BST_CHECKED : BST_UNCHECKED);
		CheckDlgButton(hwndDlg, IDC_UNDERLINE, P->bUnderline ? BST_CHECKED : BST_UNCHECKED);
		// Заполнить поле буковками
		SetDlgItemText(hwndDlg, IDC_TEXT, L"Text Text Text Text Text Text Text Text Text Text Text Text Text Text Text Text Text Text Text Text Text Text Text Text Text Text Text Text Text Text Text Text Text Text Text Text Text Text Text Text Text Text Text Text Text Text Text Text Text Text Text Text Text Text Text Text Text Text Text Text");
		
		NONCLIENTMETRICS ncm = {sizeof(ncm)};
		SystemParametersInfo(SPI_GETNONCLIENTMETRICS, (UINT)sizeof(NONCLIENTMETRICS), &ncm, 0);
		//P->lf = ncm.lfMessageFont;
		P->lf.lfHeight = ncm.lfMessageFont.lfHeight; // (ncm.lfMessageFont.lfHeight<0) ? (ncm.lfMessageFont.lfHeight-1) : (ncm.lfMessageFont.lfHeight+1);
		P->lf.lfWeight = FW_NORMAL;
		P->lf.lfCharSet = 1;
		lstrcpy(P->lf.lfFaceName, L"Lucida Console");
		P->RecreateFont(GetDlgItem(hwndDlg, IDC_TEXT));

		if (!P->bAddTransparent)
		{
			EnableWindow(GetDlgItem(hwndDlg, IDC_FORE_TRANS), FALSE);
			EnableWindow(GetDlgItem(hwndDlg, IDC_BACK_TRANS), FALSE);
			//ShowWindow(GetDlgItem(hwndDlg, IDC_FORE_TRANS), SW_HIDE);
			//ShowWindow(GetDlgItem(hwndDlg, IDC_BACK_TRANS), SW_HIDE);
			//RECT rcText, rcCheck;
			//GetWindowRect(GetDlgItem(hwndDlg, IDC_TEXT), &rcText);
			//GetWindowRect(GetDlgItem(hwndDlg, IDC_FORE_TRANS), &rcCheck);
			//SetWindowPos(GetDlgItem(hwndDlg, IDC_TEXT), 0, 0,0,
			//	rcText.right-rcText.left, rcCheck.bottom-rcText.top-10, SWP_NOMOVE|SWP_NOZORDER);
		}
		

		SetFocus(GetDlgItem(hwndDlg, IDOK));
		
		RECT rcDlg; GetWindowRect(hwndDlg, &rcDlg);
		//TODO: Пока тестируем с консолью...
		//TODO: Обработка P->Center
		HWND hTop = HWND_TOP;
		if (GetWindowLong(P->hGUIRoot ? P->hGUIRoot : P->hConsole, GWL_EXSTYLE) & WS_EX_TOPMOST)
			hTop = HWND_TOPMOST;
		if (P->bCentered)
		{
			SetWindowPos(hwndDlg, hTop,
				(P->rcParent.right+P->rcParent.left-(rcDlg.right-rcDlg.left))>>1,
				(P->rcParent.bottom+P->rcParent.top-(rcDlg.bottom-rcDlg.top))>>1,
				0, 0, SWP_NOSIZE|SWP_SHOWWINDOW);
		}
		else
		{
			// Поместить в координаты {X=37,Y=2} (0based)
			int x = max(0,(38 - P->rcBuffer.Left));
			int xShift = (P->rcParent.right - P->rcParent.left + 1) / (P->rcBuffer.Right - P->rcBuffer.Left + 1) * x;
			if ((xShift + (rcDlg.right-rcDlg.left)) > P->rcParent.right)
				xShift = max(P->rcParent.left, (P->rcParent.right - (rcDlg.right-rcDlg.left)));
			int y = max(0,(2 - P->rcBuffer.Top));
			int yShift = (P->rcParent.bottom - P->rcParent.top + 1) / (P->rcBuffer.Bottom - P->rcBuffer.Top + 1) * y;
			if ((yShift + (rcDlg.bottom-rcDlg.top)) > P->rcParent.bottom)
				yShift = max(P->rcParent.top, (P->rcParent.bottom - (rcDlg.bottom-rcDlg.top)));
			//yShift += 32; //TODO: Табы, пока так
			SetWindowPos(hwndDlg, hTop,
				P->rcParent.left+xShift, P->rcParent.top+yShift,
				0, 0, SWP_NOSIZE|SWP_SHOWWINDOW);
		}
		P->hDialog = hwndDlg;
		return FALSE;
	}

	P = (ColorParam*)GetWindowLongPtr(hwndDlg, GWLP_USERDATA);
	if (!P)
	{
		// WM_SETFONT, например, может придти перед WM_INITDIALOG O_O
		return FALSE;
	}

	switch (uMsg)
	{
	case WM_CTLCOLORSTATIC:
		if (GetDlgCtrlID((HWND)lParam) == IDC_TEXT)
		{
			//SetTextColor((HDC)wParam, P->bForeTransparent ? GetSysColor(COLOR_BTNFACE) : P->crForeColor);
			SetTextColor((HDC)wParam, P->crForeColor);
			SetBkMode((HDC)wParam, TRANSPARENT);
			return (INT_PTR)P->hbrBackground;
		}
		break;
	case WM_COMMAND:
		switch (wParam)
		{
		case IDOK:
			EndDialog(hwndDlg, 1);
			return TRUE;
		case IDCANCEL:
			EndDialog(hwndDlg, 2);
			return TRUE;
		case IDC_FORE:
		case IDC_BACK:
			{
				CHOOSECOLOR clr = {sizeof(clr),
					hwndDlg, NULL,
					(wParam == IDC_FORE) ? P->crForeColor : P->crBackColor,
					gcrCustomColors,
					((P->bTrueColorEnabled || !(wParam==IDC_FORE?P->b4bitfore:P->b4bitback)) ? (CC_FULLOPEN|CC_ANYCOLOR) : CC_SOLIDCOLOR)
					|CC_RGBINIT,
				};
				if (ChooseColor(&clr))
				{
					if (wParam == IDC_FORE)
					{
						//P->bForeTransparent = FALSE; -- надо/нет?
						//CheckDlgButton(hwndDlg, IDC_FORE_TRANS, BST_UNCHECKED);
						if (P->crForeColor != clr.rgbResult && P->bTrueColorEnabled)
						{
							P->b4bitfore = FALSE;
							CheckDlgButton(hwndDlg, IDC_FORE_4BIT, BST_UNCHECKED);
						}
						P->crForeColor = clr.rgbResult;
					}
					else
					{
						//P->bBackTransparent = FALSE; -- надо/нет?
						//CheckDlgButton(hwndDlg, IDC_BACK_TRANS, BST_UNCHECKED);
						if (P->crBackColor != clr.rgbResult)
						{
							if (P->bTrueColorEnabled)
							{
								P->b4bitback = FALSE;
								CheckDlgButton(hwndDlg, IDC_BACK_4BIT, BST_UNCHECKED);
							}
							P->crBackColor = clr.rgbResult;
							P->CreateBrush();
						}
					}
					InvalidateRect(GetDlgItem(hwndDlg, IDC_TEXT), NULL, TRUE);
				}
			}
			break;
		case IDC_FORE_4BIT:
			{
				P->b4bitfore = IsDlgButtonChecked(hwndDlg, (int)wParam)!=BST_UNCHECKED;
				if (P->b4bitfore)
				{
					WORD nIndex = 0;
					Far3Color::Color2FgIndex(P->crForeColor, nIndex);
					if (/*nIndex >= 0 &&*/ nIndex < 16)
					{
						P->crForeColor = Far3Color::GetStdColor(nIndex);
						InvalidateRect(GetDlgItem(hwndDlg, IDC_TEXT), NULL, TRUE);
					}
					else
					{
						_ASSERTE(/*nIndex >= 0 &&*/ nIndex < 16);
					}
				}
			}
			break;
		case IDC_BACK_4BIT:
			{
				P->b4bitback = IsDlgButtonChecked(hwndDlg, (int)wParam)!=BST_UNCHECKED;
				if (P->b4bitback)
				{
					WORD nIndex = 0;
					// Используем Fg, т.к. он дает "чистый" цвет, без коррекции на "читаемость"
					Far3Color::Color2FgIndex(P->crBackColor, nIndex);
					if (/*nIndex >= 0 &&*/ nIndex < 16)
					{
						P->crBackColor = Far3Color::GetStdColor(nIndex);
						P->CreateBrush();
						InvalidateRect(GetDlgItem(hwndDlg, IDC_TEXT), NULL, TRUE);
					}
					else
					{
						_ASSERTE(/*nIndex >= 0 &&*/ nIndex < 16);
					}
				}
			}
			break;
		case IDC_FORE_TRANS:
			{
				P->bForeTransparent = IsDlgButtonChecked(hwndDlg, (int)wParam)!=BST_UNCHECKED;
				InvalidateRect(GetDlgItem(hwndDlg, IDC_TEXT), NULL, TRUE);
			}
			break;
		case IDC_BACK_TRANS:
			{
				P->bBackTransparent = IsDlgButtonChecked(hwndDlg, (int)wParam)!=BST_UNCHECKED;
				P->CreateBrush();
				InvalidateRect(GetDlgItem(hwndDlg, IDC_TEXT), NULL, TRUE);
			}
			break;
		case IDC_BOLD:
			{
				P->bBold = IsDlgButtonChecked(hwndDlg, (int)wParam)!=BST_UNCHECKED;
				P->RecreateFont(GetDlgItem(hwndDlg, IDC_TEXT));
			}
			break;
		case IDC_ITALIC:
			{
				P->bItalic = IsDlgButtonChecked(hwndDlg, (int)wParam)!=BST_UNCHECKED;
				P->RecreateFont(GetDlgItem(hwndDlg, IDC_TEXT));
			}
			break;
		case IDC_UNDERLINE:
			{
				P->bUnderline = IsDlgButtonChecked(hwndDlg, (int)wParam)!=BST_UNCHECKED;
				P->RecreateFont(GetDlgItem(hwndDlg, IDC_TEXT));
			}
			break;
		}
		break;
	}
	return FALSE;
}

DWORD WINAPI ColorDialogThread(LPVOID lpParameter)
{
	ColorParam *P = (ColorParam*)lpParameter;
	if (!P)
	{
		_ASSERTE(P!=NULL);
		return 100;
	}
	P->CreateBrush();

	// NULL в качестве ParentWindow потому что планируется использование в GUI,
	// а GUI предполагает наличение более одной вкладки (консоли), таким образом,
	// создание диалога от главного окна GUI заблокирует все остальные вкладки, что плохо.
	LRESULT lRc = DialogBoxParam(ghOurModule, MAKEINTRESOURCE(IDD_COLORS), NULL, ColorDialogProc, (LPARAM)P);

	if (P->hbrBackground)
		DeleteObject(P->hbrBackground);
	if (P->hFont)
		DeleteObject(P->hFont);

	return (DWORD)lRc;
}

void CopyShaded(FAR_CHAR_INFO* Src, FAR_CHAR_INFO* Dst)
{
	*Dst = *Src;
	WORD Con = 0;
	if (Src->Attributes.Flags & FCF_FG_4BIT)
		Con = (WORD)(Dst->Attributes.ForegroundColor & 0xFF);
	else
		Far3Color::Color2FgIndex(Dst->Attributes.ForegroundColor, Con);
	Dst->Attributes.Flags |= FCF_BG_4BIT|FCF_FG_4BIT;
	Dst->Attributes.BackgroundColor = 0;
	Dst->Attributes.ForegroundColor = Con ? (Con & 7) : 8;
}

int  WINAPI GetColorDialog(FarColor* Color, BOOL Centered, BOOL AddTransparent)
{
	//TODO: Показать диалог (свой) в котором должно быть поле с текстом,
	//TODO: отрисованное активными цветами (фона/текста)
	//TODO: + два (опциональных) флажка "Transparent" (и для фона и для текста)
	//TODO: ChooseColor дергать по кнопкам "&Background color" "&Text color"

	ColorParam Parm = {*Color, 
		FALSE/*bTrueColorEnabled*/,
		(Color->Flags & FCF_FG_4BIT)==FCF_FG_4BIT/*b4bitfore*/,
		(Color->Flags & FCF_BG_4BIT)==FCF_BG_4BIT/*b4bitback*/,
		Centered, AddTransparent,
		//{0x00000000, 0x00800000, 0x00008000, 0x00808000, 0x00000080, 0x00800080, 0x00008080, 0x00c0c0c0,
		//0x00808080, 0x00ff0000, 0x0000ff00, 0x00ffff00, 0x000000ff, 0x00ff00ff, 0x0000ffff, 0x00ffffff},
	};
	////TODO: BUGBUG. При настройке НОВОГО цвета фар передает Color заполненный 0-ми
	//if (!Parm.Color.Flags && !Parm.Color.ForegroundColor && !Parm.Color.BackgroundColor && !Parm.Color.Reserved)
	//{
	//	Parm.Color.Flags = FCF_FG_4BIT|FCF_BG_4BIT;
	//	Parm.Color.ForegroundColor = 7 | SetTransparent(FALSE);
	//	Parm.Color.BackgroundColor = SetTransparent(FALSE);
	//}
	Parm.Far2Ref(&Parm.Color, TRUE, &Parm.crForeColor, &Parm.bForeTransparent);
	Parm.Far2Ref(&Parm.Color, FALSE, &Parm.crBackColor, &Parm.bBackTransparent);
	if (!AddTransparent)
	{
		Parm.bForeTransparent = Parm.bBackTransparent = FALSE;
	}
	
	//TODO: Заменить на дескриптор окна GUI
	Parm.hConsole = GetConsoleWindow();
	
	// Найти HWND GUI
	wchar_t szMapName[128];
	wsprintf(szMapName, CECONMAPNAME, (DWORD)Parm.hConsole); //-V205
	HANDLE hMap = OpenFileMapping(FILE_MAP_READ, FALSE, szMapName);
	if (hMap != NULL)
	{
		CESERVER_CONSOLE_MAPPING_HDR *pHdr = (CESERVER_CONSOLE_MAPPING_HDR*)MapViewOfFile(hMap, FILE_MAP_READ,0,0,0);
		if (pHdr)
		{
#if 1 // затычка для 110721
			if (pHdr->cbSize == 2164 && pHdr->nProtocolVersion == 67 && IsWindow(pHdr->hConEmuWnd))
			{
				Parm.hGUI = pHdr->hConEmuWnd; // DC
				Parm.hGUIRoot = pHdr->hConEmuRoot; // Main window
				if (!CheckBuffers())
				{
					MessageBoxA(NULL, MSG_NO_TRUEMOD_BUFFER, MSG_TITLE, MB_ICONSTOP|MB_SYSTEMMODAL);
				}
				else
				{
					Parm.bTrueColorEnabled = TRUE;
				}
			}
			else
#endif
			if ((pHdr->cbSize < sizeof(*pHdr))
				|| (pHdr->nProtocolVersion != CESERVER_REQ_VER) 
				|| !IsWindow(pHdr->hConEmuWnd))
			{
				if ((pHdr->cbSize >= sizeof(*pHdr)) 
					&& pHdr->hConEmuWnd && IsWindow(pHdr->hConEmuWnd)
					&& IsWindowVisible(pHdr->hConEmuWnd))
				{
					// Это все-таки может быть окно ConEmu
					Parm.hGUI = pHdr->hConEmuWnd; // DC
					if (IsWindow(pHdr->hConEmuRoot) && IsWindowVisible(pHdr->hConEmuRoot))
						Parm.hGUIRoot = pHdr->hConEmuRoot; // Main window
				}
				MessageBoxA(NULL, MSG_INVALID_CONEMU_VER, MSG_TITLE, MB_ICONSTOP|MB_SYSTEMMODAL);
			}
			else
			{
				Parm.hGUI = pHdr->hConEmuWnd; // DC
				Parm.hGUIRoot = pHdr->hConEmuRoot; // Main window
				if (!pHdr->bUseTrueColor)
				{
					MessageBoxA(NULL, MSG_TRUEMOD_DISABLED, MSG_TITLE, MB_ICONSTOP|MB_SYSTEMMODAL);
				}
				else if (!CheckBuffers())
				{
					MessageBoxA(NULL, MSG_NO_TRUEMOD_BUFFER, MSG_TITLE, MB_ICONSTOP|MB_SYSTEMMODAL);
				}
				else
				{
					Parm.bTrueColorEnabled = TRUE;
				}
			}
			UnmapViewOfFile(pHdr);
		}
		CloseHandle(hMap);
	}
	
	if (!Parm.hGUI && !IsWindowVisible(Parm.hConsole))
	{
		SystemParametersInfo(SPI_GETWORKAREA, 0, &Parm.rcParent, 0);
	}
	else
	{
		GetClientRect(Parm.hGUI ? Parm.hGUI : Parm.hConsole, &Parm.rcParent);
		MapWindowPoints(Parm.hGUI ? Parm.hGUI : Parm.hConsole, NULL, (POINT*)&Parm.rcParent, 2);
	}
	
	CONSOLE_SCREEN_BUFFER_INFO csbi = {};
	HANDLE h = GetStdHandle(STD_OUTPUT_HANDLE);
	GetConsoleScreenBufferInfo(h, &csbi);
	Parm.rcBuffer = csbi.srWindow;

	int nRc = 0;

	LPCWSTR pszTitle = L"ConEmu Colors";
	LPCWSTR pszText = L"Executing GUI dialog";
	int nWidth = Far3Color::Max(lstrlen(pszText),lstrlen(pszTitle))+10;
	int nHeight = 6;
	int nX = (csbi.srWindow.Right + csbi.srWindow.Left - nWidth) >> 1;
	int nY = (csbi.srWindow.Bottom + csbi.srWindow.Top - nHeight) >> 1;
	SMALL_RECT Region = {nX, nY, nX+nWidth-1, nY+nHeight-1};
	COORD BufSize = {nWidth,nHeight};
	COORD BufCoord = {0,0};

	FAR_CHAR_INFO* SaveBuffer = (FAR_CHAR_INFO*)calloc(nWidth*nHeight, sizeof(*SaveBuffer)); //-V106
	FAR_CHAR_INFO* WriteBuffer = (FAR_CHAR_INFO*)calloc(nWidth*nHeight, sizeof(*WriteBuffer)); //-V106

	ReadOutput(SaveBuffer, BufSize, BufCoord, &Region);
	FAR_CHAR_INFO* Src = SaveBuffer;
	FAR_CHAR_INFO* Dst = WriteBuffer;
	FAR_CHAR_INFO chr = {L' ', {FCF_FG_4BIT|FCF_BG_4BIT, 0, 7}};
	int x;
	// Первая строка - просто фон диалога
	for (x = 0; x < (nWidth-2); x++, Src++, Dst++)
		*Dst = chr;
	*(Dst++) = *(Src++); *(Dst++) = *(Src++); // содержимое консоли (правой верхний угол)
	// 2-я строка - заголовок диалога
	*(Dst++) = chr; *(Dst++) = chr; Src+=2; // фон перед рамкой
	*(Dst) = chr; Src++; (Dst++)->Char = ucBoxDblDownRight; // угол рамки
	chr.Char = ucBoxDblHorz;
	int Ln = lstrlen(pszTitle);
	int X1 = 4+((nWidth - 12 - Ln)>>1); //-V112
	int X2 = X1 + Ln + 1;
	for (x = 3; x < (nWidth-5); x++, Src++, Dst++)
	{
		*Dst = chr;
		if (x == X1)
			Dst->Char = L' ';
		else if (x == X2)
			Dst->Char = L' ';
		else if (x > X1 && x < X2)
			Dst->Char = *(pszTitle++);
	}
	*(Dst) = chr; Src++; (Dst++)->Char = ucBoxDblDownLeft; // угол рамки
	chr.Char = L' '; *(Dst++) = chr; *(Dst++) = chr; Src+=2; // фон после рамки
	CopyShaded(Src++, Dst++); CopyShaded(Src++, Dst++);
	// 3-я строка - текст
	*(Dst++) = chr; *(Dst++) = chr; Src+=2; // фон перед рамкой
	*(Dst) = chr; Src++; (Dst++)->Char = ucBoxDblVert; // рамка
	chr.Char = L' ';
	Ln = lstrlen(pszText);
	X1 = 4+((nWidth - 12 - Ln)>>1); //-V112
	X2 = X1 + Ln + 1;
	for (x = 3; x < (nWidth-5); x++, Src++, Dst++)
	{
		*Dst = chr;
		if (x == X1)
			Dst->Char = L' ';
		else if (x == X2)
			Dst->Char = L' ';
		else if (x > X1 && x < X2)
			Dst->Char = *(pszText++);
	}
	*(Dst) = chr; Src++; (Dst++)->Char = ucBoxDblVert; // рамка
	chr.Char = L' '; *(Dst++) = chr; *(Dst++) = chr; Src+=2; // фон после рамки
	CopyShaded(Src++, Dst++); CopyShaded(Src++, Dst++);
	// 4-я строка - низ рамки
	*(Dst++) = chr; *(Dst++) = chr; Src+=2; // фон перед рамкой
	*(Dst) = chr; Src++; (Dst++)->Char = ucBoxDblUpRight; // угол рамки
	chr.Char = ucBoxDblHorz;
	for (x = 3; x < (nWidth-5); x++, Src++, Dst++)
		*Dst = chr;
	*(Dst) = chr; Src++; (Dst++)->Char = ucBoxDblUpLeft; // угол рамки
	chr.Char = L' '; *(Dst++) = chr; *(Dst++) = chr; Src+=2; // фон после рамки
	CopyShaded(Src++, Dst++); CopyShaded(Src++, Dst++);
	// 5-я строка - фон внизу диалога
	for (x = 0; x < (nWidth-2); x++, Src++, Dst++)
		*Dst = chr;
	CopyShaded(Src++, Dst++); CopyShaded(Src++, Dst++);
	// 6-я строка - тень под диалогом
	*(Dst++) = *(Src++); *(Dst++) = *(Src++); // содержимое консоли (нижний левый угол)
	for (x = 0; x < (nWidth-2); x++, Src++, Dst++)
		CopyShaded(Src, Dst);

	// Вывалить в консоль "диалог"
	WriteOutput(WriteBuffer, BufSize, BufCoord, &Region);

	// Запускаем нить, чтобы 1) не драться с консолью; 2) не иметь проблем с обработчиком сообщений и дескрипторами GDI
	DWORD dwThreadID = 0;
	HANDLE hThread = CreateThread(NULL, 0, ColorDialogThread, &Parm, 0, &dwThreadID);
	if (!hThread)
	{
		nRc = -1;
	}
	else
	{
		while (WaitForSingleObject(hThread, 100) == WAIT_TIMEOUT)
		{
			if (Parm.hDialog)
			{
				SetForegroundWindow(Parm.hDialog);
				break;
			}
		}

		//TODO: Возможно здесь стоит вставить какой-то обработчик, 
		//TODO: чтобы из консоли можно было остановить GUI диалог
		WaitForSingleObject(hThread, INFINITE);
		
		
		GetExitCodeThread(hThread, (LPDWORD)&nRc);
		
		if (nRc == 1)
		{
			Parm.Ref2Far(Parm.bForeTransparent, Parm.crForeColor, TRUE, Color);
			Parm.Ref2Far(Parm.bBackTransparent, Parm.crBackColor, FALSE, Color);
		}
	}

	WriteOutput(SaveBuffer, BufSize, BufCoord, &Region);

	free(SaveBuffer);
	free(WriteBuffer);

	return (nRc == 1);
}
