
/*
Copyright (c) 2009-present Maximus5
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

#define HIDE_USE_EXCEPTION_INFO

#include "Header.h"
#include "ConEmu.h"
#include "GlobalHotkeys.h"
#include "HooksUnlocker.h"
#include "LngRc.h"
#include "Options.h"
#include "OptionsClass.h"
#include "TrayIcon.h"
#include "VConRelease.h"
#include "VirtualConsole.h"
#include "../common/MFileLogEx.h"

#define HOTKEY_CTRLWINALTSPACE_ID 0x0201 // this is wParam for WM_HOTKEY
#define HOTKEY_SETFOCUSSWITCH_ID  0x0202 // this is wParam for WM_HOTKEY
#define HOTKEY_SETFOCUSGUI_ID     0x0203 // this is wParam for WM_HOTKEY
#define HOTKEY_SETFOCUSCHILD_ID   0x0204 // this is wParam for WM_HOTKEY
#define HOTKEY_CHILDSYSMENU_ID    0x0205 // this is wParam for WM_HOTKEY
#define HOTKEY_GLOBAL_START       0x1001 // this is wParam for WM_HOTKEY

static GlobalHotkeys::RegisteredHotKeys gRegisteredHotKeys[] = {
	{vkMinimizeRestore},
	{vkMinimizeRestor2},
	{vkGlobalRestore},
	{vkForceFullScreen},
	{vkCdExplorerPath},
};

static GlobalHotkeys::RegisteredHotKeys gActiveOnlyHotKeys[] = {
	{0, HOTKEY_CTRLWINALTSPACE_ID, VK_SPACE, MOD_CONTROL|MOD_WIN|MOD_ALT},
	{vkSetFocusSwitch, HOTKEY_SETFOCUSSWITCH_ID},
	{vkSetFocusGui,    HOTKEY_SETFOCUSGUI_ID},
	{vkSetFocusChild,  HOTKEY_SETFOCUSCHILD_ID},
	{vkChildSystemMenu,HOTKEY_CHILDSYSMENU_ID},
};

void GlobalHotkeys::RegisterHotKeys()
{
	if (gpConEmu->isIconic())
	{
		UnRegisterHotKeys();
		return;
	}

	RegisterGlobalHotKeys(true);

	if (!mh_LLKeyHook)
	{
		RegisterHooks();
	}
}

HMODULE GlobalHotkeys::LoadConEmuCD()
{
	_ASSERTE(gpConEmu->GetStartupStage() < CConEmuMain::ss_Destroying);

	if (!mh_LLKeyHookDll)
	{
		wchar_t szConEmuDll[MAX_PATH+32];
		wcscpy_c(szConEmuDll, gpConEmu->ms_ConEmuBaseDir);
		wcscat_c(szConEmuDll, L"\\" ConEmuCD_DLL_3264);
		mh_LLKeyHookDll = LoadLibrary(szConEmuDll);

		GetProcAddress(mph_HookedGhostWnd, mh_LLKeyHookDll, "ghActiveGhost");
	}

	if (!mh_LLKeyHookDll)
		mph_HookedGhostWnd = nullptr;

	return mh_LLKeyHookDll;
}

void GlobalHotkeys::UpdateWinHookSettings() const
{
	if (mh_LLKeyHookDll)
	{
		gpSetCls->UpdateWinHookSettings(mh_LLKeyHookDll);

		CVConGuard VCon;
		if (gpConEmu->GetActiveVCon(&VCon) >= 0)
		{
			UpdateActiveGhost(VCon.VCon());
		}
	}
}

bool GlobalHotkeys::IsKeyboardHookRegistered() const
{
	return (mh_LLKeyHook != nullptr);
}

bool GlobalHotkeys::IsActiveHotkeyRegistered() const
{
	return mb_HotKeyRegistered;
}

// Keyboard hooks are set up globally, but only during ConEmu has focus
// We use hooks to be able use Win+Key hotkeys and some other specials
// Hooks are disabled when ConEmu loses focus to avoid injection ConEmuCD.dll into other processes
void GlobalHotkeys::RegisterHooks()
{
	// If we are in termination state - nothing to do!
	if (gpConEmu->GetStartupStage() >= CConEmuMain::ss_Destroying)
	{
		return;
	}

	// Must be executed in main thread only
	if (!isMainThread())
	{
		struct Impl
		{
			static LRESULT CallRegisterHooks(LPARAM lParam)
			{
				_ASSERTE(gpConEmu == reinterpret_cast<CConEmuMain*>(lParam));
				gpConEmu->GetGlobalHotkeys().RegisterHooks();
				return 0;
			}
		};
		gpConEmu->CallMainThread(false, Impl::CallRegisterHooks, reinterpret_cast<LPARAM>(this));
		return;
	}

	DWORD dwErr = 0;
	auto pLogger = gpConEmu->GetLogger();

	if (!mh_LLKeyHook)
	{
		// Check if user allows set up global hooks
		if (!gpSet->isKeyboardHooks())
		{
			if (gpSet->isLogging())
			{
				if (pLogger) pLogger->LogString("GlobalHotkeys::RegisterHooks() skipped, cause of !isKeyboardHooks()", TRUE);
			}
		}
		else
		{
			if (!mh_LLKeyHookDll)
			{
				LoadConEmuCD();
			}

			_ASSERTE(mh_LLKeyHook==nullptr); // was registered in different thread?

			if (!mh_LLKeyHook && mh_LLKeyHookDll)
			{
				auto pfnLLHK = reinterpret_cast<HOOKPROC>(GetProcAddress(mh_LLKeyHookDll, "LLKeybHook"));
				auto* pKeyHook = reinterpret_cast<HHOOK*>(GetProcAddress(mh_LLKeyHookDll, "ghKeyHook"));
				auto* pConEmuRoot = reinterpret_cast<HWND*>(GetProcAddress(mh_LLKeyHookDll, "ghKeyHookConEmuRoot"));

				if (pConEmuRoot)
					*pConEmuRoot = ghWnd;

				UpdateWinHookSettings();

				if (pfnLLHK)
				{
					// WH_KEYBOARD_LL - could be only global
					mh_LLKeyHook = SetWindowsHookEx(WH_KEYBOARD_LL, pfnLLHK, mh_LLKeyHookDll, 0);

					if (!mh_LLKeyHook)
					{
						dwErr = GetLastError();
						if (gpSet->isLogging())
						{
							char szErr[128];
							sprintf_c(szErr, "GlobalHotkeys::RegisterHooks() failed, Code=%u", dwErr);
							if (pLogger) pLogger->LogString(szErr, TRUE);
						}
						_ASSERTE(mh_LLKeyHook!=NULL);
					}
					else
					{
						if (pKeyHook) *pKeyHook = mh_LLKeyHook;
						if (gpSet->isLogging())
						{
							if (pLogger) pLogger->LogString("GlobalHotkeys::RegisterHooks() succeeded", TRUE);
						}
					}
				}
			}
			else
			{
				if (gpSet->isLogging())
				{
					if (pLogger) pLogger->LogString("GlobalHotkeys::RegisterHooks() failed, cause of !mh_LLKeyHookDll", TRUE);
				}
			}
		}
	}
	else
	{
		if (gpSet->isLogging(2))
		{
			if (pLogger) pLogger->LogString("GlobalHotkeys::RegisterHooks() skipped, already was set", TRUE);
		}
	}
}

void GlobalHotkeys::UnRegisterHotKeys(bool abFinal/*=FALSE*/)
{
	RegisterGlobalHotKeys(false);

	UnRegisterHooks(abFinal);
}

