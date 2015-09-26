
#pragma once

class CBackupPrivileges
{
protected:
	CAdjustProcessToken *mp_Token;
	int mn_TokenRef;
	//BOOL mb_TokenWarnedOnce;
	BOOL mb_RestoreAquired;
public:
	CBackupPrivileges()
	{
		mp_Token = NULL;
		//mb_TokenWarnedOnce = FALSE;
		mn_TokenRef = 0;
		mb_RestoreAquired = FALSE;
	};
	~CBackupPrivileges()
	{
		if (mp_Token)
		{
			mn_TokenRef = 0;
			BackupPrivilegesRelease();
		}
	};
public:
	BOOL BackupPrivilegesAquire(BOOL abRequireRestore)
	{
		if (mp_Token && abRequireRestore && !mb_RestoreAquired)
		{
			CAdjustProcessToken *p = mp_Token; mp_Token = NULL;
			delete p;
			mn_TokenRef = 0; mb_RestoreAquired = FALSE;
		}
		if (mn_TokenRef == 0 || !mp_Token)
		{
			mp_Token = new CAdjustProcessToken();
			LPCTSTR pszPriv[] = {SE_BACKUP_NAME, SE_RESTORE_NAME};
			int iRc = mp_Token->Enable(pszPriv, abRequireRestore ? 2 : 1);
			if (iRc != 1)
			{
				//if (!mb_TokenWarnedOnce && !bSkipPrivilegesDeniedMessage)
				//{
				//	mb_TokenWarnedOnce = TRUE;
				//	//MessageBox(NULL,
				//	//	L"Can't adjust current process privileges\nSE_BACKUP_NAME, SE_RESTORE_NAME",
				//	//	"ConEmu", 
				//	//	MB_STOPSIGN);
				//}
				delete mp_Token;
				mp_Token = NULL;
				mn_TokenRef = 0;
				mb_RestoreAquired = FALSE;
			}
			else
			{
				// Если удалось поднять хотя бы одну привилегию
				mn_TokenRef = 1;
				mb_RestoreAquired = abRequireRestore;
			}
		}
		else
		{
			mn_TokenRef++;
		}
		return (mn_TokenRef > 0 && mp_Token);
	};
	void BackupPrivilegesRelease()
	{
		if (mn_TokenRef > 0)
			mn_TokenRef --;

		if (mn_TokenRef <= 0)
		{
			mn_TokenRef = 0;
			if (mp_Token)
			{
				CAdjustProcessToken *p = mp_Token; mp_Token = NULL;
				delete p;
			}
		}
	};
};
