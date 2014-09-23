
/*
Copyright (c) 2009-2014 Maximus5
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


#ifdef _DEBUG
//  Раскомментировать, чтобы сразу после загрузки плагина показать MessageBox, чтобы прицепиться дебаггером
//  #define SHOW_STARTED_MSGBOX
#endif

#define SHOWDEBUGSTR
//#define MCHKHEAP
#define DEBUGSTRMENU(s) //DEBUGSTR(s)
#define DEBUGSTRINPUT(s) //DEBUGSTR(s)
#define DEBUGSTRDLGEVT(s) //DEBUGSTR(s)
#define DEBUGSTRCMD(s) //DEBUGSTR(s)
#define DEBUGSTRACTIVATE(s) //DEBUGSTR(s)


#include "ConEmuPluginBase.h"
#include "PluginHeader.h"
#include "ConEmuPluginA.h"
#include "ConEmuPlugin995.h"
#include "ConEmuPlugin1900.h"
#include "ConEmuPlugin2800.h"

extern MOUSE_EVENT_RECORD gLastMouseReadEvent;
extern LONG gnDummyMouseEventFromMacro;
extern BOOL gbUngetDummyMouseEvent;

CPluginBase* gpPlugin = NULL;

/* EXPORTS BEGIN */

#define MIN_FAR2_BUILD 1765 // svs 19.12.2010 22:52:53 +0300 - build 1765: Новая команда в FARMACROCOMMAND - MCMD_GETAREA
#define MAKEFARVERSION(major,minor,build) ( ((major)<<8) | (minor) | ((build)<<16))

int WINAPI GetMinFarVersion()
{
	// Однако, FAR2 до сборки 748 не понимал две версии плагина в одном файле
	bool bFar2 = false;

	if (!LoadFarVersion())
		bFar2 = true;
	else
		bFar2 = gFarVersion.dwVerMajor>=2;

	if (bFar2)
	{
		return MAKEFARVERSION(2,0,MIN_FAR2_BUILD);
	}

	return MAKEFARVERSION(1,71,2470);
}

int WINAPI GetMinFarVersionW()
{
	return MAKEFARVERSION(2,0,MIN_FAR2_BUILD);
}

int WINAPI ProcessSynchroEventW(int Event, void *Param)
{
	return Plugin()->ProcessSynchroEvent(Event, void *Param);
}

INT_PTR WINAPI ProcessSynchroEventW3(void* p)
{
	return Plugin()->ProcessSynchroEvent(void* p);
}

/* EXPORTS END */

CPluginBase* Plugin()
{
	if (!gpPlugin)
	{
		if (gFarVersion.dwVerMajor==1)
			gpPlugin = new CPluginAnsi();
		else if (gFarVersion.dwBuild>=FAR_Y2_VER)
			gpPlugin = new CPluginW2800();
		else if (gFarVersion.dwBuild>=FAR_Y1_VER)
			gpPlugin = new CPluginW1900();
		else
			gpPlugin = new CPluginW995();
	}

	return gpPlugin;
}

CPluginBase::CPluginBase()
{
}

CPluginBase::~CPluginBase()
{
}

int CPluginBase::ShowMessageGui(int aiMsg, int aiButtons)
{
	wchar_t wszBuf[MAX_PATH];
	LPCWSTR pwszMsg = GetMsg(aiMsg, wszBuf, countof(wszBuf));

	wchar_t szTitle[128];
	_wsprintf(szTitle, SKIPLEN(countof(szTitle)) L"ConEmu plugin (PID=%u)", GetCurrentProcessId());

	if (!pwszMsg || !*pwszMsg)
	{
		_wsprintf(wszBuf, SKIPLEN(countof(wszBuf)) L"<MsgID=%i>", aiMsg);
		pwszMsg = wszBuf;
	}

	int nRc = MessageBoxW(NULL, pwszMsg, szTitle, aiButtons);
	return nRc;
}