void GlobalHotkeys::UnRegisterHooks(bool abFinal/*=false*/)
{
	if (mh_LLKeyHook)
	{
		UnhookWindowsHookEx(mh_LLKeyHook);
		mh_LLKeyHook = nullptr;

		if (gpSet->isLogging())
		{
			auto pLogger = gpConEmu->GetLogger();
			if (pLogger) pLogger->LogString("GlobalHotkeys::UnRegisterHooks() done", TRUE);
		}
	}

	if (abFinal)
	{
		if (mh_LLKeyHookDll)
		{
			FreeLibrary(mh_LLKeyHookDll);
			mh_LLKeyHookDll = nullptr;
		}
	}
}

// Process WM_HOTKEY message
// Called from ProcessMessage to obtain nTime
// (nTime is GetTickCount() where msg was generated)
void GlobalHotkeys::OnWmHotkey(WPARAM wParam, DWORD nTime /*= 0*/) const
{
	wchar_t dbgMsg[80];
	swprintf_s(dbgMsg, L"OnWmHotkey(ID=0x%04X, time=%u)", LODWORD(wParam), nTime);
	auto pLogger = gpConEmu->GetLogger();
	if (pLogger) pLogger->LogString(dbgMsg);
	
	switch (LODWORD(wParam))
	{

	// Ctrl+Win+Alt+Space
	case HOTKEY_CTRLWINALTSPACE_ID:
	{
		gpConEmu->CtrlWinAltSpace();
		break;
	}

	// Win+Esc by default
	case HOTKEY_SETFOCUSSWITCH_ID:
	case HOTKEY_SETFOCUSGUI_ID:
	case HOTKEY_SETFOCUSCHILD_ID:
	{
		SwitchGuiFocusOp FocusOp =
			(wParam == HOTKEY_SETFOCUSSWITCH_ID) ? sgf_FocusSwitch :
			(wParam == HOTKEY_SETFOCUSGUI_ID) ? sgf_FocusGui :
			(wParam == HOTKEY_SETFOCUSCHILD_ID) ? sgf_FocusChild : sgf_None;
		gpConEmu->OnSwitchGuiFocus(FocusOp);
		break;
	}

	case HOTKEY_CHILDSYSMENU_ID:
	{
		CVConGuard VCon;
		if (gpConEmu->GetActiveVCon(&VCon) >= 0)
		{
			VCon->RCon()->ChildSystemMenu();
		}
		break;
	}

	default:
	{
		for (auto& gRegisteredHotKey : gRegisteredHotKeys)
		{
			if (gRegisteredHotKey.RegisteredID && (wParam == static_cast<WPARAM>(gRegisteredHotKey.RegisteredID)))
			{
				switch (gRegisteredHotKey.DescrID)
				{
				case vkMinimizeRestore:
				case vkMinimizeRestor2:
				case vkGlobalRestore:
					gpConEmu->ProcessMinRestoreHotkey(gRegisteredHotKey.DescrID, nTime);
					break;
				case vkForceFullScreen:
					gpConEmu->DoForcedFullScreen(true);
					break;
				case vkCdExplorerPath:
					{
						CVConGuard VCon;
						if (gpConEmu->GetActiveVCon(&VCon) >= 0)
						{
							VCon->RCon()->PasteExplorerPath(true, true);
						}
					}
					break;
				default:
					if (pLogger)
					{
						swprintf_s(dbgMsg, L"OnWmHotkey: Unsupported DescrID=%i", gRegisteredHotKey.DescrID);
						pLogger->LogString(dbgMsg);
					}
				}
				break;
			}
		}
	} // default:

	} //switch (LODWORD(wParam))
}

