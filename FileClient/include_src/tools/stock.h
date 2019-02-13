#define STOCKDLL		"stock.dll"

const int		STOCKDLL_LOAD_ERROR		= -2;	// 載入stock.dll有誤
const int		STOCKDLL_SYSTEM_ERROR	= -1;	// 系統有誤
const int		STOCKDLL_DATA_NOT_FOUND	= 0;	// 查無資料
const int		STOCKDLL_CORRECT_DATA	= 1;	// 資料正確
const int		STOCKDLL_ISSUE_DATA		= 2;	// 資料有誤(該市場別資料不及時)

#pragma pack(1)
//股票商品欄位結構宣告
typedef struct tagStockItem {
	char	MarketType[3];	// 市場類別
	char	Type[3];		// 類別代號
	char    IdCode[11];		// Stock ID
	char    Name[11];		// Stock Name
	int     Year;			// 時間:年(yyyy)
	char    Month;			// 時間:月
	char    Day;			// 時間:日
	char    Hour;			// 時間:時
	char    Minute;			// 時間:分
	char    UpDnFlag;		// 漲跌停旗標  1:跌停  2:漲停   0:正常
	double  Sell;			// Price of Sell(大盤:成交筆數)
	double  Buy;			// Price of Buy(大盤:成交總金額)
	double  Deal;			// Price of 成交價
	double  Hi;				// Price of Hi
	double  Low;			// Price of Low
	double  Volum;			// Price of 成交量
	double  YClose;			// Price of 收盤
	double  UpPrice;		// Price of 漲停
	double  DnPrice;		// Price of 跌停
	double  UpDnPrice;		// Price of 漲跌幅
	double  Open;			// Price of 開盤
	double  CBuy;			// 委買筆數(大盤:總委買筆數)
	double  CSell;			// 委賣筆數(大盤:總委賣筆數)
	double  CBVolum;		// 委買張數(各股,大盤:總委買張數)
	double  CSVolum;		// 委賣張數(各股,大盤:總委賣張數)
	char    Class;			// 上市、櫃商品	-- 00：正常；01：警示；02：處置1；03：警示及處置1；04：處置2；05：警示及處置2
							// 期權商品		-- 00：正常狀況；01~60：異常狀況時間(分鐘)；98：異常狀況解除；99：異常狀況超過60分鐘
	long    StartDay;		// 單量(原設計用途：上市日期 19980615)
	double  Capital;		// 成交值(原設計用途：股本)
	double	Buy5[6];		// 最佳五檔買進價
	double	BVolum5[6];		// 最佳五檔買進量
	double	Sell5[6];		// 最佳五檔賣出價
	double	SVolum5[6];		// 最佳五檔賣出量
	long	NOffSet;		// 未平倉合約數
	double	Reckon;			// 結算價
	int		nFlag;			// Bit 1：表示是否為台灣成分股。
	double  CUpPrice;		// 盤後Price of 漲停
	double  CDnPrice;		// 盤後Price of 跌停
} StkItem;

// Useless
// 指數類結構宣告(目前無使用)
typedef struct tagPower {
	double  Value;			// 指數值
	double  YClose;			// 昨收
	double  UpDn;			// 漲跌幅
	double  DealMoney;		// 成交金額(以下三項目前只有大盤有)
	double  DealVolum;		// 成交量
	double  DealCount;		// 成交筆數
} Pow;

typedef struct tagStock20Power {
    Pow		Pow20[20];		// 0:大盤   1->19 各分類指數
} Stk20Pow;

//新聞(目前無使用)
typedef struct tagStockNews {
	int		LastNo;
	int		MaxNo;
    char	News[81];
} StkNews;
// Useless

//類別結構
typedef struct tagStockTypeIndex {
	char	MarketType[3];	// 市場類別
	char	Type[3];		// 類別代號
    WORD	Idx;			// 該類別起始位置(即第 ? 筆開始)
    WORD	Count;			// 該類別所含商品數目
    char	Name[30];		// 該類別中文名稱
} TypeIndex;

typedef struct tagMapFileHead {
	WORD		wVersion;			// 版本別
	DWORD		dwFileSize;			// Stock.all的檔案大小
	DWORD		dwWriteOffset;		// 目前寫入Stock.all檔案的位址
	UINT		uCnts;				// Stock.all檔案中股票商品的總數
	UINT		usage;				// 開啟Stock.all檔案的程式數目
	DWORD		dwTickCnt;			// 最後更動Stock.all資料的時間
	WORD		wEnableSrc;			// 沒問題的資料
	WORD		wActiveSrc;			// 盤中的資料
	WORD		wHolidaySrc;		// 放假的資料
} MapFileHead;
#pragma pack()

//------------------ 讀寫商品價位 ------------------//
typedef	int	 (__cdecl *DLL_READSTKREC)		(char *ID, StkItem*);
typedef	BOOL (__cdecl *DLL_WRITESTKREC)		(char *ID, StkItem);
typedef	int	 (__cdecl *DLL_READSTKREC_J)	(WORD Idx, StkItem*);
typedef	BOOL (__cdecl *DLL_WRITESTKREC_J)	(WORD Idx, StkItem);
//--------------------------------------------------//

