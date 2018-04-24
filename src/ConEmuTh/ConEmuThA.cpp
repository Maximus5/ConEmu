
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

#include "../common/defines.h"
#include "../common/pluginA.hpp"
#include "../common/plugin_helper.h"
#include "ConEmuTh.h"
#include "../common/farcolor1.hpp"
#include "ImgCache.h"

//#define SHOW_DEBUG_EVENTS

// minimal(?) FAR version 1.71 alpha 4 build 2470
int WINAPI _export GetMinFarVersion(void)
{
	// Однако, FAR2 до сборки 748 не понимал две версии плагина в одном файле
	BOOL bFar2=FALSE;

	if (!LoadFarVersion())
		bFar2 = TRUE;
	else
		bFar2 = gFarVersion.dwVerMajor>=2;

	if (bFar2)
	{
		return MAKEFARVERSION(2,0,748);
	}

	return MAKEFARVERSION(1,71,2470);
}


struct PluginStartupInfo *InfoA=NULL;
struct FarStandardFunctions *FSFA=NULL;
static wchar_t* gszRootKeyA = NULL;


#if defined(__GNUC__)
#ifdef __cplusplus
extern "C" {
#endif
	void WINAPI SetStartupInfo(const struct PluginStartupInfo *aInfo);
#ifdef __cplusplus
};
#endif
#endif

void ReloadResourcesA()
{
	wchar_t szTemp[MAX_PATH];
	lstrcpynW(gsFolder, GetMsgA(CEDirFolder, szTemp), countof(gsFolder)); //-V303
	lstrcpynW(gsSymLink, GetMsgA(CEDirSymLink, szTemp), countof(gsSymLink)); //-V303
	lstrcpynW(gsJunction, GetMsgA(CEDirJunction, szTemp), countof(gsJunction)); //-V303
	lstrcpynW(gsTitleThumbs, GetMsgA(CEColTitleThumbnails, szTemp), countof(gsTitleThumbs)); //-V303
	lstrcpynW(gsTitleTiles, GetMsgA(CEColTitleTiles, szTemp), countof(gsTitleTiles)); //-V303
}

void WINAPI _export SetStartupInfo(const struct PluginStartupInfo *aInfo)
{
	//LoadFarVersion - уже вызван в GetStartupInfo

	INIT_FAR_PSI(::InfoA, ::FSFA, aInfo);

	DWORD nFarVer = 0;
	if (InfoA->AdvControl(InfoA->ModuleNumber, ACTL_GETFARVERSION, &nFarVer))
	{
		if (HIBYTE(LOWORD(nFarVer)) == 1)
		{
			gFarVersion.dwBuild = HIWORD(nFarVer);
			gFarVersion.dwVerMajor = (HIBYTE(LOWORD(nFarVer)));
			gFarVersion.dwVerMinor = (LOBYTE(LOWORD(nFarVer)));
		}
		else
		{
			_ASSERTE(HIBYTE(HIWORD(nFarVer)) == 1);
		}
	}

	int nLen = lstrlenA(InfoA->RootKey)+16; //-V303
	if (gszRootKeyA) free(gszRootKeyA);
	gszRootKeyA = (wchar_t*)calloc(nLen,2); //-V106
	MultiByteToWideChar(CP_OEMCP,0,InfoA->RootKey,-1,gszRootKeyA,nLen);
	WCHAR* pszSlash = gszRootKeyA+lstrlenW(gszRootKeyA)-1; //-V303
	if (*pszSlash != L'\\') *(++pszSlash) = L'\\';
	lstrcpyW(pszSlash+1, L"ConEmuTh\\");

	ReloadResourcesA();
	StartPlugin(FALSE);
}

