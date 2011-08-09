
/*
Copyright (c) 2009-2011 Maximus5
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


#define SHOWDEBUGSTR

#include <shlobj.h>
#include "Header.h"
#include "ScreenDump.h"
#include "DragDrop.h"
#include "Options.h"
#include "ConEmu.h"
#include "VirtualConsole.h"
#include "RealConsole.h"

//#define MAX_OVERLAY_WIDTH    300
//#define MAX_OVERLAY_HEIGHT   300
//#define OVERLAY_TEXT_SHIFT   (gpSet->isDragShowIcons ? 18 : 0)
//#define OVERLAY_COLUMN_SHIFT 5

#define DEBUGSTROVL(s) //DEBUGSTR(s)
#define DEBUGSTRBACK(s) //DEBUGSTR(s)


CDragDrop::CDragDrop()
{
	//m_hWnd = hWnd;
	mb_DragDropRegistered = FALSE;
	m_lRefCount = 1;
	m_pfpi = NULL;
	mp_DataObject = NULL;
	mb_selfdrag = NULL;
	m_DesktopID.mkid.cb = 0;
	mh_Overlapped = NULL; mh_BitsDC = NULL; mh_BitsBMP = mh_BitsBMP_Old = NULL;
	mp_Bits = NULL;
	mn_AllFiles = 0; mn_CurWritten = 0; mn_CurFile = 0;
	mb_DragWithinNow = FALSE;
	mn_ExtractIconsTID = 0;
	mh_ExtractIcons = NULL;
	//mh_DragThread = NULL; mn_DragThreadId = 0;
	InitializeCriticalSection(&m_CrThreads);
}

//BOOL CDragDrop::Init()
//{
//	BOOL lbRc = FALSE;
//	HRESULT hr = S_OK;
//
//	//wchar_t szDesktopPath[MAX_PATH+1];
//	//hr = SHGetFolderPath ( ghWnd, CSIDL_DESKTOP, NULL, SHGFP_TYPE_CURRENT, szDesktopPath );
//	//if (hr == S_OK) {
//	//	DWORD dwAttrs = GetFileAttributes(szDesktopPath);
//	//	if (dwAttrs == (DWORD)-1) {
//	//		// Папка отсутсвует
//	//
//	//	} else if ((dwAttrs & FILE_ATTRIBUTE_DIRECTORY) == FILE_ATTRIBUTE_DIRECTORY) {
//	//		// mp_DesktopID
//	//		// [out] The address of a pointer to an item identifier list structure that specifies
//	//		// the folder's location relative to the root of the namespace (the desktop). The
//	//		// ppidl parameter is set to NULL on failure. The calling application is responsible for
//	//		// freeing this resource by calling ILFree.
//	//		hr = SHGetFolderLocation ( ghWnd, CSIDL_DESKTOP, NULL, 0, &mp_DesktopID );
//	//		if (hr != S_OK) {
//	//			DisplayLastError(L"Can't get desktop folder", hr);
//	//		}
//	//
//	//	}
//	//}
//	//if (mp_DesktopID == NULL) {
//	//	wchar_t szError[MAX_PATH*2];
//	//	wcscpy(szError, L"Desktop folder not found:\n");
//	//	wcscat(szError, szDesktopPath);
//	//	wcscat(szError, L"\nSome Drag-n-Drop features will be disabled!");
//	//	DisplayLastError(szError);
//	//}
//
//	hr = RegisterDragDrop(ghWnd, this);
//	if (hr != S_OK) {
//		DisplayLastError(L"Can't register Drop target", hr);
//		goto wrap;
//	} else {
//		mb_DragDropRegistered = TRUE;
//	}
//
//	lbRc = TRUE;
//wrap:
//	return lbRc;
//}

CDragDrop::~CDragDrop()
{
	DestroyDragImageBits();
	DestroyDragImageWindow();

	if (mb_DragDropRegistered && ghWnd)
	{
		mb_DragDropRegistered = FALSE;
		RevokeDragDrop(ghWnd);
	}

	EnterCriticalSection(&m_CrThreads);
	BOOL lbEmpty = m_OpThread.empty() && !InDragDrop();
	LeaveCriticalSection(&m_CrThreads);

	if (!lbEmpty)
	{
		if (MessageBox(ghWnd, L"Not all shell operations was finished!\r\nDo You want to terminate them (it's may be harmful)?",
		              gpConEmu->ms_ConEmuVer, MB_YESNO|MB_ICONEXCLAMATION) == IDYES)
		{
			// Terminate all shell (copying) threads
			EnterCriticalSection(&m_CrThreads);
			std::vector<ThInfo>::iterator iter = m_OpThread.begin();

			while(iter != m_OpThread.end())
			{
				HANDLE hThread = iter->hThread;
				TerminateThread(hThread, 100);
				CloseHandle(hThread);
				iter = m_OpThread.erase(iter);
			}

			LeaveCriticalSection(&m_CrThreads);
		}
		else
		{
			// Будем ждать до посинения
			BOOL lbActive = TRUE;

			while(TRUE)
			{
				Sleep(100);
				EnterCriticalSection(&m_CrThreads);
				lbActive = (!m_OpThread.empty()) || InDragDrop();
				LeaveCriticalSection(&m_CrThreads);
			}
		}
	}
	else
	{
		// незаконченных нитей нет
		// -- LeaveCriticalSection(&m_CrThreads); -- 101229 секция уже закрыта
	}

	// Завершение всех нитей драга
	TerminateDrag();

	// Если драг нормально завершить не удалось
	//if (mh_DragThread && mn_DragThreadId) {
	//	TerminateThread(mh_DragThread, 100);
	//}
	//SafeCloseHandle(mh_DragThread); mn_DragThreadId = 0;

	if (m_pfpi) free(m_pfpi); m_pfpi=NULL;

	//if (mp_DesktopID) { CoTaskMemFree(mp_DesktopID); mp_DesktopID = NULL; }
	DeleteCriticalSection(&m_CrThreads);
}

void CDragDrop::Drag(BOOL abClickNeed, COORD crMouseDC)
{
	//BOOL lbNeedLBtnUp = !abClickNeed;
	DWORD dwAllowedEffects = 0, dwResult = 0, dwEffect = 0;;
	IDropSource *pDropSource = NULL;
#ifdef PRESIST_OVL
	_ASSERTE(mh_Overlapped!=NULL);
#endif

	//CConEmuPipe pipe(gpConEmu->GetFarPID(), CONEMUREADYTIMEOUT);

	if (!gpSet->isDragEnabled /*|| isInDrag */|| gpConEmu->isDragging())
	{
		gpConEmu->DebugStep(gpSet->isDragEnabled
		                    ? _T("DnD: Already in Drag loop") : _T("DnD: Drag disabled"),
		                    gpSet->isDragEnabled);
		goto wrap;
	}

	//if (mh_DragThread) {
	//	gpConEmu->DebugStep(L"DnD: Drag thread already created!");
	//	goto wrap;
	//}
	_ASSERTE(!gpConEmu->isPictureView());
	//gpConEmu->isDragProcessed=true; // чтобы не сработало два раза на один драг
	//gpConEmu->mouse.state |= (gpConEmu->mouse.state & DRAG_R_ALLOWED) ? DRAG_R_STARTED : DRAG_L_STARTED;
	MCHKHEAP
	//crMouseDC = gpConEmu->ActiveCon()->RCon()->ScreenToBuffer(crMouseDC);
	//if (m_pfpi) {free(m_pfpi); m_pfpi=NULL;}
	//if (pipe.Init(_T("CDragDrop::Drag")))
	//{
	//	//DWORD cbWritten=0;
	//	struct {
	//		BOOL  bClickNeed;
	//		COORD crMouse;
	//	} DragArg;
	//	DragArg.bClickNeed = abClickNeed;
	//	DragArg.crMouse = gpConEmu->ActiveCon()->ClientToConsole(crMouseDC.X, crMouseDC.Y);
	//	if (pipe.Execute(CMD_DRAGFROM, &DragArg, sizeof(DragArg)))
	//	{
	//		lbNeedLBtnUp = FALSE; // мышка уже обработана в плагине
	//				gpConEmu->DebugStep(_T("DnD: Recieving data"));
	//				DWORD cbBytesRead=0;
	//				int nWholeSize=0;
	//				pipe.Read(&nWholeSize, sizeof(nWholeSize), &cbBytesRead);
	//				if (nWholeSize==0) { // защита смены формата
	//					if (!pipe.Read(&nWholeSize, sizeof(nWholeSize), &cbBytesRead))
	//						nWholeSize = 0;
	//				} else {
	//					nWholeSize=0;
	//				}
	//				MCHKHEAP
	//				if (nWholeSize<=0) {
	//					gpConEmu->DebugStep(_T("DnD: Data is empty"));
	//				}
	//				else
	//				{
	//					gpConEmu->DebugStep(_T("DnD: Recieving data..."));
	//					wchar_t *szDraggedPath=NULL; //ASCIIZZ
	//					szDraggedPath=new wchar_t[nWholeSize/*(MAX_PATH+1)*FilesCount*/+1];
	//					ZeroMemory(szDraggedPath, /*((MAX_PATH+1)*FilesCount+1)*/(nWholeSize+1)*sizeof(wchar_t));
	//					wchar_t  *curr=szDraggedPath;
	//					UINT nFilesCount = 0;
	//
	//					for (;;)
	//					{
	//						int nCurSize=0;
	//						if (!pipe.Read(&nCurSize, sizeof(nCurSize), &cbBytesRead)) break;
	//						if (nCurSize==0) break;
	//						pipe.Read(curr, sizeof(WCHAR)*nCurSize, &cbBytesRead);
	//						_ASSERTE(*curr);
	//						curr+=lstrlen(curr)+1;
	//						nFilesCount ++;
	//					}
	//					int cbStructSize=0;
	//					if (m_pfpi) {free(m_pfpi); m_pfpi=NULL;}
	//					if (pipe.Read(&cbStructSize, sizeof(int), &cbBytesRead))
	//					{
	//						if ((DWORD)cbStructSize>sizeof(ForwardedPanelInfo)) {
	//							m_pfpi = (ForwardedPanelInfo*)calloc(cbStructSize, 1);
	//							pipe.Read(m_pfpi, cbStructSize, &cbBytesRead);
	//							m_pfpi->pszActivePath = (WCHAR*)(((char*)m_pfpi)+m_pfpi->ActivePathShift);
	//							//Slash на конце нам не нужен
	//							int nPathLen = lstrlenW( m_pfpi->pszActivePath );
	//							if (nPathLen>0 && m_pfpi->pszActivePath[nPathLen-1]==_T('\\'))
	//								m_pfpi->pszActivePath[nPathLen-1] = 0;
	//
	//							m_pfpi->pszPassivePath = (WCHAR*)(((char*)m_pfpi)+m_pfpi->PassivePathShift);
	//							//Slash на конце нам не нужен
	//							nPathLen = lstrlenW( m_pfpi->pszPassivePath );
	//							if (nPathLen>0 && m_pfpi->pszPassivePath[nPathLen-1]==_T('\\'))
	//								m_pfpi->pszPassivePath[nPathLen-1] = 0;
	//						}
	//					}
	//					// Сразу закроем
	//					pipe.Close();
	//
	//					int size=(curr-szDraggedPath)*sizeof(wchar_t)+2;
	//
	//					MCHKHEAP
	//					HGLOBAL file_nameW = NULL, file_PIDLs = NULL;
	//					if (nFilesCount==1 /*&& mp_DesktopID*/)
	//					{
	//						HRESULT hr = S_OK;
	//						try {
	//							file_nameW = GlobalAlloc(GPTR, sizeof(WCHAR)*(lstrlenW(szDraggedPath)+1));
	//							lstrcpyW(((WCHAR*)file_nameW), szDraggedPath);
	//							//TODO: обработка ошибок hr!
	//							WCHAR* pszSlash = wcsrchr(szDraggedPath, L'\\');
	//							if (pszSlash) {
	//								// Сначала нужно получить PIDL для родительской папки
	//								SFGAOF tmp;
	//								LPITEMIDLIST pItem=NULL;
	//								DWORD nParentSize=0, nItemSize=0;
	//								IShellFolder *pDesktop=NULL;
	//
	//								MCHKHEAP
	//								//hr = SHGetSpecialFolderLocation ( ghWnd, CSIDL_DESKTOP, &pParent );
	//								//hr = SHGetFolderLocation ( ghWnd, CSIDL_DESKTOP, NULL, 0, &pParent );
	//								//hr = SHGetFolderLocation ( ghWnd, CSIDL_DESKTOP, NULL, 0, &pItem );
	//								hr = SHGetDesktopFolder ( &pDesktop );
	//								if (hr == S_OK && pDesktop)
	//								{
	//									MCHKHEAP
	//
	//									// Потом и для собственно файла/папки...
	//									ULONG nEaten=0;
	//
	//									hr = pDesktop->ParseDisplayName(ghWnd, NULL, szDraggedPath, &nEaten, &pItem, &(tmp=0));
	//									pDesktop->Release(); pDesktop = NULL;
	//									if (hr == S_OK && pItem /*&& mp_DesktopID*/)
	//									{
	//										SHITEMID *pCur = NULL; //(SHITEMID*)&m_DesktopID; //mp_DesktopID;
	//										//while (pCur->cb) {
	//										//	pCur = (SHITEMID*)(((LPBYTE)pCur) + pCur->cb);
	//										//}
	//										nParentSize = 2; //((LPBYTE)pCur) - ((LPBYTE)mp_DesktopID)+2;
	//										pCur = (SHITEMID*)pItem;
	//										while (pCur->cb) {
	//											pCur = (SHITEMID*)(((LPBYTE)pCur) + pCur->cb);
	//										}
	//										nItemSize = ((LPBYTE)pCur) - ((LPBYTE)pItem)+2;
	//										pCur = NULL;
	//										MCHKHEAP
	//										file_PIDLs = GlobalAlloc(GPTR, 3*sizeof(UINT)+nParentSize+nItemSize);
	//										if (file_PIDLs) {
	//											CIDA* pida = (CIDA*)file_PIDLs;
	//											pida->cidl = 1;
	//											pida->aoffset[0] = 3*sizeof(UINT);
	//											pida->aoffset[1] = pida->aoffset[0]+nParentSize;
	//											memmove((((LPBYTE)pida)+(pida)->aoffset[0]), &m_DesktopID, nParentSize);
	//											memmove((((LPBYTE)pida)+(pida)->aoffset[1]), pItem, nItemSize);
	//										}
	//										MCHKHEAP
	//									}
	//									if (pItem)
	//										CoTaskMemFree(pItem);
	//								}
	//							}
	//						} catch(...) {
	//							hr = -1;
	//						}
	//						if (hr == -1) {
	//							gpConEmu->DebugStep(_T("DnD: Exception in shell"));
	//							goto wrap;
	//						}
	//					}
	//
	//					MCHKHEAP
	//					DROPFILES drop_struct = { sizeof(drop_struct), { 0, 0 }, 0, 1 };
	//
	//					HGLOBAL drop_data = GlobalAlloc(GPTR, size+sizeof(drop_struct));
	//					_ASSERTE(drop_data);
	//					if (!drop_data) {
	//						gpConEmu->DebugStep(_T("DnD: Memory allocation failed!"));
	//						goto wrap;
	//					}
	//					memcpy(drop_data, &drop_struct, sizeof(drop_struct));
	//					memcpy(((byte*)drop_data) + sizeof(drop_struct), szDraggedPath, size);
	//					//Maximus5 - а память освобождать кто будет?
	//					delete szDraggedPath; szDraggedPath=NULL;
	//
	//					HGLOBAL drop_preferredeffect = GlobalAlloc(GPTR, sizeof(DWORD));
	//					if (gpConEmu->mouse.state & DRAG_R_STARTED)
	//						*((DWORD*)drop_preferredeffect) = DROPEFFECT_LINK;
	//					else if (gpSet->isDefCopy)
	//						*((DWORD*)drop_preferredeffect) = DROPEFFECT_COPY;
	//					else
	//						*((DWORD*)drop_preferredeffect) = DROPEFFECT_MOVE;
	//					MCHKHEAP
	//
	//					HGLOBAL drag_loop = GlobalAlloc(GPTR, sizeof(DWORD));
	//					*((DWORD*)drag_loop) = 1;
	//					DragImageBits *pDragBits = CreateDragImageBits ( (wchar_t*)(((byte*)drop_data) + sizeof(drop_struct)) );
	//					FORMATETC       fmtetc[] = {
	//						{ CF_HDROP, 0, DVASPECT_CONTENT, -1, TYMED_HGLOBAL },
	//						{ RegisterClipboardFormat(CFSTR_PREFERREDDROPEFFECT), 0, DVASPECT_CONTENT, -1, TYMED_HGLOBAL },
	//						{ RegisterClipboardFormat(L"InShellDragLoop"), 0, DVASPECT_CONTENT, -1, TYMED_HGLOBAL },
	//						{ RegisterClipboardFormat(L"DragImageBits"), 0, DVASPECT_CONTENT, -1, TYMED_HGLOBAL },
	//						// Эти должны идти последними
	//						{ RegisterClipboardFormat(CFSTR_SHELLIDLIST), 0, DVASPECT_CONTENT, -1, TYMED_HGLOBAL},
	//						{ RegisterClipboardFormat(L"FileNameW"), 0, DVASPECT_CONTENT, -1, TYMED_HGLOBAL }
	//					};
	//					STGMEDIUM       stgmed[] = {
	//						{ TYMED_HGLOBAL, { (HBITMAP)drop_data }, 0 }, //чтобы не морочиться с последующими присвоениями
	//						{ TYMED_HGLOBAL, { (HBITMAP)drop_preferredeffect }, 0 },
	//						{ TYMED_HGLOBAL, { (HBITMAP)drag_loop }, 0 },
	//						{ TYMED_HGLOBAL, { (HBITMAP)pDragBits }, 0 },
	//						// Эти должны идти последними, нужны только для создания ярлыков с нажатым Alt
	//						{ TYMED_HGLOBAL, { (HBITMAP)file_PIDLs }, 0},
	//						{ TYMED_HGLOBAL, { (HBITMAP)file_nameW }, 0 }
	//					};
	//					//stgmed.hGlobal = drop_data ;
	//					//--stgmed.lpszFileName=szDraggedPath;
	CreateDropSource(&pDropSource, this);
	//
	//					MCHKHEAP

	//					int nCount = sizeof(fmtetc)/sizeof(*fmtetc);
	//					if (!pDragBits) {
	//						nCount --;
	//						stgmed[nCount-2] = stgmed[nCount-1]; fmtetc[nCount-2] = fmtetc[nCount-1];
	//						stgmed[nCount-1] = stgmed[nCount];   fmtetc[nCount-1] = fmtetc[nCount];
	//					}
	//					if (!file_PIDLs) nCount -= 2;
	//					CreateDataObject(fmtetc, stgmed, nCount, &mp_DataObject) ;//   |   Посмотреть ниже...
	//
	//					// Разрешаем все
	//					DWORD dwAllowedEffects = (file_PIDLs ? DROPEFFECT_LINK : 0)|DROPEFFECT_COPY|DROPEFFECT_MOVE;
	//
	//					MCHKHEAP
	//

	//					//CFSTR_PREFERREDDROPEFFECT, FD_LINKUI
	//
	//					// Сразу получим информацию о путях панелей...
	//					if (!m_pfpi) // если это уже не сделали
	//						RetrieveDragToInfo();
	//
	//					if (LoadDragImageBits(mp_DataObject)) {
	//						mb_DragWithinNow = TRUE;
	//						CreateDragImageWindow();
	//					}

	//					//gpConEmu->DebugStep(_T("DnD: Finally, DoDragDrop"));
	//					gpConEmu->DebugStep(NULL);

	//					MCHKHEAP

	//					//// 2009-08-20 Вынесем в отдельную нить, иначе при длителной операции Drop - весь ConEmu блокируется
	//					//DragThreadArg *pArg = new DragThreadArg;
	//					//if (!pArg) {
	//					//	DisplayLastError(L"Memory allocation error (DragThreadArg)", -1);
	//					//} else {
	//					//	pArg->pThis = this;
	//					//	pArg->pDataObject = mp_DataObject;
	//					//	pArg->pDropSource = pDropSource;
	//					//	pArg->dwAllowedEffects = dwAllowedEffects;
	//					//	// Собственно запуск драга
	//					//	mh_DragThread = CreateThread(NULL, 0, CDragDrop::DragOpThreadProc, pArg, 0, &mn_DragThreadId);
	//					//}

	if (!PrepareDrag(abClickNeed, crMouseDC, &dwAllowedEffects))
	{
		//gpConEmu->DebugStep(_T("DnD: PrepareDrag failed!")); -- уже показана
	}
	else
	{
#ifdef UNLOCKED_DRAG
		CEDragSource *pds = GetFreeSource();

		if (pds && pds->hWnd)
		{
			pds->bInDrag = TRUE;
			wchar_t szStep[255]; _wsprintf(szStep, countof(szStep), L"Posting DoDragDrop(Eff=0x%X, DataObject=0x%08X, DropSource=0x%08X)", dwAllowedEffects, (DWORD)mp_DataObject, (DWORD)pDropSource);
			gpConEmu->DebugStep(szStep);
			PostMessage(pds->hWnd, MSG_STARTDRAG, dwAllowedEffects, (LPARAM)pDropSource);
			pDropSource = NULL; // чтобы ниже не от-release-илось
		}
		else
		{
			if (pds && !pds->hWnd)
			{
				MBoxA(L"Drag failed!\nDrag thread was created, but hWnd is NULL");
			}
			else
			{
#endif
				wchar_t szStep[255]; _wsprintf(szStep, SKIPLEN(countof(szStep)) L"DoDragDrop(Eff=0x%X, DataObject=0x%08X, DropSource=0x%08X)", dwAllowedEffects, (DWORD)mp_DataObject, (DWORD)pDropSource);
				gpConEmu->DebugStep(szStep);
				dwResult = DoDragDrop(mp_DataObject, pDropSource, dwAllowedEffects, &dwEffect);
				_wsprintf(szStep, SKIPLEN(countof(szStep)) L"DoDragDrop finished, Code=0x%08X", dwResult);

				switch(dwResult)
				{
					case S_OK: wcscat_c(szStep, L" (S_OK)"); break;
					case DRAGDROP_S_DROP: wcscat_c(szStep, L" (DRAGDROP_S_DROP)"); break;
					case DRAGDROP_S_CANCEL: wcscat_c(szStep, L" (DRAGDROP_S_CANCEL)"); break;
						//case E_UNSPEC: lstrcat(szStep, L" (E_UNSPEC)"); break;
				}

				gpConEmu->DebugStep(szStep, (dwResult!=S_OK && dwResult!=DRAGDROP_S_CANCEL && dwResult!=DRAGDROP_S_DROP));
#ifdef UNLOCKED_DRAG
			}
		}

#endif
	}

	MCHKHEAP
