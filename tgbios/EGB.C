#include <DOS.H>
#include <CONIO.H>
#include "TGBIOS.H"
#include "EGB.H"
#include "MACRO.H"
#include "IODEF.H"
#include "UTIL.H"

void EGB_WaitVSYNC(void)
{
	while(0!=(_inb(TOWNSIO_HSYNC_VSYNC)&1));
	while(0==(_inb(TOWNSIO_HSYNC_VSYNC)&1));
}

static void EGB_SetUpCRTC(_Far struct EGB_Work *work,int modeComb)
{
	int reg;
	_Far unsigned short *regSet=EGB_GetCRTCRegs(modeComb);

	_PUSHFD
	_CLI

	if(NULL==regSet || EGB_INVALID_SCRNMODE==regSet[0])
	{
		// Register Set does not exist, or the set is unused.
		return;
	}
	for(reg=0; reg<NUM_CRTC_REGS; ++reg)
	{
		work->crtcRegs[reg]=regSet[2+reg];
		if(reg==2 || reg==3)
		{
			continue;
		}
		_outb(TOWNSIO_CRTC_ADDRESS,reg);
		_outw(TOWNSIO_CRTC_DATA_LOW,regSet[2+reg]);
	}
	for(reg=0; reg<2; ++reg)
	{
		work->sifter[reg]=regSet[2+32+reg];
		_outb(TOWNSIO_VIDEO_OUT_CTRL_ADDRESS,reg);
		_outb(TOWNSIO_VIDEO_OUT_CTRL_DATA,regSet[2+32+reg]);
	}

	_POPFD
}

struct EGB_PagePointerSet EGB_GetPagePointerSet(_Far struct EGB_Work *work)
{
	struct EGB_PagePointerSet pointerSet;
	if(work->writePage<2)
	{
		pointerSet.settings=&work->perPage[work->writePage];
		if(EGB_INVALID_SCRNMODE!=pointerSet.settings->screenMode)
		{
			pointerSet.modeProp=EGB_GetScreenModeProp(pointerSet.settings->screenMode);
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
			pointerSet.settings=&work->perVirtualPage[vPageIdx];
			pointerSet.modeProp=&work->virtualPage[vPageIdx];
			pointerSet.vram=work->virtualPage[vPageIdx].vram;
			pointerSet.vramSize=(pointerSet.modeProp->bytesPerLine*pointerSet.modeProp->size.y);
			return pointerSet;
		}
	}
	pointerSet.settings=NULL;
	pointerSet.modeProp=NULL;
	pointerSet.vram=NULL;
	return pointerSet;
}

////////////////////////////////////////////////////////////

void EGB_INIT(
	unsigned int EDI,
	unsigned int ESI,
	unsigned int EBP,
	unsigned int ESP,
	unsigned int EBX,
	unsigned int EDX,
	unsigned int ECX,
	unsigned int EAX,
	unsigned int DS,
	unsigned int ES,
	unsigned int GS,
	unsigned int FS)
{
	int i;

	_Far struct EGB_Work *work;
	_FP_SEG(work)=GS;
	_FP_OFF(work)=EDI;

	work->writePage=0;
	for(i=0; i<2; ++i)
	{
		_Far struct EGB_PerPage *p=&(work->perPage[i]);
		p->screenMode=12;
		p->color[EGB_FOREGROUND_COLOR]=15;
		p->color[EGB_BACKGROUND_COLOR]=0;
		p->color[EGB_TRANSPARENT_COLOR]=0;
		p->color[EGB_FILL_COLOR]=0;
		p->alpha=128;
		p->viewport[0]=0;
		p->viewport[1]=0;
		p->viewport[2]=639;
		p->viewport[3]=479;
		p->fontSpacing=0;
		p->fontRotation=0;
		p->stringRotation=0;
		p->textX=0;
		p->textY=0;
		p->paintMode=0;
		p->drawingMode=0;
		p->penWidth=1;
		p->fontStyle=0;
		p->hatchWid=0;
		p->hatchHei=0;
		p->hatchingPtn=NULL;
		p->tileWid=0;
		p->tileHei=0;
		p->tilePtn=NULL;
	}

	work->superImpose=0;
	work->superImposeArea[0]=0;
	work->superImposeArea[1]=0;
	work->superImposeArea[2]=0;
	work->superImposeArea[3]=0;
	work->superImposeBright=255;

	for(i=0; i<4; ++i)
	{
		work->virtualPage[i].visiSize.x=0;
		work->virtualPage[i].visiSize.y=0;
		work->virtualPage[i].size.x=0;
		work->virtualPage[i].size.y=0;
		work->virtualPage[i].bytesPerLine=0;
		work->virtualPage[i].bytesPerLineShift=0;  // 0:Can not shift  Non-Zero:Can shift (bytesPerLine is 2^n)
		work->virtualPage[i].bitsPerPixel=0;
		work->virtualPage[i].combination[0]=EGB_INVALID_SCRNMODE;
		work->virtualPage[i].combination[1]=EGB_INVALID_SCRNMODE;
		work->virtualPage[i].combination[2]=EGB_INVALID_SCRNMODE;
		work->virtualPage[i].combination[3]=EGB_INVALID_SCRNMODE;

		work->virtualPage[i].flags=0;
		work->virtualPage[i].defZoom.x=0;
		work->virtualPage[i].defZoom.y=0;

		work->virtualPage[i].vram=NULL;
	}

	MEMSETB_FAR(work->perVirtualPage,0,sizeof(struct EGB_PerPage)*4);

	// Clear VRAM
	{
		_Far unsigned int *vram;
		_FP_SEG(vram)=SEG_VRAM_2PG;
		_FP_OFF(vram)=0;
		for(int i=0; i<VRAM_SIZE/sizeof(unsigned int); ++i)
		{
			vram[i]=0;
		}
	}

	// Set up CRTC
	for(i=0; i<EGB_NUM_MODECOMB; ++i)
	{
		_Far unsigned short *regSet=EGB_GetCRTCRegs(i);
		if(NULL!=regSet && 3==regSet[0] && 3==regSet[1])
		{
			EGB_SetUpCRTC(work,i);
			break;
		}
	}
	_outb(TOWNSIO_CRTC_OUTPUT_CONTROL,0x0A);  // FM-R CRTC Output Control

	// Need to initialize palette

	EGB_SetError(EAX,EGB_NO_ERROR);
}

