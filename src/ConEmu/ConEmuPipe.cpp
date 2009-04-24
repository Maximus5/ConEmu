#include "header.h"

HANDLE CConEmuPipe::hMapping=NULL;
TCHAR  CConEmuPipe::sLastOp[64] = _T("");

CConEmuPipe::CConEmuPipe()
{
	MCHKHEAP
	for (int i=0; i<MAXCMDCOUNT; i++)
		hEventCmd[i] = NULL;
	hEventAlive=NULL;
	hEventReady=NULL;
	//hMapping=NULL;
	dwMaxDataSize = 0;
	lpMap = NULL;
	lpCursor = NULL;
	sMapName[0] = 0;
	nPID = 0;
	MCHKHEAP
}

CConEmuPipe::~CConEmuPipe()
{
	Close();
}

void CConEmuPipe::Close()
{
	MCHKHEAP
	for (int i=0; i<MAXCMDCOUNT; i++)
		SafeCloseHandle(hEventCmd[i]);
	SafeCloseHandle(hEventAlive);
	SafeCloseHandle(hEventReady);
	if (lpMap)
		UnmapViewOfFile(lpMap);
	MCHKHEAP
	if (hMapping) {
		SafeCloseHandle(hMapping);
		ms_LastOp[0] = 0;
		sLastOp[0] = 0;
	}
	MCHKHEAP

	OutputDebugString(_T("Pipe::Close()\n"));
}

#define CREATEEVENT(fmt,h) \
		wsprintf(szEventName, fmt, dwCurProcId ); \
		h = OpenEvent(EVENT_ALL_ACCESS, FALSE, szEventName); \
		if (h==INVALID_HANDLE_VALUE) h=NULL;

//#pragma message("warning: добавить аргумент abSilent, вдруг плагин еще не загружен?")
BOOL CConEmuPipe::Init(LPCTSTR asOp, BOOL abSilent)
{
	MCHKHEAP

	#ifdef _DEBUG
	if (ghWnd) {
		DWORD dwPID=0, dwTID = GetWindowThreadProcessId(ghWnd, &dwPID);
		DWORD dwCur = GetCurrentThreadId();
		_ASSERT(dwCur == dwTID);
	}
	#endif

	if (hMapping) {
		if (!abSilent) {
			TCHAR szMsg[0x400];
			wsprintf(szMsg, _T("Can't start '%s'\nLast operation '%s' was not finished!"), asOp, sLastOp);
			MBoxA(szMsg);
		}
		return FALSE;
	}

    if (!gConEmu.isFar() && !gConEmu.mn_TopProcessID) {
	    gConEmu.DnDstep(_T("Pipe: FAR not active"));
		OutputDebugString(_T("Pipe::FAR not active\n"));
	    return FALSE;
	}
	
	MCHKHEAP
	
	lstrcpyn(ms_LastOp, asOp, 64);

	// Сформируем ИМЯ сразу, а то вдруг процесс переключится?
	DWORD dwCurProcId = gConEmu.mn_TopProcessID;
	nPID = dwCurProcId;
	wsprintf(sMapName, CONEMUMAPPING, dwCurProcId);
	
	WCHAR szEventName[64];

	CREATEEVENT(CONEMUALIVE, hEventAlive);
	CREATEEVENT(CONEMUREADY, hEventReady);
	if (!hEventAlive || !hEventReady) {
		OutputDebugString(_T("ConEmu plugins is not installed\n"));
		if (!abSilent) {
			WCHAR szMsg[128];
			swprintf(szMsg, _T("ConEmu plugin was not installed!\r\nFAR PID: %i"), nPID);
			MBoxA(szMsg);
		}
		return FALSE;
	}

	CREATEEVENT(CONEMUDRAGFROM, hEventCmd[CMD_DRAGFROM]);
	CREATEEVENT(CONEMUDRAGTO, hEventCmd[CMD_DRAGTO]);
	CREATEEVENT(CONEMUREQTABS, hEventCmd[CMD_REQTABS]);
	CREATEEVENT(CONEMUSETWINDOW, hEventCmd[CMD_SETWINDOW]);
	CREATEEVENT(CONEMUPOSTMACRO, hEventCmd[CMD_POSTMACRO]);
	CREATEEVENT(CONEMUDEFFONT, hEventCmd[CMD_DEFFONT]);
	CREATEEVENT(CONEMULANGCHANGE, hEventCmd[CMD_LANGCHANGE]);
	CREATEEVENT(CONEMUEXIT, hEventCmd[CMD_EXIT]);

	MCHKHEAP

	if (!hEventAlive || !hEventReady || 
		!hEventCmd[CMD_DRAGFROM] || !hEventCmd[CMD_DRAGTO] || 
		!hEventCmd[CMD_REQTABS] || !hEventCmd[CMD_SETWINDOW] ||
		!hEventCmd[CMD_POSTMACRO] || !hEventCmd[CMD_DEFFONT] || 
		!hEventCmd[CMD_LANGCHANGE] || !hEventCmd[CMD_EXIT] )
	{
		OutputDebugString(_T("Create event failed\n"));
		if (!abSilent)
			MBoxA(_T("CreateEvent failed"));
		return FALSE;
	}

	OutputDebugString(_T("Pipe:Initialized\n"));

	return TRUE;
}

