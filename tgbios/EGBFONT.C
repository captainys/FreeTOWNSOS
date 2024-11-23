#include <DOS.H>
#include <CONIO.H>
#include "TGBIOS.H"
#include "EGB.H"
#include "MACRO.H"
#include "IODEF.H"
#include "UTIL.H"

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
	unsigned char AL=EAX;
	_Far struct EGB_Work *work=EGB_GetWork();
	work->fontRotation=AL;
	if(0!=AL)
	{
		TSUGARU_BREAK;
	}
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
	unsigned char AL=EAX;
	_Far struct EGB_Work *work=EGB_GetWork();
	work->stringRotation=AL;
	if(0!=AL)
	{
		TSUGARU_BREAK;
	}
	EGB_SetError(EAX,EGB_NO_ERROR);
}

void EGB_17H_TEXTSPACE(
	unsigned int EDI,
	unsigned int ESI,
	unsigned int EBP,
	unsigned int ESP,
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
	unsigned short DX=EDX;
	work->fontSpacing=DX;
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
	_Far struct EGB_Work *work=EGB_GetWork();
	if(0==(EAX&0xFF))
	{
		work->ankZoom.x=(unsigned short)EDX;
		work->ankZoom.y=(unsigned short)EBX;
	}
	else
	{
		work->kanjiZoom.x=(unsigned short)EDX;
		work->kanjiZoom.y=(unsigned short)EBX;
	}
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
	_Far struct EGB_Work *work=EGB_GetWork();
	work->fontStyle=EDX;
	EGB_SetError(EAX,EGB_NO_ERROR);
}

unsigned short EGB_SJIS2JIS(unsigned short sjis)
{
	unsigned s1=sjis>>8;
	unsigned s2=sjis&0xff;

	unsigned k,t,jis;

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

	jis=((k+0x20)<<8)|(t+0x20);
	if(0x7521<=jis) // 0x7521 and above are Fujitsu original font.
	{
		jis=0x2221;
	}
	return jis;
}

unsigned short EGB_JIS2SJIS(unsigned short jis)
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

			if(sx&1)
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
				unsigned short ptn=*(ptnBase++);
				ptn<<=8;
				if(8<wid)
				{
					ptn|=*(ptnBase++);
				}
				for(x=0; x<wid; ++x)
				{
					// Can I do SHL and use CF in C rather?
					if(ptn&0x8000)
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
				unsigned short ptn=*(ptnBase++);
				ptn<<=8;
				if(8<wid)
				{
					ptn|=*(ptnBase++);
				}
				for(x=0; x<wid; ++x)
				{
					// Can I do SHL and use CF in C rather?
					if(ptn&0x8000)
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
				unsigned short ptn=*(ptnBase++);
				ptn<<=8;
				if(8<wid)
				{
					ptn|=*(ptnBase++);
				}
				for(x=0; x<wid; ++x)
				{
					// Can I do SHL and use CF in C rather?
					if(ptn&0x8000)
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
				}
				vramAddr+=(ptrSet->mode->bytesPerLine-wid*2);
			}
		}
		break;
	}
}

