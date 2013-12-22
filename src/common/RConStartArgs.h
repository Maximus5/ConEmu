
/*
Copyright (c) 2009-2013 Maximus5
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

struct RConStartArgs
{
	BOOL     bDetached; // internal use
	BOOL     bNewConsole; // TRUE==-new_console, FALSE==-cur_console
	BOOL     bBackgroundTab;      // -new_console:b
	BOOL     bForegroungTab;      // -new_console:f
	BOOL     bNoDefaultTerm;      // -new_console:z - не использовать фичу "Default terminal". Не пишется в CreateCommandLine()

	wchar_t* pszSpecialCmd; // собственно, command line
	wchar_t* pszStartupDir; // "-new_console:d:<dir>"

	wchar_t* pszRenameTab;  // "-new_console:t:<name>"

	wchar_t* pszIconFile;   // "-new_console:C:<icon>"
	wchar_t* pszPalette;    // "-new_console:P:<palette>"
	wchar_t* pszWallpaper;  // "-new_console:W:<wallpaper>"
	
	BOOL     bRunAsAdministrator; // -new_console:a
	BOOL     bRunAsRestricted;    // -new_console:r
	wchar_t* pszUserName, *pszDomain, szUserPassword[MAX_PATH]; // "-new_console:u:<user>:<pwd>"
	BOOL     bUseEmptyPassword;   // для GUI
	BOOL     bForceUserDialog;    // -new_console:u
	//wchar_t* pszUserProfile;    // %USERPROFILE%

	BOOL     bOverwriteMode;      // -new_console:w - enable "Overwrite" mode in console prompt

	UINT     nPTY;                // -new_console:p[N] - 1 enable PTY, 2 - disable PTY (work as plain console), 0 - don't change
	
	BOOL     bBufHeight;          // -new_console:h<lines>
	UINT     nBufHeight;          //

	BOOL     bLongOutputDisable;  // -new_console:o
	BOOL     bInjectsDisable;     // -new_console:i

 	enum {
 		eConfDefault = 0,
 		eConfAlways  = 1,         // -new_console:c
 		eConfNever   = 2,         // -new_console:n
 	} eConfirmation;

	BOOL     bForceDosBox;        // -new_console:x (may be useful with .bat files)

	BOOL     bForceInherit;

	enum SplitType {              // -new_console:s[<SplitTab>T][<Percents>](H|V)
		eSplitNone = 0,
		eSplitHorz = 1,
		eSplitVert = 2,
	} eSplit;
	UINT nSplitValue; // 1..999 (0.1 - 99.9%)0, по умолчанию - "50"
    UINT nSplitPane;  // по умолчанию - "0", иначе - 1-based индекс консоли, которую нужно разбить
	
	RecreateActionParm aRecreate; // Информационно и для CRecreateDlg

	RConStartArgs();
	~RConStartArgs();

	BOOL CheckUserToken(HWND hPwd);
	HANDLE CheckUserToken();

	int ProcessNewConArg(bool bForceCurConsole = false);

	wchar_t* CreateCommandLine(bool abForTasks = false);
	void AppendServerArgs(wchar_t* rsServerCmdLine, INT_PTR cchMax);

	bool AssignFrom(const struct RConStartArgs* args);

#ifdef _DEBUG
	static void RunArgTests();
#endif
};
