#include <ctype.h>
#include "dosdiskc.h"


#define _Smaller(a,b) ((a)<(b) ? (a) : (b))


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

	DOSDISK_Init(disk);

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

int DOSDISK_CreateFromImage(DOSDISK *disk,size_t dataLen,unsigned char *data)
{
	DOSDISK_Init(disk);

	disk->isFloppyDisk=0;
	disk->dataLen=dataLen;
	disk->data=data;

	BPB bpb=DOSDISK_GetBPB(disk);
	if(bpb.mediaDesc==BPB_MEDIA_1440K ||
	   bpb.mediaDesc==BPB_MEDIA_720K ||
	   bpb.mediaDesc==BPB_MEDIA_1232K ||
	   bpb.mediaDesc==BPB_MEDIA_320K)
	{
		disk->isFloppyDisk=1;
	}

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
	if(BPB_MEDIA_720K==mediaType)
	{
		WriteWord(sect+BPB_BYTES_PER_SECTOR,512);
		sect[BPB_SECTOR_PER_CLUSTER]=2;
		WriteWord(sect+BPB_RESERVED_SECTOR_CT,1);
		sect[BPB_NUM_FATS]=2;
		WriteWord(sect+BPB_NUM_ROOT_DIR_ENT,0x70);
		WriteWord(sect+BPB_TOTALNUM_SECT,0x5A0);
		sect[BPB_MEDIA_DESC]=mediaType;
		WriteWord(sect+BPB_SECT_PER_FAT,3);
		WriteWord(sect+BPB_SECT_PER_TRACK,9);
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

uint32_t DOSDISK_GetFATEntry(const DOSDISK *disk,const unsigned char FAT[],unsigned int cluster)
{
	// If total number of clusters (DPB_MAX_CLUSTER_NUM)>0xFF6, take it as FAT16.
	// Can happen if HDD.

	BPB bpb=DOSDISK_GetBPB(disk);

	if(FAT12==BPB_GetFATType(&bpb))
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

void DOSDISK_PutFATEntry(const DOSDISK *disk,unsigned char FAT[],unsigned int cluster,uint32_t newValue)
{
	// If total number of clusters (DPB_MAX_CLUSTER_NUM)>0xFF6, take it as FAT16.
	// Can happen if HDD.

	BPB bpb=DOSDISK_GetBPB(disk);

	if(FAT12==BPB_GetFATType(&bpb))
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

uint32_t DOSDISK_FindAvailableCluster(const DOSDISK *disk,const unsigned char FAT[])
{
	BPB bpb=DOSDISK_GetBPB(disk);
	for(int i=0; i<BPB_GetNumClusters(&bpb); ++i)
	{
		auto data=DOSDISK_GetFATEntry(disk,FAT,i);
		if(0==data)
		{
			return i;
		}
	}
	return NULL_CLUSTER;
}


unsigned char *DOSDISK_GetCluster(const DOSDISK *disk,int cluster)
{
	return disk->data+DOSDISK_ClusterToOffset(disk,cluster);
}

size_t DOSDISK_ClusterToOffset(const DOSDISK *disk,uint32_t cluster)
{
	BPB bpb=DOSDISK_GetBPB(disk);
	size_t firstDataPos=bpb.bytesPerSector*BPB_GetFirstDataSector(&bpb);
	if(2<=cluster) // Cluster 2 is real first cluster.
	{
		cluster-=2;
	}
	else
	{
		cluster=0;
	}
	size_t clusterPos=firstDataPos+BPB_GetBytesPerCluster(&bpb)*cluster;
	return clusterPos;
}

uint32_t DOSDISK_OffsetToCluster(const DOSDISK *disk,size_t offset)
{
	BPB bpb=DOSDISK_GetBPB(disk);
	size_t firstDataPos=bpb.bytesPerSector*BPB_GetFirstDataSector(&bpb);
	return (offset-firstDataPos)/BPB_GetBytesPerCluster(&bpb);
}

void DOSDISK_ClusterToCHR(const DOSDISK *disk,unsigned char CHR[],int cluster)
{
	BPB bpb=DOSDISK_GetBPB(disk);

	CHR[0]=0;
	CHR[1]=0;
	CHR[2]=0;

	size_t firstDataPos=bpb.bytesPerSector*BPB_GetFirstDataSector(&bpb);
	if(2<=cluster) // Cluster 2 is real first cluster.
	{
		cluster-=2;
	}
	else
	{
		cluster=0;
	}
	size_t clusterPos=firstDataPos+BPB_GetBytesPerCluster(&bpb)*cluster;

	if(0<bpb.bytesPerSector && 0<bpb.sectorsPerTrack)
	{
		size_t lba=clusterPos/bpb.bytesPerSector;
		size_t track=lba/bpb.sectorsPerTrack;
		CHR[0]=track/2; // CYLINDER
		CHR[1]=track&1; // HEAD
		CHR[2]=lba%bpb.sectorsPerTrack;
	}
}

unsigned char *DOSDISK_FindAvailableDirEnt(const DOSDISK *disk)
{
	BPB bpb=DOSDISK_GetBPB(disk);
	unsigned char *rootDir=DOSDISK_GetRootDir(disk);
	for(int i=0; i<bpb.numRootDirEnt; ++i)
	{
		if(0==*rootDir)
		{
			return rootDir;
		}
		rootDir+=DIRENT_BYTES;
	}
	return NULL;
}

unsigned char *DOSDISK_FindAvailableDirEntSubdir(const DOSDISK *disk,uint32_t cluster)
{
	BPB bpb=DOSDISK_GetBPB(disk);
	while(DOSDISK_IsValidCluster(disk,cluster))
	{
		unsigned char *data=DOSDISK_GetCluster(disk,cluster);
		size_t numDirs=BPB_GetBytesPerCluster(&bpb);
		numDirs>>=DIRENT_SHIFT;

		for(int i=0; i<numDirs; ++i)
		{
			if(0==*data)
			{
				return data;
			}
			data+=DIRENT_BYTES;
		}

		cluster=DOSDISK_GetFATEntry(disk,DOSDISK_GetFAT(disk),cluster);
	}
	return NULL;
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

int DOSDISK_CompareDirEndFileName(unsigned char *dirEnt,char file[],const char ext[])
{
	int i;
	for(i=0; i<8 && 0!=file[i]; ++i)
	{
		if(dirEnt[DIRENT_FILENAME+i]!=toupper(file[i]))
		{
			return 1;
		}
	}
	for(i=i; i<8; ++i)
	{
		if(' '!=dirEnt[DIRENT_FILENAME+i] && 0!=dirEnt[DIRENT_FILENAME+i])
		{
			return 1;
		}
	}

	for(i=0; i<3 && 0!=ext[i]; ++i)
	{
		if(dirEnt[DIRENT_EXT+i]!=toupper(ext[i]))
		{
			return 1;
		}
	}
	for(i=i; i<3; ++i)
	{
		if(' '!=dirEnt[DIRENT_EXT+i] && 0!=dirEnt[DIRENT_EXT+i])
		{
			return 1;
		}
	}
	return 0;
}

void DOSDISK_WriteDirEnt(
	    unsigned char *dirEnt,const char file[],const char ext[],
	    uint8_t attr,
	    unsigned int hour,unsigned int min,unsigned int sec,
	    unsigned int year,unsigned int month,unsigned int day,
	    unsigned int firstCluster,
	    unsigned int fileSize)
{
	int i;
	for(i=0; i<8 && 0!=file[i]; ++i)
	{
		dirEnt[DIRENT_FILENAME+i]=toupper(file[i]);
	}
	for(i=i; i<8; ++i)
	{
		dirEnt[DIRENT_FILENAME+i]=' ';
	}
	for(i=0; i<3 && 0!=ext[i]; ++i)
	{
		dirEnt[DIRENT_EXT+i]=toupper(ext[i]);
	}
	for(i=i; i<3; ++i)
	{
		dirEnt[DIRENT_EXT+i]=' ';
	}
	dirEnt[DIRENT_ATTR]=attr;

	uint16_t time;
	time=((hour&0x1F)<<11)|((min&0x2F)<<5)|((sec>>1)&0x1F);
	uint16_t date;
	date=(((year-1980)&0x7F)<<9)|((month&0x0F)<<5)|(day&0x1F);

	WriteWord(dirEnt+DIRENT_TIME,time);
	WriteWord(dirEnt+DIRENT_DATE,date);
	WriteWord(dirEnt+DIRENT_FIRST_CLUSTER,firstCluster);
	WriteDword(dirEnt+DIRENT_FILE_SIZE,fileSize);
}

unsigned int DOSDISK_WriteData(DOSDISK *disk,size_t dataLen,const unsigned char data[])
{
	BPB bpb=DOSDISK_GetBPB(disk);
	size_t pos=0;
	unsigned int prevCluster=0,firstCluster=NULL_CLUSTER;
	while(pos<dataLen)
	{
		size_t writeSize=_Smaller(dataLen-pos,BPB_GetBytesPerCluster(&bpb));
		auto cluster=DOSDISK_FindAvailableCluster(disk,DOSDISK_GetFAT(disk));
		if(cluster!=NULL_CLUSTER)
		{
			if(0==pos)
			{
				firstCluster=cluster;
			}
			else
			{
				DOSDISK_PutFATEntry(disk,DOSDISK_GetFAT(disk),prevCluster,cluster);
				DOSDISK_PutFATEntry(disk,DOSDISK_GetBackupFAT(disk),prevCluster,cluster);
			}
			prevCluster=cluster;

			DOSDISK_PutFATEntry(disk,DOSDISK_GetFAT(disk),cluster,NULL_CLUSTER);
			DOSDISK_PutFATEntry(disk,DOSDISK_GetBackupFAT(disk),cluster,NULL_CLUSTER);

			unsigned char *ptr=DOSDISK_GetCluster(disk,cluster);
			memcpy(ptr,data+pos,writeSize);
		}
		else
		{
			break;
		}
		pos+=writeSize;
	}
	return firstCluster;
}

size_t DOSDISK_ReadData(DOSDISK *disk,size_t dataLen,unsigned char data[],uint32_t cluster)
{
	BPB bpb=DOSDISK_GetBPB(disk);
	size_t pos=0;
	unsigned char *FAT=DOSDISK_GetFAT(disk);
	while(pos<dataLen && 0!=DOSDISK_IsValidCluster(disk,cluster))
	{
		size_t readSize=_Smaller(dataLen-pos,BPB_GetBytesPerCluster(&bpb));

		const unsigned char *ptr=DOSDISK_GetCluster(disk,cluster);
		memcpy(data+pos,ptr,readSize);

		pos+=readSize;

		cluster=DOSDISK_GetFATEntry(disk,FAT,cluster);
	}
	return pos;
}

uint32_t DOSDISK_IsValidCluster(const DOSDISK *disk,uint32_t cluster)
{
	BPB bpb=DOSDISK_GetBPB(disk);
	int FATType=BPB_GetFATType(&bpb);
	if(FAT12==FATType)
	{
		return cluster<0xFFF;
	}
	else if(FAT16==FATType)
	{
		return cluster<0xFFFF;
	}
	return NULL_CLUSTER!=cluster;
}

static void InitializeSubdirectory(unsigned char *data,uint32_t ownCluster,uint32_t parentDirCluster,
	    unsigned int hour,unsigned int min,unsigned int sec,
	    unsigned int year,unsigned int month,unsigned int day)
{
	DOSDISK_WriteDirEnt(data,".","",DIRENT_ATTR_DIRECTORY,
		hour,min,sec,
		year,month,day,
		ownCluster,0);
	DOSDISK_WriteDirEnt(data+DIRENT_BYTES,"..","",DIRENT_ATTR_DIRECTORY,
		hour,min,sec,
		year,month,day,
		parentDirCluster,0);
}

/* Directory entry, dir belongs to parentDirCluster chain.
   dir points to subdirCluster.
*/
int SetUpNewSubdirectory(DOSDISK *disk,
    unsigned char *dir,uint32_t subdirCluster,uint32_t parentDirCluster,
    const char name[],const char ext[],
    unsigned int hour,unsigned int min,unsigned int sec,
    unsigned int year,unsigned int month,unsigned int day)
{
	unsigned char *data=DOSDISK_GetCluster(disk,subdirCluster);
	BPB bpb=DOSDISK_GetBPB(disk);

	memset(data,0,BPB_GetBytesPerCluster(&bpb));
	InitializeSubdirectory(data,subdirCluster,parentDirCluster,hour,min,sec,year,month,day);

	DOSDISK_WriteDirEnt(dir,name,ext,DIRENT_ATTR_DIRECTORY,
		hour,min,sec,
		year,month,day,
		subdirCluster,0);

	DOSDISK_PutFATEntry(disk,DOSDISK_GetFAT(disk),subdirCluster,NULL_CLUSTER);
	DOSDISK_PutFATEntry(disk,DOSDISK_GetBackupFAT(disk),subdirCluster,NULL_CLUSTER);

	return DOSDISK_NOERR;
}

int DOSDISK_MkDir(DOSDISK *disk,const char fileName[],
	    unsigned int hour,unsigned int min,unsigned int sec,
	    unsigned int year,unsigned int month,unsigned int day)
{
	BPB bpb=DOSDISK_GetBPB(disk);

	for(int i=0; fileName[i]!=0; ++i)
	{
		if(fileName[i]<=0x20 || 0!=(fileName[i]&0x80) || '*'==fileName[i])
		{
			return DOSDISK_ERR_BAD_FILE_NAME;
		}
	}

	uint32_t cluster=NULL_CLUSTER;
	uint32_t dirLen=0;

	int ptr=0;
	while('\\'==fileName[0] || '/'==fileName[0])
	{
		++ptr;
	}

	size_t nameLen=0,extLen=0;
	char name[8],ext[3];
	memset(name,' ',9);
	memset(ext,' ',3);
	for(;; ++ptr)
	{
		if(0==fileName[ptr] || '/'==fileName[ptr] || '\\'==fileName[ptr])
		{
			if(NULL_CLUSTER==cluster) // Root dir
			{
				unsigned char *rootDir=DOSDISK_GetRootDir(disk);
				unsigned char *dir=rootDir;
				uint32_t dirLen=bpb.numRootDirEnt;
				int i,found=0;

				// First try to find the sub-directory.
				for(i=0; i<dirLen; ++i)
				{
					if(0==DOSDISK_CompareDirEndFileName(dir,name,ext))
					{
						found=1;
						break;
					}
					dir+=DIRENT_BYTES;
				}
				// If found, move to the sub-directory and continue.
				if(0!=found)
				{
					DIRENT *dirEnt=(DIRENT *)dir;
					if(0==(dirEnt->attr&DIRENT_ATTR_DIRECTORY))
					{
						return DOSDISK_ERR_FILE_ALREADY_EXISTS;
					}
					else
					{
						cluster=dirEnt->firstCluster;
					}
				}
				// If not found, try to create one.
				else
				{
					unsigned int cluster0=0; // Parent of /subdir/.. is 0.
					unsigned char *dir=DOSDISK_FindAvailableDirEnt(disk);
					int err;

					if(NULL==dir)
					{
						return DOSDISK_ERR_DIRECTORY_FULL;
					}
					cluster=DOSDISK_FindAvailableCluster(disk,DOSDISK_GetFAT(disk));
					if(0==DOSDISK_IsValidCluster(disk,cluster))
					{
						return DOSDISK_ERR_DISK_FULL;
					}
					err=SetUpNewSubdirectory(disk,
					    dir,cluster,cluster0,
					    name,ext,hour,min,sec,year,month,day);
					if(DOSDISK_NOERR!=err)
					{
						return err;
					}
				}
			}
			else
			{
				const uint32_t cluster0=cluster;
				uint32_t lastValidCluster=NULL_CLUSTER;
				unsigned char *dir=NULL;
				uint8_t found=0;

				// First try to find the dirent.
				while(DOSDISK_IsValidCluster(disk,cluster))
				{
					lastValidCluster=cluster;

					unsigned char *data=DOSDISK_GetCluster(disk,cluster);
					size_t dataLen=BPB_GetBytesPerCluster(&bpb);
					size_t numDirs=(dataLen>>DIRENT_SHIFT);
					for(int i=0; i<numDirs; ++i)
					{
						if(0==DOSDISK_CompareDirEndFileName(data,name,ext))
						{
							found=1;
							break;
						}
						data+=DIRENT_BYTES;
					}
					if(found)
					{
						dir=data;
						break;
					}
					cluster=DOSDISK_GetFATEntry(disk,DOSDISK_GetFAT(disk),cluster);
				}
				// If found, move to the sub-directory and continue.
				if(0!=found)
				{
					DIRENT *dirEnt=(DIRENT *)dir;
					if(0==(dirEnt->attr&DIRENT_ATTR_DIRECTORY))
					{
						return DOSDISK_ERR_FILE_ALREADY_EXISTS;
					}
					else
					{
						cluster=dirEnt->firstCluster;
					}
				}
				// If not found, try to create one.
				else
				{
					int err;
					unsigned char *dir=DOSDISK_FindAvailableDirEntSubdir(disk,cluster0);
					if(0==DOSDISK_IsValidCluster(disk,lastValidCluster))
					{
						return DOSDISK_ERR_DISK_FULL;
					}

					if(NULL==dir)
					{
						uint32_t nextCluster=DOSDISK_FindAvailableCluster(disk,DOSDISK_GetFAT(disk));
						if(0==DOSDISK_IsValidCluster(disk,nextCluster))
						{
							return DOSDISK_ERR_DISK_FULL;
						}
						dir=DOSDISK_GetCluster(disk,nextCluster);
						memset(dir,0,BPB_GetBytesPerCluster(&bpb));

						DOSDISK_PutFATEntry(disk,DOSDISK_GetFAT(disk),cluster,nextCluster);
						DOSDISK_PutFATEntry(disk,DOSDISK_GetBackupFAT(disk),cluster,nextCluster);

						DOSDISK_PutFATEntry(disk,DOSDISK_GetFAT(disk),nextCluster,NULL_CLUSTER);
						DOSDISK_PutFATEntry(disk,DOSDISK_GetBackupFAT(disk),nextCluster,NULL_CLUSTER);

						// Once dir is set, can forget nextCluster.
					}

					cluster=DOSDISK_FindAvailableCluster(disk,DOSDISK_GetFAT(disk));
					if(0==DOSDISK_IsValidCluster(disk,cluster))
					{
						return DOSDISK_ERR_DISK_FULL;
					}

					err=SetUpNewSubdirectory(disk,
					    dir,cluster,cluster0,
					    name,ext,hour,min,sec,year,month,day);
					if(DOSDISK_NOERR!=err)
					{
						return err;
					}
				}
			}
			memset(name,' ',9);
			memset(ext,' ',3);
			nameLen=0;
			extLen=0;
		}
		else
		{
			if('.'==fileName[ptr])
			{
				nameLen=8;
				if(0!=extLen)
				{
					extLen=3;
				}
			}
			else
			{
				if(nameLen<8)
				{
					name[nameLen++]=toupper(fileName[ptr]);
				}
				else if(extLen<3)
				{
					ext[extLen++]=toupper(fileName[ptr]);
				}
			}
		}
		if(0==fileName[ptr])
		{
			break;
		}
	}
	return DOSDISK_NOERR;
}
