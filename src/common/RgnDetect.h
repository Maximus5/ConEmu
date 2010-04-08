#pragma once

#define MAX_DETECTED_DIALOGS 20

#include <pshpack1.h>
typedef struct tag_CharAttr
{
	TODO("OPTIMIZE: Заменить бы битовые поля на один DWORD, в котором хранить некий общий ИД стиля, заполняемый при формировании буфера");
	union {
		// Собственно цвета/шрифты
		struct {
			unsigned int crForeColor : 24; // чтобы в ui64 поместился и nFontIndex
			unsigned int nFontIndex : 8; // 0 - normal, 1 - bold, 2 - italic
			unsigned int crBackColor : 32; // Старший байт зарезервируем, вдруг для прозрачности понадобится
			unsigned int nForeIdx : 8;
			unsigned int nBackIdx : 8; // может понадобиться для ExtendColors
			unsigned int crOrigForeColor : 32;
			unsigned int crOrigBackColor : 32; // Реальные цвета в консоли, crForeColor и crBackColor могут быть изменены колорером
			// вспомогательные флаги
			unsigned int bDialog : 1;
			unsigned int bDialogVBorder : 1;
			unsigned int bDialogCorner : 1;
			unsigned int bSomeFilled : 1;
			unsigned int bTransparent : 1; // UserScreen
		};
		// А это для сравнения (поиск изменений)
		unsigned __int64 All;
		// для сравнения, когда фон не важен
		unsigned int ForeFont;
	};
	//
	//DWORD dwAttrubutes; // может когда понадобятся дополнительные флаги...
	//
    ///**
    // * Used exclusively by ConsoleView to append annotations to each character
    // */
    //AnnotationInfo annotationInfo;
} CharAttr;
#include <poppack.h>

inline bool operator==(const CharAttr& s1, const CharAttr& s2)
{
    return s1.All == s2.All;
}



class CRgnDetect
{
public:
	// Initializers
	CRgnDetect();
	~CRgnDetect();
	
public:
	// Public methods
	int GetDetectedDialogs(int anMaxCount, SMALL_RECT* rc, bool* rb);
	void PrepareTransparent(const CEFAR_INFO *apFarInfo, const COLORREF *apColors, const CONSOLE_SCREEN_BUFFER_INFO *apSbi, wchar_t* pChar, CharAttr* pAttr, int nWidth, int nHeight);
	
	
protected:
	// Private methods
	void DetectDialog(wchar_t* pChar, CharAttr* pAttr, int nWidth, int nHeight, int nFromX, int nFromY, int *pnMostRight=NULL, int *pnMostBottom=NULL);
	bool FindDialog_TopLeft(wchar_t* pChar, CharAttr* pAttr, int nWidth, int nHeight, int &nFromX, int &nFromY, int &nMostRight, int &nMostBottom, bool &bMarkBorder);
	bool FindDialog_TopRight(wchar_t* pChar, CharAttr* pAttr, int nWidth, int nHeight, int &nFromX, int &nFromY, int &nMostRight, int &nMostBottom, bool &bMarkBorder);
	bool FindDialog_Left(wchar_t* pChar, CharAttr* pAttr, int nWidth, int nHeight, int &nFromX, int &nFromY, int &nMostRight, int &nMostBottom, bool &bMarkBorder);
	bool FindDialog_Right(wchar_t* pChar, CharAttr* pAttr, int nWidth, int nHeight, int &nFromX, int &nFromY, int &nMostRight, int &nMostBottom, bool &bMarkBorder);
	bool FindDialog_Any(wchar_t* pChar, CharAttr* pAttr, int nWidth, int nHeight, int &nFromX, int &nFromY, int &nMostRight, int &nMostBottom, bool &bMarkBorder);
	bool FindDialog_Inner(wchar_t* pChar, CharAttr* pAttr, int nWidth, int nHeight, int &nFromX, int &nFromY);
	bool FindFrame_TopLeft(wchar_t* pChar, CharAttr* pAttr, int nWidth, int nHeight, int &nFromX, int &nFromY, int &nFrameX, int &nFrameY);
	bool FindFrameTop_ByRight(wchar_t* pChar, CharAttr* pAttr, int nWidth, int nHeight, int &nFromX, int &nFromY, int &nMostTop);
	bool FindFrameTop_ByLeft(wchar_t* pChar, CharAttr* pAttr, int nWidth, int nHeight, int &nFromX, int &nFromY, int &nMostTop);
	bool FindFrameBottom_ByRight(wchar_t* pChar, CharAttr* pAttr, int nWidth, int nHeight, int &nFromX, int &nFromY, int &nMostBottom);
	bool FindFrameBottom_ByLeft(wchar_t* pChar, CharAttr* pAttr, int nWidth, int nHeight, int &nFromX, int &nFromY, int &nMostBottom);
	bool FindFrameRight_ByTop(wchar_t* pChar, CharAttr* pAttr, int nWidth, int nHeight, int &nFromX, int &nFromY, int &nMostRight);
	bool FindFrameRight_ByBottom(wchar_t* pChar, CharAttr* pAttr, int nWidth, int nHeight, int &nFromX, int &nFromY, int &nMostRight);
	bool FindFrameLeft_ByTop(wchar_t* pChar, CharAttr* pAttr, int nWidth, int nHeight, int &nFromX, int &nFromY, int &nMostLeft);
	bool FindFrameLeft_ByBottom(wchar_t* pChar, CharAttr* pAttr, int nWidth, int nHeight, int &nFromX, int &nFromY, int &nMostLeft);
	// Последний шанс
	bool FindByBackground(wchar_t* pChar, CharAttr* pAttr, int nWidth, int nHeight, int &nFromX, int &nFromY, int &nMostRight, int &nMostBottom, bool &bMarkBorder);
	// Сервисная
	bool ExpandDialogFrame(wchar_t* pChar, CharAttr* pAttr, int nWidth, int nHeight, int &nFromX, int &nFromY, int nFrameX, int nFrameY, int &nMostRight, int &nMostBottom);
	void MarkDialog(wchar_t* pChar, CharAttr* pAttr, int nWidth, int nHeight, int nX1, int nY1, int nX2, int nY2, bool bMarkBorder = false, bool bFindExterior = true);
	bool ConsoleRect2ScreenRect(const RECT &rcCon, RECT *prcScr);
	

protected:
	// Members
	bool    mb_SelfBuffers;
	const CEFAR_INFO *mp_FarInfo;
	const COLORREF *mp_Colors;
	CONSOLE_SCREEN_BUFFER_INFO m_sbi;
	bool   mb_BufferHeight;


	int     mn_DetectCallCount;
	struct {
		int Count;
		SMALL_RECT Rects[MAX_DETECTED_DIALOGS];
		bool bWasFrame[MAX_DETECTED_DIALOGS];
	} m_DetectedDialogs;
};