//---------------- 新增刪除商品類別 ----------------//
typedef	BOOL (__cdecl *DLL_ADDSTKTYPE)		(char Type, char *TypeName);	// 未實現
typedef	BOOL (__cdecl *DLL_DELSTKTYPE)		(char Type);					// 未實現
typedef	int	 (__cdecl *DLL_READSTKTYPE)		(char *MarketType, char *Type, TypeIndex*);
typedef	int	 (__cdecl *DLL_READSTKTYPE_J)	(WORD TypeIdx, TypeIndex*);
//--------------------------------------------------//

//---------------- 新增刪除商品項目 ----------------//
typedef	BOOL (__cdecl *DLL_ADDSTK)			(char *ID, char *IDName, char* MarketType, char* Type);
typedef	BOOL (__cdecl *DLL_DELSTK)			(char *ID);
typedef	BOOL (__cdecl *DLL_FLUSHSTK)		(char *ID, char *MarketType);
//--------------------------------------------------//

//------------------ 商品項目索引 ------------------//
typedef	int	 (__cdecl *DLL_GETSTKCOUNT)		(void);
typedef	int	 (__cdecl *DLL_INDEXOFSTKID)	(char *Idx);
//--------------------------------------------------//

//--------------- 啟動或關畢商品通知 ---------------//
typedef	BOOL (__cdecl *DLL_SETSTKNOTIFY)	(HWND Hwnd, char *ID, UINT WM_Message, WPARAM Wparam, LPARAM Lparam);	// 未實現
typedef	BOOL (__cdecl *DLL_DELSTKNOTIFY)	(HWND Hwnd, char *ID);	// 未實現
typedef	BOOL (__cdecl *DLL_SETSTKNOTIFY_J)	(HWND Hwnd, WORD Idx, UINT WM_Message, WPARAM Wparam, LPARAM Lparam);	// 未實現
typedef	BOOL (__cdecl *DLL_DELSTKNOTIFY_J)	(HWND Hwnd, WORD Idx);	// 未實現
typedef	BOOL (__cdecl *DLL_DELSTKALLNOTIFY)	(void);					// 未實現
//--------------------------------------------------//

//---------------- 20類指數商品項目 ----------------//
typedef	BOOL (__cdecl *DLL_READ20POW)		(Stk20Pow*);	// 未實現
typedef	BOOL (__cdecl *DLL_WRITE20POW)		(Stk20Pow);		// 未實現
//--------------------------------------------------//

//---------------- 20類指數商品項目 ----------------//
typedef	int	 (__cdecl *DLL_READSTKNEWS)		(int, StkNews*);// 未實現
typedef	int	 (__cdecl *DLL_WRITESTKNEWS)	(int, char*);	// 未實現
typedef	BOOL (__cdecl *DLL_CLEARSTKNEWS)	(void);			// 未實現
//--------------------------------------------------//

//---------------- 最後一筆錯誤訊息 ----------------//
typedef	BOOL (__cdecl *DLL_GETSTKLASTERR)	(char *LastErrorMessage);	// 未實現
//--------------------------------------------------//

//---------------- 管理stock.dll工具函式 ----------------//
typedef	void (__cdecl *DLL_SETMARKETSTATUS)	(char *chMarket, BOOL bEnable);
typedef	int	 (__cdecl *DLL_GETMARKETSTATUS)	(char *chMarket);
typedef	void (__cdecl *DLL_SETMARKETACTIVE)	(char *chMarket, BOOL bEnable);
typedef	int	 (__cdecl *DLL_GETMARKETACTIVE)	(char *chMarket);
typedef	void (__cdecl *DLL_SETMARKETHOLIDAY)(char *chMarket, BOOL bEnable);
typedef	int	 (__cdecl *DLL_GETMARKETHOLIDAY)(char *chMarket);
typedef	BOOL (__cdecl *DLL_GETSTATUS)		( MapFileHead* pHead);
//--------------------------------------------------//

//------------------ 讀寫商品價位 ------------------//
extern	DLL_READSTKREC		AP_ReadStkRec;
extern	DLL_WRITESTKREC		AP_WriteStkRec;
extern	DLL_READSTKREC_J	AP_ReadStkRec_J;
extern	DLL_WRITESTKREC_J	AP_WriteStkRec_J;
//--------------------------------------------------//

//---------------- 新增刪除商品類別 ----------------//
extern	DLL_ADDSTKTYPE		AP_AddStkType;
extern	DLL_DELSTKTYPE		AP_DelStkType;
extern	DLL_READSTKTYPE		AP_ReadStkType;
extern	DLL_READSTKTYPE_J	AP_ReadStkType_J;
//--------------------------------------------------//

//---------------- 新增刪除商品項目 ----------------//
extern	DLL_ADDSTK			AP_AddStk;
extern	DLL_DELSTK			AP_DelStk;
extern	DLL_FLUSHSTK		AP_FlushStk;
//--------------------------------------------------//

//------------------ 商品項目索引 ------------------//
extern	DLL_GETSTKCOUNT		AP_GetStkCount;
extern	DLL_INDEXOFSTKID	AP_IndexOfStkID;
//--------------------------------------------------//

