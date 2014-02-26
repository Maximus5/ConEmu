
#pragma once

extern UINT_PTR gfnLoadLibrary;
extern UINT_PTR gfnLdrGetDllHandleByName;

int InjectHooks(PROCESS_INFORMATION pi, BOOL abLogProcess);
UINT_PTR GetLoadLibraryAddress();
UINT_PTR GetLdrGetDllHandleByNameAddress();