void EGB_RESOLUTION(
	unsigned int EDI,
	unsigned int ESI,
	unsigned int EBP,
	unsigned int ESP,
	unsigned int EBX,
	unsigned int EDX,
	unsigned int ECX,
	unsigned int EAX,
	unsigned int DS,
	unsigned int ES,
	unsigned int GS,
	unsigned int FS)
{
	_Far struct EGB_Work *work;
	_FP_SEG(work)=GS;
	_FP_OFF(work)=EDI;

	unsigned char AL=EAX&0xFF;
	if(EGB_INVALID_SCRNMODE==AL)
	{
		EGB_SetError(EAX,EGB_GENERAL_ERROR);
		return;
	}

	if(0==AL || 1==AL)
	{
		unsigned int newScreenMode[2];
		newScreenMode[0]=work->perPage[0].screenMode;
		newScreenMode[1]=work->perPage[1].screenMode;
		newScreenMode[AL]=EDX&0x3F;

		{
			int modeComb;
			_Far unsigned short *regSet;
			for(modeComb=0; modeComb<EGB_NUM_MODECOMB; ++modeComb)
			{
				regSet=EGB_GetCRTCRegs(modeComb);
				if(newScreenMode[0]==regSet[0] & newScreenMode[1]==regSet[1])
				{
					break;
				}
			}
			if(modeComb>=EGB_NUM_MODECOMB)
			{
				for(modeComb=0; modeComb<EGB_NUM_MODECOMB; ++modeComb)
				{
					regSet=EGB_GetCRTCRegs(modeComb);
					if(newScreenMode[AL]==regSet[AL])
					{
						newScreenMode[0]=regSet[0];
						newScreenMode[1]=regSet[1];
						break;
					}
				}
			}
			if(modeComb<EGB_NUM_MODECOMB)
			{
				work->perPage[0].screenMode=newScreenMode[0];
				work->perPage[1].screenMode=newScreenMode[1];
				if(0==(EDX&0x40))
				{
					EGB_SetUpCRTC(work,modeComb);
				}
			}
			else
			{
				TSUGARU_BREAK;
				EGB_SetError(EAX,EGB_GENERAL_ERROR);
			}
		}
	}
	else if(0x80<=AL && AL<=0x83)
	{
		TSUGARU_BREAK;
	}
	else
	{
		EGB_SetError(EAX,EGB_GENERAL_ERROR);
	}
}

void EGB_02H_DISPLAYSTART(
	unsigned int EDI,
	unsigned int ESI,
	unsigned int EBP,
	unsigned int ESP,
	unsigned int EBX,
	unsigned int EDX,
	unsigned int ECX,
	unsigned int EAX,
	unsigned int DS,
	unsigned int ES,
	unsigned int GS,
	unsigned int FS)
{
	_PUSHFD;
	unsigned char mode=(unsigned char)EAX;
	unsigned short horizontal=(unsigned short)EDX;
	unsigned short vertical=(unsigned short)EBX;

	if(0==(mode&0x40))
	{
		_CLI;
		EGB_WaitVSYNC();
		mode&=0x3F;
	}

	switch(mode)
	{
	case 0:  // Top-Left corner
		break;
	case 1:  // Scroll
		break;
	case 2:  // Zoom
		break;
	case 3:  // Display Size
		break;
	}

	TSUGARU_BREAK;
	EGB_SetError(EAX,EGB_NO_ERROR);

	_POPFD;
}

void EGB_VIEWPORT(
	unsigned int EDI,
	unsigned int ESI,
	unsigned int EBP,
	unsigned int ESP,
	unsigned int EBX,
	unsigned int EDX,
	unsigned int ECX,
	unsigned int EAX,
	unsigned int DS,
	unsigned int ES,
	unsigned int GS,
	unsigned int FS)
{
	TSUGARU_BREAK;
	EGB_SetError(EAX,EGB_NO_ERROR);
}

void EGB_PALETTE(
	unsigned int EDI,
	unsigned int ESI,
	unsigned int EBP,
	unsigned int ESP,
	unsigned int EBX,
	unsigned int EDX,
	unsigned int ECX,
	unsigned int EAX,
	unsigned int DS,
	unsigned int ES,
	unsigned int GS,
	unsigned int FS)
{
	TSUGARU_BREAK;
	EGB_SetError(EAX,EGB_NO_ERROR);
}

