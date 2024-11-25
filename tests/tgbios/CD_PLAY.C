#include <cdrfrb.h>
#include <stdio.h>

int main(void)
{
	struct TIMEADRS tracktime[256],disctime;
	int cdtype,starttrack,endtrack;
	int ah=cdr_cdinfo(0xC0,&cdtype,&starttrack,&endtrack,tracktime,&disctime);
	int i,firstAudioTrack=-1;
	printf("Return %d\n",ah);
	printf("CD Type 0x%02x\n",cdtype);
	printf("Start Track %d\n",starttrack);
	printf("End Track %d\n",endtrack);
	printf("Disc Time %02d:%02d:%02d\n",disctime.min,disctime.sec,disctime.frame);
	for(i=0; i<endtrack+1-starttrack; ++i)
	{
		if(tracktime[i].min&0x80)
		{
			printf("DATA  ");
		}
		else
		{
			printf("AUDIO ");
			if(firstAudioTrack<0)
			{
				firstAudioTrack=i;
			}
		}
		printf("Track %02d:%02d:%02d\n",tracktime[i].min&0x7F,tracktime[i].sec,tracktime[i].frame);
	}

	if(0<=firstAudioTrack)
	{
		struct TIMEADRS starttime,endtime;
		starttime=tracktime[firstAudioTrack];
		if(firstAudioTrack+1<=endtrack)
		{
			endtime=tracktime[firstAudioTrack+1];
		}
		else
		{
			endtime=disctime;
		}
		cdr_mtplay(0xC0,&starttime,&endtime);
	}

	return 0;
}