void WINAPI _export GetPluginInfo(struct PluginInfo *pi)
{
	_ASSERTE(pi->StructSize==0);
	pi->StructSize = sizeof(struct PluginInfo);
	//_ASSERTE(pi->StructSize>0 && (pi->StructSize >= sizeof(*pi)));

	static char *szMenu[1], szMenu1[255];
	szMenu[0]=szMenu1;
	lstrcpynA(szMenu1, InfoA->GetMsg(InfoA->ModuleNumber,CEPluginName), 240); //-V303
	_ASSERTE(pi->StructSize == sizeof(struct PluginInfo));
	pi->Flags = isPreloadByDefault()?PF_PRELOAD:0;
	//pi->DiskMenuStrings = NULL;
	//pi->DiskMenuNumbers = 0;
	pi->PluginMenuStrings = szMenu;
	pi->PluginMenuStringsNumber = 1;
	//pi->PluginConfigStrings = NULL;
	//pi->PluginConfigStringsNumber = 0;
	pi->CommandPrefix = NULL;
	pi->Reserved = 0;
}

HANDLE WINAPI _export OpenPlugin(int OpenFrom,INT_PTR Item)
{
	if (InfoA == NULL)
		return INVALID_HANDLE_VALUE;

	ReloadResourcesA();
	EntryPoint(OpenFrom, Item, false);
	return INVALID_HANDLE_VALUE;
}

void   WINAPI _export ExitFAR(void)
{
	ExitPlugin();

	if (InfoA)
	{
		free(InfoA);
		InfoA=NULL;
	}

	if (FSFA)
	{
		free(FSFA);
		FSFA=NULL;
	}

	if (gszRootKeyA)
	{
		free(gszRootKeyA);
		gszRootKeyA = NULL;
	}
}

int ShowMessageA(LPCSTR asMsg, int aiButtons)
{
	if (!InfoA || !InfoA->Message)
		return -1;

	return InfoA->Message(InfoA->ModuleNumber, FMSG_ALLINONE|FMSG_MB_OK|FMSG_WARNING, NULL,
	                      (const char * const *)asMsg, 0, aiButtons);
}

int ShowMessageA(int aiMsg, int aiButtons)
{
	if (!InfoA || !InfoA->Message || !InfoA->GetMsg)
		return -1;

	return ShowMessageA(
	           (LPCSTR)InfoA->GetMsg(InfoA->ModuleNumber,aiMsg), aiButtons);
}

// Warning, напрямую НЕ вызывать. Пользоваться "общей" PostMacro
void PostMacroA(char* asMacro)
{
	if (!InfoA || !InfoA->AdvControl)
		return;

	ActlKeyMacro mcr;
	mcr.Command = MCMD_POSTMACROSTRING;
	mcr.Param.PlainText.Flags = 0; // По умолчанию - вывод на экран разрешен

	if (*asMacro == '@' && asMacro[1] && asMacro[1] != ' ')
	{
		mcr.Param.PlainText.Flags |= KSFLAGS_DISABLEOUTPUT;
		asMacro ++;
	}

	mcr.Param.PlainText.SequenceText = asMacro;
	InfoA->AdvControl(InfoA->ModuleNumber, ACTL_KEYMACRO, (void*)&mcr);
}

int ShowPluginMenuA()
{
	if (!InfoA)
		return -1;

	FarMenuItemEx items[] =
	{
		{MIF_USETEXTPTR | (ghConEmuRoot ? 0UL : MIF_DISABLE)},
		{MIF_USETEXTPTR | (ghConEmuRoot ? 0UL : MIF_DISABLE)},
		{MIF_USETEXTPTR | (ghConEmuRoot ? 0UL : MIF_DISABLE)},
		{MIF_DISABLE /*MIF_USETEXTPTR | (ghConEmuRoot ? 0 : MIF_DISABLE)*/},
	};
	items[0].Text.TextPtr = InfoA->GetMsg(InfoA->ModuleNumber,CEMenuThumbnails);
	items[1].Text.TextPtr = InfoA->GetMsg(InfoA->ModuleNumber,CEMenuTiles);
	items[2].Text.TextPtr = InfoA->GetMsg(InfoA->ModuleNumber,CEMenuTurnOff);
	items[3].Text.TextPtr = InfoA->GetMsg(InfoA->ModuleNumber,CEMenuIcons);
	int nCount = sizeof(items)/sizeof(items[0]);
	CeFullPanelInfo* pi = GetFocusedThumbnails();

	if (!pi)
	{
		items[0].Flags |= MIF_SELECTED;
	}
	else
	{
		if (pi->PVM == pvm_Thumbnails)
		{
			items[0].Flags |= MIF_SELECTED|MIF_CHECKED;
		}
		else if (pi->PVM == pvm_Tiles)
		{
			items[1].Flags |= MIF_SELECTED|MIF_CHECKED;
		}
		else if (pi->PVM == pvm_Icons)
		{
			items[2].Flags |= MIF_SELECTED|MIF_CHECKED;
		}
		else
		{
			items[0].Flags |= MIF_SELECTED;
		}
	}

	#ifndef _DEBUG
	nCount--;
	#endif

	int nRc = InfoA->Menu(InfoA->ModuleNumber, -1,-1, 0,
	                      FMENU_USEEXT|FMENU_AUTOHIGHLIGHT|FMENU_CHANGECONSOLETITLE|FMENU_WRAPMODE,
	                      InfoA->GetMsg(InfoA->ModuleNumber,2),
	                      NULL, NULL, NULL, NULL, (FarMenuItem*)items, nCount);
	return nRc;
}

