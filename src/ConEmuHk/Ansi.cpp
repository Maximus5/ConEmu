
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

#define DEFINE_HOOK_MACROS

#ifdef _DEBUG
//	#define DUMP_WRITECONSOLE_LINES
	#define DUMP_UNKNOWN_ESCAPES
#endif

#include <windows.h>
#include <WinError.h>
#include <WinNT.h>
#include <TCHAR.h>
#include "../common/common.hpp"
#include "../common/ConEmuCheck.h"
#include "SetHook.h"
#include "ConEmuHooks.h"
#include "UserImp.h"
#include "../common/ConsoleAnnotation.h"
#include "ExtConsole.h"

#undef isPressed
#define isPressed(inp) ((user->getKeyState(inp) & 0x8000) == 0x8000)

#define ANSI_MAP_CHECK_TIMEOUT 1000

#ifdef _DEBUG
#define DebugString(x) OutputDebugString(x)
#else
#define DebugString(x) //OutputDebugString(x)
#endif

extern HMODULE ghOurModule; // Хэндл нашей dll'ки (здесь хуки не ставятся)
extern DWORD   gnHookMainThreadId;
extern BOOL    gbHooksTemporaryDisabled;

/* ************ Globals for SetHook ************ */
//HWND ghConEmuWndDC = NULL; // Содержит хэндл окна отрисовки. Это ДОЧЕРНЕЕ окно.
extern HWND    ghConWnd;      // RealConsole
extern HWND    ghConEmuWnd;   // Root! ConEmu window
extern HWND    ghConEmuWndDC; // ConEmu DC window
extern DWORD   gnGuiPID;
extern GetConsoleWindow_T gfGetRealConsoleWindow;
extern HWND WINAPI GetRealConsoleWindow(); // Entry.cpp
extern HANDLE ghCurrentOutBuffer;
extern HANDLE ghStdOutHandle;
extern wchar_t gsInitConTitle[512];
/* ************ Globals for SetHook ************ */


HANDLE ghLastAnsiCapable = NULL;
HANDLE ghLastAnsiNotCapable = NULL;

BOOL WINAPI OnWriteFile(HANDLE hFile, LPCVOID lpBuffer, DWORD nNumberOfBytesToWrite, LPDWORD lpNumberOfBytesWritten, LPOVERLAPPED lpOverlapped);
BOOL WINAPI OnWriteConsoleA(HANDLE hConsoleOutput, const VOID *lpBuffer, DWORD nNumberOfCharsToWrite, LPDWORD lpNumberOfCharsWritten, LPVOID lpReserved);
BOOL WINAPI OnWriteConsoleW(HANDLE hConsoleOutput, const VOID *lpBuffer, DWORD nNumberOfCharsToWrite, LPDWORD lpNumberOfCharsWritten, LPVOID lpReserved);
BOOL WINAPI OnWriteConsoleOutputCharacterA(HANDLE hConsoleOutput, LPCSTR lpCharacter, DWORD nLength, COORD dwWriteCoord, LPDWORD lpNumberOfCharsWritten);
BOOL WINAPI OnWriteConsoleOutputCharacterW(HANDLE hConsoleOutput, LPCWSTR lpCharacter, DWORD nLength, COORD dwWriteCoord, LPDWORD lpNumberOfCharsWritten);

typedef BOOL (WINAPI* OnWriteConsoleW_t)(HANDLE hConsoleOutput, const VOID *lpBuffer, DWORD nNumberOfCharsToWrite, LPDWORD lpNumberOfCharsWritten, LPVOID lpReserved);
BOOL WriteAnsiCodes(OnWriteConsoleW_t _WriteConsoleW, HANDLE hConsoleOutput, LPCWSTR lpBuffer, DWORD nNumberOfCharsToWrite, LPDWORD lpNumberOfCharsWritten);

struct DisplayParm
{
	BOOL WasSet;
	BOOL BrightOrBold;     // 1
	BOOL ItalicOrInverse;  // 3
	BOOL BackOrUnderline;  // 4
	int  TextColor;        // 30-37,38,39
	BOOL Text256;          // 38
    int  BackColor;        // 40-47,48,49
    BOOL Back256;          // 48
} gDisplayParm = {};

struct DisplayCursorPos
{
    // Internal
    COORD StoredCursorPos;
} gDisplayCursor = {};

struct DisplayOpt
{
	BOOL  WrapWasSet;
	SHORT WrapAt; // Rightmost X coord (1-based)
	//
	BOOL  AutoLfNl; // LF/NL (default off): Automatically follow echo of LF, VT or FF with CR.
} gDisplayOpt;

static wchar_t gsPrevAnsiPart[160] = {};
static wchar_t gsPrevAnsiPart2[160] = {};
static INT_PTR gnPrevAnsiPart = 0;
static INT_PTR gnPrevAnsiPart2 = 0;
const  INT_PTR MaxPrevAnsiPart = 80;

#ifdef _DEBUG
static const wchar_t szAnalogues[32] =
{
	32, 9786, 9787, 9829, 9830, 9827, 9824, 8226, 9688, 9675, 9689, 9794, 9792, 9834, 9835, 9788,
	9658, 9668, 8597, 8252,  182,  167, 9632, 8616, 8593, 8595, 8594, 8592, 8735, 8596, 9650, 9660
};
#endif

static SHORT GetDefaultTextAttr()
{
	TODO("Default console colors");
	return 7;
}


