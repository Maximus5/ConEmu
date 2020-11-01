
/*
Copyright (c) 2014-present Maximus5
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
#include "HotkeyChord.h"
#include "LngRc.h"
#include "Options.h"

const ConEmuChord NullChord = {};

// Used to get KeyName or exact VK_xxx from Mod
// So, Mod must contain single value but not a mask
// Ctrl == cvk_Ctrl, LCtrl == cvk_LCtrl, and so on...
CEVkMatch gvkMatchList[] = {
	{VK_LWIN,     true,  cvk_Win},
	{VK_APPS,     true,  cvk_Apps},
	{VK_CONTROL,  false, cvk_Ctrl,  cvk_LCtrl, cvk_RCtrl},
	{VK_LCONTROL, true,  cvk_LCtrl},
	{VK_RCONTROL, true,  cvk_RCtrl},
	{VK_MENU,     false, cvk_Alt,   cvk_LAlt, cvk_RAlt},
	{VK_LMENU,    true,  cvk_LAlt},
	{VK_RMENU,    true,  cvk_RAlt},
	{VK_SHIFT,    false, cvk_Shift, cvk_LShift, cvk_RShift},
	{VK_LSHIFT,   true,  cvk_LShift},
	{VK_RSHIFT,   true,  cvk_RShift},
	{0}
};

// Internal conversions between VK_xxx and cvk_xxx
// Used only by ConEmuHotKey::SetVkMod, so it must
// return masked values.
// LCtrl => cvk_Ctrl|cvk_LCtrl (Distinct==true)
// Ctrl => cvk_Ctrl (Distinct==true and Left/Right values)
bool CEVkMatch::GetMatchByVk(BYTE Vk, CEVkMatch& Match)
{
	switch (Vk)
	{
	case VK_LCONTROL: Match.Set(Vk, true,  cvk_Ctrl|cvk_LCtrl);  break;
	case VK_RCONTROL: Match.Set(Vk, true,  cvk_Ctrl|cvk_RCtrl);  break;
	case VK_LMENU:    Match.Set(Vk, true,  cvk_Alt|cvk_LAlt);   break;
	case VK_RMENU:    Match.Set(Vk, true,  cvk_Alt|cvk_RAlt);   break;
	case VK_LSHIFT:   Match.Set(Vk, true,  cvk_Shift|cvk_LShift); break;
	case VK_RSHIFT:   Match.Set(Vk, true,  cvk_Shift|cvk_RShift); break;
	case VK_RWIN:  // The RightWin processed the same way as LeftWin
	case VK_LWIN:     Match.Set(Vk, true,  cvk_Win);    break;
	case VK_APPS:     Match.Set(Vk, true,  cvk_Apps);   break;
	case VK_CONTROL:  Match.Set(Vk, false, cvk_Ctrl,  cvk_LCtrl, cvk_RCtrl);   break;
	case VK_MENU:     Match.Set(Vk, false, cvk_Alt,   cvk_LAlt, cvk_RAlt);     break;
	case VK_SHIFT:    Match.Set(Vk, false, cvk_Shift, cvk_LShift, cvk_RShift); break;
	default:
		return false;
	}
	return true;
#if 0
	for (size_t i = 0; gvkMatchList[i].Vk; i++)
	{
		if (Vk == gvkMatchList[i].Vk)
		{
			Match = gvkMatchList[i];
			return true;
		}
	}
	return false;
#endif
}

void CEVkMatch::Set(BYTE aVk, bool aDistinct, ConEmuModifiers aMod, ConEmuModifiers aLeft /*= cvk_NULL*/, ConEmuModifiers aRight /*= cvk_NULL*/)
{
	Vk = aVk; Distinct = aDistinct; Mod = aMod; Left = aLeft; Right = aRight;
}

ConEmuModifiers CEVkMatch::GetFlagsFromMod(DWORD vkMods)
{
	CEVkMatch Match;
	ConEmuModifiers NewMod = cvk_NULL;
	DWORD vkLeft = vkMods;

	for (int i = 4; i--;)
	{
		if (CEVkMatch::GetMatchByVk(LOBYTE(vkLeft), Match))
		{
			NewMod |= Match.Mod;
		}
		vkLeft = (vkLeft >> 8);
	}

	return NewMod;
}

