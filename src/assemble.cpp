#include <iostream>
#include <fstream>
#include <vector>


unsigned char data[512*1024];


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

int main(void)
{
	std::string files[]=
	{
		"IOSYS.bin",
		"CONDEV.bin",
		"CLOCKDEV.bin",
		"DUMMYDEV.bin",
		"INT8EH.bin",
		"INT90H.bin",
		"INT91H.bin",
		"INT93H.bin",
		"INT96H.bin",
		"INT9BH.bin",
		"INTAEH.bin",
		"INTAFH.bin",
	};
	std::vector <PositionAndSize> filePos;

	memset(data,0,sizeof(data));

	size_t ptr=0;
	for(auto file : files)
	{
		filePos.push_back(ReadFile(data,ptr,file));
	}

	// IOSYS loaded at 0040:0000
	// CONDEV will be located at 0040h+IOSYS_ptr/16 segment
	// CLOCKDEV will be located at 0040h+CLOCKDEV_ptr/16 segment
	// Question is how to tell IO.SYS about the location of CONDEV.

	auto IOSYS_ptr=filePos[0].pos;
	auto CONDEV_ptr=filePos[1].pos;
	auto CLOCKDEV_ptr=filePos[2].pos;
	auto DUMMYDEV_ptr=filePos[3].pos;

	unsigned char *IOSYSTop=data;
	const unsigned int IOSYSSEG=0x40;

	const unsigned int YSDOSSEG=IOSYSSEG+(filePos.back().pos-IOSYS_ptr)/0x10;
	*(unsigned short*)(data+0x2FA)=YSDOSSEG;

	const unsigned int CONDEVSEG=IOSYSSEG+(CONDEV_ptr-IOSYS_ptr)/0x10;
	*(unsigned short *)(data+0x2FC)=0;
	*(unsigned short *)(data+0x2FE)=CONDEVSEG;

	const unsigned int CLOCKSEG=IOSYSSEG+(CLOCKDEV_ptr-IOSYS_ptr)/0x10; 
	*(unsigned short*)(data+CONDEV_ptr)=0;
	*(unsigned short*)(data+CONDEV_ptr+2)=CLOCKSEG;

	const unsigned int DUMMYSEG=IOSYSSEG+(DUMMYDEV_ptr-IOSYS_ptr)/0x10; 
	*(unsigned short*)(data+CLOCKDEV_ptr)=0;
	*(unsigned short*)(data+CLOCKDEV_ptr+2)=DUMMYSEG;


	std::ofstream hdimg("../resources/IO.SYS",std::ios::binary);
	hdimg.write((char *)data,ptr);

	return 0;
}
