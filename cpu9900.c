/*===================================================================*/
/*                                                                   */
/*  cpu9900.cpp : C++�t�@�C��(TMS9995�G�~���[���[�^)                 */
/*                                                                   */
/*  2001/04/10    Jay's Factory                                      */
/*  1999/03/28    M.Brent ( Ami 99 - TI-99�G�~�����[�^)              */
/*                                                                   */
/*===================================================================*/

/*-------------------------------------------------------------------*/
/*  �C���N���[�h�t�@�C��                                             */
/*-------------------------------------------------------------------*/

#include "cpu9900.h"
#include "InfoPyuta.h"
#include "InfoPyuta_System.h"

/*-------------------------------------------------------------------*/
/*  CPU���ϐ�                                                      */
/*-------------------------------------------------------------------*/

Word PC;                        /* �v���O�����J�E���^ */
Word WP;                        /* ���[�N�X�y�[�X�J�E���^ */
Word X_flag;                    /* 'X'���ߎ��ɃZ�b�g */
Word ST;                        /* �X�e�[�^�X���W�X�^ */
Word in,D,S,Td,Ts,B;            /* �I�y�R�[�h�C���v���e�[�V���� */
void (*opcode[65536])(void);    /* CPU�I�y�R�[�h�A�h���X�e�[�u�� */

/*-------------------------------------------------------------------*/
/*  �X�e�[�^�X���W�X�^��`                                           */
/*-------------------------------------------------------------------*/

#define ST_LGT      (ST&0x8000)     /* �_���I��Ȃ� */ 
#define ST_AGT      (ST&0x4000)     /* �Z�p�I��Ȃ� */
#define ST_EQ       (ST&0x2000)     /* ���� */
#define ST_C        (ST&0x1000)     /* �L�����[ */
#define ST_OV       (ST&0x0800)     /* �I�[�o�[�t���[ */
#define ST_OP       (ST&0x0400)     /* ��p���e�B */
#define ST_X        (ST&0x0200)     /* XOP���ߎ��ɃZ�b�g */
#define ST_OVEN     (ST&0x0020)     /* �I�[�o�[�t���[�C�l�[�u�� */
#define ST_INTMASK  (ST&0x000f)     /* ���荞�݃}�X�N*/

#define set_LGT   (ST=ST|0x8000)    /* �t���O�Z�b�g�p */
#define set_AGT   (ST=ST|0x4000)
#define set_EQ    (ST=ST|0x2000)
#define set_C     (ST=ST|0x1000)
#define set_OV    (ST=ST|0x0800)
#define set_OP    (ST=ST|0x0400)
#define set_X     (ST=ST|0x0200)
#define set_OVEN  (ST=ST|0x0020)

#define reset_LGT   (ST=ST&0x7fff)  /* �t���O���Z�b�g�p */
#define reset_AGT   (ST=ST&0xbfff)
#define reset_EQ    (ST=ST&0xdfff)
#define reset_C     (ST=ST&0xefff)
#define reset_OV    (ST=ST&0xf7ff)
#define reset_OP    (ST=ST&0xfbff)
#define reset_X     (ST=ST&0xfdff)
#define reset_OVEN  (ST=ST&0xffdf)

#define reset_EQ_LGT (ST=ST&0x5fff)

/*-------------------------------------------------------------------*/
/*  �\�[�X�C�f�B�X�e�B�l�[�V�������C�擾�p                           */
/*-------------------------------------------------------------------*/

#define FormatI { Td=(in&0x0c00)>>10; Ts=(in&0x0030)>>4; D=(in&0x03c0)>>6; S=(in&0x000f); B=(in&0x1000)>>12; fixDS(); }
#define FormatII { D=(in&0x00ff); }
#define FormatIII { Td=0; Ts=(in&0x0030)>>4; D=(in&0x03c0)>>6; S=(in&0x000f); B=0; fixDS(); }
#define FormatIV { Td=4; D=0; Ts=(in&0x0030)>>4; S=(in&0x000f); B=0; fixDS(); D=(in&0x03c0)>>6; } 
#define FormatV { D=(in&0x00f0)>>4; S=(in&0x000f); S=WP+(S<<1); }
#define FormatVI { Td=4; D=0; Ts=(in&0x0030)>>4; S=in&0x000f; fixDS(); }  
#define FormatVII {}
#define FormatVIII_0 { D=(in&0x000f); D=WP+(D<<1); }
#define FormatVIII_1 { D=(in&0x000f); D=WP+(D<<1); S=romword(PC); PC=PC+2; }
#define FormatIX  { D=0; Td=4; Ts=(in&0x0030)>>4; S=(in&0x000f); fixDS(); D=(in&0x03c0)>>6; }

/*-------------------------------------------------------------------*/
/* �\�[�X�C�f�B�X�e�B�l�[�V����������A�h���X���擾                  */
/*-------------------------------------------------------------------*/

/*-------------------------------------------------------------------*/
/* ��: �t�H�[�}�b�g�R�[�h�����́CTI�Ђ̌����\�L�ł��D�ڍׂ́CTMS9995 */
/*     �̃h�L�������g���Q�Ƃ̂��ƁiTd, Ts, D, S, B, ���j             */
/* ��: �f�B�X�e�B�l�[�V�����A�h���X�̕s�v�ȏ������X�L�b�v���邽�߂ɁC*/
/*     �f�B�X�e�B�l�[�V�����^�C�v(Td)�ɁC'4'���Z�b�g����t�H�[�}�b�g */
/*     �R�[�h�����݂��܂�                                            */              
/*-------------------------------------------------------------------*/

/*-------------------------------------------------------------------*/
/* ���W�X�^               (R1)         ���W�X�^�̃A�h���X            */
/* ���W�X�^�C���_�C���N�g (*R1)        ���W�X�^�̓��e                */
/* �C���f�b�N�X           (@>1000(R1)) ���W�X�^�̓��e�{�����̓��e    */
/* �V���{���b�N           (@>1000)     �����̓��e                    */ 
/* �I�[�g�C���N�������g   (*R1+)       ���W�X�^�̓��e�C���W�X�^����  */
/*                                     ���I�ɃC���N�������g(1:�o�C�g */
/*                                     ,2:���[�h)                    */
/*-------------------------------------------------------------------*/

