
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
#include "PluginBackground.h"

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
	return Plugin()->ProcessSynchroEvent(Event, Param);
}

INT_PTR WINAPI ProcessSynchroEventW3(void* p)
{
	return Plugin()->ProcessSynchroEvent(p);
}

int WINAPI ProcessEditorEventW(int Event, void *Param)
{
	if (Event == 2/*EE_REDRAW*/)
		return 0;
	return Plugin()->ProcessEditorViewerEvent(Event, -1);
}

INT_PTR WINAPI ProcessEditorEventW3(void* p)
{
	return Plugin()->ProcessEditorEvent(p);
}

int WINAPI ProcessViewerEventW(int Event, void *Param)
{
	return Plugin()->ProcessEditorViewerEvent(-1, Event);
}

INT_PTR WINAPI ProcessViewerEventW3(void* p)
{
	return Plugin()->ProcessViewerEvent(p);
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
	ee_Read = ee_Save = ee_Redraw = ee_Close = ee_GotFocus = ee_KillFocus = ee_Change = -1;
	ve_Read = ve_Close = ve_GotFocus = ve_KillFocus = -1;
	se_CommonSynchro = -1;
	wt_Desktop = wt_Panels = wt_Viewer = wt_Editor = wt_Dialog = wt_VMenu = wt_Help = -1;
	ma_Other = ma_Shell = ma_Viewer = ma_Editor = ma_Dialog = ma_Search = ma_Disks = ma_MainMenu = ma_Menu = ma_Help = -1;
	ma_InfoPanel = ma_QViewPanel = ma_TreePanel = ma_FindFolder = ma_UserMenu = -1;
	ma_ShellAutoCompletion = ma_DialogAutoCompletion = -1;
	of_LeftDiskMenu = of_PluginsMenu = of_FindList = of_Shortcut = of_CommandLine = of_Editor = of_Viewer = of_FilePanel = of_Dialog = of_Analyse = of_RightDiskMenu = of_FromMacro = -1;
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
							int nOpenFrom = -1;
							int nArea = Plugin()->GetMacroArea();
							if (nArea != -1)
							{
								if (nArea == ma_Shell || nArea == ma_Search || nArea == ma_InfoPanel || nArea == ma_QViewPanel || nArea == ma_TreePanel)
									gnPluginOpenFrom = of_FilePanel;
								else if (nArea == ma_Editor)
									gnPluginOpenFrom = of_Editor;
								else if (nArea == ma_Viewer)
									gnPluginOpenFrom = of_Viewer;
							}
							gnPluginOpenFrom = nOpenFrom;
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
	if (Event != se_CommonSynchro)
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

int CPluginBase::ProcessEditorViewerEvent(int EditorEvent, int ViewerEvent)
{
	if (!gbRequestUpdateTabs)
	{
		if ((EditorEvent != -1)
			&& (EditorEvent == ee_Read || EditorEvent == ee_Close || EditorEvent == ee_GotFocus || EditorEvent == ee_KillFocus || EditorEvent == ee_Save))
		{
			gbRequestUpdateTabs = TRUE;
			//} else if (Event == EE_REDRAW && gbHandleOneRedraw) {
			//	gbHandleOneRedraw = false; gbRequestUpdateTabs = TRUE;
		}
		else if ((ViewerEvent != -1)
			&& (ViewerEvent == ve_Close || ViewerEvent == ve_GotFocus || ViewerEvent == ve_KillFocus || ViewerEvent == ve_Read))
		{
			gbRequestUpdateTabs = TRUE;
		}
	}

	if (isModalEditorViewer())
	{
		if ((EditorEvent == ee_Close) || (ViewerEvent == ve_Close))
		{
			gbClosingModalViewerEditor = TRUE;
		}
	}

	if (gpBgPlugin && (EditorEvent != ee_Redraw))
	{
		gpBgPlugin->OnMainThreadActivated(EditorEvent, ViewerEvent);
	}

	return 0;
}

bool CPluginBase::isModalEditorViewer()
{
	if (!gpTabs || !gpTabs->Tabs.nTabCount)
		return false;

	// Если последнее открытое окно - модальное
	if (gpTabs->Tabs.tabs[gpTabs->Tabs.nTabCount-1].Modal)
		return true;

	// Было раньше такое условие, по идее это не правильно
	// if (gpTabs->Tabs.tabs[0].Type != wt_Panels)
	// return true;

	return false;
}

// Плагин может быть вызван в первый раз из фоновой нити.
// Поэтому простой "gnMainThreadId = GetCurrentThreadId();" не прокатит. Нужно искать первую нить процесса!
DWORD CPluginBase::GetMainThreadId()
{
	DWORD nThreadID = 0;
	DWORD nProcID = GetCurrentProcessId();
	HANDLE h = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0);

	if (h != INVALID_HANDLE_VALUE)
	{
		THREADENTRY32 ti = {sizeof(THREADENTRY32)};

		if (Thread32First(h, &ti))
		{
			do
			{
				// Нужно найти ПЕРВУЮ нить процесса
				if (ti.th32OwnerProcessID == nProcID)
				{
					nThreadID = ti.th32ThreadID;
					break;
				}
			}
			while(Thread32Next(h, &ti));
		}

		CloseHandle(h);
	}

	// Нехорошо. Должна быть найдена. Вернем хоть что-то (текущую нить)
	if (!nThreadID)
	{
		_ASSERTE(nThreadID!=0);
		nThreadID = GetCurrentThreadId();
	}

	return nThreadID;
}

