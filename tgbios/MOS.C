#include <DOS.H>
#include <CONIO.H>
#include "TGBIOS.H"
#include "EGB.H"
#include "MOS.H"
#include "MACRO.H"
#include "IODEF.H"
#include "UTIL.H"

#define MAX_CURSOR_WIDTH 32
#define MAX_CURSOR_HEIGHT 32
#define PORT_NOT_FOUND 0xFF

#define MOS_ACTIVEFLAG_BIOS_STARTED 1
#define MOS_ACTIVEFLAG_HANDLING_INT 2
#define MOS_ACTIVEFLAG_DRAWING 3

struct MOS_Status
{
	unsigned char dispPage;
	unsigned char activeFlag; // b0:Mouse BIOS started  b1:Handling Interrupt  b2:Drawing
	unsigned short showLevel;
	unsigned char swapLR;
	unsigned char port;
	unsigned short color;
	struct POINTW pos,pulseLeftOver;
	struct POINTUW pulsePerPixel;
	struct POINTUW cursorSize,cursorCenter;
	unsigned char PSETPtn[MAX_CURSOR_WIDTH*MAX_CURSOR_HEIGHT/8];
	unsigned char ANDPtn[MAX_CURSOR_WIDTH*MAX_CURSOR_HEIGHT/8];
	unsigned char VRAMBackup[MAX_CURSOR_HEIGHT*MAX_CURSOR_WIDTH*2];
};

static _Far struct MOS_Status *MOS_GetStatus(void);

const unsigned char defCursorPtn[]=
{
0x00,0x00, // 00000000 00000000
0x7F,0xE0, // 01111111 11100000
0x7F,0xC0, // 01111111 11000000
0x7F,0x80, // 01111111 10000000
0x7F,0xC0, // 01111111 11000000
0x7F,0xE0, // 01111111 11100000
0x7F,0xF0, // 01111111 11110000
0x7F,0xF8, // 01111111 11111000
0x7F,0xFC, // 01111111 11111100
0x6F,0xF8, // 01101111 11111000
0x4F,0xF0, // 01000111 11110000
0x03,0xE0, // 00000011 11100000
0x01,0xC0, // 00000001 11000000
0x00,0x80, // 00000000 10000000
0x00,0x00, // 00000000 00000000
0x00,0x00, // 00000000 00000000
};
const unsigned char defCursorANDPtn[]=
{
0x00,0x0F, // 00000000 00001111
0x00,0x0F, // 00000000 00001111
0x00,0x1F, // 00000000 00011111
0x00,0x3F, // 00000000 00111111
0x00,0x1F, // 00000000 00011111
0x00,0x0F, // 00000000 00001111
0x00,0x07, // 00000000 00000111
0x00,0x03, // 00000000 00000011
0x00,0x01, // 00000000 00000001
0x00,0x03, // 00000000 00000011
0x10,0x07, // 00010000 00000111
0x38,0x0F, // 00111000 00001111
0xFC,0x1F, // 11111100 00011111
0xFE,0x3F, // 11111110 00111111
0xFF,0x7F, // 11111111 01111111
0xFF,0xFF, // 11111111 11111111
};

void MOS_SaveVRAM(_Far struct MOS_Status *mos,_Far struct EGB_Work *egb)
{
	unsigned char screenMode=egb->perPage[mos->dispPage&1].screenMode;
	if(EGB_INVALID_SCRNMODE!=screenMode)
	{
		_Far struct EGB_ScreenMode *scrnMode=EGB_GetScreenModeProp(screenMode);
		_Far unsigned char *vram=EGB_GetVRAMPointer(egb,mos->dispPage&1);
		struct EGB_BlockInfo blkInfo;
		blkInfo.data=mos->VRAMBackup;
		blkInfo.p0=mos->pos;
		blkInfo.p1=mos->pos;
		blkInfo.p1.x+=mos->cursorSize.x-1;
		blkInfo.p1.y+=mos->cursorSize.y-1;

		EGB_GETBLOCK_INTERNAL(scrnMode,&blkInfo,vram);
	}
}

