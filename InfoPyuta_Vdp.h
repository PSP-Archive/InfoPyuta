/*===================================================================*/
/*                                                                   */
/*  InfoPyuta_Vdp.h : �w�b�_�t�@�C��(VDP)                            */
/*                                                                   */
/*  2001/04/26    Jay's Factory                                      */
/*  1999/03/28    M.Brent ( Ami 99 - TI-99�G�~�����[�^  )            */
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
/*  �萔                                                             */
/*-------------------------------------------------------------------*/

#define VDPST_INT         0x80

/*-------------------------------------------------------------------*/
/*  �O���[�o���ϐ�                                                   */
/*-------------------------------------------------------------------*/

extern Word WorkFrame[ Pyuta_DISP_WIDTH * Pyuta_DISP_HEIGHT ];

extern Byte VDP[ 0x4000 ];									/* �r�f�IRAM */
extern Byte VDPREG[ 9 ];										/* VDP�ǂݍ��ݐ�p���W�X�^ */
extern Byte VDPBuffer[ 0x4000 ];            /* �L�����N�^�o�b�t�@*/
extern Word VDPADD;										      /* VDP�A�h���X�J�E���^ */
extern Word VDPST;											    /* VDP�X�e�[�^�X�t���O */
extern Byte INT_PIN;                        /* VDP�����݃s�� */

extern int redraw_needed;									  /* ���������t���O*/
extern Byte vdpaccess;							        /* VDP�v���t�F�b�` */

/*-------------------------------------------------------------------*/
/*  �v���g�^�C�v�錾                                                 */
/*-------------------------------------------------------------------*/

/* VDP���Z�b�g */
void VDPreset( void );

/* �O���t�B�b�N���[�h�ŕ`�� */
void VDPgraphics( void );

/* �ꎞ�o�b�t�@�ɓ_��`�� */
void pixel(Byte* pbyBuf, int x, int y, int c);

/* ��`�̈�����[�N��ʂɓ]�� */
void BltFast( int x, int y, Byte *pbyBuf, RECT_T *rect ); 

/* �X�v���C�g�`�� */
void DrawSprites();

/* �ꎞ�o�b�t�@�ɓ_��`��(�X�v���C�g�p) */
void pixel2(Byte* pbyBuf, int x, int y, int c);

/* ��`�̈�����[�N��ʂɓ]��(�W���X�v���C�g�p) */
void BltFast2( int x, int y, Byte *pbyBuf, RECT_T *rect ); 

/* ��`�̈�����[�N��ʂɓ]��(�g��X�v���C�g�p) */
void BltFast3( int x, int y, Byte *pbyBuf, RECT_T *rect ); 

#endif /* InfoPyuta_VDP_H_INCLUDED */