void CPluginBase::ShowConsoleInfo()
{
	DWORD nConIn = 0, nConOut = 0;
	HANDLE hConIn = GetStdHandle(STD_INPUT_HANDLE);
	HANDLE hConOut = GetStdHandle(STD_OUTPUT_HANDLE);
	GetConsoleMode(hConIn, &nConIn);
	GetConsoleMode(hConOut, &nConOut);

	CONSOLE_SCREEN_BUFFER_INFO csbi = {};
	GetConsoleScreenBufferInfo(hConOut, &csbi);
	CONSOLE_CURSOR_INFO ci = {};
	GetConsoleCursorInfo(hConOut, &ci);

	wchar_t szInfo[1024];
	_wsprintf(szInfo, SKIPLEN(countof(szInfo))
		L"ConEmu Console information\n"
		L"TerminalMode=%s\n"
		L"Console HWND=0x%08X; "
		L"Virtual HWND=0x%08X\n"
		L"ServerPID=%u; CurrentPID=%u\n"
		L"ConInMode=0x%08X; ConOutMode=0x%08X\n"
		L"Buffer size=(%u,%u); Rect=(%u,%u)-(%u,%u)\n"
		L"CursorInfo=(%u,%u,%u%s); MaxWndSize=(%u,%u)\n"
		L"OutputAttr=0x%02X\n"
		,
		TerminalMode ? L"Yes" : L"No",
		(DWORD)FarHwnd, (DWORD)ghConEmuWndDC,
		gdwServerPID, GetCurrentProcessId(),
		nConIn, nConOut,
		csbi.dwSize.X, csbi.dwSize.Y,
		csbi.srWindow.Left, csbi.srWindow.Top, csbi.srWindow.Right, csbi.srWindow.Bottom,
		csbi.dwCursorPosition.X, csbi.dwCursorPosition.Y, ci.dwSize, ci.bVisible ? L"V" : L"H",
		csbi.dwMaximumWindowSize.X, csbi.dwMaximumWindowSize.Y,
		csbi.wAttributes,
		0
	);

	ShowMessage(szInfo, 0, false);
}

bool CPluginBase::RunExternalProgramW(wchar_t* pszCommand, wchar_t* pszCurDir, bool bSilent/*=false*/)
{
	bool lbRc = false;
	_ASSERTE(pszCommand && *pszCommand);

	if (bSilent)
	{
		DWORD nCmdLen = lstrlen(pszCommand);
		CESERVER_REQ* pIn = ExecuteNewCmd(CECMD_NEWCMD, sizeof(CESERVER_REQ_HDR)+sizeof(CESERVER_REQ_NEWCMD)+(nCmdLen*sizeof(wchar_t)));
		if (pIn)
		{
			pIn->NewCmd.hFromConWnd = FarHwnd;
			if (pszCurDir)
				lstrcpyn(pIn->NewCmd.szCurDir, pszCurDir, countof(pIn->NewCmd.szCurDir));

			lstrcpyn(pIn->NewCmd.szCommand, pszCommand, nCmdLen+1);

			HWND hGuiRoot = GetConEmuHWND(1);
			CESERVER_REQ* pOut = ExecuteGuiCmd(hGuiRoot, pIn, FarHwnd);
			if (pOut)
			{
				if (pOut->hdr.cbSize > sizeof(pOut->hdr) && pOut->Data[0])
				{
					lbRc = true;
				}
				ExecuteFreeResult(pOut);
			}
			else
			{
				_ASSERTE(pOut!=NULL);
			}
			ExecuteFreeResult(pIn);
		}
	}
	else
	{
		STARTUPINFO cif= {sizeof(STARTUPINFO)};
		PROCESS_INFORMATION pri= {0};
		HANDLE hStdin = GetStdHandle(STD_INPUT_HANDLE);
		DWORD oldConsoleMode;
		DWORD nErr = 0;
		DWORD nExitCode = 0;
		GetConsoleMode(hStdin, &oldConsoleMode);
		SetConsoleMode(hStdin, ENABLE_ECHO_INPUT | ENABLE_LINE_INPUT | ENABLE_PROCESSED_INPUT); // подбиралось методом тыка

		#ifdef _DEBUG
		if (!bSilent)
		{
			WARNING("Посмотреть, как Update в консоль выводит.");
			wprintf(L"\nCmd: <%s>\nDir: <%s>\n\n", pszCommand, pszCurDir);
		}
		#endif

		MWow64Disable wow; wow.Disable();
		SetLastError(0);
		BOOL lb = CreateProcess(/*strCmd, strArgs,*/ NULL, pszCommand, NULL, NULL, TRUE,
		          NORMAL_PRIORITY_CLASS|CREATE_DEFAULT_ERROR_MODE, NULL, pszCurDir, &cif, &pri);
		nErr = GetLastError();
		wow.Restore();

		if (lb)
		{
			WaitForSingleObject(pri.hProcess, INFINITE);
			GetExitCodeProcess(pri.hProcess, &nExitCode);
			CloseHandle(pri.hProcess);
			CloseHandle(pri.hThread);

			#ifdef _DEBUG
			if (!bSilent)
				wprintf(L"\nConEmuC: Process was terminated, ExitCode=%i\n\n", nExitCode);
			#endif

			lbRc = true;
		}
		else
		{
			#ifdef _DEBUG
			if (!bSilent)
				wprintf(L"\nConEmuC: CreateProcess failed, ErrCode=0x%08X\n\n", nErr);
			#endif
		}

		//wprintf(L"Cmd: <%s>\nArg: <%s>\nDir: <%s>\n\n", strCmd, strArgs, pszCurDir);
		SetConsoleMode(hStdin, oldConsoleMode);
	}

	return lbRc;
}

