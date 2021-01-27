/*===================================================================*/
/*                                                                   */
/*  InfoPyuta_Vdp.cpp : C++ファイル( VDP: Video Display Processor)   */
/*                                                                   */
/*  2001/04/27    Jay's Factory                                      */
/*                                                                   */
/*===================================================================*/

/*-------------------------------------------------------------------*/
/*  インクルードファイル                                             */
/*-------------------------------------------------------------------*/

#include <stdio.h>
#include "InfoPyuta_Vdp.h"
#include "InfoPyuta_System.h"

#define ZeroMemory(lpBuf,nSize)   memset((lpBuf),(0),(nSize))

/*-------------------------------------------------------------------*/
/*  VDPリソース                                                      */
/*-------------------------------------------------------------------*/

/* ワーク画面 */
Word WorkFrame[ Pyuta_DISP_WIDTH * Pyuta_DISP_HEIGHT ];     

Byte VDP[ 0x4000 ];                  /* ビデオRAM */
Byte VDPREG[ 9 ];                    /* VDP読み込み専用レジスタ */
Byte VDPBuffer[ 0x4000 ];            /* キャラクタバッファ */
Byte SpriteBuffer[ 0x100 ];          /* スプライトバッファ */
Word VDPADD;                         /* VDPアドレスカウンタ */
Word VDPST;                          /* VDPステータスフラグ */
Byte INT_PIN;                        /* VDP割込みピン */

int redraw_needed;                   /* 書き換えフラグ*/
Byte vdpaccess;                      /* VDPプリフェッチ */

Word SIT;                            /* スクリーンイメージテーブル */
Word CT;                             /* カラーテーブル */
Word PDT;                            /* パターン記述子テーブル */
Word SAL;                            /* スプライト配置テーブル */
Word SDT;                            /* スプライト記述子テーブル */

Word p_add;                          /* アドレス */

/*-------------------------------------------------------------------*/
/*  VDPリセット                                                      */
/*-------------------------------------------------------------------*/
void VDPreset()
{
  ZeroMemory( VDP, 0x4000 );         /* ビデオRAM */
  ZeroMemory( VDPREG, 0x9 );         /* VDP読み込み専用レジスタ */
  ZeroMemory( VDPBuffer, 0x4000 );   /* キャラクタバッファ*/  
  VDPADD = 0x0000;                   /* VDPアドレスカウンタ */
  VDPST = 0x0000;                    /* VDPステータスフラグ */
  INT_PIN = 0x00;                    /* VDP割込みピン */

  redraw_needed = 0;                 /* 書き換えフラグ*/
  vdpaccess = 0;                     /* VDPプリフェッチ */

#if 0
  /* VDPテスト用 */
  VDPREG[1] = 0x03;
  VDPREG[5] = 0x20;
  VDPREG[6] = 0x04;

  VDP[ 0x1000 ] = 0x20;
  VDP[ 0x1001 ] = 0x20;
  VDP[ 0x1002 ] = 0x00;
  VDP[ 0x1003 ] = 0x04;

  VDP[ 0x2000 ] = 0x00;
  VDP[ 0x2001 ] = 0x38;
  VDP[ 0x2002 ] = 0x44;
  VDP[ 0x2003 ] = 0x44;
  VDP[ 0x2004 ] = 0x38;
  VDP[ 0x2005 ] = 0x10;
  VDP[ 0x2006 ] = 0x38;
  VDP[ 0x2007 ] = 0x10;

  VDP[ 0x2008 ] = 0x00;
  VDP[ 0x2009 ] = 0x38;
  VDP[ 0x200a ] = 0x44;
  VDP[ 0x200b ] = 0x44;
  VDP[ 0x200c ] = 0x38;
  VDP[ 0x200d ] = 0x10;
  VDP[ 0x200e ] = 0x38;
  VDP[ 0x200f ] = 0x10;

  VDP[ 0x2010 ] = 0x00;
  VDP[ 0x2011 ] = 0x38;
  VDP[ 0x2012 ] = 0x44;
  VDP[ 0x2013 ] = 0x44;
  VDP[ 0x2014 ] = 0x38;
  VDP[ 0x2015 ] = 0x10;
  VDP[ 0x2016 ] = 0x38;
  VDP[ 0x2017 ] = 0x10;

  VDP[ 0x2018 ] = 0x00;
  VDP[ 0x2019 ] = 0x38;
  VDP[ 0x201a ] = 0x44;
  VDP[ 0x201b ] = 0x44;
  VDP[ 0x201c ] = 0x38;
  VDP[ 0x201d ] = 0x10;
  VDP[ 0x201e ] = 0x38;
  VDP[ 0x201f ] = 0x10;
#endif
}