void CPluginBase::PostMacro(const wchar_t* asMacro, INPUT_RECORD* apRec)
{
	if (!asMacro || !*asMacro)
		return;

	_ASSERTE(GetCurrentThreadId()==gnMainThreadId);

	MOUSE_EVENT_RECORD mre;

	if (apRec && apRec->EventType == MOUSE_EVENT)
	{
		gLastMouseReadEvent = mre = apRec->Event.MouseEvent;
	}
	else
	{
		mre = gLastMouseReadEvent;
	}

	PostMacroApi(asMacro, apRec);

	//FAR BUGBUG: Макрос не запускается на исполнение, пока мышкой не дернем :(
	//  Это чаще всего проявляется при вызове меню по RClick
	//  Если курсор на другой панели, то RClick сразу по пассивной
	//  не вызывает отрисовку :(

#if 1
	//111002 - попробуем просто gbUngetDummyMouseEvent
	//InterlockedIncrement(&gnDummyMouseEventFromMacro);
	gnDummyMouseEventFromMacro = TRUE;
	gbUngetDummyMouseEvent = TRUE;
#endif
#if 0
	//if (!mcr.Param.PlainText.Flags) {
	INPUT_RECORD ir[2] = {{MOUSE_EVENT},{MOUSE_EVENT}};

	if (isPressed(VK_CAPITAL))
		ir[0].Event.MouseEvent.dwControlKeyState |= CAPSLOCK_ON;

	if (isPressed(VK_NUMLOCK))
		ir[0].Event.MouseEvent.dwControlKeyState |= NUMLOCK_ON;

	if (isPressed(VK_SCROLL))
		ir[0].Event.MouseEvent.dwControlKeyState |= SCROLLLOCK_ON;

	ir[0].Event.MouseEvent.dwEventFlags = MOUSE_MOVED;
	ir[0].Event.MouseEvent.dwMousePosition = mre.dwMousePosition;

	// Вроде одного хватало, правда когда {0,0} посылался
	ir[1].Event.MouseEvent.dwControlKeyState = ir[0].Event.MouseEvent.dwControlKeyState;
	ir[1].Event.MouseEvent.dwEventFlags = MOUSE_MOVED;
	//ir[1].Event.MouseEvent.dwMousePosition.X = 1;
	//ir[1].Event.MouseEvent.dwMousePosition.Y = 1;
	ir[0].Event.MouseEvent.dwMousePosition = mre.dwMousePosition;
	ir[0].Event.MouseEvent.dwMousePosition.X++;

	//2010-01-29 попробуем STD_OUTPUT
	//if (!ghConIn) {
	//	ghConIn  = CreateFile(L"CONIN$", GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_READ,
	//		0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
	//	if (ghConIn == INVALID_HANDLE_VALUE) {
	//		#ifdef _DEBUG
	//		DWORD dwErr = GetLastError();
	//		_ASSERTE(ghConIn!=INVALID_HANDLE_VALUE);
	//		#endif
	//		ghConIn = NULL;
	//		return;
	//	}
	//}
	TODO("Необязательно выполнять реальную запись в консольный буфер. Можно обойтись подстановкой в наших функциях перехвата чтения буфера.");
	HANDLE hIn = GetStdHandle(STD_INPUT_HANDLE);
	DWORD cbWritten = 0;

	// Вроде одного хватало, правда когда {0,0} посылался
	#ifdef _DEBUG
	BOOL fSuccess =
	#endif
	WriteConsoleInput(hIn/*ghConIn*/, ir, 1, &cbWritten);
	_ASSERTE(fSuccess && cbWritten==1);
	//}
	//InfoW995->AdvControl(InfoW995->ModuleNumber,ACTL_REDRAWALL,NULL);
#endif
}

bool CPluginBase::isMacroActive(int& iMacroActive)
{
	if (!FarHwnd)
	{
		return false;
	}

	if (!iMacroActive)
	{
		iMacroActive = IsMacroActive() ? 1 : 2;
	}

	return (iMacroActive == 1);
}

void CPluginBase::UpdatePanelDirs()
{
	bool bChanged = false;

	_ASSERTE(gPanelDirs.ActiveDir && gPanelDirs.PassiveDir);
	CmdArg* Pnls[] = {gPanelDirs.ActiveDir, gPanelDirs.PassiveDir};

	for (int i = 0; i <= 1; i++)
	{
		GetPanelDirFlags Flags = ((i == 0) ? gpdf_Active : gpdf_Passive) | gpdf_NoPlugin;

		wchar_t* pszDir = GetPanelDir(Flags);

		if (pszDir && (lstrcmp(pszDir, Pnls[i]->ms_Arg ? Pnls[i]->ms_Arg : L"") != 0))
		{
			Pnls[i]->Set(pszDir);
			bChanged = true;
		}
		SafeFree(pszDir);
	}

	if (bChanged)
	{
		// Send to GUI
		SendCurrentDirectory(FarHwnd, gPanelDirs.ActiveDir->ms_Arg, gPanelDirs.PassiveDir->ms_Arg);
	}
}

