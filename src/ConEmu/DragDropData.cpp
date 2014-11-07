﻿
/*
Copyright (c) 2009-2013 Maximus5
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

#include "Header.h"
#pragma warning(disable: 4091)
#include <shlobj.h>
#pragma warning(default: 4091)
#ifdef __GNUC__
#include "ShObjIdl_Part.h"
#endif

#include "DragDropData.h"
#include "DragDrop.h"

#include "ConEmu.h"
#include "ConEmuPipe.h"
#include "RealConsole.h"
#include "ScreenDump.h"
#include "Update.h"
#include "VConGroup.h"
#include "VirtualConsole.h"


#ifdef __GNUC__
#define PCUIDLIST_RELATIVE LPCITEMIDLIST
#define StringCbCopy(d,cbd,s) lstrcpynW(d,s,cbd)
#endif

#define DRAGBITSCLASS L"ConEmuDragBits"
#define DRAGBITSTITLE L"ConEmuDragBits"

#define MAX_OVERLAY_WIDTH    300
#define MAX_OVERLAY_HEIGHT   300
#define OVERLAY_ALPHA        0xAA
#define OVERLAY_TEXT_SHIFT   (gpSet->isDragShowIcons ? 18 : 0)
#define OVERLAY_COLUMN_SHIFT 5
#define OVERLAY_COLOR  RGB(254,0,254)

#define DEBUGSTROVL(s) //DEBUGSTR(s)
#define DEBUGSTRBACK(s) //DEBUGSTR(s)
#define DEBUGSTRFAR(s) //DEBUGSTR(s)

#ifdef _DEBUG
	#define DEBUG_DUMP_IMAGE
	//#undef DEBUG_DUMP_IMAGE
#else
	#undef DEBUG_DUMP_IMAGE
#endif

// COM TaskBarList interface support
#if defined(__GNUC__) && defined(USE_DRAG_HELPER)
const IID IID_IDragSourceHelper2 = {0x83E07D0D, 0x0C5F, 0x4163, {0xBF, 0x1A, 0x60, 0xB2, 0x74, 0x05, 0x1E, 0x40}};
#endif

//#define MSG_STARTDRAG (WM_APP+10)

WARNING("Заменить GlobalFree на ReleaseStgMedium, проверить, что оно реально освобождает hGlobal и кучу не рушит");

TODO("Попробовать перевести DragImage на нормальный интерфейс API");
// можно оставить Overlapped для отображения статуса драга
// однако - были какие-то проблемы. Не всплывало окошко при драге во внешние приложения?

CDragDropData::CDragDropData()
{
	m_pfpi = NULL; mp_LastRCon = NULL; mn_PfpiSizeMax = 0;
	mp_DataObject = NULL;
	m_DesktopID.mkid.cb = 0;
	//mh_Overlapped = NULL; mh_BitsDC = NULL; mh_BitsBMP = mh_BitsBMP_Old = NULL;
	#if !defined(USE_DRAG_HELPER)
	mp_Bits = NULL;
	#endif
	mb_DragWithinNow = FALSE;
	mn_ExtractIconsTID = 0;
	mh_ExtractIcons = NULL;
	mb_DragDropRegistered = FALSE;
	/* Unlocked drag support */
	mb_DragStarting = FALSE;
	ms_SourceClass[0] = 0; mh_SourceClass = 0;
	#ifdef USE_DROP_HELPER
	mp_TargetHelper = NULL;
	mb_TargetHelperFailed = false;
	#endif
	#ifdef USE_DRAG_HELPER
	mp_SourceHelper = NULL;
	mb_SourceHelperFailed = false;
	#endif
	mb_selfdrag = false;
	mb_IsUpdatePackage = false;
	mpsz_UpdatePackage = NULL;
	mp_DroppedObject = NULL;
	mp_DraggedVCon = NULL;
}

CDragDropData::~CDragDropData()
{
	#ifdef USE_DROP_HELPER
	if (mp_TargetHelper)
	{
		mp_TargetHelper->Release();
		mp_TargetHelper = NULL;
	}
	#endif

	#ifdef USE_DRAG_HELPER
	if (mp_SourceHelper)
	{
		mp_SourceHelper->Release();
		mp_SourceHelper = NULL;
	}
	#endif

	//DestroyDragImageBits();
	//DestroyDragImageWindow();

	// Final release, no more need
	if (m_pfpi) free(m_pfpi); m_pfpi = NULL;
	mp_LastRCon = NULL;
	if (mpsz_UpdatePackage) free(mpsz_UpdatePackage); mpsz_UpdatePackage = NULL;
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

#ifdef PERSIST_OVL
//	CreateDragImageWindow();
#endif

#ifdef USE_DROP_HELPER
	// Helper (drag image)
	mb_TargetHelperFailed = false;
	// Относительно длительная операция, будем выполнять при необходимости
	//IUnknown* pHelper = NULL;
	//hr = CoCreateInstance(CLSID_DragDropHelper, NULL, CLSCTX_INPROC_SERVER, IID_IUnknown, (void**)&pHelper);
	//if (pHelper)
	//{
	//	//	hr = pHelper->QueryInterface(__uuidof(mp_SourceHelper), (void**)&mp_SourceHelper);
	//	hr = pHelper->QueryInterface(__uuidof(mp_TargetHelper), (void**)&mp_TargetHelper);
	//	pHelper->Release();
	//	pHelper = NULL;
	//}
#endif

#ifdef USE_DRAG_HELPER
	 mb_SourceHelperFailed = false;
#endif

	lbRc = TRUE;
wrap:
	return lbRc;
}

bool CDragDropData::IsDragImageOsSupported()
{
	if (gnOsVer >= 0x601/*Windows7*/)
	{
		// в Windows 7 вроде бы только при включенном Aero (Glass) работают DragImage
		// или это при включении "Classic Theme"?
		//if (!gpConEmu->IsDwm())
		if (!gpConEmu->IsThemed())
			return false;
	}

	return true;
}

bool CDragDropData::UseTargetHelper(bool abSelfDrag)
{
	if (mb_TargetHelperFailed)
	{
		// Если один раз helper обломался - второй не просить
		return false;
	}

	bool lbCanUseHelper = false;
	HRESULT hr = 0;

	#ifdef USE_DROP_HELPER
	if (!IsDragImageOsSupported())
	{
		return false;
	}

	//if (mp_TargetHelper && !abSelfDrag)
	//	lbCanUseHelper = true;

	//HRESULT hr = S_FALSE;

	#ifndef USE_DRAG_HELPER
	if (!abSelfDrag)
	#endif
	{
		if (!mp_TargetHelper)
		{
			IUnknown* pHelper = NULL;
			hr = CoCreateInstance(CLSID_DragDropHelper, NULL, CLSCTX_INPROC_SERVER, IID_IUnknown, (void**)&pHelper);
			if (SUCCEEDED(hr) && pHelper)
			{
				//	hr = pHelper->QueryInterface(__uuidof(mp_SourceHelper), (void**)&mp_SourceHelper);
				hr = pHelper->QueryInterface(IID_IDropTargetHelper/*__uuidof(mp_TargetHelper)*/, (void**)&mp_TargetHelper);
				pHelper->Release();
				pHelper = NULL;
			}
		}

		if (!mp_TargetHelper)
			mb_TargetHelperFailed = true;
		else
			lbCanUseHelper = true;
	}
	#endif

	UNREFERENCED_PARAMETER(hr);
	return lbCanUseHelper;
}

// Это ТОЛЬКО при "своем" драге
bool CDragDropData::UseSourceHelper()
{
#ifndef USE_DRAG_HELPER
	return false;
#else
	if (mb_SourceHelperFailed)
	{
		// Если один раз helper обломался - второй не просить
		return false;
	}

	bool lbCanUseHelper = false;
	HRESULT hr = 0;

	#ifdef USE_DROP_HELPER
	if (!IsDragImageOsSupported())
	{
		return false;
	}

	if (!mp_SourceHelper)
	{
		IUnknown* pHelper = NULL;
		hr = CoCreateInstance(CLSID_DragDropHelper, NULL, CLSCTX_INPROC_SERVER, IID_IUnknown, (void**)&pHelper);
		if (SUCCEEDED(hr) && pHelper)
		{
			hr = pHelper->QueryInterface(IID_IDragSourceHelper, (void**)&mp_SourceHelper);
			pHelper->Release();
			pHelper = NULL;
		}
	}

	if (!mp_SourceHelper)
		mb_SourceHelperFailed = true;
	else
		lbCanUseHelper = true;
	#endif

	UNREFERENCED_PARAMETER(hr);
	return lbCanUseHelper;
#endif
}

