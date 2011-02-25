
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
#pragma warning( disable : 4995 )
#include "..\common\pluginW1867.hpp" // Far3
#pragma warning( default : 4995 )
#include "ConEmuTh.h"
#include "../ConEmu/version.h" // Far3

//#define FCTL_GETPANELDIR FCTL_GETCURRENTDIRECTORY

//#define _ACTL_GETFARRECT 32

#ifdef _DEBUG
#define SHOW_DEBUG_EVENTS
#endif

struct PluginStartupInfo *InfoW1867=NULL;
struct FarStandardFunctions *FSFW1867=NULL;

GUID guid_ConEmuTh = { /* bd454d48-448e-46cc-909d-b6cf789c2d65 */
    0xbd454d48,
    0x448e,
    0x46cc,
    {0x90, 0x9d, 0xb6, 0xcf, 0x78, 0x9c, 0x2d, 0x65}
};
GUID guid_ConEmuThPluginMenu = { /* 128414a5-68a2-44d2-b092-c9c5225324e1 */
    0x128414a5,
    0x68a2,
    0x44d2,
    {0xb0, 0x92, 0xc9, 0xc5, 0x22, 0x53, 0x24, 0xe1}
};
//INTERFACENAME = { /* 81b782cf-1d4e-4269-aeca-f3d7ac759363 */
//    0x81b782cf,
//    0x1d4e,
//    0x4269,
//    {0xae, 0xca, 0xf3, 0xd7, 0xac, 0x75, 0x93, 0x63}
//  };
//INTERFACENAME = { /* 857d6089-5fc7-4284-b66a-ce54dfae7efc */
//    0x857d6089,
//    0x5fc7,
//    0x4284,
//    {0xb6, 0x6a, 0xce, 0x54, 0xdf, 0xae, 0x7e, 0xfc}
//  };

void WINAPI GetGlobalInfoW(struct GlobalInfo *Info)
{
	//static wchar_t szTitle[16]; _wcscpy_c(szTitle, L"ConEmu");
	//static wchar_t szDescr[64]; _wcscpy_c(szTitle, L"ConEmu support for Far Manager");
	//static wchar_t szAuthr[64]; _wcscpy_c(szTitle, L"ConEmu.Maximus5@gmail.com");

	Info->StructSize = sizeof(GlobalInfo);
	Info->MinFarVersion = FARMANAGERVERSION;
#define D(N) (1##N-100)
	Info->Version = (MVV_1%100)*10000 + D(MVV_2)*100 + D(MVV_3);
	Info->Guid = guid_ConEmuTh;
	Info->Title = L"ConEmu Panel Views";
	Info->Description = L"Thumbnails and Tiles in ConEmu window";
	Info->Author = L"ConEmu.Maximus5@gmail.com";
}

void GetPluginInfoW1867(void *piv)
{
	PluginInfo *pi = (PluginInfo*)piv;
	memset(pi, 0, sizeof(PluginInfo));

	pi->StructSize = sizeof(struct PluginInfo);

	static wchar_t *szMenu[1], szMenu1[255];
	szMenu[0] = szMenu1;
	lstrcpynW(szMenu1, GetMsgW(CEPluginName), 240);

	pi->Flags = PF_PRELOAD;
	pi->PluginMenu.Guids = &guid_ConEmuThPluginMenu;
	pi->PluginMenu.Strings = szMenu;
	pi->PluginMenu.Count = 1;
}


void SetStartupInfoW1867(void *aInfo)
{
	::InfoW1867 = (PluginStartupInfo*)calloc(sizeof(PluginStartupInfo),1);
	::FSFW1867 = (FarStandardFunctions*)calloc(sizeof(FarStandardFunctions),1);

	if (::InfoW1867 == NULL || ::FSFW1867 == NULL)
		return;

	*::InfoW1867 = *((struct PluginStartupInfo*)aInfo);
	*::FSFW1867 = *((struct PluginStartupInfo*)aInfo)->FSF;
	::InfoW1867->FSF = ::FSFW1867;

	DWORD nFarVer = 0;
	if (InfoW1867->AdvControl(&guid_ConEmuTh, ACTL_GETFARVERSION, &nFarVer))
	{
		if (HIBYTE(HIWORD(nFarVer)) == 3)
		{
			gFarVersion.dwBuild = LOWORD(nFarVer);
			gFarVersion.dwVerMajor = (HIBYTE(HIWORD(nFarVer)));
			gFarVersion.dwVerMinor = (LOBYTE(HIWORD(nFarVer)));
		}
		else
		{
			_ASSERTE(HIBYTE(HIWORD(nFarVer)) == 3);
		}
	}

	//int nLen = lstrlenW(InfoW1867->RootKey)+16;
	//if (gszRootKey) free(gszRootKey);
	//gszRootKey = (wchar_t*)calloc(nLen,2);
	//lstrcpyW(gszRootKey, InfoW1867->RootKey);
	//WCHAR* pszSlash = gszRootKey+lstrlenW(gszRootKey)-1;
	//if (*pszSlash != L'\\') *(++pszSlash) = L'\\';
	//lstrcpyW(pszSlash+1, L"ConEmuTh\\");
}