wrap:
#ifndef UNLOCKED_DRAG

	if (mp_DataObject) {mp_DataObject->Release(); mp_DataObject = NULL;}

#endif

	if (pDropSource) {pDropSource->Release(); pDropSource= NULL;}

	return;
}

//DWORD CDragDrop::DragOpThreadProc(LPVOID lpParameter)
//{
//	DragThreadArg *pArg = (DragThreadArg*)lpParameter;
//	_ASSERTE(pArg);
//	if (!pArg) {
//		DisplayLastError(L"DragThreadArg is NULL", -1);
//		return 0;
//	}
//
//	DWORD dwResult = 0, dwEffect = 0;
//
//    HRESULT hr = S_OK;
//    hr = OleInitialize (NULL); // как бы попробовать включать Ole только во время драга. кажется что из-за него глючит переключалка языка
//
//	dwResult = DoDragDrop(pArg->pDataObject, pArg->pDropSource, pArg->dwAllowedEffects, &dwEffect);
//
//	MCHKHEAP
//
//	if (pArg->pThis->mp_DataObject == pArg->pDataObject) {
//		WARNING("А объекту будет выполнен Release()?");
//		pArg->pThis->mp_DataObject = NULL;
//	}
//
//	pArg->pDataObject->Release();
//		pArg->pDataObject = NULL;
//	pArg->pDropSource->Release();
//		pArg->pDropSource = NULL;
//
//	if (pArg->pThis->mn_DragThreadId == GetCurrentThreadId()) {
//		SafeCloseHandle(pArg->pThis->mh_DragThread);
//		pArg->pThis->mn_DragThreadId = 0;
//	}
//
//	delete pArg;
//
//	return 0;
//}

