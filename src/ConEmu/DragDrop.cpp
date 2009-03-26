#include <shlobj.h>
#include "Header.h"

CDragDrop::CDragDrop(HWND hWnd)
{
	HRESULT hr = S_OK;
	m_hWnd=hWnd;
	m_lRefCount = 1;
	m_pfpi = NULL;
	mp_DataObject = NULL;
	mb_selfdrag = NULL;

	hr = RegisterDragDrop(hWnd, this);
	if (hr != S_OK)
		DisplayLastError(_T("Can't register Drop target"), hr);

	mp_DesktopID = NULL;
	hr = SHGetFolderLocation ( ghWnd, CSIDL_DESKTOP, NULL, 0, &mp_DesktopID );
	if (hr != S_OK)
		DisplayLastError(_T("Can't get desktop folder"), hr);
}

CDragDrop::~CDragDrop()
{
	if (m_pfpi) free(m_pfpi); m_pfpi=NULL;
	CoTaskMemFree(mp_DesktopID);
}


HANDLE CDragDrop::FileStart(BOOL abActive, BOOL abWide, LPVOID asFileName)
{
	if (!asFileName ||
		(abWide && ((wchar_t*)asFileName)[0] == 0) ||
		(!abWide && ((char*)asFileName)[0] == 0))
	{
		MBoxA(_T("Can't drop file! Filename is empty!"));
		return INVALID_HANDLE_VALUE;
	}

	wchar_t *pszFullName = NULL;
	wchar_t* pszNameW = (wchar_t*)asFileName;
	char* pszNameA = (char*)asFileName;
	int nSize = 0;

	if (abActive) nSize = wcslen(m_pfpi->pszActivePath)+1; else nSize = wcslen(m_pfpi->pszPassivePath)+1;
	int nPathLen = nSize;

	if (abWide) {
		wchar_t* pszSlash = wcsrchr(pszNameW, L'\\');
		if (pszSlash) pszNameW = pszSlash+1;
		if (!*pszNameW) {
			MBoxA(_T("Can't drop file! Filename is empty (W)!"));
			return INVALID_HANDLE_VALUE;
		}
		nSize += wcslen(pszNameW)+1;
	} else {
		char* pszSlash = strrchr(pszNameA, '\\');
		if (pszSlash) pszNameA = pszSlash+1;
		if (!*pszNameA) {
			MBoxA(_T("Can't drop file! Filename is empty (A)!"));
			return INVALID_HANDLE_VALUE;
		}
		nSize += strlen(pszNameA)+1;
	}
	pszFullName = (wchar_t*)calloc(nSize, 2);
	wcscpy(pszFullName, abActive ? m_pfpi->pszActivePath : m_pfpi->pszPassivePath);
	wcscat(pszFullName, L"\\");
	if (abWide) {
		wcscpy(pszFullName+nPathLen, pszNameW);
	} else {
		MultiByteToWideChar(CP_ACP, 0, pszNameA, -1, pszFullName+nPathLen, nSize - nPathLen);
	}

	// Путь готов, проверяем наличие файла?
	WIN32_FIND_DATAW fnd = {0};
	HANDLE hFind = FindFirstFile(pszFullName, &fnd);
	if (hFind != INVALID_HANDLE_VALUE) {
		FindClose(hFind);

		wchar_t* pszMsg = NULL;
		if (fnd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
			pszMsg = (wchar_t*)calloc(nSize + 100,2);
			wcscpy(pszMsg, L"Can't create file! Same name folder exists!\n");
			wcscat(pszMsg, pszFullName);
			MessageBox(ghWnd, pszMsg, L"ConEmu", MB_ICONSTOP);
			free(pszMsg);
			return INVALID_HANDLE_VALUE;
		} else {
			pszMsg = (wchar_t*)calloc(nSize + 255,2);
			LARGE_INTEGER liSize;
			liSize.LowPart = fnd.nFileSizeLow; liSize.HighPart = fnd.nFileSizeHigh;
			FILETIME ftl;
			FileTimeToLocalFileTime(&fnd.ftLastWriteTime, &ftl);
			SYSTEMTIME st; FileTimeToSystemTime(&ftl, &st);
			wsprintf(pszMsg, L"File already exists!\n\n%s\nSize: %I64i\nDate: %02i.%02i.%i %02i:%02i:%02i\n\nOverwrite?",
				pszFullName, liSize.QuadPart, st.wDay, st.wMonth, st.wYear, st.wHour, st.wMinute, st.wSecond);
			int nRc = MessageBox(ghWnd, pszMsg, L"ConEmu", MB_ICONEXCLAMATION|MB_YESNO);
			free(pszMsg);

			if (nRc != IDYES)
				return INVALID_HANDLE_VALUE;

			// сброс ReadOnly, при необходимости
			if (fnd.dwFileAttributes & (FILE_ATTRIBUTE_HIDDEN|FILE_ATTRIBUTE_SYSTEM|FILE_ATTRIBUTE_READONLY))
				SetFileAttributes(pszFullName, FILE_ATTRIBUTE_NORMAL);
		}
	}

	// Создаем файл
	hFind = CreateFile(pszFullName, GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hFind == INVALID_HANDLE_VALUE) {
		DWORD dwErr = GetLastError();
		wchar_t* pszMsg = (wchar_t*)calloc(nSize + 100,2);
		wcscpy(pszMsg, L"Can't create file!\n");
		wcscat(pszMsg, pszFullName);
		DisplayLastError(pszMsg, dwErr);
		free(pszMsg);
		return INVALID_HANDLE_VALUE;
	}

	mn_CurWritten = 0;
	return hFind;
}

