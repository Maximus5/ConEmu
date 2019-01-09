// Don't convert it to UTF-8!

#include <windows.h>
// C RunTime Header Files
#include <stdlib.h>
#include <malloc.h>
#include <memory.h>
#include <tchar.h>
#include <commctrl.h>

#include "resource.h"
#include "../Setupper/VersionI.h"
#include "../Setupper/NextArg.h"

#define countof(a) (sizeof((a))/(sizeof(*(a))))

HINSTANCE hInst;
wchar_t gsTitle[128];
//OSVERSIONINFO gOSVer = {sizeof(OSVERSIONINFO)};

int ReportError(int nErr, LPCWSTR asLabel, const void* pArg)
{
	wchar_t szMessage[2048];
	DWORD nLastErr = GetLastError();
	wsprintf(szMessage, asLabel, pArg);
	if (nLastErr != 0)
	{
		wsprintf(szMessage+lstrlen(szMessage), HIWORD(nLastErr) ? L"\nErrorCode=0x%08X" : L"\nErrorCode=%u", nLastErr);
	}
	MessageBox(NULL, szMessage, gsTitle, MB_ICONSTOP|MB_SYSTEMMODAL);
	return nErr;
}

#ifdef __GNUC__
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
#else
int APIENTRY _tWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPTSTR lpCmdLine, int nCmdShow)
#endif
{
	hInst = hInstance;
	
	//GetVersionEx(&gOSVer);
	
	wsprintf(gsTitle, L"ConEmu %s installer", CONEMUVERL);
	
	LPCWSTR pszMsi = NULL, pszArguments = NULL;
	wchar_t szArg[MAX_PATH+1], szMsi[MAX_PATH+1];
	const wchar_t* pszCmdToken = GetCommandLine();
	//LPCWSTR pszCmdLineW = pszCmdToken;
	
	while ((0 == NextArg(&pszCmdToken, szArg)))
	{
		if (lstrcmpi(szArg, L"/i") == 0)
		{
			if (0 == NextArg(&pszCmdToken, szMsi) && *szMsi)
			{
				int nLen = lstrlen(szMsi);
				if (nLen < 5 || lstrcmpi(szMsi+nLen-4, L".msi") != 0)
					return ReportError(30, L"Invalid command line, '/i <msi-file>' not specified", NULL);
				pszMsi = szMsi;
				pszArguments = pszCmdToken;
			}
		}
	}
	
	if (!pszMsi)
	{
		return ReportError(31, L"Invalid command line, '/i <msi-file>' not specified", NULL);
	}
	
	int iInstRc = 0;
	
	SHELLEXECUTEINFO sei = {sizeof(sei)};
	sei.fMask = SEE_MASK_NOCLOSEPROCESS|/*SEE_MASK_NOASYNC*/0x00000100; //|/*SEE_MASK_NOZONECHECKS*/0x00800000;
	sei.lpVerb = L"open";
	sei.lpFile = pszMsi;
	sei.lpParameters = pszArguments;
	sei.lpDirectory = NULL;
	sei.nShow = SW_SHOWNORMAL;
	if (!ShellExecuteEx(&sei))
	{
		//iInstRc = ReportError(32, L"Installer failed\n%s", pszMsi);
		iInstRc = MAKE_HRESULT(SEVERITY_ERROR, FACILITY_WIN32, GetLastError());
	}
	else
	{
		if (!sei.hProcess)
		{
			//iInstRc = ReportError(33, L"Installer failed\n%s", pszMsi);
			iInstRc = MAKE_HRESULT(SEVERITY_ERROR, FACILITY_WINDOWS, GetLastError());
		}
		else
		{
			WaitForSingleObject(sei.hProcess, INFINITE);
			DWORD nCode = 0;
			SetLastError(0);
			//1602 - seems like "user cancelled"
			if (!GetExitCodeProcess(sei.hProcess, &nCode) /*|| (nCode != 0 && nCode != 1602)*/)
			{
				//wchar_t szFormat[128]; wsprintf(szFormat, L"Installer failed\n%%s\nExitCode=%u", nCode);
				//iInstRc = ReportError(nCode ? nCode : 34, szFormat, pszMsi);
				iInstRc = MAKE_HRESULT(SEVERITY_ERROR, FACILITY_WIN32, GetLastError());
			}
			else
			{
				iInstRc = nCode;
			}
		}
	}
	
	return iInstRc;
}
