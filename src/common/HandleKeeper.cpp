
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
#include "HandleKeeper.h"

#ifdef _DEBUG
#include "MStrDup.h"
#endif

namespace HandleKeeper
{
	static MMap<HANDLE,HandleInformation>* gp_Handles = NULL;

	static bool InitHandleStorage();
	static void FillHandleInfo(HandleInformation& Info, DWORD access);
	static bool CheckConName(HandleSource source, DWORD access, const VOID* as_name, HandleInformation& Info);
};

// MT-Safe, lock-free
static bool HandleKeeper::InitHandleStorage()
{
	if (gp_Handles == NULL)
	{
		MMap<HANDLE,HandleInformation>* pHandles = (MMap<HANDLE,HandleInformation>*)malloc(sizeof(*pHandles));
		if (pHandles)
		{
			if (!pHandles->Init(256/*may be auto-enlarged*/, true/*OnCreate*/))
			{
				pHandles->Release();
				free(pHandles);
			}
			else
			{
				MMap<HANDLE,HandleInformation>* p = (MMap<HANDLE,HandleInformation>*)InterlockedCompareExchangePointer((LPVOID*)&gp_Handles, pHandles, NULL);
				_ASSERTE(p == NULL || p != pHandles);
				if (pHandles != gp_Handles)
				{
					pHandles->Release();
					free(pHandles);
				}
			}
		}
	}

	return (gp_Handles != NULL);
}

void HandleKeeper::ReleaseHandleStorage()
{
	MMap<HANDLE,HandleInformation>* p = (MMap<HANDLE,HandleInformation>*)InterlockedExchangePointer((LPVOID*)&gp_Handles, NULL);
	if (p)
	{
		p->Release();
		free(p);
	}
}

// Check if handle has console capabilities
static void HandleKeeper::FillHandleInfo(HandleInformation& Info, DWORD access)
{
	// We can't set is_input/is_output/is_error by Info.source (hs_StdXXX)
	// because STD_XXX handles may be pipes or files

	// Is handle capable to full set of console API?
	BOOL b1, b2; DWORD m; CONSOLE_SCREEN_BUFFER_INFO csbi = {}; DEBUGTEST(DWORD err;)
	b1 = GetConsoleMode(Info.h, &m);
	if (!b1)
	{
		DEBUGTEST(err = GetLastError());
	}
	else
	{
		b2 = GetConsoleScreenBufferInfo(Info.h, &csbi);
		DEBUGTEST(err = GetLastError());
		if (b2)
		{
			if (!Info.is_output && !Info.is_error)
				Info.is_output = true;

			_ASSERTE(Info.is_input==false);
			Info.is_input = false;

			Info.is_ansi = true;

			#ifdef _DEBUG
			Info.csbi = csbi;
			#endif
		}
		else
		{
			_ASSERTE(Info.is_ansi == false);
			//_ASSERTE(Info.is_output == false && Info.is_error == false); -- STD_OUTPUT_HANDLE can be "virtual"
		}
	}

	// Input/Output handles must have either GENERIC_READ or GENERIC_WRITE rights
	if ((Info.source > hs_StdErr) && !(access & (GENERIC_WRITE|GENERIC_READ)))
	{
		Info.is_invalid = true;
	}
	// Generic handle "CON" opened for output (redirection to console)
	else if (Info.is_con && (access & GENERIC_WRITE))
	{
		Info.is_output = true;
	}
}

// Check if as_name is "CON", "CONIN$" or "CONOUT$"
static bool HandleKeeper::CheckConName(HandleSource source, DWORD access, const VOID* as_name, HandleInformation& Info)
{
	// Just a simple check for "string" validity
	if (as_name && (((DWORD_PTR)as_name) & ~0xFFFF))
	{
		if (source == hs_CreateFileW)
		{
			if (access & (GENERIC_READ|GENERIC_WRITE))
			{
				Info.is_con = (lstrcmpiW((LPCWSTR)as_name, L"CON") == 0);
				Info.is_input = (lstrcmpiW((LPCWSTR)as_name, L"CONIN$") == 0);
				Info.is_output = (lstrcmpiW((LPCWSTR)as_name, L"CONOUT$") == 0);
			}

			#ifdef _DEBUG
			Info.file_name_w = lstrdup((LPCWSTR)as_name); // debugging and informational purposes only
			#endif

			return (Info.is_con || Info.is_input || Info.is_output);
		}
		else if (source == hs_CreateFileA)
		{
			if (access & (GENERIC_READ|GENERIC_WRITE))
			{
				Info.is_con = (lstrcmpiA((LPCSTR)as_name, "CON") == 0);
				Info.is_input = (lstrcmpiA((LPCSTR)as_name, "CONIN$") == 0);
				Info.is_output = (lstrcmpiA((LPCSTR)as_name, "CONOUT$") == 0);
			}

			#ifdef _DEBUG
			Info.file_name_a = lstrdup((LPCSTR)as_name); // debugging and informational purposes only
			#endif

			return (Info.is_con || Info.is_input || Info.is_output);
		}
	}

	return false;
}