#ifdef _DEBUG
void ConEmuChord::ValidateChordMod(ConEmuModifiers aMod)
{
	// The idea - if cvk_Ctrl is NOT set, neither cvk_LCtrl nor cvk_RCtrl must be set too
	#if 0
	bool bCtrl = ((aMod & cvk_Ctrl) != cvk_NULL);
	bool bLCtrl = ((aMod & cvk_LCtrl) != cvk_NULL);
	bool bRCtrl = ((aMod & cvk_RCtrl) != cvk_NULL);
	_ASSERTE(bCtrl ? (bCtrl|bLCtrl|bRCtrl) : !(bLCtrl|bRCtrl));
	#endif
	_ASSERTE(((aMod & cvk_Ctrl) != cvk_NULL) || ((aMod & (cvk_LCtrl|cvk_RCtrl)) == cvk_NULL));
	_ASSERTE(((aMod & cvk_Alt) != cvk_NULL) || ((aMod & (cvk_LAlt|cvk_RAlt)) == cvk_NULL));
	_ASSERTE(((aMod & cvk_Shift) != cvk_NULL) || ((aMod & (cvk_LShift|cvk_RShift)) == cvk_NULL));
}
#else
#define ValidateChordMod(aMod)
#endif


// service initialization function, creates ready VkMod
DWORD ConEmuChord::MakeHotKey(BYTE Vk, BYTE vkMod1/*=0*/, BYTE vkMod2/*=0*/, BYTE vkMod3/*=0*/)
{
	DWORD vkHotKey = Vk;
	if (!vkMod1 && !vkMod2 && !vkMod3)
	{
		vkHotKey |= CEHOTKEY_NOMOD;
	}
	else
	{
		int iShift = 8;
		if (vkMod1)
		{
			vkHotKey |= (vkMod1 << iShift);
			iShift += 8;
		}
		if (vkMod2)
		{
			vkHotKey |= (vkMod2 << iShift);
			iShift += 8;
		}
		if (vkMod3)
		{
			vkHotKey |= (vkMod3 << iShift);
			iShift += 8;
		}
	}
	return vkHotKey;
}

void ConEmuChord::SetHotKey(const ConEmuHotKeyType HkType, BYTE Vk, BYTE vkMod1/*=0*/, BYTE vkMod2/*=0*/, BYTE vkMod3/*=0*/)
{
	SetVkMod(HkType, MakeHotKey(Vk, vkMod1, vkMod2, vkMod3));
}

// rawMod==true: don't substitute CEHOTKEY_NUMHOSTKEY/CEHOTKEY_ARRHOSTKEY
void ConEmuChord::SetVkMod(const ConEmuHotKeyType HkType, DWORD VkMod, bool rawMod)
{
	// Init
	const BYTE Vk = LOBYTE(VkMod);
	ConEmuModifiers Mod = cvk_NULL;

	// Check modifiers
	const DWORD vkLeft = (VkMod & CEHOTKEY_MODMASK);

	if (!rawMod && ((HkType == chk_NumHost) || (vkLeft == CEHOTKEY_NUMHOSTKEY)))
	{
		_ASSERTE((HkType == chk_NumHost) && (vkLeft == CEHOTKEY_NUMHOSTKEY));
		Mod = cvk_NumHost;
	}
	else if (!rawMod && ((HkType == chk_ArrHost) || (vkLeft == CEHOTKEY_ARRHOSTKEY)))
	{
		_ASSERTE((HkType == chk_ArrHost) && (vkLeft == CEHOTKEY_ARRHOSTKEY));
		Mod = cvk_ArrHost;
	}
	else if (vkLeft == CEHOTKEY_NOMOD)
	{
		Mod = cvk_Naked;
	}
	else if (vkLeft)
	{
		Mod = CEVkMatch::GetFlagsFromMod(vkLeft);
	}
	else
	{
		Mod = cvk_NULL;
	}

	Set(Vk, Mod);
}

