#include <stdio.h>
#include <stdint.h>

struct SNDHeader
{
	char name[8];
	uint32_t ID;
	uint32_t length;
	uint32_t loopStart;
	uint32_t loopLength;
	uint16_t freq;
	uint16_t freqAdjust;
	uint8_t baseNote;
	uint8_t zero[3];
};

int main(int ac,char *av[])
{
	FILE *fp=fopen(av[1],"rb");
	struct SNDHeader header;
	unsigned char readBuf[1024];
	if(NULL!=fp)
	{
		fread(&header,1,sizeof(struct SNDHeader),fp);

		printf("ID:             %08x\n",header.ID);
		printf("Length:         %d\n",header.length);
		printf("Loop Start:     %d\n",header.loopStart);
		printf("Loop Length:    %d\n",header.loopLength);
		printf("Sampling Freq:  %d\n",header.freq);
		printf("Freq Adjust:    %d\n",header.freqAdjust);
		printf("Base Note:      %d\n",header.baseNote);

		unsigned int readLen=0,ptr=0;
		while(0<(readLen=fread(readBuf,1,sizeof(readBuf),fp)))
		{
			int i;
			for(i=0; i<sizeof(readBuf); ++i)
			{
				if(0xFF==readBuf[i])
				{
					printf("Loop Stop Code at: %d\n",ptr+i);
				}
			}
			ptr+=readLen;
		}

		fclose(fp);

		return 0;
	}
	else
	{
		printf("Cannot open file.\n");
		return 1;
	}
}
