
/*
Copyright (c) 2012-2017 Maximus5
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

class CConEmuInside
{
public:
	enum InsideIntegration
	{
		ii_None = 0,
		ii_Auto,
		ii_Explorer,
		ii_Simple,
	};

public:
	CConEmuInside();
	virtual ~CConEmuInside();

	static bool InitInside(bool bRunAsAdmin, bool bSyncDir, LPCWSTR pszSyncDirCmdFmt, DWORD nParentPID, HWND hParentWnd);

public:
	// Режим интеграции. Запуститься как дочернее окно, например, в области проводника.
	InsideIntegration m_InsideIntegration;

	struct
	{
		DWORD ParentPID;           // ID of mh_InsideParentWND
		DWORD ParentParentPID;     // Parent of mn_InsideParentPID
		wchar_t ExeName[MAX_PATH]; // ParentPID executable
	} m_InsideParentInfo;

	bool  mb_InsideIntegrationAdmin; // Run first started console "As Admin"
	bool  mb_InsideSynchronizeCurDir;
	wchar_t* ms_InsideSynchronizeCurDir; // \ecd /d \1\n - \e - ESC, \b - BS, \n - ENTER, \1 - "dir", \2 - "bash dir"
	bool  mb_InsidePaneWasForced;
	DWORD mn_InsideParentPID;  // PID "родительского" процесса режима интеграции
	HWND  mh_InsideParentWND; // Это окно используется как родительское в режиме интеграции
	void  SetInsideParentWND(HWND hParent);
	bool  isInsideWndSet();
	bool  isParentProcess(HWND hParent);
	HWND  GetParentRoot();

	bool  inMinimizing(WINDOWPOS *p /*= NULL*/);
	bool  isParentIconic();
	bool  isSelfIconic();
	HWND  InsideFindParent();
	void  InsideParentMonitor();
	bool  GetInsideRect(RECT* prWnd);
	HWND  CheckInsideFocus();

private:
	HWND  mh_InitialRoot;
	HWND  mh_InsideParentRel;  // Может быть NULL (ii_Simple). HWND относительно которого нужно позиционироваться
	HWND  mh_InsideParentPath; // Win7 Text = "Address: D:\MYDOC"
	HWND  mh_InsideParentCD;   // Edit для смены текущей папки, например -> "C:\USERS"
	RECT  mrc_InsideParent, mrc_InsideParentRel; // для сравнения, чтоб знать, что подвинуться нада
	HWND  mh_TipPaneWndPost;
	bool  mb_TipPaneWasShown;
	wchar_t ms_InsideParentPath[MAX_PATH+1];
	struct EnumFindParentArg
	{
		DWORD nPID;
		HWND  hParentRoot;
	};
	static BOOL CALLBACK EnumInsideFindParent(HWND hwnd, LPARAM lParam);
	HWND  InsideFindConEmu(HWND hFrom);
	bool  InsideFindShellView(HWND hFrom);
	void  InsideUpdateDir();
	void  InsideUpdatePlacement();
	bool  TurnExplorerTipPane(wchar_t (&szAddMsg)[128]);
	bool  SendVkKeySequence(HWND hWnd, WORD* pvkKeys);
};