void MOS_DrawCursor(_Far struct MOS_Status *mos,_Far struct EGB_Work *egb)
{
	unsigned char screenMode=egb->perPage[mos->dispPage&1].screenMode;
	if(EGB_INVALID_SCRNMODE!=screenMode)
	{
		_Far struct EGB_ScreenMode *scrnMode=EGB_GetScreenModeProp(screenMode);
		_Far unsigned char *vram=EGB_GetVRAMPointer(egb,mos->dispPage&1);

		struct EGB_BlockInfo blkInfo;
		blkInfo.data=mos->ANDPtn;
		blkInfo.p0=mos->pos;
		blkInfo.p1=mos->pos;
		blkInfo.p1.x+=mos->cursorSize.x-1;
		blkInfo.p1.y+=mos->cursorSize.y-1;

		unsigned short color[4]=
		{
			mos->color,0,0,0
		};
		struct POINTW viewport[2];
		viewport[0].x=0;
		viewport[0].y=0;
		viewport[1]=scrnMode->size;
		viewport[1].x--;
		viewport[1].y--;
		EGB_PUTBLOCK1BIT_INTERNAL(scrnMode,&blkInfo,vram,color,viewport,0,EGB_FUNC_AND);

		blkInfo.data=mos->PSETPtn;
		EGB_PUTBLOCK1BIT_INTERNAL(scrnMode,&blkInfo,vram,color,viewport,0,EGB_FUNC_PSET);
	}
}

void MOS_RestoreVRAM(_Far struct MOS_Status *mos,_Far struct EGB_Work *egb)
{
	unsigned char screenMode=egb->perPage[mos->dispPage&1].screenMode;
	if(EGB_INVALID_SCRNMODE!=screenMode)
	{
		_Far struct EGB_ScreenMode *scrnMode=EGB_GetScreenModeProp(screenMode);
		_Far unsigned char *vram=EGB_GetVRAMPointer(egb,mos->dispPage&1);
		struct EGB_BlockInfo blkInfo;
		blkInfo.data=mos->VRAMBackup;
		blkInfo.p0=mos->pos;
		blkInfo.p1=mos->pos;
		blkInfo.p1.x+=mos->cursorSize.x-1;
		blkInfo.p1.y+=mos->cursorSize.y-1;

		unsigned short color[4]=
		{
			mos->color,0,0,0
		};
		struct POINTW viewport[2];
		viewport[0].x=0;
		viewport[0].y=0;
		viewport[1]=scrnMode->size;
		viewport[1].x--;
		viewport[1].y--;
		EGB_PUTBLOCK_INTERNAL(scrnMode,&blkInfo,vram,color,viewport,0,EGB_FUNC_PSET);
	}
}

void MOS_ReadRaw(unsigned char data[4],int port)
{
	// Sequence:
	//   (1) Write COM=1
	//   (2) Read X-High
	//   (3) Write COM=0
	//   (4) Read X-Low
	//   (5) Write COM=1
	//   (6) Read Y-High
	//   (7) Write COM=0
	//   (8) Read Y-Low
	// Question:
	//   How much I should wait between each read?
	//   Prob 10us?

	unsigned short IOinput;
	unsigned char COMOn;
	const unsigned char COMOff=0x0F;

	if(0==port)
	{
		IOinput=TOWNSIO_GAMEPORT_A_INPUT;
		COMOn=0x1F;
	}
	else
	{
		IOinput=TOWNSIO_GAMEPORT_B_INPUT;
		COMOn=0x2F;
	}

	_outb(TOWNSIO_GAMEPORT_OUTPUT,COMOn);

	_WAITxUS(80);	// min TCS1=80us FM TOWNS Technical Databook p.241

	data[0]=_inb(IOinput);

	_WAITxUS(20);	// Another 20us before clearing COM


	_outb(TOWNSIO_GAMEPORT_OUTPUT,COMOff);

	_WAITxUS(40);	// min TCS2 is 40us.

	data[1]=_inb(IOinput);

	_WAITxUS(10);	// Another 10us before setting COM


	_outb(TOWNSIO_GAMEPORT_OUTPUT,COMOn);

	_WAITxUS(40);	// min TCS3 is 40us.

	data[2]=_inb(IOinput);

	_WAITxUS(10);	// Another 10us before clearing COM


	_outb(TOWNSIO_GAMEPORT_OUTPUT,COMOff);

	_WAITxUS(40);	// min TCS3 is 40us.

	data[3]=_inb(IOinput);
}

