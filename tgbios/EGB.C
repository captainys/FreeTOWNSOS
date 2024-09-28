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

unsigned int EGB_CoordToVRAMOffset(_Far struct EGB_ScreenMode *mode,int x,int y)
{
	unsigned int addr=0;
	if(0!=mode->bytesPerLineShift)
	{
		addr=(y<<mode->bytesPerLineShift);
	}
	else
	{
		addr=(y*mode->bytesPerLine);
	}
	addr+=((x<<mode->bitsPerPixel)>>8);
	return addr;
}

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

	work->perPage[0].ZOOM=regSet[2+CRTC_REG_ZOOM]&0xFF;
	work->perPage[1].ZOOM=regSet[2+CRTC_REG_ZOOM]>>8;

	work->perPage[0].HDS=regSet[2+CRTC_REG_HDS0];
	work->perPage[0].HDE=regSet[2+CRTC_REG_HDE0];
	work->perPage[1].HDS=regSet[2+CRTC_REG_HDS1];
	work->perPage[1].HDE=regSet[2+CRTC_REG_HDE1];

	work->perPage[0].VDS=regSet[2+CRTC_REG_VDS0];
	work->perPage[0].VDE=regSet[2+CRTC_REG_VDE0];
	work->perPage[1].VDS=regSet[2+CRTC_REG_VDS1];
	work->perPage[1].VDE=regSet[2+CRTC_REG_VDE1];

	_POPFD
}

