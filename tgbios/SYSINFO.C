#include <DOS.H>
#include "SYSINFO.H"
#include "TGBIOS.H"
#include "UTIL.H"
#include "SND.H"
#include "EGB.H"

void SYSINFO_NOP(void)
{
}

void SYSINFO_05H_GET_PALETTE(
	unsigned int EDI,
	unsigned int ESI,
	unsigned int EBP,
	unsigned int ESP,
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
	int numPalettes=0,i;

	unsigned char page=EAX;
	_Far unsigned int *returnPtr;
	_FP_SEG(returnPtr)=DS;
	_FP_OFF(returnPtr)=EDX;

	if(1<page ||
	   EGB_INVALID_SCRNMODE==work->perPage[page].screenMode)
	{
		SET_DWORD(&EAX,-1);
		return;
	}

	_Far struct EGB_ScreenMode *scrnModeProp=EGB_GetScreenModeProp(work->perPage[page].screenMode);
	if(NULL==scrnModeProp)
	{
		SET_DWORD(&EAX,-1);
		return;
	}

	if(8==scrnModeProp->bitsPerPixel)
	{
		numPalettes=256;
		work->sifter[1]|=0x30;
	}
	else if(4==scrnModeProp->bitsPerPixel)
	{
		numPalettes=16;
		if(0==page)
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
		SET_DWORD(&EAX,-1);
		return;
	}

	_outb(TOWNSIO_VIDEO_OUT_CTRL_ADDRESS,1);
	_outb(TOWNSIO_VIDEO_OUT_CTRL_DATA,work->sifter[1]);

	returnPtr[0]=numPalettes;
	for(i=0; i<numPalettes; ++i)
	{
		unsigned char g,r,b;
		_outb(TOWNSIO_ANALOGPALETTE_CODE,i);
		b=_inb(TOWNSIO_ANALOGPALETTE_BLUE);
		r=_inb(TOWNSIO_ANALOGPALETTE_RED);
		g=_inb(TOWNSIO_ANALOGPALETTE_GREEN);

		unsigned rgb0;
		rgb0=g;
		rgb0<<=8;
		rgb0|=r;
		rgb0<<=8;
		rgb0|=b;

		returnPtr[1+i*2]=i;
		returnPtr[2+i*2]=rgb0;
	}

	SET_DWORD(&EAX,0);
}

void SYSINFO_22H_GET_ELEVOL_MUTE(
	unsigned int EDI,
	unsigned int ESI,
	unsigned int EBP,
	unsigned int ESP,
	unsigned int EBX,
	unsigned int EDX,
	unsigned int ECX,
	unsigned int EAX,
	unsigned int DS,
	unsigned int ES,
	unsigned int GS,
	unsigned int FS)
{
	_Far struct SND_Status *info=SND_GetStatus();
	SET_DWORD(&EAX,info->elevol_mute);
}

void SYSINFO_30H_SAVE_INTVEC(
	unsigned int EDI,
	unsigned int ESI,
	unsigned int EBP,
	unsigned int ESP,
	unsigned int EBX,
	unsigned int EDX,
	unsigned int ECX,
	unsigned int EAX,
	unsigned int DS,
	unsigned int ES,
	unsigned int GS,
	unsigned int FS)
{
	_Far struct TBIOS_System_Info *info=SYSINFO_GetStruct();
	unsigned char AL=(unsigned char)EAX;

	if(AL<=4)
	{
		info->INTVec[AL-1]=EDX;
	}
	else if(5==AL)
	{
		info->mouseINTCount=EDX;
	}

	SET_DWORD(&EAX,0);
}

void SYSINFO_31H_RETRIEVE_INTVEC(
	unsigned int EDI,
	unsigned int ESI,
	unsigned int EBP,
	unsigned int ESP,
	unsigned int EBX,
	unsigned int EDX,
	unsigned int ECX,
	unsigned int EAX,
	unsigned int DS,
	unsigned int ES,
	unsigned int GS,
	unsigned int FS)
{
	_Far struct TBIOS_System_Info *info=SYSINFO_GetStruct();
	unsigned char AL=(unsigned char)EAX;

	if(AL<=4)
	{
		SET_DWORD(&EDX,info->INTVec[AL-1]);
	}
	else if(5==AL)
	{
		SET_DWORD(&EDX,info->mouseINTCount);
	}

	SET_DWORD(&EAX,0);
}



// Unless some initial values are given, -RELEXE will place it at the end, and chops off.
static struct TBIOS_System_Info sysInfo=
{
	// Mouse
	0, // mouseINTCount
	// Misc.
	{0,0,0,0},
	// A Duck.
	0
};

_Far struct TBIOS_System_Info *SYSINFO_GetStruct(void)
{
	_Far struct TBIOS_System_Info *ptr;
	_FP_SEG(ptr)=SEG_TGBIOS_DATA;
	_FP_OFF(ptr)=(unsigned int)&sysInfo;
	return ptr;
}
