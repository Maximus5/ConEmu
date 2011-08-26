#pragma once
#ifndef __PLUGIN_HPP__
#define __PLUGIN_HPP__

/*
  plugin.hpp

  Plugin API for Far Manager 3.0 build 2174
*/

/*
Copyright © 1996 Eugene Roshal
Copyright © 2000 Far Group
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

THIS SOFTWARE IS PROVIDED BY THE AUTHOR `AS IS' AND ANY EXPRESS OR
IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

EXCEPTION:
Far Manager plugins that use this header file can be distributed under any
other possible license with no implications from the above license on them.
*/

#define FARMANAGERVERSION_MAJOR 3
#define FARMANAGERVERSION_MINOR 0
#define FARMANAGERVERSION_REVISION 0
#define FARMANAGERVERSION_BUILD 2174
#define FARMANAGERVERSION_STAGE VS_RELEASE

#ifndef RC_INVOKED

#include <windows.h>

#undef DefDlgProc

#define FARMACRO_KEY_EVENT  (KEY_EVENT|0x8000)


#define CP_UNICODE 1200
#define CP_REVERSEBOM 1201
#define CP_AUTODETECT ((UINT)-1)
#define CP_REDETECT   ((UINT)-2)

typedef unsigned __int64 FARCOLORFLAGS;
static const FARCOLORFLAGS
	FCF_NONE          = 0,
	FCF_FG_4BIT       = 0x0000000000000001ULL,
	FCF_BG_4BIT       = 0x0000000000000002ULL,

	FCF_4BITMASK      = 0x0000000000000003ULL, // FCF_FG_4BIT|FCF_BG_4BIT

	FCF_EXTENDEDFLAGS = 0xFFFFFFFFFFFFFFFCULL, // ~FCF_4BITMASK

	FCF_FG_BOLD       = 0x1000000000000000ULL,
	FCF_FG_ITALIC     = 0x2000000000000000ULL,
	FCF_FG_UNDERLINE  = 0x4000000000000000ULL,
	FCF_STYLEMASK     = 0x7000000000000000ULL; // FCF_FG_BOLD|FCF_FG_ITALIC|FCF_FG_UNDERLINE

struct FarColor
{
	FARCOLORFLAGS Flags;
	COLORREF ForegroundColor;
	COLORREF BackgroundColor;
	void* Reserved;
};

#define INDEXMASK 0x0000000f
#define COLORMASK 0x00ffffff
#define ALPHAMASK 0xff000000

#define INDEXVALUE(x) ((x)&INDEXMASK)
#define COLORVALUE(x) ((x)&COLORMASK)
#define ALPHAVALUE(x) ((x)&ALPHAMASK)

#define IS_OPAQUE(x) (ALPHAVALUE(x)==ALPHAMASK)
#define IS_TRANSPARENT(x) (!ALPHAVALUE(x))
#define MAKE_OPAQUE(x) (x|=ALPHAMASK)
#define MAKE_TRANSPARENT(x) (x&=COLORMASK)

typedef unsigned __int64 COLORDIALOGFLAGS;
static const COLORDIALOGFLAGS
    CDF_NONE = 0;

typedef BOOL (WINAPI *FARAPICOLORDIALOG)(
    const GUID* PluginId,
    COLORDIALOGFLAGS Flags,
    struct FarColor *Color
);

typedef unsigned __int64 FARMESSAGEFLAGS;
static const FARMESSAGEFLAGS
	FMSG_NONE                = 0,
	FMSG_WARNING             = 0x0000000000000001ULL,
	FMSG_ERRORTYPE           = 0x0000000000000002ULL,
	FMSG_KEEPBACKGROUND      = 0x0000000000000004ULL,
	FMSG_LEFTALIGN1900       = 0x0000000000000008ULL,
	FMSG_ALLINONE1900        = 0x0000000000000010ULL,
	FMSG_MB_OK               = 0x0000000000010000ULL,
	FMSG_MB_OKCANCEL         = 0x0000000000020000ULL,
	FMSG_MB_ABORTRETRYIGNORE = 0x0000000000030000ULL,
	FMSG_MB_YESNO            = 0x0000000000040000ULL,
	FMSG_MB_YESNOCANCEL      = 0x0000000000050000ULL,
	FMSG_MB_RETRYCANCEL      = 0x0000000000060000ULL;

typedef int (WINAPI *FARAPIMESSAGE)(
    const GUID* PluginId,
    const GUID* Id,
    FARMESSAGEFLAGS Flags,
    const wchar_t *HelpTopic,
    const wchar_t * const *Items,
    size_t ItemsNumber,
    int ButtonsNumber
);

enum FARDIALOGITEMTYPES
{
	DI_TEXT                         =  0,
	DI_VTEXT                        =  1,
	DI_SINGLEBOX                    =  2,
	DI_DOUBLEBOX                    =  3,
	DI_EDIT                         =  4,
	DI_PSWEDIT                      =  5,
	DI_FIXEDIT                      =  6,
	DI_BUTTON                       =  7,
	DI_CHECKBOX                     =  8,
	DI_RADIOBUTTON                  =  9,
	DI_COMBOBOX                     = 10,
	DI_LISTBOX                      = 11,

	DI_USERCONTROL                  =255,
};

/*
   Check diagol element type has inputstring?
   (DI_EDIT, DI_FIXEDIT, DI_PSWEDIT, etc)
*/
static __inline BOOL IsEdit(enum FARDIALOGITEMTYPES Type)
{
	switch (Type)
	{
		case DI_EDIT:
		case DI_FIXEDIT:
		case DI_PSWEDIT:
		case DI_COMBOBOX:
			return TRUE;
		default:
			return FALSE;
	}
}

typedef unsigned __int64 FARDIALOGITEMFLAGS;
static const FARDIALOGITEMFLAGS
	DIF_NONE                  = 0,
	DIF_BOXCOLOR              = 0x0000000000000200ULL,
	DIF_GROUP                 = 0x0000000000000400ULL,
	DIF_LEFTTEXT              = 0x0000000000000800ULL,
	DIF_MOVESELECT            = 0x0000000000001000ULL,
	DIF_SHOWAMPERSAND         = 0x0000000000002000ULL,
	DIF_CENTERGROUP           = 0x0000000000004000ULL,
	DIF_NOBRACKETS            = 0x0000000000008000ULL,
	DIF_MANUALADDHISTORY      = 0x0000000000008000ULL,
	DIF_SEPARATOR             = 0x0000000000010000ULL,
	DIF_SEPARATOR2            = 0x0000000000020000ULL,
	DIF_EDITOR                = 0x0000000000020000ULL,
	DIF_LISTNOAMPERSAND       = 0x0000000000020000ULL,
	DIF_LISTNOBOX             = 0x0000000000040000ULL,
	DIF_HISTORY               = 0x0000000000040000ULL,
	DIF_BTNNOCLOSE            = 0x0000000000040000ULL,
	DIF_CENTERTEXT            = 0x0000000000040000ULL,
	DIF_SETSHIELD             = 0x0000000000080000ULL,
	DIF_EDITEXPAND            = 0x0000000000080000ULL,
	DIF_DROPDOWNLIST          = 0x0000000000100000ULL,
	DIF_USELASTHISTORY        = 0x0000000000200000ULL,
	DIF_MASKEDIT              = 0x0000000000400000ULL,
	DIF_LISTTRACKMOUSE        = 0x0000000000400000ULL,
	DIF_LISTTRACKMOUSEINFOCUS = 0x0000000000800000ULL,
	DIF_SELECTONENTRY         = 0x0000000000800000ULL,
	DIF_3STATE                = 0x0000000000800000ULL,
	DIF_EDITPATH              = 0x0000000001000000ULL,
	DIF_LISTWRAPMODE          = 0x0000000001000000ULL,
	DIF_NOAUTOCOMPLETE        = 0x0000000002000000ULL,
	DIF_LISTAUTOHIGHLIGHT     = 0x0000000002000000ULL,
	DIF_LISTNOCLOSE           = 0x0000000004000000ULL,
	DIF_HIDDEN                = 0x0000000010000000ULL,
	DIF_READONLY              = 0x0000000020000000ULL,
	DIF_NOFOCUS               = 0x0000000040000000ULL,
	DIF_DISABLE               = 0x0000000080000000ULL,
	DIF_DEFAULTBUTTON         = 0x0000000100000000ULL,
	DIF_FOCUS                 = 0x0000000200000000ULL;

enum FARMESSAGE
{
	DM_FIRST                        = 0,
	DM_CLOSE                        = 1,
	DM_ENABLE                       = 2,
	DM_ENABLEREDRAW                 = 3,
	DM_GETDLGDATA                   = 4,
	DM_GETDLGITEM                   = 5,
	DM_GETDLGRECT                   = 6,
	DM_GETTEXT                      = 7,
	DM_GETTEXTLENGTH                = 8,
	DM_KEY                          = 9,
	DM_MOVEDIALOG                   = 10,
	DM_SETDLGDATA                   = 11,
	DM_SETDLGITEM                   = 12,
	DM_SETFOCUS                     = 13,
	DM_REDRAW                       = 14,
	DM_SETTEXT                      = 15,
	DM_SETMAXTEXTLENGTH             = 16,
	DM_SHOWDIALOG                   = 17,
	DM_GETFOCUS                     = 18,
	DM_GETCURSORPOS                 = 19,
	DM_SETCURSORPOS                 = 20,
	DM_GETTEXTPTR                   = 21,
	DM_SETTEXTPTR                   = 22,
	DM_SHOWITEM                     = 23,
	DM_ADDHISTORY                   = 24,

	DM_GETCHECK                     = 25,
	DM_SETCHECK                     = 26,
	DM_SET3STATE                    = 27,

	DM_LISTSORT                     = 28,
	DM_LISTGETITEM                  = 29,
	DM_LISTGETCURPOS                = 30,
	DM_LISTSETCURPOS                = 31,
	DM_LISTDELETE                   = 32,
	DM_LISTADD                      = 33,
	DM_LISTADDSTR                   = 34,
	DM_LISTUPDATE                   = 35,
	DM_LISTINSERT                   = 36,
	DM_LISTFINDSTRING               = 37,
	DM_LISTINFO                     = 38,
	DM_LISTGETDATA                  = 39,
	DM_LISTSETDATA                  = 40,
	DM_LISTSETTITLES                = 41,
	DM_LISTGETTITLES                = 42,

	DM_RESIZEDIALOG                 = 43,
	DM_SETITEMPOSITION              = 44,

	DM_GETDROPDOWNOPENED            = 45,
	DM_SETDROPDOWNOPENED            = 46,

	DM_SETHISTORY                   = 47,

	DM_GETITEMPOSITION              = 48,
	DM_SETMOUSEEVENTNOTIFY          = 49,

	DM_EDITUNCHANGEDFLAG            = 50,

	DM_GETITEMDATA                  = 51,
	DM_SETITEMDATA                  = 52,

	DM_LISTSET                      = 53,

	DM_GETCURSORSIZE                = 54,
	DM_SETCURSORSIZE                = 55,

	DM_LISTGETDATASIZE              = 56,

	DM_GETSELECTION                 = 57,
	DM_SETSELECTION                 = 58,

	DM_GETEDITPOSITION              = 59,
	DM_SETEDITPOSITION              = 60,

