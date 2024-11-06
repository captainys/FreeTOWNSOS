#include <stdio.h>

// Test program to find AUTOEXEC.BAT in the ISO image.
// Reference  https://wiki.osdev.org/ISO_9660

#define OK 0
#define ERR 1
#define SECTOR_LEN 2048
unsigned char sectorBuff[SECTOR_LEN];

#define PVD_PATHTABLE_SIZE_LE 132
#define PVD_PATHTABLE_SIZE_BE 136
#define PVD_PATHTABLE_LBA_LE 140
#define PVD_PATHTABLE_OPT_LBA_LE 144
#define PVD_PATHTABLE_LBA_BE 148
#define PVD_PATHTABLE_OPT_LBA_BE 152
#define PVD_ROOTDIR_LE 158
#define PVD_ROOTDIR_LEN 166

#define PATHTABLE_LEN 0
#define PATHTABLE_EXTATTRIB 1
#define PATHTABLE_LBA 2
#define PATHTABLE_PARENTINDEX 6
#define PATHTABLE_NAME 8

#define CD_DIR_DIR_LEN 0
#define CD_DIR_EXT_ATTR_LEN 1
#define CD_DIR_DATA_LBA 2
#define CD_DIR_DATA_LEN 10
#define CD_DIR_FLAGS 25
#define CD_DIR_VOL_SEQ 28
#define CD_DIR_FILENAME_LEN 32
#define CD_DIR_FILENAME 33

unsigned int GetDwordLE(const unsigned char *ptr)
{
	return *((const unsigned int *)ptr);
}

unsigned int GetDwordBE(const unsigned char *ptr)
{
	unsigned int i;
	i=ptr[0];
	i<<=8;
	i|=ptr[1];
	i<<=8;
	i|=ptr[2];
	i<<=8;
	i|=ptr[3];
	return i;
}

unsigned int GetWordLE(const unsigned char *ptr)
{
	return *((const unsigned short *)ptr);
}

unsigned int GetWordBE(const unsigned char *ptr)
{
	unsigned int i;
	i=ptr[0];
	i<<=8;
	i|=ptr[1];
	return i;
}

int ReadSector(const char fn[],unsigned int LBA)
{
	FILE *fp=fopen(fn,"rb");
	if(NULL!=fp)
	{
		size_t read;
		fseek(fp,LBA*SECTOR_LEN,SEEK_SET);
		read=fread(sectorBuff,1,SECTOR_LEN,fp);
		fclose(fp);
		return (read==SECTOR_LEN ? OK : ERR);
	}
	return ERR;
}