void EGB_WRITEPAGE(
	unsigned int EDI,
	unsigned int ESI,
	unsigned int EBP,
	unsigned int ESP,
	unsigned int EBX,
	unsigned int EDX,
	unsigned int ECX,
	unsigned int EAX,
	unsigned int DS,
	unsigned int ES,
	unsigned int GS,
	unsigned int FS)
{
	_Far struct EGB_Work *work;
	_FP_SEG(work)=GS;
	_FP_OFF(work)=EDI;

	unsigned char AL=EAX&0xFF;
	if((0==AL || 1==AL) && work->perPage[AL].screenMode!=EGB_INVALID_SCRNMODE)
	{
		work->writePage=AL;
		EGB_SetError(EAX,EGB_NO_ERROR);
	}
	else if(0x80<=AL && AL<0x84)
	{
		AL&=3;
		if(NULL!=work->virtualPage[AL].vram)
		{
			work->writePage=(AL|0x80);
			EGB_SetError(EAX,EGB_NO_ERROR);
		}
	}
	EGB_SetError(EAX,EGB_GENERAL_ERROR);
}

void EGB_06H_DISPLAYPAGE(
	unsigned int EDI,
	unsigned int ESI,
	unsigned int EBP,
	unsigned int ESP,
	unsigned int EBX,
	unsigned int EDX,
	unsigned int ECX,
	unsigned int EAX,
	unsigned int DS,
	unsigned int ES,
	unsigned int GS,
	unsigned int FS)
{
	unsigned char priority=EAX&1;
	unsigned char showPage=0;

	_Far struct EGB_Work *work;
	_FP_SEG(work)=GS;
	_FP_OFF(work)=EDI;

	work->sifter[1]&=0xFE;
	work->sifter[1]|=priority;
	_outb(TOWNSIO_VIDEO_OUT_CTRL_ADDRESS,1);
	_outb(TOWNSIO_VIDEO_OUT_CTRL_DATA,work->sifter[1]);

	if(EDX&1)
	{
		showPage=3;
	}
	if(EDX&2)
	{
		showPage|=0x0C;
	}

	_outb(TOWNSIO_CRTC_OUTPUT_CONTROL,showPage);

	EGB_SetError(EAX,EGB_NO_ERROR);
}

void EGB_COLOR(
	unsigned int EDI,
	unsigned int ESI,
	unsigned int EBP,
	unsigned int ESP,
	unsigned int EBX,
	unsigned int EDX,
	unsigned int ECX,
	unsigned int EAX,
	unsigned int DS,
	unsigned int ES,
	unsigned int GS,
	unsigned int FS)
{
	_Far struct EGB_Work *work;
	_FP_SEG(work)=GS;
	_FP_OFF(work)=EDI;
	struct EGB_PagePointerSet pointerSet=EGB_GetPagePointerSet(work);
	if(NULL!=pointerSet.settings)
	{
		pointerSet.settings->color[EAX&3]=EDX;
	}
	EGB_SetError(EAX,EGB_NO_ERROR);
}

void EGB_COLORIGRB(
	unsigned int EDI,
	unsigned int ESI,
	unsigned int EBP,
	unsigned int ESP,
	unsigned int EBX,
	unsigned int EDX,
	unsigned int ECX,
	unsigned int EAX,
	unsigned int DS,
	unsigned int ES,
	unsigned int GS,
	unsigned int FS)
{
	TSUGARU_BREAK;
	EGB_SetError(EAX,EGB_NO_ERROR);
}

void EGB_PASTEL(
	unsigned int EDI,
	unsigned int ESI,
	unsigned int EBP,
	unsigned int ESP,
	unsigned int EBX,
	unsigned int EDX,
	unsigned int ECX,
	unsigned int EAX,
	unsigned int DS,
	unsigned int ES,
	unsigned int GS,
	unsigned int FS)
{
	TSUGARU_BREAK;
	EGB_SetError(EAX,EGB_NO_ERROR);
}

void EGB_WRITEMODE(
	unsigned int EDI,
	unsigned int ESI,
	unsigned int EBP,
	unsigned int ESP,
	unsigned int EBX,
	unsigned int EDX,
	unsigned int ECX,
	unsigned int EAX,
	unsigned int DS,
	unsigned int ES,
	unsigned int GS,
	unsigned int FS)
{
	// One big question is, is write-mode per page?  Or common?
	_Far struct EGB_Work *work;
	_FP_SEG(work)=GS;
	_FP_OFF(work)=EDI;
	struct EGB_PagePointerSet pointerSet=EGB_GetPagePointerSet(work);
	if(NULL!=pointerSet.settings)
	{
		pointerSet.settings->drawingMode=EAX&0xFF;
	}
	EGB_SetError(EAX,EGB_NO_ERROR);
}

void EGB_LINEPATTERN(
	unsigned int EDI,
	unsigned int ESI,
	unsigned int EBP,
	unsigned int ESP,
	unsigned int EBX,
	unsigned int EDX,
	unsigned int ECX,
	unsigned int EAX,
	unsigned int DS,
	unsigned int ES,
	unsigned int GS,
	unsigned int FS)
{
	TSUGARU_BREAK;
	EGB_SetError(EAX,EGB_NO_ERROR);
}

void EGB_PAINTMODE(
	unsigned int EDI,
	unsigned int ESI,
	unsigned int EBP,
	unsigned int ESP,
	unsigned int EBX,
	unsigned int EDX,
	unsigned int ECX,
	unsigned int EAX,
	unsigned int DS,
	unsigned int ES,
	unsigned int GS,
	unsigned int FS)
{
	TSUGARU_BREAK;
	EGB_SetError(EAX,EGB_NO_ERROR);
}

void EGB_HATCHINGPATTERN(
	unsigned int EDI,
	unsigned int ESI,
	unsigned int EBP,
	unsigned int ESP,
	unsigned int EBX,
	unsigned int EDX,
	unsigned int ECX,
	unsigned int EAX,
	unsigned int DS,
	unsigned int ES,
	unsigned int GS,
	unsigned int FS)
{
	TSUGARU_BREAK;
	EGB_SetError(EAX,EGB_NO_ERROR);
}

