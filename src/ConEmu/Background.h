
#pragma once

class CBackground
{
	public:
		COORD    bgSize;
		HDC      hBgDc;

		CBackground();
		~CBackground();

		bool CreateField(int anWidth, int anHeight);
		bool FillBackground(const BITMAPFILEHEADER* apBkImgData, LONG X, LONG Y, LONG Width, LONG Height, BackgroundOp Operation, bool abFade);

	private:
		HBITMAP  hBgBitmap, hOldBitmap;
		void Destroy();

		#ifdef __GNUC__
		AlphaBlend_t GdiAlphaBlend;
		#endif
};