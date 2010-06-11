
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


#ifdef _DEBUG
//  Раскомментировать, чтобы сразу после загрузки плагина показать MessageBox, чтобы прицепиться дебаггером
//  #define SHOW_STARTED_MSGBOX
//  #define SHOW_WRITING_RECTS
#endif

#define TRUE_COLORER_OLD_SUPPORT

#define SHOWDEBUGSTR
#define MCHKHEAP
#define DEBUGSTRMENU(s) DEBUGSTR(s)


//#include <stdio.h>
#include <windows.h>
//#include <windowsx.h>
//#include <string.h>
//#include <tchar.h>
#include "../common/common.hpp"
#include "../common/pluginW1007.hpp" // Отличается от 995 наличием SynchoApi
#include "../common/RgnDetect.h"
#include "ConEmuTh.h"
#include "ImgCache.h"

#define Free free
#define Alloc calloc

#define MAKEFARVERSION(major,minor,build) ( ((major)<<8) | (minor) | ((build)<<16))

#define ConEmuTh_SysID 0x43455568 // 'CETh'

#ifdef _DEBUG
wchar_t gszDbgModLabel[6] = {0};
#endif

#if defined(__GNUC__)
extern "C"{
  BOOL WINAPI DllMain( HANDLE hModule, DWORD  ul_reason_for_call, LPVOID lpReserved );
  void WINAPI SetStartupInfoW(void *aInfo);
};
#endif

PanelViewSettings gThSet = {0}; // параметры получаются из мэппинга при открытии плага или при перерегистрации

HWND ghConEmuRoot = NULL;
HMODULE ghPluginModule = NULL; // ConEmuTh.dll - сам плагин
BOOL TerminalMode = FALSE;
DWORD gnSelfPID = 0;
DWORD gnMainThreadId = 0;
HANDLE ghDisplayThread = NULL; DWORD gnDisplayThreadId = 0;
HWND ghLeftView = NULL, ghRightView = NULL;
wchar_t* gszRootKey = NULL;
FarVersion gFarVersion;
//HMODULE ghConEmuDll = NULL;
RegisterPanelView_t gfRegisterPanelView = NULL;
GetFarHWND2_t gfGetFarHWND2 = NULL;
CeFullPanelInfo pviLeft = {0}, pviRight = {0};
int ShowLastError();
CRgnDetect *gpRgnDetect = NULL;
CImgCache  *gpImgCache = NULL;
CEFAR_INFO gFarInfo = {0};
COLORREF /*gcrActiveColors[16], gcrFadeColors[16],*/ *gcrCurColors = gThSet.crPalette;
bool gbFadeColors = false;
//bool gbLastCheckWindow = false;
DWORD gnRgnDetectFlags = 0;
void CheckVarsInitialized();
SECURITY_ATTRIBUTES* gpNullSecurity = NULL;
// *** lng resources begin ***
wchar_t gsFolder[64], gsHardLink[64], gsSymLink[64], gsJunction[64], gsTitleThumbs[64], gsTitleTiles[64];
// *** lng resources end ***
DWORD gnFarPanelSettings = 0, gnFarInterfaceSettings = 0;

bool gbWaitForKeySequenceEnd = false;
DWORD gnWaitForKeySeqTick = 0;
int gnUngetCount = 0;
INPUT_RECORD girUnget[100];
WORD wScanCodeUp=0, wScanCodeDown=0; //p->Event.KeyEvent.wVirtualScanCode = MapVirtualKey(vk, 0/*MAPVK_VK_TO_VSC*/);
#ifdef _DEBUG
WORD wScanCodeLeft=0, wScanCodeRight=0, wScanCodePgUp=0, wScanCodePgDn=0;
#endif
BOOL GetBufferInput(BOOL abRemove, PINPUT_RECORD lpBuffer, DWORD nBufSize, LPDWORD lpNumberOfEventsRead);
BOOL UngetBufferInput(WORD nCount, PINPUT_RECORD lpOneInput);
void ResetUngetBuffer();
BOOL ProcessConsoleInput(BOOL abUseUngetBuffer, PINPUT_RECORD lpBuffer, DWORD nBufSize, LPDWORD lpNumberOfEventsRead);
DWORD gnConsoleChanges = 0; // bitmask: 1-left, 2-right
void OnMainThreadActived();
void ReloadResourcesW();
ConEmuThSynchroArg* gpLastSynchroArg = NULL;

#ifdef _DEBUG
MFileMapping<DetectedDialogs> *gpDbgDlg = NULL;
#endif


// minimal(?) FAR version 2.0 alpha build FAR_X_VER
int WINAPI _export GetMinFarVersionW(void)
{
	// ACTL_SYNCHRO required
	return MAKEFARVERSION(2,0,max(1007,FAR_X_VER));
}

/* COMMON - пока структуры не различаются */
void WINAPI _export GetPluginInfoW(struct PluginInfo *pi)
{
    static WCHAR *szMenu[1], szMenu1[255];
    szMenu[0]=szMenu1;
	lstrcpynW(szMenu1, GetMsgW(CEPluginName), 240);


	pi->StructSize = sizeof(struct PluginInfo);
	pi->Flags = PF_PRELOAD;
	pi->DiskMenuStrings = NULL;
	pi->DiskMenuNumbers = 0;
	pi->PluginMenuStrings = szMenu;
	pi->PluginMenuStringsNumber = 1;
	pi->PluginConfigStrings = NULL;
	pi->PluginConfigStringsNumber = 0;
	pi->CommandPrefix = 0;
	pi->Reserved = ConEmuTh_SysID; // 'CETh'
}


BOOL gbInfoW_OK = FALSE;
HANDLE WINAPI _export OpenPluginW(int OpenFrom,INT_PTR Item)
{
	if (!gbInfoW_OK || !CheckConEmu())
		return INVALID_HANDLE_VALUE;

	if (ghDisplayThread && gnDisplayThreadId == 0) {
		CloseHandle(ghDisplayThread); ghDisplayThread = NULL;
	}

	ReloadResourcesW();

	//gThSet.Load();
	// При открытии плагина - загрузить информацию об обеих панелях. Нужно для определения регионов!
	ReloadPanelsInfo();

	// Получить активную
	CeFullPanelInfo* pi = GetActivePanel();
	if (!pi) {
		return INVALID_HANDLE_VALUE;
	}
	pi->OurTopPanelItem = pi->TopPanelItem;
	HWND hView = (pi->bLeftPanel) ? ghLeftView : ghRightView;

	PanelViewMode PVM = pvm_None;
	if ((OpenFrom & OPEN_FROMMACRO) == OPEN_FROMMACRO) {
		if (Item == pvm_Thumbnails || Item == pvm_Tiles)
			PVM = (PanelViewMode)Item;
	}

	if (PVM == pvm_None) {
		switch (ShowPluginMenu()) {
			case 0:
				PVM = pvm_Thumbnails;
				break;
			case 1:
				PVM = pvm_Tiles;
				break;
			default:
				return INVALID_HANDLE_VALUE;
		}
	}

	
	BOOL lbRc = FALSE;
	DWORD dwErr = 0;
	DWORD dwMode = pvm_None; //PanelViewMode

	// Если View не создан, или смена режима
	if ((hView == NULL) || (PVM != pi->PVM)) {

		// Для корректного определения положения колонок необходим один из флажков в настройке панели:
		// [x] Показывать заголовки колонок [x] Показывать суммарную информацию
		if (!CheckPanelSettings(FALSE)) {
			return INVALID_HANDLE_VALUE;
		}

		pi->PVM = PVM;
		pi->DisplayReloadPanel();
		if (hView == NULL) {
			// Нужно создать View
			hView = pi->CreateView();
		}
		if (hView == NULL) {
			// Показать ошибку
			ShowLastError();
		} else {
			// Зарегистрироваться
			_ASSERTE(pi->PVM==PVM);
			pi->RegisterPanelView();
			_ASSERTE(pi->PVM==PVM);
		}
		if (pi->hView)
			dwMode = pi->PVM;
	} else {
		// Отрегистрироваться
		pi->UnregisterPanelView();
		dwMode = pvm_None;
	}

	HKEY hk = NULL;
	if (!RegCreateKeyExW(HKEY_CURRENT_USER, gszRootKey, 0, NULL, 0, KEY_WRITE, NULL, &hk, NULL)) {
		RegSetValueEx(hk, pi->bLeftPanel ? L"LeftPanelView" : L"RightPanelView", 0,
			REG_DWORD, (LPBYTE)&dwMode, sizeof(dwMode));
		RegCloseKey(hk);
	}

	return INVALID_HANDLE_VALUE;
}



BOOL WINAPI DllMain( HANDLE hModule, DWORD  ul_reason_for_call, LPVOID lpReserved )
{
	switch (ul_reason_for_call) {
		case DLL_PROCESS_ATTACH:
			{
				ghPluginModule = (HMODULE)hModule;
				gnSelfPID = GetCurrentProcessId();
				gnMainThreadId = GetCurrentThreadId();

				gpNullSecurity = NullSecurity();
				
				_ASSERTE(FAR_X_VER<=FAR_Y_VER);
				#ifdef SHOW_STARTED_MSGBOX
				if (!IsDebuggerPresent()) MessageBoxA(NULL, "ConEmuTh*.dll loaded", "ConEmu Thumbnails", 0);
				#endif
				
			    // Check Terminal mode
			    TCHAR szVarValue[MAX_PATH];
			    szVarValue[0] = 0;
			    if (GetEnvironmentVariable(L"TERM", szVarValue, 63)) {
				    TerminalMode = TRUE;
			    }
			}
			break;
		case DLL_PROCESS_DETACH:
			CommonShutdown();
			break;
	}
	return TRUE;
}