extern BOOL gbInfoW_OK;
HANDLE WINAPI OpenPanelW(int OpenFrom,const GUID* Guid,INT_PTR Data)
{
	if (!gbInfoW_OK)
		return INVALID_HANDLE_VALUE;

	return OpenPluginWcmn(OpenFrom, Data);
}

void ExitFARW1867(void)
{
	if (InfoW1867)
	{
		free(InfoW1867);
		InfoW1867=NULL;
	}

	if (FSFW1867)
	{
		free(FSFW1867);
		FSFW1867=NULL;
	}
}

int ShowMessage1867(LPCWSTR asMsg, int aiButtons)
{
	if (!InfoW1867 || !InfoW1867->Message)
		return -1;

	return InfoW1867->Message(&guid_ConEmuTh, FMSG_ALLINONE1867|FMSG_MB_OK|FMSG_WARNING, NULL,
	                         (const wchar_t * const *)asMsg, 0, aiButtons);
}

int ShowMessage1867(int aiMsg, int aiButtons)
{
	if (!InfoW1867 || !InfoW1867->Message || !InfoW1867->GetMsg)
		return -1;

	return ShowMessage1867(
	           (LPCWSTR)InfoW1867->GetMsg(&guid_ConEmuTh,aiMsg), aiButtons);
}

LPCWSTR GetMsg1867(int aiMsg)
{
	if (!InfoW1867 || !InfoW1867->GetMsg)
		return L"";

	return InfoW1867->GetMsg(&guid_ConEmuTh,aiMsg);
}

// Warning, напрямую НЕ вызывать. Пользоваться "общей" PostMacro
void PostMacro1867(wchar_t* asMacro)
{
	if (!InfoW1867 || !InfoW1867->AdvControl)
		return;

	//ActlKeyMacro mcr;
	MacroSendMacroText mcr = {sizeof(MacroSendMacroText)};
	//mcr.Command = MCMD_POSTMACROSTRING;
	//mcr.Param.PlainText.Flags = 0; // По умолчанию - вывод на экран разрешен

	if (*asMacro == L'@' && asMacro[1] && asMacro[1] != L' ')
	{
		mcr.Flags |= KMFLAGS_DISABLEOUTPUT;
		asMacro ++;
	}

	mcr.SequenceText = asMacro;
	InfoW1867->MacroControl(INVALID_HANDLE_VALUE, MCTL_SENDSTRING, 0, (INT_PTR)&mcr);
}

int ShowPluginMenu1867()
{
	if (!InfoW1867)
		return -1;

	FarMenuItem items[] =
	{
		{ghConEmuRoot ? 0 : MIF_DISABLE,  InfoW1867->GetMsg(&guid_ConEmuTh,CEMenuThumbnails)},
		{ghConEmuRoot ? 0 : MIF_DISABLE,  InfoW1867->GetMsg(&guid_ConEmuTh,CEMenuTiles)},
	};
	int nCount = sizeof(items)/sizeof(items[0]);
	CeFullPanelInfo* pi = IsThumbnailsActive(TRUE);

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
		else
		{
			items[0].Flags |= MIF_SELECTED;
		}
	}

	int nRc = InfoW1867->Menu(&guid_ConEmuTh, -1,-1, 0,
	                         FMENU_AUTOHIGHLIGHT|FMENU_CHANGECONSOLETITLE|FMENU_WRAPMODE,
	                         InfoW1867->GetMsg(&guid_ConEmuTh,2),
	                         NULL, NULL, NULL, NULL, (FarMenuItem*)items, nCount);
	return nRc;
}

