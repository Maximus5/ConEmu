
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

private:
	HBITMAP  hBgBitmap, hOldBitmap;
	void Destroy();

	#ifdef __GNUC__
	AlphaBlend_t GdiAlphaBlend;
	#endif
};


#ifdef APPDISTINCTBACKGROUND

class CBackgroundFile
{
protected:
	wchar_t   sBgImage[MAX_PATH]; // tBgImage
	FILETIME  ftBgModified;
	DWORD     nBgModifiedTick;
	bool      mb_IsFade;
	bool      mb_NeedBgUpdate;

public:
	CBackgroundFile();
	virtual ~CBackgroundFile();

	const wchar_t* BgImage();
	bool IsFade();

	void NeedBackgroundUpdate();

	bool PollBackgroundFile(/* const Settings::AppSettings* pApp */);
	bool LoadBackgroundFile(const wchar_t *inPath, bool abShowErrors);
	bool PrepareBackground(HDC* phBgDc, COORD* pbgBmpSize);
};

#endif
