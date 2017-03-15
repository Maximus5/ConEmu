
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

#define HIDE_USE_EXCEPTION_INFO

#define SHOWDEBUGSTR

#define DEBUGSTRDEL(s) DEBUGSTR(s)

// Standard Windows' Tab control treats '&' as ‘underscore’ mark unfortunately
#define USE_ESCAPE_AMPERSAND

#include "Header.h"
#include "TabID.h"
#include "../common/MSection.h"
#include "../common/MSectionSimple.h"
#include "ConEmu.h"
#include "VConGroup.h"

#ifdef _DEBUG
#define DEBUG_TAB_LIST
#else
#undef DEBUG_TAB_LIST
#endif
#ifdef DEBUG_TAB_LIST
#include "../common/MMap.h"
MMap<CTabID*,bool> gTabIdList;
static bool bTabIdListInit = false;
#endif


/* Simple fixed-max-length string class { struct } */
TabName::TabName()
{
	nLen = 0; sz[0] = 0;
}
TabName::TabName(LPCWSTR asName)
{
	nLen = 0; sz[0] = 0;
	Set(asName);
}
LPCWSTR TabName::Set(LPCWSTR asName, bool escape /*= false*/)
{
	#ifdef _DEBUG
	// For breakpoints only...
	nLen = asName ? lstrlenW(asName) : -1;
	if ((nLen <= 0) || (nLen > 0 && asName[nLen-1] == L' '))
	{
		nLen = nLen;
	}
	if (asName && lstrcmp(asName, sz) != 0)
	{
		if (sz[0] && lstrcmp(asName, L"ConEmu") == 0)
			nLen = nLen;
	}
	#endif

	#if !defined(USE_ESCAPE_AMPERSAND)
	lstrcpynW(sz, asName ? asName : L"", countof(sz));
	#else
	wchar_t* pszDst = sz;
	if (asName && *asName)
	{
		wchar_t* pszEnd = sz + countof(sz);
		for (const wchar_t* pszSrc = asName; *pszSrc && (pszDst < pszEnd);)
		{
			if (escape && *pszSrc == L'&')
			{
				if ((pszDst + 2) >= pszEnd)
					break;
				*(pszDst++) = L'&';
				*(pszDst++) = L'&';
				++pszSrc;
			}
			else
			{
				*(pszDst++) = *(pszSrc++);
			}
		}
	}
	*pszDst = 0;
	#endif

	nLen = lstrlenW(sz);
	return sz;
}
void TabName::Release()
{
	sz[0] = 0;
	nLen = 0;
}
LPCWSTR TabName::Ptr() const
{
	// Must not return NULL. Never.
	return sz;
}
int TabName::Length() const
{
	return nLen;
}
bool TabName::Empty() const
{
	return (sz[0] == 0);
}





/* Uniqualizer for Each tab */
CTabID::CTabID(CVirtualConsole* apVCon, LPCWSTR asName, CEFarWindowType anType, int anPID, int anFarWindowID, int anViewEditID)
{
	memset(&Info, 0, sizeof(Info));
	memset(&DrawInfo, 0, sizeof(DrawInfo));

	Info.pVCon = apVCon;
	Set(asName, anType, anPID, anFarWindowID, anViewEditID);

	#ifdef DEBUG_TAB_LIST
	if (!bTabIdListInit)
	{
		gTabIdList.Init(256,true);
		bTabIdListInit = true;
	}
	gTabIdList.Set(this, true);
	#endif
}

LPCWSTR CTabID::GetName()
{
	// Empty strings must be substed in CRealConsole::GetTabTitle only!
	LPCWSTR pszName = ((Info.Type & fwt_Renamed) && (Renamed.Length() > 0)) ? Renamed.Ptr() : Name.Ptr();
	// Must not return NULL
	_ASSERTE(pszName != NULL);
	return pszName;
}

bool CTabID::SetName(LPCWSTR asName)
{
	int nCmpLen = asName ? lstrlen(asName) : 0;
	int nNameLen = Name.Length();
	if (nCmpLen == nNameLen)
	{
		LPCWSTR psz = Name.Ptr();
		if (asName && (wmemcmp(psz, asName, nNameLen) == 0))
		{
			// No changes
			return false;
		}
	}

	Name.Set(asName);
	return true;
}

LPCWSTR CTabID::GetLabel()
{
	LPCWSTR pszLabel = DrawInfo.Display.Ptr();
	if (!pszLabel || !*pszLabel)
	{
		_ASSERTE(FALSE && "Display.Ptr() must be initialized already!");
		pszLabel = GetName();
	}
	return pszLabel;
}

void CTabID::SetLabel(LPCWSTR asLabel)
{
	DrawInfo.Display.Set(asLabel, true);
}

