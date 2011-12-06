
/*
Copyright (c) 2009-2011 Maximus5
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

#include "../common/common.hpp"

// Forwards
struct TabName;
//struct TabID;
//struct TabInfo;
class CVirtualConsole;
class MSection;
class MSectionLock;
class CTabStack;


enum TabType
{
	// Эти три флага - не битовая маска, а тип панели, получающийся из (nFlags & 0xFF)
	etfPanels      = 1,
	etfViewer      = 2,
	etfEditor      = 3,
};

enum TabInfoFlags
{
	// Эти три флага - не битовая маска, а тип панели, получающийся из (nFlags & 0xFF)
	//etfPanels    = 1, --> enum TabType
	//etfViewer    = 2, --> enum TabType
	//etfEditor    = 3, --> enum TabType
	// Далее идут флаги, состояния консоли
	etfAdmin       = 0x0100, // вкладка запущена с повышенными полномочиями
	etfUser        = 0x0200, // вкладка запущена под другим пользователем
	etfRestricted  = 0x0400, // вкладка запущена под ограниченной учетной записью
	etfNonRespond  = 0x0800, // вкладка не отвечает, или только запускается
	// Флаги отображения вкладки
	etfActive      = 0x1000, // активная вкладка. она может быть в НЕ активной консоли
	etfDisabled    = 0x2000, // вкладка недоступна (заблокирована модальным диалогом, cmd.exe, и пр.)
	etfFlash       = 0x4000, // мигающая вкладка (вкладка ожидает действия пользователя)
};

enum TabIdState
{
	tisEmpty   = 0, // TabID еще не инициализирован
	tisValid   = 1, // Таб открыт, процесс активен
	tisPassive = 2, // Таб еще есть, но процесс (FAR) ушел в фон
	tisInvalid = 3, // Процесс прибит. При следующем обновлении табов объект будет освобожден
};

/* Simple string class */
struct TabName
{
private:
	int nMaxLen, nLen;
	wchar_t sz[CONEMUTABMAX];
public:
	LPCWSTR Init(LPCWSTR asName);
	LPCWSTR Set(LPCWSTR asName);
	void Release();
	LPCWSTR Upper();
	LPCWSTR Ptr() const;
	int Length() const;
};

/* Internal information for Tab drawing */
struct TabDrawInfo
{
	TabName  Display; // это уже отформатированный текст, который нужно отобразить
	//DWORD    nFlags;  // enum TabInfoFlags
	RECT     rcTab;   // Координаты относительно WindowDC
	HRGN     rgnTab;  // точный регион таба, для реакции на мышку
	bool     Clipped; // текст был обрезан при отрисовке, показывать тултип
};

struct TabInfo
{
	enum TabIdState Status;
	int  Type; // TabType { etfPanels/etfEditor/etfViewer }
	UINT Flags; // enum of TabInfoFlags
	
	CVirtualConsole* pVCon;
	
	//TabName Name;

	int nPID; // ИД процесса, содержащего таб (актуально для редакторов/вьюверов)
	int nFarWindowID; // ИД (а точнее просто 0-based index) окна в FAR. Panels==0, ViewerEditor>=1
	int nViewEditID;  // а это нам нужно потому, что во вьюверах может быть открыто несколько копий одного файла
};


/* Uniqualizer for Each tab */
class CTabID
{
protected:
	int mn_RefCount;
	~CTabID();
public:
	TabInfo Info;
	UINT Flags();
	//enum TabIdState Status;
	//int  Type; // TabType { etfPanels/etfEditor/etfViewer }
	//UINT Flags; // enum of TabInfoFlags
	//CVirtualConsole* pVCon;
	
	TabName Name;
	TabName Upper; // для облегчения сравнения
	
	//int nPID; // ИД процесса, содержащего таб (актуально для редакторов/вьюверов)
	//int nFarWindowID; // ИД (а точнее просто 0-based index) окна в FAR. Panels==0, ViewerEditor>=1
	//int nViewEditID;  // а это нам нужно потому, что во вьюверах может быть открыто несколько копий одного файла

	// Для внутреннего использования
	TabDrawInfo DrawInfo;

	CTabID(CVirtualConsole* apVCon, LPCWSTR asName, int anType, int anPID, int anFarWindowID, int anViewEditID, UINT anFlags);
	void Set(LPCWSTR asName, int anType, int anPID, int anFarWindowID, int anViewEditID, UINT anFlags);

	int AddRef();
	int Release();
	bool IsEqual(const CTabID* pTabId, bool abIgnoreWindowId = false);
	bool IsEqual(CVirtualConsole* apVCon, const TabName& asNameUpper, int anType, int anPID, int anViewEditID);
	
	void ReleaseDrawRegion();
};

// Класс для автоматического AddRef/Release
class CTab
{
protected:
	CTabID* mp_Tab;
	friend class CTabStack;
public:
	CTab() { mp_Tab = NULL; }
	~CTab() { if (mp_Tab) mp_Tab->Release(); }
	
	void Init(CTabID* apTab) { if (mp_Tab) mp_Tab->Release(); if (apTab) apTab->AddRef(); mp_Tab = apTab; }
	void Init(CTab& Tab) { Init(Tab.mp_Tab); }
	
	CTabID* operator -> () { return mp_Tab; }
	
	const CTabID* Tab() { return mp_Tab; }
};

class CTabStack
{
protected:
	CTabID** mpp_Stack;
	int mn_MaxCount, mn_Used;
protected:
	MSection* mp_Section;
	//MSectionLock* mp_UpdateLock;
	int mn_UpdatePos;
	void AppendInt(CTabID* pTab, BOOL abMoveFirst, MSectionLock* pSC);
	void RequestSize(int anCount, MSectionLock* pSC);
public:
	CTabStack();
	~CTabStack();
	
	//const CTabID* CreateOrFind(CVirtualConsole* apVCon, LPCWSTR asName, int anType, int anPID, int anFarWindowID, int anViewEditID, UINT anFlags);
	
	int GetCount();
	bool GetTabInfoByIndex(int anIndex, /*OUT*/ TabInfo& rInfo);
	bool GetTabByIndex(int anIndex, /*OUT*/ CTab& rTab);
	int GetIndexByTab(const CTabID* pTab);
	bool GetNextTab(const CTabID* pTab, BOOL abForward, /*OUT*/ CTab& rTab);
	bool GetTabDrawRect(int anIndex, RECT* rcTab);
	bool SetTabDrawRect(int anIndex, const RECT& rcTab);

	void LockTabs(MSectionLock* pLock);
	
	HANDLE UpdateBegin();
	void UpdateFarWindow(HANDLE hUpdate, CVirtualConsole* apVCon, LPCWSTR asName, int anType, int anPID, int anFarWindowID, int anViewEditID, UINT anFlags);
	void UpdateAppend(HANDLE hUpdate, CTab& Tab, BOOL abMoveFirst);
	void UpdateAppend(HANDLE hUpdate, CTabID* pTab, BOOL abMoveFirst);
	void UpdateEnd(HANDLE hUpdate, BOOL abForceReleaseTail);

	void ReleaseTabs(BOOL abInvalidOnly = TRUE);
};
