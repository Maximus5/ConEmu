
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
#pragma warning( disable : 4995 )
#include "../common/pluginW1761.hpp" // Отличается от 995 наличием SynchoApi
#pragma warning( default : 4995 )
#include "../common/plugin_helper.h"
#include "ConEmuTh.h"
#include "../common/farcolor2.hpp"

//#define FCTL_GETPANELDIR FCTL_GETCURRENTDIRECTORY

//#define _ACTL_GETFARRECT 32

#ifdef _DEBUG
#define SHOW_DEBUG_EVENTS
#endif

struct PluginStartupInfo *InfoW995=NULL;
struct FarStandardFunctions *FSFW995=NULL;
static wchar_t* gszRootKeyW995  = NULL;

void GetPluginInfoW995(void *piv)
{
	PluginInfo *pi = (PluginInfo*)piv;
	//memset(pi, 0, sizeof(PluginInfo));
	_ASSERTE(pi->StructSize==0);
	pi->StructSize = sizeof(struct PluginInfo);
	//_ASSERTE(pi->StructSize>0 && (pi->StructSize >= sizeof(*pi)));

	static WCHAR *szMenu[1], szMenu1[255];
	szMenu[0]=szMenu1;
	lstrcpynW(szMenu1, GetMsgW(CEPluginName), 240); //-V303
	_ASSERTE(pi->StructSize == sizeof(struct PluginInfo));
	pi->Flags = isPreloadByDefault()?PF_PRELOAD:0;
	pi->DiskMenuStrings = NULL;
	//pi->DiskMenuNumbers = 0;
	pi->PluginMenuStrings = szMenu;
	pi->PluginMenuStringsNumber = 1;
	//pi->PluginConfigStrings = NULL;
	//pi->PluginConfigStringsNumber = 0;
	pi->CommandPrefix = 0;
	pi->Reserved = ConEmuTh_SysID; // 'CETh'
}


void SetStartupInfoW995(void *aInfo)
{
	INIT_FAR_PSI(::InfoW995, ::FSFW995, (PluginStartupInfo*)aInfo);

	DWORD nFarVer = 0;
	if (InfoW995->AdvControl(InfoW995->ModuleNumber, ACTL_GETFARVERSION, &nFarVer))
	{
		if (HIBYTE(LOWORD(nFarVer)) == 2)
		{
			gFarVersion.dwBuild = HIWORD(nFarVer);
			gFarVersion.dwVerMajor = (HIBYTE(LOWORD(nFarVer)));
			gFarVersion.dwVerMinor = (LOBYTE(LOWORD(nFarVer)));
		}
		else
		{
			_ASSERTE(HIBYTE(HIWORD(nFarVer)) == 2);
		}
	}

	size_t nLen = lstrlenW(InfoW995->RootKey)+16; //-V303
	if (gszRootKeyW995) free(gszRootKeyW995);
	gszRootKeyW995 = (wchar_t*)calloc(nLen,sizeof(wchar_t));
	lstrcpyW(gszRootKeyW995, InfoW995->RootKey);
	WCHAR* pszSlash = gszRootKeyW995+lstrlenW(gszRootKeyW995)-1; //-V303
	if (*pszSlash != L'\\') *(++pszSlash) = L'\\';
	lstrcpyW(pszSlash+1, L"ConEmuTh\\");
}

extern BOOL gbInfoW_OK;
HANDLE WINAPI _export OpenPluginW(int OpenFrom,INT_PTR Item)
{
	if (!gbInfoW_OK)
		return INVALID_HANDLE_VALUE;

	// Far2 api!
	return OpenPluginWcmn(OpenFrom, Item, ((OpenFrom & OPEN_FROMMACRO) == OPEN_FROMMACRO));
}

void ExitFARW995(void)
{
	if (InfoW995)
	{
		free(InfoW995);
		InfoW995=NULL;
	}

	if (FSFW995)
	{
		free(FSFW995);
		FSFW995=NULL;
	}

	if (gszRootKeyW995)
	{
		free(gszRootKeyW995);
		gszRootKeyW995 = NULL;
	}
}