int MOS_ReadBtnDXDY(char *btn,char *dx,char *dy,int port)
{
	unsigned char data[4];
	MOS_ReadRaw(data,port);

	*dx=(data[0]<<4)|(data[1]&0x0F);
	*dy=(data[2]<<4)|(data[3]&0x0F);
	*btn=(data[0]>>4)&3;

	return 0;
}

int MOS_TestGamePort(int port)
{
	int i;
	for(i=0; i<3; ++i)
	{
		unsigned char data[4];
		MOS_ReadRaw(data,port);

		data[0]&=0x0F;
		data[1]&=0x0F;
		data[2]&=0x0F;
		data[3]&=0x0F;

		if(0==(data[0]|data[1]|data[2]|data[3]))
		{
			// Unless RUN & Select are both held down, all zero must mean mouse.
			return 1;
		}
		else if(data[0]==data[1] && data[1]==data[2] && data[2]==data[3])
		{
			// Unless the user is precisely moving mouse 45 degrees with respect to the mouse axes to make dx=dy=-1, the value should change.
			// If non-zero and equal, it must be game pad.
			return 0;
		}
	}
	// If the user was constantly moving the mouse, neither of the above condition will become true.
	// So, if the above loop exits, most likely it was mouse, but the user was constantly moving it.
	return 1;
}

void MOS_00H_START(
	unsigned int EDI,
	unsigned int ESI,
	unsigned int EBP,
	unsigned int ESP,
	unsigned int EBX,
	unsigned int EDX,
	unsigned int ECX,
	unsigned int EAX,
	unsigned int DS,
	unsigned int ES,
	unsigned int GS,
	unsigned int FS)
{
	_Far struct MOS_Status *stat=MOS_GetStatus();
	MEMSETB_FAR(stat,0,sizeof(struct MOS_Status));

	stat->activeFlag=MOS_ACTIVEFLAG_BIOS_STARTED;
	stat->port=PORT_NOT_FOUND;

	stat->pulsePerPixel.x=8;	// FM TOWNS Technical Databook p.389
	stat->pulsePerPixel.y=8;

	stat->cursorSize.x=16;
	stat->cursorSize.y=16;

	stat->color=0x7FFF;

	_Far unsigned char *ptnSrc;
	_FP_SEG(ptnSrc)=SEG_TGBIOS_CODE;
	_FP_OFF(ptnSrc)=(unsigned int)defCursorPtn;
	MEMCPY_FAR(stat->PSETPtn,ptnSrc,32);
	_FP_OFF(ptnSrc)=(unsigned int)defCursorANDPtn;
	MEMCPY_FAR(stat->ANDPtn,ptnSrc,32);

	if(MOS_TestGamePort(1))
	{
		stat->port=1;
	}
	else if(MOS_TestGamePort(0))
	{
		stat->port=0;
	}
}

void MOS_01H_END(
	unsigned int EDI,
	unsigned int ESI,
	unsigned int EBP,
	unsigned int ESP,
	unsigned int EBX,
	unsigned int EDX,
	unsigned int ECX,
	unsigned int EAX,
	unsigned int DS,
	unsigned int ES,
	unsigned int GS,
	unsigned int FS)
{
	_Far struct MOS_Status *stat=MOS_GetStatus();
	stat->activeFlag&=~MOS_ACTIVEFLAG_BIOS_STARTED;

	TSUGARU_BREAK
}

