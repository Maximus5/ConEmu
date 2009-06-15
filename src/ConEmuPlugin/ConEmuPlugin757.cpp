#include <windows.h>
#include "..\common\common.hpp"
#include "..\common\pluginW757.hpp"
#include "PluginHeader.h"


//#define FAR757

#ifdef FAR757
struct PluginStartupInfo *InfoW757=NULL;
struct FarStandardFunctions *FSFW757=NULL;
#endif



void ProcessDragFrom757()
{
#ifdef FAR757
	if (!InfoW757 || !InfoW757->AdvControl)
		return;

	WindowInfo WInfo;				
    WInfo.Pos=0;
	InfoW757->AdvControl(InfoW757->ModuleNumber, ACTL_GETSHORTWINDOWINFO, (void*)&WInfo);
	if (!WInfo.Current)
	{
		int ItemsCount=0;
		//WriteFile(hPipe, &ItemsCount, sizeof(int), &cout, NULL);
		OutDataAlloc(sizeof(ItemsCount));
		OutDataWrite(&ItemsCount,sizeof(ItemsCount));
		return;
	}

	PanelInfo PInfo;
	WCHAR *szCurDir=gszDir1; szCurDir[0]=0; //(WCHAR*)calloc(0x400,sizeof(WCHAR));
	InfoW757->Control(PANEL_ACTIVE, FCTL_GETPANELINFO, NULL, (LONG_PTR)&PInfo);
	if ((PInfo.PanelType == PTYPE_FILEPANEL || PInfo.PanelType == PTYPE_TREEPANEL) && PInfo.Visible)
	{
		InfoW757->Control(PANEL_ACTIVE, FCTL_GETCURRENTDIRECTORY, 0x400, (LONG_PTR)szCurDir);
		int nDirLen=0, nDirNoSlash=0;
		if (szCurDir[0])
		{
			nDirLen=lstrlen(szCurDir);
			if (nDirLen>0)
				if (szCurDir[nDirLen-1]!=L'\\')
					nDirNoSlash=1;
		}

		// Это только предполагаемый размер, при необходимости он будет увеличен
		OutDataAlloc(sizeof(int)+PInfo.SelectedItemsNumber*((MAX_PATH+2)+sizeof(int)));

		//Maximus5 - новый формат передачи
		int nNull=0; // ItemsCount
		//WriteFile(hPipe, &nNull, sizeof(int), &cout, NULL);
		OutDataWrite(&nNull/*ItemsCount*/, sizeof(int));
		
		if (PInfo.SelectedItemsNumber>0)
		{
			PluginPanelItem *pi = (PluginPanelItem*)calloc(PInfo.SelectedItemsNumber, sizeof(PluginPanelItem));
			bool *bIsFull = (bool*)calloc(PInfo.SelectedItemsNumber, sizeof(bool));
			int ItemsCount=PInfo.SelectedItemsNumber, i;

			int nMaxLen=MAX_PATH+1, nWholeLen=1;

			// сначала посчитать максимальную длину буфера
			for (i=0;i<ItemsCount;i++)
			{
				if (!InfoW757->Control(PANEL_ACTIVE, FCTL_GETSELECTEDPANELITEM, i, (LONG_PTR)&(pi[i])))
					continue;

				int nLen=nDirLen+nDirNoSlash;
				if ((pi[i].FindData.lpwszFileName[0] == L'\\' && pi[i].FindData.lpwszFileName[1] == L'\\') ||
				    (ISALPHA(pi[i].FindData.lpwszFileName[0]) && pi[i].FindData.lpwszFileName[1] == L':' && pi[i].FindData.lpwszFileName[2] == L'\\'))
				    { nLen = 0; bIsFull[i] = TRUE; } // это уже полный путь!
				nLen += lstrlenW(pi[i].FindData.lpwszFileName);
				if (nLen>nMaxLen)
					nMaxLen = nLen;
				nWholeLen += (nLen+1);
			}
			//WriteFile(hPipe, &nWholeLen, sizeof(int), &cout, NULL);
			OutDataWrite(&nWholeLen, sizeof(int));

			WCHAR* Path=new WCHAR[nMaxLen+1];

			for (i=0;i<ItemsCount;i++)
			{
				//WCHAR Path[MAX_PATH+1];
				//ZeroMemory(Path, MAX_PATH+1);
				//Maximus5 - засада с корнем диска и возможностью overflow
				//wsprintf(Path, L"%s\\%s", szCurDir, PInfo.SelectedItems[i]->FindData.lpwszFileName);
				Path[0]=0;

				if (!pi[i].FindData.lpwszFileName) continue; //этот элемент получить не удалось

				int nLen=0;
				if (nDirLen>0 && !bIsFull[i]) {
					lstrcpy(Path, szCurDir);
					if (nDirNoSlash) {
						Path[nDirLen]=L'\\';
						Path[nDirLen+1]=0;
					}
					nLen = nDirLen+nDirNoSlash;
				}
				lstrcpy(Path+nLen, pi[i].FindData.lpwszFileName);
				nLen += lstrlen(pi[i].FindData.lpwszFileName);

				nLen++;
				//WriteFile(hPipe, &nLen, sizeof(int), &cout, NULL);
				OutDataWrite(&nLen, sizeof(int));
				//WriteFile(hPipe, Path, sizeof(WCHAR)*nLen, &cout, NULL);
				OutDataWrite(Path, sizeof(WCHAR)*nLen);
			}

			for (i=0;i<ItemsCount;i++)
			{
				InfoW757->Control(PANEL_ACTIVE, FCTL_FREEPANELITEM, 0, (LONG_PTR)&(pi[i]));
			}
			free ( pi ); pi = NULL;

			free(bIsFull);
			delete [] Path; Path=NULL;

			// Конец списка
			//WriteFile(hPipe, &nNull, sizeof(int), &cout, NULL);
			OutDataWrite(&nNull, sizeof(int));
		}
	}
	else
	{
		int ItemsCount=0;
		OutDataWrite(&ItemsCount, sizeof(int));
		OutDataWrite(&ItemsCount, sizeof(int)); // смена формата
	}
	//free(szCurDir);
#endif
}

