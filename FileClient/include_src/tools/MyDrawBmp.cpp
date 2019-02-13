// MyDrawBMP.cpp: implementation of the CMyDrawBMP class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
//#include "Gateway.h"
#include "MyDrawBMP.h"
#include "tiff.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

#define WM_DATAEND WM_USER+100
#define  MAXLINES 97
#define  HI  196
#define  LO  98
#define  OFF 0
#define  ON  1
#define tagcount 15

struct BMPHEAD {
	BYTE	ID[2];
	DWORD	filesize;
	WORD	reserved[2];
	DWORD	headersize;
	DWORD	infosize;
	DWORD	width;
	DWORD	height;
	WORD	planes;
	WORD	colorbits;
	DWORD	compression;
	DWORD	imagesize;
	DWORD	xpelspermeter;
	DWORD	ypelspermeter;
	DWORD	usecolor;
	DWORD	importantcolor;
	DWORD	black;
	DWORD	white;
};

struct TIFF_TAG {
	WORD	tag;
	WORD	type;
	long	length;
	long	offset;
};

unsigned int wbuf1[1729] ;
long Width,Height;
long  Wdatacount ;
int  bitcount;
long StripBytecount ;
unsigned int  int_flag[16]=
{
 1,      //0000000000000001
 2,      //0000000000000010
 4,      //0000000000000100
 8,      //0000000000001000
 16,     //0000000000010000
 32,     //0000000000100000
 64,     //0000000001000000
 128,    //0000000010000000
 256,    //0000000100000000
 512,    //0000001000000000
 1024,   //0000010000000000
 2048,   //0000100000000000
 4096,   //0001000000000000
 8192,   //0010000000000000
 16384,  //0100000000000000
 32768   //1000000000000000
} ;

unsigned char g3_str[1728*101]; // ???????

BMPHEAD sobmphead ;
long  bit_width ;
long  byte_len ; //
unsigned char masktable[8] ={(unsigned char)0x80,(unsigned char)0x40,(unsigned char)0x20,(unsigned char)0x10
                            ,(unsigned char)0x08,(unsigned char)0x04,(unsigned char)0x02,(unsigned char)0x01};
long boffset ;
long bmp_linecount;
// FILE *fp_fax ; // sour file handle
FILE *fw_fax ; // dest file handle

char tmp ;

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CMyDrawBMP::CMyDrawBMP()
{
	hMemoryDC   = NULL ;
	hbitmap     = NULL ;
	hbitmapOrig = NULL ;
	hpen        = NULL ;
	hpenOrig    = NULL ;
}

CMyDrawBMP::~CMyDrawBMP()
{
	DeleteDC(hMemoryDC);
    DeleteObject(hpen);
	DeleteObject(hpenOrig);
    DeleteObject(hbitmap) ;
	DeleteObject(hbitmapOrig) ;
}

HDC CMyDrawBMP::GetSizeBmpHdc(HWND hwnd, int Width, int Height)
{
	HDC hDC ;
	BITMAP bm ;

    if( hbitmapOrig != NULL && hMemoryDC != NULL )
	    SelectObject(hMemoryDC,hbitmapOrig);
    if( hpenOrig != NULL && hMemoryDC != NULL )
	    SelectObject(hMemoryDC,hpenOrig);
    if( hpen != NULL) 
	    DeleteObject(hpen);
    if( hMemoryDC != NULL) 
	    DeleteDC(hMemoryDC);
    if( hbitmap != NULL) 
	    DeleteObject(hbitmap) ;
    hMemoryDC   = NULL ;
    hbitmap     = NULL ;
    hbitmapOrig = NULL ;
    hpen        = NULL ;
    hpenOrig    = NULL ;

	hDC = GetDC(hwnd) ;
    hMemoryDC = CreateCompatibleDC(hDC);
	ReleaseDC(hwnd,hDC) ;
//	hbitmap   = CreateCompatibleBitmap(hDC,Width,Height) ; //for color
	hbitmap   = CreateBitmap(Width,Height,1,1,NULL) ; //for monochrome

	hbitmapOrig = (HBITMAP)SelectObject(hMemoryDC , hbitmap) ;
    GetObject(hbitmap,sizeof(bm),(LPSTR)&bm);
	PatBlt(hMemoryDC , 0 , 0 , bm.bmWidth , bm.bmHeight , WHITENESS);
	hpen = CreatePen(PS_SOLID,0,RGB(0,0,0)) ; //Black
    hpenOrig = (HPEN)SelectObject(hMemoryDC,hpen) ;
    return hMemoryDC ;
}

