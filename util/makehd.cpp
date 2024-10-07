#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <iostream>
#include <fstream>
#include <vector>
#include <stdint.h>
#include <ctype.h>

#include "dosdisk.h"

std::vector <unsigned char> ReadBinaryFile(std::string fileName)
{
	std::vector <unsigned char> data;
	std::ifstream ifp(fileName,std::ios::binary);
	if(true==ifp.is_open())
	{
		ifp.seekg(0,std::ios::end);
		auto sz=ifp.tellg();
		ifp.seekg(0,std::ios::beg);

		data.resize(sz);
		ifp.read((char *)data.data(),data.size());
	}
	return data;
}

class CommandParameterInfo
{
public:
	class InputFile
	{
	public:
		std::string fileName;
		int partition;
	};
	class Partition
	{
	public:
		std::string label;
		int sizeInMB;
	};

	std::vector <Partition> partitions;
	std::vector <std::string> outFile;
	std::vector <InputFile> inFile;
	std::string MBRFile,IPLFile;

	bool RecognizeCommandParameter(int ac,char *av[])
	{
		if(ac<=1)
		{
			return false;
		}

		for(int i=1; i<ac; ++i)
		{
			std::string opt(av[i]);
			for(auto &c : opt)
			{
				c=tolower(c);
			}
			if("-h"==opt || "-help"==opt || "-?"==opt)
			{
				Help();
			}
			else if("-o"==opt || "-out"==opt)
			{
				if(i+1<ac)
				{
					outFile.push_back(av[i+1]);
					++i;
				}
				else
				{
					std::cout << "Missing argument for -o option.\n";
					return false;
				}
			}
			else if("-i"==opt || "-in"==opt)
			{
				if(i+2<ac)
				{
					InputFile inp;
					inp.partition=atoi(av[i+1]);
					inp.fileName=av[i+2];
					inFile.push_back(inp);
					i+=2;
				}
 				else
				{
					std::cout << "Missing argument for -i option.\n";
					return false;
				}
			}
			else if("-p"==opt || "-part"==opt)
			{
				if(i+2<ac)
				{
					Partition p;
					p.sizeInMB=atoi(av[i+1]);
					p.label=av[i+2];
					partitions.push_back(p);
					i+=2;
				}
				else
				{
					std::cout << "Missing argument for -p option.\n";
					return false;
				}
			}
			else if("-mbr"==opt)
			{
				if(i+1<ac)
				{
					if(""!=MBRFile)
					{
						std::cout << "-ipl or -bootsect option is specified multiple times.\n";
						return false;
					}
					MBRFile=av[i+1];
					++i;
				}
				else
				{
					std::cout << "Missing argument for -ipl option.\n";
					return false;
				}
			}
			else if("-ipl"==opt || "-bootsect"==opt)
			{
				if(i+1<ac)
				{
					if(""!=IPLFile)
					{
						std::cout << "-ipl or -bootsect option is specified multiple times.\n";
						return false;
					}
					IPLFile=av[i+1];
					++i;
				}
				else
				{
					std::cout << "Missing argument for -ipl option.\n";
					return false;
				}
			}
			else
			{
				std::cout << "Unrecognized option " << opt << "\n";
				return false;
			}
		}
		return true;
	}

	static void Help(void)
	{
		std::cout << "-h, -help, -?\n";
		std::cout << "  Print this message.\n";
		std::cout << "-o outfile.bin\n";
		std::cout << "-out outfile.bin\n";
		std::cout << "  Specify output file.\n";
		std::cout << "  If no output file is specified, this program does nothing.\n";
		std::cout << "  If multiple output files are specfied, this program makes multiple disk images.\n";
		std::cout << "-p sizeInMB label\n";
		std::cout << "-part sizeInMB label\n";
		std::cout << "  Partition.  Label will be used as a partition name and volume label.\n";
		std::cout << "-i partition input\n";
		std::cout << "-in partition input-file\n";
		std::cout << "  Specify input file.\n";
		std::cout << "  If no output file is specified, this program makes an empty disk.\n";
		std::cout << "  Specify partition where the file is written to.\n";
		std::cout << "-mbr mbr-image-file\n";
		std::cout << "  Specify MBR-image file. (Written to the first sector of the HDD image)\n";
		std::cout << "-ipl ipl-image-file\n";
		std::cout << "  Specify IPL-image file. (Written to the first sector of each partition)\n";
	}
};