void MOS_02H_DISP(
	unsigned int EDI,
	unsigned int ESI,
	unsigned int EBP,
	unsigned int ESP,
	unsigned int EBX,
	unsigned int EDX,
	unsigned int ECX,
	unsigned int EAX,
	unsigned int DS,
	unsigned int ES,
	unsigned int GS,
	unsigned int FS)
{
	_Far struct MOS_Status *stat=MOS_GetStatus();
	_Far struct EGB_Work *egb=EGB_GetWork();

	unsigned char AL=EAX;
	unsigned short prevShowLevel=stat->showLevel;

	if(0==AL)
	{
		stat->showLevel=0;
		if(0!=prevShowLevel)
		{
			MOS_RestoreVRAM(stat,egb);
		}
	}
	else if(1==AL)
	{
		stat->showLevel=1;
		if(0==prevShowLevel)
		{
			MOS_SaveVRAM(stat,egb);
			MOS_DrawCursor(stat,egb);
		}
	}
	else if(2==AL)
	{
		--stat->showLevel;
		if(0!=prevShowLevel)
		{
			MOS_RestoreVRAM(stat,egb);
		}
	}
	else if(3==AL)
	{
		++stat->showLevel;
		if(0==prevShowLevel)
		{
			MOS_SaveVRAM(stat,egb);
			MOS_DrawCursor(stat,egb);
		}
	}
}

void MOS_03H_RDPOS(
	unsigned int EDI,
	unsigned int ESI,
	unsigned int EBP,
	unsigned int ESP,
	unsigned int EBX,
	unsigned int EDX,
	unsigned int ECX,
	unsigned int EAX,
	unsigned int DS,
	unsigned int ES,
	unsigned int GS,
	unsigned int FS)
{
	_Far struct MOS_Status *stat=MOS_GetStatus();
	unsigned short IOinput;
	unsigned char btnState;

	if(0==stat->port)
	{
		IOinput=TOWNSIO_GAMEPORT_A_INPUT;
	}
	else
	{
		IOinput=TOWNSIO_GAMEPORT_B_INPUT;
	}

	_outb(TOWNSIO_GAMEPORT_OUTPUT,0x0F); // COM Off, Trigger On
	_WAITxUS(40);	// min TCS3 is 40us.
	btnState=_inb(IOinput);

	btnState>>=4;
	btnState=~btnState;
	btnState&=3;

	SET_SECOND_BYTE(&ECX,btnState);
	SET_LOW_WORD(&EDX,stat->pos.x);
	SET_LOW_WORD(&EBX,stat->pos.y);

	SET_SECOND_BYTE(&EAX,0);
}

void MOS_04H_SETPOS(
	unsigned int EDI,
	unsigned int ESI,
	unsigned int EBP,
	unsigned int ESP,
	unsigned int EBX,
	unsigned int EDX,
	unsigned int ECX,
	unsigned int EAX,
	unsigned int DS,
	unsigned int ES,
	unsigned int GS,
	unsigned int FS)
{
	TSUGARU_BREAK
}

void MOS_05H_RDON(
	unsigned int EDI,
	unsigned int ESI,
	unsigned int EBP,
	unsigned int ESP,
	unsigned int EBX,
	unsigned int EDX,
	unsigned int ECX,
	unsigned int EAX,
	unsigned int DS,
	unsigned int ES,
	unsigned int GS,
	unsigned int FS)
{
	TSUGARU_BREAK
}

void MOS_06H_RDOFF(
	unsigned int EDI,
	unsigned int ESI,
	unsigned int EBP,
	unsigned int ESP,
	unsigned int EBX,
	unsigned int EDX,
	unsigned int ECX,
	unsigned int EAX,
	unsigned int DS,
	unsigned int ES,
	unsigned int GS,
	unsigned int FS)
{
	TSUGARU_BREAK
}

void MOS_07H_HORIZON(
	unsigned int EDI,
	unsigned int ESI,
	unsigned int EBP,
	unsigned int ESP,
	unsigned int EBX,
	unsigned int EDX,
	unsigned int ECX,
	unsigned int EAX,
	unsigned int DS,
	unsigned int ES,
	unsigned int GS,
	unsigned int FS)
{
	TSUGARU_BREAK
}

