#include "tests.h"
#pragma warning(disable: 4474)

bool gbVerifyFailed = false;
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
	Verify_Step("-new_console parser tests");
	LPCWSTR pszTest = L"-new_console:a \\\"-new_console:c\\\" `-new_console:d:C:\\` -cur_console:b";
	LPCWSTR pszCmp = L"\\\"-new_console:c\\\" `-new_console:d:C:\\`";
	RConStartArgs arg; arg.pszSpecialCmd = lstrdup(pszTest);
	arg.ProcessNewConArg();
	int iCmp = lstrcmp(arg.pszSpecialCmd, pszCmp);
	Verify0((iCmp==0),"arg.pszSpecialCmd==\\\"-new_console:c\\\" `-new_console:d:C:\\`");

	Verify_Step("RConStartArgs::RunArgTests()");
	RConStartArgs::RunArgTests();
	Verify0(!gbVerifyFailed,"RConStartArgs tests passed");
	}

	if (gbVerifyFailed)
		Verify_MsgFail("Some tests failed!");
	else
		Verify_MsgOk("All done");
	return gbVerifyFailed ? 99 : 0;
}
