
/*
Copyright (c) 2011-present Maximus5
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

#define HIDE_USE_EXCEPTION_INFO
#define SHOWDEBUGSTR

#include "../common/defines.h"
#include "../UnitTests/gtest.h"
#include "Ansi.h"
#include "ShellProcessor.h"
#include "GuiAttach.h"
#include "DefTermHk.h"
#include "DllOptions.h"
#include <memory>


TEST(ShellProcessor, Test)
{
	CESERVER_CONSOLE_MAPPING_HDR srvMap{};
	ghConWnd = GetRealConsoleWindow();
	if (LoadSrvMapping(ghConWnd, srvMap))
	{	
		ghConEmuWnd = srvMap.hConEmuRoot;
		ghConEmuWndBack = srvMap.hConEmuWndBack;
		ghConEmuWndDC = srvMap.hConEmuWndDc;
		gnServerPID = srvMap.nServerPID;
	}
	if (!srvMap.hConEmuWndDc || !IsWindow(srvMap.hConEmuWndDc))
	{
		cdbg() << "Test is not fully functional, hConEmuWndDc is nullptr";
	}

	// #Tests Add more CShellProc tests and check the modified commands
	
	for (int i = 0; i < 10; i++)
	{
		MCHKHEAP;
		auto sp = std::make_shared<CShellProc>();
		LPCWSTR pszFile = nullptr, pszParam = nullptr;
		DWORD nCreateFlags = 0, nShowCmd = 0;
		STARTUPINFOW si = {};
		auto* psi = &si;
		si.cb = sizeof(si);
		switch (i)
		{
		case 0:
			pszFile = L"C:\\GCC\\mingw\\bin\\mingw32-make.exe";
			pszParam = L"mingw32-make \"1.cpp\" ";
			EXPECT_TRUE(sp->OnCreateProcessW(&pszFile, &pszParam, nullptr, &nCreateFlags, &psi));
			break;
		case 1:
			pszFile = L"C:\\GCC\\mingw\\bin\\mingw32-make.exe";
			pszParam = L"\"mingw32-make.exe\" \"1.cpp\" ";
			EXPECT_TRUE(sp->OnCreateProcessW(&pszFile, &pszParam, nullptr, &nCreateFlags, &psi));
			break;
		case 2:
			pszFile = L"C:\\GCC\\mingw\\bin\\mingw32-make.exe";
			pszParam = L"\"C:\\GCC\\mingw\\bin\\mingw32-make.exe\" \"1.cpp\" ";
			EXPECT_TRUE(sp->OnCreateProcessW(&pszFile, &pszParam, nullptr, &nCreateFlags, &psi));
			break;
		case 3:
			pszFile = L"F:\\VCProject\\FarPlugin\\ConEmu\\Bugs\\DOS\\Prince\\PRINCE.EXE";
			pszParam = L"prince megahit";
			EXPECT_TRUE(sp->OnCreateProcessW(&pszFile, &pszParam, nullptr, &nCreateFlags, &psi));
			break;
		case 4:
			pszFile = nullptr;
			pszParam = L" \"F:\\VCProject\\FarPlugin\\ConEmu\\Bugs\\DOS\\Prince\\PRINCE.EXE\"";
			EXPECT_TRUE(sp->OnCreateProcessW(&pszFile, &pszParam, nullptr, &nCreateFlags, &psi));
			break;
		case 5:
			pszFile = L"C:\\GCC\\mingw\\bin\\mingw32-make.exe";
			pszParam = L" \"1.cpp\" ";
			EXPECT_TRUE(sp->OnShellExecuteW(nullptr, &pszFile, &pszParam, nullptr, &nCreateFlags, &nShowCmd));
			break;
		default:
			break;
		}
		MCHKHEAP;
	}
}


// some global stubs
BOOL    gbAttachGuiClient = FALSE;
BOOL 	gbGuiClientAttached = FALSE;
GuiCui  gnAttachPortableGuiCui = e_Alone;
HWND 	ghAttachGuiClient = nullptr;
GetProcessId_t gfGetProcessId = nullptr;
DWORD gnHookMainThreadId = 0;
HANDLE CEAnsi::ghAnsiLogFile = nullptr;
LONG CEAnsi::gnEnterPressed = 0;

// some function mocks
void CheckHookServer()
{
}

void CEAnsi::ChangeTermMode(TermModeCommand mode, DWORD value, DWORD nPID /*= 0*/)
{
}

void CEAnsi::WriteAnsiLogW(LPCWSTR lpBuffer, DWORD nChars)
{
}

void CEAnsi::WriteAnsiLogFarPrompt()
{
}

bool CDefTermHk::IsDefTermEnabled()
{
	return false;
}

void CDefTermHk::DefTermLogString(LPCSTR asMessage, LPCWSTR asLabel)
{
}

void CDefTermHk::DefTermLogString(LPCWSTR asMessage, LPCWSTR asLabel)
{
}

bool CDefTermHk::LoadDefTermSrvMapping(CESERVER_CONSOLE_MAPPING_HDR& srvMapping)
{
	return false;
}

HWND CDefTermHk::AllocHiddenConsole(const bool /*bTempForVS*/)
{
	return nullptr;
}

size_t CDefTermHk::GetSrvAddArgs(bool bGuiArgs, bool forceInjects, CEStr& rsArgs, CEStr& rsNewCon)
{
	return 0;
}

bool CDefTermHk::IsInsideMode()
{
	return false;
}

void CDefTermHk::CreateChildMapping(DWORD childPid, const MHandle& childHandle, DWORD conemuInsidePid)
{
}

HWND WINAPI GetRealConsoleWindow()
{
	const MModule ConEmuHk(GetModuleHandle(ConEmuHk_DLL_3264));
	if (ConEmuHk.IsValid())
	{
		HWND(WINAPI * getConsoleWindow)() = nullptr;
		EXPECT_TRUE(ConEmuHk.GetProcAddress("GetRealConsoleWindow", getConsoleWindow));
		if (getConsoleWindow)
		{
			wcdbg() << L"Calling GetRealConsoleWindow from " << ConEmuHk_DLL_3264 << std::endl;
			return getConsoleWindow();
		}
	}
	wcdbg() << L"Calling GetConsoleWindow from WinAPI" << std::endl;
	return GetConsoleWindow();
}

bool AttachServerConsole()
{
	return false;
}

CINJECTHK_EXIT_CODES InjectHooks(PROCESS_INFORMATION pi, DWORD imageBits, BOOL abLogProcess, LPCWSTR asConEmuHkDir, HWND hConWnd)
{
	return CIH_NtdllNotLoaded;
}