	DM_SETCOMBOBOXEVENT             = 61,
	DM_GETCOMBOBOXEVENT             = 62,

	DM_GETCONSTTEXTPTR              = 63,
	DM_GETDLGITEMSHORT              = 64,
	DM_SETDLGITEMSHORT              = 65,

	DM_GETDIALOGINFO                = 66,

	DN_FIRST                        = 4096,
	DN_BTNCLICK                     = 4097,
	DN_CTLCOLORDIALOG               = 4098,
	DN_CTLCOLORDLGITEM              = 4099,
	DN_CTLCOLORDLGLIST              = 4100,
	DN_DRAWDIALOG                   = 4101,
	DN_DRAWDLGITEM                  = 4102,
	DN_EDITCHANGE                   = 4103,
	DN_ENTERIDLE                    = 4104,
	DN_GOTFOCUS                     = 4105,
	DN_HELP                         = 4106,
	DN_HOTKEY                       = 4107,
	DN_INITDIALOG                   = 4108,
	DN_KILLFOCUS                    = 4109,
	DN_LISTCHANGE                   = 4110,
	DN_DRAGGED                      = 4111,
	DN_RESIZECONSOLE                = 4112,
	DN_DRAWDIALOGDONE               = 4113,
	DN_LISTHOTKEY                   = 4114,
	DN_INPUT                        = 4115,
	DN_CONTROLINPUT                 = 4116,
	DN_CLOSE                        = 4117,

	DM_USER                         = 0x4000,

};

enum FARCHECKEDSTATE
{
	BSTATE_UNCHECKED = 0,
	BSTATE_CHECKED   = 1,
	BSTATE_3STATE    = 2,
	BSTATE_TOGGLE    = 3,
};

enum FARCOMBOBOXEVENTTYPE
{
	CBET_KEY         = 0x00000001,
	CBET_MOUSE       = 0x00000002,
};

typedef unsigned __int64 LISTITEMFLAGS;
static const LISTITEMFLAGS
	LIF_NONE               = 0,
	LIF_SELECTED           = 0x0000000000010000ULL,
	LIF_CHECKED            = 0x0000000000020000ULL,
	LIF_SEPARATOR          = 0x0000000000040000ULL,
	LIF_DISABLE            = 0x0000000000080000ULL,
	LIF_GRAYED             = 0x0000000000100000ULL,
	LIF_HIDDEN             = 0x0000000000200000ULL,
	LIF_DELETEUSERDATA     = 0x0000000080000000ULL;

struct FarListItem
{
	LISTITEMFLAGS Flags;
	const wchar_t *Text;
	DWORD_PTR Reserved[3];
};

struct FarListUpdate
{
	int Index;
	struct FarListItem Item;
};

struct FarListInsert
{
	int Index;
	struct FarListItem Item;
};

struct FarListGetItem
{
	int ItemIndex;
	struct FarListItem Item;
};

struct FarListPos
{
	int SelectPos;
	int TopPos;
};

typedef unsigned __int64 FARLISTFINDFLAGS;
static const FARLISTFINDFLAGS
	LIFIND_NONE       = 0,
	LIFIND_EXACTMATCH = 0x0000000000000001ULL;


struct FarListFind
{
	int StartIndex;
	const wchar_t *Pattern;
	FARLISTFINDFLAGS Flags;
	DWORD_PTR Reserved;
};

struct FarListDelete
{
	int StartIndex;
	int Count;
};

typedef unsigned __int64 FARLISTINFOFLAGS;
static const FARLISTINFOFLAGS
	LINFO_NONE                  = 0,
	LINFO_SHOWNOBOX             = 0x0000000000000400ULL,
	LINFO_AUTOHIGHLIGHT         = 0x0000000000000800ULL,
	LINFO_REVERSEHIGHLIGHT      = 0x0000000000001000ULL,
	LINFO_WRAPMODE              = 0x0000000000008000ULL,
	LINFO_SHOWAMPERSAND         = 0x0000000000010000ULL;

struct FarListInfo
{
	FARLISTINFOFLAGS Flags;
	size_t ItemsNumber;
	int SelectPos;
	int TopPos;
	int MaxHeight;
	int MaxLength;
	DWORD_PTR Reserved[6];
};

struct FarListItemData
{
	int Index;
	size_t DataSize;
	void *Data;
	DWORD_PTR Reserved;
};

struct FarList
{
	size_t ItemsNumber;
	struct FarListItem *Items;
};

struct FarListTitles
{
	size_t   TitleLen;
	const wchar_t *Title;
	size_t   BottomLen;
	const wchar_t *Bottom;
};

struct FarDialogItemColors
{
	unsigned __int64 Flags;
	size_t ColorsCount;
	struct FarColor* Colors;
	void* Reserved;
};

struct FAR_CHAR_INFO
{
	WCHAR Char;
	struct FarColor Attributes;
};

struct FarDialogItem
{
	enum FARDIALOGITEMTYPES Type;
	int X1,Y1,X2,Y2;
	union
	{
		int Selected;
		struct FarList *ListItems;
		struct FAR_CHAR_INFO *VBuf;
		DWORD_PTR Reserved;
	}
#ifndef __cplusplus
	Param
#endif
	;
	const wchar_t *History;
	const wchar_t *Mask;
	FARDIALOGITEMFLAGS Flags;
	const wchar_t *Data;
	size_t MaxLength; // terminate 0 not included (if == 0 string size is unlimited)
	void* UserData;
};

struct FarDialogItemData
{
	size_t  PtrLength;
	wchar_t *PtrData;
};

struct FarDialogEvent
{
	HANDLE hDlg;
	int Msg;
	int Param1;
	void* Param2;
	INT_PTR Result;
};

struct OpenDlgPluginData
{
	HANDLE hDlg;
};

struct DialogInfo
{
	size_t StructSize;
	GUID Id;
	GUID Owner;
};

struct FarGetDialogItem
{
	size_t Size;
	struct FarDialogItem* Item;
};

#define Dlg_RedrawDialog(Info,hDlg)            Info.SendDlgMessage(hDlg,DM_REDRAW,0,0)

#define Dlg_GetDlgData(Info,hDlg)              Info.SendDlgMessage(hDlg,DM_GETDLGDATA,0,0)
#define Dlg_SetDlgData(Info,hDlg,Data)         Info.SendDlgMessage(hDlg,DM_SETDLGDATA,0,(INT_PTR)Data)

#define Dlg_GetDlgItemData(Info,hDlg,ID)       Info.SendDlgMessage(hDlg,DM_GETITEMDATA,0,0)
#define Dlg_SetDlgItemData(Info,hDlg,ID,Data)  Info.SendDlgMessage(hDlg,DM_SETITEMDATA,0,(INT_PTR)Data)

#define DlgItem_GetFocus(Info,hDlg)            Info.SendDlgMessage(hDlg,DM_GETFOCUS,0,0)
#define DlgItem_SetFocus(Info,hDlg,ID)         Info.SendDlgMessage(hDlg,DM_SETFOCUS,ID,0)
#define DlgItem_Enable(Info,hDlg,ID)           Info.SendDlgMessage(hDlg,DM_ENABLE,ID,TRUE)
#define DlgItem_Disable(Info,hDlg,ID)          Info.SendDlgMessage(hDlg,DM_ENABLE,ID,FALSE)
#define DlgItem_IsEnable(Info,hDlg,ID)         Info.SendDlgMessage(hDlg,DM_ENABLE,ID,-1)
#define DlgItem_SetText(Info,hDlg,ID,Str)      Info.SendDlgMessage(hDlg,DM_SETTEXTPTR,ID,(INT_PTR)Str)

#define DlgItem_GetCheck(Info,hDlg,ID)         Info.SendDlgMessage(hDlg,DM_GETCHECK,ID,0)
#define DlgItem_SetCheck(Info,hDlg,ID,State)   Info.SendDlgMessage(hDlg,DM_SETCHECK,ID,State)

#define DlgEdit_AddHistory(Info,hDlg,ID,Str)   Info.SendDlgMessage(hDlg,DM_ADDHISTORY,ID,(INT_PTR)Str)

#define DlgList_AddString(Info,hDlg,ID,Str)    Info.SendDlgMessage(hDlg,DM_LISTADDSTR,ID,(INT_PTR)Str)
#define DlgList_GetCurPos(Info,hDlg,ID)        Info.SendDlgMessage(hDlg,DM_LISTGETCURPOS,ID,0)
#define DlgList_SetCurPos(Info,hDlg,ID,NewPos) {struct FarListPos LPos={NewPos,-1};Info.SendDlgMessage(hDlg,DM_LISTSETCURPOS,ID,(INT_PTR)&LPos);}
#define DlgList_ClearList(Info,hDlg,ID)        Info.SendDlgMessage(hDlg,DM_LISTDELETE,ID,0)
#define DlgList_DeleteItem(Info,hDlg,ID,Index) {struct FarListDelete FLDItem={Index,1}; Info.SendDlgMessage(hDlg,DM_LISTDELETE,ID,(INT_PTR)&FLDItem);}
#define DlgList_SortUp(Info,hDlg,ID)           Info.SendDlgMessage(hDlg,DM_LISTSORT,ID,0)
#define DlgList_SortDown(Info,hDlg,ID)         Info.SendDlgMessage(hDlg,DM_LISTSORT,ID,1)
#define DlgList_GetItemData(Info,hDlg,ID,Index)          Info.SendDlgMessage(hDlg,DM_LISTGETDATA,ID,Index)
#define DlgList_SetItemStrAsData(Info,hDlg,ID,Index,Str) {struct FarListItemData FLID{Index,0,Str,0}; Info.SendDlgMessage(hDlg,DM_LISTSETDATA,ID,(INT_PTR)&FLID);}

typedef unsigned __int64 FARDIALOGFLAGS;
static const FARDIALOGFLAGS
	FDLG_WARNING             = 0x0000000000000001ULL,
	FDLG_SMALLDIALOG         = 0x0000000000000002ULL,
	FDLG_NODRAWSHADOW        = 0x0000000000000004ULL,
	FDLG_NODRAWPANEL         = 0x0000000000000008ULL,
	FDLG_KEEPCONSOLETITLE    = 0x0000000000000010ULL,
	FDLG_NONE                = 0;

typedef INT_PTR(WINAPI *FARWINDOWPROC)(
    HANDLE   hDlg,
    int Msg,
    int      Param1,
    void* Param2
);

typedef INT_PTR(WINAPI *FARAPISENDDLGMESSAGE)(
    HANDLE   hDlg,
    int Msg,
    int      Param1,
    void* Param2
);

typedef INT_PTR(WINAPI *FARAPIDEFDLGPROC)(
    HANDLE   hDlg,
    int Msg,
    int      Param1,
    void* Param2
);

typedef HANDLE(WINAPI *FARAPIDIALOGINIT)(
    const GUID*           PluginId,
    const GUID*           Id,
    int                   X1,
    int                   Y1,
    int                   X2,
    int                   Y2,
    const wchar_t        *HelpTopic,
    const struct FarDialogItem *Item,
    size_t                ItemsNumber,
    DWORD_PTR             Reserved,
    FARDIALOGFLAGS        Flags,
    FARWINDOWPROC         DlgProc,
    void*                 Param
);

typedef int (WINAPI *FARAPIDIALOGRUN)(
    HANDLE hDlg
);