void EGB_TILEPATTERN(
	unsigned int EDI,
	unsigned int ESI,
	unsigned int EBP,
	unsigned int ESP,
	unsigned int EBX,
	unsigned int EDX,
	unsigned int ECX,
	unsigned int EAX,
	unsigned int DS,
	unsigned int ES,
	unsigned int GS,
	unsigned int FS)
{
	TSUGARU_BREAK;
	EGB_SetError(EAX,EGB_NO_ERROR);
}

void EGB_MASKREGION(
	unsigned int EDI,
	unsigned int ESI,
	unsigned int EBP,
	unsigned int ESP,
	unsigned int EBX,
	unsigned int EDX,
	unsigned int ECX,
	unsigned int EAX,
	unsigned int DS,
	unsigned int ES,
	unsigned int GS,
	unsigned int FS)
{
	TSUGARU_BREAK;
	EGB_SetError(EAX,EGB_NO_ERROR);
}

void EGB_MASK(
	unsigned int EDI,
	unsigned int ESI,
	unsigned int EBP,
	unsigned int ESP,
	unsigned int EBX,
	unsigned int EDX,
	unsigned int ECX,
	unsigned int EAX,
	unsigned int DS,
	unsigned int ES,
	unsigned int GS,
	unsigned int FS)
{
	TSUGARU_BREAK;
	EGB_SetError(EAX,EGB_NO_ERROR);
}

void EGB_PEN(
	unsigned int EDI,
	unsigned int ESI,
	unsigned int EBP,
	unsigned int ESP,
	unsigned int EBX,
	unsigned int EDX,
	unsigned int ECX,
	unsigned int EAX,
	unsigned int DS,
	unsigned int ES,
	unsigned int GS,
	unsigned int FS)
{
	TSUGARU_BREAK;
	EGB_SetError(EAX,EGB_NO_ERROR);
}

void EGB_PENSIZE(
	unsigned int EDI,
	unsigned int ESI,
	unsigned int EBP,
	unsigned int ESP,
	unsigned int EBX,
	unsigned int EDX,
	unsigned int ECX,
	unsigned int EAX,
	unsigned int DS,
	unsigned int ES,
	unsigned int GS,
	unsigned int FS)
{
	TSUGARU_BREAK;
	EGB_SetError(EAX,EGB_NO_ERROR);
}

void EGB_PENSTYLE(
	unsigned int EDI,
	unsigned int ESI,
	unsigned int EBP,
	unsigned int ESP,
	unsigned int EBX,
	unsigned int EDX,
	unsigned int ECX,
	unsigned int EAX,
	unsigned int DS,
	unsigned int ES,
	unsigned int GS,
	unsigned int FS)
{
	TSUGARU_BREAK;
	EGB_SetError(EAX,EGB_NO_ERROR);
}

void EGB_MASKBIT(
	unsigned int EDI,
	unsigned int ESI,
	unsigned int EBP,
	unsigned int ESP,
	unsigned int EBX,
	unsigned int EDX,
	unsigned int ECX,
	unsigned int EAX,
	unsigned int DS,
	unsigned int ES,
	unsigned int GS,
	unsigned int FS)
{
	TSUGARU_BREAK;
	EGB_SetError(EAX,EGB_NO_ERROR);
}

void EGB_TEXTDIRECTION(
	unsigned int EDI,
	unsigned int ESI,
	unsigned int EBP,
	unsigned int ESP,
	unsigned int EBX,
	unsigned int EDX,
	unsigned int ECX,
	unsigned int EAX,
	unsigned int DS,
	unsigned int ES,
	unsigned int GS,
	unsigned int FS)
{
	TSUGARU_BREAK;
	EGB_SetError(EAX,EGB_NO_ERROR);
}

void EGB_TEXTDISPLAYDIRECTION(
	unsigned int EDI,
	unsigned int ESI,
	unsigned int EBP,
	unsigned int ESP,
	unsigned int EBX,
	unsigned int EDX,
	unsigned int ECX,
	unsigned int EAX,
	unsigned int DS,
	unsigned int ES,
	unsigned int GS,
	unsigned int FS)
{
	TSUGARU_BREAK;
	EGB_SetError(EAX,EGB_NO_ERROR);
}

void EGB_TEXTSPACE(
	unsigned int EDI,
	unsigned int ESI,
	unsigned int EBP,
	unsigned int ESP,
	unsigned int EBX,
	unsigned int EDX,
	unsigned int ECX,
	unsigned int EAX,
	unsigned int DS,
	unsigned int ES,
	unsigned int GS,
	unsigned int FS)
{
	TSUGARU_BREAK;
	EGB_SetError(EAX,EGB_NO_ERROR);
}

void EGB_TEXTZOOM(
	unsigned int EDI,
	unsigned int ESI,
	unsigned int EBP,
	unsigned int ESP,
	unsigned int EBX,
	unsigned int EDX,
	unsigned int ECX,
	unsigned int EAX,
	unsigned int DS,
	unsigned int ES,
	unsigned int GS,
	unsigned int FS)
{
	TSUGARU_BREAK;
	EGB_SetError(EAX,EGB_NO_ERROR);
}

void EGB_FONTSTYLE(
	unsigned int EDI,
	unsigned int ESI,
	unsigned int EBP,
	unsigned int ESP,
	unsigned int EBX,
	unsigned int EDX,
	unsigned int ECX,
	unsigned int EAX,
	unsigned int DS,
	unsigned int ES,
	unsigned int GS,
	unsigned int FS)
{
	TSUGARU_BREAK;
	EGB_SetError(EAX,EGB_NO_ERROR);
}

