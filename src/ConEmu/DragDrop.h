#pragma once
#include "..\common\common.hpp"
#include "BaseDragDrops.h"
#include "virtualconsole.h"

#define MAX_DROP_PATH 0x800

class CDragDrop :public CBaseDropTarget
{
public:
	CDragDrop(HWND hwnd);
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
protected:
	void RetrieveDragToInfo(IDataObject * pDataObject);
	LPITEMIDLIST mp_DesktopID;
	DWORD mn_AllFiles, mn_CurFile; __int64 mn_CurWritten;
	HANDLE FileStart(BOOL abActive, BOOL abWide, LPVOID asFileName);
	HRESULT FileWrite(HANDLE ahFile, DWORD anSize, LPVOID apData);
};