// When hotkey changed in "Keys & Macro" page
void GlobalHotkeys::GlobalHotKeyChanged()
{
	if (gpConEmu->isIconic())
	{
		return;
	}

	RegisterGlobalHotKeys(false);
	RegisterGlobalHotKeys(true);
}

// Register/unregister focus-time hotkeys
void GlobalHotkeys::RegisterGlobalHotKeys(bool bRegister)
{
	if (bRegister == mb_HotKeyRegistered)
		return; // already

	auto pLogger = gpConEmu->GetLogger();

	if (bRegister)
	{
		for (auto& activeOnlyHotKey : gActiveOnlyHotKeys)
		{
			DWORD vk, mod;

			// e.g. HOTKEY_CTRLWINALTSPACE_ID, HOTKEY_CHILDSYSMENU_ID
			const DWORD globalId = activeOnlyHotKey.RegisteredID;

			wchar_t szKey[128] = L"";
			if (activeOnlyHotKey.DescrID == 0)
			{
				// VK_SPACE
				vk = activeOnlyHotKey.VK;
				// MOD_CONTROL|MOD_WIN|MOD_ALT
				mod = activeOnlyHotKey.MOD;
				wcscpy_s(szKey, L"Ctrl+Win+Alt+Space");
			}
			else
			{
				const ConEmuHotKey* pHk = nullptr;
				const DWORD VkMod = gpSet->GetHotkeyById(activeOnlyHotKey.DescrID, &pHk);
				vk = ConEmuChord::GetHotkey(VkMod);
				if (!vk)
					continue;  // was not requested
				mod = ConEmuChord::GetHotKeyMod(VkMod);
				pHk->GetHotkeyName(szKey);
			}

			if (activeOnlyHotKey.IsRegistered
					&& ((activeOnlyHotKey.VK != vk) || (activeOnlyHotKey.MOD != mod)))
			{
				// hotkey combination was changed, unregister first
				UnregisterHotkey(activeOnlyHotKey);
			}

			bool bRegRc = false;
			DWORD dwErr = 0;
			if (!activeOnlyHotKey.RegisteredID || !activeOnlyHotKey.IsRegistered)
			{
				bRegRc = RegisterHotkey(activeOnlyHotKey, vk, mod, globalId, szKey);
				dwErr = bRegRc ? 0 : GetLastError();
			}

			if (bRegRc)
			{
				mb_HotKeyRegistered = true;
			}
		}
	}
	else
	{
		for (auto& activeOnlyHotKey : gActiveOnlyHotKeys)
		{
			// e.g. HOTKEY_CTRLWINALTSPACE_ID, HOTKEY_CHILDSYSMENU_ID
			UnregisterHotkey(activeOnlyHotKey);
			// don't reset activeOnlyHotKey.RegisteredID, they are predefined
		}

		mb_HotKeyRegistered = false;
	}
}

