// MyListBox.cpp : implementation file
//

#include "stdafx.h"
#include "MyListBox.h"
#include ".\mylistbox.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CMyListBox

CMyListBox::CMyListBox()
{	
	pContextArray = new CStringArray();
	nContextArrayLastIndex = -1;	// No Data
	nMaxContextArraySize = 10;		// Default Value
	bRealTimeFlush = FALSE;
	nTimerID = (UINT_PTR)this;
}

CMyListBox::~CMyListBox()
{
	delete pContextArray;
}


BEGIN_MESSAGE_MAP(CMyListBox, CListBox)
	//{{AFX_MSG_MAP(CMyListBox)
	ON_WM_TIMER()
	//}}AFX_MSG_MAP
	ON_WM_PAINT()
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CMyListBox message handlers

BOOL CMyListBox::InitializeMyListBox(int nRow, int nFlushTimer)
{
	int			nArraySize,nListBoxSize,i;

	if ( (nArraySize=(int)pContextArray->GetSize()) > 0 )
		return FALSE;

	if ( nRow <= 0 )
		nRow = nMaxContextArraySize;
	else
		nRow = min(MAXDATASIZE,nRow);

	// ----------------------------------------------------------------------------------
	// strings in pContextArray.
	//nArraySize = pContextArray->GetSize();	// pContextArray Array Size.	

	if ( nArraySize > nRow )
	{
		// 減少strings in pContextArray.
		if ( nMaxContextArraySize - nContextArrayLastIndex + 1 > nArraySize - nRow )
			pContextArray->RemoveAt(nContextArrayLastIndex + 1,nArraySize - nRow);
		else
		{
			pContextArray->RemoveAt(nContextArrayLastIndex + 1,nMaxContextArraySize - nContextArrayLastIndex - 1);
			pContextArray->RemoveAt(0,(nArraySize-nRow)-(nMaxContextArraySize-nContextArrayLastIndex)+1);
			nContextArrayLastIndex -= ((nArraySize-nRow)-(nMaxContextArraySize-nContextArrayLastIndex)+1);
		}		
	}
	else
	{
		// 增加strings in pContextArray.
		for ( i=nArraySize ; i<nRow ; i++ )
			pContextArray->Add("");
	}
	// ----------------------------------------------------------------------------------

	// ----------------------------------------------------------------------------------
	// strings in list box.
	nListBoxSize = GetCount();				// the number of strings in a list box.
	if ( nListBoxSize >= nRow )
	{
		// 減少the number of strings in list box.
		for ( i=nListBoxSize ; i>nRow ; i-- )
			DeleteString(i-1);
		
	}
	else
	{
		// 增加the number of strings in list box.
		for ( i=nListBoxSize ; i<nRow ; i++ )
		{
			CString str;
			str.Format("%d",i);
			AddString(str);
		}
	}
	// ----------------------------------------------------------------------------------

	if ( nFlushTimer < 0 )	// RealTime
		bRealTimeFlush = TRUE;
	else
		SetTimer(nTimerID,nFlushTimer,NULL);

	nMaxContextArraySize = nRow;
	
	return TRUE;
}

void CMyListBox::DrawItem(LPDRAWITEMSTRUCT lpDrawItemStruct) 
{
	// TODO: Add your code to draw the specified item
	ASSERT(lpDrawItemStruct->CtlType == ODT_LISTBOX);
	//LPCTSTR lpszText = (LPCTSTR) lpDrawItemStruct->itemData;
	LPCTSTR lpszText = GetArrayText(lpDrawItemStruct->itemID);
	if ( lpszText == NULL )
		return;
//	ASSERT(lpszText != NULL);
	CDC dc;
	
	dc.Attach(lpDrawItemStruct->hDC);
	
	// Save these value to restore them when done drawing.
	COLORREF crOldTextColor = dc.GetTextColor();
	COLORREF crOldBkColor = dc.GetBkColor();
	
	// If this item is selected, set the background color 
	// and the text color to appropriate values. Also, erase
	// rect by filling it with the background color.
	if ((lpDrawItemStruct->itemAction | ODA_SELECT) &&
		(lpDrawItemStruct->itemState & ODS_SELECTED))
	{
		dc.SetTextColor(::GetSysColor(COLOR_HIGHLIGHTTEXT));
		dc.SetBkColor(::GetSysColor(COLOR_HIGHLIGHT));
		dc.FillSolidRect(&lpDrawItemStruct->rcItem,
			::GetSysColor(COLOR_HIGHLIGHT));
	}
	else
		dc.FillSolidRect(&lpDrawItemStruct->rcItem, crOldBkColor);
	
	// If this item has the focus, draw a red frame around the
	// item's rect.
	if ((lpDrawItemStruct->itemAction | ODA_FOCUS) &&
		(lpDrawItemStruct->itemState & ODS_FOCUS))
	{
		//CBrush br(RGB(255, 0, 0));
		CBrush br(RGB(192, 192, 192));
		dc.FrameRect(&lpDrawItemStruct->rcItem, &br);
	}
	
	// Draw the text.
	dc.DrawText(
		lpszText,
		(int)strlen(lpszText),
		&lpDrawItemStruct->rcItem,
		DT_LEFT|DT_SINGLELINE|DT_VCENTER);
	
	// Reset the background color and the text color back to their
	// original values.
	dc.SetTextColor(crOldTextColor);
	dc.SetBkColor(crOldBkColor);
	
	dc.Detach();
}

BOOL CMyListBox::InsertData(LPCTSTR chData)
{
	int		nArraySize;

	nArraySize = (int)pContextArray->GetSize();
	if ( nArraySize < nMaxContextArraySize || nArraySize == 0 )
		return FALSE;

	IncreaseLastIndex();
	pContextArray->SetAt(nContextArrayLastIndex,chData);
	if ( bRealTimeFlush )
		Invalidate();
	return TRUE;
}

void CMyListBox::IncreaseLastIndex()
{
	InterlockedIncrement(&nContextArrayLastIndex);

	nContextArrayLastIndex = nContextArrayLastIndex % nMaxContextArraySize;
}

LPCTSTR CMyListBox::GetArrayText(int nIndex)
{
	int			nContextArrayIndex;

	if ( nContextArrayLastIndex < 0 )
		return NULL;

	if ( nContextArrayLastIndex - nIndex < 0 )
		nContextArrayIndex = nMaxContextArraySize + (nContextArrayLastIndex - nIndex);
	else
		nContextArrayIndex = nContextArrayLastIndex - nIndex;

	return (pContextArray->GetAt(nContextArrayIndex));
}

BOOL CMyListBox::ResetContext()
{
	for ( int i=0 ; i<nMaxContextArraySize ; i++ )
		pContextArray->SetAt(i,"");
	Invalidate(TRUE);

	return TRUE;
}

void CMyListBox::OnTimer(UINT nIDEvent) 
{
	// TODO: Add your message handler code here and/or call default
	if ( nIDEvent == nTimerID )
		Invalidate(TRUE);
	CListBox::OnTimer(nIDEvent);
}