void fixDS()
{
  int temp,t2;                                    /* �e���|���� */

  switch (Ts)                                     /* �\�[�X�^�C�v */
  { 
  case 0: S=WP+(S<<1); break;                     /* ���W�X�^ */
  case 1: S=romword(WP+(S<<1)); break;            /* ���W�X�^�C���_�C���N�g */
  case 2: 
    if (S)
      { S=romword(PC)+romword(WP+(S<<1)); PC+=2;} /* �C���f�b�N�X */
    else                                          
      { S=romword(PC); PC+=2; }                   /* �V���{���b�N */ 
    break;
  case 3: t2=WP+(S<<1); temp=romword(t2); wrword(t2,temp+(2-B));  
      S=temp; break;                              /* �I�[�g�C���N�������g */
  }

  switch (Td)                                     /* �f�B�X�e�B�l�[�V�����^�C�v */
  {
  case 0: D=WP+(D<<1); break;                     /* ���W�X�^ */
  case 1: D=romword(WP+(D<<1)); break;            /* ���W�X�^�C���_�C���N�g */
  case 2: if (D)
      { D=romword(PC)+romword(WP+(D<<1)); PC+=2;} /* �C���f�b�N�X */
      else
      { D=romword(PC); PC+=2; }                   /* �V���{���b�N */
      break;
  case 3: t2=WP+(D<<1); temp=romword(t2); wrword(t2,temp+(2-B));  
      D=temp; break;                              /* ���W�X�^�C���_�C���N�g�I�[�g�C���N�������g */
  }
}

/*-------------------------------------------------------------------*/
/* �p���e�B���`�F�b�N���āC��p���e�B�t���O���Z�b�g                */
/*-------------------------------------------------------------------*/
void parity(Byte x)
{
  int z,y;                            /* �e���|���� */

  z=0;
  for (y=0; y<8; y++)                 /* �Z�b�g���ꂽ�r�b�g�� */
  { 
    if (x>127) z=z++;
    x=x<<1;
  }

  if (z&1)                            /* ��ł���΃Z�b�g */
    set_OP; 
  else 
    reset_OP;
}

/*-------------------------------------------------------------------*/
/*  1���߂����s                                                      */
/*-------------------------------------------------------------------*/
void do1()
{
  if (X_flag==0)
  { 
    in=romword(PC);             /* 'X'���߈ȊO */
    PC=PC+2;
  }
  (*opcode[in])();              /* ���s���ׂ����߂�in�Ɋ܂܂�� */
}

/*-------------------------------------------------------------------*/
/*   CPU���Z�b�g                                                     */
/*-------------------------------------------------------------------*/
void reset()
{
	WP=romword(0);		/* >0000 �̓��Z�b�g�x�N�^ - WP��ǂݍ��� */
	PC=romword(2);		/* �����PC��ǂݍ��� */
  ST=0x0000;        /* �X�e�[�^�X���W�X�^�����Z�b�g */
  X_flag=0;         /* ���݁C'X'���߂͎��s���łȂ� */
}

/*-------------------------------------------------------------------*/
/* CPU�I�y�R�[�h�A�h���X�e�[�u������                                 */
/*-------------------------------------------------------------------*/
void buildcpu()
{
  Word in,x;
  unsigned int i;

  for (i=0; i<65536; i++)
  { 
    in=(Word)i;

    x=(in&0xf000)>>12;
    switch(x)
    { 
    case 0: opcode0(in);    break;
    case 1: opcode1(in);    break;
    case 2: opcode2(in);    break;
    case 3: opcode3(in);    break;
    case 4: opcode[in]=op_szc;  break;
    case 5: opcode[in]=op_szcb; break;
    case 6: opcode[in]=op_s;  break;
    case 7: opcode[in]=op_sb;  break;
    case 8: opcode[in]=op_c;  break;
    case 9: opcode[in]=op_cb;  break;
    case 10:opcode[in]=op_a;  break;
    case 11:opcode[in]=op_ab;  break;
    case 12:opcode[in]=op_mov;  break;
    case 13:opcode[in]=op_movb; break;
    case 14:opcode[in]=op_soc;  break;
    case 15:opcode[in]=op_socb; break;
    default: opcode[in]=op_bad;
    }
  } 
}

/*-------------------------------------------------------------------*/
/* 0b0000�Ŏn�܂�CPU�I�y�R�[�h                                       */
/*-------------------------------------------------------------------*/
void opcode0(Word in)
{
  unsigned short x;

  x=(in&0x0f00)>>8;

  switch(x)
  { 
  case 2: opcode02(in);    break;
  case 3: opcode03(in);    break;
  case 4: opcode04(in);    break;
  case 5: opcode05(in);    break;
  case 6: opcode06(in);    break;
  case 7: opcode07(in);    break;
  case 8: opcode[in]=op_sra;  break;
  case 9: opcode[in]=op_srl;  break;
  case 10:opcode[in]=op_sla;  break;
  case 11:opcode[in]=op_src;  break;
  default: opcode[in]=op_bad;
  }
}

/*-------------------------------------------------------------------*/
/* 0b00000010�Ŏn�܂�CPU�I�y�R�[�h                                   */
/*-------------------------------------------------------------------*/
void opcode02(Word in)
{ 
  unsigned short x;

  x=(in&0x00e0)>>4;

  switch(x)
  { 
  case 0: opcode[in]=op_li;  break;
  case 2: opcode[in]=op_ai;  break;
  case 4: opcode[in]=op_andi; break;
  case 6: opcode[in]=op_ori;  break;
  case 8: opcode[in]=op_ci;  break;
  case 10:opcode[in]=op_stwp; break;
  case 12:opcode[in]=op_stst; break;
  case 14:opcode[in]=op_lwpi; break;
  default: opcode[in]=op_bad;
  }
}

/*-------------------------------------------------------------------*/
/* 0b00000011�Ŏn�܂�CPU�I�y�R�[�h                                   */
/*-------------------------------------------------------------------*/
void opcode03(Word in)
{ 
  unsigned short x;

  x=(in&0x00e0)>>4;

  switch(x)
  { 
  case 0: opcode[in]=op_limi; break;
  case 4: opcode[in]=op_idle; break;
  case 6: opcode[in]=op_rset; break;
  case 8: opcode[in]=op_rtwp; break;
  case 10:opcode[in]=op_ckon; break;
  case 12:opcode[in]=op_ckof; break;
  case 14:opcode[in]=op_lrex; break;
  default: opcode[in]=op_bad;
  }
}

/*-------------------------------------------------------------------*/
/* 0b00000100�Ŏn�܂�CPU�I�y�R�[�h                                   */
/*-------------------------------------------------------------------*/
void opcode04(Word in)
{ 
  unsigned short x;

  x=(in&0x00c0)>>4;

  switch(x)
  { 
  case 0: opcode[in]=op_blwp; break;
  case 4: opcode[in]=op_b;  break;
  case 8: opcode[in]=op_x;  break;
  case 12:opcode[in]=op_clr;  break;
  default: opcode[in]=op_bad;
  }
}

/*-------------------------------------------------------------------*/
/* 0b00000101�Ŏn�܂�CPU�I�y�R�[�h                                   */
/*-------------------------------------------------------------------*/
void opcode05(Word in)
{ 
  unsigned short x;

  x=(in&0x00c0)>>4;

  switch(x)
  { 
  case 0: opcode[in]=op_neg;  break;
  case 4: opcode[in]=op_inv;  break;
  case 8: opcode[in]=op_inc;  break;
  case 12:opcode[in]=op_inct; break;
  default: opcode[in]=op_bad;
  }
}