#if defined(CRTSTARTUP)
extern "C"{
  BOOL WINAPI _DllMainCRTStartup(HANDLE hDll,DWORD dwReason,LPVOID lpReserved);
};

BOOL WINAPI _DllMainCRTStartup(HANDLE hDll,DWORD dwReason,LPVOID lpReserved)
{
  DllMain(hDll, dwReason, lpReserved);
  return TRUE;
}
#endif




BOOL LoadFarVersion()
{
	BOOL lbRc=FALSE;
	WCHAR FarPath[MAX_PATH+1];
	if (GetModuleFileName(0,FarPath,MAX_PATH)) {
		DWORD dwRsrvd = 0;
		DWORD dwSize = GetFileVersionInfoSize(FarPath, &dwRsrvd);
		if (dwSize>0) {
			void *pVerData = Alloc(dwSize, 1);
			if (pVerData) {
				VS_FIXEDFILEINFO *lvs = NULL;
				UINT nLen = sizeof(lvs);
				if (GetFileVersionInfo(FarPath, 0, dwSize, pVerData)) {
					TCHAR szSlash[3]; lstrcpyW(szSlash, L"\\");
					if (VerQueryValue ((void*)pVerData, szSlash, (void**)&lvs, &nLen)) {
						gFarVersion.dwVer = lvs->dwFileVersionMS;
						gFarVersion.dwBuild = lvs->dwFileVersionLS;
						lbRc = TRUE;
					}
				}
				Free(pVerData);
			}
		}
	}

	if (!lbRc) {
		gFarVersion.dwVerMajor = 2;
		gFarVersion.dwVerMinor = 0;
		gFarVersion.dwBuild = FAR_X_VER;
	}

	return lbRc;
}

#define CONEMUCHECKDELTA 2000
//static DWORD nLastCheckConEmu = 0;
BOOL CheckConEmu(BOOL abSilence/*=FALSE*/)
{
	// Функция проверяет доступность плагина conemu.dll и GUI
	// делать это нужно при каждом вызове, т.к. плагин могли и выгрузить...

	//// Страховка от того, что conemu.dll могли выгрузить (unload:...)
	//if (!abForceCheck) {
	//	if ((GetTickCount() - nLastCheckConEmu) > CONEMUCHECKDELTA) {
	//		if (!ghConEmuRoot)
	//			return FALSE;
	//		return TRUE;
	//	}
	//}

	if (TerminalMode) {
		if (!abSilence)
			ShowMessage(CEUnavailableInTerminal, 0);
		return FALSE;
	}

	//nLastCheckConEmu = GetTickCount();
	HMODULE hConEmu = GetModuleHandle
		(
			#ifdef WIN64
					L"ConEmu.x64.dll"
			#else
					L"ConEmu.dll"
			#endif
		);
	if (!hConEmu) {
		gfRegisterPanelView = NULL;
		gfGetFarHWND2 = NULL;
		if (!abSilence)
			ShowMessage(CEPluginNotFound, 0);
		return FALSE;
	}

	gfRegisterPanelView = (RegisterPanelView_t)GetProcAddress(hConEmu, "RegisterPanelView");
	gfGetFarHWND2 = (GetFarHWND2_t)GetProcAddress(hConEmu, "GetFarHWND2");
	if (!gfRegisterPanelView || !gfGetFarHWND2) {
		if (!abSilence)
			ShowMessage(CEOldPluginVersion, 0);
		return FALSE;
	}
	HWND hWnd = gfGetFarHWND2(TRUE);
	HWND hRoot = hWnd ? GetParent(hWnd) : NULL;
	if (hRoot != ghConEmuRoot) {
		ghConEmuRoot = hRoot;
		if (hRoot) {
			//MFileMapping<PanelViewSettings> ThSetMap;
			//DWORD nGuiPID;
			//GetWindowThreadProcessId(ghConEmuRoot, &nGuiPID);
			//_ASSERTE(nGuiPID!=0);
			//ThSetMap.InitName(CECONVIEWSETNAME, nGuiPID);
			//if (!ThSetMap.Open()) {
			//	MessageBox(NULL, ThSetMap.GetErrorText(), L"ConEmuTh", MB_ICONSTOP|MB_SETFOREGROUND|MB_SYSTEMMODAL);
			//} else {
			//	ThSetMap.GetTo(&gThSet);
			//	ThSetMap.CloseMap();
			//}
			LoadThSet();
		}
	}

	if (!hRoot) {
		if (!abSilence)
			ShowMessage(CEFarNonGuiMode, 0);
		return FALSE;
	}
	return TRUE;
}


//HWND GetConEmuHWND()
//{
//	// Страховка от того, что conemu.dll могли выгрузить (unload:...)
//	if (!CheckConEmu() || !gfGetFarHWND2)
//		return NULL;
//
//	HWND hRoot = NULL;
//	HWND hConEmu = gfGetFarHWND2(TRUE);
//	if (hConEmu)
//		hRoot = GetParent(hConEmu);
//	
//	return hRoot;
//}











void ReloadResourcesW()
{
	lstrcpynW(gsFolder, GetMsgW(CEDirFolder), countof(gsFolder));
	//lstrcpynW(gsHardLink, GetMsgW(CEDirHardLink), countof(gsHardLink));
	lstrcpynW(gsSymLink, GetMsgW(CEDirSymLink), countof(gsSymLink));
	lstrcpynW(gsJunction, GetMsgW(CEDirJunction), countof(gsJunction));
	lstrcpynW(gsTitleThumbs, GetMsgW(CEColTitleThumbnails), countof(gsTitleThumbs));
	lstrcpynW(gsTitleTiles, GetMsgW(CEColTitleTiles), countof(gsTitleTiles));
}

void WINAPI _export SetStartupInfoW(void *aInfo)
{
	if (!gFarVersion.dwVerMajor) LoadFarVersion();

	if (gFarVersion.dwBuild>=FAR_Y_VER)
		FUNC_Y(SetStartupInfoW)(aInfo);
	else
		FUNC_X(SetStartupInfoW)(aInfo);

	_ASSERTE(gszRootKey!=NULL && *gszRootKey!=0);

	ReloadResourcesW();

	gbInfoW_OK = TRUE;

	StartPlugin(FALSE);
}







	#ifndef max
	#define max(a,b)            (((a) > (b)) ? (a) : (b))
	#endif

	#ifndef min
	#define min(a,b)            (((a) < (b)) ? (a) : (b))
	#endif






void StartPlugin(BOOL abManual)
{
	if (!CheckConEmu(!abManual))
		return;

	// Это делаем всегда, потому как это общая точка входа в плагин
	if (!gpImgCache) {
		gpImgCache = new CImgCache(ghPluginModule);
	}

	HKEY hk = NULL;
	if (!RegOpenKeyExW(HKEY_CURRENT_USER, gszRootKey, 0, KEY_READ, &hk)) {
		DWORD dwModes[2], dwSize = 4;
		if (RegQueryValueEx(hk, L"LeftPanelView", NULL, NULL, (LPBYTE)dwModes, &(dwSize=4)))
			dwModes[0] = 0;
		if (RegQueryValueEx(hk, L"RightPanelView", NULL, NULL, (LPBYTE)(dwModes+1), &(dwSize=4)))
			dwModes[1] = 0;
		RegCloseKey(hk);


		if (ghDisplayThread && gnDisplayThreadId == 0) {
			CloseHandle(ghDisplayThread); ghDisplayThread = NULL;
		}

		if (dwModes[0] || dwModes[1]) {
			if (gThSet.bRestoreOnStartup)
			{
				// Для корректного определения положения колонок необходим один из флажков в настройке панели:
				// [x] Показывать заголовки колонок [x] Показывать суммарную информацию
				if (!CheckPanelSettings(!abManual)) {
					return;
				}

				// При открытии плагина - загрузить информацию об обеих панелях. Нужно для определения регионов!
				ReloadPanelsInfo();

				CeFullPanelInfo* pi[2] = {&pviLeft, &pviRight};
				for (int i = 0; i < 2; i++)
				{
					if (!pi[i]->hView && dwModes[i]) {
						pi[i]->PVM = (PanelViewMode)dwModes[i];
						pi[i]->DisplayReloadPanel();
						if (pi[i]->hView == NULL) {
							// Нужно создать View
							pi[i]->hView = pi[i]->CreateView();
						}
						if (pi[i]->hView == NULL) {
							// Показать ошибку
							ShowLastError();
						} else {
							pi[i]->RegisterPanelView();
						}
					}
				}
			}
		}
	}
}