#define HD_PHYSICAL_SECTOR_SIZE 512
#define HD_MAX_NUM_PARTITIONS    10

#define PARTITION_TYPE_UNUSED     0
#define PARTITION_TYPE_MSDOS      1

class HardDisk
{
public:
	class Partition
	{
	public:
		std::string label;
		std::string volumeLabel;
		Disk disk;
	};

	std::vector <unsigned char> MBR_PartTable;
	std::vector <Partition> partitions;

	unsigned int bootPartition=0;

	HardDisk();
	unsigned char *GetPartitionTable(void);
	const unsigned char *GetPartitionTable(void) const;

	bool AddPartition(size_t sizeInMB,std::string label,std::string volumeLabel);
	std::vector <unsigned char> GenerateHDImage(std::string MBRFile,std::string IPLFile) const;
};

unsigned char *HardDisk::GetPartitionTable(void)
{
	return MBR_PartTable.data()+512;
}

const unsigned char *HardDisk::GetPartitionTable(void) const
{
	return MBR_PartTable.data()+512;
}

HardDisk::HardDisk()
{
	MBR_PartTable.resize(3*HD_PHYSICAL_SECTOR_SIZE);
	memset(MBR_PartTable.data(),0,MBR_PartTable.size());
	// Now GetPartitionTable is usable.

	MBR_PartTable[0]='I';
	MBR_PartTable[1]='P';
	MBR_PartTable[2]='L';
	MBR_PartTable[3]='4';
	MBR_PartTable[4]=I386_RETF;

	unsigned char *partTable=GetPartitionTable();
	partTable[0]=0x95; // "Fujitsu" in Kanji letters Shift-JIS encoding.
	partTable[1]=0x78;
	partTable[2]=0x8E;
	partTable[3]=0x6D;
	partTable[4]=0x92;
	partTable[5]=0xCA;

	WriteDword(partTable+6,3); // Probably sector LBA of the first partition.

	WriteWord(partTable+0x0E,0x200); // Mystery magic number.

	for(int i=0; i<10; ++i)
	{
		memset(partTable+0x20+i*0x30,0,16);
		memset(partTable+0x30+i*0x30,0x20,32);
	}
}

bool HardDisk::AddPartition(size_t sizeInMB,std::string label,std::string volumeLabel)
{
	if(HD_MAX_NUM_PARTITIONS<=partitions.size())
	{
		return false;
	}

	auto partitionTablePtr=GetPartitionTable();
	auto partitionPtr=partitionTablePtr+0x20+0x30*partitions.size();
	if(0==partitions.size())
	{
		partitionPtr[0]=0xFF; // Boot partition
	}
	else
	{
		partitionPtr[0]=0; // Not Boot partition
	}
	partitionPtr[1]=PARTITION_TYPE_MSDOS;
	partitionPtr[2]=3;
	WriteDword(partitionPtr+6,(sizeInMB*1024*1024)/HD_PHYSICAL_SECTOR_SIZE);
	memset(partitionPtr+16,0x20,32);
	memcpy(partitionPtr+16,"MS-DOS",6);
	for(int i=0; i<16 && i<label.size(); ++i)
	{
		partitionPtr[32+i]=toupper(label[i]);
	}


	Partition p;
	partitions.push_back(p);

	partitions.back().disk.isFloppyDisk=false;  // Do it before FAT is made by CreateHDPartitionByMegaBytes.
	partitions.back().label=label;
	partitions.back().volumeLabel=volumeLabel;
	if(true!=partitions.back().disk.CreateHDPartitionByMegaBytes(sizeInMB))
	{
		partitions.pop_back();
		return false;
	}

	{
		auto dirEnt=partitions.back().disk.FindAvailableDirEnt();
		std::string dos8,ext;
		if(nullptr!=dirEnt)
		{
			partitions.back().disk.WriteDirEnt(
				dirEnt,
			   "AOMORIKE",
			   "N  ",
			   DIRENT_ATTR_VOLLABEL|DIRENT_ATTR_ARCHIVE,
			   20,14,00,
			   2024,10,6,
			   0,
			   0);
		}
	}

	size_t totalSectors=3; // MBR+PartitionTable
	for(auto &p : partitions)
	{
		totalSectors+=(p.disk.data.size()/HD_PHYSICAL_SECTOR_SIZE);
	}
	WriteDword(partitionTablePtr+0x0A,totalSectors);

	return true;
}

