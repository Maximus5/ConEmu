
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

#ifndef HIDE_USE_EXCEPTION_INFO
#define HIDE_USE_EXCEPTION_INFO
#endif

#ifndef SHOWDEBUGSTR
#define SHOWDEBUGSTR
#endif

#define DEBUGSTRINSIDE(s) //DEBUGSTR(s)

#include "Header.h"

#include "../common/MProcess.h"

#include "ConEmu.h"
#include "Inside.h"
#include "OptionsClass.h"
#include "RealConsole.h"
#include "VConRelease.h"
#include "VirtualConsole.h"


#define EMID_TIPOFDAY 41536

namespace {

bool CheckClassName(HWND h, LPCWSTR pszNeed)
{
	wchar_t szClass[128];
	if (!GetClassName(h, szClass, countof(szClass)))
		return false;

	const int nCmp = lstrcmp(szClass, pszNeed);

	return (nCmp == 0);
}

HWND FindTopWindow(HWND hParent, LPCWSTR sClass)
{
	HWND hLast = nullptr;
	HWND hFind = nullptr;
	int Coord = 99999;
	while ((hFind = FindWindowEx(hParent, hFind, sClass, nullptr)) != nullptr)
	{
		RECT rc; GetWindowRect(hFind, &rc);
		if ((hLast == nullptr)
			|| (rc.top < Coord))
		{
			Coord = rc.top;
			hLast = hFind;
		}
	}
	return hLast;
}

struct EnumFindParentArg
{
	DWORD nPID;
	HWND  hParentRoot;
};

}

CConEmuInside::CConEmuInside()
{
}

// static
bool CConEmuInside::InitInside(bool bRunAsAdmin, bool bSyncDir, LPCWSTR pszSyncDirCmdFmt, DWORD nParentPID, HWND hParentWnd)
{
	CConEmuInside* pInside = gpConEmu->mp_Inside;
	if (!pInside)
	{
		pInside = new CConEmuInside();
		if (!pInside)
		{
			return false;
		}
		gpConEmu->mp_Inside = pInside;
	}

	pInside->mn_InsideParentPID = nParentPID;
	pInside->SetInsideParentWnd(hParentWnd);

	pInside->m_InsideIntegration = (hParentWnd == nullptr) ? ii_Auto : ii_Simple;

	if (bRunAsAdmin)
	{
		LogString(L"!!! InsideIntegration will run first console as Admin !!!");
		pInside->mb_InsideIntegrationAdmin = true;
	}
	else
	{
		pInside->mb_InsideIntegrationAdmin = false;
	}

	if (bSyncDir)
	{
		pInside->mb_InsideSynchronizeCurDir = true;
		pInside->ms_InsideSynchronizeCurDir.Set(pszSyncDirCmdFmt); // \eCD /d %1 - \e - ESC, \b - BS, \n - ENTER, %1 - "dir", %2 - "bash dir"
	}
	else
	{
		_ASSERTE(pInside->ms_InsideSynchronizeCurDir.IsEmpty());
		pInside->mb_InsideSynchronizeCurDir = false;
	}

	return true;
}

CConEmuInside::~CConEmuInside()
{
	// If "Tip pane" was shown by ConEmu - hide pane after ConEmu close
	if (mb_TipPaneWasShown && mh_TipPaneWndPost)
	{
		PostMessage(mh_TipPaneWndPost, WM_COMMAND, EMID_TIPOFDAY/*41536*/, 0);
	}

	ms_InsideSynchronizeCurDir.Release();
}

BOOL CConEmuInside::EnumInsideFindParent(HWND hwnd, LPARAM lParam)
{
	EnumFindParentArg* pFind = reinterpret_cast<EnumFindParentArg*>(lParam);
	DWORD nPID = 0;
	if (IsWindowVisible(hwnd)
		&& GetWindowThreadProcessId(hwnd, &nPID)
		&& (nPID == pFind->nPID))
	{
		const DWORD nNeedStyles = WS_CAPTION | WS_SYSMENU | WS_THICKFRAME | WS_MINIMIZEBOX | WS_MAXIMIZEBOX;
		const DWORD nStyles = GetWindowLong(hwnd, GWL_STYLE);
		if ((nStyles & nNeedStyles) == nNeedStyles)
		{
			// Found, stop enumeration
			pFind->hParentRoot = hwnd;
			return FALSE;
		}
	}

	// Next window
	return TRUE;
}

// static
HWND CConEmuInside::InsideFindConEmu(HWND hFrom)
{
	wchar_t szClass[128];
	HWND hChild = nullptr, hNext = nullptr;
	//HWND hXpView = nullptr, hXpPlace = nullptr;

	while ((hChild = FindWindowEx(hFrom, hChild, nullptr, nullptr)) != nullptr)
	{
		GetClassName(hChild, szClass, countof(szClass));
		if (lstrcmp(szClass, gsClassNameParent) == 0)
		{
			return hChild;
		}

		hNext = InsideFindConEmu(hChild);
		if (hNext)
		{
			return hNext;
		}
	}

	return nullptr;
}

