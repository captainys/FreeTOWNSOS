#include <MACRO.H>
#include <stdio.h>
#include <dos.h>

#define IO_CDC_MASTER_STATUS 0x4C0
#define IO_CDC_STATUS 0x4C2
#define IO_CDC_COMMAND 0x4C2
#define IO_CDC_PARAM  0x4C4
#define IO_1US_WAIT 0x6C

void Command(unsigned char cmd)
{
	int i;
	int nSta=0;
	unsigned char statusBytes[5][256];

	printf("Command %02x\n",cmd);

	_CLI;

	while(0==(_inb(IO_CDC_MASTER_STATUS)&1));

	for(i=0; i<8; ++i)
	{
		_outb(IO_CDC_PARAM,0); // Push 8 parameters all zero.
	}

	while(0==(_inb(IO_CDC_MASTER_STATUS)&1));

	_outb(IO_CDC_COMMAND,cmd);

	for(i=0; i<1000000 || 0==nSta; ++i)
	{
		unsigned char sta;
		_outb(IO_1US_WAIT,0);
		sta=_inb(IO_CDC_MASTER_STATUS);
		if(0!=(sta&2))
		{
			statusBytes[nSta][0]=sta;
			statusBytes[nSta][1]=_inb(IO_CDC_STATUS);
			statusBytes[nSta][2]=_inb(IO_CDC_STATUS);
			statusBytes[nSta][3]=_inb(IO_CDC_STATUS);
			statusBytes[nSta][4]=_inb(IO_CDC_STATUS);
			_outb(IO_CDC_MASTER_STATUS,0x80);
			++nSta;
		}
	}

	_STI;

	for(i=0; i<nSta; ++i)
	{
		printf("%02x| %02x %02x %02x %02x\n",statusBytes[i][0],statusBytes[i][1],statusBytes[i][2],statusBytes[i][3],statusBytes[i][4]);
	}
}

int main(void)
{
	Command(0xA0);  // Get Status
	Command(0x25);  // Get TOC
	return 0;
}
