#include <list>
#include <string>
#include <array>
#include <fstream>
#include <iostream>
#include <algorithm>
#include <map>
#include <unordered_set>
#include <unordered_map>

#include <ctype.h>
#include <stdint.h>
#include <assert.h>
#include <time.h>

#include "iso9660.h"

class ISOImage
{
public:
	enum ENDIANNESS
	{
		LITTLE_ENDIAN,
		BIG_ENDIAN
	};
	enum {
		CD_SECTOR_SIZE=2048,
	};

	static void PutDword(unsigned char *ptr,ENDIANNESS endian,uint32_t i)
	{
		if(LITTLE_ENDIAN==endian)
		{
			ptr[0]=i;
			ptr[1]=i>>8;
			ptr[2]=i>>16;
			ptr[3]=i>>24;
		}
		else
		{
			ptr[3]=i;
			ptr[2]=i>>8;
			ptr[1]=i>>16;
			ptr[0]=i>>24;
		}
	}
	static void PutWord(unsigned char *ptr,ENDIANNESS endian,uint16_t i)
	{
		if(LITTLE_ENDIAN==endian)
		{
			ptr[0]=i;
			ptr[1]=i>>8;
		}
		else
		{
			ptr[1]=i;
			ptr[0]=i>>8;
		}
	}


	class FileBase
	{
	public:
		std::string nameInISO;
		unsigned int attrib=0;
		size_t LBA=0;
		size_t len=0;
	};

	class File : public FileBase
	{
	public:
		size_t dirIndex; // Index to allDirList;
		std::string input;
		size_t CalculateDirSize(void) const;
	};
	class Dir : public FileBase
	{
	public:
		size_t ownIndex=~0;
		size_t parentDirIndex=~0;
		size_t IDinISO=~0;
		std::vector <size_t> fileList;	// Indexes to allFileList
		std::vector <size_t> subDirList;	// Indexes to allDirList.  Not id member variable.  It is index.
		size_t CalculateDirSize(void) const;
		size_t MakeISO9660Directory(struct ISO9660_Directory &dir) const;
	};

	bool caseSensitive=false;
	std::string systemLabel="TSUGARU OS";
	std::string volumeLabel="TSUGARU VOLUME";
	std::vector <File> allFileList;
	std::vector <Dir> allDirList;		// allDirList[0] is always the root directory.
	std::unordered_map <std::string,size_t> nameToDirIdx;   // "ABC/XYZ" -> index
	std::vector <unsigned char> IPLSector;

	// Steps:
	//   (1) Add files.
	//   (2) Make directory structure.
	//   (3) Calculate number of sectors for each.
	//   (4) Place elements.

	// LBA 16  Primary Volume Descriptor
	// LBA 17  Last Volume Descriptor
	// LBA 18  Path Table LSB
	// LBA ?   Path Table MSB
	// LBA ?   Root Diectory
	// LBA ?   Directory
	// LBA ?   Directory
	// LBA ?      :
	// LBA ?   File
	// LBA ?   File
	// LBA ?      :

	const unsigned int LBA_PVD=16;
	const unsigned int LBA_LAST_VD=17;
	const unsigned int LBA_PATHTABLE_LE=18;
	unsigned int LBA_PATHTABLE_BE=0;
	unsigned int LBA_FIRST_DIRECTORY=0;
	unsigned int totalSectorCount=0;


	ISOImage();

	// (1)
	bool LoadIPL(std::string file);
	bool AddFileSimple(std::string file); // As is.  If "C:/abc/xyz.txt" is given, "Will be made /XYZ.TXT in ISO."
	bool AddFile(std::string fileIn,std::string fileInISO);

	// (2)
	template <class T>
	static bool Compare(const std::pair<std::string,T> &a,const std::pair<std::string,T> &b)
	{
		return a.first<b.first;
	}
	void SortFiles(void);
	void MakeDirectoryList(void);

	// (3)
	size_t CalculatePathTableSize(void) const;

	void CalculatePathTableLBA(void);

	void CalculateDirFileLBA(void);


	// (4)
	bool MakeISOImageFile(std::string fileName) const;
	size_t WriteToFile(std::ofstream &ofp,size_t len,const void *data) const;
	bool VerifyFilePosLBA(size_t filePos,size_t LBA) const;

	static struct ISO9660_DateTime GetTodaysDateGMT(void);

	std::vector <unsigned char> MakeDescriptorTable(void) const;

