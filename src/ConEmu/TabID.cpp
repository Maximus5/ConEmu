
/*
Copyright (c) 2009-2012 Maximus5
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
#include "Header.h"
#include "TabID.h"
#include "../common/WinObjects.h"
#include "ConEmu.h"
#include "VConGroup.h"

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
LPCWSTR TabName::Set(LPCWSTR asName)
{
	#ifdef _DEBUG
	nLen = asName ? lstrlenW(asName) : -1;
	if ((nLen <= 0) || (nLen > 0 && asName[nLen-1] == L' '))
		nLen = nLen;
	#endif

	lstrcpynW(sz, asName ? asName : L"", countof(sz));

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
	return sz;
}
int TabName::Length() const
{
	return nLen;
}
bool TabName::Empty() const
{
	return (sz[0] != 0);
}





/* Uniqualizer for Each tab */
CTabID::CTabID(CVirtualConsole* apVCon, LPCWSTR asName, CEFarWindowType anType, int anPID, int anFarWindowID, int anViewEditID)
{
	memset(&Info, 0, sizeof(Info));
	memset(&DrawInfo, 0, sizeof(DrawInfo));

	//bExisted = true;
	Info.pVCon = apVCon;
	Set(asName, anType, anPID, anFarWindowID, anViewEditID);


	//Info.Status = tisValid;
	//Info.Flags = anFlags;
	//Info.Type = anType; // etfPanels/etfEditor/etfViewer
	//Info.nPID = anPID; // ИД процесса, содержащего таб (актуально для редакторов/вьюверов)
	//Info.nFarWindowID = anFarWindowID;
	//Info.nViewEditID = anViewEditID;
	//// Name
	//Name.Init(asName);
	//Upper.Init(asName);
	//Upper.MakeUpper();
}
LPCWSTR CTabID::GetName()
{
	LPCWSTR pszName = ((Info.Type & fwt_Renamed) && (Renamed.Length() > 0)) ? Renamed.Ptr() : Name.Ptr();
	if (!pszName || !*pszName)
		pszName = gpConEmu->GetDefaultTabLabel();
	return pszName;
}
LPCWSTR CTabID::GetLabel()
{
	LPCWSTR pszLabel = DrawInfo.Display.Ptr();
	if (!pszLabel || !*pszLabel)
		pszLabel = GetName();
	return pszLabel;
}
void CTabID::Set(LPCWSTR asName, CEFarWindowType anType, int anPID, int anFarWindowID, int anViewEditID)
{
	//bExisted = true;
	Info.Status = tisValid;
	Info.Type = anType; // enum CEFarWindowType (fwt_...)
	Info.nPID = anPID; // ИД процесса, содержащего таб (актуально для редакторов/вьюверов)
	Info.nFarWindowID = anFarWindowID;
	Info.nViewEditID = anViewEditID;
	// Name
	Name.Set(asName);
	//Upper.Set(asName);
	//Upper.MakeUpper();
}
CTabID::~CTabID()
{
	Name.Release();
	Renamed.Release();
	ReleaseDrawRegion();
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
	TabName t(asName);
	return IsEqual(apVCon, t, anType, anPID, anViewEditID, FlagMask);
}
bool CTabID::IsEqual(CVirtualConsole* apVCon, const TabName& asName, CEFarWindowType anType, int anPID, int anViewEditID, CEFarWindowType FlagMask)
{
	if (!this)
		return false; // Invalid arguments

	// Невалидный таб (процесс был завершен)
	if (Info.Status == tisEmpty || Info.Status == tisInvalid)
		return false;

	if (apVCon != this->Info.pVCon)
		return false;

	//// Модальный редактор должет бы соответвтсовать панелям (по логике переключений?)
	//if (anFarWindowID == 0 && this->Info.nFarWindowID == 0)
	//	return true; // OK, Это ГЛАВНЫЙ таб КОНСОЛИ

	//if (!abIgnoreWindowId)
	//{
	//	if (anFarWindowID != this->Info.nFarWindowID)
	//		return false;
	//}

	if ((anType & FlagMask) != (this->Info.Type & FlagMask))
		return false;

	//// Для редактора/вьювера проверям ИД процесса FAR & ИД редактора/вьювера
	//// FAR обещает уникальность ИД редактора/вьювера в пределах сессии
	//if (this->Info.Type == etfViewer || this->Info.Type == etfEditor)
	//{
	//	if ((anFarWindowID == 0) != (this->Info.nFarWindowID == 0))
	//		return false; // "Модальность" должна совпадать. (У модальных - WindowID==0)
	//
	//	// Редакторы считаются совпадающими только в том же процессе FAR & ИД вьювера/редактора
	//	if (anPID != this->Info.nPID
	//		|| anViewEditID != this->Info.nViewEditID)
	//		return false;
	//}

	// -- достаточно бы заложиться на ViewerEditorID, но его пока нет
	LPCWSTR psz1 = asName.Ptr();
	LPCWSTR psz2 = this->Name.Ptr();
	int nNameLen = this->Name.Length();
	if (asName.Length() != nNameLen)
		return false;
	if (wmemcmp(psz1, psz2, nNameLen))
		return false;

	// OK, различия не найдены, закладки совпадают
	return true;
}
#if 0
bool CTabID::IsEqual(const CTabID* pTabId, bool abIgnoreWindowId /*= false*/, CEFarWindowType FlagMask)
{
	if (!this || !pTabId)
		return false; // Invalid arguments

	// Невалидный таб (процесс был завершен)
	if (Info.Status == tisEmpty || Info.Status == tisInvalid)
		return false;
	if (!CVConGroup::isValid((CVirtualConsole*)Info.pVCon))
		return false;
	if (pTabId->Info.Status == tisEmpty || pTabId->Info.Status == tisInvalid)
		return false;
	if (!CVConGroup::isValid((CVirtualConsole*)pTabId->Info.pVCon))
		return false;

	if (!abIgnoreWindowId)
	{
		if (pTabId->Info.nFarWindowID != this->Info.nFarWindowID)
			return false;
	}

	return IsEqual( (CVirtualConsole*)pTabId->Info.pVCon, pTabId->Name, pTabId->Info.Type, pTabId->Info.nPID,
					pTabId->Info.nViewEditID, FlagMask );

	//if (pTabId->pVCon != this->pVCon)
	//	return false;
	//
	////// Модальный редактор должет бы соответвтсовать панелям (по логике переключений?)
	////if (pTabId->Info.nFarWindowID == 0 && this->Info.nFarWindowID == 0)
	////	return true; // OK, Это ГЛАВНЫЙ таб КОНСОЛИ
	//
	//if (!abIgnoreWindowId)
	//{
	//	if (pTabId->Info.nFarWindowID != this->Info.nFarWindowID)
	//		return false;
	//}
	//
	//if (pTabId->Info.Type != this->Info.Type)
	//	return false;
	//
	////// Для редактора/вьювера проверям ИД процесса FAR & ИД редактора/вьювера
	////// FAR обещает уникальность ИД редактора/вьювера в пределах сессии
	////if (this->Info.Type == etfViewer || this->Info.Type == etfEditor)
	////{
	////	if ((pTabId->Info.nFarWindowID == 0) != (this->Info.nFarWindowID == 0))
	////		return false; // "Модальность" должна совпадать. (У модальных - WindowID==0)
	////
	////	// Редакторы считаются совпадающими только в том же процессе FAR & ИД вьювера/редактора
	////	if (pTabId->Info.nPID != this->Info.nPID
	////		|| pTabId->Info.nViewEditID != this->Info.nViewEditID)
	////		return false;
	////}
	//
	//// -- достаточно бы заложиться на ViewerEditorID, но его пока нет
	//LPCWSTR psz1 = pTabId->Upper.Ptr();
	//LPCWSTR psz2 = this->Upper.Ptr();
	//if (pTabId->Upper.Length() != this->Upper.Length())
	//	return false;
	//if (memcmp(psz1, psz2, (this->Upper.Length()+1)*2))
	//	return false;
	//
	//// OK, различия не найдены, закладки совпадают
	//return true;
}
#endif
//UINT CTabID::Flags()
//{
//	return Info.Flags;
//}








