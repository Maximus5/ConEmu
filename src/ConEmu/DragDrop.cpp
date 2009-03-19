#include <shlobj.h>
#include "Header.h"

CDragDrop::CDragDrop(HWND hWnd)
{
	m_hWnd=hWnd;
	m_lRefCount = 1;
	m_pfpi = NULL;
	mp_DataObject = NULL;
	mb_selfdrag = NULL;
	RegisterDragDrop(hWnd, this);
}

CDragDrop::~CDragDrop()
{
	if (m_pfpi) free(m_pfpi); m_pfpi=NULL;
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

	WCHAR szStr[MAX_PATH];
	STGMEDIUM stgMedium;
	FORMATETC fmtetc = { CF_HDROP, 0, DVASPECT_CONTENT, -1, TYMED_HGLOBAL };
	HRESULT hr = pDataObject->GetData(&fmtetc, &stgMedium);

	HDROP hDrop = (HDROP)stgMedium.hGlobal;

	int iQuantity = DragQueryFile(hDrop,0xFFFFFFFF,NULL,NULL);
	ZeroMemory(szStr,sizeof(WCHAR)*MAX_PATH);

	gConEmu.DnDstep(_T("DnD: Drop starting"));

	{
		SHFILEOPSTRUCT fop = {m_hWnd};
		
		ScreenToClient(m_hWnd, (LPPOINT)&pt);
		pt.x/=gSet.LogFont.lfWidth;
		pt.y/=gSet.LogFont.lfHeight;

		if (m_pfpi==NULL) {
			//delete fop.pTo;
			return S_OK; //1;
		} else if (PtInRect(&(m_pfpi->ActiveRect), *(LPPOINT)&pt) && m_pfpi->pszActivePath[0]) 
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
		else if (PtInRect(&(m_pfpi->PassiveRect), *(LPPOINT)&pt) && m_pfpi->pszPassivePath[0])
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
		else 
		{
			*pdwEffect = DROPEFFECT_NONE;
			return S_OK;
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
			fop.wFunc=FO_MOVE;
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
		if (hr != S_OK)
			DisplayLastError(_T("Shell operation failed"), hr);
		
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

	if (m_pfpi==NULL)
		*pdwEffect = DROPEFFECT_NONE;
	else
	if ((PtInRect(&(m_pfpi->ActiveRect), *(LPPOINT)&pt) && m_pfpi->pszActivePath[0] && !mb_selfdrag) ||
		(PtInRect(&(m_pfpi->PassiveRect), *(LPPOINT)&pt) && m_pfpi->pszPassivePath[0]))
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
    IShellLink* psl; 
 
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
						
						HGLOBAL file_nameW = NULL, file_PIDLs = NULL;
						if (nFilesCount==1) {
							file_nameW = GlobalAlloc(GPTR, sizeof(WCHAR)*(lstrlenW(szDraggedPath)+1));
							lstrcpyW(((WCHAR*)file_nameW), szDraggedPath);

							//TODO: обработка ошибок hr!
							WCHAR* pszSlash = wcsrchr(szDraggedPath, L'\\');
							if (pszSlash) {
								// Сначала нужно получить PIDL для родительской папки
								*pszSlash = 0; //TODO: проверить, а для папок у нас слэша на конце нет?
								//SHParseDisplayName ?
								SFGAOF tmp;
								LPITEMIDLIST pParent=NULL, pItem=NULL;
								DWORD nParentSize=0, nItemSize=0;
								HRESULT hr = S_OK; //SHParseDisplayName(szDraggedPath, NULL, &pParent, 0, &tmp);
								IShellFolder *pDesktop=NULL; //, *pFolder=NULL;
								
								hr = SHGetSpecialFolderLocation ( ghWnd, CSIDL_DESKTOP, &pParent );

								hr = SHGetDesktopFolder ( &pDesktop );

								//hr = SHParseDisplayName(szDraggedPath, NULL, &pParent, 0, &tmp);

								//hr = pDesktop->BindToObject(pParent, NULL, IID_IShellFolder, (void**)&pFolder);
								//pDesktop->Release(); pDesktop = NULL;
								
								// Потом и для собственно файла/папки...
								*pszSlash = L'\\';
								//hr = SHParseDisplayName(pszSlash+1, NULL, &pItem, 0, &tmp);
								ULONG nEaten=0;
								//hr = pFolder->ParseDisplayName(ghWnd, NULL, pszSlash+1, &nEaten, &pItem, &(tmp=0));
								//hr = pFolder->ParseDisplayName(ghWnd, NULL, szDraggedPath, &nEaten, &pItem, &(tmp=0));
								hr = pDesktop->ParseDisplayName(ghWnd, NULL, szDraggedPath, &nEaten, &pItem, &(tmp=0));
								//pFolder->Release(); pFolder = NULL;
								pDesktop->Release(); pDesktop = NULL;

								*pszSlash = L'\\';

								SHITEMID *pCur = (SHITEMID*)pParent;
								while (pCur->cb) {
									pCur = (SHITEMID*)(((LPBYTE)pCur) + pCur->cb);
								}
								nParentSize = ((LPBYTE)pCur) - ((LPBYTE)pParent)+2;

								pCur = (SHITEMID*)pItem;
								while (pCur->cb) {
									pCur = (SHITEMID*)(((LPBYTE)pCur) + pCur->cb);
								}
								nItemSize = ((LPBYTE)pCur) - ((LPBYTE)pItem)+2;

								pCur = NULL;

								file_PIDLs = GlobalAlloc(GPTR, 3*sizeof(UINT)+nParentSize+nItemSize);
								CIDA* pida = (CIDA*)file_PIDLs;
								pida->cidl = 1;
								pida->aoffset[0] = 3*sizeof(UINT);
								pida->aoffset[1] = pida->aoffset[0]+nParentSize;
								memmove((((LPBYTE)pida)+(pida)->aoffset[0]), pParent, nParentSize);
								memmove((((LPBYTE)pida)+(pida)->aoffset[1]), pItem, nItemSize);

								LPMALLOC pMalloc=NULL;
								hr = SHGetMalloc(&pMalloc);
								if (pMalloc) {
									pMalloc->Free(pParent);
									pMalloc->Free(pItem);
									pMalloc->Release();
								}
							}
						}
					    
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
						
						int nCount = sizeof(fmtetc)/sizeof(*fmtetc);
						if (!file_PIDLs) nCount -= 2;
						CreateDataObject(fmtetc, stgmed, nCount, &mp_DataObject) ;//   |   Посмотреть ниже... 
						
						// Разрешаем все
						DWORD dwAllowedEffects = (file_PIDLs ? DROPEFFECT_LINK : 0)|DROPEFFECT_COPY|DROPEFFECT_MOVE;
						
						

						//CFSTR_PREFERREDDROPEFFECT, FD_LINKUI 
						
						// Сразу получим информацию о путях панелей...
						RetrieveDragToInfo(mp_DataObject);
						
						//gConEmu.DnDstep(_T("DnD: Finally, DoDragDrop"));
						gConEmu.DnDstep(NULL);
						
						dwResult = DoDragDrop(mp_DataObject, pDropSource, dwAllowedEffects, &dwEffect);
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