bool CConEmuInside::InsideFindShellView(HWND hFrom)
{
	wchar_t szClass[128];
	wchar_t szParent[128];
	wchar_t szRoot[128];
	HWND hChild = nullptr;
	// Для WinXP
	HWND hXpView = nullptr, hXpPlace = nullptr;

	while ((hChild = FindWindowEx(hFrom, hChild, nullptr, nullptr)) != nullptr)
	{
		// Нас интересуют только видимые окна!
		if (!IsWindowVisible(hChild))
			continue;

		// Windows 7, Windows 8.
		GetClassName(hChild, szClass, countof(szClass));
		if (lstrcmp(szClass, L"SHELLDLL_DefView") == 0)
		{
			GetClassName(hFrom, szParent, countof(szParent));
			if (lstrcmp(szParent, L"CtrlNotifySink") == 0)
			{
				// ReSharper disable once CppLocalVariableMayBeConst
				HWND hParent = GetParent(hFrom);
				if (hParent)
				{
					GetClassName(hParent, szRoot, countof(szRoot));
					_ASSERTE(lstrcmp(szRoot, L"DirectUIHWND") == 0);

					SetInsideParentWnd(hParent);
					mh_InsideParentRel = hFrom;
					m_InsideIntegration = ii_Explorer;

					return true;
				}
			}
			else if ((gnOsVer < 0x600) && (lstrcmp(szParent, L"ExploreWClass") == 0))
			{
				_ASSERTE(mh_InitialRoot == hFrom);
				hXpView = hChild;
			}
		}
		else if ((gnOsVer < 0x600) && (lstrcmp(szClass, L"BaseBar") == 0))
		{
			RECT rcBar = {}; GetWindowRect(hChild, &rcBar);
			MapWindowPoints(nullptr, hFrom, reinterpret_cast<LPPOINT>(&rcBar), 2);
			RECT rcParent = {}; GetClientRect(hFrom, &rcParent);
			if ((-10 <= (rcBar.right - rcParent.right))
				&& ((rcBar.right - rcParent.right) <= 10))
			{
				// Нас интересует область, прилепленная к правому-нижнему углу
				hXpPlace = hChild;
			}
		}
		// Путь в этом (hChild) хранится в формате "Address: D:\users\max"
		else if ((gnOsVer >= 0x600) && lstrcmp(szClass, L"ToolbarWindow32") == 0)
		{
			GetClassName(hFrom, szParent, countof(szParent));
			if (lstrcmp(szParent, L"Breadcrumb Parent") == 0)
			{
				// ReSharper disable once CppLocalVariableMayBeConst
				HWND hParent = GetParent(hFrom);
				if (hParent)
				{
					GetClassName(hParent, szRoot, countof(szRoot));
					_ASSERTE(lstrcmp(szRoot, L"msctls_progress32") == 0);

					mh_InsideParentPath = hChild;

					// Остается ComboBox/Edit, в который можно запихнуть путь, чтобы заставить эксплорер по нему перейти
					// Но есть проблема. Этот контрол не создается при открытии окна!

					return true;
				}
			}
		}

		if ((hChild != hXpView) && (hChild != hXpPlace))
		{
			if (InsideFindShellView(hChild))
			{
				if (mh_InsideParentRel && mh_InsideParentPath)
					return true;
				else
					break;
			}
		}

		if (hXpView && hXpPlace)
		{
			mh_InsideParentRel = FindWindowEx(hXpPlace, nullptr, L"ReBarWindow32", nullptr);
			if (!mh_InsideParentRel)
			{
				_ASSERTE(mh_InsideParentRel && L"ReBar must be found on XP & 2k3");
				return true; // закончить поиск
			}
			SetInsideParentWnd(hXpPlace);
			_ASSERTE(mh_InsideParentWND!=nullptr);
			// ReSharper disable once CppLocalVariableMayBeConst
			HWND hRoot = GetParentRoot();
			_ASSERTE(mh_InsideParentPath==nullptr || mh_InsideParentPath==hRoot);
			mh_InsideParentPath = hRoot;
			m_InsideIntegration = ii_Explorer;
			return true;
		}
	}

	return false;
}

