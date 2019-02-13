#if !defined(AFX_MYLISTBOX_H__F158C05D_7447_4B18_BF13_C1C14F5F0011__INCLUDED_)
#define AFX_MYLISTBOX_H__F158C05D_7447_4B18_BF13_C1C14F5F0011__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// MyListBox.h : header file
//

#define				MAXDATASIZE			256				// 最大行數
/////////////////////////////////////////////////////////////////////////////
// CMyListBox window

class CMyListBox : public CListBox
{
// Construction
public:
	CMyListBox();

// Attributes
public:

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CMyListBox)
	public:
	virtual void DrawItem(LPDRAWITEMSTRUCT lpDrawItemStruct);
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CMyListBox();

	// 新增一筆資料
	BOOL		InsertData(LPCTSTR chData);
	// 設定顯示行數(初始化, 不可動態改之[多緒會出問題])
	BOOL		InitializeMyListBox(int nRow,int nFlushTimer = -1);
	// 清除List Box中資料
	BOOL		ResetContext(void);

	// Generated message map functions
protected:
	LPCTSTR			GetArrayText(int nIndex);		// 從Buffer(pContextArray)中取得資料
	void			IncreaseLastIndex(void);		// 管理最後一筆資料的Index

	long			nContextArrayLastIndex;			// 最後一筆的Index
	int				nMaxContextArraySize;			// 顯示最大列數
	CStringArray*	pContextArray;					// 每列資料Buffer
	BOOL			bRealTimeFlush;					// 即時更新UI旗標
	UINT_PTR		nTimerID;
	//{{AFX_MSG(CMyListBox)
	afx_msg void OnTimer(UINT nIDEvent);
	//}}AFX_MSG

	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_MYLISTBOX_H__F158C05D_7447_4B18_BF13_C1C14F5F0011__INCLUDED_)