/*-------------------------------------------------------------------*/
/* 0b00000110�Ŏn�܂�CPU�I�y�R�[�h                                   */
/*-------------------------------------------------------------------*/
void opcode06(Word in)
{ 
  unsigned short x;

  x=(in&0x00c0)>>4;

  switch(x)
  { 
  case 0: opcode[in]=op_dec;  break;
  case 4: opcode[in]=op_dect; break;
  case 8: opcode[in]=op_bl;  break;
  case 12:opcode[in]=op_swpb; break;
  default: opcode[in]=op_bad;
  }
}

/*-------------------------------------------------------------------*/
/* 0b00000111�Ŏn�܂�CPU�I�y�R�[�h                                   */
/*-------------------------------------------------------------------*/
void opcode07(Word in)
{ 
  unsigned short x;

  x=(in&0x00c0)>>4;

  switch(x)
  { 
  case 0: opcode[in]=op_seto; break;
  case 4: opcode[in]=op_abs;  break;
  default: opcode[in]=op_bad;
  }  
}

/*-------------------------------------------------------------------*/
/* 0b0001�Ŏn�܂�CPU�I�y�R�[�h                                       */
/*-------------------------------------------------------------------*/
void opcode1(Word in)
{ 
  unsigned short x;

  x=(in&0x0f00)>>8;

  switch(x)
  { 
  case 0: opcode[in]=op_jmp;  break;
  case 1: opcode[in]=op_jlt;  break;
  case 2: opcode[in]=op_jle;  break;
  case 3: opcode[in]=op_jeq;  break;
  case 4: opcode[in]=op_jhe;  break;
  case 5: opcode[in]=op_jgt;  break;
  case 6: opcode[in]=op_jne;  break;
  case 7: opcode[in]=op_jnc;  break;
  case 8: opcode[in]=op_joc;  break;
  case 9: opcode[in]=op_jno;  break;
  case 10:opcode[in]=op_jl;  break;
  case 11:opcode[in]=op_jh;  break;
  case 12:opcode[in]=op_jop;  break;
  case 13:opcode[in]=op_sbo;  break;
  case 14:opcode[in]=op_sbz;  break;
  case 15:opcode[in]=op_tb;  break;
  default: opcode[in]=op_bad;
  }
}

/*-------------------------------------------------------------------*/
/* 0b0010�Ŏn�܂�CPU�I�y�R�[�h                                       */
/*-------------------------------------------------------------------*/
void opcode2(Word in)
{ 
  unsigned short x;

  x=(in&0x0c00)>>8;

  switch(x)
  { 
  case 0: opcode[in]=op_coc; break;
  case 4: opcode[in]=op_czc; break;
  case 8: opcode[in]=op_xor; break;
  case 12:opcode[in]=op_xop; break;
  default: opcode[in]=op_bad;
  }
}

/*-------------------------------------------------------------------*/
/* 0b0011�Ŏn�܂�CPU�I�y�R�[�h                                       */
/*-------------------------------------------------------------------*/
void opcode3(Word in)
{ 
  unsigned short x;

  x=(in&0x0c00)>>8;

  switch(x)
  { 
  case 0: opcode[in]=op_ldcr; break;
  case 4: opcode[in]=op_stcr; break;
  case 8: opcode[in]=op_mpy;  break;
  case 12:opcode[in]=op_div;  break;
  default: opcode[in]=op_bad;
  }
}

/*-------------------------------------------------------------------*/
/* TMS9995�I�y�R�[�h                                                 */
/* �e�I�y�R�[�h�Ɋe�֐����Ή�                                        */
/* src - �\�[�X�A�h���X (���W�X�^�C������ - ���p�\�Ȍ^�͈قȂ�)    */
/* dst - �f�B�X�e�B�l�[�V�����A�h���X                                */
/* imm - �C�~�f�B�G�[�g�l                                            */
/* dsp - ���΃f�B�X�v���C�X�����g�l                                  */
/*-------------------------------------------------------------------*/

/*-------------------------------------------------------------------*/
/* ���Z(���[�h�P��): A src, dst                                      */
/*-------------------------------------------------------------------*/
void op_a()
{
  Word x1,x2,x3;

  FormatI;
  x1=romword(S); 
  x2=romword(D);

  x3=x2+x1; 
  wrword(D,(Word)x3);

  /* ���Y�����͊e�I�y�R�[�h���� */
  reset_EQ_LGT;                                       /* EQ,LGT���ŏ��Ƀ��Z�b�g */
  if (x3) set_LGT; else set_EQ;                       /* ��0�ł����LGT���Z�b�g�C0�ł����EQ���Z�b�g */
  if ((x3)&&(x3<0x8000)) set_AGT; else reset_AGT;     /* ��0�C�񕉂ł���΁CAGT���Z�b�g */
  if (x3<x2) set_C; else reset_C;                     /* �L�����[���Z�b�g */
  if (((x1&0x8000)==(x2&0x8000))&&((x3&0x8000)!=(x2&0x8000))) set_OV; else reset_OV;
}

/*-------------------------------------------------------------------*/
/* ���Z(�o�C�g�P��): AB src, dst                                     */
/*-------------------------------------------------------------------*/
void op_ab()
{ 
  Byte x1,x2,x3;

  FormatI;
  x1=rcpubyte(S); 
  x2=rcpubyte(D);

  x3=x2+x1;
  wcpubyte(D,x3);
  
  reset_EQ_LGT;
  if (x3) set_LGT; else set_EQ;
  if ((x3)&&(x3<0x80)) set_AGT; else reset_AGT;     /* �o�C�g���Z�Ȃ̂ŁC�����Ȓl�ŃI�[�o�[�t���[ */
  if (x3<x2) set_C; else reset_C;
  if (((x1&0x80)==(x2&0x80))&&((x3&0x80)!=(x2&0x80))) set_OV; else reset_OV;
  parity(x3);                                       /* �ʏ�C�o�C�g���Z�ł́C��p���e�B���`�F�b�N */
}

/*-------------------------------------------------------------------*/
/* ��Βl: ABS src                                                   */
/*-------------------------------------------------------------------*/
void op_abs()
{ 
  Word x1,x2;

  FormatVI;
  x1=x2=romword(S);

  if (x1&0x8000) x2=~x1+1;   
  wrword(S,x2);

  reset_EQ_LGT;
  if (x1) set_LGT; else set_EQ;                     
  if ((x1)&&(x1<0x8000)) set_AGT; else reset_AGT;
  if (x1==0x8000) set_OV; else reset_OV;                   
}

