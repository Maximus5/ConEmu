#include "tests.h"
#pragma warning(disable: 4474)

bool gbVerifyFailed = false;
bool gbVerifyStepFailed = false;
bool gbVerifyVerbose = false;

int main(int argc, char** argv)
{
	HeapInitialize();

	for (int i=0; i<argc; i++)
	{
		if (strcmp(argv[i], "-verbose")==0 || strcmp(argv[i], "--verbose")==0)
			gbVerifyVerbose = true;
	}

	// Tests

	{
	Verify_Step("CEStr condition checks");
	CEStr lsVal;
	Verify0((!lsVal),"{NULL} (!ls_Val)");
	Verify0((lsVal.IsEmpty()),"{NULL} (ls_Val.IsEmpty())");

	lsVal = L"";
	Verify0((!lsVal),"{\"\"} (!ls_Val)");
	Verify0((lsVal.IsEmpty()),"{\"\"} (ls_Val.IsEmpty())");

	lsVal = L"abc";
	Verify0((lsVal),"{\"abc\"} (ls_Val)");
	Verify0((!lsVal.IsEmpty()),"{\"abc\"} (!ls_Val.IsEmpty()) ");
	}

	{
	Verify_Step("CEStr ctor_1");
	CEStr ls1; ls1.Set(L"Test");
	CEStr ls2(ls1);
	Verify0((ls1.ms_Val && ls2.ms_Val && ls1.ms_Val!=ls2.ms_Val),"ls1.ms_Val!=ls2.ms_Val)");
	}

	{
	Verify_Step("CEStr ctor_2");
	CEStr ls1; ls1.Set(L"Test");
	CEStr ls2 = ls1;
	Verify0((ls1.ms_Val && ls2.ms_Val && ls1.ms_Val!=ls2.ms_Val),"ls1.ms_Val!=ls2.ms_Val)");
	}

	{
	Verify_Step("CEStr assign_1");
	CEStr ls1; ls1.Set(L"Test");
	CEStr ls2(ls1.ms_Val);
	Verify0((ls1.ms_Val && ls2.ms_Val && ls1.ms_Val!=ls2.ms_Val),"ls1.ms_Val!=ls2.ms_Val)");
	}

	{
	Verify_Step("CEStr assign_2");
	CEStr ls1; ls1.Set(L"Test");
	CEStr ls2 = ls1.ms_Val;
	Verify0((ls1.ms_Val && ls2.ms_Val && ls1.ms_Val!=ls2.ms_Val),"ls1.ms_Val!=ls2.ms_Val)");
	}

	{
	Verify_Step("ls1(`Test`)");
	CEStr ls1(L"Test");
	Verify0((ls1.ms_Val && 0==wcscmp(ls1.ms_Val,L"Test")),"ls1==`Test`");

	Verify_Step("ls12 = ls1.Detach()");
	/* Store ptr for Verify test result */ LPCWSTR pszPtr = ls1.ms_Val;
	CEStr ls2 = ls1.Detach();
	Verify2((ls2.ms_Val && !ls1.ms_Val && ls2.ms_Val==pszPtr),"ls2.ms_Val{x%p}==pszPtr{x%p}",ls2.ms_Val,pszPtr);
	}

	{
	Verify_Step("ls3 = `Test3`");
	CEStr ls3 = L"Test3";
	Verify0((ls3.ms_Val && 0==wcscmp(ls3.ms_Val,L"Test3")),"ls3==`Test3`");

	Verify_Step("ls4 = (LPCWSTR)ls3.ms_Val");
	CEStr ls4 = static_cast<LPCWSTR>(ls3.ms_Val);
	Verify2((ls4.ms_Val && ls4.ms_Val != ls3.ms_Val),"ls4.ms_Val{x%p}!=ls3.ms_Val{x%p}",ls4.ms_Val,ls3.ms_Val);

	Verify_Step("ls5 = lstrdup(ls3)");
	CEStr ls5 = lstrdup(ls3);
	Verify0((ls5.ms_Val && 0==wcscmp(ls5.ms_Val,L"Test3")),"ls5==`Test3`");

	Verify_Step("ls6(lstrdup(ls3))");
	CEStr ls6(lstrdup(ls3));
	Verify0((ls6.ms_Val && 0==wcscmp(ls6.ms_Val,L"Test3")),"ls6==`Test3`");
	}

	{
	Verify_Step("NextArg and Switch comparison");
	LPCWSTR pszCmd = L"conemu.exe /c/dir -run -inside=0x800 /cmdlist \"-inside=\\eCD /d %1\" -bad|switch ";
	CmdArg ls;
	Verify0(((pszCmd=NextArg(pszCmd,ls))),"NextArg conemu.exe");
	Verify0((!ls.IsPossibleSwitch()),"!IsPossibleSwitch()");
	Verify0(((pszCmd=NextArg(pszCmd,ls))),"NextArg /c/dir");
	Verify0((!ls.IsPossibleSwitch()),"!IsPossibleSwitch()");
	Verify0(((pszCmd=NextArg(pszCmd,ls))),"NextArg -run");
	Verify0((ls.OneOfSwitches(L"/cmd",L"/run")),"OneOfSwitches(/cmd,/run)");
	Verify0((!ls.OneOfSwitches(L"/cmd",L"/cmdlist")),"!OneOfSwitches(/cmd,/cmdlist)");
	Verify0((ls.IsSwitch(L"-run")),"IsSwitch(-run)");
	Verify0(((pszCmd=NextArg(pszCmd,ls))),"NextArg -inside=0x800");
	Verify0((ls.IsSwitch(L"-inside=")),"IsSwitch(-inside=)");
	Verify0((ls.OneOfSwitches(L"-inside",L"-inside=")),"OneOfSwitches(-inside,-inside=)");
	Verify0((!ls.IsSwitch(L"-inside")),"!IsSwitch(-inside)");
	Verify0(((pszCmd=NextArg(pszCmd,ls))),"NextArg /cmdlist");
	Verify0((ls.IsSwitch(L"-cmdlist")),"IsSwitch(-cmdlist)");
	Verify0(((pszCmd=NextArg(pszCmd,ls))),"NextArg \"-inside=\\eCD /d %%1\"");
	Verify0((ls.IsSwitch(L"-inside:")),"IsSwitch(-inside=)");
	Verify0(((pszCmd=NextArg(pszCmd,ls))),"NextArg -bad|switch");
	Verify0((ls.Compare(L"-bad|switch")==0),"Compare(-bad|switch)");
	Verify0((!ls.IsPossibleSwitch()),"!IsPossibleSwitch");
	}

	{
	Verify_Step("-new_console parser tests");
	LPCWSTR pszTest = L"-new_console:a \\\"-new_console:c\\\" `-new_console:d:C:\\` -cur_console:b";
	LPCWSTR pszCmp = L"\\\"-new_console:c\\\" `-new_console:d:C:\\`";
	RConStartArgsEx arg; arg.pszSpecialCmd = lstrdup(pszTest);
	arg.ProcessNewConArg();
	int iCmp = lstrcmp(arg.pszSpecialCmd, pszCmp);
	Verify0((iCmp==0),"arg.pszSpecialCmd==\\\"-new_console:c\\\" `-new_console:d:C:\\`");

	Verify_Step("RConStartArgsEx::RunArgTests()");
	RConStartArgsEx::RunArgTests();
	Verify0(!gbVerifyStepFailed,"RConStartArgsEx tests passed");
	}

	{
	Verify_Step("msprintf tests");
	wchar_t szBuffer[200];
	msprintf(szBuffer, countof(szBuffer), L"%u %03u %03u %i %x %02X %02X %04x %08X",
		123, 98, 4567, -234, 0x12AB, 0x0A, 0xABC, 0x01A0, 0x0765ABCD);
	const wchar_t szStd[] = L"123 098 4567 -234 12ab 0A ABC 01a0 0765ABCD";
	int iCmp = lstrcmp(szBuffer, szStd);
	WVerify2((iCmp==0),L"`%s` (msprintf[W])\n    `%s` (standard)", szBuffer, szStd);
	char szBufA[200];
	msprintf(szBufA, countof(szBufA), "%u %i %x %02X %02X %04x %08X",
		123, -234, 0x12AB, 0x0A, 0xABC, 0x01A0, 0x0765ABCD);
	const char szStdA[] = "123 -234 12ab 0A ABC 01a0 0765ABCD";
	iCmp = lstrcmpA(szBufA, szStdA);
	Verify2((iCmp==0),"`%s` (msprintf[A])\n    `%s` (standard)", szBufA, szStdA);
	}

	if (gbVerifyFailed)
		Verify_MsgFail("Some tests failed!");
	else
		Verify_MsgOk("All done");
	return gbVerifyFailed ? 99 : 0;
}