BOOL CMyDrawBMP::WriteBmpFile(char *BmpFileName)
{
	HANDLE hdib ;
	BYTE   rc ;

    if( hbitmap == NULL) return FALSE ;

    hdib = DibFromBitmap (hbitmap,(DWORD)NULL,(WORD)NULL,(HPALETTE)NULL);
    rc = WriteDIB ( BmpFileName,  hdib ) ; 
    //CloseHandle(hdib) ;
    GlobalFree(hdib);

/*
	SelectObject(hMemoryDC,hbitmapOrig);
	SelectObject(hMemoryDC,hpenOrig);
	DeleteObject(hpen);
	DeleteDC(hMemoryDC);
	DeleteObject(hbitmap) ;

    hMemoryDC   = NULL ;
    hbitmap     = NULL ;
    hbitmapOrig = NULL ;
    hpen        = NULL ;
    hpenOrig    = NULL ;
*/
    return rc ;
}
/*
BOOL CMyDrawBMP::ShowBmpFile(HWND, int sx, int sy, int cx, int cy)
{
	HDC hDC ;

	//if(access(FBmp,0) != 0) return FALSE ;
	if(hMemoryDC == NULL) return FALSE ;
	hDC = GetDC(hwnd) ; //Display to Current Window's screen
	BitBlt(hDC , 0 , 0 , cx , cy , hMemoryDC, sx , sy ,SRCCOPY);
	ReleaseDC(hwnd,hDC) ;
	return TRUE ;
}


BOOL CMyDrawBMP::ClearBmpFile(HWND hwnd)
{
	HDC hDC ;
	BITMAP bm ;
	
	if(hMemoryDC == NULL) return FALSE ;
	GetObject(hbitmap,sizeof(bm),(LPSTR)&bm);
	PatBlt(hMemoryDC , 0 , 0 , bm.bmWidth , bm.bmHeight , WHITENESS);
	hDC = GetDC(hwnd) ; //Display to Current Window's screen
	BitBlt(hDC , 0 , 0 , bm.bmWidth , bm.bmHeight , hMemoryDC, 0 , 0 ,SRCCOPY);
	ReleaseDC(hwnd,hDC) ;
	return TRUE ;
}
*/