void ExitPlugin(void)
{
	if (ghLeftView)
		pviLeft.UnregisterPanelView();
		//PostMessage(ghLeftView, WM_CLOSE,0,0);
	if (ghRightView)
		pviRight.UnregisterPanelView();
		//PostMessage(ghRightView, WM_CLOSE,0,0);

	DWORD dwWait = 0;
	if (ghDisplayThread)
		dwWait = WaitForSingleObject(ghDisplayThread, 1000);
	if (dwWait)
		TerminateThread(ghDisplayThread, 100);
	if (ghDisplayThread) {
		CloseHandle(ghDisplayThread); ghDisplayThread = NULL;
	}
	gnDisplayThreadId = 0;
	// Освободить память
	if (gpRgnDetect) {
		delete gpRgnDetect;
		gpRgnDetect = NULL;
	}
	if (gpImgCache) {
		delete gpImgCache;
		gpImgCache = NULL;
	}
	// Сброс переменных, окон, и т.п.
	pviLeft.FinalRelease();
	pviRight.FinalRelease();

	if (gpLastSynchroArg) {
		LocalFree(gpLastSynchroArg); gpLastSynchroArg = NULL;
	}

	if (gszRootKey) {
		free(gszRootKey); gszRootKey = NULL;
	}

#ifdef _DEBUG
	if (gpDbgDlg) {
		delete gpDbgDlg; gpDbgDlg = NULL;
	}
#endif
}

void   WINAPI _export ExitFARW(void)
{
	ExitPlugin();

	if (gFarVersion.dwBuild>=FAR_Y_VER)
		FUNC_Y(ExitFARW)();
	else
		FUNC_X(ExitFARW)();
}






int ShowMessage(int aiMsg, int aiButtons)
{
	if (gFarVersion.dwVerMajor==1)
		return ShowMessageA(aiMsg, aiButtons);
	else if (gFarVersion.dwBuild>=FAR_Y_VER)
		return FUNC_Y(ShowMessage)(aiMsg, aiButtons);
	else
		return FUNC_X(ShowMessage)(aiMsg, aiButtons);
}

LPCWSTR GetMsgW(int aiMsg)
{
	if (gFarVersion.dwVerMajor==1)
		return L"";
	else if (gFarVersion.dwBuild>=FAR_Y_VER)
		return FUNC_Y(GetMsg)(aiMsg);
	else
		return FUNC_X(GetMsg)(aiMsg);
}

void PostMacro(wchar_t* asMacro)
{
	if (!asMacro || !*asMacro)
		return;
		
	if (gFarVersion.dwVerMajor==1) {
		int nLen = lstrlenW(asMacro);
		char* pszMacro = (char*)Alloc(nLen+1,1);
		if (pszMacro) {
			WideCharToMultiByte(CP_OEMCP,0,asMacro,nLen+1,pszMacro,nLen+1,0,0);
			PostMacroA(pszMacro);
			Free(pszMacro);
		}
	} else if (gFarVersion.dwBuild>=FAR_Y_VER) {
		FUNC_Y(PostMacro)(asMacro);
	} else {
		FUNC_X(PostMacro)(asMacro);
	}

	//FAR BUGBUG: Макрос не запускается на исполнение, пока мышкой не дернем :(
	//  Это чаще всего проявляется при вызове меню по RClick
	//  Если курсор на другой панели, то RClick сразу по пассивной
	//  не вызывает отрисовку :(
	//if (!mcr.Param.PlainText.Flags) {
	INPUT_RECORD ir[2] = {{MOUSE_EVENT},{MOUSE_EVENT}};
	if (isPressed(VK_CAPITAL))
		ir[0].Event.MouseEvent.dwControlKeyState |= CAPSLOCK_ON;
	if (isPressed(VK_NUMLOCK))
		ir[0].Event.MouseEvent.dwControlKeyState |= NUMLOCK_ON;
	if (isPressed(VK_SCROLL))
		ir[0].Event.MouseEvent.dwControlKeyState |= SCROLLLOCK_ON;
	ir[0].Event.MouseEvent.dwEventFlags = MOUSE_MOVED;
	ir[1].Event.MouseEvent.dwControlKeyState = ir[0].Event.MouseEvent.dwControlKeyState;
	ir[1].Event.MouseEvent.dwEventFlags = MOUSE_MOVED;
	ir[1].Event.MouseEvent.dwMousePosition.X = 1;
	ir[1].Event.MouseEvent.dwMousePosition.Y = 1;

	//2010-01-29 попробуем STD_OUTPUT
	//if (!ghConIn) {
	//	ghConIn  = CreateFile(L"CONIN$", GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_READ,
	//		0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
	//	if (ghConIn == INVALID_HANDLE_VALUE) {
	//		#ifdef _DEBUG
	//		DWORD dwErr = GetLastError();
	//		_ASSERTE(ghConIn!=INVALID_HANDLE_VALUE);
	//		#endif
	//		ghConIn = NULL;
	//		return;
	//	}
	//}
	HANDLE hIn = GetStdHandle(STD_INPUT_HANDLE);
	DWORD cbWritten = 0;
#ifdef _DEBUG
	BOOL fSuccess = 
#endif
	WriteConsoleInput(hIn/*ghConIn*/, ir, 1, &cbWritten);
	_ASSERTE(fSuccess && cbWritten==1);
	//}
	//InfoW995->AdvControl(InfoW995->ModuleNumber,ACTL_REDRAWALL,NULL);

}





int ShowPluginMenu()
{
	int nItem = -1;
	//if (!FarHwnd) {
	//	SHOWDBGINFO(L"*** ShowPluginMenu failed, FarHwnd is NULL\n");
	//	return;
	//}

	//if (nID != -1) {
	//	nItem = nID;
	//	if (nItem >= 2) nItem++; //Separator
	//	if (nItem >= 7) nItem++; //Separator
	//	if (nItem >= 9) nItem++; //Separator
	//	SHOWDBGINFO(L"*** ShowPluginMenu used default item\n");
	//} else 
	if (gFarVersion.dwVerMajor==1) {
		SHOWDBGINFO(L"*** calling ShowPluginMenuA\n");
		nItem = ShowPluginMenuA();
	} else if (gFarVersion.dwBuild>=FAR_Y_VER) {
		SHOWDBGINFO(L"*** calling ShowPluginMenuWY\n");
		nItem = FUNC_Y(ShowPluginMenu)();
	} else {
		SHOWDBGINFO(L"*** calling ShowPluginMenuWX\n");
		nItem = FUNC_X(ShowPluginMenu)();
	}

	return nItem;

	//if (nItem < 0) {
	//	SHOWDBGINFO(L"*** ShowPluginMenu cancelled, nItem < 0\n");
	//	return;
	//}

	//#ifdef _DEBUG
	//wchar_t szInfo[128]; wsprintf(szInfo, L"*** ShowPluginMenu done, nItem == %i\n", nItem);
	//SHOWDBGINFO(szInfo);
	//#endif

	//switch (nItem) {
	//	case 0: case 1:
	//	{ // Открыть в редакторе вывод последней консольной программы
	//		CESERVER_REQ* pIn = (CESERVER_REQ*)calloc(sizeof(CESERVER_REQ_HDR)+4,1);
	//		if (!pIn) return;
	//		CESERVER_REQ* pOut = NULL;
	//		ExecutePrepareCmd(pIn, CECMD_GETOUTPUTFILE, sizeof(CESERVER_REQ_HDR)+4);
	//		pIn->OutputFile.bUnicode = (gFarVersion.dwVerMajor>=2);
	//		pOut = ExecuteGuiCmd(FarHwnd, pIn, FarHwnd);
	//		if (pOut) {
	//			if (pOut->OutputFile.szFilePathName[0]) {
	//				BOOL lbRc = FALSE;
	//				if (gFarVersion.dwVerMajor==1)
	//					lbRc = EditOutputA(pOut->OutputFile.szFilePathName, (nItem==1));
	//				else if (gFarVersion.dwBuild>=FAR_Y_VER)
	//					lbRc = FUNC_Y(EditOutput)(pOut->OutputFile.szFilePathName, (nItem==1));
	//				else
	//					lbRc = FUNC_X(EditOutput)(pOut->OutputFile.szFilePathName, (nItem==1));
	//				if (!lbRc) {
	//					DeleteFile(pOut->OutputFile.szFilePathName);
	//				}
	//			}
	//			ExecuteFreeResult(pOut);
	//		}
	//		free(pIn);
	//	} break;
	//	case 3: // Показать/спрятать табы
	//	case 4: case 5: case 6:
	//	{
	//		CESERVER_REQ in, *pOut = NULL;
	//		ExecutePrepareCmd(&in, CECMD_TABSCMD, sizeof(CESERVER_REQ_HDR)+1);
	//		in.Data[0] = nItem - 3;
	//		pOut = ExecuteGuiCmd(FarHwnd, &in, FarHwnd);
	//		if (pOut) ExecuteFreeResult(pOut);
	//	} break;
	//	case 8: // Attach to GUI (если FAR был CtrlAltTab)
	//	{
	//		if (TerminalMode) break; // низзя
	//		if (ConEmuHwnd && IsWindow(ConEmuHwnd)) break; // Мы и так подключены?
	//		Attach2Gui();
	//	} break;
	//	case 10: // Start "ConEmuC.exe /DEBUGPID="
	//	{
	//		if (TerminalMode) break; // низзя
	//		StartDebugger();
	//	}
	//}
}