int main(int ac,char *av[])
{
	if(OK==ReadSector(av[1],16))
	{
		unsigned int LBA,count;

		unsigned int rootDirLBA=GetDwordLE(sectorBuff+PVD_ROOTDIR_LE);
		unsigned int rootDirLEN=GetDwordLE(sectorBuff+PVD_ROOTDIR_LEN);

		printf("%c%c%c%c%c\n",sectorBuff[1],sectorBuff[2],sectorBuff[3],sectorBuff[4],sectorBuff[5]);
		printf("RootDir LBA=0x%08x\n",rootDirLBA);
		printf("RootDir LEN=0x%08x\n",rootDirLEN);

		unsigned int pathTableSizeLE=GetDwordLE(sectorBuff+PVD_PATHTABLE_SIZE_LE);
		unsigned int pathTableSizeBE=GetDwordBE(sectorBuff+PVD_PATHTABLE_SIZE_BE);
		unsigned int pathTableLE_LBA=GetDwordLE(sectorBuff+PVD_PATHTABLE_LBA_LE);
		unsigned int pathTableBE_LBA=GetDwordBE(sectorBuff+PVD_PATHTABLE_LBA_BE);

		printf("PATH_TABLE_SIZE %d %d\n",pathTableSizeLE,pathTableSizeBE);
		printf("PATH_TABLE_LBA %d %d\n",pathTableLE_LBA,pathTableBE_LBA);
		printf("PATH_TABLE_OPT_LBA %d %d\n",GetDwordLE(sectorBuff+PVD_PATHTABLE_OPT_LBA_LE),GetDwordBE(sectorBuff+PVD_PATHTABLE_OPT_LBA_BE));

		ReadSector(av[1],pathTableLE_LBA);
		{
			unsigned int ptr=0;
			while(ptr<pathTableSizeLE)
			{
				unsigned int nameLen=sectorBuff[ptr];
				unsigned int attr=sectorBuff[ptr+PATHTABLE_EXTATTRIB];
				unsigned int parentIdx=GetWordLE(sectorBuff+ptr+PATHTABLE_PARENTINDEX);
				unsigned int LBA=GetDwordLE(sectorBuff+ptr+PATHTABLE_LBA);
				unsigned int len=nameLen+8;
				int i;

				printf("L:%d  ATTR:%02x  LBA:%d  PARENT:%d  NAME:",nameLen,attr,LBA,parentIdx);
				for(i=0; i<nameLen; ++i)
				{
					printf("%c",sectorBuff[ptr+PATHTABLE_NAME+i]);
				}

				for(i=0; i<nameLen; ++i)
				{
					printf("[%d]",sectorBuff[ptr+PATHTABLE_NAME+i]);
				}

				printf("\n");

				len=(len+1)&0xFE;
				ptr+=len;
			}
		}


		LBA=rootDirLBA;
		count=(rootDirLEN+SECTOR_LEN-1)/SECTOR_LEN;
		size_t AUTOEXEC_LBA=0,AUTOEXEC_LEN=0;
		while(0<count)
		{
			size_t ptr=0;

			ReadSector(av[1],LBA);

			while(ptr<SECTOR_LEN)
			{
				unsigned int dirLEN=sectorBuff[ptr];
				if(0==dirLEN || SECTOR_LEN<ptr+dirLEN)
				{
					// Limitation: If the file crosses the sector border, it won't find.
					break;
				}

				unsigned int fileNameLEN=sectorBuff[ptr+CD_DIR_FILENAME_LEN];
				if(0!=fileNameLEN &&
				   0==strncmp((char *)sectorBuff+ptr+CD_DIR_FILENAME,"AUTOEXEC.BAT",fileNameLEN) ||
				   0==strncmp((char *)sectorBuff+ptr+CD_DIR_FILENAME,"AUTOEXEC.BAT;1",fileNameLEN))
				{
					AUTOEXEC_LEN=GetDwordLE(sectorBuff+ptr+CD_DIR_DATA_LEN);
					AUTOEXEC_LBA=GetDwordLE(sectorBuff+ptr+CD_DIR_DATA_LBA);
				}

				printf("L:%d ",sectorBuff[ptr+CD_DIR_DIR_LEN]);
				printf("ATTR:0x%02x ",sectorBuff[ptr+CD_DIR_EXT_ATTR_LEN]);
				printf("FLAGS:0x%02x ",sectorBuff[ptr+CD_DIR_FLAGS]);
				printf("LBA:%d ",GetDwordLE(sectorBuff+ptr+CD_DIR_DATA_LBA));
				printf("NAMELEN:%d ",fileNameLEN);

				for(int i=0; i<fileNameLEN; ++i)
				{
					printf("%c",sectorBuff[ptr+CD_DIR_FILENAME+i]);
				}
				for(int i=0; i<fileNameLEN; ++i)
				{
					printf("[%d]",sectorBuff[ptr+CD_DIR_FILENAME+i]);
				}

				printf("\n");

				ptr+=dirLEN;
			}

			++LBA;
			--count;
		}

		if(0!=AUTOEXEC_LEN && 0!=AUTOEXEC_LBA)
		{
			int i;
			printf("Found AUTOEXEC.BAT\n");
			printf("Length LBA=%d LEN=%d\n",AUTOEXEC_LBA,AUTOEXEC_LEN);

			ReadSector(av[1],AUTOEXEC_LBA);
			for(i=0; i<AUTOEXEC_LEN; ++i)
			{
				printf("%c",sectorBuff[i]);
			}
			printf("\n");
		}
	}
	return 0;
}