std::vector <unsigned char> HardDisk::GenerateHDImage(std::string MBRFile,std::string IPLFile) const
{
	std::vector <unsigned char> data;
	data.insert(data.end(),MBR_PartTable.begin(),MBR_PartTable.end());

	if(""!=MBRFile)
	{
		auto MBR=ReadBinaryFile(MBRFile);
		if(MBR.size()<HD_PHYSICAL_SECTOR_SIZE)
		{
			for(int i=0; i<MBR.size(); ++i)
			{
				data[i]=MBR[i];
			}
		}
		else
		{
			std::cout << "MBR sector image exceeds physical sector size.\n";
			data.clear();
			return data;
		}
	}

	for(auto &p : partitions)
	{
		data.insert(data.end(),p.disk.data.begin(),p.disk.data.end());
	}

	return data;
}


int MakeHardDiskImage(const CommandParameterInfo &cpi)
{
	HardDisk hd;
	for(auto &p : cpi.partitions)
	{
		if(true!=hd.AddPartition(p.sizeInMB,p.label,p.label))
		{
			std::cout << "Failed to create a partition.\n";
			return 1;
		}
	}

	for(auto in : cpi.inFile)
	{
		auto &disk=hd.partitions[in.partition].disk;
		auto dirEnt=disk.FindAvailableDirEnt();
		auto file=ReadBinaryFile(in.fileName);
		if(0==file.size())
		{
			std::cout << "Cannot open input file: " << in.fileName << "\n";
			return 1;
		}

		if(hd.partitions.size()<=in.partition)
		{
			std::cout << "Destination partition does not exist.\n";
			return 1;
		}

		char DOS8PLUS3[11];
		for(auto &c : DOS8PLUS3)
		{
			c=' ';
		}
		int strPtr=in.fileName.size();
		while(0<strPtr && '/'!=in.fileName[strPtr] && '\\'!=in.fileName[strPtr] && ':'!=in.fileName[strPtr])
		{
			--strPtr;
		}
		if('/'==in.fileName[strPtr] || '\\'==in.fileName[strPtr] || ':'==in.fileName[strPtr])
		{
			++strPtr;
		}
		for(int i=0; i<8 && strPtr<in.fileName.size() && '.'!=in.fileName[strPtr]; ++i,++strPtr)
		{
			DOS8PLUS3[i]=toupper(in.fileName[strPtr]);
		}
		if('.'==in.fileName[strPtr])
		{
			++strPtr;
		}
		for(int i=0; i<3 && strPtr<in.fileName.size() && '.'!=in.fileName[strPtr]; ++i,++strPtr)
		{
			DOS8PLUS3[8+i]=toupper(in.fileName[strPtr]);
		}

		for(auto &c : DOS8PLUS3)
		{
			std::cout << c;
		}
		std::cout << "\n";

		auto firstCluster=disk.WriteData(file);
		if(nullptr!=dirEnt)
		{
			disk.WriteDirEnt(
				dirEnt,
			   DOS8PLUS3,
			   DOS8PLUS3+8,
			   DIRENT_ATTR_READONLY|DIRENT_ATTR_ARCHIVE /*|DIRENT_ATTR_SYSTEM*/,
			   23,42,00,
			   2024,8,28,
			   firstCluster,
			   file.size());
		}
	}

	auto img=hd.GenerateHDImage(cpi.MBRFile,cpi.IPLFile);
	if(0==img.size())
	{
		return 1;
	}
	for(auto outFile : cpi.outFile)
	{
		std::ofstream ofp(outFile,std::ios::binary);
		ofp.write((char *)img.data(),img.size());
	}
	return 0;
}

int main(int ac,char *av[])
{
	CommandParameterInfo cpi;
	if(true!=cpi.RecognizeCommandParameter(ac,av))
	{
		cpi.Help();
		return 1;
	}
	return MakeHardDiskImage(cpi);
}
