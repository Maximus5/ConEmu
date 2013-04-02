
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


#include "Header.h"

#include "../common/common.hpp"
#include "../common/ConEmuCheck.h"
#include "../common/WinObjects.h"
#include "ConEmu.h"
#include "ConfirmDlg.h"
#include "Inside.h"
#include "Options.h"
#include "RealConsole.h"
#include "RunQueue.h"
#include "Status.h"
#include "TabBar.h"
#include "Update.h"
#include "VConChild.h"
#include "VConGroup.h"
#include "VirtualConsole.h"

#define DEBUGSTRDRAW(s) //DEBUGSTR(s)
#define DEBUGSTRTABS(s) //DEBUGSTR(s)
#define DEBUGSTRLANG(s) //DEBUGSTR(s)
#define DEBUGSTRERR(s) DEBUGSTR(s)
#define DEBUGSTRATTACHERR(s) DEBUGSTR(s)

static CVirtualConsole* gp_VCon[MAX_CONSOLE_COUNT] = {};

static CVirtualConsole* gp_VActive = NULL;
static bool gb_CreatingActive = false, gb_SkipSyncSize = false;
static UINT gn_CreateGroupStartVConIdx = 0;
static bool gb_InCreateGroup = false;

static CRITICAL_SECTION gcs_VGroups;
static CVConGroup* gp_VGroups[MAX_CONSOLE_COUNT*2] = {}; // на каждое разбиение добавляется +Parent

//CVirtualConsole* CVConGroup::mp_GrpVCon[MAX_CONSOLE_COUNT] = {};

static COORD g_LastConSize = {0,0}; // console size after last resize (in columns and lines)


void CVConGroup::Initialize()
{
	InitializeCriticalSection(&gcs_VGroups);
}

void CVConGroup::Deinitialize()
{
	DeleteCriticalSection(&gcs_VGroups);
}


// Вызывается при создании нового таба, без разбивки
CVConGroup* CVConGroup::CreateVConGroup()
{
	_ASSERTE(gpConEmu->isMainThread()); // во избежание сбоев в индексах?
	CVConGroup* pGroup = new CVConGroup(NULL);
	return pGroup;
}

CVConGroup* CVConGroup::SplitVConGroup(RConStartArgs::SplitType aSplitType /*eSplitHorz/eSplitVert*/, UINT anPercent10 /*= 500*/)
{
	if (!this || !(aSplitType == RConStartArgs::eSplitHorz || aSplitType == RConStartArgs::eSplitVert))
	{
		_ASSERTE(this);
		return NULL;
	}

	if (mp_Item == NULL)
	{
		_ASSERTE(mp_Item && "VCon was not associated");
		return NULL;
	}
	
	// Разбивать можно только то, что еще не разбито ("листья")
	if (m_SplitType != RConStartArgs::eSplitNone)
	{
		MBoxAssert(m_SplitType == RConStartArgs::eSplitNone && "Can't split this pane");
		return CreateVConGroup();
	}
	
	// Создать две пустых панели без привязки
	_ASSERTE(mp_Grp1==NULL && mp_Grp2==NULL);
	mp_Grp1 = new CVConGroup(this);
	mp_Grp2 = new CVConGroup(this);
	if (!mp_Grp1 || !mp_Grp2)
	{
		_ASSERTE(mp_Grp1 && mp_Grp2);
		SafeDelete(mp_Grp1);
		SafeDelete(mp_Grp2);
		return NULL;
	}

	// Параметры разбиения
	m_SplitType = aSplitType; // eSplitNone/eSplitHorz/eSplitVert
	mn_SplitPercent10 = max(1,min(anPercent10,999)); // (0.1% - 99.9%)*10
	mrc_Splitter = MakeRect(0,0);

	// Перенести в mp_Grp1 текущий VCon, mp_Grp2 - будет новый пустой (пока) панелью
	mp_Grp1->mp_Item = mp_Item;
	mp_Item->mp_Group = mp_Grp1;
	mp_Item = NULL; // Отвязываемся от VCon, его обработкой теперь занимается mp_Grp1

	SetResizeFlags();

	return mp_Grp2;
}


CVirtualConsole* CVConGroup::CreateVCon(RConStartArgs *args, CVirtualConsole*& ppVConI)
{
	_ASSERTE(ppVConI == NULL);
	if (!args)
	{
		_ASSERTE(args!=NULL);
		return NULL;
	}

	_ASSERTE(gpConEmu->isMainThread()); // во избежание сбоев в индексах?

	if (args->pszSpecialCmd)
	{
		args->ProcessNewConArg();
	}

	if (args->bForceUserDialog)
	{
		_ASSERTE(args->aRecreate!=cra_RecreateTab);
		args->aRecreate = cra_CreateTab;

		int nRc = gpConEmu->RecreateDlg(args);
		if (nRc != IDC_START)
			return NULL;

		// После диалога могли измениться параметры группы
	}

	CVConGroup* pGroup = NULL;
	if (gp_VActive && args->eSplit)
	{
		CVConGuard VCon;
		if (((args->nSplitPane && GetVCon(gn_CreateGroupStartVConIdx+args->nSplitPane-1, &VCon))
				|| (GetActiveVCon(&VCon) >= 0))
			&& VCon->mp_Group)
		{
			pGroup = ((CVConGroup*)VCon->mp_Group)->SplitVConGroup(args->eSplit, args->nSplitValue);
		}
		
		_ASSERTE((pGroup!=NULL) && "No active VCon?");
	}
	// Check
	if (!pGroup)
	{
		pGroup = CreateVConGroup();

		if (!pGroup)
			return NULL;
	}

	CVirtualConsole* pVCon = new CVirtualConsole();
	ppVConI = pVCon;
	pGroup->mp_Item = pVCon;
	pVCon->mp_Group = pGroup;

	//pVCon->Constructor(args);
	//if (!pVCon->mp_RCon->PreCreate(args))

	if (!pVCon->Constructor(args))
	{
		ppVConI = NULL;
		CVConGroup::OnVConClosed(pVCon);
		#ifdef _DEBUG
		//-- must be already released!
		#ifndef _WIN64
		_ASSERTE((DWORD_PTR)pVCon->mp_RCon==0xFEEEFEEE);
		#else
		_ASSERTE((DWORD_PTR)pVCon->mp_RCon==0xFEEEFEEEFEEEFEEELL);
		#endif
		//pVCon->Release();
		#endif
		return NULL;
	}

	pGroup->GetRootGroup()->InvalidateAll();

	return pVCon;
}


CVConGroup::CVConGroup(CVConGroup *apParent)
{
	mp_Item = NULL/*apVCon*/;     // консоль, к которой привязан этот "Pane"
	//apVCon->mp_Group = this;
	m_SplitType = RConStartArgs::eSplitNone;
	mn_SplitPercent10 = 500; // Default - пополам
	mrc_Splitter = MakeRect(0,0);
	mp_Grp1 = mp_Grp2 = NULL; // Ссылки на "дочерние" панели
	mp_Parent = apParent; // Ссылка на "родительскую" панель

	EnterCriticalSection(&gcs_VGroups);
	bool bAdded = false;
	for (size_t i = 0; i < countof(gp_VGroups); i++)
	{
		if (gp_VGroups[i] == NULL)
		{
			gp_VGroups[i] = this;
			bAdded = true;
			break;
		}
	}
	_ASSERTE(bAdded && "gp_VGroups overflow");
	LeaveCriticalSection(&gcs_VGroups);
}

CVConGroup::~CVConGroup()
{
	_ASSERTE(gpConEmu->isMainThread()); // во избежание сбоев в индексах?

	// Не должно быть дочерних панелей
	_ASSERTE(mp_Grp1==NULL && mp_Grp2==NULL);
	//SafeDelete(mp_Grp1);
	//SafeDelete(mp_Grp2);

	EnterCriticalSection(&gcs_VGroups);

	if (mp_Parent)
	{
		// Если есть родитель - то нужно перекинуть "AnotherPane" в родителя,
		// чтобы не было разрывов в дереве панелей.
		CVConGroup* p = GetAnotherGroup();
		if (!p)
		{
			_ASSERTE(p);
		}
		else
		{
			p->MoveToParent(mp_Parent);
			delete p;
		}
	}

	bool bRemoved = false;
	for (size_t i = 0; i < countof(gp_VGroups); i++)
	{
		if (gp_VGroups[i] == this)
		{
			gp_VGroups[i] = NULL;
			_ASSERTE(!bRemoved);
			bRemoved = true;
		}
	}
	_ASSERTE(bRemoved && "Was not pushed in gp_VGroups?");

	LeaveCriticalSection(&gcs_VGroups);
}

void CVConGroup::OnVConDestroyed(CVirtualConsole* apVCon)
{
	if (apVCon && apVCon->mp_Group)
	{
		CVConGroup* p = (CVConGroup*)apVCon->mp_Group;
		apVCon->mp_Group = NULL;
		delete p;
	}
}

CVConGroup* CVConGroup::GetRootGroup()
{
	if (!this)
	{
		_ASSERTE(FALSE);
		return NULL;
	}

	CVConGroup* p = this;
	while (p->mp_Parent)
	{
		p = p->mp_Parent;
	}

	_ASSERTE(p && (p->mp_Parent == NULL));
	return p;
}

CVConGroup* CVConGroup::GetRootOfVCon(CVirtualConsole* apVCon)
{
	if (!apVCon)
	{
		_ASSERTE(apVCon != NULL);
		return NULL;
	}

	CVConGuard VCon(apVCon);

	if (!apVCon || !apVCon->mp_Group)
	{
		_ASSERTE(apVCon && apVCon->mp_Group);
		return NULL;
	}

	CVConGroup* p = ((CVConGroup*)apVCon->mp_Group)->GetRootGroup();
	return p;
}

CVConGroup* CVConGroup::GetAnotherGroup()
{
	if (!this)
	{
		_ASSERTE(FALSE);
		return NULL;
	}

	if (!mp_Parent)
	{
		return NULL;
	}

	CVConGroup* p = (mp_Parent->mp_Grp1 == this) ? mp_Parent->mp_Grp2 : mp_Parent->mp_Grp1;
	_ASSERTE(p && p != this && p->mp_Parent == mp_Parent);
	return p;
}

void CVConGroup::SetResizeFlags()
{
	if (!this)
	{
		_ASSERTE(this);
		return;
	}

	CVConGroup* p = GetRootGroup();
	if (p)
	{
		p->mb_ResizeFlag = true;
	}
}

void CVConGroup::MoveToParent(CVConGroup* apParent)
{
	// По идее пока только так.
	_ASSERTE(apParent && apParent == mp_Parent);

	// Не должно быть И VCon и разбиения на группы
	_ASSERTE((mp_Item!=NULL) != (mp_Grp1!=NULL || mp_Grp2!=NULL));

	apParent->SetResizeFlags();
	
	apParent->mp_Item = mp_Item;
	apParent->m_SplitType = m_SplitType; // eSplitNone/eSplitHorz/eSplitVert
	apParent->mn_SplitPercent10 = mn_SplitPercent10; // (0.1% - 99.9%)*10
	apParent->mrc_Splitter = mrc_Splitter;

	// Ссылки на "дочерние" панели
	apParent->mp_Grp1 = mp_Grp1;
	apParent->mp_Grp2 = mp_Grp2;
	// Ссылки на "родительскую" панель
	if (mp_Grp1)
	{
		mp_Grp1->mp_Parent = apParent;
		mp_Grp1 = NULL;
	}
	if (mp_Grp2)
	{
		mp_Grp2->mp_Parent = apParent;
		mp_Grp2 = NULL;
	}

	// VCon
	if (mp_Item)
	{
		_ASSERTE(mp_Grp1==NULL && mp_Grp2==NULL);
		mp_Item->mp_Group = apParent;
		mp_Item = NULL; // Отвязываемся от VCon, его обработкой теперь занимается mp_Grp1
	}

	mp_Parent = NULL;
}

void CVConGroup::GetAllTextSize(SIZE& sz, bool abMinimal /*= false*/)
{
	if (!this)
	{
		_ASSERTE(this);
		sz.cx = MIN_CON_WIDTH;
		sz.cy = MIN_CON_HEIGHT;
		return;
	}

	sz.cx = sz.cy = 0;

	_ASSERTE((m_SplitType==RConStartArgs::eSplitNone) == (mp_Grp1==NULL && mp_Grp2==NULL && mp_Item!=NULL));
	if (m_SplitType==RConStartArgs::eSplitNone)
	{
		CVConGuard VCon(mp_Item);
		if (!abMinimal && mp_Item && mp_Item->RCon())
		{
			sz.cx = mp_Item->RCon()->TextWidth();
			sz.cy = mp_Item->RCon()->TextHeight();
		}
		else
		{
			_ASSERTE(mp_Item!=NULL);
			sz.cx = MIN_CON_WIDTH;
			sz.cy = MIN_CON_HEIGHT;
		}
	}
	else
	{
		CVConGuard VCon1(mp_Grp1->mp_Item);
		CVConGuard VCon2(mp_Grp2->mp_Item);
		SIZE sz1 = {MIN_CON_WIDTH,MIN_CON_HEIGHT}, sz2 = {MIN_CON_WIDTH,MIN_CON_HEIGHT};
		
		if (mp_Grp1)
		{
			mp_Grp1->GetAllTextSize(sz1, abMinimal);
		}
		else
		{
			_ASSERTE(mp_Grp1!=NULL);
		}

		sz = sz1;

		if (mp_Grp2 /*&& (m_SplitType == RConStartArgs::eSplitHorz)*/)
		{
			mp_Grp2->GetAllTextSize(sz2, abMinimal);
		}
		else
		{
			_ASSERTE(mp_Grp2!=NULL);
		}

		// Add second pane
		if (m_SplitType == RConStartArgs::eSplitHorz)
			sz.cx += sz2.cx;
		else if (m_SplitType == RConStartArgs::eSplitVert)
			sz.cy += sz2.cy;
		else
		{
			_ASSERTE((m_SplitType == RConStartArgs::eSplitHorz) || (m_SplitType == RConStartArgs::eSplitVert));
		}
	}

	return;
}

//uint CVConGroup::AllTextHeight()
//{
//	if (!this)
//	{
//		_ASSERTE(this);
//		return 0;
//	}
//	uint nSize = 0;
//	_ASSERTE((m_SplitType==RConStartArgs::eSplitNone) == (mp_Grp1==NULL && mp_Grp2==NULL && mp_Item!=NULL));
//	if (m_SplitType==RConStartArgs::eSplitNone)
//	{
//		CVConGuard VCon(mp_Item);
//		if (mp_Item && mp_Item->RCon())
//		{
//			nSize = mp_Item->RCon()->TextHeight();
//		}
//		else
//		{
//			_ASSERTE(mp_Item!=NULL);
//		}
//	}
//	else
//	{
//		CVConGuard VCon1(mp_Grp1->mp_Item);
//		CVConGuard VCon2(mp_Grp2->mp_Item);
//	
//		if (mp_Grp1)
//		{
//			nSize += mp_Grp1->AllTextHeight();
//		}
//		else
//		{
//			_ASSERTE(mp_Grp1!=NULL);
//		}
//		
//		if (mp_Grp2 && (m_SplitType == RConStartArgs::eSplitVert))
//		{
//			nSize += mp_Grp2->AllTextHeight();
//		}
//		else
//		{
//			_ASSERTE(mp_Grp2!=NULL);
//		}
//	}
//	return nSize;
//}

void CVConGroup::LogString(LPCSTR asText, BOOL abShowTime /*= FALSE*/)
{
	if (gpSetCls->isAdvLogging && gp_VActive)
		gp_VActive->RCon()->LogString(asText, abShowTime);
}

void CVConGroup::LogString(LPCWSTR asText, BOOL abShowTime /*= FALSE*/)
{
	if (gpSetCls->isAdvLogging && gp_VActive)
		gp_VActive->RCon()->LogString(asText, abShowTime);
}

void CVConGroup::LogInput(UINT uMsg, WPARAM wParam, LPARAM lParam, LPCWSTR pszTranslatedChars /*= NULL*/)
{
	if (gpSetCls->isAdvLogging && gp_VActive)
		gp_VActive->RCon()->LogInput(uMsg, wParam, lParam, pszTranslatedChars);
}

void CVConGroup::StopSignalAll()
{
	for (size_t i = 0; i < countof(gp_VCon); i++)
	{
		if (gp_VCon[i])
		{
			gp_VCon[i]->RCon()->StopSignal();
		}
	}
}

void CVConGroup::DestroyAllVCon()
{
	for (size_t i = countof(gp_VCon); i--;)
	{
		if (gp_VCon[i])
		{
			CVirtualConsole* p = gp_VCon[i];
			gp_VCon[i] = NULL;
			p->Release();
		}
	}
}

void CVConGroup::OnDestroyConEmu()
{
	// Нужно проверить, вдруг окно закрылось без нашего ведома (InsideIntegration)
	for (size_t i = countof(gp_VCon); i--;)
	{
		if (gp_VCon[i] && gp_VCon[i]->RCon())
		{
			if (!gp_VCon[i]->RCon()->isDetached())
			{
				gp_VCon[i]->RCon()->Detach(true, true);
			}
		}
	}
}

void CVConGroup::OnAlwaysShowScrollbar(bool abSync /*= true*/)
{
	for (size_t i = 0; i < countof(gp_VCon); i++)
	{
		if (gp_VCon[i])
			gp_VCon[i]->OnAlwaysShowScrollbar(abSync);
	}
}

