
/*
Copyright (c) 2009-2017 Maximus5
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

#define DEBUGSTRRUN(s) //DEBUGSTR(s)

#include "../common/MProcess.h"

#include "../common/TokenHelper.h"
#include "../common/WSession.h"

#include "TaskScheduler.h"



/// Run process under restrictive token
BOOL CreateProcessRestricted(LPCWSTR lpApplicationName, LPWSTR lpCommandLine,
							 LPSECURITY_ATTRIBUTES lpProcessAttributes, LPSECURITY_ATTRIBUTES lpThreadAttributes,
							 BOOL bInheritHandles, DWORD dwCreationFlags, LPVOID lpEnvironment,
							 LPCWSTR lpCurrentDirectory, LPSTARTUPINFOW lpStartupInfo, LPPROCESS_INFORMATION lpProcessInformation,
							 LPDWORD pdwLastError /*= NULL*/)
{
	BOOL lbRc = FALSE;
	HANDLE hToken = NULL, hTokenRest = NULL;

	if (OpenProcessToken(GetCurrentProcess(),
	                    TOKEN_DUPLICATE | TOKEN_ASSIGN_PRIMARY | TOKEN_QUERY | TOKEN_EXECUTE,
	                    &hToken))
	{
		enum WellKnownAuthorities
		{
			NullAuthority = 0, WorldAuthority, LocalAuthority, CreatorAuthority,
			NonUniqueAuthority, NtAuthority, MandatoryLabelAuthority = 16
		};
		SID *pAdmSid = (SID*)calloc(sizeof(SID)+sizeof(DWORD)*2,1);
		pAdmSid->Revision = SID_REVISION;
		pAdmSid->SubAuthorityCount = 2;
		pAdmSid->IdentifierAuthority.Value[5] = NtAuthority;
		pAdmSid->SubAuthority[0] = SECURITY_BUILTIN_DOMAIN_RID;
		pAdmSid->SubAuthority[1] = DOMAIN_ALIAS_RID_ADMINS;
		SID_AND_ATTRIBUTES sidsToDisable[] =
		{
			{pAdmSid}
		};

		#ifdef __GNUC__
		HMODULE hAdvApi = GetModuleHandle(L"AdvApi32.dll");
		CreateRestrictedToken_t CreateRestrictedToken = (CreateRestrictedToken_t)GetProcAddress(hAdvApi, "CreateRestrictedToken");
		#endif

		if (CreateRestrictedToken(hToken, DISABLE_MAX_PRIVILEGE,
		                         countof(sidsToDisable), sidsToDisable,
		                         0, NULL, 0, NULL, &hTokenRest))
		{
			if (CreateProcessAsUserW(hTokenRest, lpApplicationName, lpCommandLine,
							 lpProcessAttributes, lpThreadAttributes,
							 bInheritHandles, dwCreationFlags, lpEnvironment,
							 lpCurrentDirectory, lpStartupInfo, lpProcessInformation))
			{
				lbRc = TRUE;
			}
			else
			{
				if (pdwLastError) *pdwLastError = GetLastError();
			}

			CloseHandle(hTokenRest); hTokenRest = NULL;
		}
		else
		{
			if (pdwLastError) *pdwLastError = GetLastError();
		}

		free(pAdmSid);
		CloseHandle(hToken); hToken = NULL;
	}
	else
	{
		if (pdwLastError) *pdwLastError = GetLastError();
	}

	return lbRc;
}