// That is what is stored in the settings
DWORD ConEmuChord::GetVkMod(const ConEmuHotKeyType HkType) const
{
	DWORD VkMod = 0;

	if (Vk)
	{
		// Let iterate
		int iPos = 8; // <<

		if (Mod & cvk_Naked)
		{
			// Nothing to add?
		}
		else if (Mod & cvk_NumHost)
		{
			VkMod = (gpSet->HostkeyNumberModifier() << 8);
		}
		else if (Mod & cvk_ArrHost)
		{
			VkMod = (gpSet->HostkeyArrowModifier() << 8);
		}
		else for (size_t i = 0; i < gvkMatchList[i].Vk; i++)
		{
			const CEVkMatch& k = gvkMatchList[i];

			if ((Mod & k.Mod) && !(Mod & (k.Left | k.Right)))
			{
				VkMod |= (k.Vk << iPos);
				iPos += 8;
				if (iPos > 24)
					break;

				if (!k.Distinct)
				{
					_ASSERTE((k.Left | k.Right) != cvk_NULL);
					i += 2;
				}
			}
		}

		if (!VkMod && (HkType != chk_Modifier))
			VkMod = CEHOTKEY_NOMOD;

		VkMod |= (DWORD)(BYTE)Vk;
	}

	return VkMod;
}

LPCWSTR ConEmuChord::GetHotkeyName(wchar_t(&szFull)[128], const DWORD aVkMod, const ConEmuHotKeyType HkType, const bool bShowNone)
{
	ConEmuChord Key{};
	Key.SetVkMod(HkType, aVkMod, true);

	wchar_t szName[32];
	szFull[0] = 0;

	switch (HkType)
	{
	case chk_Global:
	case chk_Local:
	case chk_User:
	case chk_Macro:
	case chk_Task:
	case chk_Modifier2:
	case chk_System:
	case chk_Modifier:
	case chk_NumHost:
	case chk_ArrHost:
		for (size_t i = 0; i < gvkMatchList[i].Vk; i++)
		{
			const CEVkMatch& k = gvkMatchList[i];

			if ((Key.Mod & k.Mod) && !(Key.Mod & (k.Left | k.Right)))
			{
				GetVkKeyName(k.Vk, szName, (szFull[0] == 0));
				if (szFull[0])
					wcscat_c(szFull, L"+");
				wcscat_c(szFull, szName);
			}
		}
		break;
	default:
		// Неизвестный тип!
		_ASSERTE(FALSE && "Unknown HkType");
	}

	// chk_Modifier2 is used (yet) only for "Drag window by client area"
	// This key by default is "Ctrl+Alt+LBtn"
	// And we show in the hotkeys list only "Ctrl+Alt"
	if (HkType != chk_Modifier2)
	{
		szName[0] = 0;
		GetVkKeyName(Key.Vk, szName, (szFull[0] == 0));

		if (szName[0])
		{
			if (szFull[0])
				wcscat_c(szFull, L"+");
			wcscat_c(szFull, szName);
		}
		else if (bShowNone)
		{
			wcscpy_c(szFull, gsNoHotkey);
		}
		else
		{
			szFull[0] = 0;
		}
	}

	if ((szFull[0] == 0)
		&& ((HkType == chk_Modifier) || (HkType == chk_Modifier2)))
	{
		wcscpy_c(szFull, CLngRc::getRsrc((HkType == chk_Modifier2) ? lng_KeyNoMod/*"<No-Mod>"*/ : lng_KeyNone/*"<None>"*/));
	}

	return szFull;
}

