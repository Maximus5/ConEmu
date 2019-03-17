
/*
Copyright (c) 2016-present Maximus5
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

#include "Header.h"

#define DEBUGSTRFONT(s) DEBUGSTR(s)
#define DEBUGSTRSTARTUPLOG(s) DEBUGSTR(s)

//#include <commctrl.h>

#include "../common/shlobj.h"

//#ifdef __GNUC__
//#include "ShObjIdl_Part.h"
//#endif // __GNUC__

#include "../common/Monitors.h"
#include "../common/StartupEnvDef.h"
#include "../common/WUser.h"
#include "../ConEmuCD/ExitCodes.h"
#include "../ConEmuCD/GuiHooks.h"
#include "../ConEmuPlugin/FarDefaultMacros.h"
#include "AboutDlg.h"
#include "Background.h"
#include "CmdHistory.h"
#include "ConEmu.h"
#include "ConEmuApp.h"
#include "ConEmuCtrl.h"
#include "DefaultTerm.h"
#include "DlgItemHelper.h"
#include "DpiAware.h"
#include "DynDialog.h"
#include "Font.h"
#include "FontMgr.h"
#include "FontPtr.h"
#include "HotkeyDlg.h"
#include "ImgButton.h"
#include "Inside.h"
#include "LngRc.h"
#include "LoadImg.h"
#include "Options.h"
#include "OptionsClass.h"
#include "OptionsFast.h"
#include "OptionsHelp.h"
#include "RealConsole.h"
#include "Recreate.h"
#include "SearchCtrl.h"
#include "SetCmdTask.h"
#include "SetColorPalette.h"
#include "SetDlgLists.h"
#include "Status.h"
#include "TabBar.h"
#include "TrayIcon.h"
#include "VConChild.h"
#include "VConGroup.h"
#include "version.h"
#include "VirtualConsole.h"


const int CFontMgr::FontDefWidthMin = 0;
const int CFontMgr::FontDefWidthMax = 99;
const int CFontMgr::FontZoom100 = 10000;

const wchar_t CFontMgr::RASTER_FONTS_NAME[] = L"Raster Fonts";
SIZE CFontMgr::szRasterSizes[100] = {{0,0}}; // {{16,8},{6,9},{8,9},{5,12},{7,12},{8,12},{16,12},{12,16},{10,18}};


CFontMgr::CFontMgr()
{
	ResetFontWidth();
	mn_FontZoomValue = FontZoom100;
	memset(&LogFont, 0, sizeof(LogFont));
	mn_FontWidth = 0; mn_FontHeight = 16; // сброшено будет в SettingsLoaded
	mb_Name1Ok = mb_Name2Ok = false;
	mn_AutoFontWidth = mn_AutoFontHeight = -1;
	mb_StopRegisterFonts = FALSE;
}

CFontMgr::~CFontMgr()
{
	for (int i=0; i<MAX_FONT_STYLES; i++)
	{
		m_Font[i].Release();
	}

	TODO("Очистить m_Fonts[Idx].hFonts");

	int iRef = m_Font2.Release();
	_ASSERTE(iRef == 1); // Expected to be in m_FontPtrs

	while (!m_FontPtrs.empty())
	{
		CFont* p = NULL;
		m_FontPtrs.pop_back(p);
		if (p)
		{
			iRef = p->Release();
			_ASSERTE(iRef == 0);
		}
	}

	UNREFERENCED_PARAMETER(iRef);
}

void CFontMgr::FontPtrRegister(CFont* p)
{
	#ifdef _DEBUG
	_ASSERTE(isMainThread());

	for (INT_PTR i = 0; i < m_FontPtrs.size(); i++)
	{
		if (m_FontPtrs[i] == p)
		{
			_ASSERTE(FALSE && "Ptr must not be registered yet!");
			break;
		}
	}
	#endif

	m_FontPtrs.push_back(p);
}

void CFontMgr::FontPtrUnregister(CFont* p)
{
	if (!p)
	{
		_ASSERTE(p!=NULL);
		return;
	}

	_ASSERTE(isMainThread());
	for (INT_PTR i = 0; i < m_FontPtrs.size(); i++)
	{
		if (m_FontPtrs[i] == p)
		{
			m_FontPtrs.erase(i);
			break;
		}
	}
}

// Вызывается при включенном gpSet->isFontAutoSize: подгонка размера шрифта
// под размер окна, без изменения размера в символах
bool CFontMgr::AutoRecreateFont(int nFontW, int nFontH)
{
	if (mn_AutoFontWidth == nFontW && mn_AutoFontHeight == nFontH)
		return false; // ничего не делали

	if (gpSet->isLogging())
	{
		char szInfo[128]; sprintf_c(szInfo, "AutoRecreateFont(H=%i, W=%i)", nFontH, nFontW);
		CVConGroup::LogString(szInfo);
	}

	// Сразу запомним, какой размер просили в последний раз
	mn_AutoFontWidth = nFontW; mn_AutoFontHeight = nFontH;
	// Пытаемся создать новый шрифт
	CLogFont LF(LogFont);
	LF.lfWidth = nFontW;
	LF.lfHeight = nFontH;

	if (CreateFontGroup(LF))
	{
		// Запомнить размер шрифта (AutoFontWidth/Height - может быть другим, он запоминается выше)
		SaveFontSizes(false, true);

		// Передернуть флажки, что шрифт поменялся
		gpConEmu->Update(true);
		return true;
	}

	return false;
}

BYTE CFontMgr::BorderFontCharSet()
{
	CFontPtr font;
	if (QueryFont(fnt_Alternative, NULL, font))
		return font->m_tm.tmCharSet;
	return DEFAULT_CHARSET;
}

LPCWSTR CFontMgr::BorderFontFaceName()
{
	if (m_Font2.IsSet())
		return m_Font2->m_LF.lfFaceName;
	_ASSERTE(FALSE && "Border font was not created");
	return FontFaceName();
}

// Returns real pixels
LONG CFontMgr::BorderFontHeight()
{
	if (!gpSet->isFixFarBorders)
	{
		return FontHeight();
	}
	if (!m_Font2.IsSet() || (m_Font2->m_LF.lfHeight <= 0))
	{
		_ASSERTE(m_Font2.IsSet() && (m_Font2->m_LF.lfHeight > 0));
		return FontHeight();
	}
	return m_Font2->m_LF.lfHeight;
}

// Returns real pixels
LONG CFontMgr::BorderFontWidth()
{
	if (!gpSet->isFixFarBorders)
	{
		return FontCellWidth();
	}
	if (!m_Font2.IsSet() || (m_Font2->m_LF.lfWidth <= 0))
	{
		_ASSERTE(m_Font2.IsSet() && (m_Font2->m_LF.lfWidth > 0));
		return FontCellWidth();
	}
	return m_Font2->m_LF.lfWidth;
}

// Создать шрифт для отображения символов в диалоге плагина UCharMap
bool CFontMgr::CreateOtherFont(const wchar_t* asFontName, CFontPtr& rpFont)
{
	CLogFont otherLF(asFontName, LogFont.lfHeight, 0, FW_NORMAL, DEFAULT_CHARSET, LogFont.lfQuality);
	HFONT hf = CreateFontIndirect(&otherLF);
	if (hf)
		rpFont = new CFont(hf);
	else
		rpFont.Release();
	return (rpFont.IsSet());
}

// pVCon may be NULL
bool CFontMgr::QueryFont(CEFontStyles fontStyle, CVirtualConsole* pVCon, CFontPtr& rpFont)
{
	TODO("Take into account per-VCon font size zoom value");

	TODO("Optimization: Perhaps, we may not create all set of fonts, but");

	if ((fontStyle >= fnt_Normal) && (fontStyle < MAX_FONT_STYLES))
	{
		_ASSERTE(MAX_FONT_STYLES == (CEFontStyles)8);
		if (m_Font[fontStyle].IsSet())
		{
			rpFont = m_Font[fontStyle].Ptr();
		}
		else
		{
			_ASSERTE(FALSE && "Try to recteate the font?");
			rpFont.Release();
		}
		_ASSERTE(rpFont.IsSet()); // Fonts must be created already!
	}
	else if ((fontStyle == fnt_Alternative) && m_Font2.IsSet())
	{
		rpFont = m_Font2.Ptr();
	}
	else if ((fontStyle == fnt_UCharMap) && pVCon)
	{
		pVCon->GetUCharMapFontPtr(rpFont);
	}
	else
	{
		rpFont.Release();
	}

	if (!rpFont.IsSet())
	{
		rpFont = m_Font[fnt_Normal].Ptr();
		_ASSERTE(rpFont.IsSet()); // Fonts must be created already!
	}

	return (rpFont.IsSet());
}

// Do NOT take into account current zoom value here!
// This must not be used for BDF or raster fonts, the result may be wrong.
LONG CFontMgr::EvalFontHeight(LPCWSTR lfFaceName, LONG lfHeight, BYTE nFontCharSet)
{
	if (!lfHeight || !*lfFaceName)
	{
		_ASSERTE(lfHeight != 0);
		return 0;
	}

	LONG CellHeight = 0;
	for (INT_PTR i = 0; i < m_FontHeights.size(); i++)
	{
		const FontHeightInfo& f = m_FontHeights[i];
		if ((f.lfHeight != lfHeight) || (f.lfCharSet != nFontCharSet))
			continue;
		if (lstrcmp(lfFaceName, f.lfFaceName) != 0)
			continue;
		CellHeight = f.CellHeight;
		break;
	}

	if (!CellHeight)
	{
		FontHeightInfo fi = {};
		TEXTMETRIC tm = {};
		SIZE sz = {};
		LOGFONT lf = LogFont;
		lstrcpyn(lf.lfFaceName, lfFaceName, countof(lf.lfFaceName));
		lstrcpyn(fi.lfFaceName, lfFaceName, countof(fi.lfFaceName));
		fi.lfHeight = lf.lfHeight = lfHeight;
		lf.lfWidth = 0;
		fi.lfCharSet = lf.lfCharSet = nFontCharSet;
		lf.lfPitchAndFamily = DEFAULT_PITCH | FF_MODERN;

		HDC hdc = CreateCompatibleDC(NULL);
		if (hdc)
		{
			HFONT hOld, f = CreateFontIndirect(&lf);
			if (f)
			{
				hOld = (HFONT)SelectObject(hdc, f);
				if (GetTextMetrics(hdc, &tm) && (tm.tmHeight > 0))
				{
					CellHeight = tm.tmHeight + tm.tmExternalLeading;
				}
				else if (GetTextExtentPoint(hdc, L"Yy", 2, &sz) && (sz.cy > 0))
				{
					CellHeight = sz.cy;
				}
				SelectObject(hdc, hOld);
				DeleteObject(f);
			}
			DeleteDC(hdc);
		}

		if (!CellHeight)
		{
			// Still unknown?
			_ASSERTE(CellHeight != 0);
			CellHeight = abs(lfHeight);
		}

		fi.CellHeight = CellHeight;
		m_FontHeights.push_back(fi);
	}

	return CellHeight;
}

// Сюда приходят значения из gpSet->FontSize[Y|X] или заданные пользователем через макрос
// Функция должна учесть:
// * текущий dpi (FontUseDpi);
// * масштаб (пока нету);
// * FontUseUnits (положительный или отрицательный LF.lfHeight)
void CFontMgr::EvalLogfontSizes(LOGFONT& LF, LONG lfHeight, LONG lfWidth)
{
	if (lfHeight == 0)
	{
		_ASSERTE(lfHeight != 0);
		lfHeight = gpSet->FontUseUnits ? 12 : 16;
	}

	LF.lfHeight = gpSetCls->EvalSize(lfHeight, esf_Vertical|esf_CanUseUnits|esf_CanUseDpi|esf_CanUseZoom);
	LF.lfWidth  = lfWidth ? gpSetCls->EvalSize(lfWidth, esf_Horizontal|esf_CanUseDpi|esf_CanUseZoom) : 0;
}

BOOL CFontMgr::FontBold()
{
	return LogFont.lfWeight>400;
}

LONG CFontMgr::FontCellWidth()
{
	// В mn_FontWidth сохраняется ширина шрифта с учетом FontSizeX3, поэтому возвращаем
	return FontWidth();
}

BYTE CFontMgr::FontCharSet()
{
	return LogFont.lfCharSet;
}

BOOL CFontMgr::FontClearType()
{
	return (LogFont.lfQuality!=NONANTIALIASED_QUALITY);
}

LPCWSTR CFontMgr::FontFaceName()
{
	return LogFont.lfFaceName;
}

LONG CFontMgr::FontHeight()
{
	if (m_Font[0].IsSet())
	{
		// We set m_LF to real *positive* font height
		if (m_Font[0]->m_LF.lfHeight > 0)
		{
			return m_Font[0]->m_LF.lfHeight;
		}
		_ASSERTE((m_Font[0]->m_LF.lfHeight > 0) && "m_LF is expected to hold positive height only");
	}

	_ASSERTE(FALSE && "Font[0] must be created already");

	const LONG defFontHeight = 12;

	if (LogFont.lfHeight <= 0)
	{
		// Сюда мы должны попадать только для примерных расчетов во время старта!
		_ASSERTE(LogFont.lfHeight>0 || gpConEmu->GetStartupStage()<=CConEmuMain::ss_Starting);
		int iEvalHeight = 0;
		if (gpSet->FontSizeY)
		{
			iEvalHeight = gpSetCls->EvalSize(gpSet->FontSizeY, esf_Vertical|esf_CanUseUnits|esf_CanUseDpi|esf_CanUseZoom);
			if (iEvalHeight < 0)
				iEvalHeight = -iEvalHeight * 17 / 14;
		}
		if (iEvalHeight)
			return _abs(iEvalHeight);
		return defFontHeight;
	}

	_ASSERTE(mn_FontHeight==LogFont.lfHeight);
	if (mn_FontHeight == 0)
	{
		_ASSERTE(mn_FontHeight != 0);
		return defFontHeight;
	}
	return mn_FontHeight;
}

// Возможно скорректированный размер шрифта для выгрузки фрагмента в HTML
LONG CFontMgr::FontHeightHtml()
{
	if (!m_Font[0].IsSet())
	{
		_ASSERTE(m_Font[0].IsSet());  // -V571
		return FontHeight();
	}

	CLogFont LF((LOGFONT)(m_Font[0]->m_LF));
	if (LF.lfHeight <= 0)
	{
		_ASSERTE(LF.lfHeight>0);
		return FontHeight();
	}

	LPOUTLINETEXTMETRIC lpoutl = m_Font[0]->mp_otm;
	const TEXTMETRIC& tm = m_Font[0]->m_tm;

	int iHeight, iLineGap = 0;

	if (lpoutl && (lpoutl->otmrcFontBox.top > 0))
	{
		_ASSERTE(((lpoutl->otmrcFontBox.top * 1.3) >= LF.lfHeight) && (lpoutl->otmrcFontBox.top <= LF.lfHeight));
		iHeight = lpoutl->otmrcFontBox.top;
		if ((lpoutl->otmTextMetrics.tmInternalLeading < (iHeight/2)) && (lpoutl->otmTextMetrics.tmInternalLeading > 1))
			iLineGap = lpoutl->otmTextMetrics.tmInternalLeading - 1;
	}
	else
	{
		// Bitmap (raster) font?
		_ASSERTE(mn_FontHeight==LF.lfHeight);
		iHeight = mn_FontHeight;
		if ((tm.tmInternalLeading < (iHeight/2)) && (tm.tmInternalLeading > 1))
			iLineGap = tm.tmInternalLeading - 1;
	}

	return (iHeight - iLineGap);
}

BOOL CFontMgr::FontItalic()
{
	return LogFont.lfItalic!=0;
}

bool CFontMgr::FontMonospaced()
{
	if (!m_Font[0].IsSet())
	{
		_ASSERTE(FALSE && "Font must be created already");
		return true;
	}
	if (m_Font[0]->iType == CEFONT_CUSTOM)
	{
		// BDF fonts are always treated as monospaced
		return true;
	}
	return m_Font[0]->mb_Monospace;
}

BYTE CFontMgr::FontQuality()
{
	return LogFont.lfQuality;
}

LONG CFontMgr::FontWidth()
{
	if (m_Font[0].IsSet())
	{
		// We set m_LF to real *positive* font height
		if (m_Font[0]->m_LF.lfWidth > 0)
		{
			return m_Font[0]->m_LF.lfWidth;
		}
		_ASSERTE((m_Font[0]->m_LF.lfWidth > 0) && "m_LF is expected to hold real width value");
	}

	_ASSERTE(FALSE && "Font[0] must be created already");

	const LONG defFontWidth = 8;

	if (LogFont.lfWidth <= 0)
	{
		// Сюда мы должны попадать только для примерных расчетов во время старта!
		_ASSERTE(LogFont.lfWidth>0 || gpConEmu->GetStartupStage()<=CConEmuMain::ss_Starting);
		int iEvalWidth = FontHeight() * 10 / 18;
		if (iEvalWidth)
			return _abs(iEvalWidth);
		return defFontWidth;
	}

	_ASSERTE(mn_FontWidth==LogFont.lfWidth);
	if (mn_FontWidth <= 0)
	{
		_ASSERTE((int)mn_FontWidth > 0);
		return defFontWidth;
	}
	return mn_FontWidth;
}

BOOL CFontMgr::GetFontNameFromFile(LPCTSTR lpszFilePath, wchar_t (&rsFontName)[LF_FACESIZE], wchar_t (&rsFullFontName)[LF_FACESIZE])
{
	LPCTSTR pszDot = _tcsrchr(lpszFilePath, _T('.'));
	// Неизвестные расширения - пропускаем
	if (!pszDot)
		return FALSE;

	if (!lstrcmpi(pszDot, _T(".ttf")))
		return GetFontNameFromFile_TTF(lpszFilePath, rsFontName, rsFullFontName);

	if (!lstrcmpi(pszDot, _T(".otf")))
		return GetFontNameFromFile_OTF(lpszFilePath, rsFontName, rsFullFontName);

	if (!lstrcmpi(pszDot, _T(".bdf")))
		return GetFontNameFromFile_BDF(lpszFilePath, rsFontName, rsFullFontName);

	TODO("*.fon files");

	return FALSE;
}

#define SWAPWORD(x)		MAKEWORD(HIBYTE(x), LOBYTE(x))
#define SWAPLONG(x)		MAKELONG(SWAPWORD(HIWORD(x)), SWAPWORD(LOWORD(x)))

BOOL CFontMgr::GetFontNameFromFile_TTF(LPCTSTR lpszFilePath, wchar_t (&rsFontName)[LF_FACESIZE], wchar_t (&rsFullFontName)[LF_FACESIZE])
{
	struct TT_OFFSET_TABLE
	{
		USHORT	uMajorVersion;
		USHORT	uMinorVersion;
		USHORT	uNumOfTables;
		USHORT	uSearchRange;
		USHORT	uEntrySelector;
		USHORT	uRangeShift;
	};
	struct TT_TABLE_DIRECTORY
	{
		char	szTag[4];			//table name //-V112
		ULONG	uCheckSum;			//Check sum
		ULONG	uOffset;			//Offset from beginning of file
		ULONG	uLength;			//length of the table in bytes
	};
	struct TT_NAME_TABLE_HEADER
	{
		USHORT	uFSelector;			//format selector. Always 0
		USHORT	uNRCount;			//Name Records count
		USHORT	uStorageOffset;		//Offset for strings storage, from start of the table
	};
	struct TT_NAME_RECORD
	{
		USHORT	uPlatformID;
		USHORT	uEncodingID;
		USHORT	uLanguageID;
		USHORT	uNameID;
		USHORT	uStringLength;
		USHORT	uStringOffset;	//from start of storage area
	};

	BOOL lbRc = FALSE;
	HANDLE f = NULL;
	wchar_t szRetValA[MAX_PATH];
	wchar_t szRetValU[MAX_PATH];
	DWORD dwRead;
	TT_OFFSET_TABLE ttOffsetTable;
	TT_TABLE_DIRECTORY tblDir;
	BOOL bFound = FALSE;

	// Dump found table item
	LogString(lpszFilePath);
	bool bDumpTable = RELEASEDEBUGTEST((gpSet->isLogging()!=0),true);
	wchar_t szDumpInfo[200];

	//if (f.Open(lpszFilePath, CFile::modeRead|CFile::shareDenyWrite)){
	if ((f = CreateFile(lpszFilePath, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL)) == INVALID_HANDLE_VALUE)
		goto wrap;

	//f.Read(&ttOffsetTable, sizeof(TT_OFFSET_TABLE));
	if (!ReadFile(f, &ttOffsetTable, sizeof(ttOffsetTable), &(dwRead=0), NULL) || (dwRead != sizeof(ttOffsetTable)))
		goto wrap;

	ttOffsetTable.uNumOfTables = SWAPWORD(ttOffsetTable.uNumOfTables);
	ttOffsetTable.uMajorVersion = SWAPWORD(ttOffsetTable.uMajorVersion);
	ttOffsetTable.uMinorVersion = SWAPWORD(ttOffsetTable.uMinorVersion);

	//check is this is a true type font and the version is 1.0
	if (ttOffsetTable.uMajorVersion != 1 || ttOffsetTable.uMinorVersion != 0)
		goto wrap;


	for (int i = 0; i < ttOffsetTable.uNumOfTables; i++)
	{
		//f.Read(&tblDir, sizeof(TT_TABLE_DIRECTORY));
		if (ReadFile(f, &tblDir, sizeof(tblDir), &(dwRead=0), NULL) && dwRead)
		{
			if (lstrcmpni(tblDir.szTag, "name", 4) == 0) //-V112
			{
				bFound = TRUE;
				tblDir.uLength = SWAPLONG(tblDir.uLength);
				tblDir.uOffset = SWAPLONG(tblDir.uOffset);
				break;
			}
		}
	}

	if (bFound)
	{
		if (SetFilePointer(f, tblDir.uOffset, NULL, FILE_BEGIN)!=INVALID_SET_FILE_POINTER)
		{
			TT_NAME_TABLE_HEADER ttNTHeader;

			//f.Read(&ttNTHeader, sizeof(TT_NAME_TABLE_HEADER));
			if (ReadFile(f, &ttNTHeader, sizeof(ttNTHeader), &(dwRead=0), NULL) && dwRead)
			{
				ttNTHeader.uNRCount = SWAPWORD(ttNTHeader.uNRCount);
				ttNTHeader.uStorageOffset = SWAPWORD(ttNTHeader.uStorageOffset);
				TT_NAME_RECORD ttRecord;
				bFound = FALSE;

				for (int i = 0; i < ttNTHeader.uNRCount; i++)
				{
					//f.Read(&ttRecord, sizeof(TT_NAME_RECORD));
					if (ReadFile(f, &ttRecord, sizeof(ttRecord), &(dwRead=0), NULL) && dwRead)
					{
						ttRecord.uNameID = SWAPWORD(ttRecord.uNameID);

						if (ttRecord.uNameID == 1)
						{
							ttRecord.uStringLength = SWAPWORD(ttRecord.uStringLength);
							ttRecord.uStringOffset = SWAPWORD(ttRecord.uStringOffset);
							//int nPos = f.GetPosition();
							DWORD nPos = SetFilePointer(f, 0, 0, FILE_CURRENT);

							//f.Seek(tblDir.uOffset + ttRecord.uStringOffset + ttNTHeader.uStorageOffset, CFile::begin);
							if (SetFilePointer(f, tblDir.uOffset + ttRecord.uStringOffset + ttNTHeader.uStorageOffset, 0, FILE_BEGIN)!=INVALID_SET_FILE_POINTER)
							{
								if (ttRecord.uStringLength <= (LF_FACESIZE * sizeof(wchar_t)))
								{
									//f.Read(csTemp.GetBuffer(ttRecord.uStringLength + 1), ttRecord.uStringLength);
									//csTemp.ReleaseBuffer();
									char szName[(LF_FACESIZE+3)*sizeof(wchar_t)] = "";

									if (ReadFile(f, szName, ttRecord.uStringLength + sizeof(wchar_t), &(dwRead=0), NULL) && dwRead)
									{
										// Ensure even wchar_t would be Z-terminated
										szName[ttRecord.uStringLength] = 0; szName[ttRecord.uStringLength+1] = 0; szName[ttRecord.uStringLength+2] = 0;
										LPCWSTR pszFound = NULL;

										// Dump found table item
										if (bDumpTable)
										{
											swprintf_c(szDumpInfo, L"  Platf: %u Enc: %u Lang: %u Len: %u \"", ttRecord.uPlatformID, ttRecord.uEncodingID, ttRecord.uLanguageID, ttRecord.uStringLength);
											int iLen = lstrlen(szDumpInfo);
											for (DWORD i = 0; i < dwRead; i++)
											{
												szDumpInfo[iLen++] = (szName[i]) ? (wchar_t)szName[i] : L'.';
											}
											szDumpInfo[iLen++] = L'\"';
											szDumpInfo[iLen] = 0;
											//gpConEmu->LogString(szDumpInfo); -- below
										}

										// Process read item
										if ((
											((ttRecord.uPlatformID == 768) && (ttRecord.uEncodingID == 256))
											|| ((ttRecord.uPlatformID == 0) && ((ttRecord.uEncodingID == 0) || (ttRecord.uEncodingID == 768)))
											) && ((wchar_t*)szName)[0])
										{
											// Seems like it's a 1201 │ UTF-16 (Big endian)
											int j = 0;
											while (j < ttRecord.uStringLength)
											{
												szRetValU[j] = SWAPWORD(((wchar_t*)szName)[j]);
												j++;
											}
											szRetValU[j] = 0;
											if (szRetValU[0])
											{
												lbRc = TRUE;
												pszFound = szRetValU;
											}
										}
										else if ((
											((ttRecord.uPlatformID == 256) && (ttRecord.uEncodingID == 0))
											) && szName[0])
										{
											for (int j = ttRecord.uStringLength; j >= 0 && szName[j] == ' '; j--)
												szName[j] = 0;
											szName[ttRecord.uStringLength] = 0;

											if (szName[0])
											{
												MultiByteToWideChar(CP_ACP, 0, szName, -1, szRetValA, LF_FACESIZE+1);
												pszFound = szRetValA;
												lbRc = TRUE;
											}

											//break; -- continue, may be Unicode name may be found
										}

										if (bDumpTable)
										{
											if (pszFound)
											{
												wcscat_c(szDumpInfo, L" >> \"");
												wcscat_c(szDumpInfo, pszFound);
												wcscat_c(szDumpInfo, L"\"");
											}
											else
											{
												wcscat_c(szDumpInfo, L" - UNKNOWN FORMAT");
											}
											gpConEmu->LogString(szDumpInfo);
										}
									}
								}
							}

							//f.Seek(nPos, CFile::begin);
							SetFilePointer(f, nPos, 0, FILE_BEGIN);
						}
					}
				} // for (int i = 0; i < ttNTHeader.uNRCount; i++)
			}
		}
	}

	if (lbRc)
	{
		wcscpy_c(rsFontName, *szRetValA ? szRetValA : szRetValU);
		wcscpy_c(rsFullFontName, *szRetValU ? szRetValU : szRetValA);
	}

wrap:
	if (f && (f != INVALID_HANDLE_VALUE))
		CloseHandle(f);
	return lbRc;
}

// Retrieve Family name from OTF file
BOOL CFontMgr::GetFontNameFromFile_OTF(LPCTSTR lpszFilePath, wchar_t (&rsFontName)[LF_FACESIZE], wchar_t (&rsFullFontName)[LF_FACESIZE])
{
	struct OTF_ROOT
	{
		char  szTag[4]; // 0x00010000 или 'OTTO'
		WORD  NumTables;
		WORD  SearchRange;
		WORD  EntrySelector;
		WORD  RangeShift;
	};
	struct OTF_TABLE
	{
		char  szTag[4]; // нас интересует только 'name'
		DWORD CheckSum;
		DWORD Offset; // <== начало таблицы, от начала файла
		DWORD Length;
	};
	struct OTF_NAME_TABLE
	{
		WORD  Format; // = 0
		WORD  Count;
		WORD  StringOffset; // <== начало строк, в байтах, от начала таблицы
	};
	struct OTF_NAME_RECORD
	{
		WORD  PlatformID;
		WORD  EncodingID;
		WORD  LanguageID;
		WORD  NameID; // нас интересует 4=Full font name, или (1+' '+2)=(Font Family name + Font Subfamily name)
		WORD  Length; // in BYTES
		WORD  Offset; // in BYTES from start of storage area
	};

	//-- можно вернуть так, чтобы "по тихому" пропустить этот файл
	//rsFontName[0] = 1;
	//rsFontName[1] = 0;


	BOOL lbRc = FALSE;
	HANDLE f = NULL;
	wchar_t szFullName[MAX_PATH] = {}, szName[128] = {}, szSubName[128] = {};
	char szData[MAX_PATH] = {};
	int iFullOffset = -1, iFamOffset = -1, iSubFamOffset = -1;
	int iFullLength = -1, iFamLength = -1, iSubFamLength = -1;
	DWORD dwRead;
	OTF_ROOT root;
	OTF_TABLE tbl;
	OTF_NAME_TABLE nam;
	OTF_NAME_RECORD namRec;
	BOOL bFound = FALSE;

	if ((f = CreateFile(lpszFilePath, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL)) == INVALID_HANDLE_VALUE)
		goto wrap;

	//f.Read(&ttOffsetTable, sizeof(TT_OFFSET_TABLE));
	if (!ReadFile(f, &root, sizeof(root), &(dwRead=0), NULL) || (dwRead != sizeof(root)))
		goto wrap;

	root.NumTables = SWAPWORD(root.NumTables);

	if (lstrcmpni(root.szTag, "OTTO", 4) != 0) //-V112
		goto wrap; // Не поддерживается


	for (WORD i = 0; i < root.NumTables; i++)
	{
		//f.Read(&tblDir, sizeof(TT_TABLE_DIRECTORY));
		if (ReadFile(f, &tbl, sizeof(tbl), &(dwRead=0), NULL) && dwRead)
		{
			if (lstrcmpni(tbl.szTag, "name", 4) == 0) //-V112
			{
				bFound = TRUE;
				tbl.Length = SWAPLONG(tbl.Length);
				tbl.Offset = SWAPLONG(tbl.Offset);
				break;
			}
		}
	}

	if (bFound)
	{
		if (SetFilePointer(f, tbl.Offset, NULL, FILE_BEGIN)!=INVALID_SET_FILE_POINTER)
		{
			if (ReadFile(f, &nam, sizeof(nam), &(dwRead=0), NULL) && dwRead)
			{
				nam.Format = SWAPWORD(nam.Format);
				nam.Count = SWAPWORD(nam.Count);
				nam.StringOffset = SWAPWORD(nam.StringOffset);
				if (nam.Format != 0 || !nam.Count)
					goto wrap; // Неизвестный формат

				bFound = FALSE;

				for (int i = 0; i < nam.Count; i++)
				{
					if (ReadFile(f, &namRec, sizeof(namRec), &(dwRead=0), NULL) && dwRead)
					{
						namRec.NameID = SWAPWORD(namRec.NameID);
						namRec.Offset = SWAPWORD(namRec.Offset);
						namRec.Length = SWAPWORD(namRec.Length);

						switch (namRec.NameID)
						{
						case 1:
							iFamOffset = namRec.Offset;
							iFamLength = namRec.Length;
							break;
						case 2:
							iSubFamOffset = namRec.Offset;
							iSubFamLength = namRec.Length;
							break;
						case 4:
							iFullOffset = namRec.Offset;
							iFullLength = namRec.Length;
							break;
						}
					}

					if (iFamOffset != -1 && iSubFamOffset != -1 && iFullOffset != -1)
						break;
				}

				for (int n = 0; n < 3; n++)
				{
					int iOffset, iLen;
					switch (n)
					{
					case 0:
						if (iFullOffset == -1)
							continue;
						iOffset = iFullOffset; iLen = iFullLength;
						break;
					case 1:
						if (iFamOffset == -1)
							continue;
						iOffset = iFamOffset; iLen = iFamLength;
						break;
					//case 2:
					default:
						if (iSubFamOffset == -1)
							continue;
						iOffset = iSubFamOffset; iLen = iSubFamLength;
						//break;
					}
					if (SetFilePointer(f, tbl.Offset + nam.StringOffset + iOffset, NULL, FILE_BEGIN)==INVALID_SET_FILE_POINTER)
						break;
					if (iLen >= (int)sizeof(szData))
					{
						_ASSERTE(iLen < (int)sizeof(szData));
						iLen = sizeof(szData)-1;
					}
					if (!ReadFile(f, szData, iLen, &(dwRead=0), NULL) || (dwRead != (DWORD)iLen))
						break;

					switch (n)
					{
					case 0:
						MultiByteToWideChar(CP_ACP, 0, szData, iLen, szFullName, countof(szFullName)-1);
						lbRc = TRUE;
						break;
					case 1:
						MultiByteToWideChar(CP_ACP, 0, szData, iLen, szName, countof(szName)-1);
						lbRc = TRUE;
						break;
					case 2:
						MultiByteToWideChar(CP_ACP, 0, szData, iLen, szSubName, countof(szSubName)-1);
						break;
					}
				}
			}
		}
	}

	if (lbRc)
	{
		if (!*szFullName)
		{
			// Если полное имя в файле не указано - сформируем сами
			wcscpy_c(szFullName, szName);
			if (*szSubName)
			{
				wcscat_c(szFullName, L" ");
				wcscat_c(szFullName, szSubName);
			}
		}

		szFullName[LF_FACESIZE-1] = 0;
		szName[LF_FACESIZE-1] = 0;

		if (szName[0] != 0)
		{
			wcscpy_c(rsFontName, *szName ? szName : szFullName);
		}

		if (szFullName[0] != 0)
		{
			wcscpy_c(rsFullFontName, szFullName);
		}
		else
		{
			_ASSERTE(szFullName[0] != 0);
			lbRc = FALSE;
		}
	}

wrap:
	if (f && (f != INVALID_HANDLE_VALUE))
		CloseHandle(f);
	return lbRc;
}

// Retrieve Family name from BDF file
BOOL CFontMgr::GetFontNameFromFile_BDF(LPCTSTR lpszFilePath, wchar_t (&rsFontName)[LF_FACESIZE], wchar_t (&rsFullFontName)[LF_FACESIZE])
{
	if (!BDF_GetFamilyName(lpszFilePath, rsFontName))
		return FALSE;
	wcscat_c(rsFontName, CE_BDF_SUFFIX/*L" [BDF]"*/);
	lstrcpy(rsFullFontName, rsFontName);
	return TRUE;
}