void MOS_08H_VERTICAL(
	unsigned int EDI,
	unsigned int ESI,
	unsigned int EBP,
	unsigned int ESP,
	unsigned int EBX,
	unsigned int EDX,
	unsigned int ECX,
	unsigned int EAX,
	unsigned int DS,
	unsigned int ES,
	unsigned int GS,
	unsigned int FS)
{
	TSUGARU_BREAK
}

void MOS_09H_TYPE(
	unsigned int EDI,
	unsigned int ESI,
	unsigned int EBP,
	unsigned int ESP,
	unsigned int EBX,
	unsigned int EDX,
	unsigned int ECX,
	unsigned int EAX,
	unsigned int DS,
	unsigned int ES,
	unsigned int GS,
	unsigned int FS)
{
	TSUGARU_BREAK
}

void MOS_0AH_MOTION(
	unsigned int EDI,
	unsigned int ESI,
	unsigned int EBP,
	unsigned int ESP,
	unsigned int EBX,
	unsigned int EDX,
	unsigned int ECX,
	unsigned int EAX,
	unsigned int DS,
	unsigned int ES,
	unsigned int GS,
	unsigned int FS)
{
	TSUGARU_BREAK
}

void MOS_0BH_ENTSUB(
	unsigned int EDI,
	unsigned int ESI,
	unsigned int EBP,
	unsigned int ESP,
	unsigned int EBX,
	unsigned int EDX,
	unsigned int ECX,
	unsigned int EAX,
	unsigned int DS,
	unsigned int ES,
	unsigned int GS,
	unsigned int FS)
{
	TSUGARU_BREAK
}

void MOS_0CH_PULSE(
	unsigned int EDI,
	unsigned int ESI,
	unsigned int EBP,
	unsigned int ESP,
	unsigned int EBX,
	unsigned int EDX,
	unsigned int ECX,
	unsigned int EAX,
	unsigned int DS,
	unsigned int ES,
	unsigned int GS,
	unsigned int FS)
{
	unsigned char DH=EDX>>8;
	unsigned char DL=EDX;
	_Far struct MOS_Status *stat=MOS_GetStatus();

	stat->pulsePerPixel.x=_max(1,DH);
	stat->pulsePerPixel.y=_max(1,DL);

	SET_SECOND_BYTE(&EAX,0);
}

void MOS_0DH_RESOLUTION(
	unsigned int EDI,
	unsigned int ESI,
	unsigned int EBP,
	unsigned int ESP,
	unsigned int EBX,
	unsigned int EDX,
	unsigned int ECX,
	unsigned int EAX,
	unsigned int DS,
	unsigned int ES,
	unsigned int GS,
	unsigned int FS)
{
	TSUGARU_BREAK
}

void MOS_0EH_WRITEPAGE(
	unsigned int EDI,
	unsigned int ESI,
	unsigned int EBP,
	unsigned int ESP,
	unsigned int EBX,
	unsigned int EDX,
	unsigned int ECX,
	unsigned int EAX,
	unsigned int DS,
	unsigned int ES,
	unsigned int GS,
	unsigned int FS)
{
	TSUGARU_BREAK
}

void MOS_0FH_COLOR(
	unsigned int EDI,
	unsigned int ESI,
	unsigned int EBP,
	unsigned int ESP,
	unsigned int EBX,
	unsigned int EDX,
	unsigned int ECX,
	unsigned int EAX,
	unsigned int DS,
	unsigned int ES,
	unsigned int GS,
	unsigned int FS)
{
	TSUGARU_BREAK
}

void MOS_10H_TILEPATTERN(
	unsigned int EDI,
	unsigned int ESI,
	unsigned int EBP,
	unsigned int ESP,
	unsigned int EBX,
	unsigned int EDX,
	unsigned int ECX,
	unsigned int EAX,
	unsigned int DS,
	unsigned int ES,
	unsigned int GS,
	unsigned int FS)
{
	TSUGARU_BREAK
}

