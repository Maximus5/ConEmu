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
#include "..\common\pluginW684.hpp"

#ifndef FORWARD_WM_COPYDATA
#define FORWARD_WM_COPYDATA(hwnd, hwndFrom, pcds, fn) \
    (BOOL)(UINT)(DWORD)(fn)((hwnd), WM_COPYDATA, (WPARAM)(hwndFrom), (LPARAM)(pcds))
#endif

/* COMMON - пока структуры не различаютс€ */
void WINAPI _export GetPluginInfoW(struct PluginInfo *pi)
{
	pi->StructSize = sizeof(struct PluginInfo);
	pi->Flags = PF_EDITOR | PF_VIEWER | PF_PRELOAD;
	pi->DiskMenuStrings = NULL;
	pi->DiskMenuNumbers = 0;
	pi->PluginMenuStrings = NULL;
	pi->PluginMenuStringsNumber =0;
	pi->PluginConfigStrings = NULL;
	pi->PluginConfigStringsNumber = 0;
	pi->CommandPrefix = NULL;
	pi->Reserved = 0;	
}
/* COMMON - end */



struct PluginStartupInfo *InfoW684=NULL;
struct FarStandardFunctions *FSFW684=NULL;
extern HWND ConEmuHwnd;
extern BOOL bWasSetParent;
extern HWND FarHwnd;
extern HANDLE hPipe, hPipeEvent;
extern HANDLE hThread;

extern HWND AtoH(WCHAR *Str, int Len);
extern void UpdateConEmuTabsW(int event, bool losingFocus, bool editorSave);


const WCHAR *GetMsgW684(int CompareLng)
{
	return InfoW684->GetMsg(InfoW684->ModuleNumber, CompareLng);
}


void ProcessDragFrom684()
{
	DWORD cout;
	WindowInfo WInfo;				
    WInfo.Pos=0;
	InfoW684->AdvControl(InfoW684->ModuleNumber, ACTL_GETSHORTWINDOWINFO, (void*)&WInfo);
	if (!WInfo.Current)
	{
		//InfoW684->AdvControl(InfoW684->ModuleNumber, ACTL_FREEWINDOWINFO, (void*)&WInfo);
		int ItemsCount=0;
		WriteFile(hPipe, &ItemsCount, sizeof(int), &cout, NULL);				
		return;
	}
	//InfoW684->AdvControl(InfoW684->ModuleNumber, ACTL_FREEWINDOWINFO, (void*)&WInfo);

	PanelInfo PInfo;
	InfoW684->Control(PANEL_ACTIVE, FCTL_GETPANELINFO, &PInfo);
	if ((PInfo.PanelType == PTYPE_FILEPANEL || PInfo.PanelType == PTYPE_TREEPANEL) && PInfo.Visible)
	{
		int nDirLen=0, nDirNoSlash=0;
		if (PInfo.lpwszCurDir && *PInfo.lpwszCurDir)
		{
			nDirLen=lstrlen(PInfo.lpwszCurDir);
			if (nDirLen>0)
				if (PInfo.lpwszCurDir[nDirLen-1]!=L'\\')
					nDirNoSlash=1;
		}

		//Maximus5 - новый формат передачи
		int nNull=0;
		WriteFile(hPipe, &nNull/*ItemsCount*/, sizeof(int), &cout, NULL);
		
		if (PInfo.SelectedItemsNumber>0)
		{
			int ItemsCount=PInfo.SelectedItemsNumber, i;

			int nMaxLen=MAX_PATH+1, nWholeLen=1;

			// сначала посчитать максимальную длину буфера
			for (i=0;i<ItemsCount;i++)
			{
				int nLen=nDirLen+nDirNoSlash;
				nLen += lstrlen(PInfo.SelectedItems[i]->FindData.lpwszFileName);
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
				//wsprintf(Path, L"%s\\%s", PInfo.lpwszCurDir, PInfo.SelectedItems[i]->FindData.lpwszFileName);
				Path[0]=0;

				int nLen=nDirLen+nDirNoSlash;
				nLen += lstrlen(PInfo.SelectedItems[i]->FindData.lpwszFileName);

				if (nDirLen>0) {
					lstrcpy(Path, PInfo.lpwszCurDir);
					if (nDirNoSlash) {
						Path[nDirLen]=L'\\';
						Path[nDirLen+1]=0;
					}
				}
				lstrcpy(Path+nDirLen+nDirNoSlash, PInfo.SelectedItems[i]->FindData.lpwszFileName);

				nLen++;
				WriteFile(hPipe, &nLen, sizeof(int), &cout, NULL);
				WriteFile(hPipe, Path, sizeof(WCHAR)*nLen, &cout, NULL);
			}

			delete Path; Path=NULL;

			//  онец списка
			WriteFile(hPipe, &nNull/*ItemsCount*/, sizeof(int), &cout, NULL);
		}
	}
	else
	{
		int ItemsCount=0;
		WriteFile(hPipe, &ItemsCount, sizeof(int), &cout, NULL);
		WriteFile(hPipe, &ItemsCount, sizeof(int), &cout, NULL); // смена формата
	}
	InfoW684->Control(PANEL_ACTIVE, FCTL_FREEPANELINFO, &PInfo);
}

