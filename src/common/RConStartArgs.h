
/*
Copyright (c) 2009-present Maximus5
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

typedef UINT PtyFlags;
const PtyFlags
	pty_XTerm    = 1,
	pty_BrPaste  = 2,
	pty_AppKeys  = 4,
	pty_Default  = pty_XTerm;

struct RConStartArgs
{
public:
	// If you add some members - don't forget them in RConStartArgs::AssignFrom!
	RConStartArgs() = default;
	// Use AssignFrom instead
	RConStartArgs(const RConStartArgs&) = delete;
	RConStartArgs(RConStartArgs&&) = delete;

	virtual ~RConStartArgs();

	int  ProcessNewConArg(bool bForceCurConsole = false);
	void AppendServerArgs(wchar_t* rsServerCmdLine, INT_PTR cchMax);

public:
	// internal use
	RConBoolArg     Detached = crb_Undefined;
	// TRUE==-new_console, FALSE==-cur_console
	RConBoolArg     NewConsole = crb_Undefined;
	// -new_console:b
	RConBoolArg     BackgroundTab = crb_Undefined;
	// -new_console:f
	RConBoolArg     ForegroungTab = crb_Undefined;
	// -new_console:z - don't use "Default terminal" feature. Ignored in CreateCommandLine()
	RConBoolArg     NoDefaultTerm = crb_Undefined;

	// the command line
	wchar_t* pszSpecialCmd = nullptr;
	// "-new_console:d:<dir>"
	wchar_t* pszStartupDir = nullptr;

	// no switch in -new_console, used internally by GUI
	wchar_t* pszAddGuiArg = nullptr;

	// "-new_console:t:<name>"
	wchar_t* pszRenameTab = nullptr;

	// "-new_console:C:<icon>"
	wchar_t* pszIconFile = nullptr;
	// "-new_console:P:<palette>"
	wchar_t* pszPalette = nullptr;
	// "-new_console:W:<wallpaper>"
	wchar_t* pszWallpaper = nullptr;
	// "-new_console:m:<mntroot>" - explicit prefix for mount root: /cygdrive or /mnt
	wchar_t* pszMntRoot = nullptr;
	// "-new_console:L:<AnsiLogDir>
	wchar_t* pszAnsiLog = nullptr;
	
	// -new_console:a
	RConBoolArg     RunAsAdministrator = crb_Undefined;
	// -new_console:A
	RConBoolArg     RunAsSystem = crb_Undefined;
	// -new_console:r
	RConBoolArg     RunAsRestricted = crb_Undefined;
	// -new_console:e
	RConBoolArg     RunAsNetOnly = crb_Undefined;
	// "-new_console:u:<user>:<pwd>"
	wchar_t* pszUserName = nullptr;
	wchar_t* pszDomain = nullptr;
	wchar_t  szUserPassword[MAX_PATH] = L"";
	// for GUI internally
	RConBoolArg     UseEmptyPassword = crb_Undefined;
	// -new_console:u
	RConBoolArg     ForceUserDialog = crb_Undefined;
	// Reset and clean security data
	void CleanPermissions();

	// -new_console:w - enable "Overwrite" mode in console prompt
	RConBoolArg     OverwriteMode = crb_Undefined;

	// -new_console:p[N] - change pty modes, N is bitmask: 1 - xterm keyboard, 2 - bracketed paste, 4 - app cursor keys
	PtyFlags		nPTY = 0;
	
	// -new_console:h<lines>
	RConBoolArg     BufHeight = crb_Undefined;
	UINT     		nBufHeight = 0;

	// -new_console:o
	RConBoolArg     LongOutputDisable = crb_Undefined;
	// -new_console:i
	RConBoolArg     InjectsDisable = crb_Undefined;
	// -new_console:N - Force new ConEmu window with Default terminal
	RConBoolArg     ForceNewWindow = crb_Undefined;
	// -new_console:R - Force CheckHookServer (GitShowBranch.cmd, actually used with -cur_console)
	RConBoolArg     ForceHooksServer = crb_Undefined;

	// -new_console:c[X] | -new_console:n
	enum CloseConfirm
	{
		eConfDefault = 0,
		eConfAlways  = 1,         // -new_console:c
		eConfNever   = 2,         // -new_console:n
		eConfEmpty   = 3,         // -new_console:c0
		eConfHalt    = 4,         // -new_console:c1
	} eConfirmation = eConfDefault;

	// -new_console:x (may be useful with .bat files)
	RConBoolArg     ForceDosBox = crb_Undefined;

	// -new_console:I
	RConBoolArg     ForceInherit = crb_Undefined;

	// -new_console:s[<SplitTab>T][<Percents>](H|V)
	enum SplitType {
		eSplitNone = 0,
		eSplitHorz = 1,
		eSplitVert = 2,
	} eSplit = eSplitNone;
	// 1..999 (0.1 - 99.9%), by default "50"
	UINT nSplitValue = DefaultSplitValue;
	// by default "0", otherwise 1-based index of the console to split
    UINT nSplitPane = 0;
	
	// Informational and for CRecreateDlg
	RecreateActionParm aRecreate = cra_CreateTab;

	#ifndef CONEMU_MINIMAL
	// Environment: Internal for GUI tab creation
	DWORD cchEnvStrings = 0;
	wchar_t* pszEnvStrings = nullptr;
	// Task name if defined
	wchar_t* pszTaskName = nullptr;
	#endif

};