HRESULT CDragDrop::FileWrite(HANDLE ahFile, DWORD anSize, LPVOID apData)
{
	DWORD dwWrite = 0;
	if (!WriteFile(ahFile, apData, anSize, &dwWrite, NULL) || dwWrite!=anSize) {
		DWORD dwLastError = GetLastError();
		if (!dwLastError) dwLastError = E_UNEXPECTED;
		return dwLastError;
	}

	mn_CurWritten += anSize;
	wchar_t KM = L'K';
	__int64 nPrepared = mn_CurWritten / 1000;
	if (nPrepared > 9999) {
		nPrepared /= 1000; KM = L'M';
		if (nPrepared > 9999) {
			nPrepared /= 1000; KM = L'G';
		}
	}

	wsprintf(temp, L"Copying %i of %i (%u%c)", (mn_CurFile+1), mn_AllFiles, (DWORD)nPrepared, KM);
	SetWindowText(ghWnd, temp);
	return S_OK;
}

HRESULT STDMETHODCALLTYPE CDragDrop::Drop (IDataObject * pDataObject,DWORD grfKeyState,POINTL pt,DWORD * pdwEffect)
{
	if (!gSet.isDropEnabled) {
		*pdwEffect = DROPEFFECT_NONE;
		return S_OK;
	}
	
	*pdwEffect = DROPEFFECT_NONE;
	if (S_OK != DragOver(grfKeyState, pt, pdwEffect) ||  *pdwEffect == DROPEFFECT_NONE) {
		return S_OK;
	}

	ScreenToClient(m_hWnd, (LPPOINT)&pt);
	pt.x/=gSet.LogFont.lfWidth;
	pt.y/=gSet.LogFont.lfHeight;

	BOOL lbActive = FALSE;
	if (m_pfpi==NULL) {
		//delete fop.pTo;
		return S_OK; //1;
	} else if (PtInRect(&(m_pfpi->ActiveRect), *(LPPOINT)&pt) && m_pfpi->pszActivePath[0]) 
	{
		lbActive = TRUE;

		if (mb_selfdrag) {
			*pdwEffect = DROPEFFECT_NONE;
			return S_OK; // Тащат внутри одной копии FAR с активной на активную, т.е. ничего не двигается
		}
			
		if (!*m_pfpi->pszActivePath)
			return S_OK;
		if (m_pfpi->pszActivePath[lstrlen(m_pfpi->pszActivePath)-1]==_T('\\'))
			m_pfpi->pszActivePath[lstrlen(m_pfpi->pszActivePath)-1] = 0;
	}
	else if (PtInRect(&(m_pfpi->PassiveRect), *(LPPOINT)&pt) && (m_pfpi->pszPassivePath[0] || mb_selfdrag))
	{
		lbActive = FALSE;

		if (!*m_pfpi->pszPassivePath) {
			if (!(*pdwEffect == DROPEFFECT_COPY || *pdwEffect == DROPEFFECT_MOVE))
				return 1;
		} else {
			if (m_pfpi->pszPassivePath[lstrlen(m_pfpi->pszPassivePath)-1]==_T('\\'))
				m_pfpi->pszPassivePath[lstrlen(m_pfpi->pszPassivePath)-1] = 0;
		}
	}
	else 
	{
		*pdwEffect = DROPEFFECT_NONE;
		return S_OK;
	}


	WCHAR szStr[MAX_PATH];
	STGMEDIUM stgMedium = { 0 };
	FORMATETC fmtetc = { CF_HDROP, 0, DVASPECT_CONTENT, -1, TYMED_HGLOBAL };
	HRESULT hr = pDataObject->GetData(&fmtetc, &stgMedium);
	HDROP hDrop = NULL;

	if (hr != S_OK || !stgMedium.hGlobal) {
		if (mb_selfdrag) {
			MBoxA(_T("Drag object does not contains known formats!"));
			return S_OK;
		}

		HANDLE hFile = NULL;
		#define BufferSize 0x10000
		BYTE cBuffer[BufferSize];
		DWORD dwRead = 0;
		BOOL lbWide = FALSE;

		// CF_HDROP в структуре отсутсвует!
		fmtetc.cfFormat = RegisterClipboardFormat(CFSTR_FILEDESCRIPTORW);
		if (S_OK == pDataObject->GetData(&fmtetc, &stgMedium)) {
			lbWide = TRUE;
			HGLOBAL hDesc = stgMedium.hGlobal;
			FILEGROUPDESCRIPTORW *pDesc = (FILEGROUPDESCRIPTORW*)GlobalLock(hDesc);
			fmtetc.cfFormat = RegisterClipboardFormat(CFSTR_FILECONTENTS);

			mn_AllFiles = pDesc->cItems;
			
			for (mn_CurFile = 0; mn_CurFile<mn_AllFiles; mn_CurFile++) {
				fmtetc.lindex = mn_CurFile;

				fmtetc.tymed = TYMED_ISTREAM; // Сначала пробуем IStream
				if (S_OK == pDataObject->GetData(&fmtetc, &stgMedium) || !stgMedium.pstm) {
					IStream* pFile = stgMedium.pstm;

					hFile = FileStart(lbActive, lbWide, pDesc->fgd[mn_CurFile].cFileName);
					if (hFile != INVALID_HANDLE_VALUE) {
						while ((hr = pFile->Read(cBuffer, BufferSize, &dwRead)) == S_OK && dwRead) {
							if (FileWrite(hFile, dwRead, cBuffer) != S_OK)
								break;
						}
						SafeCloseHandle(hFile);
						if (FAILED(hr))
							DisplayLastError(_T("Can't read medium!"), hr);
					}
					continue;
				}
				MBoxA(_T("Drag object does not contains known medium!"));
			}
			GlobalUnlock(hDesc);
			gConEmu.DnDstep(NULL);
			return S_OK;
		}
		// Outlook 2k передает ANSI!
		fmtetc.cfFormat = RegisterClipboardFormat(CFSTR_FILEDESCRIPTORA);
		if (S_OK == pDataObject->GetData(&fmtetc, &stgMedium)) {
			lbWide = FALSE;
			HGLOBAL hDesc = stgMedium.hGlobal;
			FILEGROUPDESCRIPTORA *pDesc = (FILEGROUPDESCRIPTORA*)GlobalLock(hDesc);
			fmtetc.cfFormat = RegisterClipboardFormat(CFSTR_FILECONTENTS);

			mn_AllFiles = pDesc->cItems;
			
			for (mn_CurFile = 0; mn_CurFile<mn_AllFiles; mn_CurFile++) {
				fmtetc.lindex = mn_CurFile;

				fmtetc.tymed = TYMED_ISTREAM; // Сначала пробуем IStream
				if (S_OK == pDataObject->GetData(&fmtetc, &stgMedium) || !stgMedium.pstm) {
					IStream* pFile = stgMedium.pstm;

					hFile = FileStart(lbActive, lbWide, pDesc->fgd[mn_CurFile].cFileName);
					if (hFile != INVALID_HANDLE_VALUE) {
						while ((hr = pFile->Read(cBuffer, BufferSize, &dwRead)) == S_OK && dwRead) {
							if (FileWrite(hFile, dwRead, cBuffer) != S_OK)
								break;
						}
						SafeCloseHandle(hFile);
						if (FAILED(hr))
							DisplayLastError(_T("Can't read medium!"), hr);
					}
					continue;
				}
				MBoxA(_T("Drag object does not contains known medium!"));
			}
			GlobalUnlock(hDesc);
			gConEmu.DnDstep(NULL);
			return S_OK;
		}

		MBoxA(_T("Drag object does not contains known formats!"));
		return S_OK;
	}

	hDrop = (HDROP)stgMedium.hGlobal;

	int iQuantity = DragQueryFile(hDrop,0xFFFFFFFF,NULL,NULL);
	ZeroMemory(szStr,sizeof(WCHAR)*MAX_PATH);

	gConEmu.DnDstep(_T("DnD: Drop starting"));

	{
		SHFILEOPSTRUCT fop  = {m_hWnd};
		
		ScreenToClient(m_hWnd, (LPPOINT)&pt);
		pt.x/=gSet.LogFont.lfWidth;
		pt.y/=gSet.LogFont.lfHeight;

		if (lbActive) 
		{
			if (mb_selfdrag) {
				*pdwEffect = DROPEFFECT_NONE;
				return S_OK; // Тащат внутри одной копии FAR с активной на активную, т.е. ничего не двигается
			}
				
			if (!*m_pfpi->pszActivePath)
				return S_OK;
			if (m_pfpi->pszActivePath[lstrlen(m_pfpi->pszActivePath)-1]==_T('\\'))
				m_pfpi->pszActivePath[lstrlen(m_pfpi->pszActivePath)-1] = 0;
			fop.pTo=new WCHAR[lstrlenW(m_pfpi->pszActivePath)+3];
			wsprintf((LPWSTR)fop.pTo, _T("%s\\\0\0"), m_pfpi->pszActivePath);
		}
		else
		{
			if (mb_selfdrag /*&& gSet.isDropEnabled==2*/ && (*pdwEffect == DROPEFFECT_COPY || *pdwEffect == DROPEFFECT_MOVE)) {
				wchar_t mcr[16];
				lstrcpyW(mcr, (*pdwEffect == DROPEFFECT_MOVE) ? L"F6" : L"F5");
				if (gSet.isDropEnabled==2)
					lstrcatW(mcr, L" Enter");

				gConEmu.PostMacro(mcr);

				return S_OK; // Тащим внутри ФАРа
			}
			
			if (!*m_pfpi->pszPassivePath) return 1;
			if (m_pfpi->pszPassivePath[lstrlen(m_pfpi->pszPassivePath)-1]==_T('\\'))
				m_pfpi->pszPassivePath[lstrlen(m_pfpi->pszPassivePath)-1] = 0;
			fop.pTo=new WCHAR[lstrlenW(m_pfpi->pszPassivePath)+3];
			wsprintf((LPWSTR)fop.pTo, _T("%s\\\0\0"), m_pfpi->pszPassivePath);
		}

		int nCount = MAX_DROP_PATH*iQuantity+iQuantity+1;
		fop.pFrom=new WCHAR[nCount];
		ZeroMemory((void*)fop.pFrom,sizeof(WCHAR)*nCount);

		WCHAR *curr=(WCHAR *)fop.pFrom;

		for ( int i = 0 ; i < iQuantity; i++ )
		{
			DragQueryFile(hDrop,i,curr,MAX_DROP_PATH);
			curr+=wcslen(curr)+1;
		}
		
		
		if (*pdwEffect == DROPEFFECT_MOVE)
			fop.wFunc=FO_MOVE;
		else if (*pdwEffect == DROPEFFECT_COPY)
			fop.wFunc=FO_COPY;
		else {
			LPCTSTR pszTitle = _tcsrchr(fop.pFrom, _T('\\'));
			if (pszTitle) pszTitle++; else pszTitle = fop.pFrom;

			int nLen = _tcslen(fop.pTo)+2+_tcslen(pszTitle)+4;
			TCHAR *szLnkPath = new TCHAR[nLen];
			_tcscpy(szLnkPath, fop.pTo);
			if (szLnkPath[_tcslen(szLnkPath)-1] != _T('\\'))
				_tcscat(szLnkPath, _T("\\"));
			_tcscat(szLnkPath, pszTitle);
			_tcscat(szLnkPath, _T(".lnk"));

			try {
				hr = CreateLink(fop.pFrom, szLnkPath, pszTitle);
			} catch(...) {
				hr = E_UNEXPECTED;
			}
			if (hr!=S_OK)
				DisplayLastError(_T("Can't create link!"), hr);
			
			if (fop.pTo) delete fop.pTo;
			if (fop.pFrom) delete fop.pFrom;
			delete szLnkPath;
			*pdwEffect = DROPEFFECT_NONE;
			return S_OK;
		}

		//fop.fFlags=FOF_SIMPLEPROGRESS; -- пусть полностью показывает
		gConEmu.DnDstep(_T("DnD: Shell operation starting"));
		
		// Собственно операция копирования/перемещения...
		try {
			hr = SHFileOperation(&fop);
		} catch(...) {
			hr = E_UNEXPECTED;
		}
		if (hr != S_OK) {
			if (hr == 7 || hr == ERROR_CANCELLED)
				MBoxA(_T("Shell operation was cancelled"));
			else
				DisplayLastError(_T("Shell operation failed"), hr);
		}
		
		if (fop.pTo) delete fop.pTo;
		if (fop.pFrom) delete fop.pFrom;
		
		gConEmu.DnDstep(NULL);
	}
	return S_OK; //1;
}