BOOL IsMacroActive1867()
{
	if (!InfoW1867) return FALSE;

	INT_PTR liRc = InfoW1867->MacroControl(INVALID_HANDLE_VALUE, MCTL_GETSTATE, 0, 0);

	if (liRc == MACROSTATE_NOMACRO)
		return FALSE;

	return TRUE;
}


void LoadPanelItemInfo1867(CeFullPanelInfo* pi, int nItem)
{
	//HANDLE hPanel = pi->hPanel;
	HANDLE hPanel = pi->Focus ? PANEL_ACTIVE : PANEL_PASSIVE;
	INT_PTR nSize = InfoW1867->Control(hPanel, FCTL_GETPANELITEM, nItem, NULL);
	//PluginPanelItem *ppi = (PluginPanelItem*)malloc(nMaxSize);
	//if (!ppi)
	//	return;

	if ((pi->pFarTmpBuf == NULL) || (pi->nFarTmpBuf < nSize))
	{
		if (pi->pFarTmpBuf) free(pi->pFarTmpBuf);

		pi->nFarTmpBuf = (int)(nSize+MAX_PATH); // + про запас немножко
		pi->pFarTmpBuf = malloc(pi->nFarTmpBuf);
	}

	PluginPanelItem *ppi = (PluginPanelItem*)pi->pFarTmpBuf;

	if (ppi)
	{
		nSize = InfoW1867->Control(hPanel, FCTL_GETPANELITEM, nItem, (LONG_PTR)ppi);
	}
	else
	{
		return;
	}

	if (!nSize)  // ошибка?
	{
		// FAR не смог заполнить ppi данными, поэтому накидаем туда нулей, чтобы мусор не рисовать
		ppi->FileName = L"???";
		ppi->Flags = 0;
		ppi->NumberOfLinks = 0;
		ppi->FileAttributes = 0;
		ppi->LastWriteTime.dwLowDateTime = ppi->LastWriteTime.dwHighDateTime = 0;
		ppi->FileSize = 0;
	}

	// Скопировать данные в наш буфер (функция сама выделит память)
	const wchar_t* pszName = ppi->FileName;

	if ((!pszName || !*pszName) && ppi->AlternateFileName && *ppi->AlternateFileName)
		pszName = ppi->AlternateFileName;
	else if (pi->ShortNames && ppi->AlternateFileName && *ppi->AlternateFileName)
		pszName = ppi->AlternateFileName;

	pi->FarItem2CeItem(nItem,
	                   pszName,
	                   ppi->Description,
	                   ppi->FileAttributes,
	                   ppi->LastWriteTime,
	                   ppi->FileSize,
	                   (pi->bPlugin && (pi->Flags & CEPFLAGS_REALNAMES) == 0) /*abVirtualItem*/,
	                   ppi->UserData,
	                   ppi->Flags,
	                   ppi->NumberOfLinks);
	// ppi не освобождаем - это ссылка на pi->pFarTmpBuf
	//// Необходимый размер буфера для хранения элемента
	//nSize = sizeof(CePluginPanelItem)
	//	+(wcslen(ppi->FileName)+1)*2
	//	+((ppi->Description ? wcslen(ppi->Description) : 0)+1)*2;
	//
	//// Уже может быть выделено достаточно памяти под этот элемент
	//if ((pi->ppItems[nItem] == NULL) || (pi->ppItems[nItem]->cbSize < (DWORD_PTR)nSize)) {
	//	if (pi->ppItems[nItem]) free(pi->ppItems[nItem]);
	//	nSize += 32;
	//	pi->ppItems[nItem] = (CePluginPanelItem*)calloc(nSize, 1);
	//	pi->ppItems[nItem]->cbSize = (int)nSize;
	//}
	//
	//// Лучше сбросить, чтобы мусор не оставался, да и поля в стуктуру могут добавляться, чтобы не забылось...
	//PRAGMA_ERROR("Если содержимое полей не менялось (атрибуты размеры и пр.), то не обнулять структуру!");
	//// Иначе сбрасываются цвета элементов...
	//memset(((LPBYTE)pi->ppItems[nItem])+sizeof(pi->ppItems[nItem]->cbSize), 0, pi->ppItems[nItem]->cbSize-sizeof(pi->ppItems[nItem]->cbSize));
	//
	//// Копируем
	//if (pi->bPlugin && (pi->Flags & CEPFLAGS_REALNAMES) == 0) {
	//	pi->ppItems[nItem]->bVirtualItem = TRUE;
	//} else {
	//	pi->ppItems[nItem]->bVirtualItem = FALSE;
	//}
	//pi->ppItems[nItem]->UserData = ppi->UserData;
	//pi->ppItems[nItem]->Flags = ppi->Flags;
	//pi->ppItems[nItem]->NumberOfLinks = ppi->NumberOfLinks;
	//pi->ppItems[nItem]->FileAttributes = ppi->FileAttributes;
	//pi->ppItems[nItem]->LastWriteTime = ppi->LastWriteTime;
	//pi->ppItems[nItem]->FindData.nFileSize = ppi->FindData.nFileSize;
	//wchar_t* psz = (wchar_t*)(pi->ppItems[nItem]+1);
	//lstrcpy(psz, ppi->FileName);
	//pi->ppItems[nItem]->FileName = psz;
	//pi->ppItems[nItem]->FileNamePart = wcsrchr(psz, L'\\');
	//if (pi->ppItems[nItem]->FileNamePart == NULL)
	//	pi->ppItems[nItem]->FileNamePart = psz;
	//pi->ppItems[nItem]->FileExt = wcsrchr(pi->ppItems[nItem]->FileNamePart, L'.');
	//// Description
	//psz += wcslen(psz)+1;
	//if (ppi->Description)
	//	lstrcpy(psz, ppi->Description);
	//else
	//	psz[0] = 0;
	//pi->ppItems[nItem]->pszDescription = psz;
	//
	//// ppi не освобождаем - это ссылка на pi->pFarTmpBuf
}