void CFontMgr::GetMainLogFont(LOGFONT& lf)
{
	lf = LogFont;
}

// в процентах (false) или mn_FontZoomValue (true)
LONG CFontMgr::GetZoom(bool bRaw /*= false*/)
{
	return bRaw ? mn_FontZoomValue : MulDiv(mn_FontZoomValue, 100, FontZoom100);
}

void CFontMgr::InitFont(LPCWSTR asFontName/*=NULL*/, int anFontHeight/*=-1*/, int anQuality/*=-1*/)
{
	lstrcpyn(LogFont.lfFaceName, (asFontName && *asFontName) ? asFontName : (*gpSet->inFont) ? gpSet->inFont : gsDefGuiFont, countof(LogFont.lfFaceName));
	if ((asFontName && *asFontName) || *gpSet->inFont)
		mb_Name1Ok = TRUE;

	if (anFontHeight && (anFontHeight != -1))
	{
		/*
		LogFont.lfHeight = mn_FontHeight = anFontHeight;
		LogFont.lfWidth = mn_FontWidth = 0;
		*/
		EvalLogfontSizes(LogFont, anFontHeight, 0);
	}
	else
	{
		/*
		LogFont.lfHeight = mn_FontHeight = gpSet->FontSizeY;
		LogFont.lfWidth = mn_FontWidth = gpSet->FontSizeX;
		*/
		EvalLogfontSizes(LogFont, gpSet->FontSizeY, gpSet->FontSizeX);
	}

	_ASSERTE(anQuality==-1 || anQuality==NONANTIALIASED_QUALITY || anQuality==ANTIALIASED_QUALITY || anQuality==CLEARTYPE_NATURAL_QUALITY);

	_ASSERTE(gpSet->mn_AntiAlias==NONANTIALIASED_QUALITY || gpSet->mn_AntiAlias==ANTIALIASED_QUALITY || gpSet->mn_AntiAlias==CLEARTYPE_NATURAL_QUALITY);
	LogFont.lfQuality = (anQuality!=-1) ? anQuality : gpSet->mn_AntiAlias;
	LogFont.lfWeight = gpSet->isBold ? FW_BOLD : FW_NORMAL;
	LogFont.lfCharSet = gpSet->mn_LoadFontCharSet;
	LogFont.lfItalic = gpSet->isItalic;

	if (*gpSet->inFont2)
		mb_Name2Ok = TRUE;

	//std::vector<RegFont>::iterator iter;

	if (!mb_Name1Ok)
	{
		for (int i = 0; !mb_Name1Ok && (i < 3); i++)
		{
			//for (iter = m_RegFonts.begin(); iter != m_RegFonts.end(); ++iter)
			for (INT_PTR j = 0; j < m_RegFonts.size(); ++j)
			{
				const RegFont* iter = &(m_RegFonts[j]);

				switch (i)
				{
					case 0:

						if (!iter->bDefault || !iter->bUnicode) continue;

						break;
					case 1:

						if (!iter->bDefault) continue;

						break;
					case 2:

						if (!iter->bUnicode) continue;

						break;
					default:
						break;
				}

				lstrcpynW(LogFont.lfFaceName, iter->szFontName, countof(LogFont.lfFaceName));
				mb_Name1Ok = TRUE;
				break;
			}
		}
	}

	if (!mb_Name2Ok)
	{
		//for (iter = m_RegFonts.begin(); iter != m_RegFonts.end(); ++iter)
		for (INT_PTR j = 0; j < m_RegFonts.size(); ++j)
		{
			const RegFont* iter = &(m_RegFonts[j]);

			if (iter->bHasBorders)
			{
				lstrcpynW(gpSet->inFont2, iter->szFontName, countof(gpSet->inFont2));
				mb_Name2Ok = TRUE;
				break;
			}
		}
	}

	if (!CreateFontGroup(LogFont))
	{
		// The font must be created, otherwise we can't draw anything
		Assert(FALSE && "Can't create main font");
	}

	//2009-06-07 Реальный размер созданного шрифта мог измениться
	SaveFontSizes((mn_AutoFontWidth == -1), false);

	MCHKHEAP
}