/// If process is started under LocalSystem account, it goes to SessionID=0
/// But we need interactive session to able to communicate with ConEmu window
BOOL CreateProcessInteractive(DWORD anSessionId, LPCWSTR lpApplicationName, LPWSTR lpCommandLine,
							 LPSECURITY_ATTRIBUTES lpProcessAttributes, LPSECURITY_ATTRIBUTES lpThreadAttributes,
							 BOOL bInheritHandles, DWORD dwCreationFlags, LPVOID lpEnvironment,
							 LPCWSTR lpCurrentDirectory, LPSTARTUPINFOW lpStartupInfo, LPPROCESS_INFORMATION lpProcessInformation,
							 LPDWORD pdwLastError /*= NULL*/)
{
	BOOL lbRc = FALSE;
	HANDLE hToken = NULL, hTokenRest = NULL;
	wchar_t szLog[128];
	DWORD nErrCode = 0, nCurSession = (DWORD)-1, nNewSessionId = (DWORD)-1, nRetSize = 0;


	#if 0
	// The switch "/GID=1234" contains expected PID which session we have to "obtain"
	// bug unfortunately ProcessIdToSessionId fails...
	DWORD nParentPID = 0;
	if (anSessionId == (DWORD)-1)
	{
		LPCWSTR pszLine = lpCommandLine;
		CEStr szArg;
		while (NextArg(&pszLine, szArg) == 0)
		{
			if (!szArg.IsSwitch(L"/GID="))
				continue;
			nParentPID = wcstoul(szArg.ms_Val+5, NULL, 10);
			break;
		}
		if (nParentPID == 0)
		{
			LogString(L"Switch `/GID=???` was not found, exiting");
			goto wrap;
		}
		msprintf(szLog, countof(szLog), L"Adopting SessionID from PID=%u", nParentPID);
		LogString(szLog);
	}
	#endif


	if (!OpenProcessToken(GetCurrentProcess(),
	                    TOKEN_DUPLICATE | TOKEN_ADJUST_SESSIONID | TOKEN_ASSIGN_PRIMARY | TOKEN_QUERY | TOKEN_EXECUTE,
	                    &hToken))
	{
		nErrCode = GetLastError();
		_wsprintf(szLog, SKIPCOUNT(szLog) L"Failed to OpenProcessToken, code=%u", nErrCode);
		LogString(szLog);
		if (pdwLastError) *pdwLastError = nErrCode;
		goto wrap;
	}

	if (!GetTokenInformation(hToken, TokenSessionId, &nCurSession, sizeof(nCurSession), &nRetSize))
		_wsprintf(szLog, SKIPCOUNT(szLog) L"Failed to query TokenSessionId, code=%u", GetLastError());
	else
		_wsprintf(szLog, SKIPCOUNT(szLog) L"Current TokenSessionId is %u", nCurSession);
	LogString(szLog);
	if (anSessionId == (DWORD)-1)
	{
		#if 1
		LogString(L"ProcessIdToSessionId is failing to obtain SessionID of GUI process, exiting");
		goto wrap;
		#else
		HANDLE hParent = OpenProcess(PROCESS_QUERY_INFORMATION, FALSE, nParentPID);
		BOOL bObtained = hParent && apiQuerySessionID(nParentPID, nNewSessionId);
		DWORD nErrCode = GetLastError();
		if (hParent) CloseHandle(hParent);
		if (!bObtained)
		{
			_wsprintf(szLog, SKIPCOUNT(szLog) L"ProcessIdToSessionId failed, code=%u", nErrCode);
			LogString(szLog);
			goto wrap;
		}
		if (!nNewSessionId)
			nNewSessionId = 1; // apiQuerySessionID returns 0 instead of correct 1
		#endif
	}
	else
	{
		nNewSessionId = anSessionId;
	}

	if (nNewSessionId != nCurSession)
	{
		if (!DuplicateTokenEx(hToken, MAXIMUM_ALLOWED, NULL, SecurityImpersonation, TokenPrimary, &hTokenRest))
		{
			_wsprintf(szLog, SKIPCOUNT(szLog) L"Failed to duplicate current token, code=%u", GetLastError());
			LogString(szLog);
			goto wrap;
		}
		else
		{
			CAdjustProcessToken tcbName;
			if (tcbName.Enable(1, SE_TCB_NAME) != 1)
			{
				_wsprintf(szLog, SKIPCOUNT(szLog) L"Failed to adjust SE_TCB_NAME privilege, code=%u", GetLastError());
				LogString(szLog);
			}

			if (!SetTokenInformation(hTokenRest, TokenSessionId, &nNewSessionId, sizeof(nNewSessionId)))
			{
				nErrCode = GetLastError();
				_wsprintf(szLog, SKIPCOUNT(szLog) L"Failed to adjust TokenSessionId to %u, code=%u", nNewSessionId, nErrCode);
				LogString(szLog);
				if (pdwLastError) *pdwLastError = nErrCode;
				goto wrap;
			}
			else
			{
				tcbName.Release();

				if (!GetTokenInformation(hTokenRest, TokenSessionId, &nCurSession, sizeof(nCurSession), &nRetSize))
					_wsprintf(szLog, SKIPCOUNT(szLog) L"Failed to query new TokenSessionId, code=%u", GetLastError());
				else
					_wsprintf(szLog, SKIPCOUNT(szLog) L"New TokenSessionId is %u", nCurSession);
				LogString(szLog);
			}
		}
	}

	// Now create the process
	if (hTokenRest)
	{
		lbRc = CreateProcessAsUserW(hTokenRest, lpApplicationName, lpCommandLine,
									lpProcessAttributes, lpThreadAttributes,
									bInheritHandles, dwCreationFlags, lpEnvironment,
									lpCurrentDirectory, lpStartupInfo, lpProcessInformation);
	}
	else
	{
		lbRc = CreateProcess(NULL, lpCommandLine,
							 lpProcessAttributes, lpThreadAttributes,
							 bInheritHandles, dwCreationFlags, lpEnvironment,
							 lpCurrentDirectory, lpStartupInfo, lpProcessInformation);
	}

	// Log errors
	if (!lbRc)
	{
		nErrCode = GetLastError();
		_wsprintf(szLog, SKIPCOUNT(szLog) L"%s failed, code=%u", hTokenRest ? L"CreateProcessAsUser" : L"CreateProcess", nErrCode);
		LogString(szLog);
		if (pdwLastError) *pdwLastError = nErrCode;
	}

	SafeCloseHandle(hTokenRest);
	SafeCloseHandle(hToken);

wrap:
	return lbRc;
}