	// MakePathTable can be used before setting LBAs for the directories and files for calculating the size of the path table.
	// In which case, all LBA will be filled as ~0.
	// Observation of some ISO images suggests that the index in the path table starts with 1 (root Dir)
	// Root-dir name length is 1.  But, the value in the name is \0.
	std::vector <unsigned char> MakePathTable(ENDIANNESS endian) const;

	std::vector <unsigned char> MakeDirectoryBinary(const Dir &dir) const;

	std::vector <unsigned char> MakeFileBinary(const File &file) const;

	// Capitalize if caseSensitive==false.
	// Backslash to slash.
	// If the first is '/', remove.
	// If the last is '/', remove.
	// If '/' or '\\' is given, returns "".
	// If the drive letter is present, remove.
	std::string NormalizeFileName(std::string fileNameIn) const;

	// "ABC/XYZ/DATA.BIN" -> [0]="ABC/XYZ", [1]="DATA.BIN"
	static std::array <std::string,2> SeparatePath(std::string path);

	// "ABC/XYZ","DATA.BIN" -> "ABC/XYZ/DATA.BIN
	static std::string Join(std::string dir,std::string name);

	bool FileExists(std::string name) const;
	size_t GetFileLength(std::string name) const;

	std::string GetTodayString(void) const;

	static void ErrorMessage(std::string func,unsigned int lineNum,std::string message);

	void Print(void) const;
	void PrintDirs(void) const;
	void PrintFiles(void) const;
};

size_t ISOImage::File::CalculateDirSize(void) const
{
	auto sep=SeparatePath(nameInISO);
	size_t len=33+sep[1].size()+2; // +2 for ";1"
	return (len+1)&~1;  // Always 2*N
}
size_t ISOImage::Dir::CalculateDirSize(void) const
{
	auto sep=SeparatePath(nameInISO);
	size_t len=33+sep[1].size(); // Directory does not have ";1"
	return (len+1)&~1;  // Always 2*N
}
size_t ISOImage::Dir::MakeISO9660Directory(struct ISO9660_Directory &dir) const
{
	auto sep=SeparatePath(nameInISO);
	size_t nameLen=0;
	char *name=dir.name;
	if(0==sep[1].size())
	{
		nameLen=1;
		name[0]=0;
		name[1]=0;
	}
	else
	{
		nameLen=sep[1].size();
		memcpy(name,sep[1].c_str(),sep[1].size());
	}

	size_t sizeInDisc=(len+CD_SECTOR_SIZE-1);
	sizeInDisc/=CD_SECTOR_SIZE;
	sizeInDisc*=CD_SECTOR_SIZE;

	size_t dirLen=(33+nameLen+1)&~1;
	dir.len=dirLen;
	dir.extAtt=0;
	PutDword(dir.LBA_LE,LITTLE_ENDIAN,LBA);
	PutDword(dir.LBA_BE,BIG_ENDIAN,LBA);
	PutDword(dir.sizeLE,LITTLE_ENDIAN,sizeInDisc);
	PutDword(dir.sizeBE,BIG_ENDIAN,sizeInDisc);
	// Leave dateTime zero.
	memset(&dir.dateTime,0,sizeof(dir.dateTime));
	dir.flags=ISO9660_FLAG_DIRECTORY;
	dir.unitSizeForInterleave=0; // Keep it zero.
	dir.interleaveGAP=0;         // Keep it zero.
	PutWord(dir.volumeSeqLE,LITTLE_ENDIAN,1);
	PutWord(dir.volumeSeqBE,BIG_ENDIAN,1);
	dir.nameLEN=nameLen;

	return dirLen;
}

ISOImage::ISOImage()
{
	IPLSector.resize(CD_SECTOR_SIZE);
	memset(IPLSector.data(),0,CD_SECTOR_SIZE);
}

bool ISOImage::LoadIPL(std::string file)
{
	std::ifstream ifp(file,std::ios::binary);
	if(true!=ifp.is_open())
	{
		ErrorMessage(__FUNCTION__,__LINE__,"Cannot open IPL sector image.\n");
		return false;
	}

	ifp.seekg(0,std::ios::end);
	size_t sz=ifp.tellg();
	if(CD_SECTOR_SIZE<sz)
	{
		ErrorMessage(__FUNCTION__,__LINE__,"IPL sector image is larger than the sector length.\n");
		return false;
	}
	ifp.seekg(0,std::ios::beg);
	ifp.read((char *)IPLSector.data(),sz);
	return true;
}

bool ISOImage::AddFileSimple(std::string file)
{
	if(true!=FileExists(file))
	{
		std::cout << "Cannot open " << file << "\n";
		return false;
	}

	auto sep=SeparatePath(NormalizeFileName(file));

	File f;
	f.input=file;
	f.len=GetFileLength(file);
	f.nameInISO=sep[1];
	allFileList.push_back(f);

	return true;
}

