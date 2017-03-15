
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

#include "../common/Common.h"
#include "RefRelease.h"

#ifdef _DEBUG
#define TAB_REF_PLACE
#else
#undef TAB_REF_PLACE
#endif
#ifdef TAB_REF_PLACE
#include "../common/MArray.h"
#endif

// Forwards
struct TabName;
//struct TabID;
//struct TabInfo;
class CVirtualConsole;
class MSection;
class MSectionLock;
struct MSectionSimple;
struct MSectionLockSimple;
class CTabStack;


/* **********************************
typedef unsigned int CEFarWindowType;
static const CEFarWindowType
	fwt_Any            = 0,
	fwt_Panels         = 1,
	fwt_Viewer         = 2,
	fwt_Editor         = 3,
	fwt_TypeMask       = 0x00FF,

	fwt_Elevated       = 0x0100,
	fwt_NonElevated    = 0x0200, // Аргумент для поиска окна
	fwt_Modal          = 0x0400,
	fwt_NonModal       = 0x0800, // Аргумент для поиска окна
	fwt_PluginRequired = 0x1000, // Аргумент для поиска окна
	fwt_ActivateFound  = 0x2000, // Активировать найденный таб. Аргумент для поиска окна
	fwt_Disabled       = 0x4000, // Таб заблокирован другим модальным табом (или диалогом?)
	fwt_Renamed        = 0x8000  // Таб был принудительно переименован пользователем
	;
  ********************************** */

//enum TabType
//{
//	// Эти три флага - не битовая маска, а тип панели, получающийся из (nFlags & 0xFF)
//	etfPanels      = 1,
//	etfViewer      = 2,
//	etfEditor      = 3,
//};
//
//enum TabInfoFlags
//{
//	// Эти три флага - не битовая маска, а тип панели, получающийся из (nFlags & 0xFF)
//	//etfPanels    = 1, --> enum TabType
//	//etfViewer    = 2, --> enum TabType
//	//etfEditor    = 3, --> enum TabType
//	// Далее идут флаги, состояния консоли
//	etfAdmin       = 0x0100, // вкладка запущена с повышенными полномочиями
//	etfUser        = 0x0200, // вкладка запущена под другим пользователем
//	etfRestricted  = 0x0400, // вкладка запущена под ограниченной учетной записью
//	etfNonRespond  = 0x0800, // вкладка не отвечает, или только запускается
//	// Флаги отображения вкладки
//	etfActive      = 0x1000, // активная вкладка. она может быть в НЕ активной консоли
//	etfDisabled    = 0x2000, // вкладка недоступна (заблокирована модальным диалогом, cmd.exe, и пр.)
//	etfFlash       = 0x4000, // мигающая вкладка (вкладка ожидает действия пользователя)
//};


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
	int nLen;
	wchar_t sz[CONEMUTABMAX]; // it's 0x400 now
public:
	TabName();
	TabName(LPCWSTR asName);
public:
	LPCWSTR Set(LPCWSTR asName, bool escape = false);
	void Release();
	LPCWSTR Ptr() const;
	int Length() const;
	bool Empty() const;
};

/* Internal information for Tab drawing */
struct TabDrawInfo
{
public:
	TabName  Display; // это уже отформатированный текст, который нужно отобразить
	//DWORD    nFlags;  // enum TabInfoFlags
	RECT     rcTab;   // Координаты относительно WindowDC
	HRGN     rgnTab;  // точный регион таба, для реакции на мышку
	bool     Clipped; // текст был обрезан при отрисовке, показывать тултип
public:
	TabDrawInfo() {memset(&rcTab, 0, sizeof(rcTab)); rgnTab = NULL; Clipped=false;};
};

struct TabInfo
{
	enum TabIdState Status;
	CEFarWindowType Type; // enum of CEFarWindowType

	// Use "void" because that is not "guarded" pointer!
	void/*CVirtualConsole*/* pVCon;

	int nPID; // ИД процесса, содержащего таб (актуально для редакторов/вьюверов)
	int nFarWindowID; // ИД окна фара (0-based). Far 4040 - new "Desktop" window type has "0" index
	int nViewEditID;  // а это нам нужно потому, что во вьюверах может быть открыто несколько копий одного файла
	int nIndex;       // Far 3 вообще не отдает индекс окна, а nFarWindowID перемешивается при каждом чихе
};

#ifdef TAB_REF_PLACE
struct TabRefPlace
{
	char filename[32];
	int  fileline;
	// Usage: SetPlace(__FILE__,__LINE__)
	void SetPlace(LPCSTR asFile, int anLine)
	{
		LPCSTR pszSlash = asFile ? strrchr(asFile, '\\') : NULL;
		lstrcpynA(filename, pszSlash ? (pszSlash+1) : asFile ? asFile : "<NULL>", countof(filename));
		fileline = anLine;
	}
};
#endif

