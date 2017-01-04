
/*
Copyright (c) 2015-2017 Maximus5
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

#include <windows.h>
#include "../common/defines.h"
#include "../common/Common.h"
#include "../ConEmuCD/ExitCodes.h"
#include "Connector.h"
#include "hkConsole.h"

static bool gbWasStarted = false;
static bool gbTermVerbose = false;
static bool gbTerminateReadInput = false;
static HANDLE ghTermInput = NULL;
static DWORD gnTermPrevMode = 0;
static UINT gnPrevConsoleCP = 0;
static LONG gnInTermInputReading = 0;

static void write_verbose(const char *buf, int arg1 = 0, int arg2 = 0, int arg3 = 0, int arg4 = 0)
{
	char szBuf[1024]; // don't use static here!
	if (strchr(buf, '%'))
	{
		msprintf(szBuf, countof(szBuf), buf, arg1, arg2, arg3);
		buf = szBuf;
	}
	WriteProcessedA(buf, (DWORD)-1, NULL, wps_Output);
}

// If ENABLE_PROCESSED_INPUT is set, cygwin application are terminated without opportunity to survive
static DWORD ProtectCtrlBreakTrap(HANDLE h_input)
{
	if (gbTerminateReadInput)
		return 0;

	DWORD conInMode = 0;
	if (GetConsoleMode(h_input, &conInMode) && (conInMode & ENABLE_PROCESSED_INPUT))
	{
		if (gbTermVerbose)
			write_verbose("\033[31;40m{PID:%u,TID:%u} dropping ENABLE_PROCESSED_INPUT flag\033[m\r\n", GetCurrentProcessId(), GetCurrentThreadId());
		if (!gbTerminateReadInput)
			SetConsoleMode(h_input, (conInMode & ~ENABLE_PROCESSED_INPUT));
	}

	return conInMode;
}

static BOOL WINAPI termReadInput(PINPUT_RECORD pir, DWORD nCount, PDWORD pRead)
{
	if (gbTerminateReadInput)
		return FALSE;

	if (!ghTermInput)
		ghTermInput = GetStdHandle(STD_INPUT_HANDLE);

	ProtectCtrlBreakTrap(ghTermInput);

	UpdateAppMapFlags(rcif_LLInput);

	InterlockedIncrement(&gnInTermInputReading);
	BOOL bRc = ReadConsoleInputW(ghTermInput, pir, nCount, pRead);
	InterlockedDecrement(&gnInTermInputReading);

	if (!bRc)
		return FALSE;
	if (gbTerminateReadInput)
		return FALSE;

	return TRUE;
}

int WINAPI RequestTermConnector(/*[IN/OUT]*/RequestTermConnectorParm* Parm)
{
	//_ASSERTE(FALSE && "ConEmuHk. Continue to RequestTermConnector");

	int iRc = CERR_CARGUMENT;
	if (!Parm || (Parm->cbSize != sizeof(*Parm)))
	{
		//TODO: Return in ->pszError pointer to error description
		goto wrap;
	}

	switch (Parm->Mode)
	{
	case rtc_Start:
	{
		gbTermVerbose = (Parm->bVerbose != FALSE);

		if (gbTermVerbose)
			write_verbose("\r\n\033[31;40m{PID:%u} Starting ConEmu xterm mode\033[m\r\n", GetCurrentProcessId());

		ghTermInput = GetStdHandle(STD_INPUT_HANDLE);
		gnTermPrevMode = ProtectCtrlBreakTrap(ghTermInput);

		gnPrevConsoleCP = GetConsoleCP();
		if (gnPrevConsoleCP != 65001)
		{
			if (gbTermVerbose)
				write_verbose("\r\n\033[31;40m{PID:%u} changing console CP from %u to utf-8\033[m\r\n", GetCurrentProcessId(), gnPrevConsoleCP);
			OnSetConsoleCP(65001);
			OnSetConsoleOutputCP(65001);
		}

		Parm->ReadInput = termReadInput;
		Parm->WriteText = WriteProcessedA;

		CEAnsi::StartXTermMode(true);
		gbWasStarted = true;
		iRc = 0;
		break;
	}

	case rtc_Stop:
	{
		gbTerminateReadInput = true;

		if (gbTermVerbose)
			write_verbose("\r\n\033[31;40m{PID:%u} Terminating input reader\033[m\r\n", GetCurrentProcessId());

		// Ensure, that ReadConsoleInputW will not block
		if (gbWasStarted || (gnInTermInputReading > 0))
		{
			INPUT_RECORD r = {KEY_EVENT}; DWORD nWritten = 0;
			WriteConsoleInputW(ghTermInput ? ghTermInput : GetStdHandle(STD_INPUT_HANDLE), &r, 1, &nWritten);
		}

		// If Console Input Mode was changed
		if (gnTermPrevMode && ghTermInput)
		{
			DWORD conInMode = 0;
			if (GetConsoleMode(ghTermInput, &conInMode) && (conInMode != gnTermPrevMode))
			{
				if (gbTermVerbose)
					write_verbose("\r\n\033[31;40m{PID:%u} reverting ConInMode to 0x%08X\033[m\r\n", GetCurrentProcessId(), gnTermPrevMode);
				SetConsoleMode(ghTermInput, gnTermPrevMode);
			}
		}

		// If Console CodePage was changed
		if (gnPrevConsoleCP && (GetConsoleCP() != gnPrevConsoleCP))
		{
			if (gbTermVerbose)
				write_verbose("\r\n\033[31;40m{PID:%u} reverting console CP from %u to %u\033[m\r\n", GetCurrentProcessId(), GetConsoleCP(), gnPrevConsoleCP);
			OnSetConsoleCP(gnPrevConsoleCP);
			OnSetConsoleOutputCP(gnPrevConsoleCP);
		}

		gbWasStarted = false;
		iRc = 0;
		break;
	}

	default:
		Parm->pszError = "Unsupported mode";
		goto wrap;
	}
wrap:
	return iRc;
}