void ReSetDisplayParm(HANDLE hConsoleOutput, BOOL bReset, BOOL bApply)
{
	WARNING("Эту фнуку нужно дергать при смене буферов, закрытии дескрипторов, и т.п.");

	if (bReset || !gDisplayParm.WasSet)
	{
		if (bReset)
			memset(&gDisplayParm, 0, sizeof(gDisplayParm));

		WORD nDefColors = GetDefaultTextAttr();
		gDisplayParm.TextColor = CONFORECOLOR(nDefColors);
		gDisplayParm.BackColor = CONBACKCOLOR(nDefColors);
		gDisplayParm.WasSet = TRUE;
	}

	if (bApply)
	{
		// на дисплей
		ExtAttributesParm attr = {sizeof(attr), hConsoleOutput};
		//DWORD wAttrs = 0;

		// Ansi colors
		static DWORD ClrMap[8] = {0,4,2,6,1,5,3,7};
		// XTerm-256 colors
		static DWORD RgbMap[256] = {0,4,2,6,1,5,3,7, 8+0,8+4,8+2,8+6,8+1,8+5,8+3,8+7, // System Ansi colors
			/*16*/0x000000, /*17*/0x5f0000, /*18*/0x870000, /*19*/0xaf0000, /*20*/0xd70000, /*21*/0xff0000, /*22*/0x005f00, /*23*/0x5f5f00, /*24*/0x875f00, /*25*/0xaf5f00, /*26*/0xd75f00, /*27*/0xff5f00,
			/*28*/0x008700, /*29*/0x5f8700, /*30*/0x878700, /*31*/0xaf8700, /*32*/0xd78700, /*33*/0xff8700, /*34*/0x00af00, /*35*/0x5faf00, /*36*/0x87af00, /*37*/0xafaf00, /*38*/0xd7af00, /*39*/0xffaf00,
			/*40*/0x00d700, /*41*/0x5fd700, /*42*/0x87d700, /*43*/0xafd700, /*44*/0xd7d700, /*45*/0xffd700, /*46*/0x00ff00, /*47*/0x5fff00, /*48*/0x87ff00, /*49*/0xafff00, /*50*/0xd7ff00, /*51*/0xffff00,
			/*52*/0x00005f, /*53*/0x5f005f, /*54*/0x87005f, /*55*/0xaf005f, /*56*/0xd7005f, /*57*/0xff005f, /*58*/0x005f5f, /*59*/0x5f5f5f, /*60*/0x875f5f, /*61*/0xaf5f5f, /*62*/0xd75f5f, /*63*/0xff5f5f,
			/*64*/0x00875f, /*65*/0x5f875f, /*66*/0x87875f, /*67*/0xaf875f, /*68*/0xd7875f, /*69*/0xff875f, /*70*/0x00af5f, /*71*/0x5faf5f, /*72*/0x87af5f, /*73*/0xafaf5f, /*74*/0xd7af5f, /*75*/0xffaf5f,
			/*76*/0x00d75f, /*77*/0x5fd75f, /*78*/0x87d75f, /*79*/0xafd75f, /*80*/0xd7d75f, /*81*/0xffd75f, /*82*/0x00ff5f, /*83*/0x5fff5f, /*84*/0x87ff5f, /*85*/0xafff5f, /*86*/0xd7ff5f, /*87*/0xffff5f,
			/*88*/0x000087, /*89*/0x5f0087, /*90*/0x870087, /*91*/0xaf0087, /*92*/0xd70087, /*93*/0xff0087, /*94*/0x005f87, /*95*/0x5f5f87, /*96*/0x875f87, /*97*/0xaf5f87, /*98*/0xd75f87, /*99*/0xff5f87,
			/*100*/0x008787, /*101*/0x5f8787, /*102*/0x878787, /*103*/0xaf8787, /*104*/0xd78787, /*105*/0xff8787, /*106*/0x00af87, /*107*/0x5faf87, /*108*/0x87af87, /*109*/0xafaf87, /*110*/0xd7af87, /*111*/0xffaf87,
			/*112*/0x00d787, /*113*/0x5fd787, /*114*/0x87d787, /*115*/0xafd787, /*116*/0xd7d787, /*117*/0xffd787, /*118*/0x00ff87, /*119*/0x5fff87, /*120*/0x87ff87, /*121*/0xafff87, /*122*/0xd7ff87, /*123*/0xffff87,
			/*124*/0x0000af, /*125*/0x5f00af, /*126*/0x8700af, /*127*/0xaf00af, /*128*/0xd700af, /*129*/0xff00af, /*130*/0x005faf, /*131*/0x5f5faf, /*132*/0x875faf, /*133*/0xaf5faf, /*134*/0xd75faf, /*135*/0xff5faf,
			/*136*/0x0087af, /*137*/0x5f87af, /*138*/0x8787af, /*139*/0xaf87af, /*140*/0xd787af, /*141*/0xff87af, /*142*/0x00afaf, /*143*/0x5fafaf, /*144*/0x87afaf, /*145*/0xafafaf, /*146*/0xd7afaf, /*147*/0xffafaf,
			/*148*/0x00d7af, /*149*/0x5fd7af, /*150*/0x87d7af, /*151*/0xafd7af, /*152*/0xd7d7af, /*153*/0xffd7af, /*154*/0x00ffaf, /*155*/0x5fffaf, /*156*/0x87ffaf, /*157*/0xafffaf, /*158*/0xd7ffaf, /*159*/0xffffaf,
			/*160*/0x0000d7, /*161*/0x5f00d7, /*162*/0x8700d7, /*163*/0xaf00d7, /*164*/0xd700d7, /*165*/0xff00d7, /*166*/0x005fd7, /*167*/0x5f5fd7, /*168*/0x875fd7, /*169*/0xaf5fd7, /*170*/0xd75fd7, /*171*/0xff5fd7,
			/*172*/0x0087d7, /*173*/0x5f87d7, /*174*/0x8787d7, /*175*/0xaf87d7, /*176*/0xd787d7, /*177*/0xff87d7, /*178*/0x00afd7, /*179*/0x5fafd7, /*180*/0x87afd7, /*181*/0xafafd7, /*182*/0xd7afd7, /*183*/0xffafd7,
			/*184*/0x00d7d7, /*185*/0x5fd7d7, /*186*/0x87d7d7, /*187*/0xafd7d7, /*188*/0xd7d7d7, /*189*/0xffd7d7, /*190*/0x00ffd7, /*191*/0x5fffd7, /*192*/0x87ffd7, /*193*/0xafffd7, /*194*/0xd7ffd7, /*195*/0xffffd7,
			/*196*/0x0000ff, /*197*/0x5f00ff, /*198*/0x8700ff, /*199*/0xaf00ff, /*200*/0xd700ff, /*201*/0xff00ff, /*202*/0x005fff, /*203*/0x5f5fff, /*204*/0x875fff, /*205*/0xaf5fff, /*206*/0xd75fff, /*207*/0xff5fff,
			/*208*/0x0087ff, /*209*/0x5f87ff, /*210*/0x8787ff, /*211*/0xaf87ff, /*212*/0xd787ff, /*213*/0xff87ff, /*214*/0x00afff, /*215*/0x5fafff, /*216*/0x87afff, /*217*/0xafafff, /*218*/0xd7afff, /*219*/0xffafff,
			/*220*/0x00d7ff, /*221*/0x5fd7ff, /*222*/0x87d7ff, /*223*/0xafd7ff, /*224*/0xd7d7ff, /*225*/0xffd7ff, /*226*/0x00ffff, /*227*/0x5fffff, /*228*/0x87ffff, /*229*/0xafffff, /*230*/0xd7ffff, /*231*/0xffffff,
			/*232*/0x080808, /*233*/0x121212, /*234*/0x1c1c1c, /*235*/0x262626, /*236*/0x303030, /*237*/0x3a3a3a, /*238*/0x444444, /*239*/0x4e4e4e, /*240*/0x585858, /*241*/0x626262, /*242*/0x6c6c6c, /*243*/0x767676,
			/*244*/0x808080, /*245*/0x8a8a8a, /*246*/0x949494, /*247*/0x9e9e9e, /*248*/0xa8a8a8, /*249*/0xb2b2b2, /*250*/0xbcbcbc, /*251*/0xc6c6c6, /*252*/0xd0d0d0, /*253*/0xdadada, /*254*/0xe4e4e4, /*255*/0xeeeeee
		};

		if (gDisplayParm.Text256)
		{
			if (gDisplayParm.TextColor > 15)
				attr.Attributes.Flags |= CECF_FG_24BIT;
			attr.Attributes.ForegroundColor = RgbMap[gDisplayParm.TextColor&0xFF];

			if (gDisplayParm.BrightOrBold)
				attr.Attributes.Flags |= CECF_FG_BOLD;
			if (gDisplayParm.ItalicOrInverse)
				attr.Attributes.Flags |= CECF_FG_ITALIC;
			if (gDisplayParm.BackOrUnderline)
				attr.Attributes.Flags |= CECF_FG_UNDERLINE;
		}
		else
		{
			attr.Attributes.ForegroundColor |= ClrMap[gDisplayParm.TextColor&0x7]
				| (gDisplayParm.BrightOrBold ? 0x08 : 0);
		}

		if (gDisplayParm.Back256)
		{
			if (gDisplayParm.BackColor > 15)
				attr.Attributes.Flags |= CECF_BG_24BIT;
			attr.Attributes.BackgroundColor = RgbMap[gDisplayParm.BackColor&0xFF];
		}
		else
		{
			attr.Attributes.BackgroundColor |= ClrMap[gDisplayParm.BackColor&0x7]
				| ((gDisplayParm.BackOrUnderline && !gDisplayParm.Text256) ? 0x8 : 0);
		}
		

		//SetConsoleTextAttribute(hConsoleOutput, (WORD)wAttrs);
		ExtSetAttributes(&attr);
	}
}


bool IsOutputHandle(HANDLE hFile, DWORD* pMode /*= NULL*/)
{
	if (!hFile)
		return false;

	if (hFile == ghLastAnsiCapable)
		return true;
	else if (hFile == ghLastAnsiNotCapable)
		return false;

	bool  bOk = false;
	DWORD Mode = 0, nErrCode = 0;
	CONSOLE_SCREEN_BUFFER_INFO csbi = {};

	// GetConsoleMode не совсем подходит, т.к. он проверяет и ConIn & ConOut
	// Поэтому, добавляем	
	if (GetConsoleMode(hFile, &Mode))
	{
		if (!GetConsoleScreenBufferInfo(hFile, &csbi))
		{
			nErrCode = GetLastError();
			_ASSERTE(nErrCode == ERROR_INVALID_HANDLE);
			ghLastAnsiNotCapable = hFile;
		}
		else
		{
			if (pMode)
				*pMode = Mode;
			//Processed = (Mode & ENABLE_PROCESSED_OUTPUT);
			ghLastAnsiCapable = hFile;
			bOk = true;
		}
	}
	else
	{
		ghLastAnsiNotCapable = hFile;
	}

	return bOk;
}

bool IsAnsiCapable(HANDLE hFile, bool* bIsConsoleOutput = NULL)
{
	bool bAnsi = false;
	DWORD Mode = 0;
	bool bIsOut = false;

	if ((hFile == INVALID_HANDLE_VALUE) && (((HANDLE)bIsConsoleOutput) == INVALID_HANDLE_VALUE))
	{
		// Проверка настроек на старте
		bIsOut = true;
		Mode = ENABLE_PROCESSED_OUTPUT;
	}
	else
	{
		bIsOut = IsOutputHandle(hFile, &Mode);
	}

	if (bIsOut)
	{
		bAnsi = ((Mode & ENABLE_PROCESSED_OUTPUT) == ENABLE_PROCESSED_OUTPUT);
		if (!bAnsi)
		{
			#ifdef _DEBUG
			HANDLE hIn  = GetStdHandle(STD_INPUT_HANDLE);
			HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
			HANDLE hErr = GetStdHandle(STD_ERROR_HANDLE);
			#endif
			// Где-то по дороге между "cmd /c test.cmd", "pause" в нем и "WriteConsole"
			// слетает ENABLE_PROCESSED_OUTPUT, поэтому пока обрабатываем всегда.
			bAnsi = true;
		}

		
		if (bAnsi)
		{
			static DWORD nLastCheck = 0;
			static bool bAnsiAllowed = true;

			if (nLastCheck || ((GetTickCount() - nLastCheck) > ANSI_MAP_CHECK_TIMEOUT))
			{
				CESERVER_CONSOLE_MAPPING_HDR* pMap = GetConMap();
				//	(CESERVER_CONSOLE_MAPPING_HDR*)malloc(sizeof(CESERVER_CONSOLE_MAPPING_HDR));
				if (pMap)
				{
					//if (!::LoadSrvMapping(ghConWnd, *pMap) || !pMap->bProcessAnsi)
					//	bAnsiAllowed = false;
					//else
					//	bAnsiAllowed = true;

					bAnsiAllowed = ((pMap != NULL) && (pMap->bProcessAnsi != FALSE));

					//free(pMap);
				}
				nLastCheck = GetTickCount();
			}

			if (!bAnsiAllowed)
				bAnsi = false;
		}
	}

	if (bIsConsoleOutput)
		*bIsConsoleOutput = bIsOut;

	return bAnsi;
}

BOOL WINAPI OnScrollConsoleScreenBufferA(HANDLE hConsoleOutput, const SMALL_RECT *lpScrollRectangle, const SMALL_RECT *lpClipRectangle, COORD dwDestinationOrigin, const CHAR_INFO *lpFill)
{
	typedef BOOL (WINAPI* OnScrollConsoleScreenBufferA_t)(HANDLE hConsoleOutput, const SMALL_RECT *lpScrollRectangle, const SMALL_RECT *lpClipRectangle, COORD dwDestinationOrigin, const CHAR_INFO *lpFill);
	ORIGINALFAST(ScrollConsoleScreenBufferA);
	BOOL bMainThread = FALSE; // поток не важен
	BOOL lbRc = FALSE;

	if (IsOutputHandle(hConsoleOutput))
	{
		WARNING("Проверка аргументов! Скролл может быть частичным");
		ExtScrollScreenParm scrl = {sizeof(scrl), essf_ExtOnly, hConsoleOutput, dwDestinationOrigin.Y - lpScrollRectangle->Top};
		ExtScrollScreen(&scrl);
	}

	lbRc = F(ScrollConsoleScreenBufferA)(hConsoleOutput, lpScrollRectangle, lpClipRectangle, dwDestinationOrigin, lpFill);

	return lbRc;
}