CTabStack::CTabStack()
{
	mn_Used = 0;
	#ifdef _DEBUG
	mn_MaxCount = 1;
	#else
	mn_MaxCount = 16;
	#endif
	mpp_Stack = (CTabID**)calloc(mn_MaxCount,sizeof(CTabID**));
	InitializeCriticalSection(&mc_Section);
	//mp_UpdateLock = NULL;
	mn_UpdatePos = -1;
	mb_FarUpdateMode = false;
}
CTabStack::~CTabStack()
{
	if (mpp_Stack)
	{
		ReleaseTabs(FALSE);
		free(mpp_Stack);
		mpp_Stack = NULL;
	}
	//if (mp_UpdateLock)
	//{
	//	delete mp_UpdateLock;
	//	mp_UpdateLock = NULL;
	//}
	DeleteCriticalSection(&mc_Section);
}
//const CTabID* CTabStack::CreateOrFind(CVirtualConsole* apVCon, LPCWSTR asName, int anType, int anPID, int anFarWindowID, int anViewEditID, CEFarWindowType anFlags)
//{
//	CTabID* pTab = NULL;
//
//	MSectionLockSimple SC; SC.Lock(&mc_Section);
//
//	if (mpp_Stack && mn_Used > 0)
//	{
//		TabName upr; upr.Init(asName); upr.Upper();
//		for (int i = 0; i < mn_Used; i++)
//		{
//			if (!mpp_Stack[i]) continue;
//			if (mpp_Stack[i]->IsEqual(apVCon, upr.Ptr(), anType,
//					anPID, anFarWindowID, anViewEditID, anFlags, true/*abIgnoreWindowId*/))
//			{
//				pTab = mpp_Stack[i];
//				break;
//			}
//		}
//		upr.Release();
//		if (pTab)
//		{
//			// Просто обновить некоторые параметры
//			pTab->Info.nFarWindowID = anFarWindowID;
//			pTab->Info.Flags = anFlags;
//			pTab->Info.Type = anType;
//			pTab->Info.nPID = anPID;
//			return pTab; // Уже есть в списке
//		}
//	}
//
//	pTab = new CTabID(apVCon, asName, anType, anPID, anFarWindowID, anViewEditID, anFlags);
//
//	AppendInt(pTab, FALSE/*abMoveFirst*/, &SC);
//
//	return pTab;
//}

