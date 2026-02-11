/* LICENSE>>
Copyright 2020 Soji Yamakawa (CaptainYS, http://www.ysflight.com)

Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.

2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.

3. Neither the name of the copyright holder nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

<< LICENSE */

#include <stdlib.h>
#include <string.h>
#include <egb.h>
#include <snd.h>
#include <mos.h>
#include <fmcfrb.h>
#include <conio.h>
#include <dos.h>
#include "EGBDEF.H"
#include "EGBCONST.H"
#include "TGGUI.H"
#include "CUTIL.H"
#include "DOSCALL.H"
#include "TMENUITM.H"


#define TSUGARU_STATUS _outp(0xEB,0);
#define TSUGARU_BREAK _outp(0xEA,0);


static char EGB_work[EgbWorkSize];

void Set4BitColors(void)
{
	EGB_resolution(EGB_work,0,3);
	EGB_resolution(EGB_work,1,3);

	EGB_writePage(EGB_work,1);
	EGB_clearScreen(EGB_work);

	EGB_writePage(EGB_work,0);
	EGB_clearScreen(EGB_work);
}

const char *const testStr[]=
{
	"FM",
	"TOWNS",
	"EMULATOR",
	"TSUGARU",
};

const char *const drives[]=
{
	"A:","B:","C:","D:","E:","F:","G:","H:","I:","J:","K:","L:","M:","N:","O:","P:","Q:",
};

const char *const types[]=
{
	"TownsMENU Items","Files"
};

void ChangeActive(unsigned curr,unsigned next,unsigned nFrames,struct TGGUI_Frame *frames[])
{
	if(curr<nFrames)
	{
		frames[curr]->state=TGGUI_FRAME_INACTIVE;
		TGGUI_Frame_Draw(EGB_work,frames[curr]);
	}
	if(next<nFrames)
	{
		frames[next]->state=TGGUI_FRAME_ACTIVE;
		TGGUI_Frame_Draw(EGB_work,frames[next]);
	}
}

/*
0--------------------------------------------------------------
(Text area)
20-------------------------------------------------------------
[A:][B:].....[Q:]                                               (32 pixels high)
52-------------------------------------------------------------
[TMENU Items][Files]                                            (32 pixels high)
84-------------------------------------------------------------
PATH
104------------------------------------------------------------

                                                                (360 pixels high)

464------------------------------------------------------------
(Text area)
480------------------------------------------------------------

*/


struct GUIApp
{
	int terminate;
	int activeFrame;
	struct DOSFileList *fileList;
	int nItems;
	struct TMENUITM tmenuItemV2;
	int tmenuItemGroup;

	struct TGGUI_Frame drive_frame;
	struct TGGUI_TableSelector driveSel;
	struct TGGUI_SelectorItem driveItems[17];

	struct TGGUI_Frame type_frame;
	struct TGGUI_TableSelector typeSel;
	struct TGGUI_SelectorItem typeItems[2];

	struct TGGUI_Frame item_frame;
	struct TGGUI_VerticalSelector itemSel;
	struct TGGUI_SelectorItem *items;

	struct TGGUI_Frame *frames[3];
};

#define TITLE_Y0 0
#define TITLE_HEI 20

#define DRIVE_FRAME_Y0 (TITLE_Y0+TITLE_HEI)
#define DRIVE_FRAME_HEI 32

#define TYPE_FRAME_Y0 (DRIVE_FRAME_Y0+DRIVE_FRAME_HEI)
#define TYPE_FRAME_HEI 32

#define PATH_Y0 (TYPE_FRAME_Y0+TYPE_FRAME_HEI)
#define PATH_HEI 20

#define ITEM_FRAME_Y0 (PATH_Y0+PATH_HEI)
#define ITEM_FRAME_HEI 360

#define FOOT_Y0 (ITEM_FRAME_Y0+ITEM_FRAME_HEI)
#define FOOT_HEI 16

char GetSelectedDriveLetter(struct GUIApp *app)
{
	return 'A'+app->driveSel.curSel;
}

void RefreshPath(struct GUIApp *app)
{
	int err=0;
	short rect[4];
	struct EGB_string str;

	rect[0]=0;
	rect[1]=PATH_Y0;
	rect[2]=639;
	rect[3]=PATH_Y0+PATH_HEI-1;
	EGB_paintMode(EGB_work,0x20);
	EGB_color(EGB_work,EGB_FILL_COLOR,0);
	EGB_writeMode(EGB_work,EGB_OPAQUE);
	EGB_rectangle(EGB_work,(char *)rect);

	str.x=0;
	str.y=PATH_Y0+2+FONT_HEI;
	if(0==app->typeSel.curSel) // Item
	{
		str.str[0]=0;
	}
	else // File
	{
		char c=GetSelectedDriveLetter(app);
		err=DOSGETCWD(str.str,c);
	}

	str.len=strlen(str.str);
	EGB_paintMode(EGB_work,0x02);
	EGB_sjisString(EGB_work,(char *)&str);
}

