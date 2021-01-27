/*===================================================================*/
/*                                                                   */
/*  InfoPyuta_Vdp.cpp : C++�t�@�C��( VDP: Video Display Processor)   */
/*                                                                   */
/*  2001/04/27    Jay's Factory                                      */
/*                                                                   */
/*===================================================================*/

/*-------------------------------------------------------------------*/
/*  �C���N���[�h�t�@�C��                                             */
/*-------------------------------------------------------------------*/

#include <stdio.h>
#include "InfoPyuta_Vdp.h"
#include "InfoPyuta_System.h"

#define ZeroMemory(lpBuf,nSize)   memset((lpBuf),(0),(nSize))

/*-------------------------------------------------------------------*/
/*  VDP���\�[�X                                                      */
/*-------------------------------------------------------------------*/

/* ���[�N��� */
Word WorkFrame[ Pyuta_DISP_WIDTH * Pyuta_DISP_HEIGHT ];     

Byte VDP[ 0x4000 ];                  /* �r�f�IRAM */
Byte VDPREG[ 9 ];                    /* VDP�ǂݍ��ݐ�p���W�X�^ */
Byte VDPBuffer[ 0x4000 ];            /* �L�����N�^�o�b�t�@ */
Byte SpriteBuffer[ 0x100 ];          /* �X�v���C�g�o�b�t�@ */
Word VDPADD;                         /* VDP�A�h���X�J�E���^ */
Word VDPST;                          /* VDP�X�e�[�^�X�t���O */
Byte INT_PIN;                        /* VDP�����݃s�� */

int redraw_needed;                   /* ���������t���O*/
Byte vdpaccess;                      /* VDP�v���t�F�b�` */

Word SIT;                            /* �X�N���[���C���[�W�e�[�u�� */
Word CT;                             /* �J���[�e�[�u�� */
Word PDT;                            /* �p�^�[���L�q�q�e�[�u�� */
Word SAL;                            /* �X�v���C�g�z�u�e�[�u�� */
Word SDT;                            /* �X�v���C�g�L�q�q�e�[�u�� */

Word p_add;                          /* �A�h���X */

