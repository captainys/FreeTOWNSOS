#include <i86.h>
#include "diskbios.h"

int DKB_restore(int devno)
{
	union REGS regs;
	regs.h.ah=0x03;
	regs.h.al=devno;
	int86(INT_DISKBIOS,&regs,&regs);
	if(0x80==regs.h.ah)
	{
		return regs.x.cx|0x8000;
	}
	else
	{
		return regs.h.ah;
	}
}

int DKB_rdstatus(int devno,unsigned int *status)
{
}

int DKB_rdstatus2(int devno,unsigned int *mode,long *numSect)
{
}

int DKB_seek(int devno,int C)
{
}

int DKB_read(int devno,int C,int H,int R,int numSect,char *buf,int *actualReadCount)
{
}

int DKB_read2(int devno,long LBA,int numSect,char *buf,int *actualReadCount)
{
}

int DKB_write(int devno,int C,int H,int R,int numSect,char *buf,int *actualWriteCount)
{
}

int DKB_write2(int devno,long LBA,int numSect,char *buf,int *actualWriteCount)
{
}
