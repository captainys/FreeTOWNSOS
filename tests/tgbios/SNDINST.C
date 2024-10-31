#include <stdio.h>
#include <stdlib.h>
#include <snd.h>

char SND_work[SndWorkSize];

char FMB[6152];
char PMB[65536];


int main(void)
{
	size_t sz;
	FILE *fpFMB=fopen("../YS.FMB","rb");
	FILE *fpPMB=fopen("../YS.PMB","rb");
	if(NULL==fpFMB)
	{
		fpFMB=fopen("YS.FMB","rb");
	}
	if(NULL==fpPMB)
	{
		fpPMB=fopen("YS.PMB","rb");
	}
	if(NULL!=fpFMB && NULL!=fpPMB)
	{
		size_t FMB_size=fread(FMB,1,6152,fpFMB);
		size_t PMB_size=fread(PMB,1,65536,fpPMB);

		fclose(fpFMB);
		fclose(fpPMB);

		SND_init(SND_work);

		SND_inst_write(0,5,FMB+8);

		SND_inst_write(64,1,PMB+8);
		// Maximum Instrument Number (DH) for RF5C68 is 1FH.
		// It only copies 128 bytes from the pointer given.

		SND_end();
	}
	printf("SND instrument test done.\n");
	return 0;
}
