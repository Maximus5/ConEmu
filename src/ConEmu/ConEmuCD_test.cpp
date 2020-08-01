
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

#define HIDE_USE_EXCEPTION_INFO
#define SHOWDEBUGSTR

#include "Font.h"
#include "../common/defines.h"
#include "../common/MFileMapping.h"
#include "../common/MModule.h"
#include "../ConEmuCD/ExitCodes.h"
#include "../UnitTests/gtest.h"
#include "../ConEmuCD/ExportedFunctions.h"


namespace
{
MModule LoadConEmuCD()
{
	wchar_t szConEmuCD[MAX_PATH] = L"";
	if (!GetModuleFileName(nullptr, szConEmuCD, std::size(szConEmuCD)))
		return {};
	auto* slash = const_cast<wchar_t*>(PointToName(szConEmuCD));
	if (!slash)
		return {};
	wcscpy_s(slash, std::size(szConEmuCD) - (slash - szConEmuCD), L"\\ConEmu\\" ConEmuCD_DLL_3264);
	return MModule(szConEmuCD);
}

// hConWnd - HWND of the _real_ console
BOOL LoadSrvMapping(HWND hConWnd, CESERVER_CONSOLE_MAPPING_HDR& srvMapping)
{
	if (!hConWnd)
		return FALSE;

	MFileMapping<CESERVER_CONSOLE_MAPPING_HDR> SrvInfoMapping;
	SrvInfoMapping.InitName(CECONMAPNAME, LODWORD(hConWnd));
	const CESERVER_CONSOLE_MAPPING_HDR* pInfo = SrvInfoMapping.Open();
	if (!pInfo || pInfo->nProtocolVersion != CESERVER_REQ_VER)
		return FALSE;

	memmove_s(&srvMapping, sizeof(srvMapping),
		pInfo, std::min<size_t>(pInfo->cbSize, sizeof(srvMapping)));

	SrvInfoMapping.CloseMap();

	return (srvMapping.cbSize != 0);
}

HWND GetRealConsoleWindow()
{
	const MModule ConEmuHk(GetModuleHandle(ConEmuHk_DLL_3264));
	if (ConEmuHk.IsLoaded())
	{
		HWND (WINAPI* getConsoleWindow)() = nullptr;
		EXPECT_TRUE(ConEmuHk.GetProcAddress("GetRealConsoleWindow", getConsoleWindow));
		if (getConsoleWindow)
		{
			return getConsoleWindow();
		}
	}
	return GetConsoleWindow();
}

bool IsConEmu()
{
	CESERVER_CONSOLE_MAPPING_HDR srvMap{};
	if (!LoadSrvMapping(GetRealConsoleWindow(), srvMap))
		return false;
	if (!srvMap.hConEmuWndDc || !IsWindow(srvMap.hConEmuWndDc))
		return false;
	return true;
}

}

TEST(ConEmuCD,RunGuiMacro_DLL)
{
	const auto ConEmuCD = LoadConEmuCD();
	EXPECT_TRUE(ConEmuCD.IsLoaded());

	const auto isConEmu = IsConEmu();

	ConsoleMain3_t consoleMain3 = nullptr;
	EXPECT_TRUE(ConEmuCD.GetProcAddress(FN_CONEMUCD_CONSOLE_MAIN_3_NAME, consoleMain3));
	if (!consoleMain3)
		return; // nothing to check more

	SetEnvironmentVariable(CEGUIMACRORETENVVAR, nullptr);
	wchar_t szResult[64] = L"";

	const auto macroRc = consoleMain3(ConsoleMainMode::GuiMacro, L"-GuiMacro IsConEmu");
	if (!isConEmu)
	{
		EXPECT_EQ(CERR_GUIMACRO_FAILED, macroRc);
	}
	else
	{
		EXPECT_EQ(0, macroRc);
		GetEnvironmentVariable(CEGUIMACRORETENVVAR, szResult, std::size(szResult));
		EXPECT_STREQ(L"Yes", szResult);
	}
}
