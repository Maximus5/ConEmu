
/*
Copyright (c) 2009-2012 Maximus5
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

#include <shlobj.h>
#include "Header.h"
#include "ScreenDump.h"
#include "DragDrop.h"
#include "Options.h"
#include "ConEmu.h"
#include "VirtualConsole.h"
#include "VConGroup.h"
#include "RealConsole.h"
#include "Menu.h"
#include "Update.h"

//#define MAX_OVERLAY_WIDTH    300
//#define MAX_OVERLAY_HEIGHT   300
//#define OVERLAY_TEXT_SHIFT   (gpSet->isDragShowIcons ? 18 : 0)
//#define OVERLAY_COLUMN_SHIFT 5

#define DEBUGSTROVL(s) //DEBUGSTR(s)
#define DEBUGSTRBACK(s) //DEBUGSTR(s)
#define DEBUGSTROVER(s) //DEBUGSTR(s)
#define DEBUGSTRSTEP(s) //DEBUGSTR(s)
#define DEBUGSTROVERINT(s) //DEBUGSTR(s)
#define DEBUGSTRFAR(s) //DEBUGSTR(s)


#define DUMP_DRAGGED_ITEMS_INFO


//#define ForwardedPanelInfo

CDragDrop::CDragDrop()
{
	mb_DragDropRegistered = FALSE;
	m_lRefCount = 1;
	m_pfpi = NULL;
	mp_DataObject = NULL;
	//mb_selfdrag = NULL;
	m_DesktopID.mkid.cb = 0;
	//mh_Overlapped = NULL; mh_BitsDC = NULL; mh_BitsBMP = mh_BitsBMP_Old = NULL;
	//mp_Bits = NULL;
	mn_AllFiles = 0; mn_CurWritten = 0; mn_CurFile = 0;
	mb_DragWithinNow = FALSE;
	mn_ExtractIconsTID = 0;
	mh_ExtractIcons = NULL;
	InitializeCriticalSection(&m_CrThreads);
}

void CDragDrop::DebugLog(LPCWSTR asInfo, BOOL abErrorSeverity/*=FALSE*/)
{
	gpConEmu->DebugStep(asInfo, abErrorSeverity);
	if (asInfo)
	{
		DEBUGSTRSTEP(asInfo);
	}
}

CDragDrop::~CDragDrop()
{
	//DestroyDragImageBits();
	//DestroyDragImageWindow();

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
		              gpConEmu->GetDefaultTitle(), MB_YESNO|MB_ICONEXCLAMATION) == IDYES)
		{
			// Terminate all shell (copying) threads
			EnterCriticalSection(&m_CrThreads);
			//std::vector<ThInfo>::iterator iter = m_OpThread.begin();

			//while (iter != m_OpThread.end())
			while (m_OpThread.size() > 0)
			{
				INT_PTR j = m_OpThread.size()-1;
				const ThInfo* iter = &(m_OpThread[j]);
				
				HANDLE hThread = iter->hThread;
				TerminateThread(hThread, 100);
				CloseHandle(hThread);

				//iter = m_OpThread.erase(iter);
				m_OpThread.erase(j);
			}

			LeaveCriticalSection(&m_CrThreads);
		}
		else
		{
			// Wait until finished
			BOOL lbActive = TRUE;

			while (lbActive)
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


	//if (m_pfpi) free(m_pfpi); m_pfpi=NULL;

	//if (mp_DesktopID) { CoTaskMemFree(mp_DesktopID); mp_DesktopID = NULL; }
	DeleteCriticalSection(&m_CrThreads);
}