bool CPluginBase::RunExternalProgram(wchar_t* pszCommand)
{
	wchar_t *pszExpand = NULL;
	CmdArg szTemp, szExpand, szCurDir;

	if (!pszCommand || !*pszCommand)
	{
		if (!InputBox(L"ConEmu", L"Start console program", L"ConEmu.CreateProcess", L"cmd", szTemp.ms_Arg))
			return false;

		pszCommand = szTemp.ms_Arg;
	}

	if (wcschr(pszCommand, L'%'))
	{
		szExpand.ms_Arg = ExpandEnvStr(pszCommand);
		if (szExpand.ms_Arg)
			pszCommand = szExpand.ms_Arg;
	}

	szCurDir.ms_Arg = GetPanelDir(gpdf_Active|gpdf_NoPlugin);
	if (!szCurDir.ms_Arg || !*szCurDir.ms_Arg)
	{
		szCurDir.Set(L"C:\\");
	}

	bool bSilent = (wcsstr(pszCommand, L"-new_console") != NULL);

	if (!bSilent)
		ShowUserScreen(true);

	RunExternalProgramW(pszCommand, szCurDir.ms_Arg, bSilent);

	if (!bSilent)
		ShowUserScreen(false);
	RedrawAll();

	return TRUE;
}

bool CPluginBase::ProcessCommandLine(wchar_t* pszCommand)
{
	if (!pszCommand)
		return false;

	if (lstrcmpni(pszCommand, L"run:", 4) == 0)
	{
		RunExternalProgram(pszCommand+4); //-V112
		return true;
	}

	return false;
}

