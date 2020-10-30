
/*
Copyright (c) 2015-present Maximus5
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

#include "ConEmuSrv.h"
#include "ConsoleState.h"
#include "../common/Common.h"
#include "../common/Dump.h"
#include "../common/ConsoleRead.h"
#include "../common/MStrSafe.h"
#include "../ConEmu/Font.h"
#include <tuple>

LPTOP_LEVEL_EXCEPTION_FILTER gpfnPrevExFilter = nullptr;
bool gbCreateDumpOnExceptionInstalled = false;

bool CopyToClipboard(LPCWSTR asText)
{
	if (!asText)
		return false;

	bool bCopied = false;

	if (OpenClipboard(nullptr))
	{
		const DWORD cch = lstrlen(asText);
		const HGLOBAL hGlobalCopy = GlobalAlloc(GMEM_MOVEABLE, (cch + 1) * sizeof(*asText));
		if (hGlobalCopy)
		{
			auto* lptStrCopy = static_cast<wchar_t*>(GlobalLock(hGlobalCopy));
			if (lptStrCopy)
			{
				_wcscpy_c(lptStrCopy, cch+1, asText);
				GlobalUnlock(hGlobalCopy);

				EmptyClipboard();
				bCopied = (SetClipboardData(CF_UNICODETEXT, hGlobalCopy) != nullptr);
			}
		}

		CloseClipboard();
	}

	return bCopied;
}

LONG WINAPI CreateDumpOnException(LPEXCEPTION_POINTERS ExceptionInfo)
{
	const bool bKernelTrap = (gnInReadConsoleOutput > 0);
	ConEmuDumpInfo dumpInfo{};
	wchar_t szAdd[2000];

	const DWORD dwErr = CreateDumpForReport(ExceptionInfo, dumpInfo);

	szAdd[0] = 0;

	if (bKernelTrap)
	{
		wcscat_c(szAdd, L"Due to Microsoft kernel bug the crash was occurred\r\n");
		wcscat_c(szAdd, CEMSBUGWIKI /* http://conemu.github.io/en/MicrosoftBugs.html */);
		wcscat_c(szAdd, L"\r\n\r\n" L"The only possible workaround: enabling ‘Inject ConEmuHk’\r\n");
		wcscat_c(szAdd, CEHOOKSWIKI /* http://conemu.github.io/en/ConEmuHk.html */);
		wcscat_c(szAdd, L"\r\n\r\n");
	}

	wcscat_c(szAdd, dumpInfo.fullInfo);

	if (dumpInfo.dumpFile[0])
	{
		wcscat_c(szAdd, L"\r\n\r\n" L"Memory dump was saved to\r\n");
		wcscat_c(szAdd, dumpInfo.dumpFile);

		if (!bKernelTrap)
		{
			wcscat_c(szAdd, L"\r\n\r\n" L"Please Zip it and send to developer (via DropBox etc.)\r\n");
			wcscat_c(szAdd, CEREPORTCRASH /* http://conemu.github.io/en/Issues.html... */);
		}
	}

	wcscat_c(szAdd, L"\r\n\r\nPress <Yes> to copy this text to clipboard");
	if (!bKernelTrap)
	{
		wcscat_c(szAdd, L"\r\nand open project web page");
	}

	// Message title
	wchar_t szTitle[100], szExe[MAX_PATH] = L"";
	GetModuleFileName(nullptr, szExe, countof(szExe));
	auto* pszExeName = const_cast<wchar_t*>(PointToName(szExe));
	if (pszExeName && lstrlen(pszExeName) > 63)
		pszExeName[63] = 0;
	swprintf_c(szTitle, L"%s crashed, PID=%u", pszExeName ? pszExeName : L"<process>", GetCurrentProcessId());

	const DWORD nMsgFlags = MB_YESNO|MB_ICONSTOP|MB_SYSTEMMODAL
		| (bKernelTrap ? MB_DEFBUTTON2 : 0);

	const int nBtn = MessageBox(nullptr, szAdd, szTitle, nMsgFlags);
	if (nBtn == IDYES)
	{
		CopyToClipboard(szAdd);
		if (!bKernelTrap)
		{
			ShellExecute(nullptr, L"open", CEREPORTCRASH, nullptr, nullptr, SW_SHOWNORMAL);
		}
	}

	LONG lExRc = EXCEPTION_EXECUTE_HANDLER;

	if (gpfnPrevExFilter)
	{
		// если фильтр уже был установлен перед нашим - будем звать его
		// все-равно уже свалились, на валидность адреса можно не проверяться
		lExRc = gpfnPrevExFilter(ExceptionInfo);
	}

	std::ignore = dwErr;
	return lExRc;
}

void SetupCreateDumpOnException()
{
	if (gbCreateDumpOnExceptionInstalled)
	{
		return;
	}

	if (gState.runMode_ == RunMode::GuiMacro)
	{
		// Must not be called in GuiMacro mode!
		_ASSERTE(gState.runMode_!=RunMode::GuiMacro);
		return;
	}

	if (gState.runMode_ == RunMode::Server || gState.runMode_ == RunMode::Comspec)
	{
		// ok, allow
	}
	else if (gState.runMode_ == RunMode::AltServer)
	{
		// By default, handler is not installed in AltServer
		// gpSet->isConsoleExceptionHandler --> CECF_ConExcHandler
		const bool allowHandler = WorkerServer::IsCrashHandlerAllowed();
		if (!allowHandler)
			return; // disabled in ConEmu settings
	}
	else
	{
		// disallow in all other modes
		return;
	}

	// Far 3.x, telnet, Vim, etc.
	// In these programs ConEmuCD.dll could be loaded to deal with alternative buffer and TrueColor
	if (!gpfnPrevExFilter && !IsDebuggerPresent())
	{
		gpfnPrevExFilter = SetUnhandledExceptionFilter(CreateDumpOnException);
		gbCreateDumpOnExceptionInstalled = true;
	}
}

void DoneCreateDumpOnException()
{
	SetUnhandledExceptionFilter(gpfnPrevExFilter);
}

bool IsCreateDumpOnExceptionInstalled()
{
	return gbCreateDumpOnExceptionInstalled;
}
