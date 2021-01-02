
/*
Copyright (c) 2012-present Maximus5
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


#pragma once

#define INSIDE_PARENT_NOT_FOUND ((HWND)-1)
#define INSIDE_DEFAULT_SYNC_DIR_CMD L"*"

#include "../common/MFileMapping.h"
#include "../common/Common.h"

class CConEmuInside final
{
public:
	enum InsideIntegration
	{
		ii_None = 0,
		ii_Auto,
		ii_Explorer,
		ii_Simple,
	};

	struct InsideParentInfo
	{
		DWORD ParentPID;           // ID of mh_InsideParentWND
		DWORD ParentParentPID;     // Parent of mn_InsideParentPID
		wchar_t ExeName[MAX_PATH]; // ParentPID executable
	};

public:
	CConEmuInside();
	~CConEmuInside();

	CConEmuInside(const CConEmuInside&) = delete;
	CConEmuInside(CConEmuInside&&) = delete;
	CConEmuInside& operator=(const CConEmuInside&) = delete;
	CConEmuInside& operator=(CConEmuInside&&) = delete;

	static bool InitInside(bool bRunAsAdmin, bool bSyncDir, LPCWSTR pszSyncDirCmdFmt, DWORD nParentPID, HWND hParentWnd);

protected:
	// Integration mode. Run as child window, e.g. in the Windows Explorer pane.
	InsideIntegration m_InsideIntegration = ii_None;

	InsideParentInfo m_InsideParentInfo = {};

	bool  mb_InsideIntegrationAdmin = false; // Run first started console "As Admin"
	bool  mb_InsideSynchronizeCurDir = false;
	CEStr ms_InsideSynchronizeCurDir; // \ecd /d \1\n - \e - ESC, \b - BS, \n - ENTER, \1 - "dir", \2 - "bash dir"
	bool  mb_InsidePaneWasForced = false;
	DWORD mn_InsideParentPID = 0;  // PID "родительского" процесса режима интеграции
	HWND  mh_InsideParentWND = nullptr; // Это окно используется как родительское в режиме интеграции

	MFileMapping<CESERVER_INSIDE_MAPPING_HDR> insideMapping_;

public:
	bool  IsInsideWndSet() const;
	void  SetInsideParentWnd(HWND hParent);
	bool  IsParentProcess(HWND hParent) const;
	HWND  GetParentRoot() const;
	HWND  GetParentWnd() const;
	InsideIntegration GetInsideIntegration() const;

	const InsideParentInfo& GetParentInfo() const;

	bool IsInsideIntegrationAdmin() const;
	void SetInsideIntegrationAdmin(bool value);

	bool IsSynchronizeCurDir() const;
	void SetSynchronizeCurDir(bool value);
	const wchar_t* GetInsideSynchronizeCurDir() const;
	void SetInsideSynchronizeCurDir(const wchar_t* value);

	bool  IsInMinimizing(WINDOWPOS *p /*= nullptr*/) const;
	bool  IsParentIconic() const;
	static bool  isSelfIconic();
	HWND  InsideFindParent();
	void  InsideParentMonitor();
	bool  GetInsideRect(RECT* prWnd) const;
	HWND  CheckInsideFocus() const;

	void UpdateDefTermMapping();

private:
	HWND  mh_InitialRoot = nullptr;
	HWND  mh_InsideParentRel = nullptr;  // Может быть nullptr (ii_Simple). HWND относительно которого нужно позиционироваться
	HWND  mh_InsideParentPath = nullptr; // Win7 Text = "Address: D:\MYDOC"
	HWND  mh_InsideParentCD = nullptr;   // Edit для смены текущей папки, например -> "C:\USERS"
	RECT  mrc_InsideParent = {};
	RECT  mrc_InsideParentRel = {}; // для сравнения, чтоб знать, что подвинуться нада
	HWND  mh_TipPaneWndPost = nullptr;
	bool  mb_TipPaneWasShown = false;
	wchar_t ms_InsideParentPath[MAX_PATH+1] = L"";

	static BOOL CALLBACK EnumInsideFindParent(HWND hwnd, LPARAM lParam);
	static HWND  InsideFindConEmu(HWND hFrom);
	bool  InsideFindShellView(HWND hFrom);
	void  InsideUpdateDir();
	void  InsideUpdatePlacement();
	bool  TurnExplorerTipPane(wchar_t (&szAddMsg)[128]);
	static bool  SendVkKeySequence(HWND hWnd, WORD* pvkKeys);
};