void CVConGroup::RepositionVCon(RECT rcNewCon, bool bVisible)
{
	if (!this)
	{
		_ASSERTE(this);
		return;
	}

	bool lbPosChanged = false;
	RECT rcCurCon = {}, rcCurBack = {};

	CVConGuard VConI(mp_Item);
	if (m_SplitType == RConStartArgs::eSplitNone)
	{
		_ASSERTE(mp_Grp1==NULL && mp_Grp2==NULL);

		//RECT rcScroll = gpConEmu->CalcMargins(CEM_SCROLL);
		//gpConEmu->AddMargins(rcNewCon, rcScroll);
		
		HWND hWndDC = mp_Item ? VConI->GetView() : NULL;
		HWND hBack = mp_Item ? VConI->GetBack() : NULL;
		if (hWndDC)
		{
			GetWindowRect(hWndDC, &rcCurCon);
			GetWindowRect(hBack, &rcCurBack);

			_ASSERTE(GetParent(hWndDC) == ghWnd && ghWnd);
			MapWindowPoints(NULL, ghWnd, (LPPOINT)&rcCurCon, 2);
			MapWindowPoints(NULL, ghWnd, (LPPOINT)&rcCurBack, 2);

			RECT rcDC = gpConEmu->CalcRect(CER_DC, rcNewCon, CER_BACK, VConI.VCon());

			if (bVisible)
			{
				lbPosChanged = memcmp(&rcCurCon, &rcDC, sizeof(RECT))!=0
					|| memcmp(&rcCurBack, &rcNewCon, sizeof(RECT))!=0;
			}
			else
			{
				// Тут нас интересует только X/Y
				lbPosChanged = memcmp(&rcCurCon, &rcDC, sizeof(POINT))!=0
					|| memcmp(&rcCurBack, &rcNewCon, sizeof(RECT))!=0;
			}

			// Двигаем/ресайзим
			if (lbPosChanged)
			{
				VConI->SetVConSizePos(rcNewCon, bVisible);
				//if (bVisible)
				//{
				//	// Двигаем/ресайзим окошко DC
				//	MoveWindow(hWndDC, rcNewCon.left, rcNewCon.top, rcNewCon.right - rcNewCon.left, rcNewCon.bottom - rcNewCon.top, 1);
				//	VConI->Invalidate();
				//}
				//else
				//{
				//	// Двигаем окошко DC
				//	SetWindowPos(hWndDC, NULL, rcNewCon.left, rcNewCon.top, 0,0, SWP_NOSIZE|SWP_NOZORDER);
				//}
			}
		}

		if (VConI->RCon()->GuiWnd())
		{
			VConI->RCon()->SyncGui2Window(NULL);
		}
	}
	else if (mp_Grp1 && mp_Grp2)
	{
		RECT rcCon1, rcCon2, rcSplitter;
		CalcSplitRect(rcNewCon, rcCon1, rcCon2, rcSplitter);

		mrc_Splitter = rcSplitter;
		mp_Grp1->RepositionVCon(rcCon1, bVisible);
		mp_Grp2->RepositionVCon(rcCon2, bVisible);
	}
	else
	{
		_ASSERTE(mp_Grp1 && mp_Grp2);
	}
}

// Разбиение в координатах DC (pixels)
void CVConGroup::CalcSplitRect(RECT rcNewCon, RECT& rcCon1, RECT& rcCon2, RECT& rcSplitter)
{
	rcCon1 = rcNewCon;
	rcCon2 = rcNewCon;
	rcSplitter = MakeRect(0,0);

	if (!this)
	{
		_ASSERTE(this);
		return;
	}

	// Заблокируем заранее
	CVConGuard VCon1(mp_Grp1 ? mp_Grp1->mp_Item : NULL);
	CVConGuard VCon2(mp_Grp2 ? mp_Grp2->mp_Item : NULL);

	if ((m_SplitType == RConStartArgs::eSplitNone) || !mp_Grp1 || !mp_Grp2)
	{
		_ASSERTE(mp_Grp1==NULL && mp_Grp2==NULL);
		_ASSERTE((m_SplitType != RConStartArgs::eSplitNone) && "Need no split");
		return;
	}

	UINT nSplit = max(1,min(mn_SplitPercent10,999));
	//UINT nPadSizeX = 0, nPadSizeY = 0;
	if (m_SplitType == RConStartArgs::eSplitHorz)
	{
		UINT nWidth = rcNewCon.right - rcNewCon.left;
		UINT nPadX = gpSet->nSplitWidth;
		if (nWidth >= nPadX)
			nWidth -= nPadX;
		else
			nPadX = 0;
		RECT rcScroll = gpConEmu->CalcMargins(CEM_SCROLL);
		_ASSERTE(rcScroll.left==0);
		if (rcScroll.right)
		{
			_ASSERTE(gpSet->isAlwaysShowScrollbar==1); // сюда должны попадать только при включенном постоянно скролле
			if (nWidth > (UINT)(rcScroll.right * 2))
				nWidth -= rcScroll.right * 2;
			else
				rcScroll.right = 0;
		}
		UINT nScreenWidth = (nWidth * nSplit / 1000);
		LONG nCellWidth = gpSetCls->FontWidth();
		LONG nCon2Width;
		if (nCellWidth > 0)
		{
			UINT nTotalCellCountX = nWidth / nCellWidth;
			UINT nCellCountX = (nScreenWidth + (nCellWidth/2)) / nCellWidth;
			if ((nTotalCellCountX >= 2) && (nCellCountX >= nTotalCellCountX))
			{
				_ASSERTE(FALSE && "Too small rect?");
				nCellCountX = nTotalCellCountX - 1;
			}
			nScreenWidth = nCellCountX * nCellWidth;
			nCon2Width = (nTotalCellCountX - nCellCountX) * nCellWidth;
			_ASSERTE(nCon2Width > 0);
		}
		else
		{
			_ASSERTE(nCellWidth > 0);
			nCon2Width = rcNewCon.right - (rcCon1.right+nPadX+rcScroll.right);
			_ASSERTE(nCon2Width > 0);
		}

		rcCon1 = MakeRect(rcNewCon.left, rcNewCon.top,
			max(rcNewCon.left + nScreenWidth,rcNewCon.right - nCon2Width - nPadX), rcNewCon.bottom);
		rcCon2 = MakeRect(rcNewCon.right - nCon2Width, rcNewCon.top, rcNewCon.right, rcNewCon.bottom);
		rcSplitter = MakeRect(rcCon1.right+1, rcCon1.top, rcCon2.left, rcCon2.bottom);
	}
	else
	{
		UINT nHeight = rcNewCon.bottom - rcNewCon.top;
		UINT nPadY = gpSet->nSplitHeight;
		if (nHeight >= nPadY)
			nHeight -= nPadY;
		else
			nPadY = 0;
		RECT rcScroll = gpConEmu->CalcMargins(CEM_SCROLL);
		_ASSERTE(rcScroll.top==0);
		if (rcScroll.bottom)
		{
			_ASSERTE(gpSet->isAlwaysShowScrollbar==1); // сюда должны попадать только при включенном постоянно скролле
			if (nHeight > (UINT)(rcScroll.bottom * 2))
				nHeight -= rcScroll.bottom * 2;
			else
				rcScroll.bottom = 0;
		}
		UINT nScreenHeight = (nHeight * nSplit / 1000);
		LONG nCellHeight = gpSetCls->FontHeight();
		LONG nCon2Height;
		if (nCellHeight > 0)
		{
			UINT nTotalCellCountY = nHeight / nCellHeight;
			UINT nCellCountY = (nScreenHeight + (nCellHeight/2)) / nCellHeight;
			if ((nTotalCellCountY >= 2) && (nCellCountY >= nTotalCellCountY))
			{
				_ASSERTE(FALSE && "Too small rect?");
				nCellCountY = nTotalCellCountY - 1;
			}
			nScreenHeight = nCellCountY * nCellHeight;
			nCon2Height = (nTotalCellCountY - nCellCountY) * nCellHeight;
			_ASSERTE(nCon2Height > 0);
		}
		else
		{
			_ASSERTE(nCellHeight > 0);
			nCon2Height = rcNewCon.bottom - (rcCon1.bottom+nPadY+rcScroll.bottom);
		}

		rcCon1 = MakeRect(rcNewCon.left, rcNewCon.top,
			rcNewCon.right, max(rcNewCon.top + nScreenHeight,rcNewCon.bottom - nCon2Height - nPadY));
		rcCon2 = MakeRect(rcCon1.left, rcNewCon.bottom - nCon2Height, rcNewCon.right, rcNewCon.bottom);
		rcSplitter = MakeRect(rcCon1.left, rcCon1.bottom, rcCon2.right, rcCon2.top);
	}
}

void CVConGroup::CalcSplitRootRect(RECT rcAll, RECT& rcCon, CVConGroup* pTarget /*= NULL*/)
{
	if (!this)
	{
		_ASSERTE(this);
		rcCon = rcAll;
		return;
	}

	if (!mp_Parent && !pTarget)
	{
		rcCon = rcAll;
		return;
	}

	RECT rc = rcAll;
	if (mp_Parent)
	{
		_ASSERTE(pTarget != this);
		mp_Parent->CalcSplitRootRect(rcAll, rc, this);
	}

	if (m_SplitType == RConStartArgs::eSplitNone)
	{
		_ASSERTE(mp_Grp1==NULL && mp_Grp2==NULL);
		rcCon = rc;
	}
	else
	{
		RECT rc1, rc2, rcSplitter;
		CalcSplitRect(rc, rc1, rc2, rcSplitter);

		_ASSERTE(pTarget == mp_Grp1 || pTarget == mp_Grp2);
		rcCon = (pTarget == mp_Grp2) ? rc2 : rc1;
	}
}

void CVConGroup::ShowAllVCon(int nShowCmd)
{
	if (!this)
	{
		_ASSERTE(this);
		return;
	}

	CVConGuard VConI(mp_Item);

	if (m_SplitType == RConStartArgs::eSplitNone)
	{
		_ASSERTE(mp_Grp1==NULL && mp_Grp2==NULL);
		if (VConI.VCon())
		{
			VConI->ShowView(nShowCmd);
		}
	}
	else if (mp_Grp1 && mp_Grp2)
	{
		mp_Grp1->ShowAllVCon(nShowCmd);
		mp_Grp2->ShowAllVCon(nShowCmd);
	}
	else
	{
		_ASSERTE(mp_Grp1 && mp_Grp2);
	}
}

void CVConGroup::MoveAllVCon(CVirtualConsole* pVConCurrent, RECT rcNewCon)
{
	CVConGroup* pGroup = GetRootOfVCon(pVConCurrent);
	if (!pGroup)
	{
		_ASSERTE(pGroup);
		return;
	}

	//bool lbPosChanged = false;
	//RECT rcCurCon = {};
	CVConGroup* pRoots[MAX_CONSOLE_COUNT+1] = {pGroup};
	CVConGuard VCon(pVConCurrent);

	WARNING("DoubleView и не только: Переделать. Ресайз должен жить в ConEmuChild/VConGroup!");

	for (size_t i = 0; i < countof(gp_VCon); i++)
	{
		CVConGuard VConI(gp_VCon[i]);
		if (!VConI.VCon())
			break;
		if (VConI.VCon() == pVConCurrent)
			continue;
		// Обрабатывать "по группам", а не "по консолям"
		bool bProcessed = false;
		CVConGroup* pCurGroup = GetRootOfVCon(VConI.VCon());
		size_t j = 0;
		while (pRoots[j])
		{
			if (pRoots[j] == pCurGroup)
			{
				bProcessed = true; break;
			}
			j++;
		}
		if (bProcessed)
			continue; // уже
		pRoots[j] = pCurGroup; // Запомним, однократно

		// Двигаем окошко DC
		pCurGroup->RepositionVCon(rcNewCon, false);

		//// Двигаем
		//HWND hWndDC = VConI->GetView();
		//if (hWndDC)
		//{
		//	WARNING("DoubleView и не только: Переделать. Ресайз должен жить в ConEmuChild!");
		//	GetWindowRect(hWndDC, &rcCurCon);
		//	MapWindowPoints(NULL, ghWnd, (LPPOINT)&rcCurCon, 2);
		//	// Тут нас интересует только X/Y
		//	lbPosChanged = memcmp(&rcCurCon, &rcNewCon, sizeof(POINT))!=0;

		//	if (lbPosChanged)
		//	{
		//		// Двигаем окошко DC
		//		SetWindowPos(hWndDC, NULL, rcNewCon.left, rcNewCon.top, 0,0, SWP_NOSIZE|SWP_NOZORDER);
		//	}
		//}
	}

	// Двигаем/ресайзим окошко DC
	pGroup->RepositionVCon(rcNewCon, true);

	//HWND hWndDC = pVConCurrent ? pVConCurrent->GetView() : NULL;
	//if (hWndDC)
	//{
	//	WARNING("DoubleView и не только: Переделать. Ресайз должен жить в ConEmuChild!");
	//	GetWindowRect(hWndDC, &rcCurCon);
	//	MapWindowPoints(NULL, ghWnd, (LPPOINT)&rcCurCon, 2);
	//	lbPosChanged = memcmp(&rcCurCon, &rcNewCon, sizeof(RECT))!=0;

	//	if (lbPosChanged)
	//	{
	//		// Двигаем/ресайзим окошко DC
	//		MoveWindow(hWndDC, rcNewCon.left, rcNewCon.top, rcNewCon.right - rcNewCon.left, rcNewCon.bottom - rcNewCon.top, 1);
	//		pVConCurrent->Invalidate();
	//	}
	//}
}

bool CVConGroup::isValid(CRealConsole* apRCon)
{
	if (!apRCon)
		return false;

	for (size_t i = 0; i < countof(gp_VCon); i++)
	{
		if (gp_VCon[i] && apRCon == gp_VCon[i]->RCon())
			return true;
	}

	return false;
}

bool CVConGroup::isValid(CVirtualConsole* apVCon)
{
	if (!apVCon)
		return false;

	for (size_t i = 0; i < countof(gp_VCon); i++)
	{
		if (apVCon == gp_VCon[i])
			return true;
	}

	return false;
}

bool CVConGroup::isVConExists(int nIdx)
{
	if (nIdx < 0 || nIdx >= (int)countof(gp_VCon))
		return false;
	return (gp_VCon[nIdx] != NULL);
}

bool CVConGroup::isVConHWND(HWND hChild, CVConGuard* rpVCon /*= NULL*/)
{
	CVConGuard VCon;

	if (hChild)
	{
		for (size_t i = 0; i < countof(gp_VCon); i++)
		{
			if (gp_VCon[i] && (gp_VCon[i]->GetView() == hChild))
			{
				VCon = gp_VCon[i];
				break;
			}
		}
	}

	if (rpVCon)
		*rpVCon = VCon.VCon();

	bool bFound = (VCon.VCon() != NULL);
	return bFound;
}

bool CVConGroup::isEditor()
{
	if (!gp_VActive) return false;

	return gp_VActive->RCon()->isEditor();
}

bool CVConGroup::isViewer()
{
	if (!gp_VActive) return false;

	return gp_VActive->RCon()->isViewer();
}

bool CVConGroup::isFar(bool abPluginRequired/*=false*/)
{
	if (!gp_VActive) return false;

	return gp_VActive->RCon()->isFar(abPluginRequired);
}

// Если ли фар где-то?
bool CVConGroup::isFarExist(CEFarWindowType anWindowType/*=fwt_Any*/, LPWSTR asName/*=NULL*/, CVConGuard* rpVCon/*=NULL*/)
{
	bool bFound = false, bLocked = false;
	CVConGuard VCon;

	if (rpVCon)
		*rpVCon = NULL;

	for (INT_PTR i = -1; !bFound && (i < (INT_PTR)countof(gp_VCon)); i++)
	{
		if (i == -1)
			VCon = gp_VActive;
		else
			VCon = gp_VCon[i];

		if (VCon.VCon())
		{
			// Это фар?
			CRealConsole* pRCon = VCon->RCon();
			if (pRCon && pRCon->isFar(anWindowType & fwt_PluginRequired))
			{
				// Ищем что-то конкретное?
				if (!(anWindowType & (fwt_TypeMask|fwt_Elevated|fwt_NonElevated|fwt_Modal|fwt_NonModal|fwt_ActivateFound)) && !(asName && *asName))
				{
					bFound = true;
					break;
				}

				if (!(anWindowType & (fwt_TypeMask|fwt_ActivateFound)) && !(asName && *asName))
				{
					CEFarWindowType t = pRCon->GetActiveTabType();

					// Этот Far Elevated?
					if ((anWindowType & fwt_Elevated) && !(t & fwt_Elevated))
						continue;
					// В табе устанавливается флаг fwt_Elevated
					// fwt_NonElevated используется только как аргумент поиска
					if ((anWindowType & fwt_NonElevated) && (t & fwt_Elevated))
						continue;

					// Модальное окно?
					WARNING("Нужно еще учитывать <модальность> заблокированным диалогом, или меню, или еще чем-либо!");
					if ((anWindowType & fwt_Modal) && !(t & fwt_Modal))
						continue;
					// В табе устанавливается флаг fwt_Modal
					// fwt_NonModal используется только как аргумент поиска
					if ((anWindowType & fwt_NonModal) && (t & fwt_Modal))
						continue;

					bFound = true;
					break;
				}
				else
				{
					// Нужны доп.проверки окон фара
					ConEmuTab tab;
					LPCWSTR pszNameOnly = asName ? PointToName(asName) : NULL;
					if (pszNameOnly)
					{
						// Обработаем как обратные (в PointToName), так и прямые слеши
						// Это может быть актуально при переходе на ошибку/гиперссылку
						LPCWSTR pszSlash = wcsrchr(pszNameOnly, L'/');
						if (pszSlash)
							pszNameOnly = pszSlash+1;
					}

					for (int j = 0; !bFound; j++)
					{
						if (!pRCon->GetTab(j, &tab))
							break;

						if ((tab.Type & fwt_TypeMask) != (anWindowType & fwt_TypeMask))
							continue;

						// Этот Far Elevated?
						if ((anWindowType & fwt_Elevated) && !(tab.Type & fwt_Elevated))
							continue;
						// В табе устанавливается флаг fwt_Elevated
						// fwt_NonElevated используется только как аргумент поиска
						if ((anWindowType & fwt_NonElevated) && (tab.Type & fwt_Elevated))
							continue;

						// Модальное окно?
						WARNING("Нужно еще учитывать <модальность> заблокированным диалогом, или меню, или еще чем-либо!");
						if ((anWindowType & fwt_Modal) && !(tab.Type & fwt_Modal))
							continue;
						// В табе устанавливается флаг fwt_Modal
						// fwt_NonModal используется только как аргумент поиска
						if ((anWindowType & fwt_NonModal) && (tab.Type & fwt_Modal))
							continue;

						// Если ищем конкретный редактор/вьювер
						if (asName && *asName)
						{
							if (lstrcmpi(tab.Name, asName) == 0)
							{
								bFound = true;
							}
							else if ((pszNameOnly != asName) && (lstrcmpi(PointToName(tab.Name), pszNameOnly) == 0))
							{
								bFound = true;
							}
						}
						else
						{
							bFound = true;
						}


						if (bFound)
						{
							if (anWindowType & fwt_ActivateFound)
							{
								if (pRCon->ActivateFarWindow(j))
								{
									gpConEmu->Activate(VCon.VCon());
									bLocked = false;
								}
								else
								{
									bLocked = true;
								}
							}

							break;
						}
					}
				}
			}
		}
	}

	// Нашли?
	if (bFound)
	{
		if (rpVCon)
		{
			*rpVCon = VCon.VCon();
			if (bLocked)
				bFound = false;
		}
	}

	return bFound;
}

