
/*
Copyright (c) 2016 Maximus5
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
#define SHOWDEBUGSTR

#include "Header.h"
#include "../common/WRegistry.h"
#include "OptTypeComSpec.h"

CEOptionComSpec::CEOptionComSpec()
	: ConEmuComspec()
{
	Reset();
}

CEOptionComSpec::~CEOptionComSpec()
{
}

bool CEOptionComSpec::Load(SettingsBase* reg)
{
	if (!reg)
		return false;
	BYTE nVal = this->csType;
	reg->Load(L"ComSpec.Type", nVal);
	if (nVal <= cst_Last) this->csType = (ComSpecType)nVal;
	//
	nVal = this->csBits;
	reg->Load(L"ComSpec.Bits", nVal);
	if (nVal <= csb_Last) this->csBits = (ComSpecBits)nVal;
	//
	nVal = this->isUpdateEnv;
	reg->Load(L"ComSpec.UpdateEnv", nVal);
	this->isUpdateEnv = (nVal != 0);
	//
	nVal = (BYTE)((this->AddConEmu2Path & CEAP_AddConEmuBaseDir) == CEAP_AddConEmuBaseDir);
	reg->Load(L"ComSpec.EnvAddPath", nVal);
	SetConEmuFlags(this->AddConEmu2Path, CEAP_AddConEmuBaseDir, nVal ? CEAP_AddConEmuBaseDir : CEAP_None);
	//
	nVal = (BYTE)((this->AddConEmu2Path & CEAP_AddConEmuExeDir) == CEAP_AddConEmuExeDir);
	reg->Load(L"ComSpec.EnvAddExePath", nVal);
	SetConEmuFlags(this->AddConEmu2Path, CEAP_AddConEmuExeDir, nVal ? CEAP_AddConEmuExeDir : CEAP_None);
	//
	nVal = this->isAllowUncPaths;
	reg->Load(L"ComSpec.UncPaths", nVal);
	this->isAllowUncPaths = (nVal != 0);
	//
	reg->Load(L"ComSpec.Path", this->ComspecExplicit, countof(this->ComspecExplicit));
	return true;
}

bool CEOptionComSpec::Save(SettingsBase* reg) const
{
	if (!reg)
		return false;
	reg->Save(L"ComSpec.Type", (BYTE)ComSpec.csType);
	reg->Save(L"ComSpec.Bits", (BYTE)ComSpec.csBits);
	reg->Save(L"ComSpec.UpdateEnv", _bool(ComSpec.isUpdateEnv));
	reg->Save(L"ComSpec.EnvAddPath", _bool((ComSpec.AddConEmu2Path & CEAP_AddConEmuBaseDir) == CEAP_AddConEmuBaseDir));
	reg->Save(L"ComSpec.EnvAddExePath", _bool((ComSpec.AddConEmu2Path & CEAP_AddConEmuExeDir) == CEAP_AddConEmuExeDir));
	reg->Save(L"ComSpec.UncPaths", _bool(ComSpec.isAllowUncPaths));
	reg->Save(L"ComSpec.Path", ComSpec.ComspecExplicit);
	return true;
}

void CEOptionComSpec::Reset()
{
	ConEmuComspec* ptr = dynamic_cast<ConEmuComspec*>(this);
	_ASSERTE(ptr && ((LPVOID)ptr == (LPVOID)&csType));

	// ConEmuComspec has not pointers at the moment, so it's valid.
	ConEmuComspec ComSpec = {};

	ComSpec.AddConEmu2Path = CEAP_AddAll;
	ComSpec.isAllowUncPaths = FALSE;

	// Load defaults from windows registry (Command Processor settings)
	SettingsRegistry UncChk;
	if (UncChk.OpenKey(HKEY_CURRENT_USER, L"Software\\Microsoft\\Command Processor", KEY_READ))
	{
		DWORD DisableUNCCheck = 0;
		if (UncChk.Load(L"DisableUNCCheck", (LPBYTE)&DisableUNCCheck, sizeof(DisableUNCCheck)))
		{
			ComSpec.isAllowUncPaths = (DisableUNCCheck == 1);
		}
	}

	*ptr = ComSpec;
}