// -1 - ошибка
// иначе - размер ASCIIZZ путей файлов в БАЙТАХ
int CDragDropData::RetrieveDragFromInfo(BOOL abClickNeed, COORD crMouseDC, wchar_t** ppszDraggedPath, UINT* pnFilesCount)
{
	DEBUGSTRFAR(L"CDragDropData::RetrieveDragFromInfo()\n");

	CVConGuard VCon;
	if (CVConGroup::GetActiveVCon(&VCon) < 0)
	{
		DEBUGSTRFAR(L"-- failed\n");
		SetDragToInfo(NULL, 0, NULL);
		return -1;
	}

	int size = -1;
	UINT nFilesCount = 0;
	wchar_t *szDraggedPath = NULL;
	bool bInfoSet = false;
	//BOOL lbNeedLBtnUp = !abClickNeed;
	CConEmuPipe pipe(gpConEmu->GetFarPID(), CONEMUREADYTIMEOUT);
	MCHKHEAP

	//if (m_pfpi) {free(m_pfpi); m_pfpi=NULL;}

	if (pipe.Init(_T("CDragDropData::Drag")))
	{
		//DWORD cbWritten=0;
		struct
		{
			BOOL  bClickNeed;
			COORD crMouse;
		} DragArg;
		DragArg.bClickNeed = abClickNeed;
		DragArg.crMouse = VCon->RCon()->ScreenToBuffer(
		                      VCon->ClientToConsole(crMouseDC.X, crMouseDC.Y)
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

				int cbStructSize = 0;
				DWORD cbPfiReadSize = 0;
				ForwardedPanelInfo* pBuf = NULL;

				//if (m_pfpi) {free(m_pfpi); m_pfpi=NULL;}

				if (pipe.Read(&cbStructSize, sizeof(int), &cbBytesRead))
				{
					if ((DWORD)cbStructSize>sizeof(ForwardedPanelInfo))
					{
						pBuf = (ForwardedPanelInfo*)calloc(cbStructSize, 1);

						if (pBuf)
						{
							pipe.Read(pBuf, cbStructSize, &cbPfiReadSize);
						}
					}
				}

				SetDragToInfo(pBuf, cbPfiReadSize, VCon->RCon());
				bInfoSet = true;

				if (pBuf)
				{
					free(pBuf);
				}

				// Сразу закроем
				pipe.Close();
				size=(curr-szDraggedPath+1)*sizeof(wchar_t);
			}
		}
	}

	if (!bInfoSet)
	{
		SetDragToInfo(NULL, 0, NULL);
	}

	*ppszDraggedPath = szDraggedPath;
	*pnFilesCount = nFilesCount;
	return size;
}

BOOL CDragDropData::AddFmt_FileNameW(wchar_t* pszDraggedPath, UINT nFilesCount, int cbSize)
{
	HRESULT hr = mp_DataObject->SetDataInt(CFSTR_FILENAMEW/*L"FileNameW"*/, pszDraggedPath, cbSize);
	return SUCCEEDED(hr);
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

	SAFETRY //__try
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
					if (!pNew)
						break;
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

		if (pItem)
		{
			CoTaskMemFree(pItem);
		}
	}
	SAFECATCH //__except(EXCEPTION_EXECUTE_HANDLER)
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
	mp_DataObject->SetDataInt(CFSTR_SHELLIDLIST, file_PIDLs);
	lbAdded = TRUE;
	file_PIDLs = NULL;

wrap:

	if (pDesktop) { pDesktop->Release(); pDesktop = NULL; }

	if (file_PIDLs) { GlobalFree(file_PIDLs); file_PIDLs = NULL; }

	return lbAdded;
}

BOOL CDragDropData::AddFmt_PREFERREDDROPEFFECT(wchar_t* pszDraggedPath, UINT nFilesCount, int cbSize)
{
	DWORD drop_preferredeffect;

	if (gpConEmu->mouse.state & DRAG_R_STARTED)
		drop_preferredeffect = DROPEFFECT_LINK;
	else if (gpSet->isDefCopy)
		drop_preferredeffect = DROPEFFECT_COPY;
	else
		drop_preferredeffect = DROPEFFECT_MOVE;

	HRESULT hr = mp_DataObject->SetDataInt(CFSTR_PREFERREDDROPEFFECT, &drop_preferredeffect, sizeof(drop_preferredeffect));

	return SUCCEEDED(hr);
}

BOOL CDragDropData::AddFmt_InShellDragLoop(wchar_t* pszDraggedPath, UINT nFilesCount, int cbSize)
{
	DWORD drag_loop = 1;
	HRESULT hr = mp_DataObject->SetDataInt(L"InShellDragLoop", &drag_loop, sizeof(drag_loop));
	return SUCCEEDED(hr);
}

BOOL CDragDropData::AddFmt_HDROP(wchar_t* pszDraggedPath, UINT nFilesCount, int cbSize)
{
	DROPFILES drop_struct = { sizeof(drop_struct), { 0, 0 }, 0, 1 };
	LPBYTE drop_data = (LPBYTE)GlobalAlloc(GPTR, cbSize+sizeof(drop_struct));

	if (!drop_data)
	{
		_ASSERTE(drop_data);
		gpConEmu->DebugStep(_T("DnD: Memory allocation failed!"), TRUE);
		goto wrap;
	}

	memcpy(drop_data, &drop_struct, sizeof(drop_struct));
	memcpy(drop_data + sizeof(drop_struct), pszDraggedPath, cbSize);
	{
		FORMATETC       fmtetc[] =
		{
			{ CF_HDROP, 0, DVASPECT_CONTENT, -1, TYMED_HGLOBAL },
		};
		STGMEDIUM       stgmed[] =
		{
			{ TYMED_HGLOBAL, { (HBITMAP)drop_data }, 0 },
		};
		mp_DataObject->SetData(fmtetc, stgmed, TRUE);
	}
wrap:
	return TRUE;
}

// pszDraggedPath - ASCIIZZ
BOOL CDragDropData::AddFmt_DragImageBits(wchar_t* pszDraggedPath, UINT nFilesCount, int cbSize)
{
	TODO("CFSTR_DROPDESCRIPTION");
#if 0
	//if (SUCCEEDED(hr))
	{
		struct DwordData {
			LPCWSTR sClipName;
			DWORD   nValue;
		} DragOptions[] = {
			// TRUE -- вызовет "создание/отрисовку" стандартной иконки винды
			{L"UsingDefaultDragImage", TRUE},
			// Флаги (Undocumented)
			{L"DragSourceHelperFlags", 1},
			// IsComputingImage
			{L"IsComputingImage", 0},
			// ComputedDragImage
			//{L"ComputedDragImage", 1},
			// IsShowingLayered
			//{L"IsShowingLayered", 0},
			// IsShowingText
			{L"IsShowingText", 1},
			// DisableDragText
			{L"DisableDragText", 0},

			// End
			{NULL}
		};

		HRESULT hrTest = S_OK;

		//for (size_t i = 0; DragOptions[i].sClipName; i++)
		//{
		//	hrTest = mp_DataObject->SetDataInt(DragOptions[i].sClipName, &DragOptions[i].nValue, sizeof(nData));
		//}

		if (gOSVer.dwMajorVersion >= 6)
		{
			// DropDescription: CFSTR_DROPDESCRIPTION, DROPDESCRIPTION, DROPIMAGETYPE
			/*
			pszData = L"ConEmu DropDescription";
			hr = mp_DataObject->SetDataInt(L"DropDescription", pszData, (_tcslen(pszData)+1)*sizeof(*pszData));
			*/
			DROPDESCRIPTION desc = {DROPIMAGE_INVALID};
			#ifdef _DEBUG
			wcscpy_c(desc.szInsert, L"Kudato");
			wcscpy_c(desc.szMessage, L"Dragging %1");
			#endif
			hrTest = mp_DataObject->SetDataInt(CFSTR_DROPDESCRIPTION, &desc, sizeof(desc));
		}

		UNREFERENCED_PARAMETER(hrTest);
	}
#endif

	HRESULT hr = E_FAIL;
	BOOL DragFullWindows = (BOOL)-1;
	SystemParametersInfo(SPI_GETDRAGFULLWINDOWS, 0, &DragFullWindows, 0);

#if defined(USE_DRAG_HELPER)
	if (UseSourceHelper())
	{
		HDC hDrawDC = NULL;
		HBITMAP hBitmap = NULL, hOldBitmap = NULL;
		POINT ptCursor = {0,0};
		int nMaxX = 0, nMaxY = 0;
		COLORREF clrKey = RGB(1,2,3); // Something dark, but not black

		if (PaintDragImageBits(pszDraggedPath, hDrawDC, hBitmap, hOldBitmap, MAX_OVERLAY_WIDTH, MAX_OVERLAY_HEIGHT, &nMaxX, &nMaxY, &ptCursor, true, clrKey))
		{
			//TODO: Need GCC check!
			SHDRAGIMAGE info = {
				{nMaxX, nMaxY},
				{ptCursor.x, ptCursor.y},
				hBitmap, clrKey
			};

			#ifdef _DEBUG
			bool bDraw = false;
			if (bDraw)
			{
				HDC hDc = GetWindowDC(ghWnd);
				BitBlt(hDc, 50,100, nMaxX, nMaxY, hDrawDC, 0,0, SRCCOPY);
				ReleaseDC(ghWnd, hDc);
				GdiFlush();
			}
			#endif

			SelectObject(hDrawDC, hOldBitmap);
			DeleteDC(hDrawDC);

			IDragSourceHelper2* pHelper2 = NULL;
			if (SUCCEEDED(mp_SourceHelper->QueryInterface(IID_IDragSourceHelper2, (void**)&pHelper2)))
			{
				TODO("DSH_ALLOWDROPDESCRIPTIONTEXT")
				hr = pHelper2->SetFlags(0/*DSH_ALLOWDROPDESCRIPTIONTEXT*/);
				pHelper2->Release();
			}

			hr = mp_SourceHelper->InitializeFromBitmap(&info, mp_DataObject);

			if (hr == E_FAIL)
			{
				// Юзер мог отключить "Show window contents while dragging"
				Assert(SUCCEEDED(hr) || (DragFullWindows==FALSE));
			}
			else
			{
				Assert(SUCCEEDED(hr));
			}
		}
	}
#endif

	//if (FAILED(hr))
	//{
		//// Must be GlobalAlloc'ed
		//DragImageBits *pDragBits = CreateDragImageBits(pszDraggedPath);

		//if (!pDragBits)
		//	return FALSE;

		//hr = mp_DataObject->SetDataInt(L"DragImageBits", pDragBits);
	//}

	return SUCCEEDED(hr);
}

