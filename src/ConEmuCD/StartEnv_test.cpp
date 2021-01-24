
/*
Copyright (c) 2016-present Maximus5
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

#include "../common/Common.h"
#include "../common/EnvVar.h"

#include "StartEnv.h"

#include "../UnitTests/gtest.h"

// Stub
enum class ConEmuExecAction;
// ReSharper disable twice CppParameterNeverUsed
int DoOutput(ConEmuExecAction eExecAction, LPCWSTR asCmdArg)
{
	return 0;
}

void StartEnvUnitTests()
{
	CStartEnv setEnv;
	wchar_t szTempName[80] = L"";
	SYSTEMTIME st = {}; GetLocalTime(&st);
	swprintf_c(szTempName, L"ce_temp_%u%u%u%u", st.wHour, st.wMinute, st.wSecond, st.wMilliseconds);
	SetEnvironmentVariable(szTempName, nullptr);

	const wchar_t szInit[] = L"initial", szPref[] = L"abc;", szSuffix[] = L";def";
	
	// ReSharper disable once CppJoinDeclarationAndAssignment
	CEStr pchValue;
	// ReSharper disable once CppInitializedValueIsAlwaysRewritten
	int iCmp = 0;

	// ce_temp="initial"
	setEnv.Set(szTempName, szInit);
	pchValue = GetEnvVar(szTempName);
	iCmp = pchValue ? wcscmp(pchValue, szInit) : -1;
	EXPECT_EQ(0, iCmp);
	pchValue.Release();

	// ce_temp="abc;initial"
	const CEStr lsSet1(szPref, L"%", szTempName, L"%");
	const CEStr lsCmd1(szPref, szInit);
	setEnv.Set(szTempName, lsSet1);
	pchValue = GetEnvVar(szTempName);
	iCmp = pchValue ? wcscmp(pchValue, lsCmd1) : -1;
	EXPECT_EQ(0, iCmp);
	pchValue.Release();
	setEnv.Set(szTempName, lsSet1);
	pchValue = GetEnvVar(szTempName);
	iCmp = pchValue ? wcscmp(pchValue, lsCmd1) : -1;
	EXPECT_EQ(0, iCmp);
	pchValue.Release();

	// ce_temp="abc;initial;def"
	const CEStr lsSet2(L"%", szTempName, L"%", szSuffix);
	const CEStr lsCmd2(szPref, szInit, szSuffix);
	setEnv.Set(szTempName, lsSet2);
	pchValue = GetEnvVar(szTempName);
	iCmp = pchValue ? wcscmp(pchValue, lsCmd2) : -1;
	EXPECT_EQ(0, iCmp);
	pchValue.Release();
	setEnv.Set(szTempName, lsSet2);
	pchValue = GetEnvVar(szTempName);
	iCmp = pchValue ? wcscmp(pchValue, lsCmd2) : -1;
	EXPECT_EQ(0, iCmp);
	pchValue.Release();

	// ce_temp="initial"
	setEnv.Set(szTempName, szInit);
	pchValue = GetEnvVar(szTempName);
	iCmp = pchValue ? wcscmp(pchValue, szInit) : -1;
	EXPECT_EQ(0, iCmp);
	pchValue.Release();

	// ce_temp="abc;initial;def"
	const CEStr lsSet3(szPref, L"%", szTempName, L"%", szSuffix);
	const CEStr lsCmd3(szPref, szInit, szSuffix);
	setEnv.Set(szTempName, lsSet3);
	pchValue = GetEnvVar(szTempName);
	iCmp = pchValue ? wcscmp(pchValue, lsCmd3) : -1;
	EXPECT_EQ(0, iCmp);
	pchValue.Release();
	setEnv.Set(szTempName, lsSet3);
	pchValue = GetEnvVar(szTempName);
	iCmp = pchValue ? wcscmp(pchValue, lsCmd3) : -1;
	EXPECT_EQ(0, iCmp);
	pchValue.Release();

	// Drop temp variable
	SetEnvironmentVariable(szTempName, nullptr);
}

TEST(StartEnv, UnitTests)
{
	StartEnvUnitTests();
}