HRESULT STDMETHODCALLTYPE CDragDrop::DragOver(DWORD grfKeyState,POINTL pt,DWORD * pdwEffect)
{
	if (!gSet.isDropEnabled && !gConEmu.isDragging()) {
		gConEmu.DnDstep(_T("DnD: Drop disabled"));
		return -1;
	}

	//gConEmu.DnDstep(_T("DnD: DragOver starting"));

	ScreenToClient(m_hWnd, (LPPOINT)&pt);
	pt.x/=gSet.LogFont.lfWidth;
	pt.y/=gSet.LogFont.lfHeight;

	BOOL lbActive = FALSE, lbPassive = FALSE;
	if (m_pfpi==NULL)
		*pdwEffect = DROPEFFECT_NONE;
	else
	if (((lbActive = PtInRect(&(m_pfpi->ActiveRect), *(LPPOINT)&pt)) && m_pfpi->pszActivePath[0] && !mb_selfdrag) ||
		((lbPassive = PtInRect(&(m_pfpi->PassiveRect), *(LPPOINT)&pt)) && (m_pfpi->pszPassivePath[0] || mb_selfdrag)))
	{

		if (grfKeyState & MK_CONTROL)
			*pdwEffect = DROPEFFECT_COPY;
		else if (grfKeyState & MK_SHIFT)
			*pdwEffect = DROPEFFECT_MOVE;
		else if (grfKeyState & (MK_ALT | MK_RBUTTON))
			*pdwEffect = DROPEFFECT_LINK;
		else if (gConEmu.mouse.state & DRAG_R_STARTED)
			*pdwEffect = DROPEFFECT_LINK; // при Drop - правая кнопка уже отпущена
		else
			*pdwEffect = (gSet.isDefCopy) ? DROPEFFECT_COPY : DROPEFFECT_MOVE;

		if (*pdwEffect == DROPEFFECT_LINK && lbPassive && m_pfpi->pszPassivePath[0] == 0)
			*pdwEffect = DROPEFFECT_NONE;
	}
	else
		*pdwEffect = DROPEFFECT_NONE;

	//gConEmu.DnDstep(_T("DnD: DragOver ok"));
	return 0;
}

