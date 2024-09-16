
/*TBIOS Sprite function compatible code*/
/*Write by bcc2528*/

#include <string.h>
#include <dos.h>
#include "MACRO.H"

void SPR_INIT(
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

	/* Clear all sprite registers */
	_outb(0x450, 0);
	_outb(0x452, 0);
	_outb(0x450, 1);
	_outb(0x452, 0);
	_outb(0x450, 2);
	_outb(0x452, 0);
	_outb(0x450, 3);
	_outb(0x452, 0);
	_outb(0x450, 4);
	_outb(0x452, 0);
	_outb(0x450, 5);
	_outb(0x452, 0);
	_outb(0x450, 6);
	_outb(0x452, 0);

	/* Clear Sprite layer frame buffer with 0x8000 */
	_Far unsigned int *vram;
	_FP_SEG(vram) = 0x104;
	_FP_OFF(vram) = 0x20000;

	for(i = 0;i < 65536;i++)
	{
		*vram = 0x80008000;
		vram++;
	}

	/* Clear Sprite RAM */
	_Far unsigned short *spr_ram;
	_FP_SEG(spr_ram) = 0x114;
	_FP_OFF(spr_ram) = 0x0;

	for(i = 0;i < 65536;i++)
	{
		*spr_ram = 0x0;
		spr_ram++;
	}
}

void SPR_DISPLAY(
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
	int i = 1024 - ECX;

	switch(EAX & 0x3)
	{
		case 0: /* Stop sprite */
			_outb(0x450, 0);
			_outb(0x452, (i & 0xff));
			_outb(0x450, 1);
			_outb(0x452, ((i >> 8) & 0x3));
			break;
		case 1: /* Start sprite */
			_outb(0x450, 0);
			_outb(0x452, (i & 0xff));
			_outb(0x450, 1);
			_outb(0x452, 0x80 | ((i >> 8) & 0x3));
			break;
		case 2: /* Wait sprite ready */
			while((_inb(0x44c) & 2)){}
			break;
	}
}

void SPR_DEFINE(
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
	int byte;

	if(EAX & 1) /* 32K Color */
	{
		byte = 512;
	}
	else /* 16 Color */
	{
		byte = 128;
	}

	_movedata(DS, ESI, 0x114, 128 * (ECX & 1023), (EDX & 0xff) * ((EDX >> 8) & 0xff) * byte);
}

void SPR_SETPALETTEBLOCK(
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
	_movedata(DS, ESI, 0x114, 32 * (ECX & 511), (EDX & 0xff) * 32);
}

void SPR_SETPOSITION(
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
	int x1, y1, x2, y2, x3, y3;
	int x_add, y_add;

	_Far unsigned short *spr_ram;
	_FP_SEG(spr_ram) = 0x114;
	_FP_OFF(spr_ram) = 0x0;

	spr_ram += 4 * (ECX & 1023);

	switch(EAX & 3)
	{
		case 0:
			x_add = 16;
			y_add = 16;
			break;
		case 1:
			x_add = 8;
			y_add = 16;
			break;
		case 2:
			x_add = 16;
			y_add = 8;
			break;
		case 3:
			x_add = 8;
			y_add = 8;
			break;
	}

	x3 = (EDX >> 8) & 0xff;
	y3 = EDX & 0xff;

	y2 = (EDI & 0xffff);
	for(y1 = 0; y1 < y3; y1++)
	{
		x2 = (ESI & 0xffff);
		for(x1 = 0; x1 < x3; x1++)
		{
			*spr_ram = x2;
			spr_ram++;
			*spr_ram = y2;
			spr_ram += 3;
			x2 += x_add;
		}
		y2 += y_add;
	}
}

void SPR_SETATTRIBUTE(
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
	int i, y;

	_Far unsigned short *spr_ram;
	_FP_SEG(spr_ram) = 0x114;
	_FP_OFF(spr_ram) = 0x0;

	spr_ram += (4 * (ECX & 1023)) + 2;

	y = ((EDX >> 8) & 0xff) * (EDX & 0xff);
	for(i = 0; i < y; i++)
	{
		*spr_ram = (ESI & 0xffff);
		spr_ram++;
		*spr_ram = (EDI & 0xffff);
		spr_ram += 3;
	}
}

void SPR_SETMOTION(
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
	int i, y;

	_Far unsigned short *spr_ram;
	_FP_SEG(spr_ram) = 0x114;
	_FP_OFF(spr_ram) = 0x0;

	spr_ram += (4 * (ECX & 1023));

	y = ((EDX >> 8) & 0xff) * (EDX & 0xff);
	for(i = 0; i < y; i++)
	{
		*spr_ram += (ESI & 0xffff);
		spr_ram++;
		*spr_ram += (EDI & 0xffff);
		spr_ram += 3;
	}
}

void SPR_SETOFFSET(
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
	_outb(0x450, 2);
	_outb(0x452, (ESI & 0xff));
	_outb(0x450, 3);
	_outb(0x452, ((ESI >> 8) & 0x1));
	_outb(0x450, 4);
	_outb(0x452, (EDI & 0xff));
	_outb(0x450, 5);
	_outb(0x452, ((EDI >> 8) & 0x1));
}

void SPR_READATTRIBUTE(
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
	_movedata(0x114, 8 * (ECX & 1023), DS, ESI, (EDX & 0xff) * ((EDX >> 8) & 0xff) * 8);
}