
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
#include "../common/StartupEnvDef.h"
#include "../common/wcwidth.h"
#include "../ConEmu/helper.h"
#include "../ConEmu/ConEmu.h"
#include "../ConEmu/FontMgr.h"
#include "../ConEmu/HotkeyList.h"
#include "../ConEmu/SettingsStorage.h"
#include "../ConEmu/Options.h"
#include "../ConEmu/OptionsClass.h"

#include <iostream>


OSVERSIONINFO gOSVer = {};
WORD gnOsVer = 0x500;
bool gbIsWine = false;
bool gbIsDBCS = false;
HWND gh__Wnd = NULL;
HWND ghWnd = NULL;
HWND ghWndWork = NULL;
HWND ghWndApp = NULL;
HWND ghConWnd = NULL;
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
CEStartupEnv* gpStartEnv = NULL;
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
wchar_t* GetDlgItemTextPtr(HWND hDlg, WORD nID) { return nullptr; }
LPCWSTR GetMouseMsgName(UINT msg) { return L""; }
wchar_t* getFocusedExplorerWindowPath() { return nullptr; }
HWND getForegroundWindow() { return NULL; }
bool isCharAltFont(ucs32 inChar) { return false; }
bool LogString(LPCWSTR asText) { return false; }
bool LogString(LPCWSTR asInfo, bool abWriteTime /*= true*/, bool abWriteLine /*= true*/) { return false; }
BOOL MoveWindowRect(HWND hWnd, const RECT& rcWnd, BOOL bRepaint) { return FALSE; }
size_t MyGetDlgItemText(HWND hDlg, WORD nID, size_t& cchMax, wchar_t*& pszText) { return 0; }
void PatchMsgBoxIcon(HWND hWnd, UINT messg, WPARAM wParam, LPARAM lParam) {}
bool ProcessMessage(MSG& Msg) { return false; }
void RaiseTestException() {}
wchar_t* SelectFile(LPCWSTR asTitle, LPCWSTR asDefFile /*= NULL*/, LPCWSTR asDefPath /*= NULL*/, HWND hParent /*= ghWnd*/, LPCWSTR asFilter /*= NULL*/, DWORD/*CESelectFileFlags*/ nFlags /*= sff_AutoQuote*/, CRealConsole* apRCon /*= NULL*/) { return nullptr; }
wchar_t* SelectFolder(LPCWSTR asTitle, LPCWSTR asDefFolder /*= NULL*/, HWND hParent /*= ghWnd*/, DWORD/*CESelectFileFlags*/ nFlags /*= sff_AutoQuote*/, CRealConsole* apRCon /*= NULL*/) { return nullptr; }
void SkipOneShowWindow() {}
void ShutdownGuiStep(LPCWSTR asInfo, int nParm1 /*= 0*/, int nParm2 /*= 0*/, int nParm3 /*= 0*/, int nParm4 /*= 0*/) {}
bool UpdateWin7TaskList(bool bForce, bool bNoSuccMsg /*= false*/) { return false; }
void WarnCreateWindowFail(LPCWSTR pszDescription, HWND hParent, DWORD nErrCode) {}

#if defined(_DEBUG)
BOOL POSTMESSAGE(HWND h, UINT m, WPARAM w, LPARAM l, BOOL extra) { return FALSE; }
#endif

namespace conemu {
namespace tests {
void PrepareGoogleTests()
{
	gOSVer = { sizeof(gOSVer) };
	GetOsVersionInformational(&gOSVer);
	gnOsVer = ((gOSVer.dwMajorVersion & 0xFF) << 8) | (gOSVer.dwMinorVersion & 0xFF);
	HeapInitialize();
	initMainThread();
	Settings settings;
	gpSet = &settings;

	gpHotKeys = new ConEmuHotKeyList;
	gpFontMgr = new CFontMgr;
	gpSetCls = new CSettings;
	gpConEmu = new CConEmuMain;
	gVConDcMap.Init(MAX_CONSOLE_COUNT, true);
	gVConBkMap.Init(MAX_CONSOLE_COUNT, true);
	gpLocalSecurity = LocalSecurity();
}
}  // namespace tests
}  // namespace conemu