HRESULT STDMETHODCALLTYPE CDragDrop::DragEnter(IDataObject * pDataObject,DWORD grfKeyState,POINTL pt,DWORD * pdwEffect)
{
	mb_selfdrag = (pDataObject == mp_DataObject);

#ifdef _DEBUG
	if (!mb_selfdrag) {
		HRESULT hr = S_OK;
		IEnumFORMATETC *pEnum = NULL;
		FORMATETC fmt[20];
		STGMEDIUM stg[20];
		LPCWSTR psz[20];
		TCHAR szName[20][MAX_PATH];
		ULONG nCnt = sizeof(fmt)/sizeof(*fmt);
		UINT i;

			memset(fmt, 0, sizeof(fmt));
			memset(stg, 0, sizeof(stg));
			memset(psz, 0, sizeof(psz));

		hr = pDataObject->EnumFormatEtc(DATADIR_GET,&pEnum);
		if (hr==S_OK) {
			
			
			hr = pEnum->Next(nCnt, fmt, &nCnt);

			pEnum->Release();

			/*
			CFSTR_PREFERREDDROPEFFECT ?
			*/
			for (i=0; i<nCnt; i++) {
				szName[i][0] = 0;
				if (!GetClipboardFormatName(fmt[i].cfFormat, szName[i], MAX_PATH))
					wsprintf(szName[i], L"ClipFormatID=%i", fmt[i].cfFormat);
					
				stg[i].tymed = TYMED_HGLOBAL;
				hr = pDataObject->GetData(fmt+i, stg+i);
				if (hr == S_OK && stg[i].hGlobal)
					psz[i] = (LPCWSTR)GlobalLock(stg[i].hGlobal);
			}
			
			for (i=0; i<nCnt; i++) {
				if (psz[i] && stg[i].hGlobal)
					GlobalUnlock(stg[i].hGlobal);
			}
		}
		hr = S_OK;
	}
#endif

	if (gSet.isDropEnabled || gConEmu.isDragging())
	{
		if (!gConEmu.isDragging()) // при "своем" драге - информация уже получена
			RetrieveDragToInfo(pDataObject);
	} else {
		gConEmu.DnDstep(_T("DnD: Drop disabled"));
	}
	return 0;
}
	
