#ifndef ISO9660_IS_INCLUDED
#define ISO9660_IS_INCLUDED
/* { */

// Based on https://wiki.osdev.org/ISO_9660

#include <stdint.h>

struct ISO9660_DateTime
{
	uint8_t yearsSince1900;
	uint8_t month; // 1 to 12
	uint8_t day;  // 1 to 31
	uint8_t hour; // 0 to 23
	uint8_t min;  // 0 to 59
	uint8_t sec;  // 0 to 59
	uint8_t offsetFromGMT; // WTF.  Keep it zero.
};

struct ISO9660_PathTableEntry
{
	uint8_t len;
	uint8_t extAtt;
	uint32_t LBA;
	uint16_t parentDirNum;
	char name[1];
};

struct ISO9660_Directory
{
	uint8_t len;
	uint8_t extAtt;
	unsigned char LBA_LE[4];
	unsigned char LBA_BE[4];
	unsigned char sizeLE[4];
	unsigned char sizeBE[4];
	struct ISO9660_DateTime dateTime;
	uint8_t flags;
	uint8_t unitSizeForInterleave; // Keep it zero.
	uint8_t interleaveGAP;         // Keep it zero.
	unsigned char volumeSeqLE[2];
	unsigned char volumeSeqBE[2];
	uint8_t nameLEN;
	char name[1];
};

// This record does not appear in many FM TOWNS Discs.
struct ISO9660_BootRecord
{
	uint8_t	zero;
	char CD001[5];
	uint8_t volumeDesc;
	char bootSys[32];
	char bootID[32];
};

struct ISO9660_PrimaryVolumeDescriptor
{
	uint8_t one; // Type Code=01
	char CD001[5];
	uint8_t version;
	uint8_t unused1;
	char systemID[32];	// Can be empty (all ' ')
	char volumeID[32];	// App Name followed by ' 's
	char unused2[8]
	unsigned char volumeSpaceLE[4];
	unsigned char volumeSpaceBE[4];
	char unused3[32]
	unsigned char volumeSetSizeLE[2]; // Make it 01 00
	unsigned char volumeSetSizeBE[2]; // Make it 00 01
	unsigned char volumeSequenceLE[2]; // Make it 01 00
	unsigned char volumeSequenceBE[2]; // Make it 00 01
	unsigned char logicalBlockLE[2]; // 08 00  0800H in Little Endian
	unsigned char logicalBlockBE[2]; // 00 08  0800H in Big Endian
	unsigned char pathTableSizeLE[4]; // Size in bytes
	unsigned char pathTableSizeBE[4];
	unsigned char LBAPathTableLE[4];
	unsigned char LBSPathTableOptLE[4];
	unsigned char LBAPathTableBE[4];
	unsigned char LBSPathTableOptBE[4];

	struct ISO9660_Directory rootDir;

	char volumeSetID[128];  // Keep it all ' '
	char publisherID[128];  // Publisher Name
	char dataPreparerID[128];   // Keep it all ' '
	char appID[128]; // Name of the program
	char copyrightFileID[37];
	char abstractFileID[37];
	char biblioFileID[37];
	char creationDate[17]; // Like "20241105123700000" Not a C-String
	char modificationDate[17];// Like "20241105123700000" Not a C-String
	char expierationDate[17]; // keep it all '0'
	char effectiveDate[17];
	unsigned char fileStructVersion;
	char unused4;
};

// Immediately follows the primary volume descriptor.
struct ISO9660_LastVolumeDescriptor
{
	uint8_t ff;
	char CD001[5];
};

/* } */
#endif
