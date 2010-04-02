
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

THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
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


#pragma once

#include "../common/common.hpp"


#define SafeCloseHandle(h) { if ((h)!=NULL) { HANDLE hh = (h); (h) = NULL; if (hh!=INVALID_HANDLE_VALUE) CloseHandle(hh); } }
#ifdef _DEBUG
	#define OUTPUTDEBUGSTRING(m) OutputDebugString(m)
	#define SHOWDBGINFO(x) OutputDebugStringW(x)
	//#include <crtdbg.h>
#else
	#define OUTPUTDEBUGSTRING(m)
	#define SHOWDBGINFO(x)
	//#define _ASSERT(x)
	//#define _ASSERTE(x)
#endif


#define ISALPHA(c) ((((c) >= (BYTE)'c') && ((c) <= (BYTE)'z')) || (((c) >= (BYTE)'C') && ((c) <= (BYTE)'Z')))
#define isPressed(inp) ((GetKeyState(inp) & 0x8000) == 0x8000)

// X - меньша€, Y - больша€
#define FAR_X_VER 995
#define FAR_Y_VER 995
#define FUNC_X(fn) fn##995
#define FUNC_Y(fn) fn##995

//typedef struct tag_FarVersion {
//	union {
//		DWORD dwVer;
//		struct {
//			WORD dwVerMinor;
//			WORD dwVerMajor;
//		};
//	};
//	DWORD dwBuild;
//} FarVersion;

struct CEFAR_FIND_DATA
{
	DWORD    dwFileAttributes;
	//FILETIME ftCreationTime;
	//FILETIME ftLastAccessTime;
	//FILETIME ftLastWriteTime;
	union {
		struct {
			DWORD nFileSizeHigh;
			DWORD nFileSizeLow;
		};
		unsigned __int64 nFileSize;
	};
	const wchar_t *lpwszFileName;
	const wchar_t *lpwszFileNamePart;
	const wchar_t *lpwszFileExt;
	//const wchar_t *lpwszAlternateFileName;
};

struct CePluginPanelItem
{
	struct CEFAR_FIND_DATA FindData;
	DWORD         Flags;
	//DWORD         NumberOfLinks;
	//wchar_t      *Description;
	//wchar_t      *Owner;
	//wchar_t     **CustomColumnData;
	//int           CustomColumnNumber;
	//DWORD_PTR     UserData;
	//DWORD         CRC32;
	//DWORD_PTR     Reserved[2];
};

enum CEPANELINFOFLAGS {
	CEPFLAGS_SHOWHIDDEN         = 0x00000001,
	CEPFLAGS_HIGHLIGHT          = 0x00000002,
	CEPFLAGS_REVERSESORTORDER   = 0x00000004,
	CEPFLAGS_USESORTGROUPS      = 0x00000008,
	CEPFLAGS_SELECTEDFIRST      = 0x00000010,
	CEPFLAGS_REALNAMES          = 0x00000020,
	CEPFLAGS_NUMERICSORT        = 0x00000040,
	CEPFLAGS_PANELLEFT          = 0x00000080,
};

struct CeFullPanelInfo
{
	DWORD cbSize;
	HWND  hView;
	//
	int nXCount; // тут четко, кусок иконки не допускаетс€
	int nYCountFull; // тут четко, кусок иконки не допускаетс€
	int nYCount; // а тут допускаетс€ отображение верхней части иконки
	//
	DWORD nFarInterfaceSettings;
	DWORD nFarPanelSettings;
	BOOL  bLeftPanel, bPlugin;
	RECT  PanelRect;
	int ItemsNumber;
	int CurrentItem;
	int TopPanelItem; // он может Ќ≈ совпадать с фаровским, чтобы CurrentItem был таки видим
	BOOL Visible;
	BOOL Focus;
	DWORD Flags; // CEPANELINFOFLAGS
	BYTE nFarColors[0x100]; // ћассив цветов фара
	wchar_t* pszPanelDir;
	CePluginPanelItem** ppItems;

	void FreeInfo() {
		if (ppItems) {
			for (int i=0; i<ItemsNumber; i++)
				if (ppItems[i]) free(ppItems[i]);
			free(ppItems);
			ppItems = NULL;
		}
		if (pszPanelDir) {
			free(pszPanelDir);
			pszPanelDir = NULL;
		}
	}
};


