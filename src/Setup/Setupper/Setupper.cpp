// NO BOM! Compiled with old gcc!

#undef TEST_BUILD

#include <windows.h>
// C RunTime Header Files
#include <stdlib.h>
#include <malloc.h>
#include <memory.h>
#include <tchar.h>
#include <commctrl.h>

#include "resource.h"
#include "VersionI.h"
#include "NextArg.h"

// String resources (messages)
#include "Setupper.h"


// Global Variables:
HINSTANCE hInst;
OSVERSIONINFO gOSVer = {sizeof(OSVERSIONINFO)};
wchar_t gsTitle[128]; // L"ConEmu %s installer"
wchar_t gsMessage[128];
wchar_t gsFull[1024];
wchar_t gsVer86[1024];
wchar_t gsVer64[1024];
wchar_t gsRunAsAdm[128];
wchar_t gsTempFolder[MAX_PATH-24];
wchar_t gsMsiFile[MAX_PATH];
wchar_t gsCabFile[MAX_PATH];
wchar_t gsExeFile[MAX_PATH];
bool gbExtractOnly = false;
bool gbUseElevation = false;
bool gbAlreadyAdmin = false;
BOOL isWin64 = FALSE;


#define countof(a) (sizeof((a))/(sizeof(*(a))))

#ifdef __GNUC__

enum _TASKDIALOG_FLAGS
{
    TDF_ENABLE_HYPERLINKS               = 0x0001,
    TDF_USE_HICON_MAIN                  = 0x0002,
    TDF_USE_HICON_FOOTER                = 0x0004,
    TDF_ALLOW_DIALOG_CANCELLATION       = 0x0008,
    TDF_USE_COMMAND_LINKS               = 0x0010,
    TDF_USE_COMMAND_LINKS_NO_ICON       = 0x0020,
    TDF_EXPAND_FOOTER_AREA              = 0x0040,
    TDF_EXPANDED_BY_DEFAULT             = 0x0080,
    TDF_VERIFICATION_FLAG_CHECKED       = 0x0100,
    TDF_SHOW_PROGRESS_BAR               = 0x0200,
    TDF_SHOW_MARQUEE_PROGRESS_BAR       = 0x0400,
    TDF_CALLBACK_TIMER                  = 0x0800,
    TDF_POSITION_RELATIVE_TO_WINDOW     = 0x1000,
    TDF_RTL_LAYOUT                      = 0x2000,
    TDF_NO_DEFAULT_RADIO_BUTTON         = 0x4000,
    TDF_CAN_BE_MINIMIZED                = 0x8000
};
typedef int TASKDIALOG_FLAGS;                         // Note: _TASKDIALOG_FLAGS is an int

enum _TASKDIALOG_COMMON_BUTTON_FLAGS
{
    TDCBF_OK_BUTTON            = 0x0001, // selected control return value IDOK
    TDCBF_YES_BUTTON           = 0x0002, // selected control return value IDYES
    TDCBF_NO_BUTTON            = 0x0004, // selected control return value IDNO
    TDCBF_CANCEL_BUTTON        = 0x0008, // selected control return value IDCANCEL
    TDCBF_RETRY_BUTTON         = 0x0010, // selected control return value IDRETRY
    TDCBF_CLOSE_BUTTON         = 0x0020  // selected control return value IDCLOSE
};
typedef int TASKDIALOG_COMMON_BUTTON_FLAGS;           // Note: _TASKDIALOG_COMMON_BUTTON_FLAGS is an int

