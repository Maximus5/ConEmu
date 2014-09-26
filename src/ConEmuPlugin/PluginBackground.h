
#pragma once

class MSection;

class CPluginBackground
{
	protected:
		struct RegisterBackgroundArg *mp_BgPlugins;
		int mn_BgPluginsCount, mn_BgPluginsMax;
		BOOL mb_BgWasSent;
		BOOL mb_BgErrorShown;
		MSection *csBgPlugins;
		DWORD m_LastThSetCheck;
		BOOL mb_ThNeedLoad;
		PanelViewSetMapping m_ThSet;
		struct PaintBackgroundArg m_Default, m_Last;
		bool IsParmChanged(struct PaintBackgroundArg* p1, struct PaintBackgroundArg* p2);
		//// Buffers
		//wchar_t ms_LeftCurDir[32768], ms_LeftFormat[MAX_PATH], ms_LeftHostFile[32768];
		//wchar_t ms_RightCurDir[32768], ms_RightFormat[MAX_PATH], ms_RightHostFile[32768];

		// bitmask values
		enum RequiredActions
		{
			ra_UpdateBackground = 1,
			ra_CheckPanelFolders = 2,
			//rs_ForceUpdate = 4,
		};
		DWORD mn_ReqActions;

		void ReallocItems(int nAddCount);
		BOOL LoadThSet(BOOL abFromMainThread);
		void SetDcPanelRect(RECT *rcDc, BkPanelInfo *Panel, PaintBackgroundArg *Arg);

		/* Вызывается только в thread-safe (Synchro) - begin */
		void CheckPanelFolders(int anForceSetPlace = 0);
		void UpdateBackground();
		static void UpdateBackground_Exec(struct RegisterBackgroundArg *pPlugin, struct PaintBackgroundArg *pArg);
		/* end- Вызывается только в thread-safe (Synchro)*/

	public:
		CPluginBackground();
		~CPluginBackground();

		void SetForceCheck();
		void SetForceUpdate(bool bFlagsOnly = false);
		void SetForceThLoad();

		// Может вызываться в произвольном потоке
		int RegisterSubplugin(RegisterBackgroundArg *pbk);

		// Вызывается только в thread-safe (Synchro)
		void OnMainThreadActivated(int anEditorEvent = -1, int anViewerEvent = -1);

		// Вызывается из фонового потока
		void MonitorBackground();
};

extern BOOL gbBgPluginsAllowed; // TRUE после ExitFar
extern BOOL gbNeedBgActivate; // требуется активация модуля в главной нити
extern CPluginBackground *gpBgPlugin;