HANDLE CMyDrawBMP::DibFromBitmap(HBITMAP hbm, DWORD biStyle, WORD biBits, HPALETTE hpal)
{
	BITMAP               bm;
    BITMAPINFOHEADER     bi;
    BITMAPINFOHEADER FAR *lpbi;
    DWORD                dwLen;
    HANDLE               hdib;
    HANDLE               h;
    HDC                  hdc;

    if (!hbm)
        return NULL;

    if (hpal == NULL)
        hpal = (HPALETTE)GetStockObject(DEFAULT_PALETTE);

    GetObject(hbm,sizeof(bm),(LPSTR)&bm);

    if (biBits == 0)
        biBits =  bm.bmPlanes * bm.bmBitsPixel;

    bi.biSize               = sizeof(BITMAPINFOHEADER);
    bi.biWidth              = bm.bmWidth;
    bi.biHeight             = bm.bmHeight;
    bi.biPlanes             = 1;
    bi.biBitCount           = biBits;
    bi.biCompression        = biStyle;
    bi.biSizeImage          = 0;
    bi.biXPelsPerMeter      = 0;
    bi.biYPelsPerMeter      = 0;
    bi.biClrUsed            = 0;
    bi.biClrImportant       = 0;

    dwLen  = bi.biSize + PaletteSize(&bi);

    hdc = GetDC(NULL);
    hpal = SelectPalette(hdc,hpal,FALSE);
         RealizePalette(hdc);

    hdib = GlobalAlloc(GHND,dwLen);

    if (!hdib){
        SelectPalette(hdc,hpal,FALSE);
        ReleaseDC(NULL,hdc);
        return NULL;
    }

    lpbi = (BITMAPINFOHEADER FAR *)GlobalLock(hdib);

    *lpbi = bi;

    //  call GetDIBits with a NULL lpBits param, so it will calculate the
    //  biSizeImage field for us
    //
    GetDIBits(hdc, hbm, 0L, (DWORD)bi.biHeight,
        (LPBYTE)NULL, (LPBITMAPINFO)lpbi, (DWORD)DIB_RGB_COLORS);

    bi = *lpbi;
    GlobalUnlock(hdib);

    // If the driver did not fill in the biSizeImage field, make one up 
    if (bi.biSizeImage == 0){
        bi.biSizeImage = WIDTHBYTES((DWORD)bm.bmWidth * biBits) * bm.bmHeight;

        if (biStyle != BI_RGB)
            bi.biSizeImage = (bi.biSizeImage * 3) / 2;
    }

    //  realloc the buffer big enough to hold all the bits 
    dwLen = bi.biSize + PaletteSize(&bi) + bi.biSizeImage;
    if (h = GlobalReAlloc(hdib,dwLen,0))
        hdib = h;
    else{
        GlobalFree(hdib);
        hdib = NULL;

        SelectPalette(hdc,hpal,FALSE);
        ReleaseDC(NULL,hdc);
        return hdib;
    }

    //  call GetDIBits with a NON-NULL lpBits param, and actualy get the
    //  bits this time
    //
    lpbi = (BITMAPINFOHEADER FAR *)GlobalLock(hdib);

    if (GetDIBits( hdc,
                   hbm,
                   0L,
                   (DWORD)bi.biHeight,
                   (LPBYTE)lpbi + (WORD)lpbi->biSize + PaletteSize(lpbi),
                   (LPBITMAPINFO)lpbi, (DWORD)DIB_RGB_COLORS) == 0){
         GlobalUnlock(hdib);
         hdib = NULL;
         SelectPalette(hdc,hpal,FALSE);
         ReleaseDC(NULL,hdc);
         return NULL;
    }

    bi = *lpbi;
    GlobalUnlock(hdib);

    SelectPalette(hdc,hpal,FALSE);
    ReleaseDC(NULL,hdc);
    return hdib;
}

BOOL CMyDrawBMP::WriteDIB(char *szFile, HANDLE hdib)
{
	BITMAPFILEHEADER    hdr;
    LPBITMAPINFOHEADER  lpbi;
//    OFSTRUCT            of;
	int rc ;

    if (!hdib)
        return FALSE;

    lpbi = (BITMAPINFOHEADER FAR *)GlobalLock (hdib);

    // Fill in the fields of the file header 
    hdr.bfType          = BFT_BITMAP;
    hdr.bfSize          = (DWORD)(GlobalSize (hdib) + SIZEOF_BITMAPFILEHEADER_PACKED);
    hdr.bfReserved1     = 0;
    hdr.bfReserved2     = 0;
    hdr.bfOffBits       = (DWORD) (SIZEOF_BITMAPFILEHEADER_PACKED + lpbi->biSize +
	                               PaletteSize(lpbi));
    rc=conv_bmp_tif(hdr , 
                    (unsigned char *)lpbi , (DWORD)GlobalSize (hdib) ,
                    szFile ,
				    196 , 40 , 0 ) ; //Quality:196   Left:80   Right:0
    GlobalUnlock (hdib);

	if(rc==-1) return FALSE ;
    return TRUE;
}

WORD CMyDrawBMP::PaletteSize(VOID *pv)
{
	LPBITMAPINFOHEADER lpbi;
    WORD               NumColors;

    lpbi      = (LPBITMAPINFOHEADER)pv;
    NumColors = DibNumColors(lpbi);

    if (lpbi->biSize == sizeof(BITMAPCOREHEADER))
        return (WORD)(NumColors * sizeof(RGBTRIPLE));
    else
        return (WORD)(NumColors * sizeof(RGBQUAD));
}

WORD CMyDrawBMP::DibNumColors(VOID *pv)
{
	INT                 bits;
    LPBITMAPINFOHEADER  lpbi;
    LPBITMAPCOREHEADER  lpbc;

    lpbi = ((LPBITMAPINFOHEADER)pv);
    lpbc = ((LPBITMAPCOREHEADER)pv);

    /*  With the BITMAPINFO format headers, the size of the palette
     *  is in biClrUsed, whereas in the BITMAPCORE - style headers, it
     *  is dependent on the bits per pixel ( = 2 raised to the power of
     *  bits/pixel).
     */
    if (lpbi->biSize != sizeof(BITMAPCOREHEADER)){
        if (lpbi->biClrUsed != 0)
            return (WORD)lpbi->biClrUsed;
        bits = lpbi->biBitCount;
    }
    else
        bits = lpbc->bcBitCount;

    switch (bits){
        case 1:
                return 2;
        case 4:
                return 16;
        case 8:
                return 256;
        default:
                /* A 24 bitcount DIB has no color table */
                return 0;
    }
}