wchar_t* CDragDrop::FileCreateName(BOOL abActive, BOOL abWide, BOOL abFolder, LPCWSTR asSubFolder, LPVOID asFileName)
{
	if (!asFileName ||
	        (abWide && ((wchar_t*)asFileName)[0] == 0) ||
	        (!abWide && ((char*)asFileName)[0] == 0))
	{
		MBoxA(_T("Can't drop file! Filename is empty!"));
		return NULL;
	}

	wchar_t *pszFullName = NULL;
	wchar_t* pszNameW = (wchar_t*)asFileName;
	char* pszNameA = (char*)asFileName;
	int nSize = 0;
	LPCWSTR pszPanelPath = abActive ? m_pfpi->pszActivePath : m_pfpi->pszPassivePath;
	int nPathLen = lstrlen(pszPanelPath) + 1;
	int nSubFolderLen = (asSubFolder && *asSubFolder) ? lstrlen(asSubFolder) : 0;
	nSize = nPathLen + nSubFolderLen + 16;

	if (abWide)
	{
		wchar_t* pszSlash = wcsrchr(pszNameW, L'\\');

		if (pszSlash)
		{
			if (!abFolder)
			{
				_ASSERTE(abFolder || (pszSlash == NULL) || wcsncmp(pszNameW, asSubFolder, lstrlen(asSubFolder))==0);
				pszNameW = pszSlash+1;
			}
		}

		if (!*pszNameW)
		{
			MBoxA(_T("Can't drop file! Filename is empty (W)!"));
			return NULL;
		}

		nSize += lstrlen(pszNameW)+1;
	}
	else
	{
		char* pszSlash = strrchr(pszNameA, '\\');

		if (pszSlash)
		{
			if (!abFolder)
			{
				_ASSERTE(abFolder || (pszSlash == NULL));
				//_ASSERTE(strncmp(pszNameW, asSubFolder, strlen(asSubFolder))==0);
				pszNameA = pszSlash+1;
			}
		}

		if (!*pszNameA)
		{
			MBoxA(_T("Can't drop file! Filename is empty (A)!"));
			return NULL;
		}

		nSize += strlen(pszNameA)+1;
	}

	MCHKHEAP;
	pszFullName = (wchar_t*)calloc(nSize, 2);

	if (!pszFullName)
	{
		_ASSERTE(pszFullName!=NULL);
		MBoxA(_T("Can't drop file! Memory allocation failed!"));
		return NULL;
	}

	// Скорее всего на панели путь НЕ в UNC формате, конвертим
	BOOL lbNeedUnc = FALSE;

	if (pszPanelPath[0] != L'\\' || pszPanelPath[1] != L'\\' || pszPanelPath[2] != L'?' || pszPanelPath[3] != L'\\')
		lbNeedUnc = TRUE;
	else if (pszPanelPath[0] == L'\\' && pszPanelPath[1] == L'\\')
		lbNeedUnc = TRUE;

	if (lbNeedUnc)
	{
		wcscpy(pszFullName, L"\\\\?\\");

		if (pszPanelPath[0] == L'\\' && pszPanelPath[1] == L'\\')
		{
			wcscpy(pszFullName+4, L"UNC");
			wcscpy(pszFullName+7, pszPanelPath+1);
		}
		else
		{
			wcscpy(pszFullName+4, pszPanelPath);
		}

		nPathLen = lstrlen(pszFullName) + 1;
	}
	else
	{
		wcscpy(pszFullName, pszPanelPath);
	}

	wcscat(pszFullName, L"\\");
	MCHKHEAP;

	// Должен содержать заключительный "\\"
	if (asSubFolder && *asSubFolder)
	{
		if (abFolder)
		{
			_ASSERTE(abFolder==FALSE);
		}
		else
		{
			wcscpy(pszFullName+nPathLen, asSubFolder);
			nPathLen += nSubFolderLen;
		}
	}

	if (abWide)
	{
		wcscpy(pszFullName+nPathLen, pszNameW);
	}
	else
	{
		MultiByteToWideChar(CP_ACP, 0, pszNameA, -1, pszFullName+nPathLen, nSize - nPathLen);
	}

	MCHKHEAP;
	// Путь готов, проверяем наличие файла?
	WIN32_FIND_DATAW fnd = {0};
	WARNING("Обломается в SymLink'е :(");
	// В SymLink проверка наличия папки не получится
	HANDLE hFind = FindFirstFile(pszFullName, &fnd);

	if (hFind != INVALID_HANDLE_VALUE)
	{
		FindClose(hFind);
		wchar_t* pszMsg = NULL;

		if (abFolder)
		{
			if (!(fnd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
			{
				pszMsg = (wchar_t*)calloc(nSize + 100,2);
				wcscpy(pszMsg, L"Can't create directory! Same name file exists!\n");
				wcscat(pszMsg, pszFullName);
				MessageBox(ghWnd, pszMsg, gpConEmu->ms_ConEmuVer, MB_ICONSTOP);
				free(pszMsg);
				free(pszFullName);
				return NULL;
			}
		}
		else if (fnd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
		{
			pszMsg = (wchar_t*)calloc(nSize + 100,2);
			wcscpy(pszMsg, L"Can't create file! Same name folder exists!\n");
			wcscat(pszMsg, pszFullName);
			MessageBox(ghWnd, pszMsg, gpConEmu->ms_ConEmuVer, MB_ICONSTOP);
			free(pszMsg);
			free(pszFullName);
			return NULL;
		}
		else
		{
			int nCchSize = nSize + 255;
			pszMsg = (wchar_t*)calloc(nCchSize,2);
			LARGE_INTEGER liSize;
			liSize.LowPart = fnd.nFileSizeLow; liSize.HighPart = fnd.nFileSizeHigh;
			FILETIME ftl;
			FileTimeToLocalFileTime(&fnd.ftLastWriteTime, &ftl);
			SYSTEMTIME st; FileTimeToSystemTime(&ftl, &st);
			_wsprintf(pszMsg, SKIPLEN(nCchSize) L"File already exists!\n\n%s\nSize: %I64i\nDate: %02i.%02i.%i %02i:%02i:%02i\n\nOverwrite?",
			          pszFullName, liSize.QuadPart, st.wDay, st.wMonth, st.wYear, st.wHour, st.wMinute, st.wSecond);
			int nRc = MessageBox(ghWnd, pszMsg, gpConEmu->ms_ConEmuVer, MB_ICONEXCLAMATION|MB_YESNO);
			free(pszMsg);

			if (nRc != IDYES)
			{
				free(pszFullName);
				return NULL;
			}

			// сброс ReadOnly, при необходимости
			if (fnd.dwFileAttributes & (FILE_ATTRIBUTE_HIDDEN|FILE_ATTRIBUTE_SYSTEM|FILE_ATTRIBUTE_READONLY))
				SetFileAttributes(pszFullName, FILE_ATTRIBUTE_NORMAL);
		}
	}

	MCHKHEAP;
	return pszFullName;
}

HANDLE CDragDrop::FileStart(LPCWSTR pszFullName)
{
	_ASSERTE(pszFullName && *pszFullName);

	if (!pszFullName || !*pszFullName)
		return INVALID_HANDLE_VALUE;

	// Создаем файл
	HANDLE hFile = CreateFile(pszFullName, GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);

	if (hFile == INVALID_HANDLE_VALUE)
	{
		DWORD dwErr = GetLastError();
		int nSize = lstrlen(pszFullName);
		wchar_t* pszMsg = (wchar_t*)calloc(nSize + 100,2);
		wcscpy(pszMsg, L"Can't create file!\n");
		wcscat(pszMsg, pszFullName);
		DisplayLastError(pszMsg, dwErr);
		free(pszMsg);
		//free(pszFullName);
		return INVALID_HANDLE_VALUE;
	}

	//free(pszFullName);
	mn_CurWritten = 0;
	return hFile;
}

HRESULT CDragDrop::FileWrite(HANDLE ahFile, DWORD anSize, LPVOID apData)
{
	DWORD dwWrite = 0;

	if (!WriteFile(ahFile, apData, anSize, &dwWrite, NULL) || dwWrite!=anSize)
	{
		DWORD dwLastError = GetLastError();

		if (!dwLastError) dwLastError = E_UNEXPECTED;

		return dwLastError;
	}

	mn_CurWritten += anSize;
	wchar_t KM = L'K';
	__int64 nPrepared = mn_CurWritten / 1000;

	if (nPrepared > 9999)
	{
		nPrepared /= 1000; KM = L'M';

		if (nPrepared > 9999)
		{
			nPrepared /= 1000; KM = L'G';
		}
	}

	wchar_t temp[64];
	_wsprintf(temp, SKIPLEN(countof(temp)) L"Copying %i of %i (%u%c)", (mn_CurFile+1), mn_AllFiles, (DWORD)nPrepared, KM);
	SetWindowText(ghWnd, temp);
	return S_OK;
}

HRESULT CDragDrop::DropFromStream(IDataObject * pDataObject, BOOL abActive)
{
	STGMEDIUM stgDescr = { 0 };
	FORMATETC fmtetc = { CF_HDROP, 0, DVASPECT_CONTENT, -1, TYMED_HGLOBAL };
	HANDLE hFile = NULL;
#define BufferSize 0x10000
	BYTE cBuffer[BufferSize];
	DWORD dwRead = 0;
	BOOL lbWide = FALSE;
	HRESULT hr = S_OK;
	HRESULT hrStg = S_OK;

	// CF_HDROP в структуре отсутсвует!
	//fmtetc.cfFormat = RegisterClipboardFormat(CFSTR_FILEDESCRIPTORW);

	// Опитизировано. объединены юникодная и ансишная ветки

	for(int iu = 0; iu <= 1; iu++)
	{
		if (iu == 0)
		{
			fmtetc.cfFormat = RegisterClipboardFormat(CFSTR_FILEDESCRIPTORW);
		}
		else
		{
			// Outlook 2k передает ANSI!
			fmtetc.cfFormat = RegisterClipboardFormat(CFSTR_FILEDESCRIPTORA);
		}

		// !! The caller then assumes responsibility for releasing the STGMEDIUM structure.
		if (S_OK == pDataObject->GetData(&fmtetc, &stgDescr))
		{
			lbWide = (iu == 0);
			wchar_t sUnknownError[512]; sUnknownError[0] = 0;
			//HGLOBAL hDesc = stgDescr.hGlobal;
			FILEGROUPDESCRIPTORA *pDescA = NULL;
			FILEGROUPDESCRIPTORW *pDescW = NULL;
			LPVOID ptrFileName = NULL;

			if (lbWide)
			{
				pDescW = (FILEGROUPDESCRIPTORW*)GlobalLock(stgDescr.hGlobal);
				mn_AllFiles = pDescW->cItems;
			}
			else
			{
				pDescA = (FILEGROUPDESCRIPTORA*)GlobalLock(stgDescr.hGlobal);
				mn_AllFiles = pDescA->cItems;
			}

			// Имена файлов теперь лежат в stgMedium, а для получения содержимого
			fmtetc.cfFormat = RegisterClipboardFormat(CFSTR_FILECONTENTS);
			wchar_t szSubFolder[32768]; szSubFolder[0] = 0;

			for(mn_CurFile = 0; mn_CurFile<mn_AllFiles; mn_CurFile++)
			{
				BOOL lbKnownMedium = FALSE;
				BOOL lbFolder = FALSE;
				fmtetc.lindex = mn_CurFile;

				if (lbWide)
				{
					ptrFileName = (LPVOID)(pDescW->fgd[mn_CurFile].cFileName);
					lbFolder = (pDescW->fgd[mn_CurFile].dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == FILE_ATTRIBUTE_DIRECTORY;
				}
				else
				{
					ptrFileName = (LPVOID)(pDescA->fgd[mn_CurFile].cFileName);
					lbFolder = (pDescA->fgd[mn_CurFile].dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == FILE_ATTRIBUTE_DIRECTORY;
				}

				wchar_t* pszNewFileName = FileCreateName(abActive, lbWide, lbFolder, lbFolder ? NULL : szSubFolder, ptrFileName);

				if (!pszNewFileName)
				{
					if (sUnknownError[0] == 0)
						_wsprintf(sUnknownError, SKIPLEN(countof(sUnknownError)) L"Drag item #%i has invalid file name!", mn_CurFile+1);

					continue;
				}

				if (lbFolder)
				{
					// Запомнить текущий путь в SubFolder
					int nFolderLen = lbWide ? (lstrlen((LPCWSTR)ptrFileName)) : (strlen((LPCSTR)ptrFileName));

					if ((nFolderLen + 1) >= countof(szSubFolder))
					{
						_wsprintf(sUnknownError, SKIPLEN(countof(sUnknownError)) L"Drag item #%i contains too long path!", mn_CurFile+1);
						gpConEmu->DebugStep(sUnknownError, TRUE);
						free(pszNewFileName); pszNewFileName = NULL;
						continue;
					}

					if (lbWide)
						lstrcpy(szSubFolder, (LPCWSTR)ptrFileName);
					else
						MultiByteToWideChar(CP_ACP, 0, (LPCSTR)ptrFileName, -1, szSubFolder, MAX_PATH);

					if (szSubFolder[nFolderLen-1] != L'\\')
						lstrcat(szSubFolder, L"\\");

					// Раз это папка - просто создать
					if (!CreateDirectory(pszNewFileName, NULL))
					{
						DWORD nErr = GetLastError();

						if (nErr != ERROR_ALREADY_EXISTS)
						{
							int nLen = lstrlen(pszNewFileName) + 128;
							wchar_t* pszErr = (wchar_t*)malloc(nLen*2);
							_wsprintf(pszErr, SKIPLEN(nLen) L"Can't create directory for drag item #%i!\n%s\nError code=0x%08X", mn_CurFile+1, pszNewFileName, nErr);
							MessageBox(NULL, pszErr, gpConEmu->ms_ConEmuVer, MB_OK|MB_ICONSTOP|MB_SYSTEMMODAL);
							free(pszErr);
							free(pszNewFileName); pszNewFileName = NULL;
							break;
						}
					}

					MCHKHEAP;
					free(pszNewFileName); pszNewFileName = NULL;
					continue; // переходим к следующему файлу
				}

				// !! The caller then assumes responsibility for releasing the STGMEDIUM structure.
				STGMEDIUM stgMedium = { 0 };
				//было if (S_OK == pDataObject->GetData(&fmtetc, &stgMedium) || !stgMedium.pstm)
				// не встречалось ни одного источника, который GetDataHere поддерживает
				//fmtetc.tymed = TYMED_FILE; // Сначала пробуем "попросить записать в файл"
				//stgMedium.tymed = TYMED_FILE;
				//stgMedium.pstm = NULL;
				//stgMedium.lpszFileName = ::SysAllocString(pszNewFileName);
				//// Попробуем? Но пока не встречал. Возвращают E_NOTIMPL
				//hrStg = pDataObject->GetDataHere(&fmtetc, &stgMedium);
				//::SysFreeString(stgMedium.lpszFileName);
				//if (hrStg == S_OK)
				//{
				//	free(pszNewFileName); pszNewFileName = NULL;
				//	continue;
				//}
				// Теперь - пробуем IStream
				fmtetc.tymed = TYMED_ISTREAM|TYMED_HGLOBAL;
				stgMedium.tymed = 0; //TYMED_ISTREAM;
				stgMedium.pstm = NULL;
				hrStg = pDataObject->GetData(&fmtetc, &stgMedium);

				if (hrStg == S_OK)
				{
					if (stgMedium.tymed == TYMED_ISTREAM && stgMedium.pstm)
					{
						lbKnownMedium = TRUE;
						IStream* pFile = stgMedium.pstm;

						if (!pszNewFileName)
							hFile = INVALID_HANDLE_VALUE;
						else
						{
							hFile = FileStart(pszNewFileName);
							free(pszNewFileName); pszNewFileName = NULL;
						}

						if (hFile != INVALID_HANDLE_VALUE)
						{
							// Может возвращать и S_FALSE (конец файла?)
							while(SUCCEEDED(hr = pFile->Read(cBuffer, BufferSize, &dwRead)) && dwRead)
							{
								TODO("Сюда прогресс с градусником прицепить можно");

								if (FileWrite(hFile, dwRead, cBuffer) != S_OK)
									break;
							}

							SafeCloseHandle(hFile);

							if (FAILED(hr))
							{
								_ASSERTE(SUCCEEDED(hr));
								DisplayLastError(_T("Can't read medium!"), hr);
							}
						}

						//pFile->Release();
						ReleaseStgMedium(&stgMedium);
						// OK! Могут быть еще файлы!
						// -- break;
					}
					else if (stgMedium.tymed == TYMED_HGLOBAL && stgMedium.hGlobal)
					{
						lbKnownMedium = TRUE;
						SIZE_T nFileSize = GlobalSize(stgMedium.hGlobal);
						LPBYTE ptrFileData = (LPBYTE)GlobalLock(stgMedium.hGlobal);

						if (!pszNewFileName)
							hFile = INVALID_HANDLE_VALUE;
						else
						{
							hFile = FileStart(pszNewFileName);
							free(pszNewFileName); pszNewFileName = NULL;
						}

						if (hFile != INVALID_HANDLE_VALUE)
						{
							if (ptrFileData)
							{
								LPBYTE ptrCur = ptrFileData;

								while(nFileSize > 0)
								{
									dwRead = min(nFileSize,65536);
									TODO("Сюда прогресс с градусником прицепить можно");

									if (FileWrite(hFile, dwRead, ptrCur) != S_OK)
										break;

									ptrCur += dwRead;
									nFileSize -= dwRead;
								}
							}

							SafeCloseHandle(hFile);
						}

						GlobalUnlock(stgMedium.hGlobal);
						//GlobalFree(stgMedium.hGlobal);
						ReleaseStgMedium(&stgMedium);
						// OK! Могут быть еще файлы!
						// -- break;
					}
					else
					{
						_ASSERTE(stgMedium.tymed == TYMED_ISTREAM || stgMedium.tymed == TYMED_HGLOBAL);
						ReleaseStgMedium(&stgMedium);
					}

					//continue; -- не нужно

					if (pszNewFileName)
					{
						// Уже должен быть освобожден, но проверим
						free(pszNewFileName); pszNewFileName = NULL;
					}
				}

				// Ошибку показать один раз на дроп (чтобы не ругаться на КАЖДЫЙ бросаемый файл)
				//MBoxA(_T("Drag object does not contains known medium!"));
				if (!lbKnownMedium && (sUnknownError[0] == 0))
					_wsprintf(sUnknownError, SKIPLEN(countof(sUnknownError)) L"Drag item #%i does not contains known medium!", mn_CurFile+1);
			}

			// CFSTR_FILEDESCRIPTORx
			GlobalUnlock(stgDescr.hGlobal);
			//GlobalFree(hDesc);
			ReleaseStgMedium(&stgDescr);

			// Если был обнаружен элемент, в котором ни один формат не известен - ругаемся
			if (sUnknownError[0])
			{
				ReportUnknownData(pDataObject, sUnknownError);
			}

			gpConEmu->DebugStep(NULL, TRUE);
			return S_OK;
		} // если удалось получить "pDataObject->GetData(&fmtetc, &stgMedium)" - уже вышли из функции
	}

	ReportUnknownData(pDataObject, L"Drag object does not contains known formats!");
	return S_OK;
}

HRESULT CDragDrop::DropNames(HDROP hDrop, int iQuantity, BOOL abActive)
{
	wchar_t szMacro[MAX_DROP_PATH*2+50];
	wchar_t szData[MAX_DROP_PATH*2+10];
	wchar_t* pszText = NULL;
	// -- HRESULT hr = S_OK;
	DWORD dwStartTick = GetTickCount();
#define OPER_TIMEOUT 5000
	BOOL lbAddGoto = isPressed(VK_MENU);
	BOOL lbAddEdit = isPressed(VK_CONTROL);
	BOOL lbAddView = isPressed(VK_SHIFT);

	CVirtualConsole* pVCon = gpConEmu->ActiveCon();
	if (!pVCon)
		return S_FALSE;
	CRealConsole* pRCon = pVCon->RCon();
	if (!pRCon)
		return S_FALSE;

	for(int i = 0 ; i < iQuantity; i++)
	{
		wcscpy(szMacro, L"$Text ");
		wcscpy(szData,  L"         ");
		pszText = szData + lstrlen(szData);
		int nLen = DragQueryFile(hDrop, i, pszText, MAX_DROP_PATH);

		if (nLen <= 0 || nLen >= MAX_DROP_PATH) continue;

		wchar_t* psz = pszText;
		// обработка кавычек и слэшей
		//while ((psz = wcschr(psz, L'"')) != NULL) {
		//	*psz = L'\'';
		//}
		nLen = lstrlen(psz);

		if (!m_pfpi->NoFarConsole)
		{

			while(nLen>0 && *psz)
			{
				if (*psz == L'"' || *psz == L'\\')
				{
					INT_PTR cch = nLen+1;
					INT_PTR cchMax = countof(szData) - ((psz+1) - szData);
					_ASSERTE(cch > 0 && cch <= cchMax && cchMax > 0 && cchMax < countof(szData));
					wmemmove_s(psz+1, cchMax, psz, cch);

					if (*psz == L'"') *psz = L'\\';

					psz++;
				}

				psz++; nLen--;
			}
		}
		else
		{
			_ASSERTE(m_pfpi->NoFarConsole);
		}

		if ((psz = wcschr(pszText, L' ')) != NULL)
		{
			// Имя нужно окавычить
			if (!m_pfpi->NoFarConsole)
			{
				*(--pszText) = L'\"';
				*(--pszText) = L'\\';
				wcscat(pszText, L"\\\"");
			}
			else
			{
				*(--pszText) = L'\"';
				wcscat(pszText, L"\"");
			}
		}

		if (!m_pfpi->NoFarConsole)
			wcscat(szMacro, L"\"");

		if (!m_pfpi->NoFarConsole && (lbAddGoto || lbAddEdit || lbAddView))
		{
			if (lbAddGoto)
				wcscat(szMacro, L"goto:");

			if (lbAddEdit)
				wcscat(szMacro, L"edit:");

			if (lbAddView)
				wcscat(szMacro, L"view:");

			lbAddGoto = FALSE; lbAddEdit = FALSE; lbAddView = FALSE;
		}

		if (!m_pfpi->NoFarConsole)
		{
			wcscat(szMacro, pszText);
			wcscat(szMacro, L" \"");
			gpConEmu->PostMacro(szMacro);
		}
		else
		{
			psz = pszText;
			INPUT_RECORD r = {KEY_EVENT};
			while (*psz)
			{
				r.Event.KeyEvent.bKeyDown = TRUE;
				r.Event.KeyEvent.wRepeatCount = 1;
				r.Event.KeyEvent.uChar.UnicodeChar = *(psz++);
				pRCon->PostConsoleEvent(&r);
				r.Event.KeyEvent.bKeyDown = FALSE;
				pRCon->PostConsoleEvent(&r);
			}
			r.Event.KeyEvent.bKeyDown = TRUE;
			r.Event.KeyEvent.wRepeatCount = 1;
			r.Event.KeyEvent.uChar.UnicodeChar = L' ';
			pRCon->PostConsoleEvent(&r);
			r.Event.KeyEvent.bKeyDown = FALSE;
			pRCon->PostConsoleEvent(&r);
		}

		if (((i + 1) < iQuantity)
		        && (GetTickCount() - dwStartTick) >= OPER_TIMEOUT)
		{
			BOOL b = gbDontEnable; gbDontEnable = TRUE;
			int nBtn = MessageBox(ghWnd, L"Drop operation is too long. Continue?", gpConEmu->ms_ConEmuVer, MB_YESNO|MB_ICONEXCLAMATION);
			gbDontEnable = b;

			if (nBtn != IDYES) break;

			dwStartTick = GetTickCount();
		}
	}

	//GlobalFree(hDrop);
	return S_OK;
}

HRESULT CDragDrop::DropLinks(HDROP hDrop, int iQuantity, BOOL abActive)
{
	TCHAR curr[MAX_DROP_PATH];
	LPCTSTR pszTo = abActive ? m_pfpi->pszActivePath : m_pfpi->pszPassivePath;
	int nToLen = _tcslen(pszTo);
	HRESULT hr = S_OK;

	for(int i = 0 ; i < iQuantity; i++)
	{
		int nLen = DragQueryFile(hDrop,i,curr,MAX_DROP_PATH);

		if (nLen <= 0 || nLen >= MAX_DROP_PATH) continue;

		LPCTSTR pszTitle = wcsrchr(curr, '\\');
		if (pszTitle) pszTitle++; else pszTitle = curr;

		nLen = nToLen+2+_tcslen(pszTitle)+4;
		TCHAR *szLnkPath = new TCHAR[nLen];
		_wcscpy_c(szLnkPath, nLen, pszTo);

		if (szLnkPath[lstrlen(szLnkPath)-1] != L'\\')
			_wcscat_c(szLnkPath, nLen, L"\\");

		_wcscat_c(szLnkPath, nLen, pszTitle);
		_wcscat_c(szLnkPath, nLen, L".lnk");

		try
		{
			hr = CreateLink(curr, szLnkPath, pszTitle);
		}
		catch(...)
		{
			hr = E_UNEXPECTED;
		}

		if (hr!=S_OK)
			DisplayLastError(_T("Can't create link!"), hr);

		delete szLnkPath;
	}

	//GlobalFree(hDrop);
	return S_OK;
}

HRESULT STDMETHODCALLTYPE CDragDrop::Drop(IDataObject * pDataObject,DWORD grfKeyState,POINTL pt,DWORD * pdwEffect)
{
	mb_DragWithinNow = FALSE;
	DestroyDragImageBits();
#ifdef PERSIST_OVL
	MoveDragWindow(FALSE);
#else
	DestroyDragImageWindow();
#endif
	gpConEmu->SetDragCursor(NULL);
	//PostMessage(ghWnd, WM_SETCURSOR, -1, -1);
	DWORD dwAllowed = *pdwEffect;
	*pdwEffect = DROPEFFECT_COPY|DROPEFFECT_MOVE;

	if (S_OK != DragOver(grfKeyState, pt, pdwEffect) ||  *pdwEffect == DROPEFFECT_NONE)
	{
		return S_OK;
	}

	// Определить, на какую панель бросаем
	ScreenToClient(ghWndDC, (LPPOINT)&pt);
	COORD cr = gpConEmu->ActiveCon()->ClientToConsole(pt.x, pt.y);
	pt.x = cr.X; pt.y = cr.Y;
	//pt.x/=gpSet->Log Font.lfWidth;
	//pt.y/=gpSet->Log Font.lfHeight;
	BOOL lbActive = PtInRect(&(m_pfpi->ActiveRect), *(LPPOINT)&pt);
	// Бросок в командную строку (или в редактор, потом...)
	BOOL lbDropFileNamesOnly = FALSE;

	if (m_pfpi->NoFarConsole ||
		(m_pfpi->ActiveRect.bottom && pt.y > m_pfpi->ActiveRect.bottom) ||
	        (m_pfpi->PassiveRect.bottom && pt.y > m_pfpi->PassiveRect.bottom))
	{
		lbDropFileNamesOnly = TRUE;
	}

	// Если тащат просто между панелями - сразу в FAR
	if (!lbDropFileNamesOnly && !lbActive && mb_selfdrag
	        && (*pdwEffect == DROPEFFECT_COPY || *pdwEffect == DROPEFFECT_MOVE))
	{
		wchar_t *mcr = (wchar_t*)calloc(128, sizeof(wchar_t));
		//2010-02-18 Не было префикса '@'
		//2010-03-26 префикс '@' ставить нельзя, ибо тогда процесса копирования видно не будет при отсутствии подтверждения
		// Если тянули ".." то перед копированием на другую панель сначала необходимо выйти на верхний уровень
		lstrcpyW(mcr, L"$If (APanel.SelCount==0 && APanel.Current==\"..\") CtrlPgUp $End ");
		// Теперь собственно клавиша запуска
		lstrcatW(mcr, (*pdwEffect == DROPEFFECT_MOVE) ? L"F6" : L"F5");

		// И если просили копировать сразу без подтверждения
		if (gpSet->isDropEnabled==2)
			lstrcatW(mcr, L" Enter "); //$MMode 1");

		gpConEmu->PostCopy(mcr);
		return S_OK; // Тащим внутри ФАРа
	}

	STGMEDIUM stgMedium = { 0 };
	FORMATETC fmtetc = { CF_HDROP, 0, DVASPECT_CONTENT, -1, TYMED_HGLOBAL };
	// !! The caller then assumes responsibility for releasing the STGMEDIUM structure.
	HRESULT hr = pDataObject->GetData(&fmtetc, &stgMedium);
	HDROP hDrop = NULL;

	if (hr != S_OK || !stgMedium.hGlobal)
	{
		if (mb_selfdrag || lbDropFileNamesOnly)
		{
			MBoxA(_T("Drag object does not contains known formats!"));
			return S_OK;
		}

		// Выполнить Drop из Outlook, и др. ShellExtensions (через IStream)
		hr = DropFromStream(pDataObject, lbActive);
		return hr;
	}

	STGMEDIUM stgMediumMap = { 0 };
	FORMATETC fmtetcMap = { RegisterClipboardFormat(CFSTR_FILENAMEMAPW), 0, DVASPECT_CONTENT, -1, TYMED_HGLOBAL };
	hDrop = (HDROP)stgMedium.hGlobal;
	int iQuantity = DragQueryFile(hDrop,0xFFFFFFFF,NULL,NULL);

	if (iQuantity < 1)
	{
		//GlobalFree(stgMedium.hGlobal);
		ReleaseStgMedium(&stgMedium);
		return S_OK; // ничего нет, выходим
	}

	gpConEmu->DebugStep(_T("DnD: Drop starting"));

	if (lbDropFileNamesOnly)
	{
		hr = DropNames(hDrop, iQuantity, lbActive);
		ReleaseStgMedium(&stgMedium);
		return hr;
	}

	// Если создавать линки - делаем сразу и выходим
	if (*pdwEffect == DROPEFFECT_LINK)
	{
		hr = DropLinks(hDrop, iQuantity, lbActive);
		ReleaseStgMedium(&stgMedium);
		return hr;
	}

	{
		//SHFILEOPSTRUCT *fop  = new SHFILEOPSTRUCT;
		ShlOpInfo *sfop = new ShlOpInfo;
		memset(sfop, 0, sizeof(ShlOpInfo));
		sfop->fop.hwnd = ghWnd;
		sfop->pDnD = this;
		hr = pDataObject->GetData(&fmtetcMap, &stgMediumMap);
		BOOL lbMultiDest = (hr == S_OK && stgMediumMap.hGlobal);
		TODO("Освободить stgMediumMap");
		LPCWSTR pszFileMap = (LPCWSTR)GlobalLock(stgMediumMap.hGlobal);

		if (!pszFileMap) lbMultiDest = FALSE;

		LPCWSTR pszDropPath = lbActive ? m_pfpi->pszActivePath : m_pfpi->pszPassivePath;
		int nCount = MAX_DROP_PATH*iQuantity+iQuantity+1;
		int nDstCount = 0;
		int nDstFolderLen = 0;

		if (!lbMultiDest)
		{
			int nCchLen = lstrlenW(pszDropPath)+3;
			sfop->fop.pTo = new WCHAR[nCchLen];
			_wsprintf((LPWSTR)sfop->fop.pTo, SKIPLEN(nCchLen) _T("%s\\\0\0"), pszDropPath);
		}
		else
		{
			nDstFolderLen = lstrlenW(pszDropPath);
			nDstCount = iQuantity*(nDstFolderLen+3+MAX_PATH)+iQuantity+1;
			sfop->fop.pTo=new WCHAR[nDstCount];
			ZeroMemory((void*)sfop->fop.pTo,sizeof(WCHAR)*nDstCount);
		}

		sfop->fop.pFrom=new WCHAR[nCount];
		ZeroMemory((void*)sfop->fop.pFrom,sizeof(WCHAR)*nCount);
		WCHAR *curr = (WCHAR*)sfop->fop.pFrom;
		WCHAR *dst = lbMultiDest ? (WCHAR*)sfop->fop.pTo : NULL;

		for(int i = 0 ; i < iQuantity; i++)
		{
			DragQueryFile(hDrop,i,curr,MAX_DROP_PATH);
			curr+=lstrlen(curr)+1;

			if (lbMultiDest)
			{
				lstrcpy(dst, pszDropPath);
				dst += nDstFolderLen;

				if (*(dst-1) != L'\\')
				{
					*dst++ = L'\\'; *dst = 0;
				}

				if (pszFileMap && *pszFileMap)
				{
					int nNameLen = lstrlen(pszFileMap);
					lstrcpyn(dst, pszFileMap, MAX_PATH);
					pszFileMap += nNameLen+1;
					dst += lstrlen(dst)+1;
					MCHKHEAP;
				}
			}
		}

		MCHKHEAP;
		//GlobalFree(stgMedium.hGlobal);
		ReleaseStgMedium(&stgMedium);
		hDrop = NULL;

		if (stgMediumMap.hGlobal)
		{
			if (pszFileMap) GlobalUnlock(stgMediumMap.hGlobal);

			//GlobalFree(stgMediumMap.hGlobal);
			ReleaseStgMedium(&stgMediumMap);
			stgMediumMap.hGlobal = NULL;
		}

		if (*pdwEffect == DROPEFFECT_MOVE)
			sfop->fop.wFunc=FO_MOVE;
		else
			sfop->fop.wFunc=FO_COPY;

		sfop->fop.fFlags = lbMultiDest ? FOF_MULTIDESTFILES : 0;
		//sfop->fop.fFlags=FOF_SIMPLEPROGRESS; -- пусть полностью показывает
		gpConEmu->DebugStep(_T("DnD: Shell operation starting"));
		ThInfo th;
		th.hThread = CreateThread(NULL, 0, CDragDrop::ShellOpThreadProc, sfop, 0, &th.dwThreadId);

		if (th.hThread == NULL)
		{
			DisplayLastError(_T("Can't create shell operation thread!"));
		}
		else
		{
			EnterCriticalSection(&m_CrThreads);
			m_OpThread.push_back(th);
			LeaveCriticalSection(&m_CrThreads);
		}

		gpConEmu->DebugStep(NULL);
	}

	return S_OK; //1;
}

DWORD CDragDrop::ShellOpThreadProc(LPVOID lpParameter)
{
	DWORD dwThreadId = GetCurrentThreadId();
	ShlOpInfo *sfop = (ShlOpInfo *)lpParameter;
	HRESULT hr = S_OK;
	CDragDrop* pDragDrop = sfop->pDnD;

	// Собственно операция копирования/перемещения...
	try
	{
		//-- не сбрасывать! может быть установлен FOF_MULTIDESTFILES
		//sfop->fop.fFlags = 0; //FOF_SIMPLEPROGRESS; -- пусть полную инфу показывает
		sfop->fop.fAnyOperationsAborted = FALSE;
		hr = SHFileOperation(&(sfop->fop));
	}
	catch(...)
	{
		hr = E_UNEXPECTED;
	}

	if (hr != S_OK || sfop->fop.fAnyOperationsAborted)
	{
		if (hr == 0x402)
		{
			// 0x402 Ошибка возникает если нет доступа к сетевому диску, а тащат "снаружи".
			// Shell уже ругнулся по этому поводу, так что свою ошибку вроде показывать лишнее.
			// An unknown error occurred. This is typically due to an invalid path in the source or destination.
			// This error does not occur on Windows Vista and later.
		}
		else if (hr == 7 || hr == ERROR_CANCELLED || hr == 0)
		{
			MBoxA(_T("Shell operation was cancelled"))
		}
		else
		{
			DisplayLastError(_T("Shell operation failed"), hr);
		}
	}

	if (sfop->fop.pTo) delete sfop->fop.pTo;

	if (sfop->fop.pFrom) delete sfop->fop.pFrom;

	delete sfop;
	HANDLE hTh = NULL;
	EnterCriticalSection(&pDragDrop->m_CrThreads);
	std::vector<ThInfo>::iterator iter = pDragDrop->m_OpThread.begin();

	while(iter != pDragDrop->m_OpThread.end())
	{
		if (dwThreadId == iter->dwThreadId)
		{
			hTh = iter->hThread;
			iter = pDragDrop->m_OpThread.erase(iter);
			break;
		}

		iter++;
	}

	LeaveCriticalSection(&pDragDrop->m_CrThreads);

	if (hTh)
		CloseHandle(hTh);

	return 0;
}

HRESULT STDMETHODCALLTYPE CDragDrop::DragOver(DWORD grfKeyState,POINTL pt,DWORD * pdwEffect)
{
	HRESULT hr = S_OK;
	DWORD dwAllowed = *pdwEffect;

	if (!gpSet->isDropEnabled && !gpConEmu->isDragging())
	{
		*pdwEffect = DROPEFFECT_NONE;
		gpConEmu->DebugStep(_T("DnD: Drop disabled"));
		hr = S_FALSE;
	}
	else if (m_pfpi==NULL)
	{
		*pdwEffect = DROPEFFECT_NONE;
	}
	else
	{
		TODO("Если drop идет ПОД панели - впечатать путь в командную строку");
		//gpConEmu->DebugStep(_T("DnD: DragOver starting"));
		POINT ptCur; ptCur.x = pt.x; ptCur.y = pt.y;
#ifdef _DEBUG
		GetCursorPos(&ptCur);
#endif
		RECT rcDC; GetWindowRect(ghWndDC, &rcDC);

		//HWND hWndFrom = WindowFromPoint(ptCur);
		if (/*(hWndFrom != ghWnd && hWndFrom != mh_Overlapped)
			||*/ !PtInRect(&rcDC, ptCur))
		{
			*pdwEffect = DROPEFFECT_NONE;
		}
		else
		{
			ScreenToClient(ghWndDC, (LPPOINT)&pt);
			COORD cr = gpConEmu->ActiveCon()->ClientToConsole(pt.x, pt.y);
			pt.x = cr.X; pt.y = cr.Y;
			//pt.x/=gpSet->Log Font.lfWidth;
			//pt.y/=gpSet->Log Font.lfHeight;
			BOOL lbActive = FALSE, lbPassive = FALSE;
			BOOL lbAllowToActive = // Можно ли бросать на активную панель
			    (!mb_selfdrag) // если тащат снаружи
			    || (grfKeyState & MK_ALT) // или нажат Alt
			    || (grfKeyState == MK_RBUTTON); // или тащат правой кнопкой без модификаторов

			if (m_pfpi->NoFarConsole)
			{
				*pdwEffect = DROPEFFECT_COPY; // Drop в консоль с cmd.exe
			}
			else
			{
				lbActive = PtInRect(&(m_pfpi->ActiveRect), *(LPPOINT)&pt);
				// пассивная панель может быть FullScreen и частично ПОД активной
				lbPassive = !lbActive && PtInRect(&(m_pfpi->PassiveRect), *(LPPOINT)&pt);

				// Проверяем, можно ли
				if ((lbActive && m_pfpi->pszActivePath && m_pfpi->pszActivePath[0] && lbAllowToActive) ||
					(lbPassive && m_pfpi->pszPassivePath && (m_pfpi->pszPassivePath[0] || mb_selfdrag)))
				{
					if (grfKeyState & MK_CONTROL)
						*pdwEffect = DROPEFFECT_COPY;
					else if (grfKeyState & MK_SHIFT)
						*pdwEffect = DROPEFFECT_MOVE;
					else if (grfKeyState & (MK_ALT | MK_RBUTTON))
						*pdwEffect = DROPEFFECT_LINK;
					else if (gpConEmu->mouse.state & DRAG_R_STARTED)
						*pdwEffect = DROPEFFECT_LINK; // при Drop - правая кнопка уже отпущена
					else
					{
						// Смотрим на допустимые эфеекты, определенные источником (иначе драг из корзины не работает)
						*pdwEffect = (gpSet->isDefCopy && (dwAllowed&DROPEFFECT_COPY)==DROPEFFECT_COPY) ? DROPEFFECT_COPY : DROPEFFECT_MOVE;
					}

					if (*pdwEffect == DROPEFFECT_LINK && lbPassive && m_pfpi->pszPassivePath[0] == 0)
						*pdwEffect = DROPEFFECT_NONE;
				}
				else if ((lbActive && m_pfpi->ActiveRect.bottom && pt.y > m_pfpi->ActiveRect.bottom) ||
						(lbPassive && m_pfpi->PassiveRect.bottom && pt.y > m_pfpi->PassiveRect.bottom))
				{
					*pdwEffect = DROPEFFECT_COPY;
				}
				else
				{
					*pdwEffect = DROPEFFECT_NONE;
				}
			}
		}
	}

	if (!mb_selfdrag)
	{
		if (*pdwEffect == DROPEFFECT_NONE)
		{
			MoveDragWindow(FALSE);
		}
		else if (mh_Overlapped && mp_Bits)
		{
			MoveDragWindow();
		}
		else if (mp_Bits)
		{
#ifdef PERSIST_OVL
			_ASSERTE(mh_Overlapped); // должно быть создано при запуске ConEmu
#else
			CreateDragImageWindow();
#endif
		}
	}

	//gpConEmu->DebugStep(_T("DnD: DragOver ok"));
	return hr;
}


//void CDragDrop::EnumDragFormats(IDataObject * pDataObject)
//{
//	//BOOL lbDoEnum = FALSE;
//	//if (!lbDoEnum) return;
//
//	HRESULT hr = S_OK;
//	IEnumFORMATETC *pEnum = NULL;
//	FORMATETC fmt[20];
//	STGMEDIUM stg[20];
//	LPCWSTR psz[20];
//	SIZE_T memsize[20];
//	TCHAR szName[20][MAX_PATH*2];
//	ULONG nCnt = sizeof(fmt)/sizeof(*fmt);
//	UINT i;
//
//		memset(fmt, 0, sizeof(fmt));
//		memset(stg, 0, sizeof(stg));
//		memset(psz, 0, sizeof(psz));
//
//	hr = pDataObject->EnumFormatEtc(DATADIR_GET,&pEnum);
//	if (hr==S_OK) {
//
//
//		hr = pEnum->Next(nCnt, fmt, &nCnt);
//
//		pEnum->Release();
//
//		/*
//		CFSTR_PREFERREDDROPEFFECT ?
//		*/
//		for (i=0; i<nCnt; i++) {
//			szName[i][0] = 0;
//			if (!GetClipboardFormatName(fmt[i].cfFormat, szName[i], MAX_PATH))
//			{
//				switch (fmt[i].cfFormat) {
//				case 1: lstrcpy(szName[i], L"CF_TEXT"); break;
//				case 2: lstrcpy(szName[i], L"CF_BITMAP"); break;
//				case 3: lstrcpy(szName[i], L"CF_METAFILEPICT"); break;
//				case 4: lstrcpy(szName[i], L"CF_SYLK"); break;
//				case 5: lstrcpy(szName[i], L"CF_DIF"); break;
//				case 6: lstrcpy(szName[i], L"CF_TIFF"); break;
//				case 7: lstrcpy(szName[i], L"CF_OEMTEXT"); break;
//				case 8: lstrcpy(szName[i], L"CF_DIB"); break;
//				case 9: lstrcpy(szName[i], L"CF_PALETTE"); break;
//				case 10: lstrcpy(szName[i], L"CF_PENDATA"); break;
//				case 11: lstrcpy(szName[i], L"CF_RIFF"); break;
//				case 12: lstrcpy(szName[i], L"CF_WAVE"); break;
//				case 13: lstrcpy(szName[i], L"CF_UNICODETEXT"); break;
//				case 14: lstrcpy(szName[i], L"CF_ENHMETAFILE"); break;
//				case 15: lstrcpy(szName[i], L"CF_HDROP"); break;
//				case 16: lstrcpy(szName[i], L"CF_LOCALE"); break;
//				case 17: lstrcpy(szName[i], L"CF_DIBV5"); break;
//				case 0x0080: lstrcpy(szName[i], L"CF_OWNERDISPLAY"); break;
//				case 0x0081: lstrcpy(szName[i], L"CF_DSPTEXT"); break;
//				case 0x0082: lstrcpy(szName[i], L"CF_DSPBITMAP"); break;
//				case 0x0083: lstrcpy(szName[i], L"CF_DSPMETAFILEPICT"); break;
//				case 0x008E: lstrcpy(szName[i], L"CF_DSPENHMETAFILE"); break;
//				default: wsprintf(szName[i], L"ClipFormatID=%i", fmt[i].cfFormat);
//				}
//			}
//
//
//			stg[i].tymed = TYMED_HGLOBAL;
//
//			// !! The caller then assumes responsibility for releasing the STGMEDIUM structure.
//			hr = pDataObject->GetData(fmt+i, stg+i);
//			if (hr == S_OK && stg[i].hGlobal) {
//				psz[i] = (LPCWSTR)GlobalLock(stg[i].hGlobal);
//				if (psz[i]) {
//					memsize[i] = GlobalSize(stg[i].hGlobal);
//					wsprintf(szName[i]+lstrlen(szName[i]), L", DataSize=%i", memsize[i]);
//					if (memsize[i] == 1) {
//						wsprintf(szName[i]+lstrlen(szName[i]), L", Data=0x%02X", (DWORD)*((LPBYTE)(psz[i])));
//					} if (memsize[i] == 4) {
//						wsprintf(szName[i]+lstrlen(szName[i]), L", Data=0x%08X", (DWORD)*((LPDWORD)(psz[i])));
//					} else if (memsize[i] == 8) {
//						wsprintf(szName[i]+lstrlen(szName[i]), L", Data=0x%08X%08X", (DWORD)((LPDWORD)(psz[i]))[0], (DWORD)((LPDWORD)psz[i])[1]);
//					} else {
//						lstrcat(szName[i], L", ");
//						const wchar_t* pwsz = (const wchar_t*)(psz[i]);
//						const char* pasz = (const char*)(psz[i]);
//						if (pasz[0] && pasz[1]) {
//							int nMaxLen = min(200,memsize[i]);
//							wchar_t* pwszDst = szName[i]+lstrlen(szName[i]);
//							MultiByteToWideChar(CP_ACP, 0, pasz, nMaxLen, pwszDst, nMaxLen);
//							pwszDst[nMaxLen] = 0;
//						} else {
//							int nMaxLen = min(200,memsize[i]/2);
//							lstrcpyn(szName[i]+lstrlen(szName[i]), pwsz, nMaxLen);
//						}
//					}
//				} else {
//					lstrcat(szName[i], L", hGlobal not available");
//					stg[i].hGlobal = NULL;
//				}
//			} else {
//				lstrcat(szName[i], L", hGlobal not available");
//				stg[i].hGlobal = NULL;
//			}
//
//			#ifdef _DEBUG
//			if (wcscmp(szName[i], L"DragImageBits") == 0) {
//				stg[i].tymed = TYMED_HGLOBAL;
//			}
//			#endif
//
//			lstrcat(szName[i], L"\n");
//			OutputDebugStringW(szName[i]);
//		}
//
//		for (i=0; i<nCnt; i++) {
//			if (psz[i] && stg[i].hGlobal) {
//				GlobalUnlock(stg[i].hGlobal);
//				GlobalFree(stg[i].hGlobal);
//			}
//		}
//	}
//	hr = S_OK;
//}


HRESULT STDMETHODCALLTYPE CDragDrop::DragEnter(IDataObject * pDataObject,DWORD grfKeyState,POINTL pt,DWORD * pdwEffect)
{
	mb_selfdrag = (pDataObject == mp_DataObject);
	mb_DragWithinNow = TRUE;

	if (gbDebugLogStarted || IsDebuggerPresent())
		EnumDragFormats(pDataObject);

	if (gpSet->isDropEnabled || mb_selfdrag)
	{
		if (!mb_selfdrag)
		{
			// при "своем" драге - информация уже получена
			RetrieveDragToInfo(/*pDataObject*/);

			if (LoadDragImageBits(pDataObject))
			{
#ifdef PERSIST_OVL
				_ASSERTE(mh_Overlapped); // должно быть создано при запуске ConEmu
#else
				CreateDragImageWindow();
#endif
			}
		}
	}
	else
	{
		gpConEmu->DebugStep(_T("DnD: Drop disabled"));
	}

	return 0;
}

//void CDragDrop::RetrieveDragToInfo()
//{
//	CConEmuPipe pipe(gpConEmu->GetFarPID(), CONEMUREADYTIMEOUT);
//
//	gpConEmu->DebugStep(_T("DnD: DragEnter starting"));
//
//	if (pipe.Init(_T("CDragDrop::DragEnter")))
//	{
//		if (pipe.Execute(CMD_DRAGTO))
//		{
//			DWORD cbBytesRead=0;
//			int cbStructSize=0;
//			if (m_pfpi) {free(m_pfpi); m_pfpi=NULL;}
//			if (pipe.Read(&cbStructSize, sizeof(int), &cbBytesRead))
//			{
//				if ((DWORD)cbStructSize>sizeof(ForwardedPanelInfo)) {
//					m_pfpi = (ForwardedPanelInfo*)calloc(cbStructSize, 1);
//
//					pipe.Read(m_pfpi, cbStructSize, &cbBytesRead);
//
//					m_pfpi->pszActivePath = (WCHAR*)(((char*)m_pfpi)+m_pfpi->ActivePathShift);
//					//Slash на конце нам не нужен
//					int nPathLen = lstrlenW( m_pfpi->pszActivePath );
//					if (nPathLen>0 && m_pfpi->pszActivePath[nPathLen-1]==_T('\\'))
//						m_pfpi->pszActivePath[nPathLen-1] = 0;
//
//					m_pfpi->pszPassivePath = (WCHAR*)(((char*)m_pfpi)+m_pfpi->PassivePathShift);
//					//Slash на конце нам не нужен
//					nPathLen = lstrlenW( m_pfpi->pszPassivePath );
//					if (nPathLen>0 && m_pfpi->pszPassivePath[nPathLen-1]==_T('\\'))
//						m_pfpi->pszPassivePath[nPathLen-1] = 0;
//				}
//			}
//		}
//	}
//	gpConEmu->DebugStep(NULL);
//}

HRESULT STDMETHODCALLTYPE CDragDrop::DragLeave(void)
{
	mb_DragWithinNow = FALSE;

	if (mb_selfdrag)
	{
		MoveDragWindow(FALSE);
	}
	else
	{
		DestroyDragImageBits();
#ifdef PERSIST_OVL
		MoveDragWindow(FALSE);
#else
		DestroyDragImageWindow();
#endif
	}

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

//BOOL CDragDrop::CreateDragImageBits(IDataObject * pDataObject)
//{
//	DEBUGSTROVL(L"CreateDragImageBits(IDataObject) - NOT IMPLEMENTED\n");
//	return FALSE;
//}

//#include <pshpack1.h>
//typedef struct tag_MyRgbQuad {
//	union {
//		DWORD dwValue;
//		struct {
//			BYTE    rgbBlue;
//			BYTE    rgbGreen;
//			BYTE    rgbRed;
//			BYTE    rgbAlpha;
//		};
//	};
//} MyRgbQuad;
//#include <poppack.h>
//
//DragImageBits* CDragDrop::CreateDragImageBits(wchar_t* pszFiles)
//{
//	if (!pszFiles) {
//		DEBUGSTROVL(L"CreateDragImageBits failed, pszFiles is NULL\n");
//		_ASSERTE(pszFiles!=NULL);
//		return NULL;
//	}
//
//	DestroyDragImageBits();
//	DestroyDragImageWindow();
//
//	DEBUGSTROVL(L"CreateDragImageBits()\n");
//
//	DragImageBits* pBits = NULL;
//
//	HDC hScreenDC = ::GetDC(0);
//	HDC hDrawDC = CreateCompatibleDC(hScreenDC);
//	HBITMAP hBitmap = CreateCompatibleBitmap(hScreenDC, MAX_OVERLAY_WIDTH, MAX_OVERLAY_HEIGHT);
//
//	if (hDrawDC && hBitmap) {
//		HFONT hOldF = NULL, hf = CreateFont(14, 0, 0, 0, 400, 0, 0, 0, CP_ACP, 0, 0, 0, 0, L"Tahoma");
//		if (hf) hOldF = (HFONT)SelectObject ( hDrawDC, hf );
//
//		int nMaxX = 0, nMaxY = 0, nX = 0;
//		// GetTextExtentPoint32 почему-то портит DC, поэтому ширину получим сначала
//		wchar_t *psz = pszFiles; int nFilesCol = 0;
//		while (*psz) {
//			SIZE sz = {0};
//			LPCWSTR pszText = wcsrchr(psz, L'\\');
//			if (!pszText) pszText = psz; else psz++;
//			GetTextExtentPoint32(hDrawDC, pszText, lstrlen(pszText), &sz);
//			if (sz.cx > MAX_OVERLAY_WIDTH)
//				sz.cx = MAX_OVERLAY_WIDTH;
//			if (sz.cx > nMaxX)
//				nMaxX = sz.cx;
//			psz += lstrlen(psz)+1; // длина полного пути и длина имени файла разные ;)
//			nFilesCol ++;
//		}
//		nMaxX = min((OVERLAY_TEXT_SHIFT + nMaxX),MAX_OVERLAY_WIDTH);
//
//		// Если тащат много файлов/папок - можно попробовать разместить их в несколько колонок
//		int nColCount = 1;
//		if (nFilesCol > 3) {
//			if (nFilesCol > 21 && (nMaxX * 3 + 32) <= MAX_OVERLAY_WIDTH) {
//				nFilesCol = (nFilesCol+3) / (nColCount = 4);
//			} else if (nFilesCol > 12 && (nMaxX * 2 + 32) <= MAX_OVERLAY_WIDTH) {
//				nFilesCol = (nFilesCol+2) / (nColCount = 3);
//			} else if ((nMaxX + 32) <= MAX_OVERLAY_WIDTH) {
//				nFilesCol = (nFilesCol+1) / (nColCount = 2);
//			}
//		}
//
//
//		HBITMAP hOldBitmap = (HBITMAP)SelectObject(hDrawDC, hBitmap);
//		SetTextColor(hDrawDC, RGB(0,0,128)); // Темно синий текст
//		//SetTextColor(hDrawDC, RGB(255,255,255)); // Белый текст
//		SetBkColor(hDrawDC, RGB(192,192,192)); // на сером фоне
//		SetBkMode(hDrawDC, OPAQUE);
//
//		// DC может быть испорчен (почему?), поэтому лучше почистим его
//		RECT rcFill = MakeRect(MAX_OVERLAY_WIDTH,MAX_OVERLAY_HEIGHT);
//		#ifdef _DEBUG
//		FillRect(hDrawDC, &rcFill, (HBRUSH)GetStockObject(BLACK_BRUSH/*WHITE_BRUSH*/));
//		#else
//		FillRect(hDrawDC, &rcFill, (HBRUSH)GetStockObject(BLACK_BRUSH));
//		#endif
//		//DumpImage(hDrawDC, MAX_OVERLAY_WIDTH, nMaxY, L"F:\\Dump.bmp");
//
//		psz = pszFiles;
//		int nFileIdx = 0, nColMaxY = 0, nAllFiles = 0;
//		nMaxX = 0;
//		int nFirstColWidth = 0;
//		while (*psz) {
//			if (!DrawImageBits ( hDrawDC, psz, &nMaxX, nX, &nColMaxY ))
//				break; // вышли за пределы MAX_OVERLAY_WIDTH x MAX_OVERLAY_HEIGHT (по высоте)
//			psz += lstrlen(psz)+1;
//			if (!*psz) break;
//			nFileIdx ++; nAllFiles ++;
//			if (nFileIdx >= nFilesCol) {
//				if (!nFirstColWidth)
//					nFirstColWidth = nMaxX + OVERLAY_TEXT_SHIFT;
//				if ((nX + nMaxX + 32) >= MAX_OVERLAY_WIDTH)
//					break;
//				nX += nMaxX + OVERLAY_TEXT_SHIFT + OVERLAY_COLUMN_SHIFT;
//				if (nColMaxY > nMaxY) nMaxY = nColMaxY;
//				nColMaxY = nMaxX = nFileIdx = 0;
//			}
//		}
//		if (nColMaxY > nMaxY) nMaxY = nColMaxY;
//		if (!nFirstColWidth) nFirstColWidth = nMaxX + OVERLAY_TEXT_SHIFT;
//
//		int nLineX = ((nX+nMaxX+OVERLAY_TEXT_SHIFT+3)>>2)<<2;
//		if (nLineX > MAX_OVERLAY_WIDTH) nLineX = MAX_OVERLAY_WIDTH;
//
//		SelectObject ( hDrawDC, hOldF );
//		GdiFlush();
//
//		#ifdef _DEBUG
//		DumpImage(hDrawDC, MAX_OVERLAY_WIDTH, nMaxY, L"F:\\Dump.bmp");
//		#endif
//
//		if (nLineX>2 && nMaxY>2) {
//			HDC hBitsDC = CreateCompatibleDC(hScreenDC);
//			if (hBitsDC && hDrawDC) {
//				SetLayout(hBitsDC, LAYOUT_BITMAPORIENTATIONPRESERVED);
//
//				BITMAPINFOHEADER bih = {sizeof(BITMAPINFOHEADER)};
//				bih.biWidth = nLineX;
//				bih.biHeight = nMaxY;
//				bih.biPlanes = 1;
//				bih.biBitCount = 24;
//				bih.biCompression = BI_RGB;
//				LPBYTE pSrc = NULL;
//				HBITMAP hBitsBitmap = CreateDIBSection(hScreenDC, (BITMAPINFO*)&bih, DIB_RGB_COLORS, (void**)&pSrc, NULL, 0);
//				if (hBitsBitmap) {
//					HBITMAP hOldBitsBitmap = (HBITMAP)SelectObject(hBitsDC, hBitsBitmap);
//
//					BitBlt(hBitsDC, 0,0,nLineX,nMaxY, hDrawDC,0,0, SRCCOPY);
//					GdiFlush();
//
//					DragImageBits* pDst = (DragImageBits*)GlobalAlloc(GPTR, sizeof(DragImageBits) + (nLineX*nMaxY - 1)*4);
//
//					if (pDst) {
//						pDst->nWidth = nLineX;
//						pDst->nHeight = nMaxY;
//						if (nColCount == 1)
//							pDst->nXCursor = nMaxX / 2;
//						else
//							pDst->nXCursor = nFirstColWidth;
//						pDst->nYCursor = 17; // под первой строкой
//						pDst->nRes1 = GetTickCount(); // что-то непонятное. Random?
//						pDst->nRes2 = 0xFFFFFFFF;
//
//
//						MyRgbQuad *pRGB = (MyRgbQuad*)pDst->pix;
//						u32 nCurBlend = 0xAA, nAllBlend = 0xAA;
//						for (int y = 0; y < nMaxY; y++) {
//							for (int x = 0; x < nLineX; x++) {
//								pRGB->rgbBlue = *(pSrc++);
//								pRGB->rgbGreen = *(pSrc++);
//								pRGB->rgbRed = *(pSrc++);
//								if ( pRGB->dwValue ) {
//									pRGB->rgbBlue = klMulDivU32(pRGB->rgbBlue, nCurBlend, 0xFF);
//									pRGB->rgbGreen = klMulDivU32(pRGB->rgbGreen, nCurBlend, 0xFF);
//									pRGB->rgbRed = klMulDivU32(pRGB->rgbRed, nCurBlend, 0xFF);
//									pRGB->rgbAlpha = nAllBlend;
//								}
//								pRGB++;
//							}
//						}
//
//						mp_Bits = (DragImageBits*)calloc(sizeof(DragImageBits)/*+(nCount-1)*4*/,1);
//						if (mp_Bits) {
//							*mp_Bits = *pDst;
//							pBits = pDst; pDst = NULL; //OK
//							mh_BitsDC = hBitsDC; hBitsDC = NULL;
//							mh_BitsBMP = hBitsBitmap; hBitsBitmap = NULL;
//						} else {
//							if (pDst) GlobalFree(pDst); pDst = NULL; // Ошибка
//						}
//
//					} else {
//						if (pDst) GlobalFree(pDst); pDst = NULL;
//					}
//
//					if (hBitsDC)
//						SelectObject(hBitsDC, hOldBitsBitmap);
//					if (hBitsBitmap) {
//						DeleteObject(hBitsBitmap); hBitsBitmap = NULL;
//					}
//				}
//			}
//			if (hBitsDC) {
//				DeleteDC(hBitsDC); hBitsDC = NULL;
//			}
//		}
//
//		SelectObject(hDrawDC, hOldBitmap);
//	}
//	if (hDrawDC) {
//		DeleteDC(hDrawDC); hDrawDC = NULL;
//	}
//	if (hBitmap) {
//		DeleteObject(hBitmap); hBitmap = NULL;
//	}
//	if (hScreenDC) {
//		::ReleaseDC(0, hScreenDC);
//		hScreenDC = NULL;
//	}
//
//	return pBits;
//}
//
//BOOL CDragDrop::DrawImageBits ( HDC hDrawDC, wchar_t* pszFile, int *nMaxX, int nX, int *nMaxY )
//{
//	if ( (*nMaxY + 17) >= MAX_OVERLAY_HEIGHT )
//		return FALSE;
//	SHFILEINFO sfi = {0};
//	DWORD nDrawRC = 0;
//
//	wchar_t* pszText = wcsrchr(pszFile, L'\\');
//	if (!pszText) pszText = pszFile; else pszText++;
//	SIZE sz = {0};
//	GetTextExtentPoint32(hDrawDC, pszText, lstrlen(pszText), &sz);
//	if (sz.cx > MAX_OVERLAY_WIDTH)
//		sz.cx = MAX_OVERLAY_WIDTH;
//	if (sz.cx > *nMaxX)
//		*nMaxX = sz.cx;
//
//	// Иконка, если просили
//	if (gpSet->isDragShowIcons)
//	{
//		UINT cbSize = sizeof(sfi);
//		DWORD_PTR shRc = SHGetFileInfo ( pszFile, 0, &sfi, cbSize,
//			SHGFI_ATTRIBUTES|SHGFI_ICON/*|SHGFI_SELECTED*/|SHGFI_SMALLICON|SHGFI_TYPENAME);
//
//		if (shRc) {
//			nDrawRC = DrawIconEx(hDrawDC, nX, *nMaxY, sfi.hIcon, 16, 16, 0, NULL, DI_NORMAL);
//		} else {
//			// Нарисовать стандартную иконку?
//		}
//	}
//
//	// А теперь - имя файла/папки
//	RECT rcText = {nX+OVERLAY_TEXT_SHIFT, *nMaxY+1, 0, (*nMaxY + 16)};
//	rcText.right = min(MAX_OVERLAY_WIDTH, (rcText.left + *nMaxX));
//
//	wchar_t szText[MAX_PATH+1]; lstrcpyn(szText, pszText, MAX_PATH); szText[MAX_PATH] = 0;
//	nDrawRC = DrawTextEx(hDrawDC, szText, lstrlen(szText), &rcText,
//		DT_LEFT|DT_TOP|DT_NOPREFIX|DT_END_ELLIPSIS|DT_SINGLELINE|DT_MODIFYSTRING, NULL);
//
//	if (*nMaxY < (rcText.bottom+1))
//		*nMaxY = min(rcText.bottom+1,300);
//
//	if (sfi.hIcon) {
//		DestroyIcon(sfi.hIcon); sfi.hIcon = NULL;
//	}
//	return TRUE;
//}
//
//BOOL CDragDrop::LoadDragImageBits(IDataObject * pDataObject)
//{
//	if (/*mb_selfdrag ||*/ mh_Overlapped)
//		return FALSE; // уже
//
//	DestroyDragImageBits();
//	DestroyDragImageWindow();
//
//	if (!gpSet->isDragOverlay)
//		return FALSE;
//
//	STGMEDIUM stgMedium = { 0 };
//	FORMATETC fmtetc = { 0, 0, DVASPECT_CONTENT, -1, TYMED_HGLOBAL };
//
//	// Это пока что-то не работает
//	//if (gpSet->isDragOverlay == 1) {
//	//	wchar_t* pszFiles = NULL;
//	//	fmtetc.cfFormat = RegisterClipboardFormat(L"FileNameW");
//	//	TODO("А освобождать полученное надо?");
//	//	if (S_OK == pDataObject->QueryGetData(&fmtetc)) {
//	//		if (S_OK == pDataObject->GetData(&fmtetc, &stgMedium) || stgMedium.hGlobal == NULL) {
//	//			pszFiles = (wchar_t*)GlobalLock(stgMedium.hGlobal);
//	//			if (pszFiles) {
//	//				return (CreateDragImageBits(pszFiles) != NULL);
//	//			}
//	//		}
//	//	}
//	//}
//
//	fmtetc.cfFormat = RegisterClipboardFormat(L"DragImageBits");
//
//	if (S_OK != pDataObject->QueryGetData(&fmtetc)) {
//		return FALSE; // Формат отсутствует
//	}
//	// !! The caller then assumes responsibility for releasing the STGMEDIUM structure.
//	if (S_OK != pDataObject->GetData(&fmtetc, &stgMedium) || stgMedium.hGlobal == NULL) {
//		return FALSE; // Формат отсутствует
//	}
//
//	SIZE_T nInfoSize = GlobalSize(stgMedium.hGlobal);
//	if (!nInfoSize) {
//		GlobalFree(stgMedium.hGlobal);
//		return FALSE; // пусто
//	}
//	DragImageBits* pInfo = (DragImageBits*)GlobalLock(stgMedium.hGlobal);
//	if (!pInfo) {
//		GlobalFree(stgMedium.hGlobal);
//		return FALSE; // Не удалось получить данные
//	}
//	_ASSERTE(pInfo->nWidth>0 && pInfo->nWidth<=400);
//	_ASSERTE(pInfo->nHeight>0 && pInfo->nHeight<=400);
//	if (nInfoSize != (sizeof(DragImageBits)+(pInfo->nWidth * pInfo->nHeight - 1)*4)) {
//		_ASSERT(FALSE); // Неизвестный формат?
//		GlobalFree(stgMedium.hGlobal);
//		return FALSE;
//	}
//
//	BOOL lbRc = FALSE;
//	//int nCount = pInfo->nWidth * pInfo->nHeight;
//	mp_Bits = (DragImageBits*)calloc(sizeof(DragImageBits)/*+(nCount-1)*4*/,1);
//	if (mp_Bits) {
//		*mp_Bits = *pInfo;
//		//memmove(mp_Bits->pix, pInfo->pix, nCount*4);
//		MCHKHEAP
//
//		HDC hdc = ::GetDC(ghWnd);
//		mh_BitsDC = CreateCompatibleDC(hdc);
//		if (mh_BitsDC) {
//			SetLayout(mh_BitsDC, LAYOUT_BITMAPORIENTATIONPRESERVED);
//
//			BITMAPINFOHEADER bih = {sizeof(BITMAPINFOHEADER)};
//			bih.biWidth = mp_Bits->nWidth;
//			bih.biHeight = mp_Bits->nHeight;
//			bih.biPlanes = 1;
//			bih.biBitCount = 32;
//			bih.biCompression = BI_RGB;
//			bih.biXPelsPerMeter = 96;
//			bih.biYPelsPerMeter = 96;
//
//			LPBYTE pDst = NULL;
//			mh_BitsBMP_Old = NULL;
//			mh_BitsBMP = CreateDIBSection(hdc, (BITMAPINFO*)&bih, DIB_RGB_COLORS, (void**)&pDst, NULL, 0);
//			if (mh_BitsBMP && pDst) {
//				mh_BitsBMP_Old = (HBITMAP)SelectObject(mh_BitsDC, mh_BitsBMP);
//
//				int cbSize = pInfo->nWidth * pInfo->nHeight * 4;
//				memmove(pDst, pInfo->pix, cbSize);
//				GdiFlush();
//
//				#ifdef _DEBUG
//				int nBits = GetDeviceCaps(mh_BitsDC, BITSPIXEL);
//				//_ASSERTE(nBits == 32);
//				#endif
//
//				lbRc = TRUE;
//			}
//		}
//		if (hdc) ::ReleaseDC(ghWnd, hdc);
//	}
//
//	// Освободим данные
//	GlobalUnlock(stgMedium.hGlobal);
//	GlobalFree(stgMedium.hGlobal);
//
//	if (!lbRc)
//		DestroyDragImageBits();
//
//	return lbRc;
//}
//
//void CDragDrop::DestroyDragImageBits()
//{
//	if (mh_BitsDC || mh_BitsBMP || mp_Bits)
//	{
//		DEBUGSTROVL(L"DestroyDragImageBits()\n");
//		if (mh_BitsDC)  {
//			if (mh_BitsBMP_Old) {
//				SelectObject(mh_BitsDC, mh_BitsBMP_Old); mh_BitsBMP_Old = NULL;
//			}
//			DeleteDC(mh_BitsDC);
//			mh_BitsDC = NULL;
//		}
//		if (mh_BitsBMP) {
//			DeleteObject(mh_BitsBMP);
//			mh_BitsBMP = NULL;
//		}
//		if (mp_Bits) {
//			free(mp_Bits);
//			mp_Bits = NULL;
//		}
//	}
//}
//
//BOOL CDragDrop::CreateDragImageWindow()
//{
//	DEBUGSTROVL(L"CreateDragImageWindow()\n");
//
//	#define DRAGBITSCLASS L"ConEmuDragBits"
//	static BOOL bClassRegistered = FALSE;
//	if (!bClassRegistered) {
//		#ifdef _DEBUG
//		DWORD dwLastError = 0;
//		#endif
//		HBRUSH hBackBrush = (HBRUSH)GetStockObject(WHITE_BRUSH);
//		WNDCLASS wc = {0/*CS_OWNDC*/ /*CS_SAVEBITS*/, DragBitsWndProc, 0, 0,
//				g_hInstance, NULL, NULL/*LoadCursor(NULL, IDC_ARROW)*/,
//				hBackBrush,  NULL, DRAGBITSCLASS};
//		if (!RegisterClass(&wc)) {
//			#ifdef _DEBUG
//			dwLastError = GetLastError();
//			#endif
//			// Ругаться не будем, чтобы драг не испортить
//			return FALSE;
//		}
//		bClassRegistered = TRUE; // регистрировать класс один раз
//	}
//
//
//	//int nCount = mp_Bits->nWidth * mp_Bits->nHeight;
//
//	// |WS_BORDER|WS_SYSMENU - создает проводник. попробуем?
//	//2009-08-20 [+] WS_EX_NOACTIVATE
//	mh_Overlapped = CreateWindowEx(
//		WS_EX_LAYERED|WS_EX_TRANSPARENT|WS_EX_PALETTEWINDOW|WS_EX_TOOLWINDOW|WS_EX_TOPMOST|WS_EX_NOACTIVATE,
//		DRAGBITSCLASS, L"Drag", WS_POPUP|WS_VISIBLE|WS_CLIPSIBLINGS|WS_BORDER|WS_SYSMENU,
//		0, 0, mp_Bits->nWidth, mp_Bits->nHeight, ghWnd, NULL, g_hInstance, (LPVOID)this);
//	if (!mh_Overlapped) {
//		DestroyDragImageBits();
//		return NULL;
//	}
//
//	MoveDragWindow(FALSE);
//
//	return TRUE;
//}
//
//void CDragDrop::MoveDragWindow(BOOL abVisible/*=TRUE*/)
//{
//	if (!mh_Overlapped) {
//		DEBUGSTRBACK(L"MoveDragWindow skipped, Overlay was not created\n");
//		return;
//	}
//
//	DEBUGSTRBACK(L"MoveDragWindow()\n");
//
//	BLENDFUNCTION bf;
//	bf.BlendOp = AC_SRC_OVER;
//	bf.AlphaFormat = AC_SRC_ALPHA;
//	bf.BlendFlags = 0;
//	bf.SourceConstantAlpha = abVisible ? 255 : 0;
//
//	POINT p = {0};
//	GetCursorPos(&p);
//	p.x -= mp_Bits->nXCursor; p.y -= mp_Bits->nYCursor;
//
//	POINT p2 = {0, 0};
//	SIZE  sz = {mp_Bits->nWidth,mp_Bits->nHeight};
//
//	#ifdef _DEBUG
//	BOOL bRet =
//	#endif
//	UpdateLayeredWindow(mh_Overlapped, NULL, &p, &sz, mh_BitsDC, &p2, 0,
//		&bf, ULW_ALPHA /*ULW_OPAQUE*/);
//	//_ASSERTE(bRet);
//
//	//apiSetForegroundWindow(ghWnd); // после создания окна фокус уходит из GUI
//}
//
//void CDragDrop::DestroyDragImageWindow()
//{
//	if (mh_Overlapped) {
//		DEBUGSTROVL(L"DestroyDragImageWindow()\n");
//		DestroyWindow(mh_Overlapped);
//		mh_Overlapped = NULL;
//	}
//}
//
//LRESULT CDragDrop::DragBitsWndProc(HWND hWnd, UINT messg, WPARAM wParam, LPARAM lParam)
//{
//	if (messg == WM_CREATE) {
//		SetWindowLongPtr(hWnd, GWLP_USERDATA,
//			(LONG_PTR)((LPCREATESTRUCT)lParam)->lpCreateParams);
//
//	} else if (messg == WM_SETFOCUS) {
//		CDragDrop *pDrag = (CDragDrop*)GetWindowLongPtr(hWnd, GWLP_USERDATA);
//		if (pDrag && pDrag->mb_DragWithinNow) {
//			TODO("Если создавать окно сразу - не понадобится с фокусом играться");
//			apiSetForegroundWindow(ghWnd); // после создания окна фокус уходит из GUI
//		}
//		return 0;
//	}
//
//	return DefWindowProc(hWnd, messg, wParam, lParam);
//}
//
//BOOL CDragDrop::InDragDrop()
//{
//	if (mp_DataObject || mh_Overlapped)
//		return TRUE;
//	return FALSE;
//}
//
//void CDragDrop::DragFeedBack(DWORD dwEffect)
//{
//#ifdef _DEBUG
//	wchar_t szDbg[128]; _wsprintf(szDbg, countof(szDbg), L"DragFeedBack(%i)\n", (int)dwEffect);
//	DEBUGSTRBACK(szDbg);
//#endif
//	// Drop или отмена драга, когда источник - ConEmu
//	if (dwEffect == (DWORD)-1) {
//		mb_DragWithinNow = FALSE;
//		DestroyDragImageWindow();
//		gpConEmu->SetDragCursor(NULL);
//	} else if (dwEffect == DROPEFFECT_NONE) {
//		MoveDragWindow(FALSE);
//	} else {
//		if (!mh_Overlapped && mp_Bits)
//			CreateDragImageWindow();
//		if (mh_Overlapped)
//			MoveDragWindow();
//	}
//
//	//2009-08-20
//	//MSG Msg = {NULL};
//	//while (PeekMessage(&Msg, NULL, 0, 0, PM_REMOVE))
//	//{
//	//	TranslateMessage(&Msg);
//	//	DispatchMessage(&Msg);
//	//}
//}

void CDragDrop::ReportUnknownData(IDataObject * pDataObject, LPCWSTR sUnknownError)
{
	HANDLE hFile = NULL;
	size_t nLen = lstrlen(sUnknownError);
	wchar_t* pszMsg = (wchar_t*)calloc((nLen+256),sizeof(wchar_t));
	lstrcpy(pszMsg, sUnknownError);
	lstrcpy(pszMsg+nLen, L"\n\nPress 'Retry' to create report for developer");
	BOOL b = gbDontEnable; gbDontEnable = TRUE;
	int nBtn = (int)MessageBox(ghWnd, pszMsg, gpConEmu->ms_ConEmuVer,
	                           MB_SYSTEMMODAL|MB_ICONINFORMATION|MB_RETRYCANCEL|MB_DEFBUTTON2);
	gbDontEnable = b;
	free(pszMsg);

	if (nBtn != IDRETRY)
		return;

	// Дать пользователю выбрать файл
	OPENFILENAME ofn; memset(&ofn,0,sizeof(ofn));
	WCHAR temp[MAX_PATH+5];
	ofn.lStructSize=sizeof(ofn);
	ofn.hwndOwner = ghWnd;
	ofn.lpstrFilter = _T("Reports (*.txt)\0*.txt\0\0");
	ofn.nFilterIndex = 1;
	ofn.lpstrFile = temp; temp[0] = 0;
	ofn.nMaxFile = MAX_PATH;
	ofn.lpstrTitle = L"Create Drag and Drop report";
	ofn.lpstrDefExt = L"txt";
	ofn.Flags = OFN_ENABLESIZING|OFN_NOCHANGEDIR
	            | OFN_PATHMUSTEXIST|OFN_EXPLORER|OFN_HIDEREADONLY|OFN_OVERWRITEPROMPT;

	if (!GetSaveFileName(&ofn))
		return;

	hFile = CreateFile(temp, GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);

	if (hFile && hFile != INVALID_HANDLE_VALUE)
	{
		EnumDragFormats(pDataObject, hFile);
		CloseHandle(hFile);
	}
}
