#include <stdio.h>
#include "dosdiskc.h"


unsigned char data[720*1024];

int main(void)
{
	DOSDISK disk;
	if(DOSDISK_NOERR!=DOSDISK_CreateFD(&disk,BPB_MEDIA_720K,sizeof(data),data))
	{
		printf("Failed to create a disk image.\n");
		return 1;
	}

	printf("Disk Image Created.\n");


	if(DOSDISK_WriteVolumeLabel(&disk,"CAPTAIN_YS",
	    12,00,00,
	    1973,1,13))
	{
		printf("Error making directory.\n");
		return 1;
	}

	printf("Volume Label Created.\n");

	FILE *fp=fopen("testvollabel.bin","wb");
	if(NULL!=fp)
	{
		fwrite(data,1,sizeof(data),fp);
		fclose(fp);
	}
	else
	{
		printf("Error writing a disk.\n");
		return 1;
	}

	return 0;
}