bool CPluginBase::StartDebugger()
{
	if (IsDebuggerPresent())
	{
		ShowMessage(CEAlreadyDebuggerPresent,0); // "ConEmu plugin\nDebugger is already attached to current process"
		return false; // Уже
	}

	if (IsTerminalMode())
	{
		ShowMessage(CECantDebugInTerminal,0); // "ConEmu plugin\nDebugger is not available in terminal mode"
		return false; // Уже
	}

	//DWORD dwServerPID = 0;
	// Create process, with flag /Attach GetCurrentProcessId()
	// Sleep for sometimes, try InitHWND(hConWnd); several times
	wchar_t  szExe[MAX_PATH*3] = {0};
	wchar_t  szConEmuC[MAX_PATH];
	bool lbRc = false;
	DWORD nLen = 0;
	PROCESS_INFORMATION pi; memset(&pi, 0, sizeof(pi));
	STARTUPINFO si; memset(&si, 0, sizeof(si));
	si.cb = sizeof(si);
	DWORD dwSelfPID = GetCurrentProcessId();

	if ((nLen = GetEnvironmentVariableW(ENV_CONEMUBASEDIR_VAR_W, szConEmuC, MAX_PATH-16)) < 1)
	{
		ShowMessage(CECantDebugNotEnvVar,0); // "ConEmu plugin\nEnvironment variable 'ConEmuBaseDir' not defined\nDebugger is not available"
		return false; // Облом
	}

	lstrcatW(szConEmuC, L"\\ConEmuC.exe");

	if (!FileExists(szConEmuC))
	{
		wchar_t* pszSlash = NULL;

		if (((nLen=GetModuleFileName(0, szConEmuC, MAX_PATH-24)) < 1) || ((pszSlash = wcsrchr(szConEmuC, L'\\')) == NULL))
		{
			ShowMessage(CECantDebugNotEnvVar,0); // "ConEmu plugin\nEnvironment variable 'ConEmuBaseDir' not defined\nDebugger is not available"
			return false; // Облом
		}

		lstrcpyW(pszSlash, L"\\ConEmu\\ConEmuC.exe");

		if (!FileExists(szConEmuC))
		{
			lstrcpyW(pszSlash, L"\\ConEmuC.exe");

			if (!FileExists(szConEmuC))
			{
				ShowMessage(CECantDebugNotEnvVar,0); // "ConEmu plugin\nEnvironment variable 'ConEmuBaseDir' not defined\nDebugger is not available"
				return false; // Облом
			}
		}
	}

	int w = 80, h = 25;
	CONSOLE_SCREEN_BUFFER_INFO csbi;

	if (GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &csbi))
	{
		w = csbi.srWindow.Right - csbi.srWindow.Left + 1;
		h = csbi.srWindow.Bottom - csbi.srWindow.Top + 1;
	}

	if (ghConEmuWndDC)
	{
		DWORD nGuiPID = 0; GetWindowThreadProcessId(ghConEmuWndDC, &nGuiPID);
		// Откроем дебаггер в новой вкладке ConEmu. При желании юзеру проще сделать Detach
		// "/DEBUGPID=" обязательно должен быть первым аргументом

		_wsprintf(szExe, SKIPLEN(countof(szExe)) L"\"%s\" /ATTACH /ROOT \"%s\" /DEBUGPID=%i /BW=%i /BH=%i /BZ=9999",
		          szConEmuC, szConEmuC, dwSelfPID, w, h);
		//_wsprintf(szExe, SKIPLEN(countof(szExe)) L"\"%s\" /ATTACH /GID=%u /GHWND=%08X /ROOT \"%s\" /DEBUGPID=%i /BW=%i /BH=%i /BZ=9999",
		//          szConEmuC, nGuiPID, (DWORD)(DWORD_PTR)ghConEmuWndDC, szConEmuC, dwSelfPID, w, h);
	}
	else
	{
		// Запустить дебаггер в новом видимом консольном окне
		_wsprintf(szExe, SKIPLEN(countof(szExe)) L"\"%s\" /DEBUGPID=%i /BW=%i /BH=%i /BZ=9999",
		          szConEmuC, dwSelfPID, w, h);
	}

	if (ghConEmuWndDC)
	{
		si.dwFlags |= STARTF_USESHOWWINDOW;
		si.wShowWindow = SW_HIDE;
	}

	if (!CreateProcess(NULL, szExe, NULL, NULL, FALSE, NORMAL_PRIORITY_CLASS|CREATE_NEW_CONSOLE, NULL,
	                  NULL, &si, &pi))
	{
		// Хорошо бы ошибку показать?
		#ifdef _DEBUG
		DWORD dwErr = GetLastError();
		#endif
		ShowMessage(CECantStartDebugger,0); // "ConEmu plugin\nНе удалось запустить процесс отладчика"
	}
	else
	{
		lbRc = true;
	}

	return lbRc;
}