/*-------------------------------------------------------------------*/
/* ���Z(�C�~�f�B�G�[�g�l): AI src, imm                               */
/*-------------------------------------------------------------------*/
void op_ai()
{ 
  Word x1,x3;

  FormatVIII_1;
  x1=romword(D);

  x3=x1+S;
  wrword(D,x3);

  reset_EQ_LGT;
  if (x3) set_LGT; else set_EQ;
  if ((x3)&&(x3<0x8000)) set_AGT; else reset_AGT;
  if (x3<x1) set_C; else reset_C;
  if (((x1&0x8000)==(S&0x8000))&&((x3&0x8000)!=(S&0x8000))) set_OV; else reset_OV;
}

/*-------------------------------------------------------------------*/
/* �f�B�N�������g: DEC src                                           */
/*-------------------------------------------------------------------*/
void op_dec()
{ 
  Word x1;

  FormatVI;
  x1=romword(S);

  x1--;
  wrword(S,x1);

  reset_EQ_LGT;
  if (x1) set_LGT; else set_EQ;
  if ((x1)&&(x1<0x8000)) set_AGT; else reset_AGT;
  if (x1!=0xffff) set_C; else reset_C;
  if (x1==0x7fff) set_OV; else reset_OV;
}

/*-------------------------------------------------------------------*/
/* �f�B�N�������g(���[�h): DECT src                                  */
/*-------------------------------------------------------------------*/
void op_dect()
{ 
  Word x1;

  FormatVI;
  x1=romword(S);

  x1-=2;
  wrword(S,x1);
  
  reset_EQ_LGT;
  if (x1) set_LGT; else set_EQ;
  if ((x1)&&(x1<0x8000)) set_AGT; else reset_AGT;
  if (x1<0xfffe) set_C; else reset_C;
  if ((x1==0x7fff)||(x1=0x7ffe)) set_OV; else reset_OV;
}

/*-------------------------------------------------------------------*/
/* ���Z: DIV src, dst                                                */
/*-------------------------------------------------------------------*/
void op_div()
{ 
  Word x1,x2; 
  unsigned long x3;

  FormatIX;
  x2=romword(S);
  D=WP+(D<<1);
  x3=romword(D);

  if (x2>x3)           
  { 
    x3=(x3<<16)+romword(D+2);
    x1=(Word)x3/x2;
    wrword(D,x1);
    x1=(Word)x3%x2;
    wrword(D+2,x1);
    reset_OV;
  }
  else
  {
    set_OV;      
  }
}

/*-------------------------------------------------------------------*/
/* �C���N�������g: INC src                                           */
/*-------------------------------------------------------------------*/
void op_inc()
{ 
  Word x1;

  FormatVI;
  x1=romword(S);
  
  x1++;
  wrword(S,x1);
  
  ST=ST&0x4fff;                        
  if (x1) set_LGT; else { set_EQ; set_C; }
  if ((x1)&&(x1<0x8000)) set_AGT; else reset_AGT;
  if (x1==0x8000) set_OV; else reset_OV;
}

/*-------------------------------------------------------------------*/
/* �C���N�������g(���[�h): INCT src                                  */
/*-------------------------------------------------------------------*/
void op_inct()
{ 
  Word x1;

  FormatVI;
  x1=romword(S);
  
  x1+=2;
  wrword(S,x1);
  
  reset_EQ_LGT;
  if (x1) set_LGT; else set_EQ;
  if ((x1)&&(x1<0x8000)) set_AGT; else reset_AGT;
  if (x1<2) set_C; else reset_C;
  if ((x1==0x8000)||(x1==0x8001)) set_OV; else reset_OV;
}

/*-------------------------------------------------------------------*/
/* ��Z: MPY src, dst                                                */
/*-------------------------------------------------------------------*/
void op_mpy()
{ 
  Word x1; 
  unsigned long x3;

  FormatIX;
  x1=romword(S);
  
  D=WP+(D<<1);
  x3=romword(D);
  x3=x3*x1;
  wrword(D,(Word)(x3>>16)); 
  wrword(D+2,(Word)(x3&0xffff));
}

/*-------------------------------------------------------------------*/
/* �␔: NEG src                                                     */
/*-------------------------------------------------------------------*/
void op_neg()
{ 
  Word x1,x2;

  FormatVI;
  x1=x2=romword(S);

  x1=~x1+1;
  wrword(S,x1);

  reset_EQ_LGT;
  if (x1) set_LGT; else set_EQ;
  if ((x1)&&(x1<0x8000)) set_AGT; else reset_AGT;
  if (x2==0x8000) set_OV; else reset_OV;
}

/*-------------------------------------------------------------------*/
/* ���Z: S src, dst                                                  */
/*-------------------------------------------------------------------*/
void op_s()
{ 
  Word x1,x2,x3;

  FormatI;
  x1=romword(S); 
  x2=romword(D);

  x3=x2-x1;
  wrword(D,x3);

  reset_EQ_LGT;
  if (x3) set_LGT; else set_EQ;
  if ((x3)&&(x3<0x8000)) set_AGT; else reset_AGT;
  if (x3<=x2) set_C; else reset_C;
  if (((x1&0x8000)!=(x2&0x8000))&&((x3&0x8000)!=(x2&0x8000))) set_OV; else reset_OV;
}

/*-------------------------------------------------------------------*/
/* ���Z(�o�C�g): SB src, dst                                         */
/*-------------------------------------------------------------------*/
void op_sb()
{ 
  Byte x1,x2,x3;

  FormatI;
  x1=rcpubyte(S); 
  x2=rcpubyte(D);

  x3=x2-x1;
  wcpubyte(D,x3);

  reset_EQ_LGT;
  if (x3) set_LGT; else set_EQ;
  if ((x3)&&(x3<0x80)) set_AGT; else reset_AGT;
  if (x3<=x2) set_C; else reset_C;
  if (((x1&0x80)!=(x2&0x80))&&((x3&0x80)!=(x2&0x80))) set_OV; else reset_OV;
  parity(x3);
}

/*-------------------------------------------------------------------*/
/* ��Ε���(������): B src                                           */
/*-------------------------------------------------------------------*/
void op_b()
{ 
  FormatVI;
  PC=S;
}

/*-------------------------------------------------------------------*/
/* ��Ε���C�����N: BL src                                          */
/*-------------------------------------------------------------------*/
void op_bl()
{
  /* ��{�I�ɂ̓T�u���[�`���W�����v - ���^�[���A�h���X��R11�Ɋi�[ */
  /* �X�^�b�N�͎����Ă��Ȃ��̂ŁC�����ȃ��^�[���@�\�͑��݂��Ȃ� */
  /* ���^�[���́C�P���� B *R11 �Ŏ���(RT�ƒ�`���Ă���A�Z���u��������) */

  FormatVI;
  wrword(WP+22,PC);
  PC=S;
}