bool CTabID::Set(LPCWSTR asName, CEFarWindowType anType, int anPID, int anFarWindowID, int anViewEditID, CEFarWindowType anFlagMask /*= fwt_Any*/)
{
	bool bChanged = false;

	if (Info.Status != tisValid)
	{
		Info.Status = tisValid;
		bChanged = true;
	}

	// enum CEFarWindowType (fwt_...)
	if (anFlagMask == fwt_Any)
	{
		if (Info.Type != anType)
		{
			Info.Type = anType;
			bChanged = true;
		}
	}
	else if ((anType & anFlagMask) != (Info.Type & anFlagMask))
	{
		_ASSERTE((anType & fwt_TypeMask) == (Info.Type & fwt_TypeMask));
		CEFarWindowType newFlags = (Info.Type & ~anFlagMask);
		newFlags |= (anType & anFlagMask);
		Info.Type = newFlags;
		bChanged = true;
	}

	// ИД процесса, содержащего таб (актуально для редакторов/вьюверов)
	DWORD nSetPID = ((anType & fwt_TypeMask) == fwt_Panels) ? 0 : anPID;
	if (Info.nPID != nSetPID)
	{
		Info.nPID = nSetPID;
		bChanged = true;
	}

	if (Info.nFarWindowID != anFarWindowID)
	{
		Info.nFarWindowID = anFarWindowID;
		bChanged = true;
	}

	if (Info.nViewEditID != anViewEditID)
	{
		Info.nViewEditID = anViewEditID;
		bChanged = true;
	}

	// Name
	if (SetName(asName))
		bChanged = true;

	return bChanged;
}

CTabID::~CTabID()
{
	#ifdef _DEBUG
	wchar_t szDbg[120];
	_wsprintf(szDbg, SKIPLEN(countof(szDbg)) L"~CTabID(PID=%u IDX=%u TYP=%u '",
		Info.nPID, Info.nFarWindowID, (UINT)Type());
	int nDbgLen = lstrlen(szDbg);
	lstrcpyn(szDbg+nDbgLen, GetName(), countof(szDbg)-nDbgLen-5);
	wcscat_c(szDbg, L"')\n");
	DEBUGSTRDEL(szDbg);
	#endif

	Name.Release();
	Renamed.Release();
	ReleaseDrawRegion();

	#ifdef DEBUG_TAB_LIST
	gTabIdList.Del(this);
	#endif
}

void CTabID::FinalRelease()
{
	CTabID* p = this;
	delete p;
}

void CTabID::ReleaseDrawRegion()
{
	if (DrawInfo.rgnTab)
	{
		DeleteObject(DrawInfo.rgnTab);
		DrawInfo.rgnTab = NULL;
	}
}

bool CTabID::IsEqual(CVirtualConsole* apVCon, LPCWSTR asName, CEFarWindowType anType, int anPID, int anViewEditID, CEFarWindowType FlagMask)
{
	if (!this)
	{
		_ASSERTE(FALSE && "Invalid pointer");
		return false; // Invalid arguments
	}

	// Невалидный таб (процесс был завершен)
	if (Info.Status == tisEmpty || Info.Status == tisInvalid)
		return false;
	if (this->Info.Status == tisEmpty || this->Info.Status == tisInvalid)
		return false;

	if (apVCon != this->Info.pVCon)
		return false;

	// Do not do excess TabName creation for ‘Panels’
	if ((FlagMask == fwt_TypeMask)
			&& ((anType & fwt_TypeMask) == fwt_Panels)
			&& ((this->Info.Type & fwt_TypeMask) == fwt_Panels))
		return true;

	if ((anType & FlagMask) != (this->Info.Type & FlagMask))
		return false;

	// If anPID specified, that must be exact Far instance
	if (anPID && (this->Info.nPID != anPID))
		return false;

	WARNING("Will be better to check ViewerEditorID, if available");

	int nCmpLen = asName ? lstrlen(asName) : 0;
	int nNameLen = this->Name.Length();
	if (nCmpLen != nNameLen)
		return false;

	LPCWSTR psz = this->Name.Ptr();
	_ASSERTE(psz!=NULL);
	if (asName && wmemcmp(psz, asName, nNameLen))
		return false;

	// No difference found, tabs are equal
	return true;
}

// For debug purposes, monitor owners of created CTabID's
#ifdef TAB_REF_PLACE
void CTabID::AddPlace(LPCSTR asName, int anLine)
{
	TabRefPlace rp; rp.SetPlace(asName, anLine);
	m_Places.push_back(rp);
}

void CTabID::DelPlace(LPCSTR asName, int anLine)
{
	for (INT_PTR i = 0; i < m_Places.size(); i++)
	{
		const TabRefPlace& rp = m_Places[i];
		if ((rp.fileline == anLine) && (lstrcmpA(rp.filename, asName) == 0))
		{
			m_Places.erase(i);
			break;
		}
	}
}

void CTabID::DelPlace(const TabRefPlace& drp)
{
	DelPlace(drp.filename, drp.fileline);
}
#endif





