
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

#include "defines.h"
#include "HandleKeeper.h"

#include "MMap.h"
#include "MArray.h"
#include "WErrGuard.h"

#include <memory>
#include <tuple>

#ifdef _DEBUG
#include "MStrDup.h"
#include <deque>
#include <mutex>
#endif

namespace HandleKeeper
{
	static std::shared_ptr<MMap<HANDLE,HandleInformation>> gpHandles{};  // NOLINT(clang-diagnostic-exit-time-destructors)
	static bool gbIsConnector = false;

	#ifdef _DEBUG
	static std::mutex gDeletedLock{};  // NOLINT(clang-diagnostic-exit-time-destructors)
	static std::deque<HandleInformation, MArrayAllocator<HandleInformation>>* gpDeletedBuffer{ nullptr };  // NOLINT(clang-diagnostic-exit-time-destructors)
	static size_t gDeletedMaxCount = 256;
	#endif

	static std::shared_ptr<MMap<HANDLE,HandleInformation>> GetHandleStorage();
	static void FillHandleInfo(HandleInformation& info, DWORD access);
	static bool CheckConName(HandleSource source, DWORD access, const VOID* as_name, HandleInformation& info);
};

// MT-Safe, lock-free
static std::shared_ptr<MMap<HANDLE,HandleInformation>> HandleKeeper::GetHandleStorage()
{
	if (gpHandles == nullptr)
	{
		auto pHandles = std::make_shared<MMap<HANDLE, HandleInformation>>();
		if (pHandles)
		{
			if (!pHandles->Init(256/*may be auto-enlarged*/, true/*OnCreate*/))
			{
				pHandles->Release();
				pHandles.reset();
			}
			else
			{
				gpHandles = std::move(pHandles);
			}
		}
	}

	return gpHandles;
}

void HandleKeeper::ReleaseHandleStorage()
{
	gpHandles.reset();
	#ifdef _DEBUG
	SafeDelete(gpDeletedBuffer);
	#endif
}

void HandleKeeper::SetConnectorMode(const bool isConnector)
{
	gbIsConnector = isConnector;
}

// Check if handle has console capabilities
static void HandleKeeper::FillHandleInfo(HandleInformation& info, const DWORD access)
{
	// We can't set is_input/is_output/is_error by info.source (hs_StdXXX)
	// because STD_XXX handles may be pipes or files

	// ReSharper disable CppJoinDeclarationAndAssignment
	BOOL b2; DWORD m; CONSOLE_SCREEN_BUFFER_INFO csbi = {}; DEBUGTEST(DWORD err;)
	// Is handle capable to full set of console API?
	const BOOL b1 = GetConsoleMode(info.h, &m);
	if (!b1)
	{
		#ifdef _DEBUG
		info.errorPlace = 1;
		info.errorCode = GetLastError();
		#endif
		// In connector mode all handles are expected to be AnsiCapable!
		_ASSERTE(!gbIsConnector);
	}
	else
	{
		b2 = GetConsoleScreenBufferInfo(info.h, &csbi);
		DEBUGTEST(err = GetLastError());
		if (b2)
		{
			if (!info.is_output && !info.is_error)
			{
				if (info.source == HandleSource::StdErr)
					info.is_error = true;
				else
					info.is_output = true;
			}

			_ASSERTE(info.is_input==false);
			info.is_input = false;

			info.is_ansi = true;

			#ifdef _DEBUG
			info.csbi = csbi;
			#endif
		}
		else
		{
			#ifdef _DEBUG
			info.errorPlace = 2;
			info.errorCode = GetLastError();
			#endif
			if (info.source == HandleSource::StdIn && !info.is_input)
				info.is_input = true;
			// In connector mode all handles are expected to be AnsiCapable!
			_ASSERTE(!gbIsConnector);
			_ASSERTE(info.is_ansi == false);
			//_ASSERTE(info.is_output == false && info.is_error == false); -- STD_OUTPUT_HANDLE can be "virtual"
		}
	}

	// Input/Output handles must have either GENERIC_READ or GENERIC_WRITE rights
	if ((info.source > HandleSource::StdErr) && !(access & (GENERIC_WRITE|GENERIC_READ)))
	{
		info.is_invalid = true;
	}
	// Generic handle "CON" opened for output (redirection to console)
	else if (info.is_con && (access & GENERIC_WRITE))
	{
		info.is_output = true;
	}
}