bool ISOImage::AddFile(std::string fileIn,std::string fileInISO)
{
	if(true!=FileExists(fileIn))
	{
		std::cout << "Cannot open " << fileIn << "\n";
		return false;
	}

	File f;
	f.input=fileIn;
	f.len=GetFileLength(fileIn);
	f.nameInISO=NormalizeFileName(fileInISO);
	allFileList.push_back(f);

	return true;
}

void ISOImage::SortFiles(void)
{
	std::pair <std::string,File> sort;
	std::vector <std::pair <std::string,File> > files;
	for(auto &f : allFileList)
	{
		auto sep=SeparatePath(f.nameInISO);
		std::pair <std::string,File> pair;
		pair.first=sep[1];
		pair.second=f;
		files.push_back(pair);
	}
	std::sort(files.begin(),files.end(),Compare<File>);

	allFileList.clear();
	for(auto &f : files)
	{
		allFileList.push_back(f.second);
	}
}

void ISOImage::MakeDirectoryList(void)
{
	std::unordered_set <std::string> visited;
	std::vector <std::pair <std::string,std::string> > dirs;
	for(auto &f : allFileList)
	{
		auto sep=SeparatePath(f.nameInISO);
		auto dirName=NormalizeFileName(sep[0]);
		// sep[0] is the directory name.
		std::string partial;
		dirName.push_back('/');
		for(auto c : dirName)
		{
			if('/'==c && visited.end()==visited.find(partial))
			{
				std::pair <std::string,std::string> pair;
				auto sepsep=SeparatePath(partial);
				pair.first=sepsep[1];
				pair.second=partial;
				dirs.push_back(pair);
				visited.insert(partial);
			}
			partial.push_back(c);
		}
	}
	std::sort(dirs.begin(),dirs.end(),Compare<std::string>);

	size_t index=0;
	for(auto &f : dirs)
	{
		Dir dir;
		dir.ownIndex=index;
		dir.IDinISO=index+1;
		dir.nameInISO=f.second;

		allDirList.push_back(dir);
		nameToDirIdx[f.second]=index;

		++index;
	}

	size_t fileIndex=0;
	for(auto &f : allFileList)
	{
		auto sep=SeparatePath(f.nameInISO);
		// sep[0] is the directory name.
		auto foundDir=nameToDirIdx.find(sep[0]);
		if(nameToDirIdx.end()!=foundDir)
		{
			auto dirIndex=foundDir->second;
			allDirList[dirIndex].fileList.push_back(fileIndex);
			f.dirIndex=dirIndex;
		}
		else
		{
			ErrorMessage(__FUNCTION__,__LINE__,"Something went wrong.");
		}
		++fileIndex;
	}

	for(auto &d : allDirList)
	{
		if(0==d.ownIndex)
		{
			// Don't add the root directory as a child of the root directory.
			// But, parent of the root directory is made the same, apparently.
			d.parentDirIndex=d.ownIndex;
			continue;
		}

		auto sep=SeparatePath(d.nameInISO);
		auto found=nameToDirIdx.find(sep[0]);
		if(nameToDirIdx.end()!=found)
		{
			auto dirIndex=found->second;
			d.parentDirIndex=dirIndex;
			allDirList[d.parentDirIndex].subDirList.push_back(d.ownIndex);
		}
		else
		{
			ErrorMessage(__FUNCTION__,__LINE__,"Something went wrong.");
		}
	}
}

size_t ISOImage::CalculatePathTableSize(void) const
{
	size_t total=0;
	for(auto &d : allDirList)
	{
		auto sep=SeparatePath(d.nameInISO);
		size_t nameLEN=std::max<size_t>(1,sep[1].size());
		size_t len=nameLEN+8;
		len=(len+1)&~1;
		total+=len;
	}
	return total;
}

void ISOImage::CalculatePathTableLBA(void)
{
	auto len=CalculatePathTableSize();
	auto nSectors=(len+CD_SECTOR_SIZE-1)/CD_SECTOR_SIZE;
	LBA_PATHTABLE_BE=LBA_PATHTABLE_LE+nSectors;
	LBA_FIRST_DIRECTORY=LBA_PATHTABLE_BE+nSectors;
}