void CConEmuInside::SetInsideParentWnd(HWND hParent)
{
	mh_InsideParentWND = hParent;

	if (!hParent || (hParent == INSIDE_PARENT_NOT_FOUND))
	{
		mh_InsideParentRel = nullptr;
	}
	else
	{
		ZeroStruct(m_InsideParentInfo);

		// Store basic information about parent process (reporting purposes)
		DWORD nPid = 0;
		if (GetWindowThreadProcessId(hParent, &nPid))
		{
			mn_InsideParentPID = nPid;
			m_InsideParentInfo.ParentPID = nPid;

			PROCESSENTRY32W info = {};
			if (GetProcessInfo(nPid, info))
			{
				m_InsideParentInfo.ParentParentPID = info.th32ParentProcessID;
				wcscpy_c(m_InsideParentInfo.ExeName, info.szExeFile);
			}
		}

		mh_InitialRoot = GetParentRoot();
	}
}

bool CConEmuInside::IsInsideWndSet() const
{
	if (!this || !mh_InsideParentWND || (mh_InsideParentWND == INSIDE_PARENT_NOT_FOUND))
		return false;
	return true;
}

bool CConEmuInside::IsSynchronizeCurDir() const
{
	return mb_InsideSynchronizeCurDir;
}

void CConEmuInside::SetSynchronizeCurDir(const bool value)
{
	mb_InsideSynchronizeCurDir = value;
}

bool CConEmuInside::IsParentProcess(HWND hParent) const
{
	if (!IsInsideWndSet())
	{
		return false;
	}

	DWORD nPID = 0;
	if (!GetWindowThreadProcessId(hParent, &nPID))
	{
		return false;
	}

	return (nPID == m_InsideParentInfo.ParentPID);
}

HWND CConEmuInside::GetParentRoot() const
{
	if (!IsInsideWndSet())
	{
		return nullptr;
	}

	// Detect top parent window
	HWND hRootParent = mh_InsideParentWND;
	while ((GetWindowLong(hRootParent, GWL_STYLE) & WS_CHILD) != 0)
	{
		// ReSharper disable once CppLocalVariableMayBeConst
		HWND hNext = GetParent(hRootParent);
		if (!hNext)
			break;
		hRootParent = hNext;
	}

	return hRootParent;
}

HWND CConEmuInside::GetParentWnd() const
{
	if (!IsInsideWndSet())
		return nullptr;
	return mh_InsideParentWND;
}

CConEmuInside::InsideIntegration CConEmuInside::GetInsideIntegration() const
{
	return m_InsideIntegration;
}

const CConEmuInside::InsideParentInfo& CConEmuInside::GetParentInfo() const
{
	return m_InsideParentInfo;
}

bool CConEmuInside::IsInsideIntegrationAdmin() const
{
	return mb_InsideIntegrationAdmin;
}

void CConEmuInside::SetInsideIntegrationAdmin(const bool value)
{
	mb_InsideIntegrationAdmin = value;
}

const wchar_t* CConEmuInside::GetInsideSynchronizeCurDir() const
{
	return ms_InsideSynchronizeCurDir.c_str(L"");
}

void CConEmuInside::SetInsideSynchronizeCurDir(const wchar_t* value)
{
	ms_InsideSynchronizeCurDir.Set(value);
}

