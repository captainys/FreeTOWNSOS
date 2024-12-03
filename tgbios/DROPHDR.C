#include <stdio.h>
#include <string.h>
#include <stdlib.h>

unsigned char buf[1024*1024];

int MakeSymbolTable(unsigned char binary[],size_t length,const char fileName[])
{
	if(4+16<=length)
	{
		if('S'==binary[0] &&
		   'Y'==binary[1] &&
		   'M'==binary[2])
		{
			printf("Confirmed 'SYM' Keyword.\n");
			printf("%c",(char)binary[0]);
			printf("%c",(char)binary[1]);
			printf("%c",(char)binary[2]);
			printf("%c",(char)binary[3]);
			printf("\n");

			printf("Subsequent 16 bytes:");
			for(int i=0; i<16 && 4+i<length; ++i)
			{
				printf("%02x ",binary[4+i]);
			}
			printf("\n");
		}
		else
		{
			printf("SYM not found.\n");
			return 1;
		}
	}

	unsigned int SYMtoProgInfo=*(unsigned short *)(binary+4);
	unsigned int ProgInfoSize=*(unsigned int *)(binary+6);
	unsigned int symbolSize=*(unsigned int *)(binary+10);

	printf("SYM1 to Program Info: %02xh bytes.\n",SYMtoProgInfo);
	printf("Program Info Size: %08xh bytes.\n",ProgInfoSize);
	printf("Symbols Size: %08xh bytes.\n",symbolSize);

	unsigned int symbolStartOffset=SYMtoProgInfo+ProgInfoSize;

	FILE *fp=fopen(fileName,"w");
	if(NULL==fp)
	{
		printf("Cannot open symbol file.\n");
		return 1;
	}

	unsigned int offset=0;
	while(offset<symbolSize)
	{
		char label[256];
		int n=binary[symbolStartOffset+offset];
		int filled=0;
		++offset;
		if(0==n)
		{
			continue;
		}
		while(0!=n)
		{
			label[filled++]=binary[symbolStartOffset+offset];
			label[filled]=0;
			++offset;
			--n;
		}

		unsigned int addr=*(unsigned int *)(binary+symbolStartOffset+offset);
		offset+=4;

		//printf("%-32s ",label);

		//printf(" %08x",addr);

		//printf(" %02x %02x\n",binary[symbolStartOffset+offset],binary[symbolStartOffset+offset+1]);
		offset+=2;

		// Assume Code Segment is 000Ch
		// i486DXCommon::FarPointer ptr;
		// ptr.SEG=0x000C;
		// ptr.OFFSET=addr;
		// SetImportedLabel(ptr,label);

		fprintf(fp,"/begin0\n");
		fprintf(fp,"T 1\n");
		fprintf(fp,"* 0110:%08x\n",addr);
		fprintf(fp,"# \n");
		fprintf(fp,"R \n");
		fprintf(fp,"L %s\n",label);
		fprintf(fp,"P \n");
		fprintf(fp,"% 0\n");
		fprintf(fp,"M 0\n");
		fprintf(fp,"B 0\n");
		fprintf(fp,"A 0\n");
		fprintf(fp,"O 0\n");
		fprintf(fp,"X \n");
		fprintf(fp,"/end\n");
	}

	fclose(fp);

	return 0;
}

size_t FindSymbolTable(const unsigned char buf[],size_t size,size_t BIOSStart)
{
	if(BIOSStart+4<size)
	{
		size_t i;
		for(i=size-4; BIOSStart<size; --i)
		{
			if(0==memcmp(buf+i,"SYM1",4))
			{
				size_t to_program=buf[i+4];
				if(0==memcmp(buf+i+to_program+1,"PROGRAM",7))
				{
					return i;
				}
			}
		}
	}
	return 0;
}

int main(int ac,char *av[])
{
	FILE *fp;
	size_t sz;
	size_t BIOSStart=0x200,BIOSEnd=0;
	size_t rexSize,rexSizeLow,rexSizeHigh;
	size_t symTabOffset=0,symTabSize=0;

	if(ac<3)
	{
		printf("Usage: RUN386 DROPHDR.EXP REXFILE.REX BINFILE.BIN (SYMFILE.SYM)\n");
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

	rexSizeLow=*(unsigned short *)(buf+2);
	rexSizeHigh=*(unsigned short *)(buf+4);
	rexSize=rexSizeLow+0x200*rexSizeHigh;
	symTabOffset=FindSymbolTable(buf,sz,BIOSStart); // REX size actually doesn't make sense.  rexSize-512 ???
	if(0!=symTabOffset)
	{
		symTabSize=sz-symTabOffset;
		BIOSEnd=symTabOffset;
	}
	else
	{
		symTabSize=0;
		BIOSEnd=sz;
	}

	fp=fopen(av[2],"wb");
	if(NULL==fp)
	{
		printf("Cannot open %s for write.\n",av[2]);
		return 1;
	}
	fwrite(buf+BIOSStart,1,BIOSEnd-BIOSStart,fp);
	fclose(fp);

	if(0!=symTabOffset && 0!=symTabSize && 4<=ac)
	{
		if(0!=MakeSymbolTable(buf+symTabOffset,symTabSize,av[3]))
		{
			return 1;
		}
	}

	printf("BIOS Start=%08xh\n",BIOSStart);
	printf("BIOS End=%08xh\n",BIOSEnd);
	printf("Symbol Table Offset=%08xh\n",symTabOffset);
	printf("Symbol Table Size=%08xh\n",symTabSize);

	return 0;
}