// Возвращает индекс (0-based) активной консоли
int CVConGroup::GetActiveVCon(CVConGuard* pVCon /*= NULL*/, int* pAllCount /*= NULL*/)
{
	int nCount = 0, nFound = -1;

	for (size_t i = 0; i < countof(gp_VCon); i++)
	{
		if (gp_VCon[i])
		{
			nCount++;

			if (gp_VCon[i] == gp_VActive)
			{
				if (pVCon)
					*pVCon = gp_VCon[i];
				nFound = i;
			}
		}
	}

	_ASSERTE((gp_VActive!=NULL) == (nFound>=0));

	if (pAllCount)
		*pAllCount = nCount;

	return nFound;
}

// Возвращает индекс (0-based) консоли, или -1, если такой нет
int CVConGroup::GetVConIndex(CVirtualConsole* apVCon)
{
	if (!apVCon)
		return -1;
	for (size_t i = 0; i < countof(gp_VCon); i++)
	{
		if (gp_VCon[i] == apVCon)
			return i;
	}
	return -1;
}

bool CVConGroup::GetProgressInfo(short* pnProgress, BOOL* pbActiveHasProgress, BOOL* pbWasError, BOOL* pbWasIndeterminate)
{
	short nProgress = -1;
	short nUpdateProgress = gpUpd ? gpUpd->GetUpdateProgress() : -1;
	short n;
	BOOL bActiveHasProgress = FALSE;
	BOOL bWasError = FALSE;
	BOOL bWasIndeterminate = FALSE;
	int  nState = 0;

	if (gp_VActive)
	{
		BOOL lbNotFromTitle = FALSE;
		if (!isValid(gp_VActive))
		{
			_ASSERTE(isValid(gp_VActive));
		}
		else if ((nProgress = gp_VActive->RCon()->GetProgress(&nState, &lbNotFromTitle)) >= 0)
		{
			bWasError = (nState & 1) == 1;
			bWasIndeterminate = (nState & 2) == 2;
			//mn_Progress = max(nProgress, nUpdateProgress);
			bActiveHasProgress = TRUE;
			_ASSERTE(lbNotFromTitle==FALSE); // CRealConsole должен проценты добавлять в GetTitle сам
			//bNeedAddToTitle = lbNotFromTitle;
		}
	}

	if (!bActiveHasProgress && nUpdateProgress >= 0)
	{
		nProgress = max(nProgress, nUpdateProgress);
	}

	// нас интересует возможное наличие ошибки во всех остальных консолях
	for (size_t i = 0; i < countof(gp_VCon); i++)
	{
		if (gp_VCon[i])
		{
			int nCurState = 0;
			n = gp_VCon[i]->RCon()->GetProgress(&nCurState);

			if ((nCurState & 1) == 1)
				bWasError = TRUE;
			if ((nCurState & 2) == 2)
				bWasIndeterminate = TRUE;

			if (!bActiveHasProgress && n > nProgress)
				nProgress = n;
		}
	}

	if (pnProgress)
		*pnProgress = nProgress;
	if (pbActiveHasProgress)
		*pbActiveHasProgress = bActiveHasProgress;
	if (pbWasError)
		*pbWasError = bWasError;
	if (pbWasIndeterminate)
		*pbWasIndeterminate = bWasIndeterminate;

	return (nProgress >= 0);
}

void CVConGroup::OnDosAppStartStop(HWND hwnd, StartStopType sst, DWORD idChild)
{
	for (size_t i = 0; i < countof(gp_VCon); i++)
	{
		if (!gp_VCon[i]) continue;

		// Запускаемые через "-new_console" цепляются через CECMD_ATTACH2GUI, а не через WinEvent
		// 111211 - "-new_console" теперь передается в GUI и исполняется в нем
		if (gp_VCon[i]->RCon()->isDetached() || !gp_VCon[i]->RCon()->isServerCreated())
			continue;

		HWND hRConWnd = gp_VCon[i]->RCon()->ConWnd();
		if (hRConWnd == hwnd)
		{
			//StartStopType sst = (anEvent == EVENT_CONSOLE_START_APPLICATION) ? sst_App16Start : sst_App16Stop;
			gp_VCon[i]->RCon()->OnDosAppStartStop(sst, idChild);
			break;
		}
	}
}

void CVConGroup::InvalidateAll()
{
	for (size_t i = 0; i < countof(gp_VCon); i++)
	{
		CVConGuard VCon(gp_VCon[i]);

		if (VCon.VCon() && isVisible(VCon.VCon()))
			VCon.VCon()->Invalidate();
	}
}

void CVConGroup::UpdateWindowChild(CVirtualConsole* apVCon)
{
	if (apVCon)
	{
		if (apVCon->isVisible())
			UpdateWindow(apVCon->GetView());
	}
	else
	{
		for (size_t i = 0; i < countof(gp_VCon); i++)
		{
			if (gp_VCon[i] && gp_VCon[i]->isVisible())
				UpdateWindow(gp_VCon[i]->GetView());
		}
	}
}

void CVConGroup::RePaint()
{
	for (size_t i = 0; i < countof(gp_VCon); i++)
	{
		if (gp_VCon[i] && gp_VCon[i]->isVisible())
		{
			CVirtualConsole* pVCon = gp_VCon[i];
			CVConGuard guard(pVCon);
			HWND hView = pVCon->GetView();
			if (hView)
			{
				HDC hDc = GetDC(hView);
				//RECT rcClient = pVCon->GetDcClientRect();
				pVCon->PaintVCon(hDc/*, rcClient*/);
				ReleaseDC(ghWnd, hDc);
			}
		}
	}
}

void CVConGroup::Update(bool isForce /*= false*/)
{
	if (isForce)
	{
		for (size_t i = 0; i < countof(gp_VCon); i++)
		{
			if (gp_VCon[i])
				gp_VCon[i]->OnFontChanged();
		}
	}

	CVirtualConsole::ClearPartBrushes();

	if (gp_VActive)
	{
		gp_VActive->Update(isForce);
		//InvalidateAll();
	}
}

bool CVConGroup::isActive(CVirtualConsole* apVCon, bool abAllowGroup /*= true*/)
{
	if (!apVCon)
		return false;

	if (apVCon == gp_VActive)
		return true;

	if (abAllowGroup)
	{
		// DoubleView: когда будет группировка ввода - чтобы курсором мигать во всех консолях
		CVConGroup* pRoot = GetRootOfVCon(apVCon);
		CVConGroup* pActiveRoot = GetRootOfVCon(gp_VActive);
		if (pRoot && (pRoot == pActiveRoot))
			return true;
	}

	return false;
}

bool CVConGroup::isVisible(CVirtualConsole* apVCon)
{
	if (!apVCon)
		return false;

	if (apVCon == gp_VActive)
		return true;

	CVConGroup* pActiveRoot = GetRootOfVCon(gp_VActive);
	CVConGroup* pRoot = GetRootOfVCon(apVCon);
	if (pRoot && pActiveRoot && (pRoot == pActiveRoot))
		return true;

	return false;
}

bool CVConGroup::isConSelectMode()
{
	//TODO: По курсору, что-ли попробовать определять?
	//return gb_ConsoleSelectMode;
	if (gp_VActive)
		return gp_VActive->RCon()->isConSelectMode();

	return false;
}

bool CVConGroup::isInCreateRoot()
{
	CVConGuard VCon;
	for (size_t i = 0; i < countof(gp_VCon); i++)
	{
		if (VCon.Attach(gp_VCon[i]))
		{
			if (VCon->RCon()->InCreateRoot())
				return true;
		}
	}
	return false;
}

bool CVConGroup::isDetached()
{
	CVConGuard VCon;
	for (size_t i = 0; i < countof(gp_VCon); i++)
	{
		if (VCon.Attach(gp_VCon[i]))
		{
			if (VCon->RCon()->isDetached())
				return true;
		}
	}
	return false;
}

bool CVConGroup::isFilePanel(bool abPluginAllowed/*=false*/)
{
	if (!gp_VActive) return false;

	bool lbIsPanels = gp_VActive->RCon()->isFilePanel(abPluginAllowed);
	return lbIsPanels;
}

bool CVConGroup::isNtvdm(BOOL abCheckAllConsoles/*=FALSE*/)
{
	if (gp_VActive)
	{
		CVConGuard VCon(gp_VActive);
		CRealConsole* pRCon = VCon.VCon() ? VCon->RCon() : NULL;
		
		if (pRCon && pRCon->isNtvdm())
			return true;
	}

	for (size_t i = 0; i < countof(gp_VCon); i++)
	{
		CVConGuard VCon(gp_VCon[i]);
		if (VCon.VCon() != gp_VActive)
			continue;
		CRealConsole* pRCon = VCon.VCon() ? VCon->RCon() : NULL;
		if (!pRCon)
			continue;

		if (pRCon->isNtvdm())
			return true;
	}

	return false;
}

bool CVConGroup::isOurConsoleWindow(HWND hCon)
{
	if (!hCon)
		return false;
	
	for (size_t i = 0; i < countof(gp_VCon); i++)
	{
		CVConGuard VCon(gp_VCon[i]);
		CRealConsole* pRCon = VCon.VCon() ? VCon->RCon() : NULL;
		if (!pRCon)
			continue;

		if (pRCon->ConWnd() == hCon)
			return true;
	}
	
	return false;
}

// Вернуть true, если hAnyWnd это наше окно, консольное окно,
// или принадлежит дочернему GUI приложению, запущенному во вкладке ConEmu
bool CVConGroup::isOurWindow(HWND hAnyWnd)
{
	if (!hAnyWnd)
		return false;

	// ConEmu/Settings?
	if ((hAnyWnd == ghWnd) || (hAnyWnd == ghOpWnd))
		return true;

	DWORD nPID = 0;
	if (!GetWindowThreadProcessId(hAnyWnd, &nPID))
		return false;
	// Еще какие-то наши окна?
	if (nPID == GetCurrentProcessId())
		return true;

	for (size_t i = 0; i < countof(gp_VCon); i++)
	{
		CVConGuard VCon(gp_VCon[i]);
		CRealConsole* pRCon = VCon.VCon() ? VCon->RCon() : NULL;
		if (!pRCon)
			continue;

		if (pRCon->ConWnd() == hAnyWnd)
			return true;

		if ((pRCon->GuiWnd() == hAnyWnd) || (pRCon->GuiWndPID() == nPID))
			return true;
	}

	return false;
}

bool CVConGroup::isChildWindowVisible()
{
	for (size_t i = 0; i < countof(gp_VCon); i++)
	{
		CVConGuard VCon(gp_VCon[i]);
		if (VCon.VCon() && isVisible(VCon.VCon()))
		{
			CRealConsole* pRCon = VCon->RCon();
			if (!pRCon)
				continue;

			if (pRCon->GuiWnd() || pRCon->isPictureView())
				return true;
		}
	}

	return false;
}

// Заголовок окна для PictureView вообще может пользователем настраиваться, так что
// рассчитывать на него при определения "Просмотра" - нельзя
bool CVConGroup::isPictureView()
{
	bool lbRc = false;

	if (gpConEmu->hPictureView && (!IsWindow(gpConEmu->hPictureView) || !isFar()))
	{
		InvalidateAll();
		gpConEmu->hPictureView = NULL;
	}

	bool lbPrevPicView = (gpConEmu->hPictureView != NULL);

	HWND hPicView = NULL;

	for (size_t i = 0; !lbRc && i < countof(gp_VCon); i++)
	{
		CVirtualConsole* pVCon = gp_VCon[i];
		// Было isVisible, но некорректно блокировать другие сплиты, в которых PicView нету
		if (!pVCon || !isActive(pVCon) || !pVCon->RCon())
			continue;

		hPicView = pVCon->RCon()->isPictureView();

		lbRc = (hPicView != NULL);

		// Если вызывали Help (F1) - окошко PictureView прячется
		if (hPicView && !IsWindowVisible(hPicView))
		{
			lbRc = false;
			hPicView = NULL;
		}
	}

	gpConEmu->hPictureView = hPicView;

	if (gpConEmu->bPicViewSlideShow && !gpConEmu->hPictureView)
	{
		gpConEmu->bPicViewSlideShow = false;
	}

	if (lbRc && !lbPrevPicView)
	{
		GetWindowRect(ghWnd, &gpConEmu->mrc_WndPosOnPicView);
	}
	else if (!lbRc)
	{
		memset(&gpConEmu->mrc_WndPosOnPicView, 0, sizeof(gpConEmu->mrc_WndPosOnPicView));
	}

	return lbRc;
}

void CVConGroup::OnRConTimerCheck()
{
	CVConGuard VCon;
	CRealConsole* pRCon;

	for (size_t i = 0; i < countof(gp_VCon); i++)
	{
		if (!VCon.Attach(gp_VCon[i]))
			continue;
		pRCon = VCon->RCon();
		if (!pRCon)
			continue;
		pRCon->OnTimerCheck();
	}
}

// nIdx - 0 based
bool CVConGroup::GetVCon(int nIdx, CVConGuard* pVCon /*= NULL*/)
{
	bool bFound = false;

	if (nIdx < 0 || nIdx >= (int)countof(gp_VCon) || gp_VCon[nIdx] == NULL)
	{
		_ASSERTE(nIdx>=0 && nIdx<(int)countof(gp_VCon));
		if (pVCon)
			*pVCon = NULL;
	}
	else
	{
		if (pVCon)
			*pVCon = gp_VCon[nIdx];
		bFound = true;
	}

	return bFound;
}

bool CVConGroup::GetVConFromPoint(POINT ptScreen, CVConGuard* pVCon /*= NULL*/)
{
	bool bFound = false;
	CVConGuard VCon;

	for (size_t i = 0; i < countof(gp_VCon); i++)
	{
		VCon = gp_VCon[i];

		if (VCon.VCon() != NULL && VCon->isVisible())
		{
			
			HWND hView = VCon->GetView();
			HWND hBack = VCon->GetBack();
			if (hView && hBack)
			{
				// Check hBack, because it is larger and ScrollBar may lie outside of hView
				RECT rcView; GetWindowRect(hBack, &rcView);

				if (PtInRect(&rcView, ptScreen))
				{
					if (pVCon)
						*pVCon = VCon.VCon();
					bFound = true;
					break;
				}
			}
			else
			{
				_ASSERTE((hView && hBack) && "(hView = VCon->GetView()) != NULL");
			}
		}
	}

	return bFound;
}

//bool CVConGroup::ResizeConsoles(const RECT &rFrom, enum ConEmuRect tFrom)
//{
//	_ASSERTE(FALSE && "TODO");
//	return false;
//}
//
//bool CVConGroup::ResizeViews(bool bResizeRCon/*=true*/, WPARAM wParam/*=0*/, WORD newClientWidth/*=(WORD)-1*/, WORD newClientHeight/*=(WORD)-1*/)
//{
//	_ASSERTE(FALSE && "TODO");
//	return false;
//}

bool CVConGroup::CloseQuery(MArray<CVConGuard*>* rpPanes, bool* rbMsgConfirmed /*= NULL*/)
{
	int nEditors = 0, nProgress = 0, i, nConsoles = 0;
	if (rbMsgConfirmed)
		*rbMsgConfirmed = false;

	if (rpPanes)
		i = (int)rpPanes->size() - 1;
	else
		i = (int)countof(gp_VCon) - 1;

	while (i >= 0)
	{
		CRealConsole* pRCon = NULL;

		CVirtualConsole* pVCon;
		if (rpPanes)
			pVCon = (*rpPanes)[i--]->VCon();
		else
			pVCon = gp_VCon[i--];

		if (pVCon && ((pRCon = pVCon->RCon()) != NULL))
		{
			nConsoles++;

			// Прогрессы (копирование, удаление, и т.п.)
			if (pRCon->GetProgress(NULL) != -1)
				nProgress ++;

			// Несохраненные редакторы
			int n = pRCon->GetModifiedEditors();

			if (n)
				nEditors += n;
		}
	}

	if (nProgress || nEditors || (gpSet->isCloseConsoleConfirm && (nConsoles > 1)))
	{
		int nBtn = IDCANCEL;

		if (rpPanes)
		{
			wchar_t szText[360], *pszText;
			//wcscpy_c(szText, L"Close confirmation.\r\n\r\n");
			_wsprintf(szText, SKIPLEN(countof(szText)) L"About to close %u console%s.\r\n", nConsoles, (nConsoles>1)?L"s":L"");
			pszText = szText+_tcslen(szText);

			if (nProgress || nEditors)
			{
				*(pszText++) = L'\r'; *(pszText++) = L'\n'; *(pszText) = 0;

				if (nProgress)
				{
					_wsprintf(pszText, SKIPLEN(countof(szText)-(pszText-szText)) L"Incomplete operations: %i\r\n", nProgress);
					pszText += _tcslen(pszText);
				}
				if (nEditors)
				{
					_wsprintf(pszText, SKIPLEN(countof(szText)-(pszText-szText)) L"Unsaved editor windows: %i\r\n", nEditors);
					pszText += _tcslen(pszText);
				}
			}

			if (rpPanes)
				wcscat_c(szText, L"\r\nProceed with close group?");
			else
    			wcscat_c(szText,
					L"\r\nPress button <No> to close active console only\r\n"
					L"\r\nProceed with close ConEmu?");

			nBtn = MessageBoxW(ghWnd, szText, gpConEmu->GetDefaultTitle(), (rpPanes ? MB_OKCANCEL : MB_YESNOCANCEL)|MB_ICONEXCLAMATION);
		}
		else if (nConsoles == 1)
		{
			CVConGuard VCon(gp_VActive);
			if (VCon.VCon())
			{
				if (VCon->RCon()->isCloseConfirmed(NULL, true))
				{
					nBtn = IDOK;
				}
			}
		}
		else
		{
			nBtn = ConfirmCloseConsoles(nConsoles, nProgress, nEditors);
		}

		if (nBtn == IDNO)
		{
			CVConGuard VCon(gp_VActive);
			if (VCon.VCon())
			{
				VCon->RCon()->CloseConsole(false, false);
			}
			return false; // Don't close others!
		}
		else if ((nBtn != IDYES) && (nBtn != IDOK))
		{
			return false; // не закрывать
		}

		if (rbMsgConfirmed)
			*rbMsgConfirmed = true;
	}

	return true; // можно
}