void ISOImage::CalculateDirFileLBA(void)
{
	size_t LBA=LBA_FIRST_DIRECTORY;
	for(auto &d : allDirList)
	{
		size_t len=34+34; // 34 bytes for "." and 34 bytes for ".."  Name of . is {0} and .. is {1}
		for(auto idx : d.fileList)
		{
			len+=allFileList[idx].CalculateDirSize();
		}
		for(auto idx : d.subDirList)
		{
			len+=allDirList[idx].CalculateDirSize();
		}

		d.LBA=LBA;
		d.len=len;

		auto nSectors=(len+CD_SECTOR_SIZE-1)/CD_SECTOR_SIZE;
		LBA+=nSectors;
	}
	for(auto &f : allFileList)
	{
		f.LBA=LBA;
		auto nSectors=(f.len+CD_SECTOR_SIZE-1)/CD_SECTOR_SIZE;
		LBA+=nSectors;
	}

	totalSectorCount=LBA;
}

std::vector <unsigned char> ISOImage::MakeDescriptorTable(void) const
{
	std::vector <unsigned char> data;
	data.resize(CD_SECTOR_SIZE*2); // PVD plus Last Volume Descriptor

	if(0==allDirList.size())
	{
		ErrorMessage(__FUNCTION__,__LINE__,"Called before making directory structure.");
		return data;
	}

	memset(data.data(),0,data.size());

	// Easy part.  Terminator in the next sector.
	data[CD_SECTOR_SIZE]=ISO9660_VD_LAST;
	data[CD_SECTOR_SIZE+1]='C';
	data[CD_SECTOR_SIZE+2]='D';
	data[CD_SECTOR_SIZE+3]='0';
	data[CD_SECTOR_SIZE+4]='0';
	data[CD_SECTOR_SIZE+5]='1';

	auto &PVD=*(struct ISO9660_PrimaryVolumeDescriptor *)data.data();
	PVD.typeCode=ISO9660_VD_PRIMARY;
	PVD.CD001[0]='C';
	PVD.CD001[1]='D';
	PVD.CD001[2]='0';
	PVD.CD001[3]='0';
	PVD.CD001[4]='1';
	PVD.version=1;
	// char unused1; already zero-cleared.
	memset(PVD.systemID,32,' ');
	memcpy(PVD.volumeID,systemLabel.c_str(),std::min<size_t>(32,systemLabel.size()));
	memset(PVD.volumeID,32,' ');
	memcpy(PVD.volumeID,volumeLabel.c_str(),std::min<size_t>(32,volumeLabel.size()));
	// char unused2[8]; already zero-cleared;

	// Existing ISO image seems to have larger totalSectorCount than actual number of sectors (file size/2048).
	// But, cannot tell how it should be calculated.
	PutDword(PVD.volumeSpaceLE,LITTLE_ENDIAN,totalSectorCount);
	PutDword(PVD.volumeSpaceBE,BIG_ENDIAN,   totalSectorCount);

	// char unused3[32]; already zero-cleared;
	PutWord(PVD.volumeSetSizeLE,LITTLE_ENDIAN,1); // Make it 01 00
	PutWord(PVD.volumeSetSizeBE,BIG_ENDIAN,1);    // Make it 00 01
	PutWord(PVD.volumeSequenceLE,LITTLE_ENDIAN,1); // Make it 01 00
	PutWord(PVD.volumeSequenceBE,BIG_ENDIAN,1); // Make it 00 01
	PutWord(PVD.logicalBlockLE,LITTLE_ENDIAN,CD_SECTOR_SIZE); // 08 00  0800H in Little Endian
	PutWord(PVD.logicalBlockBE,BIG_ENDIAN,CD_SECTOR_SIZE); // 00 08  0800H in Big Endian

	size_t pathTableSize=CalculatePathTableSize();
	PutDword(PVD.pathTableSizeLE,LITTLE_ENDIAN,pathTableSize); // Size in bytes
	PutDword(PVD.pathTableSizeBE,BIG_ENDIAN,pathTableSize);
	PutDword(PVD.LBAPathTableLE,LITTLE_ENDIAN,LBA_PATHTABLE_LE);
	PutDword(PVD.LBSPathTableOptLE,LITTLE_ENDIAN,0);
	PutDword(PVD.LBAPathTableBE,BIG_ENDIAN,LBA_PATHTABLE_BE);
	PutDword(PVD.LBSPathTableOptBE,BIG_ENDIAN,0);

	allDirList[0].MakeISO9660Directory(PVD.rootDir);
	PVD.rootDir.dateTime=GetTodaysDateGMT();

	memset(PVD.volumeSetID,' ',128);  // Keep it all ' '
	memset(PVD.publisherID,' ',128);  // Publisher Name
	PVD.publisherID[0]=0x5F;
	memset(PVD.dataPreparerID,' ',128);   // Keep it all ' '
	PVD.dataPreparerID[0]=0x5F;
	memset(PVD.appID,' ',128); // Name of the program
	PVD.appID[0]=0x5F;
	memset(PVD.copyrightFileID,37,' ');
	memset(PVD.abstractFileID,37,' ');
	memset(PVD.biblioFileID,37,' ');
	auto todayString=GetTodayString();
	memcpy(PVD.creationDate,todayString.c_str(),16); // Like "20241105123700000" Not a C-String ... I see some ISO's that makes it a C-string.
	memcpy(PVD.modificationDate,todayString.c_str(),16);// Like "20241105123700000" Not a C-String ...
	memset(PVD.expierationDate,16,'0'); // keep it all '0'
	memset(PVD.effectiveDate,16,'0');
	PVD.fileStructVersion=1;
	//	char unused4; // Already zero-cleared.

	return data;
}