// Check if as_name is "CON", "CONIN$" or "CONOUT$"
static bool HandleKeeper::CheckConName(const HandleSource source, const DWORD access, const VOID* as_name, HandleInformation& info)
{
	// Just a simple check for "string" validity
	if (as_name && (reinterpret_cast<DWORD_PTR>(as_name) & ~0xFFFF))
	{
		if (source == HandleSource::CreateFileW)
		{
			if (access & (GENERIC_READ|GENERIC_WRITE))
			{
				info.is_con = (lstrcmpiW(static_cast<LPCWSTR>(as_name), L"CON") == 0);
				info.is_input = (lstrcmpiW(static_cast<LPCWSTR>(as_name), L"CONIN$") == 0);
				info.is_output = (lstrcmpiW(static_cast<LPCWSTR>(as_name), L"CONOUT$") == 0);
			}

			#ifdef _DEBUG
			// debugging and informational purposes only, released via file_name_ptr
			info.file_name_w = CEStr(static_cast<LPCWSTR>(as_name)).Detach();
			#endif

			return (info.is_con || info.is_input || info.is_output);
		}

		if (source == HandleSource::CreateFileA)
		{
			if (access & (GENERIC_READ|GENERIC_WRITE))
			{
				info.is_con = (lstrcmpiA(static_cast<LPCSTR>(as_name), "CON") == 0);
				info.is_input = (lstrcmpiA(static_cast<LPCSTR>(as_name), "CONIN$") == 0);
				info.is_output = (lstrcmpiA(static_cast<LPCSTR>(as_name), "CONOUT$") == 0);
			}

			#ifdef _DEBUG
			// debugging and informational purposes only, released via file_name_ptr
			info.file_name_a = CEStrA(static_cast<LPCSTR>(as_name)).Detach();
			#endif

			return (info.is_con || info.is_input || info.is_output);
		}
	}

	return false;
}

bool HandleKeeper::PreCreateHandle(HandleSource source, DWORD& dwDesiredAccess, DWORD& dwShareMode, const void** as_name)
{
	if ((source != HandleSource::CreateFileA) && (source != HandleSource::CreateFileW))
	{
		_ASSERTE((source == HandleSource::CreateFileA) || (source == HandleSource::CreateFileW));
		return false;
	}
	if (!as_name || !*as_name)
	{
		_ASSERTE(as_name && *as_name);
		return false;
	}

	HandleInformation info = {};

	// Check if as_name is "CON", "CONIN$" or "CONOUT$"
	if (CheckConName(source, dwDesiredAccess, *as_name, info))
	{
		// If handle is opened with limited right, console API would be very limited
		// and we even can't query current console window properties (cursor pos, buffer size, etc.)
		// This hack may be removed if we rewrite all console API functions completely
		if (info.is_output
			|| (info.is_con && (dwDesiredAccess & GENERIC_WRITE))
			)
		{
			if ((dwDesiredAccess & (GENERIC_READ|GENERIC_WRITE)) != (GENERIC_READ|GENERIC_WRITE))
				dwDesiredAccess |= (GENERIC_READ|GENERIC_WRITE);

			if ((dwShareMode & (FILE_SHARE_READ|FILE_SHARE_WRITE)) != (FILE_SHARE_READ|FILE_SHARE_WRITE))
				dwShareMode |= (FILE_SHARE_READ|FILE_SHARE_WRITE);

			// We have to change opening name to 'CONOUT$' or CreateFile will fail
			if (info.is_con)
			{
				static char szConOut[] = "CONOUT$";
				static wchar_t wszConOut[] = L"CONOUT$";
				if (source == HandleSource::CreateFileA)
					*as_name = szConOut;
				else if (source == HandleSource::CreateFileW)
					*as_name = wszConOut;
			}
		}
	}

	#ifdef _DEBUG
	SafeFree(info.file_name_ptr);
	#endif

	return false;
}

