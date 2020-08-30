
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

#pragma once

#include "../common/defines.h"
#include "../common/MConHandle.h"

namespace StdCon {

	bool AttachParentConsole(DWORD parentPid);

	void SetWin32TermMode();

	// Restore "native" console functionality on cygwin/msys/wsl handles?
	void ReopenConsoleHandles(STARTUPINFO* si);

	struct ReopenedHandles final
	{
		static void Deleter(LPARAM lParam);

		ReopenedHandles();

		ReopenedHandles(const ReopenedHandles&) = delete;
		ReopenedHandles(ReopenedHandles&&) = delete;
		ReopenedHandles& operator=(const ReopenedHandles&) = delete;
		ReopenedHandles& operator=(ReopenedHandles&&) = delete;

		~ReopenedHandles();

		bool Reopen(STARTUPINFO* si);

		struct {
			DWORD channel_ = 0;
			const wchar_t* name_ = nullptr;
			MConHandle* handle_ = nullptr;
			HANDLE prevHandle_ = nullptr;
		} handles_[3] = {
			{ STD_INPUT_HANDLE, L"CONIN$" },  // NOLINT(clang-diagnostic-missing-field-initializers)
			{ STD_OUTPUT_HANDLE, L"CONOUT$" },  // NOLINT(clang-diagnostic-missing-field-initializers)
			// "CONERR$" does not exist, so we use "CONOUT$" for STD_ERROR_HANDLE
			{ STD_ERROR_HANDLE, L"CONOUT$" },  // NOLINT(clang-diagnostic-missing-field-initializers)
		};
		bool reopened_ = false;
		SECURITY_ATTRIBUTES sec_ = { sizeof(sec_), nullptr, TRUE };
	};

	// extern ReopenedHandles* gReopenedHandles = nullptr;

};