bool ISOImage::MakeISOImageFile(std::string fileName) const
{
	std::ofstream ofp(fileName,std::ios::binary);

	if(true!=ofp.is_open())
	{
		ErrorMessage(__FUNCTION__,__LINE__,"Cannot open ISO file.");
		return false;
	}

	std::vector <unsigned char> zeroSector;
	zeroSector.resize(CD_SECTOR_SIZE);
	memset(zeroSector.data(),0,zeroSector.size());

	size_t filePos=0;

	filePos+=WriteToFile(ofp,IPLSector.size(),IPLSector.data());
	VerifyFilePosLBA(filePos,1);

	for(int i=1; i<LBA_PVD; ++i)
	{
		filePos+=WriteToFile(ofp,zeroSector.size(),zeroSector.data());
	}
	VerifyFilePosLBA(filePos,LBA_PVD);

	{
		auto PVD=MakeDescriptorTable();
		filePos+=WriteToFile(ofp,PVD.size(),PVD.data());
		VerifyFilePosLBA(filePos,LBA_PATHTABLE_LE);
	}

	{
		auto pathTable=MakePathTable(LITTLE_ENDIAN);
		filePos+=WriteToFile(ofp,pathTable.size(),pathTable.data());
		VerifyFilePosLBA(filePos,LBA_PATHTABLE_BE);
	}

	{
		auto pathTable=MakePathTable(BIG_ENDIAN);
		filePos+=WriteToFile(ofp,pathTable.size(),pathTable.data());
		VerifyFilePosLBA(filePos,LBA_FIRST_DIRECTORY);
	}

	for(auto &d : allDirList)
	{
		VerifyFilePosLBA(filePos,d.LBA);
		auto directory=MakeDirectoryBinary(d);
		filePos+=WriteToFile(ofp,directory.size(),directory.data());
	}

	for(auto &f : allFileList)
	{
		VerifyFilePosLBA(filePos,f.LBA);
		auto file=MakeFileBinary(f);
		filePos+=WriteToFile(ofp,file.size(),file.data());
	}

	return true;
}

size_t ISOImage::WriteToFile(std::ofstream &ofp,size_t len,const void *data) const
{
	ofp.write((const char *)data,len);
	return len;
}

bool ISOImage::VerifyFilePosLBA(size_t filePos,size_t LBA) const
{
	auto fileLBA=filePos/CD_SECTOR_SIZE;
	if(fileLBA!=LBA)
	{
		ErrorMessage(__FUNCTION__,__LINE__,"File Position and LBA inconsistency.");
	}
	return fileLBA==LBA;
}

/* static */ struct ISO9660_DateTime ISOImage::GetTodaysDateGMT(void)
{
	struct ISO9660_DateTime dateTime;
	auto t=time(NULL);
	auto tm=gmtime(&t);
	dateTime.yearsSince1900=tm->tm_year;
	dateTime.month=tm->tm_mon+1; // 1 to 12
	dateTime.day=tm->tm_mday;  // 1 to 31
	dateTime.hour=tm->tm_hour; // 0 to 23
	dateTime.min=tm->tm_min;  // 0 to 59
	dateTime.sec=tm->tm_sec;  // 0 to 59
	dateTime.offsetFromGMT=0; // I use gmtime.  Must be zero.
	return dateTime;
}