bool CPluginBase::Attach2Gui()
{
	bool lbRc = false;
	DWORD dwServerPID = 0;
	BOOL lbFound = FALSE;
	WCHAR  szCmdLine[MAX_PATH+0x100] = {0};
	wchar_t szConEmuBase[MAX_PATH+1], szConEmuGui[MAX_PATH+1];
	//DWORD nLen = 0;
	PROCESS_INFORMATION pi; memset(&pi, 0, sizeof(pi));
	STARTUPINFO si = {sizeof(si)};
	DWORD dwSelfPID = GetCurrentProcessId();
	wchar_t* pszSlash = NULL;
	DWORD nStartWait = 255;

	if (!FindConEmuBaseDir(szConEmuBase, szConEmuGui, ghPluginModule))
	{
		ShowMessageGui(CECantStartServer2, MB_ICONSTOP|MB_SYSTEMMODAL);
		goto wrap;
	}

	// Нужно загрузить ConEmuHk.dll и выполнить инициализацию хуков. Учесть, что ConEmuHk.dll уже мог быть загружен
	if (!ghHooksModule)
	{
		wchar_t szHookLib[MAX_PATH+16];
		wcscpy_c(szHookLib, szConEmuBase);
		#ifdef _WIN64
			wcscat_c(szHookLib, L"\\ConEmuHk64.dll");
		#else
			wcscat_c(szHookLib, L"\\ConEmuHk.dll");
		#endif
		ghHooksModule = LoadLibrary(szHookLib);
		if (ghHooksModule)
		{
			gbHooksModuleLoaded = TRUE;
			// После подцепляния к GUI нужно выполнить StartupHooks!
			gbStartupHooksAfterMap = TRUE;
		}
	}


	if (FindServerCmd(CECMD_ATTACH2GUI, dwServerPID, true) && dwServerPID != 0)
	{
		// "Server was already started. PID=%i. Exiting...\n", dwServerPID
		gdwServerPID = dwServerPID;
		_ASSERTE(gdwServerPID!=0);
		gbTryOpenMapHeader = (gpConMapInfo==NULL);

		if (gpConMapInfo)  // 04.03.2010 Maks - Если мэппинг уже открыт - принудительно передернуть ресурсы и информацию
			CheckResources(TRUE);

		lbRc = true;
		goto wrap;
	}

	gdwServerPID = 0;
	//TODO("У сервера пока не получается менять шрифт в консоли, которую создал FAR");
	//SetConsoleFontSizeTo(GetConEmuHWND(2), 6, 4, L"Lucida Console");
	// Create process, with flag /Attach GetCurrentProcessId()
	// Sleep for sometimes, try InitHWND(hConWnd); several times

	szCmdLine[0] = L'"';
	wcscat_c(szCmdLine, szConEmuBase);
	wcscat_c(szCmdLine, L"\\");
	//if ((nLen = GetEnvironmentVariableW(ENV_CONEMUBASEDIR_VAR_W, szCmdLine+1, MAX_PATH)) > 0)
	//{
	//	if (szCmdLine[nLen] != L'\\') { szCmdLine[nLen+1] = L'\\'; szCmdLine[nLen+2] = 0; }
	//}
	//else
	//{
	//	if (!GetModuleFileName(0, szCmdLine+1, MAX_PATH) || !(pszSlash = wcsrchr(szCmdLine, L'\\')))
	//	{
	//		ShowMessageGui(CECantStartServer2, MB_ICONSTOP|MB_SYSTEMMODAL);
	//		goto wrap;
	//	}
	//	pszSlash[1] = 0;
	//}

	pszSlash = szCmdLine + lstrlenW(szCmdLine);
	//BOOL lbFound = FALSE;
	// Для фанатов 64-битных версий
#ifdef WIN64

	//if (!lbFound) -- точная папка уже найдена
	//{
	//	lstrcpyW(pszSlash, L"ConEmu\\ConEmuC64.exe");
	//	lbFound = FileExists(szCmdLine+1);
	//}

	if (!lbFound)
	{
		lstrcpyW(pszSlash, L"ConEmuC64.exe");
		lbFound = FileExists(szCmdLine+1);
	}

#endif

	//if (!lbFound) -- точная папка уже найдена
	//{
	//	lstrcpyW(pszSlash, L"ConEmu\\ConEmuC.exe");
	//	lbFound = FileExists(szCmdLine+1);
	//}

	if (!lbFound)
	{
		lstrcpyW(pszSlash, L"ConEmuC.exe");
		lbFound = FileExists(szCmdLine+1);
	}

	if (!lbFound)
	{
		ShowMessageGui(CECantStartServer3, MB_ICONSTOP|MB_SYSTEMMODAL);
		goto wrap;
	}

	//if (IsWindows64())
	//	wsprintf(szCmdLine+lstrlenW(szCmdLine), L"ConEmuC64.exe\" /ATTACH /PID=%i", dwSelfPID);
	//else
	wsprintf(szCmdLine+lstrlenW(szCmdLine), L"\" /ATTACH /FARPID=%i", dwSelfPID);
	if (gdwPreDetachGuiPID)
		wsprintf(szCmdLine+lstrlenW(szCmdLine), L" /GID=%i", gdwPreDetachGuiPID);

	if (!CreateProcess(NULL, szCmdLine, NULL, NULL, FALSE, NORMAL_PRIORITY_CLASS, NULL,
	                  NULL, &si, &pi))
	{
		// Хорошо бы ошибку показать?
		ShowMessageGui(CECantStartServer, MB_ICONSTOP|MB_SYSTEMMODAL); // "ConEmu plugin\nCan't start console server process (ConEmuC.exe)\nOK"
	}
	else
	{
		wchar_t szName[64];
		_wsprintf(szName, SKIPLEN(countof(szName)) CESRVSTARTEDEVENT, pi.dwProcessId/*gnSelfPID*/);
		// Event мог быть создан и ранее (в Far-плагине, например)
		HANDLE hServerStartedEvent = CreateEvent(LocalSecurity(), TRUE, FALSE, szName);
		_ASSERTE(hServerStartedEvent!=NULL);
		HANDLE hWait[] = {pi.hProcess, hServerStartedEvent};
		nStartWait = WaitForMultipleObjects(countof(hWait), hWait, FALSE, ATTACH_START_SERVER_TIMEOUT);

		if (nStartWait == 0)
		{
			// Server was terminated!
			ShowMessageGui(CECantStartServer, MB_ICONSTOP|MB_SYSTEMMODAL); // "ConEmu plugin\nCan't start console server process (ConEmuC.exe)\nOK"
		}
		else
		{
			// Server must be initialized ATM
			_ASSERTE(nStartWait == 1);

			// Recall initialization of ConEmuHk.dll
			if (ghHooksModule)
			{
				RequestLocalServer_t fRequestLocalServer = (RequestLocalServer_t)GetProcAddress(ghHooksModule, "RequestLocalServer");
				// Refresh ConEmu HWND's
				if (fRequestLocalServer)
				{
					RequestLocalServerParm Parm = {sizeof(Parm), slsf_ReinitWindows};
					//if (gFarVersion.dwVerMajor >= 3)
					//	Parm.Flags |=
					fRequestLocalServer(&Parm);
				}
			}

			gdwServerPID = pi.dwProcessId;
			_ASSERTE(gdwServerPID!=0);
			SafeCloseHandle(pi.hProcess);
			SafeCloseHandle(pi.hThread);
			lbRc = true;
			// Чтобы MonitorThread пытался открыть Mapping
			gbTryOpenMapHeader = (gpConMapInfo==NULL);
		}
	}

wrap:
	return lbRc;
}

