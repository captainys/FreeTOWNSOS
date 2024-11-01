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

#define TSUGARU_BREAK _inline(0xE6,0xEA);

static char EGB_work[EgbWorkSize];

void SetScreenMode(int m1,int m2);



// 4-bit Mode   PSET  putBlock 1bit
//              PSET  putBlock 1bit with viewport
//              PSET  putBlock Color
//              PSET  putBlock color with viewport
// 4-bit Mode   MATTE putBlock 1bit
//              MATTE putBlock 1bit with viewport
//              MATTE putBlock Color
//              MATTE putBlock color with viewport

// 8-bit Mode   PSET  putBlock 1bit
//              PSET  putBlock 1bit with viewport
//              PSET  putBlock Color
//              PSET  putBlock color with viewport
// 8-bit Mode   MATTE putBlock 1bit
//              MATTE putBlock 1bit with viewport
//              MATTE putBlock Color
//              MATTE putBlock color with viewport

// 16-bit Mode  PSET  putBlock 1bit
//              PSET  putBlock 1bit with viewport
//              PSET  putBlock Color
//              PSET  putBlock color with viewport
// 16-bit Mode  MATTE putBlock 1bit
//              MATTE putBlock 1bit with viewport
//              MATTE putBlock Color
//              MATTE putBlock color with viewport

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
#define EGB_OPAQUE 9
#define EGB_MASKSET 13
#define EGB_MASKRESET 14
#define EGB_MASKNOT 15

static unsigned int drawingMode[]={EGB_PSET,EGB_OPAQUE,EGB_MATTE};
#define TEST_DRAWING_MODES (sizeof(drawingMode)/sizeof(drawingMode[0]))
#define SAMPLES_PER_SCRNMODE ((TEST_DRAWING_MODES)*4)
#define BYTES_PER_SAMPLE4 (64*64/2)
#define BYTES_PER_SAMPLE8 (64*64)
#define BYTES_PER_SAMPLE16 (64*64*2)

unsigned char sample4[SAMPLES_PER_SCRNMODE*BYTES_PER_SAMPLE4];
unsigned char sample8[SAMPLES_PER_SCRNMODE*BYTES_PER_SAMPLE8];
unsigned char sample16[SAMPLES_PER_SCRNMODE*BYTES_PER_SAMPLE16];

unsigned char compare4[SAMPLES_PER_SCRNMODE*BYTES_PER_SAMPLE4];
unsigned char compare8[SAMPLES_PER_SCRNMODE*BYTES_PER_SAMPLE8];
unsigned char compare16[SAMPLES_PER_SCRNMODE*BYTES_PER_SAMPLE16];

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



void Wait500ms(void)
{
	unsigned int accum=0;
	clock_t t0=clock();
	while(accum*2<CLOCKS_PER_SEC)
	{
		clock_t t1=clock();
		accum+=t1-t0;
		t0=t1;
	}
}


