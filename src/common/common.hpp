#include <windows.h>

//#define CONEMUPIPE      L"\\\\.\\pipe\\ConEmuPipe%u"
//#define CONEMUEVENTIN   L"ConEmuInEvent%u"
//#define CONEMUEVENTOUT  L"ConEmuOutEvent%u"
//#define CONEMUEVENTPIPE L"ConEmuPipeEvent%u"

#define CESERVERPIPENAME    L"\\\\%s\\pipe\\ConEmuSrv%u"
#define CESERVERINPUTNAME   L"\\\\%s\\pipe\\ConEmuSrvInput%u"
#define CECMD_GETSHORTINFO  1
#define CECMD_GETFULLINFO   2
#define CECMD_SETSIZE       3
#define CECMD_WRITEINPUT    4

#define CESERVER_REQ_VER    1

typedef struct tag_CESERVER_REQ {
	DWORD   nSize;
	DWORD   nCmd;
	DWORD   nVersion;
	BYTE    Data[0x1000]; // вообще-то размер динамический
} CESERVER_REQ;


#define CONEMUMAPPING    L"ConEmuPluginData%u"
#define CONEMUDRAGFROM   L"ConEmuDragFrom%u"
#define CONEMUDRAGTO     L"ConEmuDragTo%u"
#define CONEMUREQTABS    L"ConEmuReqTabs%u"
#define CONEMUSETWINDOW  L"ConEmuSetWindow%u"
#define CONEMUPOSTMACRO  L"ConEmuPostMacro%u"
#define CONEMUDEFFONT    L"ConEmuDefFont%u"
#define CONEMULANGCHANGE L"ConEmuLangChange%u"
#define CONEMUEXIT       L"ConEmuExit%u"
#define CONEMUALIVE      L"ConEmuAlive%u"
#define CONEMUREADY      L"ConEmuReady%u"
#define CONEMUTABCHANGED L"ConEmuTabsChanged"
#define CMD_DRAGFROM     0
#define CMD_DRAGTO       1
#define CMD_REQTABS      2
#define CMD_SETWINDOW    3
#define CMD_POSTMACRO    4
#define CMD_DEFFONT      5
#define CMD_LANGCHANGE   6
// +2
#define MAXCMDCOUNT      8
#define CMD_EXIT         MAXCMDCOUNT-1

#define GWL_TABINDEX     0
#define GWL_LANGCHANGE   4

#ifdef _DEBUG
	#define CONEMUALIVETIMEOUT INFINITE
	#define CONEMUREADYTIMEOUT INFINITE
	#define CONEMUFARTIMEOUT   10000 // Сколько ожидать, пока ФАР среагирует на вызов плагина
#else
	#define CONEMUALIVETIMEOUT 1000  // Живость плагина ждем секунду
	#define CONEMUREADYTIMEOUT 10000 // А на выполнение команды - 10s max
	#define CONEMUFARTIMEOUT   10000 // Сколько ожидать, пока ФАР среагирует на вызов плагина
#endif

#define CONEMUTABMAX 0x400
struct ConEmuTab
{
	int  Pos;
	int  Current;
	int  Type;
	int  Modified;
	wchar_t Name[CONEMUTABMAX];
//	int  Modified;
//	int isEditor;
};

struct ForwardedPanelInfo
{
	RECT ActiveRect;
	RECT PassiveRect;
	int ActivePathShift; // сдвиг в этой структуре в байтах
	int PassivePathShift; // сдвиг в этой структуре в байтах
	WCHAR* pszActivePath/*[MAX_PATH+1]*/;
	WCHAR* pszPassivePath/*[MAX_PATH+1]*/;
};

struct FarVersion {
	union {
		DWORD dwVer;
		struct {
			WORD dwVerMinor;
			WORD dwVerMajor;
		};
	};
	DWORD dwBuild;
};

struct ForwardedFileInfo
{
	WCHAR Path[MAX_PATH+1];
};

/*enum PipeCmd
{
	SetTabs=0,
	DragFrom,
	DragTo
};*/

// ConEmu.dll экспортирует следующие функции
//HWND WINAPI GetFarHWND();
//void WINAPI _export GetFarVersion ( FarVersion* pfv );
