#include <stdio.h>
#include <stdlib.h>
#include "TMENUITM.H"


unsigned int ToUint(unsigned char *data)
{
	return (*((unsigned int *)(data)));
}
unsigned short ToUshort(unsigned char *data)
{
	return (*((unsigned short *)(data)));
}

static unsigned int CountItem(struct TMENUITM *itm)
{
	unsigned int nItems=0;
	unsigned int offset=0;
	while(offset+8<=itm->fileSize)
	{
		unsigned int size=ToUint(itm->data+offset+4);
		++nItems;
		offset+=8+size;
	}
	return nItems;
}

static void RecognizeItems(struct TMENUITM *itm)
{
	unsigned int offset=0;
	unsigned int i;
	for(i=0; i<itm->nIcons && offset+8<itm->fileSize; ++i)
	{
		unsigned char *dataPtr=itm->data+offset;
		struct TMENUIcon *icon=itm->icons+i;

		icon->offset=offset;
		icon->iconType=ToUint(dataPtr);
		icon->size=ToUint(dataPtr+4);

		icon->depth=0; /* Tentative */

		switch(icon->iconType)
		{
		case ICONTYPE_HEADER:
			icon->labelLen=8;
			icon->label="(Header)";
			break;
		case ICONTYPE_GROUP:
			icon->iconBitmapId=dataPtr[8];
			icon->unknownBytes[0]=dataPtr[9];
			icon->unknownBytes[1]=dataPtr[0x0A];
			icon->unknownBytes[2]=dataPtr[0x0B];
			icon->unknownBytes[3]=dataPtr[0x0C];
			icon->unknownBytes[4]=dataPtr[0x0D];
			icon->unknownBytes[5]=0;

			dataPtr+=0x0E;
			icon->labelLen=dataPtr[0];
			icon->label=(char *)dataPtr+1;
			dataPtr+=1+icon->labelLen+1; /* 1 for commandLen, 1 for \0 */

			icon->endOfGroupOffset=ToUint(dataPtr);

			icon->x0=ToUshort(dataPtr+4);
			icon->y0=ToUshort(dataPtr+6);
			icon->x1=ToUshort(dataPtr+8);
			icon->x1=ToUshort(dataPtr+10);
			break;
		case ICONTYPE_PROGRAM:
			icon->iconBitmapId=dataPtr[8];
			icon->unknownBytes[0]=dataPtr[9];
			icon->unknownBytes[1]=0;
			icon->unknownBytes[2]=0;
			icon->unknownBytes[3]=0;
			icon->unknownBytes[4]=0;
			icon->unknownBytes[5]=0;

			dataPtr+=0x0A;

			icon->labelLen=dataPtr[0];
			icon->label=(char *)dataPtr+1;
			dataPtr+=1+icon->labelLen+1; /* 1 for commandLen, 1 for \0 */

			icon->commandLen=ToUshort(dataPtr);
			icon->command=(char *)dataPtr+2;
			dataPtr+=2+icon->commandLen+1;

			icon->paramLen=ToUshort(dataPtr);
			icon->param=(char *)dataPtr+2;
			dataPtr+=2+icon->paramLen+1;

			icon->flags=ToUint(dataPtr);

			icon->x0=ToUshort(dataPtr+4);
			icon->y0=ToUshort(dataPtr+6);
			icon->x1=ToUshort(dataPtr+8);
			icon->x1=ToUshort(dataPtr+10);
			break;
		case ICONTYPE_V1GROUP:
			icon->unknownBytes[0]=dataPtr[8];
			icon->unknownBytes[1]=dataPtr[9];
			icon->unknownBytes[2]=dataPtr[0x0A];
			icon->unknownBytes[3]=dataPtr[0x0B];
			icon->unknownBytes[4]=dataPtr[0x0C];
			icon->unknownBytes[5]=dataPtr[0x0D];

			dataPtr+=0x0E;
			icon->labelLen=dataPtr[0];
			icon->label=(char *)dataPtr+1;
			dataPtr+=1+icon->labelLen+1; /* 1 for commandLen, 1 for \0 */

			icon->endOfGroupOffset=ToUint(dataPtr);

			icon->x0=ToUshort(dataPtr+4);
			icon->y0=ToUshort(dataPtr+6);
			icon->x1=ToUshort(dataPtr+8);
			icon->x1=ToUshort(dataPtr+10);
			break;
		}
		offset+=8+icon->size;
	}
}