char AOMORI[]=
{
0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFC,0x3F,0xFF,0xFF,
0xFF,0xFF,0xFF,0xFF,0xFC,0x1F,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFC,0x0F,0xFF,0xFF,
0xFF,0xFF,0xFF,0xFF,0xFC,0x03,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFC,0x01,0xFF,0xFF,
0xFF,0xFF,0xFF,0xFF,0xFC,0x00,0x6F,0xFF,0xFF,0xFF,0xFF,0xFF,0xFE,0x00,0x22,0x1F,
0xFF,0xFF,0xFF,0xFF,0xFE,0x00,0x00,0x0F,0xFF,0xFF,0xFF,0x0F,0xFE,0x00,0x00,0x0F,
0xE1,0xFF,0x7E,0x00,0xC0,0x00,0x00,0x03,0xE1,0x80,0x30,0x00,0x40,0x00,0x00,0x01,
0xE0,0x00,0x00,0x00,0x00,0x00,0x00,0x01,0xE0,0x00,0x00,0x00,0x00,0x00,0x00,0x03,
0xE0,0x00,0x00,0x00,0x00,0x00,0x00,0x07,0xE0,0x00,0x00,0x00,0x00,0x00,0x00,0x7F,
0xA0,0x00,0x00,0x00,0x00,0x00,0x00,0x7F,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xFF,
0x80,0x00,0x00,0x00,0x00,0x00,0x00,0xFF,0xC0,0x00,0x00,0x00,0x00,0x00,0x01,0xFF,
0xE0,0x00,0x00,0x00,0x00,0x00,0x01,0xFF,0xF0,0x00,0x00,0x00,0x00,0x00,0x01,0xFF,
0xF8,0x00,0x00,0x00,0x00,0x00,0x01,0xFF,0xF8,0x00,0x00,0x00,0x00,0x00,0x03,0xFF,
0xFE,0xE0,0x00,0x00,0x00,0x00,0x03,0xFF,0xFF,0xF8,0x00,0x00,0x00,0x00,0x03,0xFF,
0xFF,0xFC,0x00,0x00,0x00,0x00,0x03,0xFF,0xFF,0xFE,0x00,0x00,0x00,0x00,0x03,0xFF,
0xFF,0xFE,0x00,0x03,0xC0,0x00,0x03,0xFF,0xFF,0xFE,0x00,0x07,0xC0,0x18,0x03,0xFF,
0xFF,0xFE,0x00,0x07,0xE0,0x3C,0x07,0xFF,0xFF,0xFF,0x00,0x07,0xC0,0xFC,0x07,0xFF,
0xFF,0xFF,0x00,0x0F,0xC1,0xFE,0x07,0xFF,0xFF,0xFF,0x00,0x0F,0xE1,0xFE,0x07,0xFF,
0xFF,0xFF,0x00,0x0F,0xE7,0xFE,0x07,0xFF,0xFF,0xFF,0x00,0x0F,0xFF,0xFE,0x07,0xFF,
0xFF,0xFF,0x00,0x0F,0xFF,0xFF,0x03,0xFF,0xFF,0xFF,0x00,0x0F,0xFF,0xFF,0x03,0xFF,
0xFF,0xFC,0x00,0x0F,0xFF,0xFF,0x03,0xFF,0xFF,0xFC,0x00,0x0E,0x3F,0xFF,0x83,0xFF,
0xFF,0xFF,0x00,0x0E,0x0F,0xFF,0x83,0xFF,0xFF,0xFF,0x80,0x0E,0x00,0x3F,0x83,0xFF,
0xFF,0xFF,0x8E,0x1E,0x00,0x0F,0x03,0xFF,0xFF,0xFF,0x8F,0x7F,0x00,0x07,0x03,0xFF,
0xFF,0xFF,0xBF,0xFF,0x00,0x06,0x03,0xFF,0xFF,0xFF,0xFF,0xFF,0x00,0x00,0x01,0xFF,
0xFF,0xFF,0xFF,0xFF,0x80,0x00,0x01,0xFF,0x9B,0x5E,0xD7,0x7F,0x80,0x00,0x01,0xFF,
0x6B,0x5E,0xD6,0x7F,0x80,0x00,0x01,0xFF,0xE8,0x46,0x15,0x7F,0x80,0x00,0xF0,0xFF,
0xEB,0x5A,0xD5,0x7F,0xC0,0x03,0xFC,0xFF,0xEB,0x5A,0xD3,0x7F,0xC0,0x07,0xFE,0xFF,
0xEC,0xC7,0x37,0x7F,0xE0,0x0F,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xF0,0x1F,0xFF,0xFF,
0x6C,0xDD,0x9B,0x5F,0xF1,0xFF,0xFF,0xFF,0x6B,0x5D,0x6B,0x5F,0xF3,0xFF,0xFF,0xFF,
0x0B,0x5D,0x68,0xDF,0xF7,0xFF,0xFF,0xFF,0x6B,0x55,0x6B,0x5F,0xFF,0xFF,0xFF,0xFF,
0x6B,0x49,0x6B,0x5F,0xFF,0xFF,0xFF,0xFF,0x9C,0xDD,0x98,0xDF,0xFF,0xFF,0xFF,0xFF,
};

struct BitmapHeader
{
	_Far char *ptn;
	short x0,y0,x1,y1;
};

void DrawBackground(unsigned int lineColor)
{
	short x,y;
	short viewport[4]={0,0,320,240};
	EGB_viewport(EGB_work,viewport);

	EGB_color(EGB_work,EGB_FOREGROUND_COLOR,lineColor);
	EGB_writeMode(EGB_work,EGB_PSET);
	EGB_paintMode(EGB_work,2);
	for(x=0; x<320; x+=5)
	{
		short line[5]={2,x,0,x,240};
		EGB_connect(EGB_work,line);
	}
	for(y=0; y<240; y+=5)
	{
		short line[5]={2,0,y,320,y};
		EGB_unconnect(EGB_work,line);
	}
}