BOOL WINAPI OnScrollConsoleScreenBufferW(HANDLE hConsoleOutput, const SMALL_RECT *lpScrollRectangle, const SMALL_RECT *lpClipRectangle, COORD dwDestinationOrigin, const CHAR_INFO *lpFill)
{
	typedef BOOL (WINAPI* OnScrollConsoleScreenBufferW_t)(HANDLE hConsoleOutput, const SMALL_RECT *lpScrollRectangle, const SMALL_RECT *lpClipRectangle, COORD dwDestinationOrigin, const CHAR_INFO *lpFill);
	ORIGINALFAST(ScrollConsoleScreenBufferW);
	BOOL bMainThread = FALSE; // поток не важен
	BOOL lbRc = FALSE;

	if (IsOutputHandle(hConsoleOutput))
	{
		WARNING("Проверка аргументов! Скролл может быть частичным");
		ExtScrollScreenParm scrl = {sizeof(scrl), essf_ExtOnly, hConsoleOutput, dwDestinationOrigin.Y - lpScrollRectangle->Top};
		ExtScrollScreen(&scrl);
	}

	lbRc = F(ScrollConsoleScreenBufferW)(hConsoleOutput, lpScrollRectangle, lpClipRectangle, dwDestinationOrigin, lpFill);

	return lbRc;
}

BOOL WINAPI OnWriteFile(HANDLE hFile, LPCVOID lpBuffer, DWORD nNumberOfBytesToWrite, LPDWORD lpNumberOfBytesWritten, LPOVERLAPPED lpOverlapped)
{
	typedef BOOL (WINAPI* OnWriteFile_t)(HANDLE hFile, LPCVOID lpBuffer, DWORD nNumberOfBytesToWrite, LPDWORD lpNumberOfBytesWritten, LPOVERLAPPED lpOverlapped);
	ORIGINALFAST(WriteFile);
	BOOL bMainThread = FALSE; // поток не важен
	BOOL lbRc = FALSE;

	if (lpBuffer && nNumberOfBytesToWrite && IsAnsiCapable(hFile))
		lbRc = OnWriteConsoleA(hFile, lpBuffer, nNumberOfBytesToWrite, lpNumberOfBytesWritten, NULL);
	else
		lbRc = F(WriteFile)(hFile, lpBuffer, nNumberOfBytesToWrite, lpNumberOfBytesWritten, lpOverlapped);

	return lbRc;
}

BOOL WINAPI OnWriteConsoleA(HANDLE hConsoleOutput, const VOID *lpBuffer, DWORD nNumberOfCharsToWrite, LPDWORD lpNumberOfCharsWritten, LPVOID lpReserved)
{
	typedef BOOL (WINAPI* OnWriteConsoleA_t)(HANDLE hConsoleOutput, const VOID *lpBuffer, DWORD nNumberOfCharsToWrite, LPDWORD lpNumberOfCharsWritten, LPVOID lpReserved);
	ORIGINALFAST(WriteConsoleA);
	BOOL bMainThread = FALSE; // поток не важен
	BOOL lbRc = FALSE;

	if (lpBuffer && nNumberOfCharsToWrite && IsAnsiCapable(hConsoleOutput))
	{
		wchar_t* buf = NULL;
		DWORD len, cp;
		
		cp = GetConsoleOutputCP();
		len = MultiByteToWideChar(cp, 0, (LPCSTR)lpBuffer, nNumberOfCharsToWrite, 0, 0);
		buf = (wchar_t*)malloc((len+1)*sizeof(wchar_t));
		if (buf == NULL)
		{
			if (lpNumberOfCharsWritten != NULL)
				*lpNumberOfCharsWritten = 0;
		}
		else
		{
			DWORD newLen = MultiByteToWideChar(cp, 0, (LPCSTR)lpBuffer, nNumberOfCharsToWrite, buf, len);
			_ASSERTE(newLen==len);
			buf[newLen] = 0; // ASCII-Z, хотя, если функцию WriteConsoleW зовет приложение - "\0" может и не быть...

			lbRc = OnWriteConsoleW(hConsoleOutput, buf, len, lpNumberOfCharsWritten, NULL);

			free( buf );
		}
	}
	else
	{
		// По идее, сюда попадать не должны. Ошибка в параметрах?
		_ASSERTE(lpBuffer && nNumberOfCharsToWrite && IsAnsiCapable(hConsoleOutput));
		lbRc = F(WriteConsoleA)(hConsoleOutput, lpBuffer, nNumberOfCharsToWrite, lpNumberOfCharsWritten, lpReserved);
	}

	return lbRc;
}

TODO("По хорошему, после WriteConsoleOutputAttributes тоже нужно делать efof_ResetExt");
// Но пока можно это проигнорировать, большинство (?) программ, используюет ее в связке
// WriteConsoleOutputAttributes/WriteConsoleOutputCharacter

BOOL WINAPI OnWriteConsoleOutputCharacterA(HANDLE hConsoleOutput, LPCSTR lpCharacter, DWORD nLength, COORD dwWriteCoord, LPDWORD lpNumberOfCharsWritten)
{
	typedef BOOL (WINAPI* OnWriteConsoleOutputCharacterA_t)(HANDLE hConsoleOutput, LPCSTR lpCharacter, DWORD nLength, COORD dwWriteCoord, LPDWORD lpNumberOfCharsWritten);
	ORIGINALFAST(WriteConsoleOutputCharacterA);
	BOOL bMainThread = FALSE; // поток не важен

	ExtFillOutputParm fll = {sizeof(fll),
		efof_Attribute|(gDisplayParm.WasSet ? efof_Current : efof_ResetExt),
		hConsoleOutput, {}, 0, dwWriteCoord, nLength};
	ExtFillOutput(&fll);
	
	BOOL lbRc = F(WriteConsoleOutputCharacterA)(hConsoleOutput, lpCharacter, nLength, dwWriteCoord, lpNumberOfCharsWritten);

	return lbRc;
}

BOOL WINAPI OnWriteConsoleOutputCharacterW(HANDLE hConsoleOutput, LPCWSTR lpCharacter, DWORD nLength, COORD dwWriteCoord, LPDWORD lpNumberOfCharsWritten)
{
	typedef BOOL (WINAPI* OnWriteConsoleOutputCharacterW_t)(HANDLE hConsoleOutput, LPCWSTR lpCharacter, DWORD nLength, COORD dwWriteCoord, LPDWORD lpNumberOfCharsWritten);
	ORIGINALFAST(WriteConsoleOutputCharacterW);
	BOOL bMainThread = FALSE; // поток не важен

	ExtFillOutputParm fll = {sizeof(fll),
		efof_Attribute|(gDisplayParm.WasSet ? efof_Current : efof_ResetExt),
		hConsoleOutput, {}, 0, dwWriteCoord, nLength};
	ExtFillOutput(&fll);
	
	BOOL lbRc = F(WriteConsoleOutputCharacterW)(hConsoleOutput, lpCharacter, nLength, dwWriteCoord, lpNumberOfCharsWritten);

	return lbRc;
}

BOOL WriteText(OnWriteConsoleW_t _WriteConsoleW, HANDLE hConsoleOutput, LPCWSTR lpBuffer, DWORD nNumberOfCharsToWrite, LPDWORD lpNumberOfCharsWritten, BOOL abCommit = FALSE)
{
	BOOL lbRc = FALSE;
	DWORD /*nWritten = 0,*/ nTotalWritten = 0;

	ExtWriteTextParm write = {sizeof(write), ewtf_Current, hConsoleOutput};
	write.Private = _WriteConsoleW;

	if (gDisplayOpt.WrapWasSet && (gDisplayOpt.WrapAt > 0))
	{
		write.Flags |= ewtf_WrapAt;
		write.WrapAtCol = gDisplayOpt.WrapAt;
	}

	//lbRc = _WriteConsoleW(hConsoleOutput, lpBuffer, nNumberOfCharsToWrite, &nTotalWritten, NULL);
	write.Buffer = lpBuffer;
	write.NumberOfCharsToWrite = nNumberOfCharsToWrite;
	
	lbRc = ExtWriteText(&write);
	if (lbRc)
	{
		if (write.NumberOfCharsWritten)
			nTotalWritten += write.NumberOfCharsWritten;
		if (write.ScrolledRowsUp > 0)
			gDisplayCursor.StoredCursorPos.Y = max(0,((int)gDisplayCursor.StoredCursorPos.Y - (int)write.ScrolledRowsUp));
	}

	if (lpNumberOfCharsWritten)
		*lpNumberOfCharsWritten = nTotalWritten;

	return lbRc;
}

