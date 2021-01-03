
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

#define HIDE_USE_EXCEPTION_INFO

#undef USE_PTY_IMPL
//#define USE_PTY_IMPL

#include "../common/defines.h"
#include "../common/Common.h"
#include "../common/HandleKeeper.h"
#include "../common/MArray.h"
#include "../common/MPipe.h"
#include "../ConEmuCD/ExitCodes.h"
#include "Connector.h"
#include "hkConsole.h"
#include "DllOptions.h"
#include <deque>
#include <chrono>
#include <thread>

#include "Ansi.h"

namespace Connector
{

namespace {
bool gbWasStarted = false;
bool gbTermVerbose = false;
bool gbTerminateReadInput = false;
HANDLE ghTermInput = nullptr;
DWORD gnTermPrevMode = 0;
UINT gnPrevConsoleCP = 0;
std::atomic_int gnInTermInputReading;
struct BlockInputProcess
{
	HANDLE handle;
	DWORD pid;
};
BlockInputProcess gBlockInputProcess = {};
/// Pipe handles for reading keyboard input and writing application output
MPipeDual* gInOut = nullptr;
/// Input queue for keyboard/mouse events
std::deque<INPUT_RECORD, MArrayAllocator<INPUT_RECORD>>* gInputEvents = nullptr;

class InputBuffer
{
public:
	ReadInputResult ReadInput(PINPUT_RECORD pir, DWORD nCount, PDWORD pRead)
	{
		LoadBuffer();

		DWORD read = 0;
		while (read < nCount)
		{
			const auto add = GetInputBlock(pir + read, nCount - read);
			if (!add)
				break;

			#ifdef _DEBUG
			wchar_t dbgInfo[80] = L"";
			msprintf(dbgInfo, std::size(dbgInfo), L"==== ReadInput: read %u add %u %s\n",
				read, add, IsSequenceEnd(pir[read + add - 1]) ? L"end" : L"");
			OutputDebugStringW(dbgInfo);
			#endif

			read += add;
			if (IsSequenceEnd(pir[read - 1]))
				break;
		}

		*pRead = read;
		const ReadInputResult result = (read == 0) ? rir_None
			: buffer_.empty() ? rir_Ready
			: rir_Ready_More;
		if (read)
			DumpReadInput(pir, read, result);
		return result;
	}

	void DumpReadInput(const INPUT_RECORD* pir, const DWORD read, const ReadInputResult result) const
	{
		#ifdef _DEBUG
		static std::chrono::high_resolution_clock::time_point prev = {};

		DWORD down = 0;
		for (DWORD i = 0; i < read; ++i)
		{
			if (pir[i].EventType == KEY_EVENT && pir[i].Event.KeyEvent.bKeyDown)
				++down;
		}

		std::chrono::milliseconds delay = {};
		if (prev != std::chrono::high_resolution_clock::time_point())
			delay = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - prev);

		wchar_t dbgInfo[256] = L"";
		msprintf(dbgInfo, countof(dbgInfo), L"termReadInput: delay=%u count=%u down=%u%s",
			static_cast<unsigned>(delay.count()), read, down, result == rir_Ready_More ? L" (more)" : L" (none)");
		wchar_t* ptr = dbgInfo + wcslen(dbgInfo);
		const wchar_t* ptrEnd = dbgInfo + countof(dbgInfo) - 8;
		for (DWORD i = 0; i < read; ++i)
		{
			wchar_t chr[32] = L"";
			if (pir[i].EventType != KEY_EVENT)
			{
				msprintf(chr, countof(chr) - 1, L" type=%u", pir[i].EventType);
			}
			else
			{
				const auto& ke = pir[i].Event.KeyEvent;
				const auto* format = (ke.uChar.UnicodeChar && ke.bKeyDown) ? L" x%02X"
					: (ke.uChar.UnicodeChar && !ke.bKeyDown) ? L" (x%02X)"
					: (!ke.uChar.UnicodeChar && ke.bKeyDown) ? L" %u"
					: L" (%u)";
				msprintf(chr, countof(chr) - 1, format, ke.uChar.UnicodeChar ? static_cast<UINT>(ke.uChar.UnicodeChar) : ke.wVirtualKeyCode);
				if (ke.wVirtualScanCode == sequenceMark_)
				{
					wcscat_s(chr, L" <X>");
				}
			}
			const auto addLen = wcslen(chr);
			if (ptr + addLen >= ptrEnd)
			{
				wcscpy_s(ptr, 8, L" ...");
				ptr += 4;
				break;
			}
			wcscpy_s(ptr, addLen + 1, chr);
			ptr += addLen;
		}
		*(ptr++) = L'\n';
		*ptr = L'\0';
		OutputDebugStringW(dbgInfo);

