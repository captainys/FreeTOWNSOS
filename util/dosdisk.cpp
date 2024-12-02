#include <stdio.h>
#include <iostream>
#include <fstream>
#include <vector>
#include <stdint.h>
#include <ctype.h>

#include "dosdisk.h"



////////////////////////////////////////////////////////////

unsigned int Disk::BPB::GetFATType(void) const
{
	unsigned int a=bytesPerSector;
	unsigned int b=sectorsPerCluster;
	if(FAT16_SIZE_THRESHOLD<=a*b)
	{
		return FAT16;
	}
	return FAT12;
}

////////////////////////////////////////////////////////////

bool Disk::CreateFD(unsigned int BPB_mediaType)
{
	if(BPB_MEDIA_1232K==BPB_mediaType)
	{
		data.resize(1232*1024);
	}
	else
	{
		std::cout << "Media Type Not Supported Yet." << std::endl;
		return false;
	}
	MakeFDBootSectBPB(data.data(),BPB_mediaType);

	auto bpb=GetBPB();
	MakeInitialFAT(GetFAT());
	MakeInitialFAT(GetBackupFAT());
	MakeInitialRootDir(GetRootDir(),bpb.numRootDirEnt);
	return true;
}

void Disk::MakeFDBootSectBPB(unsigned char sect[],unsigned char mediaType) const
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


bool Disk::CreateHDPartitionByMegaBytes(unsigned int MB)
{
	if(0==MB)
	{
		return false;
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
		return false;
	}


	const size_t reserveSect=1;
	size_t sizeInBytes=MB*1024*1024;
	unsigned int totalSectors=(sizeInBytes/bytesPerSect);
	const size_t numFATs=2;

	data.resize(MB*1024*1024);
	memset(data.data(),0,data.size());
	memcpy(data.data(),"IPL4",4);
	data[4]=I386_RETF;

	unsigned char *sect=data.data();
	WriteWord(sect+BPB_BYTES_PER_SECTOR,bytesPerSect);
	sect[BPB_SECTOR_PER_CLUSTER]=sectorsPerCluster;
	WriteWord(sect+BPB_RESERVED_SECTOR_CT,reserveSect);
	sect[BPB_NUM_FATS]=numFATs;
	WriteWord(sect+BPB_NUM_ROOT_DIR_ENT,rootDirEnt);
		WriteWord(sect+BPB_TOTALNUM_SECT,totalSectors);
	sect[BPB_MEDIA_DESC]=mediaType;
	WriteWord(sect+BPB_SECT_PER_FAT,sectorsPerFAT);
	WriteWord(sect+BPB_SECT_PER_TRACK,16); // 16 for HD
	WriteWord(sect+BPB_NUM_HEADS,1);      // 1 for HD
	WriteWord(sect+BPB_HIDDEN_SECT,0);
	WriteDword(sect+BPB_32BIT_NUM_SECT,0);

	auto bpb=GetBPB();
	MakeInitialFAT(GetFAT());
	MakeInitialFAT(GetBackupFAT());
	MakeInitialRootDir(GetRootDir(),bpb.numRootDirEnt);

	return true;
}