/*-------------------------------------------------------------------*/
/* ��Ε���C���[�N�X�y�[�X�|�C���^�ݒ�: BLWP src                    */
/*-------------------------------------------------------------------*/
void op_blwp()
{ 
  /* �R���e�L�X�g�X�C�b�`�D�I�y�����h�̃A�h���X�|�C���^��2���[�h�� */
  /* ��1���[�h�́C�V�������[�N�X�y�[�X�|�C���^�ŁC��2���[�h�́C*/
  /* ���򂷂ׂ��A�h���X�D���݂̃��[�N�X�y�[�X�|�C���^�C�v���O���� */
  /* �J�E���^�i���^�[���A�h���X�j�C�X�e�[�^�X���W�X�^�́C���ꂼ��C*/
  /* R13, R14, R15�Ɋi�[����C���^�[����RTWP�Ŏ��s����� */

  Word x1;

  FormatVI;
  x1=WP;
  WP=romword(S);
  wrword(WP+26,x1);
  wrword(WP+28,PC);
  wrword(WP+30,ST);
  PC=romword(S+2);
}

/*-------------------------------------------------------------------*/
/* ���Ε���(����): JEQ dsp                                           */
/*-------------------------------------------------------------------*/
void op_jeq()
{ 
  /* �����t���΃W�����v�D�f�B�X�v���C�X�����g�́C�����t�o�C�g�ŕ\�� */

  char x1;

  FormatII;
  x1=(char)D;
  if (ST_EQ) 
  {
    PC+=x1+x1;
  }
}

/*-------------------------------------------------------------------*/
/* ���Ε���(�Z�p�I��Ȃ�): JGT dsp                                   */
/*-------------------------------------------------------------------*/
void op_jgt()
{ 
  char x1; 

  FormatII;
  x1=(char)D;
  if (ST_AGT) 
  {
    PC+=x1+x1;
  }
}

/*-------------------------------------------------------------------*/
/* ���Ε���(�_���I�ȏ�): JHE dsp                                     */
/*-------------------------------------------------------------------*/
void op_jhe()
{ 
  char x1;

  FormatII;
  x1=(char)D;
  if ((ST_LGT)||(ST_EQ)) 
  {
    PC+=x1+x1;
  }
}

/*-------------------------------------------------------------------*/
/* ���Ε���(�_���I��Ȃ�): JH dsp                                    */
/*-------------------------------------------------------------------*/
void op_jh()
{ 
  char x1;

  FormatII;
  x1=(char)D;
  if ((ST_LGT)&&(!ST_EQ)) 
  {
    PC+=x1+x1;
  }
}

/*-------------------------------------------------------------------*/
/* ���Ε���(�_���I���Ȃ�): JL dsp                                    */
/*-------------------------------------------------------------------*/
void op_jl()
{
  char x1;

    FormatII;
  x1=(char)D;
  if ((!ST_LGT)&&(!ST_EQ)) 
  {
    PC+=x1+x1;
  }
}

/*-------------------------------------------------------------------*/
/* ���Ε���(�_���I�ȉ�): JLE dsp                                     */
/*-------------------------------------------------------------------*/
void op_jle()
{ 
  char x1;

  FormatII;
  x1=(char)D;
  if ((!ST_LGT)||(ST_EQ)) 
  {
    PC+=x1+x1;
  }
}

/*-------------------------------------------------------------------*/
/* ���Ε���(�Z�p�I���Ȃ�): JLT dsp                                   */
/*-------------------------------------------------------------------*/
void op_jlt()
{ 
  char x1;

  FormatII;
  x1=(char)D;
  if ((!ST_AGT)&&(!ST_EQ)) 
  {
    PC+=x1+x1;
  }
}

/*-------------------------------------------------------------------*/
/* ���Ε���(������): JMP dsp                                         */
/*-------------------------------------------------------------------*/
void op_jmp()
{ 
  char x1;

  FormatII;
  x1=(char)D;
  PC+=x1+x1;
}

/*-------------------------------------------------------------------*/
/* ���Ε���(�m�[�L�����[): JNC dsp                                   */
/*-------------------------------------------------------------------*/
void op_jnc()
{ 
  char x1;

  FormatII;
  x1=(char)D;
  if (!ST_C) 
  {
    PC+=x1+x1;
  }
}

/*-------------------------------------------------------------------*/
/* ���Ε���(�s��v): JNE dsp                                         */
/*-------------------------------------------------------------------*/
void op_jne()
{ 
  char x1;

  FormatII;
  x1=(char)D;
  if (!ST_EQ) 
  {
    PC+=x1+x1;
  }
}

/*-------------------------------------------------------------------*/
/* ���Ε���(�m�[�I�[�o�[�t���[): JNO dsp                             */
/*-------------------------------------------------------------------*/
void op_jno()
{ 
  char x1;

  FormatII;
  x1=(char)D;
  if (!ST_OV) 
  {
    PC+=x1+x1;
  }
}

/*-------------------------------------------------------------------*/
/* ���Ε���(��p���e�B): JOP dsp                                   */
/*-------------------------------------------------------------------*/
void op_jop()
{ 
  char x1;

  FormatII;
  x1=(char)D;
  if (ST_OP) 
  {
    PC+=x1+x1;
  }
}

/*-------------------------------------------------------------------*/
/* ���Ε���(�L�����[): JOC dsp                                       */
/*-------------------------------------------------------------------*/
void op_joc()
{ 
  char x1;

  FormatII;
  x1=(char)D;
  if (ST_C) 
  {
    PC+=x1+x1;
  }
}

/*-------------------------------------------------------------------*/
/* ���[�N�X�y�[�X�|�C���^�l�ɂ�镜�A: RTWP                          */
/*-------------------------------------------------------------------*/
void op_rtwp()
{ 
  /* BLWP�ɑ΂��镜�A�ɑΉ��DBLWP�̋L�q���Q�� */

  ST=romword(WP+30);
  PC=romword(WP+28);
  WP=romword(WP+26);
}

/*-------------------------------------------------------------------*/
/* ���s: X src                                                       */
/*-------------------------------------------------------------------*/
void op_x()
{ 
  /* �������C���X�g���N�V�����Ƃ��ĉ��߂��Ď��s */
  /* ���������삷�邩�s���C���C�p�ɂɎg���邩���s�� */

  if (X_flag!=0) 
  {
    warn("Recursive X instruction!!!!!");       /* �ċA�͕s�� */
  }
  
  FormatVI;
  in=romword(S);
  X_flag=1;                           /* �t���O�Z�b�g */
  do1();                              /* ���ߎ��s */
  X_flag=0;                           /* �t���O�N���A */
}