bool CPluginBase::FindServerCmd(DWORD nServerCmd, DWORD &dwServerPID, bool bFromAttach /*= false*/)
{
	if (!FarHwnd)
	{
		_ASSERTE(FarHwnd!=NULL);
		return false;
	}

	bool lbRc = false;

	//111209 - пробуем через мэппинг, там ИД сервера уже должен быть
	CESERVER_CONSOLE_MAPPING_HDR SrvMapping = {};
	if (LoadSrvMapping(FarHwnd, SrvMapping))
	{
		CESERVER_REQ* pIn = ExecuteNewCmd(nServerCmd, sizeof(CESERVER_REQ_HDR)+sizeof(DWORD));
		pIn->dwData[0] = GetCurrentProcessId();
		CESERVER_REQ* pOut = ExecuteSrvCmd(SrvMapping.nServerPID, pIn, FarHwnd);

		if (pOut)
		{
			_ASSERTE(SrvMapping.nServerPID == pOut->dwData[0]);
			dwServerPID = SrvMapping.nServerPID;
			ExecuteFreeResult(pOut);
			lbRc = true;
		}
		else
		{
			_ASSERTE(pOut!=NULL);
		}

		ExecuteFreeResult(pIn);

		// Если команда успешно выполнена - выходим
		if (lbRc)
			return true;
	}
	else
	{
		_ASSERTE(bFromAttach && "LoadSrvMapping(FarHwnd, SrvMapping) failed");
		return false;
	}
	return false;

#if 0
	BOOL lbRc = FALSE;
	DWORD nProcessCount = 0, nProcesses[100] = {0};
	dwServerPID = 0;
	typedef DWORD (WINAPI* FGetConsoleProcessList)(LPDWORD lpdwProcessList, DWORD dwProcessCount);
	FGetConsoleProcessList pfnGetConsoleProcessList = NULL;
	HMODULE hKernel = GetModuleHandleW(L"kernel32.dll");

	if (hKernel)
	{
		pfnGetConsoleProcessList = (FGetConsoleProcessList)GetProcAddress(hKernel, "GetConsoleProcessList");
	}

	BOOL lbWin2kMode = (pfnGetConsoleProcessList == NULL);

	if (!lbWin2kMode)
	{
		if (pfnGetConsoleProcessList)
		{
			nProcessCount = pfnGetConsoleProcessList(nProcesses, countof(nProcesses));

			if (nProcessCount && nProcessCount > countof(nProcesses))
			{
				_ASSERTE(nProcessCount <= countof(nProcesses));
				nProcessCount = 0;
			}
		}
	}

	if (lbWin2kMode)
	{
		DWORD nSelfPID = GetCurrentProcessId();
		HANDLE hSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS,0);

		if (lbWin2kMode)
		{
			if (hSnap != INVALID_HANDLE_VALUE)
			{
				PROCESSENTRY32 prc = {sizeof(PROCESSENTRY32)};

				if (Process32First(hSnap, &prc))
				{
					do
					{
						if (prc.th32ProcessID == nSelfPID)
						{
							nProcesses[0] = prc.th32ParentProcessID;
							nProcesses[1] = nSelfPID;
							nProcessCount = 2;
							break;
						}
					}
					while(!dwServerPID && Process32Next(hSnap, &prc));
				}

				CloseHandle(hSnap);
			}
		}
	}

	if (nProcessCount >= 2)
	{
		//DWORD nParentPID = 0;
		DWORD nSelfPID = GetCurrentProcessId();
		HANDLE hSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS,0);

		if (hSnap != INVALID_HANDLE_VALUE)
		{
			PROCESSENTRY32 prc = {sizeof(PROCESSENTRY32)};

			if (Process32First(hSnap, &prc))
			{
				do
				{
					for(UINT i = 0; i < nProcessCount; i++)
					{
						if (prc.th32ProcessID != nSelfPID
						        && prc.th32ProcessID == nProcesses[i])
						{
							if (lstrcmpiW(prc.szExeFile, L"conemuc.exe")==0
							        /*|| lstrcmpiW(prc.szExeFile, L"conemuc64.exe")==0*/)
							{
								CESERVER_REQ* pIn = ExecuteNewCmd(nServerCmd, sizeof(CESERVER_REQ_HDR)+sizeof(DWORD));
								pIn->dwData[0] = GetCurrentProcessId();
								CESERVER_REQ* pOut = ExecuteSrvCmd(prc.th32ProcessID, pIn, FarHwnd);

								if (pOut) dwServerPID = prc.th32ProcessID;

								ExecuteFreeResult(pIn); ExecuteFreeResult(pOut);

								// Если команда успешно выполнена - выходим
								if (dwServerPID)
								{
									lbRc = TRUE;
									break;
								}
							}
						}
					}
				}
				while(!dwServerPID && Process32Next(hSnap, &prc));
			}

			CloseHandle(hSnap);
		}
	}

	return lbRc;
