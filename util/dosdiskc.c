#include "dosdiskc.h"


void WriteWord(unsigned char *ptr,unsigned short data)
{
	*(uint16_t *)ptr=data;
}

uint16_t ReadWord(const unsigned char *ptr)
{
	return *(uint16_t *)ptr;
}

void WriteDword(unsigned char *ptr,unsigned int data)
{
	*(uint32_t *)ptr=data;
}

unsigned short ReadDword(const unsigned char *ptr)
{
	return *(uint32_t *)ptr;
}


////////////////////////////////////////////////////////////

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

size_t BPB_GetNumClusters(const BPB *bpb)
{
	size_t numDataSectors=
	    (bpb->totalNumSectors
	    -bpb->numReservedSectors
	    -bpb->numFATs*bpb->sectorsPerFAT
	    -(bpb->numRootDirEnt*DIRENT_BYTES+bpb->bytesPerSector-1)/bpb->bytesPerSector);
	return numDataSectors/bpb->sectorsPerCluster;
}

////////////////////////////////////////////////////////////


void DOSDISK_Init(DOSDISK *disk)
{
	disk->isFloppyDisk=1;  // false for HD.
	disk->FAT12or16=FAT12;
	disk->dataLen=0;
	disk->data=NULL;
}

BPB DOSDISK_GetBPB(const DOSDISK *disk)
{
	BPB bpb;
	bpb.bytesPerSector      =ReadWord(disk->data+BPB_BYTES_PER_SECTOR);
	bpb.sectorsPerCluster   =disk->data[BPB_SECTOR_PER_CLUSTER];
	bpb.numReservedSectors  =ReadWord(disk->data+BPB_RESERVED_SECTOR_CT);
	bpb.numFATs             =disk->data[BPB_NUM_FATS];
	bpb.numRootDirEnt       =ReadWord(disk->data+BPB_NUM_ROOT_DIR_ENT);
	bpb.totalNumSectors     =ReadWord(disk->data+BPB_TOTALNUM_SECT);
	bpb.mediaDesc           =disk->data[BPB_MEDIA_DESC];
	bpb.sectorsPerFAT       =ReadWord(disk->data+BPB_SECT_PER_FAT);
	bpb.sectorsPerTrack     =ReadWord(disk->data+BPB_SECT_PER_TRACK);
	bpb.numHeads            =ReadWord(disk->data+BPB_NUM_HEADS);
	bpb.numHiddenSectors    =ReadWord(disk->data+BPB_HIDDEN_SECT);
	bpb.totalNumSectors32bit=ReadDword(disk->data+BPB_32BIT_NUM_SECT);
	return bpb;
}

void DOSDISK_MakeFDBootSectBPB(unsigned char sect[],unsigned char mediaType)
{
	for(int i=0; i<256; ++i)
	{
		sect[i]=0;
	}
	memcpy(sect,"IPL4",4);
	sect[4]=I386_RETF;

	if(BPB_MEDIA_1232K==mediaType)
	{
		WriteWord(sect+BPB_BYTES_PER_SECTOR,1024);
		sect[BPB_SECTOR_PER_CLUSTER]=1;
		WriteWord(sect+BPB_RESERVED_SECTOR_CT,1);
		sect[BPB_NUM_FATS]=2;
		WriteWord(sect+BPB_NUM_ROOT_DIR_ENT,0xC0);
		WriteWord(sect+BPB_TOTALNUM_SECT,0x4D0);  // 0x4D0=1232
		sect[BPB_MEDIA_DESC]=mediaType;
		WriteWord(sect+BPB_SECT_PER_FAT,2);
		WriteWord(sect+BPB_SECT_PER_TRACK,8);
		WriteWord(sect+BPB_NUM_HEADS,2);
		WriteWord(sect+BPB_HIDDEN_SECT,0);
		WriteDword(sect+BPB_32BIT_NUM_SECT,0);
	}
}