		prev = std::chrono::high_resolution_clock::now();
		#endif
	}

protected:
	std::deque<INPUT_RECORD, MArrayAllocator<INPUT_RECORD>> buffer_;
	const WORD sequenceMark_{ 255 };
	const std::chrono::milliseconds readToDelay_{ 250 };
	bool inSequence_ = false;

	bool IsSequenceNew(const INPUT_RECORD& ir) const
	{
		return (ir.EventType == KEY_EVENT && ir.Event.KeyEvent.bKeyDown
			&& ir.Event.KeyEvent.uChar.UnicodeChar == 0x1B && ir.Event.KeyEvent.wVirtualScanCode == sequenceMark_);
	}

	bool IsSequenceEnd(const INPUT_RECORD& ir) const
	{
		return (ir.EventType == KEY_EVENT && !ir.Event.KeyEvent.bKeyDown && ir.Event.KeyEvent.wVirtualScanCode == sequenceMark_);
	}

	DWORD GetInputBlock(PINPUT_RECORD pir, const DWORD nCount)
	{
		if (buffer_.empty())
			return 0;

		if (IsSequenceNew(buffer_.front()))
			inSequence_ = true;

		DWORD read = 0;
		const auto start = std::chrono::high_resolution_clock::now();
		for (DWORD i = 0; i < nCount && !buffer_.empty(); ++i)
		{
			const auto& ir = buffer_.front();

			// beginning of the new ESC sequence?
			if (i > 0 && IsSequenceNew(ir))
			{
				// we already collected in `pir` previous sequence or some text, return it first
				break;
			}

			pir[i] = ir;
			buffer_.pop_front();
			++read;

			if (inSequence_)
			{
				if (IsSequenceEnd(pir[i]))
				{
					inSequence_ = false;
					break;
				}
				// if we still are waiting for the sequence tail
				if (buffer_.empty())
				{
					// try to wait for a while and read input
					auto now = std::chrono::high_resolution_clock::now();
					while (now - start < readToDelay_)
					{
						std::this_thread::sleep_for(std::chrono::milliseconds(10));
						if (LoadBuffer())
							break; // success
						now = std::chrono::high_resolution_clock::now();
					}
					if (now - start >= readToDelay_)
					{
						inSequence_ = false;
					}
				}
			}
		}
		return read;
	}

	bool LoadBuffer()
	{
		INPUT_RECORD buffer[64] = {};
		DWORD peek = 0;
		if (!PeekConsoleInputW(ghTermInput, buffer, static_cast<DWORD>(std::size(buffer)), &peek) || !peek)
			return false;
		DWORD read = 0;
		if (!ReadConsoleInputW(ghTermInput, buffer, peek, &read) || !read)
			return false;
		for (size_t i = 0; i < read; ++i)
		{
			buffer_.push_back(buffer[i]);
		}
		return true;
	}
};

InputBuffer* gInputBuffer = nullptr;
}

static BOOL WINAPI writeTermOutput(LPCSTR lpBuffer, DWORD nNumberOfCharsToWrite, LPDWORD lpNumberOfCharsWritten, WriteProcessedStream Stream);

