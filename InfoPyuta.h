/*===================================================================*/
/*                                                                   */
/*  InfoPyuta.h : ヘッダファイル(ぴゅう太エミュレータ)               */
/*                                                                   */
/*  2001/04/23    Jay's Factory                                      */
/*                                                                   */
/*===================================================================*/

#ifndef INFOPYUTA_H_INCLUDED
#define INFOPYUTA_H_INCLUDED

/*-------------------------------------------------------------------*/
/*  インクルードファイル                                             */
/*-------------------------------------------------------------------*/

#include "Type.h"

/*-------------------------------------------------------------------*/
/*  ぴゅう太リソース                                                 */
/*-------------------------------------------------------------------*/

extern Byte RAM[ 0x4000 ];             /* 外部RAM */
extern Byte ROM[ 0x5000 ];             /* 外部ROM */
extern Byte INTLRAM[ 0x100 ];          /* 内部RAM */           
extern Byte CRU[ 0x1000 ];	           /* CRU空間 (不要?) */

extern Word Scanline;                  /* スキャンライン */

/*-------------------------------------------------------------------*/
/*  プロトタイプ                                                     */
/*-------------------------------------------------------------------*/

/* ぴゅう太エミュレーション */
int InfoPyuta_Main();

/* ぴゅう太初期化 */
int InfoPyuta_Reset();

/* ぴゅう太エミュレーションループ */
int InfoPyuta_Cycle();

/* 水平同期毎の処理 */
int InfoPyuta_HSync( void );

/*  カセットをぴゅう太にロード */
int InfoPyuta_Load( const char *pszFileName );

void debug_write( char* );

Word romword(Word);
void wrword( Word, Word );
Byte rcpubyte( Word );
void wcpubyte( Word, Byte );
void wcru(Word,int);
int rcru(Word);

Byte rvdpbyte(Word);
void wvdpbyte(Word, Byte);
void wVDPreg(Byte, Byte);

/*-------------------------------------------------------------------*/
/*  定数                                                             */
/*-------------------------------------------------------------------*/

/* ぴゅう太ディスプレイサイズ */
#define Pyuta_DISP_WIDTH      256
#define Pyuta_DISP_HEIGHT     192

/* 垂直復帰間隔: 19.91[ms] */
/* クロック周波数: 2 [MHz] = 0.0005[ms] */
/* 1命令実行時の平均クロック数: 7 */

/* 水平同期毎の平均命令実行数 */
#define INST_PER_HSYNC        22

/* スキャンライン関連 */
#define ON_SCREEN_START       0
#define VBLANK_START          244
#define VBLANK_END            261

#endif    /* #ifndef INFOPYUTA_H_INCLUDED */