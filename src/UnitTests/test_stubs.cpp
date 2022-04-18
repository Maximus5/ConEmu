
/*
Copyright (c) 2020-present Maximus5
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

#include "../common/Common.h"
#include "../common/ConEmuCheck.h"
#include "../common/wcwidth.h"
#include "../ConEmu/helper.h"
#include "../ConEmu/ConEmu.h"
#include "../ConEmu/FontMgr.h"
#include "../ConEmu/HotkeyList.h"
#include "../ConEmu/Options.h"
#include "../ConEmu/OptionsClass.h"
#include "../ConEmuHk/Ansi.h"
#include "../ConEmuCD/ExportedFunctions.h"
#include "../ConEmuCD/ExitCodes.h"

#include <iostream>

#include "../UnitTests/gtest.h"

bool gbVerifyFailed = false;
bool gbVerifyStepFailed = false;
bool gbVerifyIgnoreAsserts = false;
bool gbVerifyVerbose = false;


OSVERSIONINFO gOSVer = {};
WORD gnOsVer = 0x500;
bool gbIsWine = false;
bool gbIsDBCS = false;
HWND gh__Wnd = NULL;
HWND ghWnd = NULL;
HWND ghWndWork = NULL;
HWND ghWndApp = NULL;
HWND ghDlgPendingFrom = NULL;
HWND ghLastForegroundWindow = NULL;
HINSTANCE g_hInstance = NULL;
bool gbMessagingStarted = false;
BOOL gbNoDblBuffer = false;
Settings* gpSet = nullptr;
CSettings* gpSetCls = nullptr;
CConEmuMain* gpConEmu = nullptr;
HICON hClassIcon = NULL, hClassIconSm = NULL;
WPARAM gnWndSetHotkey = 0, gnWndSetHotkeyOk = 0;
LONG gnInMsgBox = 0;
LONG gnMessageNestingLevel = 0;
BOOL gbDebugLogStarted = FALSE;
BOOL gbDebugShowRects = FALSE;
CFontMgr* gpFontMgr = NULL;
ConEmuHotKeyList* gpHotKeys = NULL;
MMap<HWND, CVirtualConsole*> gVConDcMap;
MMap<HWND, CVirtualConsole*> gVConBkMap;
DWORD gnLastMsgTick = (DWORD)-1;
SYSTEMTIME gstLastTimer = {};

const TCHAR* const gsClassName = VirtualConsoleClass;
const TCHAR* const gsClassNameParent = VirtualConsoleClassMain;
const TCHAR* const gsClassNameWork = VirtualConsoleClassWork;
const TCHAR* const gsClassNameBack = VirtualConsoleClassBack;
const TCHAR* const gsClassNameApp = VirtualConsoleClassApp;
wchar_t gsDefGuiFont[32] = L"Lucida Console";
wchar_t gsAltGuiFont[32] = L"Courier New";
wchar_t gsDefConFont[32] = L"Lucida Console";
wchar_t gsAltConFont[32] = L"Courier New";
wchar_t gsDefMUIFont[32] = L"Tahoma";


void AssertBox(LPCTSTR szText, LPCTSTR szFile, UINT nLine, LPEXCEPTION_POINTERS ExceptionInfo /*= NULL*/) {
	std::wcout << szFile << L":" << nLine << std::endl << (szText ? szText : L"") << std::endl;
}
size_t gMsgBoxCallCount = 0;
int MsgBox(LPCTSTR lpText, UINT uType, LPCTSTR lpCaption /*= NULL*/, HWND ahParent /*= (HWND)-1*/, bool abModal /*= true*/) {
	++gMsgBoxCallCount;
	std::wcout << (lpCaption ? lpCaption : L"") << std::endl << (lpText ? lpText : L"") << std::endl;
	return IDCANCEL;
}

RECT CenterInParent(RECT rcDlg, HWND hParent) { return rcDlg; }
bool CheckLockFrequentExecute(DWORD& Tick, DWORD Interval) { return false; }
HICON CreateNullIcon() { return NULL; }
void DebugLogFile(LPCSTR asMessage) {}
HWND FindProcessWindow(DWORD nPID) { return NULL; }
bool GetColorRef(LPCWSTR pszText, COLORREF* pCR) { return false; }
bool GetDlgItemSigned(HWND hDlg, WORD nID, int& nValue, int nMin /*= 0*/, int nMax /*= 0*/) { return false; }
CEStr GetDlgItemTextPtr(HWND hDlg, WORD nID) { return nullptr; }
LPCWSTR GetMouseMsgName(UINT msg) { return L""; }
CEStr getFocusedExplorerWindowPath() { return nullptr; }
HWND getForegroundWindow() { return NULL; }
bool isCharAltFont(ucs32 inChar) { return false; }
bool LogString(LPCWSTR asText) { return false; }
bool LogString(LPCWSTR asInfo, bool abWriteTime /*= true*/, bool abWriteLine /*= true*/) { return false; }
BOOL MoveWindowRect(HWND hWnd, const RECT& rcWnd, BOOL bRepaint) { return FALSE; }
size_t MyGetDlgItemText(HWND hDlg, WORD nID, CEStr& pszText) { return 0; }
void PatchMsgBoxIcon(HWND hWnd, UINT messg, WPARAM wParam, LPARAM lParam) {}
bool ProcessMessage(MSG& Msg) { return false; }
void RaiseTestException() {}
CEStr SelectFile(LPCWSTR asTitle, LPCWSTR asDefFile /*= NULL*/, LPCWSTR asDefPath /*= NULL*/, HWND hParent /*= ghWnd*/, LPCWSTR asFilter /*= NULL*/, DWORD/*CESelectFileFlags*/ nFlags /*= sff_AutoQuote*/, CRealConsole* apRCon /*= NULL*/) { return nullptr; }
CEStr SelectFolder(LPCWSTR asTitle, LPCWSTR asDefFolder /*= NULL*/, HWND hParent /*= ghWnd*/, DWORD/*CESelectFileFlags*/ nFlags /*= sff_AutoQuote*/, CRealConsole* apRCon /*= NULL*/) { return nullptr; }
void SkipOneShowWindow() {}
void ShutdownGuiStep(LPCWSTR asInfo, int nParm1 /*= 0*/, int nParm2 /*= 0*/, int nParm3 /*= 0*/, int nParm4 /*= 0*/) {}
bool UpdateWin7TaskList(bool bForce, bool bNoSuccMsg /*= false*/) { return false; }
void WarnCreateWindowFail(LPCWSTR pszDescription, HWND hParent, DWORD nErrCode) {}
bool CEAnsi::GetFeatures(ConEmu::ConsoleFlags& features) { features = ConEmu::ConsoleFlags::Empty; return false; }

