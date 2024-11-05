#include <stdio.h>
#include <stdlib.h>
#include <snd.h>

char SND_work[SndWorkSize];


int main(int ac,char *av[])
{
	size_t sz;
	FILE *fp;

	if(2<=ac)
	{
		fp=fopen(av[1],"rb");
	}
	else
	{
		fp=fopen("../CARDINAL.SND","rb");
		if(NULL==fp)
		{
			fp=fopen("CARDINAL.SND","rb");
		}
	}
	if(NULL==fp)
	{
		printf("Cannot open CARDINAL.SND\n");
		return 1;
	}
	if(NULL!=fp)
	{
		char *snd;

		fseek(fp,0,SEEK_END);
		sz=ftell(fp);
		snd=malloc(sz);
		if(NULL!=snd)
		{
			int err;

			fseek(fp,0,SEEK_SET);
			fread(snd,1,sz,fp);
			fclose(fp);

			SND_init(SND_work);

			SND_elevol_mute(0x03); // WTF!  It says mute.  But, 1 means unmute.  Active-Low.  No mention in the red book.

			err=SND_pcm_mode_set(1);
			printf("%d\n",err);
			err=SND_pcm_play2(71,snd[28],127,snd);
			printf("%d\n",err);

			while(SND_pcm_status(71)!=0);

			SND_end();

			free(snd);
		}
		else
		{
			fclose(fp);
		}
	}
	printf("SND test done.\n");
	return 0;
}