void CDragDrop::RetrieveDragToInfo(IDataObject * pDataObject)
{
	CConEmuPipe pipe;

	gConEmu.DnDstep(_T("DnD: DragEnter starting"));

	if (pipe.Init(_T("CDragDrop::DragEnter")))
	{
		//PipeCmd cmd=DragTo;
		DWORD cbWritten=0;
		//WriteFile(pipe.hPipe, &cmd, sizeof(cmd), &cbWritten, NULL); 
		if (pipe.Execute(CMD_DRAGTO))
		{
			gConEmu.DnDstep(_T("DnD: Checking for plugin (1 sec)"));
			// Подождем немножко, проверим что плагин живой
			cbWritten = WaitForSingleObject(pipe.hEventAlive, CONEMUALIVETIMEOUT);
			if (cbWritten!=WAIT_OBJECT_0) {
				TCHAR szErr[MAX_PATH];
				wsprintf(szErr, _T("ConEmu plugin is not active!\r\nProcessID=%i"), pipe.nPID);
				MBoxA(szErr);
			} else {
				gConEmu.DnDstep(_T("DnD: Checking for plugin (10 sec)"));
				cbWritten = WaitForSingleObject(pipe.hEventReady, CONEMUREADYTIMEOUT);
				if (cbWritten!=WAIT_OBJECT_0) {
					TCHAR szErr[MAX_PATH];
					wsprintf(szErr, _T("Command waiting time exceeds!\r\nConEmu plugin is locked?\r\nProcessID=%i"), pipe.nPID);
					MBoxA(szErr);
				} else {
					DWORD cbBytesRead=0;
					int cbStructSize=0;
					if (m_pfpi) {free(m_pfpi); m_pfpi=NULL;}
					if (pipe.Read(&cbStructSize, sizeof(int), &cbBytesRead))
					{
						if (cbStructSize>sizeof(ForwardedPanelInfo)) {
							m_pfpi = (ForwardedPanelInfo*)calloc(cbStructSize, 1);

							pipe.Read(m_pfpi, cbStructSize, &cbBytesRead);

							m_pfpi->pszActivePath = (WCHAR*)(((char*)m_pfpi)+m_pfpi->ActivePathShift);
							m_pfpi->pszPassivePath = (WCHAR*)(((char*)m_pfpi)+m_pfpi->PassivePathShift);
						}
					}
				}
			}
		}
	}
	gConEmu.DnDstep(NULL);
}

