
#pragma once

extern UINT_PTR gfnLoadLibrary;

int InjectHooks(PROCESS_INFORMATION pi, BOOL abForceGui, BOOL abLogProcess);
UINT_PTR GetLoadLibraryAddress();