/*-------------------------------------------------------------------*/
/*  VDP���Z�b�g                                                      */
/*-------------------------------------------------------------------*/
void VDPreset()
{
  ZeroMemory( VDP, 0x4000 );         /* �r�f�IRAM */
  ZeroMemory( VDPREG, 0x9 );         /* VDP�ǂݍ��ݐ�p���W�X�^ */
  ZeroMemory( VDPBuffer, 0x4000 );   /* �L�����N�^�o�b�t�@*/  
  VDPADD = 0x0000;                   /* VDP�A�h���X�J�E���^ */
  VDPST = 0x0000;                    /* VDP�X�e�[�^�X�t���O */
  INT_PIN = 0x00;                    /* VDP�����݃s�� */

  redraw_needed = 0;                 /* ���������t���O*/
  vdpaccess = 0;                     /* VDP�v���t�F�b�` */

#if 0
  /* VDP�e�X�g�p */
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
/*  ���W�X�^����e�e�[�u���A�h���X���擾                             */
/*-------------------------------------------------------------------*/
void gettables()
{
  SIT=(VDPREG[2]<<10)&0x3fff;         /* �X�N���[���C���[�W�e�[�u�� */

  CT=(VDPREG[3]<<6)&0x3fff;           /* �J���[�e�[�u�� */

  PDT=(VDPREG[4]<<11)&0x3fff;         /* �p�^�[���L�q�q�e�[�u�� */ 

  SAL=(VDPREG[5]<<7)&0x3fff;          /* �X�v���C�g�z�u�e�[�u�� */

  SDT=(VDPREG[6]<<11)&0x3fff;         /* �X�v���C�g�L�q�q�e�[�u�� */
}

/*-------------------------------------------------------------------*/
/*  �O���t�B�b�N���[�h�ŕ`��                                         */
/*-------------------------------------------------------------------*/
void VDPgraphics()
{
  int xx,t;                           /* �e���|���� */ 
  int i1,i2,i3,i4;                    /* �e���|���� */
  int vdp_drawn[256];                 /* �L�����N�^�`��ς݃t���O */
  RECT_T myrect;
  int tcol;
  int c1, c2;

  char szMsg[512];

  gettables();

  sprintf( szMsg, "SIT: 0x%x, PDT: 0x%x, CT: 0x%x, SAL: 0x%x, SDT: 0x%x VR6: 0x%x", SIT, PDT, CT, SAL, SDT, VDPREG[6] );
#if 0
  warn( szMsg );
#endif

  /* vdp_drawn[]�z����N���A */
  ZeroMemory(&vdp_drawn[0], sizeof(int) * 256);

  /* �e�^�C�����X�N���[���o�b�t�@�ɓ]�� */
  /* VDPBUFFER�ɂ�����x�[�X�e�[�u���̈ʒu���v�Z */

  i4=0;

  for (i1=0; i1<192; i1+=8)                  /* y���[�v */
  { 
    for (i2=0; i2<256; i2+=8)                /* x���[�v */
    { 
      if (vdp_drawn[VDP[SIT]]==0 )
      {  
        /* �ЂƂ̃L�����N�^��VDPBUFFER�ɕ`�悵�C�`��ς݃t���O���Z�b�g���� */
        /* ���������āC�{�t���[���ł́C2�x�Ɠ����L�����N�^�́C�`�悳��Ȃ� */

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
  /* �X�v���C�g��`�� */
  DrawSprites();
}

/*-------------------------------------------------------------------*/
/*  ��`�̈�����[�N��ʂɓ]��                                       */
/*-------------------------------------------------------------------*/
void BltFast( int x, int y, Byte *pbyBuf, RECT_T *rect )
{
  register int rx, ry, wx, wy;

  for ( ry = rect->top, wy = y; ry < rect->bottom; ry++, wy++ ) 
  {
    for ( rx = rect->left, wx = x; rx < rect->right; rx++, wx++ )
    {
      /* �p���b�g���g�p���āC���[�N��ʂɓ]�� */
      WorkFrame[ ( wy << 8 ) + wx ] = PyutaPalette[ pbyBuf[ ( ry << 11 ) + rx ] ];
    }
  }
}

/*-------------------------------------------------------------------*/
/*  �ꎞ�o�b�t�@�ɓ_��`��                                           */
/*-------------------------------------------------------------------*/
void pixel(Byte* pbyBuf, int x, int y, int c)
{
  /* ������ */
  if ( c==0 )
    c=VDPREG[7]&0x0f;

  /* �o�b�t�@�ɓ_��`�� */
  pbyBuf[ ( y << 11 ) + x ] = c; 
}

/*-------------------------------------------------------------------*/
/*  �X�v���C�g�`��                                                   */
/*-------------------------------------------------------------------*/
void DrawSprites()
{
  RECT_T myrect;
	int i1, i3, xx, yy, pat, col, p_add, t;
  int highest;

	highest=31;

  /* �A�N�e�B�u�ŁC�D�揇�ʂ̍����X�v���C�g�����t���� */
	for (i1=0; i1<32; i1++)       /* 32�X�v���C�g */
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
		yy=VDP[SAL++]+1;            /* �X�v���C�gY���W, 0�s�ڂ�255�Ŏw�� */
		if (yy>255) yy=0;

		xx=VDP[SAL++];              /* �X�v���C�gX���W */

		pat=VDP[SAL++];             /* �X�v���C�g�p�^�[�� */
		if (VDPREG[1] & 0x2)
			pat=pat&0xfc;             /* 16x16�̎��ɂ́C�p�^�[���A�h���X��4�̔{�� */

		col=VDP[SAL]&0xf;           /* �X�v���C�g�J���[ */
	
		if (VDP[SAL++]&0x10)        /* �A���[�N���b�N */
			xx-=32;

		if (yy==0xd1)
			i1=32;                    /* 0xd0�̓X�v���C�g���X�g�̏I�� */

		if ((yy<192)&&(col))        /* �X�N���[�����ŁC�����F�łȂ��� */
		{	
      /* �o�b�t�@�𓧖��F�ŃN���A */
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

        /* 16x16�̎��ɂ́C3�L�����N�^�]���ɕ`�� */
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

		  /* �X�v���C�g�g��@�\ */
      myrect.top=0;
			myrect.left=0;
			myrect.right=myrect.left+16;
			myrect.bottom=myrect.top+16;

      if (VDPREG[1]&0x01)
			{	
        /* �g��X�v���C�g��\�� */
        BltFast3( yy, xx, SpriteBuffer, &myrect );
			}
			else
			{	
        /* �W���X�v���C�g��\�� */
        BltFast2( yy, xx, SpriteBuffer, &myrect );
			}
    }
	}
}

/*-------------------------------------------------------------------*/
/*  ��`�̈�����[�N��ʂɓ]���i�W���X�v���C�g�p�j                   */
/*-------------------------------------------------------------------*/
void BltFast2( int x, int y, Byte *pbyBuf, RECT_T *rect )
{
  register int rx, ry, wx, wy;

  for ( ry = rect->top, wy = y; ry < rect->bottom; ry++, wy++ ) 
  {
    for ( rx = rect->left, wx = x; rx < rect->right; rx++, wx++ )
    {
      /* �p���b�g���g�p���āC���[�N��ʂɓ]�� */
      if ( pbyBuf[ ( ry << 4 ) + rx ] )
      {
        WorkFrame[ ( wy << 8 ) + wx ] = PyutaPalette[ pbyBuf[ ( ry << 4 ) + rx ] ];
      }
    }
  }
}

/*-------------------------------------------------------------------*/
/*  �ꎞ�o�b�t�@�ɓ_��`��i�X�v���C�g�p�j                           */
/*-------------------------------------------------------------------*/
void pixel2(Byte* pbyBuf, int x, int y, int c)
{
  /* ������ */
  if ( c==0 )
    c=VDPREG[7]&0x0f;

  /* �o�b�t�@�ɓ_��`�� */
  pbyBuf[ ( y << 4 ) + x ] = c; 
}

/*-------------------------------------------------------------------*/
/*  ��`�̈�����[�N��ʂɓ]���i�g��X�v���C�g�p�j                   */
/*-------------------------------------------------------------------*/
void BltFast3( int x, int y, Byte *pbyBuf, RECT_T *rect )
{
  register int rx, ry, wx, wy;

  for ( ry = rect->top, wy = y; ry < rect->bottom; ry++, wy+=2 ) 
  {
    for ( rx = rect->left, wx = x; rx < rect->right; rx++, wx+=2 )
    {
      Byte col = pbyBuf[ ( ry << 4 ) + rx ];

      /* �p���b�g���g�p���āC���[�N��ʂɓ]�� */
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