// Не интересуется результатом команды!
BOOL CConEmuPipe::Execute(int nCmd)
{
	MCHKHEAP
	if (nCmd<0 || nCmd>MAXCMDCOUNT) {
		TCHAR szError[128];
		swprintf(szError, _T("Invalid command id (%i)!"), nCmd);
		MBoxA(szError);
		return FALSE;
	}
	if (!hEventCmd[nCmd]) {
		TCHAR szError[128];
		swprintf(szError, _T("Command %i was not created by plugin!"), nCmd);
		MBoxA(szError);
		return FALSE;
	}

	WCHAR szMsg[64]; swprintf(szMsg, _T("Pipe:Execute(%i)\n"), nCmd);
	OutputDebugString(szMsg);

	ResetEvent(hEventAlive);
	ResetEvent(hEventReady);
	SetEvent(hEventCmd[nCmd]);
	MCHKHEAP
	return TRUE;
}

LPBYTE CConEmuPipe::GetPtr()
{
	return lpCursor;
}

BOOL CConEmuPipe::Read(LPVOID pData, DWORD nSize, DWORD* nRead)
{
	MCHKHEAP
	if (nRead) *nRead = 0; // пока сбросим

	if (hMapping==NULL) {
		// Нужно инициализировать FileMapping
		OutputDebugString(_T("Starts reading result\n"));
		
		lstrcpy(sLastOp, ms_LastOp);
		
		
		MCHKHEAP
		hMapping = OpenFileMapping(FILE_MAP_ALL_ACCESS, // Read/write permission. 
			FALSE,                             // Do not inherit the name
			sMapName);            // of the mapping object. 
		if (hMapping == NULL) {
			hMapping = INVALID_HANDLE_VALUE;
			// Нету такого
			MessageBox(ghWnd, _T("Can't open filemapping"), sMapName, MB_ICONSTOP);
			return FALSE;
		}
		lpMap = (LPBYTE)MapViewOfFile(hMapping, FILE_MAP_ALL_ACCESS, 0,0,0);
		if (lpMap==NULL) {
			CloseHandle(hMapping); hMapping = INVALID_HANDLE_VALUE;
			MessageBox(ghWnd, _T("Can't map view of file"), sMapName, MB_ICONSTOP);
			return FALSE;
		}
		MCHKHEAP
		dwMaxDataSize = *((DWORD*)lpMap);
		lpCursor = lpMap+4;
	}
	if (hMapping==INVALID_HANDLE_VALUE || !hMapping) {
		if (nRead) *nRead=0;
		return FALSE;
	}

	MCHKHEAP
	if (!dwMaxDataSize)
		nSize = 0; else
	if ((lpCursor-lpMap+nSize)>dwMaxDataSize)
		nSize = dwMaxDataSize - (lpCursor-lpMap);

	MCHKHEAP
	if (nSize) {
		memmove(pData, lpCursor, nSize);
		MCHKHEAP
		lpCursor += nSize;
	}
	if (nRead) *nRead = nSize;

	return (nSize!=0);
}
