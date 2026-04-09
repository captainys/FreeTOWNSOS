#include <stdio.h>
#include <iostream>
#include <fstream>
#include <vector>
#include <stdint.h>
#include <ctype.h>

#include "dosdisk.h"



////////////////////////////////////////////////////////////

Disk::Disk()
{
	DOSDISK_Init(&disk);
}

bool Disk::CreateFD(unsigned int BPB_mediaType)
{
	size_t dataLen=DOSDISK_GetRequiredBytesFD(BPB_mediaType);

	if(0==dataLen)
	{
		std::cout << "Media Type Not Supported Yet." << std::endl;
		return false;
	}

	data.resize(dataLen);
	auto err=DOSDISK_CreateFD(&disk,BPB_mediaType,data.size(),data.data());
	if(0!=err)
	{
		std::cout << "Failed to create a FD image.\n";
		return false;
	}

	return true;
}

bool Disk::CreateHDPartitionByMegaBytes(unsigned int MB)
{
	if(0==MB)
	{
		return false;
	}

	data.resize(MB*1024*1024);

	return DOSDISK_NOERR==DOSDISK_CreateHDPartitionByMegaBytes(&disk,MB,data.size(),data.data());
}

void Disk::MakeInitialFAT(unsigned char FAT[]) const
{
	DOSDISK_MakeInitialFAT(&disk,FAT);
}

void Disk::MakeInitialRootDir(unsigned char rootDir[],unsigned int numEnt) const
{
	DOSDISK_MakeInitialRootDir(&disk,rootDir,numEnt);
}

void Disk::WriteIPLSector(const std::vector <unsigned char> &ipl)
{
	memcpy(data.data(),ipl.data(),ipl.size());
}

size_t Disk::GetFATLength(void) const
{
	auto bpb=DOSDISK_GetBPB(&disk);
	return BPB_GetFATLength(&bpb);
}

unsigned char *Disk::GetFAT(void)
{
	return DOSDISK_GetFAT(&disk);
}

unsigned char *Disk::GetBackupFAT(void)
{
	return DOSDISK_GetBackupFAT(&disk);
}

const unsigned char *Disk::GetFAT(void) const
{
	return DOSDISK_GetFAT(&disk);
}

const unsigned char *Disk::GetBackupFAT(void) const
{
	return DOSDISK_GetBackupFAT(&disk);
}

unsigned char *Disk::GetRootDir(void)
{
	return DOSDISK_GetRootDir(&disk);
}
const unsigned char *Disk::GetRootDir(void) const
{
	return DOSDISK_GetRootDir(&disk);
}

uint32_t Disk::GetFATEntry(const unsigned char FAT[],const BPB &bpb,unsigned int cluster) const
{
	return DOSDISK_GetFATEntry(&disk,FAT,&bpb,cluster);
}

void Disk::PutFATEntry(unsigned char FAT[],const BPB &bpb,unsigned int cluster,unsigned int incoming) const
{
	DOSDISK_PutFATEntry(&disk,FAT,&bpb,cluster,incoming);
}

uint32_t Disk::FindAvailableCluster(const unsigned char FAT[],const BPB &bpb) const
{
	for(int i=0; i<BPB_GetNumClusters(&bpb); ++i)
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
	return data.data()+clusterPos;
}
void Disk::ClusterToCHR(unsigned char CHR[3],int cluster) const
{
	CHR[0]=0;
	CHR[1]=0;
	CHR[2]=0;

	auto bpb=DOSDISK_GetBPB(&disk);

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
const unsigned char *Disk::GetCluster(int cluster,const BPB &bpb) const
{
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
	return data.data()+clusterPos;
}

unsigned char *Disk::FindAvailableDirEnt(void)
{
	auto bpb=DOSDISK_GetBPB(&disk);
	auto rootDir=DOSDISK_GetRootDir(&disk);
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
	auto bpb=DOSDISK_GetBPB(&disk);
	size_t pos=0;
	unsigned int prevCluster=0,firstCluster=NULL_CLUSTER;
	while(pos<data.size())
	{
		size_t writeSize=std::min(data.size()-pos,BPB_GetBytesPerCluster(&bpb));
		auto cluster=FindAvailableCluster(DOSDISK_GetFAT(&disk),bpb);
		if(cluster!=NULL_CLUSTER)
		{
			if(0==pos)
			{
				firstCluster=cluster;
			}
			else
			{
				PutFATEntry(DOSDISK_GetFAT(&disk),bpb,prevCluster,cluster);
				PutFATEntry(DOSDISK_GetBackupFAT(&disk),bpb,prevCluster,cluster);
			}
			prevCluster=cluster;

			PutFATEntry(DOSDISK_GetFAT(&disk),bpb,cluster,0xFFFF);
			PutFATEntry(DOSDISK_GetBackupFAT(&disk),bpb,cluster,0xFFFF);

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