BOOL IsMacroActive()
{
	BOOL lbActive = FALSE;
	if (gFarVersion.dwVerMajor==1)
		lbActive = IsMacroActiveA();
	else if (gFarVersion.dwBuild>=FAR_Y_VER)
		lbActive = FUNC_Y(IsMacroActive)();
	else
		lbActive = FUNC_X(IsMacroActive)();

	return lbActive;
}

BOOL CheckPanelSettings(BOOL abSilence)
{
	BOOL lbOk = FALSE;

	if (gFarVersion.dwVerMajor==1)
		lbOk = CheckPanelSettingsA(abSilence);
	else if (gFarVersion.dwBuild>=FAR_Y_VER)
		lbOk = FUNC_Y(CheckPanelSettings)(abSilence);
	else
		lbOk = FUNC_X(CheckPanelSettings)(abSilence);

	return lbOk;
}

void LoadPanelItemInfo(CeFullPanelInfo* pi, int nItem)
{
	if (gFarVersion.dwVerMajor==1)
		LoadPanelItemInfoA(pi, nItem);
	else if (gFarVersion.dwBuild>=FAR_Y_VER)
		FUNC_Y(LoadPanelItemInfo)(pi, nItem);
	else
		FUNC_X(LoadPanelItemInfo)(pi, nItem);
}


// Возвращает (для удобства) ссылку на одну из глобальных переменных (pviLeft/pviRight)
CeFullPanelInfo* GetActivePanel()
{
	if (pviLeft.Visible && pviLeft.Focus && pviLeft.IsFilePanel)
		return &pviLeft;
	if (pviRight.Visible && pviRight.Focus && pviRight.IsFilePanel)
		return &pviRight;
	return NULL;
}
//CeFullPanelInfo* LoadPanelInfo(BOOL abActive)
//{
//	TODO("Добавить вызов ACTL_GETWINDOW что-ли?");
//	
//	CheckVarsInitialized();
//
//	CeFullPanelInfo* ppi = NULL;
//
//	if (gFarVersion.dwVerMajor==1)
//		ppi = LoadPanelInfoA(abActive);
//	else if (gFarVersion.dwBuild>=FAR_Y_VER)
//		ppi = FUNC_Y(LoadPanelInfo)(abActive);
//	else
//		ppi = FUNC_X(LoadPanelInfo)(abActive);
//
//	if (ppi) {
//		ppi->cbSize = sizeof(*ppi);
//
//		int n = min(ppi->nMaxFarColors, countof(gFarInfo.nFarColors));
//		if (n && ppi->nFarColors) memmove(gFarInfo.nFarColors, ppi->nFarColors, n);
//		gFarInfo.nFarInterfaceSettings = ppi->nFarInterfaceSettings;
//		gFarInfo.nFarPanelSettings = ppi->nFarPanelSettings;
//		gFarInfo.bFarPanelAllowed = TRUE;
//	}
//
//	return ppi;
//}

void ReloadPanelsInfo()
{
	TODO("Добавить вызов ACTL_GETWINDOW что-ли?");

	// Хотя уже и должен быть создан	
	CheckVarsInitialized();

	// Если меняется прямоугольник панели - нужно повторно зарегистрироваться в GUI
	RECT rcLeft = pviLeft.PanelRect;
	BOOL bLeftVisible = pviLeft.Visible;
	RECT rcRight = pviRight.PanelRect;
	BOOL bRightVisible = pviRight.Visible;

	if (gFarVersion.dwVerMajor==1)
		ReloadPanelsInfoA();
	else if (gFarVersion.dwBuild>=FAR_Y_VER)
		FUNC_Y(ReloadPanelsInfo)();
	else
		FUNC_X(ReloadPanelsInfo)();


	// Обновить gFarInfo (используется в RgnDetect)
	CeFullPanelInfo* p = pviLeft.hView ? &pviLeft : &pviRight;
	int n = min(p->nMaxFarColors, countof(gFarInfo.nFarColors));
	if (n && p->nFarColors) memmove(gFarInfo.nFarColors, p->nFarColors, n);
	gFarInfo.nFarInterfaceSettings = p->nFarInterfaceSettings;
	gFarInfo.nFarPanelSettings = p->nFarPanelSettings;
	gFarInfo.bFarPanelAllowed = TRUE;
	// Положения панелей
	gFarInfo.bFarLeftPanel = pviLeft.Visible;
	gFarInfo.FarLeftPanel.PanelRect = pviLeft.PanelRect;
	gFarInfo.bFarRightPanel = pviRight.Visible;
	gFarInfo.FarRightPanel.PanelRect = pviRight.PanelRect;


	if (pviLeft.hView) {
		if (bLeftVisible && pviLeft.Visible) {
			if (memcmp(&rcLeft, &pviLeft.PanelRect, sizeof(RECT)))
				pviLeft.RegisterPanelView();
		}
	}
	if (pviRight.hView) {
		if (bRightVisible && pviRight.Visible) {
			if (memcmp(&rcRight, &pviRight.PanelRect, sizeof(RECT)))
				pviRight.RegisterPanelView();
		}
	}
}


//BOOL IsLeftPanelActive()
//{
//	BOOL lbLeftActive = FALSE;
//	if (gFarVersion.dwVerMajor==1)
//		lbLeftActive = IsLeftPanelActiveA();
//	else if (gFarVersion.dwBuild>=FAR_Y_VER)
//		lbLeftActive = FUNC_Y(IsLeftPanelActive)();
//	else
//		lbLeftActive = FUNC_X(IsLeftPanelActive)();
//	return lbLeftActive;
//}

WORD PopUngetBuffer(BOOL abRemove, PINPUT_RECORD lpDst)
{
	if (gnUngetCount<1) {
		lpDst->EventType = 0;
	} else {
		*lpDst = girUnget[0];
		BOOL lbNeedPop = FALSE;
		if (girUnget[0].EventType == EVENT_TYPE_REDRAW)
			abRemove = TRUE; // Это - сразу удаляем из буфера

		if (girUnget[0].EventType == KEY_EVENT) {
			_ASSERTE(((short)girUnget[0].Event.KeyEvent.wRepeatCount) > 0);
			lpDst->Event.KeyEvent.wRepeatCount = 1;

			if (girUnget[0].Event.KeyEvent.wRepeatCount == 1) {
				lbNeedPop = abRemove;
			} else if (abRemove) {
				girUnget[0].Event.KeyEvent.wRepeatCount --;
			}
		} else {
			lbNeedPop = abRemove;
		}

		if (lbNeedPop) {
			_ASSERTE(abRemove);
			gnUngetCount --;
			_ASSERTE(gnUngetCount >= 0);
			if (gnUngetCount > 0) {
				// Подвинуть в буфере то что осталось к началу
				memmove(girUnget, girUnget+1, sizeof(girUnget[0])*gnUngetCount);
			}
			girUnget[gnUngetCount].EventType = 0;
		}
	}

	return lpDst->EventType;
}

BOOL GetBufferInput(BOOL abRemove, PINPUT_RECORD lpBuffer, DWORD nBufSize, LPDWORD lpNumberOfEventsRead)
{
	if (gnUngetCount<0) {
		_ASSERTE(gnUngetCount>=0);
		gnUngetCount = 0;
		*lpNumberOfEventsRead = 0;
		return FALSE;
	}
	if (nBufSize < 1) {
		_ASSERTE(nBufSize>=1);
		*lpNumberOfEventsRead = 0;
		return FALSE;
	}

	BOOL lbRedraw = FALSE, lbSetEnvVar = FALSE;
	WORD wType = 0;
	PINPUT_RECORD p = lpBuffer;

	while (nBufSize && gnUngetCount) {
		if ((wType = PopUngetBuffer(abRemove, p)) == 0)
			break; // буфер кончился
		if (wType == EVENT_TYPE_REDRAW) {
			lbRedraw = TRUE;
		} else {
			if (abRemove && gnUngetCount && girUnget[0].EventType == EVENT_TYPE_REDRAW) {
				lbSetEnvVar = TRUE;
			}
			nBufSize--; p++;
			if (!abRemove) break; // В режиме Peek - возвращаем не более одного нажатияs
		}
	}

	//// Если натыкаемся на событие EVENT_TYPE_REDRAW - сбрасывать переменную EnvVar
	//// SetEnvironmentVariable(TH_ENVVAR_NAME, TH_ENVVAR_ACTIVE);
	//if (girUnget[0].EventType == EVENT_TYPE_REDRAW)
	//{
	//	lbRedraw = TRUE;
	//	gnUngetCount --;
	//	_ASSERTE(gnUngetCount >= 0);
	//	if (gnUngetCount > 0) {
	//		// Подвинуть в буфере то что осталось к началу
	//		memmove(girUnget, girUnget+1, sizeof(girUnget[0])*gnUngetCount);
	//	}
	//	girUnget[gnUngetCount].EventType = 0;
	//}

	//int nMax = countof(girUnget);
	//if (gnUngetCount>=nMax) {
	//	_ASSERTE(gnUngetCount==nMax);
	//	if (gnUngetCount>nMax) gnUngetCount = nMax;
	//}

	//nMax = min(gnUngetCount,(int)nBufSize);

	//int i = 0, j = 0;
	//while (i < nMax && j < gnUngetCount) {
	//	if (girUnget[j].EventType == EVENT_TYPE_REDRAW) {
	//		j++; lbRedraw = TRUE; continue;
	//	}

	//	lpBuffer[i++] = girUnget[j++];
	//}
	//nMax = i;

	//if (abRemove) {
	//	gnUngetCount -= j;
	//	_ASSERTE(gnUngetCount >= 0);
	//	if (gnUngetCount > 0) {
	//		// Подвинуть в буфере то что осталось к началу
	//		memmove(girUnget, girUnget+nMax, sizeof(girUnget[0])*gnUngetCount);
	//		girUnget[gnUngetCount].EventType = 0;
	//	}
	//}

	if (lbRedraw || lbSetEnvVar) {
		gbWaitForKeySequenceEnd = (gnUngetCount > 0) && !lbSetEnvVar;
		UpdateEnvVar(TRUE);

		if (IsThumbnailsActive(FALSE)) {
			if (lbRedraw && !gbWaitForKeySequenceEnd)
				OnMainThreadActived();
		}
	}

	*lpNumberOfEventsRead = (DWORD)(p - lpBuffer);

	return TRUE;
}