CTab::CTab(LPCSTR asFile, int anLine)
{
	mp_Tab = NULL;
	#ifdef TAB_REF_PLACE
	m_RefPlace.SetPlace(asFile, anLine);
	#endif
}
CTab::~CTab()
{
	if (mp_Tab)
	{
		#ifdef TAB_REF_PLACE
		mp_Tab->DelPlace(m_RefPlace);
		#endif
		mp_Tab->Release();
	}
}
void CTab::Init(CTabID* apTab)
{
	if (mp_Tab == apTab)
		return;

	if (mp_Tab)
	{
		#ifdef TAB_REF_PLACE
		mp_Tab->DelPlace(m_RefPlace);
		#endif
		mp_Tab->Release();
	}
	if (apTab)
	{
		apTab->AddRef();
		#ifdef TAB_REF_PLACE
		apTab->AddPlace(m_RefPlace.filename, m_RefPlace.fileline);
		#endif
	}
	mp_Tab = apTab;
}
void CTab::Init(CTab& Tab)
{
	Init(Tab.mp_Tab);
}
CTabID* CTab::AddRef(LPCSTR asFile, int anLine)
{
	mp_Tab->AddRef();
	#ifdef TAB_REF_PLACE
	mp_Tab->AddPlace(asFile, anLine);
	#endif
	return mp_Tab;
}






CTabStack::CTabStack()
{
	mn_Used = 0;
	// Thought, most of users will get only one tab per console
	mn_MaxCount = 1;
	mpp_Stack = (CTabID**)calloc(mn_MaxCount,sizeof(CTabID*));
	mpc_Section = new MSectionSimple(true);
	mn_UpdatePos = -1;
	mb_FarUpdateMode = false;
	#ifdef TAB_REF_PLACE
	m_rp.SetPlace(__FILE__,__LINE__);
	#endif
}

CTabStack::~CTabStack()
{
	if (mpp_Stack)
	{
		ReleaseTabs(FALSE);
		free(mpp_Stack);
		mpp_Stack = NULL;
	}
	SafeDelete(mpc_Section);
}

void CTabStack::RequestSize(int anCount, MSectionLockSimple* pSC)
{
	if (!mpp_Stack || (anCount > mn_MaxCount))
	{
		int nNewMaxCount = anCount
		#ifndef _DEBUG
			+15
		#endif
			;

		CTabID** ppNew = (CTabID**)calloc(nNewMaxCount, sizeof(CTabID*));
		if (mpp_Stack)
		{
			if (mn_Used > 0)
				memmove(ppNew, mpp_Stack, sizeof(CTabID*)*mn_Used);
			free(mpp_Stack);
		}

		mpp_Stack = ppNew;
		mn_MaxCount = nNewMaxCount;
	}
}

int CTabStack::AppendInt(CTabID* pTab, BOOL abMoveFirst, MSectionLockSimple* pSC)
{
	// Сразу накрутить счетчик таба
	pTab->AddRef();
	#ifdef TAB_REF_PLACE
	pTab->AddPlace(m_rp.filename, m_rp.fileline);
	#endif

	// Если требуется модификация списка
	#ifdef _DEBUG
	if (!mpp_Stack || (mn_Used == mn_MaxCount) || abMoveFirst)
	{
		#if 1
		_ASSERTE(pSC->isLocked());
		#else
		pSC->RelockExclusive();
		#endif
	}
	#endif

	if (!mpp_Stack || (mn_Used == mn_MaxCount))
	{
		RequestSize(mn_Used+1, pSC);
	}

	if (abMoveFirst && mn_Used > 0)
	{
		memmove(mpp_Stack+1, mpp_Stack, mn_Used*sizeof(CTabID*));
		mpp_Stack[0] = NULL;
	}

	int i = abMoveFirst ? 0 : mn_Used;
	mpp_Stack[i] = pTab;
	mn_Used++;

	return i;
}

int CTabStack::GetCount()
{
	return mn_Used;
}

// mpc_Section must be locked before call!
int CTabStack::GetVisualToRealIndex(int anVisual)
{
	int nPos = 0;

	for (int i = 0; i < mn_Used; i++)
	{
		if (mpp_Stack[i] && (mpp_Stack[i]->Info.Status == tisValid))
		{
			if (nPos == anVisual)
				return i;
			nPos++;
		}
	}

	return -1;
}

bool CTabStack::GetTabInfoByIndex(int anIndex, /*OUT*/ TabInfo& rInfo)
{
	bool lbFound = false;
	MSectionLockSimple SC; SC.Lock(mpc_Section);

	int iReal = GetVisualToRealIndex(anIndex);
	if (iReal >= 0 && iReal < mn_Used)
	{
		if (mpp_Stack[iReal])
		{
			rInfo = mpp_Stack[iReal]->Info;
			lbFound = true;
		}
	}

	SC.Unlock();
	return lbFound;
}

bool CTabStack::GetTabByIndex(int anIndex, /*OUT*/ CTab& rTab)
{
	MSectionLockSimple SC; SC.Lock(mpc_Section);

	int iReal = GetVisualToRealIndex(anIndex);
	if (iReal >= 0 && iReal < mn_Used)
	{
		rTab.Init(mpp_Stack[iReal]);
		rTab->Info.nIndex = iReal;
	}
	else
	{
		rTab.Init(NULL);
	}

	SC.Unlock();
	return (rTab.Tab() != NULL);
}