int ShowMessageW995(LPCWSTR asMsg, int aiButtons)
{
	if (!InfoW995 || !InfoW995->Message)
		return -1;

	return InfoW995->Message(InfoW995->ModuleNumber, FMSG_ALLINONE995|FMSG_MB_OK|FMSG_WARNING, NULL,
	                         (const wchar_t * const *)asMsg, 0, aiButtons);
}

int ShowMessageW995(int aiMsg, int aiButtons)
{
	if (!InfoW995 || !InfoW995->Message || !InfoW995->GetMsg)
		return -1;

	return ShowMessageW995(
	           (LPCWSTR)InfoW995->GetMsg(InfoW995->ModuleNumber,aiMsg), aiButtons);
}

LPCWSTR GetMsgW995(int aiMsg)
{
	if (!InfoW995 || !InfoW995->GetMsg)
		return L"";

	return InfoW995->GetMsg(InfoW995->ModuleNumber,aiMsg);
}

// Warning, напрямую НЕ вызывать. Пользоваться "общей" PostMacro
void PostMacroW995(wchar_t* asMacro)
{
	if (!InfoW995 || !InfoW995->AdvControl)
		return;

	ActlKeyMacro mcr;
	mcr.Command = MCMD_POSTMACROSTRING;
	mcr.Param.PlainText.Flags = 0; // По умолчанию - вывод на экран разрешен

	if (*asMacro == L'@' && asMacro[1] && asMacro[1] != L' ')
	{
		mcr.Param.PlainText.Flags |= KSFLAGS_DISABLEOUTPUT;
		asMacro ++;
	}

	mcr.Param.PlainText.SequenceText = asMacro;
	InfoW995->AdvControl(InfoW995->ModuleNumber, ACTL_KEYMACRO, (void*)&mcr);
}

int ShowPluginMenuW995()
{
	if (!InfoW995)
		return -1;

	FarMenuItemEx items[] =
	{
		{ghConEmuRoot ? 0 : MIF_DISABLE,  InfoW995->GetMsg(InfoW995->ModuleNumber,CEMenuThumbnails)},
		{ghConEmuRoot ? 0 : MIF_DISABLE,  InfoW995->GetMsg(InfoW995->ModuleNumber,CEMenuTiles)},
		{ghConEmuRoot ? 0 : MIF_DISABLE,  InfoW995->GetMsg(InfoW995->ModuleNumber,CEMenuTurnOff)},
		{/*ghConEmuRoot ? 0 :*/ MIF_DISABLE,  InfoW995->GetMsg(InfoW995->ModuleNumber,CEMenuIcons)},
	};
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

	int nRc = InfoW995->Menu(InfoW995->ModuleNumber, -1,-1, 0,
	                         FMENU_USEEXT|FMENU_AUTOHIGHLIGHT|FMENU_CHANGECONSOLETITLE|FMENU_WRAPMODE,
	                         InfoW995->GetMsg(InfoW995->ModuleNumber,2),
	                         NULL, NULL, NULL, NULL, (FarMenuItem*)items, nCount);
	return nRc;
}

BOOL IsMacroActiveW995()
{
	if (!InfoW995) return FALSE;

	ActlKeyMacro akm = {MCMD_GETSTATE};
	INT_PTR liRc = InfoW995->AdvControl(InfoW995->ModuleNumber, ACTL_KEYMACRO, &akm);

	if (liRc == MACROSTATE_NOMACRO)
		return FALSE;

	return TRUE;
}

int GetMacroAreaW995()
{
	#define MCMD_GETAREA 6
	ActlKeyMacro area = {MCMD_GETAREA};
	int nArea = (int)InfoW995->AdvControl(InfoW995->ModuleNumber, ACTL_KEYMACRO, &area);
	return nArea;
}