BOOL UngetBufferInput(WORD nCount, PINPUT_RECORD lpOneInput)
{
	_ASSERTE(nCount);
	if (gnUngetCount<0) {
		_ASSERTE(gnUngetCount>=0);
		gnUngetCount = 0;
	}
	int nMax = countof(girUnget);
	if (gnUngetCount>=nMax) {
		_ASSERTE(gnUngetCount==nMax);
		if (gnUngetCount>nMax) gnUngetCount = nMax;
		return FALSE;
	}

	girUnget[gnUngetCount] = *lpOneInput;
	if (lpOneInput->EventType == KEY_EVENT) {
		girUnget[gnUngetCount].Event.KeyEvent.wRepeatCount = nCount;
	}
	gnUngetCount ++;	

	return TRUE;
}

void ResetUngetBuffer()
{
	gnConsoleChanges = 0;
	gnUngetCount = 0;
	gbWaitForKeySequenceEnd = false;
}

// Должен активироваться или через Synchro или через PeekConsoleInput в FAR1
void OnMainThreadActived()
{
	if (!gnConsoleChanges)
		return;
	DWORD nCurChanges = gnConsoleChanges; gnConsoleChanges = 0;

	// Сбросим, чтобы RgnDetect попытался сам найти панели и диалоги.
	// Это нужно чтобы избежать возможных блокировок фара
	gFarInfo.bFarPanelInfoFilled = gFarInfo.bFarLeftPanel = gFarInfo.bFarRightPanel = FALSE;
	gpRgnDetect->PrepareTransparent(&gFarInfo, gcrCurColors);
	gnRgnDetectFlags = gpRgnDetect->GetFlags();

#ifdef _DEBUG
	if (!gpDbgDlg) {
		gpDbgDlg = new MFileMapping<DetectedDialogs>();
		gpDbgDlg->InitName(CEPANELDLGMAPNAME, GetCurrentProcessId());
		gpDbgDlg->Create();
	}
	gpDbgDlg->SetFrom(gpRgnDetect->GetDetectedDialogsPtr());	
#endif

	WARNING("Если панели скрыты (активен редактор/вьювер) - не пытаться считывать панели");

	if (!CheckWindows()) {
		// Спрятать/разрегистрировать?
	} else {
		//if (pviLeft.hView || pviRight.hView) {
		//	ReloadPanelsInfo();

		//	/* После реального получения панелей - можно повторно "обнаружить диалоги"? */
		//	CeFullPanelInfo* p = pviLeft.hView ? &pviLeft : &pviRight;
		//	gFarInfo.bFarPanelInfoFilled = TRUE;
		//	gFarInfo.bFarLeftPanel = (pviLeft.Visible!=0);
		//	gFarInfo.FarLeftPanel.PanelRect = pviLeft.PanelRect;
		//	gFarInfo.bFarRightPanel = (pviRight.Visible!=0);
		//	gFarInfo.FarRightPanel.PanelRect = pviRight.PanelRect;

		//	gpRgnDetect->PrepareTransparent(&gFarInfo, gcrColors);
		//}

		if (!gbWaitForKeySequenceEnd 
			|| girUnget[0].EventType == EVENT_TYPE_REDRAW)
		{
			if (pviLeft.hView || pviRight.hView) {
				ReloadPanelsInfo();
			}
			if (pviLeft.hView) {
				pviLeft.DisplayReloadPanel();
				InvalidateRect(pviLeft.hView, NULL, FALSE);
			}
			if (pviRight.hView) {
				pviRight.DisplayReloadPanel();
				InvalidateRect(pviRight.hView, NULL, FALSE);
			}
		}
	}

	//WARNING("Проверить список окон FAR и если активное НЕ панели - сразу выйти (и спрятать разрегистрировать)");
	//WARNING("Проверить и сравнить, если изменился регион панели - обновить PanelView");

	//!. Сначала проверить на какой панели находится фокус!
	//   Это требуется для того, чтобы НЕактивная панель не перехватывала стрелки
	//Далее
	//1. Панель видима -> считать информацию о прямоугольнике, элементах панели, и выполнить отрисовку
	//   информацию о прямоугольнике (если он изменился) передать в GUI
	//   Если окошко панели невидимо - выполнить повторную регистрацию в GUI - оно само сделает apiShowWindow
	//2. Панель невидима -> спрятать окошко панели и разрегистрироваться в GUI
}

BOOL WINAPI OnPrePeekConsole(HANDLE hInput, PINPUT_RECORD lpBuffer, DWORD nBufSize, LPDWORD lpNumberOfEventsRead, BOOL* pbResult)
{
	if (gnConsoleChanges) {
		OnMainThreadActived();
	}

	if (!lpBuffer || !lpNumberOfEventsRead) {
		*pbResult = FALSE;
		return FALSE; // ошибка в параметрах
	}

	// если в girUnget что-то есть вернуть из него и FALSE (тогда OnPostPeekConsole не будет вызван)");
	if (gnUngetCount) {
		if (GetBufferInput(FALSE/*abRemove*/, lpBuffer, nBufSize, lpNumberOfEventsRead))
			return FALSE; // PeekConsoleInput & OnPostPeekConsole не будет вызван
	}

	// Для FAR1 - эмуляция ACTL_SYNCHRO
	if (gpLastSynchroArg  // ожидает команда
		&& nBufSize == 1  // только когда размер буфера == 1 - считается что ФАР готов
		&& gFarVersion.dwVerMajor==1) // FAR1
	{
		ProcessSynchroEventW(SE_COMMONSYNCHRO, gpLastSynchroArg);
	}

	return TRUE; // продолжить без изменений
}

BOOL WINAPI OnPostPeekConsole(HANDLE hInput, PINPUT_RECORD lpBuffer, DWORD nBufSize, LPDWORD lpNumberOfEventsRead, BOOL* pbResult)
{
	if (!lpBuffer || !lpNumberOfEventsRead) {
		*pbResult = FALSE;
		return FALSE; // ошибка в параметрах
	}

	// заменить обработку стрелок, но girUnget не трогать
	if (*lpNumberOfEventsRead) {
		if (!ProcessConsoleInput(FALSE/*abUseUngetBuffer*/, lpBuffer, nBufSize, lpNumberOfEventsRead)) {
			if (*lpNumberOfEventsRead == 0) {
				*pbResult = FALSE;
			}
			return FALSE; // вернуться в вызывающую функцию
		}
	}
	
	return TRUE; // продолжить без изменений
}

BOOL WINAPI OnPreReadConsole(HANDLE hInput, PINPUT_RECORD lpBuffer, DWORD nBufSize, LPDWORD lpNumberOfEventsRead, BOOL* pbResult)
{
	if (!lpBuffer || !lpNumberOfEventsRead) {
		*pbResult = FALSE;
		return FALSE; // ошибка в параметрах
	}

	// если в girUnget что-то есть вернуть из него и FALSE (тогда OnPostPeekConsole не будет вызван)");
	if (gnUngetCount) {
		if (GetBufferInput(TRUE/*abRemove*/, lpBuffer, nBufSize, lpNumberOfEventsRead))
			return FALSE; // ReadConsoleInput & OnPostReadConsole не будет вызван
	}

	return TRUE; // продолжить без изменений
}

BOOL WINAPI OnPostReadConsole(HANDLE hInput, PINPUT_RECORD lpBuffer, DWORD nBufSize, LPDWORD lpNumberOfEventsRead, BOOL* pbResult)
{
	if (!lpBuffer || !lpNumberOfEventsRead) {
		*pbResult = FALSE;
		return FALSE; // ошибка в параметрах
	}

	// заменить обработку стрелок, если надо - поместить серию нажатий в girUnget
	if (*lpNumberOfEventsRead) {
		if (!ProcessConsoleInput(TRUE/*abUseUngetBuffer*/, lpBuffer, nBufSize, lpNumberOfEventsRead)) {
			if (*lpNumberOfEventsRead == 0) {
				*pbResult = FALSE;
			}
			return FALSE; // вернуться в вызывающую функцию
		}
	}
	
	return TRUE; // продолжить без изменений
}




