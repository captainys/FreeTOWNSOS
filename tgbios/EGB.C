#include <DOS.H>
#include <CONIO.H>
#include "TGBIOS.H"
#include "EGB.H"
#include "MACRO.H"
#include "IODEF.H"
#include "UTIL.H"

unsigned int GetExpandedColor(unsigned short color,unsigned int bitsPerPixel)
{
	unsigned int expColor;
	switch(bitsPerPixel)
	{
	case 16:
		expColor=color;
		expColor|=(expColor<<16);
		break;
	case 4:
		expColor=color&0x0F;
		expColor|=(expColor<<4);
		expColor|=(expColor<<8);
		expColor|=(expColor<<16);
		break;
	case 8:
		expColor=color;
		expColor|=(expColor<<8);
		expColor|=(expColor<<16);
		break;
	case 1:
		if(0==(expColor&1))
		{
			expColor=0;
		}
		else
		{
			expColor=~0;
		}
		break;
	default:
		return 0;
	}
	return expColor;
}

// Y-y0=(X-x0)*(y1-y0)/(x1-x0)
#define ClipX(x0,y0,x1,y1,X) (IMULDIV((X)-(x0),(y1)-(y0),(x1)-(x0))+(y0))
// (X-x0)=(Y-y0)*(x1-x0)/(y1-y0)
#define ClipY(x0,y0,x1,y1,Y) (IMULDIV((Y)-(y0),(x1)-(x0),(y1)-(y0))+(x0))

// 1: Line is visible.
// 0: Line is outside.
int ClipLine(struct POINTW *p0,struct POINTW *p1,struct POINTW min,struct POINTW max)
{
	if((p0->x<min.x && p1->x<min.x) || (max.x<p0->x && max.x<p1->x) ||
	   (p0->y<min.y && p1->y<min.y) || (max.y<p0->y && max.y<p1->y))
	{
		return 0;
	}

	int X0=p0->x,Y0=p0->y,X1=p1->x,Y1=p1->y;

	if(p0->x<min.x)
	{
		p0->x=min.x;
		p0->y=ClipX(X0,Y0,X1,Y1,min.x);
	}
	if(p1->x<min.x)
	{
		p1->x=min.x;
		p1->y=ClipX(X0,Y0,X1,Y1,min.x);
	}
	if(max.x<p0->x)
	{
		p0->x=max.x;
		p0->y=ClipX(X0,Y0,X1,Y1,max.x);
	}
	if(max.x<p1->x)
	{
		p1->x=max.x;
		p1->y=ClipX(X0,Y0,X1,Y1,max.x);
	}

	if(p0->y<min.y)
	{
		p0->x=ClipY(X0,Y0,X1,Y1,min.y);
		p0->y=min.y;
	}
	if(p1->y<min.y)
	{
		p1->x=ClipY(X0,Y0,X1,Y1,min.y);
		p1->y=min.y;
	}
	if(max.y<p0->y)
	{
		p0->x=ClipY(X0,Y0,X1,Y1,max.y);
		p0->y=max.y;
	}
	if(max.y<p1->y)
	{
		p1->x=ClipY(X0,Y0,X1,Y1,max.y);
		p1->y=max.y;
	}

	return (min.x<=p0->x && p0->x<=max.x &&
	        min.x<=p1->x && p1->x<=max.x &&
	        min.y<=p0->y && p0->y<=max.y &&
	        min.y<=p1->y && p1->y<=max.y);
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
	switch(mode->bitsPerPixel)
	{
	case 1:
		addr+=(x>>3);
		break;
	case 4:
		addr+=(x>>1);
		break;
	case 8:
		addr+=x;
		break;
	case 16:
		addr+=(x<<1);
		break;
	}
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

	work->CRTCRegSet=modeComb;

	_POPFD
}

static void EGB_WriteCRTCReg(_Far struct EGB_Work *work,unsigned char reg,unsigned short value)
{
	_outb(TOWNSIO_CRTC_ADDRESS,reg);
	_outw(TOWNSIO_CRTC_DATA_LOW,value);
	work->crtcRegs[reg]=value;
}