//--------------- 啟動或關畢商品通知 ---------------//
extern	DLL_SETSTKNOTIFY	AP_SetStkNotify;
extern	DLL_DELSTKNOTIFY	AP_DelStkNotify;
extern	DLL_SETSTKNOTIFY_J	AP_SetStkNotify_J;
extern	DLL_DELSTKNOTIFY_J	AP_DelStkNotify_J;
extern	DLL_DELSTKALLNOTIFY	AP_DelStkAllNotify;
//--------------------------------------------------//

//---------------- 20類指數商品項目 ----------------//
extern	DLL_READ20POW		AP_Read20Pow;
extern	DLL_WRITE20POW		AP_Write20Pow;
//--------------------------------------------------//

//---------------- 20類指數商品項目 ----------------//
extern	DLL_READSTKNEWS		AP_ReadStkNews;
extern	DLL_WRITESTKNEWS	AP_WriteStkNews;
extern	DLL_CLEARSTKNEWS	AP_ClearStkNews;
//--------------------------------------------------//

//---------------- 最後一筆錯誤訊息 ----------------//
extern	DLL_GETSTKLASTERR	AP_GetStkLastErr;
//--------------------------------------------------//

//---------------- 管理stock.dll工具函式 -----------//
extern	DLL_SETMARKETSTATUS	AP_SetMarketStatus;
extern	DLL_GETMARKETSTATUS	AP_GetMarketStatus;
extern	DLL_SETMARKETACTIVE	AP_SetMarketActive;
extern	DLL_GETMARKETACTIVE	AP_GetMarketActive;
extern	DLL_SETMARKETHOLIDAY	AP_SetMarketHoliday;
extern	DLL_GETMARKETHOLIDAY	AP_GetMarketHoliday;
extern	DLL_GETSTATUS		AP_GetStatus;
//--------------------------------------------------//

//--------------------------------------------------//
// Define Function
BOOL	LoadStockDll	(void);
void	UnLoadStockDll	(void);

//------------------ 讀寫商品價位 ------------------//
int		ReadStkRec		(char *ID, StkItem*);
BOOL	WriteStkRec		(char *ID, StkItem);
int		ReadStkRec_J	(WORD Idx, StkItem*);
BOOL	WriteStkRec_J	(WORD Idx, StkItem);
//--------------------------------------------------//

//---------------- 新增刪除商品類別 ----------------//
BOOL	AddStkType		(char Type, char *TypeName);	// 未實現
BOOL	DelStkType		(char Type);					// 未實現
int		ReadStkType		(char *MarketType, char *Type, TypeIndex*);
int		ReadStkType_J	(WORD TypeIdx, TypeIndex*);
//--------------------------------------------------//

//---------------- 新增刪除商品項目 ----------------//
BOOL	AddStk			(char *ID, char *IDName, char* MarketType, char* Type);
BOOL	DelStk			(char *ID);
BOOL	FlushStk		(char *ID, char* MarketType);
//--------------------------------------------------//

//------------------ 商品項目索引 ------------------//
int		GetStkCount		(void);
int		IndexOfStkID	(char *Idx);
//--------------------------------------------------//

//--------------- 啟動或關畢商品通知 ---------------//
BOOL	SetStkNotify	(HWND Hwnd, char *ID, UINT WM_Message, WPARAM Wparam, LPARAM Lparam);	// 未實現
BOOL	DelStkNotify	(HWND Hwnd, char *ID);		// 未實現
BOOL	SetStkNotify_J	(HWND Hwnd, WORD Idx, UINT WM_Message, WPARAM Wparam, LPARAM Lparam);	// 未實現
BOOL	DelStkNotify_J	(HWND Hwnd, WORD Idx );	// 未實現
BOOL	DelStkAllNotify	(void);					// 未實現
//--------------------------------------------------//

//---------------- 20類指數商品項目 ----------------//
BOOL	Read20Pow		(Stk20Pow*);	// 未實現
BOOL	Write20Pow		(Stk20Pow);	// 未實現
//--------------------------------------------------//

//---------------- 20類指數商品項目 ----------------//
int		ReadStkNews		(int, StkNews*);	// 未實現
int		WriteStkNews	(int, char*);	// 未實現
BOOL	ClearStkNews	(void);			// 未實現
//--------------------------------------------------//

//---------------- 最後一筆錯誤訊息 ----------------//
BOOL	GetStkLastErr	(char *LastErrorMessage);	// 未實現
//--------------------------------------------------//

//---------------- 管理stock.dll工具函式 -----------//
void	SetMarketStatus	(char* chMarket, BOOL bEnable);
int		GetMarketStatus	(char* chMarket);
void	SetMarketActive	(char* chMarket, BOOL bEnable);
int		GetMarketActive	(char* chMarket);
void	SetMarketHoliday(char* chMarket, BOOL bEnable);
int		GetMarketHoliday(char* chMarket);
BOOL	GetStatus		(MapFileHead* pHead);
//--------------------------------------------------//

// End Define Function
//--------------------------------------------------//