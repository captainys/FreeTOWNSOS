#include <DOS.H>
#include <CONIO.H>
#include "TGBIOS.H"
#include "EGB.H"
#include "MACRO.H"
#include "IODEF.H"
#include "UTIL.H"



void FNT_00H_GETANK(
	unsigned int EDI,
	unsigned int ESI,
	unsigned int EBP,
	unsigned int ESP,
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
	unsigned char DL=EDX;
	unsigned short BL=EBX&0xFF;
	if(0==AL)
	{
		SET_LOW_WORD(&DS,SEG_KANJI_FONT_ROM);
		if(8==DL)
		{
			SET_DWORD(&ESI,ANK8_FONT_ADDR_BASE+BL*8);
		}
		else if(16==DL)
		{
			SET_DWORD(&ESI,ANK16_FONT_ADDR_BASE+BL*16);
		}
	}
	else
	{
		unsigned int addr=0,len=0;

		if(8==DL)
		{
			addr=ANK8_FONT_ADDR_BASE+BL*8;
			len=8;
		}
		else if(16==DL)
		{
			addr=ANK16_FONT_ADDR_BASE+BL*16;
			len=16;
		}

		_Far void *from,*to;
		_FP_SEG(from)=SEG_KANJI_FONT_ROM;
		_FP_OFF(from)=addr;
		_FP_SEG(to)=DS;
		_FP_OFF(to)=ESI;
		MEMCPY_FAR(to,from,len);
	}
	SET_SECOND_BYTE(&EAX,0);
}

void FNT_01H_GETKANJI(
	unsigned int EDI,
	unsigned int ESI,
	unsigned int EBP,
	unsigned int ESP,
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
	unsigned int addr=EGB_JIS_TO_FONTROMINDEX(EBX);
	addr<<=5;
	if(0==AL)
	{
		SET_LOW_WORD(&DS,SEG_KANJI_FONT_ROM);
		SET_DWORD(&ESI,addr);
	}
	else
	{
		_Far void *from,*to;
		_FP_SEG(from)=SEG_KANJI_FONT_ROM;
		_FP_OFF(from)=addr;
		_FP_SEG(to)=DS;
		_FP_OFF(to)=ESI;
		MEMCPY_FAR(to,from,32);
	}
	SET_SECOND_BYTE(&EAX,0);
}


void FNT_02H_SJIS_TO_JIS(
	unsigned int EDI,
	unsigned int ESI,
	unsigned int EBP,
	unsigned int ESP,
	unsigned int EBX,
	unsigned int EDX,
	unsigned int ECX,
	unsigned int EAX,
	unsigned int DS,
	unsigned int ES,
	unsigned int GS,
	unsigned int FS)
{
	// Input BX=SJIS
	// Output BX=JIS, AH=00H
	SET_LOW_WORD(&EBX,EGB_SJIS2JIS(EBX));
	SET_SECOND_BYTE(&EAX,0);
}


void FNT_03H_JIS_TO_SJIS(
	unsigned int EDI,
	unsigned int ESI,
	unsigned int EBP,
	unsigned int ESP,
	unsigned int EBX,
	unsigned int EDX,
	unsigned int ECX,
	unsigned int EAX,
	unsigned int DS,
	unsigned int ES,
	unsigned int GS,
	unsigned int FS)
{
	SET_LOW_WORD(&EBX,EGB_JIS2SJIS(EBX));
	SET_SECOND_BYTE(&EAX,0);
}
