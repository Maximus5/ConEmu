typedef struct _tagTT_OFFSET_TABLE{
	USHORT	uMajorVersion;
	USHORT	uMinorVersion;
	USHORT	uNumOfTables;
	USHORT	uSearchRange;
	USHORT	uEntrySelector;
	USHORT	uRangeShift;
}TT_OFFSET_TABLE;

typedef struct _tagTT_TABLE_DIRECTORY{
	char	szTag[4];			//table name
	ULONG	uCheckSum;			//Check sum
	ULONG	uOffset;			//Offset from beginning of file
	ULONG	uLength;			//length of the table in bytes
}TT_TABLE_DIRECTORY;

typedef struct _tagTT_NAME_TABLE_HEADER{
	USHORT	uFSelector;			//format selector. Always 0
	USHORT	uNRCount;			//Name Records count
	USHORT	uStorageOffset;		//Offset for strings storage, from start of the table
}TT_NAME_TABLE_HEADER;

typedef struct _tagTT_NAME_RECORD{
	USHORT	uPlatformID;
	USHORT	uEncodingID;
	USHORT	uLanguageID;
	USHORT	uNameID;
	USHORT	uStringLength;
	USHORT	uStringOffset;	//from start of storage area
}TT_NAME_RECORD;

#define SWAPWORD(x)		MAKEWORD(HIBYTE(x), LOBYTE(x))
#define SWAPLONG(x)		MAKELONG(SWAPWORD(HIWORD(x)), SWAPWORD(LOWORD(x)))


CString GetFontNameFromFile(LPCTSTR lpszFilePath)
{
	CFile f;
	CString csRetVal;

	if(f.Open(lpszFilePath, CFile::modeRead|CFile::shareDenyWrite)){
		TT_OFFSET_TABLE ttOffsetTable;
		f.Read(&ttOffsetTable, sizeof(TT_OFFSET_TABLE));
		ttOffsetTable.uNumOfTables = SWAPWORD(ttOffsetTable.uNumOfTables);
		ttOffsetTable.uMajorVersion = SWAPWORD(ttOffsetTable.uMajorVersion);
		ttOffsetTable.uMinorVersion = SWAPWORD(ttOffsetTable.uMinorVersion);

		//check is this is a true type font and the version is 1.0
		if(ttOffsetTable.uMajorVersion != 1 || ttOffsetTable.uMinorVersion != 0)
			return csRetVal;
		
		TT_TABLE_DIRECTORY tblDir;
		BOOL bFound = FALSE;
		CString csTemp;
		
		for(int i=0; i< ttOffsetTable.uNumOfTables; i++){
			f.Read(&tblDir, sizeof(TT_TABLE_DIRECTORY));
			strncpy(csTemp.GetBuffer(5), tblDir.szTag, 4);
			csTemp.ReleaseBuffer(4);
			if(csTemp.CompareNoCase(_T("name")) == 0){
				bFound = TRUE;
				tblDir.uLength = SWAPLONG(tblDir.uLength);
				tblDir.uOffset = SWAPLONG(tblDir.uOffset);
				break;
			}
		}
		
		if(bFound){
			f.Seek(tblDir.uOffset, CFile::begin);
			TT_NAME_TABLE_HEADER ttNTHeader;
			f.Read(&ttNTHeader, sizeof(TT_NAME_TABLE_HEADER));
			ttNTHeader.uNRCount = SWAPWORD(ttNTHeader.uNRCount);
			ttNTHeader.uStorageOffset = SWAPWORD(ttNTHeader.uStorageOffset);
			TT_NAME_RECORD ttRecord;
			bFound = FALSE;
			
			for(int i=0; i<ttNTHeader.uNRCount; i++){
				f.Read(&ttRecord, sizeof(TT_NAME_RECORD));
				ttRecord.uNameID = SWAPWORD(ttRecord.uNameID);
				if(ttRecord.uNameID == 1){
					ttRecord.uStringLength = SWAPWORD(ttRecord.uStringLength);
					ttRecord.uStringOffset = SWAPWORD(ttRecord.uStringOffset);
					int nPos = f.GetPosition();
					f.Seek(tblDir.uOffset + ttRecord.uStringOffset + ttNTHeader.uStorageOffset, CFile::begin);
					f.Read(csTemp.GetBuffer(ttRecord.uStringLength + 1), ttRecord.uStringLength);
					csTemp.ReleaseBuffer();
					if(csTemp.GetLength() > 0){
						csRetVal = csTemp;
						break;
					}
					f.Seek(nPos, CFile::begin);
				}
			}			
		}
		f.Close();
	}
	return csRetVal;
}