// Register/unregister life-time global hotkeys
void GlobalHotkeys::RegisterMinRestore(bool abRegister) const
{
	wchar_t szErr[512];
	auto pLogger = gpConEmu->GetLogger();

	// Global hotkeys (like minimize/restore) has no sense in Inside mode
	if (abRegister && !gpConEmu->mp_Inside)
	{
		DWORD nextGlobalId = HOTKEY_GLOBAL_START;
		for (auto& registeredHotKey : gRegisteredHotKeys)
		{
			const auto globalId = nextGlobalId++;
			const ConEmuHotKey* pHk = nullptr;
			const DWORD VkMod = gpSet->GetHotkeyById(registeredHotKey.DescrID, &pHk);
			const UINT vk = ConEmuChord::GetHotkey(VkMod);
			if (!vk)
				continue;  // was not requested
			const UINT nMOD = ConEmuChord::GetHotKeyMod(VkMod);

			if (registeredHotKey.IsRegistered
					&& ((registeredHotKey.VK != vk) || (registeredHotKey.MOD != nMOD)))
			{
				// hotkey combination was changed, unregister first
				UnregisterHotkey(registeredHotKey);
				registeredHotKey.RegisteredID = 0;
			}

			if (!registeredHotKey.RegisteredID || !registeredHotKey.IsRegistered)
			{
				wchar_t szKey[128];
				pHk->GetHotkeyName(szKey);

				if (!RegisterHotkey(registeredHotKey, vk, nMOD, globalId, szKey))
				{
					const auto dwErr = GetLastError();
					// try not to show errors if several ConEmu instances are started simultaneously
					// unfortunately isFirstInstance doesn't work if other ConEmu instance is started under different account
					if (gpConEmu->isFirstInstance(true))
					{
						wchar_t szName[128];

						if (!CLngRc::getHint(registeredHotKey.DescrID, szName, countof(szName)))
							swprintf_c(szName, L"DescrID=%i", registeredHotKey.DescrID);

						swprintf_c(szErr,
							L"Can't register hotkey for\n%s\n%s"
							L"%s, ErrCode=%u",
							szName,
							(dwErr == ERROR_HOTKEY_ALREADY_REGISTERED) ? L"Hotkey already registered by another App\n" : L"",
							szKey, dwErr);
						Icon.ShowTrayIcon(szErr, tsa_Config_Error);
					}
				}
			}
		}
	}
	else
	{
		for (auto& registeredHotKey : gRegisteredHotKeys)
		{
			UnregisterHotkey(registeredHotKey);
			registeredHotKey.RegisteredID = 0;
		}
	}
}