static void EGB_WriteCRTCReg(_Far struct EGB_Work *work,unsigned char reg,unsigned short value)
{
	_outb(TOWNSIO_CRTC_ADDRESS,reg);
	_outw(TOWNSIO_CRTC_DATA_LOW,value);
	work->crtcRegs[reg]=value;
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

////////////////////////////////////////////////////////////

void EGB_ResetPalette(_Far struct EGB_Work *work,int writePage)
{
	if(work->perPage[writePage].screenMode!=EGB_INVALID_SCRNMODE)
	{
		int i;
		_Far struct EGB_ScreenMode *scrnModeProp=EGB_GetScreenModeProp(work->perPage[writePage].screenMode);
		if(8==scrnModeProp->bitsPerPixel)
		{
			_Far unsigned int *pal=EGB_GetDefaultPalette256();

			work->sifter[1]|=0x30;
			_outb(TOWNSIO_VIDEO_OUT_CTRL_ADDRESS,1);
			_outb(TOWNSIO_VIDEO_OUT_CTRL_DATA,work->sifter[1]);

			for(i=0; i<256; ++i)
			{
				unsigned char b= pal[i];
				unsigned char r=(pal[i]>>8);
				unsigned char g=(pal[i]>>16);

				_outb(TOWNSIO_ANALOGPALETTE_CODE,(unsigned char)i);
				_outb(TOWNSIO_ANALOGPALETTE_BLUE,b);
				_outb(TOWNSIO_ANALOGPALETTE_RED,r);
				_outb(TOWNSIO_ANALOGPALETTE_GREEN,g);
			}
		}
		else if(4==scrnModeProp->bitsPerPixel)
		{
			_Far unsigned int *pal=EGB_GetDefaultPalette16();

			if(0==writePage)
			{
				work->sifter[1]&=0x0F;
			}
			else
			{
				work->sifter[1]&=0x0F;
				work->sifter[1]|=0x20;
			}

			_outb(TOWNSIO_VIDEO_OUT_CTRL_ADDRESS,1);
			_outb(TOWNSIO_VIDEO_OUT_CTRL_DATA,work->sifter[1]);

			for(i=0; i<16; ++i)
			{
				unsigned char b= pal[i];
				unsigned char r=(pal[i]>>8);
				unsigned char g=(pal[i]>>16);

				_outb(TOWNSIO_ANALOGPALETTE_CODE,(unsigned char)i);
				_outb(TOWNSIO_ANALOGPALETTE_BLUE,b);
				_outb(TOWNSIO_ANALOGPALETTE_RED,r);
				_outb(TOWNSIO_ANALOGPALETTE_GREEN,g);
			}
		}
	}
}

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
		p->screenMode=3;
		p->color[EGB_FOREGROUND_COLOR]=32767;
		p->color[EGB_BACKGROUND_COLOR]=0;
		p->color[EGB_TRANSPARENT_COLOR]=0;
		p->color[EGB_FILL_COLOR]=0;
		p->alpha=128;
		p->viewport[0].x=0;
		p->viewport[0].y=0;
		p->viewport[1].x=639;
		p->viewport[1].y=479;
		p->fontSpacing=0;
		p->fontRotation=0;
		p->stringRotation=0;
		p->textX=0;
		p->textY=16;
		p->textZoom=EGB_NO_TEXT_ZOOM;
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


	// Initialize palette
	EGB_ResetPalette(work,0);
	EGB_ResetPalette(work,1);

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
					EGB_ResetPalette(work,AL);
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
	unsigned char writePage;
	unsigned char mode=(unsigned char)EAX;
	unsigned short horizontal=(unsigned short)EDX;
	unsigned short vertical=(unsigned short)EBX;
	_Far struct EGB_Work *work;
	_FP_SEG(work)=GS;
	_FP_OFF(work)=EDI;


	EGB_SetError(EAX,EGB_NO_ERROR);


	writePage=work->writePage;
	if(1<writePage || EGB_INVALID_SCRNMODE==work->perPage[writePage].screenMode)
	{
		EGB_SetError(EAX,EGB_GENERAL_ERROR);
		return;
	}


	_PUSHFD;

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
		// Based on Tsugaru implementation, which must be reasonably coorect,
		//   VRAM address offset = FAx*N
		//   Where N is
		//     4 in 4-bits per pixel mode
		//     8 in 8-bits per pixel mode
		//     8 if 16-bits per pixel and single-page mode
		//     4 if 16-bits per pixel and double-page mode

		// unsigned int TownsCRTC::GetPageVRAMAddressOffset(unsigned char page) const
		// {
		//	// [2] pp. 145
		// 	auto FA0=state.crtcReg[REG_FA0+page*4];
		// 	switch(GetPageBitsPerPixel(page))
		// 	{
		// 	case 4:
		// 		return FA0*4;  // 8 pixels for 1 count.
		// 	case 8:
		// 		return FA0*8;  // 8 pixels for 1 count.
		// 	case 16:
		// 		return (LowResCrtcIsInSinglePageMode() ? FA0*8 : FA0*4); // 4 pixels or 2 pixels depending on the single-page or 2-page mode.
		// 	}
		// 	return 0;
		{
			_Far struct EGB_ScreenMode *scrnModeProp=EGB_GetScreenModeProp(work->perPage[writePage].screenMode);
			if(NULL!=scrnModeProp)
			{
				if(0==(scrnModeProp->flags&(SCRNMODE_FLAG_VSCROLL|SCRNMODE_FLAG_HSCROLL)) ||
				   (0==(scrnModeProp->flags&SCRNMODE_FLAG_HSCROLL) && 0!=horizontal))
				{
					EGB_SetError(EAX,EGB_GENERAL_ERROR);
				}
				else
				{
					unsigned int FAx=(vertical*scrnModeProp->size.x)+horizontal;
					switch(scrnModeProp->bitsPerPixel)
					{
					case 4:
						// FA0*=4;
						FAx/=8; // Now FA0 is byte offset into VRAM.
						//  FA0*4=offset  ->  FA0=offset/4
						// FA0/=4;
						break;
					case 8:
						// FA0 is byte offset into VRAM.
						//  FA0*8=offset -> FA0=offset/8
						FAx/=8;
						break;
					case 16:
						FAx*=2; // Now FA0 is byte offset into VRAM.
						if(EGB_INVALID_SCRNMODE==work->perPage[1].screenMode)
						{
							// Single-page mode  offset=FA0*8 -> FA0=offset/8
							FAx/=8;
						}
						else
						{
							// Double-page mode   offset=FA0*4 -> FA0=offset/4
							FAx/=4;
						}
						break;
					}
					if(0==writePage)
					{
						EGB_WriteCRTCReg(work,CRTC_REG_FA0,FAx);
					}
					else
					{
						EGB_WriteCRTCReg(work,CRTC_REG_FA1,FAx);
					}
				}
			}
		}
		break;
	case 2:  // Zoom
		{
			_Far struct EGB_ScreenMode *scrnModeProp=EGB_GetScreenModeProp(work->perPage[writePage].screenMode);
			if(NULL!=scrnModeProp)
			{
				if(horizontal<scrnModeProp->defZoom.x || vertical<scrnModeProp->defZoom.y)
				{
					EGB_SetError(EAX,EGB_GENERAL_ERROR);
				}
				else
				{
					unsigned short ZOOM;
					unsigned char Z=((vertical-1)<<4)|(horizontal-1);
					work->perPage[writePage].ZOOM=Z;
					ZOOM=(work->perPage[1].ZOOM<<8)|work->perPage[0].ZOOM;
					EGB_WriteCRTCReg(work,CRTC_REG_ZOOM,ZOOM);
				}
			}
			else
			{
				EGB_SetError(EAX,EGB_GENERAL_ERROR);
			}
		}
		break;
	case 3:  // Display Size
		// Size is basically HDEx-HDSx,VDEx-VDSx
		// However, in 15KHz mode, 
		//    wid=(HDEx-HDSx)/2  ->  (HDEx-HDSx)=wid*2
		//    hei=(VDEx-VDSx)*2  ->  (VDEx-VDSx)=hei/2
		// Also this wid and hei are in 1x scale (640x480 pixels resolution)
		{
			_Far struct EGB_ScreenMode *scrnModeProp=EGB_GetScreenModeProp(work->perPage[writePage].screenMode);
			if(NULL!=scrnModeProp)
			{
				unsigned int wid1X,hei1X,zoomX,zoomY,HDS,VDS,HDE,VDE,FO,LO;

				if(0==writePage)
				{
					HDS=work->crtcRegs[CRTC_REG_HDS0];
					VDS=work->crtcRegs[CRTC_REG_VDS0];
					HDE=work->crtcRegs[CRTC_REG_HDE0];
					VDE=work->crtcRegs[CRTC_REG_VDE0];
					FO=work->crtcRegs[CRTC_REG_FO0];
					LO=work->crtcRegs[CRTC_REG_LO0];
				}
				else
				{
					HDS=work->crtcRegs[CRTC_REG_HDS1];
					VDS=work->crtcRegs[CRTC_REG_VDS1];
					HDE=work->crtcRegs[CRTC_REG_HDE1];
					VDE=work->crtcRegs[CRTC_REG_VDE1];
					FO=work->crtcRegs[CRTC_REG_FO1];
					LO=work->crtcRegs[CRTC_REG_LO1];
				}

				zoomX=(work->perPage[writePage].ZOOM&0x0F)+1;
				zoomY=(work->perPage[writePage].ZOOM>>4)+1;

				wid1X=horizontal*zoomX/scrnModeProp->defZoom.x;
				hei1X=vertical*zoomY/scrnModeProp->defZoom.y;

				if(15==scrnModeProp->KHz)
				{
					wid1X*=2;
					hei1X/=2;
				}

				if(0==FO || FO==LO)
				{
					hei1X*=2;
				}

				HDE=HDS+wid1X;
				VDE=VDS+hei1X;

				if(0==writePage)
				{
					EGB_WriteCRTCReg(work,CRTC_REG_HDE0,HDE);
					EGB_WriteCRTCReg(work,CRTC_REG_VDE0,VDE);
				}
				else
				{
					EGB_WriteCRTCReg(work,CRTC_REG_HDE1,HDE);
					EGB_WriteCRTCReg(work,CRTC_REG_VDE1,VDE);
				}
			}
			else
			{
				EGB_SetError(EAX,EGB_GENERAL_ERROR);
			}
		}
		break;
	}


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
	// AL=VSYNC Flag 0:No Wait  1:Wait for VSYNC
	// DS:ESI Palette
	//    +0 DW Number of palettes
	//    +4 DW Color Index
	//    +5 B  B
	//    +6 B  R
	//    +7 B  G
	//    +8 B  Zero(Ignored)
	//    Repea +4 to +8 times nuber of palettes

	int i;
	unsigned char writePage;
	unsigned char waitVSYNC=(unsigned char)EAX;
	_Far struct EGB_PaletteSet *paletteSet;
	_Far struct EGB_Work *work;
	_Far struct EGB_ScreenMode *scrnModeProp;

	_PUSHFD;

	_FP_SEG(paletteSet)=DS;
	_FP_OFF(paletteSet)=ESI;

	_FP_SEG(work)=GS;
	_FP_OFF(work)=EDI;

	writePage=work->writePage;
	if(1<writePage ||
	   EGB_INVALID_SCRNMODE==work->perPage[writePage].screenMode)
	{
		EGB_SetError(EAX,EGB_GENERAL_ERROR);
		return;
	}

	scrnModeProp=EGB_GetScreenModeProp(work->perPage[writePage].screenMode);
	if(8==scrnModeProp->bitsPerPixel)
	{
		work->sifter[1]|=0x30;
	}
	else if(4==scrnModeProp->bitsPerPixel)
	{
		if(0==writePage)
		{
			work->sifter[1]&=0x0F;
		}
		else
		{
			work->sifter[1]&=0x0F;
			work->sifter[1]|=0x20;
		}
	}
	else
	{
		// No palette for 16-bit color.
		EGB_SetError(EAX,EGB_GENERAL_ERROR);
		goto POPFD_AND_RETURN;
	}

	_outb(TOWNSIO_VIDEO_OUT_CTRL_ADDRESS,1);
	_outb(TOWNSIO_VIDEO_OUT_CTRL_DATA,work->sifter[1]);

	for(i=0; i<paletteSet->numPalettes; ++i)
	{
		_outb(TOWNSIO_ANALOGPALETTE_CODE,(unsigned char)paletteSet->palettes[i].colorIndex);
		_outb(TOWNSIO_ANALOGPALETTE_BLUE,paletteSet->palettes[i].brg[0]);
		_outb(TOWNSIO_ANALOGPALETTE_RED,paletteSet->palettes[i].brg[1]);
		_outb(TOWNSIO_ANALOGPALETTE_GREEN,paletteSet->palettes[i].brg[2]);
	}

	EGB_SetError(EAX,EGB_NO_ERROR);

