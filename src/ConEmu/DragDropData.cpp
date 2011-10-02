
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
#include "DragDropData.h"
#include "DragDrop.h"
#include "ScreenDump.h"
#include "ConEmu.h"
#include "ConEmuPipe.h"
#include "VirtualConsole.h"
#include "RealConsole.h"
//#include "../common/ConEmuCheck.h"

#define MAX_OVERLAY_WIDTH    300
#define MAX_OVERLAY_HEIGHT   300
#define OVERLAY_ALPHA        0xAA
#define OVERLAY_TEXT_SHIFT   (gpSet->isDragShowIcons ? 18 : 0)
#define OVERLAY_COLUMN_SHIFT 5
#define OVERLAY_COLOR  RGB(254,0,254)

#define DEBUGSTROVL(s) //DEBUGSTR(s)
#define DEBUGSTRBACK(s) //DEBUGSTR(s)

//#define MSG_STARTDRAG (WM_APP+10)

WARNING("Заменить GlobalFree на ReleaseStgMedium, проверить, что оно реально освобождает hGlobal и кучу не рушит");

TODO("Попробовать перевести DragImage на нормальный интерфейс API");
// можно оставить Overlapped для отображения статуса драга
// однако - были какие-то проблемы. Не всплывало окошко при драге во внешние приложения?

CDragDropData::CDragDropData()
{
	m_pfpi = NULL;
	mp_DataObject = NULL;
	m_DesktopID.mkid.cb = 0;
	mh_Overlapped = NULL; mh_BitsDC = NULL; mh_BitsBMP = mh_BitsBMP_Old = NULL;
	mp_Bits = NULL;
	mb_DragWithinNow = FALSE;
	mn_ExtractIconsTID = 0;
	mh_ExtractIcons = NULL;
	mb_DragDropRegistered = FALSE;
	/* Unlocked drag support */
	mb_DragStarting = FALSE;
	ms_SourceClass[0] = 0; mh_SourceClass = NULL;
}

CDragDropData::~CDragDropData()
{
	DestroyDragImageBits();
	DestroyDragImageWindow();

	if (m_pfpi) free(m_pfpi); m_pfpi=NULL;
}

BOOL CDragDropData::Register()
{
	BOOL lbRc = FALSE;
	HRESULT hr = S_OK;
#ifdef UNLOCKED_DRAG
	// Инициалиция для Drag
	InitialCreateSource();
	// Окно создаем сразу, чтобы фокус никуда не уходил в процессе
	CreateDragImageWindow();
	//hr = CoCreateInstance(CLSID_DragDropHelper, NULL, CLSCTX_INPROC_SERVER, IID_IUnknown, (void**)&pHelper);
	//if (pHelper) {
	//	hr = pHelper->QueryInterface(__uuidof(mp_SourceHelper), (void**)&mp_SourceHelper);
	//	hr = pHelper->QueryInterface(__uuidof(mp_TargetHelper), (void**)&mp_TargetHelper);
	//	pHelper->Release();
	//	pHelper = NULL;
	//}
#endif
	_ASSERTE(gpConEmu->mp_DragDrop);
	hr = RegisterDragDrop(ghWnd, gpConEmu->mp_DragDrop);

	if (hr != S_OK)
	{
		DisplayLastError(L"Can't register Drop target", hr);
		goto wrap;
	}
	else
	{
		mb_DragDropRegistered = TRUE;
	}

	lbRc = TRUE;
wrap:
	return lbRc;
}

// -1 - ошибка
// иначе - размер ASCIIZZ путей файлов в БАЙТАХ
int CDragDropData::RetrieveDragFromInfo(BOOL abClickNeed, COORD crMouseDC, wchar_t** ppszDraggedPath, UINT* pnFilesCount)
{
	int size = -1;
	UINT nFilesCount = 0;
	wchar_t *szDraggedPath = NULL;
	//BOOL lbNeedLBtnUp = !abClickNeed;
	CConEmuPipe pipe(gpConEmu->GetFarPID(), CONEMUREADYTIMEOUT);
	MCHKHEAP

	if (m_pfpi) {free(m_pfpi); m_pfpi=NULL;}

	if (pipe.Init(_T("CDragDropData::Drag")))
	{
		//DWORD cbWritten=0;
		struct
		{
			BOOL  bClickNeed;
			COORD crMouse;
		} DragArg;
		DragArg.bClickNeed = abClickNeed;
		DragArg.crMouse = gpConEmu->ActiveCon()->RCon()->ScreenToBuffer(
		                      gpConEmu->ActiveCon()->ClientToConsole(crMouseDC.X, crMouseDC.Y)
		                  );

		if (pipe.Execute(CMD_DRAGFROM, &DragArg, sizeof(DragArg)))
		{
			//lbNeedLBtnUp = FALSE; // мышка уже обработана в плагине
			gpConEmu->DebugStep(_T("DnD: Recieving data"));
			DWORD cbBytesRead=0;
			int nWholeSize=0;
			pipe.Read(&nWholeSize, sizeof(nWholeSize), &cbBytesRead);

			if (nWholeSize==0)    // защита смены формата
			{
				if (!pipe.Read(&nWholeSize, sizeof(nWholeSize), &cbBytesRead))
					nWholeSize = 0;
			}
			else
			{
				nWholeSize=0;
			}

			MCHKHEAP

			if (nWholeSize<=0)
			{
				gpConEmu->DebugStep(_T("DnD: Data is empty"), TRUE);
			}
			else
			{
				gpConEmu->DebugStep(_T("DnD: Recieving data..."));
				//mpsz_DraggedPath <-- wchar_t *szDraggedPath=NULL; //ASCIIZZ
				szDraggedPath = new wchar_t[nWholeSize/*(MAX_PATH+1)*FilesCount*/+1];
				ZeroMemory(szDraggedPath, /*((MAX_PATH+1)*FilesCount+1)*/(nWholeSize+1)*sizeof(wchar_t));
				wchar_t  *curr = szDraggedPath;

				for(;;)
				{
					int nCurSize=0;

					if (!pipe.Read(&nCurSize, sizeof(nCurSize), &cbBytesRead)) break;

					if (nCurSize==0) break;

					pipe.Read(curr, sizeof(WCHAR)*nCurSize, &cbBytesRead);
					_ASSERTE(*curr);
					curr+=_tcslen(curr)+1;
					nFilesCount ++;
				}

				int cbStructSize=0;

				if (m_pfpi) {free(m_pfpi); m_pfpi=NULL;}

				if (pipe.Read(&cbStructSize, sizeof(int), &cbBytesRead))
				{
					if ((DWORD)cbStructSize>sizeof(ForwardedPanelInfo))
					{
						m_pfpi = (ForwardedPanelInfo*)calloc(cbStructSize, 1);
						pipe.Read(m_pfpi, cbStructSize, &cbBytesRead);
						m_pfpi->pszActivePath = (WCHAR*)(((char*)m_pfpi)+m_pfpi->ActivePathShift);
						//Slash на конце нам не нужен
						int nPathLen = lstrlenW(m_pfpi->pszActivePath);

						if (nPathLen>0 && m_pfpi->pszActivePath[nPathLen-1]==_T('\\'))
							m_pfpi->pszActivePath[nPathLen-1] = 0;

						m_pfpi->pszPassivePath = (WCHAR*)(((char*)m_pfpi)+m_pfpi->PassivePathShift);
						//Slash на конце нам не нужен
						nPathLen = lstrlenW(m_pfpi->pszPassivePath);

						if (nPathLen>0 && m_pfpi->pszPassivePath[nPathLen-1]==_T('\\'))
							m_pfpi->pszPassivePath[nPathLen-1] = 0;
					}
				}

				// Сразу закроем
				pipe.Close();
				size=(curr-szDraggedPath)*sizeof(wchar_t)+2;
			}
		}
	}

	*ppszDraggedPath = szDraggedPath;
	*pnFilesCount = nFilesCount;
	return size;
}

BOOL CDragDropData::AddFmt_FileNameW(wchar_t* pszDraggedPath, UINT nFilesCount, int cbSize)
{
	HGLOBAL file_nameW = NULL;
	file_nameW = GlobalAlloc(GPTR, cbSize);
	memcpy(((WCHAR*)file_nameW), pszDraggedPath, cbSize);
	FORMATETC       fmtetc[] =
	{
		{ RegisterClipboardFormat(L"FileNameW"), 0, DVASPECT_CONTENT, -1, TYMED_HGLOBAL }
	};
	STGMEDIUM       stgmed[] =
	{
		{ TYMED_HGLOBAL, { (HBITMAP)file_nameW }, 0 }
	};
	mp_DataObject->SetData(fmtetc, stgmed, TRUE);
	return (file_nameW!=NULL);
}