BOOL LoadPanelInfo1867(BOOL abActive)
{
	if (!InfoW1867) return FALSE;

	CeFullPanelInfo* pcefpi = NULL;
	PanelInfo pi = {0};
	HANDLE hPanel = abActive ? PANEL_ACTIVE : PANEL_PASSIVE;
	int nRc = InfoW1867->Control(hPanel, FCTL_GETPANELINFO, 0, (LONG_PTR)&pi);

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
	pcefpi->bPlugin = (pi.Flags & PFLAGS_PLUGIN) == PFLAGS_PLUGIN;
	pcefpi->PanelRect = pi.PanelRect;
	pcefpi->ItemsNumber = pi.ItemsNumber;
	pcefpi->CurrentItem = pi.CurrentItem;
	pcefpi->TopPanelItem = pi.TopPanelItem;
	pcefpi->Visible = (pi.Flags & PFLAGS_VISIBLE) == PFLAGS_VISIBLE;
	pcefpi->ShortNames = (pi.Flags & PFLAGS_ALTERNATIVENAMES) == PFLAGS_ALTERNATIVENAMES;
	pcefpi->Focus = (pi.Flags & PFLAGS_FOCUS) == PFLAGS_FOCUS;
	pcefpi->Flags = pi.Flags; // CEPANELINFOFLAGS
	pcefpi->PanelMode = pi.ViewMode;
	pcefpi->IsFilePanel = (pi.PanelType == PTYPE_FILEPANEL);
	// Настройки интерфейса
	pcefpi->nFarInterfaceSettings = gnFarInterfaceSettings =
	                                    (DWORD)InfoW1867->AdvControl(&guid_ConEmuTh, ACTL_GETINTERFACESETTINGS, 0);
	pcefpi->nFarPanelSettings = gnFarPanelSettings =
	                                (DWORD)InfoW1867->AdvControl(&guid_ConEmuTh, ACTL_GETPANELSETTINGS, 0);
	// Цвета фара
	int nColorSize = (int)InfoW1867->AdvControl(&guid_ConEmuTh, ACTL_GETARRAYCOLOR, NULL);

	if ((pcefpi->nFarColors == NULL) || (nColorSize > pcefpi->nMaxFarColors))
	{
		if (pcefpi->nFarColors) free(pcefpi->nFarColors);

		pcefpi->nFarColors = (BYTE*)calloc(nColorSize,1);
		pcefpi->nMaxFarColors = nColorSize;
	}

	nColorSize = (int)InfoW1867->AdvControl(&guid_ConEmuTh, ACTL_GETARRAYCOLOR, pcefpi->nFarColors);
	// Текущая папка панели
	int nSize = (int)InfoW1867->Control(hPanel, FCTL_GETPANELDIR, 0, 0);

	if (nSize)
	{
		if ((pcefpi->nMaxPanelDir == NULL) || (nSize > pcefpi->nMaxPanelDir))
		{
			pcefpi->nMaxPanelDir = nSize + MAX_PATH; // + выделим немножко заранее
			pcefpi->pszPanelDir = (wchar_t*)calloc(pcefpi->nMaxPanelDir,2);
		}

		nSize = InfoW1867->Control(hPanel, FCTL_GETPANELDIR, nSize, (LONG_PTR)pcefpi->pszPanelDir);

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
	//if ((pcefpi->ppItems == NULL) || (pcefpi->nMaxItemsNumber < pcefpi->ItemsNumber))
	//{
	//	if (pcefpi->ppItems) free(pcefpi->ppItems);
	//	pcefpi->nMaxItemsNumber = pcefpi->ItemsNumber+32; // + немножно про запас
	//	pcefpi->ppItems = (CePluginPanelItem**)calloc(pcefpi->nMaxItemsNumber, sizeof(LPVOID));
	//}
	// и буфер для загрузки элемента из FAR
	nSize = sizeof(PluginPanelItem)+6*MAX_PATH;

	if ((pcefpi->pFarTmpBuf == NULL) || (pcefpi->nFarTmpBuf < nSize))
	{
		if (pcefpi->pFarTmpBuf) free(pcefpi->pFarTmpBuf);

		pcefpi->nFarTmpBuf = nSize;
		pcefpi->pFarTmpBuf = malloc(pcefpi->nFarTmpBuf);
	}

	return TRUE;
}

void ReloadPanelsInfo1867()
{
	if (!InfoW1867) return;

	// в FAR2 все просто
	LoadPanelInfo1867(TRUE);
	LoadPanelInfo1867(FALSE);
}

void SetCurrentPanelItem1867(BOOL abLeftPanel, UINT anTopItem, UINT anCurItem)
{
	if (!InfoW1867) return;

	// В Far2 можно быстро проверить валидность индексов
	HANDLE hPanel = NULL;
	PanelInfo piActive = {0}, piPassive = {0}, *pi = NULL;
	TODO("Проверять текущую видимость панелей?");
	InfoW1867->Control(PANEL_ACTIVE,  FCTL_GETPANELINFO, 0, (LONG_PTR)&piActive);

	if ((piActive.Flags & PFLAGS_PANELLEFT) == (abLeftPanel ? PFLAGS_PANELLEFT : 0))
	{
		pi = &piActive; hPanel = PANEL_ACTIVE;
	}
	else
	{
		InfoW1867->Control(PANEL_PASSIVE, FCTL_GETPANELINFO, 0, (LONG_PTR)&piPassive);
		pi = &piPassive; hPanel = PANEL_PASSIVE;
	}

	// Проверяем индексы (может фар в процессе обновления панели, и количество элементов изменено?)
	if (pi->ItemsNumber < 1)
		return;

	if ((int)anTopItem >= pi->ItemsNumber)
		anTopItem = pi->ItemsNumber - 1;

	if ((int)anCurItem >= pi->ItemsNumber)
		anCurItem = pi->ItemsNumber - 1;

	if (anCurItem < anTopItem)
		anCurItem = anTopItem;

	// Обновляем панель
	PanelRedrawInfo pri = {anCurItem, anTopItem};
	InfoW1867->Control(hPanel, FCTL_REDRAWPANEL, 0, (LONG_PTR)&pri);
}

//BOOL IsLeftPanelActive1867()
//{
//	WARNING("TODO: IsLeftPanelActive1867");
//	return TRUE;
//}

BOOL CheckPanelSettings1867(BOOL abSilence)
{
	if (!InfoW1867)
		return FALSE;

	gnFarPanelSettings =
	    (DWORD)InfoW1867->AdvControl(&guid_ConEmuTh, ACTL_GETPANELSETTINGS, 0);
	gnFarInterfaceSettings =
	    (DWORD)InfoW1867->AdvControl(&guid_ConEmuTh, ACTL_GETINTERFACESETTINGS, 0);

	if (!(gnFarPanelSettings & FPS_SHOWCOLUMNTITLES))
	{
		// Для корректного определения положения колонок необходим один из флажков в настройке панели:
		// [x] Показывать заголовки колонок [x] Показывать суммарную информацию
		if (!abSilence)
		{
			InfoW1867->Message(&guid_ConEmuTh, FMSG_ALLINONE1867|FMSG_MB_OK|FMSG_WARNING|FMSG_LEFTALIGN1867, NULL,
			                  (const wchar_t * const *)InfoW1867->GetMsg(&guid_ConEmuTh,CEInvalidPanelSettings), 0, 0);
		}

		return FALSE;
	}

	return TRUE;
}

void ExecuteInMainThread1867(ConEmuThSynchroArg* pCmd)
{
	if (!InfoW1867) return;

	InfoW1867->AdvControl(&guid_ConEmuTh, ACTL_SYNCHRO, pCmd);
}

void GetFarRect1867(SMALL_RECT* prcFarRect)
{
	if (!InfoW1867) return;

	_ASSERTE(ACTL_GETFARRECT!=32);
	if (!InfoW1867->AdvControl(&guid_ConEmuTh, ACTL_GETFARRECT, prcFarRect))
	{
		prcFarRect->Left = prcFarRect->Right = prcFarRect->Top = prcFarRect->Bottom = 0;
	}
}

// Использовать только ACTL_GETSHORTWINDOWINFO. С ней проблем с синхронизацией быть не должно
bool CheckFarPanels1867()
{
	if (!InfoW1867 || !InfoW1867->AdvControl) return false;

	WindowType wt = {sizeof(WindowType)};
	bool lbPanelsActive = false;
	//_ASSERTE(GetCurrentThreadId() == gnMainThreadId); -- ACTL_GETWINDOWTYPE - thread safe
	INT_PTR iRc = InfoW1867->AdvControl(&guid_ConEmuTh, ACTL_GETWINDOWTYPE, (LPVOID)&wt);
	lbPanelsActive = (iRc != 0) && (wt.Type == WTYPE_PANELS);
	return lbPanelsActive;
}

// Возникали проблемы с синхронизацией в FAR2 -> FindFile
//bool CheckWindows1867()
//{
//	if (!InfoW1867 || !InfoW1867->AdvControl) return false;
//
//	bool lbPanelsActive = false;
//	int nCount = (int)InfoW1867->AdvControl(&guid_ConEmuTh, ACTL_GETWINDOWCOUNT, NULL);
//	WindowInfo wi = {0};
//
//	wi.Pos = -1;
//	INT_PTR iRc = InfoW1867->AdvControl(&guid_ConEmuTh, ACTL_GETWINDOWINFO, (LPVOID)&wi);
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
//	//	INT_PTR iRc = InfoW1867->AdvControl(&guid_ConEmuTh, ACTL_GETWINDOWINFO, (LPVOID)&wi);
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

BOOL SettingsLoadW1867(LPCWSTR pszName, DWORD* pValue)
{
	BOOL lbValue = FALSE;
	FarSettingsCreate sc = {sizeof(FarSettingsCreate), guid_ConEmuTh, INVALID_HANDLE_VALUE};
	FarSettingsItem fsi = {0};
	if (InfoW1867->SettingsControl(INVALID_HANDLE_VALUE, SCTL_CREATE, 0, (INT_PTR)&sc))
	{
		DWORD nValue = *pValue;
		fsi.Name = pszName;
		fsi.Type = FST_DATA;
		fsi.Data.Size = sizeof(nValue); 
		fsi.Data.Data = &nValue;
		if (InfoW1867->SettingsControl(sc.Handle, SCTL_GET, 0, (INT_PTR)&fsi))
			*pValue = nValue;

		InfoW1867->SettingsControl(sc.Handle, SCTL_FREE, 0, 0);
	}
	return lbValue;
}
void SettingsSaveW1867(LPCWSTR pszName, DWORD* pValue)
{
	if (!InfoW1867)
		return;

	FarSettingsCreate sc = {sizeof(FarSettingsCreate), guid_ConEmuTh, INVALID_HANDLE_VALUE};
	FarSettingsItem fsi = {0};
	if (InfoW1867->SettingsControl(INVALID_HANDLE_VALUE, SCTL_CREATE, 0, (INT_PTR)&sc))
	{
		DWORD nValue = *pValue;
		fsi.Name = pszName;
		fsi.Type = FST_DATA;
		fsi.Data.Size = sizeof(nValue); 
		fsi.Data.Data = &nValue;
		InfoW1867->SettingsControl(sc.Handle, SCTL_SET, 0, (INT_PTR)&fsi);

		InfoW1867->SettingsControl(sc.Handle, SCTL_FREE, 0, 0);
	}
}

void SettingsLoadOtherW1867(void)
{
	WARNING("Как появится PanelTabs для Far3 - переделать чтение его настроек в GUID");
}