void TestPUTBLOCK_1BIT(
	unsigned int fgColor,
	unsigned int bgColor,
	unsigned int lineColor,
	int mode)
{
	int i;
	struct BitmapHeader bmp;

	DrawBackground(lineColor);

	bmp.ptn=AOMORI;
	bmp.x0=0;
	bmp.y0=0;
	bmp.x1=63;
	bmp.y1=63;

	EGB_color(EGB_work,EGB_FOREGROUND_COLOR,fgColor);
	EGB_color(EGB_work,EGB_BACKGROUND_COLOR,bgColor);
	EGB_writeMode(EGB_work,mode);
	EGB_paintMode(EGB_work,0x02);
	for(i=0; i<16; ++i)
	{
		EGB_putBlockColor(EGB_work,0,(char *)&bmp);
		bmp.x0++;
		bmp.y0++;
		bmp.x1++;
		bmp.y1++;
	}
}

void TestPUTBLOCK_1BIT_VIEWPORT(
	unsigned int fgColor,
	unsigned int bgColor,
	unsigned int lineColor,
	int mode)
{
	short viewport[4]={5,5,50,50};

	DrawBackground(lineColor);

	EGB_color(EGB_work,EGB_FOREGROUND_COLOR,fgColor);
	EGB_color(EGB_work,EGB_BACKGROUND_COLOR,bgColor);
	EGB_writeMode(EGB_work,mode);
	EGB_paintMode(EGB_work,0x02);
	EGB_viewport(EGB_work,viewport);

	struct BitmapHeader bmp;
	bmp.ptn=AOMORI;
	bmp.x0=0;
	bmp.y0=0;
	bmp.x1=63;
	bmp.y1=63;

	for(int i=0; i<16; ++i)
	{
		EGB_putBlockColor(EGB_work,1,(char *)&bmp);
		bmp.x0++;
		bmp.y0++;
		bmp.x1++;
		bmp.y1++;
	}
}

void TestPUTBLOCK_COLOR(
	int mode,
	unsigned int lineColor,
	int wid,int hei,
	const unsigned char ptn[])
{
	struct BitmapHeader bmp;

	DrawBackground(lineColor);

	bmp.ptn=ptn;
	bmp.x0=0;
	bmp.y0=0;
	bmp.x1=wid-1;
	bmp.y1=hei-1;

	EGB_writeMode(EGB_work,mode);
	EGB_paintMode(EGB_work,0x02);

	for(int i=0; i<48; ++i)
	{
		EGB_putBlock(EGB_work,0,(char *)&bmp);
		bmp.x0++;
		bmp.y0++;
		bmp.x1++;
		bmp.y1++;
	}
}

void TestPUTBLOCK_COLOR_VIEWPORT(
	int mode,
	unsigned int lineColor,
	int wid,int hei,
	const unsigned char ptn[])
{
	short viewport[4]={5,5,60,60};

	DrawBackground(lineColor);

	EGB_writeMode(EGB_work,mode);
	EGB_paintMode(EGB_work,0x02);
	EGB_viewport(EGB_work,viewport);

	struct BitmapHeader bmp;
	bmp.ptn=ptn;
	bmp.x0=0;
	bmp.y0=0;
	bmp.x1=wid-1;
	bmp.y1=hei-1;

	for(int i=0; i<48; ++i)
	{
		EGB_putBlock(EGB_work,1,(char *)&bmp);
		bmp.x0++;
		bmp.y0++;
		bmp.x1++;
		bmp.y1++;
	}
}

struct EGB_String80
{
	short x,y;
	unsigned short len;
	char str[80];
};

struct EGB_String80 egbStr;

