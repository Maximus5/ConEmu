
/*
Copyright (c) 2013-2017 Maximus5
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
#include "Hotkeys.h"
#include "ConEmu.h"
#include "LngRc.h"
#include "Options.h"
#include "OptionsClass.h"
#include "SetCmdTask.h"
#include "VirtualConsole.h"

// Некоторые комбинации нужно обрабатывать "на отпускание" во избежание глюков с интерфейсом
const struct ConEmuHotKey* ConEmuSkipHotKey = ((ConEmuHotKey*)INVALID_HANDLE_VALUE);

extern CEVkMatch gvkMatchList[];

// That is what is stored in the settings
DWORD ConEmuHotKey::GetVkMod() const
{
	DWORD VkMod = 0;
	DWORD Vk = (DWORD)(BYTE)Key.Vk;

	if (Vk)
	{
		// Let iterate
		int iPos = 8; // <<
		ConEmuModifiers Mod = Key.Mod;

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

			if ((Mod & k.Mod) && !(Mod & (k.Left|k.Right)))
			{
				VkMod |= (k.Vk << iPos);
				iPos += 8;
				if (iPos > 24)
					break;

				if (!k.Distinct)
				{
					_ASSERTE((k.Left|k.Right) != cvk_NULL);
					i += 2;
				}
			}
		}

		if (!VkMod && (HkType != chk_Modifier))
			VkMod = CEHOTKEY_NOMOD;

		VkMod |= Vk;
	}

	return VkMod;
}

void ConEmuHotKey::SetVkMod(DWORD VkMod)
{
	// Init
	BYTE Vk = LOBYTE(VkMod);
	ConEmuModifiers Mod = cvk_NULL;

	// Check modifiers
	DWORD vkLeft = (VkMod & CEHOTKEY_MODMASK);

	if ((HkType == chk_NumHost) || (vkLeft == CEHOTKEY_NUMHOSTKEY))
	{
		_ASSERTE((HkType == chk_NumHost) && (vkLeft == CEHOTKEY_NUMHOSTKEY))
		Mod = cvk_NumHost;
	}
	else if ((HkType == chk_ArrHost) || (vkLeft == CEHOTKEY_ARRHOSTKEY))
	{
		_ASSERTE((HkType == chk_ArrHost) && (vkLeft == CEHOTKEY_ARRHOSTKEY))
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

	Key.Set(Vk, Mod);
}

bool ConEmuHotKey::CanChangeVK() const
{
	//chk_System - пока не настраивается
	return (HkType==chk_User || HkType==chk_Global || HkType==chk_Local || HkType==chk_Macro || HkType==chk_Task);
}

bool ConEmuHotKey::IsTaskHotKey() const
{
	return (HkType==chk_Task && DescrLangID<0);
}

// 0-based
int ConEmuHotKey::GetTaskIndex() const
{
	if (IsTaskHotKey())
		return -(DescrLangID+1);
	return -1;
}

// 0-based
void ConEmuHotKey::SetTaskIndex(int iTaskIdx)
{
	if (iTaskIdx >= 0)
	{
		DescrLangID = -(iTaskIdx+1);
	}
	else
	{
		_ASSERTE(iTaskIdx>=0);
		DescrLangID = 0;
	}
}

LPCWSTR ConEmuHotKey::GetDescription(wchar_t* pszDescr, int cchMaxLen, bool bAddMacroIndex /*= false*/) const
{
	if (!pszDescr)
		return L"";

	_ASSERTE(cchMaxLen>200);

	LPCWSTR pszRc = pszDescr;
	bool lbColon = false;

	*pszDescr = 0;

	if (this->Enabled)
	{
		if (this->Enabled == InSelection)
		{
			lstrcpyn(pszDescr, L"[InSelection] ", cchMaxLen);
		}
		else if (!this->Enabled())
		{
			lstrcpyn(pszDescr, L"[Disabled] ", cchMaxLen);
		}

		if (*pszDescr)
		{
			int nLen = lstrlen(pszDescr);
			pszDescr += nLen;
			cchMaxLen -= nLen;
		}
	}

	if (bAddMacroIndex && (HkType == chk_Macro))
	{
		_wsprintf(pszDescr, SKIPLEN(cchMaxLen) L"Macro %02i: ", DescrLangID-vkGuiMacro01+1);
		int nLen = lstrlen(pszDescr);
		pszDescr += nLen;
		cchMaxLen -= nLen;
		lbColon = true;
	}

	if (IsTaskHotKey())
	{
		const CommandTasks* pCmd = gpSet->CmdTaskGet(GetTaskIndex());
		if (pCmd)
			lstrcpyn(pszDescr, pCmd->pszName ? pCmd->pszName : L"", cchMaxLen);
	}
	else if ((HkType != chk_Macro) && !CLngRc::getHint(DescrLangID, pszDescr, cchMaxLen))
	{
		if ((HkType == chk_User) && GuiMacro && *GuiMacro)
			lstrcpyn(pszDescr, GuiMacro, cchMaxLen);
		else
			_wsprintf(pszDescr, SKIPLEN(cchMaxLen) L"#%i", DescrLangID);
	}
	else if ((cchMaxLen >= 16) && GuiMacro && *GuiMacro)
	{
		size_t nLen = _tcslen(pszDescr);
		pszDescr += nLen;
		cchMaxLen -= nLen;

		if (!lbColon && (cchMaxLen > 2) && (pszDescr > pszRc))
		{
			lstrcpyn(pszDescr, L": ", cchMaxLen);
			pszDescr += 2;
			cchMaxLen -= 2;
		}
		lstrcpyn(pszDescr, GuiMacro, cchMaxLen);
	}

	return pszRc;
}