const wchar_t* GetMsgA(int aiMsg, wchar_t* rsMsg/*MAX_PATH*/)
{
	if (!rsMsg || !InfoA)
		return L"";

	LPCSTR pszMsg = InfoA->GetMsg(InfoA->ModuleNumber,aiMsg);

	if (pszMsg && *pszMsg)
	{
		int nLen = lstrlenA(pszMsg); //-V303

		if (nLen>=MAX_PATH) nLen = MAX_PATH - 1;

		nLen = MultiByteToWideChar(CP_OEMCP, 0, pszMsg, nLen, rsMsg, MAX_PATH-1);

		if (nLen>=0) rsMsg[nLen] = 0; //-V108
	}
	else
	{
		rsMsg[0] = 0;
	}

	return rsMsg;
}

BOOL IsMacroActiveA()
{
	if (!InfoA) return FALSE;

	ActlKeyMacro akm = {MCMD_GETSTATE};
	INT_PTR liRc = InfoA->AdvControl(InfoA->ModuleNumber, ACTL_KEYMACRO, &akm);

	if (liRc == MACROSTATE_NOMACRO)
		return FALSE;

	return TRUE;
}

void LoadPanelItemInfoA(CeFullPanelInfo* pi, INT_PTR nItem)
{
	// Вся информация считывается целиком по панели!
	return;
}

static void LoadFarSettingsA(CEFarInterfaceSettings* pInterface, CEFarPanelSettings* pPanel)
{
	DWORD nSet;
	
	nSet = (DWORD)InfoA->AdvControl(InfoA->ModuleNumber, ACTL_GETINTERFACESETTINGS, 0);
	if (pInterface)
	{
		pInterface->Raw = nSet;
		_ASSERTE((pInterface->AlwaysShowMenuBar != 0) == ((nSet & FIS_ALWAYSSHOWMENUBAR) != 0));
		_ASSERTE((pInterface->ShowKeyBar != 0) == ((nSet & FIS_SHOWKEYBAR) != 0));
	}
	    
	nSet = (DWORD)InfoA->AdvControl(InfoA->ModuleNumber, ACTL_GETPANELSETTINGS, 0);
	if (pPanel)
	{
		pPanel->Raw = nSet;
		_ASSERTE((pPanel->ShowColumnTitles != 0) == ((nSet & FPS_SHOWCOLUMNTITLES) != 0));
		_ASSERTE((pPanel->ShowStatusLine != 0) == ((nSet & FPS_SHOWSTATUSLINE) != 0));
		_ASSERTE((pPanel->ShowSortModeLetter != 0) == ((nSet & FPS_SHOWSORTMODELETTER) != 0));
	}
}