BOOL WINAPI OnWriteConsoleW(HANDLE hConsoleOutput, const VOID *lpBuffer, DWORD nNumberOfCharsToWrite, LPDWORD lpNumberOfCharsWritten, LPVOID lpReserved)
{
	ORIGINALFAST(WriteConsoleW);
	BOOL bMainThread = FALSE; // поток не важен
	BOOL lbRc = FALSE;
	//ExtWriteTextParm wrt = {sizeof(wrt), ewtf_None, hConsoleOutput};
	bool bIsConOut = false;

	// Store prompt(?) for clink
	if (nNumberOfCharsToWrite && lpBuffer && gpszLastWriteConsole && gcchLastWriteConsoleMax)
	{
		size_t cchMax = min(gcchLastWriteConsoleMax-1,nNumberOfCharsToWrite);
		gpszLastWriteConsole[cchMax] = 0;
		wmemmove(gpszLastWriteConsole, (const wchar_t*)lpBuffer, cchMax);
	}

	#ifdef DUMP_WRITECONSOLE_LINES
	wchar_t szDbg[120], *pch;
	size_t nLen = min(90,nNumberOfCharsToWrite);
	static int nWriteCallNo = 0;
	msprintf(szDbg, countof(szDbg), L"AnsiDump #%u: ", ++nWriteCallNo);
	size_t nStart = lstrlenW(szDbg);
	wmemmove(szDbg+nStart, (wchar_t*)lpBuffer, nLen);
	szDbg[nLen+nStart] = 0;
	while ((pch = wcspbrk(szDbg+nStart, L"\r\n\t\x1B")) != NULL)
		*pch = szAnalogues[*pch];
	szDbg[nStart+nLen] = L'\n'; szDbg[nStart+nLen+1] = 0;
	DebugString(szDbg);
	#endif

	if (lpBuffer && nNumberOfCharsToWrite && IsAnsiCapable(hConsoleOutput, &bIsConOut))
	{
		if (gnPrevAnsiPart || gDisplayOpt.WrapWasSet)
		{
			// Если остался "хвост" от предущей записи - сразу, без проверок
			lbRc = WriteAnsiCodes(F(WriteConsoleW), hConsoleOutput, (const wchar_t*)lpBuffer, nNumberOfCharsToWrite, lpNumberOfCharsWritten);
			goto ansidone;
		}
		else
		{
			const wchar_t* pch = (const wchar_t*)lpBuffer;
			for (size_t i = nNumberOfCharsToWrite; i--; pch++)
			{
				// Если в выводимой строке встречается "Ansi ESC Code" - выводим сами
				TODO("Non-CSI codes, like as BEL, BS, CR, LF, FF, TAB, VT, SO, SI");
				if (*pch == 27)
				{
					lbRc = WriteAnsiCodes(F(WriteConsoleW), hConsoleOutput, (const wchar_t*)lpBuffer, nNumberOfCharsToWrite, lpNumberOfCharsWritten);
					goto ansidone;
				}
			}
		}
	}

	if (!bIsConOut)
	{
		lbRc = F(WriteConsoleW)(hConsoleOutput, lpBuffer, nNumberOfCharsToWrite, lpNumberOfCharsWritten, lpReserved);
	}
	else
	{
		lbRc = WriteText(F(WriteConsoleW), hConsoleOutput, (LPCWSTR)lpBuffer, nNumberOfCharsToWrite, lpNumberOfCharsWritten, TRUE);
		//wrt.Flags = ewtf_Current|ewtf_Commit;
		//wrt.Buffer = (const wchar_t*)lpBuffer;
		//wrt.NumberOfCharsToWrite = nNumberOfCharsToWrite;
		//wrt.Private = F(WriteConsoleW);
		//lbRc = ExtWriteText(&wrt);
		//if (lbRc)
		//{
		//	if (lpNumberOfCharsWritten)
		//		*lpNumberOfCharsWritten = wrt.NumberOfCharsWritten;
		//	if (wrt.ScrolledRowsUp > 0)
		//		gDisplayCursor.StoredCursorPos.Y = max(0,((int)gDisplayCursor.StoredCursorPos.Y - (int)wrt.ScrolledRowsUp));
		//}
	}
	goto wrap;

ansidone:
	{
		ExtCommitParm cmt = {sizeof(cmt), hConsoleOutput};
		ExtCommit(&cmt);
	}
wrap:
	return lbRc;
}

struct AnsiEscCode
{
	wchar_t  First;  // ESC (27)
	wchar_t  Second; // any of 64 to 95 ('@' to '_')
	wchar_t  Action; // any of 64 to 126 (@ to ~). this is terminator
	wchar_t  Skip;   // Если !=0 - то эту последовательность нужно пропустить
	int      ArgC;
	int      ArgV[16];
	LPCWSTR  ArgSZ; // Reserved for key mapping
	size_t   cchArgSZ;

#ifdef _DEBUG
	LPCWSTR  pszEscStart;
	size_t   nTotalLen;
#endif

	int      PvtLen;
	wchar_t  Pvt[16];
};


// 0 - нет (в lpBuffer только текст)
// 1 - в Code помещена Esc последовательность (может быть простой текст ДО нее)
// 2 - нет, но кусок последовательности сохранен в gsPrevAnsiPart
int NextEscCode(LPCWSTR lpBuffer, LPCWSTR lpEnd, LPCWSTR& lpStart, LPCWSTR& lpNext, AnsiEscCode& Code, BOOL ReEntrance = FALSE)
{
	int iRc = 0;
	wchar_t wc;

	if (gnPrevAnsiPart && !ReEntrance)
	{
		if (*gsPrevAnsiPart == 27)
		{
			_ASSERTE(gnPrevAnsiPart < 79);
			INT_PTR nAdd = min((lpEnd-lpBuffer),(INT_PTR)countof(gsPrevAnsiPart)-gnPrevAnsiPart);
			wmemcpy(gsPrevAnsiPart+gnPrevAnsiPart, lpBuffer, nAdd);
			gsPrevAnsiPart[gnPrevAnsiPart+nAdd] = 0;

			LPCWSTR lpReStart, lpReNext;
			int iCall = NextEscCode(gsPrevAnsiPart, gsPrevAnsiPart+nAdd+gnPrevAnsiPart, lpReStart, lpReNext, Code, TRUE);
			if (iCall == 1)
			{
				if ((lpReNext - gsPrevAnsiPart) >= gnPrevAnsiPart)
				{
					_ASSERTE(lpReStart == gsPrevAnsiPart);
					lpStart = lpBuffer; // nothing to dump before Esc-sequence
					_ASSERTE((lpReNext - gsPrevAnsiPart) >= gnPrevAnsiPart);
					lpNext = lpBuffer + (lpReNext - gsPrevAnsiPart - gnPrevAnsiPart);
				}
				else
				{
					_ASSERTE((lpReNext - gsPrevAnsiPart) >= gnPrevAnsiPart);
					lpStart = lpNext = lpBuffer;
				}
				gnPrevAnsiPart = 0;
				gsPrevAnsiPart[0] = 0;
				return 1;
			}

			_ASSERTE((iCall == 1) && "Invalid esc sequence, need dump to screen?");
		}
		else
		{
			_ASSERTE(*gsPrevAnsiPart == 27);
		}
	}

	LPCWSTR lpSaveStart = lpBuffer;
	lpStart = lpBuffer;

	while (lpBuffer < lpEnd)
	{
		switch (*lpBuffer)
		{
		case 27:
			{
				INT_PTR nLeft;
				LPCWSTR lpEscStart = lpBuffer;

				#ifdef _DEBUG
				Code.pszEscStart = lpBuffer;
				Code.nTotalLen = 0;
				#endif

				// minimal length failed?
				if ((lpBuffer + 2) < lpEnd)
				{
					lpSaveStart = lpBuffer;
					Code.First = 27;
					Code.Second = *(++lpBuffer);
					Code.ArgC = 0;
					Code.PvtLen = 0;
					Code.Pvt[0] = 0;

					if ((Code.Second < 64) || (Code.Second > 95))
						continue; // invalid code
					
					// Теперь идут параметры.
					++lpBuffer; // переместим указатель на первый символ ЗА CSI (после '[')
					
					switch (Code.Second)
					{
					case L'[':
						// Standard
						Code.Skip = 0;
						Code.ArgSZ = NULL;
						Code.cchArgSZ = 0;
						{
							int nValue = 0, nDigits = 0;
							LPCWSTR pszSaveStart = lpBuffer;

							while (lpBuffer < lpEnd)
							{
								switch (*lpBuffer)
								{
								case L'0': case L'1': case L'2': case L'3': case L'4':
								case L'5': case L'6': case L'7': case L'8': case L'9':
									nValue = (nValue * 10) + (((int)*lpBuffer) - L'0');
									++nDigits;
									break;

								case L';':
									// Даже если цифр не было - default "0"
									if (Code.ArgC < countof(Code.ArgV))
										Code.ArgV[Code.ArgC++] = nValue; // save argument
									nDigits = nValue = 0;
									break;

								default:
									if (((wc = *lpBuffer) >= 64) && (wc <= 126))
									{
										// Fin
										Code.Action = wc;
										if (nDigits && (Code.ArgC < countof(Code.ArgV)))
										{
											Code.ArgV[Code.ArgC++] = nValue;
										}
										lpStart = lpSaveStart;
										lpEnd = lpBuffer+1;
										iRc = 1;
										goto wrap;
									}
									else
									{
										if ((Code.PvtLen+1) < countof(Code.Pvt))
										{
											Code.Pvt[Code.PvtLen++] = wc; // Skip private symbols
											Code.Pvt[Code.PvtLen] = 0;
										}
									}
								}
								++lpBuffer;
							}
						}
						// В данном запросе (на запись) конца последовательности нет,
						// оставшийся хвост нужно сохранить в буфере, для следующего запроса
						// Ниже
						break;

					case L']':
						// Finalizing with "\x1B\\" or "\x07"
						// "%]4;16;rgb:00/00/00%\" - "%" is ESC
						// "%]0;this is the window titleBEL"
						// ESC ] 0 ; txt ST        Set icon name and window title to txt.
						// ESC ] 1 ; txt ST        Set icon name to txt.
						// ESC ] 2 ; txt ST        Set window title to txt.
						// ESC ] 4 ; num; txt ST   Set ANSI color num to txt.
						// ESC ] 10 ; txt ST       Set dynamic text color to txt.
						// ESC ] 4 6 ; name ST     Change log file to name (normally disabled
						//					       by a compile-time option)
						// ESC ] 5 0 ; fn ST       Set font to fn.
						//Following 2 codes - from linux terminal
						// ESC ] P nrrggbb         Set palette, with parameter given in 7
                        //                         hexadecimal digits after the final P :-(.
						//                         Here n is the color (0-15), and rrggbb indicates
						//                         the red/green/blue values (0-255).
						// ESC ] R                 reset palette

						// ConEmu specific
						// ESC ] 9 ; 1 ; ms ST           Sleep. ms - milliseconds
						// ESC ] 9 ; 2 ; "txt" ST        Show GUI MessageBox ( txt ) for dubug purposes
						// ESC ] 9 ; 3 ; "txt" ST        Set TAB text
						// ESC ] 9 ; 4 ; st ; pr ST      When _st_ is 0: remove progress. When _st_ is 1: set progress value to _pr_ (number, 0-100). When _st_ is 2: set error state in progress on Windows 7 taskbar
						// ESC ] 9 ; 5 ST                Wait for ENTER/SPACE/ESC. Set EnvVar "ConEmuWaitKey" to ENTER/SPACE/ESC on exit.
						// ESC ] 9 ; 6 ; "txt" ST        Execute GuiMacro. Set EnvVar "ConEmuMacroResult" on exit.

						Code.ArgSZ = lpBuffer;
						Code.cchArgSZ = 0;
						//Code.Skip = Code.Second;

						while (lpBuffer < lpEnd)
						{
							if ((lpBuffer[0] == 7) ||
								((lpBuffer[0] == 27) && ((lpBuffer + 1) < lpEnd) && (lpBuffer[1] == L'\\')))
							{
								Code.Action = *Code.ArgSZ; // первый символ последовательности
								Code.cchArgSZ = (lpBuffer - Code.ArgSZ);
								lpStart = lpSaveStart;
								if (lpBuffer[0] == 27)
								{
									lpEnd = lpBuffer + 2;
								}
								else
								{
									lpEnd = lpBuffer + 1;
								}
								iRc = 1;
								goto wrap;
							}
							++lpBuffer;
						}
						// В данном запросе (на запись) конца последовательности нет,
						// оставшийся хвост нужно сохранить в буфере, для следующего запроса
						// Ниже
						break;

					default:
						// Неизвестный код, обрабатываем по общим правилам
						Code.Skip = Code.Second;
						Code.ArgSZ = lpBuffer;
						Code.cchArgSZ = 0;
						while (lpBuffer < lpEnd)
						{
							if (((wc = *lpBuffer) >= 64) && (wc <= 126))
							{
								Code.Action = wc;
								lpStart = lpSaveStart;
								lpEnd = lpBuffer+1;
								iRc = 1;
								goto wrap;
							}
							++lpBuffer;
						}

					} // end of "switch (Code.Second)"
				} // end of minimal length check

				if ((nLeft = (lpEnd - lpEscStart)) <= MaxPrevAnsiPart)
				{
					if (ReEntrance)
					{
						_ASSERTE(!ReEntrance && "Need to be checked!");
						wmemmove(gsPrevAnsiPart2, lpEscStart, nLeft);
						gsPrevAnsiPart2[nLeft] = 0;
						gnPrevAnsiPart2 = nLeft;
					}
					else
					{
						wmemmove(gsPrevAnsiPart, lpEscStart, nLeft);
						gsPrevAnsiPart[nLeft] = 0;
						gnPrevAnsiPart = nLeft;
					}
				}

				lpStart = lpEscStart;

				iRc = 2;
				goto wrap;
			} // end of "case 27:"
			break;
		} // end of "switch (*lpBuffer)"

		++lpBuffer;
	} // end of "while (lpBuffer < lpEnd)"

wrap:
	lpNext = lpEnd;

	#ifdef _DEBUG
	if (iRc == 1)
		Code.nTotalLen = (lpEnd - Code.pszEscStart);
	#endif

	return iRc;
}

