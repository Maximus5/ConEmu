#include "header.h"

CConEmuPipe::CConEmuPipe()
{
	for (int i=0; i<MAXCMDCOUNT; i++)
		hEventCmd[i] = NULL;
	hEventAlive=NULL;
	hEventReady=NULL;
	hMapping=NULL;
	dwMaxDataSize = 0;
	lpMap = NULL;
	lpCursor = NULL;
	sMapName[0] = 0;
	nPID = 0;
}

CConEmuPipe::~CConEmuPipe()
{
	for (int i=0; i<MAXCMDCOUNT; i++)
		SafeCloseHandle(hEventCmd[i]);
	SafeCloseHandle(hEventAlive);
	SafeCloseHandle(hEventReady);
	if (lpMap)
		UnmapViewOfFile(lpMap);
	SafeCloseHandle(hMapping);
}

#define CREATEEVENT(fmt,h) \
		wsprintf(szEventName, fmt, dwCurProcId ); \
		h = OpenEvent(EVENT_ALL_ACCESS, FALSE, szEventName); \
		if (h==INVALID_HANDLE_VALUE) h=NULL;

BOOL CConEmuPipe::Init()
{
    if (!gConEmu.isFar() && !gConEmu.mn_TopProcessID)
	    return FALSE;

	// Сформируем ИМЯ сразу, а то вдруг процесс переключится?
	DWORD dwCurProcId = gConEmu.mn_TopProcessID;
	nPID = dwCurProcId;
	wsprintf(sMapName, CONEMUMAPPING, dwCurProcId);
	
	WCHAR szEventName[64];

	CREATEEVENT(CONEMUDRAGFROM, hEventCmd[CMD_DRAGFROM]);
	CREATEEVENT(CONEMUDRAGTO, hEventCmd[CMD_DRAGTO]);
	CREATEEVENT(CONEMUEXIT, hEventCmd[CMD_EXIT]);
	CREATEEVENT(CONEMUALIVE, hEventAlive);
	CREATEEVENT(CONEMUREADY, hEventReady);


	if (!hEventAlive || !hEventReady || 
		!hEventCmd[CMD_DRAGFROM] || !hEventCmd[CMD_DRAGTO] || !hEventCmd[CMD_EXIT] )
	{
		MBoxA(_T("CreateEvent failed"));
		return FALSE;
	}

	return TRUE;
}

BOOL CConEmuPipe::Read(LPVOID pData, DWORD nSize, DWORD* nRead)
{
	if (nRead) *nRead = 0; // пока сбросим

	if (hMapping==NULL) {
		// Нужно инициализировать FileMapping
		
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
		dwMaxDataSize = *((DWORD*)lpMap);
		lpCursor = lpMap+4;
	}
	if (hMapping==INVALID_HANDLE_VALUE) {
		if (nRead) *nRead=0;
		return FALSE;
	}

	if ((lpCursor-lpMap+nSize)>dwMaxDataSize)
		nSize = dwMaxDataSize - (lpCursor-lpMap);

	if (nSize) {
		memmove(pData, lpCursor, nSize);
		lpCursor += nSize;
	}
	if (nRead) *nRead = nSize;

	return (nSize!=0);
}
