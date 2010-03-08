
/*
Copyright (c) 2009-2010 Maximus5
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

THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
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

#ifdef _DEBUG
//  Раскомментировать, чтобы сразу после загрузки модуля показать MessageBox, чтобы прицепиться дебаггером
//  #define SHOW_STARTED_MSGBOX
#endif

#include <windows.h>
#include "../common/common.hpp"
#include "../common/ConEmuCheck.h"


#if defined(__GNUC__)
extern "C"{
  BOOL WINAPI DllMain( HANDLE hModule, DWORD  ul_reason_for_call, LPVOID lpReserved );
  LRESULT CALLBACK LLKeybHook(int nCode,WPARAM wParam,LPARAM lParam);
  __declspec(dllexport) HHOOK KeyHook = 0;
};
#else
  __declspec(dllexport) HHOOK KeyHook = 0;
#endif

HMODULE ghOurModule = NULL; // ConEmu.dll - сам плагин
UINT gnMsgLLKeyHook = 0; //RegisterWindowMessage(CONEMUMSG_LLKEYHOOK);
SECURITY_ATTRIBUTES* gpNullSecurity = NULL;

#define isPressed(inp) ((GetKeyState(inp) & 0x8000) == 0x8000)


BOOL WINAPI DllMain( HANDLE hModule, DWORD  ul_reason_for_call, LPVOID lpReserved )
{
	switch (ul_reason_for_call) {
		case DLL_PROCESS_ATTACH:
			{
				ghOurModule = (HMODULE)hModule;
				
				#ifdef SHOW_STARTED_MSGBOX
				if (!IsDebuggerPresent()) MessageBoxA(NULL, "ConEmuHk.dll loaded", "ConEmu hooks", 0);
				#endif
				
				gpNullSecurity = NullSecurity();
			}
			break;
		case DLL_PROCESS_DETACH:
			{
				CommonShutdown();
			}
			break;
	}
	return TRUE;
}

#if defined(CRTSTARTUP)
extern "C"{
  BOOL WINAPI _DllMainCRTStartup(HANDLE hDll,DWORD dwReason,LPVOID lpReserved);
};

BOOL WINAPI _DllMainCRTStartup(HANDLE hDll,DWORD dwReason,LPVOID lpReserved)
{
  DllMain(hDll, dwReason, lpReserved);
  return TRUE;
}
#endif

DWORD gnSkipVkModCode = 0;
DWORD gnSkipVkKeyCode = 0;

LRESULT CALLBACK LLKeybHook(int nCode,WPARAM wParam,LPARAM lParam)
{
	if (nCode >= 0) {
		KBDLLHOOKSTRUCT *pKB = (KBDLLHOOKSTRUCT*)lParam;
//#ifdef _DEBUG
//		wchar_t szKH[128];
//		wsprintf(szKH, L"[hook] %s(vk=%i, flags=0x%08X)\n",
//			(wParam==WM_KEYDOWN) ? L"WM_KEYDOWN" :
//			(wParam==WM_KEYUP) ? L"WM_KEYUP" :
//			(wParam==WM_SYSKEYDOWN) ? L"WM_SYSKEYDOWN" :
//			(wParam==WM_SYSKEYUP) ? L"WM_SYSKEYUP" : L"UnknownMessage",
//			pKB->vkCode, pKB->flags);
//		OutputDebugString(szKH);
//#endif

		if (wParam == WM_KEYUP && gnSkipVkModCode && pKB->vkCode == gnSkipVkModCode) {
			gnSkipVkModCode = 0;
			return 0;
		}
		if (wParam == WM_KEYUP && gnSkipVkKeyCode && pKB->vkCode == gnSkipVkKeyCode) {
			gnSkipVkKeyCode = 0;
			return 0;
		}

		if (wParam == WM_KEYDOWN && pKB->vkCode >= (int)'0' && pKB->vkCode <= (int)'9') {
			BOOL lbLeftWin = isPressed(VK_LWIN);
			BOOL lbRightWin = isPressed(VK_RWIN);
			if (lbLeftWin || lbRightWin) {
				HWND hConEmu = GetForegroundWindow();
				// По идее, должен быть ConEmu, но необходимо проверить (может хук не снялся?)
				if (hConEmu) {
					wchar_t szClass[64];
					if (GetClassNameW(hConEmu, szClass, 63) && lstrcmpW(szClass, VirtualConsoleClass)==0)
					{
						if (!gnMsgLLKeyHook)
							gnMsgLLKeyHook = RegisterWindowMessage(CONEMUMSG_LLKEYHOOK);
						if (SendMessage(hConEmu, gnMsgLLKeyHook, wParam, pKB->vkCode) == 1) {
							gnSkipVkModCode = lbLeftWin ? VK_LWIN : VK_RWIN;
							gnSkipVkKeyCode = pKB->vkCode;
							return 1;
						}
					}
				}
			}
		}

		if (wParam == WM_KEYDOWN && gnSkipVkKeyCode) {
			// Страховка от залипаний
			gnSkipVkModCode = 0;
			gnSkipVkKeyCode = 0;
		}
	}
	return CallNextHookEx(KeyHook, nCode, wParam, lParam);
}