int CMyDrawBMP::conv_bmp_tif(BITMAPFILEHEADER BmpHead, unsigned char *BmpBody, DWORD sizeofBmpBody, char *TagetFile, int qulity, int leftborder, int rightborder)
{
	int m;
    long tifflenght ;
    long offset ;
    long l ;
	long value ;
    unsigned char buf[355] ;
    int bw_bit_count,bw_flag,rl_block_count,t_bit_count;
    if ( rightborder == 0 )
       rightborder = 1728 ;

    memset(buf ,0,255) ;
    Wdatacount = 0 ;
    StripBytecount = 0 ;
    bmp_linecount = 0 ;
    if ( (fw_fax=fopen(TagetFile,"wb"))==NULL )
    {
        return -1 ;
    }
    sobmphead.ID[0] = LOBYTE(BmpHead.bfType);
	sobmphead.ID[1] = HIBYTE(BmpHead.bfType);
    if ( memcmp(sobmphead.ID,"BM",2) != 0 )
	{
	   fclose(fw_fax);	 
       return -1 ;
	}
    sobmphead.black  = 0 ;
    sobmphead.white  = 1 ;
    sobmphead.width  = BmpBody[4]+BmpBody[5]*256+BmpBody[6]*256*256+BmpBody[7]*256*256*256 ;
	sobmphead.height = BmpBody[8]+BmpBody[9]*256+BmpBody[10]*256*256+BmpBody[11]*256*256*256 ;

    byte_len = (sobmphead.width+7L)/8L ;
    if( byte_len & 0x0003)
    {
       byte_len |= 0x0003 ;
       ++byte_len ;
    }
    fwrite("II",1,2,fw_fax);
	value = 42 ;
	fwrite(&value,1,2,fw_fax);
	value = 0 ;
	fwrite(&value,1,4,fw_fax);
    fseek(fw_fax,0L,SEEK_END);
    memset(g3_str,0,1728*100) ;
    while ( bmp_linecount < (long)sobmphead.height )
    {
       memset(wbuf1,0,1728);
       offset = (long)(sobmphead.height-bmp_linecount-1) * byte_len + 48L ; //(long)sizeof(BMPHEAD);
       // fseek(fp_fax, offset, SEEK_SET);
           /************ Error Here ************/
       memset(buf,0,255) ;
       // fread(buf,1,(int)byte_len,fp_fax);
	   memcpy(buf,BmpBody+offset,byte_len);
       bw_bit_count = leftborder ;
	   bw_flag = 0 ;
	   rl_block_count = 0 ;
	   t_bit_count = 0 ;
       for( m = 0 ;m< (int)sobmphead.width && m <= rightborder ; m++ )
       {
          if( buf[m>>3] & masktable[m&0x0007] )
		  {
             if ( bw_flag != 0 )
			 {
				 wbuf1[rl_block_count] = bw_bit_count ;
				 rl_block_count ++ ;
				 t_bit_count += bw_bit_count ;
				 bw_bit_count  = 0 ;
			 }
			 bw_flag = 0 ;
		     bw_bit_count ++ ;
			 // wbuf1[m+leftborder] = (unsigned char)sobmphead.black ;
		  }
          else
		  {
             if ( bw_flag != 1 )
			 {
				 wbuf1[rl_block_count] = bw_bit_count ;
				 rl_block_count ++ ;
				 t_bit_count += bw_bit_count ;
				 bw_bit_count  = 0 ;
			 }
			 bw_flag = 1 ;
			 bw_bit_count ++ ;
             // wbuf1[m+leftborder] = (unsigned char)sobmphead.white ;
		  }
       }
  	   if ( bw_bit_count > 0 )
	   {
		  if ( bw_flag == 0 )  
		  {
             wbuf1[rl_block_count] = (1728-t_bit_count) ;
			 rl_block_count++ ;
		  }
		  else
		  {
	 	     wbuf1[rl_block_count] = bw_bit_count ;
			 rl_block_count++ ;
			 t_bit_count += bw_bit_count ;
			 if ( (1728-t_bit_count) > 0 )
			 {
                wbuf1[rl_block_count] = (1728-t_bit_count) ;
			    rl_block_count++ ;
			 }
		  }
	   }
       // lp = m ;
       bmp_linecount++ ;
//       Wdatacount = 0 ;
       //stripcount=0 ;
//       memset(g3_str,0,500) ;
       // data_compress(wbuf1,lq1+1) ;
	   data_compress(wbuf1,rl_block_count) ;
//       fwrite(g3_str,Wdatacount,1,fw_fax) ;
    }
    if ( Wdatacount > 0 )
      fwrite( g3_str,1,Wdatacount,fw_fax);

    l=ftell(fw_fax);
	value = tagcount ;
	fwrite(&value,1,2,fw_fax);
    Writetag(fw_fax,NewSubfileType,TiffLong,1L,2) ;
    Writetag(fw_fax,ImageWidth,TiffShort,1L,1728L) ;    //width
    tifflenght = sobmphead.height ;
    Writetag(fw_fax,ImageLength,TiffLong,1L,tifflenght) ; //height
    Writetag(fw_fax,BitsPerSample,TiffShort,1L,1L) ;       //bitspersamples
    Writetag(fw_fax,Compression,TiffShort,1L,3L) ;         //compression
    Writetag(fw_fax,FillOrder,TiffShort,1L,2L) ;           //fillorder
    Writetag(fw_fax,StripOffsets,TiffLong,1L,8L) ;   // ?
    Writetag(fw_fax,SamplesPerPixel,TiffShort,1L,1L) ;
    Writetag(fw_fax,RowPerStrip,TiffLong,1L,tifflenght) ;
    Writetag(fw_fax,StripByteCounts,TiffLong,1L,StripBytecount) ;  //?
    Writetag(fw_fax,XResolution,TiffRational,1L,StripBytecount+(14+12*tagcount+0)) ;  //?
    Writetag(fw_fax,YResolution,TiffRational,1L,StripBytecount+(14+12*tagcount+8)) ;  //?
    Writetag(fw_fax,Group3Options,TiffLong,1L,0L);
    Writetag(fw_fax,PageNumber,TiffShort,2L,0x010000); //3L
    Writetag(fw_fax,ResolutionUnit,TiffShort,1L,2);
  //  Writetag(fw_fax,SoftWare,TiffAscii,26L,StripBytecount+(14+12*tagcount+16));
  //  Writetag(fw_fax,DateTime,TiffAscii,21L,StripBytecount+(14+12*tagcount+16+26));
  //  Writetag(fw_fax,Artist,TiffAscii,14L,StripBytecount+(14+12*tagcount+16+47));
  //  Writetag(fw_fax,HostComputer,TiffAscii,32L,StripBytecount+(14+12*tagcount+16+61));
	value = 0 ;
	fwrite(&value,1,4,fw_fax);
	value = 204 ;
	fwrite(&value,1,4,fw_fax);
	value = 1 ;
	fwrite(&value,1,4,fw_fax);
	value = qulity ;
	fwrite(&value,1,4,fw_fax);
	value = 1 ;
	fwrite(&value,1,4,fw_fax);
    //fwrite("MVS (Multi Voice System) \0",1,26,fw_fax);
    //get_now_time(buf) ;
    //fwrite(buf,1,20,fw_fax);
    //fwrite("\0",1,1,fw_fax);
    //fwrite("MITAKE  Corp \0",1,14,fw_fax);
    //fwrite("PC Campalible & WIN / DOS above  \0",1,32,fw_fax);
    fseek(fw_fax,4L,SEEK_SET);
	value = l ;
	fwrite(&value,1,4,fw_fax);
    bmp_linecount = 0 ;
    fclose(fw_fax);
    return  1 ;
}