HRESULT STDMETHODCALLTYPE CDragDrop::DragLeave(void)
{
	return 0;
}

// CreateLink - uses the Shell's IShellLink and IPersistFile interfaces 
//              to create and store a shortcut to the specified object. 
//
// Returns the result of calling the member functions of the interfaces. 
//
// Parameters:
// lpszPathObj  - address of a buffer containing the path of the object. 
// lpszPathLink - address of a buffer containing the path where the 
//                Shell link is to be stored. 
// lpszDesc     - address of a buffer containing the description of the 
//                Shell link. 

HRESULT CDragDrop::CreateLink(LPCTSTR lpszPathObj, LPCTSTR lpszPathLink, LPCTSTR lpszDesc) 
{ 
    HRESULT hres = S_OK; 
    IShellLink* psl = NULL;
 
    // Get a pointer to the IShellLink interface. 
    hres = CoCreateInstance(CLSID_ShellLink, NULL, CLSCTX_INPROC_SERVER, 
                            IID_IShellLink, (LPVOID*)&psl); 
    if (SUCCEEDED(hres)) 
    { 
        IPersistFile* ppf; 
 
        // Set the path to the shortcut target and add the description. 
        hres = psl->SetPath(lpszPathObj); 
        hres = psl->SetDescription(lpszDesc); 
 
        // Query IShellLink for the IPersistFile interface for saving the 
        // shortcut in persistent storage. 
        hres = psl->QueryInterface(IID_IPersistFile, (LPVOID*)&ppf); 
 
        if (SUCCEEDED(hres)) 
        { 
            //WCHAR wsz[MAX_PATH]; 
 
            // Ensure that the string is Unicode. 
            //MultiByteToWideChar(CP_ACP, 0, lpszPathLink, -1, wsz, MAX_PATH); 
			
            // Add code here to check return value from MultiByteWideChar 
            // for success.
 
            // Save the link by calling IPersistFile::Save. 
            hres = ppf->Save(lpszPathLink, TRUE); 
            ppf->Release(); 
        } 
        psl->Release(); 
    }
    return hres; 
}