#endif
}

int CPluginBase::ShowMessage(int aiMsg, int aiButtons)
{
	wchar_t szMsgText[512] = L"";
	GetMsg(aiMsg, szMsgText, countof(szMsgText));

	return ShowMessage(szMsg, aiButtons, true);
}

// Если не вызывать - буфер увеличивается автоматически. Размер в БАЙТАХ
// Возвращает FALSE при ошибках выделения памяти
bool CPluginBase::OutDataAlloc(DWORD anSize)
{
	_ASSERTE(gpCmdRet==NULL);
	// + размер заголовка gpCmdRet
	gpCmdRet = (CESERVER_REQ*)Alloc(sizeof(CESERVER_REQ_HDR)+anSize,1);

	if (!gpCmdRet)
		return false;

	// Код команды пока не известен - установит вызывающая функция
	ExecutePrepareCmd(&gpCmdRet->hdr, 0, anSize+sizeof(CESERVER_REQ_HDR));
	gpData = gpCmdRet->Data;
	gnDataSize = anSize;
	gpCursor = gpData;
	return true;
}

// Размер в БАЙТАХ. вызывается автоматически из OutDataWrite
// Возвращает FALSE при ошибках выделения памяти
bool CPluginBase::OutDataRealloc(DWORD anNewSize)
{
	if (!gpCmdRet)
		return OutDataAlloc(anNewSize);

	if (anNewSize < gnDataSize)
		return false; // нельзя выделять меньше памяти, чем уже есть

	// realloc иногда не работает, так что даже и не пытаемся
	CESERVER_REQ* lpNewCmdRet = (CESERVER_REQ*)Alloc(sizeof(CESERVER_REQ_HDR)+anNewSize,1);

	if (!lpNewCmdRet)
		return false;

	ExecutePrepareCmd(&lpNewCmdRet->hdr, gpCmdRet->hdr.nCmd, anNewSize+sizeof(CESERVER_REQ_HDR));
	LPBYTE lpNewData = lpNewCmdRet->Data;

	if (!lpNewData)
		return false;

	// скопировать существующие данные
	memcpy(lpNewData, gpData, gnDataSize);
	// запомнить новую позицию курсора
	gpCursor = lpNewData + (gpCursor - gpData);
	// И новый буфер с размером
	Free(gpCmdRet);
	gpCmdRet = lpNewCmdRet;
	gpData = lpNewData;
	gnDataSize = anNewSize;
	return true;
}

// Размер в БАЙТАХ
// Возвращает FALSE при ошибках выделения памяти
bool CPluginBase::OutDataWrite(LPVOID apData, DWORD anSize)
{
	if (!gpData)
	{
		if (!OutDataAlloc(max(1024, (anSize+128))))
			return false;
	}
	else if (((gpCursor-gpData)+anSize)>gnDataSize)
	{
		if (!OutDataRealloc(gnDataSize+max(1024, (anSize+128))))
			return false;
	}

	// Скопировать данные
	memcpy(gpCursor, apData, anSize);
	gpCursor += anSize;
	return true;
}

bool CPluginBase::CreateTabs(int windowCount)
{
	if (gpTabs && maxTabCount > (windowCount + 1))
	{
		// пересоздавать не нужно, секцию не трогаем. только запомним последнее кол-во окон
		lastWindowCount = windowCount;
		return true;
	}

	//Enter CriticalSection(csTabs);

	if ((gpTabs==NULL) || (maxTabCount <= (windowCount + 1)))
	{
		MSectionLock SC; SC.Lock(csTabs, TRUE);
		maxTabCount = windowCount + 20; // с запасом

		if (gpTabs)
		{
			Free(gpTabs); gpTabs = NULL;
		}

		gpTabs = (CESERVER_REQ*) Alloc(sizeof(CESERVER_REQ_HDR) + maxTabCount*sizeof(ConEmuTab), 1);
	}

	lastWindowCount = windowCount;

	return (gpTabs != NULL);
}

