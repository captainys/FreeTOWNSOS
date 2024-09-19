#include <DOS.H>
#include <CONIO.H>
#include "TGBIOS.H"
#include "EGB.H"
#include "MACRO.H"
#include "IODEF.H"
#include "UTIL.H"

static void EGB_SetUpCRTC(_Far struct EGB_Work *work,int modeComb)
{
	int reg;
	_Far unsigned short *regSet=EGB_GetCRTCRegs(modeComb);
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

	_Far struct EGB_Work *EGB_work;
	_FP_SEG(EGB_work)=GS;
	_FP_OFF(EGB_work)=EDI;

	EGB_work->screenMode[0]=12;    //+00H
	EGB_work->screenMode[1]=12;
	EGB_work->writePage=0;         //+02H
	EGB_work->showPageBits=3;      //+03H
	EGB_work->foregroundColor=15;  //+04H
	EGB_work->backgroundColor=0;  //+06H
	EGB_work->transparentColor=0;  //+08H
	EGB_work->alpha=128;             //+0AH
	EGB_work->viewport[0]=0;       //+0CH x0,y0,x1,y1
	EGB_work->viewport[1]=0;       //+0CH x0,y0,x1,y1
	EGB_work->viewport[2]=639;       //+0CH x0,y0,x1,y1
	EGB_work->viewport[3]=479;       //+0CH x0,y0,x1,y1
	EGB_work->fontSpacing=0;       //+14H
	EGB_work->fontRotation=0;       //+16H
	EGB_work->stringRotation=0;     //+17H
	EGB_work->textX=0;       //+18H
	EGB_work->textY=0;
	EGB_work->paintMode=0;          //+1CH
	EGB_work->padding0=0xFF;           //+1DH PADDING
	EGB_work->paintColor=0;        //+1EH
	EGB_work->drawingMode=0;        //+20H
	EGB_work->superImpose=0;        //+21H
	EGB_work->superImposeArea[0]=0;//+22H x0,y0,x1,y1
	EGB_work->superImposeArea[1]=0;//+22H x0,y0,x1,y1
	EGB_work->superImposeArea[2]=0;//+22H x0,y0,x1,y1
	EGB_work->superImposeArea[3]=0;//+22H x0,y0,x1,y1
	EGB_work->superImposeBright=255;  //+2AH
	EGB_work->penWidth=1;           //+2BH
	EGB_work->fontStyle=0;          //+2CH
	EGB_work->padding1=0xFF;           //+2DH PADDING
	EGB_work->hatchWid=0;
	EGB_work->hatchHei=0;  //+2EH
	EGB_work->hatchingPtn=NULL;  //+30H
	EGB_work->tileWid=0;
	EGB_work->tileHei=0;    //+36H
	EGB_work->tilePtn=NULL;      //+3AH

	for(i=0; i<4; ++i)
	{
		EGB_work->virtualVRAM[i].visiSize.x=0;
		EGB_work->virtualVRAM[i].visiSize.y=0;
		EGB_work->virtualVRAM[i].size.x=0;
		EGB_work->virtualVRAM[i].size.y=0;
		EGB_work->virtualVRAM[i].bytesPerLine=0;
		EGB_work->virtualVRAM[i].bytesPerLineShift=0;  // 0:Can not shift  Non-Zero:Can shift (bytesPerLine is 2^n)
		EGB_work->virtualVRAM[i].bitsPerPixel=0;
		EGB_work->virtualVRAM[i].combination[0]=0xFF;
		EGB_work->virtualVRAM[i].combination[1]=0xFF;
		EGB_work->virtualVRAM[i].combination[2]=0xFF;
		EGB_work->virtualVRAM[i].combination[3]=0xFF;

		EGB_work->virtualVRAM[i].flags=0;
		EGB_work->virtualVRAM[i].defZoom.x=0;
		EGB_work->virtualVRAM[i].defZoom.y=0;

		EGB_work->virtualVRAM[i].vram=NULL;
	}

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
			EGB_SetUpCRTC(EGB_work,i);
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
	_Far struct EGB_Work *EGB_work;
	_FP_SEG(EGB_work)=GS;
	_FP_OFF(EGB_work)=EDI;

	unsigned char AL=EAX&0xFF;
	if(EGB_INVALID_SCRNMODE==AL)
	{
		EGB_SetError(EAX,EGB_GENERAL_ERROR);
		return;
	}

	if(0==AL || 1==AL)
	{
		unsigned int newScreenMode[2];
		newScreenMode[0]=EGB_work->screenMode[0];
		newScreenMode[1]=EGB_work->screenMode[1];
		newScreenMode[AL]=EDX&0x3F;

		if(0==(EDX&0x40))
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
				EGB_work->screenMode[0]=newScreenMode[0];
				EGB_work->screenMode[1]=newScreenMode[1];
				EGB_SetUpCRTC(EGB_work,modeComb);
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

void EGB_DISPLAYSTART(
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
	_Far struct EGB_Work *EGB_work;
	_FP_SEG(EGB_work)=GS;
	_FP_OFF(EGB_work)=EDI;

	unsigned char AL=EAX&0xFF;
	if((0==AL || 1==AL) && EGB_work->screenMode[AL]!=EGB_INVALID_SCRNMODE)
	{
		EGB_work->writePage=AL;
		EGB_SetError(EAX,EGB_NO_ERROR);
	}
	else if(0x80<=AL && AL<0x84)
	{
		AL&=3;
		if(NULL!=EGB_work->virtualVRAM[AL].vram)
		{
			EGB_work->writePage=(AL|0x80);
			EGB_SetError(EAX,EGB_NO_ERROR);
		}
	}
	EGB_SetError(EAX,EGB_GENERAL_ERROR);
}

void EGB_DISPLAYPAGE(
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
	TSUGARU_BREAK;
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
	TSUGARU_BREAK;
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
	if(0==work->writePage || 1==work->writePage)
	{
		_Far struct EGB_ScreenMode *prop=EGB_GetScreenModeProp(work->screenMode[work->writePage]);
		if(NULL!=prop)
		{
			_Far unsigned char *vram;
			unsigned int vramOffset,wordData,count;
			if(4==prop->bitsPerPixel)
			{
				unsigned short wd;
				wd=work->backgroundColor;
				wd<<=4;
				wd|=work->backgroundColor;
				wordData=wd|(wd<<8);
			}
			else if(8==prop->bitsPerPixel)
			{
				wordData=work->backgroundColor|(work->backgroundColor<<8);
			}
			else
			{
				wordData=work->backgroundColor;
			}

			if(work->screenMode[1]==EGB_INVALID_SCRNMODE)
			{
				vramOffset=0;
				count=VRAM_SIZE/2;
			}
			else
			{
				vramOffset=(VRAM_SIZE/2)*work->writePage;
				count=VRAM_SIZE/4;
			}
			_FP_SEG(vram)=SEG_VRAM_2PG;
			_FP_OFF(vram)=vramOffset;
			MEMSETW_FAR(vram,wordData,count);
		}
		return;
	}
	else if(0x80<=work->writePage && work->writePage<=0x83)
	{
		TSUGARU_BREAK;
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
