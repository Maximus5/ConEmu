
#pragma once

struct InjectsFnPtr
{
	HMODULE  module;
	UINT_PTR fnPtr;
	wchar_t  szModule[32];

	InjectsFnPtr() : module(NULL), fnPtr(0) { szModule[0] = 0; };
	InjectsFnPtr(HMODULE m, UINT_PTR f, LPCWSTR n) : module(m), fnPtr(f) { lstrcpyn(szModule, n, countof(szModule)); };
};

extern InjectsFnPtr gfLoadLibrary;
extern InjectsFnPtr gfLdrGetDllHandleByName;

#include "../ConEmuCD/ExitCodes.h"

CINJECTHK_EXIT_CODES InjectHooks(PROCESS_INFORMATION pi, BOOL abLogProcess, LPCWSTR asConEmuHkDir = NULL);
UINT_PTR GetLoadLibraryAddress();
UINT_PTR GetLdrGetDllHandleByNameAddress();