BOOL CDragDropData::AddFmt_SHELLIDLIST(wchar_t* pszDraggedPath, UINT nFilesCount, int cbSize)
{
	BOOL lbAdded = FALSE;
	CIDA* file_PIDLs = NULL;
	DWORD nParentSize = 0;
	HRESULT hr = S_OK;
	UINT nAdded = 0;
	IShellFolder *pDesktop = NULL;
	//if (nFilesCount !=1 )
	//	goto wrap;
	// Прикидочно определим, сколько понадобится памяти для хранения всех PIDL
	nParentSize = ILGetSize(&m_DesktopID); _ASSERTE(nParentSize==2);
	size_t nMaxSize = cbSize + nFilesCount * 64 + 32; //-V112
	// в nCurSize сразу резервируем размер памяти под заголовок (CIDA с переменным aoffset)
	size_t nCurSize = sizeof(CIDA)+nFilesCount*sizeof(UINT);
	file_PIDLs = (CIDA*)GlobalAlloc(GPTR, nMaxSize);

	if (!file_PIDLs)
	{
		gpConEmu->DebugStep(_T("DnD: Can't allocate CIDA*"), TRUE);
		goto wrap;
	}

	// First - root folder PIDL (Desktop)
	file_PIDLs->cidl = 0; // будем инкрементить, когда понадобится
	file_PIDLs->aoffset[0] = nCurSize;
	memmove((((LPBYTE)file_PIDLs)+nCurSize), &m_DesktopID, nParentSize);
	nCurSize += nParentSize;
	_try
	{
		// Сначала нужно получить Interface для Desktop
		SFGAOF tmp;
		LPITEMIDLIST pItem = NULL;
		DWORD nItemSize = 0;


		MCHKHEAP

		hr = SHGetDesktopFolder(&pDesktop);

		for(UINT i = 0; pDesktop && i < nFilesCount; i++)
		{
			wchar_t* pszFile = pszDraggedPath;
			pszDraggedPath += lstrlenW(pszDraggedPath)+1;
			_ASSERTE(pszFile[0]);
			const wchar_t* pszSlash = wcsrchr(pszFile, L'\\');

			if (!pszSlash)
				continue;

			// теперь собственно файл/папка...
			ULONG nEaten=0;
			hr = pDesktop->ParseDisplayName(ghWnd, NULL, pszFile, &nEaten, &pItem, &(tmp=0));

			if (hr == S_OK && pItem /*&& mp_DesktopID*/)
			{
				nItemSize = ILGetSize((PCUIDLIST_RELATIVE)pItem);
				MCHKHEAP;

				if ((nCurSize + nItemSize) > nMaxSize)
				{
					nMaxSize += nItemSize;
					CIDA* pNew = (CIDA*)GlobalAlloc(GPTR, nMaxSize);
					memmove(pNew, file_PIDLs, nCurSize);
					GlobalFree(file_PIDLs);
					file_PIDLs = pNew;
					MCHKHEAP;
				}

				// Note, root folder not counted in file_PIDLs->cidl
				file_PIDLs->cidl = (++nAdded);
				file_PIDLs->aoffset[nAdded] = nCurSize;
				memmove((((LPBYTE)file_PIDLs)+nCurSize), pItem, nItemSize);
				nCurSize += nItemSize;
				MCHKHEAP;
			}

			if (pItem)
			{
				CoTaskMemFree(pItem); pItem = NULL;
			}
		}
	}
	_except(EXCEPTION_EXECUTE_HANDLER)
	{
		hr = (HRESULT)-1;
	}

	if (hr == (HRESULT)-1)
	{
		// file_PIDLs освободится после wrap;
		gpConEmu->DebugStep(_T("DnD: Exception in shell"), TRUE);
		goto wrap;
	}

	if (file_PIDLs->cidl == 0)
	{
		gpConEmu->DebugStep(_T("DnD: Failed to create ShellIdList"), TRUE);
		goto wrap;
	}

	// Добавляем
	FORMATETC       fmtetc[] =
	{
		{ RegisterClipboardFormat(CFSTR_SHELLIDLIST), 0, DVASPECT_CONTENT, -1, TYMED_HGLOBAL},
	};
	STGMEDIUM       stgmed[] =
	{
		{ TYMED_HGLOBAL, { (HBITMAP)file_PIDLs }, 0 }
	};
	mp_DataObject->SetData(fmtetc, stgmed, TRUE);
	lbAdded = TRUE;
	file_PIDLs = NULL;
wrap:

	if (pDesktop) { pDesktop->Release(); pDesktop = NULL; }

	if (file_PIDLs) { GlobalFree(file_PIDLs); file_PIDLs = NULL; }

	return lbAdded;
}

BOOL CDragDropData::AddFmt_PREFERREDDROPEFFECT(wchar_t* pszDraggedPath, UINT nFilesCount, int cbSize)
{
	HGLOBAL drop_preferredeffect = GlobalAlloc(GPTR, sizeof(DWORD));

	if (gpConEmu->mouse.state & DRAG_R_STARTED)
		*((DWORD*)drop_preferredeffect) = DROPEFFECT_LINK;
	else if (gpSet->isDefCopy)
		*((DWORD*)drop_preferredeffect) = DROPEFFECT_COPY;
	else
		*((DWORD*)drop_preferredeffect) = DROPEFFECT_MOVE;

	FORMATETC       fmtetc[] =
	{
		{ RegisterClipboardFormat(CFSTR_PREFERREDDROPEFFECT), 0, DVASPECT_CONTENT, -1, TYMED_HGLOBAL },
	};
	STGMEDIUM       stgmed[] =
	{
		{ TYMED_HGLOBAL, { (HBITMAP)drop_preferredeffect }, 0 },
	};
	mp_DataObject->SetData(fmtetc, stgmed, TRUE);
	return TRUE;
}

BOOL CDragDropData::AddFmt_InShellDragLoop(wchar_t* pszDraggedPath, UINT nFilesCount, int cbSize)
{
	HGLOBAL drag_loop = GlobalAlloc(GPTR, sizeof(DWORD));
	*((DWORD*)drag_loop) = 1;
	FORMATETC       fmtetc[] =
	{
		{ RegisterClipboardFormat(L"InShellDragLoop"), 0, DVASPECT_CONTENT, -1, TYMED_HGLOBAL },
	};
	STGMEDIUM       stgmed[] =
	{
		{ TYMED_HGLOBAL, { (HBITMAP)drag_loop }, 0 },
	};
	mp_DataObject->SetData(fmtetc, stgmed, TRUE);
	return TRUE;
}

BOOL CDragDropData::AddFmt_HDROP(wchar_t* pszDraggedPath, UINT nFilesCount, int cbSize)
{
	DROPFILES drop_struct = { sizeof(drop_struct), { 0, 0 }, 0, 1 };
	HGLOBAL drop_data = GlobalAlloc(GPTR, cbSize+sizeof(drop_struct));
	_ASSERTE(drop_data);

	if (!drop_data)
	{
		gpConEmu->DebugStep(_T("DnD: Memory allocation failed!"), TRUE);
		goto wrap;
	}

	memcpy(drop_data, &drop_struct, sizeof(drop_struct));
	memcpy(((byte*)drop_data) + sizeof(drop_struct), pszDraggedPath, cbSize);
	FORMATETC       fmtetc[] =
	{
		{ CF_HDROP, 0, DVASPECT_CONTENT, -1, TYMED_HGLOBAL },
	};
	STGMEDIUM       stgmed[] =
	{
		{ TYMED_HGLOBAL, { (HBITMAP)drop_data }, 0 },
	};
	mp_DataObject->SetData(fmtetc, stgmed, TRUE);
wrap:
	return TRUE;
}

BOOL CDragDropData::AddFmt_DragImageBits(wchar_t* pszDraggedPath, UINT nFilesCount, int cbSize)
{
	// Must be GlobalAlloc'ed
	DragImageBits *pDragBits = CreateDragImageBits(pszDraggedPath);

	if (!pDragBits)
		return FALSE;

	FORMATETC       fmtetc[] =
	{
		{ RegisterClipboardFormat(L"DragImageBits"), 0, DVASPECT_CONTENT, -1, TYMED_HGLOBAL },
	};
	STGMEDIUM       stgmed[] =
	{
		{ TYMED_HGLOBAL, { (HBITMAP)pDragBits }, 0 },
	};
	mp_DataObject->SetData(fmtetc, stgmed, TRUE);
	return TRUE;
}

// Загрузить из фара информацию для Drag
BOOL CDragDropData::PrepareDrag(BOOL abClickNeed, COORD crMouseDC, DWORD* pdwAllowedEffects)
{
	BOOL lbSucceeded = FALSE;
	int  size = -1;
	wchar_t *szDraggedPath = NULL; // ASCIIZZ
	UINT nFilesCount = 0;

	if (!gpSet->isDragEnabled /*|| isInDrag */|| gpConEmu->isDragging())
	{
		gpConEmu->DebugStep(_T("DnD: Drag disabled"), TRUE);
		goto wrap;
	}

	_ASSERTE(!gpConEmu->isPictureView());
	//gpConEmu->isDragProcessed=true; // чтобы не сработало два раза на один драг
	gpConEmu->mouse.state |= (gpConEmu->mouse.state & DRAG_R_ALLOWED) ? DRAG_R_STARTED : DRAG_L_STARTED;
	MCHKHEAP;
	size = RetrieveDragFromInfo(abClickNeed, crMouseDC, &szDraggedPath, &nFilesCount);
	MCHKHEAP;

	if (size <= 0 || nFilesCount == 0)
	{
		wchar_t szInfo[128]; _wsprintf(szInfo, SKIPLEN(countof(szInfo)) L"DnD: RetrieveDragFromInfo, Code=%i, nFilesCount=%i!", size, nFilesCount);
		gpConEmu->DebugStep(szInfo, TRUE);
		goto wrap;
	}

	*pdwAllowedEffects = DROPEFFECT_COPY|DROPEFFECT_MOVE;
	// Создать IDataObject (пока пустой)
	CreateDataObject(NULL, NULL, 0, &mp_DataObject) ;

	if (!mp_DataObject)
	{
		gpConEmu->DebugStep(_T("DnD: CreateDataObject failed!"), TRUE);
		goto wrap;
	}

	// Добавление в mp_DataObject перетаскиваемых форматов
	AddFmt_HDROP(szDraggedPath, nFilesCount, size);
	AddFmt_PREFERREDDROPEFFECT(szDraggedPath, nFilesCount, size);
	AddFmt_InShellDragLoop(szDraggedPath, nFilesCount, size);
	AddFmt_DragImageBits(szDraggedPath, nFilesCount, size);

	// Эти шли последними
	if (AddFmt_SHELLIDLIST(szDraggedPath, nFilesCount, size))
		*pdwAllowedEffects |= DROPEFFECT_LINK;

	AddFmt_FileNameW(szDraggedPath, nFilesCount, size);
	MCHKHEAP
	//Освободить память, больше не нужно
	delete szDraggedPath; szDraggedPath=NULL;

	//CFSTR_PREFERREDDROPEFFECT, FD_LINKUI

	// Сразу получим информацию о путях панелей...
	if (!m_pfpi)  // если это уже не сделали
	{
		// Ошибки пайпа отображаются через MBoxA
		RetrieveDragToInfo();
	}

	if (LoadDragImageBits(mp_DataObject))
	{
		mb_DragWithinNow = TRUE;
#ifdef UNLOCKED_DRAG
		// должно быть создано при запуске ConEmu
		_ASSERTE(mh_Overlapped);
#else
		CreateDragImageWindow();
#endif
	}

	//gpConEmu->DebugStep(_T("DnD: Finally, DoDragDrop"));
	gpConEmu->DebugStep(NULL);
	lbSucceeded = TRUE;
wrap:
	return lbSucceeded;
}