int CTabStack::GetIndexByTab(const CTabID* pTab)
{
	MSectionLockSimple SC; SC.Lock(mpc_Section);

	int nIndex = -1;
	for (int i = 0; i < mn_Used; i++)
	{
		if (!mpp_Stack[i])
			continue;

		if (mpp_Stack[i]->Info.Status == tisValid)
			nIndex++;
		else
			continue;

		if (mpp_Stack[i] == pTab)
			break;
	}

	SC.Unlock();
	return nIndex;
}

bool CTabStack::GetNextTab(const CTabID* pTab, BOOL abForward, /*OUT*/ CTab& rTab)
{
	MSectionLockSimple SC; SC.Lock(mpc_Section);
	CTabID* pNextTab = NULL;

	for (int i = 0; i < mn_Used; i++)
	{
		if (mpp_Stack[i] == pTab)
		{
			if (abForward)
			{
				if ((i + 1) < mn_Used)
					pNextTab = mpp_Stack[i+1];
			}
			else
			{
				if (i > 0)
					pNextTab = mpp_Stack[i-1];
			}
			break;
		}
	}
	rTab.Init(pNextTab);

	SC.Unlock();
	return (pNextTab!=NULL);
}

bool CTabStack::GetTabDrawRect(int anIndex, RECT* rcTab)
{
	bool lbExist = false;
	MSectionLockSimple SC; SC.Lock(mpc_Section);

	int iReal = GetVisualToRealIndex(anIndex);
	if (iReal >= 0 && iReal < mn_Used)
	{
		CTabID* pTab = mpp_Stack[iReal];
		if (pTab)
		{
			if (rcTab)
				*rcTab = pTab->DrawInfo.rcTab;
			lbExist = true;
		}
		else
		{
			_ASSERTE(pTab!=NULL);
		}
	}
	else
	{
		_ASSERTE(iReal >= 0 && iReal < mn_Used);
	}

	SC.Unlock();
	return lbExist;
}

bool CTabStack::SetTabDrawRect(int anIndex, const RECT& rcTab)
{
	bool lbExist = false;
	MSectionLockSimple SC; SC.Lock(mpc_Section);

	int iReal = GetVisualToRealIndex(anIndex);
	if (iReal >= 0 && iReal < mn_Used)
	{
		CTabID* pTab = mpp_Stack[iReal];
		if (pTab)
		{
			if (memcmp(&pTab->DrawInfo.rcTab, &rcTab, sizeof(rcTab)) != 0)
			{
				pTab->ReleaseDrawRegion();
				pTab->DrawInfo.rcTab = rcTab;
			}
			lbExist = true;
		}
		else
		{
			_ASSERTE(pTab!=NULL);
		}
	}
	else
	{
		_ASSERTE(iReal >= 0 && iReal < mn_Used);
	}

	SC.Unlock();
	return lbExist;
}

void CTabStack::LockTabs(MSectionLockSimple* pLock)
{
	pLock->Lock(mpc_Section);
}

// Должен вызываться перед UpdateOrCreate и UpdateEnd
HANDLE CTabStack::UpdateBegin()
{
	MSectionLockSimple* pUpdateLock = new MSectionLockSimple;
	LockTabs(pUpdateLock);
	mn_UpdatePos = 0;
	return (HANDLE)pUpdateLock;

	//for (int i = 0; i < mn_Used; i++)
	//{
	//	if (mpp_Stack[i])
	//		mpp_Stack[i]->bExisted = false;
	//}
}

// Должен вызываться после UpdateBegin и перед UpdateEnd
//void CTabStack::ProcessMark(int anExistPID)
//{
//	for (int i = 0; i < mn_Used; i++)
//	{
//		if (mpp_Stack[i])
//		{
//			if (mpp_Stack[i]->Info.nPID == anExistPID)
//				mpp_Stack[i]->bExisted = true;
//		}
//	}
//}

