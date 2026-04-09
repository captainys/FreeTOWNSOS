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

uint32_t Disk::GetFATEntry(const unsigned char FAT[],unsigned int cluster) const
{
	return DOSDISK_GetFATEntry(&disk,FAT,cluster);
}

void Disk::PutFATEntry(unsigned char FAT[],unsigned int cluster,unsigned int incoming) const
{
	DOSDISK_PutFATEntry(&disk,FAT,cluster,incoming);
}

uint32_t Disk::FindAvailableCluster(const unsigned char FAT[]) const
{
	return DOSDISK_FindAvailableCluster(&disk,FAT);
}

unsigned char *Disk::GetCluster(int cluster)
{
	return DOSDISK_GetCluster(&disk,cluster);
}
void Disk::ClusterToCHR(unsigned char CHR[3],int cluster) const
{
	DOSDISK_ClusterToCHR(&disk,CHR,cluster);
}

const unsigned char *Disk::GetCluster(int cluster) const
{
	return DOSDISK_GetCluster(&disk,cluster);
}

unsigned char *Disk::FindAvailableDirEnt(void)
{
	return DOSDISK_FindAvailableDirEnt(&disk);
}

void Disk::WriteDirEnt(
	    unsigned char *dirEnt,std::string file,std::string ext,
	    uint8_t attr,
	    unsigned int hour,unsigned int min,unsigned int sec,
	    unsigned int year,unsigned int month,unsigned int day,
	    unsigned int firstCluster,
	    unsigned int fileSize)
{
	DOSDISK_WriteDirEnt(dirEnt,file.c_str(),ext.c_str(),attr,hour,min,sec,year,month,day,firstCluster,fileSize);
}

unsigned int Disk::WriteData(const std::vector <unsigned char> &data)
{
	return DOSDISK_WriteData(&disk,data.size(),data.data());
}

