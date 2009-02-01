#pragma once
#include "..\common\common.hpp"
#include "BaseDragDrops.h"
#include "virtualconsole.h"

#define MAX_DROP_PATH 0x800

class CDragDrop :public CBaseDropTarget
{
public:
	CDragDrop::CDragDrop(HWND hwnd);
	CDragDrop::~CDragDrop();
	virtual HRESULT __stdcall Drop (IDataObject * pDataObject,DWORD grfKeyState,POINTL pt,DWORD * pdwEffect);
	virtual HRESULT __stdcall DragOver(DWORD grfKeyState,POINTL pt,DWORD * pdwEffect);
	virtual HRESULT __stdcall DragEnter(IDataObject * pDataObject,DWORD grfKeyState,POINTL pt,DWORD * pdwEffect);
	virtual HRESULT __stdcall DragLeave(void);
	void Drag();
	IDataObject *pDataObject;
	bool selfdrag;
	ForwardedPanelInfo *pfpi;
};
