
//#include "StdAfx.h"
#include <shlobj.h>
#include ".\basedragdrops.h"
#include "resource.h"
#include "header.h"
#include <tchar.h>

extern HINSTANCE g_hInstance;

/*
[HKEY_LOCAL_MACHINE\SOFTWARE\Classes\Interface\{000214E4-0000-0000-C000-000000000046}]
@="IContextMenu"
[HKEY_LOCAL_MACHINE\SOFTWARE\Classes\Interface\{000214F4-0000-0000-C000-000000000046}]
@="IContextMenu2"
[HKEY_LOCAL_MACHINE\SOFTWARE\Classes\Interface\{0811AEBE-0B87-4C54-9E72-548CF649016B}]
@="IContextMenuSite"
[HKEY_LOCAL_MACHINE\SOFTWARE\Classes\Interface\{15F936F5-A3FA-11D2-AEC3-00C04F79D1EB}]
@="IContextNotify"
[HKEY_LOCAL_MACHINE\SOFTWARE\Classes\Interface\{15F936F6-A3FA-11D2-AEC3-00C04F79D1EB}]
@="IContextRegisterNotify"


*/


//CBaseDropTarget::CBaseDropTarget(/*HWND hwnd*/)
//{
//	//m_hWnd = hwnd;
//	m_lRefCount = 1;
//}

CBaseDropTarget::CBaseDropTarget()
{
}

CBaseDropTarget::~CBaseDropTarget()
{
}

HRESULT STDMETHODCALLTYPE CBaseDropTarget::DragEnter(IDataObject * pDataObject,DWORD grfKeyState,POINTL pt,DWORD * pdwEffect)
{
	return 0;
}

HRESULT STDMETHODCALLTYPE CBaseDropTarget::DragOver(DWORD grfKeyState,POINTL pt,DWORD * pdwEffect)
{
	return 0;
}


HRESULT STDMETHODCALLTYPE CBaseDropTarget::Drop (IDataObject * pDataObject,DWORD grfKeyState,POINTL pt,DWORD * pdwEffect)
{
	//gConEmu.SetDragCursor(NULL);
	return 0;
}

HRESULT STDMETHODCALLTYPE CBaseDropTarget::DragLeave(void)
{
	return 0;
}

HRESULT __stdcall CBaseDropTarget::QueryInterface (REFIID iid, void ** ppvObject)
{
	if(iid == IID_IDropTarget || iid == IID_IUnknown)
	{
		AddRef();
		*ppvObject = this;
		return S_OK;
	}
	else
	{
		*ppvObject = 0;
		return E_NOINTERFACE;
	}
}

ULONG __stdcall CBaseDropTarget::AddRef(void)
{
	return InterlockedIncrement(&m_lRefCount);
}	

ULONG __stdcall CBaseDropTarget::Release(void)
{
	LONG count = InterlockedDecrement(&m_lRefCount);
		
	if(count == 0)
	{
		delete this;
		return 0;
	}
	else
	{
		return count;
	}
}




////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////




//
//	Constructor
//
CDropSource::CDropSource(CBaseDropTarget* pCallback)
{
	m_lRefCount = 1;
	mh_CurCopy = NULL; mh_CurMove = NULL; mh_CurLink = NULL;
	mp_Callback = pCallback;
}

//
//	Destructor
//
CDropSource::~CDropSource()
{
}

//
//	IUnknown::AddRef
//
ULONG __stdcall CDropSource::AddRef(void)
{
    // increment object reference count
    return InterlockedIncrement(&m_lRefCount);
}

//
//	IUnknown::Release
//
ULONG __stdcall CDropSource::Release(void)
{
    // decrement object reference count
	LONG count = InterlockedDecrement(&m_lRefCount);
		
	if(count == 0)
	{
		delete this;
		return 0;
	}
	else
	{
		return count;
	}
}

//
//	IUnknown::QueryInterface
//
HRESULT __stdcall CDropSource::QueryInterface(REFIID iid, void **ppvObject)
{
    // check to see what interface has been requested
    if(iid == IID_IDropSource || iid == IID_IUnknown)
    {
        AddRef();
        *ppvObject = this;
        return S_OK;
    }
    else
    {
        *ppvObject = 0;
        return E_NOINTERFACE;
    }
}

