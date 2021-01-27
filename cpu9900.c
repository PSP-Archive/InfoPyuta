/*===================================================================*/
/*                                                                   */
/*  cpu9900.cpp : C++ファイル(TMS9995エミューレータ)                 */
/*                                                                   */
/*  2001/04/10    Jay's Factory                                      */
/*  1999/03/28    M.Brent ( Ami 99 - TI-99エミュレータ)              */
/*                                                                   */
/*===================================================================*/

/*-------------------------------------------------------------------*/
/*  インクルードファイル                                             */
/*-------------------------------------------------------------------*/

#include "cpu9900.h"
#include "InfoPyuta.h"
#include "InfoPyuta_System.h"

/*-------------------------------------------------------------------*/
/*  CPU大域変数                                                      */
/*-------------------------------------------------------------------*/

Word PC;                        /* プログラムカウンタ */
Word WP;                        /* ワークスペースカウンタ */
Word X_flag;                    /* 'X'命令時にセット */
Word ST;                        /* ステータスレジスタ */
Word in,D,S,Td,Ts,B;            /* オペコードインプリテーション */
void (*opcode[65536])(void);    /* CPUオペコードアドレステーブル */

/*-------------------------------------------------------------------*/
/*  ステータスレジスタ定義                                           */
/*-------------------------------------------------------------------*/

#define ST_LGT      (ST&0x8000)     /* 論理的大なり */ 
#define ST_AGT      (ST&0x4000)     /* 算術的大なり */
#define ST_EQ       (ST&0x2000)     /* 等価 */
#define ST_C        (ST&0x1000)     /* キャリー */
#define ST_OV       (ST&0x0800)     /* オーバーフロー */
#define ST_OP       (ST&0x0400)     /* 奇数パリティ */
#define ST_X        (ST&0x0200)     /* XOP命令時にセット */
#define ST_OVEN     (ST&0x0020)     /* オーバーフローイネーブル */
#define ST_INTMASK  (ST&0x000f)     /* 割り込みマスク*/

#define set_LGT   (ST=ST|0x8000)    /* フラグセット用 */
#define set_AGT   (ST=ST|0x4000)
#define set_EQ    (ST=ST|0x2000)
#define set_C     (ST=ST|0x1000)
#define set_OV    (ST=ST|0x0800)
#define set_OP    (ST=ST|0x0400)
#define set_X     (ST=ST|0x0200)
#define set_OVEN  (ST=ST|0x0020)

#define reset_LGT   (ST=ST&0x7fff)  /* フラグリセット用 */
#define reset_AGT   (ST=ST&0xbfff)
#define reset_EQ    (ST=ST&0xdfff)
#define reset_C     (ST=ST&0xefff)
#define reset_OV    (ST=ST&0xf7ff)
#define reset_OP    (ST=ST&0xfbff)
#define reset_X     (ST=ST&0xfdff)
#define reset_OVEN  (ST=ST&0xffdf)

#define reset_EQ_LGT (ST=ST&0x5fff)

/*-------------------------------------------------------------------*/
/*  ソース，ディスティネーション等，取得用                           */
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
/* ソース，ディスティネーションから実アドレスを取得                  */
/*-------------------------------------------------------------------*/

/*-------------------------------------------------------------------*/
/* 注: フォーマットコード文字は，TI社の公式表記です．詳細は，TMS9995 */
/*     のドキュメントを参照のこと（Td, Ts, D, S, B, 等）             */
/* 注: ディスティネーションアドレスの不要な処理をスキップするために，*/
/*     ディスティネーションタイプ(Td)に，'4'をセットするフォーマット */
/*     コードも存在します                                            */              
/*-------------------------------------------------------------------*/

