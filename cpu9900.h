/*===================================================================*/
/*                                                                   */
/*  cpu9900.h : ヘッダファイル(TMS9995エミューレータ)                */
/*                                                                   */
/*  2001/04/10    Jay's Factory                                      */
/*  1999/03/28    M.Brent ( Ami 99 - TI-99エミュレータ  )            */
/*                                                                   */
/*===================================================================*/

#ifndef CPU9995_H_INCLUDED
#define CPU9995_H_INCLUDED

/*-------------------------------------------------------------------*/
/*  インクルードファイル                                             */
/*-------------------------------------------------------------------*/

#include "Type.h"

/*-------------------------------------------------------------------*/
/*  CPU大域変数                                                      */
/*-------------------------------------------------------------------*/

extern Word PC;                        /* プログラムカウンタ */
extern Word WP;                        /* ワークスペースカウンタ */
extern Word X_flag;                    /* 'X'命令時にセット */
extern Word ST;                        /* ステータスレジスタ */
extern Word in,D,S,Td,Ts,B;            /* オペコードインプリテーション */
extern void (*opcode[65536])(void);    /* CPUオペコードアドレステーブル */

/*-------------------------------------------------------------------*/
/*  プロトタイプ宣言                                                 */
/*-------------------------------------------------------------------*/

void opcode0(Word);
void opcode02(Word);
void opcode03(Word); 
void opcode04(Word);
void opcode05(Word);
void opcode06(Word);
void opcode07(Word);
void opcode1(Word);
void opcode2(Word);
void opcode3(Word);

void do1(void);
void reset(void);

void fixDS(void);
void parity(Byte);
void op_a(void);
void op_ab(void);
void op_abs(void);
void op_ai(void);
void op_dec(void);
void op_dect(void);
void op_div(void);
void op_inc(void);
void op_inct(void);
void op_mpy(void);
void op_neg(void);
void op_s(void);
void op_sb(void);
void op_b(void);
void op_bl(void);
void op_blwp(void);
void op_jeq(void);
void op_jgt(void);
void op_jhe(void);
void op_jh(void);
void op_jl(void);
void op_jle(void);
void op_jlt(void);
void op_jmp(void);
void op_jnc(void);
void op_jne(void);
void op_jno(void);
void op_jop(void);
void op_joc(void);
void op_rtwp(void);
void op_x(void);
void op_xop(void);
void op_c(void);
void op_cb(void);
void op_ci(void);
void op_coc(void);
void op_czc(void);
void op_ldcr(void);
void op_sbo(void);
void op_sbz(void);
void op_stcr(void);
void op_tb(void);
void op_ckof(void);
void op_ckon(void);
void op_idle(void);
void op_rset(void);
void op_lrex(void);
void op_li(void);
void op_limi(void);
void op_lwpi(void);
void op_mov(void);
void op_movb(void);
void op_stst(void);
void op_stwp(void);
void op_swpb(void);
void op_andi(void);
void op_ori(void);
void op_xor(void);
void op_inv(void);
void op_clr(void);
void op_seto(void);
void op_soc(void);
void op_socb(void);
void op_szc(void);
void op_szcb(void);
void op_sra(void);
void op_srl(void);
void op_sla(void);
void op_src(void);
void buildcpu(void);
void op_bad(void);

#endif    /* #ifndef CPU9995_H_INCLUDED */