#include <stdio.h>
#include <conio.h>
#include <dos.h>
#include <i86.h>
#include "diskbios.h"

int main(void)
{
	unsigned char buf[256];

	printf("DKB_restore returns %04xH\n",DKB_restore(BIOS_FD0));

	for(int i=0; i<16; ++i)
	{
		unsigned short err=DKB_rdsecid(BIOS_FD0,0,0,buf);
		printf("ERR:%04x ",err);
		for(int j=0; j<6; ++j)
		{
			printf("%02x",buf[j]);
		}
		printf("\n");
	}

	return 0;
}
