
#include <windows.h>


HMODULE ConEmuDll=NULL;
typedef BOOL (WINAPI *FIsConsoleActive)();
FIsConsoleActive fIsConsoleActive = NULL;


BOOL APIENTRY DllMain( HANDLE hModule, DWORD  ul_reason_for_call, LPVOID lpReserved )
{
	switch (ul_reason_for_call) {
		case DLL_PROCESS_ATTACH:
			break;
	}
	return TRUE;
}

#if defined(__GNUC__)
#ifdef __cplusplus
extern "C"{
#endif
  BOOL WINAPI DllMainCRTStartup(HANDLE hDll,DWORD dwReason,LPVOID lpReserved);
  BOOL WINAPI IsConsoleActive();
#ifdef __cplusplus
};
#endif

BOOL WINAPI DllMainCRTStartup(HANDLE hDll,DWORD dwReason,LPVOID lpReserved)
{
  /*(void) lpReserved;
  (void) dwReason;
  (void) hDll;*/
  return DllMain(hDll, dwReason,lpReserved);
}
#endif

BOOL WINAPI IsConsoleActive()
{
	BOOL lbActive = TRUE;
	
	if (ConEmuDll == NULL) {
		ConEmuDll = GetModuleHandle(L"ConEmu.dll");
	}
	if (ConEmuDll && !fIsConsoleActive) {
		fIsConsoleActive = (FIsConsoleActive)GetProcAddress(ConEmuDll, "IsConsoleActive");
	}
	if (fIsConsoleActive) {
		lbActive = fIsConsoleActive();
	}
	return lbActive;
}
