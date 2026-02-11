#include <stdio.h>
#include "DEF.H"
#include "TGGUI.H"
#include "EGBCONST.H"
#include "EGBDEF.H"


void TGGUI_DrawItems(char *egbwork,int nItems,const struct TGGUI_SelectorItem items[])
{
	int i;
	struct EGB_string str;
	EGB_writeMode(egbwork,EGB_OPAQUE);
	EGB_color(egbwork,EGB_FOREGROUND_COLOR,0xFFFF);
	EGB_paintMode(egbwork,0x2);
	for(i=0; i<nItems; ++i)
	{
		short rect[4];
		rect[0]=items[i].rect.x0;
		rect[1]=items[i].rect.y0;
		rect[2]=items[i].rect.x0+items[i].rect.wid-1;
		rect[3]=items[i].rect.y0+items[i].rect.hei-1;
		EGB_rectangle(egbwork,(char *)rect);

		str.x=items[i].rect.x0+items[i].u.t.hSpace;
		str.y=items[i].rect.y0+items[i].u.t.vSpace+FONT_HEI-1;
		strncpy(str.str,items[i].label,80);
		str.len=SMALLER(strlen(items[i].label),80);
		EGB_sjisString(egbwork,(char *)&str);
	}
}

/*!
  x0  Left.
  y0  Top.
*/
void TGGUI_DrawStringsVertical(char *egbwork,int x0,int y0,int nItems,const char *const items[])
{
	int i;
	struct EGB_string str;
	str.x=x0;
	str.y=y0+FONT_HEI-1;

	EGB_writeMode(egbwork,EGB_OPAQUE);
	EGB_color(egbwork,EGB_FOREGROUND_COLOR,0xFFFF);
	for(i=0; i<nItems; ++i)
	{
		strncpy(str.str,items[i],80);
		str.len=SMALLER(strlen(items[i]),80);
		EGB_sjisString(egbwork,(char *)&str);
		str.y+=FONT_HEI;
	}
}

void TGGUI_DrawItemsVertical(char *egbwork,int x0,int y0,int nItems,const struct TGGUI_SelectorItem items[])
{
	int i;
	struct EGB_string str;
	str.x=x0;
	str.y=y0+FONT_HEI-1;

	EGB_writeMode(egbwork,EGB_OPAQUE);
	EGB_color(egbwork,EGB_FOREGROUND_COLOR,0xFFFF);
	for(i=0; i<nItems; ++i)
	{
		strncpy(str.str,items[i].label,80);
		str.len=SMALLER(strlen(items[i].label),80);
		EGB_sjisString(egbwork,(char *)&str);
		str.y+=FONT_HEI;
	}
}

void TGGUI_VerticalSelector_Init(struct TGGUI_VerticalSelector *selector,struct TGGUI_Rect *rect)
{
	selector->rect=*rect;
	selector->nItems=0;
	selector->items=0;
	selector->curSel=TGGUI_NOT_SELECTED;
	selector->topPos=0;
	selector->nVisible=rect->hei/FONT_HEI;

	TGGUI_InitInputSet(&selector->prevInput);
}

void TGGUI_VerticalSelector_Highlight(char *egbwork,struct TGGUI_VerticalSelector *selector,int sel)
{
	if(sel!=TGGUI_NOT_SELECTED)
	{
		unsigned short rect[4];
		EGB_writeMode(egbwork,EGB_XOR);

		rect[0]=selector->rect.x0;
		rect[1]=selector->rect.y0+(sel-selector->topPos)*FONT_HEI;
		rect[2]=rect[0]+selector->rect.wid;
		rect[3]=rect[1]+FONT_HEI;

		EGB_color(egbwork,EGB_FILL_COLOR,0xFFFF);
		EGB_paintMode(egbwork,0x22);
		EGB_rectangle(egbwork,(char *)rect);
	}
}

static DrawUpArrow(char *egbwork,struct TGGUI_VerticalSelector *selector)
{
	struct TGGUI_Rect rect;
	rect.x0=selector->rect.x0;
	rect.y0=selector->rect.y0;
	rect.wid=selector->rect.wid;
	rect.hei=FONT_HEI-1;

	TGGUI_ClearRect(egbwork,&rect);
	struct EGB_string str;
	str.x=rect.x0+FONT_WID*8;
	str.y=rect.y0+FONT_HEI-1;
	str.len=2;
	str.str[0]=0x81; // Up triangle.
	str.str[1]=0xA3;
	EGB_sjisString(egbwork,(char *)&str);
}

