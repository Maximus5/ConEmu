
#include <windows.h>

static void ENotImpl()
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
}

extern "C" void* WINAPI FixEncodePointer(void* ptr)
{
	return ptr;
}

extern "C" void* WINAPI FixDecodePointer(void* ptr)
{
	return ptr;
}

extern "C" BOOL WINAPI FixGetModuleHandleExW(DWORD dwFlags, LPCWSTR lpModuleName, HMODULE *phModule)
{
	if (dwFlags)
	{
		ENotImpl();
		*phModule = NULL;
	}
	else
	{
		*phModule = GetModuleHandleW(lpModuleName);
	}
	return (*phModule != NULL);
}

// Kernel32.dll
extern "C" void WINAPI FixInitializeSListHead(PSLIST_HEADER ListHead)
{
	ENotImpl();
}

extern "C" PSLIST_ENTRY WINAPI FixInterlockedFlushSList(PSLIST_HEADER ListHead)
{
	ENotImpl();
	return NULL;
}

extern "C" PSLIST_ENTRY WINAPI FixInterlockedPushEntrySList(PSLIST_HEADER ListHead, PSLIST_ENTRY ListEntry)
{
	ENotImpl();
	return NULL;
}


// Advapi32.dll
// alias for FixRtlGenRandom
extern "C" BOOLEAN WINAPI SystemFunction036(PVOID RandomBuffer, ULONG RandomBufferLength)
{
	ENotImpl();
	return FALSE;
}