void ConEmuHotKey::Free()
{
	SafeFree(GuiMacro);
}

// nHostMod в младших 3-х байтах может содержать VK (модификаторы).
// Функция проверяет, чтобы они не дублировались
void ConEmuHotKey::TestHostkeyModifiers(DWORD& nHostMod)
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
				if (vkList[0]!=VK_LWIN && vkList[1]!=VK_LWIN && vkList[2]!=VK_LWIN)
					vkList[i++] = vk;
				break;
			case VK_APPS:
				if (vkList[0]!=VK_APPS && vkList[1]!=VK_APPS && vkList[2]!=VK_APPS)
					vkList[i++] = vk;
				break;
			case VK_LCONTROL:
			case VK_RCONTROL:
			case VK_CONTROL:
				for (int k = 0; k < 3; k++)
				{
					if (vkList[k]==VK_LCONTROL || vkList[k]==VK_RCONTROL || vkList[k]==VK_CONTROL)
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
					if (vkList[k]==VK_LMENU || vkList[k]==VK_RMENU || vkList[k]==VK_MENU)
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
					if (vkList[k]==VK_LSHIFT || vkList[k]==VK_RSHIFT || vkList[k]==VK_SHIFT)
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
DWORD ConEmuHotKey::GetHotKeyMod(DWORD VkMod)
{
	DWORD nMOD = 0;

	for (int i = 1; i <= 3; i++)
	{
		switch (GetModifier(VkMod,i))
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

#if 0
	// User want - user get
	if (!nMOD)
	{
		_ASSERTE(nMOD!=0);
		nMOD = MOD_WIN;
	}
#endif

	return nMOD;
}

void ConEmuHotKey::SetHotKey(BYTE Vk, BYTE vkMod1/*=0*/, BYTE vkMod2/*=0*/, BYTE vkMod3/*=0*/)
{
	SetVkMod(MakeHotKey(Vk, vkMod1, vkMod2, vkMod3));
}

bool ConEmuHotKey::Equal(BYTE Vk, BYTE vkMod1/*=0*/, BYTE vkMod2/*=0*/, BYTE vkMod3/*=0*/)
{
	//TODO
	DWORD VkMod = GetVkMod();
	DWORD VkModCmp = MakeHotKey(Vk, vkMod1, vkMod2, vkMod3);
	return (VkMod == VkModCmp);
}

// Сервисная функция для инициализации. Формирует готовый VkMod
DWORD ConEmuHotKey::MakeHotKey(BYTE Vk, BYTE vkMod1/*=0*/, BYTE vkMod2/*=0*/, BYTE vkMod3/*=0*/)
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
			vkHotKey |= (vkMod1<<iShift);
			iShift += 8;
		}
		if (vkMod2)
		{
			vkHotKey |= (vkMod2<<iShift);
			iShift += 8;
		}
		if (vkMod3)
		{
			vkHotKey |= (vkMod3<<iShift);
			iShift += 8;
		}
	}
	return vkHotKey;
}

