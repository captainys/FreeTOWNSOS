#include <stdio.h>
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
	disk.CreateFD(BPB_MEDIA_1232K);

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
		if(true!=MakeDisk(o,cpi))
		{
			return 1;
		}
	}

	return 0;
}
