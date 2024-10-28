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
	unsigned char dispPage,screenMode[2];
	unsigned char activeFlag; // b0:Mouse BIOS started  b1:Handling Interrupt  b2:Drawing
	unsigned char acceleration;
	unsigned short showLevel;
	unsigned char swapLR;
	unsigned char port;
	unsigned short color;
	unsigned char btn; // Last known button state.
	struct POINTW pos,pulseLeftOver;
	struct POINTW minPos,maxPos;
	struct POINTUW pulsePerPixel;
	struct POINTUW cursorSize,cursorCenter;
	struct POINTW cursorOrigin;
	unsigned char cursorType; // 0:System  1:Black&White  2:Color
	unsigned char PSETPtn[MAX_CURSOR_WIDTH*MAX_CURSOR_HEIGHT*2]; // Can be a color cursor
	unsigned char ANDPtn[MAX_CURSOR_WIDTH*MAX_CURSOR_HEIGHT/8];
	unsigned char VRAMBackup[MAX_CURSOR_HEIGHT*MAX_CURSOR_WIDTH*2];
};

static _Far struct MOS_Status *MOS_GetStatus(void);

const unsigned char defCursorPtn[]=
{
0x00,0x00, // 00000000 00000000   FM TOWNS Technical Databook p.385
0x7F,0xC0, // 01111111 11000000
0x7F,0x80, // 01111111 10000000
0x7F,0x00, // 01111111 00000000
0x7E,0x00, // 01111110 00000000
0x7F,0x00, // 01111111 00000000
0x7F,0x80, // 01111111 10000000
0x77,0xC0, // 01110111 11000000
0x63,0xE0, // 01100011 11100000
0x41,0xF0, // 01000001 11110000
0x00,0xF8, // 00000000 11111000
0x00,0x7C, // 00000000 01111100
0x00,0x3E, // 00000000 00111110
0x00,0x1c, // 00000000 00011100
0x00,0x08, // 00000000 00001000
0x00,0x00, // 00000000 00000000
};
const unsigned char defCursorANDPtn[]=
{
0x00,0x1F, // 00000000 00011111
0x00,0x1F, // 00000000 00011111
0x00,0x3F, // 00000000 00111111
0x00,0x7F, // 00000000 01111111
0x00,0xFF, // 00000000 11111111
0x00,0x7F, // 00000000 01111111
0x00,0x3F, // 00000000 00111111
0x00,0x1F, // 00000000 00011111
0x80,0x0F, // 00001000 00001111
0x1C,0x07, // 00011100 00000111
0x3E,0x03, // 00111110 00000011
0xFF,0x01, // 11111111 00000001
0xFF,0x80, // 11111111 10000000
0xFF,0xC1, // 11111111 11000001
0xFF,0xE3, // 11111111 11100011
0xFF,0xF7, // 11111111 11110111
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

		unsigned char viewportFlag=1;
		EGB_PUTBLOCK1BIT_INTERNAL(scrnMode,&blkInfo,vram,color,viewport,viewportFlag,EGB_FUNC_AND);

		blkInfo.data=mos->PSETPtn;
		if(2!=mos->cursorType)
		{
			EGB_PUTBLOCK1BIT_INTERNAL(scrnMode,&blkInfo,vram,color,viewport,viewportFlag,EGB_FUNC_PSET);
		}
		else
		{
			EGB_PUTBLOCK_INTERNAL(scrnMode,&blkInfo,vram,color,viewport,viewportFlag,EGB_FUNC_MATTE);
		}
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

		unsigned char viewportFlag=1;
		EGB_PUTBLOCK_INTERNAL(scrnMode,&blkInfo,vram,color,viewport,viewportFlag,EGB_FUNC_PSET);
	}
}

unsigned int MOS_ReadRaw(int port)
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
	unsigned int rawRead=0;

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

	_PUSHFD
	_CLI

	_outb(TOWNSIO_GAMEPORT_OUTPUT,COMOff);

	_WAIT1US;

	_outb(TOWNSIO_GAMEPORT_OUTPUT,COMOn);

	_WAITxUS(80);	// min TCS1=80us FM TOWNS Technical Databook p.241

	rawRead=_inb(IOinput);

	_WAITxUS(20);	// Another 20us before clearing COM


	_outb(TOWNSIO_GAMEPORT_OUTPUT,COMOff);

	_WAITxUS(40);	// min TCS2 is 40us.

	rawRead<<=8;
	rawRead|=_inb(IOinput);

	_WAITxUS(10);	// Another 10us before setting COM


	_outb(TOWNSIO_GAMEPORT_OUTPUT,COMOn);

	_WAITxUS(40);	// min TCS3 is 40us.

	rawRead<<=8;
	rawRead|=_inb(IOinput);

	_WAITxUS(10);	// Another 10us before clearing COM


	_outb(TOWNSIO_GAMEPORT_OUTPUT,COMOff);

	_WAITxUS(40);	// min TCS3 is 40us.

	rawRead<<=8;
	rawRead|=_inb(IOinput);

	_POPFD

	return rawRead;
}