typedef void (WINAPI *FARAPIDIALOGFREE)(
    HANDLE hDlg
);

struct FarKey
{
    WORD VirtualKeyCode;
    DWORD ControlKeyState;
};

typedef unsigned __int64 MENUITEMFLAGS;
static const MENUITEMFLAGS
	MIF_SELECTED   = 0x000000000010000ULL,
	MIF_CHECKED    = 0x000000000020000ULL,
	MIF_SEPARATOR  = 0x000000000040000ULL,
	MIF_DISABLE    = 0x000000000080000ULL,
	MIF_GRAYED     = 0x000000000100000ULL,
	MIF_HIDDEN     = 0x000000000200000ULL,
	MIF_NONE       = 0;

struct FarMenuItem
{
	MENUITEMFLAGS Flags;
	const wchar_t *Text;
	DWORD AccelKey;
	DWORD_PTR Reserved;
	DWORD_PTR UserData;
};

typedef unsigned __int64 FARMENUFLAGS;
static const FARMENUFLAGS
	FMENU_SHOWAMPERSAND        = 0x0000000000000001ULL,
	FMENU_WRAPMODE             = 0x0000000000000002ULL,
	FMENU_AUTOHIGHLIGHT        = 0x0000000000000004ULL,
	FMENU_REVERSEAUTOHIGHLIGHT = 0x0000000000000008ULL,
	FMENU_CHANGECONSOLETITLE   = 0x0000000000000010ULL,
	FMENU_NONE                 = 0;

typedef int (WINAPI *FARAPIMENU)(
	const GUID*         PluginId,
    const GUID*         Id,
    int                 X,
    int                 Y,
    int                 MaxHeight,
    FARMENUFLAGS        Flags,
    const wchar_t      *Title,
    const wchar_t      *Bottom,
    const wchar_t      *HelpTopic,
    const struct FarKey *BreakKeys,
    int                *BreakCode,
    const struct FarMenuItem *Item,
    size_t              ItemsNumber
);


typedef unsigned __int64 PLUGINPANELITEMFLAGS;
static const PLUGINPANELITEMFLAGS
	PPIF_NONE                   = 0,
	PPIF_USERDATA               = 0x0000000020000000ULL,
	PPIF_SELECTED               = 0x0000000040000000ULL,
	PPIF_PROCESSDESCR           = 0x0000000080000000ULL;

struct PluginPanelItem
{
	FILETIME CreationTime;
	FILETIME LastAccessTime;
	FILETIME LastWriteTime;
	FILETIME ChangeTime;
	unsigned __int64 FileSize;
	unsigned __int64 PackSize;
	const wchar_t *FileName;
	const wchar_t *AlternateFileName;
	const wchar_t *Description;
	const wchar_t *Owner;
	const wchar_t * const *CustomColumnData;
	size_t CustomColumnNumber;
	DWORD_PTR UserData;
	PLUGINPANELITEMFLAGS Flags;
	DWORD FileAttributes;
	DWORD NumberOfLinks;
	DWORD CRC32;
	DWORD_PTR Reserved[2];
};

struct FarGetPluginPanelItem
{
	size_t Size;
	struct PluginPanelItem* Item;
};

typedef unsigned __int64 PANELINFOFLAGS;
static const PANELINFOFLAGS
	PFLAGS_NONE               = 0,
	PFLAGS_SHOWHIDDEN         = 0x0000000000000001ULL,
	PFLAGS_HIGHLIGHT          = 0x0000000000000002ULL,
	PFLAGS_REVERSESORTORDER   = 0x0000000000000004ULL,
	PFLAGS_USESORTGROUPS      = 0x0000000000000008ULL,
	PFLAGS_SELECTEDFIRST      = 0x0000000000000010ULL,
	PFLAGS_REALNAMES          = 0x0000000000000020ULL,
	PFLAGS_NUMERICSORT        = 0x0000000000000040ULL,
	PFLAGS_PANELLEFT          = 0x0000000000000080ULL,
	PFLAGS_DIRECTORIESFIRST   = 0x0000000000000100ULL,
	PFLAGS_USECRC32           = 0x0000000000000200ULL,
	PFLAGS_CASESENSITIVESORT  = 0x0000000000000400ULL,
	PFLAGS_PLUGIN             = 0x0000000000000800ULL,
	PFLAGS_VISIBLE            = 0x0000000000001000ULL,
	PFLAGS_FOCUS              = 0x0000000000002000ULL,
	PFLAGS_ALTERNATIVENAMES   = 0x0000000000004000ULL;

enum PANELINFOTYPE
{
	PTYPE_FILEPANEL                 = 0,
	PTYPE_TREEPANEL                 = 1,
	PTYPE_QVIEWPANEL                = 2,
	PTYPE_INFOPANEL                 = 3,
};

enum OPENPANELINFO_SORTMODES
{
	SM_DEFAULT                   =  0,
	SM_UNSORTED                  =  1,
	SM_NAME                      =  2,
	SM_EXT                       =  3,
	SM_MTIME                     =  4,
	SM_CTIME                     =  5,
	SM_ATIME                     =  6,
	SM_SIZE                      =  7,
	SM_DESCR                     =  8,
	SM_OWNER                     =  9,
	SM_COMPRESSEDSIZE            = 10,
	SM_NUMLINKS                  = 11,
	SM_NUMSTREAMS                = 12,
	SM_STREAMSSIZE               = 13,
	SM_FULLNAME                  = 14,
	SM_CHTIME                    = 15,
};

struct PanelInfo
{
	size_t StructSize;
	GUID OwnerGuid;
	HANDLE PluginHandle;
	enum PANELINFOTYPE PanelType;
	RECT PanelRect;
	size_t ItemsNumber;
	size_t SelectedItemsNumber;
	int CurrentItem;
	int TopPanelItem;
	int ViewMode;
	enum OPENPANELINFO_SORTMODES SortMode;
	PANELINFOFLAGS Flags;
	DWORD_PTR Reserved;
};


struct PanelRedrawInfo
{
	int CurrentItem;
	int TopPanelItem;
};

struct CmdLineSelect
{
	int SelStart;
	int SelEnd;
};

#define PANEL_NONE    ((HANDLE)(-1))
#define PANEL_ACTIVE  ((HANDLE)(-1))
#define PANEL_PASSIVE ((HANDLE)(-2))

enum FILE_CONTROL_COMMANDS
{
	FCTL_CLOSEPANEL                 = 0,
	FCTL_GETPANELINFO               = 1,
	FCTL_UPDATEPANEL                = 2,
	FCTL_REDRAWPANEL                = 3,
	FCTL_GETCMDLINE                 = 4,
	FCTL_SETCMDLINE                 = 5,
	FCTL_SETSELECTION               = 6,
	FCTL_SETVIEWMODE                = 7,
	FCTL_INSERTCMDLINE              = 8,
	FCTL_SETUSERSCREEN              = 9,
	FCTL_SETPANELDIR                = 10,
	FCTL_SETCMDLINEPOS              = 11,
	FCTL_GETCMDLINEPOS              = 12,
	FCTL_SETSORTMODE                = 13,
	FCTL_SETSORTORDER               = 14,
	FCTL_SETCMDLINESELECTION        = 15,
	FCTL_GETCMDLINESELECTION        = 16,
	FCTL_CHECKPANELSEXIST           = 17,
	FCTL_SETNUMERICSORT             = 18,
	FCTL_GETUSERSCREEN              = 19,
	FCTL_ISACTIVEPANEL              = 20,
	FCTL_GETPANELITEM               = 21,
	FCTL_GETSELECTEDPANELITEM       = 22,
	FCTL_GETCURRENTPANELITEM        = 23,
	FCTL_GETPANELDIR                = 24,
	FCTL_GETCOLUMNTYPES             = 25,
	FCTL_GETCOLUMNWIDTHS            = 26,
	FCTL_BEGINSELECTION             = 27,
	FCTL_ENDSELECTION               = 28,
	FCTL_CLEARSELECTION             = 29,
	FCTL_SETDIRECTORIESFIRST        = 30,
	FCTL_GETPANELFORMAT             = 31,
	FCTL_GETPANELHOSTFILE           = 32,
	FCTL_SETCASESENSITIVESORT       = 33,
};

typedef void (WINAPI *FARAPITEXT)(
    int X,
    int Y,
    const struct FarColor* Color,
    const wchar_t *Str
);

typedef HANDLE(WINAPI *FARAPISAVESCREEN)(int X1, int Y1, int X2, int Y2);

typedef void (WINAPI *FARAPIRESTORESCREEN)(HANDLE hScreen);


typedef int (WINAPI *FARAPIGETDIRLIST)(
    const wchar_t *Dir,
    struct PluginPanelItem **pPanelItem,
    size_t *pItemsNumber
);

typedef int (WINAPI *FARAPIGETPLUGINDIRLIST)(
    const GUID* PluginId,
    HANDLE hPanel,
    const wchar_t *Dir,
    struct PluginPanelItem **pPanelItem,
    size_t *pItemsNumber
);

typedef void (WINAPI *FARAPIFREEDIRLIST)(struct PluginPanelItem *PanelItem, size_t nItemsNumber);
typedef void (WINAPI *FARAPIFREEPLUGINDIRLIST)(struct PluginPanelItem *PanelItem, size_t nItemsNumber);

typedef unsigned __int64 VIEWER_FLAGS;
static const VIEWER_FLAGS
	VF_NONE                  = 0,
	VF_NONMODAL              = 0x0000000000000001ULL,
	VF_DELETEONCLOSE         = 0x0000000000000002ULL,
	VF_ENABLE_F6             = 0x0000000000000004ULL,
	VF_DISABLEHISTORY        = 0x0000000000000008ULL,
	VF_IMMEDIATERETURN       = 0x0000000000000100ULL,
	VF_DELETEONLYFILEONCLOSE = 0x0000000000000200ULL;

typedef int (WINAPI *FARAPIVIEWER)(
    const wchar_t *FileName,
    const wchar_t *Title,
    int X1,
    int Y1,
    int X2,
    int Y2,
    VIEWER_FLAGS Flags,
    UINT CodePage
);

typedef unsigned __int64 EDITOR_FLAGS;
static const EDITOR_FLAGS
	EF_NONMODAL              = 0x0000000000000001ULL,
	EF_CREATENEW             = 0x0000000000000002ULL,
	EF_ENABLE_F6             = 0x0000000000000004ULL,
	EF_DISABLEHISTORY        = 0x0000000000000008ULL,
	EF_DELETEONCLOSE         = 0x0000000000000010ULL,
	EF_IMMEDIATERETURN       = 0x0000000000000100ULL,
	EF_DELETEONLYFILEONCLOSE = 0x0000000000000200ULL,
	EF_LOCKED                = 0x0000000000000400ULL,
	EF_DISABLESAVEPOS        = 0x0000000000000800ULL,
	EN_NONE                  = 0;

enum EDITOR_EXITCODE
{
	EEC_OPEN_ERROR          = 0,
	EEC_MODIFIED            = 1,
	EEC_NOT_MODIFIED        = 2,
	EEC_LOADING_INTERRUPTED = 3,
};