/*-------------------------------------------------------------------*/
/* レジスタ               (R1)         レジスタのアドレス            */
/* レジスタインダイレクト (*R1)        レジスタの内容                */
/* インデックス           (@>1000(R1)) レジスタの内容＋引数の内容    */
/* シンボリック           (@>1000)     引数の内容                    */ 
/* オートインクリメント   (*R1+)       レジスタの内容，レジスタを自  */
/*                                     動的にインクリメント(1:バイト */
/*                                     ,2:ワード)                    */
/*-------------------------------------------------------------------*/

void fixDS()
{
  int temp,t2;                                    /* テンポラリ */

  switch (Ts)                                     /* ソースタイプ */
  { 
  case 0: S=WP+(S<<1); break;                     /* レジスタ */
  case 1: S=romword(WP+(S<<1)); break;            /* レジスタインダイレクト */
  case 2: 
    if (S)
      { S=romword(PC)+romword(WP+(S<<1)); PC+=2;} /* インデックス */
    else                                          
      { S=romword(PC); PC+=2; }                   /* シンボリック */ 
    break;
  case 3: t2=WP+(S<<1); temp=romword(t2); wrword(t2,temp+(2-B));  
      S=temp; break;                              /* オートインクリメント */
  }

  switch (Td)                                     /* ディスティネーションタイプ */
  {
  case 0: D=WP+(D<<1); break;                     /* レジスタ */
  case 1: D=romword(WP+(D<<1)); break;            /* レジスタインダイレクト */
  case 2: if (D)
      { D=romword(PC)+romword(WP+(D<<1)); PC+=2;} /* インデックス */
      else
      { D=romword(PC); PC+=2; }                   /* シンボリック */
      break;
  case 3: t2=WP+(D<<1); temp=romword(t2); wrword(t2,temp+(2-B));  
      D=temp; break;                              /* レジスタインダイレクトオートインクリメント */
  }
}

/*-------------------------------------------------------------------*/
/* パリティをチェックして，奇数パリティフラグをセット                */
/*-------------------------------------------------------------------*/
void parity(Byte x)
{
  int z,y;                            /* テンポラリ */

  z=0;
  for (y=0; y<8; y++)                 /* セットされたビット数 */
  { 
    if (x>127) z=z++;
    x=x<<1;
  }

  if (z&1)                            /* 奇数であればセット */
    set_OP; 
  else 
    reset_OP;
}

/*-------------------------------------------------------------------*/
/*  1命令を実行                                                      */
/*-------------------------------------------------------------------*/
void do1()
{
  if (X_flag==0)
  { 
    in=romword(PC);             /* 'X'命令以外 */
    PC=PC+2;
  }
  (*opcode[in])();              /* 実行すべき命令はinに含まれる */
}

/*-------------------------------------------------------------------*/
/*   CPUリセット                                                     */
/*-------------------------------------------------------------------*/
void reset()
{
	WP=romword(0);		/* >0000 はリセットベクタ - WPを読み込む */
	PC=romword(2);		/* さらにPCを読み込む */
  ST=0x0000;        /* ステータスレジスタをリセット */
  X_flag=0;         /* 現在，'X'命令は実行中でない */
}

/*-------------------------------------------------------------------*/
/* CPUオペコードアドレステーブル生成                                 */
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
/* 0b0000で始まるCPUオペコード                                       */
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
/* 0b00000010で始まるCPUオペコード                                   */
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
/* 0b00000011で始まるCPUオペコード                                   */
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
/* 0b00000100で始まるCPUオペコード                                   */
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
/* 0b00000101で始まるCPUオペコード                                   */
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
/* 0b00000110で始まるCPUオペコード                                   */
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
/* 0b00000111で始まるCPUオペコード                                   */
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
/* 0b0001で始まるCPUオペコード                                       */
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
/* 0b0010で始まるCPUオペコード                                       */
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
/* 0b0011で始まるCPUオペコード                                       */
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
/* TMS9995オペコード                                                 */
/* 各オペコードに各関数が対応                                        */
/* src - ソースアドレス (レジスタ，メモリ - 利用可能な型は異なる)    */
/* dst - ディスティネーションアドレス                                */
/* imm - イミディエート値                                            */
/* dsp - 相対ディスプレイスメント値                                  */
/*-------------------------------------------------------------------*/