//lpBuffer 
//The data to be written to the console screen buffer. This pointer is treated as the origin of a 
//two-dimensional array of CHAR_INFO structures whose size is specified by the dwBufferSize parameter. 
//The total size of the array must be less than 64K.
//
//dwBufferSize 
//The size of the buffer pointed to by the lpBuffer parameter, in character cells. 
//The X member of the COORD structure is the number of columns; the Y member is the number of rows.
//
//dwBufferCoord 
//The coordinates of the upper-left cell in the buffer pointed to by the lpBuffer parameter. 
//The X member of the COORD structure is the column, and the Y member is the row.
//
//lpWriteRegion 
//A pointer to a SMALL_RECT structure. On input, the structure members specify the upper-left and lower-right 
//coordinates of the console screen buffer rectangle to write to. 
//On output, the structure members specify the actual rectangle that was used.
BOOL WINAPI OnPreWriteConsoleOutput(HANDLE hOutput,const CHAR_INFO *lpBuffer,COORD dwBufferSize,COORD dwBufferCoord,PSMALL_RECT lpWriteRegion)
{
	WARNING("После повторного отображения view - хорошо бы сначала полностью считать gpRgnDetect из консоли");
	if (gpRgnDetect && lpBuffer && lpWriteRegion) {
#ifdef SHOW_WRITING_RECTS
		if (IsDebuggerPresent()) {
			wchar_t szDbg[80]; wsprintf(szDbg, L"ConEmuTh.OnPreWriteConsoleOutput( {%ix%i} - {%ix%i} )\n", 
				lpWriteRegion->Left, lpWriteRegion->Top, lpWriteRegion->Right, lpWriteRegion->Bottom);
			OutputDebugStringW(szDbg);
		}
#endif

		SMALL_RECT rcFarRect; GetFarRect(&rcFarRect);
		gpRgnDetect->SetFarRect(&rcFarRect);

		gpRgnDetect->OnWriteConsoleOutput(lpBuffer, dwBufferSize, dwBufferCoord, lpWriteRegion, gcrCurColors);

		WARNING("Перед установкой флагов измененности - сначала хорошо бы проверить, а менялась ли сама панель?");
		if (pviLeft.hView || pviRight.hView) {
			gnConsoleChanges |= 3; // 1-left, 2-right. Но пока - ставим все, т.к. еще не проверяется что собственно изменилось!
		}
	}

	return TRUE; // Продолжить без изменений
}


//// Вернуть TRUE, если был Unget
//BOOL ProcessKeyPress(CeFullPanelInfo* pi, BOOL abUseUngetBuffer, PINPUT_RECORD lpBuffer)
//{
//	_ASSERTE(lpBuffer->EventType == KEY_EVENT);
//	
//	// Перехватываемые клавиши
//	WORD vk = lpBuffer->Event.KeyEvent.wVirtualKeyCode;
//	BOOL lbWasUnget = FALSE;
//	
//	if (vk == VK_UP || vk == VK_DOWN || vk == VK_LEFT || vk == VK_RIGHT)
//	{
//		if (!lpBuffer->Event.KeyEvent.bKeyDown) {
//			switch (vk) {
//				case VK_LEFT:  lpBuffer->Event.KeyEvent.wVirtualKeyCode = VK_UP;   break;
//				case VK_RIGHT: lpBuffer->Event.KeyEvent.wVirtualKeyCode = VK_DOWN; break;
//			}
//		
//		} else {
//			// Переработать
//			int n = 0;
//			switch (vk) {
//				case VK_UP: {
//					n = min(pi->CurrentItem,pi->nXCount);
//				} break;
//				case VK_DOWN: {
//					n = min((pi->ItemsNumber-pi->CurrentItem-1),pi->nXCount);
//				} break;
//				case VK_LEFT: {
//					lpBuffer->Event.KeyEvent.wVirtualKeyCode = VK_UP;
//				} break;
//				case VK_RIGHT: {
//					lpBuffer->Event.KeyEvent.wVirtualKeyCode = VK_DOWN;
//				} break;
//			}
//			
//			lbWasUnget = (n > 1);
//			while ((n--) > 1) {
//				UngetBufferInput(lpBuffer);
//			}
//		}
//	}
//	return lbWasUnget;
//}

// Выполнить замену клавиш и поместить в буфер (Unget) непоместившиеся в lpBuffer клавиши
// Вернуть FALSE, если замены были произведены
BOOL ProcessConsoleInput(BOOL abUseUngetBuffer, PINPUT_RECORD lpBuffer, DWORD nBufSize, LPDWORD lpNumberOfEventsRead)
{
	// Перехват управления курсором
	CeFullPanelInfo* pi = IsThumbnailsActive(TRUE/*abFocusRequired*/);
	if (!pi) {
		return TRUE; // панель создана, но она не активна или скрыта диалогом или погашена фаровская панель
	}


	if (!wScanCodeUp) {
		wScanCodeUp = MapVirtualKey(VK_UP, 0/*MAPVK_VK_TO_VSC*/);
		wScanCodeDown = MapVirtualKey(VK_DOWN, 0/*MAPVK_VK_TO_VSC*/);
#ifdef _DEBUG
		wScanCodeLeft = MapVirtualKey(VK_LEFT, 0/*MAPVK_VK_TO_VSC*/);
		wScanCodeRight = MapVirtualKey(VK_RIGHT, 0/*MAPVK_VK_TO_VSC*/);
		wScanCodePgUp = MapVirtualKey(VK_PRIOR, 0/*MAPVK_VK_TO_VSC*/);
		wScanCodePgDn = MapVirtualKey(VK_NEXT, 0/*MAPVK_VK_TO_VSC*/);
#endif
	}
	
	WARNING("Проверять один из DWORD-ов окна на предмет наличия диалогов");
	WARNING("Ставить его должен GUI");
	WARNING("Если есть хотя бы один диалог - значит перехватывать нельзя");

	PanelViewMode PVM = pi->PVM;

	// Пойдем в два прохода. В первом - обработка замен, и помещение в буфер Unget.
	PINPUT_RECORD p = lpBuffer;
	PINPUT_RECORD pEnd = lpBuffer+*lpNumberOfEventsRead;
	PINPUT_RECORD pFirstReplace = NULL;
	while (p != pEnd)
	{
		if (p->EventType == KEY_EVENT)
		{
			// Перехватываемые клавиши
			WORD vk = p->Event.KeyEvent.wVirtualKeyCode;
			if (vk == VK_UP || vk == VK_DOWN || vk == VK_LEFT || vk == VK_RIGHT || vk == VK_PRIOR || vk == VK_NEXT)
			{
				if (!(p->Event.KeyEvent.dwControlKeyState 
					& (LEFT_ALT_PRESSED|RIGHT_ALT_PRESSED|LEFT_CTRL_PRESSED|RIGHT_CTRL_PRESSED|SHIFT_PRESSED)) )
				{
					// Переработать
					int n = 1;
					switch (vk) {
						case VK_UP:
						{
							if (PVM == pvm_Thumbnails)
								n = min(pi->CurrentItem,pi->nXCountFull);
						} break;
						case VK_DOWN:
						{
							if (PVM == pvm_Thumbnails)
								n = min((pi->ItemsNumber-pi->CurrentItem-1),pi->nXCountFull);
						} break;
						case VK_LEFT:
						{
							p->Event.KeyEvent.wVirtualKeyCode = VK_UP;
							p->Event.KeyEvent.wVirtualScanCode = wScanCodeUp;
							if (PVM != pvm_Thumbnails)
								n = min(pi->CurrentItem,pi->nYCountFull);
						} break;
						case VK_RIGHT:
						{
							p->Event.KeyEvent.wVirtualKeyCode = VK_DOWN;
							p->Event.KeyEvent.wVirtualScanCode = wScanCodeDown;
							if (PVM != pvm_Thumbnails)
								n = min((pi->ItemsNumber-pi->CurrentItem-1),pi->nYCountFull);
						} break;
						case VK_PRIOR:
						{
							p->Event.KeyEvent.wVirtualKeyCode = VK_UP;
							p->Event.KeyEvent.wVirtualScanCode = wScanCodeUp;
							n = min(pi->CurrentItem,pi->nXCountFull*pi->nYCountFull);
						} break;
						case VK_NEXT:
						{
							p->Event.KeyEvent.wVirtualKeyCode = VK_DOWN;
							p->Event.KeyEvent.wVirtualScanCode = wScanCodeUp;
							n = min((pi->ItemsNumber-pi->CurrentItem-1),pi->nXCountFull*pi->nYCountFull);
						} break;
					}

					// Если это нажатие - то дополнительные нужно поместить в Unget буфер
					if (p->Event.KeyEvent.bKeyDown) {
						if (abUseUngetBuffer && n > 1) {
							pFirstReplace = p+1;
							UngetBufferInput(n-1, p);
						}
					}

					//end: if (vk == VK_UP || vk == VK_DOWN || vk == VK_LEFT || vk == VK_RIGHT)
					p++; continue;
				}
			}
			//end: if (p->EventType == KEY_EVENT)
		} else
		
		// При смене размера окна - нужно передернуть детектор
		if (p->EventType == WINDOW_BUFFER_SIZE_EVENT) {
			if (gpRgnDetect)
				gpRgnDetect->OnWindowSizeChanged();
		}

		// Если были помещены события в буфер Unget - требуется поместить в буфер и все что идут за ним,
		// т.к. между ними будет "виртуальная" вставка событий
		if (pFirstReplace && abUseUngetBuffer) {
			UngetBufferInput(1,p);
		}
		p++; continue;
	}
	// Скорректировать количество "считанных" событий
	if (pFirstReplace && abUseUngetBuffer)
	{
		DWORD nReady = (int)(pFirstReplace - lpBuffer);
		if (nReady != *lpNumberOfEventsRead) {
			_ASSERTE(nReady <= nBufSize);
			_ASSERTE(nReady < *lpNumberOfEventsRead);
			*lpNumberOfEventsRead = nReady;
		}
	}

	// Если в буфере есть "отложенные" события - возможно потребуется установка переменной окружения
	if (pFirstReplace && abUseUngetBuffer && gnUngetCount) {
		_ASSERTE(gnUngetCount>0);
		INPUT_RECORD r = {EVENT_TYPE_REDRAW};
		UngetBufferInput(1,&r);
		//SetEnvironmentVariable(TH_ENVVAR_NAME, TH_ENVVAR_SCROLL);
		gbWaitForKeySequenceEnd = true;
		UpdateEnvVar(FALSE);
	}

	// Второй проход - если буфер достаточно большой - добавить в него события из Unget буфера
	if (gnUngetCount && nBufSize > *lpNumberOfEventsRead)
	{
		DWORD nAdd = 0;
		if (GetBufferInput(abUseUngetBuffer/*abRemove*/, pFirstReplace, nBufSize-(*lpNumberOfEventsRead), &nAdd))
			*lpNumberOfEventsRead += nAdd;
	}

	return TRUE;
}

