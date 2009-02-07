#pragma once

#if !defined(_MSC_VER)
#include <wchar.h>
#include <tchar.h>
#endif



extern int lastModifiedStateW;
extern WCHAR gszDir1[CONEMUTABMAX], gszDir2[CONEMUTABMAX];
extern int maxTabCount, lastWindowCount;
extern ConEmuTab* tabs; //(ConEmuTab*) calloc(maxTabCount, sizeof(ConEmuTab));

BOOL CreateTabs(int windowCount);

BOOL AddTab(int &tabCount, bool losingFocus, bool editorSave, 
			int Type, LPCWSTR Name, LPCWSTR FileName, int Current, int Modified);

void SendTabs(int &tabCount, BOOL abForce=FALSE);

void InitHWND(HWND ahFarHwnd);

int ProcessEditorInputW684(LPCVOID Rec);
int ProcessEditorInputW757(LPCVOID Rec);
int ProcessEditorEventW684(int Event, void *Param);
int ProcessEditorEventW757(int Event, void *Param);
int ProcessViewerEventW684(int Event, void *Param);
int ProcessViewerEventW757(int Event, void *Param);
void ExitFARW684(void);
void ExitFARW757(void);
void UpdateConEmuTabsW684(int event, bool losingFocus, bool editorSave);
void UpdateConEmuTabsW757(int event, bool losingFocus, bool editorSave);
void SetStartupInfoW684(void *aInfo);
void SetStartupInfoW757(void *aInfo);
void ProcessDragFrom684();
void ProcessDragFrom757();
void ProcessDragTo684();
void ProcessDragTo757();

void CloseTabs();