void ProcessDragTo757()
{
#ifdef FAR757
	if (!InfoW757 || !InfoW757->AdvControl)
		return;

	WindowInfo WInfo;				
    WInfo.Pos=0;
	InfoW757->AdvControl(InfoW757->ModuleNumber, ACTL_GETSHORTWINDOWINFO, (void*)&WInfo);
	if (!WInfo.Current)
	{
		//InfoW757->AdvControl(InfoW757->ModuleNumber, ACTL_FREEWINDOWINFO, (void*)&WInfo);
		int ItemsCount=0;
		//WriteFile(hPipe, &ItemsCount, sizeof(int), &cout, NULL);				
		OutDataAlloc(sizeof(ItemsCount));
		OutDataWrite(&ItemsCount,sizeof(ItemsCount));
		return;
	}
	//InfoW757->AdvControl(InfoW757->ModuleNumber, ACTL_FREEWINDOWINFO, (void*)&WInfo);
	
	PanelInfo PAInfo, PPInfo;
	ForwardedPanelInfo *pfpi=NULL;
	int nStructSize = sizeof(ForwardedPanelInfo)+4; // потом увеличим на длину строк
	//ZeroMemory(&fpi, sizeof(fpi));
	BOOL lbAOK=FALSE, lbPOK=FALSE;
	WCHAR *szPDir=gszDir1; szPDir[0]=0; //(WCHAR*)calloc(0x400,sizeof(WCHAR));
	WCHAR *szADir=gszDir2; szADir[0]=0; //(WCHAR*)calloc(0x400,sizeof(WCHAR));
	
	//if (!(lbAOK=InfoW757->Control(PANEL_ACTIVE, FCTL_GETPANELSHORTINFO, &PAInfo)))
	lbAOK=InfoW757->Control(PANEL_ACTIVE, FCTL_GETPANELINFO, 0, (LONG_PTR)&PAInfo) &&
		  InfoW757->Control(PANEL_ACTIVE, FCTL_GETCURRENTDIRECTORY, 0x400, (LONG_PTR)szADir);
	if (lbAOK && szADir)
		nStructSize += (lstrlen(szADir))*sizeof(WCHAR);

	lbPOK=InfoW757->Control(PANEL_PASSIVE, FCTL_GETPANELINFO, 0, (LONG_PTR)&PPInfo) &&
		  InfoW757->Control(PANEL_PASSIVE, FCTL_GETCURRENTDIRECTORY, 0x400, (LONG_PTR)szPDir);
	if (lbPOK && szPDir)
		nStructSize += (lstrlen(szPDir))*sizeof(WCHAR); // Именно WCHAR! не TCHAR

	pfpi = (ForwardedPanelInfo*)calloc(nStructSize,1);
	if (!pfpi) {
		int ItemsCount=0;
		//WriteFile(hPipe, &ItemsCount, sizeof(int), &cout, NULL);				
		OutDataAlloc(sizeof(ItemsCount));
		OutDataWrite(&ItemsCount,sizeof(ItemsCount));
		return;
	}


	pfpi->ActivePathShift = sizeof(ForwardedPanelInfo);
	pfpi->pszActivePath = (WCHAR*)(((char*)pfpi)+pfpi->ActivePathShift);

	pfpi->PassivePathShift = pfpi->ActivePathShift+2; // если ActivePath заполнится - увеличим

	if (lbAOK)
	{
		pfpi->ActiveRect=PAInfo.PanelRect;
		if (!PAInfo.Plugin && (PAInfo.PanelType == PTYPE_FILEPANEL || PAInfo.PanelType == PTYPE_TREEPANEL) && PAInfo.Visible)
		{
			if (szADir[0]) {
				lstrcpyW(pfpi->pszActivePath, szADir);
				pfpi->PassivePathShift += lstrlenW(pfpi->pszActivePath)*2;
			}
		}
	}

	pfpi->pszPassivePath = (WCHAR*)(((char*)pfpi)+pfpi->PassivePathShift);
	if (lbPOK)
	{
		pfpi->PassiveRect=PPInfo.PanelRect;
		if (!PPInfo.Plugin && (PPInfo.PanelType == PTYPE_FILEPANEL || PPInfo.PanelType == PTYPE_TREEPANEL) && PPInfo.Visible)
		{
			if (szPDir[0])
				lstrcpyW(pfpi->pszPassivePath, szPDir);
		}
	}

	// Собственно, пересылка информации
	//WriteFile(hPipe, &nStructSize, sizeof(nStructSize), &cout, NULL);
	//WriteFile(hPipe, pfpi, nStructSize, &cout, NULL);
	OutDataAlloc(nStructSize+4);
	OutDataWrite(&nStructSize, sizeof(nStructSize));
	OutDataWrite(pfpi, nStructSize);

	free(pfpi); pfpi=NULL;
#endif
}

