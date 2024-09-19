#include <stdio.h>

unsigned char buf[1024*1024];

int main(int ac,char *av[])
{
	FILE *fp;
	size_t sz;
	size_t BIOSStart=0x200;

	if(ac<3)
	{
		printf("Usage: RUN386 DROPHDR.EXP REXFILE.REX BINFILE.BIN\n");
		return 1;
	}


	fp=fopen(av[1],"rb");
	if(NULL==fp)
	{
		printf("Cannot open %s for read.\n",av[1]);
		return 1;
	}
	sz=fread(buf,1,1024*1024,fp);
	fclose(fp);

	for(BIOSStart=0; BIOSStart<sz; ++BIOSStart)
	{
		if(0==memcmp(buf+BIOSStart,"TSUGARU BIOS",12))
		{
			break;
		}
	}
	if(sz==BIOSStart)
	{
		printf("Cannot find BIOS Header (TSUGARU BIOS).\n");
		return 1;
	}

	fp=fopen(av[2],"wb");
	if(NULL==fp)
	{
		printf("Cannot open %s for write.\n",av[2]);
		return 1;
	}
	fwrite(buf+BIOSStart,1,sz-BIOSStart,fp);
	fclose(fp);

	return 0;
}