BOOL LoadPanelInfoA(BOOL abActive)
{
	if (!InfoA) return FALSE;

	CeFullPanelInfo* pcefpi = NULL;
	PanelInfo pi = {};
	int nCmd = abActive ? FCTL_GETPANELINFO : FCTL_GETANOTHERPANELINFO;
	int nRc = InfoA->Control(INVALID_HANDLE_VALUE, nCmd, &pi);

	if (!nRc)
	{
		TODO("Показать информацию об ошибке");
		return FALSE;
	}

	// Даже если невидима - обновить информацию!
	//// Проверим, что панель видима. Иначе - сразу выходим.
	//if (!pi.Visible) {
	//	TODO("Показать информацию об ошибке");
	//	return NULL;
	//}

	if (pi.Flags & PFLAGS_PANELLEFT)
		pcefpi = &pviLeft;
	else
		pcefpi = &pviRight;

	pcefpi->cbSize = sizeof(*pcefpi);
	//pcefpi->hPanel = hPanel;

	// Если элементов на панели стало больше, чем выделено в (pviLeft/pviRight)
	if (pcefpi->ItemsNumber < pi.ItemsNumber) //-V104
	{
		if (!pcefpi->ReallocItems(pi.ItemsNumber))
			return FALSE;
	}

	// Копируем что нужно
	pcefpi->bLeftPanel = (pi.Flags & PFLAGS_PANELLEFT) == PFLAGS_PANELLEFT;
	pcefpi->bPlugin = pi.Plugin;
	pcefpi->PanelRect = pi.PanelRect;
	pcefpi->ItemsNumber = pi.ItemsNumber; //-V101
	pcefpi->CurrentItem = pi.CurrentItem; //-V101
	pcefpi->TopPanelItem = pi.TopPanelItem; //-V101
	pcefpi->Visible = (pi.PanelType == PTYPE_FILEPANEL) && pi.Visible;
	pcefpi->ShortNames = pi.ShortNames;
	pcefpi->Focus = pi.Focus;
	pcefpi->Flags = pi.Flags; // CEPANELINFOFLAGS
	pcefpi->PanelMode = pi.ViewMode;
	pcefpi->IsFilePanel = (pi.PanelType == PTYPE_FILEPANEL);
	// Настройки интерфейса
	LoadFarSettingsA(&pcefpi->FarInterfaceSettings, &pcefpi->FarPanelSettings);
	// Цвета фара
	BYTE FarConsoleColors[0x100];
	INT_PTR nColorSize = InfoA->AdvControl(InfoA->ModuleNumber, ACTL_GETARRAYCOLOR, FarConsoleColors);
#ifdef _DEBUG
	INT_PTR nDefColorSize = COL_LASTPALETTECOLOR;
	_ASSERTE(nColorSize==nDefColorSize);
#endif
	pcefpi->nFarColors[col_PanelText] = FarConsoleColors[COL_PANELTEXT];
	pcefpi->nFarColors[col_PanelSelectedCursor] = FarConsoleColors[COL_PANELSELECTEDCURSOR];
	pcefpi->nFarColors[col_PanelSelectedText] = FarConsoleColors[COL_PANELSELECTEDTEXT];
	pcefpi->nFarColors[col_PanelCursor] = FarConsoleColors[COL_PANELCURSOR];
	pcefpi->nFarColors[col_PanelColumnTitle] = FarConsoleColors[COL_PANELCOLUMNTITLE];
	pcefpi->nFarColors[col_PanelBox] = FarConsoleColors[COL_PANELBOX];
	pcefpi->nFarColors[col_HMenuText] = FarConsoleColors[COL_HMENUTEXT];
	pcefpi->nFarColors[col_WarnDialogBox] = FarConsoleColors[COL_WARNDIALOGBOX];
	pcefpi->nFarColors[col_DialogBox] = FarConsoleColors[COL_DIALOGBOX];
	pcefpi->nFarColors[col_CommandLineUserScreen] = FarConsoleColors[COL_COMMANDLINEUSERSCREEN];
	pcefpi->nFarColors[col_PanelScreensNumber] = FarConsoleColors[COL_PANELSCREENSNUMBER];
	pcefpi->nFarColors[col_KeyBarNum] = FarConsoleColors[COL_KEYBARNUM];


	// Текущая папка панели
	int nSize = lstrlenA(pi.CurDir); //-V303
	pcefpi->nMaxPanelDir = nSize + MAX_PATH; // + выделим немножко заранее
	SafeFree(pcefpi->pszPanelDir);
	pcefpi->pszPanelDir = (wchar_t*)calloc(pcefpi->nMaxPanelDir,2); //-V106
	MultiByteToWideChar(CP_OEMCP, 0, pi.CurDir, nSize+1, pcefpi->pszPanelDir, nSize + MAX_PATH);
	// Готовим буфер для информации об элементах
	pcefpi->ReallocItems(pcefpi->ItemsNumber);

	// Копирование элементов панели в нашу внутреннюю структуру
	wchar_t szName[MAX_PATH+1], szDescription[255];
	LARGE_INTEGER liFileSize;
	PluginPanelItem *ppi = pi.PanelItems;

	for(int i = 0; i < pi.ItemsNumber; i++, ppi++)
	{
		liFileSize.LowPart = ppi->FindData.nFileSizeLow; liFileSize.HighPart = ppi->FindData.nFileSizeHigh;
		MultiByteToWideChar(CP_OEMCP, 0, ppi->FindData.cFileName, -1, szName, countof(szName));

		if (ppi->Description)
			MultiByteToWideChar(CP_OEMCP, 0, ppi->Description, -1, szDescription, countof(szDescription));
		else
			szDescription[0] = 0;

		pcefpi->FarItem2CeItem(i,
		                       szName,
		                       szDescription[0] ? szDescription : NULL,
		                       ppi->FindData.dwFileAttributes,
		                       ppi->FindData.ftLastWriteTime,
		                       liFileSize.QuadPart,
		                       (pcefpi->bPlugin && (pi.Flags & CEPFLAGS_REALNAMES) == 0) /*abVirtualItem*/,
							   NULL,
		                       ppi->UserData,
							   NULL,
		                       ppi->Flags,
		                       ppi->NumberOfLinks);
	}

	return TRUE;
}