// Задать или сбросить модификатор в VkMod
DWORD ConEmuHotKey::SetModifier(DWORD VkMod, BYTE Mod, bool Xor/*=true*/)
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
		if (isKey(vkExist,Mod) || isKey(Mod,vkExist))
		{
			Processed = true;

			if (Xor)
			{
				switch (i)
				{
				case 1:
					AllMod = (GetModifier(VkMod, 2)<<8) | (GetModifier(VkMod, 3)<<16);
					break;
				case 2:
					AllMod = (GetModifier(VkMod, 1)<<8) | (GetModifier(VkMod, 3)<<16);
					break;
				case 3:
					AllMod = (GetModifier(VkMod, 1)<<8) | (GetModifier(VkMod, 2)<<16);
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
	//if (GetModifier(VkMod, 1) == Mod)
	//{
	//	AllMod = (GetModifier(VkMod, 2)<<8) | (GetModifier(VkMod, 3)<<16);
	//	Processed = true;
	//}
	//else if (GetModifier(VkMod, 2) == Mod)
	//{
	//	AllMod = (GetModifier(VkMod, 1)<<8) | (GetModifier(VkMod, 3)<<16);
	//	Processed = true;
	//}
	//else if (GetModifier(VkMod, 3) == Mod)
	//{
	//	AllMod = (GetModifier(VkMod, 1)<<8) | (GetModifier(VkMod, 2)<<16);
	//	Processed = true;
	//}

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
		_ASSERTE(AllMod!=0);
	}
	else
	{
		VkMod = GetHotkey(VkMod) | AllMod;
	}

	return VkMod;
}

// // Вернуть назначенные модификаторы (idx = 1..3). Возвращает 0 (нету) или VK
DWORD ConEmuHotKey::GetModifier(DWORD VkMod, int idx)
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
		_ASSERTE((VkMod & 0xFF)>='0' && ((VkMod & 0xFF)<='9'));
		Mod = (gpSet->HostkeyNumberModifier() << 8);
	}
	else if (Mod == CEHOTKEY_ARRHOSTKEY)
	{
		// Только для стрелок!
		_ASSERTE(((VkMod & 0xFF)==VK_LEFT) || ((VkMod & 0xFF)==VK_RIGHT) || ((VkMod & 0xFF)==VK_UP) || ((VkMod & 0xFF)==VK_DOWN));
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
		_ASSERTE(idx==1 || idx==2 || idx==3);
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
bool ConEmuHotKey::HasModifier(DWORD VkMod, BYTE Mod/*VK*/)
{
	if (Mod && ((GetModifier(VkMod, 1) == Mod) || (GetModifier(VkMod, 2) == Mod) || (GetModifier(VkMod, 3) == Mod)))
		return true;
	return false;
}

// Вернуть имя модификатора (типа "Apps+Space")
LPCWSTR ConEmuHotKey::GetHotkeyName(wchar_t (&szFull)[128], bool bShowNone /*= true*/) const
{
	_ASSERTE(this && this!=ConEmuSkipHotKey);

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

			if ((Key.Mod & k.Mod) && !(Key.Mod & (k.Left|k.Right)))
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

// Return user-friendly key name
LPCWSTR ConEmuHotKey::GetHotkeyName(DWORD aVkMod, wchar_t (&szFull)[128], bool bShowNone /*= true*/)
{
	ConEmuHotKey hk = {0, chk_User};
	hk.SetVkMod(aVkMod);
	return hk.GetHotkeyName(szFull, bShowNone);
}

LPCWSTR ConEmuHotKey::CreateNotUniqueWarning(LPCWSTR asHotkey, LPCWSTR asDescr1, LPCWSTR asDescr2, CEStr& rsWarning)
{
	CEStr lsFmt(CLngRc::getRsrc(lng_HotkeyDuplicated/*"Hotkey <%s> is not unique"*/));
	wchar_t* ptrPoint = lsFmt.ms_Val ? (wchar_t*)wcsstr(lsFmt.ms_Val, L"%s") : NULL;
	if (!ptrPoint)
	{
		rsWarning.Attach(lstrmerge(L"Hotkey <", asHotkey, L"> is not unique",
			L"\n", asDescr1, L"\n", asDescr2));
	}
	else
	{
		_ASSERTE(ptrPoint[0] == L'%' && ptrPoint[1] == L's' && ptrPoint[2]);
		*ptrPoint = 0;
		rsWarning.Attach(lstrmerge(lsFmt.ms_Val, asHotkey, ptrPoint+2,
			L"\n", asDescr1, L"\n", asDescr2));
	}
	return rsWarning.ms_Val;
}

void ConEmuHotKey::GetVkKeyName(BYTE vk, wchar_t (&szName)[32], bool bSingle /*= true*/)
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
			_wsprintf(szName, SKIPLEN(countof(szName)) L"F%u", (DWORD)vk-VK_F1+1);
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
			//	_wsprintf(szName, SKIPLEN(countof(szName)) L"<%u>", (DWORD)vk);
			// есть еще if (!GetKeyNameText((LONG)(DWORD)*m_HotKeys[i].VkPtr, szName, countof(szName)))
		}
	}
}

