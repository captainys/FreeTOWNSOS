#include <stdio.h>
#include <stdlib.h>

#pragma pack(1)  // For Visual C++

struct PartitionHeader
{
	char Fujitsu[6];
	unsigned int firstPartitionLBA;
	unsigned int sectorsFromFirstPartition;
	unsigned short two;  // What is it?
	unsigned char nothing[16];
};

struct Partition
{
	unsigned char isBootPartition;
	unsigned char type;  // 00:unused  01:MS-DOS Basic  06:MS-DOS 512(Prob)  90H:OASYS
	unsigned int three;  // Always 3
	unsigned int length; // Number of sectors
	char nothing[6];
	char typeName[16];
	char name[16];
};

struct PartitionTable
{
	struct PartitionHeader header;
	struct Partition entry[10];
};



#define READ_ERROR 0xFFFFFFFF
#define SECTOR_LENGTH 512
#define LBA_PARTITION_TABLE 1
#define MAX_NUM_PARTITIONS 10

unsigned int ReadSector(void *ptr,const char fileName[],unsigned int LBA,unsigned int len)
{
	FILE *fp=fopen(fileName,"rb");
	if(NULL!=fp)
	{
		size_t sz;
		fseek(fp,LBA*SECTOR_LENGTH,SEEK_SET);
		sz=fread(ptr,1,len*SECTOR_LENGTH,fp);
		fclose(fp);
		return sz;
	}
	return READ_ERROR;
}

struct PartitionTable partition;

unsigned int GetPartitionTopLBA(const struct PartitionTable *partition,unsigned int partID)
{
	unsigned int i,accum=partition->header.firstPartitionLBA;
	if(MAX_NUM_PARTITIONS<=partID || 0==partition->entry[partID].type)
	{
		return 0;
	}
	for(i=0; i<partID; ++i)
	{
		accum+=partition->entry[i].length;
	}
	return accum;
}

const char *GetPartitionTypeName(char str[33],const struct PartitionTable *partition,unsigned int partID)
{
	unsigned int i,accum=partition->header.firstPartitionLBA;
	str[32]=0;
	if(MAX_NUM_PARTITIONS<=partID || 0==partition->entry[partID].type)
	{
		return NULL;
	}
	for(i=0; i<32; ++i)
	{
		str[i]=partition->entry[partID].typeName[i];
	}
	str[32]=0;
	return str;
}

const char *GetPartitionName(char str[33],const struct PartitionTable *partition,unsigned int partID)
{
	unsigned int i,accum=partition->header.firstPartitionLBA;
	str[32]=0;
	if(MAX_NUM_PARTITIONS<=partID || 0==partition->entry[partID].type)
	{
		return NULL;
	}
	for(i=0; i<32; ++i)
	{
		str[i]=partition->entry[partID].name[i];
	}
	str[32]=0;
	return str;
}

int main(int ac,char *av[])
{
	if(ac<2)
	{
		return 1;
	}

	if(SECTOR_LENGTH!=ReadSector(&partition,av[1],LBA_PARTITION_TABLE,1))
	{
		printf("Cannot read the partition table.\n");
		return 0;
	}

	{
		int i;
		for(i=0; i<MAX_NUM_PARTITIONS; ++i)
		{
			char str[33];
			unsigned int LBA0=GetPartitionTopLBA(&partition,i);
			if(0!=LBA0)
			{
				printf("++++++++\n");
				printf("%s\n",GetPartitionTypeName(str,&partition,i));
				printf("%s\n",GetPartitionName(str,&partition,i));

				printf("[%d] %8d ",i,LBA0);
				printf("\n");
			}
		}
	}

	return 0;
}