/// The function starts new process using Windows Task Scheduler
/// This allows to run process under ‘LocalSystem’ account
BOOL CreateProcessSystem(DWORD anSessionId, LPWSTR lpCommandLine,
						 LPSECURITY_ATTRIBUTES lpProcessAttributes, LPSECURITY_ATTRIBUTES lpThreadAttributes,
						 BOOL bInheritHandles, DWORD dwCreationFlags, LPVOID lpEnvironment,
						 LPCWSTR lpCurrentDirectory, LPSTARTUPINFOW lpStartupInfo, LPPROCESS_INFORMATION lpProcessInformation,
						 LPDWORD pdwLastError /*= NULL*/)
{
	BOOL lbRc;

	lbRc = CreateProcessScheduled(true, anSessionId, lpCommandLine,
						   lpProcessAttributes, lpThreadAttributes,
						   bInheritHandles, dwCreationFlags, lpEnvironment,
						   lpCurrentDirectory, lpStartupInfo, lpProcessInformation,
						   pdwLastError);

	return lbRc;
}



/// The function starts new process using Windows Task Scheduler
/// This allows to run process ‘Demoted’, under ‘normal’ or non-elevated account
BOOL CreateProcessDemoted(LPWSTR lpCommandLine,
						  LPSECURITY_ATTRIBUTES lpProcessAttributes, LPSECURITY_ATTRIBUTES lpThreadAttributes,
						  BOOL bInheritHandles, DWORD dwCreationFlags, LPVOID lpEnvironment,
						  LPCWSTR lpCurrentDirectory, LPSTARTUPINFOW lpStartupInfo, LPPROCESS_INFORMATION lpProcessInformation,
						  LPDWORD pdwLastError /*= NULL*/)
{
	BOOL lbRc = FALSE;

	if (IsWin6())
	{
		lbRc = CreateProcessScheduled(false, 0/*doesn't matter*/, lpCommandLine,
						   lpProcessAttributes, lpThreadAttributes,
						   bInheritHandles, dwCreationFlags, lpEnvironment,
						   lpCurrentDirectory, lpStartupInfo, lpProcessInformation,
						   NULL);
	}

	// Obtain User account from Explorer.exe?
	if (!lbRc)
	{
		PROCESSENTRY32W explorer = {};
		HANDLE hToken = NULL, hTokenRun = NULL, hExplorer = NULL;
		if (GetProcessInfo(L"explorer.exe", &explorer))
		{
			hExplorer = OpenProcess(PROCESS_QUERY_INFORMATION, FALSE, explorer.th32ProcessID);
			if (hExplorer
				&& OpenProcessToken(hExplorer, TOKEN_DUPLICATE | TOKEN_ASSIGN_PRIMARY | TOKEN_QUERY | TOKEN_EXECUTE, &hToken))
			{
				lbRc = CreateProcessAsUserW(hToken, NULL, lpCommandLine,
							 lpProcessAttributes, lpThreadAttributes,
							 bInheritHandles, dwCreationFlags, lpEnvironment,
							 lpCurrentDirectory, lpStartupInfo, lpProcessInformation);
			}
		}
		SafeCloseHandle(hExplorer);
		SafeCloseHandle(hToken);
		SafeCloseHandle(hTokenRun);
	}

	// If all methods fails - try to execute "as is"?
	if (!lbRc)
	{

		lbRc = CreateProcess(NULL, lpCommandLine,
							 lpProcessAttributes, lpThreadAttributes,
							 bInheritHandles, dwCreationFlags, lpEnvironment,
							 lpCurrentDirectory, lpStartupInfo, lpProcessInformation);
		if (pdwLastError) *pdwLastError = GetLastError();
	}

	return lbRc;
}