std::vector <unsigned char> ISOImage::MakePathTable(ENDIANNESS endian) const
{
	std::vector <unsigned char> data;
	auto size=CalculatePathTableSize();

	size=(size+CD_SECTOR_SIZE-1);
	size/=CD_SECTOR_SIZE;
	size*=CD_SECTOR_SIZE;

	data.resize(size);
	memset(data.data(),0,data.size());

	size_t ptr=0;
	for(auto &d : allDirList)
	{
		auto *entry=(struct ISO9660_PathTableEntry *)(data.data()+ptr);
		auto sep=SeparatePath(d.nameInISO);
		size_t nameLEN=0;
		if(0==sep[1].size())
		{
			nameLEN=1;
		}
		else
		{
			nameLEN=sep[1].size();
			auto *name=entry->name;
			memcpy(name,sep[1].c_str(),sep[1].size());
		}

		entry->len=nameLEN;
		PutDword(entry->LBA,endian,d.LBA);
		PutWord(entry->parentDirNum,endian,allDirList[d.parentDirIndex].IDinISO);

		size_t len=nameLEN+8;
		len=(len+1)&~1;
		ptr+=len;
	}

	return data;
}

std::vector <unsigned char> ISOImage::MakeDirectoryBinary(const Dir &dir) const
{
	std::vector <unsigned char> data;
	size_t len=(dir.len+CD_SECTOR_SIZE-1);
	len/=CD_SECTOR_SIZE;
	len*=CD_SECTOR_SIZE;

	const Dir &parent=allDirList[dir.parentDirIndex];

	data.resize(len);
	memset(data.data(),0,data.size());

	auto thisDir=(struct ISO9660_Directory *)data.data();
	thisDir->len=34;
	PutDword(thisDir->LBA_LE,LITTLE_ENDIAN,dir.LBA);
	PutDword(thisDir->LBA_BE,BIG_ENDIAN,dir.LBA);
	PutDword(thisDir->sizeLE,LITTLE_ENDIAN,dir.len);
	PutDword(thisDir->sizeBE,BIG_ENDIAN,dir.len);
	thisDir->dateTime=GetTodaysDateGMT();
	thisDir->flags=ISO9660_FLAG_DIRECTORY;
	PutWord(thisDir->volumeSeqLE,LITTLE_ENDIAN,1);
	PutWord(thisDir->volumeSeqBE,BIG_ENDIAN,1);
	thisDir->nameLEN=1;
	thisDir->name[0]=0;

	auto parentDir=(struct ISO9660_Directory *)(data.data()+34);
	parentDir->len=34;
	PutDword(parentDir->LBA_LE,LITTLE_ENDIAN,parent.LBA);
	PutDword(parentDir->LBA_BE,BIG_ENDIAN,parent.LBA);
	PutDword(parentDir->sizeLE,LITTLE_ENDIAN,parent.len);
	PutDword(parentDir->sizeBE,BIG_ENDIAN,parent.len);
	parentDir->dateTime=GetTodaysDateGMT();
	parentDir->flags=ISO9660_FLAG_DIRECTORY;
	PutWord(parentDir->volumeSeqLE,LITTLE_ENDIAN,1);
	PutWord(parentDir->volumeSeqBE,BIG_ENDIAN,1);
	parentDir->nameLEN=1;
	parentDir->name[0]=1;

	size_t ptr=34+34;
	size_t filePos=0,subDirPos=0;
	while(filePos<dir.fileList.size() || subDirPos<dir.subDirList.size())
	{
		int fileOrDir=0;
		if(filePos<dir.fileList.size() && subDirPos<dir.subDirList.size())
		{
			const File &f=allFileList[dir.fileList[filePos]];
			const Dir &d=allDirList[dir.subDirList[subDirPos]];
			auto fileSep=SeparatePath(f.nameInISO);
			auto dirSep=SeparatePath(d.nameInISO);

			if(fileSep[1]<dirSep[1])
			{
				fileOrDir=0;
			}
			else
			{
				fileOrDir=1;
			}
		}
		else if(filePos<dir.fileList.size())
		{
			fileOrDir=0;
		}
		else
		{
			fileOrDir=1;
		}

		auto entry=(struct ISO9660_Directory *)(data.data()+ptr);
		size_t len=0;
		if(0==fileOrDir)
		{
			const File &f=allFileList[dir.fileList[filePos]];
			len=f.CalculateDirSize();

			auto sep=SeparatePath(f.nameInISO);
			auto name=sep[1];
			name.push_back(';');
			name.push_back('1');

			entry->len=len;
			PutDword(entry->LBA_LE,LITTLE_ENDIAN,f.LBA);
			PutDword(entry->LBA_BE,BIG_ENDIAN,f.LBA);
			PutDword(entry->sizeLE,LITTLE_ENDIAN,f.len);
			PutDword(entry->sizeBE,BIG_ENDIAN,f.len);
			entry->dateTime=GetTodaysDateGMT();
			entry->flags=0;
			PutWord(entry->volumeSeqLE,LITTLE_ENDIAN,1);
			PutWord(entry->volumeSeqBE,BIG_ENDIAN,1);
			entry->nameLEN=name.size();
			memcpy(entry->name,name.data(),name.size());

			++filePos;
		}
		else
		{
			const Dir &d=allDirList[dir.subDirList[subDirPos]];
			len=d.CalculateDirSize();
			auto sep=SeparatePath(d.nameInISO);
			auto name=sep[1];

			entry->len=len;
			PutDword(entry->LBA_LE,LITTLE_ENDIAN,d.LBA);
			PutDword(entry->LBA_BE,BIG_ENDIAN,d.LBA);
			PutDword(entry->sizeLE,LITTLE_ENDIAN,d.len);
			PutDword(entry->sizeBE,BIG_ENDIAN,d.len);
			entry->dateTime=GetTodaysDateGMT();
			entry->flags=ISO9660_FLAG_DIRECTORY;
			PutWord(entry->volumeSeqLE,LITTLE_ENDIAN,1);
			PutWord(entry->volumeSeqBE,BIG_ENDIAN,1);
			entry->nameLEN=name.size();
			memcpy(entry->name,name.data(),name.size());

			++subDirPos;
		}
		ptr+=len;
	}

	return data;
}