void ReloadPanelsInfoA()
{
	if (!InfoA) return;

	// возможно будет подтормаживать?
	LoadPanelInfoA(TRUE);
	LoadPanelInfoA(FALSE);
}

void SetCurrentPanelItemA(BOOL abLeftPanel, INT_PTR anTopItem, INT_PTR anCurItem)
{
	if (!InfoA) return;

	// В Far2 можно быстро проверить валидность индексов
	//HANDLE hPanel = NULL;
	int nCmd = 0;
	PanelInfo piActive = {}, piPassive = {}, *pi = NULL;
	TODO("Проверять текущую видимость панелей?");
	InfoA->Control(INVALID_HANDLE_VALUE,  FCTL_GETPANELSHORTINFO, &piActive);

	if ((piActive.Flags & PFLAGS_PANELLEFT) == (abLeftPanel ? PFLAGS_PANELLEFT : 0))
	{
		pi = &piActive; nCmd = FCTL_REDRAWPANEL;
	}
	else
	{
		InfoA->Control(INVALID_HANDLE_VALUE, FCTL_GETANOTHERPANELSHORTINFO, &piPassive);
		pi = &piPassive; nCmd = FCTL_REDRAWANOTHERPANEL;
	}

	// Проверяем индексы (может фар в процессе обновления панели, и количество элементов изменено?)
	if (pi->ItemsNumber < 1)
		return;

	if ((int)anTopItem >= pi->ItemsNumber)
		anTopItem = pi->ItemsNumber - 1; //-V101

	if ((int)anCurItem >= pi->ItemsNumber)
		anCurItem = pi->ItemsNumber - 1; //-V101

	if (anCurItem < anTopItem)
		anCurItem = anTopItem;

	// Обновляем панель
	PanelRedrawInfo pri = {(int)anCurItem, (int)anTopItem};
	InfoA->Control(INVALID_HANDLE_VALUE, nCmd, &pri);
}

//BOOL IsLeftPanelActiveA()
//{
//	TODO("IsLeftPanelActiveA");
//	return FALSE;
//}

