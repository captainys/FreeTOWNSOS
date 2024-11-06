#include <list>
#include <string>
#include <array>
#include <fstream>
#include <iostream>
#include <algorithm>
#include <ctype.h>
#include <stdint.h>
#include <map>
#include <unordered_set>
#include <unordered_map>

#include "iso9660.h"

class ISOImage
{
public:
	enum ENDIANNESS
	{
		LITTLE_ENDIAN,
		BIG_ENDIAN
	};
	const int CD_SECTOR_SIZE=2048;

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
	};

	bool caseSensitive=false;
	std::vector <File> allFileList;
	std::vector <Dir> allDirList;		// allDirList[0] is always the root directory.
	std::unordered_map <std::string,size_t> nameToDirIdx;   // "ABC/XYZ" -> index

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

	// MakePathTable can be used before setting LBAs for the directories and files for calculating the size of the path table.
	// In which case, all LBA will be filled as ~0.
	// Observation of some ISO images suggests that the index in the path table starts with 1 (root Dir)
	// Root-dir name length is 1.  But, the value in the name is \0.
	std::vector <unsigned char> MakePathTable(ENDIANNESS endian) const;


	// Capitalize if caseSensitive==false.
	// Backslash to slash.
	// If the first is '/', remove.
	// If the last is '/', remove.
	// If '/' or '\\' is given, returns "".
	// If the drive letter is present, remove.
	std::string NormalizeFileName(std::string fileNameIn) const;

	// "ABC/XYZ/DATA.BIN" -> [0]="ABC/XYZ", [1]="DATA.BIN"
	std::array <std::string,2> SeparatePath(std::string path) const;

	// "ABC/XYZ","DATA.BIN" -> "ABC/XYZ/DATA.BIN
	std::string Join(std::string dir,std::string name) const;

	bool FileExists(std::string name) const;
	size_t GetFileLength(std::string name) const;

	void ErrorMessage(std::string func,unsigned int lineNum,std::string message);

	void Print(void) const;
	void PrintDirs(void) const;
	void PrintFiles(void) const;
};

size_t ISOImage::File::CalculateDirSize(void) const
{
	size_t len=33+nameInISO.size()+2; // +2 for ";1"
	return (len+1)&~1;  // Always 2*N
}
size_t ISOImage::Dir::CalculateDirSize(void) const
{
	size_t len=33+nameInISO.size(); // Directory does not have ";1"
	return (len+1)&~1;  // Always 2*N
}


ISOImage::ISOImage()
{
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

std::vector <unsigned char> ISOImage::MakePathTable(ENDIANNESS endian) const
{
	std::vector <unsigned char> data;
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

std::array <std::string,2> ISOImage::SeparatePath(std::string path) const
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

std::string ISOImage::Join(std::string dir,std::string name) const
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
		ifp.seekg(std::ios::end,0);
		size_t sz=ifp.tellg();
		return sz;
	}
	return 0;
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
};



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
}

int main(int ac,char *av[])
{
	Test1();
	return 0;
}
