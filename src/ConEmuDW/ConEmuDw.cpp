
/*
Copyright (c) 2009-2015 Maximus5
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

#define SHOWDEBUGSTR

#include <windows.h>

#ifdef _DEBUG
//  Раскомментировать, чтобы сразу после запуска процесса (conemuc.exe) показать MessageBox, чтобы прицепиться дебаггером
//  #define SHOW_STARTED_MSGBOX
#else
//
#endif

#ifndef __GNUC__
#include <crtdbg.h>

#pragma comment(lib, "Comdlg32.lib")
#pragma comment(lib, "version.lib")
#endif

#define MASSERT_HEADER_DEFINED
//#define MEMORY_HEADER_DEFINED

#include "../common/defines.h"
#include "../common/pluginW1900.hpp"
#include "../common/ConsoleAnnotation.h"
#include "../common/ConsoleRead.h"
#include "../common/ConEmuColors3.h"
#include "../common/common.hpp"
#include "../common/ConEmuCheck.h"
#include "../common/UnicodeChars.h"
#include "../common/WThreads.h"
#include "../ConEmu/version.h"
#include "../ConEmuCD/ExitCodes.h"
#include "../ConEmuHk/SetHook.h"
#include "ConEmuDw.h"
#include "resource.h"
#ifdef _DEBUG
#include "../common/WObjects.h"
#endif

#define DEBUGSTRCALL(s) //DEBUGSTR(s)

#define MSG_TITLE "ConEmu writer"
#define MSG_INVALID_CONEMU_VER "Unsupported ConEmu version detected!\nRequired version: " CONEMUVERS "\nConsole writer'll works in 4bit mode"
#define MSG_TRUEMOD_DISABLED   "«Colorer TrueMod support» is not checked in the ConEmu settings\nConsole writer'll works in 4bit mode"
#define MSG_NO_TRUEMOD_BUFFER  "TrueMod support not enabled in the ConEmu settings\nConsole writer'll works in 4bit mode"

// "Прозрачность" в фаре хранится в старшем байте,
// используется (пока) только при обработке раскраски файлов
#define IsTransparent(C) (((C) & 0xFF000000) == 0)
#define SetTransparent(T) ((T) ? 0 : 0xFF000000)


#define MAX_READ_BUF 16384


HMODULE ghOurModule = NULL; // ConEmuDw.dll
HWND    ghVConWnd = NULL;   // VirtualCon. инициализируется в CheckBuffers()
HWND    ghRConWnd = NULL;

HMODULE ghPluginModule = NULL;
HookItemPreCallback_t PreWriteCallBack = NULL;

/* extern для MAssert, Здесь НЕ используется */
/* */       HWND ghConEmuWnd = NULL;      /* */
/* extern для MAssert, Здесь НЕ используется */

#ifdef USE_COMMIT_EVENT
bool    gbBatchStarted = false;
HANDLE  ghBatchEvent = NULL;
DWORD   gnBatchRegPID = 0;
#endif

#ifdef USE_COMMIT_EVENT
CESERVER_CONSOLE_MAPPING_HDR SrvMapping = {};
#endif

AnnotationHeader* gpTrueColor = NULL;
HANDLE ghTrueColor = NULL;
HANDLE ghFarCommitUpdateSrv = NULL;

BOOL CheckBuffers(bool abWrite = false);
void CloseBuffers();

bool gbInitialized = false;
bool gbFarBufferMode = false; // true, если Far запущен с ключом "/w"

struct {
	bool WasSet;
	WORD CONColor;
	FarColor FARColor;
	AnnotationInfo CEColor;
} gCurrentAttr;


#include "../common/SetExport.h"
ExportFunc Far3Func[] =
{
	{"ClearExtraRegions", ClearExtraRegions, ClearExtraRegionsOld},
	{NULL}
};

#include "../common/FarVersion.h"

FarVersion gFarVersion = {};
#define FAR_NEW_BUILD 3277

BOOL LoadFarVersion()
{
	wchar_t ErrText[512]; ErrText[0] = 0;
	BOOL lbRc = LoadFarVersion(gFarVersion, ErrText);

	if (!lbRc)
	{
		gFarVersion.dwVerMajor = 3;
		gFarVersion.dwVerMinor = 0;
		gFarVersion.dwBuild = FAR_NEW_BUILD;
	}

	return lbRc;
}

