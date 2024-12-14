#include <stdio.h>
#include <stdlib.h>
#include <i86.h>
#include <conio.h>
#include <dos.h>
#include "diskbios.h"

unsigned char TOC[6+99*3];


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


#define TOWNSIO_ELEVOL_1_DATA 0x4E0 // [2] pp.18, pp.174
#define TOWNSIO_ELEVOL_1_COM  0x4E1 // [2] pp.18, pp.174
#define TOWNSIO_ELEVOL_2_DATA 0x4E2 // [2] pp.18, pp.174
#define TOWNSIO_ELEVOL_2_COM  0x4E3 // [2] pp.18, pp.174

#define TOWNSIO_SOUND_MASTERSWITCH 0x4EC // [2] p.216

void _CLI();
#pragma aux _CLI="CLI";

void _STI();
#pragma aux _STI="STI";


void Command(unsigned char cmd,unsigned char params[8])
{
	long int i;
	int nSta=0;
	unsigned char statusBytes[5][256];

	printf("Command %02x\n",cmd);

	_CLI();

	while(0==(inp(IO_CDC_MASTER_STATUS)&1));

	for(i=0; i<8; ++i)
	{
		outp(IO_CDC_PARAM,params[i]); // Push 8 parameters
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

	_STI();

	for(i=0; i<nSta; ++i)
	{
		printf("%02x| %02x %02x %02x %02x\n",statusBytes[i][0],statusBytes[i][1],statusBytes[i][2],statusBytes[i][3],statusBytes[i][4]);
	}
}

unsigned char BIN2BCD(unsigned char input)
{
	return ((input/10)<<4)|(input%10);
}

int main(int ac,char *av[])
{
	if(2<ac)
	{
		printf("Usage: CDDAPLAY track\n");
		printf("Play CDDA repeat.\n");
		return 1;
	}

	union REGS regs;
	regs.x.ax=0x54C0;
	regs.x.cx=0;
	regs.x.di=(unsigned int)TOC;
	int86(INT_DISKBIOS,&regs,&regs);
	if(regs.h.ah)
	{
		printf("CD-ROM BIOS Error AH=%02x CX=%04x\n",regs.h.ah,regs.x.cx);
		return 1;
	}

	printf("Disc Type=%d\n",TOC[0]);
	printf("First Track=%d\n",TOC[1]);
	printf("Last Track=%d\n",TOC[2]);
	printf("Disc Time=%02d:%02d:%02d\n",TOC[3],TOC[4],TOC[5]);
	for(int trk=TOC[1]; trk<=TOC[2]; ++trk)
	{
		printf("Track Time=%02d:%02d:%02d\n",TOC[3+trk*3],TOC[4+trk*3],TOC[5+trk*3]);
	}
	TOC[6+TOC[2]*3]=TOC[3];
	TOC[7+TOC[2]*3]=TOC[4];
	TOC[8+TOC[2]*3]=TOC[5];

	int trk=atoi(av[1]);
	if(trk<TOC[1] || TOC[2]<trk)
	{
		printf("Track number error.\n");
		return 1;
	}

	unsigned char trackTime[8]=
	{
		TOC[3+trk*3],
		TOC[4+trk*3],
		TOC[5+trk*3],
		TOC[6+trk*3],
		TOC[7+trk*3],
		TOC[8+trk*3],
		0,
		0,
	};

	{
		unsigned char vol=0x7f;
		outp(TOWNSIO_ELEVOL_2_COM,(((~vol)>>2)&0x10)|5);
		outp(TOWNSIO_ELEVOL_2_DATA,vol&0x3f);

		outp(TOWNSIO_ELEVOL_2_COM,(((~vol)>>2)&0x10)|4);
		outp(TOWNSIO_ELEVOL_2_DATA,vol&0x3f);

		outp(TOWNSIO_SOUND_MASTERSWITCH,0xFF);
	}

	//regs.x.ax=0x50C0;
	//regs.x.cx=0xFF01;
	//regs.x.di=(unsigned int)trackTime;
	//int86(INT_DISKBIOS,&regs,&regs);
	//if(regs.h.ah)
	//{
	//	printf("CD-ROM BIOS Error AH=%02x CX=%04x\n",regs.h.ah,regs.x.cx);
	//	return 1;
	//}

	for(int i=0; i<8; ++i)
	{
		trackTime[i]=BIN2BCD(trackTime[i]);
	}
	Command(CDCCMD_CDDAPLAY+CDCCMD_STATUSREQ,trackTime);

	return 0;
}
