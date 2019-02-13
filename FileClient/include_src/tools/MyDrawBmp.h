// MyDrawBMP.h: interface for the CMyDrawBMP class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_MYDRAWBMP_H__CA4B5ACD_9518_11D5_A907_0080C8F7DF77__INCLUDED_)
#define AFX_MYDRAWBMP_H__CA4B5ACD_9518_11D5_A907_0080C8F7DF77__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#define MAXREAD  32768                 /* Number of bytes to be read during */
#define HUGE_T
#define WIDTHBYTES(i)   ((i+31)/32*4)

#define SIZEOF_BITMAPFILEHEADER_PACKED  (   \
    sizeof(WORD) +      /* bfType      */   \
    sizeof(DWORD) +     /* bfSize      */   \
    sizeof(WORD) +      /* bfReserved1 */   \
    sizeof(WORD) +      /* bfReserved2 */   \
    sizeof(DWORD))      /* bfOffBits   */

/* Header signatutes for various resources */
#define BFT_ICON   0x4349   /* 'IC' */
#define BFT_BITMAP 0x4d42   /* 'BM' */
#define BFT_CURSOR 0x5450   /* 'PT' */

class CMyDrawBMP  
{
public:
	void Tif_G3_mode(int status,int countp);
	void data_compress(unsigned int *str,int len);
	void Writetag(FILE *fpfax,int tag,int type,long length,long offset);
	int conv_bmp_tif(BITMAPFILEHEADER BmpHead,unsigned char *BmpBody ,DWORD sizeofBmpBody,char *TagetFile,int qulity,int leftborder,int rightborder);
	WORD DibNumColors (VOID FAR * pv);
	WORD PaletteSize (VOID FAR * pv);
	BOOL WriteDIB ( char *szFile,  HANDLE hdib);
	HANDLE DibFromBitmap (HBITMAP hbm,DWORD biStyle,WORD biBits,HPALETTE hpal);
	BOOL ClearBmpFile(HWND hwnd);
//	BOOL ShowBmpFile(HWND ,int sx,int sy,int cx,int cy);
	BOOL WriteBmpFile(char *BmpFileName );
	HDC GetSizeBmpHdc(HWND hwnd , int Width , int Height);
	HDC       hMemoryDC   ;
	HBITMAP   hbitmap     ;
	HBITMAP   hbitmapOrig ;
	HPEN      hpen        ;
	HPEN      hpenOrig    ;

	CMyDrawBMP();
	virtual ~CMyDrawBMP();
/*	
	HDC  GetSizeBmpHdc(HWND hwnd , int Width , int Height) ;
	BOOL WriteBmpFile(char *BmpFileName ) ;
	BOOL ShowBmpFile(HWND ,int sx,int sy,int cx,int cy) ;
	BOOL ClearBmpFile(HWND hwnd) ;
*/
};

#endif // !defined(AFX_MYDRAWBMP_H__CA4B5ACD_9518_11D5_A907_0080C8F7DF77__INCLUDED_)
