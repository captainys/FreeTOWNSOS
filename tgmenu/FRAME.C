#include "TGGUI.H"
#include "EGBCONST.H"

void TGGUI_Frame_Init(struct TGGUI_Frame *frame)
{
	frame->state=TGGUI_FRAME_INACTIVE;
}
void TGGUI_Frame_Draw(char *egbwork,const struct TGGUI_Frame *frame)
{
	int i;

	EGB_writeMode(egbwork,EGB_OPAQUE);
	EGB_color(egbwork,EGB_FOREGROUND_COLOR,0xffff);
	EGB_paintMode(egbwork,0x02);

	unsigned short rect[4];
	rect[0]=frame->rect.x0;
	rect[1]=frame->rect.y0;
	rect[2]=frame->rect.x0+frame->rect.wid-1;
	rect[3]=frame->rect.y0+frame->rect.hei-1;

	EGB_rectangle(egbwork,(char *)rect);

	if(TGGUI_FRAME_INACTIVE==frame->state)
	{
		EGB_color(egbwork,EGB_FOREGROUND_COLOR,0);
	}

	for(i=0; i<2; ++i)
	{
		++rect[0];
		++rect[1];
		--rect[2];
		--rect[3];
		EGB_rectangle(egbwork,(char *)rect);
	}
}

void TGGUI_Frame_ClearUsableRect(char *egbwork,struct TGGUI_Frame *frame)
{
	struct TGGUI_Rect usable;
	TGGUI_FrameRectToUsableRect(&usable,frame);
	TGGUI_ClearRect(egbwork,&usable);
}

void TGGUI_FrameRectToUsableRect(struct TGGUI_Rect *rect,const struct TGGUI_Frame *frame)
{
	*rect=frame->rect;
	rect->x0+=8;
	rect->y0+=8;
	rect->wid-=16;
	rect->hei-=16;
}