BOOL ScrollScreen(HANDLE hConsoleOutput, int nDir)
{
	TODO("Define scrolling region");
	ExtScrollScreenParm scrl = {sizeof(scrl), essf_Current|essf_Commit, hConsoleOutput, nDir, {}, L' '};
	BOOL lbRc = ExtScrollScreen(&scrl);
	return lbRc;

#if 0
	CONSOLE_SCREEN_BUFFER_INFO csbi = {};
	if (!GetConsoleScreenBufferInfo(hConsoleOutput, &csbi))
		return FALSE;

	BOOL lbRc = FALSE;
	CHAR_INFO Buf[200];
	CHAR_INFO* pBuf = (csbi.dwSize.X <= countof(Buf)) ? Buf : (CHAR_INFO*)malloc(csbi.dwSize.X*sizeof(*pBuf));
	DWORD nCount;
	COORD crSize = {csbi.dwSize.X,1};
	COORD cr0 = {};
	SMALL_RECT rcRgn = {0,0,csbi.dwSize.X-1,0};

	if (!pBuf)
		return FALSE;

	WARNING("Расширенные атрибуты тоже нада двигать!");
	//_ASSERTE(FALSE);

	if (nDir < 0)
	{
		// Scroll whole page up by n (default 1) lines. New lines are added at the bottom.
		for (SHORT y = 0, y1 = -nDir; y1 < csbi.dwSize.Y; y++, y1++)
		{
			rcRgn.Top = rcRgn.Bottom = y1;
			if (MyReadConsoleOutputW(hConsoleOutput, pBuf, crSize, cr0, &rcRgn))
			{
				rcRgn.Top = rcRgn.Bottom = y;
				WriteConsoleOutputW(hConsoleOutput, pBuf, crSize, cr0, &rcRgn);
			}
		}

		while (nDir < 0)
		{
			cr0.Y = csbi.dwSize.Y - 1 + nDir;
			FillConsoleOutputAttribute(hConsoleOutput, GetDefaultTextAttr(), csbi.dwSize.X, cr0, &nCount);
			FillConsoleOutputCharacter(hConsoleOutput, L' ', csbi.dwSize.X, cr0, &nCount);
			++nDir;
		}
	}
	else if (nDir > 0)
	{
		// Scroll whole page down by n (default 1) lines. New lines are added at the top.
		for (SHORT y = csbi.dwSize.Y - nDir - 1, y1 = csbi.dwSize.Y - 1; y >= 0; y--, y1--)
		{
			rcRgn.Top = rcRgn.Bottom = y;
			if (MyReadConsoleOutputW(hConsoleOutput, pBuf, crSize, cr0, &rcRgn))
			{
				rcRgn.Top = rcRgn.Bottom = y1;
				WriteConsoleOutputW(hConsoleOutput, pBuf, crSize, cr0, &rcRgn);
			}
		}

		while (nDir > 0)
		{
			cr0.Y = nDir - 1;
			FillConsoleOutputAttribute(hConsoleOutput, GetDefaultTextAttr(), csbi.dwSize.X, cr0, &nCount);
			FillConsoleOutputCharacter(hConsoleOutput, L' ', csbi.dwSize.X, cr0, &nCount);
			--nDir;
		}
	}

	if (pBuf != Buf)
		free(pBuf);

	return lbRc;
#endif
}

BOOL PadAndScroll(HANDLE hConsoleOutput, CONSOLE_SCREEN_BUFFER_INFO& csbi)
{
	BOOL lbRc = FALSE;
	COORD crFrom = {csbi.dwCursorPosition.X, csbi.dwCursorPosition.Y};
	DWORD nCount = csbi.dwSize.X - csbi.dwCursorPosition.X; //, nWritten;

	/*
	lbRc = FillConsoleOutputAttribute(hConsoleOutput, GetDefaultTextAttr(), nCount, crFrom, &nWritten)
		&& FillConsoleOutputCharacter(hConsoleOutput, L' ', nCount, crFrom, &nWritten);
	*/

	if ((csbi.dwCursorPosition.Y + 1) >= csbi.dwSize.Y)
	{
		lbRc = ScrollScreen(hConsoleOutput, -1);
		crFrom.X = 0;
	}
	else
	{
		crFrom.X = 0; crFrom.Y++;
		lbRc = TRUE;
	}

	csbi.dwCursorPosition = crFrom;
	SetConsoleCursorPosition(hConsoleOutput, crFrom);

	return lbRc;
}

#ifdef DUMP_UNKNOWN_ESCAPES
void DumpUnknownEscape(LPCWSTR buf, size_t cchLen)
{
	if (!buf || !cchLen)
	{
		_ASSERTE(buf && cchLen);
	}

	wchar_t szEsc[128], *pch;
	wcscpy_c(szEsc, L"###Unknown Esc Sequence: ");
	size_t nCurLen = lstrlen(szEsc);
	size_t nLen = min(80,cchLen);
	wmemcpy(szEsc+nCurLen, buf, nLen);
	szEsc[nCurLen+nLen] = 0;
	while ((pch = wcspbrk(szEsc, L"\r\n\t\x1B")) != NULL)
		*pch = szAnalogues[*pch];
	wcscat_c(szEsc, L"\n");

	DebugString(szEsc);
}
#else
#define DumpUnknownEscape(buf,cchLen)
#endif

static int NextNumber(LPCWSTR& asMS)
{
	wchar_t wc;
	int ms = 0;
	while (((wc = *(asMS++)) >= L'0') && (wc <= L'9'))
		ms = (ms * 10) + (int)(wc - L'0');
	return ms;
}

// ESC ] 9 ; 1 ; ms ST           Sleep. ms - milliseconds
void DoSleep(LPCWSTR asMS)
{
	int ms = NextNumber(asMS);
	if (!ms)
		ms = 100;
	else if (ms > 10000)
		ms = 10000;
	// Delay
	Sleep(ms);
}