void CDragDrop::Drag(BOOL abClickNeed, COORD crMouseDC)
{
	DWORD dwAllowedEffects = 0, dwResult = 0, dwEffect = 0;;
	IDropSource *pDropSource = NULL;
#ifdef PRESIST_OVL
	//_ASSERTE(mh_Overlapped!=NULL);
#endif

	if (!gpSet->isDragEnabled /*|| isInDrag */|| gpConEmu->isDragging())
	{
		DebugLog(gpSet->isDragEnabled
		                    ? _T("DnD: Already in Drag loop") : _T("DnD: Drag disabled"),
		                    gpSet->isDragEnabled);
		goto wrap;
	}

	_ASSERTE(!gpConEmu->isPictureView());
	MCHKHEAP

	CreateDropSource(&pDropSource, this);

	MCHKHEAP

	if (!PrepareDrag(abClickNeed, crMouseDC, &dwAllowedEffects))
	{
		DEBUGSTRSTEP(_T("DnD: PrepareDrag failed!"));
		MCHKHEAP
	}
	else
	{
		MCHKHEAP

#ifdef UNLOCKED_DRAG
		CEDragSource *pds = GetFreeSource();

		if (pds && pds->hWnd)
		{
			pds->bInDrag = TRUE;
			wchar_t szStep[255]; _wsprintf(szStep, countof(szStep), L"Posting DoDragDrop(Eff=0x%X, DataObject=0x%08X, DropSource=0x%08X)", dwAllowedEffects, (DWORD)mp_DataObject, (DWORD)pDropSource);
			DebugLog(szStep);
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
				wchar_t szStep[255]; _wsprintf(szStep, SKIPLEN(countof(szStep)) L"DoDragDrop(Eff=0x%X, DataObject=0x%08X, DropSource=0x%08X)", dwAllowedEffects, (DWORD)mp_DataObject, (DWORD)pDropSource); //-V205
				DebugLog(szStep);
				SAFETRY
				{
					dwResult = DoDragDrop(mp_DataObject, pDropSource, dwAllowedEffects, &dwEffect);
				}
				SAFECATCH
				{
					dwResult = DRAGDROP_S_CANCEL;
					MBoxA(L"Exception in DoDragDrop\nConEmu restart is recommended");
				}
				_wsprintf(szStep, SKIPLEN(countof(szStep)) L"DoDragDrop finished, Code=0x%08X", dwResult);

				switch(dwResult)
				{
					case S_OK: wcscat_c(szStep, L" (S_OK)"); break;
					case DRAGDROP_S_DROP: wcscat_c(szStep, L" (DRAGDROP_S_DROP)"); break;
					case DRAGDROP_S_CANCEL: wcscat_c(szStep, L" (DRAGDROP_S_CANCEL)"); break;
						//case E_UNSPEC: lstrcat(szStep, L" (E_UNSPEC)"); break;
				}

				DebugLog(szStep, (dwResult!=S_OK && dwResult!=DRAGDROP_S_CANCEL && dwResult!=DRAGDROP_S_DROP));
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

wchar_t* CDragDrop::FileCreateName(BOOL abActive, BOOL abFolder, LPCWSTR asSubFolder, LPCWSTR asFileName)
{
	if (!asFileName || !*asFileName)
	{
		AssertMsg(L"Can't drop file! Filename is empty!");
		return NULL;
	}

	wchar_t *pszFullName = NULL;
	LPCWSTR pszNameW = asFileName;
	INT_PTR nSize = 0;
	LPCWSTR pszPanelPath = abActive ? m_pfpi->pszActivePath : m_pfpi->pszPassivePath;
	INT_PTR nPathLen = _tcslen(pszPanelPath) + 1;
	INT_PTR nSubFolderLen = (asSubFolder && *asSubFolder) ? _tcslen(asSubFolder) : 0;
	nSize = nPathLen + nSubFolderLen + 17;


	LPCWSTR pszSlash = wcsrchr(pszNameW, L'\\');

	if (pszSlash)
	{
		if (!abFolder)
		{
			_ASSERTE(abFolder || (pszSlash == NULL) || wcsncmp(pszNameW, asSubFolder, _tcslen(asSubFolder))==0);
			pszNameW = pszSlash+1;
		}
	}

	if (!*pszNameW)
	{
		MBoxA(_T("Can't drop file! Filename is empty (W)!"));
		return NULL;
	}

	nSize += _tcslen(pszNameW)+1;

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
			wcscpy(pszFullName+4, L"UNC"); //-V112
			wcscpy(pszFullName+7, pszPanelPath+1);
		}
		else
		{
			wcscpy(pszFullName+4, pszPanelPath); //-V112
		}

		nPathLen = _tcslen(pszFullName) + 1;
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


	wcscpy(pszFullName+nPathLen, pszNameW);

	// Отрезать хвостовые пробелы. Гррр....
	INT_PTR nTotal = _tcslen(pszFullName);
	while ((nTotal > 0) && (pszFullName[nTotal-1] == L' '))
	{
		pszFullName[--nTotal] = 0;
	}
	if (!abFolder && ((nTotal <= 0) || (pszFullName[nTotal - 1] == L'\\')))
	{
		Assert((nTotal > 0) && (pszFullName[nTotal - 1] != L'\\'));
		free(pszFullName);
		return NULL;
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
				MessageBox(ghWnd, pszMsg, gpConEmu->GetDefaultTitle(), MB_ICONSTOP);
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
			MessageBox(ghWnd, pszMsg, gpConEmu->GetDefaultTitle(), MB_ICONSTOP);
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
			_wsprintf(pszMsg, SKIPLEN(nCchSize)
				L"File already exists!\n\n%s\nSize: %I64i\nDate: %02i.%02i.%i %02i:%02i:%02i\n\nOverwrite?",
				pszFullName, liSize.QuadPart, st.wDay, st.wMonth, st.wYear, st.wHour, st.wMinute, st.wSecond);
			int nRc = MessageBox(ghWnd, pszMsg, gpConEmu->GetDefaultTitle(), MB_ICONEXCLAMATION|MB_YESNO);
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

	// Folder? Add trailing slash
	if (abFolder)
	{
		nTotal = _tcslen(pszFullName);
		if ((nTotal > 0) && (pszFullName[nTotal-1] != L'\\'))
		{
			pszFullName[nTotal++] = L'\\';
			pszFullName[nTotal] = 0;
		}
		_ASSERTE((nTotal > 0) && (pszFullName[nTotal-1] == L'\\'));
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
		INT_PTR nSize = _tcslen(pszFullName);
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
	size_t BufferSize = 0x10000;
	BYTE *cBuffer = (BYTE*)malloc(BufferSize);
	DWORD dwRead = 0;
	BOOL lbWide = FALSE;
	HRESULT hr = S_OK;
	HRESULT hrStg = S_OK;

	if (!cBuffer)
	{
		_ASSERTE(cBuffer!=NULL);
		goto wrap;
	}

	// CF_HDROP в структуре отсутсвует!
	//fmtetc.cfFormat = RegisterClipboardFormat(CFSTR_FILEDESCRIPTORW);

	// Опитизировано. объединены юникодная и ансишная ветки

	for (int iu = 0; iu <= 1; iu++)
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

			int nLastProgress = -1;

			for (mn_CurFile = 0; mn_CurFile<mn_AllFiles; mn_CurFile++)
			{
				int nProgress = (mn_CurFile * 100 / mn_AllFiles);
				if (nLastProgress != nProgress)
				{
					gpConEmu->Taskbar_SetProgressValue(nProgress);
					nLastProgress = nProgress;
				}

				BOOL lbKnownMedium = FALSE;
				BOOL lbFolder = FALSE;
				fmtetc.lindex = mn_CurFile;

				// Well, here we may get different structures...
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

				// But we need UNICODE strings
				wchar_t* pszWideBuf = lbWide ? NULL : lstrdupW((LPCSTR)ptrFileName);
				LPCWSTR pszWideName = lbWide ? (LPCWSTR)ptrFileName : pszWideBuf;

				bool bCreateFolder = lbFolder;
				wchar_t* pszNewFolder = NULL;

				if (!lbFolder)
				{
					// Файл? Сменилась подпапка?
					LPCWSTR pszSlash = wcsrchr(pszWideName, L'\\');

					if (pszSlash)
					{
						INT_PTR nFolderLen = pszSlash - pszWideName; // БЕЗ слеша
						INT_PTR nCurFolderLen = _tcslen(szSubFolder); // с учетом слеша
						int nCmp = wcsncmp(pszWideName, szSubFolder, nCurFolderLen);

						if (((nFolderLen + 1) != nCurFolderLen) && (nCmp == 0))
						{
							// OK, папка уже была создана
						}
						else
						{
							//_ASSERTE(FALSE && "Folder was not processed from DragObject?");
							
							// Поместить новый путь в SubFolder

							if ((nFolderLen + 1) >= (INT_PTR)countof(szSubFolder))
							{
								_wsprintf(sUnknownError, SKIPLEN(countof(sUnknownError)) L"Drag item #%i contains too long path!", mn_CurFile+1);
								DebugLog(sUnknownError, TRUE);
								SafeFree(pszWideBuf);
								continue;
							}

							wmemmove(szSubFolder, pszWideName, (nFolderLen + 1)*sizeof(*szSubFolder));
							szSubFolder[(nFolderLen + 1)] = 0;

							pszNewFolder = FileCreateName(abActive, true, NULL, szSubFolder);
							_ASSERTE(pszNewFolder!=NULL);
							bCreateFolder = (pszNewFolder != NULL);
						}
					}
					else
					{
						_ASSERTE((szSubFolder[0]==0) && "Files in stream must contains local paths?");
						// Сброс папки. По идее, файлы должны указываться с относительными путями.
						szSubFolder[0] = 0;
					}
				}
				else
				{
					pszNewFolder = FileCreateName(abActive, true, NULL, pszWideName);
					_ASSERTE(pszNewFolder!=NULL);
					bCreateFolder = (pszNewFolder != NULL);
				}

				// Folders must be created
				if (bCreateFolder)
				{
					//SafeFree(pszWideBuf);

					if (!pszNewFolder)
					{
						Assert(pszNewFolder!=NULL);

						if (sUnknownError[0] == 0)
							_wsprintf(sUnknownError, SKIPLEN(countof(sUnknownError)) L"Drag item #%i has invalid file name!", mn_CurFile+1);

						SafeFree(pszWideBuf);
						continue;
					}

					// if !lbFolder - szSubFolder already processed
					if (lbFolder)
					{
						// Запомнить текущий путь в SubFolder
						INT_PTR nFolderLen = _tcslen(pszWideName);

						if ((nFolderLen + 1) >= (INT_PTR)countof(szSubFolder))
						{
							_wsprintf(sUnknownError, SKIPLEN(countof(sUnknownError)) L"Drag item #%i contains too long path!", mn_CurFile+1);
							DebugLog(sUnknownError, TRUE);
							SafeFree(pszNewFolder);
							SafeFree(pszWideBuf);
							continue;
						}

						wcscpy_c(szSubFolder, pszWideName);

						if (szSubFolder[nFolderLen-1] != L'\\')
						{
							szSubFolder[nFolderLen++] = L'\\';
							szSubFolder[nFolderLen] = 0;
						}
					}

					// Раз это папка - просто создать
					if (!MyCreateDirectory(pszNewFolder))
					{
						DWORD nErr = GetLastError();

						if (nErr != ERROR_ALREADY_EXISTS)
						{
							INT_PTR nLen = _tcslen(pszNewFolder) + 128;
							wchar_t* pszErr = (wchar_t*)malloc(nLen*sizeof(*pszErr));
							_wsprintf(pszErr, SKIPLEN(nLen) L"Can't create directory for drag item #%i!\n%s\nError code=0x%08X", mn_CurFile+1, pszNewFolder, nErr);
							
							AssertMsg(pszErr);

							SafeFree(pszErr);
							SafeFree(pszNewFolder);
							SafeFree(pszWideBuf);
							break;
						}
					}

					MCHKHEAP;
					SafeFree(pszNewFolder);

					if (lbFolder)
					{
						SafeFree(pszWideBuf);
						continue; // переходим к следующему файлу/папке
					}
				}


				// Files now

				wchar_t* pszNewFileName = FileCreateName(abActive, false, szSubFolder, pszWideName);

				SafeFree(pszWideBuf);

				if (!pszNewFileName)
				{
					if (sUnknownError[0] == 0)
						_wsprintf(sUnknownError, SKIPLEN(countof(sUnknownError)) L"Drag item #%i has invalid file name!", mn_CurFile+1);

					continue;
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
									dwRead = min(nFileSize,65536); //-V103
									TODO("Сюда прогресс с градусником прицепить можно");

									if (FileWrite(hFile, dwRead, ptrCur) != S_OK)
										break;

									ptrCur += dwRead; //-V102
									nFileSize -= dwRead; //-V101
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

			DebugLog(NULL, TRUE);
			goto wrap;
		} // если удалось получить "pDataObject->GetData(&fmtetc, &stgMedium)" - уже вышли из функции
	}

	ReportUnknownData(pDataObject, L"Drag object does not contains known formats!");
wrap:
	SafeFree(cBuffer);
	gpConEmu->Taskbar_SetProgressState(0);
	gpConEmu->UpdateTitle();
	return S_OK;
}

HRESULT CDragDrop::DropNames(HDROP hDrop, int iQuantity, BOOL abActive)
{
	wchar_t* pszText = NULL;
	// -- HRESULT hr = S_OK;
	DWORD dwStartTick = GetTickCount();
#define OPER_TIMEOUT 5000
	BOOL lbAddGoto = (iQuantity == 1) && isPressed(VK_MENU);
	BOOL lbAddEdit = (iQuantity == 1) && isPressed(VK_CONTROL);
	BOOL lbAddView = (iQuantity == 1) && isPressed(VK_SHIFT);

	CVConGuard VCon;
	if (CVConGroup::GetActiveVCon(&VCon) < 0)
		return S_FALSE;
	CVirtualConsole* pVCon = VCon.VCon();
	if (!pVCon)
		return S_FALSE;
	CRealConsole* pRCon = pVCon->RCon();
	if (!pRCon)
		return S_FALSE;

	BOOL bNoFarConsole = m_pfpi->NoFarConsole;
	BOOL bDontAddSpace = FALSE;

	if (!bNoFarConsole && gpSet->isDropUseMenu && pRCon->isFar(true))
	{
		// Drop в ком.строку
		HMENU hPopup = CreatePopupMenu();
		enum {
			idNamesOnly = 1,
			idGoto,
			idEdit,
			idView,
		};
		AppendMenu(hPopup, MF_STRING | MF_ENABLED, idNamesOnly, L"Paste &no prefix");
		AppendMenu(hPopup, MF_STRING | (iQuantity == 1) ? MF_ENABLED : MF_GRAYED, idGoto,      L"&Goto prefix");
		AppendMenu(hPopup, MF_STRING | (iQuantity == 1) ? MF_ENABLED : MF_GRAYED, idEdit,      L"&Edit prefix");
		AppendMenu(hPopup, MF_STRING | (iQuantity == 1) ? MF_ENABLED : MF_GRAYED, idView,      L"&View prefix");

		POINT ptCur = {}; ::GetCursorPos(&ptCur);

		SetForegroundWindow(ghWnd);

		int nId = gpConEmu->mp_Menu->trackPopupMenu(tmp_PasteCmdLine, hPopup, TPM_LEFTALIGN|TPM_BOTTOMALIGN|TPM_RETURNCMD/*|TPM_NONOTIFY*/,
	                         ptCur.x,ptCur.y, ghWnd);
		DestroyMenu(hPopup);

		lbAddGoto = lbAddEdit = lbAddView = FALSE;

		switch (nId)
		{
		case idNamesOnly:
			break;
		case idGoto:
			lbAddGoto = TRUE;
			break;
		case idEdit:
			lbAddEdit = TRUE;
			break;
		case idView:
			lbAddView = TRUE;
			break;
		default:
			return S_FALSE;
		}

		dwStartTick = GetTickCount();
	}

	if (bNoFarConsole && pRCon->isFar(true) /*&& pRCon->isAlive()*/)
	{
		bNoFarConsole = FALSE; // таки послать макросом (это может быть редактор или диалог)
		bDontAddSpace = TRUE;
	}

	size_t cchMacro = MAX_DROP_PATH*2+50;
	wchar_t *szMacro = (wchar_t*)malloc(cchMacro*sizeof(*szMacro));
	size_t cchData = MAX_DROP_PATH*2+10;
	wchar_t *szData = (wchar_t*)malloc(cchData*sizeof(*szData));

	if (!szMacro || !szData)
	{
		_ASSERTE(szMacro && szData);
		SafeFree(szMacro);
		SafeFree(szData);
		return E_UNEXPECTED;
	}

	for (int i = 0 ; i < iQuantity; i++)
	{
		_wcscpy_c(szMacro, cchMacro, L"$Text ");
		_wcscpy_c(szData, cchData,  L"         ");
		pszText = szData + _tcslen(szData);
		UINT lQueryRc = DragQueryFile(hDrop, i, pszText, MAX_DROP_PATH);

		if (lQueryRc >= MAX_DROP_PATH) continue;

		wchar_t* psz = pszText;
		// обработка кавычек и слэшей
		//while ((psz = wcschr(psz, L'"')) != NULL) {
		//	*psz = L'\'';
		//}
		INT_PTR nLen = _tcslen(psz);

		if (!bNoFarConsole)
		{

			while (nLen>0 && *psz)
			{
				if (*psz == L'"' || *psz == L'\\')
				{
					INT_PTR cch = nLen+1;
					INT_PTR cchMax = cchData - ((psz+1) - szData); UNREFERENCED_PARAMETER(cchMax);
					_ASSERTE(cch > 0 && (INT_PTR)cch <= cchMax && cchMax > 0 && cchMax < (INT_PTR)cchData);
					wmemmove_s(psz+1, cchMax, psz, cch);

					if (*psz == L'"') *psz = L'\\';

					psz++;
				}

				psz++; nLen--;
			}
		}
		else
		{
			_ASSERTE(bNoFarConsole);
		}

		if ((psz = wcschr(pszText, L' ')) != NULL)
		{
			// Имя нужно окавычить
			if (!bNoFarConsole)
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

		if (!bNoFarConsole)
			_wcscat_c(szMacro, cchMacro, L"\"");

		if (!bNoFarConsole && (lbAddGoto || lbAddEdit || lbAddView))
		{
			if (lbAddGoto)
				_wcscat_c(szMacro, cchMacro, L"goto:");

			if (lbAddEdit)
				_wcscat_c(szMacro, cchMacro, L"edit:");

			if (lbAddView)
				_wcscat_c(szMacro, cchMacro, L"view:");

			lbAddGoto = FALSE; lbAddEdit = FALSE; lbAddView = FALSE;
		}

		if (!bNoFarConsole)
		{
			_wcscat_c(szMacro, cchMacro, pszText);
			_wcscat_c(szMacro, cchMacro, bDontAddSpace ? L"\"" : L" \"");
			gpConEmu->PostMacro(szMacro);
		}
		else
		{
			pRCon->Paste(false, pszText, true, false);
			//psz = pszText;
			////INPUT_RECORD r = {KEY_EVENT};
			//while (*psz)
			//{
			//	pRCon->PostKeyPress(0, 0, *(psz++));
			//	//r.Event.KeyEvent.bKeyDown = TRUE;
			//	//r.Event.KeyEvent.wRepeatCount = 1;
			//	//r.Event.KeyEvent.uChar.UnicodeChar = *(psz++);
			//	//pRCon->PostConsoleEvent(&r);
			//	//r.Event.KeyEvent.bKeyDown = FALSE;
			//	//pRCon->PostConsoleEvent(&r);
			//}

			if (!bDontAddSpace)
				pRCon->Paste(false, L" ", true, false);
				//pRCon->PostKeyPress(0, 0, L' ');
			//r.Event.KeyEvent.bKeyDown = TRUE;
			//r.Event.KeyEvent.wRepeatCount = 1;
			//r.Event.KeyEvent.uChar.UnicodeChar = L' ';
			//pRCon->PostConsoleEvent(&r);
			//r.Event.KeyEvent.bKeyDown = FALSE;
			//pRCon->PostConsoleEvent(&r);
		}

		if (((i + 1) < iQuantity)
		        && (GetTickCount() - dwStartTick) >= OPER_TIMEOUT)
		{
			DontEnable de;
			int nBtn = MessageBox(ghWnd, L"Drop operation is too long. Continue?", gpConEmu->GetDefaultTitle(), MB_YESNO|MB_ICONEXCLAMATION);

			if (nBtn != IDYES) break;

			dwStartTick = GetTickCount();
		}
	}

	SafeFree(szMacro);
	SafeFree(szData);
	return S_OK;
}

HRESULT CDragDrop::DropLinks(HDROP hDrop, int iQuantity, BOOL abActive)
{
	TCHAR curr[MAX_DROP_PATH];
	LPCTSTR pszTo = abActive ? m_pfpi->pszActivePath : m_pfpi->pszPassivePath;
	INT_PTR nToLen = _tcslen(pszTo);
	HRESULT hr = S_OK;

	for(int i = 0 ; i < iQuantity; i++)
	{
		int nLen = DragQueryFile(hDrop,i,curr,MAX_DROP_PATH);

		if (nLen <= 0 || nLen >= MAX_DROP_PATH) continue;

		LPCTSTR pszTitle = wcsrchr(curr, '\\');
		if (pszTitle) pszTitle++; else pszTitle = curr;

		nLen = nToLen+2+_tcslen(pszTitle)+4; //-V112
		TCHAR *szLnkPath = new TCHAR[nLen];
		_wcscpy_c(szLnkPath, nLen, pszTo);

		if (szLnkPath[_tcslen(szLnkPath)-1] != L'\\')
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

		delete[] szLnkPath;
	}

	//GlobalFree(hDrop);
	return S_OK;
}

HRESULT STDMETHODCALLTYPE CDragDrop::Drop(IDataObject * pDataObject,DWORD grfKeyState,POINTL pt,DWORD * pdwEffect)
{
	HRESULT hrHelper = S_FALSE; UNREFERENCED_PARAMETER(hrHelper);

	mb_DragWithinNow = FALSE;
	//DestroyDragImageBits();
//
	//#ifdef PERSIST_OVL
	//MoveDragWindow(FALSE);
	//#else
	//DestroyDragImageWindow();
	//#endif

	gpConEmu->SetDragCursor(NULL);
	//PostMessage(ghWnd, WM_SETCURSOR, -1, -1);
	#ifdef _DEBUG
	DWORD dwAllowed = *pdwEffect;
	#endif
	*pdwEffect = DROPEFFECT_COPY|DROPEFFECT_MOVE;

	POINT ppt = {pt.x,pt.y};

	#ifdef USE_DROP_HELPER
	if (UseTargetHelper(mb_selfdrag))
	{
		hrHelper = mp_TargetHelper->Drop(pDataObject, &ppt, *pdwEffect);
	}
	#endif

	if (S_OK != DragOverInt(grfKeyState, pt, pdwEffect) ||  *pdwEffect == DROPEFFECT_NONE)
	{
		return S_OK;
	}

	CVConGuard VCon;
	if (!CVConGroup::GetVConFromPoint(ppt, &VCon))
	{
		_ASSERTE(!mb_IsUpdatePackage || mpsz_UpdatePackage);
		if (mb_IsUpdatePackage && mpsz_UpdatePackage)
		{
			CConEmuUpdate::LocalUpdate(mpsz_UpdatePackage);
			return S_OK;
		}
		return S_FALSE;
	}

	// Определить, на какую панель бросаем
	HWND hWndDC = VCon->GetView();
	if (!hWndDC)
	{
		_ASSERTE(hWndDC!=NULL);
		return S_OK;
	}
	ScreenToClient(hWndDC, (LPPOINT)&pt);
	COORD cr = VCon->ClientToConsole(pt.x, pt.y);
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

	SetForegroundWindow(ghWnd);

	// Если тащат просто между панелями - сразу в FAR
	if (!lbDropFileNamesOnly && !lbActive && mb_selfdrag
	        && (*pdwEffect == DROPEFFECT_COPY || *pdwEffect == DROPEFFECT_MOVE))
	{
		//wchar_t *mcr = (wchar_t*)calloc(128, sizeof(wchar_t));
		////2010-02-18 Не было префикса '@'
		////2010-03-26 префикс '@' ставить нельзя, ибо тогда процесса копирования видно не будет при отсутствии подтверждения
		//// Если тянули ".." то перед копированием на другую панель сначала необходимо выйти на верхний уровень
		//lstrcpyW(mcr, L"$If (APanel.SelCount==0 && APanel.Current==\"..\") CtrlPgUp $End ");
		//// Теперь собственно клавиша запуска
		//lstrcatW(mcr, (*pdwEffect == DROPEFFECT_MOVE) ? L"F6" : L"F5");

		//// И если просили копировать сразу без подтверждения
		//if (gpSet->isDropEnabled==2)
		//	lstrcatW(mcr, L" Enter "); //$MMode 1");

		//gpConEmu->PostCopy(mcr);

		gpConEmu->PostDragCopy((*pdwEffect == DROPEFFECT_MOVE));

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
			//MBoxA(_T("Drag object does not contains known formats!"));
			ReportUnknownData(pDataObject, L"Drag object does not contains real file names!\nCan't drop here from containers like Zip or Outlook!");
			return S_OK;
		}

		// Выполнить Drop из Outlook, и др. ShellExtensions (через IStream)
		hr = DropFromStream(pDataObject, lbActive);
		return hr;
	}

	STGMEDIUM stgMediumMap = { 0 };
	FORMATETC fmtetcMap = { RegisterClipboardFormat(CFSTR_FILENAMEMAPW), 0, DVASPECT_CONTENT, -1, TYMED_HGLOBAL };
	hDrop = (HDROP)stgMedium.hGlobal;
	int iQuantity = DragQueryFile(hDrop,(UINT)-1,NULL,0);

	if (iQuantity < 1)
	{
		//GlobalFree(stgMedium.hGlobal);
		ReleaseStgMedium(&stgMedium);
		return S_OK; // ничего нет, выходим
	}

	DebugLog(_T("DnD: Drop starting"));

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
		INT_PTR nCount = MAX_DROP_PATH*iQuantity+iQuantity+1;
		INT_PTR nDstCount = 0;
		INT_PTR nDstFolderLen = 0;

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
			curr+=_tcslen(curr)+1;

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
					INT_PTR nNameLen = _tcslen(pszFileMap);
					lstrcpyn(dst, pszFileMap, MAX_PATH);
					pszFileMap += nNameLen+1;
					dst += _tcslen(dst)+1;
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
		DebugLog(_T("DnD: Shell operation starting"));
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

		DebugLog(NULL);
	}

	return S_OK; //1;
}

DWORD CDragDrop::ShellOpThreadProc(LPVOID lpParameter)
{
	DWORD dwThreadId = GetCurrentThreadId();
	ShlOpInfo *sfop = (ShlOpInfo *)lpParameter;
	int hr = S_OK;
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

	if (hr != NOERROR || sfop->fop.fAnyOperationsAborted)
	{
		if (hr == 0x402)
		{
			// 0x402 Ошибка возникает если нет доступа к сетевому диску, а тащат "снаружи".
			// Shell уже ругнулся по этому поводу, так что свою ошибку вроде показывать лишнее.
			// An unknown error occurred. This is typically due to an invalid path in the source or destination.
			// This error does not occur on Windows Vista and later.
		}
		else if (hr == 7 || hr == ERROR_CANCELLED || hr == NOERROR)
		{
			MBoxA(_T("Shell operation was cancelled"));
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
	//std::vector<ThInfo>::iterator iter = pDragDrop->m_OpThread.begin();

	//while (iter != pDragDrop->m_OpThread.end())
	for (INT_PTR j = 0; j < pDragDrop->m_OpThread.size(); j++)
	{
		const ThInfo* iter = &(pDragDrop->m_OpThread[j]);

		if (dwThreadId == iter->dwThreadId)
		{
			hTh = iter->hThread;
			//iter = pDragDrop->m_OpThread.erase(iter);
			pDragDrop->m_OpThread.erase(j);
			break;
		}

		//++iter;
	}

	LeaveCriticalSection(&pDragDrop->m_CrThreads);

	if (hTh)
		CloseHandle(hTh);

	return 0;
}

HRESULT STDMETHODCALLTYPE CDragDrop::DragOver(DWORD grfKeyState,POINTL pt,DWORD * pdwEffect)
{
	// Drag over inactive pane?
	if (NeedRefreshToInfo(pt))
	{
		DEBUGSTRFAR(L"DragOver: NeedRefreshToInfo -> RetrieveDragToInfo");
		RetrieveDragToInfo(pt);
	}
	else
	{
		DEBUGSTRFAR(L"DragOver: skipping RetrieveDragToInfo");
	}

	HRESULT hrHelper = S_FALSE; UNREFERENCED_PARAMETER(hrHelper);
	HRESULT hr = DragOverInt(grfKeyState, pt, pdwEffect);

	#ifdef USE_DROP_HELPER
	if (UseTargetHelper(mb_selfdrag))
	{
		POINT ppt = {pt.x,pt.y};
		hrHelper = mp_TargetHelper->DragOver(&ppt, *pdwEffect);
		UNREFERENCED_PARAMETER(hrHelper);
	}
	#endif

	return hr;
}

HRESULT CDragDrop::DragOverInt(DWORD grfKeyState,POINTL pt,DWORD * pdwEffect)
{
	HRESULT hr = S_OK;
	DWORD dwAllowed = *pdwEffect;
	CVConGuard VCon;

	if (mb_IsUpdatePackage && !mp_LastRCon)
	{
		// OK, Запустить AutoUpdate c заранее загруженным пакетом
		*pdwEffect = DROPEFFECT_COPY;
	}
	else if (!gpSet->isDropEnabled && !gpConEmu->isDragging())
	{
		*pdwEffect = DROPEFFECT_NONE;
		DebugLog(_T("DnD: Drop disabled"));
		hr = S_FALSE;
	}
	else if (m_pfpi==NULL)
	{
		*pdwEffect = DROPEFFECT_NONE;
	}
	else if (CVConGroup::GetActiveVCon(&VCon) < 0)
	{
		*pdwEffect = DROPEFFECT_NONE;
		DebugLog(_T("DnD: No Active VCon"));
		hr = S_FALSE;
	}
	else
	{
		if (mp_DataObject == mp_DroppedObject)
		{
			mb_selfdrag = (mp_DraggedVCon == VCon.VCon());
		}

		TODO("Если drop идет ПОД панели - впечатать путь в командную строку");
		DEBUGSTRSTEP(_T("DnD: DragOverInt starting"));
		POINT ptCur; ptCur.x = pt.x; ptCur.y = pt.y;
#ifdef _DEBUG
		GetCursorPos(&ptCur);
#endif
		RECT rcDC;
		HWND hWndDC = VCon->GetView();
		if (!hWndDC)
		{
			_ASSERTE(hWndDC!=NULL);
			*pdwEffect = DROPEFFECT_NONE;
		}
		else
		{
			GetWindowRect(hWndDC, &rcDC);

			//HWND hWndFrom = WindowFromPoint(ptCur);
			if (/*(hWndFrom != ghWnd && hWndFrom != mh_Overlapped)
				||*/ !PtInRect(&rcDC, ptCur))
			{
				*pdwEffect = DROPEFFECT_NONE;
			}
			else
			{
				ScreenToClient(hWndDC, (LPPOINT)&pt);
				COORD cr = VCon->ClientToConsole(pt.x, pt.y);
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
					else if ((m_pfpi->ActiveRect.bottom && pt.y > m_pfpi->ActiveRect.bottom && pt.x >= m_pfpi->ActiveRect.left && pt.x <= m_pfpi->ActiveRect.right)
							|| (m_pfpi->PassiveRect.bottom && pt.y > m_pfpi->PassiveRect.bottom && pt.x >= m_pfpi->PassiveRect.left && pt.x <= m_pfpi->PassiveRect.right)
							|| (!m_pfpi->ActiveRect.bottom && !m_pfpi->PassiveRect.bottom))
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
	}

#ifdef _DEBUG
	wchar_t szDbgInfo[128];
	_wsprintf(szDbgInfo, SKIPLEN(countof(szDbgInfo)) L"CDragDrop::DragOverInt(%i,%i) -> %s\n",
		pt.x, pt.y,
		(*pdwEffect == DROPEFFECT_NONE) ? L"DROPEFFECT_NONE" :
		(*pdwEffect == DROPEFFECT_COPY) ? L"DROPEFFECT_COPY" :
		(*pdwEffect == DROPEFFECT_LINK) ? L"DROPEFFECT_LINK" : L"!!!UNKNOWN!!!");
	DEBUGSTROVERINT(szDbgInfo);
#endif

	//if (!mb_selfdrag)
	//{
	//	if (*pdwEffect == DROPEFFECT_NONE)
	//	{
	//		MoveDragWindow(FALSE);
	//	}
	//	else if (mh_Overlapped && mp_Bits)
	//	{
	//		MoveDragWindow(!UseTargetHelper(mb_selfdrag));
	//	}
	//	else if (mp_Bits)
	//	{
	//		#if defined(USE_CHILD_OVL)
	//		if (!mh_Overlapped)
	//			CreateDragImageWindow();
	//		#elif defined(PERSIST_OVL)
	//		_ASSERTE(mh_Overlapped); // должно быть создано при запуске ConEmu
	//		#else
	//		CreateDragImageWindow();
	//		#endif
	//	}
	//}

	DEBUGSTRSTEP(_T("DnD: DragOverInt ok"));
	return hr;
}

HRESULT STDMETHODCALLTYPE CDragDrop::DragEnter(IDataObject * pDataObject,DWORD grfKeyState,POINTL pt,DWORD * pdwEffect)
{
	HRESULT hrHelper = S_FALSE; UNREFERENCED_PARAMETER(hrHelper);
	DEBUGSTROVER(L"CDragDrop::DragEnter() called\n");

	mp_DroppedObject = pDataObject;
	mb_selfdrag = (pDataObject == mp_DataObject);
	mb_DragWithinNow = TRUE;

	if (gbDebugLogStarted
		#ifdef DUMP_DRAGGED_ITEMS_INFO
		|| IsDebuggerPresent()
		#endif
		)
	{
		EnumDragFormats(pDataObject);
	}

	CheckIsUpdatePackage(pDataObject);

	//bool bNeedLoadImg = false;

	// При "DragEnter" считывать информацию из фара нужно всегда
	if (gpSet->isDropEnabled /*&& !mb_selfdrag*/ /*&& NeedRefreshToInfo(pt)*/)
	{
		DEBUGSTRFAR(L"DragEnter: NeedRefreshToInfo -> RetrieveDragToInfo");
		// при "своем" драге - информация уже получена
		RetrieveDragToInfo(pt);
	}
	else if (!m_pfpi)
	{
		DEBUGSTRFAR(L"DragEnter: (!m_pfpi) -> RetrieveDragToInfo");
		_ASSERTE(m_pfpi!=NULL); // Must be retrieved already!
		RetrieveDragToInfo(pt);
	}
	else
	{
		DEBUGSTRFAR(L"DragEnter: skipping RetrieveDragToInfo");
	}

	// Скорректировать допустимые действия
	DragOverInt(grfKeyState, pt, pdwEffect);

	if (gpSet->isDropEnabled || mb_selfdrag)
	{
		if (!mb_selfdrag)
		{
			//// при "своем" драге - информация уже получена
			//RetrieveDragToInfo();

			//// Скорректировать допустимые действия
			//DragOverInt(grfKeyState, pt, pdwEffect);

			//if (LoadDragImageBits(pDataObject))
			//{
			//	#ifdef PERSIST_OVL
			//	if (!mh_Overlapped || !IsWindow(mh_Overlapped))
			//	{
			//		_ASSERTE(mh_Overlapped); // должно быть создано при запуске ConEmu
			//		mh_Overlapped = NULL;
			//		CreateDragImageWindow();
			//	}
			//	else
			//	{
			//		InvalidateRect(mh_Overlapped, NULL, TRUE);
			//	}
			//	#else
			//	CreateDragImageWindow();
			//	#endif
			//}
		}
	}
	else
	{
		DebugLog(_T("DnD: Drop disabled"));
	}

	#ifdef USE_DROP_HELPER
	if (UseTargetHelper(mb_selfdrag))
	{
		#ifdef UNLOCKED_DRAG
		PRAGMA_ERROR("UNLOCKED_DRAG - проверить HWND");
		#endif

		POINT ppt = {pt.x,pt.y};
		hrHelper = mp_TargetHelper->DragEnter(ghWnd, pDataObject, &ppt, *pdwEffect);
	}
	#endif

	return 0;
}

HRESULT STDMETHODCALLTYPE CDragDrop::DragLeave(void)
{
	HRESULT hrHelper = S_FALSE; UNREFERENCED_PARAMETER(hrHelper);
	DEBUGSTROVER(L"CDragDrop::DragLeave() called\n");

	mb_DragWithinNow = FALSE;

	//if (mb_selfdrag)
	//{
	//	MoveDragWindow(FALSE);
	//}
	//else
	//{
	//	DestroyDragImageBits();
//
	//	#ifdef PERSIST_OVL
	//	MoveDragWindow(FALSE);
	//	#else
	//	DestroyDragImageWindow();
	//	#endif
	//}

	#ifdef USE_DROP_HELPER
	if (UseTargetHelper(mb_selfdrag))
	{
		hrHelper = mp_TargetHelper->DragLeave();
	}
	#endif

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

void CDragDrop::ReportUnknownData(IDataObject * pDataObject, LPCWSTR sUnknownError)
{
	HANDLE hFile = NULL;
	size_t nLen = _tcslen(sUnknownError);
	wchar_t* pszMsg = (wchar_t*)calloc((nLen+256),sizeof(wchar_t));
	lstrcpy(pszMsg, sUnknownError);
	lstrcpy(pszMsg+nLen, L"\n\nPress 'Retry' to create report for developer");
	int nBtn = 0;
	{
		DontEnable de;
		nBtn = (int)MessageBox(ghWnd, pszMsg, gpConEmu->GetDefaultTitle(),
	                           MB_SYSTEMMODAL|MB_ICONINFORMATION|MB_RETRYCANCEL|MB_DEFBUTTON2);
	}
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