void CFontMgr::MacroFontSetName(LPCWSTR pszFontName, WORD anHeight /*= 0*/, WORD anWidth /*= 0*/)
{
	CLogFont LF(LogFont);
	if (pszFontName && *pszFontName)
		wcscpy_c(LF.lfFaceName, pszFontName);
	if (anHeight)
	{
		LF.lfHeight = anHeight;
		LF.lfWidth = anWidth;
	}
	else
	{
		LF.lfWidth = 0;
	}

	if (gpSet->isLogging())
	{
		char szInfo[128]; sprintf_c(szInfo, "MacroFontSetName('%s', H=%i, W=%i)", LF.lfFaceName, LF.lfHeight, LF.lfWidth);
		CVConGroup::LogString(szInfo);
	}

	if (CreateFontGroup(LF))
	{
		// SaveFontSizes выполним после обновления LogFont, т.к. там зовется gpConEmu->OnPanelViewSettingsChanged
		SaveFontSizes((mn_AutoFontWidth == -1), true);

		gpConEmu->Update(true);

		if (gpConEmu->GetWindowMode() == wmNormal)
			CVConGroup::SyncWindowToConsole(); // -- функция пустая, игнорируется
		else
			CVConGroup::SyncConsoleToWindow();

		gpConEmu->ReSize();
	}

	if (ghOpWnd)
	{
		wchar_t szSize[10];
		swprintf_c(szSize, L"%i", gpSet->FontSizeY);
		SetDlgItemText(gpSetCls->GetPage(thi_Fonts), tFontSizeY, szSize);
		gpSetCls->UpdateFontInfo();
	}

	gpConEmu->OnPanelViewSettingsChanged(TRUE);
}

