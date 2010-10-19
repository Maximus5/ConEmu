
#pragma once

class CPluginBackground
{
protected:
	struct BackgroundInfo *mp_BgPlugins;
	int mn_BgPluginsCount, mn_BgPluginsMax;
	MSection *csBgPlugins;
	DWORD m_LastThSetCheck;
	PanelViewSettings m_ThSet;
	struct UpdateBackgroundArg m_Default;
	//// Buffers
	//wchar_t ms_LeftCurDir[32768], ms_LeftFormat[MAX_PATH], ms_LeftHostFile[32768];
	//wchar_t ms_RightCurDir[32768], ms_RightFormat[MAX_PATH], ms_RightHostFile[32768];
	
	// bitmask values
	enum RequiredActions {
		ra_UpdateBackground = 1,
		ra_CheckPanelFolders = 2,
		//rs_XXX = 4,
	};
	DWORD mn_ReqActions;
	
	void ReallocItems(int nAddCount);
	BOOL LoadThSet();
	void SetDcPanelRect(RECT *rcDc, UpdateBackgroundArg::BkPanelInfo *Panel, UpdateBackgroundArg *Arg);

	/* Вызывается только в thread-safe (Synchro) - begin */
	void CheckPanelFolders();
	void UpdateBackground();
	/* end- Вызывается только в thread-safe (Synchro)*/
	
public:
	CPluginBackground();
	~CPluginBackground();
	
	// Может вызываться в произвольном потоке
	int RegisterSubplugin(BackgroundInfo *pbk);

	// Вызывается только в thread-safe (Synchro)
	void OnMainThreadActivated(int anEditorEvent = -1, int anViewerEvent = -1);
	
	// Вызывается из фонового потока
	void MonitorBackground();
};

extern BOOL gbBgPluginsAllowed; // TRUE после ExitFar
extern BOOL gbNeedBgActivate; // требуется активация модуля в главной нити
extern CPluginBackground *gpBgPlugin;