std::vector <unsigned char> ISOImage::MakeFileBinary(const File &file) const
{
	std::vector <unsigned char> data;
	size_t len=(file.len+CD_SECTOR_SIZE-1);
	len/=CD_SECTOR_SIZE;
	len*=CD_SECTOR_SIZE;

	data.resize(len);
	memset(data.data(),0,data.size());

	std::ifstream ifp(file.input,std::ios::binary);
	if(true==ifp.is_open())
	{
		ifp.read((char *)data.data(),file.len);
	}

	return data;
}

std::string ISOImage::NormalizeFileName(std::string fileName) const
{
	if(true!=caseSensitive)
	{
		for(auto &c : fileName)
		{
			c=toupper(c);
		}
	}
	for(auto &c : fileName)
	{
		if('\\'==c)
		{
			c='/';
		}
	}
	if(2<=fileName.size() && ':'==fileName[1])
	{
		fileName.erase(fileName.begin());
		fileName.erase(fileName.begin());
	}
	if('/'==fileName[0])
	{
		fileName.erase(fileName.begin());
	}
	if('/'==fileName.back())
	{
		fileName.pop_back();
	}
	return fileName;
}

std::array <std::string,2> ISOImage::SeparatePath(std::string path)
{
	std::array <std::string,2> dirFile;
	if(0<path.size())
	{
		size_t lastSlash=~0;
		for(size_t i=0; i<path.size(); ++i)
		{
			if('/'==path[i] || '\\'==path[i])
			{
				lastSlash=i;
			}
		}

		if(~0==lastSlash) // No slash.
		{
			dirFile[1]=path;
		}
		else
		{
			for(size_t i=0; i<lastSlash; ++i)
			{
				dirFile[0].push_back(path[i]);
			}
			for(size_t i=lastSlash+1; i<path.size(); ++i)
			{
				dirFile[1].push_back(path[i]);
			}
		}
	}
	return dirFile;
}

std::string ISOImage::Join(std::string dir,std::string name)
{
	if(0==dir.size())
	{
		return name;
	}
	else
	{
		if(dir.back()=='\\')
		{
			dir.back()='/';
		}
		if(dir.back()!='/')
		{
			dir.push_back('/');
		}
		dir.insert(dir.end(),name.begin(),name.end());
		return dir;
	}
}

bool ISOImage::FileExists(std::string name) const
{
	std::ifstream ifp(name,std::ios::binary);
	return ifp.is_open();
}

size_t ISOImage::GetFileLength(std::string name) const
{
	std::ifstream ifp(name,std::ios::binary);
	if(true==ifp.is_open())
	{
		ifp.seekg(0,std::ios::end);
		size_t sz=ifp.tellg();
		return sz;
	}
	return 0;
}

std::string ISOImage::GetTodayString(void) const
{
	auto t=time(NULL);
	auto tm=localtime(&t);
	const int date=tm->tm_mday;
	const int month=tm->tm_mon+1;
	const int year=std::min(9999,1900+tm->tm_year);
	const int hour=tm->tm_hour;
	const int min=tm->tm_min;
	const int sec=tm->tm_sec;

	char str[256];
	sprintf(str,"%04d%02d%02d%02d%02d%02d0000000000000000",year,month,date,hour,min,sec);
	str[16]=0;

	return str;
}