/*-------------------------------------------------------------------*/
/*  レジスタから各テーブルアドレスを取得                             */
/*-------------------------------------------------------------------*/
void gettables()
{
  SIT=(VDPREG[2]<<10)&0x3fff;         /* スクリーンイメージテーブル */

  CT=(VDPREG[3]<<6)&0x3fff;           /* カラーテーブル */

  PDT=(VDPREG[4]<<11)&0x3fff;         /* パターン記述子テーブル */ 

  SAL=(VDPREG[5]<<7)&0x3fff;          /* スプライト配置テーブル */

  SDT=(VDPREG[6]<<11)&0x3fff;         /* スプライト記述子テーブル */
}

/*-------------------------------------------------------------------*/
/*  グラフィックモードで描画                                         */
/*-------------------------------------------------------------------*/
void VDPgraphics()
{
  int xx,t;                           /* テンポラリ */ 
  int i1,i2,i3,i4;                    /* テンポラリ */
  int vdp_drawn[256];                 /* キャラクタ描画済みフラグ */
  RECT_T myrect;
  int tcol;
  int c1, c2;

  char szMsg[512];

  gettables();

  sprintf( szMsg, "SIT: 0x%x, PDT: 0x%x, CT: 0x%x, SAL: 0x%x, SDT: 0x%x VR6: 0x%x", SIT, PDT, CT, SAL, SDT, VDPREG[6] );
#if 0
  warn( szMsg );
#endif

  /* vdp_drawn[]配列をクリア */
  ZeroMemory(&vdp_drawn[0], sizeof(int) * 256);

  /* 各タイルをスクリーンバッファに転送 */
  /* VDPBUFFERにおけるベーステーブルの位置を計算 */

  i4=0;

  for (i1=0; i1<192; i1+=8)                  /* yループ */
  { 
    for (i2=0; i2<256; i2+=8)                /* xループ */
    { 
      if (vdp_drawn[VDP[SIT]]==0 )
      {  
        /* ひとつのキャラクタをVDPBUFFERに描画し，描画済みフラグをセットする */
        /* したがって，本フレームでは，2度と同じキャラクタは，描画されない */

        xx=VDP[SIT]<<3;
        p_add=PDT+xx;
    
        for (i3=0; i3<8; i3++)
        {  
          t=VDP[p_add++];
          tcol=VDP[CT + (VDP[SIT]>>3)];
          c1=tcol&0xf;
          c2=tcol>>4;

          pixel(VDPBuffer, xx, i3, t&0x80 ? c2 : c1);
          pixel(VDPBuffer, xx+1, i3, t&0x40 ? c2 : c1);
          pixel(VDPBuffer, xx+2, i3, t&0x20 ? c2 : c1);
          pixel(VDPBuffer, xx+3, i3, t&0x10 ? c2 : c1);
          pixel(VDPBuffer, xx+4, i3, t&0x08 ? c2 : c1);
          pixel(VDPBuffer, xx+5, i3, t&0x04 ? c2 : c1);
          pixel(VDPBuffer, xx+6, i3, t&0x02 ? c2 : c1);
          pixel(VDPBuffer, xx+7, i3, t&0x01 ? c2 : c1);
        }
        vdp_drawn[VDP[SIT]]=1;
      }
      myrect.top=0;
      myrect.bottom = 8;
      myrect.left=VDP[SIT++]<<3;
      myrect.right=myrect.left + 8;
      BltFast( i2, i1, VDPBuffer, &myrect );
    }
  }
  /* スプライトを描画 */
  DrawSprites();
}