bool CVConGroup::OnCloseQuery(bool* rbMsgConfirmed /*= NULL*/)
{
	if (!CloseQuery(NULL, rbMsgConfirmed))
	{
		gpConEmu->SetScClosePending(false);
		return false;
	}

	// Выставить флажок, чтобы ConEmu знал: "закрытие инициировано пользователем через крестик/меню"
	gpConEmu->SetScClosePending(true);

	#ifdef _DEBUG
	if (gbInMyAssertTrap)
		return false;
	#endif

	// Чтобы мог сработать таймер закрытия
	gpConEmu->OnRConStartedSuccess(NULL);
	return true; // можно
}

// true - found
bool CVConGroup::OnFlashWindow(DWORD nFlags, DWORD nCount, HWND hCon)
{
	if (!hCon) return false;

	bool lbFlashSimple = false;

	// Достало. Настройка полного отключения флэшинга
	if (gpSet->isDisableFarFlashing && gp_VActive->RCon()->GetFarPID(FALSE))
	{
		if (gpSet->isDisableFarFlashing == 1)
			return false;
		else
			lbFlashSimple = true;
	}
	else if (gpSet->isDisableAllFlashing)
	{
		if (gpSet->isDisableAllFlashing == 1)
			return false;
		else
			lbFlashSimple = true;
	}

	bool lbFound = false;

	for (size_t i = 0; i < countof(gp_VCon); i++)
	{
		if (!gp_VCon[i]) continue;

		if (gp_VCon[i]->RCon()->ConWnd() == hCon)
		{
			lbFound = true;

			FLASHWINFO fl = {sizeof(FLASHWINFO)};

			if (gpConEmu->isMeForeground())
			{
				if (gp_VCon[i] != gp_VActive)    // Только для неактивной консоли
				{
					fl.dwFlags = FLASHW_STOP; fl.hwnd = ghWnd;
					FlashWindowEx(&fl); // Чтобы мигание не накапливалось
					fl.uCount = 3; fl.dwFlags = lbFlashSimple ? FLASHW_ALL : FLASHW_TRAY; fl.hwnd = ghWnd;
					FlashWindowEx(&fl);
				}
			}
			else
			{
				if (lbFlashSimple)
				{
					fl.uCount = 3; fl.dwFlags = FLASHW_TRAY;
				}
				else
				{
					fl.dwFlags = FLASHW_ALL|FLASHW_TIMERNOFG;
				}

				fl.hwnd = ghWnd;
				FlashWindowEx(&fl); // Помигать в GUI
			}

			//fl.dwFlags = FLASHW_STOP; fl.hwnd = hCon; -- не требуется, т.к. это хучится
			//FlashWindowEx(&fl);
			break;
		}
	}

	return lbFound;
}

void CVConGroup::ExportEnvVarAll(CESERVER_REQ* pIn, CRealConsole* pExceptRCon)
{
	// Просто перебить заголовок на наши данные
	ExecutePrepareCmd(&pIn->hdr, CECMD_EXPORTVARS, pIn->hdr.cbSize);

	// и пробежаться по табам
	for (size_t i = 0; i < countof(gp_VCon); i++)
	{
		CVConGuard VCon(gp_VCon[i]);
		if (VCon.VCon() == NULL)
			continue;
		CRealConsole* pRCon = VCon->RCon();
		if (pRCon == pExceptRCon)
			continue;
		DWORD nSrvPID = pRCon->GetServerPID(true);
		if (!nSrvPID)
			continue;

		ConProcess* pP = NULL;
		int nCount = pRCon->GetProcesses(&pP);
		if (nCount && pP)
		{
			// Apply to all processes in this tab (console)
			for (int i = 0; i < nCount; i++)
			{
				if (pP[i].ProcessID != nSrvPID)
				{
					CESERVER_REQ* pOut = ExecuteHkCmd(pP[i].ProcessID, pIn, ghWnd);
					ExecuteFreeResult(pOut);
				}
			}
			free(pP);
		}
	}
}

void CVConGroup::OnUpdateGuiInfoMapping(ConEmuGuiMapping* apGuiInfo)
{
	for (size_t i = 0; i < countof(gp_VCon); i++)
	{
		CVConGuard VCon;
		if (VCon.Attach(gp_VCon[i]) && VCon->RCon())
		{
			VCon->RCon()->UpdateGuiInfoMapping(apGuiInfo);
		}
	}
}

void CVConGroup::OnPanelViewSettingsChanged()
{
	for (size_t i = 0; i < countof(gp_VCon); i++)
	{
		if (gp_VCon[i])
		{
			gp_VCon[i]->OnPanelViewSettingsChanged();
		}
	}
}

void CVConGroup::OnTaskbarSettingsChanged()
{
	CVConGuard VCon;
	for (size_t i = 0; i < countof(gp_VCon); i++)
	{
		if (VCon.Attach(gp_VCon[i]))
			VCon->OnTaskbarSettingsChanged();
	}
}

void CVConGroup::OnTaskbarCreated()
{
	CVConGuard VCon;
	HWND hGhost = NULL;
	for (size_t i = 0; i < countof(gp_VCon); i++)
	{
		if (VCon.Attach(gp_VCon[i]))
		{
			hGhost = VCon->GhostWnd();
			if (hGhost)
			{
				gpConEmu->OnGhostCreated(VCon.VCon(), hGhost);
			}
		}
	}
}

// true - если nPID запущен в одной из консолей
bool CVConGroup::isConsolePID(DWORD nPID)
{
	bool lbPidFound = false;

	for (size_t i = 0; i < countof(gp_VCon); i++)
	{
		if (gp_VCon[i] == NULL || gp_VCon[i]->RCon() == NULL)
			continue;

		DWORD dwFarPID    = gp_VCon[i]->RCon()->GetFarPID();
		DWORD dwActivePID = gp_VCon[i]->RCon()->GetActivePID();

		if (dwFarPID == nPID || dwActivePID == nPID)
		{
			lbPidFound = true;
			break;
		}
	}

	return lbPidFound;
}

bool CVConGroup::OnScClose()
{
	bool lbAllowed = false;
	int nConCount = 0, nDetachedCount = 0;
	bool lbProceed = false, lbMsgConfirmed = false;

	// lbMsgConfirmed - был ли показан диалог подтверждения, или юзер не включил эту опцию
	if (!OnCloseQuery(&lbMsgConfirmed))
		return false; // не закрывать

	bool bConfirmEach = (lbMsgConfirmed || !gpSet->isCloseConsoleConfirm) ? false : true;

	// Сохраним размер перед закрытием консолей, а то они могут напакостить и "вернуть" старый размер
	gpSet->SaveSizePosOnExit();
		
	for (int i = (int)(countof(gp_VCon)-1); i >= 0; i--)
	{
		if (gp_VCon[i] && gp_VCon[i]->RCon())
		{
			if (gp_VCon[i]->RCon()->isDetached())
			{
				nDetachedCount ++;
				continue;
			}

			nConCount ++;

			if (gp_VCon[i]->RCon()->ConWnd())
			{
				gp_VCon[i]->RCon()->CloseConsole(false, bConfirmEach);
			}
		}
	}

	if (nConCount == 0)
	{
		if (nDetachedCount > 0)
		{
			if (MessageBox(ghWnd, L"ConEmu is waiting for console attach.\nIt was started in 'Detached' mode.\nDo You want to cancel waiting?",
			              gpConEmu->GetDefaultTitle(), MB_YESNO|MB_ICONQUESTION) == IDYES)
        	{
				for (int i = (int)(countof(gp_VCon)-1); i >= 0; i--)
				{
					if (gp_VCon[i] && gp_VCon[i]->RCon())
					{
						if (gp_VCon[i]->RCon()->isDetached())
							gp_VCon[i]->RCon()->StopSignal();
					}
				}
        		lbAllowed = true;
			}
		}
		else
		{
			lbAllowed = true;
		}
	}

	// Закрыть окно, если просили
	if (lbAllowed && gpConEmu->isDestroyOnClose() && !nConCount && !nDetachedCount)
	{
		// Поэтому проверяем, и если никого не осталось, то по крестику - прибиваемся
		gpConEmu->Destroy();
	}

	return lbAllowed;
}

void CVConGroup::CloseGroup(CVirtualConsole* apVCon/*may be null*/)
{
	CVConGuard VCon(gp_VActive);
	if (!gp_VActive)
		return;

	CVConGroup* pActiveGroup = GetRootOfVCon(apVCon ? apVCon : gp_VActive);
	if (!pActiveGroup)
		return;

	MArray<CVConGuard*> Panes;
	int nCount = pActiveGroup->GetGroupPanes(Panes);
	if (nCount > 0)
	{
		if (CloseQuery(&Panes, NULL))
		{
			for (int i = (nCount - 1); i >= 0; i--)
			{
				CVirtualConsole* pVCon = Panes[i]->VCon();
				if (pVCon)
				{
					pVCon->RCon()->CloseConsole(false, false);
				}
			}
		}
	}

	FreePanesArray(Panes);
}

void CVConGroup::OnUpdateScrollInfo()
{
	for (size_t i = 0; i < countof(gp_VCon); i++)
	{
		CVConGuard VCon(gp_VCon[i]);
		if (!VCon.VCon())
			continue;
		if (!isActive(VCon.VCon()))
			continue;

		if (VCon->RCon())
			VCon->RCon()->UpdateScrollInfo();
	}
}

void CVConGroup::OnUpdateFarSettings()
{
	for (size_t i = 0; i < countof(gp_VCon); i++)
	{
		CVConGuard VCon(gp_VCon[i]);
		if (!VCon.VCon())
			continue;

		CRealConsole* pRCon = VCon->RCon();

		if (pRCon)
			pRCon->UpdateFarSettings();

		//DWORD dwFarPID = pRCon->GetFarPID();
		//if (!dwFarPID) continue;
		//pRCon->EnableComSpec(dwFarPID, gpSet->AutoBufferHeight);
	}
}

void CVConGroup::OnUpdateTextColorSettings(BOOL ChangeTextAttr /*= TRUE*/, BOOL ChangePopupAttr /*= TRUE*/)
{
	for (size_t i = 0; i < countof(gp_VCon); i++)
	{
		CVConGuard VCon(gp_VCon[i]);
		if (!VCon.VCon())
			continue;

		CRealConsole* pRCon = VCon->RCon();

		if (pRCon)
		{
			pRCon->UpdateTextColorSettings(ChangeTextAttr, ChangePopupAttr);
		}
	}
}

void CVConGroup::OnVConClosed(CVirtualConsole* apVCon)
{
	ShutdownGuiStep(L"OnVConClosed");

	bool bDbg1 = false, bDbg2 = false, bDbg3 = false, bDbg4 = false;
	int iDbg1 = -100, iDbg2 = -100, iDbg3 = -100;

	for (size_t i = 0; i < countof(gp_VCon); i++)
	{
		if (gp_VCon[i] == apVCon)
		{
			iDbg1 = i;

			// Сначала нужно обновить закладки, иначе в закрываемой консоли
			// может быть несколько вкладок и вместо активации другой консоли
			// будет попытка активировать другую вкладку закрываемой консоли
			//gpConEmu->mp_TabBar->Update(TRUE); -- а и не сможет он другую активировать, т.к. RCon вернет FALSE

			// Эта комбинация должна активировать предыдущую консоль (если активна текущая)
			if (gpSet->isTabRecent && apVCon == gp_VActive)
			{
				if (gpConEmu->GetVCon(1))
				{
					bDbg1 = true;
					gpConEmu->mp_TabBar->SwitchRollback();
					gpConEmu->mp_TabBar->SwitchNext();
					gpConEmu->mp_TabBar->SwitchCommit();
				}
			}

			// Теперь можно очистить переменную массива
			gp_VCon[i] = NULL;
			WARNING("Вообще-то это нужно бы в CriticalSection закрыть. Несколько консолей может одновременно закрыться");

			if (gp_VActive == apVCon)
			{
				bDbg2 = true;

				for (int j=(i-1); j>=0; j--)
				{
					if (gp_VCon[j])
					{
						iDbg2 = j;
						ConActivate(j);
						break;
					}
				}

				if (gp_VActive == apVCon)
				{
					bDbg3 = true;

					for (size_t j = (i+1); j < countof(gp_VCon); j++)
					{
						if (gp_VCon[j])
						{
							iDbg3 = j;
							ConActivate(j);
							break;
						}
					}
				}
			}

			for (size_t j = (i+1); j < countof(gp_VCon); j++)
			{
				gp_VCon[j-1] = gp_VCon[j];
			}

			gp_VCon[countof(gp_VCon)-1] = NULL;

			if (gp_VActive == apVCon)
			{
				bDbg4 = true;
				gp_VActive = NULL;
			}

			apVCon->Release();
			break;
		}
	}

	if (gp_VActive == apVCon)
	{
		// Сюда вообще попадать не должны, но на всякий случай, сбрасываем gp_VActive
		// Сюда можем попасть если не удалось создать активную консоль (облом с паролем, например)
		_ASSERTE((gp_VActive == NULL && gp_VCon[0] == NULL) || gb_CreatingActive);

		CVirtualConsole* pNewActive = NULL;
		for (size_t i = countof(gp_VCon); i--;)
		{
			if (gp_VCon[i] && gp_VCon[i] != apVCon)
			{
				pNewActive = gp_VCon[i];
				break;
			}
		}

		gp_VActive = pNewActive;
	}

	if (gp_VActive)
	{
		ShowActiveGroup(gp_VActive);
	}
}

void CVConGroup::OnUpdateProcessDisplay(HWND hInfo)
{
	if (!hInfo)
		return;

	SendDlgItemMessage(hInfo, lbProcesses, LB_RESETCONTENT, 0, 0);

	wchar_t temp[MAX_PATH];

	for (size_t j = 0; j < countof(gp_VCon); j++)
	{
		if (gp_VCon[j] == NULL) continue;

		ConProcess* pPrc = NULL;
		int nCount = gp_VCon[j]->RCon()->GetProcesses(&pPrc);

		if (pPrc && (nCount > 0))
		{
			for (int i=0; i<nCount; i++)
			{
				if (gp_VCon[j] == gp_VActive)
					_tcscpy(temp, _T("(*) "));
				else
					temp[0] = 0;

				swprintf(temp+_tcslen(temp), _T("[%i.%i] %s - PID:%i"),
						 j+1, i, pPrc[i].Name, pPrc[i].ProcessID);
				if (hInfo)
					SendDlgItemMessage(hInfo, lbProcesses, LB_ADDSTRING, 0, (LPARAM)temp);
			}

			SafeFree(pPrc);
		}
	}
}

// Возвращает HWND окна отрисовки
HWND CVConGroup::DoSrvCreated(DWORD nServerPID, HWND hWndCon, DWORD dwKeybLayout, DWORD& t1, DWORD& t2, DWORD& t3, int& iFound, HWND& hWndBack)
{
	HWND hWndDC = NULL;

	//gpConEmu->WinEventProc(NULL, EVENT_CONSOLE_START_APPLICATION, hWndCon, (LONG)nServerPID, 0, 0, 0);
	for (size_t i = 0; i < countof(gp_VCon); i++)
	{
		CVirtualConsole* pVCon = gp_VCon[i];
		CVConGuard guard(pVCon);
		CRealConsole* pRCon;
		if (pVCon && ((pRCon = pVCon->RCon()) != NULL) && pRCon->isServerCreated())
		{
			if (pRCon->GetServerPID() == nServerPID)
			{
				iFound = i;
				t1 = timeGetTime();
				
				pRCon->OnServerStarted(hWndCon, nServerPID, dwKeybLayout);
				
				t2 = timeGetTime();
				
				hWndDC = pVCon->GetView();
				hWndBack = pVCon->GetBack();

				t3 = timeGetTime();
				break;
			}
		}
	}

	return hWndDC;
}

bool CVConGroup::Activate(CVirtualConsole* apVCon)
{
	if (!isValid(apVCon))
		return false;

	bool lbRc = false;

	for (size_t i = 0; i < countof(gp_VCon); i++)
	{
		if (gp_VCon[i] == apVCon)
		{
			ConActivate(i);
			lbRc = (gp_VActive == apVCon);
			break;
		}
	}

	return lbRc;
}