// Должен вызываться только из CRealConsole!
// Возвращает "true" если были изменения
bool CTabStack::UpdateFarWindow(HANDLE hUpdate, CVirtualConsole* apVCon, LPCWSTR asName, CEFarWindowType anType, int anPID, int anFarWindowID, int anViewEditID, CTab& rActiveTab)
{
	MSectionLockSimple* pUpdateLock = (MSectionLockSimple*)hUpdate;

	// Функция должна вызваться ТОЛЬКО между UpdateBegin & UpdateEnd
	if (mn_UpdatePos < 0 || !pUpdateLock)
	{
		_ASSERTE(mn_UpdatePos>=0);
		_ASSERTE(pUpdateLock!=NULL);
		return false;
	}

	bool bChanged = false;
	CTabID* pTab = NULL;

	mb_FarUpdateMode = true;

	// Теперь - поехали обновлять. Правила такие:
	// 1. Новая вкладка в ФАР может появиться ТОЛЬКО в конце
	// 2. Закрыта может быть любая вкладка
	// 3. панели НЕ должны перебиваться на редактор при запуске "far /e"
	// 4. и вообще, первый таб мог быть добавлен изначально как редактор!

	int i = mn_UpdatePos;
	while (i < mn_Used)
	{
		if (!mpp_Stack[i])
		{
			i++;
			continue;
		}

		if (mpp_Stack[i]->IsEqual(apVCon, asName, anType, anPID, anViewEditID, fwt_TypeMask))
		{
			// OK, таб совпадает
			pTab = mpp_Stack[i];

			// Refresh status and title
			if (pTab->Set(asName, anType, anPID, anFarWindowID, anViewEditID, fwt_UpdateFlags))
				bChanged = true;

			_ASSERTE(pTab->Info.Status == tisValid);

			if (anType & fwt_CurrentFarWnd)
				rActiveTab.Init(pTab);

			mn_UpdatePos = i+1;

			// Закончили
			break;
		}

		// Tab was closed or gone to background
		bChanged = true;
		mpp_Stack[i]->Info.Status = tisPassive;
		i++;
	}

	// Если таб новый
	if (pTab == NULL)
	{
		// это новая вкладка, добавляемая в конец
		pTab = new CTabID(apVCon, asName, anType, anPID, anFarWindowID, anViewEditID);
		_ASSERTE(pTab->RefCount()==1);
		_ASSERTE(mn_Used == i);

		i = AppendInt(pTab, FALSE/*abMoveFirst*/, pUpdateLock);
		_ASSERTE(pTab->RefCount()==2);
		if (anType & fwt_CurrentFarWnd)
			rActiveTab.Init(pTab);
		pTab->Release();

		bChanged = true;
		mn_UpdatePos = i+1;
	}

	_ASSERTE(mn_UpdatePos>=1 && mn_UpdatePos<=mn_Used);

	return bChanged;
}

#if 0
void CTabStack::TabDeleted(MSectionLockSimple* pUpdateLock, int i)
{
	PRAGMA_ERROR("FAIL. Нужно убрать из стека все NULL-ячейки");
	if ((i >= 0) && (i < mn_MaxCount))
	{
		if (i < mn_Used)
		{
			#if 1
			_ASSERTE(pUpdateLock->isLocked());
			#else
			pUpdateLock->RelockExclusive();
			#endif
			// Все что между {mn_UpdatePos .. (i-1)} теперь уже забито NULL
			memmove(mpp_Stack+mn_UpdatePos, mpp_Stack+i, (mn_Used - i) * sizeof(CTabID*));
		}
		mn_Used -= (i - mn_UpdatePos);
		_ASSERTE(mn_Used>0);
		memset(mpp_Stack+mn_Used, 0, (i - mn_UpdatePos) * sizeof(CTabID*));
	}
}
#endif

void CTabStack::UpdateAppend(HANDLE hUpdate, CTab& Tab, BOOL abMoveFirst)
{
	UpdateAppend(hUpdate, Tab.mp_Tab, abMoveFirst);
}

void CTabStack::UpdateAppend(HANDLE hUpdate, CTabID* pTab, BOOL abMoveFirst)
{
	MSectionLockSimple* pUpdateLock = (MSectionLockSimple*)hUpdate;

	// Функция должна вызваться ТОЛЬКО между UpdateBegin & UpdateEnd
	if (mn_UpdatePos < 0 || !pUpdateLock)
	{
		_ASSERTE(mn_UpdatePos>=0);
		_ASSERTE(pUpdateLock!=NULL);
		return;
	}

	// Если таб в списке уже есть - то НИЧЕГО не делать (только переместить его в начало, если abActive)
	// Если таб новый - добавить в список и вызвать AddRef для таба

	if (!pTab)
	{
		_ASSERTE(pTab != NULL);
		return;
	}

	int nIndex = -1;
	for (int i = 0; i < mn_Used; i++)
	{
		if (mpp_Stack[i] == pTab)
		{
			nIndex = i;
			break;
		}
	}

	if (!abMoveFirst)
	{
		// "Обычное" население
		#ifdef _DEBUG
		if (nIndex == -1 || nIndex != mn_UpdatePos)
		{
			#if 1
			_ASSERTE(pUpdateLock->isLocked());
			#else
			pUpdateLock->RelockExclusive();
			#endif
		}
		#endif
		RequestSize(mn_UpdatePos+1, pUpdateLock);
		if (nIndex != -1 && nIndex != mn_UpdatePos)
		{
			//_ASSERTE(nIndex > mn_UpdatePos); -- may happen when creating new split from active "far /e ..." tab
			// Do NOT release tab here! Behavior can differs by arguments of UpdateEnd!
			CTabID* p = mpp_Stack[mn_UpdatePos];
			mpp_Stack[mn_UpdatePos] = mpp_Stack[nIndex];
			mpp_Stack[nIndex] = p;
			// At the moment vector must be realigned!
			_ASSERTE(mpp_Stack[mn_UpdatePos] == pTab);
		}
		if (mpp_Stack[mn_UpdatePos] != pTab)
		{
			pTab->AddRef();
			#ifdef TAB_REF_PLACE
			pTab->AddPlace(m_rp.filename, m_rp.fileline);
			#endif
		}
		mpp_Stack[mn_UpdatePos] = pTab;
		mn_UpdatePos++;
		if (mn_UpdatePos > mn_Used)
		{
			mn_Used = mn_UpdatePos;
		}
	}
	else
	{
		if (nIndex == -1)
		{
			// Таба в списке еще нет, добавляем
			AppendInt(pTab, abMoveFirst, pUpdateLock);
		}
		else if (abMoveFirst && nIndex > 0)
		{
			// Таб нужно переместить в начало списка
			#if 1
			_ASSERTE(pUpdateLock->isLocked());
			#else
			pUpdateLock->RelockExclusive();
			#endif
			memmove(mpp_Stack+1, mpp_Stack, sizeof(CTabID*) * (nIndex-1));
			mpp_Stack[0] = pTab; // AddRef не нужен, таб уже у нас в списке!
		}
		mn_UpdatePos++;
	}
}