static DrawDownArrow(char *egbwork,struct TGGUI_VerticalSelector *selector)
{
	struct TGGUI_Rect rect;
	rect.x0=selector->rect.x0;
	rect.y0=selector->rect.y0+(selector->nVisible-1)*FONT_HEI;
	rect.wid=selector->rect.wid;
	rect.hei=FONT_HEI-1;

	TGGUI_ClearRect(egbwork,&rect);
	struct EGB_string str;
	str.x=rect.x0+FONT_WID*8;
	str.y=rect.y0+FONT_HEI-1;
	str.len=2;
	str.str[0]=0x81; // Down triangle
	str.str[1]=0xA5;
	EGB_sjisString(egbwork,(char *)&str);
}

int TGGUI_VerticalSelector_Redraw(char *egbwork,struct TGGUI_VerticalSelector *selector)
{
	int nShow;
	int upArrow=0,downArrow=0;

	if(0<selector->topPos)
	{
		upArrow=1;
	}

	TGGUI_ClearRect(egbwork,&selector->rect);

	nShow=selector->nVisible;
	if(selector->nItems<selector->topPos+nShow)
	{
		nShow=selector->nItems-selector->topPos;
	}

	if(selector->topPos+selector->nVisible<selector->nItems)
	{
		downArrow=1;
	}

	TGGUI_DrawItemsVertical(egbwork,selector->rect.x0,selector->rect.y0,nShow,selector->items+selector->topPos);
	TGGUI_VerticalSelector_Highlight(egbwork,selector,selector->curSel);

	if(0!=upArrow)
	{
		DrawUpArrow(egbwork,selector);
	}
	if(0!=downArrow)
	{
		DrawDownArrow(egbwork,selector);
	}

	return 0;
}

int TGGUI_VerticalSelector_RunOneStep(char *egbwork,struct TGGUI_VerticalSelector *selector,struct TGGUI_InputSet *input)
{
	int returnValue=TGGUI_EVENT_NONE;
	int pad,nextSel;
	pad=input->pad;

	nextSel=selector->curSel;
	if((0!=(selector->prevInput.pad&1) && 0==(pad&1)) ||
	   ASCII_UP==input->inkey)
	{
		nextSel=GREATER(0,selector->curSel-1);
	}
	else if((0!=(selector->prevInput.pad&2) && 0==(pad&2)) ||
	        ASCII_DOWN==input->inkey)
	{
		nextSel=SMALLER(selector->nItems-1,selector->curSel+1);
	}

	if((0!=(selector->prevInput.pad&0x10) && 0==(pad&0x10)) ||
	   ASCII_RETURN==input->inkey ||
	   ' '==input->inkey)
	{
		returnValue=TGGUI_EVENT_SELECTED;
	}

	if(selector->curSel!=nextSel)
	{
		returnValue=TGGUI_EVENT_SELCHANGE;
		TGGUI_VerticalSelector_Highlight(egbwork,selector,selector->curSel);
		TGGUI_VerticalSelector_Highlight(egbwork,selector,nextSel);
		selector->curSel=nextSel;

		if(selector->curSel<=selector->topPos)
		{
			if(0<selector->curSel)
			{
				selector->topPos=selector->curSel-1;
			}
			else
			{
				selector->topPos=0;
			}
			TGGUI_VerticalSelector_Redraw(egbwork,selector);
		}
		if(selector->topPos+selector->nVisible-2<=selector->curSel)
		{
			selector->topPos=selector->curSel-selector->nVisible+2;
			TGGUI_VerticalSelector_Redraw(egbwork,selector);
		}
	}

	selector->prevInput=*input;

	return returnValue;
}