void LoadPanelItemInfoW995(CeFullPanelInfo* pi, INT_PTR nItem)
{
	//HANDLE hPanel = pi->hPanel;
	HANDLE hPanel = pi->Focus ? PANEL_ACTIVE : PANEL_PASSIVE;
	size_t nSize = InfoW995->Control(hPanel, FCTL_GETPANELITEM, (int)nItem, NULL); //-V107
	//PluginPanelItem *ppi = (PluginPanelItem*)malloc(nMaxSize);
	//if (!ppi)
	//	return;

	if ((pi->pFarTmpBuf == NULL) || (pi->nFarTmpBuf < nSize))
	{
		if (pi->pFarTmpBuf) free(pi->pFarTmpBuf);

		pi->nFarTmpBuf = nSize+MAX_PATH; // + про запас немножко
		pi->pFarTmpBuf = malloc(pi->nFarTmpBuf);
	}

	PluginPanelItem *ppi = (PluginPanelItem*)pi->pFarTmpBuf;

	if (ppi)
	{
		nSize = InfoW995->Control(hPanel, FCTL_GETPANELITEM, (int)nItem, (LONG_PTR)ppi);
	}
	else
	{
		return;
	}

	if (!nSize)  // ошибка?
	{
		// FAR не смог заполнить ppi данными, поэтому накидаем туда нулей, чтобы мусор не рисовать
		ppi->FindData.lpwszFileName = L"???";
		ppi->Flags = 0;
		ppi->NumberOfLinks = 0;
		ppi->FindData.dwFileAttributes = 0;
		ppi->FindData.ftLastWriteTime.dwLowDateTime = ppi->FindData.ftLastWriteTime.dwHighDateTime = 0;
		ppi->FindData.nFileSize = 0;
	}

	// Скопировать данные в наш буфер (функция сама выделит память)
	const wchar_t* pszName = ppi->FindData.lpwszFileName;

	if ((!pszName || !*pszName) && ppi->FindData.lpwszAlternateFileName && *ppi->FindData.lpwszAlternateFileName)
		pszName = ppi->FindData.lpwszAlternateFileName;
	else if (pi->ShortNames && ppi->FindData.lpwszAlternateFileName && *ppi->FindData.lpwszAlternateFileName)
		pszName = ppi->FindData.lpwszAlternateFileName;

	pi->FarItem2CeItem(nItem,
	                   pszName,
	                   ppi->Description,
	                   ppi->FindData.dwFileAttributes,
	                   ppi->FindData.ftLastWriteTime,
	                   ppi->FindData.nFileSize,
	                   (pi->bPlugin && (pi->Flags & CEPFLAGS_REALNAMES) == 0) /*abVirtualItem*/,
					   NULL,
	                   ppi->UserData,
					   NULL,
	                   ppi->Flags,
	                   ppi->NumberOfLinks);
}

static void LoadFarSettingsW995(CEFarInterfaceSettings* pInterface, CEFarPanelSettings* pPanel)
{
	DWORD nSet;

	nSet = (DWORD)InfoW995->AdvControl(InfoW995->ModuleNumber, ACTL_GETINTERFACESETTINGS, 0);
	if (pInterface)
	{
		pInterface->Raw = nSet;
		_ASSERTE((pInterface->AlwaysShowMenuBar != 0) == ((nSet & FIS_ALWAYSSHOWMENUBAR) != 0));
		_ASSERTE((pInterface->ShowKeyBar != 0) == ((nSet & FIS_SHOWKEYBAR) != 0));
	}
	    
	nSet = (DWORD)InfoW995->AdvControl(InfoW995->ModuleNumber, ACTL_GETPANELSETTINGS, 0);
	if (pPanel)
	{
		pPanel->Raw = nSet;
		_ASSERTE((pPanel->ShowColumnTitles != 0) == ((nSet & FPS_SHOWCOLUMNTITLES) != 0));
		_ASSERTE((pPanel->ShowStatusLine != 0) == ((nSet & FPS_SHOWSTATUSLINE) != 0));
		_ASSERTE((pPanel->ShowSortModeLetter != 0) == ((nSet & FPS_SHOWSORTMODELETTER) != 0));
	}
}