void CVConGroup::MoveActiveTab(CVirtualConsole* apVCon, bool bLeftward)
{
	if (!apVCon)
		apVCon = gp_VActive;

	bool lbChanged = false;

	for (size_t i = 0; i < countof(gp_VCon); i++)
	{
		if (gp_VCon[i] == apVCon)
		{
			if (bLeftward)
			{
				if (i > 0)
				{
					CVirtualConsole* p = gp_VCon[i-1];
					gp_VCon[i-1] = gp_VCon[i];
					gp_VCon[i] = p;
					apVCon->RCon()->OnActivate(i-1, i);
					lbChanged = true;
				}
			}
			else
			{
				if ((i < (countof(gp_VCon))) && gp_VCon[i+1])
				{
					CVirtualConsole* p = gp_VCon[i+1];
					gp_VCon[i+1] = gp_VCon[i];
					gp_VCon[i] = p;
					apVCon->RCon()->OnActivate(i+1, i);
					lbChanged = true;
				}
			}
			break;
		}
	}

	UNREFERENCED_PARAMETER(lbChanged);
}

// 0 - based
int CVConGroup::ActiveConNum()
{
	int nActive = -1;

	if (gp_VActive)
	{
		for (size_t i = 0; i < countof(gp_VCon); i++)
		{
			if (gp_VCon[i] == gp_VActive)
			{
				nActive = i; break;
			}
		}
	}

	return nActive;
}

int CVConGroup::GetConCount(bool bNoDetached /*= false*/)
{
	int nCount = 0;

	for (size_t i = 0; i < countof(gp_VCon); i++)
	{
		if (!gp_VCon[i])
			break;

		if (bNoDetached)
		{
			_ASSERTE(gpConEmu->isMainThread()); // чтобы не морочится с блокировками
			if (gp_VCon[i]->RCon()->isDetached())
				continue;
		}

		nCount++;
	}

	return nCount;
}

BOOL CVConGroup::AttachRequested(HWND ahConWnd, const CESERVER_REQ_STARTSTOP* pStartStop, CESERVER_REQ_STARTSTOPRET* pRet)
{
	//CVirtualConsole* pVCon = NULL;
	CVConGuard VCon;
	bool bFound = false;
	_ASSERTE(pStartStop->dwPID!=0);

	if (gpSetCls->isAdvLogging)
	{
		size_t cchAll = 255 + _tcslen(pStartStop->sCmdLine) + _tcslen(pStartStop->sModuleName);
		wchar_t* pszLog = (wchar_t*)malloc(cchAll*sizeof(*pszLog));
		_wsprintf(pszLog, SKIPLEN(cchAll)
			L"Attach requested. HWND=x%08X, AID=%u, PID=%u, Sys=%u, Bit=%u, Adm=%u, Hkl=x%08X, Wnd={%u-%u}, Buf={%u-%u}, Max={%u-%u}, Cmd=%s",
			(DWORD)(DWORD_PTR)pStartStop->hWnd, pStartStop->dwAID, pStartStop->dwPID,
			pStartStop->nSubSystem, pStartStop->nImageBits, pStartStop->bUserIsAdmin, pStartStop->dwKeybLayout,
			pStartStop->sbi.srWindow.Right-pStartStop->sbi.srWindow.Left+1, pStartStop->sbi.srWindow.Bottom-pStartStop->sbi.srWindow.Top+1,
			pStartStop->sbi.dwSize.X, pStartStop->sbi.dwSize.Y,
			pStartStop->crMaxSize.X, pStartStop->crMaxSize.Y,
			pStartStop->sCmdLine[0] ? pStartStop->sCmdLine : pStartStop->sModuleName);
		gpConEmu->LogString(pszLog);
		free(pszLog);
	}

	// Может быть какой-то VCon ждет аттача?
	if (!bFound)
	{
		#ifdef _DEBUG
		if (pStartStop->dwAID == 0)
		{
			//Штатная ситуация, если аттач (запуск ConEmu.exe) инициируется из сервера
			wchar_t szDbg[128];
			//_ASSERTE(pStartStop->dwAID!=0);
			_wsprintf(szDbg, SKIPLEN(countof(szDbg)) L"Attach was requested from ServerPID=%u without dwAID\n", pStartStop->dwPID);
			DEBUGSTRATTACHERR(szDbg);
		}
		#endif

		for (size_t i = 0; i < countof(gp_VCon); i++)
		{
			VCon = gp_VCon[i];
			CRealConsole* pRCon = VCon.VCon() ? VCon->RCon() : NULL;
			if (pRCon != NULL)
			{
				if (pStartStop->dwAID)
				{
					if (pRCon->GetMonitorThreadID() == pStartStop->dwAID)
					{
						bFound = true;
						break;
					}
				}
				else
				{
					if (pRCon->GetServerPID() == pStartStop->dwPID)
					{
						bFound = true;
						break;
					}
				}
			}
		}
	}

	// Если по ИД не нашли - ищем любую "ожидающую" аттача
	if (!bFound)
	{
		for (size_t i = 0; i < countof(gp_VCon); i++)
		{
			VCon = gp_VCon[i];
			CRealConsole* pRCon = VCon.VCon() ? VCon->RCon() : NULL;
			if (pRCon != NULL)
			{
				if (pRCon->isDetached())
				{
					bFound = true;
					break;
				}
			}
		}
	}

	// Если не нашли - определим, можно ли добавить новую консоль?
	if (!bFound)
	{
		RConStartArgs* pArgs = new RConStartArgs;
		pArgs->bDetached = TRUE;
		pArgs->bBackgroundTab = pStartStop->bRunInBackgroundTab;
		_ASSERTE(pStartStop->sCmdLine[0]!=0);
		pArgs->pszSpecialCmd = lstrdup(pStartStop->sCmdLine);

		// т.к. это приходит из серверного потока - зовем в главном
		VCon = (CVirtualConsole*)SendMessage(ghWnd, gpConEmu->mn_MsgCreateCon, gpConEmu->mn_MsgCreateCon, (LPARAM)pArgs);
		if (VCon.VCon() && !isValid(VCon.VCon()))
		{
			_ASSERTE(FALSE && "MsgCreateCon failed");
			VCon = NULL;
		}
		//if ((pVCon = CreateCon(&args)) == NULL)
		//	return FALSE;
	}

	if (!bFound || !VCon.VCon())
	{
		//Assert? Report?
		return FALSE;
	}

	// Пытаемся подцепить консоль
	if (VCon->RCon()->AttachConemuC(ahConWnd, pStartStop->dwPID, pStartStop, pRet))
	{
		return FALSE;
	}

	// OK
	return TRUE;
}

CRealConsole* CVConGroup::AttachRequestedGui(LPCWSTR asAppFileName, DWORD anAppPID)
{
	CRealConsole* pRCon;

	for (size_t i = 0; i < countof(gp_VCon); i++)
	{
		if (gp_VCon[i] && (pRCon = gp_VCon[i]->RCon()) != NULL)
		{
			if (pRCon->GuiAppAttachAllowed(asAppFileName, anAppPID))
				return pRCon;
		}
	}
	
	return NULL;
}

// Вернуть общее количество процессов по всем консолям
DWORD CVConGroup::CheckProcesses()
{
	DWORD dwAllCount = 0;

	//mn_ActiveStatus &= ~CES_PROGRAMS;
	for (size_t j = 0; j < countof(gp_VCon); j++)
	{
		if (gp_VCon[j])
		{
			int nCount = gp_VCon[j]->RCon()->GetProcesses(NULL);
			if (nCount)
				dwAllCount += nCount;
		}
	}

	//if (gp_VActive) {
	//    mn_ActiveStatus |= gp_VActive->RCon()->GetProgramStatus();
	//}
	gpConEmu->m_ProcCount = dwAllCount;
	return dwAllCount;
}

bool CVConGroup::ConActivateNext(bool abNext)
{
	int nActive = ActiveConNum(), i, j, n1, n2, n3;

	for (j = 0; j <= 1; j++)
	{
		if (abNext)
		{
			if (j == 0)
			{
				n1 = nActive+1; n2 = countof(gp_VCon); n3 = 1;
			}
			else
			{
				n1 = 0; n2 = nActive; n3 = 1;
			}

			if (n1>=n2) continue;
		}
		else
		{
			if (j == 0)
			{
				n1 = nActive-1; n2 = -1; n3 = -1;
			}
			else
			{
				n1 = countof(gp_VCon)-1; n2 = nActive; n3 = -1;
			}

			if (n1<=n2) continue;
		}

		for (i = n1; i != n2 && i >= 0 && i < (int)countof(gp_VCon); i+=n3)
		{
			if (gp_VCon[i])
			{
				return ConActivate(i);
			}
		}
	}

	return false;
}

int CVConGroup::GetGroupPanes(MArray<CVConGuard*> &rPanes)
{
	if (!this)
	{
		_ASSERTE(this);
		return 0;
	}

	int nAdd = 0;

	_ASSERTE((m_SplitType==RConStartArgs::eSplitNone && mp_Item!=NULL) || (m_SplitType!=RConStartArgs::eSplitNone && (mp_Grp1 || mp_Grp2)));

	if (m_SplitType==RConStartArgs::eSplitNone)
	{
		if (mp_Item)
		{
			CVConGuard* pVConG = new CVConGuard(mp_Item);
			rPanes.push_back(pVConG);
			nAdd++;
		}
	}
	else
	{
		if (mp_Grp1)
			nAdd += mp_Grp1->GetGroupPanes(rPanes);
		if (mp_Grp2)
			nAdd += mp_Grp2->GetGroupPanes(rPanes);
	}

	return nAdd;
}

void CVConGroup::FreePanesArray(MArray<CVConGuard*> &rPanes)
{
	for (int i = rPanes.size(); i--;)
	{
		CVConGuard* p = rPanes[i];
		rPanes.erase(i);
		SafeDelete(p);
	}
	_ASSERTE(rPanes.size()==0);
	rPanes.clear();
}

bool CVConGroup::PaneActivateNext(bool abNext)
{
	CVConGuard VCon(gp_VActive);
	if (!gp_VActive)
		return false;

	bool bOk = false;
	CVirtualConsole* pVCon = VCon.VCon();
	CVConGroup* pActiveGroup = GetRootOfVCon(gp_VActive);
	MArray<CVConGuard*> Panes;
	
	int nCount = pActiveGroup->GetGroupPanes(Panes);
	
	if (nCount > 1)
	{
		CVirtualConsole* pVNext = NULL;

		if (abNext)
		{
			for (int i = 0; i < nCount; i++)
			{
				if (Panes[i]->VCon() == pVCon)
				{
					pVNext = Panes[((i+1) < nCount) ? (i+1) : 0]->VCon();
					break;
				}
			}
		}
		else
		{
			for (int i = nCount; i--;)
			{
				if (Panes[i]->VCon() == pVCon)
				{
					pVNext = Panes[i ? (i-1) : (nCount-1)]->VCon();
					break;
				}
			}
		}

		
		if (pVNext)
		{
			bOk = Activate(pVNext);
		}
	}

	FreePanesArray(Panes);
	return bOk;
}

void CVConGroup::ShowActiveGroup(CVirtualConsole* pOldActive)
{
	CVConGuard VCon(gp_VActive);
	if (!gp_VActive)
		return;

	CVConGroup* pActiveGroup = GetRootOfVCon(gp_VActive);
	if (pActiveGroup && pActiveGroup->mb_ResizeFlag)
	{
		SyncConsoleToWindow();
		// Отресайзить, могла измениться конфигурация
		RECT mainClient = gpConEmu->CalcRect(CER_MAINCLIENT, gp_VActive);
		CVConGroup::ReSizePanes(mainClient);
	}

	// Showing...
	CVConGroup* pOldGroup = (pOldActive && (pOldActive != gp_VActive)) ? GetRootOfVCon(pOldActive) : NULL;

	// Теперь можно показать активную
	pActiveGroup->ShowAllVCon(SW_SHOW);
	// и спрятать деактивированную
	if (pOldGroup && (pOldGroup != pActiveGroup))
		pOldGroup->ShowAllVCon(SW_HIDE);

	InvalidateAll();
}

// nCon - zero-based index of console
bool CVConGroup::ConActivate(int nCon)
{
	FLASHWINFO fl = {sizeof(FLASHWINFO)}; fl.dwFlags = FLASHW_STOP; fl.hwnd = ghWnd;
	FlashWindowEx(&fl); // При многократных созданиях мигать начинает...

	if (nCon >= 0 && nCon < (int)countof(gp_VCon))
	{
		CVConGuard VCon(gp_VCon[nCon]);
		CVirtualConsole* pVCon = VCon.VCon();

		if (pVCon == NULL)
		{
			if (gpSet->isMultiAutoCreate)
			{
				// Создать новую default-консоль
				gpConEmu->RecreateAction(cra_CreateTab/*FALSE*/, FALSE, FALSE);
				return true; // создана новая консоль
			}

			return false; // консоль с этим номером не была создана!
		}

		if (pVCon == gp_VActive)
		{
			// Итерация табов
			int nTabCount;
			CRealConsole *pRCon;

			// При последовательном нажатии "Win+<Number>" - крутить табы активной консоли
			if (gpSet->isMultiIterate
			        && ((pRCon = gp_VActive->RCon()) != NULL)
			        && ((nTabCount = pRCon->GetTabCount())>1))
			{
				int nActive = pRCon->GetActiveTab()+1;

				if (nActive >= nTabCount)
					nActive = 0;

				if (pRCon->CanActivateFarWindow(nActive))
					pRCon->ActivateFarWindow(nActive);
			}

			return true; // уже
		}

		bool lbSizeOK = true;
		int nOldConNum = ActiveConNum();

		CVirtualConsole* pOldActive = gp_VActive;

		// Спрятать PictureView, или еще чего...
		if (gp_VActive && gp_VActive->RCon())
		{
			gp_VActive->RCon()->OnDeactivate(nCon);
		}

		// ПЕРЕД переключением на новую консоль - обновить ее размеры
		if (pVCon)
		{
			//int nOldConWidth = gp_VActive->RCon()->TextWidth();
			//int nOldConHeight = gp_VActive->RCon()->TextHeight();
			int nOldConWidth = pVCon->RCon()->TextWidth();
			int nOldConHeight = pVCon->RCon()->TextHeight();
			//int nNewConWidth = pVCon->RCon()->TextWidth();
			//int nNewConHeight = pVCon->RCon()->TextHeight();
			//RECT rcMainClient = gpConEmu->CalcRect(CER_MAINCLIENT, pVCon);
			RECT rcNewCon = gpConEmu->CalcRect(CER_CONSOLE_CUR, pVCon);
			int nNewConWidth = rcNewCon.right;
			int nNewConHeight = rcNewCon.bottom;

			wchar_t szInfo[128];
			_wsprintf(szInfo, SKIPLEN(countof(szInfo)) L"Activating con #%u, OldSize={%u,%u}, NewSize={%u,%u}",
				nCon, nOldConWidth, nOldConHeight, nNewConWidth, nNewConHeight);
			if (gpSetCls->isAdvLogging)
			{
				pVCon->RCon()->LogString(szInfo);
			}

			if (nOldConWidth != nNewConWidth || nOldConHeight != nNewConHeight)
			{
				lbSizeOK = pVCon->RCon()->SetConsoleSize(nNewConWidth,nNewConHeight);
			}

			// И поправить размеры VCon/Back
			RECT rcWork = {};
			rcWork = gpConEmu->CalcRect(CER_WORKSPACE, pVCon);
			CVConGroup::MoveAllVCon(pVCon, rcWork);
		}

		gp_VActive = pVCon;
		pVCon->RCon()->OnActivate(nCon, nOldConNum);

		if (!lbSizeOK)
			SyncWindowToConsole(); // -- функция пустая, игнорируется

		ShowActiveGroup(pOldActive);
	}

	return false;
}

bool CVConGroup::InCreateGroup()
{
	return gb_InCreateGroup;
}

void CVConGroup::OnCreateGroupBegin()
{
	UINT nFreeCell = 0;
	for (size_t i = 0; i < countof(gp_VCon); i++)
	{
		if (gp_VCon[i] == NULL)
		{
			nFreeCell = i;
			break;
		}
	}
	gn_CreateGroupStartVConIdx = nFreeCell;
	gb_InCreateGroup = true;
}

void CVConGroup::OnCreateGroupEnd()
{
	gn_CreateGroupStartVConIdx = 0;
	gb_InCreateGroup = false;

	// А вот теперь можно начинать запускать процессы
	gpConEmu->mp_RunQueue->ProcessRunQueue(true);
}

