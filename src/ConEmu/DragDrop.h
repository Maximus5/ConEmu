#pragma once
#include "..\common\common.hpp"
#include "BaseDragDrops.h"
#include "virtualconsole.h"

#define MAX_DROP_PATH 0x800

class CDragDrop :public CBaseDropTarget
{
public:
	CDragDrop(HWND hwnd);
	BOOL Init();
	~CDragDrop();
	virtual HRESULT __stdcall Drop (IDataObject * pDataObject,DWORD grfKeyState,POINTL pt,DWORD * pdwEffect);
	virtual HRESULT __stdcall DragOver(DWORD grfKeyState,POINTL pt,DWORD * pdwEffect);
	virtual HRESULT __stdcall DragEnter(IDataObject * pDataObject,DWORD grfKeyState,POINTL pt,DWORD * pdwEffect);
	virtual HRESULT __stdcall DragLeave(void);
	void Drag();
	IDataObject *mp_DataObject;
	bool mb_selfdrag;
	ForwardedPanelInfo *m_pfpi;
	HRESULT CreateLink(LPCTSTR lpszPathObj, LPCTSTR lpszPathLink, LPCTSTR lpszDesc);
	BOOL InDragDrop();
protected:
	BOOL mb_DragDropRegistered;
	void RetrieveDragToInfo(IDataObject * pDataObject);
	LPITEMIDLIST mp_DesktopID;
	DWORD mn_AllFiles, mn_CurFile; __int64 mn_CurWritten;
	HANDLE FileStart(BOOL abActive, BOOL abWide, LPVOID asFileName);
	HRESULT FileWrite(HANDLE ahFile, DWORD anSize, LPVOID apData);
	#ifdef MSGLOGGER
	void EnumDragFormats(IDataObject * pDataObject);
	#endif
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
	typedef struct tag_DragImageBits {
		DWORD nWidth, nHeight;
		DWORD nXCursor, nYCursor; // Позиция курсора захвата, относительно драгнутой картинки
		DWORD nRes1; // тут какой-то мусор - заняты все байты DWORD'а
		DWORD nRes2; // всегда 0xffffffff
		RGBQUAD pix[1];
	} DragImageBits;
	//DragImageBits m_BitsInfo;
	HWND mh_Overlapped;
	HDC mh_BitsDC;
	HBITMAP mh_BitsBMP;
	//int m_iBPP;
	static LRESULT CALLBACK DragBitsWndProc(HWND hWnd, UINT messg, WPARAM wParam, LPARAM lParam);
	BOOL CreateDragImageWindow();
	void DestroyDragImageWindow();
	BOOL LoadDragImageBits(IDataObject * pDataObject);
	BOOL CreateDragImageBits(IDataObject * pDataObject);
	void DestroyDragImageBits();
	void MoveDragWindow();
	//DragImageBits m_ImgInfo;
	//LPBYTE mp_ImgData;
	DragImageBits *mp_Bits;
};