void Test4Bit(void)
{
	int i;
	unsigned int palette[513];
	unsigned char ptn[32*32];
	unsigned char *samplePtr=sample4;

	struct BitmapHeader bmp;
	bmp.x0=0;
	bmp.y0=0;
	bmp.x1=63;
	bmp.y1=63;

	EGB_resolution(EGB_work,0,3);
	EGB_resolution(EGB_work,1,3);

	EGB_writePage(EGB_work,1);
	EGB_clearScreen(EGB_work);

	EGB_writePage(EGB_work,0);

	for(i=0; i<TEST_DRAWING_MODES; ++i)
	{
		EGB_clearScreen(EGB_work);
		TestPUTBLOCK_1BIT(15,0,9,drawingMode[i]);

		bmp.ptn=samplePtr;
		EGB_getBlock(EGB_work,&bmp);
		samplePtr+=BYTES_PER_SAMPLE4;

		Wait500ms();
	}

	for(i=0; i<TEST_DRAWING_MODES; ++i)
	{
		EGB_clearScreen(EGB_work);
		TestPUTBLOCK_1BIT_VIEWPORT(12,0,9,drawingMode[i]);

		bmp.ptn=samplePtr;
		EGB_getBlock(EGB_work,&bmp);
		samplePtr+=BYTES_PER_SAMPLE4;

		Wait500ms();
	}

	MakeDuck4(palette,ptn);
	EGB_palette(EGB_work,0,palette);

	for(i=0; i<TEST_DRAWING_MODES; ++i)
	{
		EGB_clearScreen(EGB_work);
		TestPUTBLOCK_COLOR(drawingMode[i],9,duckywid,duckyhei,(unsigned char *)ptn);

		bmp.ptn=samplePtr;
		EGB_getBlock(EGB_work,&bmp);
		samplePtr+=BYTES_PER_SAMPLE4;

		Wait500ms();
	}

	for(i=0; i<TEST_DRAWING_MODES; ++i)
	{
		EGB_clearScreen(EGB_work);
		TestPUTBLOCK_COLOR_VIEWPORT(drawingMode[i],9,duckywid,duckyhei,(unsigned char *)ptn);

		bmp.ptn=samplePtr;
		EGB_getBlock(EGB_work,&bmp);
		samplePtr+=BYTES_PER_SAMPLE4;

		Wait500ms();
	}
}

void Test8Bit(void)
{
	int i;
	unsigned int palette[513];
	unsigned char ptn[32*32];
	unsigned char *samplePtr=sample8;

	struct BitmapHeader bmp;
	bmp.x0=0;
	bmp.y0=0;
	bmp.x1=63;
	bmp.y1=63;

	EGB_resolution(EGB_work,0,12);
	EGB_writePage(EGB_work,0);

	for(i=0; i<TEST_DRAWING_MODES; ++i)
	{
		EGB_clearScreen(EGB_work);
		TestPUTBLOCK_1BIT(255,0,31,drawingMode[i]);

		bmp.ptn=samplePtr;
		EGB_getBlock(EGB_work,&bmp);
		samplePtr+=BYTES_PER_SAMPLE8;

		Wait500ms();
	}

	for(i=0; i<TEST_DRAWING_MODES; ++i)
	{
		EGB_clearScreen(EGB_work);
		TestPUTBLOCK_1BIT_VIEWPORT(63,0,31,drawingMode[i]);

		bmp.ptn=samplePtr;
		EGB_getBlock(EGB_work,&bmp);
		samplePtr+=BYTES_PER_SAMPLE8;

		Wait500ms();
	}

	MakeDuck256(palette,ptn);
	EGB_palette(EGB_work,0,palette);

	for(i=0; i<TEST_DRAWING_MODES; ++i)
	{
		EGB_clearScreen(EGB_work);
		TestPUTBLOCK_COLOR(drawingMode[i],31,duckywid,duckyhei,(unsigned char *)ptn);

		bmp.ptn=samplePtr;
		EGB_getBlock(EGB_work,&bmp);
		samplePtr+=BYTES_PER_SAMPLE8;

		Wait500ms();
	}

	for(i=0; i<TEST_DRAWING_MODES; ++i)
	{
		EGB_clearScreen(EGB_work);
		TestPUTBLOCK_COLOR_VIEWPORT(drawingMode[i],31,duckywid,duckyhei,(unsigned char *)ptn);
		bmp.ptn=samplePtr;
		EGB_getBlock(EGB_work,&bmp);
		samplePtr+=BYTES_PER_SAMPLE8;

		Wait500ms();
	}
}

