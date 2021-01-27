/*===================================================================*/
/*                                                                   */
/*  InfoPyuta.h : ヘッダファイル(Windows依存コード)                  */
/*                                                                   */
/*  2001/04/23    Jay's Factory                                      */
/*                                                                   */
/*===================================================================*/

#ifndef InfoPyuta_SYSTEM_H_INCLUDED
#define InfoPyuta_SYSTEM_H_INCLUDED

/*-------------------------------------------------------------------*/
/*  インクルードファイル                                             */
/*-------------------------------------------------------------------*/
#include "Type.h"

/*-------------------------------------------------------------------*/
/*  プロトタイプ宣言                                                 */
/*-------------------------------------------------------------------*/
void warn( char* );
int InfoPyuta_ReadRom( const char *pszFileName );
void InfoPyuta_LoadFrame( void );
int InfoPyuta_CreateScreen( void ); 

/* 垂直同期毎のウェイト処理 */
void InfoPyuta_Wait( void );

extern Word PyutaPalette[ 16 ];

#endif /* !InfoPyuta_SYSTEM_H_INCLUDED */