typedef int (WINAPI *FARAPIEDITOR)(
    const wchar_t *FileName,
    const wchar_t *Title,
    int X1,
    int Y1,
    int X2,
    int Y2,
    EDITOR_FLAGS Flags,
    int StartLine,
    int StartChar,
    UINT CodePage
);

typedef const wchar_t*(WINAPI *FARAPIGETMSG)(
    const GUID* PluginId,
    int MsgId
);

typedef unsigned __int64 FARHELPFLAGS;
static const FARHELPFLAGS
	FHELP_NONE        = 0,
	FHELP_NOSHOWERROR = 0x0000000080000000ULL,
	FHELP_SELFHELP    = 0x0000000000000000ULL,
	FHELP_FARHELP     = 0x0000000000000001ULL,
	FHELP_CUSTOMFILE  = 0x0000000000000002ULL,
	FHELP_CUSTOMPATH  = 0x0000000000000004ULL,
	FHELP_USECONTENTS = 0x0000000040000000ULL;

typedef BOOL (WINAPI *FARAPISHOWHELP)(
    const wchar_t *ModuleName,
    const wchar_t *Topic,
    FARHELPFLAGS Flags
);

enum ADVANCED_CONTROL_COMMANDS
{
	ACTL_GETFARMANAGERVERSION       = 0,
	ACTL_GETSYSWORDDIV              = 1,
	ACTL_WAITKEY                    = 2,
	ACTL_GETCOLOR                   = 3,
	ACTL_GETARRAYCOLOR              = 4,
	ACTL_EJECTMEDIA                 = 5,
	ACTL_GETWINDOWINFO              = 6,
	ACTL_GETWINDOWCOUNT             = 7,
	ACTL_SETCURRENTWINDOW           = 8,
	ACTL_COMMIT                     = 9,
	ACTL_GETFARHWND                 = 10,
	ACTL_GETSYSTEMSETTINGS          = 11,
	ACTL_GETPANELSETTINGS           = 12,
	ACTL_GETINTERFACESETTINGS       = 13,
	ACTL_GETCONFIRMATIONS           = 14,
	ACTL_GETDESCSETTINGS            = 15,
	ACTL_SETARRAYCOLOR              = 16,
	ACTL_GETPLUGINMAXREADDATA       = 17,
	ACTL_GETDIALOGSETTINGS          = 18,
	ACTL_REDRAWALL                  = 19,
	ACTL_SYNCHRO                    = 20,
	ACTL_SETPROGRESSSTATE           = 21,
	ACTL_SETPROGRESSVALUE           = 22,
	ACTL_QUIT                       = 23,
	ACTL_GETFARRECT                 = 24,
	ACTL_GETCURSORPOS               = 25,
	ACTL_SETCURSORPOS               = 26,
	ACTL_PROGRESSNOTIFY             = 27,
	ACTL_GETWINDOWTYPE              = 28,


};

enum FarSystemSettings
{
	FSS_CLEARROATTRIBUTE               = 0x00000001,
	FSS_DELETETORECYCLEBIN             = 0x00000002,
	FSS_USESYSTEMCOPYROUTINE           = 0x00000004,
	FSS_COPYFILESOPENEDFORWRITING      = 0x00000008,
	FSS_CREATEFOLDERSINUPPERCASE       = 0x00000010,
	FSS_SAVECOMMANDSHISTORY            = 0x00000020,
	FSS_SAVEFOLDERSHISTORY             = 0x00000040,
	FSS_SAVEVIEWANDEDITHISTORY         = 0x00000080,
	FSS_USEWINDOWSREGISTEREDTYPES      = 0x00000100,
	FSS_AUTOSAVESETUP                  = 0x00000200,
	FSS_SCANSYMLINK                    = 0x00000400,
};

enum FarPanelSettings
{
	FPS_SHOWHIDDENANDSYSTEMFILES       = 0x00000001,
	FPS_HIGHLIGHTFILES                 = 0x00000002,
	FPS_AUTOCHANGEFOLDER               = 0x00000004,
	FPS_SELECTFOLDERS                  = 0x00000008,
	FPS_ALLOWREVERSESORTMODES          = 0x00000010,
	FPS_SHOWCOLUMNTITLES               = 0x00000020,
	FPS_SHOWSTATUSLINE                 = 0x00000040,
	FPS_SHOWFILESTOTALINFORMATION      = 0x00000080,
	FPS_SHOWFREESIZE                   = 0x00000100,
	FPS_SHOWSCROLLBAR                  = 0x00000200,
	FPS_SHOWBACKGROUNDSCREENSNUMBER    = 0x00000400,
	FPS_SHOWSORTMODELETTER             = 0x00000800,
};

enum FarDialogSettings
{
	FDIS_HISTORYINDIALOGEDITCONTROLS    = 0x00000001,
	FDIS_PERSISTENTBLOCKSINEDITCONTROLS = 0x00000002,
	FDIS_AUTOCOMPLETEININPUTLINES       = 0x00000004,
	FDIS_BSDELETEUNCHANGEDTEXT          = 0x00000008,
	FDIS_DELREMOVESBLOCKS               = 0x00000010,
	FDIS_MOUSECLICKOUTSIDECLOSESDIALOG  = 0x00000020,
};

enum FarInterfaceSettings
{
	FIS_CLOCKINPANELS                  = 0x00000001,
	FIS_CLOCKINVIEWERANDEDITOR         = 0x00000002,
	FIS_MOUSE                          = 0x00000004,
	FIS_SHOWKEYBAR                     = 0x00000008,
	FIS_ALWAYSSHOWMENUBAR              = 0x00000010,
	FIS_SHOWTOTALCOPYPROGRESSINDICATOR = 0x00000100,
	FIS_SHOWCOPYINGTIMEINFO            = 0x00000200,
	FIS_USECTRLPGUPTOCHANGEDRIVE       = 0x00000800,
	FIS_SHOWTOTALDELPROGRESSINDICATOR  = 0x00001000,
};

enum FarConfirmationsSettings
{
	FCS_COPYOVERWRITE                  = 0x00000001,
	FCS_MOVEOVERWRITE                  = 0x00000002,
	FCS_DRAGANDDROP                    = 0x00000004,
	FCS_DELETE                         = 0x00000008,
	FCS_DELETENONEMPTYFOLDERS          = 0x00000010,
	FCS_INTERRUPTOPERATION             = 0x00000020,
	FCS_DISCONNECTNETWORKDRIVE         = 0x00000040,
	FCS_RELOADEDITEDFILE               = 0x00000080,
	FCS_CLEARHISTORYLIST               = 0x00000100,
	FCS_EXIT                           = 0x00000200,
	FCS_OVERWRITEDELETEROFILES         = 0x00000400,
};

enum FarDescriptionSettings
{
	FDS_UPDATEALWAYS                   = 0x00000001,
	FDS_UPDATEIFDISPLAYED              = 0x00000002,
	FDS_SETHIDDEN                      = 0x00000004,
	FDS_UPDATEREADONLY                 = 0x00000008,
};

typedef unsigned __int64 FAREJECTMEDIAFLAGS;
static const FAREJECTMEDIAFLAGS
	EJECT_NO_MESSAGE                    = 0x0000000000000001ULL,
	EJECT_LOAD_MEDIA                    = 0x0000000000000002ULL,
	EJECT_NONE                          = 0;

struct ActlEjectMedia
{
	DWORD Letter;
	FAREJECTMEDIAFLAGS Flags;
};



enum FAR_MACRO_CONTROL_COMMANDS
{
	MCTL_LOADALL           = 0,
	MCTL_SAVEALL           = 1,
	MCTL_SENDSTRING        = 2,
	MCTL_GETSTATE          = 5,
	MCTL_GETAREA           = 6,
	MCTL_ADDMACRO          = 7,
	MCTL_DELMACRO          = 8,
};

typedef unsigned __int64 FARKEYMACROFLAGS;
static const FARKEYMACROFLAGS
	KMFLAGS_NONE                = 0,
	KMFLAGS_DISABLEOUTPUT       = 0x0000000000000001,
	KMFLAGS_NOSENDKEYSTOPLUGINS = 0x0000000000000002,
	KMFLAGS_SILENTCHECK         = 0x0000000000000001;

enum FARMACROSENDSTRINGCOMMAND
{
	MSSC_POST              =0,
	MSSC_CHECK             =2,
};

enum FARMACROAREA
{
	MACROAREA_OTHER             = 0,
	MACROAREA_SHELL             = 1,
	MACROAREA_VIEWER            = 2,
	MACROAREA_EDITOR            = 3,
	MACROAREA_DIALOG            = 4,
	MACROAREA_SEARCH            = 5,
	MACROAREA_DISKS             = 6,
	MACROAREA_MAINMENU          = 7,
	MACROAREA_MENU              = 8,
	MACROAREA_HELP              = 9,
	MACROAREA_INFOPANEL         =10,
	MACROAREA_QVIEWPANEL        =11,
	MACROAREA_TREEPANEL         =12,
	MACROAREA_FINDFOLDER        =13,
	MACROAREA_USERMENU          =14,
	MACROAREA_AUTOCOMPLETION    =15,
};

enum FARMACROSTATE
{
	MACROSTATE_NOMACRO          = 0,
	MACROSTATE_EXECUTING        = 1,
	MACROSTATE_EXECUTING_COMMON = 2,
	MACROSTATE_RECORDING        = 3,
	MACROSTATE_RECORDING_COMMON = 4,
};

enum FARMACROPARSEERRORCODE
{
	MPEC_SUCCESS                = 0,
	MPEC_UNRECOGNIZED_KEYWORD   = 1,
	MPEC_UNRECOGNIZED_FUNCTION  = 2,
	MPEC_FUNC_PARAM             = 3,
	MPEC_NOT_EXPECTED_ELSE      = 4,
	MPEC_NOT_EXPECTED_END       = 5,
	MPEC_UNEXPECTED_EOS         = 6,
	MPEC_EXPECTED_TOKEN         = 7,
	MPEC_BAD_HEX_CONTROL_CHAR   = 8,
	MPEC_BAD_CONTROL_CHAR       = 9,
	MPEC_VAR_EXPECTED           =10,
	MPEC_EXPR_EXPECTED          =11,
	MPEC_ZEROLENGTHMACRO        =12,
	MPEC_INTPARSERERROR         =13,
	MPEC_CONTINUE_OTL           =14,
};

struct MacroParseResult
{
	size_t StructSize;
	DWORD ErrCode;
	COORD ErrPos;
	const wchar_t *ErrSrc;
};


struct MacroSendMacroText
{
	size_t StructSize;
	FARKEYMACROFLAGS Flags;
	INPUT_RECORD AKey;
	const wchar_t *SequenceText;
};

struct MacroCheckMacroText
{
	union
	{
		struct MacroSendMacroText Text;
		struct MacroParseResult   Result;
	}
#ifndef __cplusplus
	Check
#endif
	;
};

typedef unsigned __int64 FARADDKEYMACROFLAGS;
static const FARADDKEYMACROFLAGS
	AKMFLAGS_NONE                = 0;

typedef int (WINAPI *FARMACROCALLBACK)(void* Id,FARADDKEYMACROFLAGS Flags);

struct MacroAddMacro
{
	size_t StructSize;
	FARKEYMACROFLAGS Flags;
	INPUT_RECORD AKey;
	const wchar_t *SequenceText;
	const wchar_t *Description;
	void* Id;
	FARMACROCALLBACK Callback;
};