void EGB_SUPERIMPOSE(
	unsigned int EDI,
	unsigned int ESI,
	unsigned int EBP,
	unsigned int ESP,
	unsigned int EBX,
	unsigned int EDX,
	unsigned int ECX,
	unsigned int EAX,
	unsigned int DS,
	unsigned int ES,
	unsigned int GS,
	unsigned int FS)
{
	TSUGARU_BREAK;
	EGB_SetError(EAX,EGB_NO_ERROR);
}

void EGB_DIGITIZE(
	unsigned int EDI,
	unsigned int ESI,
	unsigned int EBP,
	unsigned int ESP,
	unsigned int EBX,
	unsigned int EDX,
	unsigned int ECX,
	unsigned int EAX,
	unsigned int DS,
	unsigned int ES,
	unsigned int GS,
	unsigned int FS)
{
	TSUGARU_BREAK;
	EGB_SetError(EAX,EGB_NO_ERROR);
}

void EGB_RESOLUTION_BY_HANDLE(
	unsigned int EDI,
	unsigned int ESI,
	unsigned int EBP,
	unsigned int ESP,
	unsigned int EBX,
	unsigned int EDX,
	unsigned int ECX,
	unsigned int EAX,
	unsigned int DS,
	unsigned int ES,
	unsigned int GS,
	unsigned int FS)
{
	TSUGARU_BREAK;
	EGB_SetError(EAX,EGB_NO_ERROR);
}

void EGB_CLEARSCREEN(
	unsigned int EDI,
	unsigned int ESI,
	unsigned int EBP,
	unsigned int ESP,
	unsigned int EBX,
	unsigned int EDX,
	unsigned int ECX,
	unsigned int EAX,
	unsigned int DS,
	unsigned int ES,
	unsigned int GS,
	unsigned int FS)
{
	_Far struct EGB_Work *work;
	_FP_SEG(work)=GS;
	_FP_OFF(work)=EDI;

	EGB_SetError(EAX,EGB_NO_ERROR);

	struct EGB_PagePointerSet pointerSet=EGB_GetPagePointerSet(work);
	if(NULL!=pointerSet.modeProp)
	{
		_Far unsigned char *vram;
		unsigned int vramOffset,wordData,count;
		if(4==pointerSet.modeProp->bitsPerPixel)
		{
			unsigned short wd;
			wd=pointerSet.settings->color[EGB_BACKGROUND_COLOR];
			wd<<=4;
			wd|=pointerSet.settings->color[EGB_BACKGROUND_COLOR];
			wordData=wd|(wd<<8);
		}
		else if(8==pointerSet.modeProp->bitsPerPixel)
		{
			wordData=pointerSet.settings->color[EGB_BACKGROUND_COLOR]|(pointerSet.settings->color[EGB_BACKGROUND_COLOR]<<8);
		}
		else
		{
			wordData=pointerSet.settings->color[EGB_BACKGROUND_COLOR];
		}

		MEMSETW_FAR(pointerSet.vram,wordData,pointerSet.vramSize/2);
		return;
	}
	EGB_SetError(EAX,EGB_GENERAL_ERROR);
}

void EGB_PARTCLEARSCREEN(
	unsigned int EDI,
	unsigned int ESI,
	unsigned int EBP,
	unsigned int ESP,
	unsigned int EBX,
	unsigned int EDX,
	unsigned int ECX,
	unsigned int EAX,
	unsigned int DS,
	unsigned int ES,
	unsigned int GS,
	unsigned int FS)
{
	TSUGARU_BREAK;
	EGB_SetError(EAX,EGB_NO_ERROR);
}

void EGB_GETBLOCKCOLOR(
	unsigned int EDI,
	unsigned int ESI,
	unsigned int EBP,
	unsigned int ESP,
	unsigned int EBX,
	unsigned int EDX,
	unsigned int ECX,
	unsigned int EAX,
	unsigned int DS,
	unsigned int ES,
	unsigned int GS,
	unsigned int FS)
{
	TSUGARU_BREAK;
	EGB_SetError(EAX,EGB_NO_ERROR);
}

void EGB_PUTBLOCKCOLOR(
	unsigned int EDI,
	unsigned int ESI,
	unsigned int EBP,
	unsigned int ESP,
	unsigned int EBX,
	unsigned int EDX,
	unsigned int ECX,
	unsigned int EAX,
	unsigned int DS,
	unsigned int ES,
	unsigned int GS,
	unsigned int FS)
{
	TSUGARU_BREAK;
	EGB_SetError(EAX,EGB_NO_ERROR);
}

void EGB_GETBLOCK(
	unsigned int EDI,
	unsigned int ESI,
	unsigned int EBP,
	unsigned int ESP,
	unsigned int EBX,
	unsigned int EDX,
	unsigned int ECX,
	unsigned int EAX,
	unsigned int DS,
	unsigned int ES,
	unsigned int GS,
	unsigned int FS)
{
	TSUGARU_BREAK;
	EGB_SetError(EAX,EGB_NO_ERROR);
}

void EGB_PUTBLOCK(
	unsigned int EDI,
	unsigned int ESI,
	unsigned int EBP,
	unsigned int ESP,
	unsigned int EBX,
	unsigned int EDX,
	unsigned int ECX,
	unsigned int EAX,
	unsigned int DS,
	unsigned int ES,
	unsigned int GS,
	unsigned int FS)
{
	TSUGARU_BREAK;
	EGB_SetError(EAX,EGB_NO_ERROR);
}

