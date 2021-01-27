/*===================================================================*/
/*                                                                   */
/*  InfoPyuta.cpp : C++�t�@�C��(�҂イ���G�~���[���[�^)              */
/*                                                                   */
/*  2001/04/19    Jay's Factory                                      */
/*                                                                   */
/*===================================================================*/

/*-------------------------------------------------------------------*/
/*  �C���N���[�h�t�@�C��                                             */
/*-------------------------------------------------------------------*/

#include <stdio.h>
#include "cpu9900.h"
#include "InfoPyuta.h"
#include "InfoPyuta_System.h"
#include "InfoPyuta_Vdp.h"
#include "pg.h"

/*-------------------------------------------------------------------*/
/*  �҂イ�����\�[�X                                                 */
/*-------------------------------------------------------------------*/

Byte RAM[ 0x4000 ];             /* �O��RAM */
Byte ROM[ 0x5000 ];             /* �O��ROM */
Byte INTLRAM[ 0x100 ];          /* ����RAM */           
Byte CRU[ 0x1000 ];	            /* CRU��� (�s�v?) */

Word Scanline;                  /* �X�L�������C�� */

/*-------------------------------------------------------------------*/
/*  �҂イ���G�~�����[�V����                                         */
/*-------------------------------------------------------------------*/
int InfoPyuta_Main()
{
  /* �҂イ�������� */
  InfoPyuta_Reset();

  /* �҂イ���N�� */
  InfoPyuta_Cycle();

  return(1);
}

/*-------------------------------------------------------------------*/
/*  �҂イ��������                                                   */
/*-------------------------------------------------------------------*/
int InfoPyuta_Reset()
{
  /* �_�~�[���߃Z�b�g(����m�F�p) */
  ROM[ 0x0000 ] = 0xf0;                 /* WP:����RAM��ݒ� */
  ROM[ 0x0001 ] = 0x00;
  ROM[ 0x0002 ] = 0x40;                 /* PC:ROM�J�[�g���b�W��ݒ� */
  ROM[ 0x0003 ] = 0x00;

#if 0
  ROM[ 0x1000 ] = 0x04;
  ROM[ 0x1001 ] = 0x50;
  ROM[ 0x1002 ] = 0x40;
  ROM[ 0x1003 ] = 0x00;
  ROM[ 0x1004 ] = 0x04;
  ROM[ 0x1005 ] = 0x40;
#endif

  /* CPU���߃Z�b�g���� */
  buildcpu();

  /* CPU���Z�b�g */
  reset();

  /* VDP���Z�b�g */
  VDPreset();

  /* �X�L�������C���������� */
  Scanline = 0;

  return(0);
}

/*-------------------------------------------------------------------*/
/*  �҂イ���G�~�����[�V�����̐�����������                           */
/*-------------------------------------------------------------------*/
int InfoPyuta_HSync()
{
  /*-------------------------------------------------------------------*/
  /*  VDP�����݃s���`�F�b�N                                            */
  /*-------------------------------------------------------------------*/
  if ( INT_PIN )
  {
    /* VDP�����݃s�������Z�b�g�����܂ő҂� */
    return ( 0 );
  }

  /*-------------------------------------------------------------------*/
  /*  ���̃X�L�������C��                                               */
  /*-------------------------------------------------------------------*/
  Scanline = ( Scanline == VBLANK_END ) ? 0 : Scanline + 1;

  /*-------------------------------------------------------------------*/
  /*  �X�L�������C�����̏���                                           */
  /*-------------------------------------------------------------------*/
  switch ( Scanline )
  {
    case ON_SCREEN_START:
      /* �Ȃɂ����Ȃ� */      
      break;

    case VBLANK_START:
      /* �������A�t���O���Z�b�g */
      VDPST |= VDPST_INT;

      /* VDP�����݃s�����Z�b�g */
      INT_PIN = 0x01;

      /* ��ʐ��� */
      VDPgraphics();

      /* ��ʍX�V */
      InfoPyuta_LoadFrame();

      break;

    case VBLANK_END:
      /* �Ȃɂ����Ȃ� */
      break;
  }
  return( 0 );
}