typedef unsigned __int64 FARSETCOLORFLAGS;
static const FARSETCOLORFLAGS

	FSETCLR_NONE                   = 0,
	FSETCLR_REDRAW                 = 0x0000000000000001ULL;

struct FarSetColors
{
	FARSETCOLORFLAGS Flags;
	size_t StartIndex;
	size_t ColorsCount;
	struct FarColor* Colors;
};

enum WINDOWINFO_TYPE
{
	WTYPE_PANELS                    = 1,
	WTYPE_VIEWER                    = 2,
	WTYPE_EDITOR                    = 3,
	WTYPE_DIALOG                    = 4,
	WTYPE_VMENU                     = 5,
	WTYPE_HELP                      = 6,
};

typedef unsigned __int64 WINDOWINFO_FLAGS;
static const WINDOWINFO_FLAGS
	WIF_MODIFIED = 0x0000000000000001ULL,
	WIF_CURRENT  = 0x0000000000000002ULL;

struct WindowInfo
{
	size_t StructSize;
	INT_PTR Id;
	wchar_t *TypeName;
	wchar_t *Name;
	int TypeNameSize;
	int NameSize;
	int Pos;
	enum WINDOWINFO_TYPE Type;
	WINDOWINFO_FLAGS Flags;
};

struct WindowType
{
	size_t StructSize;
	int Type;
};

enum PROGRESSTATE
{
	PS_NOPROGRESS   =0x0,
	PS_INDETERMINATE=0x1,
	PS_NORMAL       =0x2,
	PS_ERROR        =0x4,
	PS_PAUSED       =0x8,
};

struct ProgressValue
{
	unsigned __int64 Completed;
	unsigned __int64 Total;
};

enum VIEWER_CONTROL_COMMANDS
{
	VCTL_GETINFO                    = 0,
	VCTL_QUIT                       = 1,
	VCTL_REDRAW                     = 2,
	VCTL_SETKEYBAR                  = 3,
	VCTL_SETPOSITION                = 4,
	VCTL_SELECT                     = 5,
	VCTL_SETMODE                    = 6,
};

typedef unsigned __int64 VIEWER_OPTIONS;
static const VIEWER_OPTIONS
	VOPT_NONE               = 0x0000000000000000ULL,
	VOPT_SAVEFILEPOSITION   = 0x0000000000000001ULL,
	VOPT_AUTODETECTCODEPAGE = 0x0000000000000002ULL;

enum VIEWER_SETMODE_TYPES
{
	VSMT_HEX                        = 0,
	VSMT_WRAP                       = 1,
	VSMT_WORDWRAP                   = 2,
};

typedef unsigned __int64 VIEWER_SETMODEFLAGS_TYPES;
static const VIEWER_SETMODEFLAGS_TYPES
	VSMFL_REDRAW    = 0x0000000000000001ULL;

struct ViewerSetMode
{
	enum VIEWER_SETMODE_TYPES Type;
	union
	{
		int iParam;
		wchar_t *wszParam;
	}
#ifndef __cplusplus
	Param
#endif
	;
	VIEWER_SETMODEFLAGS_TYPES Flags;
	DWORD_PTR Reserved;
};

struct ViewerSelect
{
	__int64 BlockStartPos;
	int     BlockLen;
};

typedef unsigned __int64 VIEWER_SETPOS_FLAGS;
static const VIEWER_SETPOS_FLAGS
	VSP_NOREDRAW    = 0x0000000000000001ULL,
	VSP_PERCENT     = 0x0000000000000002ULL,
	VSP_RELATIVE    = 0x0000000000000004ULL,
	VSP_NORETNEWPOS = 0x0000000000000008ULL;

struct ViewerSetPosition
{
	VIEWER_SETPOS_FLAGS Flags;
	__int64 StartPos;
	__int64 LeftPos;
};

struct ViewerMode
{
	UINT CodePage;
	int Wrap;
	int WordWrap;
	int Hex;
	DWORD_PTR Reserved[4];
};

struct ViewerInfo
{
	size_t StructSize;
	struct ViewerMode CurMode;
	__int64 FileSize;
	__int64 FilePos;
	__int64 LeftPos;
	VIEWER_OPTIONS Options;
	const wchar_t *FileName;
	int ViewerID;
	int WindowSizeX;
	int WindowSizeY;
	int TabSize;
};

enum VIEWER_EVENTS
{
	VE_READ       =0,
	VE_CLOSE      =1,

	VE_GOTFOCUS   =6,
	VE_KILLFOCUS  =7,
};


enum EDITOR_EVENTS
{
	EE_READ       =0,
	EE_SAVE       =1,
	EE_REDRAW     =2,
	EE_CLOSE      =3,

	EE_GOTFOCUS   =6,
	EE_KILLFOCUS  =7,
};

enum DIALOG_EVENTS
{
	DE_DLGPROCINIT    =0,
	DE_DEFDLGPROCINIT =1,
	DE_DLGPROCEND     =2,
};

enum SYNCHRO_EVENTS
{
	SE_COMMONSYNCHRO  =0,
};

#define EEREDRAW_ALL    (void*)0
#define EEREDRAW_CHANGE (void*)1
#define EEREDRAW_LINE   (void*)2

enum EDITOR_CONTROL_COMMANDS
{
	ECTL_GETSTRING                  = 0,
	ECTL_SETSTRING                  = 1,
	ECTL_INSERTSTRING               = 2,
	ECTL_DELETESTRING               = 3,
	ECTL_DELETECHAR                 = 4,
	ECTL_INSERTTEXT                 = 5,
	ECTL_GETINFO                    = 6,
	ECTL_SETPOSITION                = 7,
	ECTL_SELECT                     = 8,
	ECTL_REDRAW                     = 9,
	ECTL_TABTOREAL                  = 10,
	ECTL_REALTOTAB                  = 11,
	ECTL_EXPANDTABS                 = 12,
	ECTL_SETTITLE                   = 13,
	ECTL_READINPUT                  = 14,
	ECTL_PROCESSINPUT               = 15,
	ECTL_ADDCOLOR                   = 16,
	ECTL_GETCOLOR                   = 17,
	ECTL_SAVEFILE                   = 18,
	ECTL_QUIT                       = 19,
	ECTL_SETKEYBAR                  = 20,
	ECTL_PROCESSKEY                 = 21,
	ECTL_SETPARAM                   = 22,
	ECTL_GETBOOKMARKS               = 23,
	ECTL_TURNOFFMARKINGBLOCK        = 24,
	ECTL_DELETEBLOCK                = 25,
	ECTL_ADDSTACKBOOKMARK           = 26,
	ECTL_PREVSTACKBOOKMARK          = 27,
	ECTL_NEXTSTACKBOOKMARK          = 28,
	ECTL_CLEARSTACKBOOKMARKS        = 29,
	ECTL_DELETESTACKBOOKMARK        = 30,
	ECTL_GETSTACKBOOKMARKS          = 31,
	ECTL_UNDOREDO                   = 32,
	ECTL_GETFILENAME                = 33,
	ECTL_DELCOLOR                   = 34,
};

enum EDITOR_SETPARAMETER_TYPES
{
	ESPT_TABSIZE                    = 0,
	ESPT_EXPANDTABS                 = 1,
	ESPT_AUTOINDENT                 = 2,
	ESPT_CURSORBEYONDEOL            = 3,
	ESPT_CHARCODEBASE               = 4,
	ESPT_CODEPAGE                   = 5,
	ESPT_SAVEFILEPOSITION           = 6,
	ESPT_LOCKMODE                   = 7,
	ESPT_SETWORDDIV                 = 8,
	ESPT_GETWORDDIV                 = 9,
	ESPT_SHOWWHITESPACE             = 10,
	ESPT_SETBOM                     = 11,
};



struct EditorSetParameter
{
	enum EDITOR_SETPARAMETER_TYPES Type;
	union
	{
		int iParam;
		wchar_t *wszParam;
		DWORD_PTR Reserved;
	}
#ifndef __cplusplus
	Param
#endif
	;
	unsigned __int64 Flags;
	size_t Size;
};


enum EDITOR_UNDOREDO_COMMANDS
{
	EUR_BEGIN                       = 0,
	EUR_END                         = 1,
	EUR_UNDO                        = 2,
	EUR_REDO                        = 3,
};


struct EditorUndoRedo
{
	enum EDITOR_UNDOREDO_COMMANDS Command;
	DWORD_PTR Reserved[3];
};

struct EditorGetString
{
	int StringNumber;
	int StringLength;
	const wchar_t *StringText;
	const wchar_t *StringEOL;
	int SelStart;
	int SelEnd;
};


struct EditorSetString
{
	int StringNumber;
	int StringLength;
	const wchar_t *StringText;
	const wchar_t *StringEOL;
};

enum EXPAND_TABS
{
	EXPAND_NOTABS                   = 0,
	EXPAND_ALLTABS                  = 1,
	EXPAND_NEWTABS                  = 2,
};


enum EDITOR_OPTIONS
{
	EOPT_EXPANDALLTABS     = 0x00000001,
	EOPT_PERSISTENTBLOCKS  = 0x00000002,
	EOPT_DELREMOVESBLOCKS  = 0x00000004,
	EOPT_AUTOINDENT        = 0x00000008,
	EOPT_SAVEFILEPOSITION  = 0x00000010,
	EOPT_AUTODETECTCODEPAGE= 0x00000020,
	EOPT_CURSORBEYONDEOL   = 0x00000040,
	EOPT_EXPANDONLYNEWTABS = 0x00000080,
	EOPT_SHOWWHITESPACE    = 0x00000100,
	EOPT_BOM               = 0x00000200,
};


enum EDITOR_BLOCK_TYPES
{
	BTYPE_NONE                      = 0,
	BTYPE_STREAM                    = 1,
	BTYPE_COLUMN                    = 2,
};

enum EDITOR_CURRENTSTATE
{
	ECSTATE_MODIFIED       = 0x00000001,
	ECSTATE_SAVED          = 0x00000002,
	ECSTATE_LOCKED         = 0x00000004,
};


struct EditorInfo
{
	int EditorID;
	int WindowSizeX;
	int WindowSizeY;
	int TotalLines;
	int CurLine;
	int CurPos;
	int CurTabPos;
	int TopScreenLine;
	int LeftPos;
	int Overtype;
	int BlockType;
	int BlockStartLine;
	DWORD Options;
	int TabSize;
	int BookMarkCount;
	DWORD CurState;
	UINT CodePage;
	DWORD_PTR Reserved[5];
};

struct EditorBookMarks
{
	int *Line;
	int *Cursor;
	int *ScreenLine;
	int *LeftPos;
	DWORD_PTR Reserved[4];
};

struct EditorSetPosition
{
	int CurLine;
	int CurPos;
	int CurTabPos;
	int TopScreenLine;
	int LeftPos;
	int Overtype;
};


struct EditorSelect
{
	int BlockType;
	int BlockStartLine;
	int BlockStartPos;
	int BlockWidth;
	int BlockHeight;
};


struct EditorConvertPos
{
	int StringNumber;
	int SrcPos;
	int DestPos;
};


typedef unsigned __int64 EDITORCOLORFLAGS;
static const EDITORCOLORFLAGS
	ECF_TAB1 = 0x0000000000000001ULL;