BOOL CheckPanelSettingsA(BOOL abSilence)
{
	if (!InfoA)
		return FALSE;

	LoadFarSettingsA(&gFarInterfaceSettings, &gFarPanelSettings);

	if (!(gFarPanelSettings.ShowColumnTitles))
	{
		// Для корректного определения положения колонок необходим один из флажков в настройке панели:
		// [x] Показывать заголовки колонок [x] Показывать суммарную информацию
		if (!abSilence)
		{
			InfoA->Message(InfoA->ModuleNumber, FMSG_ALLINONE|FMSG_MB_OK|FMSG_WARNING|FMSG_LEFTALIGN, NULL,
			               (const char * const *)InfoA->GetMsg(InfoA->ModuleNumber,CEInvalidPanelSettings), 0, 0);
		}

		return FALSE;
	}

	return TRUE;
}

// Использовать только ACTL_GETSHORTWINDOWINFO. С ней проблем с синхронизацией быть не должно
bool CheckFarPanelsA()
{
	if (!InfoA || !InfoA->AdvControl) return false;

	WindowInfo wi = {-1};
	bool lbPanelsActive = false;
	_ASSERTE(GetCurrentThreadId() == gnMainThreadId);
	INT_PTR iRc = InfoA->AdvControl(InfoA->ModuleNumber, ACTL_GETSHORTWINDOWINFO, (LPVOID)&wi);
	lbPanelsActive = (iRc != 0) && (wi.Type == WTYPE_PANELS);
	return lbPanelsActive;
}

// Возникали проблемы с синхронизацией в FAR2 -> FindFile
//bool CheckWindowsA()
//{
//	if (!InfoA || !InfoA->AdvControl) return false;
//
//	bool lbPanelsActive = false;
//	int nCount = InfoA->AdvControl(InfoA->ModuleNumber, ACTL_GETWINDOWCOUNT, NULL);
//	WindowInfo wi = {0};
//	char szTypeName[64];
//	char szName[MAX_PATH*2];
//	char szInfo[MAX_PATH*4];
//
//	OutputDebugStringW(L"\n\n");
//	// Pos: Номер окна, о котором нужно узнать информацию. Нумерация идет с 0. Pos = -1 вернет информацию о текущем окне.
//	for (int i=-1; i <= nCount; i++) {
//		memset(&wi, 0, sizeof(wi));
//		wi.Pos = i;
//		wi.TypeName[0] = 0;
//		wi.Name[0] = 0;
//		int iRc = InfoA->AdvControl(InfoA->ModuleNumber, ACTL_GETWINDOWINFO, (LPVOID)&wi);
//		if (iRc) {
//			wsprintfA(szInfo, "%s%i: {%s-%s} %s\n",
//				wi.Current ? "*" : "",
//				i,
//				(wi.Type==WTYPE_PANELS) ? "WTYPE_PANELS" :
//				(wi.Type==WTYPE_VIEWER) ? "WTYPE_VIEWER" :
//				(wi.Type==WTYPE_EDITOR) ? "WTYPE_EDITOR" :
//				(wi.Type==WTYPE_DIALOG) ? "WTYPE_DIALOG" :
//				(wi.Type==WTYPE_VMENU)  ? "WTYPE_VMENU"  :
//				(wi.Type==WTYPE_HELP)   ? "WTYPE_HELP"   : "Unknown",
//				szTypeName, szName);
//		} else {
//			wsprintfA(szInfo, "%i: <window absent>\n", i);
//		}
//		OutputDebugStringA(szInfo);
//	}
//
//	return lbPanelsActive;
//}

BOOL SettingsLoadA(LPCWSTR pszName, DWORD* pValue)
{
	_ASSERTE(gszRootKeyA!=NULL);
	return SettingsLoadReg(gszRootKeyA, pszName, pValue);
}
void SettingsSaveA(LPCWSTR pszName, DWORD* pValue)
{
	SettingsSaveReg(gszRootKeyA, pszName, pValue);
}

void SettingsLoadOtherA()
{
	SettingsLoadOther(gszRootKeyA);
}