/*-------------------------------------------------------------------*/
/* �g������: XOP src ???                                             */
/*-------------------------------------------------------------------*/
void op_xop()
{ 
  /* CPU�́C�e���߂ŃW�����v����W�����v�e�[�u���i0x0040����J�n�C*/
  /* BLWP�`���j���Ǘ��D����ɁC�V����R11�ɂ́C???���R�s�[����� */
  /* ���ׂẴQ�[���@�iCPU?�j���C���̋@�\���T�|�[�g���Ă���킯�łȂ� */
  /* ���Ԃ�C�܂ꂾ�Ǝv���邪�C�҂イ���ł͗��p����Ă���(�s��) */

  Word x1;

#if 0
  debug_write("Executing XOP");
#else 
  warn("Executing XOP");
#endif

  FormatIX;
  x1=WP;
  set_X;
  WP=romword(0x0040+(D<<2));
  wrword(WP+22,S);
  wrword(WP+26,x1);
  wrword(WP+28,PC);
  wrword(WP+30,ST);
  PC=romword(0x0042+(D<<2));

  /* ST7,ST8,ST9,ST10,ST11�����Z�b�g */
  ST=ST&0xfe0f;
}

/*-------------------------------------------------------------------*/
/* ��r(���[�h): C src, dst                                          */
/*-------------------------------------------------------------------*/
void op_c()
{ 
  short x1,x2; 
  Word x3,x4;

  FormatI;
  x3=romword(S); 
  x1=x3;
  x4=romword(D); 
  x2=x4;
  
  if (x3>x4) set_LGT; else reset_LGT;
  if (x1>x2) set_AGT; else reset_AGT;
  if (x3==x4) set_EQ; else reset_EQ;
}

/*-------------------------------------------------------------------*/
/* ��r(�o�C�g): CB src, dst                                         */
/*-------------------------------------------------------------------*/
void op_cb()
{ 
  char x1,x2; 
  Byte x3,x4;

  FormatI;
  x3=rcpubyte(S); 
  x1=x3;
  x4=rcpubyte(D); 
  x2=x4;
  
  if (x3>x4) set_LGT; else reset_LGT;
  if (x1>x2) set_AGT; else reset_AGT;
  if (x3==x4) set_EQ; else reset_EQ;
  parity(x3);
}

/*-------------------------------------------------------------------*/
/* ��r(�C�~�f�B�G�[�g�l): CI src, imm                               */
/*-------------------------------------------------------------------*/
void op_ci()
{ 
  short x1,x2; 
  Word x3;

  FormatVIII_1;
  x3=romword(D); 
  x1=x3;
  x2=S;
  
  if (x3>S) set_LGT; else reset_LGT;
  if (x1>x2) set_AGT; else reset_AGT;
  if (x3==S) set_EQ; else reset_EQ;
}

/*-------------------------------------------------------------------*/
/* ��r(�r�b�g�}�X�N��): COC src, dst                                */
/*-------------------------------------------------------------------*/
void op_coc()
{ 
  /* src�Ŏw�肳���}�X�N���|�������ʁCsrc��dst���r */

  Word x1,x2,x3;

  FormatIII;
  x1=romword(S);
  x2=romword(D);
  
  x3=x1&x2;
  
  if (x3==x1) set_EQ; else reset_EQ;
}

/*-------------------------------------------------------------------*/
/* �[������(�r�b�g�}�X�N��): CZC src, dst                            */
/*-------------------------------------------------------------------*/
void op_czc()
{
  /* COC�̔��΁Dsrc�Ŏw�肳�ꂽ�}�X�N���|�������ʁCdst��0���r */

  Word x1,x2,x3;

  FormatIII;
  x1=romword(S);
  x2=romword(D);
  
  x3=x1&x2;
  
  if (x3==0) set_EQ; else reset_EQ;
}

/*-------------------------------------------------------------------*/
/* CRU���[�h: LDCR src, dst                                          */
/*-------------------------------------------------------------------*/
void op_ldcr()
{
  /* CRU���W�X�^��dst�Ŏw�肳���r�b�g���V���A���ɏ������� */
  /* CRU�Ƃ́C9901�ʐM�p�`�b�v�ŁC9900�Ɩ��ڂɊ֘A���Ă��� */
  /* ����̓V���A���ɃA�N�Z�X����C4096�o�C�g��I/O���W�X�^��ێ� */
  /* 0���^�ŁC1���U�i�n�����ۂ����j*/
  /* �A�h���X�́CR12�Ɋi�[���ꂽ�l�i��2�Ŋ������l�j����̃I�t�Z�b�g�Ŏw�� */

  Word x1,x3; 
  int x2;

  FormatIV;
  if (D==0) D=16;
  if ((S&1)&&(D>8)) S-=1;
  x1=(D<9 ? rcpubyte(S) : romword(S));
  
  x3=1;
  for (x2=0; x2<D; x2++)
  { 
    wcru((romword(WP+24)>>1)+x2, (x1&x3) ? 1 : 0);
    x3=x3<<1;
  }
  
  reset_EQ_LGT;
  if (x1) set_LGT; else set_EQ;
  if ((x1)&&(x1<(D<9 ? 0x80 : 0x8000))) set_AGT; else reset_AGT;
  if (D<9) parity(x1&0xff);
}

/*-------------------------------------------------------------------*/
/* CRU�r�b�g�Z�b�g: SBO src                                          */
/*-------------------------------------------------------------------*/
void op_sbo()
{
  /* CRU�Ƀr�b�g���Z�b�g */

  char x1;

  FormatII;
  x1=(char)D;
  wcru((romword(WP+24)>>1)+x1,1);
}

/*-------------------------------------------------------------------*/
/* CRU�r�b�g���Z�b�g: SBZ src                                        */
/*-------------------------------------------------------------------*/
void op_sbz()
{ 
  /* CRU�Ƀr�b�g�����Z�b�g */

  char x1;

  FormatII;
  x1=(char)D;
  wcru((romword(WP+24)>>1)+x1,0);
}

/*-------------------------------------------------------------------*/
/* CRU�X�g�A: STCR src, dst                                          */
/*-------------------------------------------------------------------*/
void op_stcr()
{ 
  /* dst�r�b�g��CRU����src�Ɋi�[ */

  Word x1,x3,x4; 
  int x2;

  FormatIV;
  if (D==0) D=16;
  if ((S&1)&&(D>8)) S=S-1;
  x1=0; x3=1;
  
  for (x2=0; x2<D; x2++)
  { 
    x4=rcru((romword(WP+24)>>1)+x2);
    if (x4) 
    {
      x1=x1|x3;
    }
    x3=x3<<1;
  }

  if (D<9) 
  {
    wcpubyte(S,(Byte)x1);  
  }
  else 
  {
    wrword(S,x1);
  }

  reset_EQ_LGT;
  if (x1) set_LGT; else set_EQ;
  if ((x1)&&(x1<(D<9 ? 0x80 : 0x8000))) set_AGT; else reset_AGT;
  if (D<9) parity((x1&0xff00)>>8);
}

/*-------------------------------------------------------------------*/
/* CRU�e�X�g�r�b�g: TB src                                           */
/*-------------------------------------------------------------------*/
void op_tb()
{ 
  /* CRU�r�b�g���e�X�g */

  char x1;

  FormatII;
  x1=(char)D;

  if (rcru((romword(WP+24)>>1)+x1)) set_EQ; else reset_EQ;
}

