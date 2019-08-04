
/*
Copyright (c) 2011-present Maximus5
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
#include "../UnitTests/gtest.h"

#include "../common/EnvVar.h"
#include "../common/MGuiMacro.h"
#include "../common/MStrEsc.h"
#include "../common/MStrSafe.h"
#include "../common/ProcessSetEnv.h"
#include "../common/WFiles.h"

#include <set>

#include "AboutDlg.h"
#include "Attach.h"
#include "ConEmu.h"
#include "ConEmuPipe.h"
#include "MacroImpl.h"
#include "Menu.h"
#include "Options.h"
#include "OptionsClass.h"
#include "RealConsole.h"
#include "SetCmdTask.h"
#include "SetColorPalette.h"
#include "SetDlgButtons.h"
#include "SystemEnvironment.h"
#include "TabBar.h"
#include "TrayIcon.h"
#include "TrayIcon.h"
#include "VConGroup.h"
#include "VirtualConsole.h"

namespace ConEmuMacro {
TEST(ConEmuMacro,Tests)
{
	wchar_t szMacro[] = L"Function1 +1 \"Arg2\" -3 -guimacro Function2(@\"Arg1\"\"\\n999\",0x999); Function3: \"abc\\t\\e\\\"\\\"\\n999\"; Function4 abc def; InvalidArg(9q)";
	LPWSTR pszString = szMacro;

	GuiMacro* p = GetNextMacro(pszString, false, NULL);
	EXPECT_TRUE(p && lstrcmp(p->szFunc,L"Function1")==0);
	EXPECT_TRUE(p && p->argc==3 && p->argv[0].Int==1 && lstrcmp(p->argv[1].Str,L"Arg2")==0 && p->argv[2].Int==-3);
	SafeFree(p);

	p = GetNextMacro(pszString, false, NULL);
	EXPECT_TRUE(p && lstrcmp(p->szFunc,L"Function2")==0);
	EXPECT_TRUE(p && p->argc==2 && lstrcmp(p->argv[0].Str,L"Arg1\"\\n999")==0 && p->argv[1].Int==0x999);
	SafeFree(p);

	p = GetNextMacro(pszString, false, NULL);
	EXPECT_TRUE(p && lstrcmp(p->szFunc,L"Function3")==0);
	EXPECT_TRUE(p && p->argc==1 && lstrcmp(p->argv[0].Str,L"abc\t\x1B\"\"\n999")==0);
	SafeFree(p);

	p = GetNextMacro(pszString, false, NULL);
	EXPECT_TRUE(p && lstrcmp(p->szFunc,L"Function4")==0);
	EXPECT_TRUE(p && p->argc==2 && lstrcmp(p->argv[0].Str,L"abc")==0 && lstrcmp(p->argv[1].Str,L"def")==0);
	SafeFree(p);

	p = GetNextMacro(pszString, false, NULL);
	// InvalidArg(9q)
	EXPECT_TRUE(p && p->argc==1 && lstrcmp(p->argv[0].Str, L"9q")==0);
	SafeFree(p);

	// Test some invalid declarations
	wcscpy_c(szMacro, L"VeryVeryVeryVeryVeryVeryVeryVeryVeryVeryVeryVeryVeryVeryVeryLongFunction(0);");
	pszString = szMacro;
	p = GetNextMacro(pszString, false, NULL);
	EXPECT_TRUE(p==NULL);
	SafeFree(p);

	wcscpy_c(szMacro, L"InvalidArg 1 abc d\"e fgh; Function2(\"\"\"\\n0);");
	pszString = szMacro;
	p = GetNextMacro(pszString, false, NULL);
	EXPECT_TRUE(p && lstrcmp(p->szFunc,L"InvalidArg")==0);
	EXPECT_TRUE(p && p->argc==4 && p->argv[0].Int == 1 && lstrcmp(p->argv[1].Str,L"abc")==0 && lstrcmp(p->argv[2].Str,L"d\"e")==0 && lstrcmp(p->argv[3].Str,L"fgh")==0);
	EXPECT_TRUE(pszString && lstrcmp(pszString, L"Function2(\"\"\"\\n0);") == 0);
	SafeFree(p);

	wcscpy_c(szMacro, L"InvalidArg:abc\";\"");
	pszString = szMacro;
	p = GetNextMacro(pszString, false, NULL);
	EXPECT_TRUE(p && lstrcmp(p->szFunc,L"InvalidArg")==0);
	EXPECT_TRUE(p && p->argc==1 && lstrcmp(p->argv[0].Str,L"abc\";\"")==0);
	SafeFree(p);

	wchar_t szMacro2[] = L"Print(abc)";
	pszString = szMacro2;
	p = GetNextMacro(pszString, false, NULL);
	EXPECT_TRUE(p && lstrcmp(p->szFunc,L"Print")==0);
	EXPECT_TRUE(p && p->argc==1 && lstrcmp(p->argv[0].Str,L"abc")==0);
	SafeFree(p);

	wchar_t szMacro3[] = L"Print(abc);Print(\"def\")";
	pszString = szMacro3;
	p = GetNextMacro(pszString, false, NULL);
	EXPECT_TRUE(p && lstrcmp(p->szFunc,L"Print")==0);
	EXPECT_TRUE(p && p->argc==1 && lstrcmp(p->argv[0].Str,L"abc")==0);
	SafeFree(p);
	p = GetNextMacro(pszString, false, NULL);
	EXPECT_TRUE(p && lstrcmp(p->szFunc,L"Print")==0);
	EXPECT_TRUE(p && p->argc==1 && lstrcmp(p->argv[0].Str,L"def")==0);
	SafeFree(p);

	//CEStr env_list(EnvironmentList(nullptr, nullptr, false));
	//EXPECT_FALSE(env_list.IsEmpty());
}
}  // namespace ConEmuMacro