/*-------------------------------------------------------------------*/
/*  矩形領域をワーク画面に転送                                       */
/*-------------------------------------------------------------------*/
void BltFast( int x, int y, Byte *pbyBuf, RECT_T *rect )
{
  register int rx, ry, wx, wy;

  for ( ry = rect->top, wy = y; ry < rect->bottom; ry++, wy++ ) 
  {
    for ( rx = rect->left, wx = x; rx < rect->right; rx++, wx++ )
    {
      /* パレットを使用して，ワーク画面に転送 */
      WorkFrame[ ( wy << 8 ) + wx ] = PyutaPalette[ pbyBuf[ ( ry << 11 ) + rx ] ];
    }
  }
}

/*-------------------------------------------------------------------*/
/*  一時バッファに点を描画                                           */
/*-------------------------------------------------------------------*/
void pixel(Byte* pbyBuf, int x, int y, int c)
{
  /* 透明時 */
  if ( c==0 )
    c=VDPREG[7]&0x0f;

  /* バッファに点を描画 */
  pbyBuf[ ( y << 11 ) + x ] = c; 
}

/*-------------------------------------------------------------------*/
/*  スプライト描画                                                   */
/*-------------------------------------------------------------------*/
void DrawSprites()
{
  RECT_T myrect;
	int i1, i3, xx, yy, pat, col, p_add, t;
  int highest;

	highest=31;

  /* アクティブで，優先順位の高いスプライトを見付ける */
	for (i1=0; i1<32; i1++)       /* 32スプライト */
	{
		yy=VDP[SAL+(i1<<2)];
		if (yy==0xd0)
		{
			highest=i1-1;
			break;
		}
	}

	for (i1=highest; i1>=0; i1--)	
	{
		yy=VDP[SAL++]+1;            /* スプライトY座標, 0行目は255で指定 */
		if (yy>255) yy=0;

		xx=VDP[SAL++];              /* スプライトX座標 */

		pat=VDP[SAL++];             /* スプライトパターン */
		if (VDPREG[1] & 0x2)
			pat=pat&0xfc;             /* 16x16の時には，パターンアドレスは4の倍数 */

		col=VDP[SAL]&0xf;           /* スプライトカラー */
	
		if (VDP[SAL++]&0x10)        /* アリークロック */
			xx-=32;

		if (yy==0xd1)
			i1=32;                    /* 0xd0はスプライトリストの終了 */

		if ((yy<192)&&(col))        /* スクリーン内で，透明色でない時 */
		{	
      /* バッファを透明色でクリア */
      ZeroMemory( SpriteBuffer, sizeof( SpriteBuffer ) );

			p_add=SDT+(pat<<3);
		
			for (i3=0; i3<8; i3++)
			{	
				t=VDP[p_add++];

				if (t&0x80)
					pixel2(SpriteBuffer, 0, i3, col);
				if (t&0x40)
					pixel2(SpriteBuffer, 1, i3, col);
				if (t&0x20)
					pixel2(SpriteBuffer, 2, i3, col);
				if (t&0x10)
					pixel2(SpriteBuffer, 3, i3, col);
				if (t&0x08)
					pixel2(SpriteBuffer, 4, i3, col);
				if (t&0x04)
					pixel2(SpriteBuffer, 5, i3, col);
				if (t&0x02)
					pixel2(SpriteBuffer, 6, i3, col);
				if (t&0x01)
					pixel2(SpriteBuffer, 7, i3, col);

        /* 16x16の時には，3キャラクタ余分に描く */
				if (VDPREG[1]&0x02)
				{	
					t=VDP[p_add+7];
					if (t&0x80)
						pixel2(SpriteBuffer, 0, i3+8, col);
					if (t&0x40)
						pixel2(SpriteBuffer, 1, i3+8, col);
					if (t&0x20)
						pixel2(SpriteBuffer, 2, i3+8, col);
					if (t&0x10)
						pixel2(SpriteBuffer, 3, i3+8, col);
					if (t&0x08)
						pixel2(SpriteBuffer, 4, i3+8, col);
					if (t&0x04)
						pixel2(SpriteBuffer, 5, i3+8, col);
					if (t&0x02)
						pixel2(SpriteBuffer, 6, i3+8, col);
					if (t&0x01)
						pixel2(SpriteBuffer, 7, i3+8, col);

					t=VDP[p_add+15];
					if (t&0x80)
						pixel2(SpriteBuffer, 8, i3, col);
					if (t&0x40)
						pixel2(SpriteBuffer, 9, i3, col);
					if (t&0x20)
						pixel2(SpriteBuffer, 10, i3, col);
					if (t&0x10)	
						pixel2(SpriteBuffer, 11, i3, col);
					if (t&0x08)
						pixel2(SpriteBuffer, 12, i3, col);
					if (t&0x04)
						pixel2(SpriteBuffer, 13, i3, col);
					if (t&0x02)
						pixel2(SpriteBuffer, 14, i3, col);
					if (t&0x01)
						pixel2(SpriteBuffer, 15, i3, col);

					t=VDP[p_add+23];
					if (t&0x80)
						pixel2(SpriteBuffer, 8, i3+8, col);
					if (t&0x40)
						pixel2(SpriteBuffer, 9, i3+8, col);
					if (t&0x20)
						pixel2(SpriteBuffer, 10, i3+8, col);
					if (t&0x10)
						pixel2(SpriteBuffer, 11, i3+8, col);
					if (t&0x08)
						pixel2(SpriteBuffer, 12, i3+8, col);
					if (t&0x04)
						pixel2(SpriteBuffer, 13, i3+8, col);
					if (t&0x02)
						pixel2(SpriteBuffer, 14, i3+8, col);
					if (t&0x01)
						pixel2(SpriteBuffer, 15, i3+8, col);
				}
			}

		  /* スプライト拡大機能 */
      myrect.top=0;
			myrect.left=0;
			myrect.right=myrect.left+16;
			myrect.bottom=myrect.top+16;

      if (VDPREG[1]&0x01)
			{	
        /* 拡大スプライトを表示 */
        BltFast3( yy, xx, SpriteBuffer, &myrect );
			}
			else
			{	
        /* 標準スプライトを表示 */
        BltFast2( yy, xx, SpriteBuffer, &myrect );
			}
    }
	}
}