void CDragDrop::Drag()
{
	if (!gSet.isDragEnabled /*|| isInDrag */|| gConEmu.isDragging()) {
		gConEmu.DnDstep(_T("DnD: Drag disabled"));
		return;
	}

	//gConEmu.isDragProcessed=true; // чтобы не сработало два раза на один драг
	gConEmu.mouse.state |= (gConEmu.mouse.state & DRAG_R_ALLOWED) ? DRAG_R_STARTED : DRAG_L_STARTED;

	MCHKHEAP

	CConEmuPipe pipe;
	if (pipe.Init(_T("CDragDrop::Drag")))
	{
		//isInDrag=true; // return в теле не допускать - нужно сбросить в конце

		//PipeCmd cmd=DragFrom;
		DWORD cbWritten=0;
		//WriteFile(pipe.hPipe, &cmd, sizeof(cmd), &cbWritten, NULL); 
		if (pipe.Execute(CMD_DRAGFROM))
		{
			gConEmu.DnDstep(_T("DnD: Checking for plugin (1 sec)"));
			// Подождем немножко, проверим что плагин живой
			cbWritten = WaitForSingleObject(pipe.hEventAlive, CONEMUALIVETIMEOUT);
			if (cbWritten!=WAIT_OBJECT_0) {
				TCHAR szErr[MAX_PATH];
				wsprintf(szErr, _T("ConEmu plugin is not active!\r\nProcessID=%i"), pipe.nPID);
				MBoxA(szErr);
			} else {
				gConEmu.DnDstep(_T("DnD: Waiting for result (10 sec)"));
				cbWritten = WaitForSingleObject(pipe.hEventReady, CONEMUREADYTIMEOUT);
				if (cbWritten!=WAIT_OBJECT_0) {
					TCHAR szErr[MAX_PATH];
					wsprintf(szErr, _T("Command waiting time exceeds!\r\nConEmu plugin is locked?\r\nProcessID=%i"), pipe.nPID);
					MBoxA(szErr);
				} else {
					gConEmu.DnDstep(_T("DnD: Recieving data"));
					DWORD cbBytesRead=0;
					int nWholeSize=0;
					pipe.Read(&nWholeSize, sizeof(nWholeSize), &cbBytesRead); 
					if (nWholeSize==0) { // защита смены формата
						if (!pipe.Read(&nWholeSize, sizeof(nWholeSize), &cbBytesRead))
							nWholeSize = 0;
					} else {
						nWholeSize=0;
					}

					MCHKHEAP

					if (nWholeSize<=0) {
						gConEmu.DnDstep(_T("DnD: Data is empty"));
					}
					else
					{
						gConEmu.DnDstep(_T("DnD: Recieving data..."));
						wchar_t *szDraggedPath=NULL; //ASCIIZZ
						szDraggedPath=new wchar_t[nWholeSize/*(MAX_PATH+1)*FilesCount*/+1];	
						ZeroMemory(szDraggedPath, /*((MAX_PATH+1)*FilesCount+1)*/(nWholeSize+1)*sizeof(wchar_t));
						wchar_t  *curr=szDraggedPath;
						UINT nFilesCount = 0;
						
						for (;;)
						{
							int nCurSize=0;
							if (!pipe.Read(&nCurSize, sizeof(nCurSize), &cbBytesRead)) break;
							if (nCurSize==0) break;

							pipe.Read(curr, sizeof(WCHAR)*nCurSize, &cbBytesRead); 

							curr+=wcslen(curr)+1;
							nFilesCount ++;
						}
						// Сразу закроем
						pipe.Close();
						
					//		int size = MakeDropWord(szDraggedPath);// Длинна null,null строки
					//		if (size <= 1) return;
						int size=(curr-szDraggedPath)*sizeof(wchar_t)+2;
						
						MCHKHEAP

						HGLOBAL file_nameW = NULL, file_PIDLs = NULL;
						if (nFilesCount==1 && mp_DesktopID)
						{
							HRESULT hr = S_OK;
							try {
								file_nameW = GlobalAlloc(GPTR, sizeof(WCHAR)*(lstrlenW(szDraggedPath)+1));
								lstrcpyW(((WCHAR*)file_nameW), szDraggedPath);

								//TODO: обработка ошибок hr!
								WCHAR* pszSlash = wcsrchr(szDraggedPath, L'\\');
								if (pszSlash) {
									// Сначала нужно получить PIDL для родительской папки
									SFGAOF tmp;
									LPITEMIDLIST pItem=NULL;
									DWORD nParentSize=0, nItemSize=0;
									IShellFolder *pDesktop=NULL;
									
									MCHKHEAP

									//hr = SHGetSpecialFolderLocation ( ghWnd, CSIDL_DESKTOP, &pParent );
									//hr = SHGetFolderLocation ( ghWnd, CSIDL_DESKTOP, NULL, 0, &pParent );
									//hr = SHGetFolderLocation ( ghWnd, CSIDL_DESKTOP, NULL, 0, &pItem );

									hr = SHGetDesktopFolder ( &pDesktop );

									if (hr == S_OK && pDesktop)
									{
										MCHKHEAP
										
										// Потом и для собственно файла/папки...
										ULONG nEaten=0;
										
										hr = pDesktop->ParseDisplayName(ghWnd, NULL, szDraggedPath, &nEaten, &pItem, &(tmp=0));
										pDesktop->Release(); pDesktop = NULL;

										if (hr == S_OK && pItem && mp_DesktopID)
										{
											SHITEMID *pCur = (SHITEMID*)mp_DesktopID;
											while (pCur->cb) {
												pCur = (SHITEMID*)(((LPBYTE)pCur) + pCur->cb);
											}
											nParentSize = ((LPBYTE)pCur) - ((LPBYTE)mp_DesktopID)+2;

											pCur = (SHITEMID*)pItem;
											while (pCur->cb) {
												pCur = (SHITEMID*)(((LPBYTE)pCur) + pCur->cb);
											}
											nItemSize = ((LPBYTE)pCur) - ((LPBYTE)pItem)+2;

											pCur = NULL;

											MCHKHEAP

											file_PIDLs = GlobalAlloc(GPTR, 3*sizeof(UINT)+nParentSize+nItemSize);
											if (file_PIDLs) {
												CIDA* pida = (CIDA*)file_PIDLs;
												pida->cidl = 1;
												pida->aoffset[0] = 3*sizeof(UINT);
												pida->aoffset[1] = pida->aoffset[0]+nParentSize;
												memmove((((LPBYTE)pida)+(pida)->aoffset[0]), mp_DesktopID, nParentSize);
												memmove((((LPBYTE)pida)+(pida)->aoffset[1]), pItem, nItemSize);
											}

											MCHKHEAP
										}
										if (pItem)
											CoTaskMemFree(pItem);
									}
								}
							} catch(...) {
								hr = -1;
							}
							if (hr == -1) {
								gConEmu.DnDstep(_T("DnD: Exception in shell"));
								return;
							}
						}
					    
						MCHKHEAP

						IDropSource *pDropSource = NULL;

						DROPFILES drop_struct = { sizeof(drop_struct), { 0, 0 }, 0, 1 };
					    
						HGLOBAL drop_data = GlobalAlloc(GPTR, size+sizeof(drop_struct));
						memcpy(drop_data, &drop_struct, sizeof(drop_struct));

						memcpy(((byte*)drop_data) + sizeof(drop_struct), szDraggedPath, size);
						//Maximus5 - а память освобождать кто будет?
						delete szDraggedPath; szDraggedPath=NULL;
						
						HGLOBAL drop_preferredeffect = GlobalAlloc(GPTR, sizeof(DWORD));
						if (gConEmu.mouse.state & DRAG_R_STARTED)
							*((DWORD*)drop_preferredeffect) = DROPEFFECT_LINK;
						else if (gSet.isDefCopy)
							*((DWORD*)drop_preferredeffect) = DROPEFFECT_COPY;
						else
							*((DWORD*)drop_preferredeffect) = DROPEFFECT_MOVE;

						MCHKHEAP
							
						HGLOBAL drag_loop = GlobalAlloc(GPTR, sizeof(DWORD));
						*((DWORD*)drag_loop) = 1;

						DWORD           dwEffect;
						DWORD           dwResult;
						FORMATETC       fmtetc[] = {
							{ CF_HDROP, 0, DVASPECT_CONTENT, -1, TYMED_HGLOBAL },
							{ RegisterClipboardFormat(CFSTR_PREFERREDDROPEFFECT), 0, DVASPECT_CONTENT, -1, TYMED_HGLOBAL },
							{ RegisterClipboardFormat(_T("InShellDragLoop")), 0, DVASPECT_CONTENT, -1, TYMED_HGLOBAL },
							// Эти должны идти последними
							{ RegisterClipboardFormat(CFSTR_SHELLIDLIST), 0, DVASPECT_CONTENT, -1, TYMED_HGLOBAL},
							{ RegisterClipboardFormat(_T("FileNameW")), 0, DVASPECT_CONTENT, -1, TYMED_HGLOBAL }
						};
						STGMEDIUM       stgmed[] = {
							{ TYMED_HGLOBAL, { (HBITMAP)drop_data }, 0 }, //чтобы не морочиться с последующими присвоениями
							{ TYMED_HGLOBAL, { (HBITMAP)drop_preferredeffect }, 0 },
							{ TYMED_HGLOBAL, { (HBITMAP)drag_loop }, 0 },
							// Эти должны идти последними
							{ TYMED_HGLOBAL, { (HBITMAP)file_PIDLs }, 0},
							{ TYMED_HGLOBAL, { (HBITMAP)file_nameW }, 0 }
						};
						//stgmed.hGlobal = drop_data ;
						//--stgmed.lpszFileName=szDraggedPath;
						CreateDropSource(&pDropSource);
						
						MCHKHEAP

						int nCount = sizeof(fmtetc)/sizeof(*fmtetc);
						if (!file_PIDLs) nCount -= 2;
						CreateDataObject(fmtetc, stgmed, nCount, &mp_DataObject) ;//   |   Посмотреть ниже... 
						
						// Разрешаем все
						DWORD dwAllowedEffects = (file_PIDLs ? DROPEFFECT_LINK : 0)|DROPEFFECT_COPY|DROPEFFECT_MOVE;
						
						MCHKHEAP
						

						//CFSTR_PREFERREDDROPEFFECT, FD_LINKUI 
						
						// Сразу получим информацию о путях панелей...
						RetrieveDragToInfo(mp_DataObject);
						
						//gConEmu.DnDstep(_T("DnD: Finally, DoDragDrop"));
						gConEmu.DnDstep(NULL);

						MCHKHEAP
						
						dwResult = DoDragDrop(mp_DataObject, pDropSource, dwAllowedEffects, &dwEffect);

						MCHKHEAP

						mp_DataObject->Release(); mp_DataObject = NULL;
						pDropSource->Release();		
						//#ifdef _DEBUG
						//DisplayLastError(_T("DoDragDrop returns"), dwResult);
						//#endif
						//isLBDown=false; -- а ReleaseCapture кто будет делать?
					}
				}
			}
		}
	}
	//isInDrag=false;
	//isDragProcessed=false; -- иначе при бросании в пассивную панель больших файлов дроп может вызваться еще раз???
}
