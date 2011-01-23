
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

#ifdef _DEBUG
//  –аскомментировать, чтобы сразу после загрузки модул€ показать MessageBox, чтобы прицепитьс€ дебаггером
//  #define SHOW_STARTED_MSGBOX
#endif

#include <windows.h>
#include "../common/common.hpp"
#include "../common/ConEmuCheck.h"


#if defined(__GNUC__)
extern "C"{
  BOOL WINAPI DllMain( HANDLE hModule, DWORD  ul_reason_for_call, LPVOID lpReserved );
  LRESULT CALLBACK LLKeybHook(int nCode,WPARAM wParam,LPARAM lParam);
#endif
  __declspec(dllexport) HHOOK ghKeyHook = 0;
  __declspec(dllexport) DWORD gnVkWinFix = 0xF0;
  __declspec(dllexport) HWND  ghConEmuRoot = NULL;
#if defined(__GNUC__)
};
#endif

HMODULE ghOurModule = NULL; // ConEmu.dll - сам плагин
UINT gnMsgActivateCon = 0; //RegisterWindowMessage(CONEMUMSG_LLKEYHOOK);
SECURITY_ATTRIBUTES* gpLocalSecurity = NULL;

#define isPressed(inp) ((GetKeyState(inp) & 0x8000) == 0x8000)



