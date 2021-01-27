/*===================================================================*/
/*                                                                   */
/*  InfoPyuta.cpp : C++ファイル(Windows依存コード)                   */
/*                                                                   */
/*  2001/04/23    Jay's Factory                                      */
/*                                                                   */
/*===================================================================*/

/*-------------------------------------------------------------------*/
/*  インクルードファイル                                             */
/*-------------------------------------------------------------------*/
#include <stdio.h>

#include "InfoPyuta.h"
#include "InfoPyuta_Vdp.h"
#include "pg.h"

/* ぴゅう太のパレットデータ */
Word PyutaPalette[ 16 ] =
{
  0x0000,           /* 透明 */
  0x001f,           /* 青 */
  0x7c00,           /* 赤 */
  0x7c1f,           /* マゼンダ */
  0x03e0,           /* 緑 */
  0x03ff,           /* シアン */
  0x7fe0,           /* 黄 */
  0x7fff,           /* 白 */
  0x0000,           /* 黒 */
  0x000f,           /* 暗い青 */
  0x3c00,           /* 暗い赤 */
  0x3c0f,           /* 暗いマゼンダ */
  0x01e0,           /* 暗い緑 */
  0x01ef,           /* 暗いシアン */
  0x3de0,           /* 暗い黄 */
  0x3def,           /* 灰色 */
};

int getFilePath(char *out);
extern char lastpath[256];

// ----------------------------------------------------------------------------------------
int bSleep=0;
// ホームボタン終了時にコールバック
int exit_callback(void)
{
	bSleep=1;
	sceKernelExitGame();
	return 0;
}

// スリープ時や不定期にコールバック
void power_callback(int unknown, int pwrflags)
{
	if(pwrflags & POWER_CB_POWER){
		bSleep=1;
	}else if(pwrflags & POWER_CB_RESCOMP){
		bSleep=0;
	}
	// コールバック関数の再登録
	// （一度呼ばれたら再登録しとかないと次にコールバックされない）
	int cbid = sceKernelCreateCallback("Power Callback", power_callback);
	scePowerRegisterCallback(0, cbid);
}

// ポーリング用スレッド
int CallbackThread(int args, void *argp)
{
	int cbid;
	
	// コールバック関数の登録
	cbid = sceKernelCreateCallback("Exit Callback", exit_callback);
	sceKernelRegisterExitCallback(cbid);
	cbid = sceKernelCreateCallback("Power Callback", power_callback);
	scePowerRegisterCallback(0, cbid);
	
	// ポーリング
	sceKernelPollCallbacks();
}

int SetupCallbacks(void)
{
	int thid = 0;
	
	// ポーリング用スレッドの生成
	thid = sceKernelCreateThread("update_thread", CallbackThread, 0x11, 0xFA0, 0, 0);
	if(thid >= 0)
		sceKernelStartThread(thid, 0, 0);
	
	return thid;
}

// ----------------------------------------------------------------------------------------
int xmain(int argc, char *argv)
{
	int ret = 0;
	char tmp[MAX_PATH];
	
	SetupCallbacks();
	pgInit();
	
	int r, g, b;
	for(int i=0; i<16; i++)
	{
		r = (PyutaPalette[i]>>10) & 0x1F;
		g = (PyutaPalette[i]>>5) & 0x1F;
		b = PyutaPalette[i] & 0x1F;
		PyutaPalette[i] = (b<<10) | (g<<5) | r;
	}

	strcpy(lastpath, argv);
	*(strrchr(lastpath, '/')+1) = 0;
	for(;;)
	{
		pgScreenFrame(2,0);
		while(!getFilePath(tmp));
		pgScreenFrame(1,0);
		pgFillvram(0);
		
		InfoPyuta_Load(tmp);
		InfoPyuta_Main();
	}
	
	return 0;
}

/*===================================================================*/
/*                                                                   */
/*          InfoNES_ReadRom() : ROMイメージファイルを読み込む        */
/*                                                                   */
/*===================================================================*/
int InfoPyuta_ReadRom( const char *pszFileName )
{
  FILE *fp;

  /* ROMファイルを開く */
  fp = fopen( pszFileName, "rb" );
  if ( fp == NULL )
    return -1;

  /* ROMイメージを読み込む */
  fread( &ROM[ 0x1000 ], 0x4000, 1, fp );

  /* ファイルを閉じる */
  fclose( fp );

  /* 成功 */
  return 0;
}

/*===================================================================*/
/*                                                                   */
/*      InfoNES_LoadFrame() : ワーク画面を実画面に転送               */
/*                                                                   */
/*===================================================================*/
void InfoPyuta_LoadFrame()
{
  /* 画面データを設定 */
	static int x=0, y=0, vx=2, vy=2;
	
	x+=vx;
	y+=vy;
	
	pgBitBlt(0,0,Pyuta_DISP_WIDTH,Pyuta_DISP_HEIGHT,2,WorkFrame);
	
	/*
	unsigned long *v0,*d=WorkFrame;
	v0=(unsigned long *)pgGetVramAddr(x,y);
	for (int yy=0; yy<Pyuta_DISP_HEIGHT; yy++)
	{
		for (int xx=0; xx<Pyuta_DISP_WIDTH; xx+=2)
			*v0++=*d++;
		v0+=(LINESIZE-Pyuta_DISP_WIDTH)/2;
	}
	*/
	
	if(x<=0 | x>SCREEN_WIDTH-Pyuta_DISP_WIDTH)  vx*=-1;
	if(y<=0 | y>SCREEN_HEIGHT-Pyuta_DISP_HEIGHT) vy*=-1;
}

/*===================================================================*/
/*                                                                   */
/*            InfoNES_Wait() : 垂直同期毎のウェイト処理              */
/*                                                                   */
/*===================================================================*/
void InfoPyuta_Wait()
{
  //pgWaitV();
}

void warn( char *pszMsg )
{
//MessageBox( hWndMain, pszMsg, APP_NAME, MB_OK | MB_ICONINFORMATION );
}
