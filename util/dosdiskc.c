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

size_t BPB_GetFATLength(const BPB *bpb)
{
	return bpb->bytesPerSector*bpb->sectorsPerFAT;
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

int DOSDISK_CreateFD(DOSDISK *disk,unsigned int mediaDesc,size_t dataSize,unsigned char *data)
{
	int err=0;

	DOSDISK_Init(disk);
	disk->isFloppyDisk=1;
	disk->FAT12or16=FAT12;
	disk->dataLen=dataSize;
	disk->data=data;

	err=DOSDISK_MakeFDBootSectBPB(data,mediaDesc);
	if(DOSDISK_NOERR==err)
	{
		BPB bpb=DOSDISK_GetBPB(disk);
		DOSDISK_MakeInitialFAT(disk,DOSDISK_GetFAT(disk));
		DOSDISK_MakeInitialFAT(disk,DOSDISK_GetBackupFAT(disk));
		DOSDISK_MakeInitialRootDir(disk,DOSDISK_GetRootDir(disk),bpb.numRootDirEnt);
	}
	return err;
}

int DOSDISK_CreateHDPartitionByMegaBytes(DOSDISK *disk,size_t MB,size_t dataLen,unsigned char *data)
{
	if(0==MB)
	{
		return DOSDISK_ERR_WRONG_SIZE;
	}

	unsigned int bytesPerSect;
	unsigned int sectorsPerCluster;
	unsigned int rootDirEnt;
	unsigned int sectorsPerFAT;
	unsigned char mediaType;

	if(1==MB)
	{
		bytesPerSect=1024;   sectorsPerCluster=1;  rootDirEnt= 256;  sectorsPerFAT= 6;   mediaType=BPB_MEDIA_HD_FAT12;
	}
	else if(MB<4) // 2 to 3MB
	{
		bytesPerSect=1024;   sectorsPerCluster=1;  rootDirEnt= 256;  sectorsPerFAT= 3;   mediaType=BPB_MEDIA_HD_FAT12;
	}
	else if(MB<8) // 4 to 7MB
	{
		bytesPerSect=2048;   sectorsPerCluster=1;  rootDirEnt= 512;  sectorsPerFAT= 3;   mediaType=BPB_MEDIA_HD_FAT12;
	}
	else if(MB<16) // 8 to 15MB
	{
		bytesPerSect=2048;   sectorsPerCluster=2;  rootDirEnt= 512;  sectorsPerFAT= 3;   mediaType=BPB_MEDIA_HD_FAT12;
	}
	else if(MB<32) // 16 to 31MB
	{
		bytesPerSect=2048;   sectorsPerCluster=4;  rootDirEnt= 512;  sectorsPerFAT= 3;   mediaType=BPB_MEDIA_HD_FAT12;
	}
	else if(MB<64) // 32 to 63MB
	{
		bytesPerSect=2048;   sectorsPerCluster=8;  rootDirEnt= 512;  sectorsPerFAT= 3;   mediaType=BPB_MEDIA_HD_FAT12;
	}
	else if(MB<128)
	{
		bytesPerSect=2048;   sectorsPerCluster=2;  rootDirEnt=1024;  sectorsPerFAT=32;   mediaType=BPB_MEDIA_HD_FAT16;
	}
	else
	{
		// Beyond FAT16 capacity
		return DOSDISK_ERR_WRONG_SIZE;
	}

	disk->isFloppyDisk=0;
	disk->dataLen=dataLen;
	disk->data=data;

	const size_t reserveSect=1;
	size_t sizeInBytes=MB*1024*1024;
	unsigned int totalSectors=(sizeInBytes/bytesPerSect);
	const size_t numFATs=2;

	memset(data,0,dataLen);
	memcpy(data,"IPL4",4);
	data[4]=I386_RETF;

	WriteWord(data+BPB_BYTES_PER_SECTOR,bytesPerSect);
	data[BPB_SECTOR_PER_CLUSTER]=sectorsPerCluster;
	WriteWord(data+BPB_RESERVED_SECTOR_CT,reserveSect);
	data[BPB_NUM_FATS]=numFATs;
	WriteWord(data+BPB_NUM_ROOT_DIR_ENT,rootDirEnt);
	WriteWord(data+BPB_TOTALNUM_SECT,totalSectors);
	data[BPB_MEDIA_DESC]=mediaType;
	WriteWord(data+BPB_SECT_PER_FAT,sectorsPerFAT);
	WriteWord(data+BPB_SECT_PER_TRACK,16); // 16 for HD
	WriteWord(data+BPB_NUM_HEADS,1);      // 1 for HD
	WriteWord(data+BPB_HIDDEN_SECT,0);
	WriteDword(data+BPB_32BIT_NUM_SECT,0);

	BPB bpb=DOSDISK_GetBPB(disk);
	DOSDISK_MakeInitialFAT(disk,DOSDISK_GetFAT(disk));
	DOSDISK_MakeInitialFAT(disk,DOSDISK_GetBackupFAT(disk));
	DOSDISK_MakeInitialRootDir(disk,DOSDISK_GetRootDir(disk),bpb.numRootDirEnt);

	disk->FAT12or16=BPB_GetFATType(&bpb);

	return DOSDISK_NOERR;
}

size_t DOSDISK_GetRequiredBytesFD(unsigned int mediaDesc)
{
	if(BPB_MEDIA_1232K==mediaDesc)
	{
		return 1232*1024;
	}
	return 0;
}

int DOSDISK_MakeFDBootSectBPB(unsigned char sect[],unsigned char mediaType)
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
		return DOSDISK_NOERR;
	}
	return DOSDISK_ERR_MEDIA_NOT_SUPPORTED;
}