// Вызов из GUI-макросов - увеличить/уменьшить шрифт, без изменения размера (в пикселях) окна
// Функция НЕ меняет высоту шрифта настройки и изменения не будут сохранены в xml/reg
// Здесь меняется только значение "зума"
bool CFontMgr::MacroFontSetSize(int nRelative/*0/1/2/3*/, int nValue/*+-1,+-2,... | 100%*/)
{
	wchar_t szLog[128];
	if (gpSet->isLogging())
	{
		swprintf_c(szLog, L"MacroFontSetSize(%i,%i)", nRelative, nValue);
		gpConEmu->LogString(szLog);
	}

	// Validation
	if (nRelative == 0)
	{
		// Absolute height
		if (nValue < 5)
		{
			gpConEmu->LogString(L"-- Skipped! Absolute value less than 5");
			return false;
		}
	}
	else if (nRelative == 1)
	{
		// Relative
		if (nValue == 0)
		{
			gpConEmu->LogString(L"-- Skipped! Relative value is zero");
			return false;
		}
	}
	else if ((nRelative == 2) || (nRelative == 3))
	{
		// Zoom value
		if (nValue < 1)
		{
			gpConEmu->LogString(L"-- Skipped! Zoom value is too small");
			return false;
		}
	}
	else
	{
		_ASSERTE(FALSE && "Invalid nRelative value");
		gpConEmu->LogString(L"-- Skipped! Unsupported nRelative value");
		return false;
	}

	int nNewHeight = LogFont.lfHeight; // Issue 1130
	bool bWasNotZoom100 = (mn_FontZoomValue != FontZoom100);
	CLogFont LF;
	CLogFont LF_PREV(LogFont);

	for (int nRetry = 0; nRetry < 10; nRetry++)
	{
		// The function would copy LogFont to LF and update LF's Height and Width
		if (!MacroFontSetSizeInt(LF, nRelative/*0/1/2*/, nValue/*+-1,+-2,... | 100%*/))
		{
			break;
		}

		// Не должен стать менее 5 пунктов
		if (abs(LF.lfHeight) < 5)
		{
			gpConEmu->LogString(L"-- Failed! Created font height less than 5");
			return false;
		}

		// Успешно, только если:
		// шрифт изменился
		// или хотели поставить абсолютный размер
		// или был масштаб НЕ 100%, а стал 100% (гарантированный возврат к оригиналу)

		// Смысл в том, чтобы не пересоздавать все MAX_FONT_STYLES шрифтов, если размер "не подходит"
		CFontPtr font;

		// First - check if font creation is available for the size
		if (Create(LF, font)
			&& ((nRelative != 1)
				|| (font->m_LF.lfHeight != LF_PREV.lfHeight)
				|| (!bWasNotZoom100 && (mn_FontZoomValue == FontZoom100)))
			// Now we may create full group of fonts
			&& CreateFontGroup(LF))
		{
			// SaveFontSizes выполним после обновления LogFont, т.к. там зовется gpConEmu->OnPanelViewSettingsChanged

			// Запомнить размер шрифта (AutoFontWidth/Height - может быть другим, он запоминается выше)
			SaveFontSizes(false, true);

			// Передернуть размер консоли
			gpConEmu->OnSize();
			// Передернуть флажки, что шрифт поменялся
			gpConEmu->Update(true);

			if (ghOpWnd)
			{
				gpSetCls->UpdateFontInfo();

				HWND hMainPg = gpSetCls->GetPage(thi_Fonts);
				if (hMainPg)
				{
					wchar_t temp[16];
					swprintf_c(temp, L"%i", gpSet->FontSizeY);
					CSetDlgLists::SelectStringExact(hMainPg, tFontSizeY, temp);
					swprintf_c(temp, L"%i", gpSet->FontSizeX);
					CSetDlgLists::SelectStringExact(hMainPg, tFontSizeX, temp);
				}
			}

			if (gpConEmu->mp_Status)
				gpConEmu->mp_Status->UpdateStatusBar(true);

			swprintf_c(szLog, L"-- Succeeded! New font {'%s',%i,%i} was created", font->m_LF.lfFaceName, font->m_LF.lfHeight, font->m_LF.lfWidth, font->m_LF.lfHeight, font->m_LF.lfWidth);
			gpConEmu->LogString(szLog);

			return true;
		}

		if (nRelative != 1)
		{
			_ASSERTE(FALSE && "Font creation failed?");
			gpConEmu->LogString(L"-- Failed? (nRelative==0)?");
			return false;
		}

		// Если пытаются изменить относительный размер, а шрифт не создался - попробовать следующий размер
		nValue += (nValue > 0) ? 1 : -1;
	}

	swprintf_c(szLog, L"-- Failed! New font {'%s',%i,%i} was not created", LF.lfFaceName, LF.lfHeight, LF.lfWidth, LF.lfHeight, LF.lfWidth);
	gpConEmu->LogString(szLog);

	return false;
}

// Till now prcSuggested is not used
// However it may be needed for 'Auto sized' fonts
bool CFontMgr::RecreateFontByDpi(int dpiX, int dpiY, LPRECT prcSuggested)
{
	if ((_dpi_font.Xdpi == dpiX && _dpi_font.Ydpi == dpiY) || (dpiY < 72) || (dpiY > 960))
	{
		_ASSERTE(dpiY >= 72 && dpiY <= 960);
		return false;
	}

	gpSetCls->SetRequestedDpi(dpiX, dpiY);
	_dpi_font.SetDpi(dpiX, dpiY);
	//Raster fonts???
	EvalLogfontSizes(LogFont, gpSet->FontSizeY, gpSet->FontSizeX);
	RecreateFont(true);

	return true;
}

