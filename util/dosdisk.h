#ifndef DOSDISK_IS_INCLUDED
#define DOSDISK_IS_INCLUDED
/* { */

#include "dosdiskc.h"

class Disk
{
public:
	DOSDISK disk;
	std::vector <unsigned char> data;

	inline BPB GetBPB(void) const
	{
		return DOSDISK_GetBPB(&disk);
	}


	Disk();

	bool CreateFD(unsigned int BPB_mediaType);
	void MakeFDBootSectBPB(unsigned char sect[],unsigned char mediaType) const;

	bool CreateHDPartitionByMegaBytes(unsigned int MB);

	void MakeInitialFAT(unsigned char FAT[]) const;
	void MakeInitialRootDir(unsigned char rootDir[],unsigned int numEnt) const;

	void WriteIPLSector(const std::vector <unsigned char> &ipl);

	size_t GetFATLength(void) const;
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
	void ClusterToCHR(unsigned char CHR[3],int cluster) const;

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