#if defined(_DEBUG)
BOOL POSTMESSAGE(HWND h, UINT m, WPARAM w, LPARAM l, BOOL extra) { return FALSE; }
#endif

namespace conemu {
namespace tests {
void PrepareGoogleTests()
{
	gOSVer = {};
	gOSVer.dwOSVersionInfoSize = sizeof(gOSVer);
	GetOsVersionInformational(&gOSVer);
	gnOsVer = static_cast<WORD>((gOSVer.dwMajorVersion & 0xFF) << 8) | static_cast<WORD>(gOSVer.dwMinorVersion & 0xFF);
	HeapInitialize();
	initMainThread();
	static Settings settings{};  // NOLINT(clang-diagnostic-exit-time-destructors)
	gpSet = &settings;

	gpHotKeys = new ConEmuHotKeyList;
	gpFontMgr = new CFontMgr;
	gpSetCls = new CSettings;
	gpConEmu = new CConEmuMain;
	gVConDcMap.Init(MAX_CONSOLE_COUNT, true);
	gVConBkMap.Init(MAX_CONSOLE_COUNT, true);
	gpLocalSecurity = LocalSecurity();
}

void InitConEmuPathVars()
{
	CEStr testExe;
	GetModulePathName(nullptr, testExe);
	auto* namePtr = const_cast<wchar_t*>(PointToName(testExe.c_str()));
	if (namePtr && namePtr > testExe.c_str())
	{
		*(namePtr - 1) = 0;
		SetEnvironmentVariableW(ENV_CONEMUDIR_VAR_W, testExe);
		const CEStr baseDir(testExe, L"\\ConEmu");
		SetEnvironmentVariableW(ENV_CONEMUBASEDIR_VAR_W, baseDir);
	}
	else
	{
		cdbg() << "GetModulePathName failed, name is null" << std::endl;
	}

}

void WaitDebugger(const std::string& label, const DWORD milliseconds)
{
	if (::IsDebuggerPresent())
		return;
	const auto started = GetTickCount();
	cdbg() << label << " (waiting for debugger)";
	while (true)
	{
		Sleep(1000);
		if (::IsDebuggerPresent() || (GetTickCount() - started) > milliseconds)
			break;
		cdbg("", false) << ".";
	}
	cdbg("", false) << std::endl;
}

GuiMacro::GuiMacro()
{
	wchar_t szConEmuCD[MAX_PATH] = L"";
	if (!GetModuleFileName(nullptr, szConEmuCD, LODWORD(std::size(szConEmuCD))))
		throw std::runtime_error("GuiMacro failed: GetModuleFileName returns false");
	auto* slash = const_cast<wchar_t*>(PointToName(szConEmuCD));
	if (!slash)
		throw std::runtime_error("GuiMacro failed: GetModuleFileName returns invalid data");
	// Expected to be run from gtest suite
	wcscpy_s(slash, std::size(szConEmuCD) - (slash - szConEmuCD), L"\\ConEmu\\" ConEmuCD_DLL_3264);
	m_ConEmuCD = LoadLibrary(szConEmuCD);
	if (!m_ConEmuCD)
		throw std::runtime_error("GuiMacro failed: ConEmuCD LoadLibrary failed");
}

GuiMacro::~GuiMacro()
{
	if (m_ConEmuCD)
	{
		FreeLibrary(m_ConEmuCD);
		m_ConEmuCD = nullptr;
	}
}

std::wstring GuiMacro::Execute(const std::wstring& macro) const
{
	// ReSharper disable once CppLocalVariableMayBeConst
	GuiMacro_t guiMacro = reinterpret_cast<GuiMacro_t>(GetProcAddress(m_ConEmuCD, FN_CONEMUCD_GUIMACRO_NAME));
	if (!guiMacro)
	{
		return L"<GuiMacro exported name was not found>";
	}

	BSTR result = nullptr;
	// ReSharper disable once CppLocalVariableMayBeConst
	wchar_t szInstance[128] = L"T-2";
	const auto macroRc = guiMacro(szInstance, macro.c_str(), &result);
	if (macroRc != CERR_GUIMACRO_SUCCEEDED)
	{
		return L"<GuiMacro execution failed>";
	}

	std::wstring ret{result ? result : L"<nullptr>"};
	if (result)
		SysFreeString(result);

	return ret;
}

}  // namespace tests
}  // namespace conemu