struct EditorColor
{
	size_t StructSize;
	int StringNumber;
	int ColorItem;
	int StartPos;
	int EndPos;
	EDITORCOLORFLAGS Flags;
	struct FarColor Color;
	GUID Owner;
	unsigned Priority;
};

struct EditorDeleteColor
{
	size_t StructSize;
	GUID Owner;
	int StringNumber;
	int StartPos;
};

#define EDITOR_COLOR_NORMAL_PRIORITY 0x80000000U

struct EditorSaveFile
{
	const wchar_t *FileName;
	const wchar_t *FileEOL;
	UINT CodePage;
};

typedef unsigned __int64 INPUTBOXFLAGS;
static const INPUTBOXFLAGS
	FIB_ENABLEEMPTY      = 0x0000000000000001ULL,
	FIB_PASSWORD         = 0x0000000000000002ULL,
	FIB_EXPANDENV        = 0x0000000000000004ULL,
	FIB_NOUSELASTHISTORY = 0x0000000000000008ULL,
	FIB_BUTTONS          = 0x0000000000000010ULL,
	FIB_NOAMPERSAND      = 0x0000000000000020ULL,
	FIB_EDITPATH         = 0x0000000000000040ULL,
	FIB_NONE             = 0;

typedef int (WINAPI *FARAPIINPUTBOX)(
    const GUID* PluginId,
    const GUID* Id,
    const wchar_t *Title,
    const wchar_t *SubTitle,
    const wchar_t *HistoryName,
    const wchar_t *SrcText,
    wchar_t *DestText,
    int   DestLength,
    const wchar_t *HelpTopic,
    INPUTBOXFLAGS Flags
);

enum FAR_PLUGINS_CONTROL_COMMANDS
{
	PCTL_LOADPLUGIN         = 0,
	PCTL_UNLOADPLUGIN       = 1,
	PCTL_FORCEDLOADPLUGIN   = 2,
};

enum FAR_PLUGIN_LOAD_TYPE
{
	PLT_PATH = 0,
};

enum FAR_FILE_FILTER_CONTROL_COMMANDS
{
	FFCTL_CREATEFILEFILTER          = 0,
	FFCTL_FREEFILEFILTER            = 1,
	FFCTL_OPENFILTERSMENU           = 2,
	FFCTL_STARTINGTOFILTER          = 3,
	FFCTL_ISFILEINFILTER            = 4,
};

enum FAR_FILE_FILTER_TYPE
{
	FFT_PANEL                       = 0,
	FFT_FINDFILE                    = 1,
	FFT_COPY                        = 2,
	FFT_SELECT                      = 3,
	FFT_CUSTOM                      = 4,
};

enum FAR_REGEXP_CONTROL_COMMANDS
{
	RECTL_CREATE                    = 0,
	RECTL_FREE                      = 1,
	RECTL_COMPILE                   = 2,
	RECTL_OPTIMIZE                  = 3,
	RECTL_MATCHEX                   = 4,
	RECTL_SEARCHEX                  = 5,
	RECTL_BRACKETSCOUNT             = 6,
};

struct RegExpMatch
{
	int start,end;
};

struct RegExpSearch
{
	const wchar_t* Text;
	int Position;
	int Length;
	struct RegExpMatch* Match;
	int Count;
	void* Reserved;
};

enum FAR_SETTINGS_CONTROL_COMMANDS
{
	SCTL_CREATE                     = 0,
	SCTL_FREE                       = 1,
	SCTL_SET                        = 2,
	SCTL_GET                        = 3,
	SCTL_ENUM                       = 4,
	SCTL_DELETE                     = 5,
	SCTL_CREATESUBKEY               = 6,
	SCTL_OPENSUBKEY                 = 7,
};

enum FARSETTINGSTYPES
{
	FST_UNKNOWN                     = 0,
	FST_SUBKEY                      = 1,
	FST_QWORD                       = 2,
	FST_STRING                      = 3,
	FST_DATA                        = 4,
};

struct FarSettingsCreate
{
	size_t StructSize;
	GUID Guid;
	HANDLE Handle;
};

struct FarSettingsItem
{
	size_t Root;
	const wchar_t* Name;
	enum FARSETTINGSTYPES Type;
	union
	{
		unsigned __int64 Number;
		const wchar_t* String;
		struct
		{
			size_t Size;
			const void* Data;
		} Data;
	}
#ifndef __cplusplus
	Value
#endif
	;
};

struct FarSettingsName
{
	const wchar_t* Name;
	enum FARSETTINGSTYPES Type;
};

struct FarSettingsEnum
{
	size_t Root;
	size_t Count;
	const struct FarSettingsName* Items;
};

struct FarSettingsValue
{
	size_t Root;
	const wchar_t* Value;
};

typedef INT_PTR (WINAPI *FARAPIPANELCONTROL)(
    HANDLE hPanel,
    enum FILE_CONTROL_COMMANDS Command,
    int Param1,
    void* Param2
);

typedef INT_PTR(WINAPI *FARAPIADVCONTROL)(
    const GUID* PluginId,
    enum ADVANCED_CONTROL_COMMANDS Command,
    int Param1,
    void* Param2
);

typedef INT_PTR (WINAPI *FARAPIVIEWERCONTROL)(
    int ViewerID,
    enum VIEWER_CONTROL_COMMANDS Command,
    int Param1,
    void* Param2
);

typedef INT_PTR (WINAPI *FARAPIEDITORCONTROL)(
    int EditorID,
    enum EDITOR_CONTROL_COMMANDS Command,
    int Param1,
    void* Param2
);

typedef INT_PTR (WINAPI *FARAPIMACROCONTROL)(
    const GUID* PluginId,
    enum FAR_MACRO_CONTROL_COMMANDS Command,
    int Param1,
    void* Param2
);

typedef INT_PTR (WINAPI *FARAPIPLUGINSCONTROL)(
    HANDLE hHandle,
    enum FAR_PLUGINS_CONTROL_COMMANDS Command,
    int Param1,
    void* Param2
);

typedef INT_PTR (WINAPI *FARAPIFILEFILTERCONTROL)(
    HANDLE hHandle,
    enum FAR_FILE_FILTER_CONTROL_COMMANDS Command,
    int Param1,
    void* Param2
);

typedef INT_PTR (WINAPI *FARAPIREGEXPCONTROL)(
    HANDLE hHandle,
    enum FAR_REGEXP_CONTROL_COMMANDS Command,
    int Param1,
    void* Param2
);

typedef INT_PTR (WINAPI *FARAPISETTINGSCONTROL)(
    HANDLE hHandle,
    enum FAR_SETTINGS_CONTROL_COMMANDS Command,
    int Param1,
    void* Param2
);

// <C&C++>
typedef int (WINAPIV *FARSTDSPRINTF)(wchar_t *Buffer,const wchar_t *Format,...);
typedef int (WINAPIV *FARSTDSNPRINTF)(wchar_t *Buffer,size_t Sizebuf,const wchar_t *Format,...);
typedef int (WINAPIV *FARSTDSSCANF)(const wchar_t *Buffer, const wchar_t *Format,...);
// </C&C++>
typedef void (WINAPI *FARSTDQSORT)(void *base, size_t nelem, size_t width, int (__cdecl *fcmp)(const void *, const void *));
typedef void (WINAPI *FARSTDQSORTEX)(void *base, size_t nelem, size_t width, int (__cdecl *fcmp)(const void *, const void *,void *userparam),void *userparam);
typedef void   *(WINAPI *FARSTDBSEARCH)(const void *key, const void *base, size_t nelem, size_t width, int (__cdecl *fcmp)(const void *, const void *));
typedef size_t (WINAPI *FARSTDGETFILEOWNER)(const wchar_t *Computer,const wchar_t *Name,wchar_t *Owner,size_t Size);
typedef int (WINAPI *FARSTDGETNUMBEROFLINKS)(const wchar_t *Name);
typedef int (WINAPI *FARSTDATOI)(const wchar_t *s);
typedef __int64(WINAPI *FARSTDATOI64)(const wchar_t *s);
typedef wchar_t   *(WINAPI *FARSTDITOA64)(__int64 value, wchar_t *string, int radix);
typedef wchar_t   *(WINAPI *FARSTDITOA)(int value, wchar_t *string, int radix);
typedef wchar_t   *(WINAPI *FARSTDLTRIM)(wchar_t *Str);
typedef wchar_t   *(WINAPI *FARSTDRTRIM)(wchar_t *Str);
typedef wchar_t   *(WINAPI *FARSTDTRIM)(wchar_t *Str);
typedef wchar_t   *(WINAPI *FARSTDTRUNCSTR)(wchar_t *Str,int MaxLength);
typedef wchar_t   *(WINAPI *FARSTDTRUNCPATHSTR)(wchar_t *Str,int MaxLength);
typedef wchar_t   *(WINAPI *FARSTDQUOTESPACEONLY)(wchar_t *Str);
typedef const wchar_t*(WINAPI *FARSTDPOINTTONAME)(const wchar_t *Path);
typedef BOOL (WINAPI *FARSTDADDENDSLASH)(wchar_t *Path);
typedef int (WINAPI *FARSTDCOPYTOCLIPBOARD)(const wchar_t *Data);
typedef wchar_t *(WINAPI *FARSTDPASTEFROMCLIPBOARD)(void);
typedef int (WINAPI *FARSTDLOCALISLOWER)(wchar_t Ch);
typedef int (WINAPI *FARSTDLOCALISUPPER)(wchar_t Ch);
typedef int (WINAPI *FARSTDLOCALISALPHA)(wchar_t Ch);
typedef int (WINAPI *FARSTDLOCALISALPHANUM)(wchar_t Ch);
typedef wchar_t (WINAPI *FARSTDLOCALUPPER)(wchar_t LowerChar);
typedef wchar_t (WINAPI *FARSTDLOCALLOWER)(wchar_t UpperChar);
typedef void (WINAPI *FARSTDLOCALUPPERBUF)(wchar_t *Buf,int Length);
typedef void (WINAPI *FARSTDLOCALLOWERBUF)(wchar_t *Buf,int Length);
typedef void (WINAPI *FARSTDLOCALSTRUPR)(wchar_t *s1);
typedef void (WINAPI *FARSTDLOCALSTRLWR)(wchar_t *s1);
typedef int (WINAPI *FARSTDLOCALSTRICMP)(const wchar_t *s1,const wchar_t *s2);
typedef int (WINAPI *FARSTDLOCALSTRNICMP)(const wchar_t *s1,const wchar_t *s2,int n);

typedef unsigned __int64 PROCESSNAME_FLAGS;
static const PROCESSNAME_FLAGS
	PN_CMPNAME      = 0x0000000000000000ULL,
	PN_CMPNAMELIST  = 0x0000000000010000ULL,
	PN_GENERATENAME = 0x0000000000020000ULL,
	PN_SKIPPATH     = 0x0000000001000000ULL;

typedef size_t (WINAPI *FARSTDPROCESSNAME)(const wchar_t *param1, wchar_t *param2, size_t size, PROCESSNAME_FLAGS flags);

typedef void (WINAPI *FARSTDUNQUOTE)(wchar_t *Str);