template <class T> LPITEMIDLIST PidlGetNextItem(
    __in T pidl
)
{
	if (pidl)
	{
		return (LPITEMIDLIST)(LPBYTE)(((LPBYTE)pidl) + pidl->mkid.cb);
	}
	else
		return NULL;
};
template <class T> void PidlDump(
    __in T pidl, HANDLE hDumpFile = NULL
)
{
	LPITEMIDLIST p = (LPITEMIDLIST)(pidl);
	char szDump[512];
	int nLevel = 0, nLen;

	while(p && p->mkid.cb)
	{
		_wsprintfA(szDump, SKIPLEN(countof(szDump)) "%i: ", nLevel+1);
		int i = lstrlenA(szDump);
		//for (int j=0; j < nLevel; j++) { szDump[i++] = ' '; szDump[i++] = ' '; }
		//szDump[i] = 0;
		nLen = min(255,p->mkid.cb);

		for(int j = 0; j < nLen; j++)
		{
			switch(p->mkid.abID[j])
			{
				case 0: szDump[i++] = '.'; break;
				case '\n': szDump[i++] = '\\'; szDump[i++] = 'n'; break;
				case '\r': szDump[i++] = '\\'; szDump[i++] = 'r'; break;
				case '\t': szDump[i++] = '\\'; szDump[i++] = 't'; break;
				case '\\': szDump[i++] = '\\'; szDump[i++] = '\\'; break;
				default: szDump[i++] = p->mkid.abID[j];
			}
		}

		szDump[i++] = '\n';
		szDump[i++] = 0;

		if (hDumpFile)
		{
			wchar_t szDumpW[512]; DWORD nWritten;
			MultiByteToWideChar(CP_ACP, 0, szDump, -1, szDumpW, 512);
			WriteFile(hDumpFile, szDumpW, (DWORD)_tcslen(szDumpW)*2, &nWritten, NULL);
		}
		else
		{
			OutputDebugStringA(szDump);
		}

		p = (LPITEMIDLIST)(LPBYTE)(((LPBYTE)p) + p->mkid.cb);
		nLevel++;
	}
}

// Если передан hDumpFile - запись полной информации в текстовый файл
void CDragDropData::EnumDragFormats(IDataObject * pDataObject, HANDLE hDumpFile /*= NULL*/)
{
	//BOOL lbDoEnum = FALSE;
	//if (!lbDoEnum) return;
	HRESULT hr = S_OK;
	IEnumFORMATETC *pEnum = NULL;
	FORMATETC fmt[20];
	STGMEDIUM stg[20];
	LPCWSTR psz[20];
	SIZE_T memsize[20];
	wchar_t szName[20][MAX_PATH*2];
	LPWSTR pszData[20];
	ULONG nCnt = countof(fmt);
	UINT i;
	DWORD nWritten;
	memset(fmt, 0, sizeof(fmt));
	memset(stg, 0, sizeof(stg));
	memset(psz, 0, sizeof(psz));
	memset(pszData, 0, sizeof(pszData));

	if (hDumpFile) WriteFile(hDumpFile, "\xFF\xFE", 2, &nWritten, NULL);

	hr = pDataObject->EnumFormatEtc(DATADIR_GET,&pEnum);

	if (hr==S_OK)
	{
		hr = pEnum->Next(nCnt, fmt, &nCnt);
		_ASSERTE(nCnt <= countof(fmt));

		if (hDumpFile)
		{
			_wsprintf(szName[0], SKIPLEN(countof(szName[0])) L"Drag object contains %i formats\n\n", nCnt);
			WriteFile(hDumpFile, szName[0], (DWORD)_tcslen(szName[0])*2, &nWritten, NULL);
		}

		pEnum->Release();
		CLIPFORMAT cfShlPidl = RegisterClipboardFormat(CFSTR_SHELLIDLIST);

		/*
		CFSTR_PREFERREDDROPEFFECT ?
		*/
		for(i=0; i<nCnt; i++)
		{
			szName[i][0] = 0;

			if (!GetClipboardFormatName(fmt[i].cfFormat, szName[i], MAX_PATH))
			{
				switch(fmt[i].cfFormat)
				{
					case 1: wcscpy_c(szName[i], L"CF_TEXT"); break;
					case 2: wcscpy_c(szName[i], L"CF_BITMAP"); break;
					case 3: wcscpy_c(szName[i], L"CF_METAFILEPICT"); break;
					case 4: wcscpy_c(szName[i], L"CF_SYLK"); break;
					case 5: wcscpy_c(szName[i], L"CF_DIF"); break;
					case 6: wcscpy_c(szName[i], L"CF_TIFF"); break;
					case 7: wcscpy_c(szName[i], L"CF_OEMTEXT"); break;
					case 8: wcscpy_c(szName[i], L"CF_DIB"); break;
					case 9: wcscpy_c(szName[i], L"CF_PALETTE"); break;
					case 10: wcscpy_c(szName[i], L"CF_PENDATA"); break;
					case 11: wcscpy_c(szName[i], L"CF_RIFF"); break;
					case 12: wcscpy_c(szName[i], L"CF_WAVE"); break;
					case 13: wcscpy_c(szName[i], L"CF_UNICODETEXT"); break;
					case 14: wcscpy_c(szName[i], L"CF_ENHMETAFILE"); break;
					case 15: wcscpy_c(szName[i], L"CF_HDROP"); break;
					case 16: wcscpy_c(szName[i], L"CF_LOCALE"); break;
					case 17: wcscpy_c(szName[i], L"CF_DIBV5"); break;
					case 0x0080: wcscpy_c(szName[i], L"CF_OWNERDISPLAY"); break;
					case 0x0081: wcscpy_c(szName[i], L"CF_DSPTEXT"); break;
					case 0x0082: wcscpy_c(szName[i], L"CF_DSPBITMAP"); break;
					case 0x0083: wcscpy_c(szName[i], L"CF_DSPMETAFILEPICT"); break;
					case 0x008E: wcscpy_c(szName[i], L"CF_DSPENHMETAFILE"); break;
					default: _wsprintf(szName[i], SKIPLEN(countof(szName[i])) L"ClipFormatID=%i", fmt[i].cfFormat);
				}
			}

			INT_PTR nCurLen = _tcslen(szName[i]);
			_wsprintf(szName[i]+nCurLen, SKIPLEN(countof(szName[i])-nCurLen) L", tymed=0x%02X", fmt[i].tymed);
			fmt[i].tymed = TYMED_HGLOBAL;
			stg[i].tymed = 0; //TYMED_HGLOBAL;
			size_t nDataSize = 0;
			// !! The caller then assumes responsibility for releasing the STGMEDIUM structure.
			hr = pDataObject->GetData(fmt+i, stg+i);

			if (hr == S_OK && stg[i].hGlobal)
			{
				if (stg[i].tymed == TYMED_HGLOBAL)
				{
					psz[i] = (LPCWSTR)GlobalLock(stg[i].hGlobal);

					if (psz[i])
					{
						memsize[i] = GlobalSize(stg[i].hGlobal);
						nCurLen = _tcslen(szName[i]);
						_wsprintf(szName[i]+nCurLen, SKIPLEN(countof(szName[i])-nCurLen) L", DataSize=%i", (DWORD)(memsize[i]));

						if (memsize[i] == 1)
						{
							nCurLen = _tcslen(szName[i]);
							_wsprintf(szName[i]+nCurLen, SKIPLEN(countof(szName[i])-nCurLen) L", Data=0x%02X", (DWORD)*((LPBYTE)(psz[i])));
						}
						else if (memsize[i] == sizeof(DWORD))
						{
							nCurLen = _tcslen(szName[i]);
							_wsprintf(szName[i]+nCurLen, SKIPLEN(countof(szName[i])-nCurLen) L", Data=0x%08X", (DWORD)*((LPDWORD)(psz[i])));
						}
						else if (memsize[i] == sizeof(u64))
						{
							nCurLen = _tcslen(szName[i]);
							_wsprintf(szName[i]+nCurLen, SKIPLEN(countof(szName[i])-nCurLen) L", Data=0x%08X%08X", (DWORD)((LPDWORD)(psz[i]))[0], (DWORD)((LPDWORD)psz[i])[1]);
						}
						else
						{
							wcscat_c(szName[i], L", ");
							const wchar_t* pwsz = (const wchar_t*)(psz[i]);
							const char* pasz = (const char*)(psz[i]);
							nDataSize = (memsize[i]+1)*2;
							pszData[i] = (wchar_t*)calloc(nDataSize,1);

							// тупая проверка на юникод. первый символ - обычно из English char set
							if (pasz[0] && pasz[1])
							{
								//if (hDumpFile)
								//{
								MultiByteToWideChar(CP_ACP, 0, pasz, memsize[i], pszData[i], memsize[i]);
								//} else {
								//	int nMaxLen = min(200,memsize[i]);
								//	wchar_t* pwszDst = szName[i]+_tcslen(szName[i]);
								//	MultiByteToWideChar(CP_ACP, 0, pasz, nMaxLen, pwszDst, nMaxLen);
								//	pwszDst[nMaxLen] = 0;
								//}
							}
							else
							{
								//if (hDumpFile)
								//{
								StringCbCopy(pszData[i], nDataSize, pwsz);
								nDataSize = ((memsize[i]>>1)+1)<<1; // было больше, с учетом возможного MultiByteToWideChar
								//} else {
								//	int nMaxLen = min(200,memsize[i]/2);
								//	lstrcpyn(szName[i]+_tcslen(szName[i]), pwsz, nMaxLen);
								//}
							}
						}
					}
					else
					{
						wcscat_c(szName[i], L", hGlobal not available");
						stg[i].hGlobal = NULL;
					}
				}
				else
				{
					wcscat_c(szName[i], L", hGlobal not available");
					stg[i].hGlobal = NULL;
				}
			}
			else if (stg[i].tymed == TYMED_FILE)
			{
				wcscat_c(szName[i], L", Error in source! TYMED_HGLOBAL was requested, but got TYMED_FILE");
				ReleaseStgMedium(stg+i);
				stg[i].hGlobal = NULL;
			}
			else if (stg[i].tymed == TYMED_ISTREAM)
			{
				wcscat_c(szName[i], L", Error in source! TYMED_HGLOBAL was requested, but got TYMED_ISTREAM");
				ReleaseStgMedium(stg+i);
				stg[i].hGlobal = NULL;
			}
			else if (stg[i].tymed == TYMED_ISTORAGE)
			{
				wcscat_c(szName[i], L", Error in source! TYMED_HGLOBAL was requested, but got TYMED_ISTORAGE");
				ReleaseStgMedium(stg+i);
				stg[i].hGlobal = NULL;
			}
			else if (stg[i].tymed == TYMED_GDI)
			{
				wcscat_c(szName[i], L", Error in source! TYMED_HGLOBAL was requested, but got TYMED_GDI");
				ReleaseStgMedium(stg+i);
				stg[i].hGlobal = NULL;
			}
			else if (stg[i].tymed == TYMED_MFPICT)
			{
				wcscat_c(szName[i], L", Error in source! TYMED_HGLOBAL was requested, but got TYMED_MFPICT");
				ReleaseStgMedium(stg+i);
				stg[i].hGlobal = NULL;
			}
			else if (stg[i].tymed == TYMED_ENHMF)
			{
				wcscat_c(szName[i], L", Error in source! TYMED_HGLOBAL was requested, but got TYMED_ENHMF");
				ReleaseStgMedium(stg+i);
				stg[i].hGlobal = NULL;
			}
			else
			{
				int nCurLen = _tcslen(szName[i]);
				_wsprintf(szName[i], SKIPLEN(countof(szName[i])-nCurLen) L", Error in source! TYMED_HGLOBAL was requested, but got (%i)", stg[i].tymed);
				ReleaseStgMedium(stg+i);
				stg[i].hGlobal = NULL;
			}

			//#ifdef _DEBUG
			//if (wcscmp(szName[i], L"DragImageBits") == 0)
			//{
			//	stg[i].tymed = TYMED_HGLOBAL;
			//}
			//#endif
			wcscat_c(szName[i], L"\n");

			if (hDumpFile)
			{
				WriteFile(hDumpFile, szName[i], (DWORD)2*_tcslen(szName[i]), &nWritten, NULL);

				if (nDataSize && pszData[i])
				{
					WriteFile(hDumpFile, pszData[i], nDataSize, &nWritten, NULL);
					WriteFile(hDumpFile, L"\n", 2, &nWritten, NULL);
				}

				WriteFile(hDumpFile, L"\n", 2, &nWritten, NULL);
			}
			else
			{
				// Послать в отладчик
				OutputDebugStringW(szName[i]);

				if (nDataSize && pszData[i])
				{
					OutputDebugStringW(pszData[i]);
					OutputDebugStringW(L"\n");
				}
			}

			if (fmt[i].cfFormat == cfShlPidl && psz[i])
			{
				CIDA *pcida = (CIDA*)(psz[i]);
				LPCITEMIDLIST pidlRoot = HIDA_GetPIDLFolder(pcida);
				PidlDump(pidlRoot, hDumpFile);

				for(UINT i = 0; i < pcida->cidl; i++)
				{
					LPCITEMIDLIST pidl = HIDA_GetPIDLItem(pcida, i);
					PidlDump(pidl, hDumpFile);
				}
			}
		}

		for(i=0; i<nCnt; i++)
		{
			if (psz[i] && stg[i].hGlobal)
			{
				GlobalUnlock(stg[i].hGlobal);
				//GlobalFree(stg[i].hGlobal);
				ReleaseStgMedium(stg+i);
			}

			if (pszData[i])
			{
				free(pszData[i]);
			}
		}
	}

	hr = S_OK;
}

