#include <string.h>
#include "egb.h"
#include "snd.h"
#include "mos.h"
#include "fmcfrb.h"
#include "TGGUI.H"
#include "EGBCONST.H"

void TGGUI_InitInputSet(struct TGGUI_InputSet *set)
{
	memset(set,0,sizeof(struct TGGUI_InputSet));
}

void TGGUI_PollInput(struct TGGUI_InputSet *set)
{
	int pad,mos;
	SND_joy_in_2(0,&pad);
	set->pad=pad;

	MOS_openCheck(&mos);
	if(0!=(mos&1))
	{
		int btn,mx,my;
		MOS_rdpos(&btn,&mx,&my);
		set->mBtn=btn;
		set->mx=mx;
		set->my=my;
	}
	else
	{
		set->mBtn=0;
		set->mx=0;
		set->my=0;
	}

	{
		unsigned int keyCode,shift;
		keyCode=KYB_read(1,&shift);
		if(0xFF00==(keyCode&0xFF00))
		{
			keyCode=0;
		}
		else
		{
			keyCode&=0xFF;
		}
		set->inkey=keyCode;
		set->shift=shift;
	}
}

void TGGUI_PlaceTableTextItems(
	int nItems,
	struct TGGUI_SelectorItem items[],
	struct TGGUI_Rect *usableRect,
	const char *const texts[],
	int hSpace,int vSpace)
{
	int x0=usableRect->x0;
	int y0=usableRect->y0;
	int i;

	for(i=0; i<nItems; ++i)
	{
		items[i].type=TGGUI_ITEMTYPE_TABLE;
		items[i].rect.x0=x0;
		items[i].rect.y0=y0;
		items[i].rect.wid=strlen(texts[i])*FONT_WID+hSpace*2;
		items[i].rect.hei=FONT_HEI+vSpace*2;
		items[i].u.t.hSpace=hSpace;
		items[i].u.t.vSpace=vSpace;
		strncpy(items[i].label,texts[i],TGGUI_MAX_LABEL);
		items[i].label[TGGUI_MAX_LABEL-1]=0;

		x0+=items[i].rect.wid;
		if(usableRect->x0+usableRect->wid<=x0)
		{
			x0=usableRect->x0;
			y0+=FONT_HEI+vSpace*2;
		}
	}
}

void TGGUI_MakeVerticalTextItems(
    int nItems,
    struct TGGUI_SelectorItem items[],
    const char *const texts[])
{
	int i,y=0;
	for(i=0; i<nItems; ++i)
	{
		items[i].type=TGGUI_ITEMTYPE_STRING;
		items[i].rect.x0=0;
		items[i].rect.y0=y;
		items[i].rect.wid=strlen(texts[i])*FONT_WID;
		items[i].rect.hei=FONT_HEI;
		strncpy(items[i].label,texts[i],TGGUI_MAX_LABEL);
		items[i].label[TGGUI_MAX_LABEL-1]=0;
	}
}

void TGGUI_MakeVerticalFileSelectorItems(
    struct TGGUI_SelectorItem items[], // Must be as long as fileList.  Rect will be relative.
    struct DOSFileList *fileList)
{
	int i=0,y=0;
	while(NULL!=fileList)
	{
		items[i].type=TGGUI_ITEMTYPE_FILE;
		items[i].rect.x0=0;
		items[i].rect.y0=y;

		items[i].u.f.file=fileList->found;
		if(fileList->found.attrib&DOS_ATTR_DIRECTORY)
		{
			strcpy(items[i].label,"<DIR>   ");
		}
		else
		{
			Uitoa_fl(items[i].label,8,fileList->found.size);
		}
		items[i].label[8]=' ';
		strncpy(items[i].label+9,fileList->found.name,16);
		items[i].label[TGGUI_MAX_LABEL-1]=0;
		items[i].rect.wid=FONT_WID*11;

		fileList=fileList->next;
		y+=FONT_HEI;
		++i;
	}
}

int TGGUI_MakeFromTownsMENUV2Items(
	struct TGGUI_SelectorItem items[],
	struct TMENUITM *tmenuItmV2,unsigned int item0)
{
	int i=0,y=0;
	int item;
	int baseDepth=tmenuItmV2->icons[item0].depth;

	if(0<baseDepth)
	{
		items[i].type=TGGUI_ITEMTYPE_TMENUV2;
		items[i].rect.x0=0;
		items[i].rect.y0=y;
		items[i].rect.wid=tmenuItmV2->icons[item].labelLen*FONT_WID;
		items[i].rect.hei=FONT_HEI;
		strcpy(items[i].label,"GRP ..");
		items[i].u.tmv2.itemIndex=-1;
		++i;
	}

	for(item=item0; item<tmenuItmV2->nIcons; ++item,y+=FONT_HEI)
	{
		if(ICONTYPE_HEADER==tmenuItmV2->icons[item].iconType ||
		   ICONTYPE_V1GROUP==tmenuItmV2->icons[item].iconType)
		{
			continue;
		}

		if(tmenuItmV2->icons[item].depth==baseDepth)
		{
			items[i].type=TGGUI_ITEMTYPE_TMENUV2;
			items[i].rect.x0=0;
			items[i].rect.y0=y;
			items[i].rect.wid=tmenuItmV2->icons[item].labelLen*FONT_WID;
			items[i].rect.hei=FONT_HEI;

			if(ICONTYPE_GROUP==tmenuItmV2->icons[item].iconType)
			{
				strcpy(items[i].label,"GRP ");
			}
			else
			{
				strcpy(items[i].label,"    ");
			}
			strncpy(items[i].label+4,tmenuItmV2->icons[item].label,TGGUI_MAX_LABEL-4);

			items[i].label[TGGUI_MAX_LABEL-1]=0;
			items[i].u.tmv2.itemIndex=item;
			++i;
		}
		else if(tmenuItmV2->icons[item].depth<baseDepth)
		{
			break;
		}
	}
	return i;
}

void TGGUI_ClearRect(char *egbwork,struct TGGUI_Rect *R)
{
	short rect[4];
	rect[0]=R->x0;
	rect[1]=R->y0;
	rect[2]=R->x0+R->wid-1;
	rect[3]=R->y0+R->hei-1;
	EGB_paintMode(egbwork,0x20);
	EGB_color(egbwork,EGB_FILL_COLOR,0);
	EGB_writeMode(egbwork,EGB_OPAQUE);
	EGB_rectangle(egbwork,(char *)rect);
}
