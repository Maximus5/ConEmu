#pragma once

extern int lastModifiedStateW;
extern WCHAR gszDir1[0x400], gszDir2[0x400];

void AddTab(ConEmuTab* tabs, int &tabCount, bool losingFocus, bool editorSave, 
			int Type, LPCWSTR Name, LPCWSTR FileName, int Current, int Modified);

void SendTabs(ConEmuTab* tabs, int &tabCount);