typedef enum _TASKDIALOG_MESSAGES
{
    TDM_NAVIGATE_PAGE                   = WM_USER+101,
    TDM_CLICK_BUTTON                    = WM_USER+102, // wParam = Button ID
    TDM_SET_MARQUEE_PROGRESS_BAR        = WM_USER+103, // wParam = 0 (nonMarque) wParam != 0 (Marquee)
    TDM_SET_PROGRESS_BAR_STATE          = WM_USER+104, // wParam = new progress state
    TDM_SET_PROGRESS_BAR_RANGE          = WM_USER+105, // lParam = MAKELPARAM(nMinRange, nMaxRange)
    TDM_SET_PROGRESS_BAR_POS            = WM_USER+106, // wParam = new position
    TDM_SET_PROGRESS_BAR_MARQUEE        = WM_USER+107, // wParam = 0 (stop marquee), wParam != 0 (start marquee), lparam = speed (milliseconds between repaints)
    TDM_SET_ELEMENT_TEXT                = WM_USER+108, // wParam = element (TASKDIALOG_ELEMENTS), lParam = new element text (LPCWSTR)
    TDM_CLICK_RADIO_BUTTON              = WM_USER+110, // wParam = Radio Button ID
    TDM_ENABLE_BUTTON                   = WM_USER+111, // lParam = 0 (disable), lParam != 0 (enable), wParam = Button ID
    TDM_ENABLE_RADIO_BUTTON             = WM_USER+112, // lParam = 0 (disable), lParam != 0 (enable), wParam = Radio Button ID
    TDM_CLICK_VERIFICATION              = WM_USER+113, // wParam = 0 (unchecked), 1 (checked), lParam = 1 (set key focus)
    TDM_UPDATE_ELEMENT_TEXT             = WM_USER+114, // wParam = element (TASKDIALOG_ELEMENTS), lParam = new element text (LPCWSTR)
    TDM_SET_BUTTON_ELEVATION_REQUIRED_STATE = WM_USER+115, // wParam = Button ID, lParam = 0 (elevation not required), lParam != 0 (elevation required)
    TDM_UPDATE_ICON                     = WM_USER+116  // wParam = icon element (TASKDIALOG_ICON_ELEMENTS), lParam = new icon (hIcon if TDF_USE_HICON_* was set, PCWSTR otherwise)
} TASKDIALOG_MESSAGES;

typedef enum _TASKDIALOG_NOTIFICATIONS
{
    TDN_CREATED                         = 0,
    TDN_NAVIGATED                       = 1,
    TDN_BUTTON_CLICKED                  = 2,            // wParam = Button ID
    TDN_HYPERLINK_CLICKED               = 3,            // lParam = (LPCWSTR)pszHREF
    TDN_TIMER                           = 4,            // wParam = Milliseconds since dialog created or timer reset
    TDN_DESTROYED                       = 5,
    TDN_RADIO_BUTTON_CLICKED            = 6,            // wParam = Radio Button ID
    TDN_DIALOG_CONSTRUCTED              = 7,
    TDN_VERIFICATION_CLICKED            = 8,             // wParam = 1 if checkbox checked, 0 if not, lParam is unused and always 0
    TDN_HELP                            = 9,
    TDN_EXPANDO_BUTTON_CLICKED          = 10            // wParam = 0 (dialog is now collapsed), wParam != 0 (dialog is now expanded)
} TASKDIALOG_NOTIFICATIONS;

typedef struct _TASKDIALOG_BUTTON
{
    int     nButtonID;
    PCWSTR  pszButtonText;
} TASKDIALOG_BUTTON;

typedef HRESULT (WINAPI *PFTASKDIALOGCALLBACK)( HWND hwnd,  UINT msg,  WPARAM wParam,  LPARAM lParam,  LONG_PTR lpRefData);

