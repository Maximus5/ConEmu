#pragma once

extern int lastModifiedStateW;

void AddTab(ConEmuTab* tabs, int &tabCount, bool losingFocus, bool editorSave, 
			int Type, LPCWSTR Name, LPCWSTR FileName, int Current, int Modified);

void SendTabs(ConEmuTab* tabs, int &tabCount);