// Вызывается для инициализации из Settings::LoadSettings()
HWND CConEmuInside::InsideFindParent()
{
	bool bFirstStep = true;
	DWORD nParentPID = 0;
	EnumFindParentArg find = {};

	if (!m_InsideIntegration)
	{
		return nullptr;
	}

	if (mh_InsideParentWND)
	{
		if (IsWindow(mh_InsideParentWND))
		{
			if (m_InsideIntegration == ii_Simple)
			{
				// We cover all client area of mh_InsideParentWND in this mode
				_ASSERTE(mh_InsideParentRel==nullptr);
				mh_InsideParentRel = nullptr;
			}

			_ASSERTE(mh_InsideParentWND!=nullptr);
			goto wrap;
		}
		else
		{
			if (m_InsideIntegration == ii_Simple)
			{
				DisplayLastError(L"Specified window not found");
				SetInsideParentWnd(nullptr);
				goto wrap;
			}
			_ASSERTE(IsWindow(mh_InsideParentWND));
			SetInsideParentWnd(mh_InsideParentRel = nullptr);
		}
	}

	_ASSERTE(m_InsideIntegration!=ii_Simple);

	if (mn_InsideParentPID)
	{
		PROCESSENTRY32 pi = {};
		if ((mn_InsideParentPID == GetCurrentProcessId())
			|| !GetProcessInfo(mn_InsideParentPID, pi))
		{
			DisplayLastError(L"Invalid parent process specified");
			m_InsideIntegration = ii_None;
			SetInsideParentWnd(nullptr);
			goto wrap;
		}

		nParentPID = mn_InsideParentPID;
	}
	else
	{
		PROCESSENTRY32 pi = {};
		if (!GetProcessInfo(GetCurrentProcessId(), pi) || !pi.th32ParentProcessID)
		{
			DisplayLastError(L"GetProcessInfo(GetCurrentProcessId()) failed");
			m_InsideIntegration = ii_None;
			SetInsideParentWnd(nullptr);
			goto wrap;
		}

		nParentPID = pi.th32ParentProcessID;
	}

	// Do window enumeration
	find.nPID = nParentPID;
	::EnumWindows(EnumInsideFindParent, reinterpret_cast<LPARAM>(&find));


	if (!find.hParentRoot)
	{
		const int nBtn = MsgBox(L"Can't find appropriate parent window!\n\nContinue in normal mode?", MB_ICONSTOP|MB_YESNO|MB_DEFBUTTON2);
		if (nBtn != IDYES)
		{
			SetInsideParentWnd(INSIDE_PARENT_NOT_FOUND);
			return mh_InsideParentWND; // Close!
		}
		// Continue in normal mode
		m_InsideIntegration = ii_None;
		SetInsideParentWnd(nullptr);
		goto wrap;
	}

	mh_InitialRoot = find.hParentRoot;
	mn_InsideParentPID = nParentPID;

	HWND hExistConEmu;
	if ((hExistConEmu = InsideFindConEmu(find.hParentRoot)) != nullptr)
	{
		_ASSERTE(FALSE && "Continue to create tab in existing instance");
		// If Explorer already contains ConEmu instance - open the new tab there
		gpSetCls->SingleInstanceShowHide = sih_None;
		const auto* pszCmdLine = GetCommandLine();
		CmdArg lsArg;
		LPCWSTR pszCmd = pszCmdLine;
		while ((pszCmd = NextArg(pszCmd, lsArg)))
		{
			if (lsArg.OneOfSwitches(L"-runlist",L"-cmdlist"))
			{
				pszCmd = nullptr; break;
			}
			else if (lsArg.OneOfSwitches(L"-run",L"-cmd"))
			{
				break;
			}
		}
		gpConEmu->RunSingleInstance(hExistConEmu, (pszCmd && *pszCmd) ? (pszCmd) : nullptr);

		SetInsideParentWnd(INSIDE_PARENT_NOT_FOUND);
		return mh_InsideParentWND; // Close!
	}

	// Now we need to find child windows
	// 1. the one where we shall integrate
	// 2. the anchor to position our window
	// 3. the source to synchronize the CWD
	InsideFindShellView(find.hParentRoot);

RepeatCheck:
	if (!IsInsideWndSet() || (!mh_InsideParentRel && (m_InsideIntegration == ii_Explorer)))
	{
		wchar_t szAddMsg[128] = L"", szMsg[1024];
		if (bFirstStep)
		{
			bFirstStep = false;

			if (TurnExplorerTipPane(szAddMsg))
			{
				goto RepeatCheck;
			}
		}

		swprintf_c(szMsg, L"%sCan't find appropriate shell window!\nUnrecognized layout of the Explorer.\n\nContinue in normal mode?", szAddMsg);
		const int nBtn = MsgBox(szMsg, MB_ICONSTOP|MB_YESNO|MB_DEFBUTTON2);

		if (nBtn != IDYES)
		{
			SetInsideParentWnd(INSIDE_PARENT_NOT_FOUND);
			return mh_InsideParentWND; // Close!
		}
		m_InsideIntegration = ii_None;
		SetInsideParentWnd(nullptr);
		goto wrap;
	}

wrap:
	if (!IsInsideWndSet())
	{
		m_InsideIntegration = ii_None;
	}
	else
	{
		_ASSERTE(mh_InsideParentWND != nullptr);
		GetWindowThreadProcessId(mh_InsideParentWND, &mn_InsideParentPID);
		// To monitor the path
		GetCurrentDirectory(countof(ms_InsideParentPath), ms_InsideParentPath);
		const int nLen = lstrlen(ms_InsideParentPath);
		if ((nLen > 3) && (ms_InsideParentPath[nLen-1] == L'\\'))
		{
			ms_InsideParentPath[nLen-1] = 0;
		}
	}

	return mh_InsideParentWND;
}