// Загрузить из фара информацию для Drag
BOOL CDragDropData::PrepareDrag(BOOL abClickNeed, COORD crMouseDC, DWORD* pdwAllowedEffects)
{
	BOOL lbSucceeded = FALSE;
	int  size = -1;
	wchar_t *szDraggedPath = NULL; // ASCIIZZ
	UINT nFilesCount = 0;
	CVConGuard VCon;

	SetDragToInfo(NULL, 0, NULL);

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

	// Т.к. у нас появились сплиты - нужно помнить, из какой консоли вытащили файлы
	CVConGroup::GetActiveVCon(&VCon);
	mp_DraggedVCon = VCon.VCon();

	if (!mp_DataObject)
	{
		gpConEmu->DebugStep(_T("DnD: CreateDataObject failed!"), TRUE);
		goto wrap;
	}

	// Добавление в mp_DataObject перетаскиваемых форматов
	AddFmt_HDROP(szDraggedPath, nFilesCount, size);
	AddFmt_PREFERREDDROPEFFECT(szDraggedPath, nFilesCount, size);
	AddFmt_InShellDragLoop(szDraggedPath, nFilesCount, size);

	// Эти шли последними
	if (AddFmt_SHELLIDLIST(szDraggedPath, nFilesCount, size))
		*pdwAllowedEffects |= DROPEFFECT_LINK;

	AddFmt_FileNameW(szDraggedPath, nFilesCount, size);

	AddFmt_DragImageBits(szDraggedPath, nFilesCount, size);

	MCHKHEAP
	//Освободить память, больше не нужно
	delete szDraggedPath; szDraggedPath=NULL;

	//CFSTR_PREFERREDDROPEFFECT, FD_LINKUI

	// Информация о панелях уже должна быть получена!
	_ASSERTE(m_pfpi && m_pfpi->NoFarConsole==FALSE);
	//// Сразу получим информацию о путях панелей...
	//if (!m_pfpi)  // если это уже не сделали
	//{
	//	// Ошибки пайпа отображаются через MBoxA
	//	RetrieveDragToInfo();
	//}

#if 1
	mb_DragWithinNow = TRUE;
#else
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
#endif

	//gpConEmu->DebugStep(_T("DnD: Finally, DoDragDrop"));
	gpConEmu->DebugStep(NULL);
	lbSucceeded = TRUE;
wrap:
	return lbSucceeded;
}

template <class T> LPITEMIDLIST PidlGetNextItem(
	T pidl
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
	T pidl, HANDLE hDumpFile = NULL
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
	//=== codeanalyze: Excessive stack usage

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
		for (i = 0; i < nCnt; i++)
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

			if (hr == S_OK)
			{
				if (stg[i].tymed == TYMED_HGLOBAL)
				{
					if (stg[i].hGlobal)
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
					nCurLen = _tcslen(szName[i]);
					_wsprintf(szName[i]+nCurLen, SKIPLEN(countof(szName[i])-nCurLen) L", Error in source! TYMED_HGLOBAL was requested, but got (%i)", stg[i].tymed);
					ReleaseStgMedium(stg+i);
					stg[i].hGlobal = NULL;
				}
			}
			else
			{
				nCurLen = _tcslen(szName[i]);
				_wsprintf(szName[i]+nCurLen, SKIPLEN(countof(szName[i])-nCurLen) L", Can't get TYMED_HGLOBAL, rc=0x%08X", (DWORD)hr);
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

				for (UINT c = 0; c < pcida->cidl; c++)
				{
					LPCITEMIDLIST pidl = HIDA_GetPIDLItem(pcida, c);
					PidlDump(pidl, hDumpFile);
				}
			}
		}

		for (i = 0; i < nCnt; i++)
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

bool CDragDropData::CheckIsUpdatePackage(IDataObject * pDataObject)
{
	bool bHasUpdatePackage = false;

	if (mpsz_UpdatePackage)
	{
		free(mpsz_UpdatePackage);
		mpsz_UpdatePackage = NULL;
	}

	if (gpUpd && gpUpd->InUpdate())
	{
		// Already in Update stage
		goto wrap;
	}
	else
	{
		HRESULT hr;
		STGMEDIUM stgMedium = { 0 };
		FORMATETC fmtetc = { (CLIPFORMAT)RegisterClipboardFormat(CFSTR_FILENAMEW/*L"FileNameW"*/), 0, DVASPECT_CONTENT, -1, TYMED_HGLOBAL };

		hr = pDataObject->QueryGetData(&fmtetc);

		if (SUCCEEDED(hr))
		{
			hr = pDataObject->GetData(&fmtetc, &stgMedium);

			if (SUCCEEDED(hr) && stgMedium.hGlobal)
			{
				// Нас интересует только первый файл (больше одного быть не может)
				LPCWSTR pszFileNames = (LPCWSTR)GlobalLock(stgMedium.hGlobal);

				if (pszFileNames && *pszFileNames)
				{
					bHasUpdatePackage = CConEmuUpdate::IsUpdatePackage(pszFileNames);

					if (bHasUpdatePackage)
					{
						mpsz_UpdatePackage = lstrdup(pszFileNames);
					}
				}

				if (pszFileNames)
				{
					GlobalUnlock(stgMedium.hGlobal);
				}

				ReleaseStgMedium(&stgMedium);
			}
		}
	}

wrap:
	mb_IsUpdatePackage = bHasUpdatePackage;
	return bHasUpdatePackage;
}

bool CDragDropData::NeedRefreshToInfo(POINTL ptScreen)
{
	POINT pt = {ptScreen.x, ptScreen.y};

	#ifdef _DEBUG
	static int nLastX, nLastY;
	wchar_t szDbgInfo[128]; _wsprintf(szDbgInfo, SKIPLEN(countof(szDbgInfo)) L"CDragDropData::NeedRefreshToInfo(%i,%i)\n", ptScreen.x, ptScreen.y);
	bool bShow = (nLastX != ptScreen.x || nLastY != ptScreen.y);
	if (bShow)
	{
		DEBUGSTRFAR(szDbgInfo);
		nLastX = ptScreen.x; nLastY = ptScreen.y;
	}
	#endif

	CVConGuard VCon;
	CRealConsole* pRCon = NULL;
	if (CVConGroup::GetVConFromPoint(pt, &VCon))
		pRCon = VCon->RCon();

	bool bNeedRefresh = (pRCon != mp_LastRCon) || (m_pfpi == NULL);

	if (bNeedRefresh && pRCon)
	{
		// When drag over INACTIVE pane (split-screen) - need to activate this VCon
		if (!VCon->isActive(false))
		{
			DEBUGSTRFAR(L"-- need to refresh and activate console\n");
			CVConGroup::Activate(VCon.VCon());
		}
		else
		{
			DEBUGSTRFAR(L"-- need to refresh, but console already active\n");
		}
	}
	else
	{
		#ifdef _DEBUG
		if (bShow)
		{
			DEBUGSTRFAR(L"-- no need to refresh\n");
		}
		#endif
	}

	mp_LastRCon = pRCon;

	return bNeedRefresh;
}

