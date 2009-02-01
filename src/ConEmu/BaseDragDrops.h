#pragma once
#include <windows.h>

HRESULT CreateDropSource(IDropSource **ppDropSource);
HRESULT CreateDataObject (FORMATETC *fmtetc, STGMEDIUM *stgmeds, UINT count, IDataObject **ppDataObject);
HRESULT CreateEnumFormatEtc(UINT nNumFormats, FORMATETC *pFormatEtc, IEnumFORMATETC **ppEnumFormatEtc);
HANDLE StringToHandle(char *szText, int nTextLen);


class CBaseDropTarget : public IDropTarget
{
public:
	CBaseDropTarget(HWND hwnd);
	CBaseDropTarget();
	~CBaseDropTarget();
	// IUnknown implementation
	HRESULT __stdcall QueryInterface (REFIID iid, void ** ppvObject);
	ULONG	__stdcall AddRef (void);
	ULONG	__stdcall Release (void);

	// IDropTarget implementation
	virtual HRESULT __stdcall DragEnter(IDataObject * pDataObject,DWORD grfKeyState,POINTL pt,DWORD * pdwEffect);
	virtual HRESULT __stdcall DragOver(DWORD grfKeyState,POINTL pt,DWORD * pdwEffect);
	virtual HRESULT __stdcall DragLeave(void);
	virtual HRESULT __stdcall Drop (IDataObject * pDataObject,DWORD grfKeyState,POINTL pt,DWORD * pdwEffect);

	LONG	m_lRefCount;
	HWND	m_hWnd;
};


class CDropSource : public IDropSource
{
public:
	//
    // IUnknown members
	//
    HRESULT __stdcall QueryInterface	(REFIID iid, void ** ppvObject);
    ULONG   __stdcall AddRef			(void);
    ULONG   __stdcall Release			(void);
		
    //
	// IDropSource members
	//
    HRESULT __stdcall QueryContinueDrag	(BOOL fEscapePressed, DWORD grfKeyState);
	HRESULT __stdcall GiveFeedback		(DWORD dwEffect);
	
	//
    // Constructor / Destructor
	//
    CDropSource();
    ~CDropSource();
	
private:

    //
	// private members and functions
	//
    LONG	   m_lRefCount;
};




class CDataObject : public IDataObject
{
public:
	//
    // IUnknown members
	//
    HRESULT __stdcall QueryInterface (REFIID iid, void ** ppvObject);
    ULONG   __stdcall AddRef (void);
    ULONG   __stdcall Release (void);
		
    //
	// IDataObject members
	//
    HRESULT __stdcall GetData				(FORMATETC *pFormatEtc,  STGMEDIUM *pMedium);
    HRESULT __stdcall GetDataHere			(FORMATETC *pFormatEtc,  STGMEDIUM *pMedium);
    HRESULT __stdcall QueryGetData			(FORMATETC *pFormatEtc);
	HRESULT __stdcall GetCanonicalFormatEtc (FORMATETC *pFormatEct,  FORMATETC *pFormatEtcOut);
    HRESULT __stdcall SetData				(FORMATETC *pFormatEtc,  STGMEDIUM *pMedium,  BOOL fRelease);
	HRESULT __stdcall EnumFormatEtc			(DWORD      dwDirection, IEnumFORMATETC **ppEnumFormatEtc);
	HRESULT __stdcall DAdvise				(FORMATETC *pFormatEtc,  DWORD advf, IAdviseSink *pAdvSink, DWORD *pdwConnection);
	HRESULT __stdcall DUnadvise				(DWORD      dwConnection);
	HRESULT __stdcall EnumDAdvise			(IEnumSTATDATA **ppEnumAdvise);
	
	//
    // Constructor / Destructor
	//
    CDataObject(FORMATETC *fmt, STGMEDIUM *stgmed, int count);
    ~CDataObject();
	
private:

	int LookupFormatEtc(FORMATETC *pFormatEtc);

    //
	// any private members and functions
	//
    LONG	   m_lRefCount;

	FORMATETC *m_pFormatEtc;
	STGMEDIUM *m_pStgMedium;
	LONG	   m_nNumFormats;

};




class CEnumFormatEtc : public IEnumFORMATETC
{
public:

	//
	// IUnknown members
	//
	HRESULT __stdcall  QueryInterface (REFIID iid, void ** ppvObject);
	ULONG	__stdcall  AddRef (void);
	ULONG	__stdcall  Release (void);

	//
	// IEnumFormatEtc members
	//
	HRESULT __stdcall  Next  (ULONG celt, FORMATETC * rgelt, ULONG * pceltFetched);
	HRESULT __stdcall  Skip  (ULONG celt); 
	HRESULT __stdcall  Reset (void);
	HRESULT __stdcall  Clone (IEnumFORMATETC ** ppEnumFormatEtc);

	//
	// Construction / Destruction
	//
	CEnumFormatEtc(FORMATETC *pFormatEtc, int nNumFormats);
	~CEnumFormatEtc();

private:

	LONG		m_lRefCount;		// Reference count for this COM interface
	ULONG		m_nIndex;			// current enumerator index
	ULONG		m_nNumFormats;		// number of FORMATETC members
	FORMATETC * m_pFormatEtc;		// array of FORMATETC objects
};