bool CConEmuInside::TurnExplorerTipPane(wchar_t (&szAddMsg)[128])
{
	bool bRepeat = false;
	int nBtn = IDCANCEL;
	DWORD_PTR nRc = 0;
	LRESULT lSendRc = 0;

	if (gnOsVer < 0x600)
	{
		// WinXP, Win2k3
		nBtn = IDYES; // MsgBox(L"Tip pane is not found in Explorer window!\nThis pane is required for 'ConEmu Inside' mode.\nDo you want to show this pane?", MB_ICONQUESTION|MB_YESNO);
		// if (nBtn == IDYES)
		{

		#if 0

			HWND hWorker = GetDlgItem(mh_InsideParentRoot, 0xA005);

			#ifdef _DEBUG
			if (hWorker)
			{
				HWND hReBar = FindWindowEx(hWorker, nullptr, L"ReBarWindow32", nullptr);
				if (hReBar)
				{
					POINT pt = {4,4};
					HWND hTool = ChildWindowFromPoint(hReBar, pt);
					if (hTool)
					{
						//TBBUTTON tb = {};
						//lSendRc = SendMessageTimeout(mh_InsideParentRoot, TB_GETBUTTON, 2, (LPARAM)&tb, SMTO_NOTIMEOUTIFNOTHUNG, 500, &nRc);
						//if (tb.idCommand)
						//{
						//	NMTOOLBAR nm = {{hTool, 0, TBN_DROPDOWN}, 32896/*tb.idCommand*/};
						//	lSendRc = SendMessageTimeout(mh_InsideParentRoot, WM_NOTIFY, 0, (LPARAM)&nm, SMTO_NOTIMEOUTIFNOTHUNG, 500, &nRc);
						//}
						lSendRc = SendMessageTimeout(mh_InsideParentRoot, WM_COMMAND, 32896, 0, SMTO_NOTIMEOUTIFNOTHUNG, 500, &nRc);
					}
				}
			}
			// There is no way to force "Tip pane" in WinXP
			// if popup menu "View -> Explorer bar" was not _shown_ at least once
			// In that case, Explorer ignores WM_COMMAND(41536) and does not reveal tip pane.
			HMENU hMenu1 = GetMenu(mh_InsideParentRoot), hMenu2 = nullptr, hMenu3 = nullptr;
			if (hMenu1)
			{
				hMenu2 = GetSubMenu(hMenu1, 2);
				//if (!hMenu2)
				//{
				//	lSendRc = SendMessageTimeout(mh_InsideParentRoot, WM_MENUSELECT, MAKELONG(2,MF_POPUP), (LPARAM)hMenu1, SMTO_NOTIMEOUTIFNOTHUNG, 1500, &nRc);
				//	hMenu2 = GetSubMenu(hMenu1, 2);
				//}

				if (hMenu2)
				{
					hMenu3 = GetSubMenu(hMenu2, 2);
				}
			}
			#endif
		#endif

			//INPUT keys[4] = {INPUT_KEYBOARD,INPUT_KEYBOARD,INPUT_KEYBOARD,INPUT_KEYBOARD};
			//keys[0].ki.wVk = VK_F10; keys[0].ki.dwFlags = 0; //keys[0].ki.wScan = MapVirtualKey(VK_F10,MAPVK_VK_TO_VSC);
			//keys[1].ki.wVk = VK_F10; keys[1].ki.dwFlags = KEYEVENTF_KEYUP; //keys[0].ki.wScan = MapVirtualKey(VK_F10,MAPVK_VK_TO_VSC);
			//keys[1].ki.wVk = 'V';
			//keys[2].ki.wVk = 'E';
			//keys[3].ki.wVk = 'T';

			//LRESULT lSentRc = 0;
			//SetForegroundWindow(mh_InsideParentRoot);
			//LRESULT lSentRc = SendInput(2/*countof(keys)*/, keys, sizeof(keys[0]));
			//lSentRc = SendMessageTimeout(mh_InsideParentRoot, WM_SYSKEYUP, 'V', 0, SMTO_NOTIMEOUTIFNOTHUNG, 5000, &nRc);

			mh_TipPaneWndPost = mh_InitialRoot;
			lSendRc = SendMessageTimeout(mh_TipPaneWndPost, WM_COMMAND, EMID_TIPOFDAY/*41536*/, 0, SMTO_NOTIMEOUTIFNOTHUNG, 5000, &nRc);

			mb_InsidePaneWasForced = true;
			// Prepare the error message in advance
			wcscpy_c(szAddMsg, L"Forcing Explorer to show Tip pane failed.\nShow pane yourself and recall ConEmu.\n\n");
		}
	}

	if (nBtn == IDYES)
	{
		// First check
		SetInsideParentWnd(nullptr);
		m_InsideIntegration = ii_Auto;
		InsideFindShellView(mh_InitialRoot);
		if (mh_InsideParentWND && mh_InsideParentRel)
		{
			bRepeat = true;
			goto wrap;
		}
		// When not found - delay and repeat
		Sleep(500);
		SetInsideParentWnd(nullptr);
		m_InsideIntegration = ii_Auto;
		InsideFindShellView(mh_InitialRoot);
		if (mh_InsideParentWND && mh_InsideParentRel)
		{
			bRepeat = true;
			goto wrap;
		}

		// Last chance - try to post key sequence "F10 Left Left Down Down Down Left"
		// This will open popup menu containing "Tip of the day" menu item
		// "Esc Esc Esc" it and post {WM_COMMAND,EMID_TIPOFDAY} again
		WORD vkPostKeys[] = {VK_ESCAPE, VK_ESCAPE, VK_ESCAPE, VK_RIGHT, VK_RIGHT, VK_RIGHT, VK_DOWN, VK_DOWN, VK_DOWN, VK_RIGHT, VK_ESCAPE, VK_ESCAPE, VK_ESCAPE, 0};

		// Go
		if (SendVkKeySequence(mh_InitialRoot, vkPostKeys))
		{
			// Try again (Tip of the day)
			mh_TipPaneWndPost = mh_InitialRoot;
			lSendRc = SendMessageTimeout(mh_TipPaneWndPost, WM_COMMAND, EMID_TIPOFDAY/*41536*/, 0, SMTO_NOTIMEOUTIFNOTHUNG, 5000, &nRc);
			// Wait and check again
			Sleep(500);
			SetInsideParentWnd(nullptr);
			m_InsideIntegration = ii_Auto;
			InsideFindShellView(mh_InitialRoot);
			if (mh_InsideParentWND && mh_InsideParentRel)
			{
				bRepeat = true;
				// ReSharper disable once CppRedundantControlFlowJump
				goto wrap;
			}
		}
	}

wrap:
	if (bRepeat)
	{
		mb_TipPaneWasShown = true;
	}
	std::ignore = lSendRc;
	return bRepeat;
}