struct ThumbnailSettings
{
	int nWidth, nHeight; // 96x96
	int nHSpacing, nVSpacing; // 5, 25 - промежуток между двум€ рамками
	int nHPadding, nVPadding; // 1, 1 - зарезевированный отступ
};
extern ThumbnailSettings gThSet;



extern HWND ghConEmuRoot;
extern HMODULE ghPluginModule; // ConEmuTh.dll - сам плагин
extern DWORD gnSelfPID, gnMainThreadId;
//extern int lastModifiedStateW;
//extern bool gbHandleOneRedraw; //, gbHandleOneRedrawCh;
//extern WCHAR gszDir1[CONEMUTABMAX], gszDir2[CONEMUTABMAX], 
extern WCHAR gszRootKey[MAX_PATH*2];
//extern int maxTabCount, lastWindowCount;
//extern CESERVER_REQ* tabs; //(ConEmuTab*) calloc(maxTabCount, sizeof(ConEmuTab));
//extern CESERVER_REQ* gpCmdRet;
//extern HWND ConEmuHwnd;
//extern HWND FarHwnd;
extern FarVersion gFarVersion;
//extern int lastModifiedStateW;
////extern HANDLE hEventCmd[MAXCMDCOUNT];
//extern HANDLE hThread;
////extern WCHAR gcPlugKey;
////WARNING("”брать, заменить ghConIn на GetStdHandle()"); // »наче в Win7 будет буфер разрушатьс€
////extern HANDLE ghConIn;
//extern BOOL gbNeedPostTabSend;
//extern HANDLE ghServerTerminateEvent;
//extern CESERVER_REQ_CONINFO_HDR *gpConsoleInfo;
//extern DWORD gnSelfPID;
extern CeFullPanelInfo pviLeft, pviRight;
extern HANDLE ghDisplayThread; extern DWORD gnDisplayThreadId;
extern HWND ghLeftView, ghRightView;

//typedef struct tag_SynchroArg {
//	enum {
//		eCommand,
//		eInput
//	} SynchroType;
//	HANDLE hEvent;
//	LPARAM Result;
//	LPARAM Param1, Param2;
//	BOOL Obsolete;
//	//BOOL Processed;
//} SynchroArg;

BOOL LoadFarVersion();
void StopThread(void);
void FUNC_X(ExitFARW)(void);
void FUNC_Y(ExitFARW)(void);
void FUNC_X(SetStartupInfoW)(void *aInfo);
void FUNC_Y(SetStartupInfoW)(void *aInfo);
int ShowMessage(int aiMsg, int aiButtons);
int ShowMessageA(int aiMsg, int aiButtons);
int FUNC_X(ShowMessage)(int aiMsg, int aiButtons);
int FUNC_Y(ShowMessage)(int aiMsg, int aiButtons);
void PostMacro(wchar_t* asMacro);
void PostMacroA(char* asMacro);
void FUNC_X(PostMacro)(wchar_t* asMacro);
void FUNC_Y(PostMacro)(wchar_t* asMacro);
LPCWSTR GetMsgW(int aiMsg);
void GetMsgA(int aiMsg, wchar_t* rsMsg/*MAX_PATH*/);
LPCWSTR FUNC_Y(GetMsg)(int aiMsg);
LPCWSTR FUNC_X(GetMsg)(int aiMsg);
void ShowPluginMenu(int nID = -1);
int ShowPluginMenuA();
int FUNC_Y(ShowPluginMenu)();
int FUNC_X(ShowPluginMenu)();
BOOL IsMacroActive();
BOOL IsMacroActiveA();
BOOL FUNC_X(IsMacroActive)();
BOOL FUNC_Y(IsMacroActive)();
CeFullPanelInfo* LoadPanelInfo();
CeFullPanelInfo* LoadPanelInfoA();
CeFullPanelInfo* FUNC_X(LoadPanelInfo)();
CeFullPanelInfo* FUNC_Y(LoadPanelInfo)();

HWND CreateView(CeFullPanelInfo* pi);

// ConEmu.dll
BOOL CheckConEmu(BOOL abForceCheck=FALSE);
int RegisterPanelView(PanelViewInit *ppvi);
HWND GetConEmuHWND();
BOOL WINAPI OnReadConsole(PINPUT_RECORD lpBuffer, LPDWORD lpNumberOfEventsRead);
