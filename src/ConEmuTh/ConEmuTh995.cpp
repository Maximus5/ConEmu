
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

#include <windows.h>
#include "..\common\pluginW1007.hpp" // Отличается от 995 наличием SynchoApi
#include "ConEmuTh.h"

#define FCTL_GETPANELDIR FCTL_GETCURRENTDIRECTORY

#ifdef _DEBUG
#define SHOW_DEBUG_EVENTS
#endif

struct PluginStartupInfo *InfoW995=NULL;
struct FarStandardFunctions *FSFW995=NULL;


void SetStartupInfoW995(void *aInfo)
{
	::InfoW995 = (PluginStartupInfo*)calloc(sizeof(PluginStartupInfo),1);
	::FSFW995 = (FarStandardFunctions*)calloc(sizeof(FarStandardFunctions),1);
	if (::InfoW995 == NULL || ::FSFW995 == NULL)
		return;
	*::InfoW995 = *((struct PluginStartupInfo*)aInfo);
	*::FSFW995 = *((struct PluginStartupInfo*)aInfo)->FSF;
	::InfoW995->FSF = ::FSFW995;

	int nLen = lstrlenW(InfoW995->RootKey)+16;
	if (gszRootKey) free(gszRootKey);
	gszRootKey = (wchar_t*)calloc(nLen,2);
	lstrcpyW(gszRootKey, InfoW995->RootKey);
	WCHAR* pszSlash = gszRootKey+lstrlenW(gszRootKey)-1;
	if (*pszSlash != L'\\') *(++pszSlash) = L'\\';
	lstrcpyW(pszSlash+1, L"ConEmuTh\\");
}

void ExitFARW995(void)
{
	if (InfoW995) {
		free(InfoW995);
		InfoW995=NULL;
	}
	if (FSFW995) {
		free(FSFW995);
		FSFW995=NULL;
	}
}

int ShowMessage995(LPCWSTR asMsg, int aiButtons)
{
	if (!InfoW995 || !InfoW995->Message)
		return -1;
	return InfoW995->Message(InfoW995->ModuleNumber, FMSG_ALLINONE|FMSG_MB_OK|FMSG_WARNING, NULL, 
		(const wchar_t * const *)asMsg, 0, aiButtons);
}

int ShowMessage995(int aiMsg, int aiButtons)
{
	if (!InfoW995 || !InfoW995->Message || !InfoW995->GetMsg)
		return -1;
	return ShowMessage995(
		(LPCWSTR)InfoW995->GetMsg(InfoW995->ModuleNumber,aiMsg), aiButtons);
}

LPCWSTR GetMsg995(int aiMsg)
{
	if (!InfoW995 || !InfoW995->GetMsg)
		return L"";
	return InfoW995->GetMsg(InfoW995->ModuleNumber,aiMsg);
}

// Warning, напрямую НЕ вызывать. Пользоваться "общей" PostMacro
void PostMacro995(wchar_t* asMacro)
{
	if (!InfoW995 || !InfoW995->AdvControl)
		return;

	ActlKeyMacro mcr;
	mcr.Command = MCMD_POSTMACROSTRING;
	mcr.Param.PlainText.Flags = 0; // По умолчанию - вывод на экран разрешен
	if (*asMacro == L'@' && asMacro[1] && asMacro[1] != L' ') {
		mcr.Param.PlainText.Flags |= KSFLAGS_DISABLEOUTPUT;
		asMacro ++;
	}
	mcr.Param.PlainText.SequenceText = asMacro;
	InfoW995->AdvControl(InfoW995->ModuleNumber, ACTL_KEYMACRO, (void*)&mcr);
}

int ShowPluginMenu995()
{
	if (!InfoW995)
		return -1;

	FarMenuItemEx items[] = {
		{ghConEmuRoot ? 0 : MIF_DISABLE,  InfoW995->GetMsg(InfoW995->ModuleNumber,CEMenuThumbnails)},
		{ghConEmuRoot ? 0 : MIF_DISABLE,  InfoW995->GetMsg(InfoW995->ModuleNumber,CEMenuTiles)},
	};
	int nCount = sizeof(items)/sizeof(items[0]);

	CeFullPanelInfo* pi = IsThumbnailsActive(TRUE);
	if (!pi) {
		items[0].Flags |= MIF_SELECTED;
	} else {
		if (pi->PVM == pvm_Thumbnails) {
			items[0].Flags |= MIF_SELECTED|MIF_CHECKED;
		} else if (pi->PVM == pvm_Tiles) {
			items[1].Flags |= MIF_SELECTED|MIF_CHECKED;
		} else {
			items[0].Flags |= MIF_SELECTED;
		}
	}

	int nRc = InfoW995->Menu(InfoW995->ModuleNumber, -1,-1, 0, 
		FMENU_USEEXT|FMENU_AUTOHIGHLIGHT|FMENU_CHANGECONSOLETITLE|FMENU_WRAPMODE,
		InfoW995->GetMsg(InfoW995->ModuleNumber,2),
		NULL, NULL, NULL, NULL, (FarMenuItem*)items, nCount);

	return nRc;
}