//
//	CDropSource::QueryContinueDrag
//
//	Called by OLE whenever Escape/Control/Shift/Mouse buttons have changed
//
HRESULT __stdcall CDropSource::QueryContinueDrag(BOOL fEscapePressed, DWORD grfKeyState)
{
	// if the <Escape> key has been pressed since the last call, cancel the drop
	if(fEscapePressed == TRUE) {
		if (mp_Callback)
			mp_Callback->DragFeedBack((DWORD)-1);
		return DRAGDROP_S_CANCEL;	
	}
		
	DWORD nDragKey = ((gConEmu.mouse.state & DRAG_L_STARTED) == DRAG_L_STARTED) ? MK_LBUTTON : MK_RBUTTON;
	DWORD nOtherKey = ((nDragKey & MK_LBUTTON) == MK_LBUTTON) ? (MK_RBUTTON|MK_MBUTTON) : (MK_LBUTTON|MK_MBUTTON);

	// if the <LeftMouse> button has been released, then do the drop!
	if((grfKeyState & nDragKey) == 0) {
		if (mp_Callback)
			mp_Callback->DragFeedBack((DWORD)-1);
		return DRAGDROP_S_DROP;
	}

	// Если юзер нажимает другую мышиную кнопку
	if((grfKeyState & nOtherKey) == nOtherKey) {
		if (mp_Callback)
			mp_Callback->DragFeedBack((DWORD)-1);
		return DRAGDROP_S_CANCEL;
	}

	// continue with the drag-drop
	return S_OK;
}

//
//	CDropSource::GiveFeedback
//
//	Return either S_OK, or DRAGDROP_S_USEDEFAULTCURSORS to instruct OLE to use the
//  default mouse cursor images
//
HRESULT __stdcall CDropSource::GiveFeedback(DWORD dwEffect)
{
	HRESULT hr = DRAGDROP_S_USEDEFAULTCURSORS;
	HCURSOR hCur = NULL;

	if (dwEffect != DROPEFFECT_NONE)
	{
		if (dwEffect & DROPEFFECT_COPY) {
			if (!mh_CurCopy) mh_CurCopy = LoadCursor(g_hInstance, MAKEINTRESOURCE(IDC_COPY));
			hCur = mh_CurCopy;

		} else if (dwEffect & DROPEFFECT_MOVE) {
			if (!mh_CurMove) mh_CurMove = LoadCursor(g_hInstance, MAKEINTRESOURCE(IDC_MOVE));
			hCur = mh_CurMove;

		} else if (dwEffect & DROPEFFECT_LINK) {
			if (!mh_CurLink) mh_CurLink = LoadCursor(g_hInstance, MAKEINTRESOURCE(IDC_LINK));
			hCur = mh_CurLink;

		}
	} else {
		hCur = LoadCursor(NULL, IDC_NO);
	}
	
	gConEmu.SetDragCursor(hCur);

	//if (hCur) {
	//SetCursor(hCur);
	hr = S_OK;
	//}

	if (mp_Callback)
		mp_Callback->DragFeedBack(dwEffect);

	return hr;
}

//
//	Helper routine to create an IDropSource object
//	
HRESULT CreateDropSource(IDropSource **ppDropSource, CBaseDropTarget* pCallback)
{
	if(ppDropSource == 0)
		return E_INVALIDARG;

	*ppDropSource = new CDropSource(pCallback);

	return (*ppDropSource) ? S_OK : E_OUTOFMEMORY;

}




////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////

//
//	Constructor
//
CDataObject::CDataObject(FORMATETC *fmtetc, STGMEDIUM *stgmed, int count) 
{
	m_lRefCount  = 1;
	m_nMaxNumFormats = 32 + max(32,count);
	m_nNumFormats = count;
	
	m_pFormatEtc  = new FORMATETC[m_nMaxNumFormats];
	m_pStgMedium  = new STGMEDIUM[m_nMaxNumFormats];

	for(int i = 0; i < count; i++)
	{
		m_pFormatEtc[i] = fmtetc[i];
		m_pStgMedium[i] = stgmed[i];
	}
}

//
//	Destructor
//
CDataObject::~CDataObject()
{
	WARNING("Освобождать данные в m_pStgMedium.hGlobal, и т.п.?");

	// cleanup
	if(m_pFormatEtc) delete[] m_pFormatEtc;
	if(m_pStgMedium) delete[] m_pStgMedium;

	DEBUGSTR(L"oof\n");
}

