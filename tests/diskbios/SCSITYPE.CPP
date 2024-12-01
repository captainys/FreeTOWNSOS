#include <stdio.h>
#include <i86.h>
#include "diskbios.h"

int main(void)
{
	for(int SCSIID=0; SCSIID<7; ++SCSIID)
	{
		union REGS regs;
		regs.h.ah=0x76; // Tsugaru OS Extension
		regs.h.al=0xB0+SCSIID;
		int86(INT_DISKBIOS,&regs,&regs);

		printf("SCSI ID:%d ",SCSIID);
		if(0==regs.h.ah)
		{
			printf("DevType=%02x",regs.h.al);
			switch(regs.h.al)
			{
			case 0:
				printf(" HDD");
				break;
			case 4:
			case 5:
				printf(" CD");
				break;
			default:
				printf(" Unknown");
				break;
			}
		}
		else
		{
			printf("Error");
		}
		printf("\n");
	}
	return 0;
}
