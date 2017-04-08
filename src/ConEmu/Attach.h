
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

class CProcessData;
class CDpiForDialog;
class CDynDialog;

enum AttachProcessType
{
	apt_Unknown = 0,
	apt_Console,
	apt_Gui
};

struct AttachParm
{
	HWND  hAttachWnd;
	DWORD nPID, nBits;
	AttachProcessType nType;
	BOOL  bAlternativeMode;
	BOOL  bLeaveOpened;
};

class CAttachDlg
{
protected:
	HWND  mh_Dlg, mh_List;
	CDpiForDialog* mp_DpiAware;
	CDynDialog* mp_Dlg;
	// Параметры аттача
	int   mn_AttachType; // 1 - console, 2 - GUI
	DWORD mn_AttachPID;  // PID процесса, к которому цепляемся
	HWND  mh_AttachHWND; // HWND при GUI аттаче
	// Данные о запущенных процессах в системе
	CProcessData *mp_ProcessData;
	BOOL  mb_IsWin64;
	DWORD mn_ExplorerPID;
protected:
	bool OnStartAttach();
	static bool StartAttach(HWND ahAttachWnd, DWORD anPID, DWORD anBits, AttachProcessType anType, BOOL abAltMode, BOOL abLeaveOpened);
protected:
	struct AttachWndInfo {
		DWORD nPID;
		int nImageBits;
		wchar_t szPid[32];
		wchar_t szType[16];
		wchar_t szClass[MAX_PATH];
		wchar_t szTitle[MAX_PATH];
		wchar_t szExeName[MAX_PATH];
		wchar_t szExePathName[MAX_PATH*4];
	};
	static bool CanAttachWindow(HWND hFind, DWORD nSkipPID, CProcessData* apProcessData, AttachWndInfo& Info);
public:
	static DWORD WINAPI StartAttachThread(AttachParm* lpParam);
	enum AttachMacroRet
	{
		amr_Success = 0,
		amr_Ambiguous = 1,
		amr_WindowNotFound = 2,
		amr_Unexpected = 3,
	};
	static AttachMacroRet AttachFromMacro(DWORD anPID, bool abAlternative = false);
public:
	CAttachDlg();
	~CAttachDlg();

	void AttachDlg();
	HWND GetHWND();
	void Close();

	static BOOL CALLBACK AttachDlgEnumWin(HWND hFind, LPARAM lParam);
	static INT_PTR CALLBACK AttachDlgProc(HWND hDlg, UINT messg, WPARAM wParam, LPARAM lParam);
};