BOOL WINAPI DllMain( HANDLE hModule, DWORD  ul_reason_for_call, LPVOID lpReserved )
{
	switch (ul_reason_for_call)
	{
		case DLL_PROCESS_ATTACH:
			{
				ghOurModule = (HMODULE)hModule;
				
				#ifdef SHOW_STARTED_MSGBOX
				if (!IsDebuggerPresent()) MessageBoxA(NULL, "ConEmuHk*.dll loaded", "ConEmu hooks", 0);
				#endif
				
				gpLocalSecurity = LocalSecurity();

				gnMsgActivateCon = RegisterWindowMessage(CONEMUMSG_ACTIVATECON);

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

/* »спользуютс€ как extern в ConEmuCheck.cpp */
LPVOID _calloc(size_t nCount,size_t nSize) {
	return calloc(nCount,nSize);
}
LPVOID _malloc(size_t nCount) {
	return malloc(nCount);
}
void   _free(LPVOID ptr) {
	free(ptr);
}


BYTE gnOtherWin = 0;
DWORD gnSkipVkModCode = 0;
//DWORD gnSkipScanModCode = 0;
DWORD gnSkipVkKeyCode = 0;
//DWORD gnWinPressTick = 0;
//int gnMouseTouch = 0;

LRESULT CALLBACK LLKeybHook(int nCode,WPARAM wParam,LPARAM lParam)
{
	if (nCode >= 0)
	{
		KBDLLHOOKSTRUCT *pKB = (KBDLLHOOKSTRUCT*)lParam;
#ifdef _DEBUG
		wchar_t szKH[128];
		DWORD dwTick = GetTickCount();
		wsprintf(szKH, L"[hook] %s(vk=%i, flags=0x%08X, time=%i, tick=%i, delta=%i)\n",
			(wParam==WM_KEYDOWN) ? L"WM_KEYDOWN" :
			(wParam==WM_KEYUP) ? L"WM_KEYUP" :
			(wParam==WM_SYSKEYDOWN) ? L"WM_SYSKEYDOWN" :
			(wParam==WM_SYSKEYUP) ? L"WM_SYSKEYUP" : L"UnknownMessage",
			pKB->vkCode, pKB->flags, pKB->time, dwTick, (dwTick-pKB->time));
		//if (wParam == WM_KEYUP && gnSkipVkModCode && pKB->vkCode == gnSkipVkModCode) {
		//	wsprintf(szKH+lstrlen(szKH)-1, L" - WinDelta=%i\n", (pKB->time - gnWinPressTick));
		//}
		OutputDebugString(szKH);
#endif

		if (wParam == WM_KEYDOWN && ghConEmuRoot)
		{
			if ((pKB->vkCode >= (UINT)'0' && pKB->vkCode <= (UINT)'9') /*|| pKB->vkCode == (int)' '*/)
			{
				BOOL lbLeftWin = isPressed(VK_LWIN);
				BOOL lbRightWin = isPressed(VK_RWIN);
				if ((lbLeftWin || lbRightWin) && IsWindow(ghConEmuRoot))
				{
					DWORD nConNumber = (pKB->vkCode == (UINT)'0') ? 10 : (pKB->vkCode - (UINT)'0');

					PostMessage(ghConEmuRoot, gnMsgActivateCon, nConNumber, 0);

					gnSkipVkModCode = lbLeftWin ? VK_LWIN : VK_RWIN;
					gnSkipVkKeyCode = pKB->vkCode;

					// запрет обработки системой
					return 1; // Ќужно возвращать 1, чтобы нажатие не ушло в Win7 Taskbar


					////gnWinPressTick = pKB->time;

					//HWND hConEmu = GetForegroundWindow();
					//// ѕо идее, должен быть ConEmu, но необходимо проверить (может хук не сн€лс€?)
					//if (hConEmu)
					//{
					//	wchar_t szClass[64];
					//	if (GetClassNameW(hConEmu, szClass, 63) && lstrcmpW(szClass, VirtualConsoleClass)==0)
					//	{
					//		//if (!gnMsgActivateCon) --> DllMain
					//		//	gnMsgActivateCon = RegisterWindowMessage(CONEMUMSG_LLKEYHOOK);

					//		WORD nConNumber = (pKB->vkCode == (UINT)'0') ? 10 : (pKB->vkCode - (UINT)'0');

					//		if (SendMessage(hConEmu, gnMsgActivateCon, wParam, pKB->vkCode) == 1)
					//		{
					//			gnSkipVkModCode = lbLeftWin ? VK_LWIN : VK_RWIN;
					//			gnSkipVkKeyCode = pKB->vkCode;

					//			// запрет обработки системой
					//			return 1; // Ќужно возвращать 1, чтобы нажатие не ушло в Win7 Taskbar
					//		}
					//	}
					//}
				}
			}

			// на первое нажатие не приходит - только при удержании
			//if (pKB->vkCode == VK_LWIN || pKB->vkCode == VK_RWIN) {
			//	gnWinPressTick = pKB->time;
			//}

			if (gnSkipVkKeyCode && !gnOtherWin)
			{
				// —траховка от залипаний
				gnSkipVkModCode = 0;
				gnSkipVkKeyCode = 0;
			}

		}
		else if (wParam == WM_KEYUP)
		{

			if (gnSkipVkModCode && pKB->vkCode == gnSkipVkModCode)
			{
				if (gnSkipVkKeyCode)
				{
					#ifdef _DEBUG
						OutputDebugString(L"*** Win released before key ***\n");
					#endif
					// ѕри быстром нажатии Win+<кнопка> часто получаетс€ что сам Win отпускаетс€ раньше <кнопки>.
					gnOtherWin = (BYTE)gnVkWinFix;
					keybd_event(gnOtherWin, gnOtherWin, 0, 0);
				}
				else
				{
					gnOtherWin = 0;
				}
				gnSkipVkModCode = 0;
				return 0; // разрешить обработку системой, но не передавать в другие хуки
			}
			if (gnSkipVkKeyCode && pKB->vkCode == gnSkipVkKeyCode)
			{
				gnSkipVkKeyCode = 0;
				if (gnOtherWin)
				{
					keybd_event(gnOtherWin, gnOtherWin, KEYEVENTF_KEYUP, 0);
					gnOtherWin = 0;
				}
				return 0; // разрешить обработку системой, но не передавать в другие хуки
			}
		}
	}
	return CallNextHookEx(ghKeyHook, nCode, wParam, lParam);
}
