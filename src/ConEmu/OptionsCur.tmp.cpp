if (!klstricmp(curCommand, _T("/autosetup")))
{
	BOOL lbTurnOn = TRUE;

	if ((i + 1) >= params)
		return 101;

	curCommand += _tcslen(curCommand) + 1; i++;

	if (*curCommand == _T('0'))
		lbTurnOn = FALSE;
	else
	{
		if ((i + 1) >= params)
			return 101;

		curCommand += _tcslen(curCommand) + 1; i++;
		DWORD dwAttr = GetFileAttributes(curCommand);

		if (dwAttr == (DWORD)-1 || (dwAttr & FILE_ATTRIBUTE_DIRECTORY))
			return 102;
	}

	HKEY hk = NULL; DWORD dw;
	int nSetupRc = 100;

	if (0 != RegCreateKeyEx(HKEY_CURRENT_USER, _T("Software\\Microsoft\\Command Processor"),
						   0, NULL, 0, KEY_ALL_ACCESS, NULL, &hk, &dw))
		return 103;

	if (lbTurnOn)
	{
		size_t cchMax = _tcslen(curCommand);
		LPCWSTR pszArg1 = NULL;
		if ((i + 1) < params)
		{
			// Здесь может быть "/GHWND=NEW"
			pszArg1 = curCommand + cchMax + 1;
			if (!*pszArg1)
				pszArg1 = NULL;
			else
				cchMax += _tcslen(pszArg1);
		}
		cchMax += 16; // + кавычки и пробелы всякие

		wchar_t* pszCmd = (wchar_t*)calloc(cchMax, sizeof(*pszCmd));
		_wsprintf(pszCmd, SKIPLEN(cchMax) L"\"%s\"%s%s%s", curCommand,
			pszArg1 ? L" \"" : L"", pszArg1 ? pszArg1 : L"", pszArg1 ? L"\"" : L"");


		if (0 == RegSetValueEx(hk, _T("AutoRun"), 0, REG_SZ, (LPBYTE)pszCmd,
							(DWORD)sizeof(TCHAR)*(_tcslen(pszCmd)+1))) //-V220
			nSetupRc = 1;

		free(pszCmd);
	}
	else
	{
		if (0==RegDeleteValue(hk, _T("AutoRun")))
			nSetupRc = 1;
	}

	RegCloseKey(hk);
	// сбрость CreateInNewEnvironment для ConMan
	ResetConman();
	return nSetupRc;
}
else if (!klstricmp(curCommand, _T("/bypass")))
{
	// Этот ключик был придуман для прозрачного запуска консоли
	// в режиме администратора
	// (т.е. чтобы окно UAC нормально всплывало, но не мелькало консольное окно)
	// Но не получилось, пока требуются хэндлы процесса, а их не получается
	// передать в НЕ приподнятый процесс (исходный ConEmu GUI).

	if (!cmdNew || !*cmdNew)
	{
		DisplayLastError(L"Invalid cmd line. '/bypass' exists, '/cmd' not", -1);
		return 100;
	}

	// Information
	#ifdef _DEBUG
	STARTUPINFO siOur = {sizeof(siOur)};
	GetStartupInfo(&siOur);
	#endif

	STARTUPINFO si = {sizeof(si)};
	si.dwFlags = STARTF_USESHOWWINDOW;

	PROCESS_INFORMATION pi = {};

	BOOL b = CreateProcess(NULL, cmdNew, NULL, NULL, TRUE, 0, NULL, NULL, &si, &pi);
	if (b)
	{
		CloseHandle(pi.hProcess);
		CloseHandle(pi.hThread);
		return 0;
	}

	// Failed
	DisplayLastError(cmdNew);
	return 100;
}
else if (!klstricmp(curCommand, _T("/demote")))
{
	// Запуск процесса (ком.строка после "/demote") в режиме простого юзера,
	// когда текущий процесс уже запущен "под админом". "Понизить" текущие
	// привилегии просто так нельзя, поэтому запуск идет через TaskSheduler.

	if (!cmdNew || !*cmdNew)
	{
		DisplayLastError(L"Invalid cmd line. '/demote' exists, '/cmd' not", -1);
		return 100;
	}


	// Information
	#ifdef _DEBUG
	STARTUPINFO siOur = {sizeof(siOur)};
	GetStartupInfo(&siOur);
	#endif

	STARTUPINFO si = {sizeof(si)};
	PROCESS_INFORMATION pi = {};
	si.dwFlags = STARTF_USESHOWWINDOW;
	si.wShowWindow = SW_SHOWNORMAL;

	wchar_t szCurDir[MAX_PATH+1] = L"";
	GetCurrentDirectory(countof(szCurDir), szCurDir);

	BOOL b;
	DWORD nErr = 0;


	b = CreateProcessDemoted(NULL, cmdNew, NULL, NULL, FALSE, NORMAL_PRIORITY_CLASS, NULL,
			szCurDir, &si, &pi, &nErr);


	if (b)
	{
		SafeCloseHandle(pi.hProcess);
		SafeCloseHandle(pi.hThread);
		return 0;
	}

	// Failed
	DisplayLastError(cmdNew, nErr);
	return 100;
}
else if (!klstricmp(curCommand, _T("/multi")))
{
	MultiConValue = true; MultiConPrm = true;
}
else if (!klstricmp(curCommand, _T("/nomulti")))
{
	MultiConValue = false; MultiConPrm = true;
}
else if (!klstricmp(curCommand, _T("/visible")))
{
	VisValue = true; VisPrm = true;
}
else if (!klstricmp(curCommand, _T("/ct")) || !klstricmp(curCommand, _T("/cleartype"))
	|| !klstricmp(curCommand, _T("/ct0")) || !klstricmp(curCommand, _T("/ct1")) || !klstricmp(curCommand, _T("/ct2")))
{
	ClearTypePrm = true;
	switch (curCommand[3])
	{
	case L'0':
		ClearTypeVal = NONANTIALIASED_QUALITY; break;
	case L'1':
		ClearTypeVal = ANTIALIASED_QUALITY; break;
	default:
		ClearTypeVal = CLEARTYPE_NATURAL_QUALITY;
	}
}
// имя шрифта
else if (!klstricmp(curCommand, _T("/font")) && i + 1 < params)
{
	curCommand += _tcslen(curCommand) + 1; i++;

	if (!FontPrm)
	{
		FontPrm = true;
		FontVal = curCommand;
	}
}
// Высота шрифта
else if (!klstricmp(curCommand, _T("/size")) && i + 1 < params)
{
	curCommand += _tcslen(curCommand) + 1; i++;

	if (!SizePrm)
	{
		SizePrm = true;
		SizeVal = klatoi(curCommand);
	}
}
#if 0
//120714 - аналогичные параметры работают в ConEmuC.exe, а в GUI они и не работали. убрал пока
else if (!klstricmp(curCommand, _T("/attach")) /*&& i + 1 < params*/)
{
	//curCommand += _tcslen(curCommand) + 1; i++;
	if (!AttachPrm)
	{
		AttachPrm = true; AttachVal = -1;

		if ((i + 1) < params)
		{
			TCHAR *nextCommand = curCommand + _tcslen(curCommand) + 1;

			if (*nextCommand != _T('/'))
			{
				curCommand = nextCommand; i++;
				AttachVal = klatoi(curCommand);
			}
		}

		// интеллектуальный аттач - если к текущей консоли уже подцеплена другая копия
		if (AttachVal == -1)
		{
			HWND hCon = GetForegroundWindow();

			if (!hCon)
			{
				// консоли нет
				return 100;
			}
			else
			{
				TCHAR sClass[128];

				if (GetClassName(hCon, sClass, 128))
				{
					if (_tcscmp(sClass, VirtualConsoleClassMain)==0)
					{
						// Сверху УЖЕ другая копия ConEmu
						return 1;
					}

					// Если на самом верху НЕ консоль - это может быть панель проводника,
					// или другое плавающее окошко... Поищем ВЕРХНЮЮ консоль
					if (isConsoleClass(sClass))
					{
						wcscpy_c(sClass, RealConsoleClass);
						hCon = FindWindow(RealConsoleClass, NULL);
						if (!hCon)
							hCon = FindWindow(WineConsoleClass, NULL);

						if (!hCon)
							return 100;
					}

					if (isConsoleClass(sClass))
					{
						// перебрать все ConEmu, может кто-то уже подцеплен?
						HWND hEmu = NULL;

						while ((hEmu = FindWindowEx(NULL, hEmu, VirtualConsoleClassMain, NULL)) != NULL)
						{
							if (hCon == (HWND)GetWindowLongPtr(hEmu, GWLP_USERDATA))
							{
								// к этой консоли уже подцеплен ConEmu
								return 1;
							}
						}
					}
					else
					{
						// верхнее окно - НЕ консоль
						return 100;
					}
				}
			}

			gpSetCls->hAttachConWnd = hCon;
		}
	}
}
#endif
// ADD fontname; by Mors
else if (!klstricmp(curCommand, _T("/fontfile")) && i + 1 < params)
{
	bool bOk = false; TCHAR* pszFile = NULL;
	if (!GetCfgParm(i, curCommand, bOk, pszFile, MAX_PATH))
	{
		return 100;
	}
	gpSetCls->RegisterFont(pszFile, TRUE);
}
// Register all fonts from specified directory
else if (!klstricmp(curCommand, _T("/fontdir")) && i + 1 < params)
{
	bool bOk = false; TCHAR* pszDir = NULL;
	if (!GetCfgParm(i, curCommand, bOk, pszDir, MAX_PATH))
	{
		return 100;
	}
	gpSetCls->RegisterFontsDir(pszDir);
}
else if (!klstricmp(curCommand, _T("/fs")))
{
	WindowModeVal = rFullScreen; WindowPrm = true;
}
else if (!klstricmp(curCommand, _T("/max")))
{
	WindowModeVal = rMaximized; WindowPrm = true;
}
else if (!klstricmp(curCommand, _T("/min"))
	|| !klstricmp(curCommand, _T("/mintsa"))
	|| !klstricmp(curCommand, _T("/starttsa")))
{
	gpConEmu->WindowStartMinimized = true;
	if (klstricmp(curCommand, _T("/min")) != 0)
	{
		gpConEmu->WindowStartTsa = true;
		gpConEmu->WindowStartNoClose = (klstricmp(curCommand, _T("/mintsa")) == 0);
	}
}
else if (!klstricmp(curCommand, _T("/tsa")) || !klstricmp(curCommand, _T("/tray")))
{
	gpConEmu->ForceMinimizeToTray = true;
}
else if (!klstricmp(curCommand, _T("/detached")))
{
	gpConEmu->mb_StartDetached = TRUE;
}
else if (!klstricmp(curCommand, _T("/noupdate")))
{
	gpConEmu->DisableAutoUpdate = true;
}
else if (!klstricmp(curCommand, _T("/nokeyhook"))
	|| !klstricmp(curCommand, _T("/nokeyhooks"))
	|| !klstricmp(curCommand, _T("/nokeybhook"))
	|| !klstricmp(curCommand, _T("/nokeybhooks")))
{
	gpConEmu->DisableKeybHooks = true;
}
else if (!klstricmp(curCommand, _T("/nocloseconfirm")))
{
	gpConEmu->DisableCloseConfirm = true;
}
else if (!klstricmp(curCommand, _T("/nomacro")))
{
	gpConEmu->DisableAllMacro = true;
}
else if (!klstricmp(curCommand, _T("/nohotkey"))
	|| !klstricmp(curCommand, _T("/nohotkeys")))
{
	gpConEmu->DisableAllHotkeys = true;
}
else if (!klstricmp(curCommand, _T("/nodeftrm"))
	|| !klstricmp(curCommand, _T("/nodefterm")))
{
	gpConEmu->DisableSetDefTerm = true;
}
else if (!klstricmp(curCommand, _T("/noregfont"))
	|| !klstricmp(curCommand, _T("/noregfonts")))
{
	gpConEmu->DisableRegisterFonts = true;
}
else if (!klstricmp(curCommand, _T("/inside"))
	|| !lstrcmpni(curCommand, _T("/inside="), 8))
{
	bool bRunAsAdmin = isPressed(VK_SHIFT);
	bool bSyncDir = false;
	LPCWSTR pszSyncFmt = NULL;

	if (curCommand[7] == _T('='))
	{
		bSyncDir = true;
		pszSyncFmt = curCommand+8; // \eCD /d %1 - \e - ESC, \b - BS, \n - ENTER, %1 - "dir", %2 - "bash dir"
	}

	CConEmuInside::InitInside(bRunAsAdmin, bSyncDir, pszSyncFmt, 0, NULL);
}
else if (!klstricmp(curCommand, _T("/insidepid")) && ((i + 1) < params))
{
	curCommand += _tcslen(curCommand) + 1; i++;

	bool bRunAsAdmin = isPressed(VK_SHIFT);

	wchar_t* pszEnd;
	// Здесь указывается PID, в который нужно внедриться.
	DWORD nInsideParentPID = wcstol(curCommand, &pszEnd, 10);
	if (nInsideParentPID)
	{
		CConEmuInside::InitInside(bRunAsAdmin, false, NULL, nInsideParentPID, NULL);
	}
}
else if (!klstricmp(curCommand, _T("/insidewnd")) && ((i + 1) < params))
{
	curCommand += _tcslen(curCommand) + 1; i++;
	if (curCommand[0] == L'0' && (curCommand[1] == L'x' || curCommand[1] == L'X'))
		curCommand += 2;
	else if (curCommand[0] == L'x' || curCommand[0] == L'X')
		curCommand ++;

	bool bRunAsAdmin = isPressed(VK_SHIFT);

	wchar_t* pszEnd;
	// Здесь указывается HWND, в котором нужно создаваться.
	HWND hParent = (HWND)wcstol(curCommand, &pszEnd, 16);
	if (hParent && IsWindow(hParent))
	{
		CConEmuInside::InitInside(bRunAsAdmin, false, NULL, 0, hParent);
	}
}
else if (!klstricmp(curCommand, _T("/icon")) && ((i + 1) < params))
{
	curCommand += _tcslen(curCommand) + 1; i++;

	if (!IconPrm && *curCommand)
	{
		IconPrm = true;
		gpConEmu->mps_IconPath = ExpandEnvStr(curCommand);
	}
}
else if (!klstricmp(curCommand, _T("/dir")) && i + 1 < params)
{
	curCommand += _tcslen(curCommand) + 1; i++;

	if (*curCommand)
	{
		// Например, "%USERPROFILE%"
		wchar_t* pszExpand = NULL;
		if (wcschr(curCommand, L'%') && ((pszExpand = ExpandEnvStr(curCommand)) != NULL))
		{
			gpConEmu->StoreWorkDir(pszExpand);
			SafeFree(pszExpand);
		}
		else
		{
			gpConEmu->StoreWorkDir(curCommand);
		}
	}
}
else if (!klstricmp(curCommand, _T("/updatejumplist")))
{
	// Copy current Task list to Win7 Jump list (Taskbar icon)
	gpConEmu->mb_UpdateJumpListOnStartup = true;
}
else if (!klstricmp(curCommand, L"/log") || !klstricmp(curCommand, L"/log0"))
{
	gpSetCls->isAdvLogging = 1;
}
else if (!klstricmp(curCommand, L"/log1") || !klstricmp(curCommand, L"/log2")
	|| !klstricmp(curCommand, L"/log3") || !klstricmp(curCommand, L"/log4"))
{
	gpSetCls->isAdvLogging = (BYTE)(curCommand[4] - L'0'); // 1..4
}
else if (!klstricmp(curCommand, _T("/single")))
{
	gpSetCls->SingleInstanceArg = sgl_Enabled;
}
else if (!klstricmp(curCommand, _T("/nosingle")))
{
	gpSetCls->SingleInstanceArg = sgl_Disabled;
}
else if (!klstricmp(curCommand, _T("/showhide")) || !klstricmp(curCommand, _T("/showhideTSA")))
{
	gpSetCls->SingleInstanceArg = sgl_Enabled;
	gpSetCls->SingleInstanceShowHide = !klstricmp(curCommand, _T("/showhide"))
		? sih_ShowMinimize : sih_ShowHideTSA;
}
else if (!klstricmp(curCommand, _T("/reset"))
	|| !klstricmp(curCommand, _T("/resetdefault"))
	|| !klstricmp(curCommand, _T("/basic")))
{
	ResetSettings = true;
	if (!klstricmp(curCommand, _T("/resetdefault")))
	{
		gpSetCls->isFastSetupDisabled = true;
	}
	else if (!klstricmp(curCommand, _T("/basic")))
	{
		gpSetCls->isFastSetupDisabled = true;
		gpSetCls->isResetBasicSettings = true;
	}
}
else if (!klstricmp(curCommand, _T("/nocascade"))
	|| !klstricmp(curCommand, _T("/dontcascade")))
{
	gpSetCls->isDontCascade = true;
}
else if ((!klstricmp(curCommand, _T("/Buffer")) || !klstricmp(curCommand, _T("/BufferHeight")))
	&& i + 1 < params)
{
	curCommand += _tcslen(curCommand) + 1; i++;

	if (!BufferHeightPrm)
	{
		BufferHeightPrm = true;
		BufferHeightVal = klatoi(curCommand);

		if (BufferHeightVal < 0)
		{
			//setParent = true; -- Maximus5 - нефиг, все ручками
			BufferHeightVal = -BufferHeightVal;
		}

		if (BufferHeightVal < LONGOUTPUTHEIGHT_MIN)
			BufferHeightVal = LONGOUTPUTHEIGHT_MIN;
		else if (BufferHeightVal > LONGOUTPUTHEIGHT_MAX)
			BufferHeightVal = LONGOUTPUTHEIGHT_MAX;
	}
}
else if (!klstricmp(curCommand, _T("/Config")) && i + 1 < params)
{
	// используем последний из параметров, если их несколько
	if (!GetCfgParm(i, curCommand, ConfigPrm, ConfigVal, 127))
	{
		return 100;
	}
}
else if (!klstricmp(curCommand, _T("/Palette")) && i + 1 < params)
{
	// используем последний из параметров, если их несколько
	if (!GetCfgParm(i, curCommand, PalettePrm, PaletteVal, MAX_PATH))
	{
		return 100;
	}
}
else if (!klstricmp(curCommand, _T("/LoadCfgFile")) && i + 1 < params)
{
	// используем последний из параметров, если их несколько
	if (!GetCfgParm(i, curCommand, LoadCfgFilePrm, LoadCfgFile, MAX_PATH, true))
	{
		return 100;
	}
}
else if (!klstricmp(curCommand, _T("/SaveCfgFile")) && i + 1 < params)
{
	// используем последний из параметров, если их несколько
	if (!GetCfgParm(i, curCommand, SaveCfgFilePrm, SaveCfgFile, MAX_PATH, true))
	{
		return 100;
	}
}
else if (!klstricmp(curCommand, _T("/UpdateSrcSet")) && i + 1 < params)
{
	// используем последний из параметров, если их несколько
	if (!GetCfgParm(i, curCommand, UpdateSrcSetPrm, UpdateSrcSet, MAX_PATH*4, false))
	{
		return 100;
	}
}
else if (!klstricmp(curCommand, _T("/AnsiLog")) && i + 1 < params)
{
	// используем последний из параметров, если их несколько
	if (!GetCfgParm(i, curCommand, AnsiLogPathPrm, AnsiLogPath, MAX_PATH-40, true))
	{
		return 100;
	}
}
else if (!klstricmp(curCommand, _T("/SetDefTerm")))
{
	SetUpDefaultTerminal = true;
}
else if (!klstricmp(curCommand, _T("/Exit")))
{
	ExitAfterActionPrm = true;
}
else if (!klstricmp(curCommand, _T("/Title")) && i + 1 < params)
{
	bool bOk = false; TCHAR* pszTitle = NULL;
	if (!GetCfgParm(i, curCommand, bOk, pszTitle, 127))
	{
		return 100;
	}
	gpConEmu->SetTitleTemplate(pszTitle);
}
else if (!klstricmp(curCommand, _T("/FindBugMode")))
{
	gpConEmu->mb_FindBugMode = true;
}
else if (!klstricmp(curCommand, _T("/?")) || !klstricmp(curCommand, _T("/h")) || !klstricmp(curCommand, _T("/help")))
{
	//MessageBox(NULL, pHelp, L"About ConEmu...", MB_ICONQUESTION);
	ConEmuAbout::OnInfo_About();
	free(cmdLine);
	return -1;
}
else
{
	lbNotFound = true;

	if (lbSwitchChanged)
	{
		*curCommand = L'-';
	}
}
}
