/*===================================================================*/
/*                                                                   */
/*  InfoPyuta_Vdp.h : ヘッダファイル(VDP)                            */
/*                                                                   */
/*  2001/04/26    Jay's Factory                                      */
/*  1999/03/28    M.Brent ( Ami 99 - TI-99エミュレータ  )            */
/*                                                                   */
/*===================================================================*/

#ifndef InfoPyuta_VDP_H_INCLUDED
#define InfoPyuta_VDP_H_INCLUDED

#include "InfoPyuta.h"
#include "Type.h"

typedef struct
{
  int top;
  int bottom;
  int left;
  int right;
}
RECT_T;

/*-------------------------------------------------------------------*/
/*  定数                                                             */
/*-------------------------------------------------------------------*/

#define VDPST_INT         0x80

/*-------------------------------------------------------------------*/
/*  グローバル変数                                                   */
/*-------------------------------------------------------------------*/

extern Word WorkFrame[ Pyuta_DISP_WIDTH * Pyuta_DISP_HEIGHT ];

extern Byte VDP[ 0x4000 ];									/* ビデオRAM */
extern Byte VDPREG[ 9 ];										/* VDP読み込み専用レジスタ */
extern Byte VDPBuffer[ 0x4000 ];            /* キャラクタバッファ*/
extern Word VDPADD;										      /* VDPアドレスカウンタ */
extern Word VDPST;											    /* VDPステータスフラグ */
extern Byte INT_PIN;                        /* VDP割込みピン */

extern int redraw_needed;									  /* 書き換えフラグ*/
extern Byte vdpaccess;							        /* VDPプリフェッチ */

/*-------------------------------------------------------------------*/
/*  プロトタイプ宣言                                                 */
/*-------------------------------------------------------------------*/

/* VDPリセット */
void VDPreset( void );

/* グラフィックモードで描画 */
void VDPgraphics( void );

/* 一時バッファに点を描画 */
void pixel(Byte* pbyBuf, int x, int y, int c);

/* 矩形領域をワーク画面に転送 */
void BltFast( int x, int y, Byte *pbyBuf, RECT_T *rect ); 

/* スプライト描画 */
void DrawSprites();

/* 一時バッファに点を描画(スプライト用) */
void pixel2(Byte* pbyBuf, int x, int y, int c);

/* 矩形領域をワーク画面に転送(標準スプライト用) */
void BltFast2( int x, int y, Byte *pbyBuf, RECT_T *rect ); 

/* 矩形領域をワーク画面に転送(拡大スプライト用) */
void BltFast3( int x, int y, Byte *pbyBuf, RECT_T *rect ); 

#endif /* InfoPyuta_VDP_H_INCLUDED */