/* Uniqualizer for Each tab */
class CTabID : public CRefRelease
{
protected:
	virtual ~CTabID();
	virtual void FinalRelease();
public:
	CTabID(CVirtualConsole* apVCon, LPCWSTR asName, CEFarWindowType anType, int anPID, int anFarWindowID, int anViewEditID);
public:
	TabInfo Info;
	CEFarWindowType Type()  { return (Info.Type &  fwt_TypeMask); };
	CEFarWindowType Flags() { return (Info.Type & ~fwt_TypeMask); };

	// Имя и переименованное юзером
	TabName Name, Renamed;
	// С учетом возможного переименования
	LPCWSTR GetName();
	bool SetName(LPCWSTR asName);

	// Для внутреннего использования
	TabDrawInfo DrawInfo;
	LPCWSTR GetLabel();
	void SetLabel(LPCWSTR asLabel);

	// Methods
	bool Set(LPCWSTR asName, CEFarWindowType anType, int anPID, int anFarWindowID, int anViewEditID, CEFarWindowType anFlagMask = fwt_Any);

	#if 0
	bool IsEqual(const CTabID* pTabId, bool abIgnoreWindowId, CEFarWindowType FlagMask);
	#endif
	bool IsEqual(CVirtualConsole* apVCon, const TabName& asName, CEFarWindowType anType, int anPID, int anViewEditID, CEFarWindowType FlagMask);
	bool IsEqual(CVirtualConsole* apVCon, LPCWSTR asName, CEFarWindowType anType, int anPID, int anViewEditID, CEFarWindowType FlagMask);

	void ReleaseDrawRegion();

	#ifdef TAB_REF_PLACE
	protected:
	MArray<TabRefPlace> m_Places;
	public:
	void AddPlace(LPCSTR asName, int anLine);
	void DelPlace(LPCSTR asName, int anLine);
	void DelPlace(const TabRefPlace& drp);
	#endif
};

// Класс для автоматического AddRef/Release
class CTab
{
protected:
	CTabID* mp_Tab;
	friend class CTabStack;
protected:
	#ifdef TAB_REF_PLACE
	TabRefPlace m_RefPlace;
	#endif
public:
	CTab(LPCSTR asFile, int anLine);
	~CTab();

	void Init(CTabID* apTab);
	void Init(CTab& Tab);
	CTabID* AddRef(LPCSTR asFile, int anLine);

public:
	CTabID* operator -> () { return mp_Tab; }
	const CTabID* Tab() { return mp_Tab; }
};

class CTabStack
{
protected:
	CTabID** mpp_Stack;
	int mn_MaxCount, mn_Used;
protected:
	MSectionSimple* mpc_Section;
	int  mn_UpdatePos;
	bool mb_FarUpdateMode;
	int  AppendInt(CTabID* pTab, BOOL abMoveFirst, MSectionLockSimple* pSC);
	void RequestSize(int anCount, MSectionLockSimple* pSC);
	void CleanNulls();
	int  GetVisualToRealIndex(int anVisual);
	#if 0
	void TabDeleted(MSectionLockSimple* pUpdateLock, int i);
	void RecheckPassive();
	#endif
protected:
	#ifdef TAB_REF_PLACE
	TabRefPlace m_rp;
	public:
	void SetPlace(LPCSTR asPlace, int anLine) { m_rp.SetPlace(asPlace,anLine); };
	#endif
public:
	CTabStack();
	~CTabStack();

	int  GetCount();
	bool GetTabInfoByIndex(int anIndex, /*OUT*/ TabInfo& rInfo);
	bool GetTabByIndex(int anIndex, /*OUT*/ CTab& rTab);
	int  GetIndexByTab(const CTabID* pTab);
	bool GetNextTab(const CTabID* pTab, BOOL abForward, /*OUT*/ CTab& rTab);
	bool GetTabDrawRect(int anIndex, RECT* rcTab);
	bool SetTabDrawRect(int anIndex, const RECT& rcTab);

	void LockTabs(MSectionLockSimple* pLock);

	HANDLE UpdateBegin();
	bool UpdateFarWindow(HANDLE hUpdate, CVirtualConsole* apVCon, LPCWSTR asName, CEFarWindowType anType, int anPID, int anFarWindowID, int anViewEditID, CTab& rActiveTab);
	void UpdateAppend(HANDLE hUpdate, CTab& Tab, BOOL abMoveFirst);
	void UpdateAppend(HANDLE hUpdate, CTabID* pTab, BOOL abMoveFirst);
	bool UpdateEnd(HANDLE hUpdate, DWORD anActiveFarPID);

	void ReleaseTabs(BOOL abInvalidOnly = TRUE);

public:
	enum MatchTabEnum {
		MatchAll,
		MatchNonPanel,
	};
	void MarkTabsInvalid(MatchTabEnum MatchTab, DWORD nFarPID);
	bool RefreshFarStatus(DWORD nFarPID, CTab& rActiveTab, int& rnActiveIndex, int& rnActiveCount, bool& rbHasModalTab);
};
