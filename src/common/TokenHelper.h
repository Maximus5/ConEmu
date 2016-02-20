
/*
Copyright (c) 2013 Maximus5
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

#pragma once

class CAdjustProcessToken
{ //-V802
protected:
	HANDLE hToken;
	TOKEN_PRIVILEGES *tp; DWORD tp_size;
	TOKEN_PRIVILEGES *tp_prev; DWORD prev_size;
	BOOL   bEnabled;
	DWORD  dwErr;
public:
	CAdjustProcessToken()
	{
		hToken = NULL; tp_prev = NULL; prev_size = 0; tp = NULL; tp_size = 0;
		bEnabled = FALSE; dwErr = 0;
		if (!OpenProcessToken(GetCurrentProcess(), 
				//TOKEN_ASSIGN_PRIMARY|TOKEN_DUPLICATE|TOKEN_QUERY|TOKEN_ADJUST_DEFAULT,
				//TOKEN_DUPLICATE | TOKEN_ASSIGN_PRIMARY | 
				TOKEN_QUERY | TOKEN_ADJUST_PRIVILEGES,
				&hToken))
		{
			dwErr = GetLastError();
			hToken = NULL;
		}
	};
	void Release()
	{
		if (bEnabled && tp_prev)
		{
			if ( !AdjustTokenPrivileges(
				   hToken, 
				   FALSE, 
				   tp_prev, 
				   prev_size,
				   NULL, 
				   NULL) )
			{ 
				  dwErr = GetLastError();
			} 
		}
		bEnabled = FALSE;
		if (tp_prev)
		{
			free(tp_prev); tp_prev = NULL;
		}
		if (tp)
		{
			free(tp); tp = NULL;
		}
	};
	~CAdjustProcessToken()
	{
		Release();
		if (hToken)
		{
			CloseHandle(hToken); hToken = NULL;
		}
	};
	// Возвращает 1, если все привилегии успешно подняты
	//  0 - если частично?
	// -1 - при ошибке, или если hToken уже был инициализирован
	int Enable(LPCTSTR* lpszPrivilege /* SE_BACKUP_NAME */, int nCount)
	{
		LUID luid;
		
		if (!hToken)
			return -1;


		_ASSERTE(bEnabled == FALSE && tp == NULL);

		tp_size = sizeof(TOKEN_PRIVILEGES)+nCount*sizeof(LUID_AND_ATTRIBUTES);
		tp = (PTOKEN_PRIVILEGES)calloc(1,tp_size);
		tp->PrivilegeCount = 0;

		for (int i = 0; i < nCount; i++)
		{
			if ( !LookupPrivilegeValue( 
					NULL,            // lookup privilege on local system
					lpszPrivilege[i],   // privilege to lookup 
					&luid ) )        // receives LUID of privilege
			{
				//printf("LookupPrivilegeValue error: %u\n", GetLastError() ); 
				dwErr = GetLastError();
				return -1; 
			}

			tp->Privileges[i].Luid = luid;
			tp->Privileges[i].Attributes = SE_PRIVILEGE_ENABLED;
			tp->PrivilegeCount++;
		}
		

		// Enable the privilege or disable all privileges.
		
		prev_size = sizeof(TOKEN_PRIVILEGES)+80*sizeof(LUID_AND_ATTRIBUTES);
		tp_prev = (PTOKEN_PRIVILEGES)calloc(1,prev_size);
		tp_prev->PrivilegeCount = 80;

		if ( !AdjustTokenPrivileges(
			   hToken, 
			   FALSE, 
			   tp, 
			   tp_size,
			   tp_prev, 
			   &prev_size) )
		{ 
			  //printf("AdjustTokenPrivileges error: %u\n", GetLastError() ); 
			  dwErr = GetLastError();
			  return -1; 
		}

		bEnabled = TRUE;

		dwErr = GetLastError();
		if (dwErr == ERROR_NOT_ALL_ASSIGNED)
		{
			  //printf("The token does not have the specified privilege. \n");
			  return 0;
		}

		return 1;
	};
	int Enable(int nCount, ... /*LPCTSTR lpszPrivilege = SE_BACKUP_NAME, ...*/ )
	{
		va_list argptr;
		va_start(argptr, nCount);
		LPCTSTR* pszList = (LPCTSTR*)calloc(nCount, sizeof(LPCTSTR));
		
		for (int i = 0; i < nCount; i++)
		{
			pszList[i] = va_arg( argptr, LPCTSTR );
		}
		
		int lRc = Enable(pszList, nCount);
		
		free(pszList);
		va_end(argptr);
		return lRc;
	};
	DWORD LastErrorCode()
	{
		return dwErr;
	};
};