// pvkKeys - 0-terminated VKKeys array
// static
bool CConEmuInside::SendVkKeySequence(HWND hWnd, WORD* pvkKeys)
{
	bool bSent = false;
	//DWORD_PTR nRc1, nRc2;
	LRESULT lSendRc = 0;
	DWORD nErrCode = 0;

	if (!pvkKeys || !*pvkKeys)
	{
		_ASSERTE(pvkKeys && *pvkKeys);
		return false;
	}

	// Only for Windows XP
	_ASSERTE(gnOsVer < 0x600);
	// ReSharper disable once CppLocalVariableMayBeConst
	HWND hWorker1 = GetDlgItem(hWnd, 0xA005);
	if (!CheckClassName(hWorker1, L"WorkerW"))
		return false;
	// ReSharper disable once CppLocalVariableMayBeConst
	HWND hReBar1 = GetDlgItem(hWorker1, 0xA005);
	if (!CheckClassName(hReBar1, L"ReBarWindow32"))
		return false;
	// ReSharper disable once CppLocalVariableMayBeConst
	HWND hMenuBar = FindTopWindow(hReBar1, L"ToolbarWindow32");
	if (!hMenuBar)
		return false;

	size_t k = 0;

	HWND hSend = hMenuBar;

	while (pvkKeys[k])
	{
		// Prep send msg values
		const UINT nMsg1 = (pvkKeys[k] == VK_F10) ? WM_SYSKEYDOWN : WM_KEYDOWN;
		// ReSharper disable once CppDeclaratorNeverUsed
		DEBUGTEST(UINT nMsg2 = (pvkKeys[k] == VK_F10) ? WM_SYSKEYUP : WM_KEYUP);
		const UINT vkScan = MapVirtualKey(pvkKeys[k], 0/*MAPVK_VK_TO_VSC*/);
		const LPARAM lParam1 = 0x00000001 | (vkScan << 16);
		// ReSharper disable once CppDeclaratorNeverUsed
		DEBUGTEST(LPARAM lParam2 = 0xC0000001 | (vkScan << 16));

		// Post KeyDown&KeyUp
		if (pvkKeys[k] == VK_F10)
		{
			PostMessage(hMenuBar, WM_LBUTTONDOWN, 0, MAKELONG(5,5));
			PostMessage(hMenuBar, WM_LBUTTONUP, 0, MAKELONG(5,5));
			//lSendRc = PostMessage(hWnd, nMsg1, pvkKeys[k], lParam1)
			//	&& PostMessage(hWnd, nMsg2, pvkKeys[k], lParam2);
			Sleep(100);
			hSend = hMenuBar;
		}
		else
		{
			// Sequential keys send to "menu" control
			lSendRc = PostMessage(hSend, nMsg1, pvkKeys[k], lParam1);
			if (lSendRc)
			{
				Sleep(100);
				//lSendRc = PostMessage(hSend, nMsg2, pvkKeys[k], lParam2);
				//Sleep(100);
			}
		}

		if (lSendRc)
		{
			bSent = true;
		}
		else
		{
			// failed, may be ERROR_TIMEOUT?
			nErrCode = GetLastError();
			bSent = false;
			break;
		}

		k++;
	}

	// SendMessageTimeout failed?
	_ASSERTE(bSent);

	std::ignore = nErrCode;

	return bSent;
}