void ISOImage::ErrorMessage(std::string func,unsigned int lineNum,std::string message)
{
	std::cout << func << "\n";
	std::cout << lineNum << "\n";
	std::cout << message << "\n";
}

void ISOImage::Print(void) const
{
	PrintDirs();
	PrintFiles();
}

void ISOImage::PrintDirs(void) const
{
	std::cout << "-- Directories --\n";
	for(auto &d : allDirList)
	{
		std::cout << '[' << d.nameInISO << "]  LBA=" << d.LBA << "  ID in ISO=" << d.IDinISO << "  Parent dir ID=" << allDirList[d.parentDirIndex].IDinISO << "\n";
		for(auto fileIdx : d.fileList)
		{
			std::cout << "  f " << allFileList[fileIdx].nameInISO << "\n";
		}
		for(auto dirIdx : d.subDirList)
		{
			std::cout << "  d " << allDirList[dirIdx].nameInISO << "\n";
		}
	}
}

void ISOImage::PrintFiles(void) const
{
	std::cout << "-- Files --\n";
	for(auto &f : allFileList)
	{
		std::cout << '[' << f.nameInISO << "]  SRC=" << f.input << " LBA=" << f.LBA << "  Dir ID=" << allDirList[f.dirIndex].IDinISO << "\n";
	}
}


class CommandParameterInfo
{
public:
	std::string output;
	bool verbose=false;
	bool RecognizeCommandParameter(ISOImage &iso,int ac,char *av[]);
};

bool CommandParameterInfo::RecognizeCommandParameter(ISOImage &iso,int ac,char *av[])
{
	std::vector <std::string> argv;
	for(int i=1; i<ac; ++i)
	{
		std::string opt=av[i];
		for(auto &c : opt)
		{
			c=toupper(c);
		}
		if("-F"==opt && i+1<ac) // Simple file.  File added to the root dir.
		{
			if(true!=iso.AddFileSimple(av[i+1]))
			{
				std::cout << "Error while adding a file.\n";
				return false;
			}
			++i;
		}
		else if("-FF"==opt && i+2<ac) // Not so simple file.  Input file name, and the name in the ISO image.
		{
			if(true!=iso.AddFile(av[i+1],av[i+2]))
			{
				std::cout << "Error while adding a file.\n";
				return false;
			}
			i+=2;
		}
		else if("-VOL"==opt && i+1<ac) // Volume Label
		{
			iso.volumeLabel=av[i+1];
			++i;
		}
		else if("-IPL"==opt && i+1<ac)
		{
			if(true!=iso.LoadIPL(av[i+1]))
			{
				std::cout << "Error while loading the IPL sector.\n";
				return false;
			}
			++i;
		}
		else if("-VERBOSE")
		{
			verbose=true;
		}
		else if("-O"==opt && i+1<ac)
		{
			output=av[i+1];
			++i;
		}
		else
		{
			std::cout << "Undefined option: " << opt << "\n";
			return false;
		}
	}

	if(""==output)
	{
		std::cout << "No output file is specified.\n";
		return false;
	}

	return true;
}

void Test1(void)
{
	ISOImage iso;
	iso.AddFileSimple("geniso.cpp");
	iso.AddFile("iso9660.h","src/iso9660.h");
	iso.AddFile("dosdisk.h","src/dosdisk.h");
	iso.AddFile("makefd.cpp","makefdd/src/dosdisk.h");

	iso.SortFiles();
	iso.MakeDirectoryList();
	iso.CalculatePathTableLBA();
	iso.CalculateDirFileLBA();

	iso.Print();

	iso.MakeISOImageFile("test.iso");
}

int main(int ac,char *av[])
{
	// Make sure alighment is not breaking the struct.
	assert(sizeof(struct ISO9660_PrimaryVolumeDescriptor)==883);
	assert(sizeof(struct ISO9660_PathTableEntry)==10);
	assert(sizeof(struct ISO9660_Directory)==34);
	assert(sizeof(struct ISO9660_DateTime)==7);

	CommandParameterInfo cpi;
	ISOImage iso;
	if(true!=cpi.RecognizeCommandParameter(iso,ac,av))
	{
		return 1;
	}

	iso.SortFiles();
	iso.MakeDirectoryList();
	iso.CalculatePathTableLBA();
	iso.CalculateDirFileLBA();

	if(true==cpi.verbose)
	{
		iso.Print();
	}

	if(true!=iso.MakeISOImageFile(cpi.output))
	{
		std::cout << "Error while writing .ISO file.\n";
		return 1;
	}

	std::cout << "Created " << cpi.output << "\n";

	return 0;
}