// Загрузить из фара информацию для Drop
// Требуется только при Drop из внешних приложений!
void CDragDropData::RetrieveDragToInfo()
{
	if (m_pfpi) {free(m_pfpi); m_pfpi=NULL;}

	CVirtualConsole* pVCon = gpConEmu->ActiveCon();
	if (!pVCon)
		return;
	CRealConsole* pRCon = pVCon->RCon();
	if (!pRCon)
		return;

	DWORD nFarPID = pRCon->GetFarPID(TRUE);
	if (nFarPID == 0)
	{
		// 
		m_pfpi = (ForwardedPanelInfo*)calloc(sizeof(ForwardedPanelInfo)+sizeof(WCHAR)*2, 1);
		m_pfpi->ActiveRect.right = pRCon->TextWidth() - 1;
		m_pfpi->ActiveRect.bottom = pRCon->TextHeight() - 1;
		m_pfpi->NoFarConsole = TRUE;
		m_pfpi->ActivePathShift = sizeof(ForwardedPanelInfo);
		m_pfpi->pszActivePath = (WCHAR*)(((char*)m_pfpi)+m_pfpi->ActivePathShift);
		m_pfpi->PassivePathShift = sizeof(ForwardedPanelInfo)+sizeof(wchar_t);
		m_pfpi->pszPassivePath = (WCHAR*)(((char*)m_pfpi)+m_pfpi->PassivePathShift);
		m_pfpi->pszActivePath[0] = L'*';
	}
	else
	{
		CConEmuPipe pipe(nFarPID, CONEMUREADYTIMEOUT);
		gpConEmu->DebugStep(_T("DnD: DragEnter starting"));

		// Ошибки пайпа отображаются через MBoxA
		if (pipe.Init(_T("CDragDropData::DragEnter")))
		{
			if (pipe.Execute(CMD_DRAGTO))
			{
				DWORD cbBytesRead=0;
				int cbStructSize=0;

				if (m_pfpi) {free(m_pfpi); m_pfpi=NULL;}

				if (pipe.Read(&cbStructSize, sizeof(int), &cbBytesRead))
				{
					if ((DWORD)cbStructSize>sizeof(ForwardedPanelInfo))
					{
						m_pfpi = (ForwardedPanelInfo*)calloc(cbStructSize, 1);
						pipe.Read(m_pfpi, cbStructSize, &cbBytesRead);
						m_pfpi->pszActivePath = (WCHAR*)(((char*)m_pfpi)+m_pfpi->ActivePathShift);
						//Slash на конце нам не нужен
						int nPathLen = lstrlenW(m_pfpi->pszActivePath);

						if (nPathLen>0 && m_pfpi->pszActivePath[nPathLen-1]==_T('\\'))
							m_pfpi->pszActivePath[nPathLen-1] = 0;

						m_pfpi->pszPassivePath = (WCHAR*)(((char*)m_pfpi)+m_pfpi->PassivePathShift);
						//Slash на конце нам не нужен
						nPathLen = lstrlenW(m_pfpi->pszPassivePath);

						if (nPathLen>0 && m_pfpi->pszPassivePath[nPathLen-1]==_T('\\'))
							m_pfpi->pszPassivePath[nPathLen-1] = 0;
					}
				}
			}
		}

		gpConEmu->DebugStep(NULL);
	}
}

BOOL CDragDropData::CreateDragImageBits(IDataObject * pDataObject)
{
	DEBUGSTROVL(L"CreateDragImageBits(IDataObject) - NOT IMPLEMENTED\n");
	return FALSE;
}

#include <pshpack1.h>
typedef struct tag_MyRgbQuad
{
	union
	{
		DWORD dwValue;
		struct
		{
			BYTE    rgbBlue;
			BYTE    rgbGreen;
			BYTE    rgbRed;
			BYTE    rgbAlpha;
		};
	};
} MyRgbQuad;
#include <poppack.h>

