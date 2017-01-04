
/*
Copyright (c) 2009-2017 Maximus5
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

#pragma once
//#include "../common/Common.h"
#include "BaseDragDrops.h"
#include "DragDropData.h"
//#include "virtualconsole.h"
#include "../common/MArray.h"

struct MSectionSimple;


class CDragDrop :
	public CBaseDropTarget,
	public CDragDropData
{
	public:
		CDragDrop();
		//BOOL Init(); --> CDragDropData::Register
		virtual ~CDragDrop();
		virtual HRESULT __stdcall Drop(IDataObject * pDataObject, DWORD grfKeyState, POINTL pt, DWORD * pdwEffect) override;
		virtual HRESULT __stdcall DragOver(DWORD grfKeyState, POINTL pt, DWORD * pdwEffect) override;
		virtual HRESULT __stdcall DragEnter(IDataObject * pDataObject, DWORD grfKeyState, POINTL pt, DWORD * pdwEffect) override;
		virtual HRESULT __stdcall DragLeave(void) override;
		void Drag(BOOL abClickNeed, COORD crMouseDC);
		void DebugLog(LPCWSTR asInfo, BOOL abErrorSeverity=FALSE);
		//IDataObject *mp_DataObject;
		//bool mb_selfdrag;
		//ForwardedPanelInfo *m_pfpi;
		HRESULT CreateLink(LPCTSTR lpszPathObj, LPCTSTR lpszPathLink, LPCTSTR lpszDesc);
		//BOOL InDragDrop();
		//virtual void DragFeedBack(DWORD dwEffect) override;
		//BOOL IsDragStarting() {return FALSE;};
		//BOOL ForwardMessage(HWND hWnd, UINT messg, WPARAM wParam, LPARAM lParam) { return FALSE;};
	protected:
		DWORD DragStart(IDropSource *pDropSource, const DWORD dwAllowedEffects, DWORD& dwEffect);
		HRESULT DragOverInt(DWORD grfKeyState, POINTL pt, DWORD * pdwEffect);
		//BOOL mb_DragDropRegistered;
		//void RetrieveDragToInfo();
		//ITEMIDLIST m_DesktopID;
		DWORD mn_AllFiles, mn_CurFile; __int64 mn_CurWritten;
		HANDLE FileStart(LPCWSTR pszFullName);
		wchar_t* FileCreateName(BOOL abActive, BOOL abFolder, LPCWSTR asSubFolder, LPCWSTR asFileName);
		HRESULT FileWrite(HANDLE ahFile, DWORD anSize, LPVOID apData);
		//void EnumDragFormats(IDataObject * pDataObject);
		void ReportUnknownData(IDataObject * pDataObject, LPCWSTR sUnknownError);
		HRESULT DropFromStream(IDataObject * pDataObject, BOOL abActive);
		HRESULT DropLinks(HDROP hDrop, int iQuantity, BOOL abActive);
		HRESULT DropNames(HDROP hDrop, int iQuantity, BOOL abActive);
		HRESULT DropFromText(IDataObject * pDataObject);
		HRESULT DropShellOp(IDataObject * pDataObject, DWORD * pdwEffect, STGMEDIUM& stgMedium, BOOL abActive, int iQuantity);
		typedef struct _ThInfo
		{
			HANDLE hThread;
			DWORD  dwThreadId;
		} ThInfo;
		MArray<ThInfo> m_OpThread;
		MSectionSimple* mp_CrThreads;
		typedef struct _ShlOpInfo
		{
			CDragDrop* pDnD;
			SHFILEOPSTRUCT fop;
		} ShlOpInfo;
		static DWORD WINAPI ShellOpThreadProc(LPVOID lpParameter);
};
