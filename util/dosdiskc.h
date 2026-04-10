#ifndef DOSDISC_C_IS_INCLUDED
#define DOSDISC_C_IS_INCLUDED

#ifdef __cplusplus
extern "C" {
#endif


#include <stdio.h>
#include <string.h> // for memcpy
#include <stdint.h>


#define DOSDISK_NOERR                   0
#define DOSDISK_ERR                     1
#define DOSDISK_ERR_MEDIA_NOT_SUPPORTED 2
#define DOSDISK_ERR_WRONG_SIZE          3
#define DOSDISK_ERR_BAD_FILE_NAME       4
#define DOSDISK_ERR_FILE_ALREADY_EXISTS 5
#define DOSDISK_ERR_DIRECTORY_FULL      6
#define DOSDISK_ERR_DISK_FULL           7

void WriteWord(unsigned char *ptr,unsigned short data);
uint16_t ReadWord(const unsigned char *ptr);
void WriteDword(unsigned char *ptr,unsigned int data);
unsigned short ReadDword(const unsigned char *ptr);



// The disk layout:
// 
// Sector 0
//     Number of reserve sectors.  IPL etc.
// --------
//     File Allocation Table
//     Back Up File Allocation Table
//     (In total File Allocation Table times [BPB_NUM_FATS])
// --------
//     Root Directory
// --------
//     Data
// --------


#define BPB_BYTES_PER_SECTOR   0x0B
#define BPB_SECTOR_PER_CLUSTER 0x0D
#define BPB_RESERVED_SECTOR_CT 0x0E
#define BPB_NUM_FATS           0x10
#define BPB_NUM_ROOT_DIR_ENT   0x11
#define BPB_TOTALNUM_SECT      0x13
#define BPB_MEDIA_DESC         0x15
#define BPB_SECT_PER_FAT       0x16
#define BPB_SECT_PER_TRACK     0x18
#define BPB_NUM_HEADS          0x1A
#define BPB_HIDDEN_SECT        0x1C
#define BPB_32BIT_NUM_SECT     0x20  // Used to indicate the location of IO.SYS in FM-R/TOWNS IPL.

#define BPB_MEDIA_1440K        0xF0
#define BPB_MEDIA_HARD_DISK    0xF8
#define BPB_MEDIA_IC_MEMORY_CARD  0xF8   // FM TOWNS TICM.SYS
#define BPB_MEDIA_HD_FAT12     0xFD
#define BPB_MEDIA_HD_FAT16     0xFE
#define BPB_MEDIA_720K         0xF9
#define BPB_MEDIA_1232K        0xFE
#define BPB_MEDIA_640K         0xFB
#define BPB_MEDIA_320K         0xFF


#define DIRENT_ATTR_READONLY	0x01
#define DIRENT_ATTR_HIDDEN		0x02
#define DIRENT_ATTR_SYSTEM		0x04
#define DIRENT_ATTR_VOLLABEL	0x08
#define DIRENT_ATTR_DIRECTORY	0x10
#define DIRENT_ATTR_ARCHIVE		0x20

#define DIRENT_FILENAME			0x00
#define DIRENT_EXT				0x08
#define DIRENT_ATTR				0x0B
#define DIRENT_UNUSED			0x0C
#define DIRENT_TIME				0x16
#define DIRENT_DATE				0x18
#define DIRENT_FIRST_CLUSTER	0x1A
#define DIRENT_FILE_SIZE		0x1C

#define DIRENT_BYTES			32
#define DIRENT_SHIFT			5    // 32 bytes per dirent

#define NULL_CLUSTER 0xFFFFFFFF

#define I386_RETF              0xCB

#define FAT16_SIZE_THRESHOLD	(64*1024*1024)
#define FAT12					12
#define FAT16					16

#define CLUSTER_BUFFER_SIZE		1024


typedef struct
{
	char file[8];
	char ext[3];
	uint8_t attr;
	char unused[10];
	uint16_t time; // HHHHHMMMMMMSSSSS (SSSSS=seconds/2)
	uint16_t date; // YYYYYYYMMMMDDDDD (D=1 to 31, M=1 to 12, Y=Year-1980)
	uint16_t firstCluster;
	uint32_t fileSize;
} DIRENT;

typedef struct
{
	// DOS BPB is so deficient that same mediaDesc is used for FAT16 of HD and FAT12 of 1232KB floppy disk.
	// How can I identify FAT12 or FAT16 then?
	// All I can think of is sectorsPerTrack is zero for HD.  Then mediaDesc for FAT12 or FAT16.
	uint16_t bytesPerSector;
	uint8_t sectorsPerCluster;
	uint16_t numReservedSectors; // Such as IPL sector.
	uint8_t numFATs;
	uint16_t numRootDirEnt;
	uint16_t totalNumSectors; // Including reserved sectors
	uint8_t mediaDesc;
	uint16_t sectorsPerFAT;
	uint16_t sectorsPerTrack;
	uint16_t numHeads;
	uint16_t numHiddenSectors;
	uint32_t totalNumSectors32bit;
} BPB;

size_t BPB_GetBytesPerCluster(const BPB *bpb);
unsigned int BPB_GetFATSector(const BPB *bpb);
unsigned int BPB_GetBackupFATSector(const BPB *bpb); // NULL_CLUSTER if no backup FAT
unsigned int BPB_GetRootDirSector(const BPB *bpb);
unsigned int BPB_GetFirstDataSector(const BPB *bpb);
unsigned int BPB_GetFATType(const BPB *bpb);
size_t BPB_GetFATLength(const BPB *bpb);
size_t BPB_GetNumClusters(const BPB *bpb);



typedef struct
{
	unsigned char isFloppyDisk;  // non-zero means FD, zero means HD partition or ICM.
	unsigned int FAT12or16;

	// DOSDISK does not own the disk image.  Must be retained outside.
	size_t dataLen;
	unsigned char *data;
} DOSDISK;

/*! Initialize a disk to floppydisk, FAT12, and zero data.
*/
void DOSDISK_Init(DOSDISK *disk);

/*! Create a floppy disk image.  Returns DOSDISK error code.
*/
int DOSDISK_CreateFD(DOSDISK *disk,unsigned int mediaDesc,size_t dataSize,unsigned char *data);

/*! Create a hard-disk partition image.  Returns DOSDISK error code.
*/
int DOSDISK_CreateHDPartitionByMegaBytes(DOSDISK *disk,size_t MB,size_t dataSize,unsigned char *data);

/*! Create a DOSDISK from an image.
*/
int DOSDISK_CreateFromImage(DOSDISK *disk,size_t dataSize,unsigned char *data);

/*! Unsupported -> 0.
*/
size_t DOSDISK_GetRequiredBytesFD(unsigned int mediaDesc);

/*!
*/
BPB DOSDISK_GetBPB(const DOSDISK *disk);

/*!
*/
int DOSDISK_MakeFDBootSectBPB(unsigned char sect[],unsigned char mediaType);

/*!
*/
void DOSDISK_MakeInitialFAT(const DOSDISK *disk,unsigned char fat[]);

/*!
*/
void DOSDISK_MakeInitialRootDir(const DOSDISK *disk,unsigned char rootDir[],size_t numRootDirEnt);

/*!
*/
uint32_t DOSDISK_FindAvailableCluster(const DOSDISK *disk,const unsigned char FAT[]);

/*! Returns pointer to the cluster.  cluster=2 is the real first cluster that takes the
    first of the data sectors because the first two clusters are reserved in the FAT.
*/
unsigned char *DOSDISK_GetCluster(const DOSDISK *disk,int cluster);


/*! Returns the image data offset in bytes from the cluster number.
*/
size_t DOSDISK_ClusterToOffset(const DOSDISK *disk,uint32_t cluster);

/*! Returns cluster from the image data byte offset.
*/
uint32_t DOSDISK_OffsetToCluster(const DOSDISK *disk,size_t offset);


/*!
*/
unsigned char *DOSDISK_GetFAT(const DOSDISK *disk);

/*!
*/
void DOSDISK_ClusterToCHR(const DOSDISK *disk,unsigned char CHR[],int cluster);

/*!
*/
uint32_t DOSDISK_GetFATEntry(const DOSDISK *disk,const unsigned char FAT[],unsigned int cluster);

/*!
*/
void DOSDISK_PutFATEntry(const DOSDISK *disk,unsigned char FAT[],unsigned int cluster,uint32_t newValue);

/*!
*/
unsigned char *DOSDISK_GetBackupFAT(const DOSDISK *disk);

/*!
*/
unsigned char *DOSDISK_GetRootDir(const DOSDISK *disk);

/*!
*/
unsigned char *DOSDISK_FindAvailableDirEnt(const DOSDISK *disk);

/*!
*/
unsigned char *DOSDISK_FindAvailableDirEntSubdir(const DOSDISK *disk,uint32_t cluster);

/*! Returns 0 if file and ext matches.
*/
int DOSDISK_CompareDirEndFileName(unsigned char *dirEnt,char file[],const char ext[]);

/*!
*/
void DOSDISK_WriteDirEnt(
	    unsigned char *dirEnt,const char file[],const char ext[],
	    uint8_t attr,
	    unsigned int hour,unsigned int min,unsigned int sec,
	    unsigned int year,unsigned int month,unsigned int day,
	    unsigned int firstCluster,
	    unsigned int fileSize);

/*! Returns the first cluster.
*/
unsigned int DOSDISK_WriteData(DOSDISK *disk,size_t len,const unsigned char data[]);

/*! Read data from the cluster chain starting at the given cluster up to the len bytes.
    Returns the number of bytes read.
*/
size_t DOSDISK_ReadData(DOSDISK *disk,size_t len,unsigned char data[],uint32_t cluster);

/*!
*/
uint32_t DOSDISK_IsValidCluster(const DOSDISK *disk,uint32_t cluster);

/*!
*/
int DOSDISK_MkDir(DOSDISK *disk,const char fileName[],
	    unsigned int hour,unsigned int min,unsigned int sec,
	    unsigned int year,unsigned int month,unsigned int day);

/*!
*/
int DOSDISK_WriteVolumeLabel(DOSDISK *disk,const char volumeLabel[],
	    unsigned int hour,unsigned int min,unsigned int sec,
	    unsigned int year,unsigned int month,unsigned int day);

/*!
*/
int DOSDISK_WriteFile(DOSDISK *disk,const char fileName[],
	    size_t dataLen,unsigned char data[],
	    uint8_t additional_attr,
	    unsigned int hour,unsigned int min,unsigned int sec,
	    unsigned int year,unsigned int month,unsigned int day);


#ifdef __cplusplus
} // extern "C"
#endif

#endif