// asFontFile may come from: ConEmu /fontfile <path-to-font-file>
// or from RegisterFontsDir when enumerating font folder...
BOOL CFontMgr::RegisterFont(LPCWSTR asFontFile, BOOL abDefault)
{
	// Обработка параметра /fontfile
	_ASSERTE(asFontFile && *asFontFile);

	if (mb_StopRegisterFonts) return FALSE;

	//for (std::vector<RegFont>::iterator iter = m_RegFonts.begin(); iter != m_RegFonts.end(); ++iter)
	for (INT_PTR j = 0; j < m_RegFonts.size(); ++j)
	{
		RegFont* iter = &(m_RegFonts[j]);

		if (StrCmpI(iter->szFontFile, asFontFile) == 0)
		{
			// Уже добавлено
			if (abDefault && iter->bDefault == FALSE)
				iter->bDefault = TRUE;

			return TRUE;
		}
	}

	RegFont rf = {abDefault};
	wchar_t szFullFontName[LF_FACESIZE] = L"";

	if (!GetFontNameFromFile(asFontFile, rf.szFontName, szFullFontName))
	{
		//DWORD dwErr = GetLastError();
		size_t cchLen = _tcslen(asFontFile)+100;
		wchar_t* psz=(wchar_t*)calloc(cchLen,sizeof(wchar_t));
		_wcscpy_c(psz, cchLen, L"Can't retrieve font family from file:\n");
		_wcscat_c(psz, cchLen, asFontFile);
		_wcscat_c(psz, cchLen, L"\nContinue?");
		int nBtn = MsgBox(psz, MB_OKCANCEL|MB_ICONSTOP, gpConEmu->GetDefaultTitle());
		free(psz);

		if (nBtn == IDCANCEL)
		{
			mb_StopRegisterFonts = TRUE;
			return FALSE;
		}

		return TRUE; // продолжить со следующим файлом
	}
	else if (rf.szFontName[0] == 1 && rf.szFontName[1] == 0)
	{
		return TRUE;
	}

	// Проверить, может такой шрифт уже зарегистрирован в системе
	BOOL lbRegistered = FALSE, lbOneOfFam = FALSE; int iFamIndex = -1;

	//for (std::vector<RegFont>::iterator iter = m_RegFonts.begin(); iter != m_RegFonts.end(); ++iter)
	for (INT_PTR j = 0; j < m_RegFonts.size(); ++j)
	{
		const RegFont* iter = &(m_RegFonts[j]);

		// Это может быть другой тип шрифта (Liberation Mono Bold, Liberation Mono Regular, ...)
		if (lstrcmpi(iter->szFontName, rf.szFontName) == 0
			|| lstrcmpi(iter->szFontName, szFullFontName) == 0)
		{
			lbRegistered = iter->bAlreadyInSystem;
			lbOneOfFam = TRUE;
			//iFamIndex = iter - m_RegFonts.begin();
			iFamIndex = j;
			break;
		}
	}

	if (!lbOneOfFam)
	{
		// Проверяем, может в системе уже зарегистрирован такой шрифт?
		LOGFONT LF = {0};
		LF.lfOutPrecision = OUT_TT_PRECIS; LF.lfClipPrecision = CLIP_DEFAULT_PRECIS; LF.lfPitchAndFamily = FIXED_PITCH | FF_MODERN;
		wcscpy_c(LF.lfFaceName, rf.szFontName); LF.lfHeight = 10; LF.lfWeight = FW_NORMAL;
		HFONT hf = CreateFontIndirect(&LF);
		BOOL lbFail = FALSE;

		if (hf)
		{
			LPOUTLINETEXTMETRICW lpOutl = LoadOutline(NULL, hf);

			if (lpOutl)
			{
				if (lstrcmpi((wchar_t*)lpOutl->otmpFamilyName, rf.szFontName) == 0)
					lbRegistered = TRUE;
				else
					lbFail = TRUE;

				free(lpOutl);
			}

			DeleteObject(hf);
		}

		if ((!hf || lbFail) && (lstrcmp(rf.szFontName, szFullFontName) != 0))
		{
			wcscpy_c(LF.lfFaceName, szFullFontName);
			hf = CreateFontIndirect(&LF);
			LPOUTLINETEXTMETRICW lpOutl = LoadOutline(NULL, hf);

			if (lpOutl)
			{
				if (lstrcmpi((wchar_t*)lpOutl->otmpFamilyName, rf.szFontName) == 0)
				{
					// Таки создавать нужно по полному имени
					wcscpy_c(rf.szFontName, szFullFontName);
					lbRegistered = TRUE;
				}

				free(lpOutl);
			}

			DeleteObject(hf);
		}
	}

	// Запомним, что такое имя шрифта в системе уже есть, но зарегистрируем. Может в этом файле какие-то модификации...
	rf.bAlreadyInSystem = lbRegistered;
	wcscpy_c(rf.szFontFile, asFontFile);

	LPCTSTR pszDot = _tcsrchr(asFontFile, _T('.'));
	if (pszDot && lstrcmpi(pszDot, _T(".bdf"))==0)
	{
		WARNING("Не загружать шрифт полностью - только имена/заголовок, а то слишком накладно по времени. Загружать при первом вызове.");
		CustomFont* pFont = BDF_Load(asFontFile);
		if (!pFont)
		{
			size_t cchLen = _tcslen(asFontFile)+100;
			wchar_t* psz=(wchar_t*)calloc(cchLen,sizeof(wchar_t));
			_wcscpy_c(psz, cchLen, L"Can't load BDF font:\n");
			_wcscat_c(psz, cchLen, asFontFile);
			_wcscat_c(psz, cchLen, L"\nContinue?");
			int nBtn = MsgBox(psz, MB_OKCANCEL|MB_ICONSTOP, gpConEmu->GetDefaultTitle());
			free(psz);

			if (nBtn == IDCANCEL)
			{
				mb_StopRegisterFonts = TRUE;
				return FALSE;
			}

			return TRUE; // продолжить со следующим файлом
		}

		if (lbOneOfFam)
		{
			// Добавим в существующее семейство
			_ASSERTE(iFamIndex >= 0);
			m_RegFonts[iFamIndex].pCustom->AddFont(pFont);
			return TRUE;
		}

		rf.pCustom = new CustomFontFamily();
		rf.pCustom->AddFont(pFont);
		rf.bUnicode = pFont->HasUnicode();
		rf.bHasBorders = pFont->HasBorders();

		// Запомнить шрифт
		m_RegFonts.push_back(rf);
		return TRUE;
	}

	if (!AddFontResourceEx(asFontFile, FR_PRIVATE, NULL))  //ADD fontname; by Mors
	{
		size_t cchLen = _tcslen(asFontFile)+100;
		wchar_t* psz=(wchar_t*)calloc(cchLen,sizeof(wchar_t));
		_wcscpy_c(psz, cchLen, L"Can't register font:\n");
		_wcscat_c(psz, cchLen, asFontFile);
		_wcscat_c(psz, cchLen, L"\nContinue?");
		int nBtn = MsgBox(psz, MB_OKCANCEL|MB_ICONSTOP, gpConEmu->GetDefaultTitle());
		free(psz);

		if (nBtn == IDCANCEL)
		{
			mb_StopRegisterFonts = TRUE;
			return FALSE;
		}

		return TRUE; // продолжить со следующим файлом
	}

	// Теперь его нужно добавить в вектор независимо от успешности определения рамок
	// будет нужен RemoveFontResourceEx(asFontFile, FR_PRIVATE, NULL);
	// Определить наличие рамок и "юникодности" шрифта
	HDC hdc = CreateCompatibleDC(0);

	if (hdc)
	{
		BOOL lbFail = FALSE;
		HFONT hf = CreateFont(18, 0, 0, 0, FW_NORMAL, 0, 0, 0, DEFAULT_CHARSET,
		                      OUT_TT_PRECIS, CLIP_DEFAULT_PRECIS,
		                      NONANTIALIASED_QUALITY/*ANTIALIASED_QUALITY*/, 0,
		                      rf.szFontName);

		wchar_t szDbg[1024]; szDbg[0] = 0;
		if (hf)
		{

			LPOUTLINETEXTMETRICW lpOutl = LoadOutline(NULL, hf);
			if (lpOutl)
			{
				if (lstrcmpi((wchar_t*)lpOutl->otmpFamilyName, rf.szFontName) != 0)
				{

					swprintf_c(szDbg, L"!!! RegFont failed: '%s'. Req: %s, Created: %s\n",
						asFontFile, rf.szFontName, (wchar_t*)lpOutl->otmpFamilyName);
					lbFail = TRUE;
				}
				free(lpOutl);
			}
		}

		// Попробовать по полному имени?
		if ((!hf || lbFail) && (lstrcmp(rf.szFontName, szFullFontName) != 0))
		{
			if (hf)
				DeleteObject(hf);
			hf = CreateFont(18, 0, 0, 0, FW_NORMAL, 0, 0, 0, DEFAULT_CHARSET,
		                      OUT_TT_PRECIS, CLIP_DEFAULT_PRECIS,
		                      NONANTIALIASED_QUALITY/*ANTIALIASED_QUALITY*/, 0,
		                      szFullFontName);
			LPOUTLINETEXTMETRICW lpOutl = LoadOutline(NULL, hf);

			if (lpOutl)
			{
				if (lstrcmpi((wchar_t*)lpOutl->otmpFamilyName, szFullFontName) == 0)
				{
					// Таки создавать нужно по полному имени
					wcscpy_c(rf.szFontName, szFullFontName);
					lbFail = FALSE;
				}

				free(lpOutl);
			}
		}

		// При обломе шрифт таки зарегистрируем, но как умолчание чего-либо брать не будем
		if (hf && lbFail)
		{
			DeleteObject(hf);
			hf = NULL;
		}
		if (lbFail && *szDbg)
		{
			// Показать в отладчике что стряслось
			OutputDebugString(szDbg);
		}

		if (hf)
		{
			HFONT hOldF = (HFONT)SelectObject(hdc, hf);
			LPGLYPHSET pSets = NULL;
			DWORD nSize = GetFontUnicodeRanges(hdc, NULL);

			if (nSize)
			{
				pSets = (LPGLYPHSET)calloc(nSize,1);

				if (pSets)
				{
					pSets->cbThis = nSize;

					if (GetFontUnicodeRanges(hdc, pSets))
					{
						rf.bUnicode = (pSets->flAccel != 1/*GS_8BIT_INDICES*/);

						// Поиск рамок
						if (rf.bUnicode)
						{
							for(DWORD r = 0; r < pSets->cRanges; r++)
							{
								if (pSets->ranges[r].wcLow < ucBoxDblDownRight
								        && (pSets->ranges[r].wcLow + pSets->ranges[r].cGlyphs - 1) > ucBoxDblDownRight)
								{
									rf.bHasBorders = TRUE; break;
								}
							}
						}
						else
						{
							_ASSERTE(rf.bUnicode);
						}
					}

					free(pSets);
				}
			}

			SelectObject(hdc, hOldF);
			DeleteObject(hf);
		}

		DeleteDC(hdc);
	}

	// Запомнить шрифт
	m_RegFonts.push_back(rf);
	return TRUE;
}

void CFontMgr::RegisterFonts()
{
	if (!gpSet->isAutoRegisterFonts || gpConEmu->DisableRegisterFonts)
		return; // Если поиск шрифтов не требуется

	DEBUGSTRSTARTUPLOG(L"Registering local fonts");

	// Сначала - регистрация шрифтов в папке программы
	RegisterFontsDir(gpConEmu->ms_ConEmuExeDir);

	// Если папка запуска отличается от папки программы
	if (lstrcmpW(gpConEmu->ms_ConEmuExeDir, gpConEmu->ms_ConEmuBaseDir))
		RegisterFontsDir(gpConEmu->ms_ConEmuBaseDir); // зарегистрировать шрифты и из базовой папки

	// Если папка запуска отличается от папки программы
	if (lstrcmpiW(gpConEmu->ms_ConEmuExeDir, gpConEmu->WorkDir()))
	{
		BOOL lbSkipCurDir = FALSE;
		wchar_t szFontsDir[MAX_PATH+1];

		if (SHGetSpecialFolderPath(ghWnd, szFontsDir, CSIDL_FONTS, FALSE))
		{
			// Oops, папка запуска совпала с системной папкой шрифтов?
			lbSkipCurDir = (lstrcmpiW(szFontsDir, gpConEmu->WorkDir()) == 0);
		}

		if (!lbSkipCurDir)
		{
			// зарегистрировать шрифты и из папки запуска
			RegisterFontsDir(gpConEmu->WorkDir());
		}
	}

	// Теперь можно смотреть, зарегистрировались ли какие-то шрифты... И выбрать из них подходящие
	// Это делается в InitFont
}

