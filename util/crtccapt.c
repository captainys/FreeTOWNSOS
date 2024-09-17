/* LICENSE>>
Copyright 2020 Soji Yamakawa (CaptainYS, http://www.ysflight.com)

Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.

2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.

3. Neither the name of the copyright holder nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

<< LICENSE */
#include "egb.h"
#include "io.h"

static char EGB_work[EgbWorkSize];

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

short seihou[]=
{
	5,
	 20, 20,
	 20,100,
	100,100,
	100, 20,
	 20, 20
};

short hishigata[]=
{
	5,
	  80,120,
	  20,180,
	  80,240,
	 140,180,
	  80,120,
};

unsigned char modeComb[33][2]=
{
	{ 12,255},//1   Register Set number shown in FM TOWNS Technical Data Book p.136
	{ 13,255},//2   What's the hell is this sequence?
	{ 14,255},//3
	{ 15,255},//4
	{ 16,255},//5
	{  1,  1},//6
	{255,255},//7
	{  2,  2},//8
	{  3,  3},//9
	{  4,  4},//10
	{  6,  4},//11
	{  9,  9},//12
	{  7,  9},//13
	{ 11, 11},//14
	{  8, 11},//15
	{  6,  6},//16
	{  7,  7},//17
	{  8,  8},//18
	{  5,  3},//19
	{  5,  5},//20
	{255,255},//21
	{  3,  5},//22
	{  4,  6},//23
	{  9,  7},//24
	{ 11,  8},//25
	{  3, 10},//26
	{ 10,  3},//27
	{ 10, 10},//28
	{ 10,  5},//29
	{  5, 10},//30
	{ 17,255},//31
	{ 18,255},//32
	{  1,  4},//FMR
};

int main(void)
{
	EGB_init(EGB_work,EgbWorkSize);

	for(int i=0; i<33; ++i)
	{
		EGB_resolution(EGB_work,0,modeComb[i][0]);
		EGB_resolution(EGB_work,1,modeComb[i][1]);
		// CaptureCRTC registers
	}
	return 0;
}