void EGB_GETBLOCKZOOM(
	unsigned int EDI,
	unsigned int ESI,
	unsigned int EBP,
	unsigned int ESP,
	unsigned int EBX,
	unsigned int EDX,
	unsigned int ECX,
	unsigned int EAX,
	unsigned int DS,
	unsigned int ES,
	unsigned int GS,
	unsigned int FS)
{
	TSUGARU_BREAK;
	EGB_SetError(EAX,EGB_NO_ERROR);
}

void EGB_PUTBLOCKZOOM(
	unsigned int EDI,
	unsigned int ESI,
	unsigned int EBP,
	unsigned int ESP,
	unsigned int EBX,
	unsigned int EDX,
	unsigned int ECX,
	unsigned int EAX,
	unsigned int DS,
	unsigned int ES,
	unsigned int GS,
	unsigned int FS)
{
	TSUGARU_BREAK;
	EGB_SetError(EAX,EGB_NO_ERROR);
}

void EGB_GRAPHICCURSOR(
	unsigned int EDI,
	unsigned int ESI,
	unsigned int EBP,
	unsigned int ESP,
	unsigned int EBX,
	unsigned int EDX,
	unsigned int ECX,
	unsigned int EAX,
	unsigned int DS,
	unsigned int ES,
	unsigned int GS,
	unsigned int FS)
{
	TSUGARU_BREAK;
	EGB_SetError(EAX,EGB_NO_ERROR);
}

void EGB_MASKDATA(
	unsigned int EDI,
	unsigned int ESI,
	unsigned int EBP,
	unsigned int ESP,
	unsigned int EBX,
	unsigned int EDX,
	unsigned int ECX,
	unsigned int EAX,
	unsigned int DS,
	unsigned int ES,
	unsigned int GS,
	unsigned int FS)
{
	TSUGARU_BREAK;
	EGB_SetError(EAX,EGB_NO_ERROR);
}

void EGB_SCROLL(
	unsigned int EDI,
	unsigned int ESI,
	unsigned int EBP,
	unsigned int ESP,
	unsigned int EBX,
	unsigned int EDX,
	unsigned int ECX,
	unsigned int EAX,
	unsigned int DS,
	unsigned int ES,
	unsigned int GS,
	unsigned int FS)
{
	TSUGARU_BREAK;
	EGB_SetError(EAX,EGB_NO_ERROR);
}

void EGB_PARTSCROLL(
	unsigned int EDI,
	unsigned int ESI,
	unsigned int EBP,
	unsigned int ESP,
	unsigned int EBX,
	unsigned int EDX,
	unsigned int ECX,
	unsigned int EAX,
	unsigned int DS,
	unsigned int ES,
	unsigned int GS,
	unsigned int FS)
{
	TSUGARU_BREAK;
	EGB_SetError(EAX,EGB_NO_ERROR);
}

void EGB_REGION(
	unsigned int EDI,
	unsigned int ESI,
	unsigned int EBP,
	unsigned int ESP,
	unsigned int EBX,
	unsigned int EDX,
	unsigned int ECX,
	unsigned int EAX,
	unsigned int DS,
	unsigned int ES,
	unsigned int GS,
	unsigned int FS)
{
	TSUGARU_BREAK;
	EGB_SetError(EAX,EGB_NO_ERROR);
}

void EGB_COPY(
	unsigned int EDI,
	unsigned int ESI,
	unsigned int EBP,
	unsigned int ESP,
	unsigned int EBX,
	unsigned int EDX,
	unsigned int ECX,
	unsigned int EAX,
	unsigned int DS,
	unsigned int ES,
	unsigned int GS,
	unsigned int FS)
{
	TSUGARU_BREAK;
	EGB_SetError(EAX,EGB_NO_ERROR);
}

void EGB_ROTATE(
	unsigned int EDI,
	unsigned int ESI,
	unsigned int EBP,
	unsigned int ESP,
	unsigned int EBX,
	unsigned int EDX,
	unsigned int ECX,
	unsigned int EAX,
	unsigned int DS,
	unsigned int ES,
	unsigned int GS,
	unsigned int FS)
{
	TSUGARU_BREAK;
	EGB_SetError(EAX,EGB_NO_ERROR);
}

void EGB_RESOLVE(
	unsigned int EDI,
	unsigned int ESI,
	unsigned int EBP,
	unsigned int ESP,
	unsigned int EBX,
	unsigned int EDX,
	unsigned int ECX,
	unsigned int EAX,
	unsigned int DS,
	unsigned int ES,
	unsigned int GS,
	unsigned int FS)
{
	TSUGARU_BREAK;
	EGB_SetError(EAX,EGB_NO_ERROR);
}

void EGB_PSET(
	unsigned int EDI,
	unsigned int ESI,
	unsigned int EBP,
	unsigned int ESP,
	unsigned int EBX,
	unsigned int EDX,
	unsigned int ECX,
	unsigned int EAX,
	unsigned int DS,
	unsigned int ES,
	unsigned int GS,
	unsigned int FS)
{
	TSUGARU_BREAK;
	EGB_SetError(EAX,EGB_NO_ERROR);
}

void EGB_CONNECT(
	unsigned int EDI,
	unsigned int ESI,
	unsigned int EBP,
	unsigned int ESP,
	unsigned int EBX,
	unsigned int EDX,
	unsigned int ECX,
	unsigned int EAX,
	unsigned int DS,
	unsigned int ES,
	unsigned int GS,
	unsigned int FS)
{
	TSUGARU_BREAK;
	EGB_SetError(EAX,EGB_NO_ERROR);
}