void CConEmuInside::InsideParentMonitor()
{
	// При смене положения "сплиттеров" - обновить свой размер/положение
	InsideUpdatePlacement();

	if (mb_InsideSynchronizeCurDir)
	{
		// При смене папки в проводнике
		InsideUpdateDir();
	}
}

void CConEmuInside::InsideUpdateDir()
{
	CVConGuard VCon;

	if (mh_InsideParentPath && IsWindow(mh_InsideParentPath) && (gpConEmu->GetActiveVCon(&VCon) >= 0) && VCon->RCon())
	{
		wchar_t szCurText[512] = {};
		DWORD_PTR lRc = 0;
		if (SendMessageTimeout(mh_InsideParentPath, WM_GETTEXT, countof(szCurText), reinterpret_cast<LPARAM>(szCurText), SMTO_ABORTIFHUNG|SMTO_NORMAL, 300, &lRc))
		{
			if (gnOsVer < 0x600)
			{
				// Если в заголовке нет полного пути
				if (wcschr(szCurText, L'\\') == nullptr)
				{
					// Сразу выходим
					return;
				}
			}

			LPCWSTR pszPath = nullptr;
			// Если тут уже путь - то префикс не отрезать
			if ((szCurText[0] == L'\\' && szCurText[1] == L'\\' && szCurText[2]) // сетевой путь
				|| (szCurText[0] && szCurText[1] == L':' && szCurText[2] == L'\\' /*&& szCurText[3]*/)) // Путь через букву диска
			{
				pszPath = szCurText;
			}
			else
			{
				// Иначе - отрезать префикс. На английской винде это "Address: D:\dir1\dir2"
				pszPath = wcschr(szCurText, L':');
				if (pszPath)
					pszPath = SkipNonPrintable(pszPath+1);
			}

			// Если успешно - сравниваем с ms_InsideParentPath
			if (pszPath && *pszPath && (lstrcmpi(ms_InsideParentPath, pszPath) != 0))
			{
				const int nLen = lstrlen(pszPath);
				if (nLen >= static_cast<int>(countof(ms_InsideParentPath)))
				{
					_ASSERTE((nLen < static_cast<int>(countof(ms_InsideParentPath))) && "Too long path?");
				}
				else //if (VCon->RCon())
				{
					// Запомнить для сравнения
					lstrcpyn(ms_InsideParentPath, pszPath, countof(ms_InsideParentPath));
					// Подготовить команду для выполнения в Shell
					VCon->RCon()->PostPromptCmd(true, pszPath);
				}
			}
		}
	}
}

void CConEmuInside::InsideUpdatePlacement()
{
	if (!IsInsideWndSet() || !IsWindow(mh_InsideParentWND))
		return;

	if ((m_InsideIntegration != ii_Explorer) && (m_InsideIntegration != ii_Simple))
		return;

	if ((m_InsideIntegration == ii_Explorer) && mh_InsideParentRel
		&& (!IsWindow(mh_InsideParentRel) || !IsWindowVisible(mh_InsideParentRel)))
	{
		//Vista: Проводник мог пересоздать окошко со списком файлов, его нужно найти повторно
		HWND hChild = nullptr;
		bool bFound = false;
		while (((hChild = FindWindowEx(mh_InsideParentWND, hChild, nullptr, nullptr)) != nullptr))
		{
			// ReSharper disable once CppLocalVariableMayBeConst
			HWND hView = FindWindowEx(hChild, nullptr, L"SHELLDLL_DefView", nullptr);
			if (hView && IsWindowVisible(hView))
			{
				bFound = true;
				mh_InsideParentRel = hView;
				break;
			}
		}

		if (!bFound)
			return;
	}

	if (!IsParentIconic())
	{
		RECT rcParent = {}, rcRelative = {};
		GetClientRect(mh_InsideParentWND, &rcParent);
		if (m_InsideIntegration != ii_Simple)
		{
			GetWindowRect(mh_InsideParentRel, &rcRelative);
			MapWindowPoints(nullptr, mh_InsideParentWND, reinterpret_cast<LPPOINT>(&rcRelative), 2);
		}

		if (memcmp(&mrc_InsideParent, &rcParent, sizeof(rcParent)) != 0
			|| ((m_InsideIntegration != ii_Simple)
				&& memcmp(&mrc_InsideParentRel, &rcRelative, sizeof(rcRelative)) != 0))
		{
			// Evaluate
			RECT rcWnd = {}; // GetDefaultRect();
			if (GetInsideRect(&rcWnd))
			{
				// Store new parent coords
				mrc_InsideParent = rcParent;
				mrc_InsideParentRel = rcRelative;

				// Correct our window position
				gpConEmu->UpdateInsideRect(rcWnd);
			}
		}
	}
}

