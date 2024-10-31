/* LICENSE>>
Copyright 2020 Soji Yamakawa (CaptainYS, http://www.ysflight.com)

Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.

2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.

3. Neither the name of the copyright holder nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

<< LICENSE */
#include <stdio.h>
#include <string.h>
#include <time.h>
#include "egb.h"
#include "snd.h"
#include "io.h"
#include "DUCK.H"

static char EGB_work[EgbWorkSize];

char buf[32*32*2];

void SetScreenMode(int m1,int m2);

// Test 16-color mode

#define EGB_FOREGROUND_COLOR 0
#define EGB_BACKGROUND_COLOR 1
#define EGB_FILL_COLOR 2
#define EGB_TRANSPARENT_COLOR 3

#define EGB_PSET 0
#define EGB_PRESET 1
#define EGB_OR 2
#define EGB_AND 3
#define EGB_XOR 4
#define EGB_NOT 5
#define EGB_MATTE 6
#define EGB_PASTEL 7
#define EGB_OPAGUE 9
#define EGB_MASKSET 13
#define EGB_MASKRESET 14
#define EGB_MASKNOT 15

void WaitForPad(void)
{
	int status=0xFF;
	while(0x30==(status&0x30))
	{
		SND_joy_in_2(0,&status);
	}
	while(0x30!=(status&0x30))
	{
		SND_joy_in_2(0,&status);
	}
}



void Wait3Sec(void)
{
	unsigned int accum=0;
	clock_t t0=clock();
	while(accum<CLOCKS_PER_SEC*3)
	{
		clock_t t1=clock();
		accum+=t1-t0;
		t0=t1;
	}
}


struct BitmapHeader
{
	_Far char *ptn;
	short x0,y0,x1,y1;
};

void TestGETBLOCK_COLOR(
	int mode,
	int wid,int hei,
	const unsigned char ptn[])
{
	struct BitmapHeader bmp;
	bmp.ptn=ptn;

	EGB_writeMode(EGB_work,mode);
	EGB_paintMode(EGB_work,0x02);

	bmp.x0=0;
	bmp.y0=0;
	bmp.x1=wid-1;
	bmp.y1=hei-1;
	EGB_putBlock(EGB_work,0,(char *)&bmp);

	bmp.x0=wid;
	bmp.y0=0;
	bmp.x1=wid*2-1;
	bmp.y1=hei-1;
	EGB_putBlock(EGB_work,0,(char *)&bmp);

	bmp.x0=0;
	bmp.y0=hei;
	bmp.x1=wid-1;
	bmp.y1=hei*2-1;
	EGB_putBlock(EGB_work,0,(char *)&bmp);

	bmp.x0=wid;
	bmp.y0=hei;
	bmp.x1=wid*2-1;
	bmp.y1=hei*2-1;
	EGB_putBlock(EGB_work,0,(char *)&bmp);


	bmp.ptn=buf;
	bmp.x0=wid/2;
	bmp.y0=hei/2;
	bmp.x1=wid/2+wid-1;
	bmp.y1=hei/2+hei-1;
	EGB_getBlock(EGB_work,(char *)&bmp);

	bmp.x0+=128;
	bmp.y0+=128;
	bmp.x1+=128;
	bmp.y1+=128;

	EGB_putBlock(EGB_work,0,(char *)&bmp);


	bmp.ptn=buf;
	bmp.x0=-wid/2;
	bmp.y0=-hei/2;
	bmp.x1=-wid/2+wid-1;
	bmp.y1=-hei/2+hei-1;
	EGB_getBlock(EGB_work,(char *)&bmp);

	bmp.x0+=96;
	bmp.y0+=96;
	bmp.x1+=96;
	bmp.y1+=96;

	EGB_putBlock(EGB_work,0,(char *)&bmp);
}


void Test4Bit(void)
{
	unsigned int palette[513];
	unsigned char ptn[32*32];

	EGB_resolution(EGB_work,0,3);
	EGB_resolution(EGB_work,1,3);

	EGB_writePage(EGB_work,1);
	EGB_clearScreen(EGB_work);

	EGB_writePage(EGB_work,0);
	EGB_clearScreen(EGB_work);

	MakeDuck4(palette,ptn);

	EGB_palette(EGB_work,0,palette);
	TestGETBLOCK_COLOR(EGB_PSET,duckywid,duckyhei,(unsigned char *)ptn);

	Wait3Sec();
}

void Test8Bit(void)
{
	unsigned int palette[513];
	unsigned char ptn[32*32];

	EGB_resolution(EGB_work,0,12);

	EGB_writePage(EGB_work,0);
	EGB_clearScreen(EGB_work);

	MakeDuck256(palette,ptn);

	EGB_palette(EGB_work,0,palette);
	TestGETBLOCK_COLOR(EGB_PSET,duckywid,duckyhei,(unsigned char *)ptn);

	Wait3Sec();
}

void Test16Bit(void)
{
	EGB_resolution(EGB_work,0,10);
	EGB_resolution(EGB_work,1,10);

	EGB_writePage(EGB_work,1);
	EGB_clearScreen(EGB_work);

	EGB_writePage(EGB_work,0);
	EGB_clearScreen(EGB_work);

	TestGETBLOCK_COLOR(EGB_PSET,duckywid,duckyhei,(unsigned char *)duck16);

	Wait3Sec();
}

void Swap(char *a,char *b)
{
	char c=*a;
	*a=*b;
	*b=c;
}

int main(void)
{
	EGB_init(EGB_work,EgbWorkSize);

	Test4Bit();
	Test8Bit();
	Test16Bit();

	return 0;
}