int ShowLastError()
{
	if (gnCreateViewError) {
		wchar_t szErrMsg[512];
		const wchar_t* pszTempl = GetMsgW(gnCreateViewError);
		if (pszTempl && *pszTempl) {
			wsprintfW(szErrMsg, pszTempl, gnWin32Error);
			if (gFarVersion.dwBuild>=FAR_Y_VER)
				return FUNC_Y(ShowMessage)(szErrMsg, 0);
			else
				return FUNC_X(ShowMessage)(szErrMsg, 0);
		}
	}
	return 0;
}

void UpdateEnvVar(BOOL abForceRedraw)
{
	WARNING("сбрасывать скролл нужно ПЕРЕД возвратом последней клавиши ДО REDRAW");
	if (gbWaitForKeySequenceEnd) {
		SetEnvironmentVariable(TH_ENVVAR_NAME, TH_ENVVAR_SCROLL);
	} else if (IsThumbnailsActive(FALSE/*abFocusRequired*/)) {
		SetEnvironmentVariable(TH_ENVVAR_NAME, TH_ENVVAR_ACTIVE);
		abForceRedraw = TRUE;
	} else {
		SetEnvironmentVariable(TH_ENVVAR_NAME, NULL);
	}

	if (abForceRedraw) {
		if (pviLeft.hView && IsWindowVisible(pviLeft.hView))
			InvalidateRect(pviLeft.hView, NULL, FALSE);
		if (pviRight.hView && IsWindowVisible(pviRight.hView))
			InvalidateRect(pviRight.hView, NULL, FALSE);
	}
}

CeFullPanelInfo* IsThumbnailsActive(BOOL abFocusRequired)
{
	if (pviLeft.hView == NULL && pviRight.hView == NULL)
		return NULL;

	if (!CheckWindows())
		return NULL;

	CeFullPanelInfo* pi = NULL;
	
	if (gpRgnDetect) {
		DWORD dwFlags = gpRgnDetect->GetFlags();
		if ((dwFlags & FR_ACTIVEMENUBAR) == FR_ACTIVEMENUBAR)
			return NULL; // активно меню
		if ((dwFlags & FR_FREEDLG_MASK) != 0)
			return NULL; // есть какой-то диалог
	}

	if (abFocusRequired) {
		if (pviLeft.hView && pviLeft.Focus && pviLeft.Visible)
			pi = &pviLeft;
		else if (pviRight.hView && pviRight.Focus && pviRight.Visible)
			pi = &pviRight;
		// Видим?
		if (pi) {
			if (!pi->hView || !IsWindowVisible(pi->hView)) {
				return NULL; // панель полностью скрыта диалогом или погашена фаровская панель
			}
		}
	} else {
		if (pviLeft.hView && IsWindowVisible(pviLeft.hView))
			pi = &pviLeft;
		else if (pviRight.hView && IsWindowVisible(pviRight.hView))
			pi = &pviRight;
	}
	
	// Может быть PicView/MMView...
	if (pi) {
		RECT rc;
		GetClientRect(pi->hView, &rc);
		POINT pt = {((rc.left+rc.right)>>1),((rc.top+rc.bottom)>>1)};
		MapWindowPoints(pi->hView, ghConEmuRoot, &pt, 1);
		HWND hChild[2];
		hChild[0] = ChildWindowFromPointEx(ghConEmuRoot, pt, CWP_SKIPINVISIBLE|CWP_SKIPTRANSPARENT);
		// Теперь проверим полноэкранные окна
		MapWindowPoints(ghConEmuRoot, NULL, &pt, 1);
		hChild[1] = WindowFromPoint(pt);

		for (int i = 0; i <= 1; i++) {
			// В принципе, может быть и NULL, если координата попала в "прозрачную" часть hView
			if (hChild[i] && hChild[i] != pi->hView) {
				wchar_t szClass[128];
				if (GetClassName(hChild[i], szClass, 128)) {
					if (lstrcmpi(szClass, L"FarPictureViewControlClass") == 0)
						return NULL; // активен PicView!
					if (lstrcmpi(szClass, L"FarMultiViewControlClass") == 0)
						return NULL; // активен MMView!
				}
			}
		}
	}

	return pi;
}

// Должен вернуть true, если активны только панели (нет диалогов или еще каких меню)
bool CheckWindows()
{
	//if (gFarVersion.dwVerMajor==1)
	//	gbLastCheckWindow = CheckWindowsA();
	//else if (gFarVersion.dwBuild>=FAR_Y_VER)
	//	gbLastCheckWindow = FUNC_Y(CheckWindows)();
	//else
	//	gbLastCheckWindow = FUNC_X(CheckWindows)();
	//return gbLastCheckWindow;
	bool lbRc = false;
	if (gpRgnDetect) {
		//WARNING: Диалоги уже должны быть "обнаружены"
		// используем gnRgnDetectFlags, т.к. gpRgnDetect может оказаться в процессе распознавания
		DWORD dwFlags = gnRgnDetectFlags; // gpRgnDetect->GetFlags();

		// вдруг панелей вообще не обнаружено?
		if ((dwFlags & (FR_LEFTPANEL|FR_RIGHTPANEL|FR_FULLPANEL)) != 0) {
			// нет диалогов
			if ((dwFlags & FR_FREEDLG_MASK) == 0) {
				// и нет активированного меню
				if ((dwFlags & FR_ACTIVEMENUBAR) != FR_ACTIVEMENUBAR) {
					lbRc = true;
				}
			}
		}
	}
	//gbLastCheckWindow = lbRc;
	return lbRc;
}

void CheckVarsInitialized()
{
	if (!gpRgnDetect) {
		gpRgnDetect = new CRgnDetect();
	}

	// Должно инититься в SetStartupInfo	
	_ASSERTE(gpImgCache!=NULL);
	//if (!gpImgCache) {
	//	gpImgCache = new CImgCache(ghPluginModule);
	//}

	//CeFullPanelInfo* p = pviLeft.hView ? &pviLeft : &pviRight;

	if (gFarInfo.cbSize == 0) {
		gFarInfo.cbSize = sizeof(gFarInfo);
		gFarInfo.FarVer = gFarVersion;
		gFarInfo.nFarPID = GetCurrentProcessId();
		gFarInfo.nFarTID = GetCurrentThreadId();
		gFarInfo.bFarPanelAllowed = TRUE;
		// Загрузить из реестра настройки PanelTabs
		gFarInfo.PanelTabs.SeparateTabs = gFarInfo.PanelTabs.ButtonColor = -1;
		if (gszRootKey && *gszRootKey) {
			int nLen = lstrlenW(gszRootKey);
			wchar_t* pszTabsKey = (wchar_t*)malloc((nLen+32)*2);
			lstrcpyW(pszTabsKey, gszRootKey);
			pszTabsKey[nLen-1] = 0;
			wchar_t* pszSlash = wcsrchr(pszTabsKey, L'\\');
			if (pszSlash) {
				lstrcpyW(pszSlash, L"\\Plugins\\PanelTabs");
				HKEY hk;
				if (0 == RegOpenKeyExW(HKEY_CURRENT_USER, pszTabsKey, 0, KEY_READ, &hk)) {
					DWORD dwVal, dwSize;
					if (!RegQueryValueExW(hk, L"SeparateTabs", NULL, NULL, (LPBYTE)&dwVal, &(dwSize = 4)))
						gFarInfo.PanelTabs.SeparateTabs = dwVal ? 1 : 0;
					if (!RegQueryValueExW(hk, L"ButtonColor", NULL, NULL, (LPBYTE)&dwVal, &(dwSize = 4)))
						gFarInfo.PanelTabs.ButtonColor = dwVal & 0xFF;
					RegCloseKey(hk);
				}
			}
			free(pszTabsKey);
		}
	}

	if (!pviLeft.pSection) {
		pviLeft.pSection = new MSection();
	}
	if (!pviRight.pSection) {
		pviRight.pSection = new MSection();
	}
}

