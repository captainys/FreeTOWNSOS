#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <snd.h>

char SND_work[SndWorkSize];


int main(void)
{
	size_t sz;
	FILE *fp=fopen("../YS.FMB","rb");
	if(NULL==fp)
	{
		fp=fopen("YS.FMB","rb");
	}
	if(NULL!=fp)
	{
		char *fmb;

		fseek(fp,0,SEEK_END);
		sz=ftell(fp);
		sz-=8; // Skip FMB Name
		fmb=malloc(sz);
		if(NULL!=fmb)
		{
			int i,err;

			fseek(fp,0,SEEK_SET);
			fread(fmb,1,8,fp); // Skip FMB Name
			fread(fmb,1,sz,fp);
			fclose(fp);

			SND_init(SND_work);

			SND_elevol_mute(0x03); // WTF!  It says mute.  But, 1 means unmute.  Active-Low.  No mention in the red book.


			for(i=0; i+48<=sz; i+=48)
			{
				SND_inst_write(0,i/48,fmb+i);
			}

			SND_inst_change(0,0); // Ch=0, Inst=0

			for(i=0; i<127; ++i)
			{
				clock_t c0=clock();
				SND_key_on(0,i,64);
				while((clock()-c0)<CLOCKS_PER_SEC/20);
				_outb(0x2386,0x0C); // Capture F_NUM and BLOCK of YM2612 Ch0
				SND_key_off(0);
			}

			SND_end();

			free(fmb);
		}
		else
		{
			fclose(fp);
		}
	}
	printf("SND test done.\n");
	return 0;
}