void ConEmuChord::TestHostkeyModifiers(DWORD& nHostMod)
{
	//memset(mn_HostModOk, 0, sizeof(mn_HostModOk));
	//memset(mn_HostModSkip, 0, sizeof(mn_HostModSkip));

	if (!nHostMod)
	{
		nHostMod = VK_LWIN;
	}
	else
	{
		BYTE vk, vkList[3] = {};
		int i = 0;
		DWORD nTest = nHostMod;
		while (nTest && (i < 3))
		{
			vk = (nTest & 0xFF);
			nTest = nTest >> 8;

			switch (vk)
			{
			case 0:
				break;
			case VK_LWIN: case VK_RWIN:
				if (vkList[0] != VK_LWIN && vkList[1] != VK_LWIN && vkList[2] != VK_LWIN)
					vkList[i++] = vk;
				break;
			case VK_APPS:
				if (vkList[0] != VK_APPS && vkList[1] != VK_APPS && vkList[2] != VK_APPS)
					vkList[i++] = vk;
				break;
			case VK_LCONTROL:
			case VK_RCONTROL:
			case VK_CONTROL:
				for (int k = 0; k < 3; k++)
				{
					if (vkList[k] == VK_LCONTROL || vkList[k] == VK_RCONTROL || vkList[k] == VK_CONTROL)
					{
						vkList[k] = VK_CONTROL;
						vk = 0;
						break;
					}
				}
				if (vk)
					vkList[i++] = vk;
				break;
			case VK_LMENU:
			case VK_RMENU:
			case VK_MENU:
				for (int k = 0; k < 3; k++)
				{
					if (vkList[k] == VK_LMENU || vkList[k] == VK_RMENU || vkList[k] == VK_MENU)
					{
						vkList[k] = VK_MENU;
						vk = 0;
						break;
					}
				}
				if (vk)
					vkList[i++] = vk;
				break;
			case VK_LSHIFT:
			case VK_RSHIFT:
			case VK_SHIFT:
				for (int k = 0; k < 3; k++)
				{
					if (vkList[k] == VK_LSHIFT || vkList[k] == VK_RSHIFT || vkList[k] == VK_SHIFT)
					{
						vkList[k] = VK_SHIFT;
						vk = 0;
						break;
					}
				}
				if (vk)
					vkList[i++] = vk;
				break;
			}
		}

		nHostMod = (((DWORD)vkList[0]))
			| (((DWORD)vkList[1]) << 8)
			| (((DWORD)vkList[2]) << 16);
	}
}

// набор флагов MOD_xxx для RegisterHotKey
DWORD ConEmuChord::GetHotKeyMod(DWORD VkMod)
{
	DWORD nMOD = 0;

	for (int i = 1; i <= 3; i++)
	{
		switch (GetModifier(VkMod, i))
		{
		case VK_LWIN: case VK_RWIN:
			nMOD |= MOD_WIN;
			break;
		case VK_CONTROL: case VK_LCONTROL: case VK_RCONTROL:
			nMOD |= MOD_CONTROL;
			break;
		case VK_SHIFT: case VK_LSHIFT: case VK_RSHIFT:
			nMOD |= MOD_SHIFT;
			break;
		case VK_MENU: case VK_LMENU: case VK_RMENU:
			nMOD |= MOD_ALT;
			break;
		}
	}

	return nMOD;
}

DWORD ConEmuChord::GetHotkey(DWORD VkMod)
{
	return (VkMod & 0xFF);
}