//
//	IUnknown::AddRef
//
ULONG __stdcall CDataObject::AddRef(void)
{
    // increment object reference count
    return InterlockedIncrement(&m_lRefCount);
}

//
//	IUnknown::Release
//
ULONG __stdcall CDataObject::Release(void)
{
    // decrement object reference count
	LONG count = InterlockedDecrement(&m_lRefCount);
		
	if(count == 0)
	{
		delete this;
		return 0;
	}
	else
	{
		return count;
	}
}

//
//	IUnknown::QueryInterface
//
HRESULT __stdcall CDataObject::QueryInterface(REFIID iid, void **ppvObject)
{
    // check to see what interface has been requested
    if(iid == IID_IDataObject || iid == IID_IUnknown)
    {
        AddRef();
        *ppvObject = this;
        return S_OK;
    }
    else
    {
        *ppvObject = 0;
        return E_NOINTERFACE;
    }
}

HGLOBAL DupMem(HGLOBAL hMem)
{
	// lock the source memory object
	SIZE_T  len    = GlobalSize(hMem);
	PVOID   source = GlobalLock(hMem);
	
	// create a fixed "global" block - i.e. just
	// a regular lump of our process heap
	PVOID   dest   = GlobalAlloc(GMEM_FIXED, len);

	memcpy(dest, source, len);

	GlobalUnlock(hMem);

	return dest;
}

int CDataObject::LookupFormatEtc(FORMATETC *pFormatEtc)
{
	for(int i = 0; i < m_nNumFormats; i++)
	{
		if((pFormatEtc->tymed    &  m_pFormatEtc[i].tymed)   &&
			pFormatEtc->cfFormat == m_pFormatEtc[i].cfFormat && 
			pFormatEtc->dwAspect == m_pFormatEtc[i].dwAspect)
		{
			return i;
		}
	}

	return -1;

}

//
//	IDataObject::GetData
//
HRESULT __stdcall CDataObject::GetData (FORMATETC *pFormatEtc, STGMEDIUM *pMedium)
{
	int idx;

	//
	// try to match the requested FORMATETC with one of our supported formats
	//
	if((idx = LookupFormatEtc(pFormatEtc)) == -1)
	{
		return DV_E_FORMATETC;
	}

	//
	// found a match! transfer the data into the supplied storage-medium
	//
	pMedium->tymed			 = m_pFormatEtc[idx].tymed;
	pMedium->pUnkForRelease  = 0;
	//pMedium->lpszFileName = 
	
	switch(m_pFormatEtc[idx].tymed)
	{
	case TYMED_HGLOBAL:
		{
			pMedium->hGlobal = DupMem(m_pStgMedium[idx].hGlobal);
		//return S_OK;
			break;
		}

	default:
		return DV_E_FORMATETC;
	}

	return S_OK;
}

//
//	IDataObject::GetDataHere
//
HRESULT __stdcall CDataObject::GetDataHere (FORMATETC *pFormatEtc, STGMEDIUM *pMedium)
{
	// GetDataHere is only required for IStream and IStorage mediums
	// It is an error to call GetDataHere for things like HGLOBAL and other clipboard formats
	//
	//	OleFlushClipboard 
	//
	return DATA_E_FORMATETC;
}

//
//	IDataObject::QueryGetData
//
//	Called to see if the IDataObject supports the specified format of data
//
HRESULT __stdcall CDataObject::QueryGetData (FORMATETC *pFormatEtc)
{
	return (LookupFormatEtc(pFormatEtc) == -1) ? DV_E_FORMATETC : S_OK;
}

//
//	IDataObject::GetCanonicalFormatEtc
//
HRESULT __stdcall CDataObject::GetCanonicalFormatEtc (FORMATETC *pFormatEct, FORMATETC *pFormatEtcOut)
{
	// Apparently we have to set this field to NULL even though we don't do anything else
	pFormatEtcOut->ptd = NULL;
	return E_NOTIMPL;
}

