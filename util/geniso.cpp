#include <list>
#include <string>
#include <array>
#include <fstream>
#include <iostream>
#include <ctype.h>
#include <unordered_map>

#include "iso9660.h"

class ISOImage
{
public:
	class FileBase
	{
	public:
		std::string nameInISO;
		unsigned int attrib=0;
		size_t LBA=0;
	};

	class File : public FileBase
	{
	public:
		std::string input;
	};
	class Dir : public FileBase
	{
	public:
		int id;
		std::vector <size_t> fileList;	// Indexes to allFileList
		std::vector <size_t> subDirList;	// Indexes to allDirList
	};

	bool caseSensitive=false;
	std::vector <File> allFileList;
	std::vector <Dir> allDirList;		// allDirList[0] is always the root directory.
	std::unordered_map <std::string,size_t> nameToFileIdx;  // "ABC/XYZ/DATA.BIN" -> index
	std::unordered_map <std::string,size_t> nameToDirIdx;   // "ABC/XYZ" -> index

	// Steps:
	//   (1) Add files.
	//   (2) Make directory structure.
	//   (3) Calculate number of sectors for each.
	//   (4) Place elements.

	ISOImage();

	bool AddFileSimple(std::string file); // As is.  If "C:/abc/xyz.txt" is given, "Will be made /XYZ.TXT in ISO."
	bool AddFile(std::string fileIn,std::string fileInISO);
	bool AddDir(std::string dir);

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
};


ISOImage::ISOImage()
{
	Dir dir;
	dir.id=1; // Looks to be 1 is for root dir.

	auto dirIdx=allDirList.size(); // Should be zero.
	allDirList.push_back(dir);

	nameToDirIdx[""]=dirIdx;
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
	f.nameInISO=sep[1];

	size_t fileIdx=allFileList.size();
	allFileList.push_back(f);

	nameToFileIdx[f.nameInISO]=fileIdx;

	return true;
}

bool ISOImage::AddFile(std::string fileIn,std::string fileInISO)
{
	if(true!=FileExists(fileIn))
	{
		std::cout << "Cannot open " << fileIn << "\n";
		return false;
	}

	fileInISO=NormalizeFileName(fileInISO);
	auto sep=SeparatePath(fileInISO);

	auto dirFound=nameToDirIdx.find(sep[0]);
	if(nameToDirIdx.end()==dirFound)
	{
		AddDir(sep[0]);
		dirFound=nameToDirIdx.find(sep[0]);

		if(nameToDirIdx.end()==dirFound)
		{
			std::cout << __FUNCTION__ << "\n";
			std::cout << __LINE__ << "\n";
			std::cout << "Something went wrong.\n";
			return false;
		}
	}

	File f;
	f.input=fileIn;
	f.nameInISO=sep[1];

	size_t fileIdx=allFileList.size();
	allFileList.push_back(f);

	nameToFileIdx[f.nameInISO]=fileIdx;

	allDirList[dirFound->second].fileList.push_back(fileIdx);

	return true;
}

bool ISOImage::AddDir(std::string dir)
{
	std::string partial;
	dir.push_back('/');
	for(auto c : dir)
	{
		if(c=='/' || c=='\\')
		{
			auto found=nameToDirIdx.find(partial);
			if(nameToDirIdx.end()==found)
			{
				Dir dir;
				dir.nameInISO=partial;

				auto dirIdx=allDirList.size(); // Should be zero.
				allDirList.push_back(dir);

				nameToDirIdx[partial]=dirIdx;
			}
		}
		partial.push_back(c);
	}
	return 0;
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
}

int main(int ac,char *av[])
{
	Test1();
	return 0;
}