void Test16Bit(void)
{
	int i;
	unsigned char *samplePtr=sample16;

	struct BitmapHeader bmp;
	bmp.x0=0;
	bmp.y0=0;
	bmp.x1=63;
	bmp.y1=63;

	EGB_resolution(EGB_work,0,10);
	EGB_resolution(EGB_work,1,10);

	EGB_writePage(EGB_work,1);
	EGB_clearScreen(EGB_work);

	EGB_writePage(EGB_work,0);

	for(i=0; i<TEST_DRAWING_MODES; ++i)
	{
		EGB_clearScreen(EGB_work);
		TestPUTBLOCK_1BIT(32767,0x8000,31,drawingMode[i]);

		bmp.ptn=samplePtr;
		EGB_getBlock(EGB_work,&bmp);
		samplePtr+=BYTES_PER_SAMPLE16;

		Wait500ms();
	}

	for(i=0; i<TEST_DRAWING_MODES; ++i)
	{
		EGB_clearScreen(EGB_work);
		TestPUTBLOCK_1BIT_VIEWPORT(0x7C00,0x8000,31,drawingMode[i]);

		bmp.ptn=samplePtr;
		EGB_getBlock(EGB_work,&bmp);
		samplePtr+=BYTES_PER_SAMPLE16;

		Wait500ms();
	}

	for(i=0; i<TEST_DRAWING_MODES; ++i)
	{
		EGB_clearScreen(EGB_work);
		TestPUTBLOCK_COLOR(drawingMode[i],31,duckywid,duckyhei,(unsigned char *)duck16);

		bmp.ptn=samplePtr;
		EGB_getBlock(EGB_work,&bmp);
		samplePtr+=BYTES_PER_SAMPLE16;

		Wait500ms();
	}

	for(i=0; i<TEST_DRAWING_MODES; ++i)
	{
		EGB_clearScreen(EGB_work);
		TestPUTBLOCK_COLOR_VIEWPORT(drawingMode[i],31,duckywid,duckyhei,(unsigned char *)duck16);
		bmp.ptn=samplePtr;
		EGB_getBlock(EGB_work,&bmp);
		samplePtr+=BYTES_PER_SAMPLE16;

		Wait500ms();
	}
}

void Swap(char *a,char *b)
{
	char c=*a;
	*a=*b;
	*b=c;
}

int memcmp32(void *P1,void *P2,size_t sz)
{
	unsigned char *p1=P1;
	unsigned char *p2=P2;
	while(0<sz)
	{
		int cmp;
		size_t s;
		s=_min(0xFFFF,sz);
		cmp=memcmp(p1,p2,s);
		if(0!=cmp)
		{
			return cmp;
		}
		p1+=s;
		p2+=s;
		sz-=s;
	}
	return 0;
}

int main(int ac,char *av[])
{
	int generateMode=0;
	if(2<=ac && (0==strcmp(av[1],"-g") || 0==strcmp(av[1],"-G")))
	{
		generateMode=1;
	}


	if(0==generateMode)
	{
		FILE *fp=fopen("EGB_PUT.bin","rb");
		if(NULL==fp)
		{
			fp=fopen("../EGB_PUT.bin","rb");
		}
		if(NULL==fp)
		{
			printf("Cannot open reference binary.\n");
			return 1;
		}
		fread(compare4,1,sizeof(compare4),fp);
		fread(compare8,1,sizeof(compare8),fp);
		fread(compare16,1,sizeof(compare16),fp);
		fclose(fp);
	}


	EGB_init(EGB_work,EgbWorkSize);

	for(int y=0; y<32; ++y)
	{
		Swap(&AOMORI[y*8  ],&AOMORI[(63-y)*8  ]);
		Swap(&AOMORI[y*8+1],&AOMORI[(63-y)*8+1]);
		Swap(&AOMORI[y*8+2],&AOMORI[(63-y)*8+2]);
		Swap(&AOMORI[y*8+3],&AOMORI[(63-y)*8+3]);
		Swap(&AOMORI[y*8+4],&AOMORI[(63-y)*8+4]);
		Swap(&AOMORI[y*8+5],&AOMORI[(63-y)*8+5]);
		Swap(&AOMORI[y*8+6],&AOMORI[(63-y)*8+6]);
		Swap(&AOMORI[y*8+7],&AOMORI[(63-y)*8+7]);
	}

	Test4Bit();
	Test8Bit();
	Test16Bit();

	int err=0;
	if(generateMode)
	{
		FILE *fp=fopen("EGB_PUT.bin","wb");
		fwrite(sample4,1,sizeof(sample4),fp);
		fwrite(sample8,1,sizeof(sample8),fp);
		fwrite(sample16,1,sizeof(sample16),fp);
		fclose(fp);
	}
	else
	{
		if(0!=memcmp32(sample4,compare4,sizeof(sample4)))
		{
			printf("Error in comparison in 4-bit color mode.\n");
			err=1;
		}
		if(0!=memcmp32(sample8,compare8,sizeof(sample8)))
		{
			printf("Error in comparison in 8-bit color mode.\n");
			err=1;
		}
		// WTF!? The third parameter to memcmp is unsigned short!?
		if(0!=memcmp32(sample16,compare16,sizeof(sample16)))
		{
			printf("Error in comparison in 16-bit color mode.\n");
			err=1;
		}

		if(0==err)
		{
			printf("Comparison Successful.\n");
		}
	}

	return err;
}