//
//	IDataObject::SetData
//
HRESULT __stdcall CDataObject::SetData (FORMATETC *pFormatEtc, STGMEDIUM *pMedium,  BOOL fRelease)
{
	_ASSERTE(fRelease);

	// Если нужно - увеличим размерность массивов
	if (m_nNumFormats >= m_nMaxNumFormats) {
		_ASSERTE(m_nNumFormats == m_nMaxNumFormats);
		LONG nNewMaxNumFormats = m_nNumFormats + 32;
		FORMATETC *pNewFormatEtc = new FORMATETC[nNewMaxNumFormats];
		STGMEDIUM *pNewStgMedium = new STGMEDIUM[nNewMaxNumFormats];
		if (!pNewFormatEtc || !pNewStgMedium) {
			if(pNewFormatEtc) delete[] pNewFormatEtc;
			if(pNewStgMedium) delete[] pNewStgMedium;
			return E_OUTOFMEMORY;
		}
		for (LONG i = 0; i < m_nNumFormats; i++) {
			pNewFormatEtc[i] = m_pFormatEtc[i];
			pNewStgMedium[i] = m_pStgMedium[i];
		}

		m_nMaxNumFormats = nNewMaxNumFormats;
		if(m_pFormatEtc) delete[] m_pFormatEtc;
		m_pFormatEtc = pNewFormatEtc;
		if(m_pStgMedium) delete[] m_pStgMedium;
		m_pStgMedium = pNewStgMedium;
	}

	m_pFormatEtc[m_nNumFormats] = *pFormatEtc;
	m_pStgMedium[m_nNumFormats] = *pMedium;
	m_nNumFormats++;

	return S_OK;
}

//
//	IDataObject::EnumFormatEtc
//
HRESULT __stdcall CDataObject::EnumFormatEtc (DWORD dwDirection, IEnumFORMATETC **ppEnumFormatEtc)
{
	if(dwDirection == DATADIR_GET)
	{
		// for Win2k+ you can use the SHCreateStdEnumFmtEtc API call, however
		// to support all Windows platforms we need to implement IEnumFormatEtc ourselves.
		return CreateEnumFormatEtc(m_nNumFormats, m_pFormatEtc, ppEnumFormatEtc);
	}
	else
	{
		// the direction specified is not support for drag+drop
		return E_NOTIMPL;
	}
}

//
//	IDataObject::DAdvise
//
HRESULT __stdcall CDataObject::DAdvise (FORMATETC *pFormatEtc, DWORD advf, IAdviseSink *pAdvSink, DWORD *pdwConnection)
{
	return OLE_E_ADVISENOTSUPPORTED;
}

//
//	IDataObject::DUnadvise
//
HRESULT __stdcall CDataObject::DUnadvise (DWORD dwConnection)
{
	return OLE_E_ADVISENOTSUPPORTED;
}

//
//	IDataObject::EnumDAdvise
//
HRESULT __stdcall CDataObject::EnumDAdvise (IEnumSTATDATA **ppEnumAdvise)
{
	return OLE_E_ADVISENOTSUPPORTED;
}

//
//	Helper function
//
HRESULT CreateDataObject (FORMATETC *fmtetc, STGMEDIUM *stgmeds, UINT count, IDataObject **ppDataObject)
{
	if(ppDataObject == 0)
		return E_INVALIDARG;

	*ppDataObject = new CDataObject(fmtetc, stgmeds, count);

	return (*ppDataObject) ? S_OK : E_OUTOFMEMORY;
}








////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////












//
//	"Drop-in" replacement for SHCreateStdEnumFmtEtc. Called by CDataObject::EnumFormatEtc
//
HRESULT CreateEnumFormatEtc(UINT nNumFormats, FORMATETC *pFormatEtc, IEnumFORMATETC **ppEnumFormatEtc)
{
	if(nNumFormats == 0 || pFormatEtc == 0 || ppEnumFormatEtc == 0)
		return E_INVALIDARG;

	*ppEnumFormatEtc = new CEnumFormatEtc(pFormatEtc, nNumFormats);

	return (*ppEnumFormatEtc) ? S_OK : E_OUTOFMEMORY;
}

//
//	Helper function to perform a "deep" copy of a FORMATETC
//
static void DeepCopyFormatEtc(FORMATETC *dest, FORMATETC *source)
{
	// copy the source FORMATETC into dest
	*dest = *source;
	
	if(source->ptd)
	{
		// allocate memory for the DVTARGETDEVICE if necessary
		dest->ptd = (DVTARGETDEVICE*)CoTaskMemAlloc(sizeof(DVTARGETDEVICE));

		// copy the contents of the source DVTARGETDEVICE into dest->ptd
		*(dest->ptd) = *(source->ptd);
	}
}