void RefreshItem(struct GUIApp *app)
{
	struct TGGUI_Rect rect;
	TGGUI_FrameRectToUsableRect(&rect,&app->item_frame);
	TGGUI_ClearRect(EGB_work,&rect);
	TGGUI_VerticalSelector_Init(&app->itemSel,&rect);

	if(NULL!=app->items)
	{
		free(app->items);
		app->items=NULL;
	}
	app->nItems=0;

	if(0==app->typeSel.curSel) // Item
	{
		DeleteTMENUITM(&app->tmenuItemV2);
		LoadTMENUITM(&app->tmenuItemV2,"\\TMENU.ITM");

		if(0<app->tmenuItemV2.nIcons)
		{
			app->items=(struct TGGUI_SelectorItem *)malloc(sizeof(struct TGGUI_SelectorItem)*app->tmenuItemV2.nIcons);
		}
		app->nItems=TGGUI_MakeFromTownsMENUV2Items(app->items,&app->tmenuItemV2,app->tmenuItemGroup);
	}
	else // File
	{
		DOSFreeFiles(app->fileList);
		app->fileList=NULL;

		struct DOSFileList *fileList=DOSGetFiles("*.*");
		app->nItems=DOSCountFiles(fileList);
		if(0<app->nItems)
		{
			app->items=(struct TGGUI_SelectorItem *)malloc(sizeof(struct TGGUI_SelectorItem)*app->nItems);
			TGGUI_MakeVerticalFileSelectorItems(app->items,fileList);
		}
	}

	app->itemSel.curSel=0;
	app->itemSel.topPos=0;
	app->itemSel.nItems=app->nItems;
	app->itemSel.items=app->items;
	TGGUI_VerticalSelector_Redraw(EGB_work,&app->itemSel);
}

void GUIApp_Setup(struct GUIApp *app)
{
	struct TGGUI_Rect rect;
	unsigned int currentDrive;

	_dos_getdrive(&currentDrive);

	app->terminate=0;
	app->nItems=0;
	app->fileList=NULL;
	app->items=NULL;
	InitTMENUITM(&app->tmenuItemV2);
	app->tmenuItemGroup=0;

	/* Drive Selector */
	TGGUI_Frame_Init(&app->drive_frame);
	app->drive_frame.rect.x0=0;
	app->drive_frame.rect.y0=DRIVE_FRAME_Y0;
	app->drive_frame.rect.wid=640;
	app->drive_frame.rect.hei=DRIVE_FRAME_HEI;

	TGGUI_FrameRectToUsableRect(&rect,&app->drive_frame);
	TGGUI_PlaceTableTextItems(17,app->driveItems,&rect,drives,2,2);
	TGGUI_TableSelector_Init(&app->driveSel,&rect,17,app->driveItems);
	app->driveSel.flags|=TGGUI_SEL_TMPSEL;
	app->driveSel.curSel=currentDrive-1;
	app->driveSel.tmpSel=currentDrive-1;


	/* Type Frame (Items or Files) */
	TGGUI_Frame_Init(&app->type_frame);
	app->type_frame.rect.x0=0;
	app->type_frame.rect.y0=52;
	app->type_frame.rect.wid=640;
	app->type_frame.rect.hei=32;

	TGGUI_FrameRectToUsableRect(&rect,&app->type_frame);
	TGGUI_PlaceTableTextItems(2,app->typeItems,&rect,types,2,2);
	TGGUI_TableSelector_Init(&app->typeSel,&rect,2,app->typeItems);
	app->typeSel.tmpSel=0;


	/* Item(File) Selector Frame */
	TGGUI_Frame_Init(&app->item_frame);
	app->item_frame.rect.x0=0;
	app->item_frame.rect.y0=104;
	app->item_frame.rect.wid=640;
	app->item_frame.rect.hei=360;

	TGGUI_FrameRectToUsableRect(&rect,&app->item_frame);
	TGGUI_VerticalSelector_Init(&app->itemSel,&rect);
	app->itemSel.nItems=0;
	app->itemSel.items=NULL;
	app->itemSel.curSel=0;

	if(TMENUITM_OK==LoadTMENUITM(&app->tmenuItemV2,"\\TMENU.ITM"))
	{
		app->typeSel.curSel=0;
	}
	else
	{
		app->typeSel.curSel=1;
	}
	RefreshItem(app);

	/* */
	app->activeFrame=2;
	app->frames[0]=&app->drive_frame;
	app->frames[1]=&app->type_frame;
	app->frames[2]=&app->item_frame;
	app->frames[app->activeFrame]->state=TGGUI_FRAME_ACTIVE;
}