BOOL LoadPanelInfo995(BOOL abActive)
{
	if (!InfoW995) return FALSE;

	CeFullPanelInfo* pcefpi = NULL;
	PanelInfo pi = {};
	HANDLE hPanel = abActive ? PANEL_ACTIVE : PANEL_PASSIVE;
	int nRc = InfoW995->Control(hPanel, FCTL_GETPANELINFO, 0, (LONG_PTR)&pi);

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
	if (pcefpi->ItemsNumber < pi.ItemsNumber)
	{
		if (!pcefpi->ReallocItems(pi.ItemsNumber))
			return FALSE;
	}

	// Копируем что нужно
	pcefpi->bLeftPanel = (pi.Flags & PFLAGS_PANELLEFT) == PFLAGS_PANELLEFT;
	pcefpi->bPlugin = pi.Plugin;
	pcefpi->PanelRect = pi.PanelRect;
	pcefpi->ItemsNumber = pi.ItemsNumber;
	pcefpi->CurrentItem = pi.CurrentItem;
	pcefpi->TopPanelItem = pi.TopPanelItem;
	pcefpi->Visible = (pi.PanelType == PTYPE_FILEPANEL) && pi.Visible;
	pcefpi->ShortNames = pi.ShortNames;
	pcefpi->Focus = pi.Focus;
	pcefpi->Flags = pi.Flags; // CEPANELINFOFLAGS
	pcefpi->PanelMode = pi.ViewMode;
	pcefpi->IsFilePanel = (pi.PanelType == PTYPE_FILEPANEL);
	// Настройки интерфейса
	LoadFarSettingsW995(&pcefpi->FarInterfaceSettings, &pcefpi->FarPanelSettings);
	// Цвета фара
	BYTE FarConsoleColors[0x100];
	INT_PTR nColorSize = InfoW995->AdvControl(InfoW995->ModuleNumber, ACTL_GETARRAYCOLOR, FarConsoleColors);
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
	//int nColorSize = InfoW995->AdvControl(InfoW995->ModuleNumber, ACTL_GETARRAYCOLOR, NULL);
	//if ((pcefpi->nFarColors == NULL) || (nColorSize > pcefpi->nMaxFarColors))
	//{
	//	if (pcefpi->nFarColors) free(pcefpi->nFarColors);
	//	pcefpi->nFarColors = (BYTE*)calloc(nColorSize,1);
	//	pcefpi->nMaxFarColors = nColorSize;
	//}
	//nColorSize = InfoW995->AdvControl(InfoW995->ModuleNumber, ACTL_GETARRAYCOLOR, pcefpi->nFarColors);
	
	// Текущая папка панели
	size_t nSize = InfoW995->Control(hPanel, FCTL_GETPANELDIR, 0, 0);

	if (nSize)
	{
		if ((pcefpi->pszPanelDir == NULL) || (nSize > pcefpi->nMaxPanelDir))
		{
			pcefpi->nMaxPanelDir = nSize + MAX_PATH; // + выделим немножко заранее
			SafeFree(pcefpi->pszPanelDir);
			pcefpi->pszPanelDir = (wchar_t*)calloc(pcefpi->nMaxPanelDir,2);
		}

		nSize = InfoW995->Control(hPanel, FCTL_GETPANELDIR, (int)nSize, (LONG_PTR)pcefpi->pszPanelDir);

		if (!nSize)
		{
			free(pcefpi->pszPanelDir); pcefpi->pszPanelDir = NULL;
			pcefpi->nMaxPanelDir = 0;
		}
	}
	else
	{
		if (pcefpi->pszPanelDir) { free(pcefpi->pszPanelDir); pcefpi->pszPanelDir = NULL; }
	}

	// Готовим буфер для информации об элементах
	pcefpi->ReallocItems(pcefpi->ItemsNumber);

	// и буфер для загрузки элемента из FAR
	nSize = sizeof(PluginPanelItem)+6*MAX_PATH;

	if ((pcefpi->pFarTmpBuf == NULL) || (pcefpi->nFarTmpBuf < nSize))
	{
		if (pcefpi->pFarTmpBuf) free(pcefpi->pFarTmpBuf);

		pcefpi->nFarTmpBuf = nSize; //-V101
		pcefpi->pFarTmpBuf = malloc(pcefpi->nFarTmpBuf);
	}

	return TRUE;
}