unsigned int MOS_ReadBtnDXDY(int port)
{
	unsigned int raw=MOS_ReadRaw(port);
	unsigned char data[4];

	data[3]=( raw&0x3F );
	data[2]=((raw>>8)&0x3F);
	data[1]=((raw>>16)&0x3F);
	data[0]=((raw>>24)&0x3F);

	unsigned int dx,dy,btn;
	dx=(data[0]<<4)|(data[1]&0x0F);
	dy=(data[2]<<4)|(data[3]&0x0F);
	btn=(~data[0]>>4)&3;

	dx&=0xFF;
	dy&=0xFF;

	return (btn<<16)|(dy<<8)|dx;
}

int MOS_TestGamePort(int port)
{
	int i;
	for(i=0; i<3; ++i)
	{
		unsigned char data[4];
		unsigned int raw=MOS_ReadRaw(port);

		data[3]=( raw&0x0F );
		data[2]=((raw>>8)&0x0F);
		data[1]=((raw>>16)&0x0F);
		data[0]=((raw>>24)&0x0F);

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
	// Undocumented: TOWNS OS assumes Mouse-BIOS Work Area is stored in 110:0058H
	_Far unsigned int *WorkPtr;
	_FP_SEG(WorkPtr)=SEG_TGBIOS_DATA;
	_FP_OFF(WorkPtr)=0x0058;
	WorkPtr[0]=EDI;
	WorkPtr[1]=GS;

	_Far struct MOS_Status *stat=MOS_GetStatus();
	MEMSETB_FAR(stat,0,sizeof(struct MOS_Status));

	stat->screenMode[0]=3;
	stat->screenMode[1]=3;

	stat->minPos.x=0;
	stat->minPos.y=0;
	stat->maxPos.x=639;
	stat->maxPos.y=479;

	stat->activeFlag=MOS_ACTIVEFLAG_BIOS_STARTED;
	stat->acceleration=1;
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

	_Far unsigned int *WorkPtr;
	_FP_SEG(WorkPtr)=SEG_TGBIOS_DATA;
	_FP_OFF(WorkPtr)=0x0058;
	WorkPtr[0]=0;
	WorkPtr[1]=0;
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

	_PUSHFD
	_CLI
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
	_POPFD
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

	SET_SECOND_BYTE(&ECX,stat->btn);
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
	short min=EDX;
	short max=EBX;

	if(max<min)
	{
		short c=min;
		min=max;
		max=c;
	}

	_Far struct MOS_Status *stat=MOS_GetStatus();
	stat->minPos.x=_max(0,min);
	stat->maxPos.x=max;

	if(stat->pos.x<stat->minPos.x || stat->maxPos.x<stat->pos.x)
	{
		_Far struct EGB_Work *egb=EGB_GetWork();
		_PUSHFD
		if(0!=stat->showLevel)
		{
			_CLI
			MOS_RestoreVRAM(stat,egb);
		}
		stat->pos.x=_min(stat->maxPos.x,stat->pos.x);
		stat->pos.x=_max(stat->minPos.x,stat->pos.x);
		if(0!=stat->showLevel)
		{
			MOS_SaveVRAM(stat,egb);
			MOS_DrawCursor(stat,egb);
		}
		_POPFD
	}

	SET_SECOND_BYTE(&EAX,0);
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
	short min=EDX;
	short max=EBX;

	if(max<min)
	{
		short c=min;
		min=max;
		max=c;
	}

	_Far struct MOS_Status *stat=MOS_GetStatus();
	stat->minPos.y=_max(0,min);
	stat->maxPos.y=max;

	if(stat->pos.y<stat->minPos.y || stat->maxPos.y<stat->pos.y)
	{
		_Far struct EGB_Work *egb=EGB_GetWork();
		_PUSHFD
		if(0!=stat->showLevel)
		{
			_CLI
			MOS_RestoreVRAM(stat,egb);
		}
		stat->pos.y=_min(stat->maxPos.y,stat->pos.y);
		stat->pos.y=_max(stat->minPos.y,stat->pos.y);
		if(0!=stat->showLevel)
		{
			MOS_SaveVRAM(stat,egb);
			MOS_DrawCursor(stat,egb);
		}
		_POPFD
	}

	SET_SECOND_BYTE(&EAX,0);
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
	_Far struct MOS_Status *stat=MOS_GetStatus();
	_Far struct EGB_Work *egb=EGB_GetWork();
	_PUSHFD
	if(0!=stat->showLevel)
	{
		_CLI
		MOS_RestoreVRAM(stat,egb);
	}

	unsigned char AL=EAX;
	unsigned char DH=(EDX>>8);
	unsigned char DL=EDX;

	stat->cursorOrigin.x=DH;
	stat->cursorOrigin.y=DL;
	if(128<=DH)
	{
		stat->cursorOrigin.y-=256;
	}
	if(128<=DL)
	{
		stat->cursorOrigin.y-=256;
	}

	if(0==AL)
	{
		stat->cursorType=AL;
		stat->cursorSize.x=16;
		stat->cursorSize.y=16;

		_Far unsigned char *ptnSrc;
		_FP_SEG(ptnSrc)=SEG_TGBIOS_CODE;
		_FP_OFF(ptnSrc)=(unsigned int)defCursorPtn;
		MEMCPY_FAR(stat->PSETPtn,ptnSrc,32);
		_FP_OFF(ptnSrc)=(unsigned int)defCursorANDPtn;
		MEMCPY_FAR(stat->ANDPtn,ptnSrc,32);
	}
	else if(1==AL)
	{
		_Far unsigned char *ptnSrc;
		_FP_SEG(ptnSrc)=DS;
		_FP_OFF(ptnSrc)=ESI;

		stat->cursorType=AL;
		stat->cursorSize.x=(ptnSrc[0]<<3);
		stat->cursorSize.y=ptnSrc[1];

		unsigned int len=ptnSrc[0]*ptnSrc[1];
		MEMCPY_FAR(stat->PSETPtn,ptnSrc+2,len);
		MEMCPY_FAR(stat->ANDPtn,ptnSrc+2+len,len);
	}
	else if(2==AL)
	{
		unsigned char scrnModeID=stat->screenMode[stat->dispPage&1];
		_Far struct EGB_ScreenMode *scrnMode=EGB_GetScreenModeProp(scrnModeID);
		_Far unsigned char *ptnSrc;
		_FP_SEG(ptnSrc)=DS;
		_FP_OFF(ptnSrc)=ESI;

		stat->cursorType=AL;
		stat->cursorSize.x=(ptnSrc[0]<<3);
		stat->cursorSize.y=ptnSrc[1];

		unsigned int PSETLen=ptnSrc[0]*ptnSrc[1],ANDLen=ptnSrc[0]*ptnSrc[1];;
		PSETLen*=scrnMode->bitsPerPixel;  // ptnSrc[0] is already divided by 8.

		MEMCPY_FAR(stat->PSETPtn,ptnSrc+2,PSETLen);
		MEMCPY_FAR(stat->ANDPtn,ptnSrc+2+PSETLen,ANDLen);

		// By default, EGB uses mask.  Which is extremely inefficient.
		// Clear transparent bits here.
		switch(scrnMode->bitsPerPixel)
		{
		case 4:
			{
				int i;
				_Far unsigned char *PSETPtn=stat->PSETPtn;
				_Far unsigned char *ANDPtn=stat->ANDPtn;
				unsigned char bit=0x80;
				for(i=0; i<PSETLen; ++i)
				{
					if(bit&*ANDPtn)
					{
						*PSETPtn&=0xF0;
					}
					bit>>=1;
					if(bit&*ANDPtn)
					{
						*PSETPtn&=0x0F;
					}
					bit>>=1;
					++PSETPtn;
					if(0==bit)
					{
						bit=0x80;
						++ANDPtn;
					}
				}
			}
			break;
		case 8:
			{
				int i;
				_Far unsigned char *PSETPtn=stat->PSETPtn;
				_Far unsigned char *ANDPtn=stat->ANDPtn;
				unsigned char bit=0x80;
				for(i=0; i<PSETLen; ++i)
				{
					if(bit&*ANDPtn)
					{
						*PSETPtn=0;
					}
					bit>>=1;
					++PSETPtn;
					if(0==bit)
					{
						bit=0x80;
						++ANDPtn;
					}
				}
			}
			break;
		case 16:
			{
				int i;
				_Far unsigned char *PSETPtn=stat->PSETPtn;
				_Far unsigned char *ANDPtn=stat->ANDPtn;
				unsigned char bit=0x80;
				for(i=0; i<PSETLen; ++i)
				{
					if(bit&*ANDPtn)
					{
						*(unsigned short *)PSETPtn=0;
					}
					bit>>=1;
					PSETPtn+=2;
					if(0==bit)
					{
						bit=0x80;
						++ANDPtn;
					}
				}
			}
			break;
		}
	}

	if(0!=stat->showLevel)
	{
		MOS_SaveVRAM(stat,egb);
		MOS_DrawCursor(stat,egb);
	}
	_POPFD
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
	unsigned char page=EAX&1;
	unsigned short scrnMode=EDX;

	_Far struct MOS_Status *stat=MOS_GetStatus();
	stat->screenMode[page]=scrnMode;

	SET_SECOND_BYTE(&EAX,0);
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
	_Far struct MOS_Status *stat=MOS_GetStatus();
	unsigned char AL=EAX;

	if(0==AL)
	{
		stat->color=EDX;
	}
	else
	{
		unsigned char scrnModeID=stat->screenMode[stat->dispPage&1];
		_Far struct EGB_ScreenMode *scrnMode=EGB_GetScreenModeProp(scrnModeID);
		switch(scrnMode->bitsPerPixel)
		{
		case 1: // ?? But, p.390 tells it is possible.
		case 4:
			stat->color=((EDX>>7)&1);
			stat->color|=((EDX>>14)&2);
			stat->color|=((EDX>>21)&4);
			stat->color|=((EDX>>28)&8);
			break;
		case 8:
			stat->color=((EDX>>6)&3);
			stat->color|=((EDX>>11)&0x1C);
			stat->color|=((EDX>>16)&0xE0);
			break;
		case 16:
			stat->color=((EDX>>3)&0x1F);
			stat->color|=((EDX>>6)&0x3E0);
			stat->color|=((EDX>>9)&0x7C00);
			stat->color|=((EDX>>16)&0x8000);
			break;
		}
	}

	SET_SECOND_BYTE(&EAX,0);
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
	short dx,dy,DX=0,DY=0;

	unsigned int dxdybtn=MOS_ReadBtnDXDY(stat->port);
	dx=dxdybtn&255;
	dy=(dxdybtn>>8)&255;
	stat->btn=(dxdybtn>>16)&255;

	// High-C doesn't sign-extend if I copy char to short.  WTF?
	if(128<=dx)
	{
		dx-=256;
	}
	if(128<=dy)
	{
		dy-=256;
	}

	if(stat->acceleration)
	{
		const int maxStep=2048;
		if(dx<0)
		{
			dx=-_min(dx*dx,maxStep);
		}
		else
		{
			dx=_min(dx*dx,maxStep);
		}
		if(dy<0)
		{
			dy=-_min(dy*dy,maxStep);
		}
		else
		{
			dy=_min(dy*dy,maxStep);
		}
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
		_PUSHFD

		_Far struct EGB_Work *egb=EGB_GetWork();
		if(0!=stat->showLevel)
		{
			_CLI
			MOS_RestoreVRAM(stat,egb);
		}
		stat->pos.x+=DX;
		stat->pos.y+=DY;


		stat->pos.x=_min(stat->maxPos.x,stat->pos.x);
		stat->pos.y=_min(stat->maxPos.y,stat->pos.y);

		stat->pos.x=_max(stat->minPos.x,stat->pos.x);
		stat->pos.y=_max(stat->minPos.y,stat->pos.y);


		if(0!=stat->showLevel)
		{
			MOS_SaveVRAM(stat,egb);
			MOS_DrawCursor(stat,egb);
		}

		_POPFD
	}

	// Notify mouse to Tsugaru.
	_outb(TOWNSIO_VM_HOST_IF_DATA,stat->pos.x);
	_outb(TOWNSIO_VM_HOST_IF_DATA,stat->pos.x>>8);
	_outb(TOWNSIO_VM_HOST_IF_DATA,stat->pos.y);
	_outb(TOWNSIO_VM_HOST_IF_DATA,stat->pos.y>>8);
	_outb(TOWNSIO_VM_HOST_IF_CMD_STATUS,TOWNS_VMIF_CMD_NOTIFY_MOUSE);
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
