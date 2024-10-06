#ifndef DOSDISK_IS_INCLUDED
#define DOSDISK_IS_INCLUDED
/* { */


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
#define BPB_MEDIA_HD_FAT12     0xFD
#define BPB_MEDIA_HD_FAT16     0xFE
#define BPB_MEDIA_720K         0xF9
#define BPB_MEDIA_1232K        0xFE
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

class DIRENT
{
public:
	char file[8];
	char ext[3];
	uint8_t attr;
	char unused[10];
	uint16_t time; // HHHHHMMMMMMSSSSS (SSSSS=seconds/2)
	uint16_t date; // YYYYYYYMMMMDDDDD (D=1 to 31, M=1 to 12, Y=Year-1980)
	uint16_t firstCluster;
	uint32_t fileSize;
};

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

class Disk
{
public:
	class BPB
	{
	public:
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

		size_t GetBytesPerCluster(void) const
		{
			return sectorsPerCluster*bytesPerSector;
		}

		unsigned int GetFATSector(void) const
		{
			return numReservedSectors;  // Skip IPL
		}
		unsigned int GetBackupFATSector(void) const // NULL_CLUSTER if no backup FAT
		{
			if(2==numFATs)
			{
				return numReservedSectors+sectorsPerFAT;
			}
			else
			{
				return NULL_CLUSTER;
			}
		}
		unsigned int GetRootDirSector(void) const
		{
			return numReservedSectors+sectorsPerFAT*numFATs;
		}
		unsigned int GetFirstDataSector(void) const
		{
			unsigned int dirEntPerSector=(bytesPerSector>>DIRENT_SHIFT);
			unsigned int numDirEntSectors=(numRootDirEnt+dirEntPerSector-1)/dirEntPerSector;
			return numReservedSectors+sectorsPerFAT*numFATs+numDirEntSectors;
		}
		unsigned int GetFATType(void) const; // Returns FAT12 or FAT16
	};

	unsigned int FAT12or16=FAT12;
	std::vector <unsigned char> data;

	bool CreateFD(unsigned int BPB_mediaType);
	void MakeFDBootSectBPB(unsigned char sect[],unsigned char mediaType) const;

	bool CreateHDPartitionByMegaBytes(unsigned int MB);

	void MakeInitialFAT(unsigned char FAT[]) const;
	void MakeInitialRootDir(unsigned char rootDir[],unsigned int numEnt) const;

	void WriteIPLSector(const std::vector <unsigned char> &ipl);

	BPB GetBPB(void) const;

	size_t GetFATLength(void) const;
	size_t GetNumClusters(const BPB &bpb) const;
	unsigned char *GetFAT(void);
	unsigned char *GetBackupFAT(void);
	const unsigned char *GetFAT(void) const;
	const unsigned char *GetBackupFAT(void) const;

	unsigned char *GetRootDir(void);
	const unsigned char *GetRootDir(void) const;

	uint32_t GetFATEntry(const unsigned char FAT[],const BPB &bpb,unsigned int cluster) const;
	void PutFATEntry(unsigned char FAT[],const BPB &bpb,unsigned int cluster,unsigned int incoming) const;
	uint32_t FindAvailableCluster(const unsigned char FAT[],const BPB &bpb) const;
	unsigned char *GetCluster(int cluster,const BPB &bpb);
	const unsigned char *GetCluster(int cluster,const BPB &bpb) const;

	unsigned char *FindAvailableDirEnt(void);
	void WriteDirEnt(
	    unsigned char *dirEnt,std::string file,std::string ext,
	    uint8_t attr,
	    unsigned int hour,unsigned int min,unsigned int sec,
	    unsigned int year,unsigned int month,unsigned int date,
	    unsigned int firstCluster,
	    unsigned int fileSize);

	unsigned int WriteData(const std::vector <unsigned char> &data);


	void ReadSector(unsigned char data[],int trk,int sid,int sec);
	void WriteSector(unsigned char data[],int trk,int sid,int sec);
};

/* } */
#endif