typedef struct _TASKDIALOGCONFIG
{
    UINT        cbSize;
    HWND        hwndParent;
    HINSTANCE   hInstance;                              // used for MAKEINTRESOURCE() strings
    TASKDIALOG_FLAGS                dwFlags;            // TASKDIALOG_FLAGS (TDF_XXX) flags
    TASKDIALOG_COMMON_BUTTON_FLAGS  dwCommonButtons;    // TASKDIALOG_COMMON_BUTTON (TDCBF_XXX) flags
    PCWSTR      pszWindowTitle;                         // string or MAKEINTRESOURCE()
    union
    {
        HICON   hMainIcon;
        PCWSTR  pszMainIcon;
    } DUMMYUNIONNAME;
    PCWSTR      pszMainInstruction;
    PCWSTR      pszContent;
    UINT        cButtons;
    const TASKDIALOG_BUTTON  *pButtons;
    int         nDefaultButton;
    UINT        cRadioButtons;
    const TASKDIALOG_BUTTON  *pRadioButtons;
    int         nDefaultRadioButton;
    PCWSTR      pszVerificationText;
    PCWSTR      pszExpandedInformation;
    PCWSTR      pszExpandedControlText;
    PCWSTR      pszCollapsedControlText;
    union
    {
        HICON   hFooterIcon;
        PCWSTR  pszFooterIcon;
    } DUMMYUNIONNAME2;
    PCWSTR      pszFooter;
    PFTASKDIALOGCALLBACK pfCallback;
    LONG_PTR    lpCallbackData;
    UINT        cxWidth;                                // width of the Task Dialog's client area in DLU's. If 0, Task Dialog will calculate the ideal width.
} TASKDIALOGCONFIG;

#endif


BOOL IsWindows64()
{
	BOOL is64bitOs = FALSE, isWow64process = FALSE;

	// Проверяем, где мы запущены
	isWow64process = FALSE;
	HMODULE hKernel = GetModuleHandleW(L"kernel32.dll");

	if (hKernel)
	{
		typedef BOOL (WINAPI* IsWow64Process_t)(HANDLE hProcess, PBOOL Wow64Process);
		IsWow64Process_t IsWow64Process_f = (IsWow64Process_t)GetProcAddress(hKernel, "IsWow64Process");

		if (IsWow64Process_f)
		{
			BOOL bWow64 = FALSE;

			if (IsWow64Process_f(GetCurrentProcess(), &bWow64) && bWow64)
			{
				isWow64process = TRUE;
			}
		}
	}

	is64bitOs = isWow64process;

	return is64bitOs;
}

bool IsUserAdmin()
{
	// Проверять нужно только для висты и выше
	if (gOSVer.dwMajorVersion < 6)
		return FALSE;

	BOOL b;
	SID_IDENTIFIER_AUTHORITY NtAuthority = {SECURITY_NT_AUTHORITY};
	PSID AdministratorsGroup;
	b = AllocateAndInitializeSid(
	        &NtAuthority,
	        2,
	        SECURITY_BUILTIN_DOMAIN_RID,
	        DOMAIN_ALIAS_RID_ADMINS,
	        0, 0, 0, 0, 0, 0,
	        &AdministratorsGroup);

	if (b)
	{
		if (!CheckTokenMembership(NULL, AdministratorsGroup, &b))
		{
			b = FALSE;
		}

		FreeSid(AdministratorsGroup);
	}

	return (b != 0);
}

HRESULT CALLBACK Callback(HWND hwnd, UINT uNotification, WPARAM wParam, LPARAM lParam, LONG_PTR dwRefData)
{
	switch (uNotification)
	{
		case TDN_CREATED:
		{
			if (!gbAlreadyAdmin)
			{
				SendMessage(hwnd, TDM_SET_BUTTON_ELEVATION_REQUIRED_STATE, Ver86, 1);
				SendMessage(hwnd, TDM_SET_BUTTON_ELEVATION_REQUIRED_STATE, Ver64, 1);
			}
			break;
		}
		case TDN_VERIFICATION_CLICKED:
		{
			SendMessage(hwnd, TDM_SET_BUTTON_ELEVATION_REQUIRED_STATE, Ver86, wParam);
			SendMessage(hwnd, TDM_SET_BUTTON_ELEVATION_REQUIRED_STATE, Ver64, wParam);
			break;
		}
		case TDN_HYPERLINK_CLICKED:
		{
			PCWSTR pszHREF = (PCWSTR)lParam;
			if (pszHREF && *pszHREF)
				ShellExecute(hwnd, L"open", pszHREF, NULL, NULL, SW_SHOWNORMAL);
			break;
		}
	}
	return S_OK;
}