BOOL IsMacroActive995()
{
	if (!InfoW995) return FALSE;

	ActlKeyMacro akm = {MCMD_GETSTATE};
	INT_PTR liRc = InfoW995->AdvControl(InfoW995->ModuleNumber, ACTL_KEYMACRO, &akm);
	if (liRc == MACROSTATE_NOMACRO)
		return FALSE;
	return TRUE;
}


void LoadPanelItemInfo995(CeFullPanelInfo* pi, int nItem)
{
	HANDLE hPanel = pi->hPanel;
	INT_PTR nSize = InfoW995->Control(hPanel, FCTL_GETPANELITEM, nItem, NULL);
	//PluginPanelItem *ppi = (PluginPanelItem*)malloc(nMaxSize);
	//if (!ppi)
	//	return;
	
	if ((pi->pFarTmpBuf == NULL) || (pi->nFarTmpBuf < nSize)) {
		if (pi->pFarTmpBuf) free(pi->pFarTmpBuf);
		pi->nFarTmpBuf = (int)(nSize+MAX_PATH); // + про запас немножко
		pi->pFarTmpBuf = malloc(pi->nFarTmpBuf);
	}
	PluginPanelItem *ppi = (PluginPanelItem*)pi->pFarTmpBuf;
	if (ppi) {
		nSize = InfoW995->Control(hPanel, FCTL_GETPANELITEM, nItem, (LONG_PTR)ppi);
	} else {
		return;
	}
	
	if (!nSize) { // ошибка?
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
		ppi->UserData,
		ppi->Flags,
		ppi->NumberOfLinks);
	
	// ppi не освобождаем - это ссылка на pi->pFarTmpBuf


	//// Необходимый размер буфера для хранения элемента
	//nSize = sizeof(CePluginPanelItem)
	//	+(wcslen(ppi->FindData.lpwszFileName)+1)*2
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
	//pi->ppItems[nItem]->FindData.dwFileAttributes = ppi->FindData.dwFileAttributes;
	//pi->ppItems[nItem]->FindData.ftLastWriteTime = ppi->FindData.ftLastWriteTime;
	//pi->ppItems[nItem]->FindData.nFileSize = ppi->FindData.nFileSize;
	//wchar_t* psz = (wchar_t*)(pi->ppItems[nItem]+1);
	//lstrcpy(psz, ppi->FindData.lpwszFileName);
	//pi->ppItems[nItem]->FindData.lpwszFileName = psz;
	//pi->ppItems[nItem]->FindData.lpwszFileNamePart = wcsrchr(psz, L'\\');
	//if (pi->ppItems[nItem]->FindData.lpwszFileNamePart == NULL)
	//	pi->ppItems[nItem]->FindData.lpwszFileNamePart = psz;
	//pi->ppItems[nItem]->FindData.lpwszFileExt = wcsrchr(pi->ppItems[nItem]->FindData.lpwszFileNamePart, L'.');
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

BOOL LoadPanelInfo995(BOOL abActive)
{
	if (!InfoW995) return FALSE;


	CeFullPanelInfo* pcefpi = NULL;
	PanelInfo pi = {0};
	HANDLE hPanel = abActive ? PANEL_ACTIVE : PANEL_PASSIVE;
	int nRc = InfoW995->Control(hPanel, FCTL_GETPANELINFO, 0, (LONG_PTR)&pi);
	if (!nRc) {
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
	pcefpi->hPanel = hPanel;
	

	// Если элементов на панели стало больше, чем выделено в (pviLeft/pviRight)
	if (pcefpi->ItemsNumber < pi.ItemsNumber) {
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
	pcefpi->Visible = pi.Visible;
	pcefpi->ShortNames = pi.ShortNames;
	pcefpi->Focus = pi.Focus;
	pcefpi->Flags = pi.Flags; // CEPANELINFOFLAGS
	pcefpi->PanelMode = pi.ViewMode;
	pcefpi->IsFilePanel = (pi.PanelType == PTYPE_FILEPANEL);


	// Настройки интерфейса
	pcefpi->nFarInterfaceSettings = gnFarInterfaceSettings =
		(DWORD)InfoW995->AdvControl(InfoW995->ModuleNumber, ACTL_GETINTERFACESETTINGS, 0);
	pcefpi->nFarPanelSettings = gnFarPanelSettings =
		(DWORD)InfoW995->AdvControl(InfoW995->ModuleNumber, ACTL_GETPANELSETTINGS, 0);

	
	// Цвета фара
	int nColorSize = (int)InfoW995->AdvControl(InfoW995->ModuleNumber, ACTL_GETARRAYCOLOR, NULL);
	if ((pcefpi->nFarColors == NULL) || (nColorSize > pcefpi->nMaxFarColors)) {
		if (pcefpi->nFarColors) free(pcefpi->nFarColors);
		pcefpi->nFarColors = (BYTE*)calloc(nColorSize,1);
		pcefpi->nMaxFarColors = nColorSize;
	}
	nColorSize = (int)InfoW995->AdvControl(InfoW995->ModuleNumber, ACTL_GETARRAYCOLOR, pcefpi->nFarColors);

	
	// Текущая папка панели
	int nSize = (int)InfoW995->Control(hPanel, FCTL_GETPANELDIR, 0, 0);
	if (nSize) {
		if ((pcefpi->nMaxPanelDir == NULL) || (nSize > pcefpi->nMaxPanelDir)) {
			pcefpi->nMaxPanelDir = nSize + MAX_PATH; // + выделим немножко заранее
			pcefpi->pszPanelDir = (wchar_t*)calloc(pcefpi->nMaxPanelDir,2);
		}
		nSize = InfoW995->Control(hPanel, FCTL_GETPANELDIR, nSize, (LONG_PTR)pcefpi->pszPanelDir);
		if (!nSize) {
			free(pcefpi->pszPanelDir); pcefpi->pszPanelDir = NULL;
			pcefpi->nMaxPanelDir = 0;
		}
	} else {
		if (pcefpi->pszPanelDir) { free(pcefpi->pszPanelDir); pcefpi->pszPanelDir = NULL; }
	}
	

	// Готовим буфер для информации об элементах
	if ((pcefpi->ppItems == NULL) || (pcefpi->nMaxItemsNumber < pcefpi->ItemsNumber)) {
		if (pcefpi->ppItems) free(pcefpi->ppItems);
		pcefpi->nMaxItemsNumber = pcefpi->ItemsNumber+32; // + немножно про запас
		pcefpi->ppItems = (CePluginPanelItem**)calloc(pcefpi->nMaxItemsNumber, sizeof(LPVOID));
	}
	// и буфер для загрузки элемента из FAR
	nSize = sizeof(PluginPanelItem)+6*MAX_PATH;
	if ((pcefpi->pFarTmpBuf == NULL) || (pcefpi->nFarTmpBuf < nSize)) {
		if (pcefpi->pFarTmpBuf) free(pcefpi->pFarTmpBuf);
		pcefpi->nFarTmpBuf = nSize;
		pcefpi->pFarTmpBuf = malloc(pcefpi->nFarTmpBuf);
	}
	
	return TRUE;
}

void ReloadPanelsInfo995()
{
	if (!InfoW995) return;

	// в FAR2 все просто
	LoadPanelInfo995(TRUE);
	LoadPanelInfo995(FALSE);
}

//BOOL IsLeftPanelActive995()
//{
//	WARNING("TODO: IsLeftPanelActive995");
//	return TRUE;
//}

BOOL CheckPanelSettings995(BOOL abSilence)
{
	if (!InfoW995)
		return FALSE;

	gnFarPanelSettings =
		(DWORD)InfoW995->AdvControl(InfoW995->ModuleNumber, ACTL_GETPANELSETTINGS, 0);
	gnFarInterfaceSettings =
		(DWORD)InfoW995->AdvControl(InfoW995->ModuleNumber, ACTL_GETINTERFACESETTINGS, 0);

	if (!(gnFarPanelSettings & FPS_SHOWCOLUMNTITLES))
	{
		// Для корректного определения положения колонок необходим один из флажков в настройке панели:
		// [x] Показывать заголовки колонок [x] Показывать суммарную информацию
		if (!abSilence)
		{
			InfoW995->Message(InfoW995->ModuleNumber, FMSG_ALLINONE|FMSG_MB_OK|FMSG_WARNING|FMSG_LEFTALIGN, NULL, 
				(const wchar_t * const *)InfoW995->GetMsg(InfoW995->ModuleNumber,CEInvalidPanelSettings), 0, 0);
		}
		return FALSE;
	}
	return TRUE;
}

void ExecuteInMainThread995(ConEmuThSynchroArg* pCmd)
{
	if (!InfoW995) return;

	InfoW995->AdvControl(InfoW995->ModuleNumber, ACTL_SYNCHRO, pCmd);
}

// Возникали проблемы с синхронизацией в FAR2 -> FindFile
//bool CheckWindows995()
//{
//	if (!InfoW995 || !InfoW995->AdvControl) return false;
//	
//	bool lbPanelsActive = false;
//	int nCount = (int)InfoW995->AdvControl(InfoW995->ModuleNumber, ACTL_GETWINDOWCOUNT, NULL);
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
//	//		wsprintfW(szInfo, L"%s%i: {%s-%s} %s\n",
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
//	//		wsprintfW(szInfo, L"%i: <window absent>\n", i);
//	//	}
//	//	OutputDebugStringW(szInfo);
//	//}
//	
//	return lbPanelsActive;
//}
