// 定義一些常用的工具函式
#include "stdafx.h"

BOOL DoParseaData(char *chData, int nDataLen, char cDivideSymbol, CMapStringToString *pMap)
{
	int		i,j,k,l;
	char	Key[128],Value[4069];
	BOOL	bStartFlag;

	if ( !pMap )
		return FALSE;

	bStartFlag = TRUE;
	for ( i=0,j=0,k=0 ; i<nDataLen ; i++ )
	{
		if ( bStartFlag )
		{
			Key[j] = chData[i];
			if ( chData[i] == '=' )
			{
				Key[j] = 0;
				bStartFlag = FALSE;
				k = 0;
			}
			j++;
		}
		else
		{
			Value[k] = chData[i];
			if ( chData[i] == cDivideSymbol )
			{
				Value[k] = 0;
				// 去除空白
				for ( l=k-1 ; l>=0 ; l-- )
				{
					if ( Value[l] == 0x20 )
						Value[l] = 0;
					else
						break;
				}
				bStartFlag = TRUE;
				if ( l != -1 )
					pMap->SetAt(Key,Value);
				j = 0;
			}
			k++;
		}
	}
	
	if ( !bStartFlag )
	{
		Value[k] = 0;
		pMap->SetAt(Key,Value);
	}
	return TRUE;
}

// 檢查目錄是否存在
BOOL IsDirectoryExists(CString strDirectory)
{
	DWORD		nCode;

	nCode = GetFileAttributes(strDirectory);
	return (nCode != -1 && (FILE_ATTRIBUTE_DIRECTORY & nCode) != 0 );
}

// 強制產生目錄
BOOL ForceDirectories(CString strDirectory)
{
	CString	strBuf;
	int		nLen;

	if ( (nLen=strDirectory.GetLength()) == 0 )
		return FALSE;

	if ( IsDirectoryExists(strDirectory) )
		return TRUE;
	else
	{
		strBuf = strDirectory.Left(strDirectory.ReverseFind('\\'));
		return (ForceDirectories(strBuf) && CreateDirectory(strDirectory,NULL));
	}
}

// 更改檔名
BOOL RenameFile(CString strFrom, CString strTo)
{
	SHFILEOPSTRUCT		FileOp;
	char				buf[1024],buf1[1024];

	memset(&FileOp,NULL,sizeof(SHFILEOPSTRUCT));
	FileOp.hwnd = NULL;
	FileOp.wFunc = FO_RENAME;
	sprintf(buf,"%s%c",strFrom,0);
	FileOp.pFrom = buf;
	sprintf(buf1,"%s%c",strTo,0);
	FileOp.pTo = buf1;
	FileOp.fFlags = FOF_NOCONFIRMATION | FOF_NOERRORUI | FOF_SILENT;
	return ( SHFileOperation(&FileOp) == 0 );
}

// 複製檔案
BOOL MyCopyFile(CString strFrom, CString strTo)
{
	SHFILEOPSTRUCT		FileOp;
	char				buf[1024],buf1[1024];

	memset(&FileOp,NULL,sizeof(SHFILEOPSTRUCT));
	FileOp.hwnd = NULL;
	FileOp.wFunc = FO_COPY;
	sprintf(buf,"%s%c",strFrom,0);
	FileOp.pFrom = buf;
	sprintf(buf1,"%s%c",strTo,0);
	FileOp.pTo = buf1;
	FileOp.fFlags = FOF_NOCONFIRMATION | FOF_NOERRORUI | FOF_SILENT;
	return ( SHFileOperation(&FileOp) == 0 );
}

// 移動檔案
BOOL MyMoveFile(CString strFrom, CString strTo)
{
	SHFILEOPSTRUCT		FileOp;
	char				buf[1024],buf1[1024];

	memset(&FileOp,NULL,sizeof(SHFILEOPSTRUCT));
	FileOp.hwnd = NULL;
	FileOp.wFunc = FO_MOVE;
	sprintf(buf,"%s%c",strFrom,0);
	FileOp.pFrom = buf;
	sprintf(buf1,"%s%c",strTo,0);
	FileOp.pTo = buf1;
	FileOp.fFlags = FOF_NOCONFIRMATION | FOF_NOERRORUI | FOF_SILENT;
	return ( SHFileOperation(&FileOp) == 0 );
}

// 刪除檔案
BOOL MyDeleteFile(CString strFilePath)
{
	SHFILEOPSTRUCT		FileOp;
	char				buf[1024];

	memset(&FileOp,NULL,sizeof(SHFILEOPSTRUCT));
	FileOp.hwnd = NULL;
	FileOp.wFunc = FO_DELETE;
	sprintf(buf,"%s%c",strFilePath,0);
	FileOp.pFrom = buf;
	FileOp.fFlags = FOF_NOCONFIRMATION | FOF_NOERRORUI | FOF_SILENT;
	return ( SHFileOperation(&FileOp) == 0 );
}

// 檢查檔案是否存在
BOOL IsFileExist(CString strFileName)
{
	WIN32_FIND_DATA		FD;
	HANDLE				hFile;
	BOOL				bRetCode;

	if ( (hFile=FindFirstFile((LPCTSTR)strFileName,&FD)) == INVALID_HANDLE_VALUE )
		bRetCode = FALSE;
	else
	{
		bRetCode = TRUE;
		FindClose(hFile);
	}

	return bRetCode;
}

// 刪除空白及換行符號
void Cut_Space(char* chData)
{
	char* pt = chData + strlen(chData) - 1;

	while ( pt >= chData )
	{
		if ( *pt != 0x20 && *pt != 0x0d && *pt != 0x0a )
		{
			*(pt+1) = 0;
			break;
		}
		pt --;
	}

	if ( pt + 1 == chData )
		*(pt+1) = 0;
}

// 去除小數點以下為0的數字
void Cut_Zero(char *chData)
{
	char	*pDot,*pt;

	if ( (pDot=strchr(chData,'.')) != NULL )
	{
		pt = chData + strlen(chData) - 1;
		while ( pt >= pDot )
		{
			if ( *pt != '0' )
			{
				if ( *pt == '.' )
					*pt = 0;
				else
					*(pt+1) = 0;
				break;
			}
			pt --;
		}
	}
}

// 將簡體中文轉成UTF8
int StringToUTF8(char* chData, int nSize)
{
	WCHAR	wszData[128];

	// char轉成wide-char(Unicode)
	if ( MultiByteToWideChar( 936 , 0, chData, -1, wszData, sizeof(wszData)/sizeof(wszData[0])) == 0 )
		return 0;

	// wide-char(Unicode)轉成UTF8
	return (WideCharToMultiByte( CP_UTF8 , 0, wszData, -1, chData, nSize, NULL, NULL));
}