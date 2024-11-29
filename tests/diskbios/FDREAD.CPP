#include <stdio.h>
#include "diskbios.h"

unsigned char buf[4096];

int main(void)
{
	int actual=0;
	printf("DKB_restore returns %04xH\n",DKB_restore(BIOS_FD0));

	printf("DKB_read returns %04xH\n",DKB_read(BIOS_FD0,0,0,1, 1,(char *)buf,&actual));
	printf("actual number of sectors read=%d\n",actual);

	printf("Showing first 256 bytes.\n");
	for(int i=0; i<256; ++i)
	{
		printf("%02x ",buf[i]);
		if(15==i%16)
		{
			printf("\n");
		}
	}

	return 0;
}
