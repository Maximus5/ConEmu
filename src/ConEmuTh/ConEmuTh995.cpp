
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

	lstrcpynW(gszRootKey, InfoW995->RootKey, MAX_PATH);
	WCHAR* pszSlash = gszRootKey+lstrlenW(gszRootKey)-1;
	if (*pszSlash == L'\\') *(pszSlash--) = 0;
	while (pszSlash>gszRootKey && *pszSlash!=L'\\') pszSlash--;
	*pszSlash = 0;
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

int ShowMessage995(int aiMsg, int aiButtons)
{
	if (!InfoW995 || !InfoW995->Message || !InfoW995->GetMsg)
		return -1;
	return InfoW995->Message(InfoW995->ModuleNumber, FMSG_ALLINONE, NULL, 
		(const wchar_t * const *)InfoW995->GetMsg(InfoW995->ModuleNumber,aiMsg), 0, aiButtons);
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

	return 0;

	//FarMenuItemEx items[] = {
	//	{ConEmuHwnd ? MIF_SELECTED : MIF_DISABLE,  InfoW995->GetMsg(InfoW995->ModuleNumber,3)},
	//	{ConEmuHwnd ? 0 : MIF_DISABLE,             InfoW995->GetMsg(InfoW995->ModuleNumber,4)},
	//	{MIF_SEPARATOR},
	//	{ConEmuHwnd ? 0 : MIF_DISABLE,             InfoW995->GetMsg(InfoW995->ModuleNumber,6)},
	//	{ConEmuHwnd ? 0 : MIF_DISABLE,             InfoW995->GetMsg(InfoW995->ModuleNumber,7)},
	//	{ConEmuHwnd ? 0 : MIF_DISABLE,             InfoW995->GetMsg(InfoW995->ModuleNumber,8)},
	//	{ConEmuHwnd ? 0 : MIF_DISABLE,             InfoW995->GetMsg(InfoW995->ModuleNumber,9)},
	//	{MIF_SEPARATOR},
	//	{ConEmuHwnd||IsTerminalMode() ? MIF_DISABLE : MIF_SELECTED,  InfoW995->GetMsg(InfoW995->ModuleNumber,13)},
	//	{MIF_SEPARATOR},
	//	{IsDebuggerPresent()||IsTerminalMode() ? MIF_DISABLE : 0,    InfoW995->GetMsg(InfoW995->ModuleNumber,14)}
	//};
	//int nCount = sizeof(items)/sizeof(items[0]);

	//int nRc = InfoW995->Menu(InfoW995->ModuleNumber, -1,-1, 0, 
	//	FMENU_USEEXT|FMENU_AUTOHIGHLIGHT|FMENU_CHANGECONSOLETITLE|FMENU_WRAPMODE,
	//	InfoW995->GetMsg(InfoW995->ModuleNumber,2),
	//	NULL, NULL, NULL, NULL, (FarMenuItem*)items, nCount);

	//return nRc;
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

CeFullPanelInfo* LoadPanelInfo995()
{
	if (!InfoW995) return NULL;


	CeFullPanelInfo* pcefpi = NULL;
	PanelInfo pi = {0};
	HANDLE hPanel = PANEL_ACTIVE;
	int nRc = InfoW995->Control(hPanel, FCTL_GETPANELINFO, 0, (LONG_PTR)&pi);
	if (!nRc) {
		TODO("Показать информацию об ошибке");
		return NULL;
	}
	// Проверим, что панель видима. Иначе - сразу выходим.
	if (!pi.Visible) {
		TODO("Показать информацию об ошибке");
		return NULL;
	}
	if (pi.Flags & PFLAGS_PANELLEFT)
		pcefpi = &pviLeft;
	else
		pcefpi = &pviRight;
	// На всякий случай - позовем
	pcefpi->FreeInfo();

	// Копируем что нужно
	pcefpi->bLeftPanel = (pi.Flags & PFLAGS_PANELLEFT) == PFLAGS_PANELLEFT;
	pcefpi->bPlugin = pi.Plugin;
	pcefpi->PanelRect = pi.PanelRect;
	pcefpi->ItemsNumber = pi.ItemsNumber;
	pcefpi->CurrentItem = pi.CurrentItem;
	pcefpi->TopPanelItem = pi.TopPanelItem;
	pcefpi->Visible = pi.Visible;
	pcefpi->Focus = pi.Focus;
	pcefpi->Flags = pi.Flags; // CEPANELINFOFLAGS

	// Настройки интерфейса
	pcefpi->nFarInterfaceSettings =
		(DWORD)InfoW995->AdvControl(InfoW995->ModuleNumber, ACTL_GETINTERFACESETTINGS, 0);
	pcefpi->nFarPanelSettings =
		(DWORD)InfoW995->AdvControl(InfoW995->ModuleNumber, ACTL_GETPANELSETTINGS, 0);
		
	// Цвета фара
	INT_PTR nColorSize = InfoW995->AdvControl(InfoW995->ModuleNumber, ACTL_GETARRAYCOLOR, NULL);
	if (nColorSize <= (INT_PTR)sizeof(pcefpi->nFarColors)) {
		nColorSize = InfoW995->AdvControl(InfoW995->ModuleNumber, ACTL_GETARRAYCOLOR, pcefpi->nFarColors);
	} else {
		_ASSERTE(nColorSize <= sizeof(pcefpi->nFarColors));
		BYTE* ptr = (BYTE*)calloc(nColorSize,1);
		if (!ptr) {
			memset(pcefpi->nFarColors, 7, sizeof(pcefpi->nFarColors));
		} else {
			nColorSize = InfoW995->AdvControl(InfoW995->ModuleNumber, ACTL_GETARRAYCOLOR, ptr);
			memmove(pcefpi->nFarColors, ptr, sizeof(pcefpi->nFarColors));
			free(ptr);
		}
	}

	// Текущая папка панели
	INT_PTR nSize = InfoW995->Control(hPanel, FCTL_GETPANELDIR, 0, 0);
	if (nSize) {
		pcefpi->pszPanelDir = (wchar_t*)calloc(nSize,2);
		nSize = InfoW995->Control(hPanel, FCTL_GETPANELDIR, nSize, (LONG_PTR)pcefpi->pszPanelDir);
		if (!nSize) {
			free(pcefpi->pszPanelDir); pcefpi->pszPanelDir = NULL;
		}
	}

	// Теперь - сформируем информацию об элементах панели
	pcefpi->ppItems = (CePluginPanelItem**)calloc(pcefpi->ItemsNumber, sizeof(LPVOID));
	TODO("Оптимизировать - в большинстве случаев наверное не нужно загружать все элементы сразу");
	INT_PTR nMaxSize = sizeof(PluginPanelItem)+6*MAX_PATH;
	nSize = 0;
	PluginPanelItem *ppi = (PluginPanelItem*)malloc(nMaxSize);
	for (int i=0; i<pi.ItemsNumber; i++) {
		nSize = InfoW995->Control(hPanel, FCTL_GETPANELITEM, i, NULL);
		if (nSize > nMaxSize) {
			nMaxSize = nSize+MAX_PATH*2;
			free(ppi);
			ppi = (PluginPanelItem*)malloc(nMaxSize);
		}
		nSize = InfoW995->Control(hPanel, FCTL_GETPANELITEM, i, (LONG_PTR)ppi);
		if (!nSize) continue; // ошибка?
		// Копируем
		nSize = sizeof(CePluginPanelItem)+(wcslen(ppi->FindData.lpwszFileName)+1)*2;
		pcefpi->ppItems[i] = (CePluginPanelItem*)malloc(nSize);
		pcefpi->ppItems[i]->Flags = ppi->Flags;
		pcefpi->ppItems[i]->FindData.dwFileAttributes = ppi->FindData.dwFileAttributes;
		pcefpi->ppItems[i]->FindData.nFileSize = ppi->FindData.nFileSize;
		wchar_t* psz = (wchar_t*)(pcefpi->ppItems[i]+1);
		lstrcpy(psz, ppi->FindData.lpwszFileName);
		pcefpi->ppItems[i]->FindData.lpwszFileName = psz;
		pcefpi->ppItems[i]->FindData.lpwszFileNamePart = wcsrchr(psz, L'\\');
		if (pcefpi->ppItems[i]->FindData.lpwszFileNamePart == NULL)
			pcefpi->ppItems[i]->FindData.lpwszFileNamePart = psz;
		pcefpi->ppItems[i]->FindData.lpwszFileExt = wcsrchr(pcefpi->ppItems[i]->FindData.lpwszFileNamePart, L'.');
	}
	free(ppi);
	
	return pcefpi;
}