void SetStartupInfoW757(void *aInfo)
{
#ifdef FAR757
	::InfoW757 = (PluginStartupInfo*)calloc(sizeof(PluginStartupInfo),1);
	::FSFW757 = (FarStandardFunctions*)calloc(sizeof(FarStandardFunctions),1);
	if (::InfoW757 == NULL || ::FSFW757 == NULL)
		return;
	*::InfoW757 = *((struct PluginStartupInfo*)aInfo);
	*::FSFW757 = *((struct PluginStartupInfo*)aInfo)->FSF;
	::InfoW757->FSF = ::FSFW757;
	
	lstrcpynW(gszRootKey, InfoW757->RootKey, MAX_PATH);
	WCHAR* pszSlash = gszRootKey+lstrlenW(gszRootKey)-1;
	if (*pszSlash == L'\\') *(pszSlash--) = 0;
	while (pszSlash>gszRootKey && *pszSlash!=L'\\') pszSlash--;
	*pszSlash = 0;

	/*if (!FarHwnd)

		InitHWND((HWND)InfoW757->AdvControl(InfoW757->ModuleNumber, ACTL_GETFARHWND, 0));*/
#endif
}

extern int lastModifiedStateW;
// watch non-modified -> modified editor status change
int ProcessEditorInputW757(LPCVOID aRec)
{
#ifdef FAR757
	if (!InfoW757)
		return 0;

	const INPUT_RECORD *Rec = (const INPUT_RECORD*)aRec;
	// only key events with virtual codes > 0 are likely to cause status change (?)
	if (Rec->EventType == KEY_EVENT && Rec->Event.KeyEvent.wVirtualKeyCode > 0  && Rec->Event.KeyEvent.bKeyDown)
	{
		EditorInfo ei;
		InfoW757->EditorControl(ECTL_GETINFO, &ei);
		int currentModifiedState = ei.CurState == ECSTATE_MODIFIED ? 1 : 0;
		InfoW757->EditorControl(ECTL_FREEINFO, &ei);
		if (lastModifiedStateW != currentModifiedState)
		{
			// !!! Именно UpdateConEmuTabsW, без версии !!!
			UpdateConEmuTabsW(0, false, false);
			lastModifiedStateW = currentModifiedState;
		}
	}
#endif
	return 0;
}

