#include <DOS.H>
#include <CONIO.H>
#include "TGBIOS.H"
#include "EGB.H"
#include "MACRO.H"
#include "IODEF.H"
#include "UTIL.H"

void EGB_DrawLine(_Far struct EGB_Work *work,struct EGB_PagePointerSet *ptrSet,struct POINTW p0,struct POINTW p1)
{
	int dx=p1.x-p0.x;
	int dy=p1.y-p0.y;
	unsigned int wid,hei;
	int vx,vy,VRAMStep;

	if(~0!=work->lineStipple)
	{
		TSUGARU_BREAK;
		_PUSH_DS	// Just to make it distinctive
		_POP_DS
	}


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

#define EGB_DO_STIPPLE(stipple,lambda) \
	if((stipple)&0x80000000) \
	{ \
		{lambda}; \
		(stipple)<<=1; \
		(stipple)|=1; \
	} \
	else \
	{ \
		(stipple)<<=1; \
	}

void EGB_DrawLineStipple(_Far struct EGB_Work *work,struct EGB_PagePointerSet *ptrSet,struct POINTW p0,struct POINTW p1,unsigned int *stipplePtr)
{
	int dx=p1.x-p0.x;
	int dy=p1.y-p0.y;
	unsigned int wid,hei;
	int vx,vy,VRAMStep;
	unsigned int stipple=*stipplePtr;

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
					EGB_DO_STIPPLE(stipple,
						{ptrSet->vram[vramAddr]&=andPtn;
						 ptrSet->vram[vramAddr]|=color;});
					vramAddr+=ptrSet->mode->bytesPerLine;
				}
				break;
			case EGB_FUNC_XOR:
				EGB_DO_STIPPLE(stipple,{ptrSet->vram[vramAddr]^=color;});
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
					EGB_DO_STIPPLE(stipple,{ptrSet->vram[vramAddr]=color;});
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
					EGB_DO_STIPPLE(stipple,{*(_Far unsigned short*)(ptrSet->vram+vramAddr)=color;});
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
					EGB_DO_STIPPLE(stipple,
						{ptrSet->vram[vramAddr]&=0x0F;
						 ptrSet->vram[vramAddr]|=(color&0xF0);});
					++vramAddr;
					++xMin;
				}
				{
					unsigned int count=(xMax+1-xMin)/2;
					unsigned char ORTable[4]={0x00,0x0F,0xF0,0xFF};
					while(0<count)
					{
						unsigned char bits,ORPtn,ANDPtn;
						bits=(stipple>>30);
						stipple<<=2;
						stipple|=bits;

						ORPtn=ORTable[bits];
						ANDPtn=~ORPtn;
						ORPtn&=color;
						*(ptrSet->vram+vramAddr)&=ANDPtn;
						*(ptrSet->vram+vramAddr)|=ORPtn;
						++vramAddr;
						--count;
					}
				}
				if(!(xMax&1))
				{
					EGB_DO_STIPPLE(stipple,
						{ptrSet->vram[vramAddr]&=0xF0;
						ptrSet->vram[vramAddr]|=(color&0x0F);});
				}
				break;
			case EGB_FUNC_XOR:
				if(xMin&1)
				{
					EGB_DO_STIPPLE(stipple,{ptrSet->vram[vramAddr]^=(color&0xF0);});
					++vramAddr;
					++xMin;
				}
				{
					unsigned int count=(xMax+1-xMin)/2;
					unsigned char ORTable[4]={0x00,0x0F,0xF0,0xFF};
					while(0<count)
					{
						unsigned char bits,XORPtn;
						bits=(stipple>>30);
						stipple<<=2;
						stipple|=bits;

						XORPtn=ORTable[bits]&color;
						*(ptrSet->vram+vramAddr)^=XORPtn;
						++vramAddr;
						--count;
					}
				}
				if(!(xMax&1))
				{
					EGB_DO_STIPPLE(stipple,{ptrSet->vram[vramAddr]^=(color&0x0F);});
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
				{
					unsigned int count=xMax-xMin+1;
					while(0<count)
					{
						EGB_DO_STIPPLE(stipple,*(ptrSet->vram+vramAddr)=color;);
						++vramAddr;
						--count;
					}
				}
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
				{
					unsigned int count=xMax-xMin+1;
					_Far unsigned short *vram=(_Far unsigned short *)(ptrSet->vram+vramAddr);
					while(0<count)
					{
						EGB_DO_STIPPLE(stipple,(*vram)=color;);
						++vram;
						--count;
					}
				}
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
						EGB_DO_STIPPLE(stipple,
							{VRAM[vramAddr]&=ANDPtn;
							 VRAM[vramAddr]|=ORPtn;});
						break;
					case EGB_FUNC_XOR:
						EGB_DO_STIPPLE(stipple,{VRAM[vramAddr]^=ORPtn;});
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
						EGB_DO_STIPPLE(stipple,{VRAM[vramAddr]=col;});
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
						EGB_DO_STIPPLE(stipple,{*((_Far unsigned short *)(VRAM+vramAddr))=col;});
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
						EGB_DO_STIPPLE(stipple,
							{VRAM[vramAddr]&=ANDPtn;
							 VRAM[vramAddr]|=ORPtn;});
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
						EGB_DO_STIPPLE(stipple,{VRAM[vramAddr]=col;});
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
						EGB_DO_STIPPLE(stipple,{*((_Far unsigned short *)(VRAM+vramAddr))=col;});
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

	*stipplePtr=stipple;
}
