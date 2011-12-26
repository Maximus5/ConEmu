
#pragma once

struct RConStartArgs
{
	BOOL     bDetached;
	wchar_t* pszSpecialCmd;
	wchar_t* pszStartupDir;
	
	BOOL     bRunAsAdministrator, bRunAsRestricted;
	wchar_t* pszUserName, *pszDomain, szUserPassword[MAX_PATH];
	BOOL     bForceUserDialog;
	
	BOOL     bBackgroundTab;
	
	BOOL     bBufHeight;
	UINT     nBufHeight;

 	enum {
 		eConfDefault = 0,
 		eConfAlways = 1,
 		eConfNever = 2,
 	} eConfirmation;
	
	BOOL     bRecreate; // Информационно и для CRecreateDlg

	RConStartArgs();
	~RConStartArgs();

	BOOL CheckUserToken(HWND hPwd);

	void ProcessNewConArg();
};