static void writeVerbose(const char *buf, int arg1 = 0, int arg2 = 0, int arg3 = 0, int arg4 = 0)
{
	char szBuf[1024]; // don't use static here!
	if (strchr(buf, '%'))
	{
		msprintf(szBuf, countof(szBuf), buf, arg1, arg2, arg3);
		buf = szBuf;
	}
	writeTermOutput(buf, static_cast<DWORD>(-1), nullptr, wps_Output);
}

/// If ENABLE_PROCESSED_INPUT is set, cygwin applications are terminated without opportunity to survive
static DWORD protectCtrlBreakTrap(HANDLE h_input)
{
	if (gbTerminateReadInput)
		return 0;

	DWORD conInMode = 0;
	if (GetConsoleMode(h_input, &conInMode) && (conInMode & ENABLE_PROCESSED_INPUT))
	{
		if (gbTermVerbose)
			writeVerbose("\033[31;40m{PID:%u,TID:%u} dropping ENABLE_PROCESSED_INPUT flag\033[m\r\n", GetCurrentProcessId(), GetCurrentThreadId());
		if (!gbTerminateReadInput)
			SetConsoleMode(h_input, (conInMode & ~ENABLE_PROCESSED_INPUT));
	}

	return conInMode;
}

/// Called after "ConEmuC -STD -c bla-bla-bla" to allow WinAPI read input exclusively
void pauseReadInput(DWORD pid)
{
	if (!pid)
		return;
	HANDLE h = OpenProcess(SYNCHRONIZE, FALSE, pid);
	// Don't try again to open process
	gBlockInputProcess.pid = pid;
	if (!h || h == INVALID_HANDLE_VALUE)
		return; // -V773
	std::swap(h, gBlockInputProcess.handle);
	SafeCloseHandle(h);
}

/// If user started "ConEmuC -STD -c far.exe" we shall not read CONIN$ anymore
static bool mayReadInput()
{
	if (gbTerminateReadInput)
		return false;

	if (!gBlockInputProcess.handle)
	{
		const CESERVER_CONSOLE_MAPPING_HDR* pSrv = GetConMap();
		if (pSrv && pSrv->stdConBlockingPID && pSrv->stdConBlockingPID != gBlockInputProcess.pid)
		{
			pauseReadInput(pSrv->stdConBlockingPID);
		}
	}

	if (gBlockInputProcess.handle)
	{
		const DWORD rc = WaitForSingleObject(gBlockInputProcess.handle, 15);
		if (rc == WAIT_TIMEOUT)
			return false;
		_ASSERTE(rc == WAIT_OBJECT_0);
		// ConEmuC was terminated, return to normal operation
		gBlockInputProcess.pid = 0;
		HANDLE h = nullptr; std::swap(h, gBlockInputProcess.handle);
		SafeCloseHandle(h);
		// Restore previous modes
		CEAnsi::RefreshXTermModes();
	}

	return true;
}

/// Called from connector binary
static ReadInputResult WINAPI termReadInput(PINPUT_RECORD pir, DWORD nCount, PDWORD pRead)
{
	if (!mayReadInput() || !pir || !pRead)
		return rir_None;

	if (!ghTermInput)
		ghTermInput = GetStdHandle(STD_INPUT_HANDLE);

	protectCtrlBreakTrap(ghTermInput);

	UpdateAppMapFlags(rcif_LLInput);

	BOOL bRc = FALSE;
	ReadInputResult result = rir_None;
	++gnInTermInputReading;
	if (!gInOut || !gInputEvents)
	{
		result = gInputBuffer->ReadInput(pir, nCount, pRead);
		bRc = (result != rir_None);
	}
	else
	{
		const auto events = gInOut->Read(false);
		if (events.first && events.second)
		{
			const INPUT_RECORD* p = static_cast<const INPUT_RECORD*>(events.first);
			const INPUT_RECORD* const pEnd = reinterpret_cast<const INPUT_RECORD*>(static_cast<char*>(events.first) + events.second);
			for (; p + 1 <= pEnd; ++p)
			{
				gInputEvents->push_back(*p);
			}
			if (p != pEnd)
			{
				_ASSERTE(p == pEnd); // broken format of block?
				gInputBuffer->DumpReadInput(pir, 0, result);
				return rir_None;
			}
		}
		if (!gInputEvents->empty())
		{
			bRc = TRUE;
			DWORD i = 0;
			for (; i < nCount && !gInputEvents->empty(); ++i)
			{
				pir[i] = gInputEvents->front();
				gInputEvents->pop_front();
			}
			*pRead = i;
			if (!gInputEvents->empty())
				result = rir_Ready_More;
			else
				result = rir_Ready;

			gInputBuffer->DumpReadInput(pir, i, result);
		}
	}
	--gnInTermInputReading;

	if (!bRc)
		return rir_None;
	if (gbTerminateReadInput)
		return rir_None;

	return result;
}

