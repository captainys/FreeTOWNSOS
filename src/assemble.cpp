#include <iostream>
#include <fstream>


unsigned char data[512*1024];


size_t read(unsigned char *ptr,std::ifstream &ifp)
{
	ifp.seekg(std::ios::end);
	size_t sz=ifp.tellg();
	ifp.seekg(std::ios::beg);
	ifp.read((char *)ptr,sz);
	return sz;
}


int main(void)
{
	memset(data,0,sizeof(data));

	std::ifstream ipl("IPL",std::ios::binary);
	ipl.read((char *)data,512);

	size_t ptr=512;

	std::ifstream iosys("IOSYS",std::ios::binary);
	size_t IOSYS_size=(read(data+ptr,iosys)+15)&~15;
	size_t IOSYS_ptr=ptr;

	ptr+=IOSYS_size;

	std::ifstream condev("CONDEV",std::ios::binary);
	size_t CONDEV_size=(read(data+ptr,condev)+15)&~15;
	size_t CONDEV_ptr=ptr;

	ptr+=CONDEV_size;

	std::ifstream clockdev("CLOCKDEV",std::ios::binary);
	size_t CLOCKDEV_size=(read(data+ptr,clockdev)+15)&~15;
	size_t CLOCKDEV_ptr=ptr;

	ptr+=CLOCKDEV_size;

	std::cout << "CONDEV at " << CONDEV_ptr << "\n";
	std::cout << "CLOCKDEV at " << CLOCKDEV_ptr << "\n";

	// IOSYS loaded at 0040:0000
	// CONDEV will be located at 0040h+IOSYS_ptr/16 segment
	// CLOCKDEV will be located at 0040h+CLOCKDEV_ptr/16 segment
	// Question is how to tell IO.SYS about the location of CONDEV.

	std::ofstream hdimg("HDIMG.h3",std::ios::binary);
	hdimg.write((char *)data,sizeof(data));

	return 0;
}