void EGB_UNCONNECT(
	unsigned int EDI,
	unsigned int ESI,
	unsigned int EBP,
	unsigned int ESP,
	unsigned int EBX,
	unsigned int EDX,
	unsigned int ECX,
	unsigned int EAX,
	unsigned int DS,
	unsigned int ES,
	unsigned int GS,
	unsigned int FS)
{
	TSUGARU_BREAK;
	EGB_SetError(EAX,EGB_NO_ERROR);
}

void EGB_POLYGON(
	unsigned int EDI,
	unsigned int ESI,
	unsigned int EBP,
	unsigned int ESP,
	unsigned int EBX,
	unsigned int EDX,
	unsigned int ECX,
	unsigned int EAX,
	unsigned int DS,
	unsigned int ES,
	unsigned int GS,
	unsigned int FS)
{
	TSUGARU_BREAK;
	EGB_SetError(EAX,EGB_NO_ERROR);
}

void EGB_ROTATEPOLYGON(
	unsigned int EDI,
	unsigned int ESI,
	unsigned int EBP,
	unsigned int ESP,
	unsigned int EBX,
	unsigned int EDX,
	unsigned int ECX,
	unsigned int EAX,
	unsigned int DS,
	unsigned int ES,
	unsigned int GS,
	unsigned int FS)
{
	TSUGARU_BREAK;
	EGB_SetError(EAX,EGB_NO_ERROR);
}

void EGB_TRIANGLE(
	unsigned int EDI,
	unsigned int ESI,
	unsigned int EBP,
	unsigned int ESP,
	unsigned int EBX,
	unsigned int EDX,
	unsigned int ECX,
	unsigned int EAX,
	unsigned int DS,
	unsigned int ES,
	unsigned int GS,
	unsigned int FS)
{
	TSUGARU_BREAK;
	EGB_SetError(EAX,EGB_NO_ERROR);
}

void EGB_RECTANGLE(
	unsigned int EDI,
	unsigned int ESI,
	unsigned int EBP,
	unsigned int ESP,
	unsigned int EBX,
	unsigned int EDX,
	unsigned int ECX,
	unsigned int EAX,
	unsigned int DS,
	unsigned int ES,
	unsigned int GS,
	unsigned int FS)
{
	TSUGARU_BREAK;
	EGB_SetError(EAX,EGB_NO_ERROR);
}

void EGB_CIRCLE(
	unsigned int EDI,
	unsigned int ESI,
	unsigned int EBP,
	unsigned int ESP,
	unsigned int EBX,
	unsigned int EDX,
	unsigned int ECX,
	unsigned int EAX,
	unsigned int DS,
	unsigned int ES,
	unsigned int GS,
	unsigned int FS)
{
	TSUGARU_BREAK;
	EGB_SetError(EAX,EGB_NO_ERROR);
}

void EGB_ARC(
	unsigned int EDI,
	unsigned int ESI,
	unsigned int EBP,
	unsigned int ESP,
	unsigned int EBX,
	unsigned int EDX,
	unsigned int ECX,
	unsigned int EAX,
	unsigned int DS,
	unsigned int ES,
	unsigned int GS,
	unsigned int FS)
{
	TSUGARU_BREAK;
	EGB_SetError(EAX,EGB_NO_ERROR);
}

void EGB_FAN(
	unsigned int EDI,
	unsigned int ESI,
	unsigned int EBP,
	unsigned int ESP,
	unsigned int EBX,
	unsigned int EDX,
	unsigned int ECX,
	unsigned int EAX,
	unsigned int DS,
	unsigned int ES,
	unsigned int GS,
	unsigned int FS)
{
	TSUGARU_BREAK;
	EGB_SetError(EAX,EGB_NO_ERROR);
}

void EGB_ELLIPSE(
	unsigned int EDI,
	unsigned int ESI,
	unsigned int EBP,
	unsigned int ESP,
	unsigned int EBX,
	unsigned int EDX,
	unsigned int ECX,
	unsigned int EAX,
	unsigned int DS,
	unsigned int ES,
	unsigned int GS,
	unsigned int FS)
{
	TSUGARU_BREAK;
	EGB_SetError(EAX,EGB_NO_ERROR);
}

void EGB_ELLIPTICARC(
	unsigned int EDI,
	unsigned int ESI,
	unsigned int EBP,
	unsigned int ESP,
	unsigned int EBX,
	unsigned int EDX,
	unsigned int ECX,
	unsigned int EAX,
	unsigned int DS,
	unsigned int ES,
	unsigned int GS,
	unsigned int FS)
{
	TSUGARU_BREAK;
	EGB_SetError(EAX,EGB_NO_ERROR);
}

void EGB_ELLIPTICFAN(
	unsigned int EDI,
	unsigned int ESI,
	unsigned int EBP,
	unsigned int ESP,
	unsigned int EBX,
	unsigned int EDX,
	unsigned int ECX,
	unsigned int EAX,
	unsigned int DS,
	unsigned int ES,
	unsigned int GS,
	unsigned int FS)
{
	TSUGARU_BREAK;
	EGB_SetError(EAX,EGB_NO_ERROR);
}

void EGB_PAINT(
	unsigned int EDI,
	unsigned int ESI,
	unsigned int EBP,
	unsigned int ESP,
	unsigned int EBX,
	unsigned int EDX,
	unsigned int ECX,
	unsigned int EAX,
	unsigned int DS,
	unsigned int ES,
	unsigned int GS,
	unsigned int FS)
{
	TSUGARU_BREAK;
	EGB_SetError(EAX,EGB_NO_ERROR);
}