/*-------------------------------------------------------------------*/
/* 加算(ワード単位): A src, dst                                      */
/*-------------------------------------------------------------------*/
void op_a()
{
  Word x1,x2,x3;

  FormatI;
  x1=romword(S); 
  x2=romword(D);

  x3=x2+x1; 
  wrword(D,(Word)x3);

  /* 当該処理は各オペコード共通 */
  reset_EQ_LGT;                                       /* EQ,LGTを最初にリセット */
  if (x3) set_LGT; else set_EQ;                       /* 非0であればLGTをセット，0であればEQをセット */
  if ((x3)&&(x3<0x8000)) set_AGT; else reset_AGT;     /* 非0，非負であれば，AGTをセット */
  if (x3<x2) set_C; else reset_C;                     /* キャリーをセット */
  if (((x1&0x8000)==(x2&0x8000))&&((x3&0x8000)!=(x2&0x8000))) set_OV; else reset_OV;
}

/*-------------------------------------------------------------------*/
/* 加算(バイト単位): AB src, dst                                     */
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
  if ((x3)&&(x3<0x80)) set_AGT; else reset_AGT;     /* バイト演算なので，小さな値でオーバーフロー */
  if (x3<x2) set_C; else reset_C;
  if (((x1&0x80)==(x2&0x80))&&((x3&0x80)!=(x2&0x80))) set_OV; else reset_OV;
  parity(x3);                                       /* 通常，バイト演算では，奇数パリティをチェック */
}

/*-------------------------------------------------------------------*/
/* 絶対値: ABS src                                                   */
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
/* 加算(イミディエート値): AI src, imm                               */
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
/* ディクリメント: DEC src                                           */
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
/* ディクリメント(ワード): DECT src                                  */
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
/* 除算: DIV src, dst                                                */
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
/* インクリメント: INC src                                           */
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
/* インクリメント(ワード): INCT src                                  */
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
/* 乗算: MPY src, dst                                                */
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
/* 補数: NEG src                                                     */
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
/* 減算: S src, dst                                                  */
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
/* 減算(バイト): SB src, dst                                         */
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
/* 絶対分岐(無条件): B src                                           */
/*-------------------------------------------------------------------*/
void op_b()
{ 
  FormatVI;
  PC=S;
}

/*-------------------------------------------------------------------*/
/* 絶対分岐，リンク: BL src                                          */
/*-------------------------------------------------------------------*/
void op_bl()
{
  /* 基本的にはサブルーチンジャンプ - リターンアドレスはR11に格納 */
  /* スタックは持っていないので，正式なリターン機能は存在しない */
  /* リターンは，単純に B *R11 で実装(RTと定義しているアセンブラもある) */

  FormatVI;
  wrword(WP+22,PC);
  PC=S;
}