bool GlobalHotkeys::RegisterHotkey(RegisteredHotKeys& hotkey, DWORD vk, DWORD mod, DWORD globalId, const wchar_t* description) const
{
	_ASSERTE(globalId != 0);
	_ASSERTE(hotkey.RegisteredID == 0 || hotkey.RegisteredID == static_cast<int>(globalId));
	
	wchar_t szStatus[64] = L"";
	DWORD dwErr = 0;

	if (hotkey.IsRegistered)
	{
		wcscpy_c(szStatus, L"SKIPPED");
	}
	else
	{
		hotkey.IsRegistered = RegisterHotKey(ghWnd, globalId, mod, vk);
		if (hotkey.IsRegistered)
		{
			wcscpy_c(szStatus, L"OK");
		}
		else
		{
			dwErr = GetLastError();
			swprintf_s(szStatus, L"FAILED, Code=%u%s",
				dwErr, (dwErr == ERROR_HOTKEY_ALREADY_REGISTERED) ? L"already registered by another app" : L"");
		}
	}

	if (gpSet->isLogging())
	{
		wchar_t szErr[256];
		swprintf_s(szErr, L"RegisterHotKey(ID=0x%04X, %s, VK=%u, MOD=x%X) - %s",
			globalId, description ? description : L"unknown-key", vk, mod, szStatus);
		auto pLogger = gpConEmu->GetLogger();
		if (pLogger) pLogger->LogString(szErr, TRUE);
	}

	if (hotkey.IsRegistered)
	{
		hotkey.RegisteredID = globalId;
		hotkey.VK = vk;
		hotkey.MOD = mod;
	}

	SetLastError(dwErr);
	return hotkey.IsRegistered;
}

void GlobalHotkeys::UnregisterHotkey(RegisteredHotKeys& hotkey) const
{
	if (!hotkey.RegisteredID || !hotkey.IsRegistered)
		return;

	UnregisterHotKey(ghWnd, hotkey.RegisteredID);
	hotkey.IsRegistered = false;

	if (gpSet->isLogging())
	{
		wchar_t szErr[80];
		auto pLogger = gpConEmu->GetLogger();
		swprintf_c(szErr, L"UnregisterHotKey(ID=0x%04X)", hotkey.RegisteredID);
		if (pLogger) pLogger->LogString(szErr);
	}
}

void GlobalHotkeys::UpdateActiveGhost(CVirtualConsole* apVCon) const
{
	_ASSERTE(apVCon->isVisible());
	if (mh_LLKeyHookDll && mph_HookedGhostWnd)
	{
		// Win7 и выше!
		if (IsWindows7 || !gpSet->isTabsOnTaskBar())
			*mph_HookedGhostWnd = nullptr; //ghWndApp ? ghWndApp : ghWnd;
		else
			*mph_HookedGhostWnd = apVCon->GhostWnd();
	}
}

void GlobalHotkeys::OnTerminate() const
{
	_ASSERTE(mh_LLKeyHookDll==NULL);
}
