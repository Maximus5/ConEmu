
/*
Copyright (c) 2009-present Maximus5
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


#ifdef _DEBUG
//  Раскомментировать, чтобы сразу после загрузки плагина показать MessageBox, чтобы прицепиться дебаггером
//  #define SHOW_STARTED_MSGBOX
#endif

//#define TRUE_COLORER_OLD_SUPPORT

#define SHOWDEBUGSTR
//#define MCHKHEAP
#define DEBUGSTRMENU(s) //DEBUGSTR(s)
#define DEBUGSTRINPUT(s) //DEBUGSTR(s)
#define DEBUGSTRDLGEVT(s) //DEBUGSTR(s)
#define DEBUGSTRCMD(s) DEBUGSTR(s)
#define DEBUGSTRACTIVATE(s) DEBUGSTR(s)


#include "../common/Common.h"
#include "../ConEmuHk/ConEmuHooks.h"
#ifdef _DEBUG
#pragma warning( disable : 4995 )
#endif
#include "../common/pluginW1761.hpp"
#ifdef _DEBUG
#pragma warning( default : 4995 )
#endif
#include "../common/ConsoleAnnotation.h"
#include "../common/SetEnvVar.h"
#include "../common/WObjects.h"
#include "../common/WConsole.h"
#include "../common/TerminalMode.h"
#include "../common/MFileMapping.h"
#include "../common/MSection.h"
#include "../ConEmu/version.h"
#include "PluginHeader.h"
#include "ConEmuPluginBase.h"
#include "PluginBackground.h"
#include <Tlhelp32.h>

#include "../common/ConEmuCheck.h"
#include "PluginSrv.h"


#if defined(__GNUC__)
extern "C"
#endif
BOOL WINAPI DllMain(HANDLE hModule, DWORD  ul_reason_for_call, LPVOID lpReserved)
{
	switch (ul_reason_for_call)
	{
	case DLL_PROCESS_ATTACH:
		CPluginBase::DllMain_ProcessAttach((HMODULE)hModule);
		break;
	case DLL_PROCESS_DETACH:
		CPluginBase::DllMain_ProcessDetach();
		break;
	}

	return TRUE;
}

#if defined(CRTSTARTUP)
#pragma message("!!!CRTSTARTUP defined!!!")
extern "C" {
	BOOL WINAPI _DllMainCRTStartup(HANDLE hDll,DWORD dwReason,LPVOID lpReserved);
};

BOOL WINAPI _DllMainCRTStartup(HANDLE hDll,DWORD dwReason,LPVOID lpReserved)
{
	DllMain(hDll, dwReason, lpReserved);
	return TRUE;
}
#endif



// Эта функция используется только в ANSI
void WINAPI GetPluginInfo(void *piv)
{
	if (gFarVersion.dwVerMajor != 1)
	{
		gFarVersion.dwVerMajor = 1;
		gFarVersion.dwVerMinor = 75;
	}

	Plugin()->GetPluginInfoPtr(piv);
}

void WINAPI GetPluginInfoW(void *piv)
{
	Plugin()->GetPluginInfoPtr(piv);
}




BOOL WINAPI IsConsoleActive()
{
	if (ghConEmuWndDC)
	{
		if (IsWindow(ghConEmuWndDC))
		{
			HWND hParent = GetParent(ghConEmuWndDC);

			if (hParent)
			{
				HWND hTest = (HWND)GetWindowLongPtr(hParent, GWLP_USERDATA);
				return (hTest == FarHwnd);
			}
		}
	}

	return TRUE;
}

// anConEmuOnly
//	0 - если в ConEmu - вернуть окно отрисовки, иначе - вернуть окно консоли
//	1 - вернуть окно отрисовки
//	2 - вернуть главное окно ConEmu
//	3 - вернуть окно консоли
HWND WINAPI GetFarHWND2(int anConEmuOnly)
{
	// Если просили реальное окно консоли - вернем сразу
	if (anConEmuOnly == 3)
	{
		return FarHwnd;
	}

	if (ghConEmuWndDC)
	{
		if (IsWindow(ghConEmuWndDC))
		{
			if (anConEmuOnly == 2)
				return GetConEmuHWND(1);
			return ghConEmuWndDC;
		}

		//
		ghConEmuWndDC = NULL;
		//
		SetConEmuEnvVar(NULL);
		SetConEmuEnvVarChild(NULL,NULL);
	}

	if (anConEmuOnly)
		return NULL;

	return FarHwnd;
}

HWND WINAPI GetFarHWND()
{
	return GetFarHWND2(FALSE);
}

BOOL WINAPI IsTerminalMode()
{
	return TerminalMode;
}

void WINAPI GetFarVersion(FarVersion* pfv)
{
	if (!pfv)
		return;

	*pfv = gFarVersion;
}

int WINAPI RegisterPanelView(PanelViewInit *ppvi)
{
	if (!ppvi)
	{
		_ASSERTE(ppvi != NULL);
		return -2;
	}

	if (ppvi->cbSize != sizeof(PanelViewInit))
	{
		_ASSERTE(ppvi->cbSize == sizeof(PanelViewInit));
		return -2;
	}

	PanelViewRegInfo *pp = (ppvi->bLeftPanel) ? &gPanelRegLeft : &gPanelRegRight;

	if (ppvi->bRegister)
	{
		pp->pfnPeekPreCall = ppvi->pfnPeekPreCall.f;
		pp->pfnPeekPostCall = ppvi->pfnPeekPostCall.f;
		pp->pfnReadPreCall = ppvi->pfnReadPreCall.f;
		pp->pfnReadPostCall = ppvi->pfnReadPostCall.f;
		pp->pfnWriteCall = ppvi->pfnWriteCall.f;
	}
	else
	{
		pp->pfnPeekPreCall = pp->pfnPeekPostCall = pp->pfnReadPreCall = pp->pfnReadPostCall = NULL;
		pp->pfnWriteCall = NULL;
	}

	pp->bRegister = ppvi->bRegister;
	CESERVER_REQ In;
	int nSize = sizeof(CESERVER_REQ_HDR) + sizeof(In.PVI);
	ExecutePrepareCmd(&In, CECMD_REGPANELVIEW, nSize);
	In.PVI = *ppvi;
	CESERVER_REQ* pOut = ExecuteGuiCmd(FarHwnd, &In, FarHwnd);

	if (!pOut)
	{
		pp->pfnPeekPreCall = pp->pfnPeekPostCall = pp->pfnReadPreCall = pp->pfnReadPostCall = NULL;
		pp->pfnWriteCall = NULL;
		pp->bRegister = FALSE;
		return -3;
	}

	*ppvi = pOut->PVI;
	ExecuteFreeResult(pOut);

	if (ppvi->cbSize == 0)
	{
		pp->pfnPeekPreCall = pp->pfnPeekPostCall = pp->pfnReadPreCall = pp->pfnReadPostCall = NULL;
		pp->pfnWriteCall = NULL;
		pp->bRegister = FALSE;
		return -1;
	}

	return 0;
}



//struct RegisterBackgroundArg gpBgPlugin = NULL;
//int gnBgPluginsCount = 0, gnBgPluginsMax = 0;
//MSection *csBgPlugins = NULL;

int WINAPI RegisterBackground(RegisterBackgroundArg *pbk)
{
	if (!pbk)
	{
		_ASSERTE(pbk != NULL);
		return esbr_InvalidArg;
	}

	if (!gbBgPluginsAllowed)
	{
		_ASSERTE(gbBgPluginsAllowed == TRUE);
		return esbr_PluginForbidden;
	}

	if (pbk->cbSize != sizeof(*pbk))
	{
		_ASSERTE(pbk->cbSize == sizeof(*pbk));
		return esbr_InvalidArgSize;
	}

#ifdef _DEBUG

	if (pbk->Cmd == rbc_Register)
	{
		_ASSERTE(pbk->dwPlaces != 0);
	}

#endif

	if (gpBgPlugin == NULL)
	{
		gpBgPlugin = new CPluginBackground;
	}

	return gpBgPlugin->RegisterSubplugin(pbk);
}

// export
// Возвращает TRUE в случае успешного выполнения
// (удалось активировать главную нить и выполнить в ней функцию CallBack)
// FALSE - в случае ошибки.
int WINAPI SyncExecute(HMODULE ahModule, SyncExecuteCallback_t CallBack, LONG_PTR lParam)
{
	BOOL bResult = FALSE;
	SyncExecuteArg args = {CMD__EXTERNAL_CALLBACK, ahModule, CallBack, lParam};
	bResult = Plugin()->ProcessCommand(CMD__EXTERNAL_CALLBACK, TRUE/*bReqMainThread*/, &args);
	return bResult;
}

