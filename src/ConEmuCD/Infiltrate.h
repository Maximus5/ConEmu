
#ifndef _INFILTRATE_HEADER_
#define _INFILTRATE_HEADER_

typedef HMODULE (WINAPI* LoadLibraryW_t)(LPCWSTR);
typedef DWORD (WINAPI* GetLastError_t)();
typedef void (WINAPI* SetLastError_t)(DWORD dwErrCode);
typedef HANDLE (WINAPI* CreateRemoteThread_t)(HANDLE hProcess, LPSECURITY_ATTRIBUTES lpThreadAttributes, SIZE_T dwStackSize, LPTHREAD_START_ROUTINE lpStartAddress, LPVOID lpParameter, DWORD dwCreationFlags, LPDWORD lpThreadId);


struct InfiltrateArg
{
	GetLastError_t _GetLastError;
	SetLastError_t _SetLastError;
	LoadLibraryW_t _LoadLibraryW;
	HINSTANCE hInst;
	DWORD_PTR ErrCode;
	wchar_t   szConEmuHk[MAX_PATH];
};

size_t GetInfiltrateProc(void** ppCode);

#endif