int ChooseVersion()
{
	int nInstallVer = 0; // IDCANCEL/Ver86/Ver64
	
	#ifndef TEST_BUILD
	if (!isWin64 && (gOSVer.dwMajorVersion <= 5))
		nInstallVer = Ver86;
	#endif

	if (!nInstallVer && gOSVer.dwMajorVersion >= 6)
	{
	    HRESULT hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
	    if (SUCCEEDED(hr))
	    {
			
			int nButtonPressed                  = 0;
			TASKDIALOGCONFIG config             = {0};
			const TASKDIALOG_BUTTON buttons[]   = { 
			                                        { Ver64, gsVer64 },
			                                        { Ver86, gsVer86 },			                                        
			                                      };
			config.cbSize                       = sizeof(config);
			config.hInstance                    = hInst;
			config.dwFlags                      = TDF_USE_COMMAND_LINKS|TDF_ALLOW_DIALOG_CANCELLATION
			                                     |TDF_CAN_BE_MINIMIZED|TDF_ENABLE_HYPERLINKS; //|TDIF_SIZE_TO_CONTENT;
			config.pszMainIcon                  = MAKEINTRESOURCE(IDI_ICON1);
			config.pszWindowTitle               = gsTitle;
			config.pszMainInstruction           = gsMessage;
			//config.pszContent                 = L"Choose between x86 and x64 versions";
			config.pButtons                     = isWin64 ? buttons : buttons+1;
			config.cButtons                     = isWin64 ? countof(buttons) : countof(buttons)-1;
			config.nDefaultButton               = isWin64 ? Ver64 : Ver86;
			config.pszFooter                    = gsWWW;
			config.pfCallback                   = Callback;
			
			gbAlreadyAdmin = IsUserAdmin();
			
			if (!gbAlreadyAdmin)
			{
				config.dwFlags |= TDF_VERIFICATION_FLAG_CHECKED;
				config.pszVerificationText = gsRunAsAdm;
			}
			
			HMODULE hDll = LoadLibrary(L"comctl32.dll");
			typedef HRESULT (WINAPI* TaskDialogIndirect_t)(const TASKDIALOGCONFIG *pTaskConfig, int *pnButton, int *pnRadioButton, BOOL *pfVerificationFlagChecked);
			TaskDialogIndirect_t TaskDialogIndirect_f = (TaskDialogIndirect_t)(hDll?GetProcAddress(hDll, "TaskDialogIndirect"):NULL);

			BOOL lbCheckBox = TRUE;
			if (TaskDialogIndirect_f && TaskDialogIndirect_f(&config, &nButtonPressed, NULL, &lbCheckBox) == S_OK)
			{
				switch (nButtonPressed)
				{
			    case IDCANCEL: // user cancelled the dialog
				case Ver86:
				case Ver64:
					if (lbCheckBox)
						gbUseElevation = true;
					nInstallVer = nButtonPressed;
					break;
			    default:
			        break; // should never happen
				}
			}
			
			CoUninitialize();
		}
	}
	
	if (!nInstallVer)
	{
		// "Old" UI Controls dialog
		int nButtonPressed = MessageBox(NULL, gsFull, gsTitle, MB_ICONQUESTION|MB_YESNOCANCEL);
		switch (nButtonPressed)
		{
		case IDYES:
			nInstallVer = Ver64;
			break;
		case IDNO:
			nInstallVer = Ver86;
			break;
		default:
			nInstallVer = IDCANCEL;
			break;
		}
	}
	
	return nInstallVer;
}

int ReportError(int nErr, LPCWSTR asLabel, void* pArg)
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