// export
// Активировать текущую консоль в ConEmu
int WINAPI ActivateConsole()
{
	CESERVER_REQ In;
	int nSize = sizeof(CESERVER_REQ_HDR) + sizeof(In.ActivateCon);
	ExecutePrepareCmd(&In, CECMD_ACTIVATECON, nSize);
	In.ActivateCon.hConWnd = FarHwnd;
	CESERVER_REQ* pOut = ExecuteGuiCmd(FarHwnd, &In, FarHwnd);

	if (!pOut)
	{
		return FALSE;
	}

	BOOL lbSucceeded = (pOut->ActivateCon.hConWnd == FarHwnd);
	ExecuteFreeResult(pOut);
	return lbSucceeded;
}




void WINAPI SetStartupInfo(void *aInfo)
{
	if (gFarVersion.dwVerMajor != 1)
	{
		gFarVersion.dwVerMajor = 1;
		gFarVersion.dwVerMinor = 75;
	}

	Plugin()->SetStartupInfoPtr(aInfo);

	Plugin()->CommonPluginStartup();
}

void WINAPI SetStartupInfoW(void *aInfo)
{
	#if 0
	HMODULE h = LoadLibrary (L"Kernel32.dll");
	FreeLibrary(h);
	#endif

	Plugin()->SetStartupInfoPtr(aInfo);

	Plugin()->CommonPluginStartup();
}

