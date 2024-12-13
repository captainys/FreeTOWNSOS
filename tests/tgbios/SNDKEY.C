#include <SND.H>
#include <time.h>
#include <stdio.h>


unsigned char *ReadBinaryFile(const char fName[])
{
	FILE *fp=fopen(fName,"rb");
	if(NULL!=fp)
	{
		unsigned char *data;
		size_t sz;

		fseek(fp,0,SEEK_END);
		sz=ftell(fp);
		fseek(fp,0,SEEK_SET);

		data=(unsigned char *)malloc(sz);
		fread(data,1,sz,fp);

		fclose(fp);
		return data;
	}
	return NULL;
}

int main(int argc,char *argv[])
{
	if(argc<2)
	{
		printf("Usage: free386 SNDKEY pmbfile.pmb\n");
		return 1;
	}

	unsigned char *pmb=ReadBinaryFile(argv[1]);
	if(NULL==pmb)
	{
		printf("Cannot read pmb\n");
		return 1;
	}

	clock_t t0=clock();
	unsigned int time=0,nextKeyOnTime=0;
	int prevSta=0xFF;
	int inst=0;

	SND_init();
	{
		int i;
		size_t offset=8;
		for(i=0; i<32; ++i)
		{
			SND_inst_write(0x40,i,pmb+offset);
			offset+=128;
		}
	}

	for(;;)
	{
		int sta;
		SND_joy_in_2(0,&sta);
		if(0==(sta&0x80))
		{
			break;
		}

		clock_t t=clock();
		clock_t dt=t-t0;
		t0=t;

		if(0!=(prevSta&0x01) && 0==(sta&0x01))
		{
			++inst;
			pritntf("Inst=%d\n",inst);
		}
		if(0!=(prevSta&0x02) && 0==(sta&0x02) && 0<inst)
		{
			--inst;
			pritntf("Inst=%d\n",inst);
		}
		prevSta=sta;

		if(0==(sta&0x10))
		{
			if(nextKeyOnTime<time)
			{

				nextKeyOnTime=time+CLOCKS_PER_SEC/20;
			}
		}

		time+=dt;
	}

	SND_end();

	return 0;
}
