
#pragma once

extern UINT_PTR gfnLoadLibrary;
extern UINT_PTR gfnLdrGetDllHandleByName;

#include "../ConEmuCD/ExitCodes.h"

CINJECTHK_EXIT_CODES InjectHooks(PROCESS_INFORMATION pi, BOOL abLogProcess, LPCWSTR asConEmuHkDir = NULL);
UINT_PTR GetLoadLibraryAddress();
UINT_PTR GetLdrGetDllHandleByNameAddress();