void EscCopyCtrlString(wchar_t* pszDst, LPCWSTR asMsg, INT_PTR cchMaxLen)
{
	if (!pszDst)
	{
		_ASSERTEX(pszDst!=NULL);
		return;
	}

	if (cchMaxLen < 0)
	{
		_ASSERTE(cchMaxLen >= 0);
		cchMaxLen = 0;
	}
	if (cchMaxLen > 1)
	{
		if ((asMsg[0] == L'"') && (asMsg[cchMaxLen-1] == L'"'))
		{
			asMsg++;
			cchMaxLen -= 2;
		}
	}

	if (cchMaxLen > 0)
		wmemmove(pszDst, asMsg, cchMaxLen);
	pszDst[max(cchMaxLen,0)] = 0;
}

// ESC ] 9 ; 2 ; "txt" ST          Show GUI MessageBox ( txt ) for dubug purposes
void DoMessage(LPCWSTR asMsg, INT_PTR cchLen)
{
	//if (cchLen < 0)
	//{
	//	_ASSERTE(cchLen >= 0);
	//	cchLen = 0;
	//}
	//if (cchLen > 1)
	//{
	//	if ((asMsg[0] == L'"') && (asMsg[cchLen-1] == L'"'))
	//	{
	//		asMsg++;
	//		cchLen -= 2;
	//	}
	//}
	wchar_t* pszText = (wchar_t*)malloc((cchLen+1)*sizeof(*pszText));

	if (pszText)
	{
		EscCopyCtrlString(pszText, asMsg, cchLen);
		//if (cchLen > 0)
		//	wmemmove(pszText, asMsg, cchLen);
		//pszText[cchLen] = 0;

		wchar_t szExe[MAX_PATH] = {};
		GetModuleFileName(NULL, szExe, countof(szExe));
		wchar_t szTitle[MAX_PATH+64];
		msprintf(szTitle, countof(szTitle), L"PID=%u, %s", GetCurrentProcessId(), PointToName(szExe));

		GuiMessageBox(ghConEmuWnd, pszText, szTitle, MB_ICONINFORMATION|MB_SYSTEMMODAL);
		
		free(pszText);
	}
}