CVirtualConsole* CVConGroup::CreateCon(RConStartArgs *args, bool abAllowScripts /*= false*/, bool abForceCurConsole /*= false*/)
{
	_ASSERTE(args!=NULL);
	if (!gpConEmu->isMainThread())
	{
		// Создание VCon в фоновых потоках не допускается, т.к. здесь создаются HWND
		MBoxAssert(gpConEmu->isMainThread());
		return NULL;
	}

	CVirtualConsole* pVCon = NULL;

	// When no command specified - choose default one. Now!
	if (!args->bDetached && (!args->pszSpecialCmd || !*args->pszSpecialCmd))
	{
		_ASSERTE(args->pszSpecialCmd==NULL);

		args->pszSpecialCmd = lstrdup(gpSet->GetCmd());

		_ASSERTE(args->pszSpecialCmd && *args->pszSpecialCmd);
	}

	if (args->pszSpecialCmd)
	{
		args->ProcessNewConArg(abForceCurConsole);
	}

	if (gpConEmu->mp_Inside && gpConEmu->mp_Inside->m_InsideIntegration && gpConEmu->mp_Inside->mb_InsideIntegrationShift)
	{
		gpConEmu->mp_Inside->mb_InsideIntegrationShift = false;
		args->bRunAsAdministrator = true;
	}

	//wchar_t* pszScript = NULL; //, szScript[MAX_PATH];

	_ASSERTE(args->pszSpecialCmd!=NULL);

	if (!args->bDetached
		&& args->pszSpecialCmd
		&& (*args->pszSpecialCmd == CmdFilePrefix
			|| *args->pszSpecialCmd == DropLnkPrefix
			|| *args->pszSpecialCmd == TaskBracketLeft))
	{
		if (!abAllowScripts)
		{
			DisplayLastError(L"Console script are not supported here!", -1);
			return NULL;
		}

		// В качестве "команды" указан "пакетный файл" или "группа команд" одновременного запуска нескольких консолей
		wchar_t* pszDataW = gpConEmu->LoadConsoleBatch(args->pszSpecialCmd, &args->pszStartupDir);
		if (!pszDataW)
			return NULL;

		// GO
		pVCon = gpConEmu->CreateConGroup(pszDataW, args->bRunAsAdministrator, NULL/*ignored when 'args' specified*/, args);

		SafeFree(pszDataW);
		return pVCon;
	}

	// Если на ярлык ConEmu наброшен другой ярлык или программа

	// Ok, Теперь смотрим свободную ячейку в gp_VCon и запускаемся
	for (size_t i = 0; i < countof(gp_VCon); i++)
	{
		if (gp_VCon[i])
		{
			CVConGuard VCon(gp_VCon[i]);
			CRealConsole* pRCon = VCon.VCon() ? VCon->RCon() : NULL;
			if (pRCon && !pRCon->InCreateRoot() && pRCon->isDetached())
			{
				// isDetached() means, that ConEmu.exe was started with "/detached" flag
				// so, it is safe to close "dummy" console, that was created on GUI startup
				pRCon->CloseConsole(false, false);
			}
		}

		if (!gp_VCon[i])
		{
			CVirtualConsole* pOldActive = gp_VActive;
			gb_CreatingActive = true;
			pVCon = CVConGroup::CreateVCon(args, gp_VCon[i]);
			gb_CreatingActive = false;

			BOOL lbInBackground = args->bBackgroundTab && (pOldActive != NULL) && !args->eSplit;

			if (pVCon)
			{
				if (!lbInBackground && pOldActive && pOldActive->RCon())
				{
					pOldActive->RCon()->OnDeactivate(i);
				}

				_ASSERTE(gp_VCon[i] == pVCon);
				gp_VCon[i] = pVCon;
				
				if (!lbInBackground)
				{
					gp_VActive = pVCon;
				}
				else
				{
					_ASSERTE(gp_VActive==pOldActive);
				}
				
				pVCon->InitGhost();

				if (!lbInBackground)
				{
					pVCon->RCon()->OnActivate(i, ActiveConNum());

					//mn_ActiveCon = i;
					//Update(true);

					ShowActiveGroup(pOldActive);
					//TODO("DoubleView: показать на неактивной?");
					//// Теперь можно показать активную
					//gp_VActive->ShowView(SW_SHOW);
					////ShowWindow(gp_VActive->GetView(), SW_SHOW);
					//// и спрятать деактивированную
					//if (pOldActive && (pOldActive != gp_VActive) && !pOldActive->isVisible())
					//	pOldActive->ShowView(SW_HIDE);
					//	//ShowWindow(pOldActive->GetView(), SW_HIDE);
				}
			}

			break;
		}
	}

	return pVCon;
}

HRGN CVConGroup::GetExclusionRgn(bool abTestOnly/*=false*/)
{
	HRGN hExclusion = NULL;
	int iComb = 0;

	// DoubleView: Если видимы несколько консолей - нужно совместить регионы

	for (size_t i = 0; i < countof(gp_VCon); i++)
	{
		CVConGuard VCon(gp_VCon[i]);
		if (VCon.VCon() && VCon->isVisible())
		{
			HRGN hVCon = VCon->GetExclusionRgn(abTestOnly);

			if (abTestOnly && hVCon)
			{

				_ASSERTE(hVCon == (HRGN)1);
				return (HRGN)1;
			}

			if (hVCon)
			{
				if (!hExclusion)
				{
					hExclusion = hVCon; // Первый (или единственный)
				}
				else
				{
					iComb = CombineRgn(hExclusion, hExclusion, hVCon, RGN_OR);
					_ASSERTE(iComb != ERROR);
					DeleteObject(hVCon);
				}
			}
		}
	}

	return hExclusion;
}

RECT CVConGroup::CalcRect(enum ConEmuRect tWhat, RECT rFrom, enum ConEmuRect tFrom, CVirtualConsole* pVCon, enum ConEmuMargins tTabAction/*=CEM_TAB*/)
{
	RECT rc = rFrom;
	//RECT rcShift = MakeRect(0,0);

	CVConGuard VCon(pVCon);
	if (!pVCon && (GetActiveVCon(&VCon) >= 0))
	{
		pVCon = VCon.VCon();
	}

	CVConGroup* pGroup = pVCon ? ((CVConGroup*)pVCon->mp_Group) : NULL;

	// Теперь rc должен соответствовать CER_MAINCLIENT/CER_BACK
	RECT rcAddShift = MakeRect(0,0);

	if ((tWhat == CER_DC) && (tFrom != CER_CONSOLE_CUR))
	{
		_ASSERTE(pVCon!=NULL);
		WARNING("warning: DoubleView - тут нужно допиливать");
		RECT rcCalcBack = rFrom;
		// нужно посчитать дополнительные сдвиги
		if (tFrom != CER_BACK)
		{
			_ASSERTE(tFrom==CER_BACK);
			rcCalcBack = gpConEmu->CalcRect(CER_BACK, pVCon);
		}
		//--// расчетный НЕ ДОЛЖЕН быть меньше переданного
		//#ifdef MSGLOGGER
		//_ASSERTE((rcCalcDC.right - rcCalcDC.left)>=(prDC->right - prDC->left));
		//_ASSERTE((rcCalcDC.bottom - rcCalcDC.top)>=(prDC->bottom - prDC->top));
		//#endif
		RECT rcCalcCon = rFrom;
		if (tFrom != CER_CONSOLE_CUR)
		{
			rcCalcCon = CalcRect(CER_CONSOLE_CUR, rcCalcBack, CER_BACK, pVCon);
		}
		// Расчетное DC (размер)
		_ASSERTE(rcCalcCon.left==0 && rcCalcCon.top==0);
		RECT rcCalcDC = MakeRect(0,0,rcCalcCon.right*gpSetCls->FontWidth(), rcCalcCon.bottom*gpSetCls->FontHeight());

		RECT rcScrollPad = gpConEmu->CalcMargins(CEM_SCROLL|CEM_PAD);
		gpConEmu->AddMargins(rcCalcBack, rcScrollPad, FALSE);

		int nDeltaX = (rcCalcBack.right - rcCalcBack.left) - (rcCalcDC.right - rcCalcDC.left);
		int nDeltaY = (rcCalcBack.bottom - rcCalcBack.top) - (rcCalcDC.bottom - rcCalcDC.top);

		// Теперь сдвиги
		if (gpSet->isTryToCenter && (gpConEmu->isZoomed() || gpConEmu->isFullScreen() || gpSet->isQuakeStyle))
		{
			// считаем доп.сдвиги. ТОЧНО
			if (nDeltaX > 0)
			{
				rcAddShift.left = nDeltaX >> 1;
				rcAddShift.right = nDeltaX - rcAddShift.left;
			}
		}
		else
		{
			if (nDeltaX > 0)
				rcAddShift.right = nDeltaX;
		}

		if (gpSet->isTryToCenter && (gpConEmu->isZoomed() || gpConEmu->isFullScreen()))
		{
			if (nDeltaY > 0)
			{
				rcAddShift.top = nDeltaY >> 1;
				rcAddShift.bottom = nDeltaY - rcAddShift.top;
			}
		}
		else
		{
			if (nDeltaY > 0)
				rcAddShift.bottom = nDeltaY;
		}
	}
	
	switch (tWhat)
	{
		case CER_BACK: // switch (tWhat)
		{
			TODO("DoubleView");
			RECT rcShift = gpConEmu->CalcMargins(tTabAction|CEM_CLIENT_MARGINS);
			CConEmuMain::AddMargins(rc, rcShift);
		} break;
		case CER_SCROLL: // switch (tWhat)
		{
			RECT rcShift = gpConEmu->CalcMargins(tTabAction);
			CConEmuMain::AddMargins(rc, rcShift);
			rc.left = rc.right - GetSystemMetrics(SM_CXVSCROLL);
			return rc; // Иначе внизу еще будет коррекция по DC (rcAddShift)
		} break;
		case CER_DC: // switch (tWhat)
		case CER_CONSOLE_ALL: // switch (tWhat)
		case CER_CONSOLE_CUR: // switch (tWhat)
		case CER_CONSOLE_NTVDMOFF: // switch (tWhat)
		{
			_ASSERTE(tWhat!=CER_DC || (tFrom==CER_BACK || tFrom==CER_CONSOLE_CUR)); // CER_DC должен считаться от CER_BACK

			if (tFrom == CER_MAINCLIENT)
			{
				// Учесть высоту закладок (табов)
				RECT rcShift = gpConEmu->CalcMargins(tTabAction|CEM_CLIENT_MARGINS);
				CConEmuMain::AddMargins(rc, rcShift);
			}
			else if (tFrom == CER_BACK || tFrom == CER_WORKSPACE)
			{
				// -- отрезание полосы прокрутки ПОСЛЕ разбиения
				//rcShift = gpConEmu->CalcMargins(CEM_SCROLL);
				//CConEmuMain::AddMargins(rc, rcShift);
			}
			else
			{
				// Другие значения - не допускаются
				_ASSERTE(tFrom == CER_MAINCLIENT);
			}

			// Теперь обработка SplitScreen/DoubleView/....
			RECT rcAll = rc;
			if (pGroup && (tWhat != CER_CONSOLE_ALL)
				&& (tFrom == CER_MAINCLIENT || tFrom == CER_WORKSPACE))
			{
				//
				pGroup->CalcSplitRootRect(rcAll, rc);
			}

			// Коррекция отступов
			if (tFrom == CER_MAINCLIENT || tFrom == CER_BACK || tFrom == CER_WORKSPACE)
			{
				// Прокрутка. Пока только справа (планируется и внизу)
				RECT rcShift = gpConEmu->CalcMargins(CEM_SCROLL);
				CConEmuMain::AddMargins(rc, rcShift);
			}

			TODO("Вообще, для CER_CONSOLE_ALL CEM_PAD считать не нужно, но т.к. пока используется SetAllConsoleWindowsSize - оставим");
			if (tWhat == CER_DC || tWhat == CER_CONSOLE_CUR || tWhat == CER_CONSOLE_ALL)
			{
				// Pad. Отступ от всех краев (gpSet->nCenterConsolePad)
				RECT rcShift = gpConEmu->CalcMargins(CEM_PAD);
				CConEmuMain::AddMargins(rc, rcShift);
			}

			//// Для корректного деления на размер знакоместа...
			//         if (gpSetCls->FontWidth()==0 || gpSetCls->FontHeight()==0)
			//             pVCon->InitDC(false, true); // инициализировать ширину шрифта по умолчанию
			//rc.right ++;
			//int nShift = (gpSetCls->FontWidth() - 1) / 2; if (nShift < 1) nShift = 1;
			//rc.right += nShift;
			// Если есть вкладки
			//if (rcShift.top || rcShift.bottom || )
			//nShift = (gpSetCls->FontWidth() - 1) / 2; if (nShift < 1) nShift = 1;

			// Если активен NTVDM.
			TODO("Вообще это нужно расширить. Размер может менять и любое консольное приложение.");
			if (tWhat != CER_CONSOLE_NTVDMOFF && pVCon && pVCon->RCon() && pVCon->RCon()->isNtvdm())
			{
				// NTVDM устанавливает ВЫСОТУ экранного буфера... в 25/28/43/50 строк
				// путем округления текущей высоты (то есть если до запуска 16bit
				// было 27 строк, то скорее всего будет установлена высота в 28 строк)
				RECT rc1 = MakeRect(pVCon->TextWidth*gpSetCls->FontWidth(), pVCon->TextHeight*gpSetCls->FontHeight());

				//gpSet->ntvdmHeight /* pVCon->TextHeight */ * gpSetCls->FontHeight());
				if (rc1.bottom > (rc.bottom - rc.top))
					rc1.bottom = (rc.bottom - rc.top); // Если размер вылез за текущий - обрежем снизу :(

				int nS = rc.right - rc.left - rc1.right;

				RECT rcShift = {0,0,0,0};

				if (nS>=0)
				{
					rcShift.left = nS / 2;
					rcShift.right = nS - rcShift.left;
				}
				else
				{
					rcShift.left = 0;
					rcShift.right = -nS;
				}

				nS = rc.bottom - rc.top - rc1.bottom;

				if (nS>=0)
				{
					rcShift.top = nS / 2;
					rcShift.bottom = nS - rcShift.top;
				}
				else
				{
					rcShift.top = 0;
					rcShift.bottom = -nS;
				}

				CConEmuMain::AddMargins(rc, rcShift);
			}

			// Если нужен размер консоли в символах сразу делим и выходим
			if (tWhat == CER_CONSOLE_ALL || tWhat == CER_CONSOLE_CUR || tWhat == CER_CONSOLE_NTVDMOFF)
			{
				//2009-07-09 - ClientToConsole использовать нельзя, т.к. после его
				//  приближений высота может получиться больше Ideal, а ширина - меньше
				//120822 - было "(rc.right - rc.left + 1)"
				int nW = (rc.right - rc.left) / gpSetCls->FontWidth();
				int nH = (rc.bottom - rc.top) / gpSetCls->FontHeight();
				rc.left = 0; rc.top = 0; rc.right = nW; rc.bottom = nH;

				//2010-01-19
				if (gpSet->isFontAutoSize)
				{
					if (gpConEmu->wndWidth && rc.right > (LONG)gpConEmu->wndWidth)
						rc.right = gpConEmu->wndWidth;

					if (gpConEmu->wndHeight && rc.bottom > (LONG)gpConEmu->wndHeight)
						rc.bottom = gpConEmu->wndHeight;
				}

				#ifdef _DEBUG
				_ASSERTE(rc.bottom>=MIN_CON_HEIGHT);
				#endif

				// Возможно, что в RealConsole выставлен большой шрифт, который помешает установке этого размера
				if (pVCon)
				{
					CRealConsole* pRCon = pVCon->RCon();

					if (pRCon)
					{
						COORD crMaxConSize = {0,0};

						// Даже если открыто несколько паналей - не разрешим устанавливать
						// в общей сложности больший размер, т.к. при закрытии одной панели
						// консоль слелит (превысит максимально допустимый размер)
						if (pRCon->GetMaxConSize(&crMaxConSize))
						{
							if (rc.right > crMaxConSize.X)
								rc.right = crMaxConSize.X;

							if (rc.bottom > crMaxConSize.Y)
								rc.bottom = crMaxConSize.Y;
						}
					}
				}
			}
			else
			{
				// корректировка, центрирование
				CConEmuMain::AddMargins(rc, rcAddShift);
			}
		}
		break;

	default:
		_ASSERTE(FALSE && "No supported value");
		break;
	}

	return rc;
}

void CVConGroup::CalcSplitConSize(COORD size, COORD& sz1, COORD& sz2)
{
	sz1 = size; sz2 = size;

	RECT rcCon = MakeRect(size.X, size.Y);
	RECT rcPixels = gpConEmu->CalcRect(CER_DC, rcCon, CER_CONSOLE_CUR, gp_VActive);
	// в пикселях нужно учитывать разделители и прокрутки
	_ASSERTE(gpSet->isAlwaysShowScrollbar!=1); // доделать
	if (m_SplitType == RConStartArgs::eSplitHorz)
	{
		if (gpSet->nSplitWidth && (rcPixels.right > (LONG)gpSet->nSplitWidth))
			rcPixels.right -= gpSet->nSplitWidth;
	}
	else
	{
		if (gpSet->nSplitHeight && (rcPixels.bottom > (LONG)gpSet->nSplitHeight))
			rcPixels.bottom -= gpSet->nSplitHeight;
	}

	RECT rc1 = rcPixels, rc2 = rcPixels, rcSplitter;
	CalcSplitRect(rcPixels, rc1, rc2, rcSplitter);

	RECT rcCon1 = CVConGroup::CalcRect(CER_CONSOLE_CUR, rc1, CER_BACK, gp_VActive);
	RECT rcCon2 = CVConGroup::CalcRect(CER_CONSOLE_CUR, rc2, CER_BACK, gp_VActive);

	//int nSplit = max(1,min(mn_SplitPercent10,999));

	//RECT rcScroll = gpConEmu->CalcMargins(CEM_SCROLL);

	if (m_SplitType == RConStartArgs::eSplitHorz)
	{
		//if (size.X >= gpSet->nSplitWidth)
		//	size.X -= gpSet->nSplitWidth;

		//_ASSERTE(rcScroll.left==0);
		//if (size.X > (UINT)(rcScroll.right * 2))
		//{
		//	_ASSERTE(gpSet->isAlwaysShowScrollbar==1); // сюда должны попадать только при включенном постоянно скролле
		//	size.X -= rcScroll.right * 2;
		//}

		//sz1.X = max(((size.X+1) * nSplit / 1000),MIN_CON_WIDTH);
		//sz2.X = max((size.X - sz1.X),MIN_CON_WIDTH);
		sz1.X = max(rcCon1.right,MIN_CON_WIDTH);
		sz2.X = max(rcCon2.right,MIN_CON_WIDTH);
	}
	else
	{
		//sz1.Y = max(((size.Y+1) * nSplit / 1000),MIN_CON_HEIGHT);
		//sz2.Y = max((size.Y - sz1.Y),MIN_CON_HEIGHT);
		sz1.Y = max(rcCon1.bottom,MIN_CON_HEIGHT);
		sz2.Y = max(rcCon2.bottom,MIN_CON_HEIGHT);
	}
}