void CPluginBase::ShowPluginMenu(PluginCallCommands nCallID /*= pcc_None*/)
{
	int nItem = -1;

	if (!FarHwnd)
	{
		ShowMessage(CEInvalidConHwnd,0); // "ConEmu plugin\nGetConsoleWindow()==FarHwnd is NULL"
		return;
	}

	if (IsTerminalMode())
	{
		ShowMessage(CEUnavailableInTerminal,0); // "ConEmu plugin\nConEmu is not available in terminal mode\nCheck TERM environment variable"
		return;
	}

	CheckConEmuDetached();

	if (nCallID != pcc_None)
	{
		// Команды CallPlugin
		for (size_t i = 0; i < countof(gpPluginMenu); i++)
		{
			if (gpPluginMenu[i].CallID == nCallID)
			{
				nItem = gpPluginMenu[i].MenuID;
				break;
			}
		}
		_ASSERTE(nItem!=-1);

		SHOWDBGINFO(L"*** ShowPluginMenu used default item\n");
	}
	else
	{
		ConEmuPluginMenuItem items[menu_Last] = {};
		int nCount = menu_Last; //sizeof(items)/sizeof(items[0]);
		_ASSERTE(nCount == countof(gpPluginMenu));
		for (int i = 0; i < nCount; i++)
		{
			if (!gpPluginMenu[i].LangID)
			{
				items[i].Separator = true;
				continue;
			}
			_ASSERTE(i == gpPluginMenu[i].MenuID);
			items[i].Selected = pcc_Selected((PluginMenuCommands)i);
			items[i].Disabled = pcc_Disabled((PluginMenuCommands)i);
			items[i].MsgID = gpPluginMenu[i].LangID;
		}

		SHOWDBGINFO(L"*** calling ShowPluginMenu\n");
		nItem = ShowPluginMenu(items, nCount);
	}

	if (nItem < 0)
	{
		SHOWDBGINFO(L"*** ShowPluginMenu cancelled, nItem < 0\n");
		return;
	}

	#ifdef _DEBUG
	wchar_t szInfo[128]; _wsprintf(szInfo, SKIPLEN(countof(szInfo)) L"*** ShowPluginMenu done, nItem == %i\n", nItem);
	SHOWDBGINFO(szInfo);
	#endif

	switch (nItem)
	{
		case menu_EditConsoleOutput:
		case menu_ViewConsoleOutput:
		{
			// Открыть в редакторе вывод последней консольной программы
			CESERVER_REQ* pIn = (CESERVER_REQ*)calloc(sizeof(CESERVER_REQ_HDR)+sizeof(DWORD),1);

			if (!pIn) return;

			CESERVER_REQ* pOut = NULL;
			ExecutePrepareCmd(&pIn->hdr, CECMD_GETOUTPUTFILE, sizeof(CESERVER_REQ_HDR)+sizeof(DWORD));
			pIn->OutputFile.bUnicode = (gFarVersion.dwVerMajor>=2);
			pOut = ExecuteGuiCmd(FarHwnd, pIn, FarHwnd);

			if (pOut)
			{
				if (pOut->OutputFile.szFilePathName[0])
				{
					bool lbRc = OpenEditor(pOut->OutputFile.szFilePathName, (nItem==1)/*abView*/, true);

					if (!lbRc)
					{
						DeleteFile(pOut->OutputFile.szFilePathName);
					}
				}

				ExecuteFreeResult(pOut);
			}

			free(pIn);
		} break;

		case menu_SwitchTabVisible: // Показать/спрятать табы
		case menu_SwitchTabNext:
		case menu_SwitchTabPrev:
		case menu_SwitchTabCommit:
		{
			CESERVER_REQ* pIn = ExecuteNewCmd(CECMD_TABSCMD, sizeof(CESERVER_REQ_HDR)+sizeof(pIn->Data));
			// Data[0] <== enum ConEmuTabCommand
			switch (nItem)
			{
			case menu_SwitchTabVisible: // Показать/спрятать табы
				pIn->Data[0] = ctc_ShowHide; break;
			case menu_SwitchTabNext:
				pIn->Data[0] = ctc_SwitchNext; break;
			case menu_SwitchTabPrev:
				pIn->Data[0] = ctc_SwitchPrev; break;
			case menu_SwitchTabCommit:
				pIn->Data[0] = ctc_SwitchCommit; break;
			default:
				_ASSERTE(nItem==menu_SwitchTabVisible); // неизвестная команда!
				pIn->Data[0] = ctc_ShowHide;
			}

			CESERVER_REQ* pOut = ExecuteGuiCmd(FarHwnd, pIn, FarHwnd);
			if (pOut) ExecuteFreeResult(pOut);
		} break;

		case menu_ShowTabsList:
		{
			CESERVER_REQ* pIn = ExecuteNewCmd(CECMD_GETALLTABS, sizeof(CESERVER_REQ_HDR));
			CESERVER_REQ* pOut = ExecuteGuiCmd(FarHwnd, pIn, FarHwnd);
			if (pOut && (pOut->GetAllTabs.Count > 0))
			{
				INT_PTR nMenuRc = -1;

				int Count = pOut->GetAllTabs.Count;
				int AllCount = Count + pOut->GetAllTabs.Tabs[Count-1].ConsoleIdx;
				ConEmuPluginMenuItem* pItems = (ConEmuPluginMenuItem*)calloc(AllCount,sizeof(*pItems));
				if (pItems)
				{
					int nLastConsole = 0;
					for (int i = 0, k = 0; i < Count; i++, k++)
					{
						if (nLastConsole != pOut->GetAllTabs.Tabs[i].ConsoleIdx)
						{
							pItems[k++].Separator = true;
							nLastConsole = pOut->GetAllTabs.Tabs[i].ConsoleIdx;
						}
						_ASSERTE(k < AllCount);
						pItems[k].Selected = (pOut->GetAllTabs.Tabs[i].ActiveConsole && pOut->GetAllTabs.Tabs[i].ActiveTab);
						pItems[k].Checked = pOut->GetAllTabs.Tabs[i].ActiveTab;
						pItems[k].Disabled = pOut->GetAllTabs.Tabs[i].Disabled;
						pItems[k].MsgText = pOut->GetAllTabs.Tabs[i].Title;
						pItems[k].UserData = i;
					}

					nMenuRc = ShowPluginMenu(pItems, AllCount);

					if ((nMenuRc >= 0) && (nMenuRc < AllCount))
					{
						nMenuRc = pItems[nMenuRc].UserData;

						if (pOut->GetAllTabs.Tabs[nMenuRc].ActiveConsole && !pOut->GetAllTabs.Tabs[nMenuRc].ActiveTab)
						{
							DWORD nTab = pOut->GetAllTabs.Tabs[nMenuRc].TabIdx;
							switch (Plugin()->GetMacroArea())
							{
							case MACROAREA_SHELL:
							case MACROAREA_SEARCH:
							case MACROAREA_INFOPANEL:
							case MACROAREA_QVIEWPANEL:
							case MACROAREA_TREEPANEL:
								gnPluginOpenFrom = OPEN_FILEPANEL;
								break;
							case MACROAREA_EDITOR:
								gnPluginOpenFrom = OPEN_EDITOR;
								break;
							case MACROAREA_VIEWER:
								gnPluginOpenFrom = OPEN_VIEWER;
								break;
							default:
								gnPluginOpenFrom = -1;
							}
							ProcessCommand(CMD_SETWINDOW, FALSE, &nTab, NULL, true/*bForceSendTabs*/);
						}
						else if (!pOut->GetAllTabs.Tabs[nMenuRc].ActiveConsole || !pOut->GetAllTabs.Tabs[nMenuRc].ActiveTab)
						{
							CESERVER_REQ* pActIn = ExecuteNewCmd(CECMD_ACTIVATETAB, sizeof(CESERVER_REQ_HDR)+2*sizeof(DWORD));
							pActIn->dwData[0] = pOut->GetAllTabs.Tabs[nMenuRc].ConsoleIdx;
							pActIn->dwData[1] = pOut->GetAllTabs.Tabs[nMenuRc].TabIdx;
							CESERVER_REQ* pActOut = ExecuteGuiCmd(FarHwnd, pActIn, FarHwnd);
							ExecuteFreeResult(pActOut);
							ExecuteFreeResult(pActIn);
						}
					}

					SafeFree(pItems);
				}
				ExecuteFreeResult(pOut);
			}
			else
			{
				ShowMessage(CEGetAllTabsFailed, 0);
			}
			ExecuteFreeResult(pIn);
		} break;

		case menu_ConEmuMacro: // Execute GUI macro (gialog)
		{
			if (gFarVersion.dwVerMajor==1)
				GuiMacroDlgA();
			else if (gFarVersion.dwBuild>=FAR_Y2_VER)
				FUNC_Y2(GuiMacroDlgW)();
			else if (gFarVersion.dwBuild>=FAR_Y1_VER)
				FUNC_Y1(GuiMacroDlgW)();
			else
				FUNC_X(GuiMacroDlgW)();
		} break;

		case menu_AttachToConEmu: // Attach to GUI (если FAR был CtrlAltTab)
		{
			if (TerminalMode) break;  // низзя

			if (ghConEmuWndDC && IsWindow(ghConEmuWndDC)) break;  // Мы и так подключены?

			Attach2Gui();
		} break;

		//#ifdef _DEBUG
		//case 11: // Start "ConEmuC.exe /DEBUGPID="
		//#else
		case menu_StartDebug: // Start "ConEmuC.exe /DEBUGPID="
			//#endif
		{
			if (TerminalMode) break;  // низзя

			StartDebugger();
		} break;

		case menu_ConsoleInfo:
		{
			ShowConsoleInfo();
		} break;
	}
}