// x0 must be <=x1.
// No bound check or viewport check.
// vramLine points to the first byte of the line of Y.
void EGB_DrawHorizontalLinePSET_NO_CHECK(_Far struct EGB_Work *work,struct EGB_PagePointerSet *ptrSet,short x0,short x1,_Far unsigned char *vramLine,unsigned int color)
{
	color=GetExpandedColor(color,ptrSet->mode->bitsPerPixel);
	switch(ptrSet->mode->bitsPerPixel)
	{
	case 4:
		{
			if(x0&1)
			{
				(*(vramLine+x0/2))&=0x0F;
				(*(vramLine+x0/2))|=(color&0xF0);
				++x0;
			}
			if(x0<=x1)
			{
				unsigned int count=(x1+1-x0)/2;
				MEMSETB_FAR(vramLine+x0/2,color,count);
			}
			if(0==(x1&1))
			{
				(*(vramLine+x1/2))&=0xF0;
				(*(vramLine+x1/2))|=(color&0x0F);
			}
		}
		break;
	case 8:
		{
			unsigned int count=x1-x0+1;
			MEMSETB_FAR(vramLine+x0,color,count);
		}
		break;
	case 16:
		{
			unsigned int count=x1-x0+1;
			MEMSETW_FAR(vramLine+x0*2,color,count);
		}
		break;
	}
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

	_Far struct EGB_Work *work=EGB_GetWork();

	work->writePage=0;
	for(i=0; i<2; ++i)
	{
		_Far struct EGB_PerPage *p=&(work->perPage[i]);
		p->screenMode=3;
		p->alpha=128;
		p->viewport[0].x=0;
		p->viewport[0].y=0;
		p->viewport[1].x=639;
		p->viewport[1].y=479;
		p->textX=0;
		p->textY=16;
		p->penWidth=1;
		p->hatchWid=0;
		p->hatchHei=0;
		p->hatchingPtn=NULL;
		p->tileWid=0;
		p->tileHei=0;
		p->tilePtn=NULL;
	}

	work->color[EGB_FOREGROUND_COLOR]=32767;
	work->color[EGB_BACKGROUND_COLOR]=0;
	work->color[EGB_TRANSPARENT_COLOR]=0;
	work->color[EGB_FILL_COLOR]=0;
	work->paintMode=EGB_PAINTFLAG_LINE_NORMAL;
	work->drawingMode=0;

	work->fontStyle=0;
	work->fontSpacing=0;
	work->fontRotation=0;
	work->stringRotation=0;
	work->kanjiZoom.x=16;
	work->kanjiZoom.y=16;
	work->ankZoom.x=8;
	work->ankZoom.y=16;

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
	_Far struct EGB_Work *work=EGB_GetWork();

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

				for(int page=0; page<2; ++page)
				{
					_Far struct EGB_ScreenMode *mode=EGB_GetScreenModeProp(work->perPage[page].screenMode);
					if(NULL!=mode)
					{
						work->perPage[page].viewport[0].x=0;
						work->perPage[page].viewport[0].y=0;
						work->perPage[page].viewport[1]=mode->size;
						work->perPage[page].viewport[1].x--;
						work->perPage[page].viewport[1].y--;

						unsigned fgColor=(1<<mode->bitsPerPixel)-1,trspColor;
						trspColor=fgColor&0x8000;
						fgColor&=0x7FFF;
						work->color[EGB_FOREGROUND_COLOR]=fgColor;
						work->color[EGB_BACKGROUND_COLOR]=0;
						work->color[EGB_FILL_COLOR]=0;
						work->color[EGB_TRANSPARENT_COLOR]=trspColor;
					}
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
	_Far struct EGB_Work *work=EGB_GetWork();


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
		// _CLI;  <- Must not CLI.  It will halt all interrupts for 16ms and break BGM play back.
		EGB_WaitVSYNC();
		mode&=0x3F;
	}

	switch(mode)
	{
	case 0:  // Top-Left corner
		{
			unsigned int CLKSEL=work->crtcRegs[CRTC_REG_CR1]&3;
			// _Far unsigned short *regSet=EGB_GetCRTCRegs(work->CRTCRegSet);
			_Far struct EGB_ScreenMode *mode=EGB_GetScreenModeProp(work->perPage[writePage].screenMode);
			unsigned int HDS,VDS,x0,y0,zoomX,zoomY;
			unsigned HDSReg,HDEReg,HAJReg,VDSReg,VDEReg;
			if(0==writePage)
			{
				HDSReg=CRTC_REG_HDS0;
				HDEReg=CRTC_REG_HDE0;
				VDSReg=CRTC_REG_VDS0;
				VDEReg=CRTC_REG_VDE0;
				HAJReg=CRTC_REG_HAJ0;
			}
			else
			{
				HDSReg=CRTC_REG_HDS1;
				HDEReg=CRTC_REG_HDE1;
				VDSReg=CRTC_REG_VDS1;
				VDEReg=CRTC_REG_VDE1;
				HAJReg=CRTC_REG_HAJ1;
			}

			zoomX=(work->perPage[writePage].ZOOM&0x0F)+1;
			zoomY=(work->perPage[writePage].ZOOM>>4)+1;
			x0=horizontal*zoomX;
			y0=vertical*zoomY;

			if(15==mode->KHz)
			{
				x0>>=1; // For Alltynex.  I have zero confidence.
				y0>>=1;
			}
			switch(CLKSEL)
			{
			default:
			case 0:
				HDS=(x0<<1)+0x129;
				VDS=(y0<<1)+0x2a;
				break;
			case 1:
				HDS=(x0<<1)+0xe7;
				VDS=(y0<<1)+0x2a;
				break;
			case 2:
				HDS=x0     +0x8a;
				VDS=(y0<<1)+0x46;
				break;
			case 3:
				HDS=(x0   )+0x9c;
				VDS=(y0<<1)+0x40;
				break;
			}

			// Also need to preserve HDS-HAJ
			unsigned int prevHAJ=work->crtcRegs[HAJReg];
			unsigned int prevHDS=work->crtcRegs[HDSReg];
			unsigned int HAJ=HDS+prevHAJ-prevHDS;

			unsigned int W=work->crtcRegs[HDEReg]-work->crtcRegs[HDSReg];
			unsigned int H=work->crtcRegs[VDEReg]-work->crtcRegs[VDSReg];
			if(EGB_INVALID_SCRNMODE==work->perPage[1].screenMode)
			{
				// Single-page mode
				EGB_WriteCRTCReg(work,CRTC_REG_HDS0,HDS);
				EGB_WriteCRTCReg(work,CRTC_REG_HDE0,HDS+W);
				EGB_WriteCRTCReg(work,CRTC_REG_VDS0,VDS);
				EGB_WriteCRTCReg(work,CRTC_REG_VDE0,VDS+H);
				EGB_WriteCRTCReg(work,CRTC_REG_HAJ0,HAJ);
				EGB_WriteCRTCReg(work,CRTC_REG_HDS1,HDS);
				EGB_WriteCRTCReg(work,CRTC_REG_HDE1,HDS+W);
				EGB_WriteCRTCReg(work,CRTC_REG_VDS1,VDS);
				EGB_WriteCRTCReg(work,CRTC_REG_VDE1,VDS+H);
				EGB_WriteCRTCReg(work,CRTC_REG_HAJ1,HAJ);
			}
			else
			{
				// Double-page mode
				EGB_WriteCRTCReg(work,HDSReg,HDS);
				EGB_WriteCRTCReg(work,HDEReg,HDS+W);
				EGB_WriteCRTCReg(work,VDSReg,VDS);
				EGB_WriteCRTCReg(work,VDEReg,VDS+H);
				EGB_WriteCRTCReg(work,HAJReg,HAJ);
			}
		}
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
					if(EGB_INVALID_SCRNMODE==work->perPage[1].screenMode)
					{
						// Single-page mode
						EGB_WriteCRTCReg(work,CRTC_REG_FA0,FAx);
						EGB_WriteCRTCReg(work,CRTC_REG_FA1,FAx);
					}
					else
					{
						// Double-page mode
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
					if(EGB_INVALID_SCRNMODE==work->perPage[1].screenMode)
					{
						// Single-page mode
						ZOOM=(Z<<8)|Z;
					}
					else
					{
						// Double-page mode
						ZOOM=(work->perPage[1].ZOOM<<8)|work->perPage[0].ZOOM;
					}
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

		// FM TOWNS Technical Databook p.306 tells, the display size cannot be smaller than
		// the default display size.
		// Alltynex tries to make it 480x480 for screen mode 3.  But, TBIOS returns AH=FFh and fails.
		// Also says the display size can be changed only if the display size is smaller than the monitor size.
		// Probably screen modes 3 and 4 cannot change the display size.
		{
			_Far struct EGB_ScreenMode *scrnModeProp=EGB_GetScreenModeProp(work->perPage[writePage].screenMode);
			if(NULL!=scrnModeProp)
			{
				unsigned int wid1X,hei1X,zoomX,zoomY,HDS,VDS,HDE,VDE,FO,LO;

				if(0==(scrnModeProp->flags&SCRNMODE_FLAG_DISPSIZE))
				{
					EGB_SetError(EAX,EGB_GENERAL_ERROR);
					break;
				}

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

				wid1X=_min(horizontal*zoomX,scrnModeProp->crtcSize.x);
				hei1X=_min(vertical*zoomY,scrnModeProp->crtcSize.y);

				// Is it as easy as just multiplying zoom?
				// This method works for Alltynex 15KHz mode, but ends up making SkyDuel 800x480 where should be 640x480.
				// >>
				// wid1X=horizontal*zoomX;
				// hei1X=vertical*zoomY;
				// <<

				// This method works for VSGP, Panic Ball II, and SkyDuel, but not for 15KHz mode.
				// >>
				// wid1X=horizontal*zoomX/scrnModeProp->defZoom.x;
				// hei1X=vertical*zoomY/scrnModeProp->defZoom.y;
				// wid1X=_min(wid1X,_max(640,scrnModeProp->visiSize.x));
				// hei1X=_min(hei1X,480);
				// if(15==scrnModeProp->KHz)
				// {
				// 	wid1X*=2;
				// 	hei1X/=2;
				// }
				// <<

				if(0==FO || FO==LO)
				{
					hei1X*=2;
				}

				HDE=HDS+wid1X;
				VDE=VDS+hei1X;

				if(EGB_INVALID_SCRNMODE==work->perPage[1].screenMode)
				{
					// Single-page mode
					EGB_WriteCRTCReg(work,CRTC_REG_HDE0,HDE);
					EGB_WriteCRTCReg(work,CRTC_REG_VDE0,VDE);
					EGB_WriteCRTCReg(work,CRTC_REG_HDE1,HDE);
					EGB_WriteCRTCReg(work,CRTC_REG_VDE1,VDE);
				}
				else
				{
					// Double-page mode
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
	_Far struct EGB_Work *work=EGB_GetWork();

	_Far const short *rect;
	_FP_SEG(rect)=DS;
	_FP_OFF(rect)=ESI;

	struct POINTW p0,p1;
	struct EGB_PagePointerSet ptrSet=EGB_GetPagePointerSet(work);

	if(ptrSet.page)
	{
		p0.x=rect[0];
		p0.y=rect[1];
		p1.x=rect[2];
		p1.y=rect[3];

		EGB_MakeP0SmallerThanP1(&p0,&p1);

		p0.x=_max(0,p0.x);
		p0.y=_max(0,p0.y);
		p1.x=_min(ptrSet.mode->size.x-1,p1.x);
		p1.y=_min(ptrSet.mode->size.y-1,p1.y);

		ptrSet.page->viewport[0]=p0;
		ptrSet.page->viewport[1]=p1;
	}
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
	_Far struct EGB_Work *work=EGB_GetWork();
	_Far struct EGB_ScreenMode *scrnModeProp;

	_PUSHFD;

	_FP_SEG(paletteSet)=DS;
	_FP_OFF(paletteSet)=ESI;

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
	_Far struct EGB_Work *work=EGB_GetWork();

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

	_Far struct EGB_Work *work=EGB_GetWork();

	work->sifter[1]&=0xFE;
	work->sifter[1]|=priority;
	_outb(TOWNSIO_VIDEO_OUT_CTRL_ADDRESS,1);
	_outb(TOWNSIO_VIDEO_OUT_CTRL_DATA,work->sifter[1]);

	if(EDX&2)
	{
		showPage=3;
	}
	if(EDX&1)
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
	_Far struct EGB_Work *work=EGB_GetWork();
	work->color[EAX&3]=EDX;
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
	_Far struct EGB_Work *work=EGB_GetWork();

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
		work->color[EAX&3]=color;
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
	_Far struct EGB_Work *work=EGB_GetWork();
	work->drawingMode=EAX&0xFF;
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
	_Far struct EGB_Work *work=EGB_GetWork();

	work->paintMode=EDX;

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
	unsigned char AL=EAX;
	if(EGB_FOREGROUND_COLOR==AL ||
	   EGB_BACKGROUND_COLOR==AL ||
	   EGB_FILL_COLOR==AL)
	{
		unsigned char BH=EBX>>8;
		unsigned char BL=EBX;
		if(0<BH && BH<=4 && 0<BL && BL<=32)
		{
			_Far struct EGB_Work *work=EGB_GetWork();
			unsigned int len=BH*BL;  // BH*8*BL/8
			_Far unsigned char *src;
			_FP_SEG(src)=DS;
			_FP_OFF(src)=ESI;

			work->hatch[AL].size.x=BH;
			work->hatch[AL].size.y=BL;
			MEMCPY_FAR(&work->hatch[AL].ptn,src,len);

			EGB_SetError(EAX,EGB_NO_ERROR);
		}
	}
	else
	{
		EGB_SetError(EAX,EGB_GENERAL_ERROR);
	}
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

void EGB_0FH_MASKREGION(
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

void EGB_10H_MASK(
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
	_Far struct EGB_Work *work=EGB_GetWork();

	if(EAX&2)
	{
		TSUGARU_BREAK;
	}
	if(0==(EAX&0x80))
	{
		work->maskMode=EGB_MASKMODE_DISABLE;
	}
	else
	{
		if(EAX&1)
		{
			work->maskMode=EGB_MASKMODE_OPEN;
		}
		else
		{
			work->maskMode=EGB_MASKMODE_CLOSED;
		}
	}

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
	_Far struct EGB_Work *work=EGB_GetWork();

	EGB_SetError(EAX,EGB_NO_ERROR);

	struct EGB_PagePointerSet pointerSet=EGB_GetPagePointerSet(work);
	if(NULL!=pointerSet.mode)
	{
		unsigned int wordData;
		if(4==pointerSet.mode->bitsPerPixel)
		{
			unsigned short wd;
			wd=work->color[EGB_BACKGROUND_COLOR];
			wd<<=4;
			wd|=work->color[EGB_BACKGROUND_COLOR];
			wordData=wd|(wd<<8);
		}
		else if(8==pointerSet.mode->bitsPerPixel)
		{
			wordData=work->color[EGB_BACKGROUND_COLOR]|(((unsigned short)work->color[EGB_BACKGROUND_COLOR])<<8);
		}
		else
		{
			wordData=work->color[EGB_BACKGROUND_COLOR];
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
	_Far struct EGB_Work *work=EGB_GetWork();

	EGB_SetError(EAX,EGB_NO_ERROR);

	struct EGB_PagePointerSet ptrSet=EGB_GetPagePointerSet(work);
	if(NULL!=ptrSet.mode)
	{
		_Far unsigned char *vram=ptrSet.vram+EGB_CoordToVRAMOffset(ptrSet.mode,0,ptrSet.page->viewport[0].y);
		int y;
		for(y=ptrSet.page->viewport[0].y; y<=ptrSet.page->viewport[1].y; ++y)
		{
			EGB_DrawHorizontalLinePSET_NO_CHECK(work,&ptrSet,ptrSet.page->viewport[0].x,ptrSet.page->viewport[1].x,vram,work->color[EGB_BACKGROUND_COLOR]);
			vram+=ptrSet.mode->bytesPerLine;
		}
	}
	EGB_SetError(EAX,EGB_NO_ERROR);
}

unsigned char EGB_GETBLOCK1BIT_INTERNAL(
	_Far struct EGB_ScreenMode *scrnMode,
	_Far struct EGB_BlockInfoAndColor *blkInfo,
	_Far unsigned char *vramIn)
{
	struct POINTW p0,p1,viewport[2];
	unsigned int dstWid,dstHei,dstBytesPerLine;

	p0=blkInfo->p0;
	p1=blkInfo->p1;
	EGB_MakeP0SmallerThanP1(&p0,&p1);

	dstWid=p1.x-p0.x+1;
	dstHei=p1.y-p0.y+1;
	dstBytesPerLine=(dstWid+7)/8;

	viewport[0].x=0;
	viewport[0].y=0;
	viewport[1].x=scrnMode->size.x-1;
	viewport[1].y=scrnMode->size.y-1;

	MEMSETB_FAR(blkInfo->data,0,dstBytesPerLine*dstHei);

	// Out of the viewport?
	if(p1.x<viewport[0].x || viewport[1].x<p0.x ||
	   p1.y<viewport[0].y || viewport[1].y<p0.y)
	{
		return EGB_NO_ERROR;
	}

	{
		unsigned int xSkip=0,ySkip=0,ySkipBytes=0;
		_Far unsigned char *dst=blkInfo->data;

		if(p0.x<viewport[0].x)
		{
			xSkip=viewport[0].x-p0.x;
			p0.x=viewport[0].x;
		}
		if(p0.y<viewport[0].y)
		{
			ySkip=viewport[0].y-p0.y;
			ySkipBytes=ySkip*dstBytesPerLine;
			p0.y=viewport[0].y;
		}

		MEMSETB_FAR(dst,0,ySkipBytes);
		dst+=ySkipBytes;

		p1.x=_min(viewport[1].x,p1.x);
		p1.y=_min(viewport[1].y,p1.y);
		if(viewport[1].x<p1.x)
		{
			p1.x=viewport[1].x;
		}

		unsigned int vramOffset=EGB_CoordToVRAMOffset(scrnMode,p0.x,p0.y);
		if(1==scrnMode->bitsPerPixel)
		{
			// I'll come back when someone do it.
			TSUGARU_BREAK;
		}
		else
		{
			int x,y;
			_Far unsigned char *vram=vramIn+vramOffset,*nextVram;

			switch(scrnMode->bitsPerPixel)
			{
			case 1:
				TSUGARU_BREAK;
				break;
			case 4:
				{
					for(y=p0.y; y<=p1.y; ++y)
					{
						unsigned char bits,srcShift=0;
						_Far unsigned char *nextDst=dst+dstBytesPerLine;

						nextVram=vram+scrnMode->bytesPerLine;

						dst+=(xSkip/8);
						bits=0x80;
						bits>>=(xSkip&7);

						if(p0.x&1)
						{
							srcShift=4;
						}
						for(x=p0.x; x<=p1.x; ++x)
						{
							unsigned char color=(*vram);
							unsigned int i;
							color>>=srcShift;
							color&=0x0F;
							for(i=0; i<blkInfo->numColors; ++i)
							{
								if(color==blkInfo->colors[i])
								{
									*dst|=bits;
									break;
								}
							}
							srcShift=4-srcShift;
							bits>>=1;
							if(0==bits)
							{
								bits=0x80;
								++dst;
							}
						}

						vram=nextVram;
						dst=nextDst;
					}
				}
				break;
			case 8:
				{
					for(y=p0.y; y<=p1.y; ++y)
					{
						unsigned char bits;
						_Far unsigned char *nextDst=dst+dstBytesPerLine;

						nextVram=vram+scrnMode->bytesPerLine;

						dst+=(xSkip/8);
						bits=0x80;
						bits>>=(xSkip&7);

						for(x=p0.x; x<=p1.x; ++x)
						{
							unsigned int i;
							unsigned char color=*vram;
							for(i=0; i<blkInfo->numColors; ++i)
							{
								if(color==blkInfo->colors[i])
								{
									*dst|=bits;
									break;
								}
							}
							bits>>=1;
							if(0==bits)
							{
								bits=0x80;
								++dst;
							}
							++vram;
							++dst;
						}
						vram=nextVram;
						dst=nextDst;
					}
				}
				break;
			case 16:
				{
					for(y=p0.y; y<=p1.y; ++y)
					{
						unsigned char bits;
						_Far unsigned char *nextDst=dst+dstBytesPerLine;

						nextVram=vram+scrnMode->bytesPerLine;

						dst+=(xSkip/8);
						bits=0x80;
						bits>>=(xSkip&7);

						for(x=p0.x; x<=p1.x; ++x)
						{
							int i;
							unsigned short color=*((_Far unsigned short *)vram);
							color&=0x7FFF;
							for(i=0; i<blkInfo->numColors; ++i)
							{
								if(color==*((_Far unsigned short *)(blkInfo->colors+i*2)))
								{
									*dst|=bits;
									break;
								}
							}
							bits>>=1;
							if(0==bits)
							{
								bits=0x80;
								++dst;
							}
							vram+=2;
							dst+=2;
						}

						vram=nextVram;
						dst=nextDst;
					}
				}
				break;
			}
		}
	}

	return EGB_NO_ERROR;
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
	_Far struct EGB_BlockInfoAndColor *blkInfo;
	_FP_SEG(blkInfo)=DS;
	_FP_OFF(blkInfo)=ESI;

	_Far struct EGB_Work *work=EGB_GetWork();

	struct EGB_PagePointerSet pointerSet=EGB_GetPagePointerSet(work);

	if(NULL==pointerSet.page)
	{
		EGB_SetError(EAX,EGB_GENERAL_ERROR);
		return;
	}

	EGB_SetError(EAX,
		EGB_GETBLOCK1BIT_INTERNAL(
			pointerSet.mode,
			blkInfo,
			pointerSet.vram));

	EGB_SetError(EAX,EGB_NO_ERROR);
}

unsigned char EGB_PUTBLOCK1BIT_INTERNAL(
	_Far struct EGB_ScreenMode *scrnMode,
	_Far struct EGB_BlockInfo *blkInfo,
	_Far unsigned char *vramIn,
	_Far unsigned short color[4],
	_Far const struct POINTW viewportIn[2],
	unsigned int flags,
	unsigned int drawingMode)
{
	struct POINTW p0,p1,viewport[2];

	p0=blkInfo->p0;
	p1=blkInfo->p1;
	EGB_MakeP0SmallerThanP1(&p0,&p1);

	if(!(flags&1)) // View-port flag
	{
		// Out of the screen?
		if(p1.x<0 || scrnMode->size.x<=p0.x ||
		   p1.y<0 || scrnMode->size.y<=p0.y)
		{
			return EGB_NO_ERROR;
		}

		// If not using viewport, coordinates must fit inside the screen.
		if(p0.x<0 || scrnMode->size.x<=p1.x ||
		   p0.y<0 || scrnMode->size.y<=p1.y)
		{
			return EGB_GENERAL_ERROR;
		}

		viewport[0].x=0;
		viewport[0].y=0;
		viewport[1].x=scrnMode->size.x-1;
		viewport[1].y=scrnMode->size.y-1;
	}
	else
	{
		viewport[0]=viewportIn[0];
		viewport[1]=viewportIn[1];

		// Out of the viewport?
		if(p1.x<viewport[0].x || viewport[1].x<p0.x ||
		   p1.y<viewport[0].y || viewport[1].y<p0.y)
		{
			return EGB_NO_ERROR;
		}

		if(viewport[0].x<=p0.x && p1.x<=viewport[1].x &&
		   viewport[0].y<=p0.y && p1.y<=viewport[1].y)
		{
			flags&=0xFE; // Entirely inide the viewport.  Don't have to be worried about the viewport.
		}
	}



	if(0==(flags&1))
	{
		unsigned int vramOffset=EGB_CoordToVRAMOffset(scrnMode,p0.x,p0.y);
		if(1==scrnMode->bitsPerPixel)
		{
			// I'll come back when someone do it.
			TSUGARU_BREAK;
		}
		else
		{
			int x,y;
			_Far unsigned char *src=blkInfo->data;
			_Far unsigned char *vram=vramIn+vramOffset,*nextVram;

			switch(scrnMode->bitsPerPixel)
			{
			case 1:
				TSUGARU_BREAK;
				break;
			case 4:
				{
					for(y=p0.y; y<=p1.y; ++y)
					{
						unsigned char fgCol=color[EGB_FOREGROUND_COLOR]&0x0F;
						unsigned char bgCol=color[EGB_BACKGROUND_COLOR]&0x0F;
						unsigned char ANDPtn;
						unsigned char bitCount=0,bits;
						if(0==(p0.x&1))
						{
							ANDPtn=0xF0;
						}
						else
						{
							ANDPtn=0x0F;
							fgCol<<=4;
							bgCol<<=4;
						}

						nextVram=vram+scrnMode->bytesPerLine;
						bits=*src;
						bitCount=0;
						for(x=p0.x; x<=p1.x; ++x)
						{
							switch(drawingMode)
							{
							case EGB_FUNC_OPAQUE:
								*vram&=ANDPtn;
								if(bits&0x80)
								{
									*vram|=fgCol;
								}
								else
								{
									*vram|=bgCol;
								}
								break;
							case EGB_FUNC_PSET:
							case EGB_FUNC_MATTE:
								if(bits&0x80)
								{
									*vram&=ANDPtn;
									*vram|=fgCol;
								}
								break;
							case EGB_FUNC_AND:
								if(!(bits&0x80))
								{
									*vram&=ANDPtn;
								}
								break;
							default:
								TSUGARU_BREAK;
								break;
							}

							if(0xF0==ANDPtn)
							{
								ANDPtn=0x0F;
								fgCol<<=4;
								bgCol<<=4;
							}
							else
							{
								ANDPtn=0xF0;
								fgCol>>=4;
								bgCol>>=4;
								++vram;
							}

							bits<<=1;
							++bitCount;
							if(8==bitCount)
							{
								bitCount=0;
								bits=*(++src);
							}
						}
						if(0!=bitCount)
						{
							++src;
						}
						vram=nextVram;
					}
				}
				break;
			case 8:
				{
					for(y=p0.y; y<=p1.y; ++y)
					{
						unsigned char fgCol=color[EGB_FOREGROUND_COLOR];
						unsigned char bgCol=color[EGB_BACKGROUND_COLOR];
						unsigned char bitCount=0,bits;

						nextVram=vram+scrnMode->bytesPerLine;
						bits=*src;
						bitCount=0;
						for(x=p0.x; x<=p1.x; ++x)
						{
							switch(drawingMode)
							{
							case EGB_FUNC_OPAQUE:
								if(bits&0x80)
								{
									*vram=fgCol;
								}
								else
								{
									*vram=bgCol;
								}
								break;
							case EGB_FUNC_PSET:
							case EGB_FUNC_MATTE:
								if(bits&0x80)
								{
									*vram=fgCol;
								}
								break;
							case EGB_FUNC_AND:
								if(!(bits&0x80))
								{
									*vram=0;
								}
								break;
							default:
								TSUGARU_BREAK;
								break;
							}
							++vram;

							bits<<=1;
							++bitCount;
							if(8==bitCount)
							{
								bitCount=0;
								bits=*(++src);
							}
						}
						if(0!=bitCount)
						{
							++src;
						}
						vram=nextVram;
					}
				}
				break;
			case 16:
				{
					for(y=p0.y; y<=p1.y; ++y)
					{
						unsigned short fgCol=color[EGB_FOREGROUND_COLOR];
						unsigned short bgCol=color[EGB_BACKGROUND_COLOR];
						unsigned char bitCount=0,bits;

						nextVram=vram+scrnMode->bytesPerLine;
						bits=*src;
						bitCount=0;
						for(x=p0.x; x<=p1.x; ++x)
						{
							switch(drawingMode)
							{
							case EGB_FUNC_OPAQUE:
								if(bits&0x80)
								{
									*((_Far unsigned short *)vram)=fgCol;
								}
								else
								{
									*((_Far unsigned short *)vram)=bgCol;
								}
								break;
							case EGB_FUNC_PSET:
							case EGB_FUNC_MATTE:
								if(bits&0x80)
								{
									*((_Far unsigned short *)vram)=fgCol;
								}
								break;
							case EGB_FUNC_AND:
								if(!(bits&0x80))
								{
									*((_Far unsigned short *)vram)=0;
								}
								break;
							default:
								TSUGARU_BREAK;
								break;
							}
							vram+=2;

							bits<<=1;
							++bitCount;
							if(8==bitCount)
							{
								bitCount=0;
								bits=*(++src);
							}
						}
						if(0!=bitCount)
						{
							++src;
						}
						vram=nextVram;
					}
				}
				break;
			}
		}
	}
	else
	{
		unsigned int xSkip=0,ySkip=0;
		unsigned int srcWid;
		unsigned int srcBytesPerLine;
		srcWid=p1.x-p0.x+1;
		srcBytesPerLine=(srcWid+7)/8;

		if(p0.x<viewport[0].x)
		{
			xSkip=viewport[0].x-p0.x;
			p0.x=viewport[0].x;
		}
		if(p0.y<viewport[0].y)
		{
			ySkip=viewport[0].y-p0.y;
			p0.y=viewport[0].y;
		}

		p1.x=_min(viewport[1].x,p1.x);
		p1.y=_min(viewport[1].y,p1.y);
		if(viewport[1].x<p1.x)
		{
			p1.x=viewport[1].x;
		}

		unsigned int vramOffset=EGB_CoordToVRAMOffset(scrnMode,p0.x,p0.y);
		if(1==scrnMode->bitsPerPixel)
		{
			// I'll come back when someone do it.
			TSUGARU_BREAK;
		}
		else
		{
			int x,y;
			_Far unsigned char *src=blkInfo->data+ySkip*srcBytesPerLine;
			_Far unsigned char *vram=vramIn+vramOffset,*nextVram;

			switch(scrnMode->bitsPerPixel)
			{
			case 1:
				TSUGARU_BREAK;
				break;
			case 4:
				{
					for(y=p0.y; y<=p1.y; ++y)
					{
						unsigned char fgCol=color[EGB_FOREGROUND_COLOR]&0x0F;
						unsigned char bgCol=color[EGB_BACKGROUND_COLOR]&0x0F;
						unsigned char ANDPtn;
						unsigned char bitCount=0,bits;
						_Far unsigned char *nextSrc=src+srcBytesPerLine;
						if(0==(p0.x&1))
						{
							ANDPtn=0xF0;
						}
						else
						{
							ANDPtn=0x0F;
							fgCol<<=4;
							bgCol<<=4;
						}

						nextVram=vram+scrnMode->bytesPerLine;
						src+=(xSkip/8);
						bits=*src;
						bitCount=xSkip%8;
						bits<<=bitCount;
						for(x=p0.x; x<=p1.x; ++x)
						{
							switch(drawingMode)
							{
							case EGB_FUNC_OPAQUE:
								*vram&=ANDPtn;
								if(bits&0x80)
								{
									*vram|=fgCol;
								}
								else
								{
									*vram|=bgCol;
								}
								break;
							case EGB_FUNC_PSET:
							case EGB_FUNC_MATTE:
								if(bits&0x80)
								{
									*vram&=ANDPtn;
									*vram|=fgCol;
								}
								break;
							case EGB_FUNC_AND:
								if(!(bits&0x80))
								{
									*vram&=ANDPtn;
								}
								break;
							default:
								TSUGARU_BREAK;
								break;
							}

							if(0xF0==ANDPtn)
							{
								ANDPtn=0x0F;
								fgCol<<=4;
								bgCol<<=4;
							}
							else
							{
								ANDPtn=0xF0;
								fgCol>>=4;
								bgCol>>=4;
								++vram;
							}

							bits<<=1;
							++bitCount;
							if(8==bitCount)
							{
								bitCount=0;
								bits=*(++src);
							}
						}
						if(0!=bitCount)
						{
							++src;
						}
						vram=nextVram;
						src=nextSrc;
					}
				}
				break;
			case 8:
				{
					for(y=p0.y; y<=p1.y; ++y)
					{
						unsigned char fgCol=color[EGB_FOREGROUND_COLOR];
						unsigned char bgCol=color[EGB_BACKGROUND_COLOR];
						unsigned char bitCount=0,bits;
						_Far unsigned char *nextSrc=src+srcBytesPerLine;

						nextVram=vram+scrnMode->bytesPerLine;
						src+=(xSkip/8);
						bits=*src;
						bitCount=xSkip%8;
						bits<<=bitCount;
						for(x=p0.x; x<=p1.x; ++x)
						{
							switch(drawingMode)
							{
							case EGB_FUNC_OPAQUE:
								if(bits&0x80)
								{
									*vram=fgCol;
								}
								else
								{
									*vram=bgCol;
								}
								break;
							case EGB_FUNC_PSET:
							case EGB_FUNC_MATTE:
								if(bits&0x80)
								{
									*vram=fgCol;
								}
								break;
							case EGB_FUNC_AND:
								if(!(bits&0x80))
								{
									*vram=0;
								}
								break;
							default:
								TSUGARU_BREAK;
								break;
							}
							++vram;

							bits<<=1;
							++bitCount;
							if(8==bitCount)
							{
								bitCount=0;
								bits=*(++src);
							}
						}
						if(0!=bitCount)
						{
							++src;
						}
						vram=nextVram;
						src=nextSrc;
					}
				}
				break;
			case 16:
				{
					for(y=p0.y; y<=p1.y; ++y)
					{
						unsigned short fgCol=color[EGB_FOREGROUND_COLOR];
						unsigned short bgCol=color[EGB_BACKGROUND_COLOR];
						unsigned char bitCount=0,bits;
						_Far unsigned char *nextSrc=src+srcBytesPerLine;

						nextVram=vram+scrnMode->bytesPerLine;
						src+=(xSkip/8);
						bits=*src;
						bitCount=xSkip%8;
						bits<<=bitCount;
						for(x=p0.x; x<=p1.x; ++x)
						{
							switch(drawingMode)
							{
							case EGB_FUNC_OPAQUE:
								if(bits&0x80)
								{
									*((_Far unsigned short *)vram)=fgCol;
								}
								else
								{
									*((_Far unsigned short *)vram)=bgCol;
								}
								break;
							case EGB_FUNC_PSET:
							case EGB_FUNC_MATTE:
								if(bits&0x80)
								{
									*((_Far unsigned short *)vram)=fgCol;
								}
								break;
							case EGB_FUNC_AND:
								if(!(bits&0x80))
								{
									*((_Far unsigned short *)vram)=0;
								}
								break;
							default:
								TSUGARU_BREAK;
								break;
							}
							vram+=2;

							bits<<=1;
							++bitCount;
							if(8==bitCount)
							{
								bitCount=0;
								bits=*(++src);
							}
						}
						if(0!=bitCount)
						{
							++src;
						}
						vram=nextVram;
						src=nextSrc;
					}
				}
				break;
			}
		}
	}

	return EGB_NO_ERROR;
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
	unsigned char flags=EAX;
	_Far struct EGB_BlockInfo *blkInfo;
	_FP_SEG(blkInfo)=DS;
	_FP_OFF(blkInfo)=ESI;

	_Far struct EGB_Work *work=EGB_GetWork();

	struct EGB_PagePointerSet pointerSet=EGB_GetPagePointerSet(work);
	struct POINTW viewport[2];

	if(NULL==pointerSet.page)
	{
		EGB_SetError(EAX,EGB_GENERAL_ERROR);
		return;
	}

	if((flags&2) && EGB_MASKMODE_DISABLE!=work->maskMode)
	{
		// I don't think mask is of very high priority.
		TSUGARU_BREAK;
	}

	viewport[0]=pointerSet.page->viewport[0];
	viewport[1]=pointerSet.page->viewport[1];

	EGB_SetError(EAX,
		EGB_PUTBLOCK1BIT_INTERNAL(
			pointerSet.mode,
			blkInfo,
			pointerSet.vram,
			work->color,
			pointerSet.page->viewport,
			flags,
			work->drawingMode));
}

unsigned char EGB_GETBLOCK_INTERNAL(
	_Far struct EGB_ScreenMode *scrnMode,
	_Far struct EGB_BlockInfo *blkInfo,
	_Far unsigned char *vramIn)
{
	struct POINTW p0,p1;

	p0=blkInfo->p0;
	p1=blkInfo->p1;
	EGB_MakeP0SmallerThanP1(&p0,&p1);

	unsigned int dx=p1.x-p0.x+1;
	unsigned int dy=p1.y-p0.y+1;

	// Out of the screen?
	if(p1.x<0 || scrnMode->size.x<=p0.x ||
	   p1.y<0 || scrnMode->size.y<=p0.y)
	{
		unsigned int bytesToFill=0;
		switch(scrnMode->bitsPerPixel)
		{
		case 1:
			bytesToFill=(dx+7)/8*dy;		// byte-aligned
			break;
		case 4:
			bytesToFill=(dx+7)/8*4*dy;	// dword-aligned
			break;
		case 8:
			bytesToFill=dx*dy;
			break;
		case 16:
			bytesToFill=dx*dy*2;
			break;
		}
		MEMSETB_FAR(blkInfo->data,0,bytesToFill);
		return EGB_NO_ERROR;
	}

	unsigned int dstBytesPerLine=0,transferBytesPerLine=0;
	switch(scrnMode->bitsPerPixel)
	{
	case 1:
		dstBytesPerLine=(dx+7)/8;
		transferBytesPerLine=dx/8;
		break;
	case 4:
		dstBytesPerLine=((dx+7)/8)*4;
		transferBytesPerLine=dx/2;
		break;
	case 8:
		dstBytesPerLine=dx;
		transferBytesPerLine=dx;
		break;
	case 16:
		dstBytesPerLine=dx*2;
		transferBytesPerLine=dx*2;
		break;
	}


	_Far unsigned char *dst=blkInfo->data;
	if(p0.y<0)
	{
		unsigned int toFill=-p0.y*dstBytesPerLine;
		MEMSETB_FAR(dst,0,toFill);
		p0.y=0;
		dst+=toFill;
	}

	unsigned int vramOffset=EGB_CoordToVRAMOffset(scrnMode,_max(p0.x,0),p0.y);
	if(1==scrnMode->bitsPerPixel)
	{
		// I'll come back when someone do it.
		TSUGARU_BREAK;
	}
	else
	{
		int y;
		unsigned int yBelowScreen=0;
		_Far unsigned char *vram=vramIn+vramOffset;

		if(scrnMode->size.y<=p1.y)
		{
			yBelowScreen=p1.y-scrnMode->size.y+1;
			p1.y=scrnMode->size.y-1;
		}

		unsigned int xLeft=0,xRight=0;
		int x0=p0.x,x1=p1.x;

		if(x0<0)
		{
			xLeft=-x0;
			x0=0;
		}
		if(scrnMode->size.x<=x1)
		{
			xRight=x1+1-scrnMode->size.x;
			x1=scrnMode->size.x-1;
		}

		if(4==scrnMode->bitsPerPixel && ((p0.x&1) || (dx&1) || 0<xLeft))
		{
			for(y=p0.y; y<=p1.y; ++y)
			{
				_Far unsigned char *nextDst=dst+dstBytesPerLine;
				_Far unsigned char *nextVram=vram+scrnMode->bytesPerLine;
				unsigned char dstShift=0,srcShift=0;
				int x=x0;

				if(0!=xLeft)
				{
					// If x starts left of the screen, (0<xLeft)
					//   (1) If xLeft is even, just clear xLeft/2 (which is same as (xLeft+1)/2) bytes, fast-forward dst by xLeft/2.
					//   (2) If xLeft is odd, clear (xLeft+1)/2 bytes, fast forward dst by xLeft-1/2 (which is same as xLeft/2) bytes.
					//       The low 4-bits of the first vram byte will be high 4-bits of the next *dst.
					MEMSETB_FAR(dst,0,(xLeft+1)/2);
					dst+=xLeft/2;
					if(xLeft&1)
					{
						dstShift=4;  // Next pixel will be high-bits of the destination.
					}
				}
				if(x&1)
				{
					srcShift=4;
				}

				// Either way, x is even at this point.

				if(srcShift==dstShift)
				{
					if(4==dstShift && 4==srcShift)
					{
						(*dst)&=0x0F;
						(*dst)|=(*vram)&0xF0;
						++dst;
						++vram;
						++x;
					}
					unsigned int w=x1+1-x;
					unsigned int bytes=w/2;
					MEMCPY_FAR(dst,vram,bytes);
					dst+=bytes;
					vram+=bytes;
					if(w&1)
					{
						(*dst)=(*vram)&0x0F;
						++x;
						--w;
					}
					if(xRight)
					{
						MEMSETB_FAR(dst,0,xRight/2);
					}
				}
				else
				{
					while(x<=x1)
					{
						if(dstShift)
						{
							(*dst)&=0x0F;
							(*dst)|=((*vram)<<4);
							++dst;
							dstShift=0;
							srcShift=4;
						}
						else
						{
							(*dst)&=0xF0;
							(*dst)|=((*vram)>>4);
							++vram;
							dstShift=4;
							srcShift=0;
						}
						++x;
					}

					if(dstShift)
					{
						(*dst)&=0x0F;
						++dst;
						++x;
					}
					if(xRight)
					{
						MEMSETB_FAR(dst,0,xRight/2);
					}
				}

				dst=nextDst;
				vram=nextVram;
			}
		}
		else if(0<=p0.x && p1.x<scrnMode->size.x)
		{
			for(y=p0.y; y<=p1.y; ++y)
			{
				MEMCPY_FAR(dst,vram,transferBytesPerLine);
				dst+=dstBytesPerLine;
				vram+=scrnMode->bytesPerLine;
			}
		}
		else if(8==scrnMode->bitsPerPixel)
		{
			for(y=p0.y; y<=p1.y; ++y)
			{
				_Far unsigned char *nextDst=dst+dstBytesPerLine;
				_Far unsigned char *nextVram=vram+scrnMode->bytesPerLine;

				MEMSETB_FAR(dst,0,xLeft);
				dst+=xLeft;
				// If x<0, vramOffset is set to x=0.

				unsigned int w=x1+1-x0;
				MEMCPY_FAR(dst,vram,w);
				dst+=w;

				MEMSETB_FAR(dst,0,xRight);

				dst=nextDst;
				vram=nextVram;
			}
		}
		else if(16==scrnMode->bitsPerPixel)
		{
			for(y=p0.y; y<=p1.y; ++y)
			{
				_Far unsigned char *nextDst=dst+dstBytesPerLine;
				_Far unsigned char *nextVram=vram+scrnMode->bytesPerLine;

				MEMSETB_FAR(dst,0,xLeft*2);
				dst+=xLeft*2;
				// If x<0, vramOffset is set to x=0.

				unsigned int w=x1+1-x0;
				MEMCPY_FAR(dst,vram,w*2);
				dst+=w*2;

				MEMSETB_FAR(dst,0,xRight*2);

				dst=nextDst;
				vram=nextVram;
			}
		}

		if(0<yBelowScreen)
		{
			MEMSETB_FAR(dst,0,yBelowScreen*dstBytesPerLine);
		}
	}

	return EGB_NO_ERROR;
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
	_Far struct EGB_BlockInfo *blkInfo;
	_FP_SEG(blkInfo)=DS;
	_FP_OFF(blkInfo)=ESI;

	_Far struct EGB_Work *work=EGB_GetWork();

	struct EGB_PagePointerSet ptrSet=EGB_GetPagePointerSet(work);

	if(NULL==ptrSet.page)
	{
		EGB_SetError(EAX,EGB_GENERAL_ERROR);
		return;
	}

	EGB_SetError(EAX,
		EGB_GETBLOCK_INTERNAL(
			ptrSet.mode,
			blkInfo,
			ptrSet.vram));
}

unsigned char EGB_PUTBLOCK_INTERNAL(
	_Far struct EGB_ScreenMode *scrnMode,
	_Far struct EGB_BlockInfo *blkInfo,
	_Far unsigned char *vramIn,
	_Far unsigned short color[4],
	_Far const struct POINTW viewportIn[2],
	unsigned int flags,
	unsigned int drawingMode)
{
	struct POINTW p0,p1,viewport[2];

	p0=blkInfo->p0;
	p1=blkInfo->p1;
	EGB_MakeP0SmallerThanP1(&p0,&p1);

	if(!(flags&1)) // View-port flag
	{
		// Out of the screen?
		if(p1.x<0 || scrnMode->size.x<=p0.x ||
		   p1.y<0 || scrnMode->size.y<=p0.y)
		{
			return EGB_NO_ERROR;
		}

		// If not using viewport, coordinates must fit inside the screen.
		if(p0.x<0 || scrnMode->size.x<=p1.x ||
		   p0.y<0 || scrnMode->size.y<=p1.y)
		{
			return EGB_GENERAL_ERROR;
		}

		viewport[0].x=0;
		viewport[0].y=0;
		viewport[1].x=scrnMode->size.x-1;
		viewport[1].y=scrnMode->size.y-1;
	}
	else
	{
		viewport[0]=viewportIn[0];
		viewport[1]=viewportIn[1];

		// Out of the viewport?
		if(p1.x<viewport[0].x || viewport[1].x<p0.x ||
		   p1.y<viewport[0].y || viewport[1].y<p0.y)
		{
			return EGB_NO_ERROR;
		}

		if(viewport[0].x<=p0.x && p1.x<=viewport[1].x &&
		   viewport[0].y<=p0.y && p1.y<=viewport[1].y)
		{
			flags&=0xFE; // Entirely inide the viewport.  Don't have to be worried about the viewport.
		}
	}


	unsigned int dx=(p1.x-p0.x+1);
	unsigned int srcBytesPerLine=0,transferBytesPerLine=0;
	switch(scrnMode->bitsPerPixel)
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

	if(0==(flags&1)) // Fully inside the viewport or screen
	{
		unsigned int vramOffset=EGB_CoordToVRAMOffset(scrnMode,p0.x,p0.y);
		if(1==scrnMode->bitsPerPixel)
		{
			// I'll come back when someone do it.
			TSUGARU_BREAK;
		}
		else
		{
			int x,y;
			_Far unsigned char *src=blkInfo->data;
			_Far unsigned char *vram=vramIn+vramOffset;
			switch(drawingMode)
			{
			case EGB_FUNC_OPAQUE:
			case EGB_FUNC_PSET:
				if(4==scrnMode->bitsPerPixel && ((p0.x&1) || (dx&1)))
				{
					for(y=p0.y; y<=p1.y; ++y)
					{
						_Far unsigned char *nextSrc=src+srcBytesPerLine;
						_Far unsigned char *nextVram=vram+scrnMode->bytesPerLine;
						if(p0.x&1)
						{
							unsigned char srcMask=0x0F,srcShift=0,dstAndPtn=0x0F,dstShift=4;
							for(x=p0.x; x<=p1.x; ++x)
							{
								(*vram)&=dstAndPtn;
								(*vram)|=(((*src)&srcMask)>>srcShift)<<dstShift;
								srcMask=~srcMask;
								srcShift=4-srcShift;
								dstAndPtn=~dstAndPtn;
								dstShift=4-dstShift;
								if(0==srcShift)
								{
									++src;
								}
								else
								{
									++vram;
								}
							}
						}
						else
						{
							for(x=p0.x; x+1<=p1.x; x+=2)
							{
								*(vram++)=*(src++);
							}
							if(x==p1.x)
							{
								(*vram)&=0xF0;
								(*vram)|=((*src)&0x0F);
							}
						}
						src=nextSrc;
						vram=nextVram;
					}
				}
				else
				{
					for(y=p0.y; y<=p1.y; ++y)
					{
						MEMCPY_FAR(vram,src,transferBytesPerLine);
						src+=srcBytesPerLine;
						vram+=scrnMode->bytesPerLine;
					}
				}
				break;
			case EGB_FUNC_MATTE:
				{
					unsigned short transparentColor=color[EGB_TRANSPARENT_COLOR];
					register unsigned int count;
					unsigned int xCount=p1.x+1-p0.x;
					{
						switch(scrnMode->bitsPerPixel)
						{
						case 1:
							transparentColor&=1;
							TSUGARU_BREAK;
							break;
						case 4:
							transparentColor&=15;
							for(y=p0.y; y<=p1.y; ++y)
							{
								register _Far unsigned char *srcPtr,*dstPtr;
								unsigned char srcShift,dstAndPtn,dstShift;
								srcPtr=src;
								srcShift=0;
								dstPtr=vram;
								if(!(p0.x&1))
								{
									dstAndPtn=0xF0;
									dstShift=0;
								}
								else
								{
									dstAndPtn=0x0F;
									dstShift=4;
								}
								for(count=xCount; 0<count; --count)
								{
									unsigned short srcColor;
									srcColor=((*srcPtr)>>srcShift)&0x0F;
									if(srcColor!=transparentColor)
									{
										*dstPtr&=dstAndPtn;
										*dstPtr|=(srcColor<<dstShift);
									}
									dstAndPtn=~dstAndPtn;
									dstShift=4-dstShift;
									if(0==dstShift)
									{
										++dstPtr;
									}
									srcShift=4-srcShift;
									if(0==srcShift)
									{
										++srcPtr;
									}
								}
								src+=srcBytesPerLine;
								vram+=scrnMode->bytesPerLine;
							}
							break;
						case 8:
							transparentColor&=255;
							for(y=p0.y; y<=p1.y; ++y)
							{
								register _Far unsigned char *srcPtr,*dstPtr;
								srcPtr=src;
								dstPtr=vram;
								for(count=xCount; 0<count; --count)
								{
									if(*srcPtr!=transparentColor)
									{
										*dstPtr=*srcPtr;
									}
									++dstPtr;
									++srcPtr;
								}
								src+=srcBytesPerLine;
								vram+=scrnMode->bytesPerLine;
							}
							break;
						case 16:
							transparentColor&=0x7FFF;
							for(y=p0.y; y<=p1.y; ++y)
							{
								register _Far unsigned short *srcPtr,*dstPtr;
								srcPtr=(_Far unsigned short *)src;
								dstPtr=(_Far unsigned short *)vram;
								for(count=xCount; 0<count; --count)
								{
									if(*srcPtr!=transparentColor)
									{
										*dstPtr=*srcPtr;
									}
									++dstPtr;
									++srcPtr;
								}
								src+=srcBytesPerLine;
								vram+=scrnMode->bytesPerLine;
							}
							break;
						}
					}
				}
				break;
			default:
				TSUGARU_BREAK;
				break;
			}
		}
	}
	else // Partially visible.
	{
		unsigned int xSkip=0,ySkip=0;

		if(p0.x<viewport[0].x)
		{
			xSkip=viewport[0].x-p0.x;
			p0.x=viewport[0].x;
		}
		if(p0.y<viewport[0].y)
		{
			ySkip=viewport[0].y-p0.y;
			p0.y=viewport[0].y;
		}

		p1.x=_min(viewport[1].x,p1.x);
		p1.y=_min(viewport[1].y,p1.y);
		if(viewport[1].x<p1.x)
		{
			p1.x=viewport[1].x;
		}

		unsigned int vramOffset=EGB_CoordToVRAMOffset(scrnMode,p0.x,p0.y);
		if(1==scrnMode->bitsPerPixel)
		{
			// I'll come back when someone do it.
			TSUGARU_BREAK;
		}
		else
		{
			int x,y;
			_Far unsigned char *src=blkInfo->data+srcBytesPerLine*ySkip;
			_Far unsigned char *vram=vramIn+vramOffset;
			switch(drawingMode)
			{
			case EGB_FUNC_OPAQUE:
			case EGB_FUNC_PSET:
				switch(scrnMode->bitsPerPixel)
				{
				case 1:
					TSUGARU_BREAK
					break;
				case 4:
					for(y=p0.y; y<=p1.y; ++y)
					{
						_Far unsigned char *srcPtr,*dstPtr;
						unsigned char srcShift=0,dstAndPtn=0xF0,dstShift=0;

						srcPtr=src+xSkip/2;
						dstPtr=vram;
						if(p0.x&1)
						{
							dstAndPtn=0x0F;
							dstShift=4;
						}
						if(xSkip&1)
						{
							srcShift=4;
						}

						for(x=p0.x; x<=p1.x; ++x)
						{
							unsigned short srcColor;
							srcColor=((*srcPtr)>>srcShift)&0x0F;

							*dstPtr&=dstAndPtn;
							*dstPtr|=(srcColor<<dstShift);

							dstAndPtn=~dstAndPtn;
							dstShift=4-dstShift;
							if(0==dstShift)
							{
								++dstPtr;
							}
							srcShift=4-srcShift;
							if(0==srcShift)
							{
								++srcPtr;
							}
						}
						src+=srcBytesPerLine;
						vram+=scrnMode->bytesPerLine;
					}
					break;
				case 8:
					transferBytesPerLine=p1.x-p0.x+1;
					for(y=p0.y; y<=p1.y; ++y)
					{
						src+=xSkip;
						MEMCPY_FAR(vram,src,transferBytesPerLine);
						src+=(srcBytesPerLine-xSkip);
						vram+=scrnMode->bytesPerLine;
					}
					break;
				case 16:
					transferBytesPerLine=p1.x-p0.x+1;
					for(y=p0.y; y<=p1.y; ++y)
					{
						src+=xSkip*2;
						MEMCPY_FAR(vram,src,transferBytesPerLine*2);
						src+=(srcBytesPerLine-xSkip*2);
						vram+=scrnMode->bytesPerLine;
					}
					break;
				}
				break;
			case EGB_FUNC_MATTE:
				{
					unsigned short transparentColor=color[EGB_TRANSPARENT_COLOR];
					register unsigned int count;
					unsigned int xCount=p1.x+1-p0.x;
					switch(scrnMode->bitsPerPixel)
					{
					case 1:
						transparentColor&=1;
						TSUGARU_BREAK;
						break;
					case 4:
						transparentColor&=15;
						src+=xSkip/2;
						for(y=p0.y; y<=p1.y; ++y)
						{
							_Far unsigned char *srcPtr,*dstPtr;
							unsigned char srcShift=0,dstAndPtn=0xF0,dstShift=0;

							srcPtr=src;
							dstPtr=vram;
							if(p0.x&1)
							{
								dstAndPtn=0x0F;
								dstShift=4;
							}
							if(xSkip&1)
							{
								srcShift=4;
							}

							for(count=xCount; 0<count; --count)
							{
								unsigned short srcColor;
								srcColor=((*srcPtr)>>srcShift)&0x0F;

								if(srcColor!=transparentColor)
								{
									*dstPtr&=dstAndPtn;
									*dstPtr|=(srcColor<<dstShift);
								}

								dstAndPtn=~dstAndPtn;
								dstShift=4-dstShift;
								if(0==dstShift)
								{
									++dstPtr;
								}
								srcShift=4-srcShift;
								if(0==srcShift)
								{
									++srcPtr;
								}
							}
							src+=srcBytesPerLine;
							vram+=scrnMode->bytesPerLine;
						}
						break;
					case 8:
						transparentColor&=255;
						src+=xSkip;
						for(y=p0.y; y<=p1.y; ++y)
						{
							register _Far unsigned char *srcPtr,*dstPtr;
							srcPtr=src;
							dstPtr=vram;
							for(count=xCount; 0<count; --count)
							{
								if(*srcPtr!=transparentColor)
								{
									*dstPtr=*srcPtr;
								}
								++dstPtr;
								++srcPtr;
							}
							src+=srcBytesPerLine;
							vram+=scrnMode->bytesPerLine;
						}
						break;
					case 16:
						transparentColor&=0x7FFF;
						src+=xSkip;
						for(y=p0.y; y<=p1.y; ++y)
						{
							register _Far unsigned short *srcPtr,*dstPtr;
							srcPtr=(_Far unsigned short *)src;
							dstPtr=(_Far unsigned short *)vram;
							for(count=xCount; 0<count; --count)
							{
								if(*srcPtr!=transparentColor)
								{
									*dstPtr=*srcPtr;
								}
								++dstPtr;
								++srcPtr;
							}
							src+=srcBytesPerLine;
							vram+=scrnMode->bytesPerLine;
						}
						break;
					}
				}
				break;
			default:
				TSUGARU_BREAK;
				break;
			}
		}
	}

	return EGB_NO_ERROR;
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

	_Far struct EGB_Work *work=EGB_GetWork();

	struct EGB_PagePointerSet pointerSet=EGB_GetPagePointerSet(work);

	if(NULL==pointerSet.page)
	{
		EGB_SetError(EAX,EGB_GENERAL_ERROR);
		return;
	}

	if((flags&2) && EGB_MASKMODE_DISABLE!=work->maskMode)
	{
		// I don't think mask is of very high priority.
		TSUGARU_BREAK;
	}

	EGB_SetError(EAX,
		EGB_PUTBLOCK_INTERNAL(
			pointerSet.mode,
			blkInfo,
			pointerSet.vram,
			work->color,
			pointerSet.page->viewport,
			flags,
			work->drawingMode));
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

void EGB_40H_PSET(
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

void EGB_DrawLine(_Far struct EGB_Work *work,struct EGB_PagePointerSet *ptrSet,struct POINTW p0,struct POINTW p1)
{
	int dx=p1.x-p0.x;
	int dy=p1.y-p0.y;
	unsigned int wid,hei;
	int vx,vy,VRAMStep;

	// The following expression here will step on High-C's bug.  The result from the above computation is stored in EDX and ECX,
	// but it destroys EDX in the subsequent CDQ.
	//unsigned int wid=_abs(dx);
	//unsigned int hei=_abs(dy);
	// 0110:00002188 55                        PUSH    EBP
	// 0110:00002189 8BEC                      MOV     EBP,ESP
	// 0110:0000218B 83EC28                    SUB     ESP,28H
	// 0110:0000218E 53                        PUSH    EBX
	// 0110:0000218F 56                        PUSH    ESI
	// 0110:00002190 57                        PUSH    EDI
	// 0110:00002191 8B7D10                    MOV     EDI,[EBP+10H]
	// 0110:00002194 0FBF4514                  MOVSX   EAX,WORD PTR [EBP+14H]
	// 0110:00002198 0FBF5518                  MOVSX   EDX,WORD PTR [EBP+18H]
	// 0110:0000219C 2BD0                      SUB     EDX,EAX
	// 0110:0000219E 8955FC                    MOV     [EBP-04H],EDX              ; EDX is dx
	// 0110:000021A1 0FBF4516                  MOVSX   EAX,WORD PTR [EBP+16H]
	// 0110:000021A5 0FBF4D1A                  MOVSX   ECX,WORD PTR [EBP+1AH]
	// 0110:000021A9 2BC8                      SUB     ECX,EAX
	// 0110:000021AB 894DF8                    MOV     [EBP-08H],ECX              ; ECX is dy
	// 0110:000021AE 99                        CDQ                                ; EDX destroyed.  MOV EAX,EDX is missing.
	// 0110:000021AF 33C2                      XOR     EAX,EDX
	// 0110:000021B1 2BC2                      SUB     EAX,EDX                    ; Ah, it's a smart way of taking ABS, if only it works.
	// 0110:000021B3 8955F4                    MOV     [EBP-0CH],EDX
	// 0110:000021B6 99                        CDQ                                ; Hey, High-C, dy is ECX.  Did you forget MOV EAX,ECX?
	// 0110:000021B7 33C2                      XOR     EAX,EDX
	// 0110:000021B9 2BC2                      SUB     EAX,EDX
	// 0110:000021BB 894DF0                    MOV     [EBP-10H],ECX
	// 0110:000021BE 23D2                      AND     EDX,EDX
	// 0110:000021C0 0F85DD010000              JNE     000023A3

	if(0==dx)
	{
		int y;
		int yMin=_min(p0.y,p1.y);
		int yMax=_max(p0.y,p1.y);
		unsigned int vramAddr;
		unsigned char andPtn;
		unsigned short color;
		if(yMax<ptrSet->page->viewport[0].y || ptrSet->page->viewport[1].y<yMax)
		{
			return;
		}

		yMin=_max(yMin,ptrSet->page->viewport[0].y);
		yMax=_min(yMax,ptrSet->page->viewport[1].y);

		if(0!=ptrSet->mode->bytesPerLineShift)
		{
			vramAddr=(yMin<<ptrSet->mode->bytesPerLineShift);
			vramAddr+=(p0.x*ptrSet->mode->bitsPerPixel)/8;
		}
		else
		{
			vramAddr=((yMin*ptrSet->mode->bytesPerLine+p0.x)*ptrSet->mode->bitsPerPixel)>>3;
		}

		if(4==ptrSet->mode->bitsPerPixel)
		{
			if(p0.x&1)
			{
				andPtn=0x0F;
				color=work->color[EGB_FOREGROUND_COLOR]<<4;
			}
			else
			{
				andPtn=0xF0;
				color=work->color[EGB_FOREGROUND_COLOR];
			}
		}
		else
		{
			color=work->color[EGB_FOREGROUND_COLOR];
		}

		switch(ptrSet->mode->bitsPerPixel)
		{
		case 4:
			switch(work->drawingMode)
			{
			case EGB_FUNC_PSET:
			case EGB_FUNC_OPAQUE:
			case EGB_FUNC_MATTE:
				for(y=yMin; y<=yMax; ++y)
				{
					ptrSet->vram[vramAddr]&=andPtn;
					ptrSet->vram[vramAddr]|=color;
					vramAddr+=ptrSet->mode->bytesPerLine;
				}
				break;
			case EGB_FUNC_XOR:
				ptrSet->vram[vramAddr]^=color;
				vramAddr+=ptrSet->mode->bytesPerLine;
				break;
			default:
				TSUGARU_BREAK;
				break;
			}
			break;
		case 8:
			switch(work->drawingMode)
			{
			case EGB_FUNC_PSET:
			case EGB_FUNC_OPAQUE:
			case EGB_FUNC_MATTE:
				for(y=yMin; y<=yMax; ++y)
				{
					ptrSet->vram[vramAddr]=color;
					vramAddr+=ptrSet->mode->bytesPerLine;
				}
				break;
			default:
				TSUGARU_BREAK;
				break;
			}
			break;
		case 16:
			switch(work->drawingMode)
			{
			case EGB_FUNC_PSET:
			case EGB_FUNC_OPAQUE:
			case EGB_FUNC_MATTE:
				for(y=yMin; y<=yMax; ++y)
				{
					*(_Far unsigned short*)(ptrSet->vram+vramAddr)=color;
					vramAddr+=ptrSet->mode->bytesPerLine;
				}
				break;
			default:
				TSUGARU_BREAK;
				break;
			}
			break;
		default:
			TSUGARU_BREAK;
			break;
		}
		return;
	}
	else if(0==dy)
	{
		int xMin=_min(p0.x,p1.x);
		int xMax=_max(p0.x,p1.x);
		unsigned int vramAddr,color;

		if(xMax<ptrSet->page->viewport[0].x || ptrSet->page->viewport[1].x<xMax)
		{
			return;
		}

		xMin=_max(xMin,ptrSet->page->viewport[0].x);
		xMax=_min(xMax,ptrSet->page->viewport[1].x);

		EGB_CalcVRAMAddr(&vramAddr,xMin,p0.y,ptrSet->mode);

		color=GetExpandedColor(work->color[EGB_FOREGROUND_COLOR],ptrSet->mode->bitsPerPixel);
		switch(ptrSet->mode->bitsPerPixel)
		{
		case 4:
			switch(work->drawingMode)
			{
			case EGB_FUNC_PSET:
			case EGB_FUNC_OPAQUE:
			case EGB_FUNC_MATTE:
				if(xMin&1)
				{
					ptrSet->vram[vramAddr]&=0x0F;
					ptrSet->vram[vramAddr]|=(color&0xF0);
					++vramAddr;
					++xMin;
				}
				{
					unsigned int count=(xMax+1-xMin)/2;
					MEMSETB_FAR(ptrSet->vram+vramAddr,color,count);
					vramAddr+=count;
				}
				if(!(xMax&1))
				{
					ptrSet->vram[vramAddr]&=0xF0;
					ptrSet->vram[vramAddr]|=(color&0x0F);
				}
				break;
			case EGB_FUNC_XOR:
				if(xMin&1)
				{
					ptrSet->vram[vramAddr]^=(color&0xF0);
					++vramAddr;
					++xMin;
				}
				{
					int i;
					unsigned int count=(xMax+1-xMin)/2;
					unsigned int countDiv4=(count>>2),countMod4=(count&3);
					for(i=0; i<countDiv4; ++i)
					{
						*((_Far unsigned int *)(ptrSet->vram+vramAddr))^=color;
						vramAddr+=4;
					}
					for(i=0; i<countMod4; ++i)
					{
						*(ptrSet->vram+vramAddr)^=color;
						++vramAddr;
					}
				}
				if(!(xMax&1))
				{
					ptrSet->vram[vramAddr]^=(color&0x0F);
				}
				break;
			default:
				TSUGARU_BREAK;
				break;
			}
			break;
		case 8:
			switch(work->drawingMode)
			{
			case EGB_FUNC_PSET:
			case EGB_FUNC_OPAQUE:
			case EGB_FUNC_MATTE:
				MEMSETB_FAR(ptrSet->vram+vramAddr,color,xMax-xMin+1);
				break;
			default:
				TSUGARU_BREAK;
				break;
			}
			break;
		case 16:
			switch(work->drawingMode)
			{
			case EGB_FUNC_PSET:
			case EGB_FUNC_OPAQUE:
			case EGB_FUNC_MATTE:
				MEMSETW_FAR(ptrSet->vram+vramAddr,color,xMax-xMin+1);
				break;
			default:
				TSUGARU_BREAK;
				break;
			}
			break;
		}
		return;
	}

	// Note to myself.  High-C's inline _abs is dangerous.
	if(dx<0)
	{
		struct POINTW p;
		p=p0;
		p0=p1;
		p1=p;
		dx=-dx;
		dy=-dy;
	}

	// Always left to right
	wid=dx;
	vx=1;
	if(0<dy)
	{
		hei=dy;
		vy=1;
		VRAMStep=ptrSet->mode->bytesPerLine;
	}
	else
	{
		hei=-dy;
		vy=-1;
		VRAMStep=-ptrSet->mode->bytesPerLine;
	}

	if(hei<wid)
	{
		int balance=wid/2;
		short x=p0.x,y=p0.y;
		unsigned int vramAddr;
		_Far unsigned char *VRAM=ptrSet->vram;
		EGB_CalcVRAMAddr(&vramAddr,x,y,ptrSet->mode);

		switch(ptrSet->mode->bitsPerPixel)
		{
		case 1:
			TSUGARU_BREAK;
			break;
		case 4:
			{
				unsigned char ANDPtn,ORPtn;

				if(0==(x&1))
				{
					ANDPtn=0xF0;
					ORPtn=(work->color[EGB_FOREGROUND_COLOR]&0x0F);
				}
				else
				{
					ANDPtn=0x0F;
					ORPtn=(work->color[EGB_FOREGROUND_COLOR]&0x0F)<<4;
				}

				for(;;)
				{
					switch(work->drawingMode)
					{
					case EGB_FUNC_PSET:
					case EGB_FUNC_OPAQUE:
					case EGB_FUNC_MATTE:
						VRAM[vramAddr]&=ANDPtn;
						VRAM[vramAddr]|=ORPtn;
						break;
					case EGB_FUNC_XOR:
						VRAM[vramAddr]^=ORPtn;
						break;
					default:
						TSUGARU_BREAK;
						break;
					}

					if(x==p1.x && y==p1.y)
					{
						break;
					}

					if(0xF0==ANDPtn)
					{
						ANDPtn=0x0F;
						ORPtn<<=4;
					}
					else
					{
						++vramAddr;
						ANDPtn=0xF0;
						ORPtn>>=4;
					}
					x+=vx;
					balance-=hei;
					if(balance<0)
					{
						y+=vy;
						vramAddr+=VRAMStep;
						balance+=wid;
					}
				}
			}
			break;
		case 8:
			{
				unsigned char col=work->color[EGB_FOREGROUND_COLOR];

				for(;;)
				{
					switch(work->drawingMode)
					{
					case EGB_FUNC_PSET:
					case EGB_FUNC_OPAQUE:
					case EGB_FUNC_MATTE:
						VRAM[vramAddr]=col;
						break;
					default:
						TSUGARU_BREAK;
						break;
					}

					if(x==p1.x && y==p1.y)
					{
						break;
					}

					++vramAddr;
					x+=vx;
					balance-=hei;
					if(balance<0)
					{
						y+=vy;
						vramAddr+=VRAMStep;
						balance+=wid;
					}
				}
			}
			break;
		case 16:
			{
				unsigned short col=work->color[EGB_FOREGROUND_COLOR];

				for(;;)
				{
					switch(work->drawingMode)
					{
					case EGB_FUNC_PSET:
					case EGB_FUNC_OPAQUE:
					case EGB_FUNC_MATTE:
						*((_Far unsigned short *)(VRAM+vramAddr))=col;
						break;
					default:
						TSUGARU_BREAK;
						break;
					}

					if(x==p1.x && y==p1.y)
					{
						break;
					}

					vramAddr+=2;
					x+=vx;
					balance-=hei;
					if(balance<0)
					{
						y+=vy;
						vramAddr+=VRAMStep;
						balance+=wid;
					}
				}
			}
			break;
		}
	}
	else // if(wid<hei)
	{
		int balance=hei/2;
		short x=p0.x,y=p0.y;
		unsigned int vramAddr;
		_Far unsigned char *VRAM=ptrSet->vram;
		EGB_CalcVRAMAddr(&vramAddr,x,y,ptrSet->mode);

		switch(ptrSet->mode->bitsPerPixel)
		{
		case 1:
			TSUGARU_BREAK;
			break;
		case 4:
			{
				unsigned char ANDPtn,ORPtn;

				if(0==(x&1))
				{
					ANDPtn=0xF0;
					ORPtn=(work->color[EGB_FOREGROUND_COLOR]&0x0F);
				}
				else
				{
					ANDPtn=0x0F;
					ORPtn=(work->color[EGB_FOREGROUND_COLOR]&0x0F)<<4;
				}

				for(;;)
				{
					switch(work->drawingMode)
					{
					case EGB_FUNC_PSET:
					case EGB_FUNC_OPAQUE:
					case EGB_FUNC_MATTE:
						VRAM[vramAddr]&=ANDPtn;
						VRAM[vramAddr]|=ORPtn;
						break;
					default:
						TSUGARU_BREAK;
						break;
					}

					if(x==p1.x && y==p1.y)
					{
						break;
					}

					y+=vy;
					balance-=wid;
					vramAddr+=VRAMStep;
					if(balance<0)
					{
						if(0xF0==ANDPtn)
						{
							ANDPtn=0x0F;
							ORPtn<<=4;
						}
						else
						{
							++vramAddr;
							ANDPtn=0xF0;
							ORPtn>>=4;
						}
						x+=vx;
						balance+=hei;
					}
				}
			}
			break;
		case 8:
			{
				unsigned char col=work->color[EGB_FOREGROUND_COLOR];

				for(;;)
				{
					switch(work->drawingMode)
					{
					case EGB_FUNC_PSET:
					case EGB_FUNC_OPAQUE:
					case EGB_FUNC_MATTE:
						VRAM[vramAddr]=col;
						break;
					default:
						TSUGARU_BREAK;
						break;
					}

					if(x==p1.x && y==p1.y)
					{
						break;
					}

					y+=vy;
					vramAddr+=VRAMStep;
					balance-=wid;
					if(balance<0)
					{
						x+=vx;
						vramAddr++;
						balance+=hei;
					}
				}
			}
			break;
		case 16:
			{
				unsigned short col=work->color[EGB_FOREGROUND_COLOR];

				for(;;)
				{
					switch(work->drawingMode)
					{
					case EGB_FUNC_PSET:
					case EGB_FUNC_OPAQUE:
					case EGB_FUNC_MATTE:
						*((_Far unsigned short *)(VRAM+vramAddr))=col;
						break;
					default:
						TSUGARU_BREAK;
						break;
					}

					if(x==p1.x && y==p1.y)
					{
						break;
					}

					y+=vy;
					vramAddr+=VRAMStep;
					balance-=wid;
					if(balance<0)
					{
						vramAddr+=2;
						x+=vx;
						balance+=hei;
					}
				}
			}
			break;
		}
	}
}

void EGB_41H_CONNECT(
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
	unsigned char flags=EAX;
	_Far struct EGB_BlockInfo *blkInfo;
	_FP_SEG(blkInfo)=DS;
	_FP_OFF(blkInfo)=ESI;

	_Far struct EGB_Work *work=EGB_GetWork();

	struct EGB_PagePointerSet pointerSet=EGB_GetPagePointerSet(work);

	_Far unsigned short *count;
	_FP_SEG(count)=DS;
	_FP_OFF(count)=ESI;
	if(2<=*count)
	{
		_Far short *points=(_Far short *)(count+1);
		for(i=0; i+1<*count; ++i)
		{
			struct POINTW p0,p1;
			p0.x=points[i*2];
			p0.y=points[i*2+1];
			p1.x=points[i*2+2];
			p1.y=points[i*2+3];
			if(ClipLine(&p0,&p1,pointerSet.page->viewport[0],pointerSet.page->viewport[1]))
			{
				EGB_DrawLine(work,&pointerSet,p0,p1);
			}
		}
	}

	EGB_SetError(EAX,EGB_NO_ERROR);
}

void EGB_42H_UNCONNECT(
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
	unsigned char flags=EAX;
	_Far struct EGB_BlockInfo *blkInfo;
	_FP_SEG(blkInfo)=DS;
	_FP_OFF(blkInfo)=ESI;

	_Far struct EGB_Work *work=EGB_GetWork();

	struct EGB_PagePointerSet pointerSet=EGB_GetPagePointerSet(work);

	_Far unsigned short *count;
	_FP_SEG(count)=DS;
	_FP_OFF(count)=ESI;
	if(2<=*count)
	{
		_Far short *points=(_Far short *)(count+1);
		for(i=0; i+1<*count; i+=2)
		{
			struct POINTW p0,p1;
			p0.x=points[i*2];
			p0.y=points[i*2+1];
			p1.x=points[i*2+2];
			p1.y=points[i*2+3];
			if(ClipLine(&p0,&p1,pointerSet.page->viewport[0],pointerSet.page->viewport[1]))
			{
				EGB_DrawLine(work,&pointerSet,p0,p1);
			}
		}
	}

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
	_Far struct EGB_Work *work=EGB_GetWork();

	struct EGB_PagePointerSet ptrSet=EGB_GetPagePointerSet(work);

	_Far short *diagonal;
	_FP_SEG(diagonal)=DS;
	_FP_OFF(diagonal)=ESI;

	struct POINTW p0,p1;
	p0.x=diagonal[0];
	p0.y=diagonal[1];
	p1.x=diagonal[2];
	p1.y=diagonal[3];
	EGB_MakeP0SmallerThanP1(&p0,&p1);

	if(BoxIsOutsideOfViewport(p0.x,p0.y,p1.x,p1.y,ptrSet.page))
	{
		EGB_SetError(EAX,EGB_NO_ERROR);
		return;
	}

	if(work->paintMode&EGB_PAINTFLAG_FILL_HATCH)
	{
		unsigned int vramAddr;
		unsigned int color=GetExpandedColor(work->color[EGB_FILL_COLOR],ptrSet.mode->bitsPerPixel);
		unsigned int xMin=_max(p0.x,ptrSet.page->viewport[0].x);
		unsigned int xMax=_min(p1.x,ptrSet.page->viewport[1].x);
		unsigned int yMin=_max(p0.y,ptrSet.page->viewport[0].y);
		unsigned int yMax=_min(p1.y,ptrSet.page->viewport[1].y);
		_Far unsigned char *hatchLine;
		unsigned int hatchY=0;

		EGB_CalcVRAMAddr(&vramAddr,p0.x,p0.y,ptrSet.mode);
		hatchLine=work->hatch[EGB_FILL_COLOR].ptn;
		for(int y=yMin; y<=yMax; ++y)
		{
			unsigned int hatchPtn,hatchX=0,hatchW=work->hatch[EGB_FILL_COLOR].size.x;
			unsigned int hatchBit,hatchBitReset=0x80;

			switch(hatchW)
			{
			case 1:
				hatchPtn=hatchLine[0];
				hatchPtn<<=8;
				hatchPtn|=hatchLine[1];
				hatchPtn<<=8;
				hatchPtn|=hatchLine[2];
				hatchPtn<<=8;
				hatchPtn|=hatchLine[3];
				hatchBitReset=0x80000000;
				break;
			case 2:
				hatchPtn=hatchLine[0];
				hatchPtn<<=8;
				hatchPtn|=hatchLine[1];
				hatchPtn<<=8;
				hatchPtn|=hatchLine[0];
				hatchPtn<<=8;
				hatchPtn|=hatchLine[1];
				hatchBitReset=0x80000000;
				break;
			case 3:
				hatchPtn|=hatchLine[0];
				hatchPtn<<=8;
				hatchPtn|=hatchLine[1];
				hatchPtn<<=8;
				hatchPtn|=hatchLine[2];
				hatchBitReset=0x800000;
				break;
			}

			hatchBit=hatchBitReset;

			unsigned int nextVramAddr=vramAddr+ptrSet.mode->bytesPerLine;
			switch(ptrSet.mode->bitsPerPixel)
			{
			case 4:
				switch(work->drawingMode)
				{
				case EGB_FUNC_PSET:
				case EGB_FUNC_OPAQUE:
					{
						unsigned int shift=0;
						if(xMin&1)
						{
							if(hatchPtn&hatchBit)
							{
								ptrSet.vram[vramAddr]&=0xF0;
								ptrSet.vram[vramAddr]|=(color&0x0F);
							}
							hatchBit>>=1; // Will never fall below bit 0 for the first pixel
							++vramAddr;
							++xMin;
							shift=4;
						}
						unsigned int count=(xMax+1-xMin);
						for(int i=0; i<count; ++i)
						{
							if(hatchPtn&hatchBit)
							{
								*(ptrSet.vram+vramAddr)&=(0xF0>>shift);
								*(ptrSet.vram+vramAddr)|=(color&(0x0F<<shift));
							}
							if(4==shift)
							{
								++vramAddr;
							}
							shift=4-shift;
							hatchBit>>=1;
							if(0==hatchBit)
							{
								hatchBit=hatchBitReset;
							}
						}
						if(!(xMax&1) && (hatchPtn&hatchBit))
						{
							ptrSet.vram[vramAddr]&=0x0F;
							ptrSet.vram[vramAddr]|=(color&0xF0);
						}
					}
					break;
				default:
					TSUGARU_BREAK;
					break;
				}
				break;
			case 8:
				switch(work->drawingMode)
				{
				case EGB_FUNC_PSET:
				case EGB_FUNC_OPAQUE:
					TSUGARU_BREAK;
					MEMSETB_FAR(ptrSet.vram+vramAddr,color,xMax-xMin+1);
					break;
				default:
					TSUGARU_BREAK;
					break;
				}
				break;
			case 16:
				switch(work->drawingMode)
				{
				case EGB_FUNC_PSET:
				case EGB_FUNC_OPAQUE:
					TSUGARU_BREAK;
					MEMSETW_FAR(ptrSet.vram+vramAddr,color,xMax-xMin+1);
					break;
				default:
					TSUGARU_BREAK;
					break;
				}
				break;
			}
			vramAddr=nextVramAddr;

			++hatchY;
			if(hatchY<work->hatch[EGB_FILL_COLOR].size.y)
			{
				hatchLine+=work->hatch[EGB_FILL_COLOR].size.x;
			}
			else
			{
				hatchLine=work->hatch[EGB_FILL_COLOR].ptn;
				hatchY=0;
			}
		}
	}

	if(work->paintMode&EGB_PAINTFLAG_FILL_NORMAL)
	{
		unsigned int vramAddr;
		unsigned int color=GetExpandedColor(work->color[EGB_FILL_COLOR],ptrSet.mode->bitsPerPixel);
		unsigned int xMin=_max(p0.x,ptrSet.page->viewport[0].x);
		unsigned int xMax=_min(p1.x,ptrSet.page->viewport[1].x);
		unsigned int yMin=_max(p0.y,ptrSet.page->viewport[0].y);
		unsigned int yMax=_min(p1.y,ptrSet.page->viewport[1].y);

		EGB_CalcVRAMAddr(&vramAddr,p0.x,p0.y,ptrSet.mode);
		for(int y=yMin; y<=yMax; ++y)
		{
			unsigned int nextVramAddr=vramAddr+ptrSet.mode->bytesPerLine;
			switch(ptrSet.mode->bitsPerPixel)
			{
			case 4:
				switch(work->drawingMode)
				{
				case EGB_FUNC_PSET:
				case EGB_FUNC_OPAQUE:
					if(xMin&1)
					{
						ptrSet.vram[vramAddr]&=0xF0;
						ptrSet.vram[vramAddr]|=(color&0x0F);
						++vramAddr;
						++xMin;
					}
					{
						unsigned int count=(xMax+1-xMin)/2;
						MEMSETB_FAR(ptrSet.vram+vramAddr,color,count);
						vramAddr+=count;
					}
					if(!(xMax&1))
					{
						ptrSet.vram[vramAddr]&=0x0F;
						ptrSet.vram[vramAddr]|=(color&0xF0);
					}
					break;
				case EGB_FUNC_XOR:
					if(xMin&1)
					{
						ptrSet.vram[vramAddr]^=(color&0x0F);
						++vramAddr;
						++xMin;
					}
					{
						int i;
						unsigned int count=(xMax+1-xMin)/2;
						unsigned int countDiv4=(count>>2),countMod4=(count&3);
						for(i=0; i<countDiv4; ++i)
						{
							*((_Far unsigned int *)(ptrSet.vram+vramAddr))^=color;
							vramAddr+=4;
						}
						for(i=0; i<countMod4; ++i)
						{
							*(ptrSet.vram+vramAddr)^=color;
							++vramAddr;
						}
					}
					if(!(xMax&1))
					{
						ptrSet.vram[vramAddr]^=(color&0xF0);
					}
					break;
				default:
					TSUGARU_BREAK;
					break;
				}
				break;
			case 8:
				switch(work->drawingMode)
				{
				case EGB_FUNC_PSET:
				case EGB_FUNC_OPAQUE:
					MEMSETB_FAR(ptrSet.vram+vramAddr,color,xMax-xMin+1);
					break;
				default:
					TSUGARU_BREAK;
					break;
				}
				break;
			case 16:
				switch(work->drawingMode)
				{
				case EGB_FUNC_PSET:
				case EGB_FUNC_OPAQUE:
					MEMSETW_FAR(ptrSet.vram+vramAddr,color,xMax-xMin+1);
					break;
				default:
					TSUGARU_BREAK;
					break;
				}
				break;
			}
			vramAddr=nextVramAddr;
		}
	}

	if(work->paintMode&EGB_PAINTFLAG_LINE_NORMAL)
	{
		struct POINTW a,b;
		a=p0;
		b.x=p1.x;
		b.y=p0.y;
		EGB_DrawLine(work,&ptrSet,a,b);
		a=b;
		b.y=p1.y;
		EGB_DrawLine(work,&ptrSet,a,b);
		a=b;
		b.x=p0.x;
		EGB_DrawLine(work,&ptrSet,a,b);
		a=b;
		b.y=p0.y;
		EGB_DrawLine(work,&ptrSet,a,b);
	}

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

void EGB_4EH_CLOSEPAINT(
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
	_Far struct EGB_Work *work=EGB_GetWork();
	struct EGB_PagePointerSet ptrSet=EGB_GetPagePointerSet(work);
	_Far struct EGB_CoordAddrSet *fifoBuf=(_Far struct EGB_CoordAddrSet *)EGB_GetBuffer();
	_Far short *startCoord;
	_FP_SEG(startCoord)=DS;
	_FP_OFF(startCoord)=ESI;

	unsigned int fifoBufUsed=0;
	unsigned int fifoBufHead=0;
	unsigned int VRAMAddr;

	if(startCoord[0]<ptrSet.page->viewport[0].x ||ptrSet.page->viewport[1].x<startCoord[0] ||
	   startCoord[1]<ptrSet.page->viewport[0].y ||ptrSet.page->viewport[1].y<startCoord[1])
	{
		// Outside of the viewport. Nothing to do.
		EGB_SetError(EAX,EGB_NO_ERROR);
		return;
	}

	EGB_CalcVRAMAddr(&VRAMAddr,startCoord[0],startCoord[1],ptrSet.mode);

	unsigned short srcColor=0,fillColor=work->color[EGB_FILL_COLOR];
	switch(ptrSet.mode->bitsPerPixel)
	{
	case 4:
		fillColor&=0x0F;
		if(startCoord[0]&1)
		{
			srcColor=(ptrSet.vram[VRAMAddr]>>4);
			ptrSet.vram[VRAMAddr]&=0x0F;
			ptrSet.vram[VRAMAddr]|=(fillColor<<4);
		}
		else
		{
			srcColor=(ptrSet.vram[VRAMAddr]&0x0F);
			ptrSet.vram[VRAMAddr]&=0xF0;
			ptrSet.vram[VRAMAddr]|=fillColor;
		}
		break;
	case 8:
		fillColor&=0xFF;
		srcColor=ptrSet.vram[VRAMAddr];
		ptrSet.vram[VRAMAddr]=fillColor;
		break;
	case 16:
		srcColor=*(_Far unsigned short *)(ptrSet.vram+VRAMAddr);
		*(_Far unsigned short *)(ptrSet.vram+VRAMAddr)=fillColor;
		break;
	}

	if(srcColor==fillColor)
	{
		// Already same color.  Nothing to do.
		EGB_SetError(EAX,EGB_NO_ERROR);
		return;
	}

	fifoBuf[(fifoBufHead+fifoBufUsed)&(EGB_PAINT_FIFOBUF_SIZE-1)].VRAMAddr=VRAMAddr;
	fifoBuf[(fifoBufHead+fifoBufUsed)&(EGB_PAINT_FIFOBUF_SIZE-1)].pos.x=startCoord[0];
	fifoBuf[(fifoBufHead+fifoBufUsed)&(EGB_PAINT_FIFOBUF_SIZE-1)].pos.y=startCoord[1];
	++fifoBufUsed;

	while(0<fifoBufUsed)
	{
		int i;
		struct EGB_CoordAddrSet head=fifoBuf[fifoBufHead];
		--fifoBufUsed;
		fifoBufHead=(fifoBufHead+1)&(EGB_PAINT_FIFOBUF_SIZE-1);
		short bytesPerPixel=ptrSet.mode->bitsPerPixel/8;
		unsigned short cmpColor;

		for(i=0; i<4; ++i)
		{
			struct EGB_CoordAddrSet next=head;
			switch(i)
			{
			case 0:
				--next.pos.x;
				if(4==ptrSet.mode->bitsPerPixel)
				{
					if(next.pos.x&1)
					{
						--next.VRAMAddr;
					}
				}
				else
				{
					next.VRAMAddr-=bytesPerPixel;
				}
				break;
			case 1:
				++next.pos.x;
				if(4==ptrSet.mode->bitsPerPixel)
				{
					if(!(next.pos.x&1))
					{
						++next.VRAMAddr;
					}
				}
				else
				{
					next.VRAMAddr+=bytesPerPixel;
				}
				break;
			case 2:
				--next.pos.y;
				next.VRAMAddr-=ptrSet.mode->bytesPerLine;
				break;
			case 3:
				++next.pos.y;
				next.VRAMAddr+=ptrSet.mode->bytesPerLine;
				break;
			}

			if(next.pos.x<ptrSet.page->viewport[0].x || ptrSet.page->viewport[1].x<next.pos.x ||
			   next.pos.y<ptrSet.page->viewport[0].y || ptrSet.page->viewport[1].y<next.pos.y)
			{
				continue;
			}

			switch(ptrSet.mode->bitsPerPixel)
			{
			case 4:
				if(next.pos.x&1)
				{
					cmpColor=(ptrSet.vram[next.VRAMAddr]>>4);
					if(cmpColor!=srcColor)
					{
						continue;
					}
					ptrSet.vram[next.VRAMAddr]&=0x0F;
					ptrSet.vram[next.VRAMAddr]|=(fillColor<<4);
				}
				else
				{
					cmpColor=(ptrSet.vram[next.VRAMAddr]&0x0F);
					if(cmpColor!=srcColor)
					{
						continue;
					}
					ptrSet.vram[next.VRAMAddr]&=0xF0;
					ptrSet.vram[next.VRAMAddr]|=fillColor;
				}
				break;
			case 8:
				if(srcColor!=ptrSet.vram[next.VRAMAddr])
				{
					continue;
				}
				ptrSet.vram[next.VRAMAddr]=fillColor;
				break;
			case 16:
				if(srcColor!=*(_Far unsigned short *)(ptrSet.vram+next.VRAMAddr))
				{
					continue;
				}
				*(_Far unsigned short *)(ptrSet.vram+next.VRAMAddr)=fillColor;
				break;
			}

			if(fifoBufUsed<EGB_PAINT_FIFOBUF_SIZE)
			{
				fifoBuf[(fifoBufHead+fifoBufUsed)&(EGB_PAINT_FIFOBUF_SIZE-1)]=next;
				++fifoBufUsed;
			}
		}
	}
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



static struct EGB_Work EGB_work;

_Far struct EGB_Work *EGB_GetWork(void)
{
	_Far struct EGB_Work *ptr;
	_FP_SEG(ptr)=SEG_TGBIOS_DATA;
	_FP_OFF(ptr)=(unsigned int)&EGB_work;
	return ptr;
}

static unsigned char buffer[EGB_BUFFER_SIZE];

_Far unsigned char *EGB_GetBuffer(void)
{
	_Far unsigned char *ptr;
	_FP_SEG(ptr)=SEG_TGBIOS_DATA;
	_FP_OFF(ptr)=(unsigned int)buffer;
	return ptr;
}