/* �ȉ��̖��߂́C9995�Ƃ��Ă͐��������߂����C�҂イ���ɂ����� */
/* ���p����Ă��邩�͕s�� */

/*-------------------------------------------------------------------*/
/* ���[�U��`: CKOF                                                  */
/*-------------------------------------------------------------------*/
void op_ckof()
{ 
  warn("ClocK OFf instruction encountered!");  
}

/*-------------------------------------------------------------------*/
/* ���[�U��`: CKON                                                  */
/*-------------------------------------------------------------------*/
void op_ckon()
{ 
  warn("ClocK ON instruction encountered!");    
}

/*-------------------------------------------------------------------*/
/* �A�C�h��: IDLE                                                    */
/*-------------------------------------------------------------------*/
void op_idle()
{
  warn("IDLE instruction encountered!"); 
}

/*-------------------------------------------------------------------*/
/* ���Z�b�g: RESET                                                   */
/*-------------------------------------------------------------------*/
void op_rset()
{
  warn("ReSET instruction encountered!");
}

/*-------------------------------------------------------------------*/
/* ���[�U��`: LREX                                                  */
/*-------------------------------------------------------------------*/
void op_lrex()
{
  warn("Load or REstart eXecution instruction encountered!");
}

/*-------------------------------------------------------------------*/
/* ���[�h(�C�~�f�B�G�[�g�l): LI src, imm                             */
/*-------------------------------------------------------------------*/
void op_li()
{
  /* �C�~�f�B�G�[�g�l�����[�h */

  FormatVIII_1;
  wrword(D,S);
  
  reset_EQ_LGT;
  if (S) set_LGT; else set_EQ;
  if ((S)&&(S<0x8000)) set_AGT; else reset_AGT;
}

/*-------------------------------------------------------------------*/
/* ���[�h(�C�~�f�B�G�[�g�����݃}�X�N): LIMI imm                      */
/*-------------------------------------------------------------------*/
void op_limi()
{ 
  /* CPU�̊����݃}�X�N���Z�b�g */

  FormatVIII_1;
  ST=(ST&0xfff0)|(S&0xf);
}

/*-------------------------------------------------------------------*/
/* ���[�h(�C�~�f�B�G�[�gWP): LWPI imm                                */
/*-------------------------------------------------------------------*/
void op_lwpi()
{ 
  /* ���[�N�X�y�[�X�|�C���^��ݒ� */

  FormatVIII_1;
  WP=S;
}

/*-------------------------------------------------------------------*/
/* ���[�u(���[�h): MOV src, dst                                      */
/*-------------------------------------------------------------------*/
void op_mov()
{ 
  Word x1;

  FormatI;
  x1=romword(S);
  
  wrword(D,x1);
  
  reset_EQ_LGT;
  if (x1) set_LGT; else set_EQ;
  if ((x1)&&(x1<0x8000)) set_AGT; else reset_AGT;
}

/*-------------------------------------------------------------------*/
/* ���[�u(�o�C�g): MOVB src, dst                                     */
/*-------------------------------------------------------------------*/
void op_movb()
{ 
  Byte x1;

  FormatI;
  x1=rcpubyte(S);
  
  wcpubyte(D,x1);
  
  reset_EQ_LGT;
  if (x1) set_LGT; else set_EQ;
  if ((x1)&&(x1<0x80)) set_AGT; else reset_AGT;
  parity(x1);
}

/*-------------------------------------------------------------------*/
/* �X�g�A(�X�e�[�^�X): STST src                                     */
/*-------------------------------------------------------------------*/
void op_stst()
{ 
  /* �X�e�[�^�X���W�X�^���������ɃR�s�[ */

  FormatVIII_0;
  wrword(D,ST);
}

/*-------------------------------------------------------------------*/
/* �X�g�A(���[�N�X�y�[�X�|�C���^): STWP src                          */
/*-------------------------------------------------------------------*/
void op_stwp()
{ 
  /* ���������烏�[�N�X�y�[�X�|�C���^���R�s�[ */

  FormatVIII_0;
  wrword(D,WP);
}

/*-------------------------------------------------------------------*/
/* ����(�o�C�g): SWPB src                                            */
/*-------------------------------------------------------------------*/
void op_swpb()
{ 
  /* ���[�h���̏�ʃo�C�g�Ɖ��ʃo�C�g������ */

  Word x1,x2;

  FormatVI;
  x1=romword(S);

  x2=((x1&0xff)<<8)|((x1&0xff00)>>8);
  wrword(S,x2);
}

/*-------------------------------------------------------------------*/
/* �_����(�C�~�f�B�G�[�g): ANDI src, imm                             */
/*-------------------------------------------------------------------*/
void op_andi()
{ 
  Word x1,x2;

  FormatVIII_1;
  x1=romword(D);
  x2=x1&S;
  
  wrword(D,x2);
  
  reset_EQ_LGT;
  if (x2) set_LGT; else set_EQ;
  if ((x2)&&(x2<0x8000)) set_AGT; else reset_AGT;
}

/*-------------------------------------------------------------------*/
/* �_���a(�C�~�f�B�G�[�g): ORI src, imm                              */
/*-------------------------------------------------------------------*/
void op_ori()
{ 
  Word x1,x2;

  FormatVIII_1;
  x1=romword(D);
  
  x2=x1|S;
  wrword(D,x2);
  
  reset_EQ_LGT;
  if (x2) set_LGT; else set_EQ;
  if ((x2)&&(x2<0x8000)) set_AGT; else reset_AGT;
}

/*-------------------------------------------------------------------*/
/* �r���I�_���a: XOR src, dst                                        */
/*-------------------------------------------------------------------*/
void op_xor()
{ 
  Word x1,x2,x3;

  FormatIII;
  x1=romword(S);
  x2=romword(D);
  
  x3=x1^x2;
  wrword(D,x3);
  
  reset_EQ_LGT;
  if (x3) set_LGT; else set_EQ;
  if ((x3)&&(x3<0x8000)) set_AGT; else reset_AGT;
}

/*-------------------------------------------------------------------*/
/* �r�b�g���]: INV src                                               */
/*-------------------------------------------------------------------*/
void op_inv()
{ 
  Word x1;

  FormatVI;
  x1=romword(S);
  
  x1=~x1;
  wrword(S,x1);
  
  reset_EQ_LGT;
  if (x1) set_LGT; else set_EQ;
  if ((x1)&&(x1<0x8000)) set_AGT; else reset_AGT;
}

/*-------------------------------------------------------------------*/
/* �r�b�g�N���A: CLR src                                             */
/*-------------------------------------------------------------------*/
void op_clr()
{
  /* ���[�h��0�ɃZ�b�g */

  FormatVI;
  wrword(S,0);
}