static void RecognizeDepth(struct TMENUITM *itm,unsigned int *iPtr,unsigned int endOfGroupOffset,struct TMENUIcon *groupIcon)
{
	while(*iPtr<itm->nIcons)
	{
		struct TMENUIcon *icon=&itm->icons[*iPtr];

		if(endOfGroupOffset<=icon->offset)
		{
			return;
		}

		if(NULL!=groupIcon)
		{
			icon->depth=groupIcon->depth+1;
		}

		++(*iPtr);
		if(ICONTYPE_GROUP==icon->iconType)
		{
			RecognizeDepth(itm,iPtr,icon->endOfGroupOffset,icon);
		}
	}
}

void InitTMENUITM(struct TMENUITM *itm)
{
	itm->fileSize=0;
	itm->data=NULL;
	itm->nIcons=0;
	itm->icons=0;
}

int LoadTMENUITM(struct TMENUITM *itm,const char fileName[])
{
	FILE *fp;
	fp=fopen(fileName,"rb");
	InitTMENUITM(itm);
	if(NULL!=fp)
	{
		fseek(fp,0,SEEK_END);
		itm->fileSize=ftell(fp);
		fseek(fp,0,SEEK_SET);

		itm->data=(unsigned char *)malloc(itm->fileSize);
		if(NULL==itm->data)
		{
			fclose(fp);
			DeleteTMENUITM(itm);
			return TMENUITM_ERR;
		}

		fread(itm->data,1,itm->fileSize,fp);
		fclose(fp);

		itm->nIcons=CountItem(itm);
		itm->icons=(struct TMENUIcon *)malloc(sizeof(struct TMENUIcon)*itm->nIcons);
		if(NULL==itm->icons)
		{
			/* File is already closed. */
			DeleteTMENUITM(itm);
			return TMENUITM_ERR;
		}

		RecognizeItems(itm);
		{
			unsigned int i=0;
			RecognizeDepth(itm,&i,itm->fileSize,NULL);
		}

		return TMENUITM_OK;
	}
	return TMENUITM_ERR;
}

void DeleteTMENUITM(struct TMENUITM *itm)
{
	if(NULL!=itm->data)
	{
		free(itm->data);
	}
	if(NULL!=itm->icons)
	{
		free(itm->icons);
	}
	InitTMENUITM(itm);
}

/*****************************************************************************/

void InitTMENUINF(struct TMENUINF *inf)
{
	inf->fileSize=0;
	inf->data=NULL;
	inf->nIcons=0;
	inf->icons=NULL;
}

int LoadTMENUINF(struct TMENUINF *inf,const char fileName[])
{
	FILE *fp;
	fp=fopen(fileName,"rb");
	InitTMENUINF(inf);
	if(NULL!=fp)
	{
		int i;

		fseek(fp,0,SEEK_END);
		inf->fileSize=ftell(fp);
		fseek(fp,0,SEEK_SET);

		if(0!=(inf->fileSize&0x7F))
		{
			fclose(fp);
			return TMENUITM_ERR;
		}

		inf->data=(unsigned char *)malloc(inf->fileSize);
		if(NULL==inf->data)
		{
			fclose(fp);
			DeleteTMENUINF(inf);
			return TMENUITM_ERR;
		}

		fread(inf->data,1,inf->fileSize,fp);
		fclose(fp);

		inf->nIcons=(inf->fileSize>>7);
		inf->icons=(struct TMENUV1Icon *)malloc(sizeof(struct TMENUV1Icon)*inf->nIcons);
		if(NULL==inf->icons)
		{
			DeleteTMENUINF(inf);
			return TMENUITM_ERR;
		}

		for(i=0; i<inf->nIcons; ++i)
		{
			inf->icons[i].offset=i*128;
			inf->icons[i].eightDotThree=(char *)inf->data+i*128;
			inf->icons[i].iconBitmapId=inf->data[i*128+14];
			inf->icons[i].flags=inf->data[i*128+0x19];
			inf->icons[i].label=(char *)inf->data+i*128+0x20;
			inf->icons[i].commandLine=(char *)inf->data+i*128+0x40;
		}
	}

	return TMENUITM_OK;
}

void DeleteTMENUINF(struct TMENUINF *inf)
{
	if(NULL!=inf->data)
	{
		free(inf->data);
	}
	if(NULL!=inf->icons)
	{
		free(inf->icons);
	}
	InitTMENUINF(inf);
}