bool HandleKeeper::AllocHandleInfo(HANDLE h, HandleSource source, DWORD access, const void* as_name, HandleInformation* pInfo /*= nullptr*/)
{
	if (!h || (h == INVALID_HANDLE_VALUE))
		return false;

	auto storage = GetHandleStorage();
	if (!storage)
		return false;

	HandleInformation info = {};
	info.h = h;
	info.source = source;

	// Check if as_name is "CON", "CONIN$" or "CONOUT$"
	if ((((source == HandleSource::CreateFileA) || (source == HandleSource::CreateFileW))
			&& CheckConName(source, access, as_name, info))
		|| (source <= HandleSource::CreateConsoleScreenBuffer)
		)
	{
		// Check if handle has console capabilities
		FillHandleInfo(info, access);
	}

	if (pInfo)
		*pInfo = info;

	storage->Set(h, info);

	return true;
}

bool HandleKeeper::QueryHandleInfo(HANDLE h, HandleInformation& info, const bool ansiOutExpected)
{
	CLastErrorGuard errGuard;

	if (!h || (h == INVALID_HANDLE_VALUE))
		return false;
	bool bFound = false;

	auto storage = GetHandleStorage();
	if (storage)
	{
		bFound = storage->Get(h, &info);
	}

	if (bFound && ansiOutExpected)
	{
		// Force to refresh handle information
		if (gbIsConnector && (!info.is_ansi || !(info.is_output || info.is_error)))
		{
			// In connector mode all handles are expected to be AnsiCapable!
			_ASSERTE((info.is_ansi && (info.is_output || info.is_error)) || !gbIsConnector);
			bFound = false;
		}
	}

	if (!bFound)
		bFound = AllocHandleInfo(h, HandleSource::Unknown, 0, nullptr, &info);

	return bFound;
}

void HandleKeeper::CloseHandleInfo(HANDLE h)
{
	HandleInformation info = {};
	auto storage = GetHandleStorage();
	if (storage)
	{
		if (storage->Get(h, &info, true))
		{
			#ifdef _DEBUG
			if (info.file_name_ptr)
				free(info.file_name_ptr);
			std::lock_guard<std::mutex> lock(gDeletedLock);
			if (!gpDeletedBuffer)
				gpDeletedBuffer = new std::deque<HandleInformation, MArrayAllocator<HandleInformation>>;
			gpDeletedBuffer->push_back(info);
			while (gpDeletedBuffer->size() > gDeletedMaxCount)
				gpDeletedBuffer->pop_front();
			#endif

			std::ignore = info.h;
		}
	}
}

bool HandleKeeper::IsAnsiCapable(HANDLE hFile, bool *pbIsConOut)
{
	HandleInformation info = {};
	if (!QueryHandleInfo(hFile, info, true))
		return false;
	if (pbIsConOut)
		*pbIsConOut = (info.is_output || info.is_error);
	return (info.is_ansi);
}

bool HandleKeeper::IsOutputHandle(HANDLE hFile)
{
	HandleInformation info = {};
	if (!QueryHandleInfo(hFile, info, true))
		return false;
	return (info.is_output || info.is_error);
}

bool HandleKeeper::IsInputHandle(HANDLE hFile)
{
	HandleInformation info = {};
	if (!QueryHandleInfo(hFile, info, false))
		return false;
	return (info.is_input);
}