void CDragDropData::SetDragToInfo(const ForwardedPanelInfo* pInfo, size_t cbInfoSize, CRealConsole* pRCon)
{
	if (m_pfpi && (cbInfoSize > mn_PfpiSizeMax))
	{
		// Need larger buffer
		free(m_pfpi); m_pfpi=NULL;
	}

	mp_LastRCon = pRCon;

	if (!pInfo || (cbInfoSize <= sizeof(ForwardedPanelInfo)) || pInfo->NoFarConsole)
	{
		DEBUGSTRFAR(pInfo ? L"CDragDropData::SetDragToInfo(NoFarConsole)\n" : L"CDragDropData::SetDragToInfo(NULL)\n");

		if (!m_pfpi)
		{
			mn_PfpiSizeMax = sizeof(ForwardedPanelInfo)+(MAX_PATH*5*sizeof(wchar_t));
			m_pfpi = (ForwardedPanelInfo*)calloc(1,mn_PfpiSizeMax);
		}

		if (!m_pfpi)
		{
			DEBUGSTRFAR(L"-- skipped, m_pfpi==NULL\n");
			return;
		}

		m_pfpi->NoFarConsole = TRUE;
		if (pRCon)
		{
			m_pfpi->ActiveRect.right = pRCon->TextWidth() - 1;
			m_pfpi->ActiveRect.bottom = pRCon->TextHeight() - 1;
		}
		else
		{
			// Если консоли нет - то и бросать некуда
			m_pfpi->ActiveRect.right = 0;
			m_pfpi->ActiveRect.bottom = 0;
		}
		wcscpy_c(m_pfpi->szDummy, L"*");
		m_pfpi->pszActivePath = m_pfpi->szDummy;
		m_pfpi->pszPassivePath = m_pfpi->szDummy+1;
		m_pfpi->ActivePathShift = (int)(((LPBYTE)m_pfpi->pszActivePath) - ((LPBYTE)m_pfpi));
		m_pfpi->PassivePathShift = (int)(((LPBYTE)m_pfpi->pszPassivePath) - ((LPBYTE)m_pfpi));
		//m_pfpi->ActivePathShift = sizeof(ForwardedPanelInfo);
		//m_pfpi->pszActivePath = (WCHAR*)(((char*)m_pfpi)+m_pfpi->ActivePathShift);
		//m_pfpi->PassivePathShift = sizeof(ForwardedPanelInfo)+sizeof(wchar_t);
		//m_pfpi->pszPassivePath = (WCHAR*)(((char*)m_pfpi)+m_pfpi->PassivePathShift);
		//m_pfpi->pszActivePath[0] = L'*';
		DEBUGSTRFAR(L"-- dummy path was set\n");
		return;
	}

	// Reserve additional space
	mn_PfpiSizeMax = cbInfoSize+(MAX_PATH*5*sizeof(wchar_t));
	m_pfpi = (ForwardedPanelInfo*)calloc(1, mn_PfpiSizeMax);
	if (!m_pfpi)
	{
		DEBUGSTRFAR(L"CDragDropData::SetDragToInfo() memory allocation failed\n");
		return;
	}

	memmove(m_pfpi, pInfo, cbInfoSize);

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

	DEBUGSTRFAR(L"CDragDropData::SetDragToInfo()\n");
	DEBUGSTRFAR(m_pfpi->pszActivePath);
	DEBUGSTRFAR(m_pfpi->pszPassivePath);

	// Done
}

// Загрузить из фара информацию для Drop
// Требуется только при Drop из внешних приложений!
void CDragDropData::RetrieveDragToInfo(POINTL pt)
{
	DEBUGSTRFAR(L"CDragDropData::RetrieveDragFromInfo()\n");
	//if (m_pfpi) {free(m_pfpi); m_pfpi=NULL;}

	CVConGuard VCon;
	CVirtualConsole* pVCon = NULL;
	CRealConsole* pRCon = NULL;
	POINT ptScreen = {pt.x, pt.y};

	if (!CVConGroup::GetVConFromPoint(ptScreen, &VCon)
		|| ((pVCon = VCon.VCon()) == NULL)
		|| ((pRCon = pVCon->RCon()) == NULL))
	{
		DEBUGSTRFAR(L"CDragDropData::RetrieveDragFromInfo() -> NULL\n");
		SetDragToInfo(NULL, 0, NULL);
		return;
	}

	CEFarWindowType tabType = pRCon->GetActiveTabType();
	DWORD nFarPID = pRCon->GetFarPID(TRUE);
	UNREFERENCED_PARAMETER(tabType);
	bool bInfoSet = false;

	if (nFarPID == 0)
	{
		//SetDragToInfo(NULL, 0, pRCon);
		DEBUGSTRFAR(L"CDragDropData::RetrieveDragFromInfo() -> (nFarPID == 0)\n");
	}
	else if (!pRCon->isAlive())
	{
		DEBUGSTRFAR(L"CDragDropData::RetrieveDragFromInfo() -> (!pRCon->isAlive())\n");
		gpConEmu->DebugStep(_T("DnD: Far is not alive, drop disabled"));
		//Don't show assert (debug) - simple text may be dropped
		//_ASSERTE(FALSE && "DnD: Far is not alive, drop disabled");
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

				//if (m_pfpi) {free(m_pfpi); m_pfpi=NULL;}

				if (pipe.Read(&cbStructSize, sizeof(int), &cbBytesRead))
				{
					if ((DWORD)cbStructSize>=sizeof(ForwardedPanelInfo))
					{
						ForwardedPanelInfo* pBuf = (ForwardedPanelInfo*)calloc(cbStructSize, 1);

						if (pBuf)
						{
							pipe.Read(pBuf, cbStructSize, &cbBytesRead);
						}

						TODO("можно бы получать координаты диалога/полей/редактора, чтобы бросать только в него...");

						SetDragToInfo(pBuf, cbBytesRead, pRCon);
						bInfoSet = true;

						if (pBuf)
						{
							free(pBuf);
						}
					}
					else
					{
						DEBUGSTRFAR(L"CDragDropData::RetrieveDragFromInfo() -> cbStructSize failed\n");
					}
				}
				else
				{
					DEBUGSTRFAR(L"CDragDropData::RetrieveDragFromInfo() -> pipe.Read failed\n");
				}
			}
			else
			{
				DEBUGSTRFAR(L"CDragDropData::RetrieveDragFromInfo() -> pipe.Execute failed\n");
			}
		}
		else
		{
			DEBUGSTRFAR(L"CDragDropData::RetrieveDragFromInfo() -> pipe.Init failed\n");
		}

		gpConEmu->DebugStep(NULL);
	}

	if (!bInfoSet)
	{
		SetDragToInfo(NULL, 0, pRCon);
	}
}

#if 0
BOOL CDragDropData::CreateDragImageBits(IDataObject * pDataObject)
{
	AssertMsg(L"CreateDragImageBits(IDataObject) - NOT IMPLEMENTED\n");
	return FALSE;
}
#endif

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

#if 0
bool CDragDropData::AllocDragImageBits()
{
	if (mp_Bits)
	{
		_ASSERTE(mp_Bits==NULL);
		SafeFree(mp_Bits);
	}

	mp_Bits = (DragImageBits*)calloc(sizeof(DragImageBits),1);

	return (mp_Bits!=NULL);
}
#endif

