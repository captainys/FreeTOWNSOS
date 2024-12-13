#include <stdio.h>
#include <conio.h>
#include <dos.h>
#include <i86.h>

#define IO_CDC_MASTER_STATUS 0x4C0
#define IO_CDC_STATUS 0x4C2
#define IO_CDC_COMMAND 0x4C2
#define IO_CDC_PARAM  0x4C4
#define IO_1US_WAIT 0x6C

#define CDCCMD_SEEK				0x00
#define CDCCMD_MODE2_READ		0x01
#define CDCCMD_MODE1_READ		0x02
#define CDCCMD_RAWREAD			0x03
#define CDCCMD_RAW_READ			0x03
#define CDCCMD_CDDAPLAY			0x04
#define CDCCMD_TOCREAD			0x05
#define CDCCMD_SUBQREAD			0x06
#define CDCCMD_UNKNOWN1			0x1F //; NOP and requst status? I guess?
#define CDCCMD_GET_STATE		0x80 //; Linux for FM TOWNS source label this as "SETSTATE" but isn't it "GETSTATE"?
#define CDCCMD_CDDA_SET			0x81
#define CDCCMD_CDDA_STOP		0x84
#define CDCCMD_CDDA_PAUSE		0x85
#define CDCMD_UNKNOWN2			0x86
#define CDCCMD_CDDA_RESUME		0x87
#define CDCCMD_UNKNOWN3			0x9F //; Used from Windows 95 Internal CD-ROM Driver.

#define CDCCMD_IRQ				0x40
#define CDCCMD_STATUSREQ		0x20

void _CLI();
#pragma aux _CLI="CLI";

void _STI();
#pragma aux _STI="STI";


void Command(unsigned char cmd)
{
	long int i;
	int nSta=0;
	unsigned char statusBytes[5][256];

	printf("Command %02x\n",cmd);

	_CLI;

	while(0==(inp(IO_CDC_MASTER_STATUS)&1));

	for(i=0; i<8; ++i)
	{
		outp(IO_CDC_PARAM,0); // Push 8 parameters all zero.
	}

	while(0==(inp(IO_CDC_MASTER_STATUS)&1));

	outp(IO_CDC_COMMAND,cmd);

	for(i=0; i<1000000 || 0==nSta; ++i)
	{
		unsigned char sta;
		outp(IO_1US_WAIT,0);
		sta=inp(IO_CDC_MASTER_STATUS);
		if(0!=(sta&2))
		{
			statusBytes[nSta][0]=sta;
			statusBytes[nSta][1]=inp(IO_CDC_STATUS);
			statusBytes[nSta][2]=inp(IO_CDC_STATUS);
			statusBytes[nSta][3]=inp(IO_CDC_STATUS);
			statusBytes[nSta][4]=inp(IO_CDC_STATUS);
			outp(IO_CDC_MASTER_STATUS,0x80);
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
	Command(CDCCMD_GET_STATE|CDCCMD_IRQ|CDCCMD_STATUSREQ);
	return 0;
}