/*int ProcessEditorEventW757(int Event, void *Param)
{
	switch (Event)
	{
	case EE_CLOSE:
		OUTPUTDEBUGSTRING(L"EE_CLOSE"); break;
	case EE_GOTFOCUS:
		OUTPUTDEBUGSTRING(L"EE_GOTFOCUS"); break;
	case EE_KILLFOCUS:
		OUTPUTDEBUGSTRING(L"EE_KILLFOCUS"); break;
	case EE_SAVE:
		OUTPUTDEBUGSTRING(L"EE_SAVE"); break;
	//case EE_READ: -- в этот момент количество окон еще не изменилось
	default:
		return 0;
	}
	// !!! Именно UpdateConEmuTabsW, без версии !!!
	UpdateConEmuTabsW(Event, Event == EE_KILLFOCUS, Event == EE_SAVE);
	return 0;
}*/

/*int ProcessViewerEventW757(int Event, void *Param)
{
	switch (Event)
	{
	case VE_CLOSE:
	//case VE_READ:
	case VE_KILLFOCUS:
	case VE_GOTFOCUS:
		{
			// !!! Именно UpdateConEmuTabsW, без версии !!!
			UpdateConEmuTabsW(Event, Event == VE_KILLFOCUS, false);
		}
	}
	return 0;
}*/


void UpdateConEmuTabsW757(int event, bool losingFocus, bool editorSave, void* Param/*=NULL*/)
{
#ifdef FAR757
	if (!InfoW757 || !InfoW757->AdvControl)
		return;

    BOOL lbCh = FALSE;
	WindowInfo WInfo;

	int windowCount = (int)InfoW757->AdvControl(InfoW757->ModuleNumber, ACTL_GETWINDOWCOUNT, NULL);
	lbCh = (lastWindowCount != windowCount);
	
	if (!CreateTabs ( windowCount ))
		return;

	EditorInfo ei;
	if (editorSave)
	{
		InfoW757->EditorControl(ECTL_GETINFO, &ei);
	}

	int tabCount = 0;
	for (int i = 0; i < windowCount; i++)
	{
		WInfo.Pos = i;
		InfoW757->AdvControl(InfoW757->ModuleNumber, ACTL_GETWINDOWINFO, (void*)&WInfo);
		if (WInfo.Type == WTYPE_EDITOR || WInfo.Type == WTYPE_VIEWER || WInfo.Type == WTYPE_PANELS)
			lbCh |= AddTab(tabCount, losingFocus, editorSave, 
				WInfo.Type, WInfo.Name, editorSave ? ei.FileName : NULL, 
				WInfo.Current, WInfo.Modified);
		InfoW757->AdvControl(InfoW757->ModuleNumber, ACTL_FREEWINDOWINFO, (void*)&WInfo);
	}
	
	if (editorSave) 
		InfoW757->EditorControl(ECTL_FREEINFO, &ei);

	SendTabs(tabCount, FALSE, lbCh);
#endif
}

void ExitFARW757(void)
{
#ifdef FAR757
	if (InfoW757) {
		free(InfoW757);
		InfoW757=NULL;
	}
	if (FSFW757) {
		free(FSFW757);
		FSFW757=NULL;
	}
#endif
}

