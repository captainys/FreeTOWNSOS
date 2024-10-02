#include <DOS.H>
#include <CONIO.H>
#include "TGBIOS.H"
#include "EGB.H"
#include "MACRO.H"
#include "IODEF.H"
#include "UTIL.H"

void SwapShort(short *a,short *b)
{
	short c=*a;
	*a=*b;
	*b=c;
}

void EGB_MakeP0SmallerThanP1(struct POINTW *p0,struct POINTW *p1)
{
	if(p0->x>p1->x)
	{
		SwapShort(&p0->x,&p1->x);
	}
	if(p0->y>p1->y)
	{
		SwapShort(&p0->y,&p1->y);
	}
}

struct EGB_PagePointerSet EGB_GetPagePointerSet(_Far struct EGB_Work *work)
{
	struct EGB_PagePointerSet pointerSet;
	if(work->writePage<2)
	{
		pointerSet.page=&work->perPage[work->writePage];
		if(EGB_INVALID_SCRNMODE!=pointerSet.page->screenMode)
		{
			pointerSet.mode=EGB_GetScreenModeProp(pointerSet.page->screenMode);
			if(EGB_INVALID_SCRNMODE==work->perPage[1].screenMode)
			{
				_FP_SEG(pointerSet.vram)=SEG_VRAM_1PG;
				_FP_OFF(pointerSet.vram)=0;
				pointerSet.vramSize=VRAM_SIZE;
			}
			else
			{
				_FP_SEG(pointerSet.vram)=SEG_VRAM_2PG;
				_FP_OFF(pointerSet.vram)=(VRAM_SIZE/2)*work->writePage;
				pointerSet.vramSize=VRAM_SIZE/2;
			}
			return pointerSet;
		}
	}
	else if(0x80<=work->writePage && work->writePage<0x84)
	{
		unsigned int vPageIdx=(work->writePage&3);
		if(NULL!=work->virtualPage[vPageIdx].vram)
		{
			pointerSet.page=&work->perVirtualPage[vPageIdx];
			pointerSet.mode=&work->virtualPage[vPageIdx];
			pointerSet.vram=work->virtualPage[vPageIdx].vram;
			pointerSet.vramSize=(pointerSet.mode->bytesPerLine*pointerSet.mode->size.y);
			return pointerSet;
		}
	}
	pointerSet.page=NULL;
	pointerSet.mode=NULL;
	pointerSet.vram=NULL;
	return pointerSet;
}