void CVConGroup::SetConsoleSizes(const COORD& size, const RECT& rcNewCon, bool abSync)
{
	CVConGuard VCon(mp_Item);

	// Некорректно. Нужно прокрутку просто вводить. А игнорировать установку размера окна нельзя.
#if 0
	// Это не совсем корректно... ntvdm.exe не выгружается после выхода из 16бит приложения
	if (isNtvdm())
	{
		//if (size.X == 80 && size.Y>25 && lastSize1.X != size.X && size.Y == lastSize1.Y) {
		TODO("Ntvdm почему-то не всегда устанавливает ВЫСОТУ консоли в 25/28/50 символов...")
		//}
		return; // запрет изменения размеров консоли для 16бит приложений
	}
#endif

	//#ifdef MSGLOGGER
	//	char szDbg[100]; wsprintfA(szDbg, "SetAllConsoleWindowsSize({%i,%i},%i)\n", size.X, size.Y, updateInfo);
	//	DEBUGLOGFILE(szDbg);
	//#endif
	//	//g_LastConSize = size;

	if (isPictureView())
	{
		_ASSERTE(FALSE && "isPictureView() must distinct by panes/consoles");
		gpConEmu->isPiewUpdate = true;
		return;
	}




	// Заблокируем заранее
	CVConGuard VCon1(mp_Grp1 ? mp_Grp1->mp_Item : NULL);
	CVConGuard VCon2(mp_Grp2 ? mp_Grp2->mp_Item : NULL);

	if ((m_SplitType == RConStartArgs::eSplitNone) || !mp_Grp1 || !mp_Grp2)
	{
		_ASSERTE(mp_Grp1==NULL && mp_Grp2==NULL);


		RECT rcCon = MakeRect(size.X,size.Y);

		if (VCon.VCon() && VCon->RCon())
		{
			CRealConsole* pRCon = VCon->RCon();
			COORD CurSize = {(SHORT)pRCon->TextWidth(), (SHORT)pRCon->TextHeight()};

			// split-size-calculation by COORD only - fails. Need to use pixels...

			//RECT calcSize = gpConEmu->CalcRect(CER_CONSOLE_CUR, VCon.VCon());
			//_ASSERTE(calcSize.left==0 && calcSize.top==0 && calcSize.right>0 && calcSize.bottom>0);

			if ((CurSize.X != size.X) || (CurSize.Y != size.Y))
			{
				if (!VCon->RCon()->SetConsoleSize(size.X, size.Y,
					0/*don't change scroll*/, abSync ? CECMD_SETSIZESYNC : CECMD_SETSIZENOSYNC))
				{
					// Debug purposes
					rcCon = MakeRect(VCon->TextWidth,VCon->TextHeight);
				}
			}
		}

		return;
	}


	// Do Split

	RECT rcCon1, rcCon2, rcSplitter, rcSize1, rcSize2;
	CalcSplitRect(rcNewCon, rcCon1, rcCon2, rcSplitter);

	rcSize1 = CalcRect(CER_CONSOLE_CUR, rcCon1, CER_BACK, mp_Grp1->mp_Item);
	rcSize2 = CalcRect(CER_CONSOLE_CUR, rcCon2, CER_BACK, mp_Grp2->mp_Item);

	COORD sz1 = {rcSize1.right,rcSize1.bottom}, sz2 = {rcSize2.right,rcSize2.bottom};

#if 0
	COORD sz1 = size, sz2 = size;
	CalcSplitConSize(size, sz1, sz2);
#endif

	mp_Grp1->SetConsoleSizes(sz1, rcCon1, abSync);
	mp_Grp2->SetConsoleSizes(sz2, rcCon2, abSync);
}

// size in columns and lines
// здесь нужно привести размер GUI к "общей" ширине консолей в символах
// т.е. если по горизонтали есть 2 консоли, и просят размер 80x25
// то эти две консоли должны стать 40x25 а GUI отресайзиться под 80x25
// В принципе, эту функцию можно было бы и в CConEmu оставить, но для общности путь здесь будет
void CVConGroup::SetAllConsoleWindowsSize(const COORD& size, /*bool updateInfo,*/ bool bSetRedraw /*= false*/, bool bResizeConEmuWnd /*= false*/)
{
	CVConGuard VCon(gp_VActive);
	CVConGroup* pRoot = GetRootOfVCon(VCon.VCon());

	// Некорректно. Нужно прокрутку просто вводить. А игнорировать установку размера окна нельзя.
#if 0
	// Это не совсем корректно... ntvdm.exe не выгружается после выхода из 16бит приложения
	if (isNtvdm())
	{
		//if (size.X == 80 && size.Y>25 && lastSize1.X != size.X && size.Y == lastSize1.Y) {
		TODO("Ntvdm почему-то не всегда устанавливает ВЫСОТУ консоли в 25/28/50 символов...")
		//}
		return; // запрет изменения размеров консоли для 16бит приложений
	}
#endif

#ifdef MSGLOGGER
	char szDbg[100]; wsprintfA(szDbg, "SetAllConsoleWindowsSize({%i,%i},%i)\n", size.X, size.Y, bSetRedraw);
	DEBUGLOGFILE(szDbg);
#endif
	g_LastConSize = size;

	if (!pRoot)
	{
		_ASSERTE(pRoot && "Must be defined already!");
		return;
	}

	if (isPictureView())
	{
		gpConEmu->isPiewUpdate = true;
		return;
	}

	RECT rcCon = MakeRect(size.X,size.Y);

	if (bSetRedraw /*&& gp_VActive*/)
	{
		SetRedraw(FALSE);
	}

	// Для разбиения имеет смысл использовать текущий размер окна в пикселях
	RECT rcWorkspace = gpConEmu->CalcRect(CER_WORKSPACE);

	// Go (size real consoles)
	pRoot->SetConsoleSizes(size, rcWorkspace, bSetRedraw/*as Sync*/);


	if (bSetRedraw /*&& gp_VActive*/)
	{
		SetRedraw(TRUE);
		Redraw();
	}


	//// update size info
	//// !!! Это вроде делает консоль
	//WARNING("updateInfo убить");
	///*if (updateInfo && !mb_isFullScreen && !isZoomed() && !isIconic())
	//{
	//    gpSet->UpdateSize(size.X, size.Y);
	//}*/
	//RECT rcCon = MakeRect(size.X,size.Y);

	//if (apVCon)
	//{
	//	if (!apVCon->RCon()->SetConsoleSize(size.X,size.Y))
	//		rcCon = MakeRect(apVCon->TextWidth,apVCon->TextHeight);
	//}

	// При вызове из диалога "Settings..." и нажатия кнопки "Apply" (Size & Pos)
	if (bResizeConEmuWnd)
	{
		/* Считать БЕЗОТНОСИТЕЛЬНО активной консоли */
		RECT rcWnd = gpConEmu->CalcRect(CER_MAIN, rcCon, CER_CONSOLE_ALL, NULL);
		RECT wndR; GetWindowRect(ghWnd, &wndR); // текущий XY
		MOVEWINDOW(ghWnd, wndR.left, wndR.top, rcWnd.right, rcWnd.bottom, 1);
	}
}

void CVConGroup::SyncAllConsoles2Window(RECT rcWnd, enum ConEmuRect tFrom /*= CER_MAIN*/, bool bSetRedraw /*= false*/)
{
	if (!isVConExists(0))
		return;
	CVConGuard VCon(gp_VActive);
	RECT rcAllCon = gpConEmu->CalcRect(CER_CONSOLE_ALL, rcWnd, tFrom, VCon.VCon());
	COORD crNewAllSize = {rcAllCon.right,rcAllCon.bottom};
	SetAllConsoleWindowsSize(crNewAllSize, bSetRedraw);
}

void CVConGroup::LockSyncConsoleToWindow(bool abLockSync)
{
	gb_SkipSyncSize = abLockSync;
}

// Изменить размер консоли по размеру окна (главного)
void CVConGroup::SyncConsoleToWindow(LPRECT prcNewWnd/*=NULL*/, bool bSync/*=false*/)
{
	if (gb_SkipSyncSize || isNtvdm())
		return;

	CVConGuard VCon(gp_VActive);
	if (!VCon.VCon())
		return;

	_ASSERTE(gpConEmu->mn_InResize <= 1);

	#ifdef _DEBUG
	// Не должно вызываться в процессе изменения режима окна
	if (gpConEmu->changeFromWindowMode!=wmNotChanging)
	{
		_ASSERTE(gpConEmu->changeFromWindowMode==wmNotChanging);
	}
	#endif

	//gp_VActive->RCon()->SyncConsole2Window();

	RECT rcWnd;
	if (prcNewWnd)
		rcWnd = gpConEmu->CalcRect(CER_MAINCLIENT, *prcNewWnd, CER_MAIN, VCon.VCon());
	else
		rcWnd = gpConEmu->CalcRect(CER_MAINCLIENT, VCon.VCon());

	SyncAllConsoles2Window(rcWnd, CER_MAINCLIENT, bSync);
}

// -- функция пустая, игнорируется
// Установить размер основного окна по текущему размеру gp_VActive
void CVConGroup::SyncWindowToConsole()
{
	TODO("warning: Допилить прокрутки, чтобы при изменении размера в КОНСОЛИ ее можно было показать в ConEmu");

#if 0
	_ASSERTE(FALSE && "May be this function must be eliminated!");
	// Наверное имеет смысл вообще не ресайзить GUI по размеру консоли.
	// Если консоль больше - прокрутка, если меньше - центрирование.

	DEBUGLOGFILE("SyncWindowToConsole\n");

	if (gb_SkipSyncSize || !gp_VActive)
		return;

#ifdef _DEBUG
	_ASSERTE(GetCurrentThreadId() == gpConEmu->mn_MainThreadId);
	if (gp_VActive->TextWidth == 80)
	{
		int nDbg = gp_VActive->TextWidth;
	}
#endif

	CVConGuard VCon(gp_VActive);
	CRealConsole* pRCon = gp_VActive->RCon();

	if (pRCon && (gp_VActive->TextWidth != pRCon->TextWidth() || gp_VActive->TextHeight != pRCon->TextHeight()))
	{
		_ASSERTE(FALSE);
		gp_VActive->Update();
	}

	RECT rcDC = gp_VActive->GetRect();
	/*MakeRect(gp_VActive->Width, gp_VActive->Height);
	if (gp_VActive->Width == 0 || gp_VActive->Height == 0) {
	rcDC = MakeRect(gp_VActive->winSize.X, gp_VActive->winSize.Y);
	}*/
	//_ASSERTE(rcDC.right>250 && rcDC.bottom>200);
	RECT rcWnd = CalcRect(CER_MAIN, rcDC, CER_DC, gp_VActive); // размеры окна
	//GetCWShift(ghWnd, &cwShift);
	RECT wndR; GetWindowRect(ghWnd, &wndR); // текущий XY

	if (gpSetCls->isAdvLogging)
	{
		char szInfo[128]; wsprintfA(szInfo, "SyncWindowToConsole(Cols=%i, Rows=%i)", gp_VActive->TextWidth, gp_VActive->TextHeight);
		CVConGroup::LogString(szInfo, TRUE);
	}

	gpSetCls->UpdateSize(gp_VActive->TextWidth, gp_VActive->TextHeight);
	MOVEWINDOW(ghWnd, wndR.left, wndR.top, rcWnd.right, rcWnd.bottom, 1);
#endif
}

// Это некие сводные размеры, соответствующие тому, как если бы была
// только одна активная консоль, БЕЗ Split-screen
uint CVConGroup::TextWidth()
{
	uint nWidth = gpSet->_wndWidth;
	if (!gp_VActive)
	{
		_ASSERTE(FALSE && "No active VCon");
	}
	else
	{
		CVConGuard VCon(gp_VActive);
		CVConGroup* p = NULL;
		if (gp_VActive && gp_VActive->mp_Group)
		{
			p = ((CVConGroup*)gp_VActive->mp_Group)->GetRootGroup();
		}

		if (p != NULL)
		{
			SIZE sz; p->GetAllTextSize(sz);
        	nWidth = sz.cx; // p->AllTextWidth();
		}
		else
		{
			_ASSERTE(p && "CVConGroup MUST BE DEFINED!");

			if (gp_VActive->RCon())
			{
				// При ресайзе через окно настройки - gp_VActive еще не перерисовался
				// так что и TextWidth/TextHeight не обновился
				//-- gpSetCls->UpdateSize(gp_VActive->TextWidth, gp_VActive->TextHeight);
				nWidth = gp_VActive->RCon()->TextWidth();
			}
		}
	}
	return nWidth;
}
uint CVConGroup::TextHeight()
{
	uint nHeight = gpSet->_wndHeight;
	if (!gp_VActive)
	{
		_ASSERTE(FALSE && "No active VCon");
	}
	else
	{
		CVConGuard VCon(gp_VActive);
		CVConGroup* p = NULL;
		if (gp_VActive && gp_VActive->mp_Group)
		{
			p = ((CVConGroup*)gp_VActive->mp_Group)->GetRootGroup();
		}

		if (p != NULL)
		{
			SIZE sz; p->GetAllTextSize(sz);
        	nHeight = sz.cy; // p->AllTextHeight();
		}
		else
		{
			_ASSERTE(p && "CVConGroup MUST BE DEFINED!");

			if (gp_VActive->RCon())
			{
				// При ресайзе через окно настройки - gp_VActive еще не перерисовался
				// так что и TextWidth/TextHeight не обновился
				//-- gpSetCls->UpdateSize(gp_VActive->TextWidth, gp_VActive->TextHeight);
				nHeight = gp_VActive->RCon()->TextHeight();
			}
		}
	}
	return nHeight;
}

RECT CVConGroup::AllTextRect(bool abMinimal /*= false*/)
{
	RECT rcText = MakeRect(MIN_CON_WIDTH,MIN_CON_HEIGHT);

	if (!gp_VActive)
	{
		_ASSERTE(FALSE && "No active VCon");
	}
	else
	{
		CVConGuard VCon(gp_VActive);
		CVConGroup* p = NULL;
		if (gp_VActive && gp_VActive->mp_Group)
		{
			p = ((CVConGroup*)gp_VActive->mp_Group)->GetRootGroup();
		}

		if (p != NULL)
		{
			SIZE sz = {MIN_CON_WIDTH,MIN_CON_HEIGHT};
			p->GetAllTextSize(sz, abMinimal);
			rcText.right = sz.cx;
			rcText.bottom = sz.cy;
		}
		else
		{
			_ASSERTE(p && "CVConGroup MUST BE DEFINED!");

			if (gp_VActive->RCon())
			{
				// При ресайзе через окно настройки - gp_VActive еще не перерисовался
				// так что и TextWidth/TextHeight не обновился
				//-- gpSetCls->UpdateSize(gp_VActive->TextWidth, gp_VActive->TextHeight);
				int nWidth = gp_VActive->RCon()->TextWidth();
				int nHeight = gp_VActive->RCon()->TextHeight();
				if (nWidth <= 0 || nHeight <= 0)
				{
					_ASSERTE(FALSE && "gp_VActive->RCon()->TextWidth()>=0 && gp_VActive->RCon()->TextHeight()>=0");
				}
				else
				{
					rcText.right = nWidth;
					rcText.bottom = nHeight;
				}
			}
		}
	}

	return rcText;
}

// WindowMode: rNormal, rMaximized, rFullScreen
// rcWnd: размер ghWnd
// Returns: true - если успешно, можно продолжать
bool CVConGroup::PreReSize(uint WindowMode, RECT rcWnd, enum ConEmuRect tFrom /*= CER_MAIN*/, bool bSetRedraw /*= false*/)
{
	bool lbRc = true;
	CVConGuard VCon(gp_VActive);

	if (gp_VActive)
	{
		gpConEmu->AutoSizeFont(rcWnd, tFrom);
	}

	if (tFrom == CER_MAIN)
	{
		rcWnd = gpConEmu->CalcRect(CER_MAINCLIENT, rcWnd, CER_MAIN);
		tFrom = CER_MAINCLIENT;
	}

	RECT rcCon = CalcRect(CER_CONSOLE_ALL, rcWnd, tFrom, gp_VActive);

	if (!rcCon.right || !rcCon.bottom) { rcCon.right = gpConEmu->wndWidth; rcCon.bottom = gpConEmu->wndHeight; }

	COORD size = {rcCon.right, rcCon.bottom};
	if (isVConExists(0))
	{
		SetAllConsoleWindowsSize(size, bSetRedraw);
	}

	//if (bSetRedraw /*&& gp_VActive*/)
	//{
	//	SetRedraw(FALSE);
	//}

	//TODO("DoubleView");
	//if (gp_VActive && !gp_VActive->RCon()->SetConsoleSize(rcCon.right, rcCon.bottom))
	//{
	//	LogString("!!!SetConsoleSize FAILED!!!");

	//	lbRc = false;
	//}

	//if (bSetRedraw /*&& gp_VActive*/)
	//{
	//	SetRedraw(TRUE);
	//	Redraw();
	//}

	return lbRc;
}

void CVConGroup::SetRedraw(bool abRedrawEnabled)
{
	for (size_t i = 0; i < countof(gp_VCon); i++)
	{
		CVConGuard VCon(gp_VCon[i]);
		if (VCon.VCon() && VCon->isVisible())
			VCon->SetRedraw(abRedrawEnabled);
	}
}

void CVConGroup::Redraw()
{
	for (size_t i = 0; i < countof(gp_VCon); i++)
	{
		CVConGuard VCon(gp_VCon[i]);
		if (VCon.VCon() && VCon->isVisible())
			VCon->Redraw();
	}
}

void CVConGroup::InvalidateGaps()
{
	if (ghWndWork)
	{
		InvalidateRect(ghWndWork, NULL, FALSE);
	}
	else
	{
		_ASSERTE(ghWndWork!=NULL);
	}

#if 0
	int iRc = SIMPLEREGION;

	RECT rc = {};
	GetClientRect(ghWnd, &rc);
	HRGN h = CreateRectRgn(rc.left, rc.top, rc.right, rc.bottom);

	#if 0
	TODO("DoubleView");
	if ((iRc != NULLREGION) && mp_TabBar->GetRebarClientRect(&rc))
	{
		HRGN h2 = CreateRectRgn(rc.left, rc.top, rc.right, rc.bottom);
		iRc = CombineRgn(h, h, h2, RGN_DIFF);
		DeleteObject(h2);

		if (iRc == NULLREGION)
			goto wrap;
	}

	if ((iRc != NULLREGION) && mp_Status->GetStatusBarClientRect(&rc))
	{
		HRGN h2 = CreateRectRgn(rc.left, rc.top, rc.right, rc.bottom);
		CombineRgn(h, h, h2, RGN_DIFF);
		DeleteObject(h2);

		if (iRc == NULLREGION)
			goto wrap;
	}
	#endif

	// Теперь - VConsole (все видимые!)
	if (iRc != NULLREGION)
	{
		TODO("DoubleView");
		TODO("Заменить на Background, когда будет");
		HWND hView = gp_VActive ? gp_VActive->GetView() : NULL;
		if (hView && GetWindowRect(hView, &rc))
		{
			MapWindowPoints(NULL, ghWnd, (LPPOINT)&rc, 2);
			HRGN h2 = CreateRectRgn(rc.left, rc.top, rc.right, rc.bottom);
			iRc = CombineRgn(h, h, h2, RGN_DIFF);
			DeleteObject(h2);

			if (iRc == NULLREGION)
				goto wrap;
		}
	}

	InvalidateRgn(ghWnd, h, FALSE);

wrap:
	DeleteObject(h);
#endif
}

