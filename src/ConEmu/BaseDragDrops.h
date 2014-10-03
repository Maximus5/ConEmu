
#pragma once
#include <windows.h>

#include "../common/MArray.h"

#define DROPEFFECT_STOP_INTERNAL ((DWORD)-1)

class CDragDropData;

class CBaseDropTarget : public IDropTarget
{
	public:
		//CBaseDropTarget(/*HWND hwnd*/);
		CBaseDropTarget();
		virtual ~CBaseDropTarget();
		// IUnknown implementation
		HRESULT __stdcall QueryInterface(REFIID iid, void ** ppvObject);
		ULONG	__stdcall AddRef(void);
		ULONG	__stdcall Release(void);

		// IDropTarget implementation
		virtual HRESULT __stdcall DragEnter(IDataObject * pDataObject, DWORD grfKeyState, POINTL pt, DWORD * pdwEffect) override;
		virtual HRESULT __stdcall DragOver(DWORD grfKeyState, POINTL pt, DWORD * pdwEffect) override;
		virtual HRESULT __stdcall DragLeave(void) override;
		virtual HRESULT __stdcall Drop(IDataObject * pDataObject, DWORD grfKeyState, POINTL pt, DWORD * pdwEffect) override;

		LONG	m_lRefCount;
		//HWND	m_hWnd;

	public:
		virtual void DragFeedBack(DWORD dwEffect) {};
};


class CDropSource : public IDropSource
{
	public:
		//
		// IUnknown members
		//
		HRESULT __stdcall QueryInterface(REFIID iid, void ** ppvObject);
		ULONG   __stdcall AddRef(void);
		ULONG   __stdcall Release(void);

		//
		// IDropSource members
		//
		HRESULT __stdcall QueryContinueDrag(BOOL fEscapePressed, DWORD grfKeyState);
		HRESULT __stdcall GiveFeedback(DWORD dwEffect);

		//
		// Constructor / Destructor
		//
		CDropSource(CDragDropData* pCallback);
		virtual ~CDropSource();

	private:

		HCURSOR mh_CurCopy, mh_CurMove, mh_CurLink;
		CDragDropData* mp_Callback;

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
		HRESULT __stdcall QueryInterface(REFIID iid, void ** ppvObject);
		ULONG   __stdcall AddRef(void);
		ULONG   __stdcall Release(void);

		//
		// IDataObject members
		//
		HRESULT __stdcall GetData(FORMATETC *pFormatEtc,  STGMEDIUM *pMedium);
		HRESULT __stdcall GetDataHere(FORMATETC *pFormatEtc,  STGMEDIUM *pMedium);
		HRESULT __stdcall QueryGetData(FORMATETC *pFormatEtc);
		HRESULT __stdcall GetCanonicalFormatEtc(FORMATETC *pFormatEct,  FORMATETC *pFormatEtcOut);
		HRESULT __stdcall SetData(FORMATETC *pFormatEtc,  STGMEDIUM *pMedium,  BOOL fRelease);
		HRESULT __stdcall EnumFormatEtc(DWORD      dwDirection, IEnumFORMATETC **ppEnumFormatEtc);
		HRESULT __stdcall DAdvise(FORMATETC *pFormatEtc,  DWORD advf, IAdviseSink *pAdvSink, DWORD *pdwConnection);
		HRESULT __stdcall DUnadvise(DWORD      dwConnection);
		HRESULT __stdcall EnumDAdvise(IEnumSTATDATA **ppEnumAdvise);

		//
		// Constructor / Destructor
		//
		CDataObject(FORMATETC *fmt, STGMEDIUM *stgmed, int count);
		virtual ~CDataObject();

		// Helper methods
		HRESULT SetDataInt(LPCWSTR sFmtName, const void* hData, DWORD nDataSize = 0);

	private:

		int LookupFormatEtc(FORMATETC *pFormatEtc);
		LPCWSTR GetFormatName(CLIPFORMAT cfFormat, bool bRaw = false);

		//
		// any private members and functions
		//
		LONG	   m_lRefCount;

		// Data storage
		struct DragData
		{
			STGMEDIUM StgMedium; // 12 bytes (x86) or 20 bytes (x64)
			BOOL fRelease;
			FORMATETC FormatEtc; // 20 bytes (x86)
			BOOL fUsed;
		};
		MArray<DragData> m_Data;
};




class CEnumFormatEtc : public IEnumFORMATETC
{
	public:

		//
		// IUnknown members
		//
		HRESULT __stdcall  QueryInterface(REFIID iid, void ** ppvObject);
		ULONG	__stdcall  AddRef(void);
		ULONG	__stdcall  Release(void);

		//
		// IEnumFormatEtc members
		//
		HRESULT __stdcall  Next(ULONG celt, FORMATETC * rgelt, ULONG * pceltFetched);
		HRESULT __stdcall  Skip(ULONG celt);
		HRESULT __stdcall  Reset(void);
		HRESULT __stdcall  Clone(IEnumFORMATETC ** ppEnumFormatEtc);

		//
		// Construction / Destruction
		//
		CEnumFormatEtc(FORMATETC *pFormatEtc, int nNumFormats);
		virtual ~CEnumFormatEtc();

	private:

		LONG		m_lRefCount;		// Reference count for this COM interface
		ULONG		m_nIndex;			// current enumerator index
		ULONG		m_nNumFormats;		// number of FORMATETC members
		FORMATETC * m_pFormatEtc;		// array of FORMATETC objects
};


HRESULT CreateDropSource(IDropSource **ppDropSource, CDragDropData* pCallback);
HRESULT CreateDataObject(FORMATETC *fmtetc, STGMEDIUM *stgmeds, UINT count, CDataObject **ppDataObject);
HRESULT CreateEnumFormatEtc(UINT nNumFormats, FORMATETC *pFormatEtc, IEnumFORMATETC **ppEnumFormatEtc);
//HANDLE StringToHandle(char *szText, int nTextLen);