void DOSDISK_MakeInitialFAT(const DOSDISK *disk,unsigned char FAT[])
{
	BPB bpb=DOSDISK_GetBPB(disk);
	size_t len=BPB_GetFATLength(&bpb);
	memset(FAT,0,len);
	if(FAT12==BPB_GetFATType(&bpb))
	{
		if(disk->isFloppyDisk)
		{
			FAT[0]=0xFE;
			FAT[1]=0xFF;
			FAT[2]=0xFF;
		}
		else
		{
			FAT[0]=0xF9;
			FAT[1]=0xFF;
			FAT[2]=0xFF;
		}
	}
	else
	{
		FAT[0]=0xFA;  // If HDD, FA FF FF FF.
		FAT[1]=0xFF;
		FAT[2]=0xFF;
		FAT[3]=0xFF;
	}
}

void DOSDISK_MakeInitialRootDir(const DOSDISK *disk,unsigned char rootDir[],size_t numRootDirEnt)
{
	size_t bytes=(numRootDirEnt<<DIRENT_SHIFT);
	memset(rootDir,0,bytes);
}

unsigned char *DOSDISK_GetFAT(const DOSDISK *disk)
{
	BPB bpb=DOSDISK_GetBPB(disk);
	// FAT is located immediately after reserved sectors.
	size_t pos=bpb.bytesPerSector*bpb.numReservedSectors;
	return disk->data+pos;
}

uint32_t DOSDISK_GetFATEntry(const DOSDISK *disk,const unsigned char FAT[],const BPB *bpb,unsigned int cluster)
{
	// If total number of clusters (DPB_MAX_CLUSTER_NUM)>0xFF6, take it as FAT16.
	// Can happen if HDD.

	if(FAT12==BPB_GetFATType(bpb))
	{
		if(0==(cluster&1))
		{
			uint32_t data;
			data=ReadWord(FAT+(cluster/2)*3);
			data&=0xFFF;
			return data;
		}
		else
		{
			uint32_t data;
			data=ReadWord(FAT+(cluster/2)*3+1);
			data>>=4;
			data&=0xFFF;
			return data;
		}
	}
	else
	{
		return ReadWord(FAT+cluster*2);
	}
}

void DOSDISK_PutFATEntry(const DOSDISK *disk,unsigned char FAT[],const BPB *bpb,unsigned int cluster,uint32_t newValue)
{
	// If total number of clusters (DPB_MAX_CLUSTER_NUM)>0xFF6, take it as FAT16.
	// Can happen if HDD.

	if(FAT12==BPB_GetFATType(bpb))
	{
		if(0==(cluster&1))
		{
			uint32_t data;
			data=ReadWord(FAT+(cluster/2)*3);
			data&=0xF000;
			data|=(newValue&0xFFF);
			WriteWord(FAT+(cluster/2)*3,data);
		}
		else
		{
			uint32_t data;
			data=ReadWord(FAT+(cluster/2)*3+1);
			data&=0x000F;
			data|=(newValue<<4);
			WriteWord(FAT+(cluster/2)*3+1,data);
		}
	}
	else
	{
		WriteWord(FAT+cluster*2,newValue);
	}
}

unsigned char *DOSDISK_GetBackupFAT(const DOSDISK *disk)
{
	BPB bpb=DOSDISK_GetBPB(disk);
	// Backup FAT is located immediately after the primary FAT.
	size_t pos=bpb.bytesPerSector*(bpb.numReservedSectors+bpb.sectorsPerFAT);
	return disk->data+pos;
}

unsigned char *DOSDISK_GetRootDir(const DOSDISK *disk)
{
	BPB bpb=DOSDISK_GetBPB(disk);
	size_t pos=bpb.bytesPerSector*BPB_GetRootDirSector(&bpb);
	return disk->data+pos;
}
