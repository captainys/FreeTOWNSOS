#include <DOS.H>
#include <CONIO.H>
#include "TGBIOS.H"
#include "EGB.H"
#include "MACRO.H"
#include "IODEF.H"
#include "UTIL.H"

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
	_Far struct EGB_Work *work,
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
		vramAddr=(((sy-15)*ptrSet->mode->bytesPerLine+sx)*ptrSet->mode->bitsPerPixel)>>3;
	}

	switch(ptrSet->mode->bitsPerPixel)
	{
	case 4:
		{
			unsigned char andPtn,color;

			if(0==(sx&1))
			{
				andPtn=0x0F;
				color=work->color[EGB_FOREGROUND_COLOR]<<4;
			}
			else
			{
				andPtn=0xF0;
				color=work->color[EGB_FOREGROUND_COLOR];
			}

			for(y=0; y<16; ++y)
			{
				unsigned char ptn=*ptnBase;
				for(x=0; x<wid; ++x)
				{
					// Can I do SHL and use CF in C rather?
					if(ptn&0x80)
					{
						//switch(work->drawingMode) // May be it is a common property across pages.
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

			color=work->color[EGB_FOREGROUND_COLOR];

			for(y=0; y<16; ++y)
			{
				unsigned char ptn=*ptnBase;
				for(x=0; x<wid; ++x)
				{
					// Can I do SHL and use CF in C rather?
					if(ptn&0x80)
					{
						//switch(work->drawingMode) // May be it is a common property across pages.
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

			color=work->color[EGB_FOREGROUND_COLOR];

			for(y=0; y<16; ++y)
			{
				unsigned short ptn=*ptnBase;
				for(x=0; x<wid; ++x)
				{
					// Can I do SHL and use CF in C rather?
					if(ptn&0x80)
					{
						//switch(work->drawingMode) // May be it is a common property across pages.
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

				EGB_PUTX16BW_NOCHECK(work,&ptrSet,sx,sy,fontROM+ptnAddr,16);

				sx+=16;
				i+=2;
			}
			else
			{
				unsigned int ptnAddr=ANK16_FONT_ADDR_BASE+((unsigned short)strInfo->str[i])*16;

				EGB_PUTX16BW_NOCHECK(work,&ptrSet,sx,sy,fontROM+ptnAddr,8);

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
