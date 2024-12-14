#include <stdio.h>
#include <stdlib.h>
#include <i86.h>
#include <conio.h>
#include <dos.h>
#include "diskbios.h"

unsigned char TOC[6+99*3];

#define TOWNSIO_ELEVOL_1_DATA 0x4E0 // [2] pp.18, pp.174
#define TOWNSIO_ELEVOL_1_COM  0x4E1 // [2] pp.18, pp.174
#define TOWNSIO_ELEVOL_2_DATA 0x4E2 // [2] pp.18, pp.174
#define TOWNSIO_ELEVOL_2_COM  0x4E3 // [2] pp.18, pp.174

#define TOWNSIO_SOUND_MASTERSWITCH 0x4EC // [2] p.216

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

	unsigned char trackTime[6]=
	{
		TOC[3+trk*3],
		TOC[4+trk*3],
		TOC[5+trk*3],
		TOC[6+trk*3],
		TOC[7+trk*3],
		TOC[8+trk*3],
	};

	{
		unsigned char vol=0x7f;
		outp(TOWNSIO_ELEVOL_2_COM,(((~vol)>>2)&0x10)|5);
		outp(TOWNSIO_ELEVOL_2_DATA,vol&0x3f);

		outp(TOWNSIO_ELEVOL_2_COM,(((~vol)>>2)&0x10)|4);
		outp(TOWNSIO_ELEVOL_2_DATA,vol&0x3f);

		outp(TOWNSIO_SOUND_MASTERSWITCH,0xFF);
	}

	regs.x.ax=0x50C0;
	regs.x.cx=0xFF01;
	regs.x.di=(unsigned int)trackTime;
	int86(INT_DISKBIOS,&regs,&regs);
	if(regs.h.ah)
	{
		printf("CD-ROM BIOS Error AH=%02x CX=%04x\n",regs.h.ah,regs.x.cx);
		return 1;
	}

	return 0;
}