typedef unsigned __int64 XLAT_FLAGS;
static const XLAT_FLAGS
	XLAT_SWITCHKEYBLAYOUT  = 0x0000000000000001ULL,
	XLAT_SWITCHKEYBBEEP    = 0x0000000000000002ULL,
	XLAT_USEKEYBLAYOUTNAME = 0x0000000000000004ULL,
	XLAT_CONVERTALLCMDLINE = 0x0000000000010000ULL;


typedef size_t (WINAPI *FARSTDINPUTRECORDTOKEYNAME)(const INPUT_RECORD* Key, wchar_t *KeyText, size_t Size);

typedef wchar_t*(WINAPI *FARSTDXLAT)(wchar_t *Line,int StartPos,int EndPos,XLAT_FLAGS Flags);

typedef BOOL (WINAPI *FARSTDKEYNAMETOINPUTRECORD)(const wchar_t *Name,INPUT_RECORD* Key);

typedef int (WINAPI *FRSUSERFUNC)(
    const struct PluginPanelItem *FData,
    const wchar_t *FullName,
    void *Param
);

typedef unsigned __int64 FRSMODE;
static const FRSMODE
	FRS_RETUPDIR             = 0x0000000000000001ULL,
	FRS_RECUR                = 0x0000000000000002ULL,
	FRS_SCANSYMLINK          = 0x0000000000000004ULL;

typedef void (WINAPI *FARSTDRECURSIVESEARCH)(const wchar_t *InitDir,const wchar_t *Mask,FRSUSERFUNC Func,FRSMODE Flags,void *Param);
typedef size_t (WINAPI *FARSTDMKTEMP)(wchar_t *Dest, size_t DestSize, const wchar_t *Prefix);
typedef void (WINAPI *FARSTDDELETEBUFFER)(void *Buffer);
typedef size_t (WINAPI *FARSTDGETPATHROOT)(const wchar_t *Path,wchar_t *Root, size_t DestSize);

enum LINK_TYPE
{
	LINK_HARDLINK         = 1,
	LINK_JUNCTION         = 2,
	LINK_VOLMOUNT         = 3,
	LINK_SYMLINKFILE      = 4,
	LINK_SYMLINKDIR       = 5,
	LINK_SYMLINK          = 6,
};

typedef unsigned __int64 MKLINK_FLAGS;
static const MKLINK_FLAGS
	MLF_NONE             = 0,
	MLF_SHOWERRMSG       = 0x0000000000010000ULL,
	MLF_DONOTUPDATEPANEL = 0x0000000000020000ULL;

typedef BOOL (WINAPI *FARSTDMKLINK)(const wchar_t *Src,const wchar_t *Dest,enum LINK_TYPE Type, MKLINK_FLAGS Flags);
typedef size_t (WINAPI *FARGETREPARSEPOINTINFO)(const wchar_t *Src, wchar_t *Dest, size_t DestSize);

enum CONVERTPATHMODES
{
	CPM_FULL                        = 0,
	CPM_REAL                        = 1,
	CPM_NATIVE                      = 2,
};

typedef size_t (WINAPI *FARCONVERTPATH)(enum CONVERTPATHMODES Mode, const wchar_t *Src, wchar_t *Dest, size_t DestSize);

typedef size_t (WINAPI *FARGETCURRENTDIRECTORY)(size_t Size, wchar_t* Buffer);

typedef struct FarStandardFunctions
{
	size_t StructSize;

	FARSTDATOI                 atoi;
	FARSTDATOI64               atoi64;
	FARSTDITOA                 itoa;
	FARSTDITOA64               itoa64;
	// <C&C++>
	FARSTDSPRINTF              sprintf;
	FARSTDSSCANF               sscanf;
	// </C&C++>
	FARSTDQSORT                qsort;
	FARSTDBSEARCH              bsearch;
	FARSTDQSORTEX              qsortex;
	// <C&C++>
	FARSTDSNPRINTF             snprintf;
	// </C&C++>

	DWORD_PTR                  Reserved[8];

	FARSTDLOCALISLOWER         LIsLower;
	FARSTDLOCALISUPPER         LIsUpper;
	FARSTDLOCALISALPHA         LIsAlpha;
	FARSTDLOCALISALPHANUM      LIsAlphanum;
	FARSTDLOCALUPPER           LUpper;
	FARSTDLOCALLOWER           LLower;
	FARSTDLOCALUPPERBUF        LUpperBuf;
	FARSTDLOCALLOWERBUF        LLowerBuf;
	FARSTDLOCALSTRUPR          LStrupr;
	FARSTDLOCALSTRLWR          LStrlwr;
	FARSTDLOCALSTRICMP         LStricmp;
	FARSTDLOCALSTRNICMP        LStrnicmp;

	FARSTDUNQUOTE              Unquote;
	FARSTDLTRIM                LTrim;
	FARSTDRTRIM                RTrim;
	FARSTDTRIM                 Trim;
	FARSTDTRUNCSTR             TruncStr;
	FARSTDTRUNCPATHSTR         TruncPathStr;
	FARSTDQUOTESPACEONLY       QuoteSpaceOnly;
	FARSTDPOINTTONAME          PointToName;
	FARSTDGETPATHROOT          GetPathRoot;
	FARSTDADDENDSLASH          AddEndSlash;
	FARSTDCOPYTOCLIPBOARD      CopyToClipboard;
	FARSTDPASTEFROMCLIPBOARD   PasteFromClipboard;
	FARSTDINPUTRECORDTOKEYNAME FarInputRecordToName;
	FARSTDKEYNAMETOINPUTRECORD FarNameToInputRecord;
	FARSTDXLAT                 XLat;
	FARSTDGETFILEOWNER         GetFileOwner;
	FARSTDGETNUMBEROFLINKS     GetNumberOfLinks;
	FARSTDRECURSIVESEARCH      FarRecursiveSearch;
	FARSTDMKTEMP               MkTemp;
	FARSTDDELETEBUFFER         DeleteBuffer;
	FARSTDPROCESSNAME          ProcessName;
	FARSTDMKLINK               MkLink;
	FARCONVERTPATH             ConvertPath;
	FARGETREPARSEPOINTINFO     GetReparsePointInfo;
	FARGETCURRENTDIRECTORY     GetCurrentDirectory;
} FARSTANDARDFUNCTIONS;

struct PluginStartupInfo
{
	size_t StructSize;
	const wchar_t *ModuleName;
	FARAPIMENU             Menu;
	FARAPIMESSAGE          Message;
	FARAPIGETMSG           GetMsg;
	FARAPIPANELCONTROL     PanelControl;
	FARAPISAVESCREEN       SaveScreen;
	FARAPIRESTORESCREEN    RestoreScreen;
	FARAPIGETDIRLIST       GetDirList;
	FARAPIGETPLUGINDIRLIST GetPluginDirList;
	FARAPIFREEDIRLIST      FreeDirList;
	FARAPIFREEPLUGINDIRLIST FreePluginDirList;
	FARAPIVIEWER           Viewer;
	FARAPIEDITOR           Editor;
	FARAPITEXT             Text;
	FARAPIEDITORCONTROL    EditorControl;

	FARSTANDARDFUNCTIONS  *FSF;

	FARAPISHOWHELP         ShowHelp;
	FARAPIADVCONTROL       AdvControl;
	FARAPIINPUTBOX         InputBox;
	FARAPICOLORDIALOG      ColorDialog;
	FARAPIDIALOGINIT       DialogInit;
	FARAPIDIALOGRUN        DialogRun;
	FARAPIDIALOGFREE       DialogFree;

	FARAPISENDDLGMESSAGE   SendDlgMessage;
	FARAPIDEFDLGPROC       DefDlgProc;
	DWORD_PTR              Reserved;
	FARAPIVIEWERCONTROL    ViewerControl;
	FARAPIPLUGINSCONTROL   PluginsControl;
	FARAPIFILEFILTERCONTROL FileFilterControl;
	FARAPIREGEXPCONTROL    RegExpControl;
	FARAPIMACROCONTROL     MacroControl;
	FARAPISETTINGSCONTROL  SettingsControl;
};


typedef unsigned __int64 PLUGIN_FLAGS;
static const PLUGIN_FLAGS
	PF_NONE           = 0,
	PF_PRELOAD        = 0x0000000000000001ULL,
	PF_DISABLEPANELS  = 0x0000000000000002ULL,
	PF_EDITOR         = 0x0000000000000004ULL,
	PF_VIEWER         = 0x0000000000000008ULL,
	PF_FULLCMDLINE    = 0x0000000000000010ULL,
	PF_DIALOG         = 0x0000000000000020ULL;

struct PluginMenuItem
{
	const GUID *Guids;
	const wchar_t * const *Strings;
	int Count;
};

enum VERSION_STAGE
{
	VS_RELEASE                      = 0,
	VS_ALPHA                        = 1,
	VS_BETA                         = 2,
	VS_RC                           = 3,
};

struct VersionInfo
{
	DWORD Major;
	DWORD Minor;
	DWORD Revision;
	DWORD Build;
	enum VERSION_STAGE Stage;
};

static __inline BOOL CheckVersion(const struct VersionInfo* Current, const struct VersionInfo* Required)
{
	return (Current->Major > Required->Major) || (Current->Major == Required->Major && Current->Minor > Required->Minor) || (Current->Major == Required->Major && Current->Minor == Required->Minor && Current->Revision > Required->Revision) || (Current->Major == Required->Major && Current->Minor == Required->Minor && Current->Revision == Required->Revision && Current->Build >= Required->Build);
}

static __inline struct VersionInfo MAKEFARVERSION(DWORD Major, DWORD Minor, DWORD Revision, DWORD Build, enum VERSION_STAGE Stage)
{
	struct VersionInfo Info = {Major, Minor, Revision, Build, Stage};
	return Info;
}

#define FARMANAGERVERSION MAKEFARVERSION(FARMANAGERVERSION_MAJOR,FARMANAGERVERSION_MINOR, FARMANAGERVERSION_REVISION, FARMANAGERVERSION_BUILD, FARMANAGERVERSION_STAGE)

struct GlobalInfo
{
	size_t StructSize;
	struct VersionInfo MinFarVersion;
	struct VersionInfo Version;
	GUID Guid;
	const wchar_t *Title;
	const wchar_t *Description;
	const wchar_t *Author;
};

struct PluginInfo
{
	size_t StructSize;
	PLUGIN_FLAGS Flags;
	struct PluginMenuItem DiskMenu;
	struct PluginMenuItem PluginMenu;
	struct PluginMenuItem PluginConfig;
	const wchar_t *CommandPrefix;
};



struct InfoPanelLine
{
	const wchar_t *Text;
	const wchar_t *Data;
	int  Separator;
};

typedef unsigned __int64 PANELMODE_FLAGS;
static const PANELMODE_FLAGS
	PMFLAGS_FULLSCREEN      = 0x0000000000000001ULL,
	PMFLAGS_DETAILEDSTATUS  = 0x0000000000000002ULL,
	PMFLAGS_ALIGNEXTENSIONS = 0x0000000000000004ULL,
	PMFLAGS_CASECONVERSION  = 0x0000000000000008ULL;

struct PanelMode
{
	size_t StructSize;
	const wchar_t *ColumnTypes;
	const wchar_t *ColumnWidths;
	const wchar_t * const *ColumnTitles;
	const wchar_t *StatusColumnTypes;
	const wchar_t *StatusColumnWidths;
	PANELMODE_FLAGS Flags;
};