int ShowMessage757(int aiMsg, int aiButtons)
{
#ifdef FAR757
	if (!InfoW757 || !InfoW757->Message || !InfoW757->GetMsg)
		return -1;
	return InfoW757->Message(InfoW757->ModuleNumber, FMSG_ALLINONE, NULL, 
		(const wchar_t * const *)InfoW757->GetMsg(InfoW757->ModuleNumber,aiMsg), 0, aiButtons);
#endif
	return 0;
}

LPCWSTR GetMsg757(int aiMsg)
{
#ifdef FAR757
	if (!InfoW757 || !InfoW757->GetMsg)
		return L"";
	return InfoW757->GetMsg(InfoW757->ModuleNumber,aiMsg);
#endif
	return NULL;
}

void ReloadMacro757()
{
#ifdef FAR757
	if (!InfoW757 || !InfoW757->AdvControl)
		return;

	ActlKeyMacro command;
	command.Command=MCMD_LOADALL;
	InfoW757->AdvControl(InfoW757->ModuleNumber,ACTL_KEYMACRO,&command);
#endif
}

void SetWindow757(int nTab)
{
#ifdef FAR757
	if (!InfoW757 || !InfoW757->AdvControl)
		return;

	if (InfoW757->AdvControl(InfoW757->ModuleNumber, ACTL_SETCURRENTWINDOW, (void*)nTab))
		InfoW757->AdvControl(InfoW757->ModuleNumber, ACTL_COMMIT, 0);
#endif
}

void PostMacro757(wchar_t* asMacro)
{
#ifdef FAR757
	if (!InfoW757 || !InfoW757->AdvControl) return;

	ActlKeyMacro mcr;
	mcr.Command = MCMD_POSTMACROSTRING;
	mcr.Param.PlainText.SequenceText = asMacro;
	mcr.Param.PlainText.Flags = KSFLAGS_DISABLEOUTPUT;
	InfoW757->AdvControl(InfoW757->ModuleNumber, ACTL_KEYMACRO, (void*)&mcr);
#endif
}

int ShowPluginMenu757()
{
#ifdef FAR757
	if (!InfoW757)
		return -1;

	FarMenuItem items[] = {
		{(wchar_t*)InfoW757->GetMsg(InfoW757->ModuleNumber,3), 1, 0, 0},
		{(wchar_t*)InfoW757->GetMsg(InfoW757->ModuleNumber,4), 1, 0, 0}
	};
	int nCount = sizeof(items)/sizeof(items[0]);

	int nRc = InfoW757->Menu(InfoW757->ModuleNumber, -1,-1, 0, 
		FMENU_AUTOHIGHLIGHT|FMENU_CHANGECONSOLETITLE|FMENU_WRAPMODE,
		InfoW757->GetMsg(InfoW757->ModuleNumber,2),
		NULL, NULL, NULL, NULL, items, nCount);

	return nRc;
#endif
	return 0;
}

BOOL EditOutput757(LPCWSTR asFileName, BOOL abView)
{
#ifdef FAR757
	if (!InfoW757)
		return FALSE;

	BOOL lbRc = FALSE;
	if (!abView) {
		int iRc =
			InfoW757->Editor(asFileName, InfoW757->GetMsg(InfoW757->ModuleNumber,5), 0,0,-1,-1, 
			EF_NONMODAL|EF_IMMEDIATERETURN|EF_DELETEONLYFILEONCLOSE|EF_ENABLE_F6|EF_DISABLEHISTORY,
			0, 1, 1200);
		lbRc = (iRc != EEC_OPEN_ERROR);
	} else {
		#ifdef _DEBUG
		int iRc =
		#endif
			InfoW757->Viewer(asFileName, InfoW757->GetMsg(InfoW757->ModuleNumber,5), 0,0,-1,-1, 
			VF_NONMODAL|VF_IMMEDIATERETURN|VF_DELETEONLYFILEONCLOSE|VF_ENABLE_F6|VF_DISABLEHISTORY,
			1200);
		lbRc = TRUE;
	}

	return lbRc;
#endif
	return FALSE;
}
