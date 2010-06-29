BOOL WINAPI CET_Init(struct CET_Init* pInit)
{
	_ASSERTE(pInit->cbSize >= sizeof(struct CET_Init));
	if (pInit->cbSize < sizeof(struct CET_Init)) {
		pInit->nErrNumber = PGE_OLD_PLUGIN;
		return FALSE;
	}

	ghModule = pInit->hModule;

	GDIPlusDecoder *pDecoder = (GDIPlusDecoder*) CALLOC(sizeof(GDIPlusDecoder));
	if (!pDecoder) {
		pInit->nErrNumber = PGE_NOT_ENOUGH_MEMORY;
		return FALSE;
	}
	pDecoder->nMagic = eGdiStr_Decoder;
	if (!pDecoder->Init(pInit)) {
		pInit->nErrNumber = pDecoder->nErrNumber;
		pDecoder->Close();
		//FREE(pDecoder);
		return FALSE;
	}

	_ASSERTE(pInit->pContext == (LPVOID)pDecoder); // ”же вернул pDecoder->Init
	return TRUE;
}

VOID WINAPI CET_Done(struct CET_Init* pInit)
{
	if (pInit) {
		GDIPlusDecoder *pDecoder = (GDIPlusDecoder*)pInit->pContext;
		if (pDecoder) {
			_ASSERTE(pDecoder->nMagic == eGdiStr_Decoder);
			if (pDecoder->nMagic == eGdiStr_Decoder) {
				pDecoder->Close();
				//FREE(pDecoder);
			}
		}
	}
}


#define SETERROR(n) if (pLoadPreview) pLoadPreview->nErrNumber = n;


BOOL WINAPI CET_Load(struct CET_LoadInfo* pLoadPreview)
{
	if (!pLoadPreview || *((LPDWORD)pLoadPreview) != sizeof(struct CET_LoadInfo)) {
		_ASSERTE(*((LPDWORD)pLoadPreview) == sizeof(struct CET_LoadInfo));
		SETERROR(PGE_INVALID_VERSION);
		return FALSE;
	}
	
	
	if (!pLoadPreview->pContext) {
		SETERROR(PGE_INVALID_CONTEXT);
		return FALSE;
	}
	
	if (pLoadPreview->bVirtualItem && (!pLoadPreview->pFileData || !pLoadPreview->nFileSize)) {
		SETERROR(PGE_FILE_NOT_FOUND);
		return FALSE;
	}
	
	if (pLoadPreview->nFileSize < 16 || pLoadPreview->nFileSize > 209715200/*200 MB*/) {
		SETERROR(PGE_UNSUPPORTEDFORMAT);
		return FALSE;
	}
	
	BOOL lbKnown = FALSE;
	
	if (pLoadPreview->pFileData) {
		const BYTE  *pb  = (const BYTE*)pLoadPreview->pFileData;
		const WORD  *pw  = (const WORD*)pLoadPreview->pFileData;
		const DWORD *pdw = (const DWORD*)pLoadPreview->pFileData;
		
		TODO("ICO - убрать. пусть специализированный занимаетс€")
		
		if (*pdw==0x474E5089 /* ЙPNG */)
			lbKnown = TRUE;
		else if (*pw==0x4D42 /* BM */)
			lbKnown = TRUE;
		else if (pb[0]==0xFF && pb[1]==0xD8 && pb[2]==0xFF) // JPEG?
			lbKnown = TRUE;
		else if (pw[0]==0x4949) // TIFF?
			lbKnown = TRUE;
		else if (pw[0]==0 && (pw[1]==1/*ICON*/ || pw[1]==2/*CURSOR*/) && (pw[2]>0 && pw[2]<=64/*IMG COUNT*/)) // .ico, .cur
			lbKnown = TRUE;
		else if (*pdw == 0x38464947 /*GIF8*/)
			lbKnown = TRUE;
		else
		{
			const wchar_t* pszFile = NULL;
			wchar_t szExt[6]; szExt[0] = 0;
			pszFile = wcsrchr(pLoadPreview->sFileName, L'\\');
			if (pszFile) {
				pszFile = wcsrchr(pszFile, L'.');
				if (pszFile) {
					int nLen = lstrlenW(pszFile);
					if (nLen && nLen<5) {
						lstrcpyW(szExt, pszFile+1);
						CharLowerBuffW(szExt, nLen-1);
					}
				}
			}
		
			if ((szExt[0]==L'w' || szExt[0]==L'e') && szExt[1]==L'm' && szExt[2]==L'f')
				lbKnown = TRUE;
		}
	}
	if (!lbKnown) {
		SETERROR(PGE_UNSUPPORTEDFORMAT);
		return FALSE;
	}

	
	GDIPlusImage *pImage = (GDIPlusImage*)CALLOC(sizeof(GDIPlusImage));
	if (!pImage) {
		SETERROR(PGE_NOT_ENOUGH_MEMORY);
		return FALSE;
	}
	pImage->nMagic = eGdiStr_Image;
	pLoadPreview->pFileContext = (void*)pImage;

	
	pImage->gdi = (GDIPlusDecoder*)pLoadPreview->pContext;
	pImage->gdi->bCancelled = FALSE;
	

	if (!pImage->Open(
				(pLoadPreview->bVirtualItem!=FALSE), pLoadPreview->sFileName,
				pLoadPreview->pFileData, pLoadPreview->nFileSize, pLoadPreview->crLoadSize))
	{
		SETERROR(pImage->nErrNumber);
		pImage->Close();
		pLoadPreview->pFileContext = NULL;
		return FALSE;
	}
	
	if (pLoadPreview) {
		if (!pImage->GetPageBits(pLoadPreview)) {
			SETERROR(pImage->nErrNumber);
			pImage->Close();
			pLoadPreview->pFileContext = NULL;
			return FALSE;
		}
	}
	
	if (pLoadPreview->pFileContext == (void*)pImage)
		pLoadPreview->pFileContext = NULL;
	
	pImage->Close();
	return TRUE;
}

VOID WINAPI CET_Free(struct CET_LoadInfo* pLoadPreview)
{
	if (!pLoadPreview || *((LPDWORD)pLoadPreview) != sizeof(struct CET_LoadInfo)) {
		_ASSERTE(*((LPDWORD)pLoadPreview) == sizeof(struct CET_LoadInfo));
		SETERROR(PGE_INVALID_VERSION);
		return;
	}
	if (!pLoadPreview->pFileContext) {
		SETERROR(PGE_INVALID_CONTEXT);
		return;
	}

	switch (*(LPDWORD)pLoadPreview->pFileContext) {
		case eGdiStr_Image: {
			// —юда мы попадем если был exception в CET_Load
			GDIPlusImage *pImg = (GDIPlusImage*)pLoadPreview->pFileContext;
			pImg->Close();
		} break;
		case eGdiStr_Bits: {
			GDIPlusData *pData = (GDIPlusData*)pLoadPreview->pFileContext;
			if (pData->pImg) {
				pData->pImg->Close();
				pData->pImg = NULL;
			}
			pData->Close();
		} break;
		
		#ifdef _DEBUG
		default:
			_ASSERTE(*(LPDWORD)pLoadPreview->pFileContext == eGdiStr_Bits);
		#endif
	}
}

VOID WINAPI CET_Cancel(LPVOID pContext)
{
	if (!pContext) return;
	GDIPlusDecoder *pDecoder = (GDIPlusDecoder*)pContext;
	if (pDecoder->nMagic == eGdiStr_Decoder) {
		pDecoder->bCancelled = TRUE;
	}
}