void MOS_11H_VIEWHORIZON(
	unsigned int EDI,
	unsigned int ESI,
	unsigned int EBP,
	unsigned int ESP,
	unsigned int EBX,
	unsigned int EDX,
	unsigned int ECX,
	unsigned int EAX,
	unsigned int DS,
	unsigned int ES,
	unsigned int GS,
	unsigned int FS)
{
	TSUGARU_BREAK
}

void MOS_12H_VIEWVERTICAL(
	unsigned int EDI,
	unsigned int ESI,
	unsigned int EBP,
	unsigned int ESP,
	unsigned int EBX,
	unsigned int EDX,
	unsigned int ECX,
	unsigned int EAX,
	unsigned int DS,
	unsigned int ES,
	unsigned int GS,
	unsigned int FS)
{
	TSUGARU_BREAK
}

void MOS_13H_BTNXCHG(
	unsigned int EDI,
	unsigned int ESI,
	unsigned int EBP,
	unsigned int ESP,
	unsigned int EBX,
	unsigned int EDX,
	unsigned int ECX,
	unsigned int EAX,
	unsigned int DS,
	unsigned int ES,
	unsigned int GS,
	unsigned int FS)
{
	_Far struct MOS_Status *stat=MOS_GetStatus();
	stat->swapLR=EAX;
	SET_SECOND_BYTE(&EAX,0);
}

void MOS_14H_ACCELERATION(
	unsigned int EDI,
	unsigned int ESI,
	unsigned int EBP,
	unsigned int ESP,
	unsigned int EBX,
	unsigned int EDX,
	unsigned int ECX,
	unsigned int EAX,
	unsigned int DS,
	unsigned int ES,
	unsigned int GS,
	unsigned int FS)
{
	TSUGARU_BREAK
}

void MOS_INTERVAL(void)
{
	_Far struct MOS_Status *stat=MOS_GetStatus();
	char btn,bx,by;
	short dx,dy,DX,DY;

	MOS_ReadBtnDXDY(&btn,&bx,&by,stat->port);
	dx=bx;
	dy=by;
	// High-C doesn't sign-extend if I copy char to short.  WTF?
	if(128<=dx)
	{
		dx-=256;
	}
	if(128<=dy)
	{
		dy-=256;
	}
	stat->pulseLeftOver.x-=dx;
	stat->pulseLeftOver.y-=dy;

	if(0!=stat->pulsePerPixel.x)
	{
		DX=stat->pulseLeftOver.x/stat->pulsePerPixel.x;
		stat->pulseLeftOver.x-=DX*stat->pulsePerPixel.x;
	}
	if(0!=stat->pulsePerPixel.y)
	{
		DY=stat->pulseLeftOver.y/stat->pulsePerPixel.y;
		stat->pulseLeftOver.y-=DY*stat->pulsePerPixel.y;
	}

	if(DX || DY)
	{
		_Far struct EGB_Work *egb=EGB_GetWork();
		if(0!=stat->showLevel)
		{
			MOS_RestoreVRAM(stat,egb);
		}
		stat->pos.x+=DX;
		stat->pos.y+=DY;
		if(0!=stat->showLevel)
		{
			MOS_SaveVRAM(stat,egb);
			MOS_DrawCursor(stat,egb);
		}
	}
}



unsigned int MOS_GetActive(void)
{
	_Far struct MOS_Status *stat=MOS_GetStatus();
	return stat->activeFlag;
}

unsigned int MOS_GetDisp(void)
{
	_Far struct MOS_Status *stat=MOS_GetStatus();
	unsigned int disp;
	disp=stat->showLevel<<16;
	if(0!=disp)
	{
		disp|=1;
	}
	return disp;
}


static struct MOS_Status status={0};

static _Far struct MOS_Status *MOS_GetStatus(void)
{
	_Far struct MOS_Status *stat;
	_FP_SEG(stat)=SEG_TGBIOS_DATA;
	_FP_OFF(stat)=(unsigned int)&status;
	return stat;
}