void CFontMgr::RegisterFontsDir(LPCWSTR asFromDir)
{
	if (!asFromDir || !*asFromDir)
		return;

	// Регистрация шрифтов в папке ConEmu
	WIN32_FIND_DATA fnd;
	wchar_t szFind[MAX_PATH*2]; wcscpy_c(szFind, asFromDir);
	wchar_t* pszSlash = szFind + lstrlenW(szFind);
	_ASSERTE(pszSlash > szFind);
	if (*(pszSlash-1) == L'\\')
		pszSlash--;
	wcscpy_add(pszSlash, szFind, L"\\*.*");
	HANDLE hFind = FindFirstFile(szFind, &fnd);

	if (hFind != INVALID_HANDLE_VALUE)
	{
		do
		{
			if ((fnd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0)
			{
				pszSlash[1] = 0;

				TCHAR* pszDot = _tcsrchr(fnd.cFileName, _T('.'));
				// Неизвестные расширения - пропускаем
				TODO("Register *.fon font files"); // Формат шрифта разобран в ImpEx
				if (!pszDot || (lstrcmpi(pszDot, _T(".ttf")) && lstrcmpi(pszDot, _T(".otf")) /*&& lstrcmpi(pszDot, _T(".fon"))*/ && lstrcmpi(pszDot, _T(".bdf")) ))
					continue;

				if ((_tcslen(fnd.cFileName)+_tcslen(szFind)) >= MAX_PATH)
				{
					size_t cchLen = _tcslen(fnd.cFileName)+100;
					wchar_t* psz=(wchar_t*)calloc(cchLen,sizeof(wchar_t));
					_wcscpy_c(psz, cchLen, L"Too long full pathname for font:\n");
					_wcscat_c(psz, cchLen, fnd.cFileName);
					int nBtn = MsgBox(psz, MB_OKCANCEL|MB_ICONSTOP, gpConEmu->GetDefaultTitle());
					free(psz);

					if (nBtn == IDCANCEL) break;
					continue;
				}


				wcscat_c(szFind, fnd.cFileName);

				// Возвращает FALSE если произошла ошибка и юзер сказал "Не продолжать"
				if (!RegisterFont(szFind, FALSE))
					break;
			}
		}
		while(FindNextFile(hFind, &fnd));

		FindClose(hFind);
	}
}

void CFontMgr::UnregisterFonts()
{
	//for(std::vector<RegFont>::iterator iter = m_RegFonts.begin();
	//        iter != m_RegFonts.end(); iter = m_RegFonts.erase(iter))
	while (m_RegFonts.size() > 0)
	{
		INT_PTR j = m_RegFonts.size()-1;
		RegFont* iter = &(m_RegFonts[j]);

		if (iter->pCustom)
			delete iter->pCustom;
		else
			RemoveFontResourceEx(iter->szFontFile, FR_PRIVATE, NULL);

		m_RegFonts.erase(j);
	}
}



// Вызывается из
// -- первичная инициализация
// void CFontMgr::InitFont(LPCWSTR asFontName/*=NULL*/, int anFontHeight/*=-1*/, int anQuality/*=-1*/)
// -- смена шрифта из фара (через Gui Macro "FontSetName")
// void CFontMgr::MacroFontSetName(LPCWSTR pszFontName, WORD anHeight /*= 0*/, WORD anWidth /*= 0*/)
// -- смена _размера_ шрифта из фара (через Gui Macro "FontSetSize")
// bool CFontMgr::MacroFontSetSize(int nRelative/*0/1/2*/, int nValue/*+-1,+-2,... | 100%*/)
// -- пересоздание шрифта по изменению контрола окна настроек
// void CSettings::RecreateFont(WORD wFromID)
// -- подгонка шрифта под размер окна GUI (если включен флажок "Auto")
// bool CFontMgr::AutoRecreateFont(int nFontW, int nFontH)
bool CFontMgr::CreateFontGroup(CLogFont inFont)
{
	bool bSucceeded = false;

	//ResetFontWidth(); -- перенесено вниз, после того, как убедимся в валидности шрифта
	//lfOutPrecision = OUT_RASTER_PRECIS,

	ms_FontError.Clear();

	HWND hMainPg = gpSetCls->GetPage(thi_Fonts);

	// For *.bdf fonts
	CustomFontFamily* pCustom = NULL;

	{
	CFontPtr font;
	if (!Create(inFont, font, &pCustom))
	{
		goto wrap;
	}

	m_Font[0] = font.Ptr();
	bSucceeded = true;
	}

	#ifdef _DEBUG
	DumpFontMetrics(L"m_Font[0]", m_Font[0]);
	#endif

	TODO("Move this into CFont");
	ResetFontWidth();

	if (m_Font[0]->mb_RasterFont)
	{
		gpSet->FontSizeY = m_Font[0]->m_LF.lfHeight;
		gpSet->FontSizeX = gpSet->FontSizeX3 = m_Font[0]->m_LF.lfWidth;

		if (gpSet->isFontAutoSize)
		{
			gpSet->isFontAutoSize = false;

			if (hMainPg)
				CDlgItemHelper::checkDlgButton(hMainPg, cbFontAuto, BST_UNCHECKED);

			gpSetCls->ShowFontErrorTip(CLngRc::getRsrc(lng_RasterAutoError)/*"Font auto size is not allowed for a fixed raster font size. Select ‘Terminal’ instead of ‘[Raster Fonts ...]’"*/);
		}

		if (hMainPg)
		{
			wchar_t temp[32];
			swprintf_c(temp, L"%i", gpSet->FontSizeY);
			CSetDlgLists::SelectStringExact(hMainPg, tFontSizeY, temp);
			swprintf_c(temp, L"%i", gpSet->FontSizeX);
			CSetDlgLists::SelectStringExact(hMainPg, tFontSizeX, temp);
			CSetDlgLists::SelectStringExact(hMainPg, tFontSizeX3, temp);
		}
	}

	#if 0
	// -- don't update Settings dialog here to avoid unexpected font creations on scrolling through font families
	if (lbTM && m_tm->tmCharSet != DEFAULT_CHARSET)
	{
		inFont.lfCharSet = m_tm->tmCharSet;

		const ListBoxItem* pCharSets = NULL;
		uint nCount = CSetDlgLists::GetListItems(CSetDlgLists::eCharSets, pCharSets);
		for (uint i = 0; i < nCount; i++)
		{
			if (pCharSets[i].nValue == m_tm->tmCharSet)
			{
				SendDlgItemMessage(hMainPg, tFontCharset, CB_SETCURSEL, i, 0);
				break;
			}
		}
	}
	#endif

	// Create set of bold/italic/underlined fonts
	_ASSERTE(MAX_FONT_STYLES == 8);
	for (int i = 1; i < MAX_FONT_STYLES; i++)
	{
		CLogFont tempFont(inFont);
		tempFont.lfWeight = (i & AI_STYLE_BOLD) ? FW_BOLD : FW_NORMAL;
		tempFont.lfItalic = (i & AI_STYLE_ITALIC   ) ? TRUE : FALSE;
		tempFont.lfUnderline = (i & AI_STYLE_UNDERLINE) ? TRUE : FALSE;
		Create(tempFont, m_Font[i], &pCustom);
	}

	// Fin
	LogFont = (LOGFONT)inFont;

	// Alternative font for user-defined char range
	RecreateAlternativeFont();

	// If Settings dialog is open - show tool balloon error
	if (m_Font[0]->ms_FontError || (m_Font2.Ptr() && m_Font2->ms_FontError))
	{
		if (m_Font[0]->ms_FontError && (m_Font2.Ptr() && m_Font2->ms_FontError))
		{
			ms_FontError = lstrmerge(m_Font[0]->ms_FontError, L"\n", m_Font2->ms_FontError);
		}
		else if (m_Font2.Ptr() && m_Font2->ms_FontError)
		{
			ms_FontError.Set(m_Font2->ms_FontError);
		}
		else
		{
			ms_FontError.Set(m_Font[0]->ms_FontError);
		}
	}

	if (ghOpWnd)
	{
		gpSetCls->ShowFontErrorTip(ms_FontError);
	}
wrap:
	return bSucceeded;
}

bool CFontMgr::Create(CLogFont inFont, CFontPtr& rpFont, CustomFontFamily** ppCustom /*= NULL*/)
{
	bool bSucceeded = false;

	if (rpFont.Ptr())
		rpFont->ms_FontError.Clear();

	WARNING("TODO: Use font cache");

	rpFont.Release();

	wchar_t szFontError[512] = L"";

	// Search through *.bdf font (drawn by ConEmu internally)
	bool isBdf = false;
	CustomFont* pFont = NULL;
	CustomFontFamily* pCustom = NULL;

	// If bdf font family was already found by CreateFontGroup
	if (ppCustom && *ppCustom)
	{
		pCustom = *ppCustom;
		pFont = pCustom->GetFont(inFont.lfHeight,
				inFont.lfWeight >= FW_BOLD, inFont.lfItalic, inFont.lfUnderline);
	}

	// bdf?
	if (pFont
		|| FindCustomFont(inFont.lfFaceName, inFont.lfHeight,
				inFont.lfWeight >= FW_BOLD, inFont.lfItalic, inFont.lfUnderline,
				&pCustom, &pFont))
	{
		if (!pFont)
		{
			MBoxAssert(pFont != NULL);
			goto wrap;
		}

		// Get real font size (update inFont)
		pFont->GetBoundingBox(&inFont.lfWidth, &inFont.lfHeight);

		WARNING("Move ResetFontWidth to rpFont");
		//ResetFontWidth();

		TODO("Use font cache");
		rpFont = new CFont(pFont);

		if (!rpFont.IsSet())
			goto wrap;

		// Must be set in ctor
		_ASSERTE(rpFont->mb_RasterFont && rpFont->mb_Monospace);

		// rpFont->m_LF would be updated below!

		// Optimization for batch creation
		if (ppCustom)
			*ppCustom = pCustom;

		bSucceeded = true;
	}
	else
	{
		if (ppCustom)
			*ppCustom = NULL;

		HFONT hFont = NULL;
		static int nRastNameLen = _tcslen(RASTER_FONTS_NAME);
		int nRastHeight = 0, nRastWidth = 0;
		bool bRasterFont = false; // Specially for [Raster Fonts WxH]
		LOGFONT tmpFont = (LOGFONT)inFont;

		if (inFont.lfFaceName[0] == L'[' && wcsncmp(inFont.lfFaceName+1, RASTER_FONTS_NAME, nRastNameLen) == 0)
		{
			wchar_t *pszEnd = NULL;
			wchar_t *pszSize = inFont.lfFaceName + nRastNameLen + 2;
			nRastWidth = wcstol(pszSize, &pszEnd, 10);

			if (nRastWidth && pszEnd && *pszEnd == L'x')
			{
				pszSize = pszEnd + 1;
				nRastHeight = wcstol(pszSize, &pszEnd, 10);

				if (nRastHeight)
				{
					bRasterFont = true;
					inFont.lfHeight = nRastHeight;
					inFont.lfWidth = nRastWidth;
				}
			}

			inFont.lfCharSet = OEM_CHARSET;
			tmpFont = inFont;
			wcscpy_c(tmpFont.lfFaceName, L"Terminal");
		}
		else
		{
			// "Terminal" + OEM == Raster Font
			if ((wcscmp(inFont.lfFaceName, L"Terminal") == 0) && (inFont.lfCharSet == OEM_CHARSET))
			{
				bRasterFont = true;
			}
		}


		if (gpSet->isMonospace)
		{
			// Preferred flags
			tmpFont.lfPitchAndFamily = FIXED_PITCH | FF_MODERN;
		}

		hFont = CreateFontIndirect(&tmpFont);

		wchar_t szFontFace[32];
		CEDC hDC(NULL);
		hDC.Create(800, 600);
		MBoxAssert(hDC);

		if (hFont)
		{
			rpFont = new CFont(hFont);
		}

		if (rpFont.IsSet())
		{
			DWORD dwFontErr = 0;
			SetLastError(0);
			HFONT hOldF = (HFONT)SelectObject(hDC, hFont);
			dwFontErr = GetLastError();
			BOOL lbTM = GetTextMetrics(hDC, &rpFont->m_tm);

			if (!lbTM && !bRasterFont)
			{
				// Считаем, что шрифт НЕ валиден!!!
				dwFontErr = GetLastError();
				SelectObject(hDC, hOldF);
				DeleteDC(hDC);

				swprintf_c(szFontError, L"GetTextMetrics failed for non Raster font '%s'", inFont.lfFaceName);
				if (dwFontErr)
				{
					int nCurLen = _tcslen(szFontError);
					_wsprintf(szFontError + nCurLen, SKIPLEN(countof(szFontError)-nCurLen)
							  L"\r\nErrorCode = 0x%08X", dwFontErr);
				}

				DeleteObject(hFont);

				goto wrap;
			}

			if (!lbTM)
			{
				_ASSERTE(lbTM);  // -V571
			}

			if (bRasterFont)
			{
				rpFont->m_tm.tmHeight = nRastHeight;
				rpFont->m_tm.tmAveCharWidth = rpFont->m_tm.tmMaxCharWidth = nRastWidth;
			}

			rpFont->mp_otm = LoadOutline(hDC, NULL/*hFont*/); // шрифт УЖЕ выбран в DC

			if (!rpFont->mp_otm)
			{
				dwFontErr = GetLastError();
			}

			if (GetTextFace(hDC, countof(szFontFace), szFontFace))
			{
				if (!bRasterFont)
				{
					szFontFace[31] = 0;

					if (lstrcmpi(inFont.lfFaceName, szFontFace))
					{
						int nCurLen = _tcslen(szFontError);
						_wsprintf(szFontError+nCurLen, SKIPLEN(countof(szFontError)-nCurLen)
								  L"Failed to create font!\nRequested: %s\nCreated: %s\n", inFont.lfFaceName, szFontFace);
						lstrcpyn(inFont.lfFaceName, szFontFace, countof(inFont.lfFaceName)); inFont.lfFaceName[countof(inFont.lfFaceName)-1] = 0;
						wcscpy_c(tmpFont.lfFaceName, inFont.lfFaceName);
					}
				}
			}

			rpFont->mb_RasterFont = bRasterFont;
			rpFont->mb_Monospace = bRasterFont ? true : IsAlmostMonospace(inFont.lfFaceName, &rpFont->m_tm, rpFont->mp_otm);

			// у Arial'а например MaxWidth слишком большой (в два и более раз больше ВЫСОТЫ шрифта)
			if (rpFont->m_tm.tmMaxCharWidth > (rpFont->m_tm.tmHeight * 15 / 10))
				rpFont->m_tm.tmMaxCharWidth = rpFont->m_tm.tmHeight; // иначе зашкалит - текст очень сильно разъедется

			// Лучше поставим AveCharWidth. MaxCharWidth для "условно моноширинного" Consolas почти равен высоте.
			if (gpSet->FontSizeX3 && ((int)gpSet->FontSizeX3 > FontDefWidthMin) && ((int)gpSet->FontSizeX3 <= FontDefWidthMax))
			{
				inFont.lfWidth = EvalCellWidth();
			}
			else if (rpFont->mb_Monospace)
			{
				// Same logic as in CEDC::TextExtentPoint!
				wchar_t text[] = L"N";
				RECT rc = {};
				if (::DrawText(hDC, text, 1, &rc, DT_CALCRECT|DT_NOPREFIX) != 0)
					inFont.lfWidth = rc.right;
				else
					inFont.lfWidth = rpFont->m_tm.tmAveCharWidth;
			}
			else
			{
				inFont.lfWidth = rpFont->m_tm.tmAveCharWidth;
			}

			// Обновлять реальный размер шрифта в диалоге настройки не будем, были случаи, когда
			// tmHeight был меньше, чем запрашивалось, однако, если пытаться создать шрифт с этим "обновленным"
			// размером - в реале создавался совсем другой шрифт...
			inFont.lfHeight = rpFont->m_tm.tmHeight;
			_ASSERTE(inFont.lfHeight > 0);

			if (lbTM && rpFont->m_tm.tmCharSet != DEFAULT_CHARSET)
			{
				inFont.lfCharSet = rpFont->m_tm.tmCharSet;
			}

			SelectObject(hDC, hOldF);

			_ASSERTE(hFont != NULL);
			bSucceeded = (hFont != NULL);
		}

		hDC.Delete();
	}

	if (bSucceeded)
	{
		_ASSERTE(rpFont.IsSet());
		rpFont->m_LF = (LOGFONT)inFont;
	}

wrap:
	if (rpFont.Ptr())
	{
		rpFont->ms_FontError.Set(szFontError);
	}
	return bSucceeded;
}

LONG CFontMgr::EvalCellWidth()
{
	return (LONG)gpSet->FontSizeX3;
}

bool CFontMgr::FindCustomFont(LPCWSTR lfFaceName, int iSize, BOOL bBold, BOOL bItalic, BOOL bUnderline, CustomFontFamily** ppCustom, CustomFont** ppFont)
{
	*ppFont = NULL;
	*ppCustom = NULL;

	// Поиск по шрифтам рисованным ConEmu (bdf)
	//for (std::vector<RegFont>::iterator iter = m_RegFonts.begin(); iter != m_RegFonts.end(); ++iter)
	for (INT_PTR j = 0; j < m_RegFonts.size(); ++j)
	{
		const RegFont* iter = &(m_RegFonts[j]);

		if (iter->pCustom && lstrcmp(lfFaceName, iter->szFontName)==0)
		{
			*ppCustom = iter->pCustom;
			*ppFont = (*ppCustom)->GetFont(iSize, bBold, bItalic, bUnderline);

			if (!*ppFont)
			{
				MBoxAssert(*ppFont != NULL);
			}

			return true; // [bdf] шрифт. ошибка определяется по (*ppFont==NULL)
		}
	}

	return false; // НЕ [bdf] шрифт
}

bool CFontMgr::MacroFontSetSizeInt(LOGFONT& LF, int nRelative/*0/1/2/3*/, int nValue/*+-1,+-2,... | 100%*/)
{
	bool bChanged = false;
	int nCurHeight = gpSetCls->EvalSize(gpSet->FontSizeY, esf_Vertical|esf_CanUseZoom);
	int nNeedHeight = nCurHeight;
	int nNewZoomValue = mn_FontZoomValue;

	// The current defaults
	LF = LogFont;

	// The LogFont member does not contains "Units" (negative values)
	// So we need to reevaluate "current" font descriptor (the height actually)
	LOGFONT lfCur = {};
	EvalLogfontSizes(lfCur, gpSet->FontSizeY, gpSet->FontSizeX);

	switch (nRelative)
	{
	case 0:
		// Absolute
		if (nValue < 5)
		{
			_ASSERTE(nValue >= 5);
			gpConEmu->LogString(L"-- Skipped! Absolute value less than 5");
			return false;
		}
		// Set the new value
		nNeedHeight = nValue;
		break;

	case 1:
		// Relative
		if (nValue == 0)
		{
			_ASSERTE(nValue != 0);
			gpConEmu->LogString(L"-- Skipped! Relative value is zero");
			return false;
		}
		// Decrease/increate font height
		nNeedHeight += nValue;
		break;

	case 2:
	case 3:
		// Zoom value
		nNewZoomValue = (nRelative == 2) ? MulDiv(nValue, FontZoom100, 100) : nValue;
		if (nNewZoomValue < 10)
		{
			_ASSERTE(nNewZoomValue >= 10);
			gpConEmu->LogString(L"-- Skipped! Zoom value is too small");
			return false;
		}
		// Force font recreation, even if zoom value is the same
		bChanged = true;
		goto wrap;
	}

	if ((gpSet->FontSizeY <= 0) || (nNeedHeight <= 0))
	{
		_ASSERTE((gpSet->FontSizeY > 0) && (nNeedHeight >= 0));
		gpConEmu->LogString(L"-- Skipped! FontSizeY and nNeedHeight must be positive");
		return false;
	}

	// Eval new zoom value
	nNewZoomValue = MulDiv(nNeedHeight, FontZoom100, gpSet->FontSizeY);
	// If relative, let easy return to 100%
	if (nRelative == 1)
	{
		if (((mn_FontZoomValue > FontZoom100) && (nNewZoomValue < FontZoom100)) || ((mn_FontZoomValue < FontZoom100) && (nNewZoomValue > FontZoom100)))
		{
			nNewZoomValue = FontZoom100;
			bChanged = true;
		}
	}

wrap:
	// And set the Zoom value
	TODO("Move Zoom to VCon");
	mn_FontZoomValue = nNewZoomValue;

	// Now we can set the font
	EvalLogfontSizes(LF, gpSet->FontSizeY, gpSet->FontSizeX);

	// Check the height
	if (!bChanged && (LF.lfHeight != lfCur.lfHeight))
		bChanged = true;
	#if _DEBUG
	if (!bChanged)
	{
		_ASSERTE(bChanged && "lfHeight must be changed");
		gpConEmu->LogString(L"-- Skipped! lfHeight was not changed");
	}
	#endif
	// Ready
	return bChanged;
}

void CFontMgr::RecreateAlternativeFont()
{
	if (!m_Font[0].IsSet())
	{
		_ASSERTE(FALSE && "Must not be called before main font creation");
		return;
	}

	// If pseudographics was included in the alternative font char range
	bool isBorderFont = false;

	// Used for font creation (may contain negative lfHeight)
	CLogFont LogFont2((LOGFONT)LogFont);

	lstrcpyn(LogFont2.lfFaceName, (*gpSet->inFont2) ? gpSet->inFont2 : gsDefGuiFont, countof(LogFont2.lfFaceName));

	// This structure contains ‘real’ positive values
	CLogFont inFont((LOGFONT)m_Font[0]->m_LF);


	// Eval first to consider DPI and FontUseUnits options
	// Force the same height in pixels as main font
	EvalLogfontSizes(LogFont2, gpSet->FontSizeY, gpSet->FontSizeX2);

	// Font for pseudographics may differ a lot in height,
	// so, to avoid vertically-dashed frames...
	if (gpSet->CheckCharAltFont(ucBoxDblVert))
	{
		isBorderFont = true;

		// Previously, it used only positive values. But generally we use negative.
		// So, compare them by absolute value.
		if (LogFont.lfHeight && (_abs(LogFont.lfHeight) > _abs(LogFont2.lfHeight)))
		{
			LogFont2.lfHeight = LogFont.lfHeight;
		}
	}

	// Font width was not defined?
	if (gpSet->FontSizeX2 <= 0)
	{
		// Use main font width
		LogFont2.lfWidth = inFont.lfWidth;
	}

	// если ширина шрифта стала больше ширины FixFarBorders - сбросить ширину FixFarBorders в 0
	if (gpSet->FontSizeX2 && (LONG)gpSet->FontSizeX2 < inFont.lfWidth)
	{
		gpSet->FontSizeX2 = 0;

		HWND hMainPg = gpSetCls->GetPage(thi_Fonts);
		if (hMainPg)
			CSetDlgLists::SelectStringExact(hMainPg, tFontSizeX2, L"0");
	}

	// To avoid dashed frames, alternative font was created with NONANTIALIASED_QUALITY
	// But now, due to user-defined ranges, the font may be used for arbitrary characters,
	// so it's better to give user control over this
	LogFont2.lfQuality = (!gpSet->isAntiAlias2) ? NONANTIALIASED_QUALITY
		// so, if clear-type or other anti-aliasing is ON for main font...
		: (gpSet->mn_AntiAlias != NONANTIALIASED_QUALITY) ? gpSet->mn_AntiAlias
		// otherwise - use clear-type
		: CLEARTYPE_NATURAL_QUALITY;

	LogFont2.lfCharSet = DEFAULT_CHARSET;

	if (!Create(LogFont2, m_Font2))
	{
		_ASSERTE(FALSE && "Alternative font creation failed");
		return;
	}

	if (m_Font2->iType == CEFONT_CUSTOM)
	{
		// Nothing to check
		return; // Succeeded
	}

	// Ensure, the font has pseudographics?
	if (isBorderFont)
	{
		// Проверяем, совпадает ли имя созданного шрифта с запрошенным?
		if (lstrcmpi(m_Font2->m_LF.lfFaceName, LogFont2.lfFaceName) != 0)
		{
			wchar_t szFontError[512] = L"";

			//if (szFontError[0]) wcscat_c(szFontError, L"\n");

			swprintf_c(szFontError,
			          L"Failed to create border font!\nRequested: %s\nCreated: %s",
			          LogFont2.lfFaceName, m_Font2->m_LF.lfFaceName);

			// Lucida may be not installed too
			// So, try to create Lucida or Courier (we need font with 'frames')
			bool bCreated = false;
			LPCWSTR szAltNames[] = {gsDefGuiFont, gsAltGuiFont};
			for (size_t a = 0; a < countof(szAltNames); a++)
			{
				if (!a && lstrcmpi(LogFont2.lfFaceName, gsDefGuiFont) == 0)
					continue; // It was already failed...

				wcscpy_c(LogFont2.lfFaceName, szAltNames[a]);
				if (Create(LogFont2, m_Font2))
				{
					// Проверяем что создалось, и ругаемся, если что...
					if (lstrcmpi(m_Font2->m_LF.lfFaceName, LogFont2.lfFaceName) == 0)
					{
						bCreated = true;
						wcscat_c(szFontError, L"\nUsing: ");
						wcscat_c(szFontError, m_Font2->m_LF.lfFaceName);
						break;
					}
				}
			}

			m_Font2->ms_FontError.Set(szFontError);
		}
	}

	#ifdef _DEBUG
	DumpFontMetrics(L"m_Font2", m_Font2);
	#endif
}

// abReset = true : during initialization
// abReset = false : when calling from Settings dialog
void CFontMgr::RecreateFont(bool abReset, bool abRecreateControls /*= false*/)
{
	CLogFont LF;

	HWND hMainPg = gpSetCls->GetPage(thi_Fonts);

	if (abReset || (ghOpWnd == NULL))
	{
		LF = LogFont;
	}
	else
	{
		LF.lfOutPrecision = OUT_TT_PRECIS;
		LF.lfClipPrecision = CLIP_DEFAULT_PRECIS;
		LF.lfPitchAndFamily = FIXED_PITCH | FF_MODERN;
		GetDlgItemText(hMainPg, tFontFace, LF.lfFaceName, countof(LF.lfFaceName));
		gpSet->FontSizeY = CDlgItemHelper::GetNumber(hMainPg, tFontSizeY);
		gpSet->FontSizeX = CDlgItemHelper::GetNumber(hMainPg, tFontSizeX);
		gpSet->FontUseDpi = _bool(CDlgItemHelper::isChecked(hMainPg, cbFontMonitorDpi));
		gpSet->FontUseUnits = _bool(CDlgItemHelper::isChecked(hMainPg, cbFontAsDeviceUnits));
		EvalLogfontSizes(LF, gpSet->FontSizeY, gpSet->FontSizeX);
		LF.lfWeight = CDlgItemHelper::isChecked(hMainPg, cbBold) ? FW_BOLD : FW_NORMAL;
		LF.lfItalic = CDlgItemHelper::isChecked(hMainPg, cbItalic);
		LF.lfCharSet = gpSet->mn_LoadFontCharSet;

		if (gpSet->mb_CharSetWasSet)
		{
			UINT lfCharSet = DEFAULT_CHARSET;
			if (CSetDlgLists::GetListBoxItem(hMainPg, tFontCharset, CSetDlgLists::eCharSets, lfCharSet))
				LF.lfCharSet = LOBYTE(lfCharSet);
			else
				LF.lfCharSet = DEFAULT_CHARSET;
		}

		if (CDlgItemHelper::isChecked(hMainPg, rNoneAA))
			LF.lfQuality = NONANTIALIASED_QUALITY;
		else if (CDlgItemHelper::isChecked(hMainPg, rStandardAA))
			LF.lfQuality = ANTIALIASED_QUALITY;
		else if (CDlgItemHelper::isChecked(hMainPg, rCTAA))
			LF.lfQuality = CLEARTYPE_NATURAL_QUALITY;

		GetDlgItemText(hMainPg, tFontFace2, gpSet->inFont2, countof(gpSet->inFont2));
		gpSet->FontSizeX2 = CDlgItemHelper::GetNumber(hMainPg, tFontSizeX2, FontDefWidthMin, FontDefWidthMax);
		gpSet->FontSizeX3 = CDlgItemHelper::GetNumber(hMainPg, tFontSizeX3, FontDefWidthMin, FontDefWidthMax);

		if (gpSet->isLogging())
		{
			char szInfo[128]; sprintf_c(szInfo, "AutoRecreateFont(H=%i, W=%i)", LF.lfHeight, LF.lfWidth);
			CVConGroup::LogString(szInfo);
		}
	}

	_ASSERTE(LF.lfWidth >= 0 && LF.lfHeight != 0);

	if (CreateFontGroup(LF))
	{
		_ASSERTE(m_Font[0].IsSet() && m_Font[0]->m_LF.lfWidth >= 0 && m_Font[0]->m_LF.lfHeight > 0);

		// SaveFontSizes выполним после обновления LogFont, т.к. там зовется gpConEmu->OnPanelViewSettingsChanged

		SaveFontSizes((mn_AutoFontWidth == -1), true);

		{
			if (abRecreateControls)
				gpConEmu->RecreateControls(true, true, false);

			gpConEmu->Update(true);

			if (gpConEmu->GetWindowMode() == wmNormal)
				CVConGroup::SyncWindowToConsole(); // -- функция пустая, игнорируется
			else
				CVConGroup::SyncConsoleToWindow();

			gpConEmu->ReSize();
		}
	}

	if (ghOpWnd)
	{
		gpSetCls->UpdateFontInfo();
	}

	if (gpConEmu->GetStartupStage() >= CConEmuMain::ss_Started)
	{
		gpConEmu->OnPanelViewSettingsChanged(TRUE);
	}
}

void CFontMgr::ResetFontWidth()
{
	TODO("Use m_FontPtrs instead?");
	for (size_t i = 0; i < countof(m_Font); i++)
	{
		if (m_Font[i].IsSet())
			m_Font[i]->ResetFontWidth();
	}
	if (m_Font2.IsSet())
		m_Font2->ResetFontWidth();
}

void CFontMgr::SaveFontSizes(bool bAuto, bool bSendChanges)
{
	if (!m_Font[0].IsSet())
	{
		_ASSERTE(FALSE && "Font was not created!");
		return;
	}

	// Even if font was created with FontUseUnits option (negative lfHeight)
	// CreateFontGroup MUST return in the lfHeight & lfWidth ACTUAL
	// bounding rectangle, so we can just store them
	_ASSERTE(m_Font[0]->m_LF.lfWidth > 0 && m_Font[0]->m_LF.lfHeight);

	mn_FontWidth = m_Font[0]->m_LF.lfWidth;
	mn_FontHeight = m_Font[0]->m_LF.lfHeight;

	wchar_t szLog[120];
	swprintf_c(szLog,
		L"Main font was created Face='%s' lfHeight=%i lfWidth=%i use-dpi=%u dpi=%i zoom=%i",
		LogFont.lfFaceName, LogFont.lfHeight, LogFont.lfWidth,
		(UINT)gpSet->FontUseDpi, gpSetCls->_dpi.Ydpi, mn_FontZoomValue);
	LogString(szLog);

	if (bAuto)
	{
		mn_AutoFontWidth = mn_FontWidth;
		mn_AutoFontHeight = mn_FontHeight;
	}

	// Применить в Mapping (там заодно и палитра копируется)
	gpConEmu->OnPanelViewSettingsChanged(bSendChanges);
}

void CFontMgr::SettingsLoaded(SettingsLoadedFlags slfFlags)
{
	_ASSERTE(gpConEmu != nullptr);

	/*
	LogFont.lfHeight = mn_FontHeight = gpSet->FontSizeY;
	LogFont.lfWidth = mn_FontWidth = gpSet->FontSizeX;
	*/
	EvalLogfontSizes(LogFont, gpSet->FontSizeY, gpSet->FontSizeX);
	if (gpSet->inFont && *gpSet->inFont)
		lstrcpyn(LogFont.lfFaceName, gpSet->inFont, countof(LogFont.lfFaceName));
	LogFont.lfQuality = gpSet->mn_AntiAlias;
	LogFont.lfWeight = gpSet->isBold ? FW_BOLD : FW_NORMAL;
	LogFont.lfCharSet = (BYTE)gpSet->mn_LoadFontCharSet;
	LogFont.lfItalic = gpSet->isItalic;

	if (slfFlags & slf_OnStartupLoad)
	{
		RegisterFonts();
	}

	InitFont(
		gpConEmu->opt.FontVal.GetStr(),
		gpConEmu->opt.SizeVal.Exists ? gpConEmu->opt.SizeVal.GetInt() : -1,
		gpConEmu->opt.ClearTypeVal.Exists ? gpConEmu->opt.ClearTypeVal.GetInt() : -1
	);

	if (slfFlags & slf_OnResetReload)
	{
		// Шрифт пере-создать сразу, его характеристики используются при ресайзе окна
		RecreateFont(true);
	}
	else
	{
		_ASSERTE(ghWnd==NULL);
	}
}

void CFontMgr::SettingsPreSave()
{
	lstrcpyn(gpSet->inFont, LogFont.lfFaceName, countof(gpSet->inFont));
	#if 0
	// was #ifdef UPDATE_FONTSIZE_RECREATE
	gpSet->FontSizeY = LogFont.lfHeight;
	#endif
	gpSet->mn_LoadFontCharSet = LogFont.lfCharSet;
	gpSet->mn_AntiAlias = LogFont.lfQuality;
	gpSet->isBold = (LogFont.lfWeight >= FW_BOLD);
	gpSet->isItalic = (LogFont.lfItalic != 0);
}

bool CFontMgr::IsAlmostMonospace(LPCWSTR asFaceName, LPTEXTMETRIC lptm, LPOUTLINETEXTMETRIC lpotm)
{
	if (!lptm && lpotm)
		lptm = &lpotm->otmTextMetrics;
	if (!lptm)
	{
		_ASSERTE(lptm || lpotm);
		return false;
	}

	bool bPanMono = false, bSelfOtm = false;

	if (!lpotm && (lptm->tmPitchAndFamily & (TMPF_TRUETYPE)))
	{
		HFONT hFont = CreateFont(-lptm->tmHeight, 0, 0, 0, lptm->tmWeight, lptm->tmItalic, 0, 0, lptm->tmCharSet, 0, 0, 0,
			0, asFaceName);
		if (hFont)
		{
			lpotm = LoadOutline(NULL, hFont);
			bSelfOtm = (lpotm != NULL);
			DeleteObject(hFont);
		}
	}

	if (lpotm)
	{
		// #LOG Log info about monospace decision
		if (lpotm->otmPanoseNumber.bProportion == PAN_PROP_MONOSPACED)
			bPanMono = true;
		if (bSelfOtm)
			SafeFree(lpotm);
	}

	if (bPanMono)
	{
		return true;
	}

	// Некоторые шрифты (Consolas) достаточно странные. Заявлены как моноширинные (PAN_PROP_MONOSPACED),
	// похожи на моноширинные, но tmMaxCharWidth у них очень широкий (иероглифы что-ли?)
	if (lstrcmp(asFaceName, L"Consolas") == 0)
		return true;

	int tmMaxCharWidth = lptm->tmMaxCharWidth, tmAveCharWidth = lptm->tmAveCharWidth, tmHeight = lptm->tmHeight;

	// у Arial'а например MaxWidth слишком большой (в два и более раз больше ВЫСОТЫ шрифта)
	bool bAlmostMonospace = false;

	if (tmMaxCharWidth && tmAveCharWidth && tmHeight)
	{
		int nRelativeDelta = (tmMaxCharWidth - tmAveCharWidth) * 100 / tmHeight;

		// Если расхождение менее 16% высоты - считаем шрифт моноширинным
		// Увеличил до 16%. Win7, Courier New, 6x4
		if (nRelativeDelta <= 16)
			bAlmostMonospace = true;

		//if (abs(m_tm->tmMaxCharWidth - m_tm->tmAveCharWidth)<=2)
		//{ -- это была попытка прикинуть среднюю ширину по английским буквам
		//  -- не нужно, т.к. затевалось из-за проблем с ClearType на больших размерах
		//  -- шрифтов, а это лечится аргументом pDX в TextOut
		//	int nTestLen = _tcslen(TEST_FONT_WIDTH_STRING_EN);
		//	SIZE szTest = {0,0};
		//	if (GetTextExtentPoint32(hDC, TEST_FONT_WIDTH_STRING_EN, nTestLen, &szTest)) {
		//		int nAveWidth = (szTest.cx + nTestLen - 1) / nTestLen;
		//		if (nAveWidth > m_tm->tmAveCharWidth || nAveWidth > m_tm->tmMaxCharWidth)
		//			m_tm->tmMaxCharWidth = m_tm->tmAveCharWidth = nAveWidth;
		//	}
		//}
	}
	else
	{
		_ASSERTE(tmMaxCharWidth);
		_ASSERTE(tmAveCharWidth);
		_ASSERTE(tmHeight);
	}

	return bAlmostMonospace;
}

LPOUTLINETEXTMETRIC CFontMgr::LoadOutline(HDC hDC, HFONT hFont)
{
	BOOL lbSelfDC = FALSE;

	if (!hDC)
	{
		HDC hScreenDC = GetDC(0);
		hDC = CreateCompatibleDC(hScreenDC);
		lbSelfDC = TRUE;
		ReleaseDC(0, hScreenDC);
	}

	HFONT hOldF = NULL;

	if (hFont)
	{
		hOldF = (HFONT)SelectObject(hDC, hFont);
	}

	LPOUTLINETEXTMETRIC pOut = NULL;
	UINT nSize = GetOutlineTextMetrics(hDC, 0, NULL);

	if (nSize)
	{
		pOut = (LPOUTLINETEXTMETRIC)calloc(nSize,1);

		if (pOut)
		{
			pOut->otmSize = nSize;

			if (!GetOutlineTextMetricsW(hDC, nSize, pOut))
			{
				free(pOut); pOut = NULL;
			}
			else
			{
				pOut->otmpFamilyName = (PSTR)(((LPBYTE)pOut) + (DWORD_PTR)pOut->otmpFamilyName);
				pOut->otmpFaceName = (PSTR)(((LPBYTE)pOut) + (DWORD_PTR)pOut->otmpFaceName);
				pOut->otmpStyleName = (PSTR)(((LPBYTE)pOut) + (DWORD_PTR)pOut->otmpStyleName);
				pOut->otmpFullName = (PSTR)(((LPBYTE)pOut) + (DWORD_PTR)pOut->otmpFullName);
			}
		}
	}

	if (hFont)
	{
		SelectObject(hDC, hOldF);
	}

	if (lbSelfDC)
	{
		DeleteDC(hDC);
	}

	return pOut;
}

void CFontMgr::DumpFontMetrics(LPCWSTR szType, CFontPtr& Font)
{
	wchar_t szFontDump[255];

	if (!Font.IsSet())
	{
		swprintf_c(szFontDump, L"*** gpSet->%s: WAS NOT CREATED!\n", szType);
	}
	else
	{
		TEXTMETRIC ltm = Font->m_tm;
		LPOUTLINETEXTMETRIC lpOutl = Font->mp_otm;
		swprintf_c(szFontDump, L"*** gpSet->%s: '%s', Height=%i, Ave=%i, Max=%i, Over=%i, Angle*10=%i\n",
		          szType, Font->m_LF.lfFaceName, ltm.tmHeight, ltm.tmAveCharWidth, ltm.tmMaxCharWidth, ltm.tmOverhang,
		          lpOutl ? lpOutl->otmItalicAngle : 0);
	}

	DEBUGSTRFONT(szFontDump);
}
