#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>

#define MAX_PATH 260
#define MAX_PATTERN 256


char infile[MAX_PATH],outfile[MAX_PATH];

int nSrcPtn=0;
unsigned char srcPtn[MAX_PATTERN];
int nNewPtn=0;
unsigned char newPtn[MAX_PATTERN];


struct Binary
{
	size_t len;
	unsigned char *data;
};

struct Binary bin;

void Help(void)
{
	printf("-infile filename.bin\n");
	printf("-in filename.bin\n");
	printf("  Input\n");
	printf("-outfile filename.bin\n");
	printf("-out filename.bin\n");
	printf("  Output\n");
	printf("-from AA BB CC DD ...\n");
	printf("-src AA BB CC DD ...\n");
	printf("  Source pattern\n");
	printf("  Always two letters per byte.  Do not add 0x, $, H, or &H.\n");
	printf("-to AA BB CC DD ...\n");
	printf("-dst AA BB CC DD ...\n");
	printf("  Source pattern\n");
	printf("  Always two letters per byte.  Do not add 0x, $, H, or &H.\n");
}

#define INFILE_SET  1
#define OUTFILE_SET 2
#define FROMPTN_SET 4
#define TOPTN_SET   8

#define ALL_SET 15

int HexByteToNumber(char a)
{
	a=toupper(a);
	if('0'<=a && a<='9')
	{
		return a-'0';
	}
	else if('A'<=a && a<='F')
	{
		return 10+a-'A';
	}
	return -1;
}

int ReadPattern(unsigned char ptn[],int ac,char *av[],int i)
{
	int len=0;
	while(i<ac && 0<=HexByteToNumber(av[i][0]) && 0<=HexByteToNumber(av[i][1]))
	{
		ptn[len++]=(HexByteToNumber(av[i][0])<<4)|HexByteToNumber(av[i][1]);
		++i;
	}
	return len;
}

int RecognizeCommandParameters(int ac,char *av[])
{
	int i;
	unsigned int flags=0;
	infile[0]=0;
	outfile[0]=0;
	for(i=1; i<ac; ++i)
	{
		int j;
		for(j=0; 0!=av[i][j]; ++j)
		{
			av[i][j]=tolower(av[i][j]);
		}
		if(0==strcmp(av[i],"-infile") || 0==strcmp(av[i],"-in"))
		{
			if(i+1<ac)
			{
				strncpy(infile,av[i+1],MAX_PATH-1);
				flags|=INFILE_SET;
				infile[MAX_PATH-1]=0;
				++i;
			}
			else
			{
				printf("Insufficient argument for -infile\n");
				return -1;
			}
		}
		else if(0==strcmp(av[i],"-outfile") || 0==strcmp(av[i],"-out"))
		{
			if(i+1<ac)
			{
				strncpy(outfile,av[i+1],MAX_PATH-1);
				flags|=OUTFILE_SET;
				outfile[MAX_PATH-1]=0;
				++i;
			}
			else
			{
				printf("Insufficient argument for -outfile\n");
				return -1;
			}
		}
		else if(0==strcmp(av[i],"-from") || 0==strcmp(av[i],"-src"))
		{
			nSrcPtn=ReadPattern(srcPtn,ac,av,i+1);
			printf("From:");
			for(j=0; j<nSrcPtn; ++j)
			{
				printf(" %02x",srcPtn[j]);
			}
			printf("\n");
			i+=nSrcPtn;
			flags|=FROMPTN_SET;
		}
		else if(0==strcmp(av[i],"-to") || 0==strcmp(av[i],"-to"))
		{
			int nNewPtn=ReadPattern(newPtn,ac,av,i+1);
			printf("To:  ");
			for(j=0; j<nNewPtn; ++j)
			{
				printf(" %02x",newPtn[j]);
			}
			printf("\n");
			i+=nNewPtn;
			flags|=TOPTN_SET;
		}
		else if(0==strcmp(av[i],"-h") || 0==strcmp(av[i],"-help") || 0==strcmp(av[i],"-?"))
		{
			Help();
			return 1;
		}
		else
		{
			printf("Unrecognized Parameter %s\n",av[i]);
			return -1;
		}
	}

	if(flags!=ALL_SET)
	{
		printf("Missing parameter(s)\n");
		return -1;
	}

	return 0;
}


struct Binary ReadBinaryFile(const char fName[])
{
	FILE *fp=fopen(fName,"rb");
	struct Binary bin;
	bin.len=0;
	bin.data=NULL;
	if(NULL!=fp)
	{
		fseek(fp,0,SEEK_END);
		bin.len=ftell(fp);
		fseek(fp,0,SEEK_SET);

		bin.data=(unsigned char *)malloc(bin.len);
		fread(bin.data,1,bin.len,fp);

		fclose(fp);
	}
	return bin;
}

int ApplyPatch(void)
{
	int n=0;
	for(size_t i=0; i+nSrcPtn<=bin.len && i+nNewPtn<=bin.len; ++i)
	{
		if(0==memcmp(bin.data+i,srcPtn,nSrcPtn))
		{
			++n;
			printf("Found at %08x\n",(unsigned int)i);
			memcpy(bin.data+i,newPtn,nNewPtn);
		}
	}
	return n;
}

int main(int ac,char *av[])
{
	int cmdParam=RecognizeCommandParameters(ac,av);
	if(cmdParam<0)
	{
		Help();
		return 1;
	}
	else if(0<cmdParam)
	{
		return 1;
	}

	bin=ReadBinaryFile(infile);
	if(0==bin.len || NULL==bin.data)
	{
		printf("Failed to read file\n");
		return 1;
	}

	if(0<ApplyPatch())
	{
		FILE *fp=fopen(outfile,"wb");
		if(NULL!=fp)
		{
			size_t len=fwrite(bin.data,1,bin.len,fp);
			fclose(fp);
			if(len==bin.len)
			{
				printf("Patched.\n");
			}
			else
			{
				printf("Write error\n");
				return 1;
			}
		}
		else
		{
			printf("Cannot open output file.\n");
			return 1;
		}
	}
	else
	{
		printf("Pattern not found.\n");
		return 1;
	}
	return 0;
}
