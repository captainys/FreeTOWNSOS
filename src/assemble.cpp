#include <iostream>
#include <fstream>


unsigned char data[512*1024];


int main(void)
{
	memset(data,0,sizeof(data));

	std::ifstream ipl("IPL",std::ios::binary);
	ipl.read((char *)data,512);

	std::ifstream iosys("IOSYS",std::ios::binary);
	iosys.read((char *)data+512,128*1024);


	std::ofstream hdimg("HDIMG.h3",std::ios::binary);
	hdimg.write((char *)data,sizeof(data));

	return 0;
}
