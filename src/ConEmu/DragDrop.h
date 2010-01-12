#pragma once
#include "..\common\common.hpp"
#include "BaseDragDrops.h"
#include "virtualconsole.h"

#define MAX_DROP_PATH 0x800

#include <pshpack1.h>
typedef struct tag_DragImageBits {
	DWORD nWidth, nHeight; // XP max 301x301
	DWORD nXCursor, nYCursor; // Позиция курсора захвата, относительно драгнутой картинки
	DWORD nRes1; // тут какой-то мусор - заняты все байты DWORD'а
	DWORD nRes2; // всегда 0xffffffff
	RGBQUAD pix[1];
} DragImageBits;
#include <poppack.h>

class CDragDrop :public CBaseDropTarget
{
public:
	CDragDrop();
	BOOL Init();
	~CDragDrop();
	virtual HRESULT __stdcall Drop (IDataObject * pDataObject,DWORD grfKeyState,POINTL pt,DWORD * pdwEffect);
	virtual HRESULT __stdcall DragOver(DWORD grfKeyState,POINTL pt,DWORD * pdwEffect);
	virtual HRESULT __stdcall DragEnter(IDataObject * pDataObject,DWORD grfKeyState,POINTL pt,DWORD * pdwEffect);
	virtual HRESULT __stdcall DragLeave(void);
	void Drag(BOOL abClickNeed, COORD crMouseDC);
	IDataObject *mp_DataObject;
	bool mb_selfdrag;
	ForwardedPanelInfo *m_pfpi;
	HRESULT CreateLink(LPCTSTR lpszPathObj, LPCTSTR lpszPathLink, LPCTSTR lpszDesc);
	BOOL InDragDrop();
	virtual void DragFeedBack(DWORD dwEffect);
	BOOL IsDragStarting() {return FALSE;};
	BOOL ForwardMessage(HWND hWnd, UINT messg, WPARAM wParam, LPARAM lParam) { return FALSE;};
protected:
	BOOL mb_DragDropRegistered;
	void RetrieveDragToInfo(IDataObject * pDataObject);
	LPITEMIDLIST mp_DesktopID;
	DWORD mn_AllFiles, mn_CurFile; __int64 mn_CurWritten;
	HANDLE FileStart(BOOL abActive, BOOL abWide, LPVOID asFileName);
	HRESULT FileWrite(HANDLE ahFile, DWORD anSize, LPVOID apData);
	void EnumDragFormats(IDataObject * pDataObject);
	HRESULT DropFromStream(IDataObject * pDataObject, BOOL abActive);
	HRESULT DropLinks(HDROP hDrop, int iQuantity, BOOL abActive);
	HRESULT DropNames(HDROP hDrop, int iQuantity, BOOL abActive);
	typedef struct _ThInfo {
		HANDLE hThread;
		DWORD  dwThreadId;
	} ThInfo;
	std::vector<ThInfo> m_OpThread;
	CRITICAL_SECTION m_CrThreads;
	typedef struct _ShlOpInfo {
		CDragDrop* pDnD;
		SHFILEOPSTRUCT fop;
	} ShlOpInfo;
	static DWORD WINAPI ShellOpThreadProc(LPVOID lpParameter);
	//DragImageBits m_BitsInfo;
	HWND mh_Overlapped;
	HDC mh_BitsDC;
	HBITMAP mh_BitsBMP, mh_BitsBMP_Old;
	//int m_iBPP;
	static LRESULT CALLBACK DragBitsWndProc(HWND hWnd, UINT messg, WPARAM wParam, LPARAM lParam);
	BOOL CreateDragImageWindow();
	void DestroyDragImageWindow();
	BOOL LoadDragImageBits(IDataObject * pDataObject);
	BOOL CreateDragImageBits(IDataObject * pDataObject);
	DragImageBits* CreateDragImageBits(wchar_t* pszFiles);
	BOOL DrawImageBits ( HDC hDrawDC, wchar_t* pszFile, int *nMaxX, int nX, int *nMaxY );
	void DestroyDragImageBits();
	void MoveDragWindow(BOOL abVisible=TRUE);
	//DragImageBits m_ImgInfo;
	//LPBYTE mp_ImgData;
	DragImageBits *mp_Bits;
	BOOL mb_DragWithinNow;
	//
	static DWORD WINAPI ExtractIconsThread(LPVOID lpParameter);
	DWORD mn_ExtractIconsTID;
	HANDLE mh_ExtractIcons;
	//
	typedef struct _DragThreadArg {
		CDragDrop   *pThis;
		IDataObject *pDataObject;
		IDropSource *pDropSource;
		DWORD        dwAllowedEffects;
	} DragThreadArg;
	static DWORD WINAPI DragOpThreadProc(LPVOID lpParameter);
	HANDLE mh_DragThread; DWORD mn_DragThreadId;
};
