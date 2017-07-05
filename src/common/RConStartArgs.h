
/*
Copyright (c) 2009-2017 Maximus5
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

enum RecreateActionParm
{
	cra_CreateTab    = 0,
	cra_RecreateTab  = 1,
	cra_CreateWindow = 2,
	cra_EditTab      = 3,
};

enum RConBoolArg
{
	crb_Undefined = 0,
	crb_Off,
	crb_On,
};

#define DefaultSplitValue 500
#define DefaultPtyFlags (1|4)

struct RConStartArgs
{
	RConBoolArg     Detached; // internal use
	RConBoolArg     NewConsole; // TRUE==-new_console, FALSE==-cur_console
	RConBoolArg     BackgroundTab;      // -new_console:b
	RConBoolArg     ForegroungTab;      // -new_console:f
	RConBoolArg     NoDefaultTerm;      // -new_console:z - не использовать фичу "Default terminal". Не пишется в CreateCommandLine()

	wchar_t* pszSpecialCmd; // собственно, command line
	wchar_t* pszStartupDir; // "-new_console:d:<dir>"

	wchar_t* pszAddGuiArg;  // аналога для -new_console нет, используется для внутренних нужд GUI

	wchar_t* pszRenameTab;  // "-new_console:t:<name>"

	wchar_t* pszIconFile;   // "-new_console:C:<icon>"
	wchar_t* pszPalette;    // "-new_console:P:<palette>"
	wchar_t* pszWallpaper;  // "-new_console:W:<wallpaper>"
	wchar_t* pszMntRoot;    // "-new_console:m:<mntroot>" - explicit prefix for mount root: /cygdrive or /mnt
	
	RConBoolArg     RunAsAdministrator; // -new_console:a
	RConBoolArg     RunAsSystem;        // -new_console:A
	RConBoolArg     RunAsRestricted;    // -new_console:r
	RConBoolArg     RunAsNetOnly;       // -new_console:e
	wchar_t* pszUserName, *pszDomain, szUserPassword[MAX_PATH]; // "-new_console:u:<user>:<pwd>"
	RConBoolArg     UseEmptyPassword;   // для GUI
	RConBoolArg     ForceUserDialog;    // -new_console:u
	void CleanSecure();
	//wchar_t* pszUserProfile;    // %USERPROFILE%

	RConBoolArg     OverwriteMode;      // -new_console:w - enable "Overwrite" mode in console prompt

	UINT     		nPTY;               // -new_console:p[N] - change pty modes, N is bitmask: 1 - xterm keyboard, 2 - bracketed paste, 4 - app cursor keys
	
	RConBoolArg     BufHeight;          // -new_console:h<lines>
	UINT     		nBufHeight;         //

	RConBoolArg     LongOutputDisable;  // -new_console:o
	RConBoolArg     InjectsDisable;     // -new_console:i
	RConBoolArg     ForceNewWindow;     // -new_console:N - Force new ConEmu window with Default terminal
	RConBoolArg     ForceHooksServer;   // -new_console:R - Force CheckHookServer (GitShowBranch.cmd, actually used with -cur_console)

	enum CloseConfirm
	{
		eConfDefault = 0,
		eConfAlways  = 1,         // -new_console:c
		eConfNever   = 2,         // -new_console:n
		eConfEmpty   = 3,         // -new_console:c0
		eConfHalt    = 4,         // -new_console:c1
	} eConfirmation;

	RConBoolArg     ForceDosBox;        // -new_console:x (may be useful with .bat files)

	RConBoolArg     ForceInherit;

	enum SplitType {              // -new_console:s[<SplitTab>T][<Percents>](H|V)
		eSplitNone = 0,
		eSplitHorz = 1,
		eSplitVert = 2,
	} eSplit;
	UINT nSplitValue; // 1..999 (0.1 - 99.9%)0, по умолчанию - "50"
    UINT nSplitPane;  // по умолчанию - "0", иначе - 1-based индекс консоли, которую нужно разбить
	
	RecreateActionParm aRecreate; // Информационно и для CRecreateDlg

	// Internal for GUI tab creation
	#ifndef CONEMU_MINIMAL
	// Environment
	DWORD cchEnvStrings;
	wchar_t* pszEnvStrings;
	// Task name if defined
	wchar_t* pszTaskName;
	#endif

	RConStartArgs();
	~RConStartArgs();

	int ProcessNewConArg(bool bForceCurConsole = false);

	void AppendServerArgs(wchar_t* rsServerCmdLine, INT_PTR cchMax);


	// Hide unused (in dll's) methods
#ifndef CONEMU_MINIMAL
	bool CheckUserToken(HWND hPwd);
	HANDLE CheckUserToken();
	wchar_t* CreateCommandLine(bool abForTasks = false) const;
	bool AssignFrom(const struct RConStartArgs* args, bool abConcat = false);
	bool AssignUserArgs(const struct RConStartArgs* args, bool abConcat = false);
	bool HasInheritedArgs() const;
	bool AssignInheritedArgs(const struct RConStartArgs* args, bool abConcat = false);
#endif

	#if 0
	// Case insensitive search
	HMODULE hShlwapi;
	typedef LPWSTR (WINAPI* StrStrI_t)(LPWSTR pszFirst, LPCWSTR pszSrch);
	StrStrI_t WcsStrI;
	#endif

#ifdef _DEBUG
	static void RunArgTests();
#endif
};