// Dummy external requirement for FileExistsSearch.
// But it's better to splite FileExistsSearch code...
bool SearchAppPaths(wchar_t const *,struct CmdArg &,bool,struct CmdArg *)
{
	return false;
}


#if defined(__GNUC__)
extern "C"
#endif
BOOL WINAPI DllMain(HANDLE hModule, DWORD  ul_reason_for_call, LPVOID lpReserved)
{
	switch (ul_reason_for_call)
	{
		case DLL_PROCESS_ATTACH:
			{
				//HeapInitialize();
				ghOurModule = (HMODULE)hModule;
				ghWorkingModule = (u64)hModule;
				HeapInitialize();
				
				#ifdef SHOW_STARTED_MSGBOX
				char szMsg[128]; wsprintfA(szMsg, "ExtendedConsole, FAR Pid=%u", GetCurrentProcessId());
				if (!IsDebuggerPresent()) MessageBoxA(NULL, "ExtendedConsole*.dll loaded", szMsg, 0);
				#endif

				bool lbExportsChanged = false;
				if (LoadFarVersion())
				{
					if ((gFarVersion.dwVerMajor == 3) && (gFarVersion.dwBuild < FAR_NEW_BUILD))
					{
						lbExportsChanged = ChangeExports( Far3Func, ghOurModule );
						if (!lbExportsChanged)
						{
							_ASSERTE(lbExportsChanged);
						}
					}
				}

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

bool __stdcall SetHookCallbacksExt(const char* ProcName, const wchar_t* DllName, HMODULE hCallbackModule,
                                HookItemPreCallback_t PreCallBack, HookItemPostCallback_t PostCallBack,
                                HookItemExceptCallback_t ExceptCallBack)
{
	if (!ProcName || lstrcmpA(ProcName, "WriteConsoleOutputW") != 0 || !DllName || lstrcmp(DllName, L"kernel32.dll") != 0)
	{
		_ASSERTE(ProcName!=NULL && DllName!=NULL);
		_ASSERTE(lstrcmpA(ProcName, "WriteConsoleOutputW") != 0);
		_ASSERTE(lstrcmp(DllName, L"kernel32.dll") != 0);
		return false;
	}

	ghPluginModule = hCallbackModule;
	PreWriteCallBack = PreCallBack;

	return true;
}

#if defined(CRTSTARTUP)
extern "C"
BOOL WINAPI _DllMainCRTStartup(HANDLE hDll,DWORD dwReason,LPVOID lpReserved)
{
	DllMain(hDll, dwReason, lpReserved);
	return TRUE;
}
#endif


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

int WINAPI RequestLocalServer(/*[IN/OUT]*/RequestLocalServerParm* Parm)
{
	int iRc = CERR_SRVLOADFAILED;
	if (!Parm || (Parm->StructSize != sizeof(*Parm)))
	{
		iRc = CERR_CARGUMENT;
		goto wrap;
	}

	HMODULE hHkDll = NULL;
	RequestLocalServer_t fRequestLocalServer = NULL;
	wchar_t *pszSlash, szFile[MAX_PATH+1] = {};
	LPCWSTR pszSrvName = WIN3264TEST(L"ConEmuHk.dll",L"ConEmuHk64.dll");

	GetModuleFileName(ghOurModule, szFile, MAX_PATH);
	pszSlash = wcsrchr(szFile, L'\\');
	if (!pszSlash)
		goto wrap;
	pszSlash[1] = 0;
	wcscat_c(szFile, pszSrvName);

	hHkDll = GetModuleHandle(szFile);
	if (!hHkDll)
	{
		#ifdef _DEBUG
		// Unless hooks were disabled (with env var for example)
		wchar_t szEnv[64] = L""; GetEnvironmentVariable(ENV_CONEMU_HOOKS_W, szEnv, countof(szEnv));
		CharUpperBuff(szEnv, lstrlen(szEnv));
		_ASSERTE((hHkDll || wcsstr(szEnv,ENV_CONEMU_HOOKS_DISABLED)) && "Must be already injected");
		#endif

		hHkDll = LoadLibrary(szFile);
	}
	if (!hHkDll)
		goto wrap;

	fRequestLocalServer = (RequestLocalServer_t)GetProcAddress(hHkDll, "RequestLocalServer");

	if (!fRequestLocalServer)
	{
		_ASSERTE(fRequestLocalServer && "RequestLocalServer export not found in ConEmuHk");
		goto wrap;
	}

	_ASSERTE(CheckCallbackPtr(hHkDll, 1, (FARPROC*)&fRequestLocalServer, TRUE));

	iRc = fRequestLocalServer(Parm);

wrap:
	return iRc;
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

	// Проверить, не изменился ли HWND окна отрисовки (DC)...
	HWND hCon = GetConEmuHWND(0);
	if (!hCon)
	{
		CloseBuffers();
		SetLastError(E_HANDLE);
		return FALSE;
	}
	if (hCon != ghVConWnd)
	{
		#ifdef _DEBUG
		// After minhook implementation GetConsoleWindow will return VCon HWND
		HWND hApiCon = GetConsoleWindow();
		HWND hRealCon = GetConEmuHWND(2);
		HWND hRootWnd = GetConEmuHWND(1);
		_ASSERTE(hApiCon == hCon && hApiCon != hRealCon);
		#endif

		ghRConWnd = GetConEmuHWND(2);

		ghVConWnd = hCon;
		CloseBuffers();

		
		//TODO: Пока работаем "по-старому", через буфер TrueColor. Переделать, он не оптимален
		RequestLocalServerParm prm = {sizeof(prm), slsf_RequestTrueColor|slsf_GetCursorEvent|slsf_GetFarCommitEvent};
		int iFRc = RequestLocalServer(&prm);
		if (iFRc == 0)
		{
			ghFarCommitUpdateSrv = prm.hFarCommitEvent;
			gpTrueColor = prm.pAnnotation;
		}

		if (!gpTrueColor)
		{
			wchar_t szMapName[128];
			wsprintf(szMapName, AnnotationShareName, (DWORD)sizeof(AnnotationInfo), (DWORD)hCon); //-V205
			// AnnotationShareName is CREATED in ConEmu.exe
			// May be it would be better, to avoid hooking and cycling (minhook),
			// call CreateFileMapping instead of OpenFileMapping...
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

		if (!ghFarCommitUpdateSrv)
		{
			_ASSERTE(ghFarCommitUpdateSrv!=NULL);
		//	RequestLocalServerParm prm = {sizeof(prm)};
		//	prm.Flags = slsf_GetFarCommitEvent;
		//	int iFRc = RequestLocalServer(&prm);
		//	if (iFRc == 0)
		//	{
		//		ghFarCommitUpdateSrv = prm.hFarCommitEvent;
		//	}
		}
		
		#ifdef USE_COMMIT_EVENT
		if (!LoadSrvMapping(ghRConWnd, SrvMapping))
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
								CESERVER_REQ* pOut = ExecuteSrvCmd(gnBatchRegPID, pIn, ghRConWnd);
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
	// All buffers may be managed through the AltServer
	if (ghTrueColor)
	{
		if (gpTrueColor)
			UnmapViewOfFile(gpTrueColor);
		CloseHandle(ghTrueColor);
		ghTrueColor = NULL;
	}
	gpTrueColor = NULL;

	if (ghFarCommitUpdateSrv)
	{
		CloseHandle(ghFarCommitUpdateSrv);
		ghFarCommitUpdateSrv = NULL;
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
	#ifdef _DEBUG
	wchar_t szCall[100]; _wsprintf(szCall, SKIPCOUNT(szCall) L"ExtCon::GetTextAttributes\n");
	DEBUGSTRCALL(szCall);
	#endif

	BOOL lbTrueColor = CheckBuffers();
	UNREFERENCED_PARAMETER(lbTrueColor);
	
	CONSOLE_SCREEN_BUFFER_INFO csbi = {};
	SMALL_RECT srWork = {};
	HANDLE h;
	if (!GetBufferInfo(h, csbi, srWork))
	{
		gCurrentAttr.WasSet = false;
		_ASSERTE(FALSE && "GetBufferInfo failed");
		return FALSE;
	}

	// Что-то в некоторых случаях сбивается цвет вывода для printf
	//_ASSERTE("GetTextAttributes" && ((csbi.wAttributes & 0xFF) == 0x07));

	TODO("По хорошему, gCurrentAttr нада ветвить по разным h");
	if (gCurrentAttr.WasSet && (csbi.wAttributes == gCurrentAttr.CONColor))
	{
		*Attributes = gCurrentAttr.FARColor;
	}
	else
	{
		gCurrentAttr.WasSet = false;
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

WORD Far2ConEmuColor(const FarColor* Attributes, AnnotationInfo& t)
{
	ZeroStruct(t);
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

	return n;
}

//TODO: Юзкейс понят неправильно.
//TODO: Это не "всего видимого экрана", это "цвет по умолчанию". То, что соответствует 
//TODO: "Screen Text" и "Screen Background" в свойствах консоли. То, что задаётся командой
//TODO: color в cmd.exe. То, что будет использовано если в консоль просто писать 
//TODO: по printf/std::cout/WriteConsole, без явного указания цвета.
BOOL WINAPI SetTextAttributes(const FarColor* Attributes)
{
	#ifdef _DEBUG
	wchar_t szCall[100]; _wsprintf(szCall, SKIPCOUNT(szCall) L"ExtCon::SetTextAttributes\n");
	DEBUGSTRCALL(szCall);
	#endif

	if (!Attributes)
	{
		// ConEmu internals: сбросить запомненный "атрибут"
		gCurrentAttr.WasSet = false;
		return TRUE;
	}

	BOOL lbTrueColor = CheckBuffers();
	UNREFERENCED_PARAMETER(lbTrueColor);

	CONSOLE_SCREEN_BUFFER_INFO csbi = {};
	SMALL_RECT srWork = {};
	HANDLE h;
	if (!GetBufferInfo(h, csbi, srWork))
		return FALSE;
	
	//WORD nDefWriteBuf[1024];
	//WORD *pnWriteAttr = NULL;
	//int nBufWidth  = srWork.Right - srWork.Left + 1;
	//int nBufHeight = srWork.Bottom - srWork.Top + 1;
	//int nBufCount  = nBufWidth*nBufHeight;
	//if (nBufWidth <= (int)countof(nDefWriteBuf))
	//	pnWriteAttr = nDefWriteBuf;
	//else
	//	pnWriteAttr = (WORD*)calloc(nBufCount,sizeof(*pnWriteAttr));
	//if (!pnWriteAttr)
	//{
	//	SetLastError(E_OUTOFMEMORY);
	//	return FALSE;
	//}
	
	BOOL lbRc = TRUE;
	//COORD cr = {srWork.Left, srWork.Top};
	//DWORD nWritten;
	
	#ifdef _DEBUG
	AnnotationInfo* pTrueColor = (AnnotationInfo*)(gpTrueColor ? (((LPBYTE)gpTrueColor) + gpTrueColor->struct_size) : NULL);
	AnnotationInfo* pTrueColorEnd = pTrueColor ? (pTrueColor + gpTrueColor->bufferSize) : NULL;
	#endif

	// <G|S>etTextAttributes должны ожидать указатель на ОДИН FarColor.
	AnnotationInfo t = {};
	WORD n = Far2ConEmuColor(Attributes, t);

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
	
	TODO("По хорошему, gCurrentAttr нада ветвить по разным h");
	// запомнить, что WriteConsole должен писать атрибутом "t"
	gCurrentAttr.WasSet = true;
	gCurrentAttr.CONColor = n;
	gCurrentAttr.FARColor = *Attributes;
	gCurrentAttr.CEColor = t;
	
	//if (pnWriteAttr != nDefWriteBuf)
	//	free(pnWriteAttr);
	
	return lbRc;
}

enum CLEAR_REGION
{
	CR_TOP=0x1,
	CR_RIGHT=0x2,
	CR_BOTH=CR_TOP|CR_RIGHT,
};

BOOL WINAPI ClearExtraRegions(const FarColor* Color, int Mode)
{
	#ifdef _DEBUG
	wchar_t szCall[100]; _wsprintf(szCall, SKIPCOUNT(szCall) L"ExtCon::ClearExtraRegions\n");
	DEBUGSTRCALL(szCall);
	#endif

	CONSOLE_SCREEN_BUFFER_INFO csbi = {};
	SMALL_RECT srWork = {};
	HANDLE h;
	if (!CheckBuffers() || !GetBufferInfo(h, csbi, srWork))
		return FALSE;

	AnnotationInfo t = {};
	WORD ConColor = Far2ConEmuColor(Color, t);

	if (Mode & CR_TOP)
	{
		DWORD CharsWritten, TopSize = csbi.dwSize.X*csbi.srWindow.Top;
		COORD TopCoord = {};
		FillConsoleOutputCharacter(h, L' ', TopSize, TopCoord, &CharsWritten);
		FillConsoleOutputAttribute(h, ConColor, TopSize, TopCoord, &CharsWritten );
	}

	if (Mode & CR_RIGHT)
	{
		DWORD CharsWritten, RightSize = csbi.dwSize.X-csbi.srWindow.Right;
		COORD RightCoord = {csbi.srWindow.Right,csbi.dwSize.Y-(csbi.srWindow.Bottom-csbi.srWindow.Top+1)};
		for (; RightCoord.Y<csbi.dwSize.Y; RightCoord.Y++)
		{
			FillConsoleOutputCharacter(h, L' ', RightSize, RightCoord, &CharsWritten);
			FillConsoleOutputAttribute(h, ConColor, RightSize, RightCoord, &CharsWritten);
		}
	}

	//TODO: Пока работаем через старый Annotation buffer (переделать нужно)
	//TODO: который по определению соответствует видимой области экрана
	//return SetTextAttributes(Color);
	return TRUE;
}

BOOL WINAPI ClearExtraRegionsOld(const FarColor* Color)
{
	#ifdef _DEBUG
	wchar_t szCall[100]; _wsprintf(szCall, SKIPCOUNT(szCall) L"ExtCon::ClearExtraRegionsOld\n");
	DEBUGSTRCALL(szCall);
	#endif

	return ClearExtraRegions(Color, CR_BOTH);
}

BOOL WINAPI ReadOutput(FAR_CHAR_INFO* Buffer, COORD BufferSize, COORD BufferCoord, SMALL_RECT* ReadRegion)
{
	#ifdef _DEBUG
	wchar_t szCall[100]; _wsprintf(szCall, SKIPCOUNT(szCall) L"ExtCon::ReadOutput({%i,%i}-{%i,%i})\n", ReadRegion->Left, ReadRegion->Top, ReadRegion->Right, ReadRegion->Bottom);
	DEBUGSTRCALL(szCall);
	#endif

	BOOL lbTrueColor = CheckBuffers();
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
	BOOL WINAPI ReadConsoleOutput(
	  __in     HANDLE hConsoleOutput,
	  __out    PCHAR_INFO lpBuffer,
	  __in     COORD dwBufferSize,
	  __in     COORD dwBufferCoord,
	  __inout  PSMALL_RECT lpReadRegion
	);
	*/

	CONSOLE_SCREEN_BUFFER_INFO csbi = {};
	SMALL_RECT srWork = {};
	HANDLE h;
	if (!GetBufferInfo(h, csbi, srWork))
		return FALSE;

	CHAR_INFO cDefReadBuf[256];
	CHAR_INFO *pcReadBuf = NULL;
	int nWindowWidth  = srWork.Right - srWork.Left + 1;
	if (BufferSize.X <= (int)countof(cDefReadBuf))
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
	AnnotationInfo* pTrueColorLine = (AnnotationInfo*)(pTrueColorStart ? (pTrueColorStart + nWindowWidth * (ReadRegion->Top /*- srWork.Top*/)) : NULL);

	FAR_CHAR_INFO* pFarEnd = Buffer + BufferSize.X*BufferSize.Y; //-V104

	SMALL_RECT rcRead = *ReadRegion;
	COORD MyBufferSize = {BufferSize.X, 1};
	//COORD MyBufferCoord = {BufferCoord.X, 0};
	SHORT YShift = gbFarBufferMode ? (csbi.dwSize.Y - (srWork.Bottom - srWork.Top + 1)) : 0;
	SHORT Y1 = ReadRegion->Top + YShift;
	SHORT Y2 = ReadRegion->Bottom + YShift;
	SHORT BufferShift = ReadRegion->Top + YShift;

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
		BOOL lbRead = ReadConsoleOutputEx(h, pcReadBuf, MyBufferSize, /*MyBufferCoord,*/ rcRead);

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
	#ifdef _DEBUG
	wchar_t szCall[100]; _wsprintf(szCall, SKIPCOUNT(szCall) L"ExtCon::WriteOutput({%i,%i}-{%i,%i})\n", WriteRegion->Left, WriteRegion->Top, WriteRegion->Right, WriteRegion->Bottom);
	DEBUGSTRCALL(szCall);
	#endif

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

	CHAR_INFO cDefWriteBuf[256];
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
		
		if (PreWriteCallBack)
		{
			bool bMainThread = true;
			SETARGS5(&lbRc, h, pcWriteBuf, &MyBufferSize, &MyBufferCoord, &rcWrite);
			PreWriteCallBack(&args);
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
	#ifdef _DEBUG
	wchar_t szCall[100]; _wsprintf(szCall, SKIPCOUNT(szCall) L"ExtCon::WriteText(chars=%u)\n", nNumberOfCharsToWrite);
	DEBUGSTRCALL(szCall);
	#endif

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

	TODO("Обработка символов \\t\\n\\r?");

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
	#ifdef _DEBUG
	wchar_t szCall[100]; _wsprintf(szCall, SKIPCOUNT(szCall) L"ExtCon::Commit\n");
	DEBUGSTRCALL(szCall);
	#endif

	// Если буфер не был создан - то и передергивать нечего
	if (gpTrueColor != NULL)
	{
		if (gpTrueColor->locked)
		{
			gpTrueColor->flushCounter++;
			gpTrueColor->locked = FALSE;
		}
	}

	if (ghFarCommitUpdateSrv)
	{
		// Разрешить передернуть сервер
		SetEvent(ghFarCommitUpdateSrv);
	}
	
	#ifdef USE_COMMIT_EVENT
	// "Отпустить" сервер
	if (ghBatchEvent)
		SetEvent(ghBatchEvent);
	gbBatchStarted = false;
	#endif
	
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
COLORREF gcrPalette[16] = {
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
					*cr = Far3Color::GetStdColor(nClr, gcrPalette);
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
					*cr = Far3Color::GetStdColor(nClr, gcrPalette);
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
			//for (int i = 0; i < countof(crCustom); i++)
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
				Far3Color::Color2FgIndex(cr, nIndex, gcrPalette);
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


// gcrCustomColors
void ReloadGuiPalette()
{
	CESERVER_REQ* pIn = ExecuteNewCmd(CECMD_QUERYPALETTE, sizeof(CESERVER_REQ_HDR));
	if (pIn)
	{
		CESERVER_REQ* pOut = ExecuteGuiCmd(ghRConWnd, pIn, ghVConWnd);
		if (pOut->DataSize() >= sizeof(CESERVER_PALETTE))
		{
			memmove(gcrPalette, pOut->Palette.crPalette, sizeof(gcrPalette));
			memmove(gcrCustomColors, pOut->Palette.crPalette, sizeof(gcrCustomColors));
		}
		ExecuteFreeResult(pOut);
		ExecuteFreeResult(pIn);
	}
}


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
					Far3Color::Color2FgIndex(P->crForeColor, nIndex, gcrPalette);
					if (/*nIndex >= 0 &&*/ nIndex < 16)
					{
						P->crForeColor = Far3Color::GetStdColor(nIndex, gcrPalette);
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
					Far3Color::Color2FgIndex(P->crBackColor, nIndex, gcrPalette);
					if (/*nIndex >= 0 &&*/ nIndex < 16)
					{
						P->crBackColor = Far3Color::GetStdColor(nIndex, gcrPalette);
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
	#ifdef _DEBUG
	wchar_t szCall[100]; _wsprintf(szCall, SKIPCOUNT(szCall) L"ExtCon::GetColorDialog\n");
	DEBUGSTRCALL(szCall);
	#endif

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
	ReloadGuiPalette();
	Parm.Far2Ref(&Parm.Color, TRUE, &Parm.crForeColor, &Parm.bForeTransparent);
	Parm.Far2Ref(&Parm.Color, FALSE, &Parm.crBackColor, &Parm.bBackTransparent);
	if (!AddTransparent)
	{
		Parm.bForeTransparent = Parm.bBackTransparent = FALSE;
	}
	
	// Заменить на дескриптор окна GUI
	Parm.hConsole = GetConEmuHWND(1);
	
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
			if (pHdr->cbSize == 2164 && pHdr->nProtocolVersion == 67 && IsWindow(pHdr->hConEmuWndDc))
			{
				Parm.hGUI = pHdr->hConEmuWndDc; // DC
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
				|| !IsWindow(pHdr->hConEmuWndDc))
			{
				if ((pHdr->cbSize >= sizeof(*pHdr)) 
					&& pHdr->hConEmuWndDc && IsWindow(pHdr->hConEmuWndDc)
					&& IsWindowVisible(pHdr->hConEmuWndDc))
				{
					// Это все-таки может быть окно ConEmu
					Parm.hGUI = pHdr->hConEmuWndDc; // DC
					if (IsWindow(pHdr->hConEmuRoot) && IsWindowVisible(pHdr->hConEmuRoot))
						Parm.hGUIRoot = pHdr->hConEmuRoot; // Main window
				}
				MessageBoxA(NULL, MSG_INVALID_CONEMU_VER, MSG_TITLE, MB_ICONSTOP|MB_SYSTEMMODAL);
			}
			else
			{
				Parm.hGUI = pHdr->hConEmuWndDc; // DC
				Parm.hGUIRoot = pHdr->hConEmuRoot; // Main window
				if (!(pHdr->Flags & CECF_UseTrueColor))
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
	
	int nRc = 0;
	LPCWSTR pszTitle = L"ConEmu Colors";
	LPCWSTR pszText = L"Executing GUI dialog";
	
	FAR_CHAR_INFO* SaveBuffer = NULL;
	FAR_CHAR_INFO* WriteBuffer = NULL;
	SMALL_RECT Region = {0, 0, 0, 0};
	COORD BufSize = {0,0};
	COORD BufCoord = {0,0};
	CONSOLE_SCREEN_BUFFER_INFO csbi = {};
	SMALL_RECT srWork = {};
	HANDLE h;
	if (GetBufferInfo(h, csbi, srWork))
	{
		Parm.rcBuffer = srWork;

		int nWidth = Far3Color::Max(lstrlen(pszText),lstrlen(pszTitle))+10;
		int nHeight = 6;
		int nX = (srWork.Right - srWork.Left - nWidth) >> 1;
		int nY = (srWork.Bottom - srWork.Top - nHeight) >> 1;
		Region.Left = nX; Region.Top = nY; Region.Right = nX+nWidth-1; Region.Bottom = nY+nHeight-1;
		BufSize.X = nWidth; BufSize.Y = nHeight;

		SaveBuffer = (FAR_CHAR_INFO*)calloc(nWidth*nHeight, sizeof(*SaveBuffer)); //-V106
		WriteBuffer = (FAR_CHAR_INFO*)calloc(nWidth*nHeight, sizeof(*WriteBuffer)); //-V106
		
		if (!SaveBuffer || !WriteBuffer)
		{
			if (SaveBuffer)
			{
				free(SaveBuffer);
				SaveBuffer = NULL;
			}
		}
		else
		{
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
		}
	}

	// Запускаем нить, чтобы 1) не драться с консолью; 2) не иметь проблем с обработчиком сообщений и дескрипторами GDI
	DWORD dwThreadID = 0;
	HANDLE hThread = apiCreateThread(ColorDialogThread, &Parm, &dwThreadID, "ColorDialogThread");
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

	if (SaveBuffer)
	{
		WriteOutput(SaveBuffer, BufSize, BufCoord, &Region);

		free(SaveBuffer);
	}
	if (WriteBuffer)
	{
		free(WriteBuffer);
	}

	return (nRc == 1);
}