void EGB_PUT_BW(
	_Far struct EGB_Work *work,
	struct EGB_PagePointerSet *ptrSet, // Should be in the SS.
	int x0,int y1,struct POINTUW fontSize,
	_Far unsigned char *ptnBase,int srcw,int srch,unsigned short lineAtY)
{
	unsigned int vramAddr;
	int x1,y0;
	int X,srcX,Y,srcY;
	int balanceX0=fontSize.x,balanceY=fontSize.y;
	int xStart,yStart,srcXStart,srcYStart;
	int xEnd,yEnd;
	unsigned int fontStyle=work->fontStyle;
	unsigned short srcBytesPerLine=(srcw/8);

	int dx=fontSize.x,dy=fontSize.y;

	if(0==fontSize.x || 0==fontSize.y) // Not visible.
	{
		return;
	}
	x1=x0+fontSize.x-1;
	y0=y1-fontSize.y+1;
	if(y1<ptrSet->page->viewport[0].y || // Completely above the viewport
	   ptrSet->page->viewport[1].y<y0 || // Completely below the viewport
	   x1<ptrSet->page->viewport[0].x || // Completely left of the viewport
	   ptrSet->page->viewport[1].x<x0) // Completely right of vhew viewport
	{
		return;
	}

	xStart=x0;
	yStart=y0;
	srcXStart=0;
	srcYStart=0;

	xEnd=_min(x1,ptrSet->page->viewport[1].x);
	yEnd=_min(y1,ptrSet->page->viewport[1].y);

	if(x0<ptrSet->page->viewport[0].x)
	{
		unsigned left=ptrSet->page->viewport[0].x-x0;
		xStart=ptrSet->page->viewport[0].x;
		srcXStart=srcw*left/dx;
		balanceX0=-srcw*(ptrSet->page->viewport[0].x-x0)+dx*(srcXStart+1);
	}
	if(y0<ptrSet->page->viewport[0].y)
	{
		unsigned up=ptrSet->page->viewport[0].y-y0;
		yStart=ptrSet->page->viewport[0].x;
		srcYStart=up*srch/dy;
		balanceY=-srch*(ptrSet->page->viewport[0].y-y0)+dy*(srcYStart+1);
	}


	if(0!=ptrSet->mode->bytesPerLineShift)
	{
		vramAddr=(yStart<<ptrSet->mode->bytesPerLineShift);
		vramAddr+=((xStart*ptrSet->mode->bitsPerPixel)/8);
	}
	else
	{
		vramAddr=((yStart*ptrSet->mode->bytesPerLine+xStart)*ptrSet->mode->bitsPerPixel)>>3;
	}


	ptnBase+=srcBytesPerLine*srcYStart;


	switch(ptrSet->mode->bitsPerPixel)
	{
	case 4:
		{
			unsigned char andPtn,color;

			srcY=srcYStart;
			for(Y=yStart; Y<=yEnd; ++Y)
			{
				int balanceX=balanceX0;
				srcX=srcXStart;

				unsigned short ptn,prevPtn=0;

				ptn=(*ptnBase);
				ptn<<=8;
				if(1<srcBytesPerLine)
				{
					ptn|=*(ptnBase+1);
				}

				ptn<<=srcX;

				if(lineAtY&(1<<srcY))
				{
					ptn=0xFFFF;
				}

				// Need to reset andPtn and color for each line.  Maybe only odd number of pixels per row is visible.
				if(xStart&1)
				{
					andPtn=0x0F;
					color=work->color[EGB_FOREGROUND_COLOR]<<4;
				}
				else
				{
					andPtn=0xF0;
					color=work->color[EGB_FOREGROUND_COLOR];
				}

				unsigned int nextVramAddr=vramAddr+ptrSet->mode->bytesPerLine;

				int realXStart=xStart,realXEnd=xEnd;
				if(work->fontStyle&EGB_FONTSTYLE_ITALIC)
				{
					int offset=4*fontSize.x/srcw;
					offset*=(y1-Y);
					offset/=fontSize.y;

					realXStart+=offset;
					realXEnd+=offset;
					realXEnd=_min(realXEnd,ptrSet->page->viewport[1].x);
					while(0<offset)
					{
						// Žè”²‚«B
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
						--offset;
					}
				}
				for(X=realXStart; X<=realXEnd; ++X)
				{
					// Can I do SHL and use CF in C rather?
					if((ptn&0x8000) || ((prevPtn&0x8000) && (fontStyle&EGB_FONTSTYLE_BOLD)))
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

					balanceX-=srcw;
					while(balanceX<=0)
					{
						prevPtn=ptn;
						ptn<<=1;
						balanceX+=dx;
					}

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
				}
				vramAddr=nextVramAddr;

				balanceY-=srch;
				while(balanceY<=0)
				{
					++srcY;
					ptnBase+=srcBytesPerLine;
					balanceY+=dy;
				}
			}
		}
		break;
	case 8:
		{
			unsigned char color;

			color=work->color[EGB_FOREGROUND_COLOR];

			srcY=srcYStart;
			for(Y=yStart; Y<=yEnd; ++Y)
			{
				int balanceX=balanceX0;
				unsigned short ptn,prevPtn=0;
				ptn=*ptnBase;
				ptn<<=8;

				srcX=srcXStart;

				if(1<srcBytesPerLine)
				{
					ptn|=*(ptnBase+1);
				}
				ptn<<=srcX;

				if(lineAtY&(1<<srcY))
				{
					ptn=0xFFFF;
				}

				unsigned int nextVramAddr=vramAddr+ptrSet->mode->bytesPerLine;

				int realXStart=xStart,realXEnd=xEnd;
				if(work->fontStyle&EGB_FONTSTYLE_ITALIC)
				{
					int offset=4*fontSize.x/srcw;
					offset*=(y1-Y);
					offset/=fontSize.y;

					realXStart+=offset;
					realXEnd+=offset;
					realXEnd=_min(realXEnd,ptrSet->page->viewport[1].x);
					vramAddr+=offset;
				}
				for(X=realXStart; X<=realXEnd; ++X)
				{
					// Can I do SHL and use CF in C rather?
					if(ptn&0x8000 || ((prevPtn&0x8000) && (fontStyle&EGB_FONTSTYLE_BOLD)))
					{
						//switch(work->drawingMode) // May be it is a common property across pages.
						//{
						//case EGB_PSET:
							ptrSet->vram[vramAddr]=color;
						//	break;
						//}
						//
					}
					balanceX-=srcw;
					while(balanceX<=0)
					{
						prevPtn=ptn;
						ptn<<=1;
						balanceX+=dx;
					}
					++vramAddr;
				}
				vramAddr=nextVramAddr;

				balanceY-=srch;
				while(balanceY<=0)
				{
					++srcY;
					ptnBase+=srcBytesPerLine;
					balanceY+=dy;
				}
			}
		}
		break;
	case 16:
		{
			unsigned short color;
			srcY=srcYStart;

			color=work->color[EGB_FOREGROUND_COLOR];

			for(Y=yStart; Y<=yEnd; ++Y)
			{
				int balanceX=balanceX0;
				unsigned short ptn,prevPtn=0;

				srcX=srcXStart;

				ptn=*ptnBase;
				ptn<<=8;
				if(1<srcBytesPerLine)
				{
					ptn|=*(ptnBase+1);
				}
				ptn<<=srcX;

				if(lineAtY&(1<<srcY))
				{
					ptn=0xFFFF;
				}

				unsigned int nextVramAddr=vramAddr+ptrSet->mode->bytesPerLine;

				int realXStart=xStart,realXEnd=xEnd;
				if(work->fontStyle&EGB_FONTSTYLE_ITALIC)
				{
					int offset=4*fontSize.x/srcw;
					offset*=(y1-Y);
					offset/=fontSize.y;

					realXStart+=offset;
					realXEnd+=offset;
					realXEnd=_min(realXEnd,ptrSet->page->viewport[1].x);
					vramAddr+=offset*2;
				}
				for(X=realXStart; X<=realXEnd; ++X)
				{
					// Can I do SHL and use CF in C rather?
					if(ptn&0x8000 || ((prevPtn&0x8000) && (fontStyle&EGB_FONTSTYLE_BOLD)))
					{
						//switch(work->drawingMode) // May be it is a common property across pages.
						//{
						//case EGB_PSET:
							*(_Far unsigned short *)(ptrSet->vram+vramAddr)=color;
						//	break;
						//}
						//
					}

					balanceX-=srcw;
					while(balanceX<=0)
					{
						prevPtn=ptn;
						ptn<<=1;
						balanceX+=dx;
					}

					vramAddr+=2;
				}
				vramAddr=nextVramAddr;

				balanceY-=srch;
				while(balanceY<=0)
				{
					++srcY;
					ptnBase+=srcBytesPerLine;
					balanceY+=dy;
				}
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

static unsigned int DrawText(_Far struct EGB_Work *work,int xx,int yy,int len,_Far unsigned char *str)
{
	struct EGB_PagePointerSet ptrSet;
	struct POINTUW dimension,ankDim,kanjiDim;
	struct POINTW minmax[2];

	ptrSet=EGB_GetPagePointerSet(work);

	kanjiDim=work->kanjiZoom;
	ankDim=work->ankZoom;

	if(0==work->fontSpacing &&
	   16==kanjiDim.x && 16==kanjiDim.y &&
	   8==ankDim.x && 16==ankDim.y)
	{
		dimension.x=len*8;
		dimension.y=16;
	}
	else
	{
		int i;
		dimension.x=0;
		dimension.y=0;
		for(i=0; i<len; ++i)
		{
			if(IS_SJIS_FIRST_BYTE(str[i]))
			{
				dimension.x+=work->fontSpacing+kanjiDim.x;
				dimension.y=_max(dimension.y,kanjiDim.y);
			}
			else
			{
				dimension.x+=work->fontSpacing+ankDim.x;
				dimension.y=_max(dimension.y,ankDim.y);
			}
		}
	}

	minmax[0].x=xx;
	minmax[0].y=yy-dimension.y+1;
	minmax[1].x=xx+dimension.x-1;
	minmax[1].y=yy;


	if(0==work->fontStyle &&
	   16==work->kanjiZoom.x &&
	   16==work->kanjiZoom.y &&
	   8==work->ankZoom.x &&
	   16==work->ankZoom.y &&
	   ptrSet.page->viewport[0].x<=minmax[0].x && minmax[1].x<=ptrSet.page->viewport[1].x &&
	   ptrSet.page->viewport[0].y<=minmax[0].y && minmax[1].y<=ptrSet.page->viewport[1].y)
	{
		int sx=xx;
		int sy=yy;
		int i=0;
		_Far unsigned char *fontROM;

		_FP_SEG(fontROM)=SEG_KANJI_FONT_ROM;
		_FP_OFF(fontROM)=0;

		while(i<len)
		{
			if(IS_SJIS_FIRST_BYTE(str[i]))
			{
				unsigned int ptnAddr;
				SJISPointerToROMAddress(str+i,ptnAddr);

				EGB_PUTX16BW_NOCHECK(work,&ptrSet,sx,sy,fontROM+ptnAddr,16);

				sx+=16;
				i+=2;
			}
			else
			{
				unsigned int ptnAddr=ANK16_FONT_ADDR_BASE+((unsigned short)str[i])*16;

				EGB_PUTX16BW_NOCHECK(work,&ptrSet,sx,sy,fontROM+ptnAddr,8);

				sx+=8;
				++i;
			}
			sx+=work->fontSpacing;
		}
	}
	else
	{
		int sx=xx;
		int sy=yy;
		int i=0;
		_Far unsigned char *fontROM;
		unsigned short lineAtY=0;

		_FP_SEG(fontROM)=SEG_KANJI_FONT_ROM;
		_FP_OFF(fontROM)=0;

		if(work->fontStyle&EGB_FONTSTYLE_UNDERLINE)
		{
			lineAtY=0x8000;
		}
		if(work->fontStyle&EGB_FONTSTYLE_OVERLINE)
		{
			lineAtY|=0x0001;
		}
		if(work->fontStyle&EGB_FONTSTYLE_MIDDLELINE)
		{
			lineAtY|=0x0100;
		}

		while(i<len)
		{
			if(IS_SJIS_FIRST_BYTE(str[i]))
			{
				unsigned int ptnAddr;
				SJISPointerToROMAddress(str+i,ptnAddr);

				EGB_PUT_BW(work,&ptrSet,sx,sy,work->kanjiZoom,fontROM+ptnAddr,16,16,lineAtY);

				sx+=work->kanjiZoom.x;
				i+=2;
			}
			else
			{
				if(ankDim.y<16)
				{
					unsigned int ptnAddr=ANK8_FONT_ADDR_BASE+((unsigned short)str[i])*8;
					EGB_PUT_BW(work,&ptrSet,sx,sy,work->ankZoom,fontROM+ptnAddr,8,8,lineAtY);
				}
				else
				{
					unsigned int ptnAddr=ANK16_FONT_ADDR_BASE+((unsigned short)str[i])*16;
					EGB_PUT_BW(work,&ptrSet,sx,sy,work->ankZoom,fontROM+ptnAddr,8,16,lineAtY);
				}

				sx+=work->ankZoom.x;
				++i;
			}
			sx+=work->fontSpacing;
		}
	}

	return dimension.x;
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
	_Far struct EGB_Work *work=EGB_GetWork();
	_Far struct EGB_String *strInfo;

	_FP_SEG(strInfo)=DS;
	_FP_OFF(strInfo)=ESI;

	EGB_SetError(EAX,EGB_NO_ERROR);

	unsigned wid=DrawText(work,strInfo->x,strInfo->y,strInfo->len,strInfo->str);

	// I thought it's as easy as drawing another text with 1-pixel shift,
	// but that won't work if OPAQUE mode....
	// if(work->fontStyle&EGB_FONTSTYLE_BOLD)...

	struct EGB_PagePointerSet ptrSet;
	ptrSet=EGB_GetPagePointerSet(work);
	ptrSet.page->textX+=wid;
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