void ExecuteInMainThread(ConEmuThSynchroArg* pCmd)
{
	if (!pCmd) return;

	if (gpLastSynchroArg && gpLastSynchroArg != pCmd) {
		LocalFree(gpLastSynchroArg); gpLastSynchroArg = NULL;
	}
	gpLastSynchroArg = pCmd;
	_ASSERTE(gpLastSynchroArg->bValid==1 && gpLastSynchroArg->bExpired==0);

	BOOL lbLeftActive = FALSE;
	if (gFarVersion.dwVerMajor==1) {
		; // в 1.75 такой функции нет, придется хаком
	} else if (gFarVersion.dwBuild>=FAR_Y_VER) {
		FUNC_Y(ExecuteInMainThread)(pCmd);
	} else {
		FUNC_Y(ExecuteInMainThread)(pCmd);
	}
}

int WINAPI ProcessSynchroEventW(int Event, void *Param)
{
	if (Event != SE_COMMONSYNCHRO) return 0;

	if (Param) {
		ConEmuThSynchroArg* pCmd = (ConEmuThSynchroArg*)Param;
		if (gpLastSynchroArg == pCmd) gpLastSynchroArg = NULL;

		if (pCmd->bValid == 1) {
			if (pCmd->bExpired == 0) {
				if (pCmd->nCommand == ConEmuThSynchroArg::eExecuteMacro) {
					PostMacro((wchar_t*)pCmd->Data);
				}
			}

			LocalFree(pCmd);
		}
	}

	return 0;
}


BOOL LoadThSet(DWORD anGuiPid/* =-1 */)
{
	BOOL lbRc = FALSE;
	MFileMapping<PanelViewSettings> ThSetMap;
	_ASSERTE(ghConEmuRoot!=NULL);

	DWORD nGuiPID;
	GetWindowThreadProcessId(ghConEmuRoot, &nGuiPID);
	if (anGuiPid != -1) {
		_ASSERTE(nGuiPID == anGuiPid);
		nGuiPID = anGuiPid;
	}

	ThSetMap.InitName(CECONVIEWSETNAME, nGuiPID);
	if (!ThSetMap.Open()) {
		MessageBox(NULL, ThSetMap.GetErrorText(), L"ConEmuTh", MB_ICONSTOP|MB_SETFOREGROUND|MB_SYSTEMMODAL);
	} else {
		ThSetMap.GetTo(&gThSet);
		ThSetMap.CloseMap();
		lbRc = TRUE;
	}

	return lbRc;
}

//sx, sy - смещение правого верхнего угла панелей от правого верхнего угла консоли
//cx, cy - если не 0 - то определяет ширину и высоту панелей
BOOL GetFarRect(SMALL_RECT* prcFarRect)
{
	BOOL lbFarBuffer = FALSE;
	prcFarRect->Left = prcFarRect->Right = prcFarRect->Top = prcFarRect->Bottom = 0;
	if (gFarVersion.dwVerMajor>2
		|| (gFarVersion.dwVerMajor==2 && gFarVersion.dwBuild>=1573/*FAR_Y_VER*/))
	{
		//_ASSERTE(1573<=FAR_Y_VER);
		FUNC_Y(GetFarRect)(prcFarRect);
		lbFarBuffer = (prcFarRect->Bottom && prcFarRect->Right);
	}
	return lbFarBuffer;
}




void CeFullPanelInfo::FinalRelease()
{
	if (ppItems) {
		for (int i=0; i<ItemsNumber; i++)
			if (ppItems[i]) free(ppItems[i]);
		free(ppItems);
		ppItems = NULL;
	}
	nMaxItemsNumber = 0;
	if (pszPanelDir) {
		free(pszPanelDir);
		pszPanelDir = NULL;
	}
	nMaxPanelDir = 0;
	if (nFarColors) {
		free(nFarColors);
		nFarColors = NULL;
	}
	nMaxFarColors = 0;
	if (pFarTmpBuf) {
		free(pFarTmpBuf);
		pFarTmpBuf = NULL;
	}
	nFarTmpBuf = 0;

	if (hView) {
		BOOL bValid = IsWindow(hView);
		_ASSERTE(bValid==FALSE);
		hView = NULL;
	}

	if (pSection) {
		delete pSection; pSection = NULL;
	}
}

BOOL CeFullPanelInfo::ReallocItems(int anCount)
{
	CePluginPanelItem** ppNew = NULL;

	if ((ppItems == NULL) || (nMaxItemsNumber < anCount)) {
		MSectionLock CS;
		if (!CS.Lock(pSection, TRUE, 5000))
			return FALSE;

		int nNewMax = anCount+255; // + немножно про запас
		ppNew = (CePluginPanelItem**)calloc(nNewMax, sizeof(LPVOID));
		if (!ppNew) {
			_ASSERTE(ppNew!=NULL);
			return FALSE;
		}

		if (ppItems) {
			if (nMaxItemsNumber) {
				memmove(ppNew, ppItems, nMaxItemsNumber*sizeof(LPVOID));
			}
			free(ppItems);
		}
		nMaxItemsNumber = nNewMax;
		ppItems = ppNew;
	}

	return TRUE;
}

BOOL CeFullPanelInfo::FarItem2CeItem(int anIndex,
	const wchar_t*   asName,
	const wchar_t*   asDesc,
	DWORD            dwFileAttributes,
	FILETIME         ftLastWriteTime,
	unsigned __int64 anFileSize,
	BOOL             abVirtualItem,
	DWORD_PTR        apUserData,
	DWORD            anFlags,
	DWORD            anNumberOfLinks)
{
	// Необходимый размер буфера для хранения элемента
	size_t nSize = sizeof(CePluginPanelItem)
		+(wcslen(asName)+1)*2
		+((asDesc ? wcslen(asDesc) : 0)+1)*2;
	
	// Уже может быть выделено достаточно памяти под этот элемент
	if ((ppItems[anIndex] == NULL) || (ppItems[anIndex]->cbSize < (DWORD_PTR)nSize)) {
		if (ppItems[anIndex]) free(ppItems[anIndex]);
		nSize += 32;
		ppItems[anIndex] = (CePluginPanelItem*)calloc(nSize, 1);
		if (ppItems[anIndex] == NULL) {
			_ASSERTE(ppItems[anIndex] != NULL);
			return FALSE;
		}
		ppItems[anIndex]->cbSize = (int)nSize;
	}
	
	// Указатель на буфер для имени файла (он идет сразу за структурой, память выделена)
	wchar_t* psz = (wchar_t*)(ppItems[anIndex]+1);

	
	if (ppItems[anIndex]->bIsCurrent != (CurrentItem == anIndex) ||
		ppItems[anIndex]->bVirtualItem != abVirtualItem ||
		ppItems[anIndex]->UserData != apUserData ||
		ppItems[anIndex]->Flags != anFlags ||
		ppItems[anIndex]->NumberOfLinks != anNumberOfLinks ||
		ppItems[anIndex]->FindData.dwFileAttributes != dwFileAttributes ||
		ppItems[anIndex]->FindData.ftLastWriteTime.dwLowDateTime != ftLastWriteTime.dwLowDateTime ||
		ppItems[anIndex]->FindData.ftLastWriteTime.dwHighDateTime != ftLastWriteTime.dwHighDateTime ||
		ppItems[anIndex]->FindData.nFileSize != anFileSize ||
		lstrcmp(psz, asName))
	{
		// Лучше сбросить, чтобы мусор не оставался, да и поля в стуктуру могут добавляться, чтобы не забылось...	
		memset(((LPBYTE)ppItems[anIndex])+sizeof(ppItems[anIndex]->cbSize), 0, ppItems[anIndex]->cbSize-sizeof(ppItems[anIndex]->cbSize));
	}

	
	// Копируем
	ppItems[anIndex]->bIsCurrent = (CurrentItem == anIndex);
	ppItems[anIndex]->bVirtualItem = abVirtualItem;
	ppItems[anIndex]->UserData = apUserData;
	ppItems[anIndex]->Flags = anFlags;
	ppItems[anIndex]->NumberOfLinks = anNumberOfLinks;
	ppItems[anIndex]->FindData.dwFileAttributes = dwFileAttributes;
	ppItems[anIndex]->FindData.ftLastWriteTime = ftLastWriteTime;
	ppItems[anIndex]->FindData.nFileSize = anFileSize;
	lstrcpy(psz, asName);
	ppItems[anIndex]->FindData.lpwszFileName = psz;
	ppItems[anIndex]->FindData.lpwszFileNamePart = wcsrchr(psz, L'\\');
	if (ppItems[anIndex]->FindData.lpwszFileNamePart == NULL)
		ppItems[anIndex]->FindData.lpwszFileNamePart = psz;
	ppItems[anIndex]->FindData.lpwszFileExt = wcsrchr(ppItems[anIndex]->FindData.lpwszFileNamePart, L'.');
	// Description
	psz += wcslen(psz)+1;
	if (asDesc)
		lstrcpy(psz, asDesc);
	else
		psz[0] = 0;
	ppItems[anIndex]->pszDescription = psz;

	return TRUE;
}
