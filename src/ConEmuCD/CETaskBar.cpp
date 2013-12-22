
#include <windows.h>
#include "CETaskBar.h"

const CLSID CLSID_TaskbarList = {0x56FDF344, 0xFD6D, 0x11d0, {0x95, 0x8A, 0x00, 0x60, 0x97, 0xC9, 0xA0, 0x90}};
const IID IID_ITaskbarList  = {0x56FDF342, 0xFD6D, 0x11D0, {0x95, 0x8A, 0x00, 0x60, 0x97, 0xC9, 0xA0, 0x90}};
const IID IID_ITaskbarList2 = {0x602D4995, 0xB13A, 0x429B, {0xA6, 0x6E, 0x19, 0x35, 0xE4, 0x4F, 0x43, 0x17}};
const IID IID_ITaskbarList3 = {0xEA1AFB91, 0x9E28, 0x4B86, {0x90, 0xE9, 0x9E, 0x9F, 0x8A, 0x5E, 0xEF, 0xAF}};
const IID IID_ITaskbarList4 = {0xC43DC798, 0x95D1, 0x4BEA, {0x90, 0x30, 0xBB, 0x99, 0xE2, 0x98, 0x3A, 0x1A}};


CETaskbarList4::CETaskbarList4()
{
}

HRESULT CETaskbarList4::InitInterfaces(REFIID required)
{
}

CETaskbarList4::~CETaskbarList4()
{
}


// ITaskbarList4 : public ITaskbarList3
virtual HRESULT STDMETHODCALLTYPE SetTabProperties( 
    /* [in] */ HWND hwndTab,
    /* [in] */ STPFLAG stpFlags)
{
	HRESULT hr = E_NOINTERFACE;
	if (mp_ITaskbarList)
	{
		hr = mp_ITaskbarList->XXX(YYY);
	}
	return hr;
}
// ITaskbarList3 : public ITaskbarList2
HRESULT CETaskbarList4::SetProgressValue( 
    /* [in] */ HWND hwnd,
    /* [in] */ ULONGLONG ullCompleted,
    /* [in] */ ULONGLONG ullTotal)
{
	HRESULT hr = E_NOINTERFACE;
	if (mp_ITaskbarList)
	{
		hr = mp_ITaskbarList->XXX(YYY);
	}
	return hr;
}

HRESULT CETaskbarList4::SetProgressState( 
    /* [in] */ HWND hwnd,
    /* [in] */ TBPFLAG tbpFlags)
{
	HRESULT hr = E_NOINTERFACE;
	if (mp_ITaskbarList)
	{
		hr = mp_ITaskbarList->XXX(YYY);
	}
	return hr;
}

HRESULT CETaskbarList4::RegisterTab( 
    /* [in] */ HWND hwndTab,
    /* [in] */ HWND hwndMDI)
{
	HRESULT hr = E_NOINTERFACE;
	if (mp_ITaskbarList)
	{
		hr = mp_ITaskbarList->XXX(YYY);
	}
	return hr;
}

HRESULT CETaskbarList4::UnregisterTab( 
    /* [in] */ HWND hwndTab)
{
	HRESULT hr = E_NOINTERFACE;
	if (mp_ITaskbarList)
	{
		hr = mp_ITaskbarList->XXX(YYY);
	}
	return hr;
}

HRESULT CETaskbarList4::SetTabOrder( 
    /* [in] */ HWND hwndTab,
    /* [in] */ HWND hwndInsertBefore)
{
	HRESULT hr = E_NOINTERFACE;
	if (mp_ITaskbarList)
	{
		hr = mp_ITaskbarList->XXX(YYY);
	}
	return hr;
}

HRESULT CETaskbarList4::SetTabActive( 
    /* [in] */ HWND hwndTab,
    /* [in] */ HWND hwndMDI,
    /* [in] */ DWORD dwReserved)
{
	HRESULT hr = E_NOINTERFACE;
	if (mp_ITaskbarList)
	{
		hr = mp_ITaskbarList->XXX(YYY);
	}
	return hr;
}

HRESULT CETaskbarList4::ThumbBarAddButtons( 
    /* [in] */ HWND hwnd,
    /* [in] */ UINT cButtons,
    /* [size_is][in] */ __RPC__in_ecount_full(cButtons) LPTHUMBBUTTON pButton)
{
	HRESULT hr = E_NOINTERFACE;
	if (mp_ITaskbarList)
	{
		hr = mp_ITaskbarList->XXX(YYY);
	}
	return hr;
}