// Result Must be GlobalAlloc'ed
DragImageBits* CDragDropData::CreateDragImageBits(wchar_t* pszFiles)
{
	if (!pszFiles)
	{
		DEBUGSTROVL(L"CreateDragImageBits failed, pszFiles is NULL\n");
		_ASSERTE(pszFiles!=NULL);
		return NULL;
	}

	DestroyDragImageBits();
#ifdef PERSIST_OVL
	MoveDragWindow(FALSE);
#else
	DestroyDragImageWindow();
#endif
	DEBUGSTROVL(L"CreateDragImageBits()\n");
	DragImageBits* pBits = NULL;
	HDC hScreenDC = ::GetDC(0);
	HDC hDrawDC = CreateCompatibleDC(hScreenDC);
	HBITMAP hBitmap = CreateCompatibleBitmap(hScreenDC, MAX_OVERLAY_WIDTH, MAX_OVERLAY_HEIGHT);

	if (hDrawDC && hBitmap)
	{
		HFONT hOldF = NULL, hf = CreateFont(14, 0, 0, 0, 400, 0, 0, 0, CP_ACP, 0, 0, 0, 0, L"Tahoma");

		if (hf) hOldF = (HFONT)SelectObject(hDrawDC, hf);

		int nMaxX = 0, nMaxY = 0, nX = 0;
		// GetTextExtentPoint32 почему-то портит DC, поэтому ширину получим сначала
		wchar_t *psz = pszFiles; int nFilesCol = 0;

		while(*psz)
		{
			SIZE sz = {0};
			LPCWSTR pszText = wcsrchr(psz, L'\\');
			if (!pszText) pszText = psz; else psz++;

			GetTextExtentPoint32(hDrawDC, pszText, _tcslen(pszText), &sz); //-V107

			if (sz.cx > MAX_OVERLAY_WIDTH)
				sz.cx = MAX_OVERLAY_WIDTH;

			if (sz.cx > nMaxX)
				nMaxX = sz.cx;

			psz += _tcslen(psz)+1; // длина полного пути и длина имени файла разные ;)
			nFilesCol ++;
		}

		nMaxX = min((OVERLAY_TEXT_SHIFT + nMaxX),MAX_OVERLAY_WIDTH);
		// Если тащат много файлов/папок - можно попробовать разместить их в несколько колонок
		int nColCount = 1;

		if (nFilesCol > 3)
		{
			if (nFilesCol > 21 && (nMaxX * 3 + 32) <= MAX_OVERLAY_WIDTH) //-V112
			{
				nFilesCol = (nFilesCol+3) / (nColCount = 4); // располагаем в 4 колонки //-V112
			}
			else if (nFilesCol > 12 && (nMaxX * 2 + 32) <= MAX_OVERLAY_WIDTH) //-V112
			{
				nFilesCol = (nFilesCol+2) / (nColCount = 3); // располагаем в 3 колонки
			}
			else if ((nMaxX + 32) <= MAX_OVERLAY_WIDTH) //-V112
			{
				nFilesCol = (nFilesCol+1) / (nColCount = 2); // располагаем в 2 колонки
			}
		}

		HBITMAP hOldBitmap = (HBITMAP)SelectObject(hDrawDC, hBitmap);
		SetTextColor(hDrawDC, RGB(0,0,128)); // Темно синий текст
		//SetTextColor(hDrawDC, RGB(255,255,255)); // Белый текст
		SetBkColor(hDrawDC, RGB(192,192,192)); // на сером фоне
		SetBkMode(hDrawDC, OPAQUE);
		// DC может быть испорчен (почему?), поэтому лучше почистим его
		RECT rcFill = MakeRect(MAX_OVERLAY_WIDTH,MAX_OVERLAY_HEIGHT);
#ifdef PERSIST_OVL
		HBRUSH hBr = CreateSolidBrush(OVERLAY_COLOR);
		FillRect(hDrawDC, &rcFill, hBr);
		DeleteObject(hBr);
#else
#ifdef _DEBUG
		FillRect(hDrawDC, &rcFill, (HBRUSH)GetStockObject(BLACK_BRUSH/*WHITE_BRUSH*/));
#else
		FillRect(hDrawDC, &rcFill, (HBRUSH)GetStockObject(BLACK_BRUSH));
#endif
#endif
		//DumpImage(hDrawDC, MAX_OVERLAY_WIDTH, nMaxY, L"F:\\Dump.bmp");
		psz = pszFiles;
		int nFileIdx = 0, nColMaxY = 0, nAllFiles = 0;
		nMaxX = 0;
		int nFirstColWidth = 0;

		while(*psz)
		{
			if (!DrawImageBits(hDrawDC, psz, &nMaxX, nX, &nColMaxY))
				break; // вышли за пределы MAX_OVERLAY_WIDTH x MAX_OVERLAY_HEIGHT (по высоте)

			psz += _tcslen(psz)+1;

			if (!*psz) break;

			nFileIdx ++; nAllFiles ++;

			if (nFileIdx >= nFilesCol)
			{
				if (!nFirstColWidth)
					nFirstColWidth = nMaxX + OVERLAY_TEXT_SHIFT;

				if ((nX + nMaxX + 32) >= MAX_OVERLAY_WIDTH) //-V112
					break;

				nX += nMaxX + OVERLAY_TEXT_SHIFT + OVERLAY_COLUMN_SHIFT;

				if (nColMaxY > nMaxY) nMaxY = nColMaxY;

				nColMaxY = nMaxX = nFileIdx = 0;
			}
		}

		if (nColMaxY > nMaxY) nMaxY = nColMaxY;

		if (!nFirstColWidth) nFirstColWidth = nMaxX + OVERLAY_TEXT_SHIFT;

		int nLineX = ((nX+nMaxX+OVERLAY_TEXT_SHIFT+3)>>2)<<2;

		if (nLineX > MAX_OVERLAY_WIDTH) nLineX = MAX_OVERLAY_WIDTH;

		SelectObject(hDrawDC, hOldF);
		GdiFlush();
#ifdef _DEBUG
		DumpImage(hDrawDC, MAX_OVERLAY_WIDTH, nMaxY, L"F:\\Dump.bmp");
#endif

		if (nLineX>2 && nMaxY>2)
		{
			HDC hBitsDC = CreateCompatibleDC(hScreenDC);

			if (hBitsDC && hDrawDC)
			{
				SetLayout(hBitsDC, LAYOUT_BITMAPORIENTATIONPRESERVED);
				BITMAPINFOHEADER bih = {sizeof(BITMAPINFOHEADER)};
				bih.biWidth = nLineX;
				bih.biHeight = nMaxY;
				bih.biPlanes = 1;
				bih.biBitCount = 24;
				bih.biCompression = BI_RGB;
				LPBYTE pSrc = NULL;
				HBITMAP hBitsBitmap = CreateDIBSection(hScreenDC, (BITMAPINFO*)&bih, DIB_RGB_COLORS, (void**)&pSrc, NULL, 0);

				if (hBitsBitmap)
				{
					HBITMAP hOldBitsBitmap = (HBITMAP)SelectObject(hBitsDC, hBitsBitmap);
					BitBlt(hBitsDC, 0,0,nLineX,nMaxY, hDrawDC,0,0, SRCCOPY);
					GdiFlush();
					DragImageBits* pDst = (DragImageBits*)GlobalAlloc(GPTR, sizeof(DragImageBits) + (nLineX*nMaxY - 1)*sizeof(RGBQUAD));

					if (pDst)
					{
						pDst->nWidth = nLineX;
						pDst->nHeight = nMaxY;

						if (nColCount == 1)
							pDst->nXCursor = nMaxX / 2;
						else
							pDst->nXCursor = nFirstColWidth;

						pDst->nYCursor = 17; // под первой строкой
						pDst->nRes1 = GetTickCount(); // что-то непонятное. Random?
						pDst->nRes2 = (DWORD)-1;
						MyRgbQuad *pRGB = (MyRgbQuad*)pDst->pix;
#ifdef PERSIST_OVL
						u32 nCurBlend = OVERLAY_ALPHA, nAllBlend = OVERLAY_ALPHA;
#else
						u32 nCurBlend = 0xAA, nAllBlend = 0xAA;
#endif

						for(int y = 0; y < nMaxY; y++)
						{
							for(int x = 0; x < nLineX; x++)
							{
								pRGB->rgbBlue = *(pSrc++);
								pRGB->rgbGreen = *(pSrc++);
								pRGB->rgbRed = *(pSrc++);

								if (pRGB->dwValue)
								{
#ifdef PERSIST_OVL

									if (pRGB->dwValue == OVERLAY_COLOR)
									{
										pRGB->dwValue = 0;
									}
									else
									{
#endif
										pRGB->rgbBlue = klMulDivU32(pRGB->rgbBlue, nCurBlend, 0xFF);
										pRGB->rgbGreen = klMulDivU32(pRGB->rgbGreen, nCurBlend, 0xFF);
										pRGB->rgbRed = klMulDivU32(pRGB->rgbRed, nCurBlend, 0xFF);
										pRGB->rgbAlpha = nAllBlend;
#ifdef PERSIST_OVL
									}

#endif
								}

								pRGB++;
							}
						}

						mp_Bits = (DragImageBits*)calloc(sizeof(DragImageBits)/*+(nCount-1)*4*/,1);

						if (mp_Bits)
						{
							*mp_Bits = *pDst;
							pBits = pDst; pDst = NULL; //OK
							mh_BitsDC = hBitsDC; hBitsDC = NULL;
							mh_BitsBMP = hBitsBitmap; hBitsBitmap = NULL;
						}
						else
						{
							if (pDst) GlobalFree(pDst); pDst = NULL;  // Ошибка
						}
					}

					if (hBitsDC)
						SelectObject(hBitsDC, hOldBitsBitmap);

					if (hBitsBitmap)
					{
						DeleteObject(hBitsBitmap); hBitsBitmap = NULL;
					}
				}
			}

			if (hBitsDC)
			{
				DeleteDC(hBitsDC); hBitsDC = NULL;
			}
		}

		SelectObject(hDrawDC, hOldBitmap);
	}

	if (hDrawDC)
	{
		DeleteDC(hDrawDC); hDrawDC = NULL;
	}

	if (hBitmap)
	{
		DeleteObject(hBitmap); hBitmap = NULL;
	}

	if (hScreenDC)
	{
		::ReleaseDC(0, hScreenDC);
		hScreenDC = NULL;
	}

	return pBits;
}