/*-------------------------------------------------------------------*/
/*  �҂イ���G�~�����[�V�������[�v                                   */
/*-------------------------------------------------------------------*/
int InfoPyuta_Cycle()
{
  /* �G�~�����[�V�������[�v */
  for (;;)
  {
    /* �����������ɉ����ߎ��s���邩(�b��) */
    for ( int i = 0; i < INST_PER_HSYNC; i++ )
    {
      /* 1���ߎ��s */
      do1();
    }

    /* �����������̏��� */
    InfoPyuta_HSync();

    /* �E�F�C�g���� */
    InfoPyuta_Wait();

    /* ����m�F�p */
    char szMsg[ 256 ];
    sprintf( szMsg, "Opcode: 0x%x, PC: 0x%x, R1: 0x%x", in, PC, romword( 0xf002 ) );
#if 0
    warn( szMsg );
#endif
	sceCtrlPeekBufferPositive(&paddata, 1);
	if(paddata.buttons & CTRL_LTRIGGER) break;
	
	extern int bSleep;
	if(bSleep)
		while(bSleep) pgWaitV();
  }

  return(0);
}

/*-------------------------------------------------------------------*/
/*  �J�Z�b�g���҂イ���Ƀ��[�h                                       */
/*-------------------------------------------------------------------*/
int InfoPyuta_Load( const char *pszFileName )
{
  /* ROM�C���[�W���������ɓǂݍ��� */
  if ( InfoPyuta_ReadRom( pszFileName ) < 0 )
    return -1;

  /* InfoPyuta�����Z�b�g */
  if ( InfoPyuta_Reset() < 0 )
    return -1;

  /* ���� */
  return 0;
}

/*-------------------------------------------------------------------*/
/* ��z��̂҂イ���������}�b�v                                      */
/* >0000 - >0fff  �O��ROM(BIOS)                                      */
/* >1000 - >3fff  �s��                                               */
/* >4000 - >7fff  �O��ROM(G-BASIC , �J�[�g���b�W)                    */
/* >8000 - >8800  �s��                                               */
/* >8800 - >8fff  �������}�b�vI/O ( VDP )                            */
/* >9000 - >efff  �s��                                               */
/* >f000 - >f0fb  ����RAM                                            */
/* >f0fc - >fff9  �s��                                               */
/* >fffa - >fffb  �����������}�b�v�hI/O                              */
/* >fffc - >ffff  ����RAM(NMI�x�N�^)                                 */
/*-------------------------------------------------------------------*/

/*-------------------------------------------------------------------*/
/*  CPU�チ��������o�C�g��ǂݍ���                                  */
/*-------------------------------------------------------------------*/

Byte rcpubyte(Word x)
{
  switch ( x & 0xc000 )
  {
    /* �O��ROM(BIOS) */
    case 0x0000:
     
      if ( 0x0000 <= x && x <= 0x0fff )
      {
          return ROM[ x & 0x0fff ];
      }
      break;

    /* �O��ROM(G-BASIC�C�J�[�g���b�W) */
    case 0x4000:
      return ROM[ ( x & 0x3fff ) + 0x1000 ];

    /* �������}�b�vI/O ( VDP ) */
	  case 0x8000:
	  if ( 0x8800 <= x && x <= 0x8fff )
	  {
      /* VDP�f�[�^�ǂݍ��� */
		  return( rvdpbyte( x ) );
	  }
    break;

#if 0
    /* �O��RAM */
    case 0xa000:
      return RAM[ x & 0x3fff ];
#endif

    /* ����RAM */
    case 0xc000:
      if ( 0xf000 <= x && x <= 0xf0f9 )
      {
        return INTLRAM[ x & 0x00ff ];
      } 
      else if ( 0xfffc <= x && x <= 0xffff )
      {
        return INTLRAM[ x & 0x00ff ];
      }
      break;
  }

  /* �f�[�^�o�X��Ɏc���Ă���l */
  return 0x00;
}

/*-------------------------------------------------------------------*/
/*  CPU�チ�����Ƀo�C�g����������                                    */
/*-------------------------------------------------------------------*/
void wcpubyte(Word x, Byte c)
{
  switch ( x & 0xc000 )
  {
    /* �������}�b�vI/O ( VDP ) */ 
    case 0x8000:
      if ( 0x8800 <= x && x <= 0x8fff )
	    {  
        /* VDP�ɏ������� */
		    wvdpbyte(x,c);
	    }
      break;
    
#if 0
    /* �O��RAM */
    case 0xa000:
      RAM[ x & 0x3fff ] = c;
      break;
#endif

    /* ����RAM */
    case 0xc000:
      if ( 0xf000 <= x && x <= 0xf0f9 )
      {
        INTLRAM[ x & 0x00ff ] = c;
      } 
      else if ( 0xfffc <= x && x <= 0xffff )
      {
        INTLRAM[ x & 0x00ff ] = c;
      }
      break;
  }
}