void CTabStack::RequestSize(int anCount, MSectionLockSimple* pSC)
{
	if (!mpp_Stack || (anCount > mn_MaxCount))
	{
		int nNewMaxCount = anCount
		#ifndef _DEBUG
			+15
		#endif
			;

		CTabID** ppNew = (CTabID**)calloc(nNewMaxCount, sizeof(CTabID**));
		if (mpp_Stack)
		{
			if (mn_Used > 0)
				memmove(ppNew, mpp_Stack, sizeof(CTabID**)*mn_Used);
			free(mpp_Stack);
		}

		mpp_Stack = ppNew;
		mn_MaxCount = nNewMaxCount;
	}
}

void CTabStack::AppendInt(CTabID* pTab, BOOL abMoveFirst, MSectionLockSimple* pSC)
{
	// Сразу накрутить счетчик таба
	pTab->AddRef();

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
		//int nNewMaxCount = mn_Used;
		//#ifdef _DEBUG
		//nNewMaxCount += 1;
		//#else
		//nNewMaxCount += 16;
		//#endif
		//
		//CTabID** ppNew = (CTabID**)calloc(nNewMaxCount, sizeof(CTabID**));
		//if (mpp_Stack)
		//{
		//	if (mn_Used > 0)
		//		memmove(ppNew, mpp_Stack, sizeof(CTabID**)*mn_Used);
		//	free(mpp_Stack);
		//}
		//
		//mpp_Stack = ppNew;
		//mn_MaxCount = nNewMaxCount;
	}

	if (abMoveFirst && mn_Used > 0)
	{
		memmove(mpp_Stack+1, mpp_Stack, mn_Used*sizeof(CTabID**));
		mpp_Stack[0] = NULL;
	}

	mpp_Stack[abMoveFirst ? 0 : mn_Used] = pTab;
	mn_Used++;
}
int CTabStack::GetCount()
{
	return mn_Used;
}
bool CTabStack::GetTabInfoByIndex(int anIndex, /*OUT*/ TabInfo& rInfo)
{
	bool lbFound = false;
	MSectionLockSimple SC; SC.Lock(&mc_Section);
	if (anIndex >= 0 && anIndex < mn_Used)
	{
		if (mpp_Stack[anIndex])
		{
			rInfo = mpp_Stack[anIndex]->Info;
			lbFound = true;
		}
	}
	SC.Unlock();
	return lbFound;
}
bool CTabStack::GetTabByIndex(int anIndex, /*OUT*/ CTab& rTab)
{
	MSectionLockSimple SC; SC.Lock(&mc_Section);

	if (anIndex >= 0 && anIndex < mn_Used)
	{
		rTab.Init(mpp_Stack[anIndex]);
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
	MSectionLockSimple SC; SC.Lock(&mc_Section);
	int nIndex = -1;
	for (int i = 0; i < mn_Used; i++)
	{
		if (mpp_Stack[i] == pTab)
		{
			nIndex = i;
			break;
		}
	}
	SC.Unlock();
	return nIndex;
}
bool CTabStack::GetNextTab(const CTabID* pTab, BOOL abForward, /*OUT*/ CTab& rTab)
{
	MSectionLockSimple SC; SC.Lock(&mc_Section);
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
	MSectionLockSimple SC; SC.Lock(&mc_Section);
	if (anIndex >= 0 && anIndex < mn_Used)
	{
		CTabID* pTab = mpp_Stack[anIndex];
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
		_ASSERTE(anIndex >= 0 && anIndex < mn_Used);
	}
	SC.Unlock();
	return lbExist;
}
bool CTabStack::SetTabDrawRect(int anIndex, const RECT& rcTab)
{
	bool lbExist = false;
	MSectionLockSimple SC; SC.Lock(&mc_Section);
	if (anIndex >= 0 && anIndex < mn_Used)
	{
		CTabID* pTab = mpp_Stack[anIndex];
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
		_ASSERTE(anIndex >= 0 && anIndex < mn_Used);
	}
	SC.Unlock();
	return lbExist;
}
void CTabStack::LockTabs(MSectionLockSimple* pLock)
{
	pLock->Lock(&mc_Section);
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
bool CTabStack::UpdateFarWindow(HANDLE hUpdate, CVirtualConsole* apVCon, LPCWSTR asName, CEFarWindowType anType, int anPID, int anFarWindowID, int anViewEditID)
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

	// Первый (FarWindow==0) таб консоли всегда обновляется!
	if (mn_UpdatePos == 0)
	{
		if (mn_Used == 0)
		{
			pTab = new CTabID(apVCon, asName, anType, anPID, anFarWindowID, anViewEditID);
			// Вобщем-то без разницы abMoveFirst или нет - массив сейчас все-равно пуст (консоль только что открыта)
			AppendInt(pTab, TRUE/*abMoveFirst*/, pUpdateLock);
			bChanged = true;
		}
		else
		{
			pTab = mpp_Stack[0];
			if (!pTab)
			{
				_ASSERTE(pTab!=NULL);
				return false;
			}
			// Изменился?
			bChanged = (pTab->Info.nFarWindowID != anFarWindowID)
				|| (pTab->Info.Status == tisEmpty || pTab->Info.Status == tisInvalid)
				|| (!pTab->IsEqual(apVCon, asName, anType, anPID, anViewEditID, (fwt_TypeMask|fwt_CompareFlags)));
			// Обновляем все подряд. Вместо панелей теперь может быть модальный редактор/вьювер
			pTab->Set(asName, anType, anPID, anFarWindowID, anViewEditID);
		}
		// Следующий
		mn_UpdatePos = 1;

		// OK, с первым табом (FarWindow==0) закончили
	}
	else
	{
		// Теперь - поехали обновлять. Правила такие:
		// 1. Новая вкладка в ФАР может появиться ТОЛЬКО в конце
		// 2. Закрыта может быть любая вкладка

		pTab = NULL;
		int i = mn_UpdatePos;
		while (i < mn_Used)
		{
			if (mpp_Stack[i]->IsEqual(apVCon, asName, anType, anPID, anViewEditID, fwt_TypeMask))
			{
				// OK, таб совпадает
				pTab = mpp_Stack[i];

				// Сменились флаги (Modifed/Current/...)?
				if ((anType & fwt_CompareFlags) != (mpp_Stack[i]->Info.Type & fwt_CompareFlags))
				{
					CEFarWindowType newFlags = (mpp_Stack[i]->Info.Type & ~fwt_CompareFlags);
					newFlags |= (anType & fwt_CompareFlags);
					mpp_Stack[i]->Info.Type = newFlags;
				}

				// Закончили
				break;
			}
			// таб был закрыт
			bChanged = true;
			mpp_Stack[i]->Info.Status = tisInvalid;
			mpp_Stack[i]->Release();
			mpp_Stack[i] = NULL;
			i++;
		}

		// Если перед совпавшим найдены закрытые - следует сдвинуть список табов
		if (i > mn_UpdatePos)
		{
			if ((i+1) < mn_Used)
			{
				#if 1
				_ASSERTE(pUpdateLock->isLocked());
				#else
				pUpdateLock->RelockExclusive();
				#endif
				// Все что между {mn_UpdatePos .. (i-1)} теперь уже забито NULL
				memmove(mpp_Stack+mn_UpdatePos, mpp_Stack+i, (mn_Used - i) * sizeof(CTabID**));
			}
			mn_Used -= (i - mn_UpdatePos);
			memset(mpp_Stack+mn_Used, 0, (i - mn_UpdatePos) * sizeof(CTabID**));
		}

		// Если таб новый
		if (pTab == NULL)
		{
			// это новая вкладка, добавляемая в конец
			pTab = new CTabID(apVCon, asName, anType, anPID, anFarWindowID, anViewEditID);
			_ASSERTE(mn_Used == mn_UpdatePos);
			AppendInt(pTab, FALSE/*abMoveFirst*/, pUpdateLock);
			bChanged = true;
		}

		mn_UpdatePos++;
		_ASSERTE(mn_UpdatePos>1 && mn_UpdatePos<=mn_Used);
	}

	return bChanged;
}

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
			if (mpp_Stack[mn_UpdatePos])
				mpp_Stack[mn_UpdatePos]->Release();
			mpp_Stack[nIndex] = NULL;
		}
		if (mpp_Stack[mn_UpdatePos] != pTab)
			pTab->AddRef();
		mpp_Stack[mn_UpdatePos] = pTab;
		mn_UpdatePos++;
		if (mn_UpdatePos > mn_Used)
			mn_Used = mn_UpdatePos;
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
			memmove(mpp_Stack+1, mpp_Stack, sizeof(CTabID**) * (nIndex-1));
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
bool CTabStack::UpdateEnd(HANDLE hUpdate, BOOL abForceReleaseTail)
{
	MSectionLockSimple* pUpdateLock = (MSectionLockSimple*)hUpdate;

	// Функция должна вызваться ТОЛЬКО между UpdateBegin & UpdateEnd
	if (mn_UpdatePos < 0)
	{
		_ASSERTE(mn_UpdatePos>=0);
		return false;
	}

	if (!mb_FarUpdateMode && (mn_UpdatePos == 0))
	{
		if (!CVConGroup::isVConExists(0))
		{
			abForceReleaseTail = TRUE;
		}
		else
		{
			// Фукнция UpdateFarWindow должна была быть вызвана хотя бы раз!
			//UpdateFarWindow(CVirtualConsole* apVCon, LPCWSTR asName, int anType, int anPID, int anFarWindowID, int anViewEditID, CEFarWindowType anFlags)
			_ASSERTE(mn_UpdatePos>0 || CVConGroup::GetConCount()==0);
			pUpdateLock->Unlock();
			delete pUpdateLock;
			//mp_UpdateLock = NULL;
			mn_UpdatePos = -1;
			return false;
		}
	}

	if (!abForceReleaseTail && mn_UpdatePos > 1)
		abForceReleaseTail = TRUE;

	bool bChanged = (mn_Used != mn_UpdatePos);

	if (abForceReleaseTail && mn_UpdatePos < mn_Used)
	{
		#if 1
		_ASSERTE(pUpdateLock->isLocked());
		#else
		pUpdateLock->RelockExclusive();
		#endif
		// Освободить все элементы за mn_UpdatePos
		for (int i = mn_UpdatePos; i < mn_Used; i++)
		{
			CTabID *pTab = mpp_Stack[i];
			mpp_Stack[i] = NULL;
			if (pTab)
			{
				pTab->Info.Status = tisInvalid;
				pTab->Release();
			}
		}
		mn_Used = mn_UpdatePos;
	}

	pUpdateLock->Unlock();
	delete pUpdateLock;
	//mp_UpdateLock = NULL;

	return bChanged;
}
void CTabStack::ReleaseTabs(BOOL abInvalidOnly /*= TRUE*/)
{
	if (!this || !mpp_Stack || !mn_Used || !mn_MaxCount)
		return;

	MSectionLockSimple SC; SC.Lock(&mc_Section, TRUE); // Сразу Exclusive lock

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
		// Именно Release, т.к. ИД может быть использован в других стеках
		p->Release();

		mn_Used--;

		// Сдвинуть хвост, если есть
		if ((mn_Used > 1) && (i < (mn_Used-1)))
		{
			memmove(mpp_Stack+i, mpp_Stack+i+1, sizeof(CTabID**) * (mn_Used - i));
		}
	}
}
