#include <stdio.h>
#include <stdlib.h>


#define _STI _inline(0xfb);


int main(void)
{
	size_t sz;
	FILE *fp;

	fp=fopen("../CARDINAL.SND","rb");
	if(NULL==fp)
	{
		fp=fopen("CARDINAL.SND","rb");
	}
	if(NULL==fp)
	{
		printf("Cannot open CARDINAL.SND\n");
		_STI;
		return 1;
	}
	if(NULL!=fp)
	{
		char *snd;

		fseek(fp,0,SEEK_END);
		sz=ftell(fp);
		printf("File Size=%d\n",sz);

		snd=malloc(sz);
		if(NULL!=snd)
		{
			size_t realRead;

			fseek(fp,0,SEEK_SET);
			realRead=fread(snd,1,sz,fp);
			fclose(fp);

			printf("Read Size=%d\n",realRead);
			if(realRead!=sz)
			{
				printf("Read size and reported file size do not match!\n");
				_STI;
				return 1;
			}
		}
		else
		{
			printf("Cannot malloc.\n");
			_STI;
			return 1;
		}
	}
	_STI;
	return 0;
}