HRESULT CETaskbarList4::ThumbBarUpdateButtons( 
    /* [in] */ HWND hwnd,
    /* [in] */ UINT cButtons,
    /* [size_is][in] */ __RPC__in_ecount_full(cButtons) LPTHUMBBUTTON pButton)
{
	HRESULT hr = E_NOINTERFACE;
	if (mp_ITaskbarList)
	{
		hr = mp_ITaskbarList->XXX(YYY);
	}
	return hr;
}

HRESULT CETaskbarList4::ThumbBarSetImageList( 
    /* [in] */ HWND hwnd,
    /* [in] */ __RPC__in_opt HIMAGELIST himl)
{
	HRESULT hr = E_NOINTERFACE;
	if (mp_ITaskbarList)
	{
		hr = mp_ITaskbarList->XXX(YYY);
	}
	return hr;
}

HRESULT CETaskbarList4::SetOverlayIcon( 
    /* [in] */ HWND hwnd,
    /* [in] */ HICON hIcon,
    /* [string][unique][in] */ __RPC__in_opt_string LPCWSTR pszDescription)
{
	HRESULT hr = E_NOINTERFACE;
	if (mp_ITaskbarList)
	{
		hr = mp_ITaskbarList->XXX(YYY);
	}
	return hr;
}

HRESULT CETaskbarList4::SetThumbnailTooltip( 
    /* [in] */ HWND hwnd,
    /* [string][unique][in] */ __RPC__in_opt_string LPCWSTR pszTip)
{
	HRESULT hr = E_NOINTERFACE;
	if (mp_ITaskbarList)
	{
		hr = mp_ITaskbarList->XXX(YYY);
	}
	return hr;
}

HRESULT CETaskbarList4::SetThumbnailClip( 
    /* [in] */ HWND hwnd,
    /* [in] */ RECT *prcClip)
{
	HRESULT hr = E_NOINTERFACE;
	if (mp_ITaskbarList)
	{
		hr = mp_ITaskbarList->XXX(YYY);
	}
	return hr;
}
// ITaskbarList2 : public ITaskbarList
HRESULT CETaskbarList4::MarkFullscreenWindow( 
    /* [in] */ HWND hwnd,
    /* [in] */ BOOL fFullscreen)
{
	HRESULT hr = E_NOINTERFACE;
	if (mp_ITaskbarList)
	{
		hr = mp_ITaskbarList->XXX(YYY);
	}
	return hr;
}
// ITaskbarList : public IUnknown
HRESULT CETaskbarList4::HrInit( void)
{
	HRESULT hr = E_NOINTERFACE;
	if (mp_ITaskbarList)
	{
		hr = mp_ITaskbarList->XXX(YYY);
	}
	return hr;
}

HRESULT CETaskbarList4::AddTab( 
    /* [in] */ HWND hwnd)
{
	HRESULT hr = E_NOINTERFACE;
	if (mp_ITaskbarList)
	{
		hr = mp_ITaskbarList->XXX(YYY);
	}
	return hr;
}

HRESULT CETaskbarList4::DeleteTab( 
    /* [in] */ HWND hwnd)
{
	HRESULT hr = E_NOINTERFACE;
	if (mp_ITaskbarList)
	{
		hr = mp_ITaskbarList->XXX(YYY);
	}
	return hr;
}

HRESULT CETaskbarList4::ActivateTab( 
    /* [in] */ HWND hwnd)
{
	HRESULT hr = E_NOINTERFACE;
	if (mp_ITaskbarList)
	{
		hr = mp_ITaskbarList->XXX(YYY);
	}
	return hr;
}

HRESULT CETaskbarList4::SetActiveAlt( 
    /* [in] */ HWND hwnd)
{
	HRESULT hr = E_NOINTERFACE;
	if (mp_ITaskbarList)
	{
		hr = mp_ITaskbarList->XXX(YYY);
	}
	return hr;
}
// IUnknown
HRESULT CETaskbarList4::QueryInterface( 
    /* [in] */ REFIID riid,
    /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject)
{
	HRESULT hr = E_NOINTERFACE;
	if (mp_ITaskbarList)
	{
		hr = mp_ITaskbarList->XXX(YYY);
	}
	return hr;
}

ULONG CETaskbarList4::AddRef( void)
{
	HRESULT hr = E_NOINTERFACE;
	if (mp_ITaskbarList)
	{
		hr = mp_ITaskbarList->XXX(YYY);
	}
	return hr;
}

ULONG CETaskbarList4::Release( void)
{
	HRESULT hr = E_NOINTERFACE;
	if (mp_ITaskbarList)
	{
		hr = mp_ITaskbarList->XXX(YYY);
	}
	return hr;
}
