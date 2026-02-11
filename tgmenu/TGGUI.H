#ifndef TGGUI_H_IS_INCLUDED
#define TGGUI_H_IS_INCLUDED

#include <dos.h>
#include <DOSCALL.H>
#include <TMENUITM.H>

#define FONT_WID 8
#define FONT_HEI 16
#define KANJI_WID (FONT_WID*2)

#define ASCII_BACK 8
#define ASCII_TAB 9
#define ASCII_RETURN 13
#define ASCII_ESC 27
#define ASCII_UP 30
#define ASCII_DOWN 31
#define ASCII_LEFT 29
#define ASCII_RIGHT 28

#define TGGUI_NOT_SELECTED -1


#define TGGUI_EVENT_NONE         0
#define TGGUI_EVENT_SELCHANGE    1
#define TGGUI_EVENT_TMPSELCHANGE 2
#define TGGUI_EVENT_SELECTED     3


struct TGGUI_InputSet
{
	unsigned short inkey,shift;
	short mx,my;
	unsigned char mBtn;
	unsigned char pad;
};

struct TGGUI_Rect
{
	short x0,y0;
	unsigned short wid,hei;
};

#define TGGUI_FRAME_INACTIVE 0
#define TGGUI_FRAME_ACTIVE 1

struct TGGUI_Frame
{
	int state;
	struct TGGUI_Rect rect;
};


/* --------------------------------------------------------------------------*/


struct TGGUI_TableSelectorItemProp
{
	int vSpace,hSpace;
};

struct TGGUI_FileSelectorItemProp
{
	struct _find_t file;
};

struct TGGUI_TownsMENUV2SelectorItemProp
{
	int itemIndex; /* -1: Go up a level. */
};

#define TGGUI_ITEMTYPE_STRING  0
#define TGGUI_ITEMTYPE_TABLE   1
#define TGGUI_ITEMTYPE_FILE    2
#define TGGUI_ITEMTYPE_TMENUV2 3
#define TGGUI_ITEMTYPE_TMENUV1 4

#define TGGUI_MAX_LABEL 80

struct TGGUI_SelectorItem
{
	int type;
	char label[TGGUI_MAX_LABEL];
	struct TGGUI_Rect rect;
	union
	{
		struct TGGUI_TableSelectorItemProp t;
		struct TGGUI_FileSelectorItemProp f;
		struct TGGUI_TownsMENUV2SelectorItemProp tmv2;
	} u;
};


struct TGGUI_VerticalSelector
{
	struct TGGUI_Rect rect;
	struct TGGUI_InputSet prevInput;
	int nItems;
	const struct TGGUI_SelectorItem *items;
	int curSel,topPos,nVisible;
};


/* --------------------------------------------------------------------------*/


#define TGGUI_SEL_TMPSEL 1	// Have separate temporary and current positions.

struct TGGUI_TableSelector
{
	unsigned int flags;

	struct TGGUI_Rect rect;
	struct TGGUI_InputSet prevInput;
	int nItems;
	const struct TGGUI_SelectorItem *items;
	int curSel;
	int tmpSel;
};

/* TGGUI.C */
void TGGUI_InitInputSet(struct TGGUI_InputSet *set);
void TGGUI_PollInput(struct TGGUI_InputSet *set);
void TGGUI_PlaceTableTextItems(
	int nItems,
	struct TGGUI_SelectorItem items[], // Must be as long as the number of texts.
	struct TGGUI_Rect *usableRect,
	const char *const texts[],
	int hSpace,int vSpace);  //
void TGGUI_MakeVerticalTextItems(
    int nItems,
    struct TGGUI_SelectorItem items[],
    const char *const texts[]);
int TGGUI_MakeFromTownsMENUV2Items(
	struct TGGUI_SelectorItem items[],
	struct TMENUITM *tmenuItmV2,unsigned int item0);
void TGGUI_MakeVerticalFileSelectorItems(
    struct TGGUI_SelectorItem items[], // Must be as long as fileList.  Rect will be relative to the top left of the usable rect.
    struct DOSFileList *fileList);

void TGGUI_ClearRect(char *egbwork,struct TGGUI_Rect *R);

/* FRAME.C */
void TGGUI_Frame_Init(struct TGGUI_Frame *frame);
void TGGUI_Frame_Draw(char *egbwork,const struct TGGUI_Frame *frame);
void TGGUI_Frame_ClearUsableRect(char *egbwork,struct TGGUI_Frame *frame);
void TGGUI_FrameRectToUsableRect(struct TGGUI_Rect *rect,const struct TGGUI_Frame *frame);

/* SELECT.C */
void TGGUI_DrawItems(char *egbwork,int nItems,const struct TGGUI_SelectorItem items[]);

/* Vertical Selector */
void TGGUI_DrawStringsVertical(char *egbwork,int x0,int y0,int nItems,const char *const items[]);

void TGGUI_VerticalSelector_Init(struct TGGUI_VerticalSelector *selector,struct TGGUI_Rect *rect);
int TGGUI_VerticalSelector_Redraw(char *egbwork,struct TGGUI_VerticalSelector *selector);
void TGGUI_VerticalSelector_Highlight(char *egbwork,struct TGGUI_VerticalSelector *selector,int sel);

int TGGUI_VerticalSelector_RunOneStep(char *egbwork,struct TGGUI_VerticalSelector *selector,struct TGGUI_InputSet *input);

/* Table Selector */
void TGGUI_TableSelector_Init(struct TGGUI_TableSelector *selector,struct TGGUI_Rect *rect,int nItems,const struct TGGUI_SelectorItem *items);
void TGGUI_TableSelector_Highlight(char *egbwork,struct TGGUI_TableSelector *selector,int curSel,int tmpSel);
int TGGUI_TableSelector_RunOneStep(char *egbwork,struct TGGUI_TableSelector *selector,struct TGGUI_InputSet *input);

#endif
