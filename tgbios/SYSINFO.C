#include <DOS.H>
#include "SYSINFO.H"
#include "TGBIOS.H"
#include "UTIL.H"

void SYSINFO_NOP(void)
{
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
	_Far struct TBIOS_System_Info *info=SYSINFO_GetStruct();
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
struct TBIOS_System_Info sysInfo=
{
	// Sound
	0,
	0,
	// Mouse
	0,
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
