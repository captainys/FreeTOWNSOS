#include <cdrfrb.h>
#include <stdio.h>

int main(void)
{
	struct TIMEADRS tracktime[256],disktime;
	int cdtype,starttrack,endtrack;
	int ah=cdr_cdinfo(0xC0,&cdtype,&starttrack,&endtrack,tracktime,&disktime);
	int i;
	printf("Return %d\n",ah);
	printf("CD Type 0x%02x\n",cdtype);
	printf("Start Track %d\n",starttrack);
	printf("End Track %d\n",endtrack);
	printf("Disk Time %02d:%02d:%02d\n",disktime.min,disktime.sec,disktime.frame);
	for(i=0; i<endtrack+1-starttrack; ++i)
	{
		if(tracktime[i].min&0x80)
		{
			printf("DATA  ");
		}
		else
		{
			printf("AUDIO ");
		}
		printf("Track %02d:%02d:%02d\n",tracktime[i].min&0x7F,tracktime[i].sec,tracktime[i].frame);
	}
	return 0;
}