// Должен вызываться после xxx и xxx как часть закрытия убитых процессов
// для завершенных процессов установится Info.Status == tisInvalid. фактический delete НЕ производится
//void CTabStack::UpdateEnd()
//{
//	bool lbExclusive = false;
//	_ASSERTE(mp_MarkTemp!=NULL);
//
//	for (int i = 0; i < mn_Used; i++)
//	{
//		if (mpp_Stack[i])
//		{
//			if (!mpp_Stack[i]->bExisted)
//			{
//				if (!lbExclusive)
//				{
//					mp_MarkTemp->RelockExclusive();
//					lbExclusive = true;
//				}
//
//				// Пометить таб невалидным
//				mpp_Stack[i]->Info.Status = tisInvalid;
//			}
//		}
//	}
//
//	mp_MarkTemp->Unlock();
//	delete mp_MarkTemp;
//	mp_MarkTemp = NULL;
//}

// Возвращает "true" если были изменения в КОЛИЧЕСТВЕ табов (ЗДЕСЬ больше ничего не проверяется)
bool CTabStack::UpdateEnd(HANDLE hUpdate, DWORD anActiveFarPID)
{
	MSectionLockSimple* pUpdateLock = (MSectionLockSimple*)hUpdate;

	// Функция должна вызваться ТОЛЬКО между UpdateBegin & UpdateEnd
	if (mn_UpdatePos < 0)
	{
		_ASSERTE(mn_UpdatePos>=0);
		return false;
	}

	bool bVConClosed = false;

	if (!mb_FarUpdateMode && (mn_UpdatePos == 0))
	{
		if (!CVConGroup::isVConExists(0))
		{
			bVConClosed = true;
		}
		else
		{
			// Фукнция UpdateFarWindow должна была быть вызвана хотя бы раз!
			//UpdateFarWindow(CVirtualConsole* apVCon, LPCWSTR asName, int anType, int anPID, int anFarWindowID, int anViewEditID, CEFarWindowType anFlags)
			_ASSERTE(mn_UpdatePos>0 || CVConGroup::GetConCount()==0);
			pUpdateLock->Unlock();
			delete pUpdateLock;
			mn_UpdatePos = -1;
			return false;
		}
	}

	bool bChanged = (mn_Used != mn_UpdatePos);

	#if 1
	_ASSERTE(pUpdateLock->isLocked());
	#else
	pUpdateLock->RelockExclusive();
	#endif

	if (mb_FarUpdateMode)
	{
		_ASSERTE(mn_UpdatePos > 0);

		// Все табы ПОСЛЕ последнего добавленного/обновленного - помечаем неактивными
		for (int i = mn_UpdatePos; i < mn_Used; i++)
		{
			CTabID *pTab = mpp_Stack[i];
			if (pTab && (pTab->Info.Status != tisInvalid))
			{
				pTab->Info.Status = tisPassive;
			}
		}

		// Активен Far? ВСЕ tisPassive табы этого Far-а - разрушаем
		if (anActiveFarPID)
		{
			for (int i = 0; i < mn_Used; i++)
			{
				CTabID *pTab = mpp_Stack[i];

				// Проверяем только табы активного фара (пассивные [Far -> Far -> Far] не трогаем)
				if (!pTab || (pTab->Info.nPID != anActiveFarPID))
					continue;
				// Панели и табы с не пассивным статусом - не трогаем
				if ((pTab->Type() == fwt_Panels) || (pTab->Info.Status != tisPassive))
					continue;

				// Таб (редактор/вьювер) был закрыт, разрушаем
				_ASSERTE(pTab->Type()==fwt_Editor || pTab->Type()==fwt_Viewer);

				// Kill this tab
				pTab->Info.Status = tisInvalid;
			}
		}
	}
	else
	{
		// Для самого таббара - весь хвост просто выкинуть из списка, но сами CTabID не разрушать (tisInvalid не ставить)
		for (int i = mn_UpdatePos; i < mn_Used; i++)
		{
			CTabID *pTab = mpp_Stack[i];
			if (pTab)
			{
				if (!CVConGroup::isValid((CVirtualConsole*)pTab->Info.pVCon))
					pTab->Info.Status = tisInvalid;
				#ifdef TAB_REF_PLACE
				pTab->DelPlace(m_rp);
				#endif
				pTab->Release();
				// Remove from list
				mpp_Stack[i] = NULL;

				bChanged = true;
			}
		}
	}

	for (int i = 0; i < mn_Used; i++)
	{
		CTabID *pTab = mpp_Stack[i];

		// Kill all invalid tabs
		if (!pTab || (pTab->Info.Status != tisInvalid))
			continue;

		#ifdef TAB_REF_PLACE
		pTab->DelPlace(m_rp);
		#endif
		pTab->Release();
		// Remove from list
		mpp_Stack[i] = NULL;

		bChanged = true;
	}

	CleanNulls();

	pUpdateLock->Unlock();
	delete pUpdateLock;

	return bChanged;
}