BOOL CDragDropData::DrawImageBits(HDC hDrawDC, wchar_t* pszFile, int *nMaxX, int nX, int *nMaxY)
{
	if ((*nMaxY + 17) >= MAX_OVERLAY_HEIGHT)
		return FALSE;

	SHFILEINFO sfi = {0};
	DWORD nDrawRC = 0;
	wchar_t* pszText = wcsrchr(pszFile, L'\\');
	if (!pszText) pszText = pszFile; else pszText++;

	SIZE sz = {0};
	GetTextExtentPoint32(hDrawDC, pszText, _tcslen(pszText), &sz); //-V107

	if (sz.cx > MAX_OVERLAY_WIDTH)
		sz.cx = MAX_OVERLAY_WIDTH;

	if (sz.cx > *nMaxX)
		*nMaxX = sz.cx;

	// Иконка, если просили
	if (gpSet->isDragShowIcons)
	{
		UINT cbSize = sizeof(sfi);
		DWORD_PTR shRc = SHGetFileInfo(pszFile, 0, &sfi, cbSize,
		                               SHGFI_ATTRIBUTES|SHGFI_ICON/*|SHGFI_SELECTED*/|SHGFI_SMALLICON|SHGFI_TYPENAME);

		if (shRc)
		{
			nDrawRC = DrawIconEx(hDrawDC, nX, *nMaxY, sfi.hIcon, 16, 16, 0, NULL, DI_NORMAL);
		}
		else
		{
			// Нарисовать стандартную иконку?
		}
	}

	// А теперь - имя файла/папки
	RECT rcText = {nX+OVERLAY_TEXT_SHIFT, *nMaxY+1, 0, (*nMaxY + 16)};
	rcText.right = min(MAX_OVERLAY_WIDTH, (rcText.left + *nMaxX));
	wchar_t szText[MAX_PATH+1]; lstrcpyn(szText, pszText, MAX_PATH); szText[MAX_PATH] = 0;
	nDrawRC = DrawTextEx(hDrawDC, szText, _tcslen(szText), &rcText,
	                     DT_LEFT|DT_TOP|DT_NOPREFIX|DT_END_ELLIPSIS|DT_SINGLELINE|DT_MODIFYSTRING, NULL);

	if (*nMaxY < (rcText.bottom+1))
		*nMaxY = min(rcText.bottom+1,300);

	if (sfi.hIcon)
	{
		DestroyIcon(sfi.hIcon); sfi.hIcon = NULL;
	}

	return TRUE;
}

BOOL CDragDropData::LoadDragImageBits(IDataObject * pDataObject)
{
	if (/*mb_selfdrag ||*/ mh_Overlapped)
		return FALSE; // уже

	DestroyDragImageBits();
#ifdef PERSIST_OVL
	MoveDragWindow(FALSE);
#else
	DestroyDragImageWindow();
#endif

	if (!gpSet->isDragOverlay)
		return FALSE;

	STGMEDIUM stgMedium = { 0 };
	FORMATETC fmtetc = { 0, 0, DVASPECT_CONTENT, -1, TYMED_HGLOBAL };
	// Это пока что-то не работает
	//if (gpSet->isDragOverlay == 1) {
	//	wchar_t* pszFiles = NULL;
	//	fmtetc.cfFormat = RegisterClipboardFormat(L"FileNameW");
	//	TODO("А освобождать полученное надо?");
	//	if (S_OK == pDataObject->QueryGetData(&fmtetc)) {
	//		if (S_OK == pDataObject->GetData(&fmtetc, &stgMedium) || stgMedium.hGlobal == NULL) {
	//			pszFiles = (wchar_t*)GlobalLock(stgMedium.hGlobal);
	//			if (pszFiles) {
	//				return (CreateDragImageBits(pszFiles) != NULL);
	//			}
	//		}
	//	}
	//}
	fmtetc.cfFormat = RegisterClipboardFormat(L"DragImageBits");

	if (S_OK != pDataObject->QueryGetData(&fmtetc))
	{
		return FALSE; // Формат отсутствует
	}

	// !! The caller then assumes responsibility for releasing the STGMEDIUM structure.
	if (S_OK != pDataObject->GetData(&fmtetc, &stgMedium) || stgMedium.hGlobal == NULL)
	{
		return FALSE; // Формат отсутствует
	}

	SIZE_T nInfoSize = GlobalSize(stgMedium.hGlobal);

	if (!nInfoSize)
	{
		//GlobalFree(stgMedium.hGlobal);
		ReleaseStgMedium(&stgMedium);
		return FALSE; // пусто
	}

	if (nInfoSize <= sizeof(DragImageBits))
	{
		_ASSERTE(nInfoSize > sizeof(DragImageBits));
		ReleaseStgMedium(&stgMedium);
		return FALSE;
	}

	DragImageBits* pInfo = (DragImageBits*)GlobalLock(stgMedium.hGlobal);

	if (!pInfo)
	{
		//GlobalFree(stgMedium.hGlobal);
		ReleaseStgMedium(&stgMedium);
		return FALSE; // Не удалось получить данные
	}

	_ASSERTE(pInfo->nWidth>0 && pInfo->nWidth<=400);
	_ASSERTE(pInfo->nHeight>0 && pInfo->nHeight<=400);
	SIZE_T nReqSize = (sizeof(DragImageBits)+(pInfo->nWidth * pInfo->nHeight - 1)*sizeof(RGBQUAD));
	// На Win7x64 + 2*DWORD
	SIZE_T nReqSize2 = (sizeof(DragImageBits)+(pInfo->nWidth * pInfo->nHeight - 1)*sizeof(RGBQUAD))+2*sizeof(DWORD);
	int nShift = 0;

	if (nInfoSize != nReqSize && nInfoSize != nReqSize2 /*|| (nInfoSize - nReqSize) > 32*/)
	{
#ifdef _DEBUG

		if (!IsDebuggerPresent())
		{
			_ASSERT(nInfoSize == nReqSize); // Неизвестный формат?
			wchar_t szDbg[128]; _wsprintf(szDbg, SKIPLEN(countof(szDbg)) L"DragImageBits unknown format? (nInfoSize=%u) != (nReqSize=%u)", (DWORD)nInfoSize, (DWORD)nReqSize);
			gpConEmu->DebugStep(szDbg, TRUE);
		}

#endif

		if ((nReqSize < 16) || (nInfoSize < nReqSize) || (nInfoSize - nReqSize) > 8)
		{
			//GlobalFree(stgMedium.hGlobal);
			ReleaseStgMedium(&stgMedium);
			return FALSE;
		}
	}

	if (nInfoSize == nReqSize2)
		nShift = 8;

	BOOL lbRc = FALSE;
	//int nCount = pInfo->nWidth * pInfo->nHeight;
	mp_Bits = (DragImageBits*)calloc(sizeof(DragImageBits)/*+(nCount-1)*4*/,1);

	if (mp_Bits)
	{
		*mp_Bits = *pInfo;
		//memmove(mp_Bits->pix, pInfo->pix, nCount*4);
		MCHKHEAP
		HDC hdc = ::GetDC(ghWnd);
		mh_BitsDC = CreateCompatibleDC(hdc);

		if (mh_BitsDC)
		{
			SetLayout(mh_BitsDC, LAYOUT_BITMAPORIENTATIONPRESERVED);
			BITMAPINFOHEADER bih = {sizeof(BITMAPINFOHEADER)};
			bih.biWidth = mp_Bits->nWidth;
			bih.biHeight = mp_Bits->nHeight;
			bih.biPlanes = 1;
			bih.biBitCount = 32; //-V112
			bih.biCompression = BI_RGB;
			bih.biXPelsPerMeter = 96;
			bih.biYPelsPerMeter = 96;
			LPBYTE pDst = NULL;
			mh_BitsBMP_Old = NULL;
			mh_BitsBMP = CreateDIBSection(hdc, (BITMAPINFO*)&bih, DIB_RGB_COLORS, (void**)&pDst, NULL, 0);

			if (mh_BitsBMP && pDst)
			{
				mh_BitsBMP_Old = (HBITMAP)SelectObject(mh_BitsDC, mh_BitsBMP);
				int cbSize = pInfo->nWidth * pInfo->nHeight * sizeof(RGBQUAD);
				memmove(pDst, ((LPBYTE)pInfo->pix)+nShift, cbSize);
				GdiFlush();
#ifdef _DEBUG
				int nBits = GetDeviceCaps(mh_BitsDC, BITSPIXEL);
				//_ASSERTE(nBits == 32);
#endif
				lbRc = TRUE;
			}
		}

		if (hdc) ::ReleaseDC(ghWnd, hdc);
	}

	// Освободим данные
	GlobalUnlock(stgMedium.hGlobal);
	//GlobalFree(stgMedium.hGlobal);
	ReleaseStgMedium(&stgMedium);

	if (!lbRc)
		DestroyDragImageBits();

	return lbRc;
}

void CDragDropData::DestroyDragImageBits()
{
	if (mh_BitsDC || mh_BitsBMP || mp_Bits)
	{
		DEBUGSTROVL(L"DestroyDragImageBits()\n");

		if (mh_BitsDC)
		{
			if (mh_BitsBMP_Old)
			{
				SelectObject(mh_BitsDC, mh_BitsBMP_Old); mh_BitsBMP_Old = NULL;
			}

			DeleteDC(mh_BitsDC);
			mh_BitsDC = NULL;
		}

		if (mh_BitsBMP)
		{
			DeleteObject(mh_BitsBMP);
			mh_BitsBMP = NULL;
		}

		if (mp_Bits)
		{
			free(mp_Bits);
			mp_Bits = NULL;
		}
	}
}