void EGB_CLOSEPAINT(
	unsigned int EDI,
	unsigned int ESI,
	unsigned int EBP,
	unsigned int ESP,
	unsigned int EBX,
	unsigned int EDX,
	unsigned int ECX,
	unsigned int EAX,
	unsigned int DS,
	unsigned int ES,
	unsigned int GS,
	unsigned int FS)
{
	TSUGARU_BREAK;
	EGB_SetError(EAX,EGB_NO_ERROR);
}

void EGB_POINT(
	unsigned int EDI,
	unsigned int ESI,
	unsigned int EBP,
	unsigned int ESP,
	unsigned int EBX,
	unsigned int EDX,
	unsigned int ECX,
	unsigned int EAX,
	unsigned int DS,
	unsigned int ES,
	unsigned int GS,
	unsigned int FS)
{
	TSUGARU_BREAK;
	EGB_SetError(EAX,EGB_NO_ERROR);
}

void EGB_BOW(
	unsigned int EDI,
	unsigned int ESI,
	unsigned int EBP,
	unsigned int ESP,
	unsigned int EBX,
	unsigned int EDX,
	unsigned int ECX,
	unsigned int EAX,
	unsigned int DS,
	unsigned int ES,
	unsigned int GS,
	unsigned int FS)
{
	TSUGARU_BREAK;
	EGB_SetError(EAX,EGB_NO_ERROR);
}

void EGB_BOW2(
	unsigned int EDI,
	unsigned int ESI,
	unsigned int EBP,
	unsigned int ESP,
	unsigned int EBX,
	unsigned int EDX,
	unsigned int ECX,
	unsigned int EAX,
	unsigned int DS,
	unsigned int ES,
	unsigned int GS,
	unsigned int FS)
{
	TSUGARU_BREAK;
	EGB_SetError(EAX,EGB_NO_ERROR);
}

void EGB_SJISSTRING(
	unsigned int EDI,
	unsigned int ESI,
	unsigned int EBP,
	unsigned int ESP,
	unsigned int EBX,
	unsigned int EDX,
	unsigned int ECX,
	unsigned int EAX,
	unsigned int DS,
	unsigned int ES,
	unsigned int GS,
	unsigned int FS)
{
	TSUGARU_BREAK;
	EGB_SetError(EAX,EGB_NO_ERROR);
}

void EGB_CONNECTSJISSTRING(
	unsigned int EDI,
	unsigned int ESI,
	unsigned int EBP,
	unsigned int ESP,
	unsigned int EBX,
	unsigned int EDX,
	unsigned int ECX,
	unsigned int EAX,
	unsigned int DS,
	unsigned int ES,
	unsigned int GS,
	unsigned int FS)
{
	TSUGARU_BREAK;
	EGB_SetError(EAX,EGB_NO_ERROR);
}

void EGB_ASCIISTRING(
	unsigned int EDI,
	unsigned int ESI,
	unsigned int EBP,
	unsigned int ESP,
	unsigned int EBX,
	unsigned int EDX,
	unsigned int ECX,
	unsigned int EAX,
	unsigned int DS,
	unsigned int ES,
	unsigned int GS,
	unsigned int FS)
{
	TSUGARU_BREAK;
	EGB_SetError(EAX,EGB_NO_ERROR);
}

void EGB_CONNECTASCIISTRING(
	unsigned int EDI,
	unsigned int ESI,
	unsigned int EBP,
	unsigned int ESP,
	unsigned int EBX,
	unsigned int EDX,
	unsigned int ECX,
	unsigned int EAX,
	unsigned int DS,
	unsigned int ES,
	unsigned int GS,
	unsigned int FS)
{
	TSUGARU_BREAK;
	EGB_SetError(EAX,EGB_NO_ERROR);
}

void EGB_JISSTRING(
	unsigned int EDI,
	unsigned int ESI,
	unsigned int EBP,
	unsigned int ESP,
	unsigned int EBX,
	unsigned int EDX,
	unsigned int ECX,
	unsigned int EAX,
	unsigned int DS,
	unsigned int ES,
	unsigned int GS,
	unsigned int FS)
{
	TSUGARU_BREAK;
	EGB_SetError(EAX,EGB_NO_ERROR);
}

void EGB_CONNECTJISSTRING(
	unsigned int EDI,
	unsigned int ESI,
	unsigned int EBP,
	unsigned int ESP,
	unsigned int EBX,
	unsigned int EDX,
	unsigned int ECX,
	unsigned int EAX,
	unsigned int DS,
	unsigned int ES,
	unsigned int GS,
	unsigned int FS)
{
	TSUGARU_BREAK;
	EGB_SetError(EAX,EGB_NO_ERROR);
}

void EGB_ANYCHAR(
	unsigned int EDI,
	unsigned int ESI,
	unsigned int EBP,
	unsigned int ESP,
	unsigned int EBX,
	unsigned int EDX,
	unsigned int ECX,
	unsigned int EAX,
	unsigned int DS,
	unsigned int ES,
	unsigned int GS,
	unsigned int FS)
{
	TSUGARU_BREAK;
	EGB_SetError(EAX,EGB_NO_ERROR);
}

void EGB_UNSUPPORTED(
	unsigned int EDI,
	unsigned int ESI,
	unsigned int EBP,
	unsigned int ESP,
	unsigned int EBX,
	unsigned int EDX,
	unsigned int ECX,
	unsigned int EAX,
	unsigned int DS,
	unsigned int ES,
	unsigned int GS,
	unsigned int FS)

{
	TSUGARU_BREAK;
}