bool CPluginBase::AddTab(int &tabCount, int WindowPos, bool losingFocus, bool editorSave,
			int Type, LPCWSTR Name, LPCWSTR FileName,
			int Current, int Modified, int Modal,
			int EditViewId)
{
	bool lbCh = false;
	DEBUGSTR(L"--AddTab\n");

	if (Type == wt_Panels)
	{
		lbCh = (gpTabs->Tabs.tabs[0].Current != (Current/*losingFocus*/ ? 1 : 0)) ||
		       (gpTabs->Tabs.tabs[0].Type != wt_Panels);
		gpTabs->Tabs.tabs[0].Current = Current/*losingFocus*/ ? 1 : 0;
		//lstrcpyn(gpTabs->Tabs.tabs[0].Name, FUNC_Y(GetMsgW)(0), CONEMUTABMAX-1);
		gpTabs->Tabs.tabs[0].Name[0] = 0;
		gpTabs->Tabs.tabs[0].Pos = (WindowPos >= 0) ? WindowPos : 0;
		gpTabs->Tabs.tabs[0].Type = wt_Panels;
		gpTabs->Tabs.tabs[0].Modified = 0; // Иначе GUI может ошибочно считать, что есть несохраненные редакторы
		gpTabs->Tabs.tabs[0].EditViewId = 0;
		gpTabs->Tabs.tabs[0].Modal = 0;

		if (!tabCount)
			tabCount++;

		if (Current)
		{
			gpTabs->Tabs.CurrentType = gnCurrentWindowType = Type;
			gpTabs->Tabs.CurrentIndex = 0;
		}
	}
	else if (Type == wt_Editor || Type == wt_Viewer)
	{
		// Первое окно - должно быть панели. Если нет - значит фар открыт в режиме редактора
		if (tabCount == 1)
		{
			// 04.06.2009 Maks - Не, чего-то не то... при открытии редактора из панелей - он заменяет панели
			//gpTabs->Tabs.tabs[0].Type = Type;
		}

		// when receiving saving event receiver is still reported as modified
		if (editorSave && lstrcmpi(FileName, Name) == 0)
			Modified = 0;


		// Облагородить заголовок таба с Ctrl-O
		wchar_t szConOut[MAX_PATH];
		LPCWSTR pszName = PointToName(Name);
		if (pszName && (wmemcmp(pszName, L"CEM", 3) == 0))
		{
			LPCWSTR pszExt = PointToExt(pszName);
			if (lstrcmpi(pszExt, L".tmp") == 0)
			{
				if (gFarVersion.dwVerMajor==1)
				{
					GetMsgA(CEConsoleOutput, szConOut);
				}
				else
					lstrcpyn(szConOut, GetMsgW(CEConsoleOutput), countof(szConOut));

				Name = szConOut;
			}
		}


		lbCh = (gpTabs->Tabs.tabs[tabCount].Current != (Current/*losingFocus*/ ? 1 : 0)/*(losingFocus ? 0 : Current)*/)
		    || (gpTabs->Tabs.tabs[tabCount].Type != Type)
		    || (gpTabs->Tabs.tabs[tabCount].Modified != Modified)
			|| (gpTabs->Tabs.tabs[tabCount].Modal != Modal)
		    || (lstrcmp(gpTabs->Tabs.tabs[tabCount].Name, Name) != 0);
		// when receiving losing focus event receiver is still reported as current
		gpTabs->Tabs.tabs[tabCount].Type = Type;
		gpTabs->Tabs.tabs[tabCount].Current = (Current/*losingFocus*/ ? 1 : 0)/*losingFocus ? 0 : Current*/;
		gpTabs->Tabs.tabs[tabCount].Modified = Modified;
		gpTabs->Tabs.tabs[tabCount].Modal = Modal;
		gpTabs->Tabs.tabs[tabCount].EditViewId = EditViewId;

		if (gpTabs->Tabs.tabs[tabCount].Current != 0)
		{
			lastModifiedStateW = Modified != 0 ? 1 : 0;
			gpTabs->Tabs.CurrentType = gnCurrentWindowType = Type;
			gpTabs->Tabs.CurrentIndex = tabCount;
		}

		//else
		//{
		//	lastModifiedStateW = -1; //2009-08-17 при наличии более одного редактора - сносит крышу
		//}
		int nLen = min(lstrlen(Name),(CONEMUTABMAX-1));
		lstrcpyn(gpTabs->Tabs.tabs[tabCount].Name, Name, nLen+1);
		gpTabs->Tabs.tabs[tabCount].Name[nLen]=0;
		gpTabs->Tabs.tabs[tabCount].Pos = (WindowPos >= 0) ? WindowPos : tabCount;
		tabCount++;
	}

	return lbCh;
}