UINT ConEmuHotKey::GetVkByKeyName(LPCWSTR asName, int* pnScanCode/*=NULL*/, DWORD* pnControlState/*=NULL*/)
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
		{NULL}
	};

	// Fn?
	if (asName[0] == L'F' && isDigit(asName[1]) && (asName[2] == 0 || isDigit(asName[2])))
	{
		wchar_t* pEnd;
		long iNo = wcstol(asName+1, &pEnd, 10);
		if (iNo < 1 || iNo > 24)
			return 0;
		return (VK_F1 + (iNo-1));
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
					*pnControlState |= SHIFT_PRESSED;
				if ((HIBYTE(vkScan) & 2) && !((*pnControlState) & RIGHT_CTRL_PRESSED))
					*pnControlState |= LEFT_CTRL_PRESSED;
				if ((HIBYTE(vkScan) & 4) && !((*pnControlState) & RIGHT_ALT_PRESSED))
					*pnControlState |= LEFT_ALT_PRESSED;
			}
			UINT SC = MapVirtualKey(VK, 0/*MAPVK_VK_TO_VSC*/);
			if (pnScanCode)
				*pnScanCode = SC;
			return VK;
		}
	}

	// Unknown key
	return 0;
}

// Извлечь сам VK
DWORD ConEmuHotKey::GetHotkey(DWORD VkMod)
{
	return (VkMod & 0xFF);
}

// Some statics - context checks
bool ConEmuHotKey::UseWinNumber()
{
	return gpSetCls->IsMulti() && gpSet->isUseWinNumber;
}

bool ConEmuHotKey::UseWinArrows()
{
	return gpSet->isUseWinArrows;
}

bool ConEmuHotKey::InSelection()
{
	CVConGuard VCon;
	if (gpConEmu->GetActiveVCon(&VCon) < 0)
		return false;
	if (!VCon->RCon()->isSelectionPresent())
		return false;
	return true;
}

bool ConEmuHotKey::UseCTSShiftArrow()
{
	CVConGuard VCon;
	if (gpConEmu->GetActiveVCon(&VCon) < 0)
		return false;

	CRealConsole* pRCon = VCon->RCon();
	if (!pRCon || pRCon->isFar() || pRCon->isSelectionPresent())
		return false;

	const AppSettings* pApp = gpSet->GetAppSettings(pRCon->GetActiveAppSettingsId());
	if (!pApp)
		return false;

	return pApp->CTSShiftArrowStart();
}

bool ConEmuHotKey::UseCtrlTab()
{
	return gpSet->isTabSelf;
}