int ExportFile(int ResID, wchar_t* FilePath)
{
	HRSRC hResInfo = FindResource(hInst, MAKEINTRESOURCE(ResID), L"DATA");
	if (!hResInfo)
		return ReportError(1, msgFindResourceFailed, (void*)ResID);
		
	HGLOBAL hGbl = LoadResource(hInst, hResInfo);
	if (!hGbl)
		return ReportError(2, msgLoadResourceFailed, (void*)ResID);
		
	DWORD nResSize = hResInfo ? SizeofResource(hInst, hResInfo) : 0;
	LPVOID pRes = hGbl ? LockResource(hGbl) : NULL;
	
	int iRc = 0;
	
	if (!nResSize || !pRes)
	{
		iRc = ReportError(3, msgLoadResourceFailed, (void*)ResID);
	}
	else
	{
		HANDLE hFile = CreateFile(FilePath, GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
		if (!hFile || hFile == INVALID_HANDLE_VALUE)
		{
			iRc = ReportError(4, msgCreateFileFailed, FilePath);
		}
		else
		{
			DWORD nWritten = 0;
			if (!WriteFile(hFile, pRes, nResSize, &nWritten, NULL) || nWritten != nResSize)
			{
				iRc = ReportError(5, msgWriteFileFailed, FilePath);
			}
			
			CloseHandle(hFile);
			
			if (iRc != 0)
				DeleteFile(FilePath);
		}
	}
	
	return iRc;
}

#ifdef __GNUC__
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
#else
int APIENTRY _tWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPTSTR lpCmdLine, int nCmdShow)
#endif
{
	hInst = hInstance;
	isWin64 = IsWindows64();
	
	GetVersionEx(&gOSVer);
	
	int nInstallVer = 0;
	
	wsprintf(gsTitle, msgConEmuInstaller, CONEMUVERL);
	lstrcpyn(gsRunAsAdm, msgRunSetupAsAdmin, countof(gsRunAsAdm));

	wchar_t szArg[MAX_PATH+1];
	LPCWSTR pszCmdToken = GetCommandLine();
	LPCWSTR pszCmdLineW = pszCmdToken;
	
	gsTempFolder[0] = 0;
	
	while (0 == NextArg(&pszCmdToken, szArg))
	{
		if (lstrcmp(szArg, L"/?") == 0 || lstrcmp(szArg, L"-?") == 0 || lstrcmp(szArg, L"-h") == 0
			|| lstrcmp(szArg, L"-help") == 0 || lstrcmp(szArg, L"--help") == 0)
		{
			MessageBox(NULL, msgUsageExample, gsTitle, MB_ICONINFORMATION);
			return 1;
		}
		
		if (*szArg == L'/')
		{
			if (szArg[1] == L'e' || szArg[1] == L'E')
			{
				gbExtractOnly = true;
				if (szArg[2] == L':' && szArg[3])
				{
					lstrcpyn(gsTempFolder, (szArg[3]==L'"') ? (szArg+4) : (szArg+3), countof(gsTempFolder));
				}
				continue;
			}
		
		    if (memcmp(szArg, L"/p:x", 4*sizeof(*szArg)) == 0)
		    {
		    	gbAlreadyAdmin = IsUserAdmin();
				if (lstrcmpi(szArg+4, L"86") == 0)
				{
					nInstallVer = Ver86;
				}
				else if (lstrcmpi(szArg+4, L"86,adm") == 0)
				{
					nInstallVer = Ver86;
					gbUseElevation = !gbAlreadyAdmin;
				}
				else if (lstrcmpi(szArg+4, L"64") == 0)
				{
					nInstallVer = Ver64;
				}
				else if (lstrcmpi(szArg+4, L"64,adm") == 0)
				{
					nInstallVer = Ver64;
					gbUseElevation = !gbAlreadyAdmin;
				}
			}
			else
				pszCmdToken = pszCmdLineW;
			break;
		}
		else if (*szArg == L'-')
		{
			pszCmdToken = pszCmdLineW;
			break;
		}
			
		pszCmdLineW = pszCmdToken;
	}
	

	if (!gbExtractOnly)
	{
		wchar_t szInstallPath[MAX_PATH+32];
		bool bInstalled;
		HKEY hk;
	
		lstrcpyn(gsMessage, msgChooseInstallVer, countof(gsMessage));
		
			szInstallPath[0] = 0; bInstalled = false;
			struct {HKEY hk; LPCWSTR path; LPCWSTR name; bool our;}
				Keys[] = {
					{HKEY_LOCAL_MACHINE,L"SOFTWARE\\ConEmu",L"InstallDir",true},
					{HKEY_LOCAL_MACHINE,L"SOFTWARE\\Far Manager",L"InstallDir"},
					{HKEY_LOCAL_MACHINE,L"SOFTWARE\\Far2",L"InstallDir"},
					{HKEY_LOCAL_MACHINE,L"SOFTWARE\\Far",L"InstallDir"},
				};
			for (size_t s = 0; s < countof(Keys); s++)
			{
				if (!RegOpenKeyEx(Keys[s].hk, Keys[s].path, 0, KEY_READ, &hk)
					|| !RegOpenKeyEx(Keys[s].hk, Keys[s].path, 0, KEY_READ|KEY_WOW64_32KEY, &hk))
				{
					wchar_t szPath[MAX_PATH+1] = {}; DWORD cbSize = sizeof(szPath)-2;
					LONG lRc = RegQueryValueEx(hk, Keys[s].name, NULL, NULL, (LPBYTE)szPath, &cbSize);
					RegCloseKey(hk);
					if (!lRc && *szPath)
					{
						bInstalled = Keys[s].our;
						lstrcpy(szInstallPath, szPath);
						cbSize = lstrlen(szInstallPath);
						if (szInstallPath[cbSize-1] == L'\\') szInstallPath[cbSize-1] = 0;
						break;
					}
				}
			}
			if (szInstallPath[0] == 0)
			{
				GetEnvironmentVariable(L"ProgramFiles", szInstallPath, MAX_PATH);
				int nLen = lstrlen(szInstallPath);
				lstrcat(szInstallPath, (nLen > 0 && szInstallPath[nLen-1] != L'\\') ? L"\\ConEmu" : L"ConEmu");
			}
			wsprintf(gsVer86, msgInstallFolderIs, CONEMUVERL, L"x86", bInstalled ? msgPathCurrent : msgPathDefault, szInstallPath);

			
		if (isWin64)
		{
			
				szInstallPath[0] = 0; bInstalled = false;
				struct {HKEY hk; LPCWSTR path; LPCWSTR name; bool our;}
					Keys[] = {
						{HKEY_LOCAL_MACHINE,L"SOFTWARE\\ConEmu",L"InstallDir_x64",true},
						{HKEY_LOCAL_MACHINE,L"SOFTWARE\\Far Manager",L"InstallDir_x64"},
						{HKEY_LOCAL_MACHINE,L"SOFTWARE\\Far2",L"InstallDir_x64"},
						{HKEY_LOCAL_MACHINE,L"SOFTWARE\\Far",L"InstallDir_x64"},
					};
				for (size_t s = 0; s < countof(Keys); s++)
				{
					if (!RegOpenKeyEx(Keys[s].hk, Keys[s].path, 0, KEY_READ|KEY_WOW64_64KEY, &hk))
					{
						wchar_t szPath[MAX_PATH+1] = {}; DWORD cbSize = sizeof(szPath)-2;
						LONG lRc = RegQueryValueEx(hk, Keys[s].name, NULL, NULL, (LPBYTE)szPath, &cbSize);
						RegCloseKey(hk);
						if (!lRc && *szPath)
						{
							bInstalled = Keys[s].our;
							lstrcpy(szInstallPath, szPath);
							cbSize = lstrlen(szInstallPath);
							if (szInstallPath[cbSize-1] == L'\\') szInstallPath[cbSize-1] = 0;
							break;
						}
					}
				}
				if (szInstallPath[0] == 0)
				{
					GetEnvironmentVariable(L"ProgramW6432", szInstallPath, MAX_PATH);
					int nLen = lstrlen(szInstallPath);
					lstrcat(szInstallPath, (nLen > 0 && szInstallPath[nLen-1] != L'\\') ? L"\\ConEmu" : L"ConEmu");
				}
				wsprintf(gsVer64, msgInstallFolderIs, CONEMUVERL, L"x64", bInstalled ? msgPathCurrent : msgPathDefault, szInstallPath);

			wsprintf(gsFull, msgInstallConfirm, gsMessage);
		}
		else
		{
			gsVer64[0] = 0;
		}
	}
	else
	{
		wchar_t szPath[MAX_PATH+1];
		if (*gsTempFolder)
		{
			lstrcpy(szPath, gsTempFolder);
		}
		else
		{
			GetTempPath(countof(szPath) - 14, szPath);
			wchar_t* pszSubDir = szPath+lstrlen(szPath);
			lstrcpy(pszSubDir, L"ConEmu");
			pszSubDir += 6;
			lstrcpy(pszSubDir, CONEMUVERL);
		}
		
		lstrcpyn(gsMessage, msgChooseExtractVer, countof(gsMessage));
		wsprintf(gsVer86, msgExtractX86X64, CONEMUVERL, L"x86", szPath);
		wsprintf(gsVer64, msgExtractX86X64, CONEMUVERL, L"x64", szPath);
		wsprintf(gsFull, msgExtractConfirm, gsMessage, szPath);
	}
	
	if (nInstallVer == 0)
		nInstallVer = ChooseVersion(); // IDCANCEL/Ver86/Ver64
		
	if (nInstallVer != Ver86 && nInstallVer != Ver64)
		return 1;
	
	if (gbExtractOnly && *gsTempFolder)
	{
		CreateDirectory(gsTempFolder, NULL);
	}
	else
	{
		GetTempPath(countof(gsTempFolder) - 14, gsTempFolder);
		
		wchar_t* pszSubDir = gsTempFolder+lstrlen(gsTempFolder);
		lstrcpy(pszSubDir, L"ConEmu");
		pszSubDir += 6;
		lstrcpy(pszSubDir, CONEMUVERL);
		pszSubDir += lstrlen(pszSubDir);
		if (!CreateDirectory(gsTempFolder, NULL))
		{
			bool lbCreated = false;
			SYSTEMTIME st = {}; GetLocalTime(&st);
			for (int i = 0; i < 100; i++)
			{
				wsprintf(pszSubDir, L"_%02i%02i%02i%i", st.wHour, st.wMinute, st.wSecond, i);
				if (CreateDirectory(gsTempFolder, NULL))
				{
					lbCreated = true;
					break;
				}
			}
			if (!lbCreated)
			{
				return ReportError(10, msgTempFolderFailed, gsTempFolder);
			}
		}
	}

	wsprintf(gsMsiFile, L"%s\\ConEmu.%s.%s.msi", gsTempFolder, CONEMUVERL, (nInstallVer == Ver86) ? L"x86" : L"x64");
	wsprintf(gsCabFile, L"%s\\ConEmu.cab", gsTempFolder);
	
	bool lbNeedExe = false;
	if (!gbExtractOnly && gOSVer.dwMajorVersion >= 6)
		lbNeedExe = true;

	if (!lbNeedExe)
		gsExeFile[0] = 0;
	else
		wsprintf(gsExeFile, L"%s\\ConEmuSetup.exe", gsTempFolder);
	
	int iExpMsi = ExportFile(nInstallVer, gsMsiFile);
	int iExpCab = (iExpMsi == 0) ? ExportFile(CABFILE, gsCabFile) : -1;
	int iExpExe = (!lbNeedExe) ? 0 : (iExpCab == 0) ? ExportFile(EXEFILE, gsExeFile) : -1;
	if (iExpMsi != 0 || iExpCab != 0 || iExpExe != 0)
	{
		DeleteFile(gsMsiFile);
		DeleteFile(gsCabFile);
		if (*gsExeFile)
			DeleteFile(gsExeFile);
		RemoveDirectory(gsTempFolder);
		return (iExpMsi != 0) ? iExpMsi : iExpCab;
	}
	
	if (gbExtractOnly)
	{
		wchar_t szMessage[MAX_PATH*2];
		wsprintf(szMessage, msgExtractedSuccessfully, gsTempFolder);
		MessageBox(NULL, szMessage, gsTitle, MB_ICONINFORMATION);
		return 0;
	}

	int iInstRc = 0;
	SHELLEXECUTEINFO sei = {sizeof(sei)};
	wchar_t* pszParms = NULL;
	sei.fMask = SEE_MASK_NOCLOSEPROCESS|/*SEE_MASK_NOASYNC*/0x00000100; //|/*SEE_MASK_NOZONECHECKS*/0x00800000;
	sei.lpVerb = L"open";
	if (gOSVer.dwMajorVersion<=5 || !gbUseElevation)
	{
		sei.lpFile = gsMsiFile;
		sei.lpParameters = pszCmdToken;
	}
	else
	{
		// Executor has `<requestedExecutionLevel level="requireAdministrator" ...>` in manifest
		sei.lpFile = gsExeFile;
		int nMaxLen = lstrlen(gsMsiFile) + (pszCmdToken ? lstrlen(pszCmdToken) : 0) + 64;
		pszParms = (wchar_t*)malloc(nMaxLen*sizeof(wchar_t));
		wsprintf(pszParms, L"/i \"%s\" %s", gsMsiFile, pszCmdToken ? pszCmdToken : L"");
		sei.lpParameters = pszParms;
	}
	sei.lpDirectory = gsTempFolder;
	sei.nShow = SW_SHOWNORMAL;
	
	BOOL lbExecute = ShellExecuteEx(&sei);
	
	#if 0
	if (!lbExecute && lbNeedExe)
	{
		DWORD nErr = GetLastError();
		if (nErr == 1223)
		{
			// Отмена пользователем UAC, или правов не хватило?
			sei.fMask = SEE_MASK_NOCLOSEPROCESS|/*SEE_MASK_NOASYNC*/0x00000100; //|/*SEE_MASK_NOZONECHECKS*/0x00800000;
			sei.lpVerb = L"open";
			sei.lpFile = gsMsiFile;
			sei.lpParameters = pszCmdToken;
			sei.lpDirectory = gsTempFolder;
			sei.nShow = SW_SHOWNORMAL;
			
			lbExecute = ShellExecuteEx(&sei);
		}
	}
	#endif
	
	if (!lbExecute)
	{
		iInstRc = ReportError(20, msgInstallerFailed, gsMsiFile);
	}
	else
	{
		if (!sei.hProcess)
		{
			iInstRc = ReportError(21, msgInstallerFailed, gsMsiFile);
		}
		else
		{
			WaitForSingleObject(sei.hProcess, INFINITE);
			DWORD nCode = 0;
			SetLastError(0);
			wchar_t szFormat[256];
			if (GetExitCodeProcess(sei.hProcess, &nCode))
			{
				switch (nCode)
				{
				case 0:
					iInstRc = 0;
					break;
				case 1602: // cancelled by user
					iInstRc = 0; // don't show any errors
					break;
				case 3010: // reboot is required
					wsprintf(szFormat, msgRebootRequired, nCode);
					iInstRc = ReportError(100+nCode, szFormat, gsMsiFile);
					break;
				default:
					wsprintf(szFormat, msgInstallerFailedEx, nCode);
					iInstRc = ReportError(100+nCode, szFormat, gsMsiFile);
				}
			}
			else
			{
				lstrcpyn(szFormat, msgExitCodeFailed, countof(szFormat));
				iInstRc = ReportError(30, szFormat, gsMsiFile);
			}
		}
	}
	
	DeleteFile(gsMsiFile);
	DeleteFile(gsCabFile);
	if (*gsExeFile)
		DeleteFile(gsExeFile);
	RemoveDirectory(gsTempFolder);
	
	return iInstRc;
}