void CMyDrawBMP::Writetag(FILE *fpfax, int tag, int type, long length, long offset)
{
	TIFF_TAG ttag;

	ttag.tag = tag ;
	ttag.type = type ;
	ttag.length = length ;
	ttag.offset = offset ;
	fwrite(&ttag,1,sizeof(TIFF_TAG),fw_fax);
}

void CMyDrawBMP::data_compress(unsigned int *str, int len)
{
	int i;
   int plen ;
   plen = len ;
//   ch_val = str[0] ;
//   bit_status = ch_val ;

   g3_str[Wdatacount] = 0x0 ;
   if ( Wdatacount >= 1728*100 )
   {
      fwrite( g3_str,1,Wdatacount,fw_fax);
 	  Wdatacount = 0 ;
	  memset(g3_str,0,1728*100) ;
   }
   Wdatacount ++ ;
   StripBytecount ++ ;
   g3_str[Wdatacount] = 0x08 ;

   bitcount = 4 ;

   for (i=0;i<plen;i++)
   {
      Tif_G3_mode(i%2,str[i]) ;
   }
   if ( Wdatacount >= 1728*100 )
   {
      fwrite( g3_str,1,Wdatacount,fw_fax);
 	  Wdatacount = 0 ;
	  memset(g3_str,0,1728*100) ;
   }
   Wdatacount++ ;
   StripBytecount++ ;
}