void CPluginBase::SendTabs(int tabCount, bool abForceSend/*=false*/)
{
	MSectionLock SC; SC.Lock(csTabs);

	if (!gpTabs)
	{
		_ASSERTE(gpTabs!=NULL);
		return;
	}

	gnCurTabCount = tabCount; // сразу запомним!, А то при ретриве табов количество еще старым будет...
	gpTabs->Tabs.nTabCount = tabCount;
	gpTabs->hdr.cbSize = sizeof(CESERVER_REQ_HDR) + sizeof(CESERVER_REQ_CONEMUTAB)
	                     + sizeof(ConEmuTab) * ((tabCount > 1) ? (tabCount - 1) : 0);
	// Обновляем структуру сразу, чтобы она была готова к отправке в любой момент
	ExecutePrepareCmd(&gpTabs->hdr, CECMD_TABSCHANGED, gpTabs->hdr.cbSize);

	// Это нужно делать только если инициировано ФАРОМ. Если запрос прислал ConEmu - не посылать...
	if (tabCount && ghConEmuWndDC && IsWindow(ghConEmuWndDC) && abForceSend)
	{
		gpTabs->Tabs.bMacroActive = Plugin()->IsMacroActive();
		gpTabs->Tabs.bMainThread = (GetCurrentThreadId() == gnMainThreadId);

		// Если выполняется макрос и отложенная отсылка (по окончанию) уже запрошена
		if (gpTabs->Tabs.bMacroActive && gbNeedPostTabSend)
		{
			gnNeedPostTabSendTick = GetTickCount(); // Обновить тик
			return;
		}

		gbNeedPostTabSend = FALSE;
		CESERVER_REQ* pOut =
		    ExecuteGuiCmd(FarHwnd, gpTabs, FarHwnd);

		if (pOut)
		{
			if (pOut->hdr.cbSize >= (sizeof(CESERVER_REQ_HDR) + sizeof(CESERVER_REQ_CONEMUTAB_RET)))
			{
				if (gpTabs->Tabs.bMacroActive && pOut->TabsRet.bNeedPostTabSend)
				{
					// Отослать после того, как макрос завершится
					gbNeedPostTabSend = TRUE;
					gnNeedPostTabSendTick = GetTickCount();
				}
				else if (pOut->TabsRet.bNeedResize)
				{
					// Если это отложенная отсылка табов после выполнения макросов
					if (GetCurrentThreadId() == gnMainThreadId)
					{
						FarSetConsoleSize(pOut->TabsRet.crNewSize.X, pOut->TabsRet.crNewSize.Y);
					}
				}
			}

			ExecuteFreeResult(pOut);
		}
	}

	SC.Unlock();
}

void CPluginBase::CloseTabs()
{
	if (ghConEmuWndDC && IsWindow(ghConEmuWndDC) && FarHwnd)
	{
		CESERVER_REQ in; // Пустая команда - значит FAR закрывается
		ExecutePrepareCmd(&in, CECMD_TABSCHANGED, sizeof(CESERVER_REQ_HDR));
		CESERVER_REQ* pOut = ExecuteGuiCmd(FarHwnd, &in, FarHwnd);

		if (pOut) ExecuteFreeResult(pOut);
	}
}

bool CPluginBase::UpdateConEmuTabs(bool abSendChanges)
{
	bool lbCh = false;
	// Блокируем сразу, т.к. ниже по коду gpTabs тоже используется
	MSectionLock SC; SC.Lock(csTabs);
	// На случай, если текущее окно заблокировано диалогом - не получится точно узнать
	// какое окно фара активно. Поэтому вернем последнее известное.
	int nLastCurrentTab = -1, nLastCurrentType = -1;

	if (gpTabs && gpTabs->Tabs.nTabCount > 0)
	{
		nLastCurrentTab = gpTabs->Tabs.CurrentIndex;
		nLastCurrentType = gpTabs->Tabs.CurrentType;
	}

	if (gpTabs)
	{
		gpTabs->Tabs.CurrentIndex = -1; // для строгости
	}

	if (!gbIgnoreUpdateTabs)
	{
		if (gbRequestUpdateTabs)
			gbRequestUpdateTabs = FALSE;

		if (ghConEmuWndDC && FarHwnd)
			CheckResources(FALSE);

		lbCh = UpdateConEmuTabsApi();
	}

	if (gpTabs)
	{
		if (gpTabs->Tabs.CurrentIndex == -1 && nLastCurrentTab != -1 && gpTabs->Tabs.nTabCount > 0)
		{
			// Активное окно определить не удалось
			if ((UINT)nLastCurrentTab >= gpTabs->Tabs.nTabCount)
				nLastCurrentTab = (gpTabs->Tabs.nTabCount - 1);

			gpTabs->Tabs.CurrentIndex = nLastCurrentTab;
			gpTabs->Tabs.tabs[nLastCurrentTab].Current = TRUE;
			gpTabs->Tabs.CurrentType = gpTabs->Tabs.tabs[nLastCurrentTab].Type;
		}

		if (gpTabs->Tabs.CurrentType == 0)
		{
			if (gpTabs->Tabs.CurrentIndex >= 0 && gpTabs->Tabs.CurrentIndex < (int)gpTabs->Tabs.nTabCount)
				gpTabs->Tabs.CurrentType = gpTabs->Tabs.tabs[nLastCurrentTab].Type;
			else
				gpTabs->Tabs.CurrentType = WTYPE_PANELS;
		}

		gnCurrentWindowType = gpTabs->Tabs.CurrentType;

		if (abSendChanges || gbForceSendTabs)
		{
			_ASSERTE((gbForceSendTabs==FALSE || IsDebuggerPresent()) && "Async SetWindow was timeouted?");
			gbForceSendTabs = FALSE;
			SendTabs(gpTabs->Tabs.nTabCount, lbCh && (gnReqCommand==(DWORD)-1));
		}
	}

	if (lbCh && gpBgPlugin)
	{
		gpBgPlugin->SetForceUpdate();
		gpBgPlugin->OnMainThreadActivated();
		gbNeedBgActivate = FALSE;
	}

	return lbCh;
}