BOOL CDragDropData::CreateDragImageWindow()
{
	DEBUGSTROVL(L"CreateDragImageWindow()\n");
#define DRAGBITSCLASS L"ConEmuDragBits"
	static BOOL bClassRegistered = FALSE;

	if (!bClassRegistered)
	{
#ifdef _DEBUG
		DWORD dwLastError = 0;
#endif
		HBRUSH hBackBrush = (HBRUSH)GetStockObject(WHITE_BRUSH);
		WNDCLASS wc = {0/*CS_OWNDC*/ /*CS_SAVEBITS*/, DragBitsWndProc, 0, 0,
		               g_hInstance, NULL, NULL/*LoadCursor(NULL, IDC_ARROW)*/,
		               hBackBrush,  NULL, DRAGBITSCLASS
		              };

		if (!RegisterClass(&wc))
		{
#ifdef _DEBUG
			dwLastError = GetLastError();
#endif
			// Ругаться не будем, чтобы драг не испортить
			return FALSE;
		}

		bClassRegistered = TRUE; // регистрировать класс один раз
	}

	//int nCount = mp_Bits->nWidth * mp_Bits->nHeight;
	_ASSERTE(mh_Overlapped==NULL);
	int nWidth = MAX_OVERLAY_WIDTH, nHeight = MAX_OVERLAY_HEIGHT;
	// |WS_BORDER|WS_SYSMENU - создает проводник. попробуем?
	//2009-08-20 [+] WS_EX_NOACTIVATE
#ifdef PERSIST_OVL
	mh_Overlapped = CreateWindowEx(
	                    WS_EX_LAYERED|WS_EX_TRANSPARENT|WS_EX_PALETTEWINDOW|WS_EX_TOOLWINDOW|WS_EX_TOPMOST|WS_EX_NOACTIVATE,
	                    DRAGBITSCLASS, L"Drag", WS_POPUP|WS_VISIBLE|WS_CLIPSIBLINGS|WS_BORDER|WS_SYSMENU,
	                    -32000, -32000, nWidth, nHeight,
	                    ghWnd, NULL, g_hInstance, (LPVOID)this);
#else
	mh_Overlapped = CreateWindowEx(
	                    WS_EX_LAYERED|WS_EX_TRANSPARENT|WS_EX_PALETTEWINDOW|WS_EX_TOOLWINDOW|WS_EX_TOPMOST|WS_EX_NOACTIVATE,
	                    DRAGBITSCLASS, L"Drag", WS_POPUP|WS_VISIBLE|WS_CLIPSIBLINGS|WS_BORDER|WS_SYSMENU,
	                    0, 0, mp_Bits->nWidth, mp_Bits->nHeight, ghWnd, NULL, g_hInstance, (LPVOID)this);
#endif

	if (!mh_Overlapped)
	{
		_ASSERTE(mh_Overlapped!=NULL);
#ifdef PERSIST_OVL
		//DestroyDragImageBits(); -- смысла уже нет, т.к. создается только при старте
#else
		DestroyDragImageBits();
#endif
		return NULL;
	}

#ifdef PERSIST_OVL
	SetLayeredWindowAttributes(mh_Overlapped, OVERLAY_COLOR, OVERLAY_ALPHA, LWA_COLORKEY|LWA_ALPHA);
#endif
	MoveDragWindow(FALSE);
	return TRUE;
}

void CDragDropData::MoveDragWindow(BOOL abVisible/*=TRUE*/)
{
	if (!mh_Overlapped)
	{
		DEBUGSTRBACK(L"MoveDragWindow skipped, Overlay was not created\n");
		return;
	}

	DEBUGSTRBACK(L"MoveDragWindow()\n");
#ifdef PERSIST_OVL

	if (abVisible && !mh_BitsDC)
	{
		_ASSERTE(mh_BitsDC!=NULL);
		abVisible = FALSE;
	}

	POINT p = {0};

	if (abVisible)
	{
		GetCursorPos(&p);

		if (mp_Bits)
		{
			p.x -= mp_Bits->nXCursor; p.y -= mp_Bits->nYCursor;
		}
	}
	else
	{
		p.x = p.y = -32000;
	}

	POINT p2 = {0, 0};
	SIZE  sz = {MAX_OVERLAY_WIDTH, MAX_OVERLAY_HEIGHT};

	if (mp_Bits)
	{
		sz.cx = mp_Bits->nWidth; sz.cy = mp_Bits->nHeight;
	}

	static BOOL bLastVisible = FALSE;
	MoveWindow(mh_Overlapped, p.x, p.y, sz.cx, sz.cy, abVisible && (abVisible != bLastVisible));
	bLastVisible = abVisible;
#else
	BLENDFUNCTION bf;
	bf.BlendOp = AC_SRC_OVER;
	bf.AlphaFormat = AC_SRC_ALPHA;
	bf.BlendFlags = 0;
	bf.SourceConstantAlpha = abVisible ? 255 : 0;
	POINT p = {0};
	GetCursorPos(&p);
	p.x -= mp_Bits->nXCursor; p.y -= mp_Bits->nYCursor;
	POINT p2 = {0, 0};
	SIZE  sz = {mp_Bits->nWidth,mp_Bits->nHeight};
#ifdef _DEBUG
	BOOL bRet =
#endif
	    UpdateLayeredWindow(mh_Overlapped, NULL, &p, &sz, mh_BitsDC, &p2, 0,
	                        &bf, ULW_ALPHA /*ULW_OPAQUE*/);
	//_ASSERTE(bRet);
	//apiSetForegroundWindow(ghWnd); // после создания окна фокус уходит из GUI
#endif
}

void CDragDropData::DestroyDragImageWindow()
{
	if (mh_Overlapped)
	{
		DEBUGSTROVL(L"DestroyDragImageWindow()\n");
		DestroyWindow(mh_Overlapped);
		mh_Overlapped = NULL;
	}
}

LRESULT CDragDropData::DragBitsWndProc(HWND hWnd, UINT messg, WPARAM wParam, LPARAM lParam)
{
	if (messg == WM_CREATE)
	{
		SetWindowLongPtr(hWnd, GWLP_USERDATA,
		                 (LONG_PTR)((LPCREATESTRUCT)lParam)->lpCreateParams);
	}
	else if (messg == WM_SETFOCUS)
	{
		CDragDropData *pDrag = (CDragDropData*)GetWindowLongPtr(hWnd, GWLP_USERDATA);

		if (pDrag && pDrag->mb_DragWithinNow)
		{
			TODO("Если создавать окно сразу - не понадобится с фокусом играться");
			DEBUGSTROVL(L"CDragDrop::DragBitsWndProc --> apiSetForegroundWindow");
			apiSetForegroundWindow(ghWnd); // после создания окна фокус уходит из GUI
		}

		return 0;
	}

#ifdef PERSIST_OVL
	else if (messg == WM_PAINT)
	{
		CDragDrop *pDrag = (CDragDrop*)GetWindowLongPtr(hWnd, GWLP_USERDATA);

		if (pDrag)
		{
			PAINTSTRUCT ps = {0};
			HDC hdc = BeginPaint(hWnd, &ps);

			if (hdc)
			{
				int nWidth = 0, nHeight = 0;

				if (pDrag->mp_Bits)
				{
					nWidth = pDrag->mp_Bits->nWidth; nHeight = pDrag->mp_Bits->nHeight;
					BitBlt(hdc, 0,0,nWidth,nHeight, pDrag->mh_BitsDC,0,0, SRCCOPY);
				}
				else
				{
					//_ASSERTE(pDrag->mp_Bits!=NULL); -- вполне допустимо, WM_PAINT приходит сразу после создания окна
				}

				if (nWidth<MAX_OVERLAY_WIDTH || nHeight<MAX_OVERLAY_HEIGHT)
				{
					RECT rc;
					HBRUSH hBr = CreateSolidBrush(OVERLAY_COLOR);

					if (nWidth<MAX_OVERLAY_WIDTH)
					{
						rc.left = nWidth; rc.top = 0; rc.right = MAX_OVERLAY_WIDTH; rc.bottom = MAX_OVERLAY_HEIGHT;
						FillRect(hdc, &rc, hBr);
					}

					if (nWidth && nHeight<MAX_OVERLAY_HEIGHT)
					{
						rc.left = 0; rc.top = nHeight; rc.right = MAX_OVERLAY_WIDTH; rc.bottom = MAX_OVERLAY_HEIGHT;
						FillRect(hdc, &rc, hBr);
					}

					DeleteObject(hBr);
				}

				EndPaint(hWnd, &ps);
			}
		}
	}

#endif
	return DefWindowProc(hWnd, messg, wParam, lParam);
}

BOOL CDragDropData::InDragDrop()
{
#ifdef UNLOCKED_DRAG
	PRAGMA_ERROR("Проверить все нити драга, а не только последний mp_DataObject");
#endif

	//!!! mh_Overlapped теперь всегда создается!
	if (mp_DataObject /*|| mh_Overlapped*/)
		return TRUE;

	return FALSE;
}

void CDragDropData::DragFeedBack(DWORD dwEffect)
{
#ifdef _DEBUG
	wchar_t szDbg[128]; _wsprintf(szDbg, SKIPLEN(countof(szDbg)) L"DragFeedBack(%i)\n", (int)dwEffect);
	DEBUGSTRBACK(szDbg);
#endif

	// Drop или отмена драга, когда источник - ConEmu
	if (dwEffect == (DWORD)-1)
	{
		mb_DragWithinNow = FALSE;
#ifdef PERSIST_OVL
		MoveDragWindow(FALSE);
#else
		DestroyDragImageWindow();
#endif
		gpConEmu->SetDragCursor(NULL);
	}
	else if (dwEffect == DROPEFFECT_NONE)
	{
		MoveDragWindow(FALSE);
	}
	else
	{
		if (!mh_Overlapped && mp_Bits)
		{
#ifdef PERSIST_OVL
			//CreateDragImageWindow(); -- должно быть создано при запуске ConEmu
			_ASSERTE(mh_Overlapped!=NULL);
#else
			CreateDragImageWindow();
#endif
		}

		if (mh_Overlapped)
			MoveDragWindow();
	}

	//2009-08-20
	//MSG Msg = {NULL};
	//while (PeekMessage(&Msg, NULL, 0, 0, PM_REMOVE))
	//{
	//	TranslateMessage(&Msg);
	//	DispatchMessage(&Msg);
	//}
}
















