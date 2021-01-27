/*===================================================================*/
/*                                                                   */
/*  InfoPyuta.cpp : C++�t�@�C��(Windows�ˑ��R�[�h)                   */
/*                                                                   */
/*  2001/04/23    Jay's Factory                                      */
/*                                                                   */
/*===================================================================*/

/*-------------------------------------------------------------------*/
/*  �C���N���[�h�t�@�C��                                             */
/*-------------------------------------------------------------------*/
#include <stdio.h>

#include "InfoPyuta.h"
#include "InfoPyuta_Vdp.h"
#include "pg.h"

/* �҂イ���̃p���b�g�f�[�^ */
Word PyutaPalette[ 16 ] =
{
  0x0000,           /* ���� */
  0x001f,           /* �� */
  0x7c00,           /* �� */
  0x7c1f,           /* �}�[���_ */
  0x03e0,           /* �� */
  0x03ff,           /* �V�A�� */
  0x7fe0,           /* �� */
  0x7fff,           /* �� */
  0x0000,           /* �� */
  0x000f,           /* �Â��� */
  0x3c00,           /* �Â��� */
  0x3c0f,           /* �Â��}�[���_ */
  0x01e0,           /* �Â��� */
  0x01ef,           /* �Â��V�A�� */
  0x3de0,           /* �Â��� */
  0x3def,           /* �D�F */
};

int getFilePath(char *out);
extern char lastpath[256];

// ----------------------------------------------------------------------------------------
int bSleep=0;
// �z�[���{�^���I�����ɃR�[���o�b�N
int exit_callback(void)
{
	bSleep=1;
	sceKernelExitGame();
	return 0;
}

// �X���[�v����s����ɃR�[���o�b�N
void power_callback(int unknown, int pwrflags)
{
	if(pwrflags & POWER_CB_POWER){
		bSleep=1;
	}else if(pwrflags & POWER_CB_RESCOMP){
		bSleep=0;
	}
	// �R�[���o�b�N�֐��̍ēo�^
	// �i��x�Ă΂ꂽ��ēo�^���Ƃ��Ȃ��Ǝ��ɃR�[���o�b�N����Ȃ��j
	int cbid = sceKernelCreateCallback("Power Callback", power_callback);
	scePowerRegisterCallback(0, cbid);
}

// �|�[�����O�p�X���b�h
int CallbackThread(int args, void *argp)
{
	int cbid;
	
	// �R�[���o�b�N�֐��̓o�^
	cbid = sceKernelCreateCallback("Exit Callback", exit_callback);
	sceKernelRegisterExitCallback(cbid);
	cbid = sceKernelCreateCallback("Power Callback", power_callback);
	scePowerRegisterCallback(0, cbid);
	
	// �|�[�����O
	sceKernelPollCallbacks();
}

int SetupCallbacks(void)
{
	int thid = 0;
	
	// �|�[�����O�p�X���b�h�̐���
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
/*          InfoNES_ReadRom() : ROM�C���[�W�t�@�C����ǂݍ���        */
/*                                                                   */
/*===================================================================*/
int InfoPyuta_ReadRom( const char *pszFileName )
{
  FILE *fp;

  /* ROM�t�@�C�����J�� */
  fp = fopen( pszFileName, "rb" );
  if ( fp == NULL )
    return -1;

  /* ROM�C���[�W��ǂݍ��� */
  fread( &ROM[ 0x1000 ], 0x4000, 1, fp );

  /* �t�@�C������� */
  fclose( fp );

  /* ���� */
  return 0;
}

/*===================================================================*/
/*                                                                   */
/*      InfoNES_LoadFrame() : ���[�N��ʂ�����ʂɓ]��               */
/*                                                                   */
/*===================================================================*/
void InfoPyuta_LoadFrame()
{
  /* ��ʃf�[�^��ݒ� */
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
/*            InfoNES_Wait() : �����������̃E�F�C�g����              */
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
