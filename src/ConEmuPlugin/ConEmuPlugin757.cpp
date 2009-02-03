/* ****************************************** 
   Changes history 
   Maximus5: убрал все static
****************************************** */

//#include <stdio.h>
#include <windows.h>
//#include <windowsx.h>
//#include <string.h>
//#include <tchar.h>
#include "..\common\common.hpp"
#include "..\common\pluginW757.hpp"
#include "PluginHeader.h"

#ifndef FORWARD_WM_COPYDATA
#define FORWARD_WM_COPYDATA(hwnd, hwndFrom, pcds, fn) \
    (BOOL)(UINT)(DWORD)(fn)((hwnd), WM_COPYDATA, (WPARAM)(hwndFrom), (LPARAM)(pcds))
#endif


struct PluginStartupInfo *InfoW757=NULL;
struct FarStandardFunctions *FSFW757=NULL;
extern HWND ConEmuHwnd;
extern BOOL bWasSetParent;
extern HWND FarHwnd;
extern HANDLE hPipe, hPipeEvent;
extern HANDLE hThread;

extern HWND AtoH(WCHAR *Str, int Len);
extern void UpdateConEmuTabsW(int event, bool losingFocus, bool editorSave);


/*const WCHAR *GetMsgW757(int CompareLng)
{
	return InfoW757->GetMsg(InfoW757->ModuleNumber, CompareLng);
}*/


void ProcessDragFrom757()
{
	DWORD cout;
	WindowInfo WInfo;				
    WInfo.Pos=0;
	InfoW757->AdvControl(InfoW757->ModuleNumber, ACTL_GETSHORTWINDOWINFO, (void*)&WInfo);
	if (!WInfo.Current)
	{
		int ItemsCount=0;
		WriteFile(hPipe, &ItemsCount, sizeof(int), &cout, NULL);				
		return;
	}

	PanelInfo PInfo;
	WCHAR *szCurDir=gszDir1; //(WCHAR*)calloc(0x400,sizeof(WCHAR));
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

		//Maximus5 - новый формат передачи
		int nNull=0; // ItemsCount
		WriteFile(hPipe, &nNull, sizeof(int), &cout, NULL);
		
		if (PInfo.SelectedItemsNumber>0)
		{
			PluginPanelItem *pi = (PluginPanelItem*)calloc(PInfo.SelectedItemsNumber, sizeof(PluginPanelItem));
			int ItemsCount=PInfo.SelectedItemsNumber, i;

			int nMaxLen=MAX_PATH+1, nWholeLen=1;

			// сначала посчитать максимальную длину буфера
			for (i=0;i<ItemsCount;i++)
			{
				if (!InfoW757->Control(PANEL_ACTIVE, FCTL_GETSELECTEDPANELITEM, i, (LONG_PTR)&(pi[i])))
					continue;

				int nLen=nDirLen+nDirNoSlash;
				nLen += lstrlen(pi[i].FindData.lpwszFileName);
				if (nLen>nMaxLen)
					nMaxLen = nLen;
				nWholeLen += (nLen+1);
			}
			WriteFile(hPipe, &nWholeLen, sizeof(int), &cout, NULL);

			WCHAR* Path=new WCHAR[nMaxLen+1];

			for (i=0;i<ItemsCount;i++)
			{
				//WCHAR Path[MAX_PATH+1];
				//ZeroMemory(Path, MAX_PATH+1);
				//Maximus5 - засада с корнем диска и возможностью overflow
				//wsprintf(Path, L"%s\\%s", szCurDir, PInfo.SelectedItems[i]->FindData.lpwszFileName);
				Path[0]=0;

				int nLen=nDirLen+nDirNoSlash;
				nLen += lstrlen(pi[i].FindData.lpwszFileName);

				if (nDirLen>0) {
					lstrcpy(Path, szCurDir);
					if (nDirNoSlash) {
						Path[nDirLen]=L'\\';
						Path[nDirLen+1]=0;
					}
				}
				lstrcpy(Path+nDirLen+nDirNoSlash, pi[i].FindData.lpwszFileName);

				nLen++;
				WriteFile(hPipe, &nLen, sizeof(int), &cout, NULL);
				WriteFile(hPipe, Path, sizeof(WCHAR)*nLen, &cout, NULL);
			}

			for (i=0;i<ItemsCount;i++)
			{
				InfoW757->Control(PANEL_ACTIVE, FCTL_FREEPANELITEM, 0, (LONG_PTR)&(pi[i]));
			}
			free ( pi ); pi = NULL;

			delete Path; Path=NULL;

			// Конец списка
			WriteFile(hPipe, &nNull, sizeof(int), &cout, NULL);
		}
	}
	else
	{
		int ItemsCount=0;
		WriteFile(hPipe, &ItemsCount, sizeof(int), &cout, NULL);
		WriteFile(hPipe, &ItemsCount, sizeof(int), &cout, NULL); // смена формата
	}
	//free(szCurDir);
}