#if 0
void CTabStack::RecheckPassive()
{
	if (mn_UpdatePos < mn_Used)
	{

		// Освободить все элементы за mn_UpdatePos
		int i = mn_UpdatePos;
		DEBUGTEST(int iPrev = mn_Used);
		while (i < mn_Used)
		{
			CTabID *pTab = mpp_Stack[i];

			if (pTab)
			{
				_ASSERTE(pTab->Info.pVCon!=NULL);
				if (abRConTabs && pTab->Info.pVCon)
				{
					bool bVConValid = false, bPidValid = false, bPassive = false;
					// Only for RCon->tabs.m_Tabs we must to check, if tab is:
					// 'Still Alive'
					// 'Hidden' (Another started Far hides editor/viewer of parent Far)
					// 'Terminated' (Editor/viewer was closed or Far instance terminated)
					CVConGroup::CheckTabValid(pTab, bVConValid, bPidValid, bPassive);

					if (bPidValid)
					{
						if (bPassive)
						{
							// Пока оставить этот таб
							i++;
							continue;
						}
					}
				}

				// If we get here - tab was terminated

				if (abRConTabs)
					pTab->Info.Status = tisInvalid;

				#ifdef TAB_REF_PLACE
				pTab->DelPlace(m_rp);
				#endif

				// Kill this tab
				pTab->Release();
				mpp_Stack[i] = NULL;
			}

			DEBUGTEST(iPrev = mn_Used);
			PRAGMA_ERROR("FAIL. Нужно убрать из стека все NULL-ячейки");
			TabDeleted(pUpdateLock, i);
			_ASSERTE(mn_Used < iPrev);
		}

		mn_Used = mn_UpdatePos;
	}
}
#endif

void CTabStack::CleanNulls()
{
	// Убрать дырки (NULL) из списка
	int i = 0;
	while (i < mn_Used)
	{
		// Empty cell?
		if (!(mpp_Stack[i]))
		{
			int j = i+1;
			while (j < mn_Used)
			{
				if (mpp_Stack[j])
				{
					mpp_Stack[i] = mpp_Stack[j];
					mpp_Stack[j] = NULL;
					break;
				}
				j++;
			}
		}

		// Still NULL?
		if (!(mpp_Stack[i]))
			break;

		i++;
	}

	mn_Used = mn_UpdatePos = i;
}

void CTabStack::ReleaseTabs(BOOL abInvalidOnly /*= TRUE*/)
{
	if (!this || !mpp_Stack || !mn_Used || !mn_MaxCount)
		return;

	MSectionLockSimple SC; SC.Lock(mpc_Section); // Сразу Exclusive lock

	// Идем сзади, т.к. нужно будет сдвигать элементы
	for (int i = (mn_Used - 1); i >= 0; i--)
	{
		if (!mpp_Stack[i])
			continue;
		if (abInvalidOnly)
		{
			if (mpp_Stack[i]->Info.Status == tisValid || mpp_Stack[i]->Info.Status == tisPassive)
				continue;
		}
		CTabID *p = mpp_Stack[i];
		mpp_Stack[i] = NULL;
		#ifdef TAB_REF_PLACE
		p->DelPlace(m_rp);
		#endif
		// Именно Release, т.к. ИД может быть использован в других стеках
		p->Release();

		mn_Used--;

		// Сдвинуть хвост, если есть
		if (abInvalidOnly && (mn_Used > 1) && (i < (mn_Used-1)))
		{
			memmove(mpp_Stack+i, mpp_Stack+i+1, sizeof(CTabID*) * (mn_Used - i));
		}
	}
}

