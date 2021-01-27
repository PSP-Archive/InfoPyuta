/*===================================================================*/
/*                                                                   */
/*  InfoPyuta.h : �w�b�_�t�@�C��(Windows�ˑ��R�[�h)                  */
/*                                                                   */
/*  2001/04/23    Jay's Factory                                      */
/*                                                                   */
/*===================================================================*/

#ifndef InfoPyuta_SYSTEM_H_INCLUDED
#define InfoPyuta_SYSTEM_H_INCLUDED

/*-------------------------------------------------------------------*/
/*  �C���N���[�h�t�@�C��                                             */
/*-------------------------------------------------------------------*/
#include "Type.h"

/*-------------------------------------------------------------------*/
/*  �v���g�^�C�v�錾                                                 */
/*-------------------------------------------------------------------*/
void warn( char* );
int InfoPyuta_ReadRom( const char *pszFileName );
void InfoPyuta_LoadFrame( void );
int InfoPyuta_CreateScreen( void ); 

/* �����������̃E�F�C�g���� */
void InfoPyuta_Wait( void );

extern Word PyutaPalette[ 16 ];

#endif /* !InfoPyuta_SYSTEM_H_INCLUDED */