BOOL CDragDropData::PaintDragImageBits(wchar_t* pszFiles, HDC& hDrawDC, HBITMAP& hBitmap, HBITMAP& hOldBitmap, int nWidth, int nHeight, int* pnMaxX, int* pnMaxY, POINT* ptCursor, bool bUseColorKey, COLORREF clrKey)
{
	HDC hScreenDC = ::GetDC(NULL);
	bool bCreateDC = false; //, bCreateBmp = false;

	if (!hDrawDC)
	{
		hDrawDC = CreateCompatibleDC(hScreenDC);
		if (!hDrawDC)
		{
			_ASSERTE(hDrawDC);
			if (hScreenDC) ::ReleaseDC(0, hScreenDC);
			return FALSE;
		}
		bCreateDC = true;
	}

	if (!hBitmap)
	{
		hBitmap = CreateCompatibleBitmap(hScreenDC, nWidth, nHeight);
		if (!hBitmap)
		{
			_ASSERTE(hBitmap);
			if (bCreateDC) DeleteDC(hDrawDC);
			if (hScreenDC) ::ReleaseDC(0, hScreenDC);
			return FALSE;
		}
		hOldBitmap = (HBITMAP)SelectObject(hDrawDC, hBitmap);
	}

	if (hScreenDC)
	{
		::ReleaseDC(0, hScreenDC);
		hScreenDC = NULL;
	}

	HFONT hOldF = NULL, hf = CreateFont(14, 0, 0, 0, 400, 0, 0, 0, CP_ACP, 0, 0, 0, 0, L"Tahoma");

	if (hf) hOldF = (HFONT)SelectObject(hDrawDC, hf);

	int nMaxX = 0, nMaxY = 0;
	// GetTextExtentPoint32 почему-то портит DC, поэтому ширину получим сначала
	wchar_t *psz = pszFiles; int nFilesCol = 0;

	while (*psz)
	{
		SIZE sz = {0};
		LPCWSTR pszText = PointToName(psz);

		GetTextExtentPoint32(hDrawDC, pszText, _tcslen(pszText), &sz); //-V107

		if (sz.cx > nWidth)
			sz.cx = nWidth;

		if (sz.cx > nMaxX)
			nMaxX = sz.cx;

		psz += _tcslen(psz)+1; // длина полного пути и длина имени файла разные ;)
		nFilesCol ++;
	}

	nMaxX = min((OVERLAY_TEXT_SHIFT + nMaxX),nWidth);
	// Если тащат много файлов/папок - можно попробовать разместить их в несколько колонок
	int nColCount = 1;
	// Win8 bug.
	// It does not cares about difference in struct sizes between 64/32 bit processes.
	// Need to reserve one pix to avoid cut/trunsfer first pixels row.
	int nX = 1;

	if (nFilesCol > 3)
	{
		if (nFilesCol > 21 && (nMaxX * 3 + 32) <= nWidth) //-V112
		{
			nFilesCol = (nFilesCol+3) / (nColCount = 4); // располагаем в 4 колонки //-V112
		}
		else if (nFilesCol > 12 && (nMaxX * 2 + 32) <= nWidth) //-V112
		{
			nFilesCol = (nFilesCol+2) / (nColCount = 3); // располагаем в 3 колонки
		}
		else if ((nMaxX + 32) <= nWidth) //-V112
		{
			nFilesCol = (nFilesCol+1) / (nColCount = 2); // располагаем в 2 колонки
		}
	}

	hOldBitmap = (HBITMAP)SelectObject(hDrawDC, hBitmap);
	SetTextColor(hDrawDC, RGB(0,0,128)); // Темно синий текст
	//SetTextColor(hDrawDC, RGB(255,255,255)); // Белый текст
	SetBkColor(hDrawDC, RGB(192,192,192)); // на сером фоне
	SetBkMode(hDrawDC, OPAQUE);
	// DC может быть испорчен (почему?), поэтому лучше почистим его
	RECT rcFill = MakeRect(nWidth,nHeight);

	if (bUseColorKey)
	{
		HBRUSH hBr = CreateSolidBrush(clrKey/*OVERLAY_COLOR*/);
		FillRect(hDrawDC, &rcFill, hBr);
		DeleteObject(hBr);
	}
	else
	{
		FillRect(hDrawDC, &rcFill, (HBRUSH)GetStockObject(RELEASEDEBUGTEST(BLACK_BRUSH,BLACK_BRUSH/*WHITE_BRUSH*/)));
	}

	psz = pszFiles;
	int nFileIdx = 0, nColMaxY = 0, nAllFiles = 0;
	nMaxX = 0;
	int nFirstColWidth = 0;

	while (*psz)
	{
		if (!DrawImageBits(hDrawDC, psz, &nMaxX, nX, &nColMaxY))
			break; // вышли за пределы nWidth x nHeight (по высоте)

		psz += _tcslen(psz)+1;

		if (!*psz) break;

		nFileIdx ++; nAllFiles ++;

		if (nFileIdx >= nFilesCol)
		{
			if (!nFirstColWidth)
				nFirstColWidth = nMaxX + OVERLAY_TEXT_SHIFT;

			if ((nX + nMaxX + 32) >= nWidth) //-V112
				break;

			nX += nMaxX + OVERLAY_TEXT_SHIFT + OVERLAY_COLUMN_SHIFT;

			if (nColMaxY > nMaxY) nMaxY = nColMaxY;

			nColMaxY = nMaxX = nFileIdx = 0;
		}
	}

	if (nColMaxY > nMaxY) nMaxY = nColMaxY;

	if (!nFirstColWidth) nFirstColWidth = nMaxX + OVERLAY_TEXT_SHIFT;

	int nLineX = ((nX+nMaxX+OVERLAY_TEXT_SHIFT+3)>>2)<<2;

	if (nLineX > nWidth) nLineX = nWidth;

	SelectObject(hDrawDC, hOldF);
	//GdiFlush();

	#ifdef DEBUG_DUMP_IMAGE
	DumpImage(hDrawDC, NULL, nWidth, nMaxY, L"T:\\DragDump.bmp");
	#endif

	//if (nLineX>2 && nMaxY>2)
	//{
	//	HDC hBitsDC = CreateCompatibleDC(hScreenDC);

	//	if (hBitsDC && hDrawDC)
	//	{
	//		SetLayout(hBitsDC, LAYOUT_BITMAPORIENTATIONPRESERVED);
	//		BITMAPINFOHEADER bih = {sizeof(BITMAPINFOHEADER)};
	//		bih.biWidth = nLineX;
	//		bih.biHeight = nMaxY;
	//		bih.biPlanes = 1;
	//		bih.biBitCount = 32;
	//		bih.biCompression = BI_RGB;
	//		MyRgbQuad* pSrc = NULL;
	//		HBITMAP hBitsBitmap = CreateDIBSection(hScreenDC, (BITMAPINFO*)&bih, DIB_RGB_COLORS, (void**)&pSrc, NULL, 0);

	//		DragImageBits* pDst = NULL;
	//		bool UseDragHelper = false;

	//		if (hBitsBitmap)
	//		{
	//			HBITMAP hOldBitsBitmap = (HBITMAP)SelectObject(hBitsDC, hBitsBitmap);
	//			BitBlt(hBitsDC, 0,0,nLineX,nMaxY, hDrawDC,0,0, SRCCOPY);
	//			GdiFlush();

	//			#if defined(USE_DRAG_HELPER)
	//			{
	//				// Local, use DragHelper
	//				pDst = (DragImageBits*)calloc(sizeof(DragImageBits),1);
	//				UseDragHelper = true;
	//			}
	//			#else
	//			{
	//				pDst = (DragImageBits*)GlobalAlloc(GPTR, sizeof(DragImageBits) + (nLineX*nMaxY - 1)*sizeof(RGBQUAD));
	//				UseDragHelper = false;
	//			}
	//			#endif


	//			if (pDst)
	//			{
	//				pDst->nWidth = nLineX;
	//				pDst->nHeight = nMaxY;

	//				if (nColCount == 1)
	//					pDst->nXCursor = nMaxX / 2;
	//				else
	//					pDst->nXCursor = nFirstColWidth;

	//				pDst->nYCursor = 17; // под первой строкой
	//				pDst->nRes1 = GetTickCount(); // что-то непонятное. Random?
	//				pDst->nRes2 = (DWORD)-1;

	//				const u32 nCurBlend = OVERLAY_ALPHA, nAllBlend = OVERLAY_ALPHA;

	//				INT_PTR PixCount = nMaxY * nLineX;
	//				Assert(PixCount>0);

	//				#if defined(USE_DRAG_HELPER)
	//				{
	//					MyRgbQuad *pRGB = pSrc;

	//					for (INT_PTR i = PixCount; i--;)
	//					{
	//						if (pRGB->dwValue)
	//						{
	//						#if !defined(USE_ALPHA_OVL)
	//							if (pRGB->dwValue == OVERLAY_COLOR)
	//							{
	//								WARNING("OVERLAY_COLOR?");
	//								//pRGB->dwValue = 0;
	//							}
	//							else
	//							{
	//						#endif

	//								pRGB->rgbBlue = klMulDivU32(pRGB->rgbBlue, nCurBlend, 0xFF);
	//								pRGB->rgbGreen = klMulDivU32(pRGB->rgbGreen, nCurBlend, 0xFF);
	//								pRGB->rgbRed = klMulDivU32(pRGB->rgbRed, nCurBlend, 0xFF);
	//								pRGB->rgbAlpha = nAllBlend;

	//						#if !defined(USE_ALPHA_OVL)
	//							}
	//						#endif
	//						}

	//						pRGB++;
	//					}
	//				}
	//				#else
	//				{
	//					MyRgbQuad *pRGB = (MyRgbQuad*)pDst->pix;

	//					for (INT_PTR i = PixCount; i--;)
	//					{
	//						//pRGB->rgbBlue = *(pSrc++);
	//						//pRGB->rgbGreen = *(pSrc++);
	//						//pRGB->rgbRed = *(pSrc++);

	//						if (pSrc->dwValue)
	//						{
	//						#if !defined(USE_ALPHA_OVL)

	//							if (pRGB->dwValue == OVERLAY_COLOR)
	//							{
	//								WARNING("OVERLAY_COLOR?");
	//								//pRGB->dwValue = 0;
	//							}
	//							else
	//							{
	//						#endif
	//								pRGB->rgbBlue = klMulDivU32(pSrc->rgbBlue, nCurBlend, 0xFF);
	//								pRGB->rgbGreen = klMulDivU32(pSrc->rgbGreen, nCurBlend, 0xFF);
	//								pRGB->rgbRed = klMulDivU32(pSrc->rgbRed, nCurBlend, 0xFF);
	//								pRGB->rgbAlpha = nAllBlend;
	//						#if !defined(USE_ALPHA_OVL)
	//							}
	//						#endif
	//						}

	//						pRGB++; pSrc++;
	//					}
	//				}
	//				#endif

	//				if (AllocDragImageBits())
	//				{
	//					*mp_Bits = *pDst;
	//					pBits = pDst; pDst = NULL; //OK
	//					mh_BitsDC = hBitsDC; hBitsDC = NULL;
	//					mh_BitsBMP = hBitsBitmap; hBitsBitmap = NULL;
	//				}
	//				else
	//				{
	//					if (pDst) GlobalFree(pDst); pDst = NULL;  // Ошибка
	//				}
	//			}

	//			Assert((hBitsDC==NULL) && (hBitsBitmap==NULL) && "AllocDragImageBits was not succeeded");

	//			if (hBitsDC)
	//			{
	//				SelectObject(hBitsDC, hOldBitsBitmap);
	//			}

	//			if (hBitsBitmap)
	//			{
	//				DeleteObject(hBitsBitmap); hBitsBitmap = NULL;
	//			}
	//		}

	//		UNREFERENCED_PARAMETER(UseDragHelper);
	//	}

	//	if (hBitsDC)
	//	{
	//		DeleteDC(hBitsDC); hBitsDC = NULL;
	//	}
	//}

	//-- this is responsibility of calling method!
	//SelectObject(hDrawDC, hOldBitmap);

	if (pnMaxX) *pnMaxX  = nLineX;
	if (pnMaxY) *pnMaxY  = nMaxY;
	if (ptCursor)
	{
		if (nColCount == 1)
			ptCursor->x = nMaxX / 2;
		else
			ptCursor->x = nFirstColWidth;

		ptCursor->y = 17; // под первой строкой
	}
	return TRUE;
}

