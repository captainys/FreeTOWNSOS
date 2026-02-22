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
#include "egb.h"
#include "snd.h"
#include "io.h"
#include "math.h"
#include "WAITFOR.H"

static char EGB_work[EgbWorkSize];

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



short polygon[256];


void MakeStar(double cx,double cy,double rad)
{
	int angle=90;
	for(int i=0; i<5; ++i,angle+=72*2)
	{
		double a=(double)angle*_PI/180.0;
		double x=cx+cos(a)*rad;
		double y=cy-sin(a)*rad;
		polygon[i*2+1]=(int)x;
		polygon[i*2+2]=(int)y;
	}
	polygon[0]=5;
}


int main(void)
{
	EGB_init(EGB_work,EgbWorkSize);

	EGB_resolution(EGB_work,0,3);  // 640x480 16 colors
	EGB_resolution(EGB_work,1,10); // 320x240 32K colors

	EGB_writePage(EGB_work,1);
	EGB_displayStart(EGB_work,2,2,2);
	EGB_displayStart(EGB_work,3,320,240);
	EGB_clearScreen(EGB_work);

	EGB_paintMode(EGB_work,0x22);

	EGB_writeMode(EGB_work,EGB_PSET);

	EGB_color(EGB_work,EGB_FOREGROUND_COLOR,0x1F);
	EGB_color(EGB_work,EGB_FILL_COLOR,0xffff);
	MakeStar(80,60,50);
	EGB_polygon(EGB_work,polygon);

	EGB_writePage(EGB_work,0);
	EGB_color(EGB_work,EGB_FOREGROUND_COLOR,0x0A);
	EGB_color(EGB_work,EGB_FILL_COLOR,0xffff);
	MakeStar(480,400,60);
	EGB_polygon(EGB_work,polygon);

	WaitForPad();

	EGB_resolution(EGB_work,0,12);  // 640x480 256 colors
	EGB_clearScreen(EGB_work);

	EGB_color(EGB_work,EGB_FOREGROUND_COLOR,0x3F);
	EGB_color(EGB_work,EGB_FILL_COLOR,0xffff);

	MakeStar(320,240,120);
	EGB_polygon(EGB_work,polygon);

	WaitForPad();

	return 0;
}