void ConEmuChord::GetVkKeyName(BYTE vk, wchar_t(&szName)[32], bool bSingle /*= true*/)
{
	szName[0] = 0;

	switch (vk)
	{
	case 0:
		break;
	case VK_LWIN:
	case VK_RWIN:
		wcscat_c(szName, L"Win"); break;
	case VK_CONTROL:
		wcscat_c(szName, L"Ctrl"); break;
	case VK_LCONTROL:
		wcscat_c(szName, L"LCtrl"); break;
	case VK_RCONTROL:
		wcscat_c(szName, L"RCtrl"); break;
	case VK_MENU:
		wcscat_c(szName, L"Alt"); break;
	case VK_LMENU:
		wcscat_c(szName, L"LAlt"); break;
	case VK_RMENU:
		wcscat_c(szName, L"RAlt"); break;
	case VK_SHIFT:
		wcscat_c(szName, L"Shift"); break;
	case VK_LSHIFT:
		wcscat_c(szName, L"LShift"); break;
	case VK_RSHIFT:
		wcscat_c(szName, L"RShift"); break;
	case VK_APPS:
		wcscat_c(szName, L"Apps"); break;
	case VK_LEFT:
		wcscat_c(szName, L"Left"); break;
	case VK_RIGHT:
		wcscat_c(szName, L"Right"); break;
	case VK_UP:
		wcscat_c(szName, L"Up"); break;
	case VK_DOWN:
		wcscat_c(szName, L"Down"); break;
	case VK_PRIOR:
		wcscat_c(szName, L"PgUp"); break;
	case VK_NEXT:
		wcscat_c(szName, L"PgDn"); break;
	case VK_SPACE:
		wcscat_c(szName, L"Space"); break;
	case VK_TAB:
		wcscat_c(szName, L"Tab"); break;
	case VK_ESCAPE:
		wcscat_c(szName, L"Esc"); break;
	case VK_INSERT:
		wcscat_c(szName, L"Insert"); break;
	case VK_DELETE:
		wcscat_c(szName, L"Delete"); break;
	case VK_HOME:
		wcscat_c(szName, L"Home"); break;
	case VK_END:
		wcscat_c(szName, L"End"); break;
	case VK_PAUSE:
		// Return ‘Ctrl+Break’ but ‘Pause’
		wcscat_c(szName, bSingle ? L"Pause" : L"Break"); break;
	case VK_CANCEL:
		// It's may be pressed with Ctrl only
		wcscat_c(szName, L"Break"); break;
	case VK_RETURN:
		wcscat_c(szName, L"Enter"); break;
	case VK_BACK:
		wcscat_c(szName, L"Backspace"); break;
	case 0xbd:
		wcscat_c(szName, L"-_"); break;
	case 0xbb:
		wcscat_c(szName, L"+="); break;

	case VK_WHEEL_UP:
		wcscat_c(szName, L"WheelUp"); break;
	case VK_WHEEL_DOWN:
		wcscat_c(szName, L"WheelDown"); break;
	case VK_WHEEL_LEFT:
		wcscat_c(szName, L"WheelLeft"); break;
	case VK_WHEEL_RIGHT:
		wcscat_c(szName, L"WheelRight"); break;

	case VK_LBUTTON:
		wcscat_c(szName, L"LButton"); break;
	case VK_RBUTTON:
		wcscat_c(szName, L"RButton"); break;
	case VK_MBUTTON:
		wcscat_c(szName, L"MButton"); break;

	default:
		if (vk >= VK_F1 && vk <= VK_F24)
		{
			swprintf_c(szName, L"F%u", (DWORD)vk - VK_F1 + 1);
		}
		else if ((vk >= (BYTE)'A' && vk <= (BYTE)'Z') || (vk >= (BYTE)'0' && vk <= (BYTE)'9'))
		{
			szName[0] = vk;
			szName[1] = 0;
		}
		else
		{
			szName[0] = MapVirtualKey(vk, MAPVK_VK_TO_CHAR);
			szName[1] = 0;
			//BYTE States[256] = {};
			//// Скорее всго не сработает
			//if (!ToUnicode(vk, 0, States, szName, countof(szName), 0))
			//	swprintf_c(szName, L"<%u>", (DWORD)vk);
			// есть еще if (!GetKeyNameText((LONG)(DWORD)*m_HotKeys[i].VkPtr, szName, countof(szName)))
		}
	}
}