/*-------------------------------------------------------------------*/
/*  CPU�チ�������烏�[�h��ǂݍ���                                  */
/*-------------------------------------------------------------------*/
Word romword(Word x)
{ 
	x&=0xfffe;		/* LSB�͗��Ƃ� */
	return((rcpubyte(x)<<8)+rcpubyte(x+1));
}

/*-------------------------------------------------------------------*/
/*  CPU�チ�����Ƀ��[�h����������                                    */
/*-------------------------------------------------------------------*/
void wrword(Word x, Word y)
{ 
	x&=0xfffe;		/* LSB�͗��Ƃ� */
	wcpubyte(x,(Byte)(y>>8));
	wcpubyte(x+1,(Byte)(y&0xff));
}

/*-------------------------------------------------------------------*/
/*  CRU����r�b�g��ǂݍ���                                          */
/*-------------------------------------------------------------------*/
int rcru(Word ad)
{
	int ret;                      /* �e���|���� */

	ad=(ad&0x0fff);               /* ���ۂ�CRU�����擾 */
	ret=1;												/* �f�t�H���g�̕Ԓl */

	return(ret);
}

/*-------------------------------------------------------------------*/
/*  CRU�Ƀr�b�g����������                                            */
/*-------------------------------------------------------------------*/
void wcru(Word ad, int bt)
{
	ad=(ad&0x0fff);										/* ���ۂ�CRU�����擾 */
	
	if (bt)												    /* �f�[�^���������� */
		CRU[ad]=1;
	else
		CRU[ad]=0;
}

/*-------------------------------------------------------------------*/
/*  VDP�`�b�v����ǂݍ���                                            */
/*-------------------------------------------------------------------*/
Byte rvdpbyte(Word x)
{ 
  Word z;

	if ( ( x >= 0x8c00 ) || ( x & 0x0001 ) ) 
	{
    /* �������ݗp�A�h���X */
    return(0);                        
	}

	if (x&0x0002)
	{
    /* �X�e�[�^�X�ǂݍ��݁C�N���A, �����݃N���A */
		z=VDPST;
		VDPST = 0;
    INT_PIN = 0x00;
		return((Byte)z);                  
	}
	else
	{
    /* �f�[�^�ǂݍ��� */
		z=VDP[VDPADD&0x3fff];           

    /* �����I�ɃC���N�������g */
	  VDPADD++;									      
		return ((Byte)z);
  }
}

/*-------------------------------------------------------------------*/
/*  VDP�`�b�v�ւ̏�������                                            */
/*-------------------------------------------------------------------*/
void wvdpbyte(Word x, Byte c)
{
  char szMsg[512];

	if ( (x < 0x8c00) || ( x & 0x0001 ) ) 
	{
    /* �ǂݍ��ݗp�A�h���X */
		return;				
	}

	if (x&0x0002)
	{
    /* VDP�������݃A�h���X���X�V */
		VDPADD=(VDPADD>>8)|(c<<8);

    /* �A�N�Z�X�J�E���g���C���N�������g */
		vdpaccess++;
		if (vdpaccess==2)
		{ 
			if (VDPADD&0x8000)
			{ 
        /* VDP���W�X�^�ւ̏������� */
				wVDPreg((Byte)((VDPADD&0x0700)>>8),(Byte)(VDPADD&0x00ff));
        /* �ĕ`��t���O���Z�b�g */
				redraw_needed=1;
			}
      /* �A�N�Z�X�J�E���g�����Z�b�g */
			vdpaccess=0;
		}
	}
	else
	{ 
    /* VDP RAM�ɏ������� */
		VDP[(VDPADD++)&0x3fff]=c;

    sprintf( szMsg, "VDPADD: 0x%x, c: 0x%x", ((VDPADD-1)&0x3fff), c);
#if 0
    warn( szMsg );
#endif

    /* �ĕ`��t���O���Z�b�g�D���������X�}�[�g�ȕ��@�͂Ȃ����Ȃ�? */
    redraw_needed=1;
	}
}

/*-------------------------------------------------------------------*/
/*  VDP���W�X�^�ɏ�������                                            */
/*-------------------------------------------------------------------*/
void wVDPreg(Byte r, Byte v)
{ 
	int t;                        /* �e���|���� */

	VDPREG[r]=v;                  /* ���W�X�^�ɃZ�b�g */

	if (r==7)
	{
		t=v&0xf;                    /* TODO: BG�F�ł���΁C�p���b�g���X�V */
#if 0
    TIPAL[0].r=red[t];
		TIPAL[0].g=green[t];
		TIPAL[0].b=blue[t];
#endif
  }
}