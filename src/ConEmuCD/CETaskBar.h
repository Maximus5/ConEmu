
#pragma once

#include "../ConEmu/ShObjIdl_Part.h"

// CoCreateInstance(CLSID_TaskbarList,nullptr,CLSCTX_INPROC_SERVER,IID_PPV_ARGS(&pTaskbarList));
/* Ole32.dll */ HRESULT OnCoCreateInstance(REFCLSID rclsid, LPUNKNOWN pUnkOuter, DWORD dwClsContext, REFIID riid, LPVOID *ppv);

class CETaskbarList4 : public ITaskbarList4
{
private:
	int mn_RefCount;
	ITaskbarList4* mp_ITaskbarList4;
	ITaskbarList3* mp_ITaskbarList3;
	ITaskbarList2* mp_ITaskbarList2;
	ITaskbarList*  mp_ITaskbarList;
public:
	CETaskbarList4();
	HRESULT InitInterfaces(REFIID required);
private:
	~CETaskbarList4();
//public:
//	// ITaskbarList4 : public ITaskbarList3
//	    virtual HRESULT STDMETHODCALLTYPE SetTabProperties( 
//	        /* [in] */ HWND hwndTab,
//	        /* [in] */ STPFLAG stpFlags);
//    // ITaskbarList3 : public ITaskbarList2
//        virtual HRESULT STDMETHODCALLTYPE SetProgressValue( 
//            /* [in] */ HWND hwnd,
//            /* [in] */ ULONGLONG ullCompleted,
//            /* [in] */ ULONGLONG ullTotal);
//        
//        virtual HRESULT STDMETHODCALLTYPE SetProgressState( 
//            /* [in] */ HWND hwnd,
//            /* [in] */ TBPFLAG tbpFlags);
//        
//        virtual HRESULT STDMETHODCALLTYPE RegisterTab( 
//            /* [in] */ HWND hwndTab,
//            /* [in] */ HWND hwndMDI);
//        
//        virtual HRESULT STDMETHODCALLTYPE UnregisterTab( 
//            /* [in] */ HWND hwndTab);
//        
//        virtual HRESULT STDMETHODCALLTYPE SetTabOrder( 
//            /* [in] */ HWND hwndTab,
//            /* [in] */ HWND hwndInsertBefore);
//        
//        virtual HRESULT STDMETHODCALLTYPE SetTabActive( 
//            /* [in] */ HWND hwndTab,
//            /* [in] */ HWND hwndMDI,
//            /* [in] */ DWORD dwReserved);
//        
//        virtual HRESULT STDMETHODCALLTYPE ThumbBarAddButtons( 
//            /* [in] */ HWND hwnd,
//            /* [in] */ UINT cButtons,
//            /* [size_is][in] */ __RPC__in_ecount_full(cButtons) LPTHUMBBUTTON pButton);
//        
//        virtual HRESULT STDMETHODCALLTYPE ThumbBarUpdateButtons( 
//            /* [in] */ HWND hwnd,
//            /* [in] */ UINT cButtons,
//            /* [size_is][in] */ __RPC__in_ecount_full(cButtons) LPTHUMBBUTTON pButton);
//        
//        virtual HRESULT STDMETHODCALLTYPE ThumbBarSetImageList( 
//            /* [in] */ HWND hwnd,
//            /* [in] */ __RPC__in_opt HIMAGELIST himl);
//        
//        virtual HRESULT STDMETHODCALLTYPE SetOverlayIcon( 
//            /* [in] */ HWND hwnd,
//            /* [in] */ HICON hIcon,
//            /* [string][unique][in] */ __RPC__in_opt_string LPCWSTR pszDescription);
//        
//        virtual HRESULT STDMETHODCALLTYPE SetThumbnailTooltip( 
//            /* [in] */ HWND hwnd,
//            /* [string][unique][in] */ __RPC__in_opt_string LPCWSTR pszTip);
//        
//        virtual HRESULT STDMETHODCALLTYPE SetThumbnailClip( 
//            /* [in] */ HWND hwnd,
//            /* [in] */ RECT *prcClip);
//	// ITaskbarList2 : public ITaskbarList
//        virtual HRESULT STDMETHODCALLTYPE MarkFullscreenWindow( 
//            /* [in] */ HWND hwnd,
//            /* [in] */ BOOL fFullscreen);
//	// ITaskbarList : public IUnknown
//        virtual HRESULT STDMETHODCALLTYPE HrInit( void);
//        
//        virtual HRESULT STDMETHODCALLTYPE AddTab( 
//            /* [in] */ HWND hwnd);
//        
//        virtual HRESULT STDMETHODCALLTYPE DeleteTab( 
//            /* [in] */ HWND hwnd);
//        
//        virtual HRESULT STDMETHODCALLTYPE ActivateTab( 
//            /* [in] */ HWND hwnd);
//        
//        virtual HRESULT STDMETHODCALLTYPE SetActiveAlt( 
//            /* [in] */ HWND hwnd);
//	// IUnknown
//        virtual HRESULT STDMETHODCALLTYPE QueryInterface( 
//            /* [in] */ REFIID riid,
//            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
//
//        virtual ULONG STDMETHODCALLTYPE AddRef( void);
//
//        virtual ULONG STDMETHODCALLTYPE Release( void);
};