UINT ConEmuChord::GetVkByKeyName(LPCWSTR asName, int* pnScanCode/*=nullptr*/, DWORD* pnControlState/*=nullptr*/)
{
	if (!asName || !*asName)
		return 0;

	// pnScanCode и pnControlState заранее не трогаем специально, вызывающая функция может в них что-то хранить

	static struct NameToVk { LPCWSTR sName; UINT VK; } Keys[] =
	{
		{L"Win", VK_LWIN}, {L"LWin", VK_LWIN}, {L"RWin", VK_RWIN},
		{L"Ctrl", VK_CONTROL}, {L"LCtrl", VK_LCONTROL}, {L"RCtrl", VK_RCONTROL},
		{L"Alt", VK_MENU}, {L"LAlt", VK_LMENU}, {L"RAlt", VK_RMENU},
		{L"Shift", VK_SHIFT}, {L"LShift", VK_LSHIFT}, {L"RShift", VK_RSHIFT},
		{L"Apps", VK_APPS},
		{L"Left", VK_LEFT}, {L"Right", VK_RIGHT}, {L"Up", VK_UP}, {L"Down", VK_DOWN},
		{L"PgUp", VK_PRIOR}, {L"PgDn", VK_NEXT}, {L"Home", VK_HOME}, {L"End", VK_END},
		{L"Space", VK_SPACE}, {L" ", VK_SPACE}, {L"Tab", VK_TAB}, {L"\t", VK_TAB},
		{L"Esc", VK_ESCAPE}, {L"\x1B", VK_ESCAPE},
		{L"Insert", VK_INSERT}, {L"Delete", VK_DELETE}, {L"Backspace", VK_BACK},
		{L"Enter", VK_RETURN}, {L"Return", VK_RETURN}, {L"NumpadEnter", VK_RETURN},
		{L"Pause", VK_PAUSE}, {L"Break", VK_CANCEL},
		//{L"_", 0xbd}, {L"=", 0xbb},
		{L"Numpad0", VK_NUMPAD0}, {L"Numpad1", VK_NUMPAD1}, {L"Numpad2", VK_NUMPAD2}, {L"Numpad3", VK_NUMPAD3}, {L"Numpad4", VK_NUMPAD4},
		{L"Numpad5", VK_NUMPAD5}, {L"Numpad6", VK_NUMPAD6}, {L"Numpad7", VK_NUMPAD7}, {L"Numpad8", VK_NUMPAD8}, {L"Numpad9", VK_NUMPAD9},
		{L"NumpadDot", VK_DECIMAL}, {L"NumpadDiv", VK_DIVIDE}, {L"NumpadMult", VK_MULTIPLY}, {L"NumpadAdd", VK_ADD}, {L"NumpadSub", VK_SUBTRACT},
		// ConEmu internal codes
		{L"WheelUp", VK_WHEEL_UP}, {L"WheelDown", VK_WHEEL_DOWN}, {L"WheelLeft", VK_WHEEL_LEFT}, {L"WheelRight", VK_WHEEL_RIGHT},
		// End of predefined codes
		{nullptr}
	};

	// Fn?
	if (asName[0] == L'F' && isDigit(asName[1]) && (asName[2] == 0 || isDigit(asName[2])))
	{
		wchar_t* pEnd;
		long iNo = wcstol(asName + 1, &pEnd, 10);
		if (iNo < 1 || iNo > 24)
			return 0;
		return (VK_F1 + (iNo - 1));
	}

	// From table
	for (NameToVk* pk = Keys; pk->sName; pk++)
	{
		if (lstrcmp(asName, pk->sName) == 0)
			return pk->VK;
	}

	// Now, only One-char (OEM-ASCII) keys
	if (asName[1] == 0)
	{
		UINT vkScan = VkKeyScan(*asName);
		if (LOBYTE(vkScan))
		{
			UINT VK = LOBYTE(vkScan);
			if (pnControlState)
			{
				if (HIBYTE(vkScan) & 1)
					* pnControlState |= SHIFT_PRESSED;
				if ((HIBYTE(vkScan) & 2) && !((*pnControlState) & RIGHT_CTRL_PRESSED))
					* pnControlState |= LEFT_CTRL_PRESSED;
				if ((HIBYTE(vkScan) & 4) && !((*pnControlState) & RIGHT_ALT_PRESSED))
					* pnControlState |= LEFT_ALT_PRESSED;
			}
			UINT SC = MapVirtualKey(VK, 0/*MAPVK_VK_TO_VSC*/);
			if (pnScanCode)
				* pnScanCode = SC;
			return VK;
		}
	}

	// Unknown key
	return 0;
}

