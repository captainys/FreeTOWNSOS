#include <stdio.h>

unsigned char buf[1024*1024];

int main(int ac,char *av[])
{
	FILE *fp;
	size_t sz;

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

	if(sz<0x200)
	{
		printf("%s is too small.\n",av[1]);
	}

	fp=fopen(av[2],"wb");
	if(NULL==fp)
	{
		printf("Cannot open %s for write.\n",av[2]);
		return 1;
	}
	fwrite(buf+0x200,1,sz-0x200,fp);
	fclose(fp);

	return 0;
}