int CPluginBase::ProcessSynchroEvent(int Event, void *Param)
{
	if (Event != 0/*SE_COMMONSYNCHRO*/)
		return;

	if (gbInputSynchroPending)
		gbInputSynchroPending = false;

	// Некоторые плагины (NetBox) блокируют главный поток, и открывают
	// в своем потоке диалог. Это ThreadSafe. Некорректные открытия
	// отследить не удастся. Поэтому, считаем, если Far дернул наш
	// ProcessSynchroEventW, то это (временно) стала главная нить
	DWORD nPrevID = gnMainThreadId;
	gnMainThreadId = GetCurrentThreadId();

	#ifdef _DEBUG
	{
		static int nLastType = -1;
		int nCurType = GetActiveWindowType();

		if (nCurType != nLastType)
		{
			LPCWSTR pszCurType = GetWindowTypeName(nCurType);

			LPCWSTR pszLastType = GetWindowTypeName(nLastType);

			wchar_t szDbg[255];
			_wsprintf(szDbg, SKIPLEN(countof(szDbg)) L"FarWindow: %s activated (was %s)\n", pszCurType, pszLastType);
			DEBUGSTR(szDbg);
			nLastType = nCurType;
		}
	}
	#endif

	if (!gbSynchroProhibited)
	{
		OnMainThreadActivated();
	}

	if (gnSynchroCount > 0)
		gnSynchroCount--;

	if (gbSynchroProhibited && (gnSynchroCount == 0))
	{
		Plugin()->StopWaitEndSynchro();
	}

	gnMainThreadId = nPrevID;

	return 0;
}