// Должно вызываться ТОЛЬКО для DC в ghWndWork!!!
void CVConGroup::PaintGaps(HDC hDC)
{
	bool lbReleaseDC = false;

	if (hDC == NULL)
	{
		hDC = GetDC(ghWnd); // Главное окно!
		lbReleaseDC = true;
	}

	HBRUSH hBrush = NULL;


	bool lbFade = gpSet->isFadeInactive && !gpConEmu->isMeForeground(true);


	////RECT rcClient = GetGuiClientRect(); // Клиентская часть главного окна
	//RECT rcClient = gpConEmu->CalcRect(CER_WORKSPACE);

	_ASSERTE(ghWndWork!=NULL);
	RECT rcClient = {};
	GetClientRect(ghWndWork, &rcClient);
	//MapWindowPoints(ghWndWork, ghWnd, (LPPOINT)&rcClient, 2);

	//HWND hView = gp_VActive ? gp_VActive->GetView() : NULL;

	//if (!hView || !IsWindowVisible(hView))
	if (!isVConExists(0))
	{
		int nColorIdx = RELEASEDEBUGTEST(0/*Black*/,1/*Blue*/);
		HBRUSH hBrush = CreateSolidBrush(gpSet->GetColors(-1, lbFade)[nColorIdx]);

		FillRect(hDC, &rcClient, hBrush);
	}
	else
	{
		COLORREF crBack = lbFade ? gpSet->GetFadeColor(gpSet->nStatusBarBack) : gpSet->nStatusBarBack;
		COLORREF crText = lbFade ? gpSet->GetFadeColor(gpSet->nStatusBarLight) : gpSet->nStatusBarLight;
		COLORREF crDash = lbFade ? gpSet->GetFadeColor(gpSet->nStatusBarDark) : gpSet->nStatusBarDark;

		hBrush = CreateSolidBrush(crBack);

		TODO("DoubleView: красивая отрисовка выпуклых сплиттеров");

		FillRect(hDC, &rcClient, hBrush);

		//int iRc = SIMPLEREGION;

		//HRGN h = CreateRectRgn(rc.left, rc.top, rc.right, rc.bottom);

		//TODO("DoubleView");
		//if ((iRc != NULLREGION) && gpConEmu->mp_TabBar->GetRebarClientRect(&rc))
		//{
		//	HRGN h2 = CreateRectRgn(rc.left, rc.top, rc.right, rc.bottom);
		//	iRc = CombineRgn(h, h, h2, RGN_DIFF);
		//	DeleteObject(h2);
		//}

		//if ((iRc != NULLREGION) && gpConEmu->mp_Status->GetStatusBarClientRect(&rc))
		//{
		//	HRGN h2 = CreateRectRgn(rc.left, rc.top, rc.right, rc.bottom);
		//	CombineRgn(h, h, h2, RGN_DIFF);
		//	DeleteObject(h2);
		//}

		//// Теперь - VConsole (все видимые!)
		//if (iRc != NULLREGION)
		//{
		//	for (size_t i = 0; i < countof(gp_VCon); i++)
		//	{
		//		CVConGuard VCon(gp_VCon[i]);
		//		if (VCon.VCon() && VCon->isVisible())
		//		{
		//			HWND hView = VCon.VCon() ? VCon->GetView() : NULL;
		//			if (hView && GetWindowRect(hView, &rc))
		//			{
		//				MapWindowPoints(NULL, ghWnd, (LPPOINT)&rc, 2);
		//				HRGN h2 = CreateRectRgn(rc.left, rc.top, rc.right, rc.bottom);
		//				iRc = CombineRgn(h, h, h2, RGN_DIFF);
		//				DeleteObject(h2);
		//				if (iRc == NULLREGION)
		//					break;
		//			}
		//		}
		//	}
		//}

		//if (iRc != NULLREGION)
		//	FillRgn(hDC, h, hBrush);

		//DeleteObject(h);

		////RECT rcMargins = gpConEmu->CalcMargins(CEM_TAB); // Откусить площадь, занятую строкой табов
		////AddMargins(rcClient, rcMargins, FALSE);
		////// На старте при /max - ghWnd DC еще не изменил свое положение
		//////RECT offsetRect; Get ClientRect(ghWnd DC, &offsetRect);
		////RECT rcWndClient; Get ClientRect(ghWnd, &rcWndClient);
		////RECT rcCalcCon = gpConEmu->CalcRect(CER_BACK, rcWndClient, CER_MAINCLIENT);
		////RECT rcCon = gpConEmu->CalcRect(CER_CONSOLE, rcCalcCon, CER_BACK);
		//// -- работает не правильно - не учитывает центрирование в Maximized
		////RECT offsetRect = gpConEmu->CalcRect(CER_BACK, rcCon, CER_CONSOLE);
		///*
		//RECT rcClient = {0};
		//if (ghWnd DC) {
		//	Get ClientRect(ghWnd DC, &rcClient);
		//	MapWindowPoints(ghWnd DC, ghWnd, (LPPOINT)&rcClient, 2);
		//}
		//*/
		//RECT dcSize = CalcRect(CER_DC, rcClient, CER_MAINCLIENT);
		//RECT client = CalcRect(CER_DC, rcClient, CER_MAINCLIENT, NULL, &dcSize);
		//WARNING("Вынести в CalcRect");
		//RECT offsetRect; memset(&offsetRect,0,sizeof(offsetRect));

		//if (gp_VActive && gp_VActive->Width && gp_VActive->Height)
		//{
		//	if ((gpSet->isTryToCenter && (isZoomed() || mb_isFullScreen || gpSet->isQuakeStyle))
		//			|| isNtvdm())
		//	{
		//		offsetRect.left = (client.right+client.left-(int)gp_VActive->Width)/2;
		//		offsetRect.top = (client.bottom+client.top-(int)gp_VActive->Height)/2;
		//	}

		//	if (offsetRect.left<client.left) offsetRect.left=client.left;

		//	if (offsetRect.top<client.top) offsetRect.top=client.top;

		//	offsetRect.right = offsetRect.left + gp_VActive->Width;
		//	offsetRect.bottom = offsetRect.top + gp_VActive->Height;

		//	if (offsetRect.right>client.right) offsetRect.right=client.right;

		//	if (offsetRect.bottom>client.bottom) offsetRect.bottom=client.bottom;
		//}
		//else
		//{
		//	offsetRect = client;
		//}

		//// paint gaps between console and window client area with first color
		//RECT rect;
		////TODO:!!!
		//// top
		//rect = rcClient;
		//rect.bottom = offsetRect.top;

		//if (!IsRectEmpty(&rect))
		//	FillRect(hDC, &rect, hBrush);

		//#ifdef _DEBUG
		////GdiFlush();
		//#endif
		//// right
		//rect.left = offsetRect.right;
		//rect.bottom = rcClient.bottom;

		//if (!IsRectEmpty(&rect))
		//	FillRect(hDC, &rect, hBrush);

		//#ifdef _DEBUG
		////GdiFlush();
		//#endif
		//// left
		//rect.left = 0;
		//rect.right = offsetRect.left;
		//rect.bottom = rcClient.bottom;

		//if (!IsRectEmpty(&rect))
		//	FillRect(hDC, &rect, hBrush);

		//#ifdef _DEBUG
		////GdiFlush();
		//#endif
		//// bottom
		//rect.left = 0;
		//rect.right = rcClient.right;
		//rect.top = offsetRect.bottom;
		//rect.bottom = rcClient.bottom;

		//if (!IsRectEmpty(&rect))
		//	FillRect(hDC, &rect, hBrush);

		//#ifdef _DEBUG
		////GdiFlush();
		//#endif

	}

	if (hBrush)
		DeleteObject(hBrush);

	if (lbReleaseDC)
		ReleaseDC(ghWnd, hDC);
}

DWORD CVConGroup::GetFarPID(BOOL abPluginRequired/*=FALSE*/)
{
	DWORD dwPID = 0;

	if (gp_VActive && gp_VActive->RCon())
		dwPID = gp_VActive->RCon()->GetFarPID(abPluginRequired);

	return dwPID;
}

// Чтобы при создании ПЕРВОЙ консоли на экране сразу можно было что-то нарисовать
void CVConGroup::OnVConCreated(CVirtualConsole* apVCon, const RConStartArgs *args)
{
	if (!gp_VActive || (gb_CreatingActive && !args->bBackgroundTab))
	{
		gp_VActive = apVCon;

		HWND hWndDC = gp_VActive->GetView();
		if (hWndDC != NULL)
		{
			_ASSERTE(hWndDC==NULL && "Called from constructor, NULL expected");
			// Теперь можно показать созданную консоль
			apiShowWindow(gp_VActive->GetView(), SW_SHOW);
		}
	}
}

void CVConGroup::OnGuiFocused(BOOL abFocus, BOOL abForceChild /*= FALSE*/)
{
	for (size_t i = 0; i < countof(gp_VCon); i++)
	{
		CVConGuard VCon(gp_VCon[i]);
		if (VCon.VCon() && VCon->isVisible())
			VCon->RCon()->OnGuiFocused(abFocus, abForceChild);
	}
}

void CVConGroup::OnConsoleResize(bool abSizingToDo)
{
	//DEBUGSTRERR(L"На удаление. ConEmu не должен дергаться при смене размера ИЗ КОНСОЛИ\n");
	DEBUGSTRERR(L"CVConGroup::OnConsoleResize must NOT!!! be called while CONSOLE size is changed (from console)\n");

	//MSetter lInConsoleResize(&mb_InConsoleResize);
	// Выполняться должно в нити окна, иначе можем повиснуть
	_ASSERTE(gpConEmu->isMainThread() && !gpConEmu->isIconic());

	//COORD c = ConsoleSizeFromWindow();
	RECT client = gpConEmu->GetGuiClientRect();

	// Проверим, вдруг не отработал isIconic
	if (client.bottom > 10)
	{
		CVConGuard VCon(gp_VActive);
		gpConEmu->AutoSizeFont(client, CER_MAINCLIENT);
		RECT c = CalcRect(CER_CONSOLE_CUR, client, CER_MAINCLIENT, gp_VActive);
		// чтобы не насиловать консоль лишний раз - реальное измененение ее размеров только
		// при отпускании мышкой рамки окна
		BOOL lbSizeChanged = FALSE;
		int nCurConWidth = (int)gp_VActive->RCon()->TextWidth();
		int nCurConHeight = (int)gp_VActive->RCon()->TextHeight();

		if (gp_VActive)
		{
			lbSizeChanged = (c.right != nCurConWidth || c.bottom != nCurConHeight);
		}

		if (gpSetCls->isAdvLogging)
		{
			char szInfo[160]; wsprintfA(szInfo, "OnConsoleResize: lbSizeChanged=%i, client={{%i,%i},{%i,%i}}, CalcCon={%i,%i}, CurCon={%i,%i}",
			                            lbSizeChanged, client.left, client.top, client.right, client.bottom,
			                            c.right, c.bottom, nCurConWidth, nCurConHeight);
			CVConGroup::LogString(szInfo, TRUE);
		}

		if (!gpConEmu->isSizing() &&
		        (abSizingToDo /*после реального ресайза мышкой*/ ||
		         gpConEmu->gbPostUpdateWindowSize /*после появления/скрытия табов*/ ||
		         lbSizeChanged /*или размер в виртуальной консоли не совпадает с расчетным*/))
		{
			gpConEmu->gbPostUpdateWindowSize = false;

			if (isNtvdm())
			{
				gpConEmu->SyncNtvdm();
			}
			else
			{
				if ((gpConEmu->WindowMode == wmNormal) && !abSizingToDo)
					SyncWindowToConsole(); // -- функция пустая, игнорируется
				else
					SyncConsoleToWindow();

				gpConEmu->OnSize(true, 0, client.right, client.bottom);
			}

			//_ASSERTE(gp_VActive!=NULL);
			if (gp_VActive)
			{
				g_LastConSize = MakeCoord(gp_VActive->TextWidth,gp_VActive->TextHeight);
			}

			//// Запомнить "идеальный" размер окна, выбранный пользователем
			//if (abSizingToDo)
			//	gpConEmu->UpdateIdealRect();

			//if (lbSizingToDo && !mb_isFullScreen && !isZoomed() && !isIconic()) {
			//	GetWindowRect(ghWnd, &mrc_Ideal);
			//}
		}
		else if (gp_VActive
		        && (g_LastConSize.X != (int)gp_VActive->TextWidth
		            || g_LastConSize.Y != (int)gp_VActive->TextHeight))
		{
			// По идее, сюда мы попадаем только для 16-бит приложений
			if (isNtvdm())
				gpConEmu->SyncNtvdm();

			g_LastConSize = MakeCoord(gp_VActive->TextWidth,gp_VActive->TextHeight);
		}
	}
}

// вызывается из CConEmuMain::OnSize
void CVConGroup::ReSizePanes(RECT mainClient)
{
	//RECT mainClient = MakeRect(newClientWidth,newClientHeight);
	if (!gp_VActive)
	{
		// Еще нету никого
		_ASSERTE(gp_VActive);
		return;
	}

	CVConGuard VCon(gp_VActive);
	CVirtualConsole* pVCon = VCon.VCon();

	RECT rcNewCon = {};

	rcNewCon = gpConEmu->CalcRect(CER_WORKSPACE, mainClient, CER_MAINCLIENT, pVCon);

	CVConGroup::MoveAllVCon(pVCon, rcNewCon);
}

void CVConGroup::NotifyChildrenWindows()
{
	// Issue 878: ConEmu - Putty: Can't select in putty when ConEmu change display
	for (size_t i = 0; i < countof(gp_VCon); i++)
	{
		CVConGuard VCon(gp_VCon[i]);
		if (VCon.VCon() && isVisible(VCon.VCon()))
		{
			HWND hGuiWnd = VCon->GuiWnd(); // Child GUI Window
			if (hGuiWnd)
			{
				CRealConsole* pRCon = VCon->RCon();
				pRCon->GuiNotifyChildWindow();

				////VCon->RCon()->SetOtherWindowPos(hGuiWnd, NULL, int X, int Y, int cx, int cy, UINT uFlags)

				//RECT rcChild = {};
				//GetWindowRect(hGuiWnd, &rcChild);
				////MapWindowPoints(NULL, VCon->GetBack(), (LPPOINT)&rcChild, 2);

				//WPARAM wParam = 0;
				//LPARAM lParam = MAKELPARAM(rcChild.left, rcChild.top);
				////pRCon->PostConsoleMessage(hGuiWnd, WM_MOVE, wParam, lParam);

				//wParam = ::IsZoomed(hGuiWnd) ? SIZE_MAXIMIZED : SIZE_RESTORED;
				//lParam = MAKELPARAM(rcChild.right-rcChild.left, rcChild.bottom-rcChild.top);
				////pRCon->PostConsoleMessage(hGuiWnd, WM_SIZE, wParam, lParam);
			}
		}
	}
}

bool CVConGroup::isInGroup(CVirtualConsole* apVCon, CVConGroup* apGroup)
{
	if (!apVCon)
		return false;
	if (!apGroup)
		return true;

	CVConGroup* pGr = ((CVConGroup*)apVCon->mp_Group);
	while (pGr)
	{
		if (pGr == apGroup)
			return true;
		pGr = pGr->mp_Parent;
	}

	return false;
}

wchar_t* CVConGroup::GetTasks(CVConGroup* apRoot /*= NULL*/)
{
	wchar_t* pszAll = NULL;
	wchar_t* pszTask[MAX_CONSOLE_COUNT] = {};
	size_t nTaskLen[MAX_CONSOLE_COUNT] = {};
	size_t t = 0, i, nAllLen = 0;

	for (i = 0; i < countof(gp_VCon); i++)
	{
		CVConGuard VCon;
		if (!GetVCon(i, &VCon))
			break;
		if (!isInGroup(VCon.VCon(), apRoot))
			continue;

		wchar_t* pszCmd = VCon->RCon()->CreateCommandLine(true);
		if (!pszCmd)
			continue;
		if (!*pszCmd)
		{
			SafeFree(pszCmd);
			continue;
		}

		size_t nLen = nTaskLen[t] = _tcslen(pszCmd);
		pszTask[t++] = pszCmd;
		nAllLen += nLen + 6; // + "\r\n\r\n"
	}

	if (nAllLen == NULL)
		return NULL; // Nothing to return

	nAllLen += 2;
	pszAll = (wchar_t*)malloc(nAllLen*sizeof(*pszAll));
	if (!pszAll)
		return NULL;
	wchar_t* psz = pszAll;

	for (i = 0; i < countof(gp_VCon) && pszTask[i]; i++)
	{
		if (i)
		{
			_wcscpy_c(psz, 3, L"\r\n");
			psz += 2;
		}

		if (gp_VCon[i] == gp_VActive)
		{
			*(psz++) = L'>';
		}

		_wcscpy_c(psz, nTaskLen[i]+1, pszTask[i]);
		psz += nTaskLen[i];

		_wcscpy_c(psz, 3, L"\r\n");
		psz += _tcslen(psz);

		SafeFree(pszTask[i]);
	}

	*psz = 0; // ASCIIZ
	return pszAll;
}