bool CConEmuInside::GetInsideRect(RECT* prWnd) const
{
	RECT rcWnd = {};

	if (m_InsideIntegration == ii_None)
	{
		_ASSERTE(m_InsideIntegration != ii_None);
		return false;
	}

	if (IsParentIconic())
	{
		return false;
	}

	RECT rcParent = {}, rcRelative = {};
	GetClientRect(mh_InsideParentWND, &rcParent);
	if (m_InsideIntegration == ii_Simple)
	{
		rcWnd = rcParent;
	}
	else
	{
		RECT rcChild = {};
		GetWindowRect(mh_InsideParentRel, &rcChild);
		MapWindowPoints(nullptr, mh_InsideParentWND, reinterpret_cast<LPPOINT>(&rcChild), 2);
		IntersectRect(&rcRelative, &rcParent, &rcChild);

		// WinXP & Win2k3
		if (gnOsVer < 0x600)
		{
			rcWnd = rcRelative;
		}
		// Windows 7
		else if ((rcParent.bottom - rcRelative.bottom) >= 100)
		{
			// Предпочтительно
			// Далее - ветвимся по OS
			if (gnOsVer < 0x600)
			{
				rcWnd = MakeRect(rcRelative.left, rcRelative.bottom + 4, rcParent.right, rcParent.bottom);
			}
			else
			{
				rcWnd = MakeRect(rcParent.left, rcRelative.bottom + 4, rcParent.right, rcParent.bottom);
			}
		}
		else if ((rcParent.right - rcRelative.right) >= 200)
		{
			rcWnd = MakeRect(rcRelative.right + 4, rcRelative.top, rcParent.right, rcRelative.bottom);
		}
		else
		{
			TODO("Другие системы и проверки на валидность");
			rcWnd = MakeRect(rcParent.left, rcParent.bottom - 100, rcParent.right, rcParent.bottom);
		}
	}

	if (prWnd)
		*prWnd = rcWnd;

	return true;
}

bool CConEmuInside::IsParentIconic() const
{
	BOOL bIconic;
	// ReSharper disable once CppLocalVariableMayBeConst
	HWND hParent = GetParentRoot();
	if (hParent)
		bIconic = ::IsIconic(hParent);
	else if (mh_InsideParentWND)
		bIconic = ::IsIconic(mh_InsideParentWND);
	else
		bIconic = FALSE;
	return (bIconic != FALSE);
}

// When parent is minimized, it may force our window to be shrinked
// due to auto-resize its contents...
// static
bool CConEmuInside::isSelfIconic()
{
	RECT rcOur = {};
	WINDOWPLACEMENT wpl = {};
	wpl.length = sizeof(wpl);
	GetWindowRect(ghWnd, &rcOur);
	GetWindowPlacement(ghWnd, &wpl);
	const int nMinWidth = 32;
	const int nMinHeight = 32;
	if ((RectWidth(rcOur) < nMinWidth) || (RectHeight(rcOur) < nMinHeight))
		return true;
	return false;
}

bool CConEmuInside::IsInMinimizing(WINDOWPOS *p /*= nullptr*/) const
{
	if (IsParentIconic())
		return true;
	return false;
}

HWND CConEmuInside::CheckInsideFocus() const
{
	if (!IsInsideWndSet())
	{
		//_ASSERTE(FALSE && "Inside was not initialized");
		return nullptr;
	}

	wchar_t szInfo[512];
	GUITHREADINFO tif = {};
	tif.cbSize = sizeof(tif);
	// ReSharper disable once CppLocalVariableMayBeConst
	HWND hParentWnd = GetParentRoot();
	const DWORD nTID = GetWindowThreadProcessId(hParentWnd, nullptr);

	if (!GetGUIThreadInfo(nTID, &tif))
	{
		swprintf_c(szInfo, L"GetGUIThreadInfo(%u) failed, code=%u", nTID, GetLastError());
		LogString(szInfo);
		return nullptr;
	}

	static GUITHREADINFO last_tif = {};
	if (memcmp(&last_tif, &tif, sizeof(tif)) != 0)
	{
		last_tif = tif;

		swprintf_c(szInfo,
			L"ParentInputInfo: flags=x%X Active=x%X Focus=x%X Capture=x%X Menu=x%X MoveSize=x%X Caret=x%X (%i,%i)-(%i,%i)",
			tif.flags, LODWORD(tif.hwndActive), LODWORD(tif.hwndFocus), LODWORD(tif.hwndCapture), LODWORD(tif.hwndMenuOwner),
			LODWORD(tif.hwndMoveSize), LODWORD(tif.hwndCaret), LOGRECTCOORDS(tif.rcCaret));
		LogString(szInfo);
	}

	return tif.hwndFocus;
}
