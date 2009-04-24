#include <Windows.h>
#include <stdio.h>
#include <Shlwapi.h>

//int main(int argc, char* argv[])
//int _cdecl wmain(int Argc, wchar_t *Argv[])
int main()
{
	wchar_t sComSpec[MAX_PATH];
	wchar_t* psFilePart;
	wchar_t* psCmdLine = GetCommandLineW();
	size_t nCmdLine = lstrlenW(psCmdLine);
	wchar_t* psNewCmd = NULL;
	
	if (!psCmdLine)
	{
		printf ("GetCommandLineW failed!\n" );
		return 100;
	}
	if ((psCmdLine = wcsstr(psCmdLine, L"/C")) == NULL)
	{
		printf ("/C argument not found!\n" );
		return 100;
	}
	psCmdLine += 2; // нас интересует все что ПОСЛЕ /C
	
	//TODO: Хорошо бы попробовать через переменную окружения "старый" процессор
	//TODO: находить. Например, перед заменой можно сохранять ComSpec в ComSpecC
	if (!SearchPathW(NULL, L"cmd.exe", NULL, MAX_PATH, sComSpec, &psFilePart))
	{
		printf ("Can't find cmd.exe!\n");
		return 101;
	}
	
	nCmdLine += lstrlenW(sComSpec)+10;
	
	psNewCmd = new wchar_t[nCmdLine];
	if (!psCmdLine)
	{
		printf ("Can't allocate %i wchars!\n", nCmdLine);
		return 102;
	}
	
	psNewCmd[0] = L'"';
	lstrcpyW( psNewCmd+1, sComSpec );
	lstrcatW( psNewCmd, L"\" /C " );
	lstrcatW( psNewCmd, psCmdLine );
	
	PROCESS_INFORMATION pi; memset(&pi, 0, sizeof(pi));
	STARTUPINFOW si; memset(&si, 0, sizeof(si));
	
	si.cb = sizeof(si);
	
	if (!CreateProcessW(NULL, psNewCmd, NULL,NULL, TRUE, NORMAL_PRIORITY_CLASS, NULL, NULL, &si, &pi))
	{
		printf ("Can't create process!\n" );
		delete psNewCmd;
		return 103;
	}
	 
	
	delete psNewCmd;
    return 0;
}