void Disk::MakeInitialFAT(unsigned char FAT[]) const
{
	auto BPB=GetBPB();
	size_t len=GetFATLength();
	for(int i=0; i<len; ++i)
	{
		FAT[i]=0;
	}
	if(FAT12==BPB.GetFATType())
	{
		if(true==isFloppyDisk)
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

void Disk::MakeInitialRootDir(unsigned char rootDir[],unsigned int numEnt) const
{
	size_t bytes=(numEnt<<DIRENT_SHIFT);
	for(int i=0; i<bytes; ++i)
	{
		rootDir[i]=0;
	}
}

void Disk::WriteIPLSector(const std::vector <unsigned char> &ipl)
{
	memcpy(data.data(),ipl.data(),ipl.size());
}

Disk::BPB Disk::GetBPB(void) const
{
	auto sect=data.data();
	BPB bpb;
	bpb.bytesPerSector      =ReadWord(sect+BPB_BYTES_PER_SECTOR);
	bpb.sectorsPerCluster   =sect[BPB_SECTOR_PER_CLUSTER];
	bpb.numReservedSectors  =ReadWord(sect+BPB_RESERVED_SECTOR_CT);
	bpb.numFATs             =sect[BPB_NUM_FATS];
	bpb.numRootDirEnt       =ReadWord(sect+BPB_NUM_ROOT_DIR_ENT);
	bpb.totalNumSectors     =ReadWord(sect+BPB_TOTALNUM_SECT);
	bpb.mediaDesc           =sect[BPB_MEDIA_DESC];
	bpb.sectorsPerFAT       =ReadWord(sect+BPB_SECT_PER_FAT);
	bpb.sectorsPerTrack     =ReadWord(sect+BPB_SECT_PER_TRACK);
	bpb.numHeads            =ReadWord(sect+BPB_NUM_HEADS);
	bpb.numHiddenSectors    =ReadWord(sect+BPB_HIDDEN_SECT);
	bpb.totalNumSectors32bit=ReadDword(sect+BPB_32BIT_NUM_SECT);
	return bpb;
}

size_t Disk::GetFATLength(void) const
{
	auto bpb=GetBPB();
	return bpb.bytesPerSector*bpb.sectorsPerFAT;
}

size_t Disk::GetNumClusters(const BPB &bpb) const
{
	// if(FAT12)
	{
		return 2*(GetFATLength()/3);
	}
}

unsigned char *Disk::GetFAT(void)
{
	auto bpb=GetBPB();
	// FAT is located immediately after reserved sectors.
	size_t pos=bpb.bytesPerSector*bpb.numReservedSectors;
	return data.data()+pos;
}

unsigned char *Disk::GetBackupFAT(void)
{
	auto bpb=GetBPB();
	// Backup FAT is located immediately after the primary FAT.
	size_t pos=bpb.bytesPerSector*(bpb.numReservedSectors+bpb.sectorsPerFAT);
	return data.data()+pos;
}

const unsigned char *Disk::GetFAT(void) const
{
	auto bpb=GetBPB();
	// FAT is located immediately after reserved sectors.
	size_t pos=bpb.bytesPerSector*bpb.numReservedSectors;
	return data.data()+pos;
}

const unsigned char *Disk::GetBackupFAT(void) const
{
	auto bpb=GetBPB();
	// Backup FAT is located immediately after the primary FAT.
	size_t pos=bpb.bytesPerSector*(bpb.numReservedSectors+bpb.sectorsPerFAT);
	return data.data()+pos;
}

unsigned char *Disk::GetRootDir(void)
{
	auto bpb=GetBPB();
	size_t pos=bpb.bytesPerSector*bpb.GetRootDirSector();
	return data.data()+pos;
}
const unsigned char *Disk::GetRootDir(void) const
{
	auto bpb=GetBPB();
	size_t pos=bpb.bytesPerSector*bpb.GetRootDirSector();
	return data.data()+pos;
}

uint32_t Disk::GetFATEntry(const unsigned char FAT[],const BPB &bpb,unsigned int cluster) const
{
	// If total number of clusters (DPB_MAX_CLUSTER_NUM)>0xFF6, take it as FAT16.
	// Can happen if HDD.

	if(FAT12==bpb.GetFATType())
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

void Disk::PutFATEntry(unsigned char FAT[],const BPB &bpb,unsigned int cluster,unsigned int incoming) const
{
	// If total number of clusters (DPB_MAX_CLUSTER_NUM)>0xFF6, take it as FAT16.
	// Can happen if HDD.

	if(FAT12==bpb.GetFATType())
	{
		if(0==(cluster&1))
		{
			uint32_t data;
			data=ReadWord(FAT+(cluster/2)*3);
			data&=0xF000;
			data|=(incoming&0xFFF);
			WriteWord(FAT+(cluster/2)*3,data);
		}
		else
		{
			uint32_t data;
			data=ReadWord(FAT+(cluster/2)*3+1);
			data&=0x000F;
			data|=(incoming<<4);
			WriteWord(FAT+(cluster/2)*3+1,data);
		}
	}
	else
	{
		WriteWord(FAT+cluster*2,incoming);
	}
}

uint32_t Disk::FindAvailableCluster(const unsigned char FAT[],const BPB &bpb) const
{
	for(int i=0; i<GetNumClusters(bpb); ++i)
	{
		auto data=GetFATEntry(FAT,bpb,i);
		if(0==data)
		{
			return i;
		}
	}
	return ~0;
}

unsigned char *Disk::GetCluster(int cluster,const BPB &bpb)
{
	size_t firstDataPos=bpb.bytesPerSector*bpb.GetFirstDataSector();
	if(2<=cluster) // Cluster 2 is real first cluster.
	{
		cluster-=2;
	}
	else
	{
		cluster=0;
	}
	size_t clusterPos=firstDataPos+bpb.GetBytesPerCluster()*cluster;
	return data.data()+clusterPos;
}
void Disk::ClusterToCHR(unsigned char CHR[3],int cluster) const
{
	CHR[0]=0;
	CHR[1]=0;
	CHR[2]=0;

	auto bpb=GetBPB();

	size_t firstDataPos=bpb.bytesPerSector*bpb.GetFirstDataSector();
	if(2<=cluster) // Cluster 2 is real first cluster.
	{
		cluster-=2;
	}
	else
	{
		cluster=0;
	}
	size_t clusterPos=firstDataPos+bpb.GetBytesPerCluster()*cluster;

	if(0<bpb.bytesPerSector && 0<bpb.sectorsPerTrack)
	{
		size_t lba=clusterPos/bpb.bytesPerSector;
		size_t track=lba/bpb.sectorsPerTrack;
		CHR[0]=track/2; // CYLINDER
		CHR[1]=track&1; // HEAD
		CHR[2]=lba%bpb.sectorsPerTrack;
	}
}
const unsigned char *Disk::GetCluster(int cluster,const BPB &bpb) const
{
	size_t firstDataPos=bpb.bytesPerSector*bpb.GetFirstDataSector();
	if(2<=cluster) // Cluster 2 is real first cluster.
	{
		cluster-=2;
	}
	else
	{
		cluster=0;
	}
	size_t clusterPos=firstDataPos+bpb.GetBytesPerCluster()*cluster;
	return data.data()+clusterPos;
}

unsigned char *Disk::FindAvailableDirEnt(void)
{
	auto bpb=GetBPB();
	auto rootDir=GetRootDir();
	size_t dirEntSize=(1<<DIRENT_SHIFT);
	for(int i=0; i<bpb.numRootDirEnt; ++i)
	{
		if(0==*rootDir)
		{
			return rootDir;
		}
		rootDir+=dirEntSize;
	}
	return nullptr;
}

void Disk::WriteDirEnt(
	    unsigned char *dirEnt,std::string file,std::string ext,
	    uint8_t attr,
	    unsigned int hour,unsigned int min,unsigned int sec,
	    unsigned int year,unsigned int month,unsigned int day,
	    unsigned int firstCluster,
	    unsigned int fileSize)
{
	for(int i=0; i<8; ++i)
	{
		dirEnt[DIRENT_FILENAME+i]=toupper(file[i]);
	}
	for(int i=0; i<3; ++i)
	{
		dirEnt[DIRENT_EXT+i]=toupper(ext[i]);
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

unsigned int Disk::WriteData(const std::vector <unsigned char> &data)
{
	auto bpb=GetBPB();
	size_t pos=0;
	unsigned int prevCluster=0,firstCluster=NULL_CLUSTER;
	while(pos<data.size())
	{
		size_t writeSize=std::min(data.size()-pos,bpb.GetBytesPerCluster());
		auto cluster=FindAvailableCluster(GetFAT(),bpb);
		if(cluster!=NULL_CLUSTER)
		{
			if(0==pos)
			{
				firstCluster=cluster;
			}
			else
			{
				PutFATEntry(GetFAT(),bpb,prevCluster,cluster);
				PutFATEntry(GetBackupFAT(),bpb,prevCluster,cluster);
			}
			prevCluster=cluster;

			PutFATEntry(GetFAT(),bpb,cluster,0xFFFF);
			PutFATEntry(GetBackupFAT(),bpb,cluster,0xFFFF);

			auto ptr=GetCluster(cluster,bpb);
			memcpy(ptr,data.data()+pos,writeSize);
		}
		else
		{
			break;
		}
		pos+=writeSize;
	}
	return firstCluster;
}