bool HandleKeeper::PreCreateHandle(HandleSource source, DWORD& dwDesiredAccess, DWORD& dwShareMode, const void** as_name)
{
	if ((source != hs_CreateFileA) && (source != hs_CreateFileW))
	{
		_ASSERTE((source == hs_CreateFileA) || (source == hs_CreateFileW));
		return false;
	}
	if (!as_name || !*as_name)
	{
		_ASSERTE(as_name && *as_name);
		return false;
	}

	HandleInformation Info = {};

	// Check if as_name is "CON", "CONIN$" or "CONOUT$"
	if (CheckConName(source, dwDesiredAccess, *as_name, Info))
	{
		// If handle is opened with limited right, console API would be very limited
		// and we even can't query current console window properties (cursor pos, buffer size, etc.)
		// This hack may be removed if we rewrite all console API functions completely
		if (Info.is_output
			|| (Info.is_con && (dwDesiredAccess & GENERIC_WRITE))
			)
		{
			if ((dwDesiredAccess & (GENERIC_READ|GENERIC_WRITE)) != (GENERIC_READ|GENERIC_WRITE))
				dwDesiredAccess |= (GENERIC_READ|GENERIC_WRITE);

			if ((dwShareMode & (FILE_SHARE_READ|FILE_SHARE_WRITE)) != (FILE_SHARE_READ|FILE_SHARE_WRITE))
				dwShareMode |= (FILE_SHARE_READ|FILE_SHARE_WRITE);

			// We have to change opening name to 'CONOUT$' or CreateFile will fail
			if (Info.is_con)
			{
				static char szConOut[] = "CONOUT$";
				static wchar_t wszConOut[] = L"CONOUT$";
				if (source == hs_CreateFileA)
					*as_name = szConOut;
				else if (source == hs_CreateFileW)
					*as_name = wszConOut;
			}
		}
	}

	#ifdef _DEBUG
	SafeFree(Info.file_name_ptr);
	#endif

	return false;
}

bool HandleKeeper::AllocHandleInfo(HANDLE h, HandleSource source, DWORD access, const void* as_name, HandleInformation* pInfo /*= NULL*/)
{
	if (!h || (h == INVALID_HANDLE_VALUE))
		return false;

	if (!InitHandleStorage())
		return false;

	HandleInformation Info = {h};
	Info.source = source;

	// Check if as_name is "CON", "CONIN$" or "CONOUT$"
	if ((((source == hs_CreateFileA) || (source == hs_CreateFileW))
			&& CheckConName(source, access, as_name, Info))
		|| (source <= hs_CreateConsoleScreenBuffer)
		)
	{
		// Check if handle has console capabilities
		FillHandleInfo(Info, access);
	}

	if (pInfo)
		*pInfo = Info;

	gp_Handles->Set(h, Info);

	return true;
}

bool HandleKeeper::QueryHandleInfo(HANDLE h, HandleInformation& Info)
{
	if (!h || (h == INVALID_HANDLE_VALUE))
		return false;
	bool bFound = false;
	if (gp_Handles)
		bFound = gp_Handles->Get(h, &Info);
	if (!bFound)
		bFound = AllocHandleInfo(h, hs_Unknown, 0, NULL, &Info);
	return bFound;
}

void HandleKeeper::CloseHandleInfo(HANDLE h)
{
	HandleInformation Info = {};
	if (gp_Handles)
	{
		if (gp_Handles->Get(h, &Info, true))
		{
			#ifdef _DEBUG
			if (Info.file_name_ptr)
				free(Info.file_name_ptr)
			#endif
				;
		}
	}
}

bool HandleKeeper::IsAnsiCapable(HANDLE hFile, bool *pbIsConOut)
{
	HandleInformation Info = {};
	if (!QueryHandleInfo(hFile, Info))
		return false;
	if (pbIsConOut)
		*pbIsConOut = (Info.is_output || Info.is_error);
	return (Info.is_ansi);
}

bool HandleKeeper::IsOutputHandle(HANDLE hFile)
{
	HandleInformation Info = {};
	if (!QueryHandleInfo(hFile, Info))
		return false;
	return (Info.is_output || Info.is_error);
}

bool HandleKeeper::IsInputHandle(HANDLE hFile)
{
	HandleInformation Info = {};
	if (!QueryHandleInfo(hFile, Info))
		return false;
	return (Info.is_input);
}
