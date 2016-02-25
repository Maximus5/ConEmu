
/*
Copyright (c) 2014-2016 Maximus5
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

#pragma warning(disable: 4091)
#include <ShlObj.h>

#include "ConEmu.h"
#include "Hotkeys.h"
#include "Macro.h"
#include "Match.h"
#include "../common/WFiles.h"
#include "../common/WUser.h"

#include "UnitTests.h"

#ifdef _DEBUG
#include "DynDialog.h"
#include "../common/ProcessData.h"
#include "../common/ProcessSetEnv.h"
#endif

#ifdef _DEBUG
#include "../common/WModuleCheck.cpp"
#endif

#ifdef _DEBUG
#include "../common/MJsonReader.h"
#endif

#ifdef _DEBUG
void UnitMaskTests()
{
	struct {
		LPCWSTR asFileName, asMask;
		bool Result;
	} Tests[] = {
		{L"FileName.txt", L"FileName.txt", true},
		{L"FileName.txt", L"FileName", false},
		{L"FileName.txt", L"FileName.doc", false},
		{L"FileName.txt", L"*", true},
		{L"FileName.txt", L"FileName*", true},
		{L"FileName.txt", L"FileName*txt", true},
		{L"FileName.txt", L"FileName*.txt", true},
		{L"FileName.txt", L"FileName*e.txt", false},
		{L"FileName.txt", L"File*qqq", false},
		{L"FileName.txt", L"File*txt", true},
		{L"FileName.txt", L"Name*txt", false},
		{NULL}
	};
	bool bCheck;
	for (size_t i = 0; Tests[i].asFileName; i++)
	{
		bCheck = CompareFileMask(Tests[i].asFileName, Tests[i].asMask);
		_ASSERTE(bCheck == Tests[i].Result);
	}
	bCheck = true;
}

void UnitDriveTests()
{
	struct {
		LPCWSTR asPath, asResult;
	} Tests[] = {
		{L"C:", L"C:"},
		{L"C:\\Dir1\\Dir2\\File.txt", L"C:"},
		{L"\\\\Server\\Share", L"\\\\Server\\Share"},
		{L"\\\\Server\\Share\\Dir1\\Dir2\\File.txt", L"\\\\Server\\Share"},
		{L"\\\\?\\UNC\\Server\\Share", L"\\\\?\\UNC\\Server\\Share"},
		{L"\\\\?\\UNC\\Server\\Share\\Dir1\\Dir2\\File.txt", L"\\\\?\\UNC\\Server\\Share"},
		{L"\\\\?\\C:", L"\\\\?\\C:"},
		{L"\\\\?\\C:\\Dir1\\Dir2\\File.txt", L"\\\\?\\C:"},
		{NULL}
	};
	bool bCheck;
	wchar_t szDrive[MAX_PATH];
	for (size_t i = 0; Tests[i].asPath; i++)
	{
		wmemset(szDrive, L'z', countof(szDrive));
		bCheck = (wcscmp(GetDrive(Tests[i].asPath, szDrive, countof(szDrive)), Tests[i].asResult) == 0);
		_ASSERTE(bCheck);
	}
	bCheck = true;
}

void UnitFileNamesTest()
{
	_ASSERTE(IsDotsName(L"."));
	_ASSERTE(IsDotsName(L".."));
	_ASSERTE(!IsDotsName(L"..."));
	_ASSERTE(!IsDotsName(L""));

	struct {
		LPCWSTR asPath, asPart1, asPart2, asResult;
	} Tests[] = {
		{L"C:", L"Dir", L"File.txt", L"C:\\Dir\\File.txt"},
		{L"C:\\", L"\\Dir\\", L"\\File.txt", L"C:\\Dir\\File.txt"},
		{L"C:\\", L"\\File.txt", NULL, L"C:\\File.txt"},
		{L"C:", L"\\File.txt", NULL, L"C:\\File.txt"},
		{L"C:\\", L"File.txt", NULL, L"C:\\File.txt"},
		{NULL}
	};
	bool bCheck;
	wchar_t* pszJoin;
	for (size_t i = 0; Tests[i].asPath; i++)
	{
		pszJoin = JoinPath(Tests[i].asPath, Tests[i].asPart1, Tests[i].asPart2);
		bCheck = (pszJoin && (lstrcmp(pszJoin, Tests[i].asResult) == 0));
		_ASSERTE(bCheck);
		SafeFree(pszJoin);
	}
	bCheck = true;
}

void UnitExpandTest()
{
	CEStr szExe;
	wchar_t szChoc[MAX_PATH] = L"powershell -NoProfile -ExecutionPolicy unrestricted -Command \"iex ((new-object net.webclient).DownloadString('https://chocolatey.org/install.ps1'))\" && SET PATH=%PATH%;%systemdrive%\\chocolatey\\bin";
	wchar_t* pszExpanded = ExpandEnvStr(szChoc);
	int nLen = pszExpanded ? lstrlen(pszExpanded) : 0;
	BOOL bFound = FileExistsSearch(szChoc, szExe, false);
	wcscpy_c(szChoc, gpConEmu->ms_ConEmuExeDir);
	wcscat_c(szChoc, L"\\Tests\\Executables\\abcd");
	bFound = FileExistsSearch(szChoc, szExe, false);
	// TakeCommand
	ConEmuComspec tcc = {cst_AutoTccCmd};
	FindComspec(&tcc, false);
}

void UnitModuleTest()
{
	wchar_t* pszConEmuCD = lstrmerge(gpConEmu->ms_ConEmuBaseDir, WIN3264TEST(L"\\ConEmuCD.dll",L"\\ConEmuCD64.dll"));
	HMODULE hMod, hGetMod;
	bool bTest;

	_ASSERTE(!IsModuleValid((HMODULE)NULL));
	_ASSERTE(!IsModuleValid((HMODULE)INVALID_HANDLE_VALUE));

	hMod = GetModuleHandle(L"kernel32.dll");
	if (hMod)
	{
		bTest = IsModuleValid(hMod);
		_ASSERTE(bTest);
	}
	else
	{
		_ASSERTE(FALSE && "GetModuleHandle(kernel32) failed");
	}

	hMod = LoadLibrary(pszConEmuCD);
	if (hMod)
	{
		bTest = IsModuleValid(hMod);
		_ASSERTE(bTest);

		FreeLibrary(hMod);
		bTest = IsModuleValid(hMod);
		// Due to unknown reason (KIS?) FreeLibrary was not able to release hMod sometimes
		hGetMod = GetModuleHandle(pszConEmuCD);
		if (!hGetMod)
			bTest = IsModuleValid(hMod);
		_ASSERTE(!bTest || (hGetMod!=NULL));
	}
	else
	{
		_ASSERTE(FALSE && "LoadLibrary(pszConEmuCD) failed");
	}
}

void DebugUnitMprintfTest()
{
	wchar_t szTest[80];
	msprintf(szTest, countof(szTest), L"%u,%02u,%02u,%02u,%02x,%08x", 12345, 1, 23, 4567, 0xFE, 0xABCD1234);
	int nDbg = lstrcmp(szTest, L"12345,01,23,4567,fe,abcd1234");
	_ASSERTE(nDbg==0);
}

void DebugVersionTest()
{
	DWORDLONG const dwlConditionMask = VerSetConditionMask(VerSetConditionMask(0, VER_MAJORVERSION, VER_GREATER_EQUAL), VER_MINORVERSION, VER_GREATER_EQUAL);

	_ASSERTE(_WIN32_WINNT_WIN7==0x601);
	OSVERSIONINFOEXW osvi7 = {sizeof(osvi7), HIBYTE(_WIN32_WINNT_WIN7), LOBYTE(_WIN32_WINNT_WIN7)};
	bool bWin7 = VerifyVersionInfoW(&osvi7, VER_MAJORVERSION | VER_MINORVERSION, dwlConditionMask) != 0;

	_ASSERTE(_WIN32_WINNT_VISTA==0x600);
	OSVERSIONINFOEXW osvi6 = {sizeof(osvi6), HIBYTE(_WIN32_WINNT_VISTA), LOBYTE(_WIN32_WINNT_VISTA)};
	bool bWin6 = VerifyVersionInfoW(&osvi6, VER_MAJORVERSION | VER_MINORVERSION, dwlConditionMask) != 0;

	OSVERSIONINFOW osv = {sizeof(OSVERSIONINFOW)};
	GetOsVersionInformational(&osv);
	bool bVerWin7 = ((osv.dwMajorVersion > 6) || ((osv.dwMajorVersion == 6) && (osv.dwMinorVersion >= 1)));
	bool bVerWin6 = (osv.dwMajorVersion >= 6);

	_ASSERTE(bWin7 == bVerWin7);
	_ASSERTE(bWin6 == bVerWin6);
}

void DebugFileExistTests()
{
	bool b;
	b = DirectoryExists(L"C:\\Documents and Settings\\Maks\\.ipython\\");
	b = DirectoryExists(L"C:\\Documents and Settings\\Maks\\.ipython\\.");
	b = DirectoryExists(L"C:\\Documents and Settings\\Maks\\.ipython");
	b = DirectoryExists(L"C:\\Documents and Settings\\Maks\\.ipython-qq");
	b = FileExists(L"C:\\Documents and Settings\\Maks\\.ipython");
	b = FileExists(L"C:\\Documents and Settings\\Maks\\.ipython\\README");
	b = FileExists(L"C:\\Documents and Settings\\Maks\\.ipython\\.");
}

void DebugNeedCmdUnitTests()
{
	BOOL b;
	struct strTests { LPCWSTR pszCmd; BOOL bNeed; }
	Tests[] = {
		{L"\"C:\\windows\\notepad.exe -f \"makefile\" COMMON=\"../../../plugins/common\"\"", FALSE},
		{L"\"\"C:\\windows\\notepad.exe  -new_console\"\"", FALSE},
		{L"\"\"cmd\"\"", FALSE},
		{L"cmd /c \"\"C:\\Program Files\\Windows NT\\Accessories\\wordpad.exe\" -?\"", FALSE},
		{L"cmd /c \"dir c:\\\"", FALSE},
		{L"abc.cmd", TRUE},
		// Do not do too many heuristic. If user really needs redirection (for 'root'!)
		// he must explicitly call "cmd /c ...". With only exception if first exe not found.
		{L"notepad text & start explorer", FALSE},
	};
	LPCWSTR psArgs;
	BOOL bNeedCut, bRootIsCmd, bAlwaysConfirm, bAutoDisable;
	CEStr szExe;
	for (INT_PTR i = 0; i < countof(Tests); i++)
	{
		szExe.Empty();
		RConStartArgs rcs; rcs.pszSpecialCmd = lstrdup(Tests[i].pszCmd);
		rcs.ProcessNewConArg();
		b = IsNeedCmd(TRUE, rcs.pszSpecialCmd, szExe, &psArgs, &bNeedCut, &bRootIsCmd, &bAlwaysConfirm, &bAutoDisable);
		_ASSERTE(b == Tests[i].bNeed);
	}
}

void DebugCmdParserTests()
{
	RConStartArgs::RunArgTests();

	struct strTests { wchar_t szTest[100], szCmp[100]; }
	Tests[] = {
		{ L"\"Test1 & ^ \"\" Test2\"  Test3  \"Test \"\" 4\"", L"Test1 & ^ \" Test2\0Test3\0Test \" 4\0\0" }
	};

	LPCWSTR pszSrc, pszCmp;
	CEStr ls;
	int iCmp;
	for (INT_PTR i = 0; i < countof(Tests); i++)
	{
		pszSrc = Tests[i].szTest;
		pszCmp = Tests[i].szCmp;
		while (0 == NextArg(&pszSrc, ls))
		{
			DemangleArg(ls, ls.mb_Quoted);
			iCmp = wcscmp(ls.ms_Val, pszCmp);
			if (iCmp != 0)
			{
				_ASSERTE(lstrcmp(ls.ms_Val, pszCmp) == 0);
				break;
			}
			pszCmp += wcslen(pszCmp)+1;
		}
		_ASSERTE(*pszCmp == 0);
	}
}

void DebugStrUnitTest()
{
	struct strTests { wchar_t szTest[100], szCmp[100]; }
	Tests[] = {
		{L"Line1\n#Comment1\nLine2\r\n#Comment2\r\nEnd of file", L"Line1\nLine2\r\nEnd of file"},
		{L"Line1\n#Comment1\r\n", L"Line1\n"}
	};
	int iCmp;
	for (INT_PTR i = 0; i < countof(Tests); i++)
	{
		StripLines(Tests[i].szTest, L"#");
		iCmp = wcscmp(Tests[i].szTest, Tests[i].szCmp);
		_ASSERTE(iCmp == 0);
		StripLines(Tests[i].szTest, L"#");
		iCmp = wcscmp(Tests[i].szTest, Tests[i].szCmp);
		_ASSERTE(iCmp == 0);
	}

	// Some compilation and operators check

	CEStr str1;
	LPCWSTR pszTest = Tests[0].szTest;
	{
		str1 = lstrdup(pszTest);
	}
	iCmp = lstrcmp(str1, pszTest);
	_ASSERTE(iCmp == 0);

	{
		CEStr str2(lstrdup(pszTest));
		wchar_t* pszDup = lstrdup(pszTest);
		iCmp = lstrcmp(str2, pszTest);
		_ASSERTE(iCmp == 0 && str2.ms_Val && str2.ms_Val != pszTest);
	}

	// The following block must to be able to compile
	#if 0
	{
		CEStr str3(pszDup);
		str3 = pszTest;
	}

	{
		CEStr str4;
		str4 = pszTest;
		iCmp = lstrcmp(str4, pszTest);
		_ASSERTE(iCmp == 0 && str4.ms_Val && str4.ms_Val != pszTest);
	}

	{
		CEStr str5;
		str5 = str1;
		iCmp = lstrcmp(str5, pszTest);
		_ASSERTE(iCmp == 0 && str5.ms_Val && str3.ms_Val != str1.ms_Val);
	}
	#endif

	{
		CEStr strSelf;
		const wchar_t szTest1[] = L"Just a test";
		const wchar_t szTest2[] = L"Just a";
		const wchar_t szTest3[] = L"a";
		strSelf.Set(szTest1, 64); // Try to assign larger length, must accept ASCIIZ string length
		iCmp = lstrcmp(strSelf, szTest1);
		_ASSERTE(iCmp == 0);
		strSelf.Set((LPCTSTR)strSelf, 6);
		iCmp = lstrcmp(strSelf, szTest2);
		_ASSERTE(iCmp == 0);
		strSelf.Set(((LPCTSTR)strSelf) + 5, 1);
		iCmp = lstrcmp(strSelf, szTest3);
		_ASSERTE(iCmp == 0);
	}

	iCmp = lstrcmp(str1, pszTest);
	_ASSERTE(iCmp == 0);

	{
		LPCWSTR pszTest;
		CEStr szStr1(L"Test");
		CEStr szStr2;
		//pszTest = szStr1 ? szStr1 : L"<NULL>"; // -- expected to be cl error in VC14
		pszTest = szStr1 ? (LPCWSTR)szStr1 : L"<NULL>";
		pszTest = szStr2 ? (LPCWSTR)szStr2 : L"<NULL>";
		//msprintf(szStr2.GetBuffer(128), 128, L"Str1=`%s`", szStr1); //-- would be nice to forbid or assert this. strict `(LPCWSTR)szStr1` is required here
		UNREFERENCED_PARAMETER(pszTest);
	}
}

void DebugCpUnitTest()
{
	typedef struct {
		LPCWSTR sString;
		UINT nCP;
		wchar_t cEnd;
	} Test;
	Test Tests[] = {
		{L"Utf-8", CP_UTF8},
		{L"Utf-8;Acp", CP_UTF8, L';'},
		{L"ansi", CP_ACP},
		{L"ansicp;none", CP_ACP, L';'},
		{L"65001:1251", 65001, L':'},
		{NULL},
	};

	LPCWSTR pszEnd;
	UINT nCP;

	_ASSERTE(GetCpFromString(L"866") == 866);

	for (INT_PTR i = 0; Tests[i].sString; i++)
	{
		const Test& p = Tests[i];
		nCP = GetCpFromString(p.sString, &pszEnd);
		_ASSERTE(nCP == p.nCP);
		_ASSERTE((pszEnd == NULL && p.cEnd == 0) || (pszEnd && *pszEnd == p.cEnd));
	}
}

void DebugProcessNameTest()
{
	CProcessData processes;
	CEStr lsName(lstrdup(L"xxx.exe")), lsPath;
	DWORD nPID = GetCurrentProcessId();
	bool bRc = processes.GetProcessName(nPID, lsName.GetBuffer(MAX_PATH), MAX_PATH, lsPath.GetBuffer(MAX_PATH*2), MAX_PATH*2, NULL);
	_ASSERTE(bRc);
}

void DebugTestSetParser()
{
	CProcessEnvCmd cmd;
	cmd.AddLines(L"set  V1=From Settings \r\nset V2=Value2 & Value2\r\n set \"V3=\"Value \"\" 3 \" \r\n", false);
	_ASSERTE(cmd.mn_SetCount==3
		&& !wcscmp(cmd.m_Commands[0]->pszName, L"V1") && !wcscmp(cmd.m_Commands[0]->pszValue, L"From Settings")
		&& !wcscmp(cmd.m_Commands[1]->pszName, L"V2") && !wcscmp(cmd.m_Commands[1]->pszValue, L"Value2 & Value2")
		&& !wcscmp(cmd.m_Commands[2]->pszName, L"V3") && !wcscmp(cmd.m_Commands[2]->pszValue, L"\"Value \"\" 3 ")
		);
	CEStr lsTemp = cmd.Allocate(NULL);
}

void DebugMapsTests()
{
	MCircular<LONG,16> circ = {};
	circ.AddValue(1);
	circ.AddValue(2);
	_ASSERTE(circ.HasValue(1));
	_ASSERTE(circ.HasValue(2));
	_ASSERTE(!circ.HasValue(3));
	circ.DelValue(1);
	_ASSERTE(!circ.HasValue(1));
	_ASSERTE(circ.HasValue(2));
	for (int i = 100; i <= 116; i++)
		circ.AddValue(i);
	_ASSERTE(!circ.HasValue(2));
	_ASSERTE(!circ.HasValue(100));
	_ASSERTE(circ.HasValue(115));
	_ASSERTE(circ.HasValue(116));
}

void DebugArrayTests()
{
	MArray<int> arr;
	arr.push_back(1);
	arr.push_back(2);
	arr.push_back(3);
	arr.push_back(4);
	arr.insert(2, 5);
	_ASSERTE(arr[0]==1 && arr[1]==2 && arr[2]==5 && arr[3]==3 && arr[4]==4);
}

void DebugJsonTest()
{
	MJsonValue value, v1, v2, v3, v4;
	bool b;

	b = value.ParseJson(L"{ \"languages\": [ {\"id\": \"en\", \"name\": \"English\" } , {\"id\": \"ru\", \"name\": \"Русский\" } ] }");
	_ASSERTE(b);

	b = value.ParseJson(L"{ \"hints\": { \"bSaveSettings\": {"
		L"\"en\": [\"Save settings to registry/xml\\r\\n\""
		L", \"Don't close dialog if Shift pressed\"],"
		L"\"id\" : 1610"
		L"} } }");
	_ASSERTE(b);
	b = value.getItem(L"hints", v1);
	b = v1.getItem(L"bSaveSettings", v2);
	b = v2.getItem(L"id", v3);
	b = v2.getItem(L"en", v4);
}

void DebugUnitTests()
{
	DebugNeedCmdUnitTests();
	DebugCmdParserTests();
	UnitMaskTests();
	UnitDriveTests();
	UnitFileNamesTest();
	UnitExpandTest();
	UnitModuleTest();
	DebugUnitMprintfTest();
	DebugVersionTest();
	DebugFileExistTests();
	ConEmuMacro::UnitTests();
	DebugStrUnitTest();
	DebugCpUnitTest();
	CMatch::UnitTests();
	ConEmuChord::ChordUnitTests();
	ConEmuHotKey::HotkeyNameUnitTests();
	CDynDialog::UnitTests();
	DebugProcessNameTest();
	DebugTestSetParser();
	DebugMapsTests();
	DebugArrayTests();
	DebugJsonTest();
}
#endif

#if defined(__GNUC__) || defined(_DEBUG)
void GnuUnitTests()
{
	int nRegW = RegisterClipboardFormatW(CFSTR_FILENAMEW/*L"FileNameW"*/);
	int nRegA = RegisterClipboardFormatA("FileNameW");
	Assert(nRegW && nRegA && nRegW==nRegA);

	wchar_t szHex[] = L"\x2018"/*�*/ L"\x2019"/*�*/;
	wchar_t szNum[] = {0x2018, 0x2019, 0};
	int iDbg = lstrcmp(szHex, szNum);
	Assert(iDbg==0);
}
#endif
