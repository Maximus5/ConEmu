
#pragma once

#define BACKGROUND_FILE_POLL 5000

class CBackground
{
public:
	COORD    bgSize;
	HDC      hBgDc;

	CBackground();
	virtual ~CBackground();

	bool CreateField(int anWidth, int anHeight);
	bool FillBackground(const BITMAPFILEHEADER* apBkImgData, LONG X, LONG Y, LONG Width, LONG Height, BackgroundOp Operation, bool abFade);

	SetBackgroundResult SetPluginBackgroundImageData(CESERVER_REQ_SETBACKGROUND* apImgData, bool&/*OUT*/ bUpdate);
	bool HasPluginBackgroundImage(LONG* pnBgWidth, LONG* pnBgHeight);
	bool PrepareBackground(CVirtualConsole* pVCon, HDC&/*OUT*/ phBgDc, COORD&/*OUT*/ pbgBmpSize);

	static UINT IsBackgroundValid(const CESERVER_REQ_SETBACKGROUND* apImgData, bool* rpIsEmf);

private:
	HBITMAP  hBgBitmap, hOldBitmap;
	void Destroy();

	#ifdef __GNUC__
	AlphaBlend_t GdiAlphaBlend;
	#endif

	void NeedBackgroundUpdate();


	bool PutPluginBackgroundImage(/*CBackground* pBack,*/ LONG X, LONG Y, LONG Width, LONG Height);

	// Данные, которые приходят из плагина ("ConEmu Background" и пр.)
	bool mb_NeedBgUpdate;
	bool mb_BgLastFade;
	//CBackground* mp_PluginBg;
	MSection *mcs_BkImgData;
	size_t mn_BkImgDataMax;
	CESERVER_REQ_SETBACKGROUND* mp_BkImgData; // followed by image data
	bool mb_BkImgChanged; // Данные в mp_BkImgData были изменены плагином, требуется отрисовка
	bool mb_BkImgExist; //, mb_BkImgDelete;
	LONG mn_BkImgWidth, mn_BkImgHeight;
	// Поддержка EMF
	size_t mn_BkEmfDataMax;
	CESERVER_REQ_SETBACKGROUND* mp_BkEmfData; // followed by EMF data
	bool mb_BkEmfChanged; // Данные в mp_BkEmfData были изменены плагином, требуется отрисовка
};


#ifdef APPDISTINCTBACKGROUND

#include "RefRelease.h"

class CBackgroundInfo : public CRefRelease
{
protected:
	wchar_t   ms_BgImage[MAX_PATH]; // tBgImage
	FILETIME  ftBgModified;
	DWORD     nBgModifiedTick;
	bool      mb_NeedBgUpdate;

	bool mb_IsBackgroundImageValid;

	BITMAPFILEHEADER* mp_BgImgData; // Результат LoadImageEx

protected:
	virtual ~CBackgroundInfo();

public:
	CBackgroundInfo();

	void FinalRelease();

	const wchar_t* BgImage();
	//bool IsFade();

	void NeedBackgroundUpdate();

	bool PollBackgroundFile(/* const Settings::AppSettings* pApp */);
	bool LoadBackgroundFile(const wchar_t *inPath, bool abShowErrors);
	bool PrepareBackground(HDC* phBgDc, COORD* pbgBmpSize);
};

#endif