//
//	Constructor 
//
CEnumFormatEtc::CEnumFormatEtc(FORMATETC *pFormatEtc, int nNumFormats)
{
	m_lRefCount   = 1;
	m_nIndex      = 0;
	m_nNumFormats = nNumFormats;
	m_pFormatEtc  = new FORMATETC[nNumFormats];
	
	// copy the FORMATETC structures
	for(int i = 0; i < nNumFormats; i++)
	{	
		DeepCopyFormatEtc(&m_pFormatEtc[i], &pFormatEtc[i]);
	}
}

//
//	Destructor
//
CEnumFormatEtc::~CEnumFormatEtc()
{
	if(m_pFormatEtc)
	{
		for(ULONG i = 0; i < m_nNumFormats; i++)
		{
			if(m_pFormatEtc[i].ptd)
				CoTaskMemFree(m_pFormatEtc[i].ptd);
		}

		delete[] m_pFormatEtc;
	}
}

//
//	IUnknown::AddRef
//
ULONG __stdcall CEnumFormatEtc::AddRef(void)
{
    // increment object reference count
    return InterlockedIncrement(&m_lRefCount);
}

//
//	IUnknown::Release
//
ULONG __stdcall CEnumFormatEtc::Release(void)
{
    // decrement object reference count
	LONG count = InterlockedDecrement(&m_lRefCount);
		
	if(count == 0)
	{
		delete this;
		return 0;
	}
	else
	{
		return count;
	}
}

//
//	IUnknown::QueryInterface
//
HRESULT __stdcall CEnumFormatEtc::QueryInterface(REFIID iid, void **ppvObject)
{
    // check to see what interface has been requested
    if(iid == IID_IEnumFORMATETC || iid == IID_IUnknown)
    {
        AddRef();
        *ppvObject = this;
        return S_OK;
    }
    else
    {
        *ppvObject = 0;
        return E_NOINTERFACE;
    }
}

//
//	IEnumFORMATETC::Next
//
//	If the returned FORMATETC structure contains a non-null "ptd" member, then
//  the caller must free this using CoTaskMemFree (stated in the COM documentation)
//
HRESULT __stdcall CEnumFormatEtc::Next(ULONG celt, FORMATETC *pFormatEtc, ULONG * pceltFetched)
{
	ULONG copied  = 0;

	// validate arguments
	if(celt == 0 || pFormatEtc == 0)
		return E_INVALIDARG;

	// copy FORMATETC structures into caller's buffer
	while(m_nIndex < m_nNumFormats && copied < celt)
	{
		DeepCopyFormatEtc(&pFormatEtc[copied], &m_pFormatEtc[m_nIndex]);
		copied++;
		m_nIndex++;
	}

	// store result
	if(pceltFetched != 0) 
		*pceltFetched = copied;

	// did we copy all that was requested?
	return (copied == celt) ? S_OK : S_FALSE;
}

//
//	IEnumFORMATETC::Skip
//
HRESULT __stdcall CEnumFormatEtc::Skip(ULONG celt)
{
	m_nIndex += celt;
	return (m_nIndex <= m_nNumFormats) ? S_OK : S_FALSE;
}

//
//	IEnumFORMATETC::Reset
//
HRESULT __stdcall CEnumFormatEtc::Reset(void)
{
	m_nIndex = 0;
	return S_OK;
}

//
//	IEnumFORMATETC::Clone
//
HRESULT __stdcall CEnumFormatEtc::Clone(IEnumFORMATETC ** ppEnumFormatEtc)
{
	HRESULT hResult;

	// make a duplicate enumerator
	hResult = CreateEnumFormatEtc(m_nNumFormats, m_pFormatEtc, ppEnumFormatEtc);

	if(hResult == S_OK)
	{
		// manually set the index state
		((CEnumFormatEtc *) *ppEnumFormatEtc)->m_nIndex = m_nIndex;
	}

	return hResult;
}









HANDLE StringToHandle(LPCWSTR szText, int nTextLen)
{
    void  *ptr;

    // if text length is -1 then treat as a nul-terminated string
    if(nTextLen == -1)
        nTextLen = lstrlen(szText) + 1;
    
    // allocate and lock a global memory buffer. Make it fixed
    // data so we don't have to use GlobalLock
    ptr = (void *)GlobalAlloc(GMEM_FIXED, nTextLen);

    // copy the string into the buffer
    memcpy(ptr, szText, nTextLen);

    return ptr;
}