void ReloadPanelsInfoW995()
{
	if (!InfoW995) return;

	// в FAR2 все просто
	LoadPanelInfo995(TRUE);
	LoadPanelInfo995(FALSE);
}

void SetCurrentPanelItemW995(BOOL abLeftPanel, INT_PTR anTopItem, INT_PTR anCurItem)
{
	if (!InfoW995) return;

	// В Far2 можно быстро проверить валидность индексов
	HANDLE hPanel = NULL;
	PanelInfo piActive = {}, piPassive = {}, *pi = NULL;
	TODO("Проверять текущую видимость панелей?");
	InfoW995->Control(PANEL_ACTIVE,  FCTL_GETPANELINFO, 0, (LONG_PTR)&piActive);

	if ((piActive.Flags & PFLAGS_PANELLEFT) == (abLeftPanel ? PFLAGS_PANELLEFT : 0))
	{
		pi = &piActive; hPanel = PANEL_ACTIVE;
	}
	else
	{
		InfoW995->Control(PANEL_PASSIVE, FCTL_GETPANELINFO, 0, (LONG_PTR)&piPassive);
		pi = &piPassive; hPanel = PANEL_PASSIVE;
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
	InfoW995->Control(hPanel, FCTL_REDRAWPANEL, 0, (LONG_PTR)&pri);
}

//BOOL IsLeftPanelActive995()
//{
//	WARNING("TODO: IsLeftPanelActive995");
//	return TRUE;
//}

BOOL CheckPanelSettingsW995(BOOL abSilence)
{
	if (!InfoW995)
		return FALSE;

	LoadFarSettingsW995(&gFarInterfaceSettings, &gFarPanelSettings);

	if (!(gFarPanelSettings.ShowColumnTitles))
	{
		// Для корректного определения положения колонок необходим один из флажков в настройке панели:
		// [x] Показывать заголовки колонок [x] Показывать суммарную информацию
		if (!abSilence)
		{
			InfoW995->Message(InfoW995->ModuleNumber, FMSG_ALLINONE995|FMSG_MB_OK|FMSG_WARNING|FMSG_LEFTALIGN995, NULL,
			                  (const wchar_t * const *)InfoW995->GetMsg(InfoW995->ModuleNumber,CEInvalidPanelSettings), 0, 0);
		}

		return FALSE;
	}

	return TRUE;
}

void ExecuteInMainThreadW995(ConEmuThSynchroArg* pCmd)
{
	if (!InfoW995) return;

	InfoW995->AdvControl(InfoW995->ModuleNumber, ACTL_SYNCHRO, pCmd);
}

void GetFarRectW995(SMALL_RECT* prcFarRect)
{
	if (!InfoW995) return;

	_ASSERTE(ACTL_GETFARRECT==32); //-V112
	if (!InfoW995->AdvControl(InfoW995->ModuleNumber, ACTL_GETFARRECT, prcFarRect))
	{
		prcFarRect->Left = prcFarRect->Right = prcFarRect->Top = prcFarRect->Bottom = 0;
	}
}

bool CheckFarPanelsW995()
{
	if (!InfoW995 || !InfoW995->AdvControl) return false;

	WindowInfo wi = {-1};
	bool lbPanelsActive = false;

	_ASSERTE(gFarVersion.dwBuild >= 1765);

	enum FARMACROAREA
	{
		MACROAREA_OTHER             = 0,
		MACROAREA_SHELL             = 1,
		MACROAREA_VIEWER            = 2,
		MACROAREA_EDITOR            = 3,
		MACROAREA_DIALOG            = 4,
		MACROAREA_SEARCH            = 5,
		MACROAREA_DISKS             = 6,
		MACROAREA_MAINMENU          = 7,
		MACROAREA_MENU              = 8,
		MACROAREA_HELP              = 9,
		MACROAREA_INFOPANEL         =10,
		MACROAREA_QVIEWPANEL        =11,
		MACROAREA_TREEPANEL         =12,
		MACROAREA_FINDFOLDER        =13,
		MACROAREA_USERMENU          =14,
		MACROAREA_AUTOCOMPLETION    =15,
	};
#define MCMD_GETAREA 6
	ActlKeyMacro area = {MCMD_GETAREA};
	INT_PTR nArea = InfoW995->AdvControl(InfoW995->ModuleNumber, ACTL_KEYMACRO, &area);

	lbPanelsActive = (nArea == MACROAREA_SHELL || nArea == MACROAREA_SEARCH);

	//}
	//else
	//{
	//	_ASSERTE(GetCurrentThreadId() == gnMainThreadId);
	//	INT_PTR iRc = InfoW995->AdvControl(InfoW995->ModuleNumber, ACTL_GETSHORTWINDOWINFO, (LPVOID)&wi);
	//	lbPanelsActive = (iRc != 0) && (wi.Type == WTYPE_PANELS);
	//}
	
	return lbPanelsActive;
}

// Возникали проблемы с синхронизацией в FAR2 -> FindFile
//bool CheckWindows995()
//{
//	if (!InfoW995 || !InfoW995->AdvControl) return false;
//
//	bool lbPanelsActive = false;
//	int nCount = InfoW995->AdvControl(InfoW995->ModuleNumber, ACTL_GETWINDOWCOUNT, NULL);
//	WindowInfo wi = {0};
//
//	wi.Pos = -1;
//	INT_PTR iRc = InfoW995->AdvControl(InfoW995->ModuleNumber, ACTL_GETWINDOWINFO, (LPVOID)&wi);
//	if (wi.Type == WTYPE_PANELS) {
//		lbPanelsActive = (wi.Current != 0);
//	}
//
//	//wchar_t szTypeName[64];
//	//wchar_t szName[MAX_PATH*2];
//	//wchar_t szInfo[MAX_PATH*4];
//	//
//	//OutputDebugStringW(L"\n\n");
//	//// Pos: Номер окна, о котором нужно узнать информацию. Нумерация идет с 0. Pos = -1 вернет информацию о текущем окне.
//	//for (int i=-1; i <= nCount; i++) {
//	//	memset(&wi, 0, sizeof(wi));
//	//	wi.Pos = i;
//	//	wi.TypeName = szTypeName; wi.TypeNameSize = sizeofarray(szTypeName); szTypeName[0] = 0;
//	//	wi.Name = szName; wi.NameSize = sizeofarray(szName); szName[0] = 0;
//	//	INT_PTR iRc = InfoW995->AdvControl(InfoW995->ModuleNumber, ACTL_GETWINDOWINFO, (LPVOID)&wi);
//	//	if (iRc) {
//	//		StringCchPrintf(szInfo, countof(szInfo), L"%s%i: {%s-%s} %s\n",
//	//			wi.Current ? L"*" : L"",
//	//			i,
//	//			(wi.Type==WTYPE_PANELS) ? L"WTYPE_PANELS" :
//	//			(wi.Type==WTYPE_VIEWER) ? L"WTYPE_VIEWER" :
//	//			(wi.Type==WTYPE_EDITOR) ? L"WTYPE_EDITOR" :
//	//			(wi.Type==WTYPE_DIALOG) ? L"WTYPE_DIALOG" :
//	//			(wi.Type==WTYPE_VMENU)  ? L"WTYPE_VMENU"  :
//	//			(wi.Type==WTYPE_HELP)   ? L"WTYPE_HELP"   : L"Unknown",
//	//			szTypeName, szName);
//	//	} else {
//	//		StringCchPrintf(szInfo, countof(szInfo), L"%i: <window absent>\n", i);
//	//	}
//	//	OutputDebugStringW(szInfo);
//	//}
//
//	return lbPanelsActive;
//}

BOOL SettingsLoadW995(LPCWSTR pszName, DWORD* pValue)
{
	_ASSERTE(gszRootKeyW995!=NULL);
	return SettingsLoadReg(gszRootKeyW995, pszName, pValue);
}
void SettingsSaveW995(LPCWSTR pszName, DWORD* pValue)
{
	SettingsSaveReg(gszRootKeyW995, pszName, pValue);
}

void SettingsLoadOtherW995()
{
	SettingsLoadOther(gszRootKeyW995);
}