typedef unsigned __int64 OPENPANELINFO_FLAGS;
static const OPENPANELINFO_FLAGS
	OPIF_NONE                    = 0,
	OPIF_DISABLEFILTER           = 0x0000000000000001ULL,
	OPIF_DISABLESORTGROUPS       = 0x0000000000000002ULL,
	OPIF_DISABLEHIGHLIGHTING     = 0x0000000000000004ULL,
	OPIF_ADDDOTS                 = 0x0000000000000008ULL,
	OPIF_RAWSELECTION            = 0x0000000000000010ULL,
	OPIF_REALNAMES               = 0x0000000000000020ULL,
	OPIF_SHOWNAMESONLY           = 0x0000000000000040ULL,
	OPIF_SHOWRIGHTALIGNNAMES     = 0x0000000000000080ULL,
	OPIF_SHOWPRESERVECASE        = 0x0000000000000100ULL,
	OPIF_COMPAREFATTIME          = 0x0000000000000400ULL,
	OPIF_EXTERNALGET             = 0x0000000000000800ULL,
	OPIF_EXTERNALPUT             = 0x0000000000001000ULL,
	OPIF_EXTERNALDELETE          = 0x0000000000002000ULL,
	OPIF_EXTERNALMKDIR           = 0x0000000000004000ULL,
	OPIF_USEATTRHIGHLIGHTING     = 0x0000000000008000ULL,
	OPIF_USECRC32                = 0x0000000000010000ULL,
	OPIF_USEFREESIZE             = 0x0000000000020000ULL;

struct KeyBarLabel
{
	struct FarKey Key;
	const wchar_t *Text;
	const wchar_t *LongText;
};

struct KeyBarTitles
{
	size_t CountLabels;
	struct KeyBarLabel *Labels;
};

typedef unsigned __int64 OPERATION_MODES;
static const OPERATION_MODES
	OPM_NONE       =0,
	OPM_SILENT     =0x0000000000000001ULL,
	OPM_FIND       =0x0000000000000002ULL,
	OPM_VIEW       =0x0000000000000004ULL,
	OPM_EDIT       =0x0000000000000008ULL,
	OPM_TOPLEVEL   =0x0000000000000010ULL,
	OPM_DESCR      =0x0000000000000020ULL,
	OPM_QUICKVIEW  =0x0000000000000040ULL,
	OPM_PGDN       =0x0000000000000080ULL;

struct OpenPanelInfo
{
	size_t                       StructSize;
	HANDLE                       hPanel;
	OPENPANELINFO_FLAGS          Flags;
	const wchar_t               *HostFile;
	const wchar_t               *CurDir;
	const wchar_t               *Format;
	const wchar_t               *PanelTitle;
	const struct InfoPanelLine  *InfoLines;
	size_t                       InfoLinesNumber;
	const wchar_t * const       *DescrFiles;
	size_t                       DescrFilesNumber;
	const struct PanelMode      *PanelModesArray;
	size_t                       PanelModesNumber;
	int                          StartPanelMode;
	enum OPENPANELINFO_SORTMODES StartSortMode;
	int                          StartSortOrder;
	const struct KeyBarTitles   *KeyBar;
	const wchar_t               *ShortcutData;
	unsigned __int64             FreeSize;
};

struct AnalyseInfo
{
	size_t          StructSize;
	const wchar_t  *FileName;
	void           *Buffer;
	size_t          BufferSize;
	OPERATION_MODES OpMode;
};

enum OPENFROM
{
	OPEN_FROM_MASK          = 0x000000FF,

	OPEN_LEFTDISKMENU       = 0,
	OPEN_PLUGINSMENU        = 1,
	OPEN_FINDLIST           = 2,
	OPEN_SHORTCUT           = 3,
	OPEN_COMMANDLINE        = 4,
	OPEN_EDITOR             = 5,
	OPEN_VIEWER             = 6,
	OPEN_FILEPANEL          = 7,
	OPEN_DIALOG             = 8,
	OPEN_ANALYSE            = 9,
	OPEN_RIGHTDISKMENU      = 10,

	OPEN_FROMMACRO_MASK     = 0x000F0000,

	OPEN_FROMMACRO          = 0x00010000,
	OPEN_FROMMACROSTRING    = 0x00020000,
};


enum FAR_EVENTS
{
	FE_CHANGEVIEWMODE =0,
	FE_REDRAW         =1,
	FE_IDLE           =2,
	FE_CLOSE          =3,
	FE_BREAK          =4,
	FE_COMMAND        =5,

	FE_GOTFOCUS       =6,
	FE_KILLFOCUS      =7,
};

struct OpenInfo
{
	size_t StructSize;
	enum OPENFROM OpenFrom;
	const GUID* Guid;
	INT_PTR Data;
};

struct SetDirectoryInfo
{
	size_t StructSize;
	HANDLE hPanel;
	const wchar_t *Dir;
	OPERATION_MODES OpMode;
	INT_PTR UserData;
};

struct SetFindListInfo
{
	size_t StructSize;
	HANDLE hPanel;
	const struct PluginPanelItem *PanelItem;
	size_t ItemsNumber;
};

struct PutFilesInfo
{
	size_t StructSize;
	HANDLE hPanel;
	struct PluginPanelItem *PanelItem;
	size_t ItemsNumber;
	BOOL Move;
	const wchar_t *SrcPath;
	OPERATION_MODES OpMode;
};

struct ProcessHostFileInfo
{
	size_t StructSize;
	HANDLE hPanel;
	struct PluginPanelItem *PanelItem;
	size_t ItemsNumber;
	OPERATION_MODES OpMode;
};

struct MakeDirectoryInfo
{
	size_t StructSize;
	HANDLE hPanel;
	const wchar_t *Name;
	OPERATION_MODES OpMode;
};

struct CompareInfo
{
	size_t StructSize;
	HANDLE hPanel;
	const struct PluginPanelItem *Item1;
	const struct PluginPanelItem *Item2;
	enum OPENPANELINFO_SORTMODES Mode;
};

struct GetFindDataInfo
{
	size_t StructSize;
	HANDLE hPanel;
	struct PluginPanelItem *PanelItem;
	size_t ItemsNumber;
	OPERATION_MODES OpMode;
};

struct GetVirtualFindDataInfo
{
	size_t StructSize;
	HANDLE hPanel;
	struct PluginPanelItem *PanelItem;
	size_t ItemsNumber;
	const wchar_t *Path;
};

struct FreeFindDataInfo
{
	size_t StructSize;
	HANDLE hPanel;
	struct PluginPanelItem *PanelItem;
	size_t ItemsNumber;
};

struct GetFilesInfo
{
	size_t StructSize;
	HANDLE hPanel;
	struct PluginPanelItem *PanelItem;
	size_t ItemsNumber;
	BOOL Move;
	const wchar_t *DestPath;
	OPERATION_MODES OpMode;
};

struct DeleteFilesInfo
{
	size_t StructSize;
	HANDLE hPanel;
	struct PluginPanelItem *PanelItem;
	size_t ItemsNumber;
	OPERATION_MODES OpMode;
};

struct ProcessPanelInputInfo
{
	size_t StructSize;
	HANDLE hPanel;
	INPUT_RECORD Rec;
};

struct ProcessEditorInputInfo
{
	size_t StructSize;
	INPUT_RECORD Rec;
};

typedef unsigned __int64 PROCESSCONSOLEINPUT_FLAGS;
static const PROCESSCONSOLEINPUT_FLAGS
	PCIF_NONE            =0,
	PCIF_FROMMAIN        =0x0000000000000001ULL;

struct ProcessConsoleInputInfo
{
	size_t StructSize;
	PROCESSCONSOLEINPUT_FLAGS Flags;
	const INPUT_RECORD *Rec;
	HANDLE hPanel;
};

struct ExitInfo
{
	size_t StructSize;
};

struct ProcessPanelEventInfo
{
	size_t StructSize;
	HANDLE hPanel;
	int Event;
	void* Param;
};

struct ProcessEditorEventInfo
{
	size_t StructSize;
	int Event;
	void* Param;
};

struct ProcessDialogEventInfo
{
	size_t StructSize;
	int Event;
	struct FarDialogEvent* Param;
};

struct ProcessSynchroEventInfo
{
	size_t StructSize;
	int Event;
	void* Param;
};

struct ProcessViewerEventInfo
{
	size_t StructSize;
	int Event;
	void* Param;
};

struct ClosePanelInfo
{
	size_t StructSize;
	HANDLE hPanel;
};

struct ConfigureInfo
{
	size_t StructSize;
	const GUID* Guid;
};

#ifdef __cplusplus
extern "C"
{
#endif
// Exported Functions

	int    WINAPI AnalyseW(const struct AnalyseInfo *Info);
	void   WINAPI ClosePanelW(const struct ClosePanelInfo *Info);
	int    WINAPI CompareW(const struct CompareInfo *Info);
	int    WINAPI ConfigureW(const struct ConfigureInfo *Info);
	int    WINAPI DeleteFilesW(const struct DeleteFilesInfo *Info);
	void   WINAPI ExitFARW(const struct ExitInfo *Info);
	void   WINAPI FreeFindDataW(const struct FreeFindDataInfo *Info);
	void   WINAPI FreeVirtualFindDataW(const struct FreeFindDataInfo *Info);
	int    WINAPI GetFilesW(struct GetFilesInfo *Info);
	int    WINAPI GetFindDataW(struct GetFindDataInfo *Info);
	void   WINAPI GetGlobalInfoW(struct GlobalInfo *Info);
	void   WINAPI GetOpenPanelInfoW(struct OpenPanelInfo *Info);
	void   WINAPI GetPluginInfoW(struct PluginInfo *Info);
	int    WINAPI GetVirtualFindDataW(struct GetVirtualFindDataInfo *Info);
	int    WINAPI MakeDirectoryW(struct MakeDirectoryInfo *Info);
	HANDLE WINAPI OpenW(const struct OpenInfo *Info);
	//int    WINAPI ProcessDialogEventW(const struct ProcessDialogEventInfo *Info);
	//int    WINAPI ProcessEditorEventW(const struct ProcessEditorEventInfo *Info);
	int    WINAPI ProcessEditorInputW(const struct ProcessEditorInputInfo *Info);
	int    WINAPI ProcessPanelEventW(const struct ProcessPanelEventInfo *Info);
	int    WINAPI ProcessHostFileW(const struct ProcessHostFileInfo *Info);
	int    WINAPI ProcessPanelInputW(const struct ProcessPanelInputInfo *Info);
	int    WINAPI ProcessConsoleInputW(struct ProcessConsoleInputInfo *Info);
	//int    WINAPI ProcessSynchroEventW(const struct ProcessSynchroEventInfo *Info);
	//int    WINAPI ProcessViewerEventW(const struct ProcessViewerEventInfo *Info);
	int    WINAPI PutFilesW(const struct PutFilesInfo *Info);
	int    WINAPI SetDirectoryW(const struct SetDirectoryInfo *Info);
	int    WINAPI SetFindListW(const struct SetFindListInfo *Info);
	void   WINAPI SetStartupInfoW(const struct PluginStartupInfo *Info);

#ifdef __cplusplus
};
#endif

#endif /* RC_INVOKED */

#endif /* __PLUGIN_HPP__ */
