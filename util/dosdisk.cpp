#include <stdio.h>
#include <iostream>
#include <fstream>
#include <vector>
#include <stdint.h>
#include <ctype.h>



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



#define DIRENT_SHIFT			5    // 32 bytes per dirent

#define NULL_CLUSTER 0xFFFFFFFF

#define I386_RETF              0xCB


static void WriteWord(unsigned char *ptr,unsigned short data)
{
	*(uint16_t *)ptr=data;
}

static uint16_t ReadWord(const unsigned char *ptr)
{
	return *(uint16_t *)ptr;
}

static void WriteDword(unsigned char *ptr,unsigned short data)
{
	*(uint32_t *)ptr=data;
}

static unsigned short ReadDword(const unsigned char *ptr)
{
	return *(uint32_t *)ptr;
}

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
	};

	std::vector <unsigned char> data;

	bool Create(unsigned int BPB_mediaType);
	void MakeBootSectBPB(unsigned char sect[],unsigned char mediaType) const;
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

bool Disk::Create(unsigned int BPB_mediaType)
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
	MakeBootSectBPB(data.data(),BPB_mediaType);

	auto bpb=GetBPB();
	MakeInitialFAT(GetFAT());
	MakeInitialFAT(GetBackupFAT());
	MakeInitialRootDir(GetRootDir(),bpb.numRootDirEnt);
	return true;
}

void Disk::MakeBootSectBPB(unsigned char sect[],unsigned char mediaType) const
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
		WriteWord(sect+BPB_TOTALNUM_SECT,0x4D0);
		sect[BPB_MEDIA_DESC]=mediaType;
		WriteWord(sect+BPB_SECT_PER_FAT,2);
		WriteWord(sect+BPB_SECT_PER_TRACK,8);
		WriteWord(sect+BPB_NUM_HEADS,2);
		WriteWord(sect+BPB_HIDDEN_SECT,0);
		WriteDword(sect+BPB_32BIT_NUM_SECT,0);
	}
}

void Disk::MakeInitialFAT(unsigned char FAT[]) const
{
	auto BPB=GetBPB();
	size_t len=GetFATLength();
	for(int i=0; i<len; ++i)
	{
		FAT[i]=0;
	}
	FAT[0]=0xFE;  // If HDD, FA FF FF FF.
	FAT[1]=0xFF;
	FAT[2]=0xFF;
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

	// if(FAT12)
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
}

void Disk::PutFATEntry(unsigned char FAT[],const BPB &bpb,unsigned int cluster,unsigned int incoming) const
{
	// If total number of clusters (DPB_MAX_CLUSTER_NUM)>0xFF6, take it as FAT16.
	// Can happen if HDD.

	// if(FAT12)
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

class CommandParameterInfo
{
public:
	std::vector <std::string> outFile;
	std::vector <std::string> inFile;
	std::string IPLFile;

	bool RecognizeCommandParameter(int ac,char *av[])
	{
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
				if(i+1<ac)
				{
					inFile.push_back(av[i+1]);
					++i;
				}
				else
				{
					std::cout << "Missing argument for -i option.\n";
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
		std::cout << "-i input\n";
		std::cout << "-in input-file\n";
		std::cout << "  Specify input file.\n";
		std::cout << "  If no output file is specified, this program makes an empty disk.\n";
		std::cout << "-ipl ipl-image-file\n";
		std::cout << "  Specify IPL-image file.\n";
	}
};

bool MakeDisk(std::string outFile,const CommandParameterInfo &cpi)
{
	Disk disk;
	disk.Create(BPB_MEDIA_1232K);

	if(""!=cpi.IPLFile)
	{
		auto file=ReadBinaryFile(cpi.IPLFile);
		if(0==file.size())
		{
			std::cout << "Cannot open the IPL image: "<< cpi.IPLFile << "\n";
			return false;
		}
		disk.WriteIPLSector(file);
	}

	std::cout << "Making: " << outFile << "\n";

	auto bpb=disk.GetBPB();

	{
		auto dirEnt=disk.FindAvailableDirEnt();
		if(nullptr!=dirEnt)
		{
			disk.WriteDirEnt(
				dirEnt,
			   "AOMORIKE",
			   "N  ",
			   DIRENT_ATTR_VOLLABEL|DIRENT_ATTR_ARCHIVE,
			   23,42,00,
			   2024,8,28,
			   0,
			   0);
		}
	}

	for(auto inFile : cpi.inFile)
	{
		auto dirEnt=disk.FindAvailableDirEnt();
		auto file=ReadBinaryFile(inFile);
		if(0==file.size())
		{
			std::cout << "Cannot open input file: " << inFile << "\n";
			return false;
		}

		char DOS8PLUS3[11];
		for(auto &c : DOS8PLUS3)
		{
			c=' ';
		}
		int strPtr=inFile.size();
		while(0<strPtr && '/'!=inFile[strPtr] && '\\'!=inFile[strPtr] && ':'!=inFile[strPtr])
		{
			--strPtr;
		}
		if('/'==inFile[strPtr] || '\\'==inFile[strPtr] || ':'==inFile[strPtr])
		{
			++strPtr;
		}
		for(int i=0; i<8 && strPtr<inFile.size() && '.'!=inFile[strPtr]; ++i,++strPtr)
		{
			DOS8PLUS3[i]=toupper(inFile[strPtr]);
		}
		if('.'==inFile[strPtr])
		{
			++strPtr;
		}
		for(int i=0; i<3 && strPtr<inFile.size() && '.'!=inFile[strPtr]; ++i,++strPtr)
		{
			DOS8PLUS3[8+i]=toupper(inFile[strPtr]);
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

	std::ofstream ofp(outFile,std::ios::binary);
	ofp.write((char *)disk.data.data(),disk.data.size());

	return true;
}

int main(int ac,char *av[])
{
	CommandParameterInfo cpi;
	if(true!=cpi.RecognizeCommandParameter(ac,av))
	{
		std::cout << "Error in the command parameter(s)\n";
		CommandParameterInfo::Help();
		return 1;
	}

	for(auto &o : cpi.outFile)
	{
		MakeDisk(o,cpi);
	}

	return 0;
}