DWORD ConEmuChord::GetModifier(DWORD VkMod, int idx)
{
	DWORD Mod = VkMod & CEHOTKEY_MODMASK;

	if ((Mod == CEHOTKEY_NOMOD) || (Mod == 0))
	{
		_ASSERTE(((VkMod & CEHOTKEY_MODMASK) != 0) || (VkMod == 0));
		return 0;
	}
	else if (Mod == CEHOTKEY_NUMHOSTKEY)
	{
		// Только для цифирок!
		_ASSERTE((VkMod & 0xFF) >= '0' && ((VkMod & 0xFF) <= '9'));
		Mod = (gpSet->HostkeyNumberModifier() << 8);
	}
	else if (Mod == CEHOTKEY_ARRHOSTKEY)
	{
		// Только для стрелок!
		_ASSERTE(((VkMod & 0xFF) == VK_LEFT) || ((VkMod & 0xFF) == VK_RIGHT) || ((VkMod & 0xFF) == VK_UP) || ((VkMod & 0xFF) == VK_DOWN));
		Mod = (gpSet->HostkeyArrowModifier() << 8);
	}

	switch (idx)
	{
	case 1:
		Mod = ((Mod & 0xFF00) >> 8);
		break;
	case 2:
		Mod = ((Mod & 0xFF0000) >> 16);
		break;
	case 3:
		Mod = ((Mod & 0xFF000000) >> 24);
		break;
	default:
		_ASSERTE(idx == 1 || idx == 2 || idx == 3);
		Mod = 0;
	}

	if (Mod == VK_RWIN)
	{
		_ASSERTE(Mod != VK_RWIN); // Храниться должен LWIN
		Mod = VK_LWIN;
	}

	return Mod;
}

// Есть ли в этом (VkMod) хоткее - модификатор Mod (VK)
bool ConEmuChord::HasModifier(DWORD VkMod, BYTE Mod/*VK*/)
{
	if (Mod && ((GetModifier(VkMod, 1) == Mod) || (GetModifier(VkMod, 2) == Mod) || (GetModifier(VkMod, 3) == Mod)))
		return true;
	return false;
}

DWORD ConEmuChord::SetModifier(DWORD VkMod, BYTE Mod, bool Xor/*=true*/)
{
	DWORD AllMod = VkMod & CEHOTKEY_MODMASK;
	if ((VkMod == CEHOTKEY_NUMHOSTKEY) || (VkMod == CEHOTKEY_ARRHOSTKEY))
	{
		// Низя
		_ASSERTE(!((VkMod == CEHOTKEY_NUMHOSTKEY) || (VkMod == CEHOTKEY_ARRHOSTKEY)));
		return VkMod;
	}

	if (AllMod == CEHOTKEY_NOMOD)
		AllMod = 0;

	bool Processed = false;

	// Младший байт - VK. Старшие три - модификаторы. Их и трогаем
	for (int i = 1; i <= 3; i++)
	{
		DWORD vkExist = GetModifier(VkMod, i);
		if (isKey(vkExist, Mod) || isKey(Mod, vkExist))
		{
			Processed = true;

			if (Xor)
			{
				switch (i)
				{
				case 1:
					AllMod = (GetModifier(VkMod, 2) << 8) | (GetModifier(VkMod, 3) << 16);
					break;
				case 2:
					AllMod = (GetModifier(VkMod, 1) << 8) | (GetModifier(VkMod, 3) << 16);
					break;
				case 3:
					AllMod = (GetModifier(VkMod, 1) << 8) | (GetModifier(VkMod, 2) << 16);
					break;
				}
			}
			else if (vkExist != Mod)
			{
				// Например, заменить LShift на Shift
				switch (i)
				{
				case 1:
					AllMod = (VkMod & 0xFFFF0000) | (Mod << 8);
					break;
				case 2:
					AllMod = (VkMod & 0xFF00FF00) | (Mod << 16);
					break;
				case 3:
					AllMod = (VkMod & 0x00FFFF00) | (Mod << 24);
					break;
				}
			}

			break;
		}
	}

	if (!Processed)
	{
		DWORD AddMod = 0;

		if (!GetModifier(VkMod, 1))
			AddMod = (((DWORD)Mod) << 8);
		else if (!GetModifier(VkMod, 2))
			AddMod = (((DWORD)Mod) << 16);
		else if (!GetModifier(VkMod, 3))
			AddMod = (((DWORD)Mod) << 24);
		else
		{
			// Иначе - некуда модификатор пихать, а так уже три
			_ASSERTE(GetModifier(VkMod, 3) == 0);
		}

		if (AddMod != 0)
			AllMod |= AddMod;
	}

	// Нельзя сбрасывать единственный модификатор
	if (!AllMod)
	{
		_ASSERTE(AllMod != 0);
	}
	else
	{
		VkMod = GetHotkey(VkMod) | AllMod;
	}

	return VkMod;
}