POPFD_AND_RETURN:
	_POPFD;
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
	if(NULL!=pointerSet.page)
	{
		pointerSet.page->color[EAX&3]=EDX;
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
	_Far struct EGB_Work *work;
	_FP_SEG(work)=GS;
	_FP_OFF(work)=EDI;

	struct EGB_PagePointerSet pointerSet=EGB_GetPagePointerSet(work);

	if(NULL!=pointerSet.page)
	{
		unsigned int color=0;
		switch(pointerSet.mode->bitsPerPixel)
		{
		case 1:
		case 4:
			color|=((EDX>>28)&8);
			color|=((EDX>>21)&4);
			color|=((EDX>>14)&2);
			color|=((EDX>> 7)&1);
			break;
		case 8:
			color|=((EDX>> 6)&   3);
			color|=((EDX>>11)&0x1C);
			color|=((EDX>>16)&0xE0);
			break;
		case 16:
			color|=((EDX>> 3)&  0x1F);
			color|=((EDX>> 6)& 0x3E0);
			color|=((EDX>> 9)&0x7C00);
			color|=((EDX>>16)&0x8000);
			break;
		}
		pointerSet.page->color[EAX&3]=color;
	}
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
	if(NULL!=pointerSet.page)
	{
		pointerSet.page->drawingMode=EAX&0xFF;
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
	if(NULL!=pointerSet.mode)
	{
		unsigned int wordData;
		if(4==pointerSet.mode->bitsPerPixel)
		{
			unsigned short wd;
			wd=pointerSet.page->color[EGB_BACKGROUND_COLOR];
			wd<<=4;
			wd|=pointerSet.page->color[EGB_BACKGROUND_COLOR];
			wordData=wd|(wd<<8);
		}
		else if(8==pointerSet.mode->bitsPerPixel)
		{
			wordData=pointerSet.page->color[EGB_BACKGROUND_COLOR]|(((unsigned short)pointerSet.page->color[EGB_BACKGROUND_COLOR])<<8);
		}
		else
		{
			wordData=pointerSet.page->color[EGB_BACKGROUND_COLOR];
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

void EGB_22H_GETBLOCK1BIT(
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

void EGB_23H_PUTBLOCK1BIT(
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

void EGB_24H_GETBLOCK(
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

void EGB_25H_PUTBLOCK(
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
	unsigned char flags=EAX;
	_Far struct EGB_BlockInfo *blkInfo;
	_FP_SEG(blkInfo)=DS;
	_FP_OFF(blkInfo)=ESI;

	_Far struct EGB_Work *work;
	_FP_SEG(work)=GS;
	_FP_OFF(work)=EDI;

	struct EGB_PagePointerSet pointerSet=EGB_GetPagePointerSet(work);
	struct POINTW p0,p1;

	if(NULL==pointerSet.page)
	{
		EGB_SetError(EAX,EGB_GENERAL_ERROR);
		return;
	}

	if(flags&2)
	{
		// I don't think mask is of very high priority.
		TSUGARU_BREAK;
	}

	p0=blkInfo->p0;
	p1=blkInfo->p1;
	EGB_MakeP0SmallerThanP1(&p0,&p1);

	if(flags&1) // View-port flag
	{
		// Out of the screen?
		if(p1.x<0 || pointerSet.mode->size.x<=p0.x ||
		   p1.y<0 || pointerSet.mode->size.y<=p0.y)
		{
			EGB_SetError(EAX,EGB_NO_ERROR);
			return;
		}

		// If not using viewport, coordinates must fit inside the screen.
		if(p0.x<0 || pointerSet.mode->size.x<=p1.x ||
		   p0.y<0 || pointerSet.mode->size.y<=p1.y)
		{
			EGB_SetError(EAX,EGB_GENERAL_ERROR);
			return;
		}
	}
	else
	{
		// Out of the viewport?
		if(p1.x<pointerSet.page->viewport[0].x || pointerSet.page->viewport[1].x<p0.x ||
		   p1.y<pointerSet.page->viewport[0].y || pointerSet.page->viewport[1].y<p0.y)
		{
			EGB_SetError(EAX,EGB_NO_ERROR);
			return;
		}

		if(pointerSet.page->viewport[0].x<=p0.x && p1.x<=pointerSet.page->viewport[1].x &&
		   pointerSet.page->viewport[0].y<=p0.y && p1.y<=pointerSet.page->viewport[1].y)
		{
			flags&=0xFE; // Entirely inide the viewport.  Don't have to be worried about the viewport.
		}
	}


	unsigned int dx=(p1.x-p0.x+1);
	unsigned int srcBytesPerLine=0,transferBytesPerLine=0;
	switch(pointerSet.mode->bitsPerPixel)
	{
	case 1:
		srcBytesPerLine=(dx+7)/8;
		transferBytesPerLine=dx/8;
		break;
	case 4:
		srcBytesPerLine=((dx+7)/8)*4;
		transferBytesPerLine=dx/2;
		break;
	case 8:
		srcBytesPerLine=dx;
		transferBytesPerLine=dx;
		break;
	case 16:
		srcBytesPerLine=dx*2;
		transferBytesPerLine=dx*2;
		break;
	}

	if(0==(flags&1))
	{
		unsigned int vramOffset=EGB_CoordToVRAMOffset(pointerSet.mode,p0.x,p0.y);
		if(1==pointerSet.mode->bitsPerPixel)
		{
			// I'll come back when someone do it.
			TSUGARU_BREAK;
		}
		else if(4==pointerSet.mode->bitsPerPixel && ((p0.x&1) || (dx&1)))
		{
			// Oh damn it.  Why do you do it?
			TSUGARU_BREAK;
		}
		else
		{
			_Far unsigned char *src=blkInfo->data;
			_Far unsigned char *vram=pointerSet.vram+vramOffset;
			for(int y=p0.y; y<=p1.y; ++y)
			{
				MEMCPY_FAR(vram,src,transferBytesPerLine);
				src+=srcBytesPerLine;
				vram+=pointerSet.mode->bytesPerLine;
			}
		}
	}
	else
	{
		// I'll come back when someone do it.
		TSUGARU_BREAK;
	}

	EGB_SetError(EAX,EGB_NO_ERROR);
}

void EGB_26H_GETBLOCKZOOM(
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

void EGB_27H_PUTBLOCKZOOM(
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

static unsigned short EGB_SJIS2JIS(unsigned short sjis)
{
	unsigned s1=sjis>>8;
	unsigned s2=sjis&0xff;

	unsigned k,t;

	if(224<=s1)
	{
		k=s1*2-385;
	}
	else
	{
		k=s1*2-257;
	}

	if(s2<=126)
	{
		t=s2-63;
	}
	else if(s2<=158)
	{
		t=s2-64;
	}
	else
	{
		t=s2-158;
		++k;
	}

	return ((k+0x20)<<8)|(t+0x20);
}

static unsigned short EGB_JIS2SJIS(unsigned short jis)
{
	unsigned k=(jis>>8)-0x20;
	unsigned t=(jis&0xFF)-0x20;
	unsigned s1,s2;

	if(k<=62)
	{
		s1=(k+257)/2;
	}
	else
	{
		s1=(k+385)/2;
	}

	if(0!=(k&1) && t<64)
	{
		s2=t+63;
	}
	else if(0!=(k&1) && t<=94)
	{
		s2=t+64;
	}
	else
	{
		s2=t+158;
	}

	return (s1<<8)|s2;
}

void EGB_PUTX16BW_NOCHECK(
	struct EGB_PagePointerSet *ptrSet, // Should be in the SS.
	int sx,int sy,
	_Far unsigned char *ptnBase,int wid)
{
	unsigned int vramAddr;
	int x,y;

	if(0!=ptrSet->mode->bytesPerLineShift)
	{
		vramAddr=((sy-15)<<ptrSet->mode->bytesPerLineShift);
		vramAddr+=((sx*ptrSet->mode->bitsPerPixel)/8);
	}
	else
	{
		vramAddr=((sy-15)*ptrSet->mode->bytesPerLine+sx)>>1;
	}

	switch(ptrSet->mode->bitsPerPixel)
	{
	case 4:
		{
			unsigned char andPtn,color;

			if(0==(sx&1))
			{
				andPtn=0x0F;
				color=ptrSet->page->color[EGB_FOREGROUND_COLOR]<<4;
			}
			else
			{
				andPtn=0xF0;
				color=ptrSet->page->color[EGB_FOREGROUND_COLOR];
			}

			for(y=0; y<16; ++y)
			{
				unsigned char ptn=*ptnBase;
				for(x=0; x<wid; ++x)
				{
					// Can I do SHL and use CF in C rather?
					if(ptn&0x80)
					{
						//switch(ptrSet->page->drawingMode) // May be it is a common property across pages.
						//{
						//case EGB_PSET:
							ptrSet->vram[vramAddr]&=andPtn;
							ptrSet->vram[vramAddr]|=color;
						//	break;
						//}
						//
					}
					ptn<<=1;
					if(0x0F==andPtn)
					{
						andPtn=0xF0;
						color>>=4;
						++vramAddr;
					}
					else
					{
						andPtn=0x0F;
						color<<=4;
					}
					if(7==(x&7))
					{
						++ptnBase;
						ptn=*ptnBase;
					}
				}
				vramAddr+=(ptrSet->mode->bytesPerLine-wid/2);
			}
		}
		break;
	case 8:
		{
			unsigned char color;

			color=ptrSet->page->color[EGB_FOREGROUND_COLOR];

			for(y=0; y<16; ++y)
			{
				unsigned char ptn=*ptnBase;
				for(x=0; x<wid; ++x)
				{
					// Can I do SHL and use CF in C rather?
					if(ptn&0x80)
					{
						//switch(ptrSet->page->drawingMode) // May be it is a common property across pages.
						//{
						//case EGB_PSET:
							ptrSet->vram[vramAddr]=color;
						//	break;
						//}
						//
					}
					ptn<<=1;
					++vramAddr;
					if(7==(x&7))
					{
						++ptnBase;
						ptn=*ptnBase;
					}
				}
				vramAddr+=(ptrSet->mode->bytesPerLine-wid);
			}
		}
		break;
	case 16:
		{
			unsigned short color;

			color=ptrSet->page->color[EGB_FOREGROUND_COLOR];

			for(y=0; y<16; ++y)
			{
				unsigned short ptn=*ptnBase;
				for(x=0; x<wid; ++x)
				{
					// Can I do SHL and use CF in C rather?
					if(ptn&0x80)
					{
						//switch(ptrSet->page->drawingMode) // May be it is a common property across pages.
						//{
						//case EGB_PSET:
							*(_Far unsigned short *)(ptrSet->vram+vramAddr)=color;
						//	break;
						//}
						//
					}
					ptn<<=1;
					vramAddr+=2;
					if(7==(x&7))
					{
						++ptnBase;
						ptn=*ptnBase;
					}
				}
				vramAddr+=(ptrSet->mode->bytesPerLine-wid*2);
			}
		}
		break;
	}
}

// https://ja.wikipedia.org/wiki/Shift_JIS
// JIS  1st byte=K   2nd byte=T
//
//Sjis 1st=[(k+257)/2] if 1<=k<=62        (129 to 159)
//         [(k+385)/2] if 63<=k<=94       (224 to 239)  <- Incorrect.  Must be 224 to 254.
//     2nd=t+63 if k is odd and 1<=t<=63  (64 to 126)
//         t+64 if k is odd and 64<=t<=94 (128 to 158)
//         t+158 if k is even.            (158 to 255)
//
//JIS
//224<=s1 -> (k+385)/2=s1 -> j1=s1*2-385+x  (x=0 or 1)
//else    -> (k+257)/2=s1 -> j1=s1*2-257+x  (x=0 or 1)
//
//64<=s2<=126 -> j2=s2-63, x=1
//128<=x2<=158-> j2=s2-64, x=1
//else        -> j2=s2-158, x=0

// Multiply by 32 to get to the font pattern.
unsigned int EGB_JIS_TO_FONTROMINDEX(unsigned short jis)
{
	unsigned int JISCodeLow=(unsigned char)jis;
	unsigned int JISCodeHigh=(unsigned char)(jis>>8);

	if(JISCodeHigh<0x28)
	{
		// 32x8 Blocks
		unsigned int BLK=(JISCodeLow-0x20)>>5;
		unsigned int x=JISCodeLow&0x1F;
		unsigned int y=JISCodeHigh&7;
		if(BLK==1)
		{
			BLK=2;
		}
		else if(BLK==2)
		{
			BLK=1;
		}
		return BLK*32*8+y*32+x;
	}
	else
	{
		// 32x16 Blocks;
		unsigned int BlkX=(JISCodeLow-0x20)>>5;
		unsigned int BlkY=(JISCodeHigh-0x30)>>4;
		unsigned int BLK=BlkY*3+BlkX;
		unsigned int x=JISCodeLow&0x1F;
		unsigned int y=JISCodeHigh&0x0F;
		return 0x400+BLK*32*16+y*32+x;
	}
}

#define SJISPointerToROMAddress(from,to) \
{ \
	unsigned short sjis=(from)[0],jis; \
	sjis<<=8; \
	sjis|=(from)[1]; \
	jis=EGB_SJIS2JIS(sjis); \
	(to)=EGB_JIS_TO_FONTROMINDEX(jis); \
	(to)<<=5; \
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
	_Far struct EGB_Work *work;
	struct EGB_PagePointerSet ptrSet;
	_Far struct EGB_String *strInfo;
	struct POINTUW dimension,ankDim,kanjiDim;
	struct POINTW minmax[2];

	_FP_SEG(strInfo)=DS;
	_FP_OFF(strInfo)=ESI;

	_FP_SEG(work)=GS;
	_FP_OFF(work)=EDI;

	ptrSet=EGB_GetPagePointerSet(work);

	EGB_SetError(EAX,EGB_NO_ERROR);

	kanjiDim.x=ptrSet.page->textZoom&0xFF;
	kanjiDim.y=(ptrSet.page->textZoom>>8)&0xFF;
	ankDim.x=(ptrSet.page->textZoom>>16)&0xFF;
	ankDim.y=(ptrSet.page->textZoom>>24)&0xFF;

	if(0==ptrSet.page->fontSpacing &&
	   EGB_NO_TEXT_ZOOM==ptrSet.page->textZoom)
	{
		dimension.x=strInfo->len*8;
		dimension.y=16;
	}
	else
	{
		int i;
		dimension.x=0;
		dimension.y=0;
		for(i=0; i<strInfo->len; ++i)
		{
			if(IS_SJIS_FIRST_BYTE(strInfo->str[i]))
			{
				dimension.x+=ptrSet.page->fontSpacing+kanjiDim.x;
				dimension.y=_max(dimension.y,kanjiDim.y);
			}
			else
			{
				dimension.x+=ptrSet.page->fontSpacing+ankDim.x;
				dimension.y=_max(dimension.y,ankDim.y);
			}
		}
	}

	minmax[0].x=strInfo->x;
	minmax[0].y=strInfo->y-dimension.y+1;
	minmax[1].x=strInfo->x+dimension.x-1;
	minmax[1].y=strInfo->y;


	if(EGB_NO_TEXT_ZOOM==ptrSet.page->textZoom &&
	   ptrSet.page->viewport[0].x<=minmax[0].x && minmax[1].x<=ptrSet.page->viewport[1].x &&
	   ptrSet.page->viewport[0].y<=minmax[0].y && minmax[1].y<=ptrSet.page->viewport[1].y)
	{
		int sx=strInfo->x;
		int sy=strInfo->y;
		int i=0;
		unsigned int addr;
		_Far unsigned char *fontROM;

		_FP_SEG(fontROM)=SEG_KANJI_FONT_ROM;
		_FP_OFF(fontROM)=0;

		addr=strInfo->y;
		if(ptrSet.mode->bytesPerLineShift)
		{
			addr<<=ptrSet.mode->bytesPerLineShift;
		}
		else
		{
			addr*=ptrSet.mode->bytesPerLine;
		}
		addr+=(strInfo->x*ptrSet.mode->bitsPerPixel)/8;

		while(i<strInfo->len)
		{
			if(IS_SJIS_FIRST_BYTE(strInfo->str[i]))
			{
				unsigned int ptnAddr;
				SJISPointerToROMAddress(strInfo->str+i,ptnAddr);

				EGB_PUTX16BW_NOCHECK(&ptrSet,sx,sy,fontROM+ptnAddr,16);

				sx+=16;
				i+=2;
			}
			else
			{
				unsigned int ptnAddr=ANK16_FONT_ADDR_BASE+((unsigned short)strInfo->str[i])*16;

				EGB_PUTX16BW_NOCHECK(&ptrSet,sx,sy,fontROM+ptnAddr,8);

				sx+=8;
				++i;
			}
			sx+=ptrSet.page->fontSpacing;
		}

		ptrSet.page->textX=sx;
		ptrSet.page->textY=sy;
	}
	else
	{
		TSUGARU_BREAK;
	}

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
