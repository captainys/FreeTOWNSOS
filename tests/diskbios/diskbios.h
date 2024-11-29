#ifndef DISKBIOS_H_IS_INCLUDED
#define DISKBIOS_H_IS_INCLUDED

#ifdef __cplusplus
extern "C" {
#endif

#define BIOS_FD0 0x20
#define INT_DISKBIOS 0x93

int DKB_restore(int devno);
int DKB_rdstatus(int devno,unsigned int *status);
int DKB_rdstatus2(int devno,unsigned int *mode,long *numSect);
int DKB_seek(int devno,int C);
int DKB_read(int devno,int C,int H,int R,int numSect,char *buf,int *actualReadCount);
int DKB_read2(int devno,long LBA,int numSect,char *buf,int *actualReadCount);
int DKB_write(int devno,int C,int H,int R,int numSect,char *buf,int *actualWriteCount);
int DKB_write2(int devno,long LBA,int numSect,char *buf,int *actualWriteCount);

#ifdef __cplusplus
}
#endif

#endif