void GUIApp_Draw(struct GUIApp *app)
{
	struct EGB_string str;
	EGB_writeMode(EGB_work,EGB_OPAQUE);
	EGB_color(EGB_work,EGB_FOREGROUND_COLOR,0xFFFF);
	EGB_paintMode(EGB_work,0x2);
	str.x=0;
	str.y=18;
	strcpy(str.str,"TSUGARU MENU by CaptainYS");
	str.len=strlen(str.str);
	EGB_sjisString(EGB_work,&str);

	TGGUI_Frame_Draw(EGB_work,&app->drive_frame);
	TGGUI_Frame_Draw(EGB_work,&app->type_frame);
	TGGUI_Frame_Draw(EGB_work,&app->item_frame);

	TGGUI_DrawItems(EGB_work,17,app->driveItems);
	TGGUI_TableSelector_Highlight(EGB_work,&app->driveSel,app->driveSel.curSel,app->driveSel.tmpSel);
	TGGUI_DrawItems(EGB_work,2,app->typeItems);
	TGGUI_TableSelector_Highlight(EGB_work,&app->typeSel,app->typeSel.curSel,app->typeSel.tmpSel);

	TGGUI_VerticalSelector_Redraw(EGB_work,&app->itemSel);
}

void GUIApp_RunOneStep(struct GUIApp *app,struct TGGUI_InputSet input)
{
	int evt=TGGUI_EVENT_NONE;
	switch(app->activeFrame)
	{
	case 0:
		evt=TGGUI_TableSelector_RunOneStep(EGB_work,&app->driveSel,&input);
		if(TGGUI_EVENT_SELCHANGE==evt)
		{
			unsigned int c=GetSelectedDriveLetter(app);
			_dos_setdrive(c&0x1F,&c);
			RefreshPath(app);
			RefreshItem(app);
		}
		break;
	case 1:
		evt=TGGUI_TableSelector_RunOneStep(EGB_work,&app->typeSel,&input);
		if(TGGUI_EVENT_SELCHANGE==evt)
		{
			RefreshPath(app);
			RefreshItem(app);
		}
		break;
	case 2:
		evt=TGGUI_VerticalSelector_RunOneStep(EGB_work,&app->itemSel,&input);
		if(TGGUI_EVENT_SELECTED==evt)
		{
			if(app->itemSel.curSel<app->itemSel.nItems)
			{
				if(TGGUI_ITEMTYPE_TMENUV2==app->itemSel.items[app->itemSel.curSel].type)
				{
					int itemIndex=app->itemSel.items[app->itemSel.curSel].u.tmv2.itemIndex;
					if(itemIndex<0) // Go up a level.
					{
						if(0<app->tmenuItemGroup && app->tmenuItemGroup<app->tmenuItemV2.nIcons)
						{
							const int depth=app->tmenuItemV2.icons[app->tmenuItemGroup].depth-1;
							int i;
							app->tmenuItemGroup=0; /* Tentatively reset. */
							for(i=0; i<app->tmenuItemGroup<app->tmenuItemV2.nIcons; ++i)
							{
								if(depth==app->tmenuItemV2.icons[i].depth)
								{
									app->tmenuItemGroup=i;
									break;
								}
							}
						}
						else
						{
							app->tmenuItemGroup=0;
						}
						RefreshPath(app);
						RefreshItem(app);
					}
					else if(itemIndex<app->tmenuItemV2.nIcons)
					{
						if(ICONTYPE_GROUP==app->tmenuItemV2.icons[itemIndex].iconType)
						{
							if(itemIndex+1<app->tmenuItemV2.nIcons &&
							   app->tmenuItemV2.icons[itemIndex].depth<app->tmenuItemV2.icons[itemIndex+1].depth)
							{
								app->tmenuItemGroup=itemIndex+1;
								RefreshPath(app);
								RefreshItem(app);
							}
						}
						else if(ICONTYPE_PROGRAM==app->tmenuItemV2.icons[itemIndex].iconType)
						{
							// Set executable file name and parameters.
							app->terminate=1;
						}
					}
				}
				if(TGGUI_ITEMTYPE_FILE==app->itemSel.items[app->itemSel.curSel].type)
				{
					if(0!=(DOS_ATTR_DIRECTORY&app->itemSel.items[app->itemSel.curSel].u.f.file.attrib))
					{
						DOSCHDIR(app->itemSel.items[app->itemSel.curSel].u.f.file.name);
						RefreshPath(app);
						RefreshItem(app);
					}
					else
					{
						// if(filename is an executable type)
						{
							// Set executable file name and parameters.
							app->terminate=1;
						}
					}
				}
			}
		}
		break;
	};

	if(0!=input.inkey)
	{
		if(ASCII_TAB==(input.inkey&0xFF))
		{
			int nextActive=app->activeFrame;
			if(0==(input.shift&1))
			{
				nextActive=(app->activeFrame+1)%3;
			}
			else
			{
				nextActive=(app->activeFrame+2)%3;
			}
			ChangeActive(app->activeFrame,nextActive,3,app->frames);
			app->activeFrame=nextActive;
		}

		// char str[8];
		// str[7]=0;
		// Uitoa_fl(str,7,input.inkey&255);
		// puts(str);
	}
}

struct GUIApp app;

int main(void)
{
	KYB_init();

	EGB_init(EGB_work,EgbWorkSize);

	GUIApp_Setup(&app);
	GUIApp_Draw(&app);

	while(0==app.terminate)
	{
		struct TGGUI_InputSet input;
		TGGUI_PollInput(&input);
		GUIApp_RunOneStep(&app,input);
	}

	return 0;
}
