// 定義一些常用的工具函式
BOOL	DoParseaData(char *chData, int nDataLen, char cDivideSymbol, CMapStringToString *pMap);
BOOL	IsDirectoryExists(CString strDirectory);	// 檢查目錄是否存在
BOOL	ForceDirectories(CString strDirectory);		// 強制產生目錄
BOOL	RenameFile(CString strFrom, CString strTo);	// 更改檔名
BOOL	MyCopyFile(CString strFrom, CString strTo);	// 複製檔案
BOOL	MyMoveFile(CString strFrom, CString strTo);	// 移動檔案
BOOL	MyDeleteFile(CString strFilePath);			// 刪除檔案
BOOL	IsFileExist(CString strFileName);			// 檢查檔案是否存在
void	Cut_Space(char* chData);					// 刪除空白及換行符號
void	Cut_Zero(char *chData);						// 去除小數點以下為0的數字
int		StringToUTF8(char* chData, int nSize);		// 將簡體中文轉成UTF8