/// called from connector binary
static BOOL WINAPI writeTermOutput(LPCSTR lpBuffer, DWORD nNumberOfCharsToWrite, LPDWORD lpNumberOfCharsWritten, WriteProcessedStream Stream)
{
	if (!lpBuffer || !*lpBuffer)
		return false;
	if (nNumberOfCharsToWrite == static_cast<DWORD>(-1))
		nNumberOfCharsToWrite = lstrlenA(lpBuffer);

	BOOL rc;
	DWORD temp = 0;

	// If server was not prepared yet...
	if (!gInOut)
	{
		rc = WriteProcessedA(lpBuffer, nNumberOfCharsToWrite, lpNumberOfCharsWritten ? lpNumberOfCharsWritten : &temp, Stream);
	}
	else
	{
		rc = gInOut->Write(lpBuffer, nNumberOfCharsToWrite);
		if (lpNumberOfCharsWritten)
			*lpNumberOfCharsWritten = rc ? nNumberOfCharsToWrite : 0;
	}

	return rc;
}

/// Prepare ConEmuHk to process calls from connector binary
/// @param Parm - In/Out parameter with initialization info
/// @result returns 0 on success, otherwise error code with description in *Parm.pszError*
static int startConnector(/*[IN/OUT]*/RequestTermConnectorParm& Parm)
{
	gbTermVerbose = (Parm.bVerbose != FALSE);

	if (gbTermVerbose)
		writeVerbose("\r\n\033[31;40m{PID:%u} Starting ConEmu xterm mode\033[m\r\n", GetCurrentProcessId());

	#ifdef USE_PTY_IMPL
	BYTE start = TRUE;
	if (CESERVER_REQ* pOut = ExecuteSrvCmd(gnServerPID, CECMD_STARTPTYSRV, sizeof(start), &start, ghConWnd))
	{
		if (gInOut)
		{
			_ASSERTE(gInOut==nullptr);
			delete gInOut;
		}
		gInOut = new MPipeDual(HANDLE(pOut->qwData[0]), HANDLE(pOut->qwData[1]));
		gInputEvents = new std::deque<INPUT_RECORD, MArrayAllocator<INPUT_RECORD>>();
		ExecuteFreeResult(pOut);
	}
	else
	{
		SafeDelete(gInOut);
		SafeDelete(gInputEvents);
		Parm.pszError = "CECMD_STARTPTYSRV failed";
		return -1;
	}
	#endif

	if (gInputBuffer)
	{
		_ASSERTE(gInputBuffer != nullptr);
		SafeDelete(gInputBuffer);
	}
	gInputBuffer = new InputBuffer;

	ghTermInput = GetStdHandle(STD_INPUT_HANDLE);
	gnTermPrevMode = protectCtrlBreakTrap(ghTermInput);

	gnPrevConsoleCP = GetConsoleCP();
	if (gnPrevConsoleCP != 65001)
	{
		if (gbTermVerbose)
			writeVerbose("\r\n\033[31;40m{PID:%u} changing console CP from %u to utf-8\033[m\r\n", GetCurrentProcessId(), gnPrevConsoleCP);
		OnSetConsoleCP(65001);
		OnSetConsoleOutputCP(65001);
	}

	Parm.ReadInput = termReadInput;
	Parm.WriteText = writeTermOutput;

	if (Parm.pszMntPrefix)
	{
		CESERVER_REQ* pOut = ExecuteGuiCmd(ghConWnd, CECMD_STARTCONNECTOR,
			lstrlenA(Parm.pszMntPrefix)+1, reinterpret_cast<LPCBYTE>(Parm.pszMntPrefix), ghConWnd);
		ExecuteFreeResult(pOut);
	}

	CEAnsi::StartXTermMode(true);
	gbWasStarted = true;
	HandleKeeper::SetConnectorMode(true);

	return 0;
}