void CMyDrawBMP::Tif_G3_mode(int status, int countp)
{
	int run_lenght[2][2];
   int make_code,term_code;
   int i,j,num;
   unsigned char mark;
   int CodeVal,CodeLen;

   make_code=countp/64;
   term_code=countp%64;
/*
   for (i=0;i<2;i++)
      for (j=0;j<2;j++)
        run_lenght[i][j]=0;
*/
   num=0;

   if ( (make_code == 0) && (term_code == 0) )
   {
      if (status==0)
      {
         run_lenght[0][0]=WhiteRun[0].wCodeWord;
         run_lenght[0][1]=WhiteRun[0].wCodeLen;
      }
      else if (status==1)
      {
         run_lenght[0][0]=BlackRun[0].wCodeWord;
         run_lenght[0][1]=BlackRun[0].wCodeLen;
      }
      num = 1 ;
   }
   else
   {
      if (make_code != 0)
	  {
         num=1;
         if (status==0)
		 {
            run_lenght[0][0]=WhiteRun[63+make_code].wCodeWord;
            run_lenght[0][1]=WhiteRun[63+make_code].wCodeLen;
		 }
         else if (status==1)
		 {
            run_lenght[0][0]=BlackRun[63+make_code].wCodeWord;
            run_lenght[0][1]=BlackRun[63+make_code].wCodeLen;
		 }
	  }

      if (term_code != 0)
	  {
         if (status==0)
		 {
            run_lenght[num][0]=WhiteRun[term_code].wCodeWord;
            run_lenght[num][1]=WhiteRun[term_code].wCodeLen;
		 }
         else if (status==1)
		 {
            run_lenght[num][0]=BlackRun[term_code].wCodeWord;
            run_lenght[num][1]=BlackRun[term_code].wCodeLen;
		 }
         num++;
	  }

      if (term_code == 0  && make_code != 0)
	  {
         if (status==0)
		 {
            run_lenght[num][0]=WhiteRun[term_code].wCodeWord;
            run_lenght[num][1]=WhiteRun[term_code].wCodeLen;
		 }
         else if (status==1)
		 {
            run_lenght[num][0]=BlackRun[term_code].wCodeWord;
            run_lenght[num][1]=BlackRun[term_code].wCodeLen;
		 }
         num++;
	  }
   }

   mark = g3_str[Wdatacount] ;// [Wdatacount] ;
   for (i=0;i<num;i++)
   {
      CodeLen=run_lenght[i][1];
      for (j=CodeLen-1;j>=0;j--)
      {
         CodeVal= run_lenght[i][0];   
         CodeVal= CodeVal & int_flag[j];
		 if ( CodeVal > 0 )
            mark = mark | masktable[7-bitcount];
         bitcount++;
         if (bitcount == 8)
         {
			if ( Wdatacount >= 1728*100 )
			{
			   fwrite( g3_str,1,Wdatacount,fw_fax);
			   Wdatacount = 0 ;
			   memset(g3_str,0,1728*100) ;
			}
            g3_str[Wdatacount]=mark;
            Wdatacount++;
            StripBytecount++;
            mark=0;
            bitcount=0;
         }
      }
   }
   g3_str[Wdatacount]=mark;
}