#define MAKEFARVERSION(major,minor,build) ( ((major)<<8) | (minor) | ((build)<<16))

int WINAPI GetMinFarVersion()
{
	// Однако, FAR2 до сборки 748 не понимал две версии плагина в одном файле
	bool bFar2 = false;

	if (!CPluginBase::LoadFarVersion())
		bFar2 = true;
	else
		bFar2 = gFarVersion.dwVerMajor>=2;

	if (bFar2)
	{
		return MAKEFARVERSION(2,0,MIN_FAR2_BUILD);
	}

	return MAKEFARVERSION(1,71,2470);
}

int WINAPI GetMinFarVersionW()
{
	return MAKEFARVERSION(2,0,MIN_FAR2_BUILD);
}

int WINAPI ProcessSynchroEventW(int Event, void *Param)
{
	return Plugin()->ProcessSynchroEvent(Event, Param);
}

INT_PTR WINAPI ProcessSynchroEventW3(void* p)
{
	return Plugin()->ProcessSynchroEventPtr(p);
}

int WINAPI ProcessEditorEvent(int Event, void *Param)
{
	return Plugin()->ProcessEditorViewerEvent(Event, -1);
}

int WINAPI ProcessEditorEventW(int Event, void *Param)
{
	return Plugin()->ProcessEditorViewerEvent(Event, -1);
}

INT_PTR WINAPI ProcessEditorEventW3(void* p)
{
	return Plugin()->ProcessEditorEventPtr(p);
}

int WINAPI ProcessViewerEvent(int Event, void *Param)
{
	return Plugin()->ProcessEditorViewerEvent(-1, Event);
}

int WINAPI ProcessViewerEventW(int Event, void *Param)
{
	return Plugin()->ProcessEditorViewerEvent(-1, Event);
}

INT_PTR WINAPI ProcessViewerEventW3(void* p)
{
	return Plugin()->ProcessViewerEventPtr(p);
}

// watch non-modified -> modified editor status change
int WINAPI ProcessEditorInput(const INPUT_RECORD *Rec)
{
	// Даже если мы не под эмулятором - просто запомним текущее состояние
	//if (!ghConEmuWndDC) return 0; // Если мы не под эмулятором - ничего
	Plugin()->ProcessEditorInputInternal(*Rec);
	return 0;
}

int WINAPI ProcessEditorInputW(void* Rec)
{
	// Даже если мы не под эмулятором - просто запомним текущее состояние
	//if (!ghConEmuWndDC) return 0; // Если мы не под эмулятором - ничего
	return Plugin()->ProcessEditorInputPtr((LPCVOID)Rec);
}

HANDLE WINAPI OpenPlugin(int OpenFrom,INT_PTR Item)
{
	return Plugin()->OpenPluginCommon(OpenFrom, Item, false);
}

HANDLE WINAPI OpenPluginW(int OpenFrom, INT_PTR Item)
{
	CPluginBase* p = Plugin();
	return p->OpenPluginCommon(OpenFrom, Item, ((OpenFrom & p->of_FromMacro) == p->of_FromMacro));
}

HANDLE WINAPI OpenW(const void* Info)
{
	return Plugin()->Open(Info);
}

void WINAPI ExitFAR(void)
{
	CPluginBase::ShutdownPluginStep(L"ExitFAR");

	Plugin()->ExitFarCommon();
	Plugin()->ExitFar();

	CPluginBase::ShutdownPluginStep(L"ExitFAR - done");
}

void WINAPI ExitFARW(void)
{
	CPluginBase::ShutdownPluginStep(L"ExitFARW");

	Plugin()->ExitFarCommon();
	Plugin()->ExitFar();

	CPluginBase::ShutdownPluginStep(L"ExitFARW - done");
}

void WINAPI ExitFARW3(void*)
{
	CPluginBase::ShutdownPluginStep(L"ExitFARW3");

	Plugin()->ExitFarCommon();
	Plugin()->ExitFar();

	CPluginBase::ShutdownPluginStep(L"ExitFARW3 - done");
}

#include "../common/SetExport.h"
ExportFunc Far3Func[] =
{
	{"ExitFARW", (void*)ExitFARW, (void*)ExitFARW3},
	{"ProcessEditorEventW", (void*)ProcessEditorEventW, (void*)ProcessEditorEventW3},
	{"ProcessViewerEventW", (void*)ProcessViewerEventW, (void*)ProcessViewerEventW3},
	{"ProcessSynchroEventW", (void*)ProcessSynchroEventW, (void*)ProcessSynchroEventW3},
	{NULL}
};

void SetupExportsFar3()
{
	bool lbExportsChanged = ChangeExports( Far3Func, ghPluginModule );
	if (!lbExportsChanged)
	{
		_ASSERTE(lbExportsChanged);
	}
}


/* Используются как extern в ConEmuCheck.cpp */
LPVOID _calloc(size_t nCount,size_t nSize)
{
	return calloc(nCount,nSize);
}
LPVOID _malloc(size_t nCount)
{
	return malloc(nCount);
}
void   _free(LPVOID ptr)
{
	free(ptr);
}