void ProcessDragTo757()
{
	DWORD cout;
	WindowInfo WInfo;				
    WInfo.Pos=0;
	InfoW757->AdvControl(InfoW757->ModuleNumber, ACTL_GETSHORTWINDOWINFO, (void*)&WInfo);
	if (!WInfo.Current)
	{
		//InfoW757->AdvControl(InfoW757->ModuleNumber, ACTL_FREEWINDOWINFO, (void*)&WInfo);
		int ItemsCount=0;
		WriteFile(hPipe, &ItemsCount, sizeof(int), &cout, NULL);				
		return;
	}
	//InfoW757->AdvControl(InfoW757->ModuleNumber, ACTL_FREEWINDOWINFO, (void*)&WInfo);
	
	PanelInfo PAInfo, PPInfo;
	ForwardedPanelInfo *pfpi=NULL;
	int nStructSize = sizeof(ForwardedPanelInfo)+4; // потом увеличим на длину строк
	//ZeroMemory(&fpi, sizeof(fpi));
	BOOL lbAOK=FALSE, lbPOK=FALSE;
	WCHAR *szPDir=gszDir1; //(WCHAR*)calloc(0x400,sizeof(WCHAR));
	WCHAR *szADir=gszDir2; //(WCHAR*)calloc(0x400,sizeof(WCHAR));
	
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


	pfpi->ActivePathShift = sizeof(ForwardedPanelInfo);
	pfpi->pszActivePath = (WCHAR*)(((char*)pfpi)+pfpi->ActivePathShift);

	pfpi->PassivePathShift = pfpi->ActivePathShift+2; // если ActivePath заполнится - увеличим

	if (lbAOK)
	{
		pfpi->ActiveRect=PAInfo.PanelRect;
		if ((PAInfo.PanelType == PTYPE_FILEPANEL || PAInfo.PanelType == PTYPE_TREEPANEL) && PAInfo.Visible)
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
		if ((PPInfo.PanelType == PTYPE_FILEPANEL || PPInfo.PanelType == PTYPE_TREEPANEL) && PPInfo.Visible)
		{
			if (szPDir[0])
				lstrcpyW(pfpi->pszPassivePath, szPDir);
		}
	}

	// Собственно, пересылка информации
	WriteFile(hPipe, &nStructSize, sizeof(nStructSize), &cout, NULL);
	WriteFile(hPipe, pfpi, nStructSize, &cout, NULL);

	free(pfpi); pfpi=NULL;
	//free(szADir);
	//free(szPDir);
}

void SetStartupInfoW757(void *aInfo)
{
	::InfoW757 = (PluginStartupInfo*)calloc(sizeof(PluginStartupInfo),1);
	::FSFW757 = (FarStandardFunctions*)calloc(sizeof(FarStandardFunctions),1);
	*::InfoW757 = *((struct PluginStartupInfo*)aInfo);
	*::FSFW757 = *((struct PluginStartupInfo*)aInfo)->FSF;
	::InfoW757->FSF = ::FSFW757;
	
	ConEmuHwnd = NULL;
	FarHwnd = (HWND)InfoW757->AdvControl(InfoW757->ModuleNumber, ACTL_GETFARHWND, 0);
	ConEmuHwnd = GetAncestor(FarHwnd, GA_PARENT);
	if (ConEmuHwnd != NULL)
	{
		WCHAR className[100];
		GetClassName(ConEmuHwnd, (LPWSTR)className, 100);
		if (FSFW757->LStricmp(className, L"VirtualConsoleClass") != 0 &&
			FSFW757->LStricmp(className, L"VirtualConsoleClassMain") != 0)
		{
			ConEmuHwnd = NULL;
		} else {
			bWasSetParent = TRUE;
		}
	}
}

extern int lastModifiedStateW;
// watch non-modified -> modified editor status change
int ProcessEditorInputW757(LPCVOID aRec)
{
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
	return 0;
}

int ProcessEditorEventW757(int Event, void *Param)
{
	switch (Event)
	{
	case EE_CLOSE:
	case EE_GOTFOCUS:
	case EE_KILLFOCUS:
	case EE_SAVE:
	case EE_READ:
		{
			// !!! Именно UpdateConEmuTabsW, без версии !!!
			UpdateConEmuTabsW(Event, Event == EE_KILLFOCUS, Event == EE_SAVE);
			break;
		}
	}
	return 0;
}

int ProcessViewerEventW757(int Event, void *Param)
{
	switch (Event)
	{
	case VE_CLOSE:
	case VE_READ:
	case VE_KILLFOCUS:
	case VE_GOTFOCUS:
		{
			// !!! Именно UpdateConEmuTabsW, без версии !!!
			UpdateConEmuTabsW(Event, Event == VE_KILLFOCUS, false);
		}
	}
	return 0;
}


void UpdateConEmuTabsW757(int event, bool losingFocus, bool editorSave)
{
	WindowInfo WInfo;

	int windowCount = (int)InfoW757->AdvControl(InfoW757->ModuleNumber, ACTL_GETWINDOWCOUNT, NULL);
	int maxTabCount = windowCount + 1;

	ConEmuTab* tabs = (ConEmuTab*) calloc(maxTabCount, sizeof(ConEmuTab));

	EditorInfo ei;
	if (editorSave)
	{
		InfoW757->EditorControl(ECTL_GETINFO, &ei);
	}

	int tabCount = 1;
	for (int i = 0; i < windowCount; i++)
	{
		WInfo.Pos = i;
		InfoW757->AdvControl(InfoW757->ModuleNumber, ACTL_GETWINDOWINFO, (void*)&WInfo);
		if (WInfo.Type == WTYPE_EDITOR || WInfo.Type == WTYPE_VIEWER || WInfo.Type == WTYPE_PANELS)
			AddTab(tabs, tabCount, losingFocus, editorSave, 
				WInfo.Type, WInfo.Name, editorSave ? ei.FileName : NULL, 
				WInfo.Current, WInfo.Modified);
		InfoW757->AdvControl(InfoW757->ModuleNumber, ACTL_FREEWINDOWINFO, (void*)&WInfo);
	}
	
	if (editorSave) 
		InfoW757->EditorControl(ECTL_FREEINFO, &ei);

	SendTabs(tabs, tabCount);
}

/*extern "C"
{
void _chkstk()
{
}
}*/

void ExitFARW757(void)
{
	if (InfoW757) {
		free(InfoW757);
		InfoW757=NULL;
	}
	if (FSFW757) {
		free(FSFW757);
		FSFW757=NULL;
	}
}