/* *********************************** */
/*       Unlocked drag support         */
/* *********************************** */

BOOL CDragDropData::IsDragStarting()
{
	if (!this)
		return FALSE;

	if (!mb_DragStarting)
		return FALSE;

	BOOL lbDragStarting = FALSE;
	std::vector<CEDragSource*>::iterator iter;
	CEDragSource* pds = NULL;

	for (iter = m_Sources.begin(); iter != m_Sources.end(); ++iter)
	{
		pds = *iter;

		if (pds->hThread && pds->bInDrag)
		{
			lbDragStarting = TRUE;
			break;
		}
	}

	return lbDragStarting;
}

BOOL CDragDropData::ForwardMessage(UINT messg, WPARAM wParam, LPARAM lParam)
{
	if (!this)
		return FALSE;

	if (!mb_DragStarting)
		return FALSE;

	BOOL lbDragStarting = FALSE;
	std::vector<CEDragSource*>::iterator iter;
	CEDragSource* pds = NULL;

	for (iter = m_Sources.begin(); iter != m_Sources.end(); ++iter)
	{
		pds = *iter;

		if (pds->hThread && pds->bInDrag)
		{
			PostMessage(pds->hWnd, messg, wParam, lParam);
			lbDragStarting = TRUE;
			break;
		}
	}

	return lbDragStarting;
}

LRESULT CDragDropData::DragProc(HWND hWnd, UINT messg, WPARAM wParam, LPARAM lParam)
{
	LRESULT lRc = 0;

	switch(messg)
	{
		case WM_CREATE:
		{
			LPCREATESTRUCT pc = (LPCREATESTRUCT)lParam;
			CEDragSource *pds = (CEDragSource*)pc->lpCreateParams;
			_ASSERTE(pds);
			_ASSERTE(pds->pDrag);
			SetWindowLongPtr(hWnd, GWLP_USERDATA, (LONG_PTR)pds);
			lRc = 0;
		}
		break;
		case MSG_STARTDRAG:
		{
			CEDragSource* pds = (CEDragSource*)GetWindowLongPtr(hWnd, GWLP_USERDATA);
			_ASSERTE(pds);
			_ASSERTE(pds->pDrag);
			DWORD dwResult = 0, dwEffect = 0;
			DWORD dwAllowedEffects = wParam; //-V103
			IDropSource *pDropSource = (IDropSource*)lParam;
			RECT rcWnd; GetWindowRect(ghWnd, &rcWnd);
			// Чтобы движения мышки "могли" обработаться
			MoveWindow(hWnd, rcWnd.left, rcWnd.top, rcWnd.right-rcWnd.left, rcWnd.bottom-rcWnd.top, 0);
			pds->bInDrag = TRUE;
			pds->pDrag->mb_DragStarting = TRUE;
			wchar_t szStep[255]; _wsprintf(szStep, SKIPLEN(countof(szStep)) L"DoDragDrop.Thread(Eff=0x%X, DataObject=0x%08X, DropSource=0x%08X)", dwAllowedEffects, (DWORD)pds->pDrag->mp_DataObject, (DWORD)pDropSource); //-V205
			gpConEmu->DebugStep(szStep);
			dwResult = DoDragDrop(pds->pDrag->mp_DataObject, pDropSource, dwAllowedEffects, &dwEffect);
			_wsprintf(szStep, SKIPLEN(countof(szStep)) L"DoDragDrop finished, Code=0x%08X", dwResult);

			switch(dwResult)
			{
				case S_OK: wcscat_c(szStep, L" (S_OK)"); break;
				case DRAGDROP_S_DROP: wcscat_c(szStep, L" (DRAGDROP_S_DROP)"); break;
				case DRAGDROP_S_CANCEL: wcscat_c(szStep, L" (DRAGDROP_S_CANCEL)"); break;
					//case E_UNSPEC: lstrcat(szStep, L" (E_UNSPEC)"); break;
			}

			gpConEmu->DebugStep(szStep, (dwResult!=S_OK && dwResult!=DRAGDROP_S_CANCEL && dwResult!=DRAGDROP_S_DROP));
			MCHKHEAP;
			pds->pDrag->mp_DataObject->Release();
			pds->pDrag->mp_DataObject = NULL;
			pDropSource->Release();
			pds->pDrag->mb_DragStarting = FALSE;
			pds->bInDrag = FALSE;
			// может другая нить есть?
			pds->pDrag->mb_DragStarting = pds->pDrag->IsDragStarting();
			lRc = 0;
		}
		break;
		default:
			lRc = DefWindowProc(hWnd, messg, wParam, lParam);
	}

	return lRc;
}

DWORD CDragDropData::DragThread(LPVOID lpParameter)
{
	DWORD nResult = 0;
	CEDragSource* pds = (CEDragSource*)lpParameter;
	//CoInitialize();
	OleInitialize(NULL);
	pds->hWnd = CreateWindowEx(WS_EX_TOOLWINDOW, pds->pDrag->ms_SourceClass, L"ConEmu Drag Source",
	                           WS_POPUP, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, NULL, NULL, (HINSTANCE)g_hInstance, pds);

	if (pds->hWnd == NULL)
	{
		DisplayLastError(L"Can't create drag source window.");
		return 100;
	}

	TODO("Может RegisterDragDrop не нужен?");
	// Инициализация окна для Drop
	//HRESULT hr = RegisterDragDrop(pds->hWnd, pds->pDrag);
	//if (hr != S_OK) {
	//	DisplayLastError(L"Can't register Drop target (thread)", hr);
	//	DestroyWindow(pds->hWnd);
	//	pds->hWnd = NULL;
	//	nResult = 100;
	//} else {
	MSG msg;
	SetEvent(pds->hReady);

	// Чтобы драг жил - запускаем обработку сообщений
	// Сама обработка (Оконная процедура) происходит в DragProc (ниже)
	while(GetMessage(&msg, 0,0,0))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	ResetEvent(pds->hReady);
	//}
	return nResult;
}

CDragDropData::CEDragSource* CDragDropData::InitialCreateSource()
{
	if (!mh_SourceClass)
	{
		wcscpy_c(ms_SourceClass, VirtualConsoleClass);
		wcscat_c(ms_SourceClass, L"DragSource");
		WNDCLASSEX wc = {sizeof(WNDCLASSEX), CS_CLASSDC, DragProc, 0, 0,
		                 g_hInstance, hClassIcon, LoadCursor(NULL, IDC_ARROW),
		                 NULL, NULL, ms_SourceClass, hClassIconSm
		                };// | CS_DROPSHADOW
		mh_SourceClass = RegisterClassEx(&wc);

		if (!mh_SourceClass)
		{
			DisplayLastError(L"Can't register drag class.");
			return NULL;
		}
	}

	CEDragSource *pds = (CEDragSource*)calloc(sizeof(CEDragSource),1);
	pds->hReady = CreateEvent(0,TRUE,FALSE,0);
	pds->pDrag = this;
	pds->hThread = CreateThread(0,0,DragThread,pds,0,&(pds->nTID));

	if (pds->hThread == NULL)
	{
		DisplayLastError(L"Can't create drag thread");
		CloseHandle(pds->hReady);
		free(pds);
		return NULL;
	}

	m_Sources.push_back(pds);
	return pds;
}

CDragDropData::CEDragSource* CDragDropData::GetFreeSource()
{
	std::vector<CEDragSource*>::iterator iter;
	CEDragSource* pds = NULL;

	for (iter = m_Sources.begin(); iter != m_Sources.end(); ++iter)
	{
		pds = *iter;

		if (!pds->hThread || pds->bInDrag) continue;  // ищем готовый поток

		if (WaitForSingleObject(pds->hThread,0)!=WAIT_OBJECT_0)
		{
			// OK
			break;
		}
	}

	if (!pds)
		pds = InitialCreateSource();

	if (pds)
	{
		gpConEmu->DebugStep(L"Drag: waiting for drag thread initialization");
		DWORD nWait = WaitForSingleObject(pds->hReady, 10000);
		gpConEmu->DebugStep(NULL);

		if (nWait != WAIT_OBJECT_0)
		{
			MBoxA(L"Drag thread initialization failed!");
			pds = NULL;
		}
	}

	return pds;
}

void CDragDropData::TerminateDrag()
{
	std::vector<CEDragSource*>::iterator iter;
	CEDragSource* pds = NULL;

	// Сначала во все нити послать QUIT
	for (iter = m_Sources.begin(); iter != m_Sources.end(); ++iter)
	{
		pds = *iter;

		if (!pds->hThread) continue;

		if (WaitForSingleObject(pds->hThread,0)!=WAIT_OBJECT_0)
			PostThreadMessage(pds->nTID, WM_QUIT, 0, 0);
	}

	// После этого попробовать немного подождать, и Terminate...
	for(iter = m_Sources.begin(); iter != m_Sources.end(); iter = m_Sources.erase(iter))
	{
		pds = *iter;

		if (pds->hThread)
		{
			if (WaitForSingleObject(pds->hThread,100)!=WAIT_OBJECT_0)
			{
				TerminateThread(pds->hThread, 100);
			}

			CloseHandle(pds->hThread);
		}

		CloseHandle(pds->hReady);
		free(pds);
	}
}