#if 0
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

#if 0
	if (abForceToTop)
	{
		MoveDragWindowToTop();
		SetForegroundWindow(ghWnd);
	}
#endif

	DEBUGSTROVL(L"CreateDragImageBits()\n");
	DragImageBits* pBits = NULL;
	HDC hScreenDC = ::GetDC(0);
	HDC hDrawDC = NULL;
	HBITMAP hBitmap = NULL, hOldBitmap = NULL;
	int nMaxX = 0, nMaxY = 0;
	POINT ptCursor = {0,0};

	if (PaintDragImageBits(pszFiles, hDrawDC, hBitmap, hOldBitmap, MAX_OVERLAY_WIDTH, MAX_OVERLAY_HEIGHT,
		&nMaxX, &nMaxY, &ptCursor,
		#ifdef USE_ALPHA_OVL
		false, 0
		#else
		true, OVERLAY_COLOR
		#endif
		))
	{
		//HFONT hOldF = NULL, hf = CreateFont(14, 0, 0, 0, 400, 0, 0, 0, CP_ACP, 0, 0, 0, 0, L"Tahoma");

		//if (hf) hOldF = (HFONT)SelectObject(hDrawDC, hf);

		//int nMaxX = 0, nMaxY = 0, nX = 0;
		//// GetTextExtentPoint32 почему-то портит DC, поэтому ширину получим сначала
		//wchar_t *psz = pszFiles; int nFilesCol = 0;

		//while (*psz)
		//{
		//	SIZE sz = {0};
		//	LPCWSTR pszText = PointToName(psz);

		//	GetTextExtentPoint32(hDrawDC, pszText, _tcslen(pszText), &sz); //-V107

		//	if (sz.cx > MAX_OVERLAY_WIDTH)
		//		sz.cx = MAX_OVERLAY_WIDTH;

		//	if (sz.cx > nMaxX)
		//		nMaxX = sz.cx;

		//	psz += _tcslen(psz)+1; // длина полного пути и длина имени файла разные ;)
		//	nFilesCol ++;
		//}

		//nMaxX = min((OVERLAY_TEXT_SHIFT + nMaxX),MAX_OVERLAY_WIDTH);
		//// Если тащат много файлов/папок - можно попробовать разместить их в несколько колонок
		//int nColCount = 1;

		//if (nFilesCol > 3)
		//{
		//	if (nFilesCol > 21 && (nMaxX * 3 + 32) <= MAX_OVERLAY_WIDTH) //-V112
		//	{
		//		nFilesCol = (nFilesCol+3) / (nColCount = 4); // располагаем в 4 колонки //-V112
		//	}
		//	else if (nFilesCol > 12 && (nMaxX * 2 + 32) <= MAX_OVERLAY_WIDTH) //-V112
		//	{
		//		nFilesCol = (nFilesCol+2) / (nColCount = 3); // располагаем в 3 колонки
		//	}
		//	else if ((nMaxX + 32) <= MAX_OVERLAY_WIDTH) //-V112
		//	{
		//		nFilesCol = (nFilesCol+1) / (nColCount = 2); // располагаем в 2 колонки
		//	}
		//}

		//HBITMAP hOldBitmap = (HBITMAP)SelectObject(hDrawDC, hBitmap);
		//SetTextColor(hDrawDC, RGB(0,0,128)); // Темно синий текст
		////SetTextColor(hDrawDC, RGB(255,255,255)); // Белый текст
		//SetBkColor(hDrawDC, RGB(192,192,192)); // на сером фоне
		//SetBkMode(hDrawDC, OPAQUE);
		//// DC может быть испорчен (почему?), поэтому лучше почистим его
		//RECT rcFill = MakeRect(MAX_OVERLAY_WIDTH,MAX_OVERLAY_HEIGHT);

		//#if !defined(USE_ALPHA_OVL)
		//HBRUSH hBr = CreateSolidBrush(OVERLAY_COLOR);
		//FillRect(hDrawDC, &rcFill, hBr);
		//DeleteObject(hBr);
		//#else
		//FillRect(hDrawDC, &rcFill, (HBRUSH)GetStockObject(RELEASEDEBUGTEST(BLACK_BRUSH,BLACK_BRUSH/*WHITE_BRUSH*/)));
		//#endif

		//psz = pszFiles;
		//int nFileIdx = 0, nColMaxY = 0, nAllFiles = 0;
		//nMaxX = 0;
		//int nFirstColWidth = 0;

		//while (*psz)
		//{
		//	if (!DrawImageBits(hDrawDC, psz, &nMaxX, nX, &nColMaxY))
		//		break; // вышли за пределы MAX_OVERLAY_WIDTH x MAX_OVERLAY_HEIGHT (по высоте)

		//	psz += _tcslen(psz)+1;

		//	if (!*psz) break;

		//	nFileIdx ++; nAllFiles ++;

		//	if (nFileIdx >= nFilesCol)
		//	{
		//		if (!nFirstColWidth)
		//			nFirstColWidth = nMaxX + OVERLAY_TEXT_SHIFT;

		//		if ((nX + nMaxX + 32) >= MAX_OVERLAY_WIDTH) //-V112
		//			break;

		//		nX += nMaxX + OVERLAY_TEXT_SHIFT + OVERLAY_COLUMN_SHIFT;

		//		if (nColMaxY > nMaxY) nMaxY = nColMaxY;

		//		nColMaxY = nMaxX = nFileIdx = 0;
		//	}
		//}

		//if (nColMaxY > nMaxY) nMaxY = nColMaxY;

		//if (!nFirstColWidth) nFirstColWidth = nMaxX + OVERLAY_TEXT_SHIFT;

		//int nLineX = ((nX+nMaxX+OVERLAY_TEXT_SHIFT+3)>>2)<<2;

		//if (nLineX > MAX_OVERLAY_WIDTH) nLineX = MAX_OVERLAY_WIDTH;

		//SelectObject(hDrawDC, hOldF);
		//GdiFlush();

		//#ifdef DEBUG_DUMP_IMAGE
		//DumpImage(hDrawDC, NULL, MAX_OVERLAY_WIDTH, nMaxY, L"T:\\DragDump.bmp");
		//#endif

		if (nMaxX>2 && nMaxY>2)
		{
			HDC hBitsDC = CreateCompatibleDC(hScreenDC);

			if (hBitsDC && hDrawDC)
			{
				SetLayout(hBitsDC, LAYOUT_BITMAPORIENTATIONPRESERVED);
				BITMAPINFOHEADER bih = {sizeof(BITMAPINFOHEADER)};
				bih.biWidth = nMaxX;
				bih.biHeight = nMaxY;
				bih.biPlanes = 1;
				bih.biBitCount = 32;
				bih.biCompression = BI_RGB;
				MyRgbQuad* pSrc = NULL;
				HBITMAP hBitsBitmap = CreateDIBSection(hScreenDC, (BITMAPINFO*)&bih, DIB_RGB_COLORS, (void**)&pSrc, NULL, 0);

				DragImageBits* pDst = NULL;
				bool UseDragHelper = false;

				if (hBitsBitmap)
				{
					HBITMAP hOldBitsBitmap = (HBITMAP)SelectObject(hBitsDC, hBitsBitmap);
					BitBlt(hBitsDC, 0,0,nMaxX,nMaxY, hDrawDC,0,0, SRCCOPY);
					GdiFlush();

					#if defined(USE_DRAG_HELPER)
					{
						// Local, use DragHelper
						pDst = (DragImageBits*)calloc(sizeof(DragImageBits),1);
						UseDragHelper = true;
					}
					#else
					{
						pDst = (DragImageBits*)GlobalAlloc(GPTR, sizeof(DragImageBits) + (nMaxX*nMaxY - 1)*sizeof(RGBQUAD));
						UseDragHelper = false;
					}
					#endif


					if (pDst)
					{
						pDst->nWidth = nMaxX;
						pDst->nHeight = nMaxY;

						pDst->nXCursor = ptCursor.x;
						pDst->nYCursor = ptCursor.y; // под первой строкой

						pDst->nRes1 = GetTickCount(); // что-то непонятное. Random?
						pDst->nRes2 = (DWORD)-1;

						const u32 nCurBlend = OVERLAY_ALPHA, nAllBlend = OVERLAY_ALPHA;

						INT_PTR PixCount = nMaxY * nMaxX;
						Assert(PixCount>0);

						#if defined(USE_DRAG_HELPER)
						{
							MyRgbQuad *pRGB = pSrc;

							for (INT_PTR i = PixCount; i--;)
							{
								if (pRGB->dwValue)
								{
								#if !defined(USE_ALPHA_OVL)
									if (pRGB->dwValue == OVERLAY_COLOR)
									{
										WARNING("OVERLAY_COLOR?");
										//pRGB->dwValue = 0;
									}
									else
									{
								#endif

										pRGB->rgbBlue = klMulDivU32(pRGB->rgbBlue, nCurBlend, 0xFF);
										pRGB->rgbGreen = klMulDivU32(pRGB->rgbGreen, nCurBlend, 0xFF);
										pRGB->rgbRed = klMulDivU32(pRGB->rgbRed, nCurBlend, 0xFF);
										pRGB->rgbAlpha = nAllBlend;

								#if !defined(USE_ALPHA_OVL)
									}
								#endif
								}

								pRGB++;
							}
						}
						#else
						{
							MyRgbQuad *pRGB = (MyRgbQuad*)pDst->pix;

							for (INT_PTR i = PixCount; i--;)
							{
								//pRGB->rgbBlue = *(pSrc++);
								//pRGB->rgbGreen = *(pSrc++);
								//pRGB->rgbRed = *(pSrc++);

								if (pSrc->dwValue)
								{
								#if !defined(USE_ALPHA_OVL)

									if (pRGB->dwValue == OVERLAY_COLOR)
									{
										WARNING("OVERLAY_COLOR?");
										//pRGB->dwValue = 0;
									}
									else
									{
								#endif
										pRGB->rgbBlue = klMulDivU32(pSrc->rgbBlue, nCurBlend, 0xFF);
										pRGB->rgbGreen = klMulDivU32(pSrc->rgbGreen, nCurBlend, 0xFF);
										pRGB->rgbRed = klMulDivU32(pSrc->rgbRed, nCurBlend, 0xFF);
										pRGB->rgbAlpha = nAllBlend;
								#if !defined(USE_ALPHA_OVL)
									}
								#endif
								}

								pRGB++; pSrc++;
							}
						}
						#endif

						if (AllocDragImageBits())
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

					Assert((hBitsDC==NULL) && (hBitsBitmap==NULL) && "AllocDragImageBits was not succeeded");

					if (hBitsDC)
					{
						SelectObject(hBitsDC, hOldBitsBitmap);
					}

					if (hBitsBitmap)
					{
						DeleteObject(hBitsBitmap); hBitsBitmap = NULL;
					}
				}

				UNREFERENCED_PARAMETER(UseDragHelper);
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
#endif

BOOL CDragDropData::DrawImageBits(HDC hDrawDC, wchar_t* pszFile, int *nMaxX, int nX, int *nMaxY)
{
	if ((*nMaxY + 17) >= MAX_OVERLAY_HEIGHT)
		return FALSE;

	SHFILEINFO sfi = {0};
	DWORD nDrawRC;
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

	UNREFERENCED_PARAMETER(nDrawRC);
	return TRUE;
}

#if 0
BOOL CDragDropData::LoadDragImageBits(IDataObject * pDataObject)
{
	if (UseTargetHelper(false))
		return false;

#ifdef PERSIST_OVL
	if (mh_Overlapped && mp_Bits)
		return FALSE; // уже
#else
	if (/*mb_selfdrag ||*/ mh_Overlapped)
		return FALSE; // уже
#endif

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
	//	fmtetc.cfFormat = RegisterClipboardFormat(CFSTR_FILENAMEW/*L"FileNameW"*/);
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

	if (AllocDragImageBits())
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


				#if defined(PERSIST_OVL) && !defined(USE_ALPHA_OVL)
				MyRgbQuad *pRGB = (MyRgbQuad*)pDst;
				int nMaxY = (bih.biHeight > 0) ? bih.biHeight : -bih.biHeight;
				int nMaxX = bih.biWidth;
				for (int y = 0; y < nMaxY; y++)
				{
					for (int x = 0; x < nMaxX; x++)
					{
						if (pRGB->rgbAlpha <= 10)
						{
							pRGB->dwValue = OVERLAY_COLOR;
						}
						pRGB++;
					}
				}
				#endif


				#ifdef _DEBUG
				int nBits = GetDeviceCaps(mh_BitsDC, BITSPIXEL);
				//_ASSERTE(nBits == 32);
				#endif
				lbRc = TRUE;

				#ifdef DEBUG_DUMP_IMAGE
				DumpImage(&bih, pDst, L"T:\\DropDump.bmp");
				#endif
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
#endif

#if 0
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
	UNREFERENCED_PARAMETER(nWidth); UNREFERENCED_PARAMETER(nHeight);

	HWND hParent = ghWnd;
	//hParent = GetForegroundWindow();

	//#ifdef _DEBUG
	//#undef DRAGBITSTITLE
	//#define DRAGBITSTITLE gpConEmu->ms_ConEmuExe
	//#endif

	// |WS_BORDER|WS_SYSMENU - создает проводник. попробуем?
	//2009-08-20 [+] WS_EX_NOACTIVATE
#if defined(USE_CHILD_OVL)
	if (!ghWnd)
	{
		_ASSERTE(ghWnd!=NULL && "Must be created as child of ghWnd!");
		return FALSE;
	}
	// WS_EX_TRANSPARENT does not fit for our drawing behavior?
	mh_Overlapped = CreateWindowEx(0, DRAGBITSCLASS, DRAGBITSTITLE,
						WS_CHILD|WS_VISIBLE|WS_CLIPSIBLINGS,
	                    -32000, -32000, nWidth, nHeight,
	                    hParent, NULL, g_hInstance, (LPVOID)(CDragDropData*)this);
#elif defined(PERSIST_OVL)
	mh_Overlapped = CreateWindowEx(
	                    WS_EX_LAYERED|WS_EX_TRANSPARENT|WS_EX_PALETTEWINDOW|WS_EX_TOOLWINDOW|WS_EX_TOPMOST|WS_EX_NOACTIVATE,
	                    DRAGBITSCLASS, DRAGBITSTITLE, WS_POPUP|WS_VISIBLE|WS_CLIPSIBLINGS/*|WS_BORDER|WS_SYSMENU*/,
	                    -32000, -32000, nWidth, nHeight,
	                    hParent, NULL, g_hInstance, (LPVOID)(CDragDropData*)this);
#else
	mh_Overlapped = CreateWindowEx(
	                    WS_EX_LAYERED|WS_EX_TRANSPARENT|WS_EX_PALETTEWINDOW|WS_EX_TOOLWINDOW|WS_EX_TOPMOST|WS_EX_NOACTIVATE,
	                    DRAGBITSCLASS, DRAGBITSTITLE, WS_POPUP|WS_VISIBLE|WS_CLIPSIBLINGS|WS_BORDER|WS_SYSMENU,
	                    0, 0, mp_Bits->nWidth, mp_Bits->nHeight, hParent, NULL, g_hInstance, (LPVOID)(CDragDropData*)this);
#endif

	if (!mh_Overlapped)
	{
		_ASSERTE(mh_Overlapped!=NULL);
#ifdef PERSIST_OVL
		//DestroyDragImageBits(); -- смысла уже нет, т.к. создается только при старте
#else
		DestroyDragImageBits();
#endif
		return FALSE;
	}

	#ifdef PERSIST_OVL
	SetLayeredWindowAttributes(mh_Overlapped, OVERLAY_COLOR, OVERLAY_ALPHA, LWA_COLORKEY|LWA_ALPHA);
	#endif

	MoveDragWindowToTop();

	MoveDragWindow(FALSE);
	return TRUE;
}
#endif

void CDragDropData::OnTaskbarCreated()
{
#if 0
	if (mh_Overlapped)
		MoveDragWindowToTop();
#endif
}

#if 0
void CDragDropData::MoveDragWindowToTop()
{
	if (mh_Overlapped)
		SetWindowPos(mh_Overlapped, HWND_TOP, 0,0,0,0, SWP_NOMOVE|SWP_NOSIZE);
}

void CDragDropData::MoveDragWindow(BOOL abVisible/*=TRUE*/)
{
	if (!mh_Overlapped)
	{
		DEBUGSTRBACK(L"MoveDragWindow skipped, Overlay was not created\n");
		ghWndDrag = NULL;
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
		#if defined(USE_CHILD_OVL)
		MapWindowPoints(NULL, ghWnd, &p, 1);
		#endif

		if (mp_Bits)
		{
			p.x -= mp_Bits->nXCursor; p.y -= mp_Bits->nYCursor;
		}

		#if defined(USE_CHILD_OVL)
		ghWndDrag = mh_Overlapped;
		#endif
	}
	else
	{
		p.x = p.y = -32000;
		ghWndDrag = NULL;
	}

	//POINT p2 = {0, 0};
	SIZE  sz = {MAX_OVERLAY_WIDTH, MAX_OVERLAY_HEIGHT};

	if (mp_Bits)
	{
		sz.cx = mp_Bits->nWidth; sz.cy = mp_Bits->nHeight;
	}

	static BOOL bLastVisible = FALSE;
	#if defined(USE_CHILD_OVL)
	SetWindowPos(mh_Overlapped, HWND_TOP, p.x, p.y, sz.cx, sz.cy, SWP_NOACTIVATE|SWP_NOCOPYBITS);
	if (!IsWindowVisible(mh_Overlapped))
	{
		_ASSERTE(FALSE && "DragWindow must be visible");
		apiShowWindow(mh_Overlapped, SW_SHOWNA);
	}
	#else
	MoveWindow(mh_Overlapped, p.x, p.y, sz.cx, sz.cy, abVisible && (abVisible != bLastVisible));
	#endif
	bLastVisible = abVisible;
#else
	#if defined(USE_CHILD_OVL)
		_ASSERTE(FALSE && "Must not be child in this mode!");
	#endif

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
	BOOL bRet = UpdateLayeredWindow(mh_Overlapped, NULL, &p, &sz, mh_BitsDC, &p2, 0,
	                        &bf, ULW_ALPHA /*ULW_OPAQUE*/);
	UNREFERENCED_PARAMETER(bRet);
	//_ASSERTE(bRet);
	//apiSetForegroundWindow(ghWnd); // после создания окна фокус уходит из GUI
#endif
}
#endif

#if 0
void CDragDropData::DestroyDragImageWindow()
{
	if (mh_Overlapped)
	{
		DEBUGSTROVL(L"DestroyDragImageWindow()\n");
		DestroyWindow(mh_Overlapped);
		mh_Overlapped = NULL;
	}
	ghWndDrag = NULL;
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
	else if (messg == WM_ERASEBKGND)
	{
		return 1;
	}
	else if (messg == WM_PAINT)
	{
		CDragDropData *pDrag = (CDragDropData*)GetWindowLongPtr(hWnd, GWLP_USERDATA);

		if (pDrag)
		{
			PAINTSTRUCT ps = {0};
			HDC hdc = BeginPaint(hWnd, &ps);

			if (hdc)
			{
				int nWidth = 0, nHeight = 0;

				#if defined(USE_CHILD_OVL)
				#endif

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
#endif

BOOL CDragDropData::InDragDrop()
{
#ifdef UNLOCKED_DRAG
	PRAGMA_ERROR("Проверить все нити драга, а не только последний mp_DataObject");
#endif

	//!!! mh_Overlapped теперь всегда создается!
	if (mp_DataObject /*|| mh_Overlapped*/ || mb_DragWithinNow)
		return TRUE;

	return FALSE;
}

void CDragDropData::DragFeedBack(DWORD dwEffect)
{
	HRESULT hrHelper = 0;
#ifdef _DEBUG
	wchar_t szDbg[128]; _wsprintf(szDbg, SKIPLEN(countof(szDbg)) L"DragFeedBack(%i)\n", (int)dwEffect);
	DEBUGSTRBACK(szDbg);
#endif

	// Drop или отмена драга, когда источник - ConEmu
	if (dwEffect == DROPEFFECT_STOP_INTERNAL)
	{
		#ifdef USE_DROP_HELPER
		if (UseTargetHelper(mb_selfdrag))
			mp_TargetHelper->DragLeave();
		#endif

		mb_DragWithinNow = FALSE;
//#ifdef PERSIST_OVL
//		MoveDragWindow(FALSE);
//#else
//		DestroyDragImageWindow();
//#endif
//		DestroyDragImageBits();
		gpConEmu->SetDragCursor(NULL);
	}
	else
	{
		TODO("CFSTR_DROPDESCRIPTION");
#if 0
		if (mp_DataObject && (gOSVer.dwMajorVersion >= 6))
		{
			DROPDESCRIPTION desc = {DROPIMAGE_INVALID};
			#ifdef _DEBUG
			wcscpy_c(desc.szInsert, L"Kudato");
			wcscpy_c(desc.szMessage, L"Dragging %1");
			#endif
			if (m_pfpi)
			{
				;
			}

			if (dwEffect == DROPEFFECT_NONE)
			{
				desc.type = DROPIMAGE_NONE;
			}
			else if (dwEffect & DROPEFFECT_COPY)
			{
				desc.type = DROPIMAGE_COPY;
			}
			else if (dwEffect & DROPEFFECT_MOVE)
			{
				desc.type = DROPIMAGE_MOVE;
			}
			else if (dwEffect & DROPEFFECT_LINK)
			{
				desc.type = DROPIMAGE_LINK;
			}
			else
			{
				desc.type = DROPIMAGE_WARNING;
			}

			hrHelper = mp_DataObject->SetDataInt(CFSTR_DROPDESCRIPTION, &desc, sizeof(desc));
		}
#endif

		#ifdef USE_DROP_HELPER
		if (UseTargetHelper(mb_selfdrag))
		{
			POINT ppt = {}; GetCursorPos(&ppt);
			hrHelper = mp_TargetHelper->DragOver(&ppt, dwEffect);
		}
		#endif

		if (dwEffect == DROPEFFECT_NONE)
		{
			//MoveDragWindow(FALSE);
		}
		else
		{
			//if (!mh_Overlapped && mp_Bits)
			//{
			//	#ifdef PERSIST_OVL
			//	//CreateDragImageWindow(); -- должно быть создано при запуске ConEmu
			//	_ASSERTE(mh_Overlapped!=NULL);
			//	#else
			//	CreateDragImageWindow();
			//	#endif
			//}

			//if (mh_Overlapped)
			//{
			//	MoveDragWindow(!UseTargetHelper(mb_selfdrag));
			//}
		}
	}

	//2009-08-20
	//MSG Msg = {NULL};
	//while (PeekMessage(&Msg, NULL, 0, 0, PM_REMOVE))
	//{
	//	ConEmuMsgLogger::Log(Msg);
	//	TranslateMessage(&Msg);
	//	DispatchMessage(&Msg);
	//}

	UNREFERENCED_PARAMETER(hrHelper);
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
	//std::vector<CEDragSource*>::iterator iter;
	CEDragSource* pds = NULL;

	//for (iter = m_Sources.begin(); iter != m_Sources.end(); ++iter)
	for (INT_PTR j = 0; j < m_Sources.size(); j++)
	{
		//pds = *iter;
		pds = m_Sources[j];

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
	//std::vector<CEDragSource*>::iterator iter;
	CEDragSource* pds = NULL;

	//for (iter = m_Sources.begin(); iter != m_Sources.end(); ++iter)
	for (INT_PTR j = 0; j < m_Sources.size(); j++)
	{
		//pds = *iter;
		pds = m_Sources[j];

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
			MoveWindowRect(hWnd, rcWnd);
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
	                           WS_POPUP, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
	                           NULL, NULL, (HINSTANCE)g_hInstance, pds);

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
	while (GetMessage(&msg, 0,0,0))
	{
		ConEmuMsgLogger::Log(msg, ConEmuMsgLogger::msgCommon);
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
		SafeCloseHandle(pds->hReady);
		free(pds);
		return NULL;
	}

	m_Sources.push_back(pds);
	return pds;
}

CDragDropData::CEDragSource* CDragDropData::GetFreeSource()
{
	//std::vector<CEDragSource*>::iterator iter;
	CEDragSource* pds = NULL;

	//for (iter = m_Sources.begin(); iter != m_Sources.end(); ++iter)
	for (INT_PTR j = 0; j < m_Sources.size(); j++)
	{
		//pds = *iter;
		pds = m_Sources[j];

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
	//std::vector<CEDragSource*>::iterator iter;
	CEDragSource* pds = NULL;

	// Сначала во все нити послать QUIT
	//for (iter = m_Sources.begin(); iter != m_Sources.end(); ++iter)
	for (INT_PTR j = 0; j < m_Sources.size(); j++)
	{
		//pds = *iter;
		pds = m_Sources[j];

		if (!pds->hThread) continue;

		if (WaitForSingleObject(pds->hThread,0)!=WAIT_OBJECT_0)
			PostThreadMessage(pds->nTID, WM_QUIT, 0, 0);
	}

	// После этого попробовать немного подождать, и Terminate...
	//for (INT_PTR iter = m_Sources.begin(); iter != m_Sources.end(); iter = m_Sources.erase(iter))
	while (!m_Sources.empty())
	{
		INT_PTR j = m_Sources.size()-1;
		//pds = *iter;
		pds = m_Sources[j];

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

		m_Sources.erase(j);
	}
}