/*-------------------------------------------------------------------*/
/* 絶対分岐，ワークスペースポインタ設定: BLWP src                    */
/*-------------------------------------------------------------------*/
void op_blwp()
{ 
  /* コンテキストスイッチ．オペランドのアドレスポインタは2ワード長 */
  /* 第1ワードは，新しいワークスペースポインタで，第2ワードは，*/
  /* 分岐すべきアドレス．現在のワークスペースポインタ，プログラム */
  /* カウンタ（リターンアドレス），ステータスレジスタは，それぞれ，*/
  /* R13, R14, R15に格納され，リターンはRTWPで実行される */

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
/* 相対分岐(等価): JEQ dsp                                           */
/*-------------------------------------------------------------------*/
void op_jeq()
{ 
  /* 条件付相対ジャンプ．ディスプレイスメントは，符号付バイトで表現 */

  char x1;

  FormatII;
  x1=(char)D;
  if (ST_EQ) 
  {
    PC+=x1+x1;
  }
}

/*-------------------------------------------------------------------*/
/* 相対分岐(算術的大なり): JGT dsp                                   */
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
/* 相対分岐(論理的以上): JHE dsp                                     */
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
/* 相対分岐(論理的大なり): JH dsp                                    */
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
/* 相対分岐(論理的小なり): JL dsp                                    */
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
/* 相対分岐(論理的以下): JLE dsp                                     */
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
/* 相対分岐(算術的小なり): JLT dsp                                   */
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
/* 相対分岐(無条件): JMP dsp                                         */
/*-------------------------------------------------------------------*/
void op_jmp()
{ 
  char x1;

  FormatII;
  x1=(char)D;
  PC+=x1+x1;
}

/*-------------------------------------------------------------------*/
/* 相対分岐(ノーキャリー): JNC dsp                                   */
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
/* 相対分岐(不一致): JNE dsp                                         */
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
/* 相対分岐(ノーオーバーフロー): JNO dsp                             */
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
/* 相対分岐(奇数パリティ): JOP dsp                                   */
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
/* 相対分岐(キャリー): JOC dsp                                       */
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
/* ワークスペースポインタ値による復帰: RTWP                          */
/*-------------------------------------------------------------------*/
void op_rtwp()
{ 
  /* BLWPに対する復帰に対応．BLWPの記述を参照 */

  ST=romword(WP+30);
  PC=romword(WP+28);
  WP=romword(WP+26);
}

/*-------------------------------------------------------------------*/
/* 実行: X src                                                       */
/*-------------------------------------------------------------------*/
void op_x()
{ 
  /* 引数をインストラクションとして解釈して実行 */
  /* 正しく動作するか不明，かつ，頻繁に使われるかも不明 */

  if (X_flag!=0) 
  {
    warn("Recursive X instruction!!!!!");       /* 再帰は不可 */
  }
  
  FormatVI;
  in=romword(S);
  X_flag=1;                           /* フラグセット */
  do1();                              /* 命令実行 */
  X_flag=0;                           /* フラグクリア */
}

/*-------------------------------------------------------------------*/
/* 拡張命令: XOP src ???                                             */
/*-------------------------------------------------------------------*/
void op_xop()
{ 
  /* CPUは，各命令でジャンプするジャンプテーブル（0x0040から開始，*/
  /* BLWP形式）を管理．さらに，新しいR11には，???がコピーされる */
  /* すべてのゲーム機（CPU?）が，この機能をサポートしているわけでない */
  /* たぶん，まれだと思われるが，ぴゅう太では利用されている(不明) */

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

  /* ST7,ST8,ST9,ST10,ST11をリセット */
  ST=ST&0xfe0f;
}

/*-------------------------------------------------------------------*/
/* 比較(ワード): C src, dst                                          */
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
/* 比較(バイト): CB src, dst                                         */
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
/* 比較(イミディエート値): CI src, imm                               */
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
/* 比較(ビットマスク後): COC src, dst                                */
/*-------------------------------------------------------------------*/
void op_coc()
{ 
  /* srcで指定されるマスクを掛けた結果，srcとdstを比較 */

  Word x1,x2,x3;

  FormatIII;
  x1=romword(S);
  x2=romword(D);
  
  x3=x1&x2;
  
  if (x3==x1) set_EQ; else reset_EQ;
}

/*-------------------------------------------------------------------*/
/* ゼロ検査(ビットマスク後): CZC src, dst                            */
/*-------------------------------------------------------------------*/
void op_czc()
{
  /* COCの反対．srcで指定されたマスクを掛けた結果，dstと0を比較 */

  Word x1,x2,x3;

  FormatIII;
  x1=romword(S);
  x2=romword(D);
  
  x3=x1&x2;
  
  if (x3==0) set_EQ; else reset_EQ;
}

/*-------------------------------------------------------------------*/
/* CRUロード: LDCR src, dst                                          */
/*-------------------------------------------------------------------*/
void op_ldcr()
{
  /* CRUレジスタにdstで指定されるビットをシリアルに書き込む */
  /* CRUとは，9901通信用チップで，9900と密接に関連している */
  /* これはシリアルにアクセスされ，4096バイトのI/Oレジスタを保持 */
  /* 0が真で，1が偽（馬鹿っぽいが）*/
  /* アドレスは，R12に格納された値（を2で割った値）からのオフセットで指定 */

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
/* CRUビットセット: SBO src                                          */
/*-------------------------------------------------------------------*/
void op_sbo()
{
  /* CRUにビットをセット */

  char x1;

  FormatII;
  x1=(char)D;
  wcru((romword(WP+24)>>1)+x1,1);
}

/*-------------------------------------------------------------------*/
/* CRUビットリセット: SBZ src                                        */
/*-------------------------------------------------------------------*/
void op_sbz()
{ 
  /* CRUにビットをリセット */

  char x1;

  FormatII;
  x1=(char)D;
  wcru((romword(WP+24)>>1)+x1,0);
}

/*-------------------------------------------------------------------*/
/* CRUストア: STCR src, dst                                          */
/*-------------------------------------------------------------------*/
void op_stcr()
{ 
  /* dstビットをCRUからsrcに格納 */

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
/* CRUテストビット: TB src                                           */
/*-------------------------------------------------------------------*/
void op_tb()
{ 
  /* CRUビットをテスト */

  char x1;

  FormatII;
  x1=(char)D;

  if (rcru((romword(WP+24)>>1)+x1)) set_EQ; else reset_EQ;
}

/* 以下の命令は，9995としては正しい命令だが，ぴゅう太において */
/* 利用されているかは不明 */

/*-------------------------------------------------------------------*/
/* ユーザ定義: CKOF                                                  */
/*-------------------------------------------------------------------*/
void op_ckof()
{ 
  warn("ClocK OFf instruction encountered!");  
}

/*-------------------------------------------------------------------*/
/* ユーザ定義: CKON                                                  */
/*-------------------------------------------------------------------*/
void op_ckon()
{ 
  warn("ClocK ON instruction encountered!");    
}

/*-------------------------------------------------------------------*/
/* アイドル: IDLE                                                    */
/*-------------------------------------------------------------------*/
void op_idle()
{
  warn("IDLE instruction encountered!"); 
}

/*-------------------------------------------------------------------*/
/* リセット: RESET                                                   */
/*-------------------------------------------------------------------*/
void op_rset()
{
  warn("ReSET instruction encountered!");
}

/*-------------------------------------------------------------------*/
/* ユーザ定義: LREX                                                  */
/*-------------------------------------------------------------------*/
void op_lrex()
{
  warn("Load or REstart eXecution instruction encountered!");
}

/*-------------------------------------------------------------------*/
/* ロード(イミディエート値): LI src, imm                             */
/*-------------------------------------------------------------------*/
void op_li()
{
  /* イミディエート値をロード */

  FormatVIII_1;
  wrword(D,S);
  
  reset_EQ_LGT;
  if (S) set_LGT; else set_EQ;
  if ((S)&&(S<0x8000)) set_AGT; else reset_AGT;
}

/*-------------------------------------------------------------------*/
/* ロード(イミディエート割込みマスク): LIMI imm                      */
/*-------------------------------------------------------------------*/
void op_limi()
{ 
  /* CPUの割込みマスクをセット */

  FormatVIII_1;
  ST=(ST&0xfff0)|(S&0xf);
}

/*-------------------------------------------------------------------*/
/* ロード(イミディエートWP): LWPI imm                                */
/*-------------------------------------------------------------------*/
void op_lwpi()
{ 
  /* ワークスペースポインタを設定 */

  FormatVIII_1;
  WP=S;
}

/*-------------------------------------------------------------------*/
/* ムーブ(ワード): MOV src, dst                                      */
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
/* ムーブ(バイト): MOVB src, dst                                     */
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
/* ストア(ステータス): STST src                                     */
/*-------------------------------------------------------------------*/
void op_stst()
{ 
  /* ステータスレジスタをメモリにコピー */

  FormatVIII_0;
  wrword(D,ST);
}

/*-------------------------------------------------------------------*/
/* ストア(ワークスペースポインタ): STWP src                          */
/*-------------------------------------------------------------------*/
void op_stwp()
{ 
  /* メモリからワークスペースポインタをコピー */

  FormatVIII_0;
  wrword(D,WP);
}

/*-------------------------------------------------------------------*/
/* 交換(バイト): SWPB src                                            */
/*-------------------------------------------------------------------*/
void op_swpb()
{ 
  /* ワード内の上位バイトと下位バイトを交換 */

  Word x1,x2;

  FormatVI;
  x1=romword(S);

  x2=((x1&0xff)<<8)|((x1&0xff00)>>8);
  wrword(S,x2);
}

/*-------------------------------------------------------------------*/
/* 論理積(イミディエート): ANDI src, imm                             */
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
/* 論理和(イミディエート): ORI src, imm                              */
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
/* 排他的論理和: XOR src, dst                                        */
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
/* ビット反転: INV src                                               */
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
/* ビットクリア: CLR src                                             */
/*-------------------------------------------------------------------*/
void op_clr()
{
  /* ワードを0にセット */

  FormatVI;
  wrword(S,0);
}

/*-------------------------------------------------------------------*/
/* ビットセット: SETO src                                            */
/*-------------------------------------------------------------------*/
void op_seto()
{ 
  /* ワードを0xffffにセット */

  FormatVI;
  wrword(S,0xffff);
}

/*-------------------------------------------------------------------*/
/* 任意ビットセット: SOC src, dst                                    */
/*-------------------------------------------------------------------*/
void op_soc()
{ 
  /* 基本的にORを実行 - srcでセットされているビットを */
  /* srcにおいてセット */

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
/* 任意ビットセット(バイト): SOCB src, dst                           */
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
/* 任意ビットクリア: SZC src, dst                                    */
/*-------------------------------------------------------------------*/
void op_szc()
{ 
  /* srcにおいてリセットされているビットをdstにおいてリセット */

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
/* 任意ビットクリア: SZCB src, dst                                   */
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
/* 算術的右シフト: SRA src, dst                                      */
/*-------------------------------------------------------------------*/
void op_sra()
{ 
  /* シフト命令においては，'0'はレジスタ0の値を利用することを */
  /* 意味する．算術的な命令では，符号ビットは保存される */

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
      x3=x1&1;   /* キャリーを保存 */
      x1=x1>>1;  /* 1ビットシフト */
      x1=x1|x4;  /* 符号ビットを拡張 */
  }
  wrword(S,x1);
  
  if (x3) set_C; else reset_C;
  reset_EQ_LGT;
  if (x1) set_LGT; else set_EQ;
  if ((x1)&&(x1<0x8000)) set_AGT; else reset_AGT;
}

/*-------------------------------------------------------------------*/
/* 論理的右シフト: SRL src, dst                                      */
/*-------------------------------------------------------------------*/
void op_srl()
{ 
  /* 論理シフトは，符号を保存しない */

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
/* 算術的左シフト: SLA src, dst                                      */
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
/* 循環的右シフト: SRC src, dst                                      */
/*-------------------------------------------------------------------*/
void op_src()
{ 
  /* 循環シフトは，最後のビットが先頭にポップ */
  /* ここでは，キャリービットは含まない */

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
/* 未定義命令:                                                       */
/*-------------------------------------------------------------------*/
void op_bad()
{ 
  warn("Illegal opcode!");       
}