/*-------------------------------------------------------------------*/
/* �r�b�g�Z�b�g: SETO src                                            */
/*-------------------------------------------------------------------*/
void op_seto()
{ 
  /* ���[�h��0xffff�ɃZ�b�g */

  FormatVI;
  wrword(S,0xffff);
}

/*-------------------------------------------------------------------*/
/* �C�Ӄr�b�g�Z�b�g: SOC src, dst                                    */
/*-------------------------------------------------------------------*/
void op_soc()
{ 
  /* ��{�I��OR�����s - src�ŃZ�b�g����Ă���r�b�g�� */
  /* src�ɂ����ăZ�b�g */

  Word x1,x2,x3;

  FormatI;
  x1=romword(S);
  x2=romword(D);
  
  x3=x1|x2;
  wrword(D,x3);
  
  reset_EQ_LGT;
  if (x3) set_LGT; else set_EQ;
  if ((x3)&&(x3<0x8000)) set_AGT; else reset_AGT;
}

/*-------------------------------------------------------------------*/
/* �C�Ӄr�b�g�Z�b�g(�o�C�g): SOCB src, dst                           */
/*-------------------------------------------------------------------*/
void op_socb()
{ 
  Byte x1,x2,x3;

  FormatI;
  x1=rcpubyte(S);
  x2=rcpubyte(D);
  
  x3=x1|x2;
    wcpubyte(D,x3);

  reset_EQ_LGT;
  if (x3) set_LGT; else set_EQ;
  if ((x3)&&(x3<0x80)) set_AGT; else reset_AGT;
  parity(x3);
}

/*-------------------------------------------------------------------*/
/* �C�Ӄr�b�g�N���A: SZC src, dst                                    */
/*-------------------------------------------------------------------*/
void op_szc()
{ 
  /* src�ɂ����ă��Z�b�g����Ă���r�b�g��dst�ɂ����ă��Z�b�g */

  Word x1,x2,x3;

  FormatI;
  x1=romword(S);
  x2=romword(D);
  
  x3=(~x1)&x2;
  wrword(D,x3);
  
  reset_EQ_LGT;
  if (x3) set_LGT; else set_EQ;
  if ((x3)&&(x3<0x8000)) set_AGT; else reset_AGT;
}

/*-------------------------------------------------------------------*/
/* �C�Ӄr�b�g�N���A: SZCB src, dst                                   */
/*-------------------------------------------------------------------*/
void op_szcb()
{ 
  Byte x1,x2,x3;

  FormatI;
  x1=rcpubyte(S);
  x2=rcpubyte(D);
  
  x3=(~x1)&x2;
  wcpubyte(D,x3);

  reset_EQ_LGT;
  if (x3) set_LGT; else set_EQ;
  if ((x3)&&(x3<0x80)) set_AGT; else reset_AGT;
  parity(x3);
}

/*-------------------------------------------------------------------*/
/* �Z�p�I�E�V�t�g: SRA src, dst                                      */
/*-------------------------------------------------------------------*/
void op_sra()
{ 
  /* �V�t�g���߂ɂ����ẮC'0'�̓��W�X�^0�̒l�𗘗p���邱�Ƃ� */
  /* �Ӗ�����D�Z�p�I�Ȗ��߂ł́C�����r�b�g�͕ۑ������ */

  Word x1,x3,x4; 
  int x2;

  FormatV;
  if (D==0)
  { 
    D=romword(WP) & 0xf;
    if (D==0) D=16;
  }
  x1=romword(S);
  x4=x1&0x8000;
  
  for (x2=0; x2<D; x2++)
  { 
      x3=x1&1;   /* �L�����[��ۑ� */
      x1=x1>>1;  /* 1�r�b�g�V�t�g */
      x1=x1|x4;  /* �����r�b�g���g�� */
  }
  wrword(S,x1);
  
  if (x3) set_C; else reset_C;
  reset_EQ_LGT;
  if (x1) set_LGT; else set_EQ;
  if ((x1)&&(x1<0x8000)) set_AGT; else reset_AGT;
}

/*-------------------------------------------------------------------*/
/* �_���I�E�V�t�g: SRL src, dst                                      */
/*-------------------------------------------------------------------*/
void op_srl()
{ 
  /* �_���V�t�g�́C������ۑ����Ȃ� */

  Word x1,x3; 
  int x2;

  FormatV;
  if (D==0)
  { 
    D=romword(WP)&0xf;
    if (D==0) D=16;
  }
  x1=romword(S);
  
  for (x2=0; x2<D; x2++)
  { 
    x3=x1&1;
      x1=x1>>1;
  }
  wrword(S,x1);

  if (x3) set_C; else reset_C;
  reset_EQ_LGT;
  if (x1) set_LGT; else set_EQ;
  if ((x1)&&(x1<0x8000)) set_AGT; else reset_AGT;
}

/*-------------------------------------------------------------------*/
/* �Z�p�I���V�t�g: SLA src, dst                                      */
/*-------------------------------------------------------------------*/
void op_sla()
{ 
  Word x1,x3,x4; 
  int x2;

  FormatV;
  if (D==0)
  { 
    D=romword(WP)&0xf;
    if (D==0) D=16;
  }
  x1=romword(S);
  x4=x1&0x8000;
  
  for (x2=0; x2<D; x2++)
  { 
    x3=x1&0x8000;
    x1=x1<<1;
  }
  wrword(S,x1);
  
  if (x3) set_C; else reset_C;
  if (x4!=(x1&0x8000)) set_OV; else reset_OV;
  reset_EQ_LGT;
  if (x1) set_LGT; else set_EQ;
  if ((x1)&&(x1<0x8000)) set_AGT; else reset_AGT;
}

/*-------------------------------------------------------------------*/
/* �z�I�E�V�t�g: SRC src, dst                                      */
/*-------------------------------------------------------------------*/
void op_src()
{ 
  /* �z�V�t�g�́C�Ō�̃r�b�g���擪�Ƀ|�b�v */
  /* �����ł́C�L�����[�r�b�g�͊܂܂Ȃ� */

  Word x1,x3,x4; 
  int x2;

  FormatV;
  if (D==0)
  { 
    D=romword(WP)&0xf;
    if (D==0) D=16;
  }
  x1=romword(S);
  for (x2=0; x2<D; x2++)
  { 
    x3=x1&0x8000;
    x4=x1&0x1;
    x1=x1>>1;
    if (x4) 
    {
      x1=x1|0x8000;
    }
  }
  wrword(S,x1);
  
  if (x3) set_C; else reset_C;
  reset_EQ_LGT;
  if (x1) set_LGT; else set_EQ;
  if ((x1)&&(x1<0x8000)) set_AGT; else reset_AGT;
}

/*-------------------------------------------------------------------*/
/* ����`����:                                                       */
/*-------------------------------------------------------------------*/
void op_bad()
{ 
  warn("Illegal opcode!");       
}