bool ConEmuHotKey::UseCtrlBS()
{
	return (gpSet->GetAppSettings()->CTSDeleteLeftWord() != 0);
}

bool ConEmuHotKey::UseDndLKey()
{
	return ((gpSet->isDragEnabled & DRAG_L_ALLOWED) == DRAG_L_ALLOWED);
}

bool ConEmuHotKey::UseDndRKey()
{
	return ((gpSet->isDragEnabled & DRAG_R_ALLOWED) == DRAG_R_ALLOWED);
}

bool ConEmuHotKey::UseWndDragKey()
{
	return gpSet->isMouseDragWindow;
}

bool ConEmuHotKey::UsePromptFind()
{
	CVConGuard VCon;
	if (gpConEmu->GetActiveVCon(&VCon) < 0 || !VCon->RCon())
		return false;
	CRealConsole* pRCon = VCon->RCon();
	if (!pRCon->isFar())
		return true;
	// Even if Far we may search for the prompt, if we are in panels and they are OFF
	if (!pRCon->isFarPanelAllowed())
		return false;
	DWORD dwFlags = pRCon->GetActiveDlgFlags();
	if (((dwFlags & FR_FREEDLG_MASK) != 0)
		|| pRCon->isEditor() || pRCon->isViewer()
		|| pRCon->isFilePanel(true, true, true))
		return true;
	return false;
}

/*
bool ConEmuHotKey::DontHookJumps(const ConEmuHotKey* pHK)
{
	bool bDontHook = false;
#if 0
	switch (pHK->DescrLangID)
	{
	case vkJumpPrevMonitor:
		bDontHook = (pHK->VkMod == MakeHotKey(VK_LEFT,VK_LWIN,VK_SHIFT));
		break;
	case vkJumpNextMonitor:
		bDontHook = (pHK->VkMod == MakeHotKey(VK_RIGHT,VK_LWIN,VK_SHIFT));
		break;
	}
#endif
	return bDontHook;
}
*/

#ifdef _DEBUG
/*
	Unit tests begin
*/
void ConEmuHotKey::HotkeyNameUnitTests()
{
	// Key names order:
	//    Win, Apps, Ctrl, Alt, Shift
	// Modifiers:
	//    if "Ctrl" (any) - only cvk_Ctrl must be set
	//    if "RCtrl" (or L) - then cvk_Ctrl AND cvk_RCtrl must be set
	//    if both "LCtrl+RCtrl" - then ALL cvk_Ctrl, cvk_LCtrl, cvk_RCtrl must be set
	struct {
		BYTE Vk;
		BYTE Mod[3];
		ConEmuModifiers ModTest;
		LPCWSTR KeyName;
	} Tests[] = {
			{VK_OEM_5/*'|\'*/, {VK_LCONTROL, VK_RCONTROL}, cvk_Ctrl|cvk_LCtrl|cvk_RCtrl, L"LCtrl+RCtrl+\\"},
			{VK_SPACE, {VK_CONTROL, VK_LWIN, VK_MENU}, cvk_Ctrl|cvk_Alt|cvk_Win, L"Win+Ctrl+Alt+Space"},
			{'W', {VK_LWIN, VK_SHIFT}, cvk_Win|cvk_Shift, L"Win+Shift+W"},
			{'L', {VK_RCONTROL}, cvk_Ctrl|cvk_RCtrl, L"RCtrl+L"},
			{'C', {VK_CONTROL}, cvk_Ctrl, L"Ctrl+C"},
			{0, {}, cvk_Naked, gsNoHotkey},
	};

	ConEmuHotKey HK = {0, chk_User};
	wchar_t szFull[128];
	int iCmp;

	for (size_t i = 0; i < countof(Tests); i++)
	{
		HK.SetHotKey(Tests[i].Vk, Tests[i].Mod[0], Tests[i].Mod[1], Tests[i].Mod[2]);
		_ASSERTE(HK.Key.Mod == Tests[i].ModTest);
		iCmp = wcscmp(HK.GetHotkeyName(szFull, true), Tests[i].KeyName);
		_ASSERTE(iCmp == 0);
	}
}
/*
	Unit tests end
*/
#endif