int stopConnector(/*[IN/OUT]*/RequestTermConnectorParm& Parm)
{
	gbTerminateReadInput = true;

	if (gbTermVerbose)
		writeVerbose("\r\n\033[31;40m{PID:%u} Terminating input reader\033[m\r\n", GetCurrentProcessId());

	// Ensure, that ReadConsoleInputW will not block
	if (gbWasStarted || (gnInTermInputReading > 0))
	{
		INPUT_RECORD r = {KEY_EVENT, {}}; DWORD nWritten = 0;
		WriteConsoleInputW(ghTermInput ? ghTermInput : GetStdHandle(STD_INPUT_HANDLE), &r, 1, &nWritten);
	}

	// If Console Input Mode was changed
	if (gnTermPrevMode && ghTermInput)
	{
		DWORD conInMode = 0;
		if (GetConsoleMode(ghTermInput, &conInMode) && (conInMode != gnTermPrevMode))
		{
			if (gbTermVerbose)
				writeVerbose("\r\n\033[31;40m{PID:%u} reverting ConInMode to 0x%08X\033[m\r\n", GetCurrentProcessId(), gnTermPrevMode);
			SetConsoleMode(ghTermInput, gnTermPrevMode);
		}
	}

	// If Console CodePage was changed
	if (gnPrevConsoleCP && (GetConsoleCP() != gnPrevConsoleCP))
	{
		if (gbTermVerbose)
			writeVerbose("\r\n\033[31;40m{PID:%u} reverting console CP from %u to %u\033[m\r\n", GetCurrentProcessId(), GetConsoleCP(), gnPrevConsoleCP);
		OnSetConsoleCP(gnPrevConsoleCP);
		OnSetConsoleOutputCP(gnPrevConsoleCP);
	}

	#ifdef USE_PTY_IMPL
	BYTE start = FALSE;
	if (CESERVER_REQ* pOut = ExecuteSrvCmd(gnServerPID, CECMD_STARTPTYSRV, sizeof(start), &start, ghConWnd))
	{
		ExecuteFreeResult(pOut);
	}

	SafeDelete(gInOut);
	SafeDelete(gInputEvents);
	#endif

	SafeDelete(gInputBuffer);

	SafeCloseHandle(gBlockInputProcess.handle);

	gbWasStarted = false;
	HandleKeeper::SetConnectorMode(false);

	return 0;
}

}

bool isConnectorStarted()
{
	return Connector::gbWasStarted;
}

/// exported function, connector entry point
/// The function is called from `conemu-cyg-32.exe` and other builds of connector
int WINAPI RequestTermConnector(/*[IN/OUT]*/RequestTermConnectorParm* Parm)
{
	// point to attach the debugger
	//_ASSERTE(FALSE && "ConEmuHk. Continue to RequestTermConnector");

	int iRc = CERR_CARGUMENT;
	if (!Parm || (Parm->cbSize != sizeof(*Parm)))
	{
		// #Connector Return in ->pszError pointer to error description?
		goto wrap;
	}

	switch (Parm->Mode)
	{
	case rtc_Start:
	{
		iRc = Connector::startConnector(*Parm);
		break;
	}

	case rtc_Stop:
	{
		iRc = Connector::stopConnector(*Parm);
		break;
	}

	default:
		Parm->pszError = "Unsupported mode";
		goto wrap;
	}
wrap:
	return iRc;
}
