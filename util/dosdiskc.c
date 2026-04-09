#include "dosdiskc.h"


size_t BPB_GetBytesPerCluster(const BPB *bpb)
{
	return bpb->sectorsPerCluster*bpb->bytesPerSector;
}

unsigned int BPB_GetFATSector(const BPB *bpb)
{
	return bpb->numReservedSectors;  // Skip IPL
}
unsigned int BPB_GetBackupFATSector(const BPB *bpb) // NULL_CLUSTER if no backup FAT
{
	if(2==bpb->numFATs)
	{
		return bpb->numReservedSectors+bpb->sectorsPerFAT;
	}
	else
	{
		return NULL_CLUSTER;
	}
}
unsigned int BPB_GetRootDirSector(const BPB *bpb)
{
	return bpb->numReservedSectors+bpb->sectorsPerFAT*bpb->numFATs;
}
unsigned int BPB_GetFirstDataSector(const BPB *bpb)
{
	unsigned int dirEntPerSector=(bpb->bytesPerSector>>DIRENT_SHIFT);
	unsigned int numDirEntSectors=(bpb->numRootDirEnt+dirEntPerSector-1)/dirEntPerSector;
	return bpb->numReservedSectors+bpb->sectorsPerFAT*bpb->numFATs+numDirEntSectors;
}
unsigned int BPB_GetFATType(const BPB *bpb)
{
	unsigned int a=bpb->bytesPerSector;
	unsigned int b=bpb->sectorsPerCluster;
	if(FAT16_SIZE_THRESHOLD<=a*b)
	{
		return FAT16;
	}
	return FAT12;
}


////////////////////////////////////////////////////////////


void DOSDISK_Init(DOSDISK *disk)
{
	disk->isFloppyDisk=1;  // false for HD.
	disk->FAT12or16=FAT12;
	disk->dataLen=0;
	disk->data=NULL; // DOSDISK does not own the data.  Must be managed outside.
}