BOOL WriteAnsiCodes(OnWriteConsoleW_t _WriteConsoleW, HANDLE hConsoleOutput, LPCWSTR lpBuffer, DWORD nNumberOfCharsToWrite, LPDWORD lpNumberOfCharsWritten)
{
	BOOL lbRc = TRUE, lbApply = FALSE;
	LPCWSTR lpEnd = (lpBuffer + nNumberOfCharsToWrite);
	AnsiEscCode Code = {};
	CONSOLE_SCREEN_BUFFER_INFO csbi = {};

	//ExtWriteTextParm write = {sizeof(write), ewtf_Current, hConsoleOutput};
	//write.Private = _WriteConsoleW;

	while (lpBuffer < lpEnd)
	{
		LPCWSTR lpStart, lpNext;

		// '^' is ESC
		// ^[0;31;47m   $E[31;47m   ^[0m ^[0;1;31;47m  $E[1;31;47m  ^[0m

		int iEsc = NextEscCode(lpBuffer, lpEnd, lpStart, lpNext, Code);

		if (iEsc != 0)
		{
			if (lpStart > lpBuffer)
			{
				if (lbApply)
				{
					ReSetDisplayParm(hConsoleOutput, FALSE, TRUE);
					lbApply = FALSE;
				}

				DWORD nWrite = (DWORD)(lpStart - lpBuffer);
				//lbRc = WriteText(_WriteConsoleW, hConsoleOutput, lpBuffer, nWrite, lpNumberOfCharsWritten);
				lbRc = WriteText(_WriteConsoleW, hConsoleOutput, lpBuffer, nWrite, lpNumberOfCharsWritten);
				if (!lbRc)
					goto wrap;
				//write.Buffer = lpBuffer;
				//write.NumberOfCharsToWrite = nWrite;
				//lbRc = ExtWriteText(&write);
				//if (!lbRc)
				//	goto wrap;
				//else
				//{
				//	if (lpNumberOfCharsWritten)
				//		*lpNumberOfCharsWritten = write.NumberOfCharsWritten;
				//	if (write.ScrolledRowsUp > 0)
				//		gDisplayCursor.StoredCursorPos.Y = max(0,((int)gDisplayCursor.StoredCursorPos.Y - (int)write.ScrolledRowsUp));
				//}
			}

			if (iEsc == 1)
			{
				if (Code.Skip)
				{
					DumpUnknownEscape(Code.pszEscStart,Code.nTotalLen);
				}
				else
				{
					switch (Code.Second)
					{
					case L'[':
						{
							lbApply = TRUE;

							switch (Code.Action) // case sensitive
							{
							case L's':
								// Save cursor position (can not be nested)
								if (GetConsoleScreenBufferInfo(hConsoleOutput, &csbi))
									gDisplayCursor.StoredCursorPos = csbi.dwCursorPosition;
								break;

							case L'u':
								// Restore cursor position
								SetConsoleCursorPosition(hConsoleOutput, gDisplayCursor.StoredCursorPos);
								break;

							case L'H': // Set cursor position (1-based)
							case L'f': // Same as 'H'
							case L'A': // Cursor up by N rows
							case L'B': // Cursor right by N cols
							case L'C': // Cursor right by N cols
							case L'D': // Cursor left by N cols
							case L'E': // Moves cursor to beginning of the line n (default 1) lines down.
							case L'F': // Moves cursor to beginning of the line n (default 1) lines up.
							case L'G': // Moves the cursor to column n.
								// Change cursor position
								if (GetConsoleScreenBufferInfo(hConsoleOutput, &csbi))
								{
									COORD crNewPos = csbi.dwCursorPosition;

									switch (Code.Action)
									{
									case L'H':
									case L'f':
										// Set cursor position (1-based)
										crNewPos.Y = (Code.ArgC > 0 && Code.ArgV[0]) ? (Code.ArgV[0] - 1) : 0;
										crNewPos.X = (Code.ArgC > 1 && Code.ArgV[1]) ? (Code.ArgV[1] - 1) : 0;
										break;
									case L'A':
										// Cursor up by N rows
										crNewPos.Y -= (Code.ArgC > 0 && Code.ArgV[0]) ? Code.ArgV[0] : 1;
										break;
									case L'B':
										// Cursor down by N rows
										crNewPos.Y += (Code.ArgC > 0 && Code.ArgV[0]) ? Code.ArgV[0] : 1;
										break;
									case L'C':
										// Cursor right by N cols
										crNewPos.X += (Code.ArgC > 0 && Code.ArgV[0]) ? Code.ArgV[0] : 1;
										break;
									case L'D':
										// Cursor left by N cols
										crNewPos.X -= (Code.ArgC > 0 && Code.ArgV[0]) ? Code.ArgV[0] : 1;
										break;
									case L'E':
										// Moves cursor to beginning of the line n (default 1) lines down.
										crNewPos.Y += (Code.ArgC > 0 && Code.ArgV[0]) ? Code.ArgV[0] : 1;
										crNewPos.X = 0;
										break;
									case L'F':
										// Moves cursor to beginning of the line n (default 1) lines up.
										crNewPos.Y -= (Code.ArgC > 0 && Code.ArgV[0]) ? Code.ArgV[0] : 1;
										crNewPos.X = 0;
										break;
									case L'G':
										// Moves the cursor to column n.
										crNewPos.X = (Code.ArgC > 0) ? (Code.ArgV[0] - 1) : 0;
										break;
									#ifdef _DEBUG
									default:
										_ASSERTE(FALSE && "Missed (sub)case value!");
									#endif	
									}

									// проверка Row
									if (crNewPos.Y < 0)
										crNewPos.Y = 0;
									else if (crNewPos.Y >= csbi.dwSize.Y)
										crNewPos.Y = csbi.dwSize.Y - 1;
									// проверка Col
									if (crNewPos.X < 0)
										crNewPos.X = 0;
									else if (crNewPos.X >= csbi.dwSize.X)
										crNewPos.X = csbi.dwSize.X - 1;
									// Goto
                                    SetConsoleCursorPosition(hConsoleOutput, crNewPos);
								} // case L'H': case L'f': case 'A': case L'B': case L'C': case L'D':
								break;

							case L'J': // Clears part of the screen
								// Clears the screen and moves the cursor to the home position (line 0, column 0). 
								if (GetConsoleScreenBufferInfo(hConsoleOutput, &csbi))
								{
									int nCmd = (Code.ArgC > 0) ? Code.ArgV[0] : 0;
									COORD cr0 = {};
									int nChars = 0;

									switch (nCmd)
									{
									case 0:
										// clear from cursor to end of screen
										cr0 = csbi.dwCursorPosition;
										nChars = (csbi.dwSize.X - csbi.dwCursorPosition.X)
											+ csbi.dwSize.X * (csbi.dwSize.Y - csbi.dwCursorPosition.Y - 1);
										break;
									case 1:
										// clear from cursor to beginning of the screen
										nChars = csbi.dwCursorPosition.X + 1
											+ csbi.dwSize.X * csbi.dwCursorPosition.Y;
										break;
									case 2:
										// clear entire screen and moves cursor to upper left
										nChars = csbi.dwSize.X * csbi.dwSize.Y;
										break;
									default:
										DumpUnknownEscape(Code.pszEscStart,Code.nTotalLen);
									}

									if (nChars > 0)
									{
										ExtFillOutputParm fill = {sizeof(fill), efof_Current|efof_Attribute|efof_Character,
											hConsoleOutput, {}, L' ', cr0, nChars};
										ExtFillOutput(&fill);
										//DWORD nWritten = 0;
										//FillConsoleOutputAttribute(hConsoleOutput, GetDefaultTextAttr(), nChars, cr0, &nWritten);
										//FillConsoleOutputCharacter(hConsoleOutput, L' ', nChars, cr0, &nWritten);
									}

									if (nCmd == 2)
									{
										SetConsoleCursorPosition(hConsoleOutput, cr0);
									}

									TODO("Need to clear attributes?");
									ReSetDisplayParm(hConsoleOutput, TRUE/*bReset*/, TRUE/*bApply*/);
								} // case L'J':
								break;

							case L'K': // Erases part of the line
								// Clears all characters from the cursor position to the end of the line
								// (including the character at the cursor position).
								if (GetConsoleScreenBufferInfo(hConsoleOutput, &csbi))
								{
									TODO("Need to clear attributes?");
									int nChars = 0;
									int nCmd = (Code.ArgC > 0) ? Code.ArgV[0] : 0;
									COORD cr0 = csbi.dwCursorPosition;

									switch (nCmd)
									{
									case 0: // clear from cursor to the end of the line
										nChars = csbi.dwSize.X - csbi.dwCursorPosition.X - 1;
										break;
									case 1: // clear from cursor to beginning of the line
										cr0.X = cr0.Y = 0;
										nChars = csbi.dwCursorPosition.X + 1;
										break;
									case 2: // clear entire line
										cr0.X = cr0.Y = 0;
										nChars = csbi.dwSize.X;
										break;
									default:
										DumpUnknownEscape(Code.pszEscStart,Code.nTotalLen);
									}


									if (nChars > 0)
									{
										ExtFillOutputParm fill = {sizeof(fill), efof_Current|efof_Attribute|efof_Character,
											hConsoleOutput, {}, L' ', cr0, nChars};
										ExtFillOutput(&fill);
										//DWORD nWritten = 0;
										//FillConsoleOutputAttribute(hConsoleOutput, GetDefaultTextAttr(), nChars, cr0, &nWritten);
										//FillConsoleOutputCharacter(hConsoleOutput, L' ', nChars, cr0, &nWritten);
									}
								} // case L'K':
								break;

							case L'r':
								//\027[Pt;Pbr
								//
								//Pt is the number of the top line of the scrolling region;
								//Pb is the number of the bottom line of the scrolling region 
								// and must be greater than Pt.
								//(The default for Pt is line 1, the default for Pb is the end 
								// of the screen)
								_ASSERTE(FALSE && "Define scrolling region");
								break;

							case L'S':
								// Scroll whole page up by n (default 1) lines. New lines are added at the bottom.
								TODO("Define scrolling region");
								ScrollScreen(hConsoleOutput, (Code.ArgC > 0 && Code.ArgV[0] > 0) ? -Code.ArgV[0] : -1);
								break;

							case L'T':
								// Scroll whole page down by n (default 1) lines. New lines are added at the top.
								TODO("Define scrolling region");
								ScrollScreen(hConsoleOutput, (Code.ArgC > 0 && Code.ArgV[0] > 0) ? Code.ArgV[0] : 1);
								break;

							case L'h':
							case L'l':
								// Set/ReSet Mode
								if (Code.ArgC > 0)
								{
									//ESC [ 3 h
									//       DECCRM (default off): Display control chars.

									//ESC [ 4 h
									//       DECIM (default off): Set insert mode.

									//ESC [ 20 h
									//       LF/NL (default off): Automatically follow echo of LF, VT or FF with CR.

									//ESC [ ? 1 h
									//	  DECCKM (default off): When set, the cursor keys send an ESC O prefix,
									//	  rather than ESC [.

									//ESC [ ? 3 h
									//	  DECCOLM (default off = 80 columns): 80/132 col mode switch.  The driver
									//	  sources note that this alone does not suffice; some user-mode utility
									//	  such as resizecons(8) has to change the hardware registers on the
									//	  console video card.

									//ESC [ ? 5 h
									//	  DECSCNM (default off): Set reverse-video mode.

									//ESC [ ? 6 h
									//	  DECOM (default off): When set, cursor addressing is relative to the
									//	  upper left corner of the scrolling region.


									//ESC [ ? 8 h
									//	  DECARM (default on): Set keyboard autorepeat on.

									//ESC [ ? 9 h
									//	  X10 Mouse Reporting (default off): Set reporting mode to 1 (or reset to
									//	  0) -- see below.

									//ESC [ ? 25 h
									//	  DECTECM (default on): Make cursor visible.

									//ESC [ ? 1000 h
									//	  X11 Mouse Reporting (default off): Set reporting mode to 2 (or reset to
									//	  0) -- see below.

									switch (Code.ArgV[0])
									{
									case 7:
										//ESC [ ? 7 h
										//	  DECAWM (default off): Set autowrap on.  In this mode, a graphic
										//	  character emitted after column 80 (or column 132 of DECCOLM is on)
										//	  forces a wrap to the beginning of the following line first.
										//ESC [ = 7 h
										//    Enables line wrapping 
										//ESC [ 7 ; _col_ h
										//    Our extension. _col_ - wrap at column (1-based), default = 80
										if ((gDisplayOpt.WrapWasSet = (Code.Action == L'h')))
										{
											gDisplayOpt.WrapAt = ((Code.ArgC > 1) && (Code.ArgV[1] > 0)) ? Code.ArgV[1] : 80;
										}
										break;
									case 20:
										// Ignored for now
										gDisplayOpt.AutoLfNl = (Code.Action == L'h');
										break;
									case 25:
										{
											CONSOLE_CURSOR_INFO ci = {};
											if (GetConsoleCursorInfo(hConsoleOutput, &ci))
											{
												ci.bVisible = (Code.Action == L'h');
												SetConsoleCursorInfo(hConsoleOutput, &ci);
											}
										}
										break;
									default:
										DumpUnknownEscape(Code.pszEscStart,Code.nTotalLen);
									}

									//switch (Code.ArgV[0])
									//{
									//case 0: case 1:
									//	// 40x25
									//	if ((gDisplayOpt.WrapWasSet = (Code.Action == L'h')))
									//	{
									//		gDisplayOpt.WrapAt = 40;
									//	}
									//	break;
									//case 2: case 3:
									//	// 80x25
									//	if ((gDisplayOpt.WrapWasSet = (Code.Action == L'h')))
									//	{
									//		gDisplayOpt.WrapAt = 80;
									//	}
									//	break;
									//case 7:
									//	{
									//		DWORD Mode = 0;
									//		GetConsoleMode(hConsoleOutput, &Mode);
									//		if (Code.Action == L'h')
									//			Mode |= ENABLE_WRAP_AT_EOL_OUTPUT;
									//		else
									//			Mode &= ~ENABLE_WRAP_AT_EOL_OUTPUT;
									//		SetConsoleMode(hConsoleOutput, Mode);
									//	} // enable/disable line wrapping
									//	break;
									//}
								}
								else
								{
									DumpUnknownEscape(Code.pszEscStart,Code.nTotalLen);
								}
								break; // case L'h': case L'l':

							case L'n':
								{
									TODO("ECMA-48 Status Report Commands");
									//ESC [ 5 n
									//      Device status report (DSR): Answer is ESC [ 0 n (Terminal OK).
									//
									//ESC [ 6 n
									//      Cursor position report (CPR): Answer is ESC [ y ; x R, where x,y is the
									//      cursor location.
									DumpUnknownEscape(Code.pszEscStart,Code.nTotalLen);
								}
								break;

							case L'm':
								// Set display mode (colors, fonts, etc.)
								if (!Code.ArgC)
								{
									ReSetDisplayParm(hConsoleOutput, TRUE, FALSE);
								}
								else
								{
									for (int i = 0; i < Code.ArgC; i++)
									{
										switch (Code.ArgV[i])
										{
										case 0:
											ReSetDisplayParm(hConsoleOutput, TRUE, FALSE);
											break;
										case 1:
											gDisplayParm.BrightOrBold = TRUE;
											gDisplayParm.WasSet = TRUE;
											break;
										case 2:
											TODO("Check standard");
											gDisplayParm.BrightOrBold = FALSE;
											gDisplayParm.WasSet = TRUE;
											break;
										case 3:
											gDisplayParm.ItalicOrInverse = TRUE;
											gDisplayParm.WasSet = TRUE;
											break;
										case 4:
										case 5:
											TODO("Check standard");
											gDisplayParm.BackOrUnderline = TRUE;
											gDisplayParm.WasSet = TRUE;
											break;
										case 7:
											// Reverse video
											TODO("Check standard");
											break;
										case 30: case 31: case 32: case 33: case 34: case 35: case 36: case 37:
											gDisplayParm.TextColor = (Code.ArgV[i] - 30);
											gDisplayParm.Text256 = FALSE;
											gDisplayParm.WasSet = TRUE;
											break;
										case 38:
											// 256 colors
											if (((i+2) < Code.ArgC) && (Code.ArgV[i+1] == 5))
											{
												gDisplayParm.TextColor = (Code.ArgV[i+2] & 0xFF);
												gDisplayParm.Text256 = TRUE;
												gDisplayParm.WasSet = TRUE;
												i += 2;
											}
											break;
										case 39:
											// Reset
											gDisplayParm.TextColor = GetDefaultTextAttr() & 0xF;
											gDisplayParm.Text256 = FALSE;
											gDisplayParm.WasSet = TRUE;
											break;
										case 40: case 41: case 42: case 43: case 44: case 45: case 46: case 47:
											gDisplayParm.BackColor = (Code.ArgV[i] - 40);
											gDisplayParm.Back256 = FALSE;
											gDisplayParm.WasSet = TRUE;
											break;
										case 48:
											// 256 colors
											if (((i+2) < Code.ArgC) && (Code.ArgV[i+1] == 5))
											{
												gDisplayParm.BackColor = (Code.ArgV[i+2] & 0xFF);
												gDisplayParm.Back256 = TRUE;
												gDisplayParm.WasSet = TRUE;
												i += 2;
											}
											break;
										case 49:
											// Reset
											gDisplayParm.BackColor = (GetDefaultTextAttr() & 0xF0) >> 4;
											gDisplayParm.Back256 = FALSE;
											gDisplayParm.WasSet = TRUE;
											break;
										default:
											DumpUnknownEscape(Code.pszEscStart,Code.nTotalLen);
										}
									}
								}
								break; // "[...m"

							default:
								DumpUnknownEscape(Code.pszEscStart,Code.nTotalLen);
							} // switch (Code.Action)

						} // case L'[':
						break;

					case L']':
						{
							//ESC ] 0 ; txt ST        Set icon name and window title to txt.
							//ESC ] 1 ; txt ST        Set icon name to txt.
							//ESC ] 2 ; txt ST        Set window title to txt.
							//ESC ] 4 ; num; txt ST   Set ANSI color num to txt.
							//ESC ] 10 ; txt ST       Set dynamic text color to txt.
							//ESC ] 4 6 ; name ST     Change log file to name (normally disabled
							//					      by a compile-time option)
							//ESC ] 5 0 ; fn ST       Set font to fn.

							switch (*Code.ArgSZ)
							{
							case L'2':
								if (Code.ArgSZ[1] == L';' && Code.ArgSZ[2])
								{
									wchar_t* pszNewTitle = (wchar_t*)malloc(sizeof(wchar_t)*(Code.cchArgSZ));
									if (pszNewTitle)
									{
										EscCopyCtrlString(pszNewTitle, Code.ArgSZ+2, Code.cchArgSZ-2);
										SetConsoleTitle(*pszNewTitle ? pszNewTitle : gsInitConTitle);
										free(pszNewTitle);
									}
								}
								break;

							case L'9':
								// ConEmu specific
								// ESC ] 9 ; 1 ; ms ST           Sleep. ms - milliseconds
								// ESC ] 9 ; 2 ; "txt" ST        Show GUI MessageBox ( txt ) for dubug purposes
								// ESC ] 9 ; 3 ; "txt" ST        Set TAB text
								// ESC ] 9 ; 4 ; st ; pr ST      When _st_ is 0: remove progress. When _st_ is 1: set progress value to _pr_ (number, 0-100). When _st_ is 2: set error state in progress on Windows 7 taskbar
								// ESC ] 9 ; 5 ST                Wait for ENTER/SPACE/ESC. Set EnvVar "ConEmuWaitKey" to ENTER/SPACE/ESC on exit.
								// ESC ] 9 ; 6 ; "txt" ST        Execute GuiMacro. Set EnvVar "ConEmuMacroResult" on exit.
								// -- You may specify timeout _s_ in seconds. - не работает
								if (Code.ArgSZ[1] == L';')
								{
									if (Code.ArgSZ[2] == L'1' && Code.ArgSZ[3] == L';')
									{
										DoSleep(Code.ArgSZ+4);
									}
									else if (Code.ArgSZ[2] == L'2' && Code.ArgSZ[3] == L';')
									{
										DoMessage(Code.ArgSZ+4, Code.cchArgSZ - 4);
									}
									else if (Code.ArgSZ[2] == L'3' && Code.ArgSZ[3] == L';')
									{
										CESERVER_REQ* pIn = ExecuteNewCmd(CECMD_SETTABTITLE, sizeof(CESERVER_REQ_HDR)+sizeof(wchar_t)*(Code.cchArgSZ));
										if (pIn)
										{
											EscCopyCtrlString((wchar_t*)pIn->wData, Code.ArgSZ+4, Code.cchArgSZ-4);
											CESERVER_REQ* pOut = ExecuteGuiCmd(ghConWnd, pIn, ghConWnd);
											ExecuteFreeResult(pIn);
											ExecuteFreeResult(pOut);
										}
									}
									else if (Code.ArgSZ[2] == L'4')
									{
										WORD st = 0, pr = 0;
										if (Code.ArgSZ[3] == L';')
										{
											switch (Code.ArgSZ[4])
											{
											case L'0':
												break;
											case L'1':
												st = 1;
												if (Code.ArgSZ[5] == L';')
												{
													LPCWSTR pszValue = Code.ArgSZ + 6;
													pr = NextNumber(pszValue);
												}
												break;
											case L'2':
												st = 2;
												break;
											}
										}
										GuiSetProgress(st,pr);
									}
									else if (Code.ArgSZ[2] == L'5')
									{
										//int s = 0;
										//if (Code.ArgSZ[3] == L';')
										//	s = NextNumber(Code.ArgSZ+4);
										BOOL bSucceeded = FALSE;
										DWORD nRead = 0;
										INPUT_RECORD r = {};
										HANDLE hIn = GetStdHandle(STD_INPUT_HANDLE);
										//DWORD nStartTick = GetTickCount();
										while ((bSucceeded = ReadConsoleInput(hIn, &r, 1, &nRead)) && nRead)
										{
											if ((r.EventType == KEY_EVENT) && r.Event.KeyEvent.bKeyDown)
											{
												if ((r.Event.KeyEvent.wVirtualKeyCode == VK_RETURN)
													|| (r.Event.KeyEvent.wVirtualKeyCode == VK_SPACE)
													|| (r.Event.KeyEvent.wVirtualKeyCode == VK_ESCAPE))
												{
													break;
												}
											}
										}
										if (bSucceeded && ((r.Event.KeyEvent.wVirtualKeyCode == VK_RETURN)
													|| (r.Event.KeyEvent.wVirtualKeyCode == VK_SPACE)
													|| (r.Event.KeyEvent.wVirtualKeyCode == VK_ESCAPE)))
										{
											SetEnvironmentVariable(ENV_CONEMUANSI_WAITKEY,
												(r.Event.KeyEvent.wVirtualKeyCode == VK_RETURN) ? L"RETURN" :
												(r.Event.KeyEvent.wVirtualKeyCode == VK_SPACE)  ? L"SPACE" :
												(r.Event.KeyEvent.wVirtualKeyCode == VK_ESCAPE) ? L"ESC" :
												L"???");
										}
										else
										{
											SetEnvironmentVariable(ENV_CONEMUANSI_WAITKEY, L"");
										}
									}
									else if (Code.ArgSZ[2] == L'6' && Code.ArgSZ[3] == L';')
									{
										CESERVER_REQ* pOut = NULL;
										CESERVER_REQ* pIn = ExecuteNewCmd(CECMD_GUIMACRO, sizeof(CESERVER_REQ_HDR)+sizeof(sizeof(CESERVER_REQ_GUIMACRO))+sizeof(wchar_t)*(Code.cchArgSZ));

										if (pIn)
										{
											EscCopyCtrlString(pIn->GuiMacro.sMacro, Code.ArgSZ+4, Code.cchArgSZ-4);
											pOut = ExecuteGuiCmd(ghConWnd, pIn, ghConWnd);
										}

										// EnvVar "ConEmuMacroResult"
										SetEnvironmentVariable(CEGUIMACRORETENVVAR, pOut && pOut->GuiMacro.nSucceeded ? pOut->GuiMacro.sMacro : NULL);

										ExecuteFreeResult(pOut);
										ExecuteFreeResult(pIn);
									}
								}
								break;

							default:
								DumpUnknownEscape(Code.pszEscStart,Code.nTotalLen);
							}
						} // case L']':
						break;
					
					default:
						DumpUnknownEscape(Code.pszEscStart,Code.nTotalLen);
					}
				}
			}
		}
		else //if (iEsc != 2) // 2 - means "Esc part stored in buffer"
		{
			_ASSERTE(iEsc == 0);
			if (lpNext > lpBuffer)
			{
				if (lbApply)
				{
					ReSetDisplayParm(hConsoleOutput, FALSE, TRUE);
					lbApply = FALSE;
				}

				DWORD nWrite = (DWORD)(lpNext - lpBuffer);
				//lbRc = WriteText(_WriteConsoleW, hConsoleOutput, lpBuffer, nWrite, lpNumberOfCharsWritten);
				lbRc = WriteText(_WriteConsoleW, hConsoleOutput, lpBuffer, nWrite, lpNumberOfCharsWritten);
				if (!lbRc)
					goto wrap;
				//write.Buffer = lpBuffer;
				//write.NumberOfCharsToWrite = nWrite;
				//lbRc = ExtWriteText(&write);
				//if (!lbRc)
				//	goto wrap;
				//else
				//{
				//	if (lpNumberOfCharsWritten)
				//		*lpNumberOfCharsWritten = write.NumberOfCharsWritten;
				//	if (write.ScrolledRowsUp > 0)
				//		gDisplayCursor.StoredCursorPos.Y = max(0,((int)gDisplayCursor.StoredCursorPos.Y - (int)write.ScrolledRowsUp));
				//}
			}
		}

		if (lpNext > lpBuffer)
		{
			lpBuffer = lpNext;
		}
		else
		{
			_ASSERTE(lpNext > lpBuffer);
			++lpBuffer;
		}
	}

	if (lbRc && lpNumberOfCharsWritten)
		*lpNumberOfCharsWritten = nNumberOfCharsToWrite;

wrap:
	if (lbApply)
	{
		ReSetDisplayParm(hConsoleOutput, FALSE, TRUE);
		lbApply = FALSE;
	}
	return lbRc;
}

BOOL WINAPI OnSetConsoleMode(HANDLE hConsoleHandle, DWORD dwMode)
{
	typedef BOOL (WINAPI* OnSetConsoleMode_t)(HANDLE hConsoleHandle, DWORD dwMode);
	ORIGINALFAST(SetConsoleMode);
	BOOL bMainThread = FALSE; // поток не важен
	BOOL lbRc = FALSE;

	#if 0
	if (!(dwMode & ENABLE_PROCESSED_OUTPUT))
	{
		_ASSERTE((dwMode & ENABLE_PROCESSED_OUTPUT)==ENABLE_PROCESSED_OUTPUT);
	}
	#endif

	lbRc = F(SetConsoleMode)(hConsoleHandle, dwMode);

	return lbRc;
}