void ProcessDragTo684()
{
	DWORD cout;
	WindowInfo WInfo;				
    WInfo.Pos=0;
	InfoW684->AdvControl(InfoW684->ModuleNumber, ACTL_GETSHORTWINDOWINFO, (void*)&WInfo);
	if (!WInfo.Current)
	{
		//InfoW684->AdvControl(InfoW684->ModuleNumber, ACTL_FREEWINDOWINFO, (void*)&WInfo);
		int ItemsCount=0;
		WriteFile(hPipe, &ItemsCount, sizeof(int), &cout, NULL);				
		return;
	}
	//InfoW684->AdvControl(InfoW684->ModuleNumber, ACTL_FREEWINDOWINFO, (void*)&WInfo);
	
	PanelInfo PAInfo, PPInfo;
	ForwardedPanelInfo *pfpi=NULL;
	int nStructSize = sizeof(ForwardedPanelInfo)+4; // потом увеличим на длину строк
	//ZeroMemory(&fpi, sizeof(fpi));
	BOOL lbAOK=FALSE, lbPOK=FALSE;
	
	//Maximus5 - к сожалению, FCTL_GETPANELSHORTINFO не возвращает lpwszCurDir :-(

	//if (!(lbAOK=InfoW684->Control(PANEL_ACTIVE, FCTL_GETPANELSHORTINFO, &PAInfo)))
	lbAOK=InfoW684->Control(PANEL_ACTIVE, FCTL_GETPANELINFO, &PAInfo);
	if (lbAOK && PAInfo.lpwszCurDir)
		nStructSize += (lstrlen(PAInfo.lpwszCurDir))*sizeof(WCHAR);

	//if (!(lbPOK=InfoW684->Control(PANEL_PASSIVE, FCTL_GETPANELSHORTINFO, &PPInfo)))
	lbPOK=InfoW684->Control(PANEL_PASSIVE, FCTL_GETPANELINFO, &PPInfo);
	if (lbPOK && PPInfo.lpwszCurDir)
		nStructSize += (lstrlen(PPInfo.lpwszCurDir))*sizeof(WCHAR); // »менно WCHAR! не TCHAR

	pfpi = (ForwardedPanelInfo*)calloc(nStructSize,1);


	pfpi->ActivePathShift = sizeof(ForwardedPanelInfo);
	pfpi->pszActivePath = (WCHAR*)(((char*)pfpi)+pfpi->ActivePathShift);

	pfpi->PassivePathShift = pfpi->ActivePathShift+2; // если ActivePath заполнитс€ - увеличим

	if (lbAOK)
	{
		pfpi->ActiveRect=PAInfo.PanelRect;
		if ((PAInfo.PanelType == PTYPE_FILEPANEL || PAInfo.PanelType == PTYPE_TREEPANEL) && PAInfo.Visible)
		{
			if (PAInfo.lpwszCurDir != NULL) {
				lstrcpyW(pfpi->pszActivePath, PAInfo.lpwszCurDir);
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
			if (PPInfo.lpwszCurDir != NULL)
				lstrcpyW(pfpi->pszPassivePath, PPInfo.lpwszCurDir);
		}
	}

	// —обственно, пересылка информации
	WriteFile(hPipe, &nStructSize, sizeof(nStructSize), &cout, NULL);
	WriteFile(hPipe, pfpi, nStructSize, &cout, NULL);

	free(pfpi); pfpi=NULL;
	InfoW684->Control(PANEL_ACTIVE, FCTL_FREEPANELINFO, &PAInfo);
	InfoW684->Control(PANEL_ACTIVE, FCTL_FREEPANELINFO, &PPInfo);
}

void SetStartupInfoW684(void *aInfo)
{
	::InfoW684 = (PluginStartupInfo*)calloc(sizeof(PluginStartupInfo),1);
	::FSFW684 = (FarStandardFunctions*)calloc(sizeof(FarStandardFunctions),1);
	*::InfoW684 = *((struct PluginStartupInfo*)aInfo);
	*::FSFW684 = *((struct PluginStartupInfo*)aInfo)->FSF;
	::InfoW684->FSF = ::FSFW684;
	
	ConEmuHwnd = NULL;
	FarHwnd = (HWND)InfoW684->AdvControl(InfoW684->ModuleNumber, ACTL_GETFARHWND, 0);
	ConEmuHwnd = GetAncestor(FarHwnd, GA_PARENT);
	if (ConEmuHwnd != NULL)
	{
		WCHAR className[100];
		GetClassName(ConEmuHwnd, (LPWSTR)className, 100);
		if (FSFW684->LStricmp(className, L"VirtualConsoleClass") != 0 &&
			FSFW684->LStricmp(className, L"VirtualConsoleClassDC") != 0)
		{
			ConEmuHwnd = NULL;
		} else {
			bWasSetParent = TRUE;
		}
	}
}

extern int lastModifiedStateW;
// watch non-modified -> modified editor status change
int ProcessEditorInputW684(LPCVOID aRec)
{
	const INPUT_RECORD *Rec = (const INPUT_RECORD*)aRec;
	// only key events with virtual codes > 0 are likely to cause status change (?)
	if (Rec->EventType == KEY_EVENT && Rec->Event.KeyEvent.wVirtualKeyCode > 0  && Rec->Event.KeyEvent.bKeyDown)
	{
		EditorInfo ei;
		// ѕока что PluginStartupInfo по верси€м не различаетс€...
		InfoW684->EditorControl(ECTL_GETINFO, &ei);
		int currentModifiedState = ei.CurState == ECSTATE_MODIFIED ? 1 : 0;
		InfoW684->EditorControl(ECTL_FREEINFO, &ei);
		if (lastModifiedStateW != currentModifiedState)
		{
			// !!! »менно UpdateConEmuTabsW, без версии !!!
			UpdateConEmuTabsW(0, false, false);
			lastModifiedStateW = currentModifiedState;
		}
	}
	return 0;
}

int ProcessEditorEventW684(int Event, void *Param)
{
	switch (Event)
	{
	case EE_CLOSE:
	case EE_GOTFOCUS:
	case EE_KILLFOCUS:
	case EE_SAVE:
	case EE_READ:
		{
			// !!! »менно UpdateConEmuTabsW, без версии !!!
			UpdateConEmuTabsW(Event, Event == EE_KILLFOCUS, Event == EE_SAVE);
			break;
		}
	}
	return 0;
}

int ProcessViewerEventW684(int Event, void *Param)
{
	switch (Event)
	{
	case VE_CLOSE:
	case VE_READ:
	case VE_KILLFOCUS:
	case VE_GOTFOCUS:
		{
			// !!! »менно UpdateConEmuTabsW, без версии !!!
			UpdateConEmuTabsW(Event, Event == VE_KILLFOCUS, false);
		}
	}

	return 0;
}


void UpdateConEmuTabsW684(int event, bool losingFocus, bool editorSave)
{
	if (ConEmuHwnd == NULL)
	{
		//return;
	}
	WindowInfo WInfo;
	
//	int nLenOfWideCharStr;

	// ѕока что PluginStartupInfo по верси€м не различаетс€...
	int windowCount = (int)InfoW684->AdvControl(InfoW684->ModuleNumber, ACTL_GETWINDOWCOUNT, NULL);
	int maxTabCount = windowCount + 1;

	ConEmuTab* tabs = (ConEmuTab*) calloc(maxTabCount, sizeof(ConEmuTab));

	EditorInfo ei;
	if (editorSave)
	{
		// ѕока что PluginStartupInfo по верси€м не различаетс€...
		InfoW684->EditorControl(ECTL_GETINFO, &ei);
	}

	// default "Panels" tab
	tabs[0].Current = 0;
	lstrcpyn(tabs[0].Name, GetMsgW684(0), CONEMUTABMAX-1);
	tabs[0].Pos = 0;
	tabs[0].Type = WTYPE_PANELS;

	int tabCount = 1;
	for (int i = 0; i < windowCount; i++)
	{
		WInfo.Pos = i;
		InfoW684->AdvControl(InfoW684->ModuleNumber, ACTL_GETWINDOWINFO, (void*)&WInfo);
		if (WInfo.Type == WTYPE_EDITOR || WInfo.Type == WTYPE_VIEWER)
		{
			bool Editor;
			int Modified;
			// when receiving losing focus event receiver is still reported as current
			tabs[tabCount].Type = WInfo.Type;
			tabs[tabCount].Current = losingFocus ? 0 : WInfo.Current;
			// when receiving saving event receiver is still reported as modified
			if (editorSave && FSFW684->LStricmp(ei.FileName, WInfo.Name) == 0)
			{
				Modified = 0;
			} 
			else
			{
				Modified = WInfo.Modified;
			}

			if (tabs[tabCount].Current != 0)
			{
				lastModifiedStateW = Modified != 0 ? 1 : 0;
			}
			else
			{
				lastModifiedStateW = -1;
			}

			Editor=(WInfo.Type == WTYPE_EDITOR);

			//wsprintf(tabs[tabCount].Name, L"%s%s%s", WInfo.Name, Editor?GetMsgW(1):GetMsgW(2), Modified?GetMsgW(3):L"");
			int nLen=min(lstrlen(WInfo.Name),(CONEMUTABMAX-100)), nCurLen=0;
			lstrcpyn(tabs[tabCount].Name, WInfo.Name, nLen+1);
			nCurLen+=nLen;
			LPCWSTR psz = Editor?GetMsgW684(1):GetMsgW684(2); nLen=lstrlen(psz);
			lstrcpy(tabs[tabCount].Name+nCurLen, psz);
			nCurLen+=nLen;
			psz = Modified?GetMsgW684(3):L""; nLen=lstrlen(psz);
			lstrcpy(tabs[tabCount].Name+nCurLen, psz);

			tabs[tabCount].Pos = tabCount;
			tabCount++;
		}
	}
	
	InfoW684->AdvControl(InfoW684->ModuleNumber, ACTL_FREEWINDOWINFO, (void*)&WInfo);
	if (editorSave) 
		InfoW684->EditorControl(ECTL_FREEINFO, &ei);

	if (losingFocus)
	{
		tabs[0].Current = 1;
	}

	COPYDATASTRUCT cds;
	cds.dwData = tabCount;
	cds.cbData = maxTabCount * sizeof(ConEmuTab);
	cds.lpData = tabs;
	FORWARD_WM_COPYDATA(ConEmuHwnd, FarHwnd, &cds, SendMessage);
	free(tabs);
}

void ExitFARW684(void)
{
	if (InfoW684) {
		free(InfoW684);
		InfoW684=NULL;
	}
	if (FSFW684) {
		free(FSFW684);
		FSFW684=NULL;
	}
}
