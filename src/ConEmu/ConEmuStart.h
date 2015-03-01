
/*
Copyright (c) 2015 Maximus5
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

#include <windows.h>

class CConEmuMain;

class CConEmuStart
{
private:
	CConEmuMain* mp_ConEmu;

public:
	CConEmuStart(CConEmuMain* pOwner);
	virtual ~CConEmuStart();

private:
	/* 'Default' command line (if nor Registry, nor /cmd specified) */
	wchar_t  szDefCmd[MAX_PATH+32];
	CEStr ms_DefNewTaskName;
	/* Current command line, specified with "/cmd" or "/cmdlist" switches */
	wchar_t* pszCurCmd;
	bool isCurCmdList; // а это если был указан /cmdlist

public:
	/* OUR(!) startup info */
	STARTUPINFOW ourSI;

public:
	/* switch -detached in the ConEmu.exe arguments */
	bool mb_StartDetached;

public:
	/* Store/retrieve command line, specified with "/cmd" or "/cmdlist" switches */
	void SetCurCmd(wchar_t*& pszNewCmd, bool bIsCmdList);
	LPCTSTR GetCurCmd(bool *pIsCmdList = NULL);

	/* "Active" command line */
	LPCTSTR GetCmd(bool *pIsCmdList = NULL, bool bNoTask = false);
	/* If some task was marked ad "Default for new consoles" */
	LPCTSTR GetDefaultTask();

	/* "Default" command line "far/cmd" */
	LPCTSTR GetDefaultCmd();
	void    SetDefaultCmd(LPCWSTR asCmd);

	void ResetCmdArg();
};