void ConEmuChord::Set(BYTE aVk, ConEmuModifiers aMod)
{
	ValidateChordMod(aMod);
	Vk = aVk;
	Mod = aMod;
}

UINT ConEmuChord::GetModifiers(BYTE (&Mods)[3]) const
{
	UINT m = 0;
	for (size_t i = 0; gvkMatchList[i].Vk; i++)
	{
		if (Mod & gvkMatchList[i].Mod)
		{
			Mods[m] = gvkMatchList[i].Vk;
			m++;
			if (m >= countof(Mods))
				break;
		}
	}
	for (size_t k = m; k < countof(Mods); k++)
	{
		Mods[k] = 0;
	}
	return m;
}

bool ConEmuChord::IsEmpty() const
{
	return (Vk == 0);
}

bool ConEmuChord::IsEqual(const ConEmuChord& Key) const
{
	return IsEqual(Key.Vk, Key.Mod);
}

bool ConEmuChord::IsEqual(BYTE aVk, ConEmuModifiers aMod) const
{
	_ASSERTE(Vk && aVk);

	if (Vk != aVk)
		return false;

	// Правила проверки
	// Если требуется нажатие Ctrl, то неважно левый или правый нажат в действительности
	// Если требуется нажатие LCtrl/RCtrl, то именно этот модификатор должен присутствовать
	// При этом не должны быть нажаты другой модификатор (если не было задано явно Левый+Правый)
	// То же для Alt и Shift
	// Win и Apps проверяются без деления правый/левый

	ValidateChordMod(aMod);
	ValidateChordMod(Mod);

	if (!ConEmuChord::Compare(aMod, Mod))
		return false;

	// However, that is not enough
	// In some cases (both left/right modifiers set?)
	// we need to check flags in reverse mode too

	if (!ConEmuChord::Compare(Mod, aMod))
		return false;

	// OK, all tests passed, key match
	return true;
}

bool ConEmuChord::Compare(const ConEmuModifiers aMod1, const ConEmuModifiers aMod2)
{
	ConEmuModifiers nTest;

	for (size_t i = 0; i < gvkMatchList[i].Vk; i++)
	{
		const CEVkMatch& k = gvkMatchList[i];

		if (!k.Distinct)
		{	// Ctrl/LCtrl/RCtrl etc
			_ASSERTE((k.Left|k.Right) != cvk_NULL);
			nTest = (aMod1 & (k.Left|k.Right));
			if (nTest)
			{
				if ((aMod2 & (k.Left|k.Right)) != nTest)
				{
					// Allowed only in case of
					// 1) aMod2 has cvk_Ctrl (insensitive key)
					// 2) aMod1 has NOT both modifiers at once
					if ((aMod2 & k.Mod)
						&& (!(nTest & k.Left) || !(nTest & k.Right))
						&& (!(aMod2 & (k.Left|k.Right))))
					{
						i += 2; // skip next two keys (LCtr, RCtrl), they are already checked
						continue;
					}
					return false;
				}
				//PressTest |= nTest;
				//DePressMask &= ~(k.Mod|nTest); // Clear all Ctrl's bits
			}
			else if (aMod1 & k.Mod)
			{	// Insensitive (left or right)
				if (!(aMod2 & k.Mod))
					return false;
				//PressTest |= k.Mod;
				//DePressMask &= ~(k.Mod|k.Left|k.Right); // Clear all Ctrl's bits
			}
		}
		else
		{	// Win, Apps
			_ASSERTE((k.Left|k.Right) == cvk_NULL);
			if (aMod1 & k.Mod)
			{	// Insensitive key (no left/right)
				if (!(aMod2 & k.Mod))
					return false;
			}
		}

		if (!k.Distinct)
			i += 2; // skip next two keys (LCtr, RCtrl), they are already checked
	}

	return true;
}
