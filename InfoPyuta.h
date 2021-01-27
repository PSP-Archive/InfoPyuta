/*===================================================================*/
/*                                                                   */
/*  InfoPyuta.h : �w�b�_�t�@�C��(�҂イ���G�~�����[�^)               */
/*                                                                   */
/*  2001/04/23    Jay's Factory                                      */
/*                                                                   */
/*===================================================================*/

#ifndef INFOPYUTA_H_INCLUDED
#define INFOPYUTA_H_INCLUDED

/*-------------------------------------------------------------------*/
/*  �C���N���[�h�t�@�C��                                             */
/*-------------------------------------------------------------------*/

#include "Type.h"

/*-------------------------------------------------------------------*/
/*  �҂イ�����\�[�X                                                 */
/*-------------------------------------------------------------------*/

extern Byte RAM[ 0x4000 ];             /* �O��RAM */
extern Byte ROM[ 0x5000 ];             /* �O��ROM */
extern Byte INTLRAM[ 0x100 ];          /* ����RAM */           
extern Byte CRU[ 0x1000 ];	           /* CRU��� (�s�v?) */

extern Word Scanline;                  /* �X�L�������C�� */

/*-------------------------------------------------------------------*/
/*  �v���g�^�C�v                                                     */
/*-------------------------------------------------------------------*/

/* �҂イ���G�~�����[�V���� */
int InfoPyuta_Main();

/* �҂イ�������� */
int InfoPyuta_Reset();

/* �҂イ���G�~�����[�V�������[�v */
int InfoPyuta_Cycle();

/* �����������̏��� */
int InfoPyuta_HSync( void );

/*  �J�Z�b�g���҂イ���Ƀ��[�h */
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
/*  �萔                                                             */
/*-------------------------------------------------------------------*/

/* �҂イ���f�B�X�v���C�T�C�Y */
#define Pyuta_DISP_WIDTH      256
#define Pyuta_DISP_HEIGHT     192

/* �������A�Ԋu: 19.91[ms] */
/* �N���b�N���g��: 2 [MHz] = 0.0005[ms] */
/* 1���ߎ��s���̕��σN���b�N��: 7 */

/* �����������̕��ϖ��ߎ��s�� */
#define INST_PER_HSYNC        22

/* �X�L�������C���֘A */
#define ON_SCREEN_START       0
#define VBLANK_START          244
#define VBLANK_END            261

#endif    /* #ifndef INFOPYUTA_H_INCLUDED */