/*-------------------------------------------------------------------*/
/*  矩形領域をワーク画面に転送（標準スプライト用）                   */
/*-------------------------------------------------------------------*/
void BltFast2( int x, int y, Byte *pbyBuf, RECT_T *rect )
{
  register int rx, ry, wx, wy;

  for ( ry = rect->top, wy = y; ry < rect->bottom; ry++, wy++ ) 
  {
    for ( rx = rect->left, wx = x; rx < rect->right; rx++, wx++ )
    {
      /* パレットを使用して，ワーク画面に転送 */
      if ( pbyBuf[ ( ry << 4 ) + rx ] )
      {
        WorkFrame[ ( wy << 8 ) + wx ] = PyutaPalette[ pbyBuf[ ( ry << 4 ) + rx ] ];
      }
    }
  }
}

/*-------------------------------------------------------------------*/
/*  一時バッファに点を描画（スプライト用）                           */
/*-------------------------------------------------------------------*/
void pixel2(Byte* pbyBuf, int x, int y, int c)
{
  /* 透明時 */
  if ( c==0 )
    c=VDPREG[7]&0x0f;

  /* バッファに点を描画 */
  pbyBuf[ ( y << 4 ) + x ] = c; 
}

/*-------------------------------------------------------------------*/
/*  矩形領域をワーク画面に転送（拡大スプライト用）                   */
/*-------------------------------------------------------------------*/
void BltFast3( int x, int y, Byte *pbyBuf, RECT_T *rect )
{
  register int rx, ry, wx, wy;

  for ( ry = rect->top, wy = y; ry < rect->bottom; ry++, wy+=2 ) 
  {
    for ( rx = rect->left, wx = x; rx < rect->right; rx++, wx+=2 )
    {
      Byte col = pbyBuf[ ( ry << 4 ) + rx ];

      /* パレットを使用して，ワーク画面に転送 */
      if ( col )
      {
        WorkFrame[ ( wy << 8 ) + wx ]                 = PyutaPalette[ col ];
        WorkFrame[ ( wy << 8 ) + ( wx + 1 ) ]         = PyutaPalette[ col ];
        WorkFrame[ ( ( wy + 1 ) << 8 ) + wx ]         = PyutaPalette[ col ];
        WorkFrame[ ( ( wy + 1 ) << 8 ) + ( wx + 1 ) ] = PyutaPalette[ col ];
      }
    }
  }
}