void TGGUI_TableSelector_Init(struct TGGUI_TableSelector *selector,struct TGGUI_Rect *rect,int nItems,const struct TGGUI_SelectorItem *items)
{
	selector->flags=0;
	TGGUI_InitInputSet(&selector->prevInput);
	selector->rect=*rect;
	selector->nItems=nItems;
	selector->items=items;
	selector->curSel=TGGUI_NOT_SELECTED;
	selector->tmpSel=TGGUI_NOT_SELECTED;
}
void TGGUI_TableSelector_Highlight(char *egbwork,struct TGGUI_TableSelector *selector,int curSel,int tmpSel)
{
	if(curSel!=TGGUI_NOT_SELECTED && curSel<selector->nItems)
	{
		struct TGGUI_Rect itemRect=selector->items[curSel].rect;

		unsigned short rect[4];
		rect[0]=itemRect.x0;
		rect[1]=itemRect.y0;
		rect[2]=rect[0]+itemRect.wid-1;
		rect[3]=rect[1]+itemRect.hei-1;

		EGB_writeMode(egbwork,EGB_XOR);
		EGB_color(egbwork,EGB_FILL_COLOR,0xFFFF);
		EGB_paintMode(egbwork,0x22);
		EGB_rectangle(egbwork,(char *)rect);
	}
	if(selector->flags&TGGUI_SEL_TMPSEL)
	{
		if(tmpSel!=TGGUI_NOT_SELECTED && tmpSel<selector->nItems)
		{
			struct TGGUI_Rect itemRect=selector->items[tmpSel].rect;

			unsigned short rect[4];
			rect[0]=itemRect.x0-2;
			rect[1]=itemRect.y0-2;
			rect[2]=rect[0]+itemRect.wid+2;
			rect[3]=rect[1]+itemRect.hei+2;

			EGB_writeMode(egbwork,EGB_XOR);
			EGB_color(egbwork,EGB_FILL_COLOR,0xFFFF);
			EGB_paintMode(egbwork,0x2);
			EGB_rectangle(egbwork,(char *)rect);
		}
	}
}
int TGGUI_TableSelector_RunOneStep(char *egbwork,struct TGGUI_TableSelector *selector,struct TGGUI_InputSet *input)
{
	int returnValue=TGGUI_EVENT_NONE;
	int pad,nextSel,nextTmpSel;
	pad=input->pad;

	nextSel=selector->curSel;
	nextTmpSel=selector->tmpSel;
	if((0!=(selector->prevInput.pad&4) && 0==(pad&4)) ||
	   ASCII_LEFT==input->inkey ||
	   ' '==input->inkey)
	{
		if(selector->flags&TGGUI_SEL_TMPSEL)
		{
			nextTmpSel=GREATER(0,selector->tmpSel-1);
		}
		else
		{
			nextSel=GREATER(0,selector->curSel-1);
		}
	}
	else if((0!=(selector->prevInput.pad&8) && 0==(pad&8)) ||
	        ASCII_RIGHT==input->inkey)
	{
		if(selector->flags&TGGUI_SEL_TMPSEL)
		{
			nextTmpSel=SMALLER(selector->nItems-1,selector->tmpSel+1);
		}
		else
		{
			nextSel=SMALLER(selector->nItems-1,selector->curSel+1);
		}
	}

	if((0!=(selector->prevInput.pad&0x10) && 0==(pad&0x10)) ||
	   ASCII_RETURN==input->inkey ||
	   ' '==input->inkey)
	{
		if(selector->flags&TGGUI_SEL_TMPSEL)
		{
			nextSel=selector->tmpSel;
		}
		else
		{
			returnValue=TGGUI_EVENT_SELECTED;
		}
	}

	if(selector->curSel!=nextSel)
	{
		returnValue=TGGUI_EVENT_SELCHANGE;
	}
	else if(selector->tmpSel!=nextTmpSel)
	{
		returnValue=TGGUI_EVENT_TMPSELCHANGE;
	}

	if(selector->curSel!=nextSel ||
	   selector->tmpSel!=nextTmpSel)
	{
		TGGUI_TableSelector_Highlight(egbwork,selector,selector->curSel,selector->tmpSel);
		selector->curSel=nextSel;
		selector->tmpSel=nextTmpSel;
		TGGUI_TableSelector_Highlight(egbwork,selector,selector->curSel,selector->tmpSel);
	}

	selector->prevInput=*input;

	return returnValue;
}