// Вызывается при
// а) закрытии консоли - убить все табы
// б) рестарте консоли - убить все кроме панелей
// в) смене активного Far (запуск "far /e ..." например) - пометить пассивными все кроме панелей (некрасиво немного)
void CTabStack::MarkTabsInvalid(MatchTabEnum MatchTab, DWORD nFarPID)
{
	MSectionLockSimple SC; SC.Lock(mpc_Section); // Сразу Exclusive lock
	int iSkipped = 0;

	for (int i = 0; i < mn_Used; i++)
	{
		if (!mpp_Stack[i])
			continue;

		if (MatchTab == MatchNonPanel)
		{
			// Смысл в том, чтобы даже при рестарте консоли, при запуске Far->Cmd->Far,
			// и любых других изменениях табов - ПЕРВЫЙ таб всегда оставался живой
			// для этой консоли. Для того, чтобы на нем не терялись назначенные юзером
			// дополнительные флаги отрисовки (цвет таба или метка, например)

			// При [ре]старте консоли - таб панелей оставить
			// или при закрытии фара - убить все его редакторы/вьюверы
			if (nFarPID)
			{
				if (mpp_Stack[i]->Info.nPID != nFarPID)
				{
					iSkipped++;
					continue;
				}
				_ASSERTE(mpp_Stack[i]->Type() != fwt_Panels);
			}
			else if (mpp_Stack[i]->Type() == fwt_Panels)
			{
				if (!iSkipped)
				{
					iSkipped++;
					continue;
				}
				_ASSERTE(iSkipped==0 && "Must be only one fwt_Panels in RCon.m_Tabs");
			}
		}

		mpp_Stack[i]->Info.Status = tisInvalid;
	}
}

bool CTabStack::RefreshFarStatus(DWORD nFarPID, CTab& rActiveTab, int& rnActiveIndex, int& rnActiveCount, bool& rbHasModalTab)
{
	MSectionLockSimple SC; SC.Lock(mpc_Section); // Сразу Exclusive lock
	bool bChanged = false;
	int iCount = 0;
	int iActive = -1;
	int iModal = -1;
	int iFirstAvailable = -1;
	int iFirstPanels = -1;
	DEBUGTEST(int iPanelsCount = 0);
	bool bPanels;

	for (int i = 0; i < mn_Used; i++)
	{
		if (!mpp_Stack[i])
			continue;

		// fwt_Panels всегда должен быть
		bPanels = (mpp_Stack[i]->Type() == fwt_Panels);
		#ifdef _DEBUG
		if (bPanels) iPanelsCount++;
		#endif

		// When returning to Far - mark its editors/viewers as Valid
		if (nFarPID && (mpp_Stack[i]->Info.nPID == nFarPID))
		{
			if (mpp_Stack[i]->Info.Status == tisPassive)
			{
				mpp_Stack[i]->Info.Status = tisValid;
				bChanged = true;
			}
		}
		else if (!bPanels)
		{
			if (mpp_Stack[i]->Info.Status == tisValid)
			{
				mpp_Stack[i]->Info.Status = tisPassive;
				bChanged = true;
			}
		}

		// Find active tabs
		if (mpp_Stack[i]->Info.Status == tisValid)
		{
			iCount++;
			if (mpp_Stack[i]->Info.Type & fwt_CurrentFarWnd)
			{
				if (iActive >= 0)
				{
					// That may be happened if some external command was started from Editor/Viewer
					// so the first tab was marked as active (console/aka panels)
					// And when the Far was back - there are two active tabs... but Editor/Viewer is active
					_ASSERTE(((iActive == -1) || (mpp_Stack[iActive]->Type() == fwt_Panels)) && "Only one active tab per console!");
					// To avoid weird behavior in future
					mpp_Stack[iActive]->Info.Type &= ~fwt_CurrentFarWnd;
				}
				iActive = i;
			}
			if (mpp_Stack[i]->Info.Type & fwt_ModalFarWnd)
			{
				_ASSERTE((mpp_Stack[i]->Info.Type & fwt_CurrentFarWnd) && "Modal must be current");
				iModal = i;
			}
			if (iFirstAvailable < 0)
			{
				iFirstAvailable = i;
			}
		}
		else if ((iFirstPanels < 0) && bPanels && (mpp_Stack[i]->Info.Status == tisPassive))
		{
			// May be when returning from "far /e ..." to cmd or another std Far instance
			iFirstPanels = i;
		}
	}

	_ASSERTE(iPanelsCount==1);

	// Во время закрытия фара могут быть пертурбации
	if (iActive < 0)
	{
		if (iModal >= 0)
		{
			iActive = iModal;
		}
		else if (iFirstAvailable >= 0)
		{
			iActive = iFirstAvailable;
		}
		else if (iFirstPanels >= 0)
		{
			_ASSERTE(iCount==0);
			iActive = iFirstPanels;
			// There was no one "Active" tab. Mark panels tab as active!
			_ASSERTE(nFarPID == 0); // returning from "far /e ..."?
			_ASSERTE(mpp_Stack[iFirstPanels] != NULL); // Protected with CS, no need to check
			mpp_Stack[iFirstPanels]->Info.Status = tisValid;
			iCount = 1;
		}
		// Check return value
		if (!bChanged)
			bChanged = (iActive >= 0);
	}

	_ASSERTE(iCount>0 && "At least one tab must be available");

	// Во время закрытия фара может возникать...
	if ((iActive >= 0) && !(mpp_Stack[iActive]->Info.Type & fwt_CurrentFarWnd))
	{
		mpp_Stack[iActive]->Info.Type |= fwt_CurrentFarWnd;
	}

	rbHasModalTab = (iModal >= 0);
	rnActiveCount = iCount;
	rActiveTab.Init((iActive >= 0) ? mpp_Stack[iActive] : NULL);
	rnActiveIndex = iActive;

	return bChanged;
}
