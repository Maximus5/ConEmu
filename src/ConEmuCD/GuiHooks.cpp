
/*
Copyright (c) 2009-2017 Maximus5
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

#define DEBUGSTRHOOK(s) //OutputDebugString(s)

#include <windows.h>

#ifndef TESTLINK
#include "../common/Common.h"
#include "../common/ConEmuCheck.h"
#include "../common/execute.h"
#endif

#include "ConEmuC.h"
#include "GuiHooks.h"


#if defined(__GNUC__)
extern "C" {
	//BOOL WINAPI DllMain(HANDLE hModule, DWORD  ul_reason_for_call, LPVOID lpReserved);
	LRESULT CALLBACK LLKeybHook(int nCode,WPARAM wParam,LPARAM lParam);
#endif
	__declspec(dllexport) HHOOK ghKeyHook = 0;
	__declspec(dllexport) DWORD gnVkWinFix = 0xF0;
	__declspec(dllexport) BOOL  gbWinTabHook = FALSE;
	__declspec(dllexport) BYTE  gnConsoleKeyShortcuts = 0;
	__declspec(dllexport) DWORD gnHookedKeys[HookedKeysMaxCount] = {};
	__declspec(dllexport) HWND  ghKeyHookConEmuRoot = NULL;
	__declspec(dllexport) HWND  ghActiveGhost = NULL;
#if defined(__GNUC__)
};
#endif


//extern UINT gnMsgActivateCon;      // RegisterWindowMessage(CONEMUMSG_ACTIVATECON)
extern UINT gnMsgSwitchCon;        // RegisterWindowMessage(CONEMUMSG_SWITCHCON)
extern UINT gnMsgHookedKey;        // RegisterWindowMessage(CONEMUMSG_HOOKEDKEY)
//extern UINT gnMsgConsoleHookedKey; // RegisterWindowMessage(CONEMUMSG_CONSOLEHOOKEDKEY)

#define isPressed(inp) ((GetKeyState(inp) & 0x8000) == 0x8000)

extern HANDLE ghHeap;

//enum CONSOLE_KEY_ID
//{
//	ID_ALTTAB,
//	ID_ALTESC,
//	ID_ALTSPACE,
//	ID_ALTENTER,
//	ID_ALTPRTSC,
//	ID_PRTSC,
//	ID_CTRLESC,
//};

struct ConsoleKeysStr
{
	int  iMask;
	UINT vk;
	int  Mod;
}
ConsoleKeys[]=
{
	{1<<ID_ALTTAB,   VK_TAB, VK_MENU},
	{1<<ID_ALTESC,   VK_ESCAPE, VK_MENU},
	{1<<ID_ALTSPACE, 0, 0}, // Internally by ConEmu
	{1<<ID_ALTENTER, 0, 0}, // Internally by ConEmu
	{1<<ID_ALTPRTSC, VK_SNAPSHOT, VK_MENU},
	{1<<ID_PRTSC,    VK_SNAPSHOT, 0},
	{1<<ID_CTRLESC,  VK_ESCAPE, VK_CONTROL},
};


BYTE gnOtherWin = 0;
DWORD gnSkipVkModCode = 0;
WPARAM gnSkipVkMessage = 0;
DWORD gnSkipVkKeyCode = 0;

DWORD LoadPressedMods()
{
	DWORD lMods = 0;

	if (isPressed(VK_LCONTROL))
		lMods |= cvk_LCtrl|cvk_Ctrl;
	if (isPressed(VK_RCONTROL))
		lMods |= cvk_RCtrl|cvk_Ctrl;
	if (isPressed(VK_LMENU))
		lMods |= cvk_LAlt|cvk_Alt;
	if (isPressed(VK_RMENU))
		lMods |= cvk_RAlt|cvk_Alt;
	if (isPressed(VK_LSHIFT))
		lMods |= cvk_LShift|cvk_Shift;
	if (isPressed(VK_RSHIFT))
		lMods |= cvk_RShift|cvk_Shift;
	if (isPressed(VK_LWIN) || isPressed(VK_RWIN))
		lMods |= cvk_Win;
	if (isPressed(VK_APPS))
		lMods |= cvk_Apps;

	return lMods;
}

LRESULT CALLBACK LLKeybHook(int nCode,WPARAM wParam,LPARAM lParam)
{
	if (nCode >= 0)
	{
		KBDLLHOOKSTRUCT *pKB = (KBDLLHOOKSTRUCT*)lParam;
#ifdef _DEBUG
		wchar_t szKH[128];
		DWORD dwTick = GetTickCount();
		_wsprintf(szKH, SKIPLEN(countof(szKH)) L"[hook] %s(vk=%i, flags=0x%08X, time=%i, tick=%i, delta=%i)\n",
		          (wParam==WM_KEYDOWN) ? L"WM_KEYDOWN" :
		          (wParam==WM_KEYUP) ? L"WM_KEYUP" :
		          (wParam==WM_SYSKEYDOWN) ? L"WM_SYSKEYDOWN" :
		          (wParam==WM_SYSKEYUP) ? L"WM_SYSKEYUP" : L"UnknownMessage",
		          pKB->vkCode, pKB->flags, pKB->time, dwTick, (dwTick-pKB->time));
		//if (wParam == WM_KEYUP && gnSkipVkModCode && pKB->vkCode == gnSkipVkModCode) {
		//	wsprintf(szKH+lstrlen(szKH)-1, L" - WinDelta=%i\n", (pKB->time - gnWinPressTick));
		//}
		DEBUGSTRHOOK(szKH);
#endif

		if ((pKB->vkCode >= VK_WHEEL_FIRST) && (pKB->vkCode <= VK_WHEEL_LAST))
		{
			// Такие коды с клавиатуры приходить не должны, а то для "мышки" ничего не останется
			_ASSERTE(!((pKB->vkCode >= VK_WHEEL_FIRST) && (pKB->vkCode <= VK_WHEEL_LAST)));
		}
		else if (((wParam == WM_KEYDOWN) || (wParam == WM_SYSKEYDOWN)) && ghKeyHookConEmuRoot)
		{
			WPARAM lMods = 0; BOOL bModsLoaded = FALSE;
			BOOL lbHooked = FALSE, lbConsoleKey = FALSE;
			if (wParam == WM_KEYDOWN)
			{
				//if (pKB->vkCode >= (UINT)'0' && pKB->vkCode <= (UINT)'9') /*|| pKB->vkCode == (int)' '*/
				//	lbHooked = TRUE;
				//else
				if (gbWinTabHook && pKB->vkCode == VK_TAB)
					lbHooked = TRUE;
				else
				{
					for (size_t i = 0; i < countof(gnHookedKeys); i++)
					{
						// gnHookedKeys теперь содержит VkMod, а не просто VK
						if ((gnHookedKeys[i] & 0xFF) == pKB->vkCode)
						{
							if (!bModsLoaded)
								lMods = LoadPressedMods();

							bool bCurr, bNeed;

							bCurr = ((lMods & cvk_Win) != 0);		bNeed = ((gnHookedKeys[i] & cvk_Win) != 0);
							if (bCurr != bNeed) continue;
							bCurr = ((lMods & cvk_Apps) != 0);		bNeed = ((gnHookedKeys[i] & cvk_Apps) != 0);
							if (bCurr != bNeed) continue;
							bCurr = ((lMods & cvk_CtrlAny) != 0);	bNeed = ((gnHookedKeys[i] & cvk_CtrlAny) != 0);
							if (bCurr != bNeed) continue;
							bCurr = ((lMods & cvk_AltAny) != 0);	bNeed = ((gnHookedKeys[i] & cvk_AltAny) != 0);
							if (bCurr != bNeed) continue;
							bCurr = ((lMods & cvk_ShiftAny) != 0);	bNeed = ((gnHookedKeys[i] & cvk_ShiftAny) != 0);
							if (bCurr != bNeed) continue;

							lbHooked = TRUE;
							break;
						}
					}
				}
			}
			if (!lbHooked)
			{
				#ifdef _DEBUG
				DEBUGSTRHOOK(L"[hook] checking vk ");
				#endif
				for (size_t h = 0; h < countof(ConsoleKeys); h++)
				{
					if (ConsoleKeys[h].vk)
					{
						#ifdef _DEBUG
						_wsprintf(szKH, SKIPLEN(countof(szKH)) L"%i ", ConsoleKeys[h].vk);
						DEBUGSTRHOOK(szKH);
						#endif
						if ((ConsoleKeys[h].vk == pKB->vkCode)
							&& (gnConsoleKeyShortcuts & ConsoleKeys[h].iMask)
							&& (ConsoleKeys[h].Mod == 0 || isPressed(ConsoleKeys[h].Mod)))
						{
							lbHooked = lbConsoleKey = TRUE;
							#ifdef _DEBUG
							DEBUGSTRHOOK(L"proceed");
							#endif
							break;
						}
					}
				}
				#ifdef _DEBUG
				DEBUGSTRHOOK(L"\n");
				#endif
			}

			// Win2k & WinXP: при размножении на таскбаре кнопок под каждую консоль
			// возникает неприятный эффект при AltTab
			// lbHooked - AltTab может вообще игнорироваться по настройке в ConEmu
			if (!lbHooked && ghActiveGhost && wParam == WM_SYSKEYDOWN)
			{
				if (pKB->vkCode == VK_TAB && IsWindow(ghActiveGhost))
				{
					wchar_t szEvtName[64];
					_wsprintf(szEvtName, SKIPLEN(countof(szEvtName)) CEGHOSTSKIPACTIVATE, LODWORD(ghActiveGhost));
					HANDLE hSkipActivateEvent = OpenEvent(EVENT_MODIFY_STATE, FALSE, szEvtName);
					if (hSkipActivateEvent)
					{
						SetEvent(hSkipActivateEvent);
						CloseHandle(hSkipActivateEvent);
					}

					SetActiveWindow(ghActiveGhost);
				}
			}

			if (lbHooked)
			{
				BOOL lbLeftWin = isPressed(VK_LWIN);
				BOOL lbRightWin = isPressed(VK_RWIN);
				//BOOL lbShiftPressed = isPressed(VK_SHIFT);

				if (IsWindow(ghKeyHookConEmuRoot))
				{
					if (!bModsLoaded)
						lMods = LoadPressedMods();

					if (lbConsoleKey)
					{
						PostMessage(ghKeyHookConEmuRoot, gnMsgHookedKey, (pKB->vkCode & cvk_VK_MASK)|lMods, 0);

						gnSkipVkModCode = 0;
						gnSkipVkKeyCode = pKB->vkCode;
						gnSkipVkMessage = wParam;
						#ifdef _DEBUG
						_wsprintf(szKH, SKIPLEN(countof(szKH)) L"[hook] vk=%i (press) blocked\n", pKB->vkCode);
						DEBUGSTRHOOK(szKH);
						#endif
						return 1; // Нужно возвращать 1, чтобы нажатие не ушло в Win7 Taskbar
					}
					else if (lbLeftWin || lbRightWin)
					{
						if (pKB->vkCode == VK_TAB)
						{
							PostMessage(ghKeyHookConEmuRoot, gnMsgSwitchCon, (pKB->vkCode & cvk_VK_MASK)|lMods, 0);
						}
						//else if (pKB->vkCode >= (UINT)'0' && pKB->vkCode <= (UINT)'9')
						//{
						//	//DWORD nConNumber = (pKB->vkCode == (UINT)'0') ? 10 : (pKB->vkCode - (UINT)'0');
						//	//PostMessage(ghKeyHookConEmuRoot, gnMsgActivateCon, nConNumber, 0);
						//	PostMessage(ghKeyHookConEmuRoot, gnMsgActivateCon, pKB->vkCode, 0);
						//}
						else
						{
							PostMessage(ghKeyHookConEmuRoot, gnMsgHookedKey, (pKB->vkCode & cvk_VK_MASK)|lMods, 0);
						}
						gnSkipVkModCode = lbLeftWin ? VK_LWIN : lbRightWin ? VK_RWIN : 0;
						gnSkipVkKeyCode = pKB->vkCode;
						// запрет обработки системой
						#ifdef _DEBUG
						_wsprintf(szKH, SKIPLEN(countof(szKH)) L"[hook] vk=%i (press) blocked\n", pKB->vkCode);
						DEBUGSTRHOOK(szKH);
						#endif
						return 1; // Нужно возвращать 1, чтобы нажатие не ушло в Win7 Taskbar
						////gnWinPressTick = pKB->time;
						//HWND hConEmu = GetForegroundWindow();
						//// По идее, должен быть ConEmu, но необходимо проверить (может хук не снялся?)
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
						//			return 1; // Нужно возвращать 1, чтобы нажатие не ушло в Win7 Taskbar
						//		}
						//	}
						//}
					}
				}
			}

			// на первое нажатие не приходит - только при удержании
			//if (pKB->vkCode == VK_LWIN || pKB->vkCode == VK_RWIN) {
			//	gnWinPressTick = pKB->time;
			//}

			if (gnSkipVkKeyCode && !gnOtherWin)
			{
				// Страховка от залипаний
				gnSkipVkModCode = 0;
				gnSkipVkKeyCode = 0;
				gnSkipVkMessage = 0;
			}
		}
		else if (wParam == WM_KEYUP || (wParam == WM_SYSKEYUP && gnSkipVkMessage == WM_SYSKEYDOWN))
		{
			if (gnSkipVkModCode && pKB->vkCode == gnSkipVkModCode)
			{
				if (gnSkipVkKeyCode)
				{
					#ifdef _DEBUG
					DEBUGSTRHOOK(L"*** Win released before key ***\n");
					#endif

					// При быстром нажатии Win+<кнопка> часто получается что сам Win отпускается раньше <кнопки>.
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

				if (gnSkipVkMessage)
				{
					gnSkipVkMessage = 0;
					#ifdef _DEBUG
					_wsprintf(szKH, SKIPLEN(countof(szKH)) L"[hook] vk=%i (release) blocked\n", pKB->vkCode);
					DEBUGSTRHOOK(szKH);
					#endif
					return 1;
				}
				#ifdef _DEBUG
				_wsprintf(szKH, SKIPLEN(countof(szKH)) L"[hook] vk=%i processed\n", pKB->vkCode);
				DEBUGSTRHOOK(szKH);
				#endif
				return 0; // разрешить обработку системой, но не передавать в другие хуки
			}
		}
	}

	return CallNextHookEx(ghKeyHook, nCode, wParam, lParam);
}
