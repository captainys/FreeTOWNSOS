#include <iostream>
#include <fstream>
#include <vector>
#include <map>


unsigned char data[512*1024];


// Segments for the BIOS are found at these offsets in segment 0050H
const unsigned int SEG_INT8EH=0x02;
const unsigned int SEG_INT90H=0x04;
const unsigned int SEG_INT91H=0x06;
const unsigned int SEG_INT92H=0x08;// Reserved for FM-R Graphics BIOS
const unsigned int SEG_INT93H=0x0A;//
const unsigned int SEG_INT94H=0x0C;// Reserved for FM-R Printer BIOS
const unsigned int SEG_INT95H=0x0E;// Reserved for FM-R Hard-Copy BIOS
const unsigned int SEG_INT96H=0x10;// Reserved for FM-R Calendar BIOS
const unsigned int SEG_INT97H=0x12;//
const unsigned int SEG_INT98H=0x14;// Reserved for FM-R Mouse BIOS
const unsigned int SEG_INT9BH=0x16;//
const unsigned int SEG_INT9EH=0x18;// Reserved for FM-R Buzzer BIOS
const unsigned int SEG_INTAEH=0x1A;//
const unsigned int SEG_INTAFH=0x1C;//
const unsigned int SEG_INTECH=0x1E;// Reserved for FM-R OAK BIOS
const unsigned int SEG_INTEDH=0x20;// Reserved for FM-R OAK BIOS
const unsigned int SEG_INTFDH=0x22;// Reserved for FM-R Software Timer BIOS



class PositionAndSize
{
public:
	size_t pos,size;
};

PositionAndSize ReadFile(unsigned char *data,size_t &ptr,std::string fileName)
{
	PositionAndSize ps;

	std::ifstream ifp(fileName,std::ios::binary);

	if(true!=ifp.is_open())
	{
		std::cout << "Error opening " << fileName << "\n";
		exit(1);
	}

	ifp.seekg(0,std::ios::end);
	size_t sz=ifp.tellg();
	ifp.seekg(0,std::ios::beg);
	ifp.read((char *)(data+ptr),sz);

	ps.pos=ptr;
	ps.size=(sz+15)&~15;

	ptr+=ps.size;

	return ps;
}



class File
{
public:
	std::string fileName;
	int INTNum;
	PositionAndSize pos;

	File(std::string fileName,int INTNum)
	{
		this->fileName=fileName;
		this->INTNum=INTNum;
	}
};


int main(void)
{
	std::map <unsigned,unsigned> INTtoOFFSET;

	INTtoOFFSET[0x8E]=0x02;//
	INTtoOFFSET[0x90]=0x04;//
	INTtoOFFSET[0x91]=0x06;//
	INTtoOFFSET[0x92]=0x08;// Reserved for FM-R Graphics BIOS
	INTtoOFFSET[0x93]=0x0A;//
	INTtoOFFSET[0x94]=0x0C;// Reserved for FM-R Printer BIOS
	INTtoOFFSET[0x95]=0x0E;// Reserved for FM-R Hard-Copy BIOS
	INTtoOFFSET[0x96]=0x10;// Reserved for FM-R Calendar BIOS
	INTtoOFFSET[0x97]=0x12;//
	INTtoOFFSET[0x98]=0x14;// Reserved for FM-R Mouse BIOS
	INTtoOFFSET[0x9B]=0x16;//
	INTtoOFFSET[0x9E]=0x18;// Reserved for FM-R Buzzer BIOS
	INTtoOFFSET[0xAE]=0x1A;//
	INTtoOFFSET[0xAF]=0x1C;//
	INTtoOFFSET[0xEC]=0x1E;// Reserved for FM-R OAK BIOS
	INTtoOFFSET[0xED]=0x20;// Reserved for FM-R OAK BIOS
	INTtoOFFSET[0xFD]=0x22;
	

	File files[]=
	{
		File("IOSYS.bin",   0),
		File("INT8EH.bin",  0x8E),
		File("INT90H.bin",  0x90),
		File("INT91H.bin",  0x91),
		File("INT93H.bin",  0x93),
		File("INT96H.bin",  0x96),
		File("INT97H.bin",  0x97),
		File("INT9BH.bin",  0x9B),
		File("INTAEH.bin",  0xAE),
		File("INTAFH.bin",  0xAF),
		File("INTFDH.bin",  0xFD),
	};

	memset(data,0,sizeof(data));

	size_t ptr=0;
	for(auto &file : files)
	{
		file.pos=ReadFile(data,ptr,file.fileName);
	}

	for(auto &file : files)
	{
		auto found=INTtoOFFSET.find(file.INTNum);
		if(found!=INTtoOFFSET.end())
		{
			auto offset=found->second;
			*(uint16_t *)(data+offset)=0x50+file.pos.pos/16;
		}
	}


	char first256bytes[256];
	memset(first256bytes,0,sizeof(first256bytes));
	memcpy(first256bytes,"FBIOS",5);
	*(uint32_t *)(first256bytes+6)=ptr;

	std::ofstream diskimg("../resources/IO.SYS",std::ios::binary);
	diskimg.write(first256bytes,256);
	diskimg.write((char *)data,ptr);

	return 0;
}
