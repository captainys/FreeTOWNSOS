#include "EGB.H"
#include "TGBIOS.H"
#include <DOS.H>


struct EGB_ScreenMode EGB_SCREENMODES[19]=
{
	{ // Zero is not used
		{0,0}, //struct POINTW visiSize;
		{0,0}, //struct POINTW size;
		0,//unsigned short bytesPerLine;
		0,//unsigned short bytesPerLineShift;  // 0:Can not shift  Non-Zero:Can shift (bytesPerLine is 2^n)
		0,//unsigned char bitsPerPixel;
		{EGB_INVALID_SCRNMODE,EGB_INVALID_SCRNMODE,EGB_INVALID_SCRNMODE,EGB_INVALID_SCRNMODE},//unsigned char combination[4];

		0,//unsigned int flags;
		{0,0},//struct POINTW defZoom;

		NULL,//_Far unsigned char *vram;
	},
	{ // Mode 1
		{640,400}, //struct POINTW visiSize;
		{640,819}, //struct POINTW size;
		320,//unsigned short bytesPerLine;
		0,//unsigned short bytesPerLineShift;  // 0:Can not shift  Non-Zero:Can shift (bytesPerLine is 2^n)
		4,//unsigned char bitsPerPixel;
		{1,EGB_INVALID_SCRNMODE,EGB_INVALID_SCRNMODE,EGB_INVALID_SCRNMODE},//unsigned char combination[4];

		0,//unsigned int flags;
		{1,1},//struct POINTW defZoom;

		NULL,//_Far unsigned char *vram;
	},
	{ // Mode 2
		{640,200}, //struct POINTW visiSize;
		{640,819}, //struct POINTW size;
		320,//unsigned short bytesPerLine;
		0,//unsigned short bytesPerLineShift;  // 0:Can not shift  Non-Zero:Can shift (bytesPerLine is 2^n)
		4,//unsigned char bitsPerPixel;
		{2,EGB_INVALID_SCRNMODE,EGB_INVALID_SCRNMODE,EGB_INVALID_SCRNMODE},//unsigned char combination[4];

		0,//unsigned int flags;
		{1,2},//struct POINTW defZoom;

		NULL,//_Far unsigned char *vram;
	},
	{ // Mode 3
		{640,480}, //struct POINTW visiSize;
		{1024,512}, //struct POINTW size;
		512,//unsigned short bytesPerLine;
		9,//unsigned short bytesPerLineShift;  // 0:Can not shift  Non-Zero:Can shift (bytesPerLine is 2^n)
		4,//unsigned char bitsPerPixel;
		{3,5,10,EGB_INVALID_SCRNMODE},//unsigned char combination[4];

		SCRNMODE_FLAG_VSCROLL,//unsigned int flags;
		{1,1},//struct POINTW defZoom;

		NULL,//_Far unsigned char *vram;
	},
	{ // Mode 4
		{640,400}, //struct POINTW visiSize;
		{1024,512}, //struct POINTW size;
		512,//unsigned short bytesPerLine;
		9,//unsigned short bytesPerLineShift;  // 0:Can not shift  Non-Zero:Can shift (bytesPerLine is 2^n)
		4,//unsigned char bitsPerPixel;
		{4,6,EGB_INVALID_SCRNMODE,EGB_INVALID_SCRNMODE},//unsigned char combination[4];

		SCRNMODE_FLAG_VSCROLL,//unsigned int flags;
		{1,1},//struct POINTW defZoom;

		NULL,//_Far unsigned char *vram;
	},
	{ // Mode 5
		{256,256}, //struct POINTW visiSize;
		{256,512}, //struct POINTW size;
		512,//unsigned short bytesPerLine;
		9,//unsigned short bytesPerLineShift;  // 0:Can not shift  Non-Zero:Can shift (bytesPerLine is 2^n)
		16,//unsigned char bitsPerPixel;
		{5,3,10,EGB_INVALID_SCRNMODE},//unsigned char combination[4];

		SCRNMODE_FLAG_STARTPOS|SCRNMODE_FLAG_VSCROLL,//unsigned int flags;
		{1,1},//struct POINTW defZoom;

		NULL,//_Far unsigned char *vram;
	},
	{ // Mode 6
		{256,256}, //struct POINTW visiSize;
		{256,512}, //struct POINTW size;
		512,//unsigned short bytesPerLine;
		9,//unsigned short bytesPerLineShift;  // 0:Can not shift  Non-Zero:Can shift (bytesPerLine is 2^n)
		16,//unsigned char bitsPerPixel;
		{4,6,EGB_INVALID_SCRNMODE,EGB_INVALID_SCRNMODE},//unsigned char combination[4];

		SCRNMODE_FLAG_STARTPOS|SCRNMODE_FLAG_VSCROLL,//unsigned int flags;
		{1,1},//struct POINTW defZoom;

		NULL,//_Far unsigned char *vram;
	},
	{ // Mode 7
		{256,240}, //struct POINTW visiSize;
		{256,512}, //struct POINTW size;
		512,//unsigned short bytesPerLine;
		9,//unsigned short bytesPerLineShift;  // 0:Can not shift  Non-Zero:Can shift (bytesPerLine is 2^n)
		16,//unsigned char bitsPerPixel;
		{7,9,EGB_INVALID_SCRNMODE,EGB_INVALID_SCRNMODE},//unsigned char combination[4];

		SCRNMODE_FLAG_STARTPOS|SCRNMODE_FLAG_VSCROLL,//unsigned int flags;
		{4,1},//struct POINTW defZoom;

		NULL,//_Far unsigned char *vram;
	},
	{ // Mode 8
		{256,240}, //struct POINTW visiSize;
		{256,512}, //struct POINTW size;
		512,//unsigned short bytesPerLine;
		9,//unsigned short bytesPerLineShift;  // 0:Can not shift  Non-Zero:Can shift (bytesPerLine is 2^n)
		16,//unsigned char bitsPerPixel;
		{8,11,EGB_INVALID_SCRNMODE,EGB_INVALID_SCRNMODE},//unsigned char combination[4];

		SCRNMODE_FLAG_STARTPOS|SCRNMODE_FLAG_VSCROLL,//unsigned int flags;
		{4,1},//struct POINTW defZoom;

		NULL,//_Far unsigned char *vram;
	},
	{ // Mode 9
		{360,240}, //struct POINTW visiSize;
		{512,256}, //struct POINTW size;
		1024,//unsigned short bytesPerLine;
		10,//unsigned short bytesPerLineShift;  // 0:Can not shift  Non-Zero:Can shift (bytesPerLine is 2^n)
		16,//unsigned char bitsPerPixel;
		{7,9,EGB_INVALID_SCRNMODE,EGB_INVALID_SCRNMODE},//unsigned char combination[4];

		SCRNMODE_FLAG_VSCROLL|SCRNMODE_FLAG_HSCROLL,//unsigned int flags;
		{4,1},//struct POINTW defZoom;

		NULL,//_Far unsigned char *vram;
	},
	{ // Mode 10
		{320,240}, //struct POINTW visiSize;
		{512,256}, //struct POINTW size;
		1024,//unsigned short bytesPerLine;
		10,//unsigned short bytesPerLineShift;  // 0:Can not shift  Non-Zero:Can shift (bytesPerLine is 2^n)
		16,//unsigned char bitsPerPixel;
		{3,5,10,EGB_INVALID_SCRNMODE},//unsigned char combination[4];

		SCRNMODE_FLAG_STARTPOS|SCRNMODE_FLAG_VSCROLL|SCRNMODE_FLAG_HSCROLL,//unsigned int flags;
		{1,1},//struct POINTW defZoom;

		NULL,//_Far unsigned char *vram;
	},
	{ // Mode 11
		{320,240}, //struct POINTW visiSize;
		{512,256}, //struct POINTW size;
		1024,//unsigned short bytesPerLine;
		10,//unsigned short bytesPerLineShift;  // 0:Can not shift  Non-Zero:Can shift (bytesPerLine is 2^n)
		16,//unsigned char bitsPerPixel;
		{8,11,EGB_INVALID_SCRNMODE,EGB_INVALID_SCRNMODE},//unsigned char combination[4];

		SCRNMODE_FLAG_VSCROLL|SCRNMODE_FLAG_HSCROLL,//unsigned int flags;
		{4,1},//struct POINTW defZoom;

		NULL,//_Far unsigned char *vram;
	},
	{ // Mode 12
		{640,480}, //struct POINTW visiSize;
		{1024,512}, //struct POINTW size;
		1024,//unsigned short bytesPerLine;
		10,//unsigned short bytesPerLineShift;  // 0:Can not shift  Non-Zero:Can shift (bytesPerLine is 2^n)
		8,//unsigned char bitsPerPixel;
		{EGB_INVALID_SCRNMODE,EGB_INVALID_SCRNMODE,EGB_INVALID_SCRNMODE,EGB_INVALID_SCRNMODE},//unsigned char combination[4];

		SCRNMODE_FLAG_VSCROLL|SCRNMODE_FLAG_HSCROLL,//unsigned int flags;
		{1,1},//struct POINTW defZoom;

		NULL,//_Far unsigned char *vram;
	},
	{ // Mode 13
		{640,400}, //struct POINTW visiSize;
		{1024,512}, //struct POINTW size;
		1024,//unsigned short bytesPerLine;
		10,//unsigned short bytesPerLineShift;  // 0:Can not shift  Non-Zero:Can shift (bytesPerLine is 2^n)
		8,//unsigned char bitsPerPixel;
		{EGB_INVALID_SCRNMODE,EGB_INVALID_SCRNMODE,EGB_INVALID_SCRNMODE,EGB_INVALID_SCRNMODE},//unsigned char combination[4];

		SCRNMODE_FLAG_VSCROLL|SCRNMODE_FLAG_HSCROLL,//unsigned int flags;
		{1,1},//struct POINTW defZoom;

		NULL,//_Far unsigned char *vram;
	},
	{ // Mode 14
		{720,480}, //struct POINTW visiSize;
		{1024,512}, //struct POINTW size;
		1024,//unsigned short bytesPerLine;
		10,//unsigned short bytesPerLineShift;  // 0:Can not shift  Non-Zero:Can shift (bytesPerLine is 2^n)
		8,//unsigned char bitsPerPixel;
		{EGB_INVALID_SCRNMODE,EGB_INVALID_SCRNMODE,EGB_INVALID_SCRNMODE,EGB_INVALID_SCRNMODE},//unsigned char combination[4];

		SCRNMODE_FLAG_VSCROLL|SCRNMODE_FLAG_HSCROLL,//unsigned int flags;
		{2,1},//struct POINTW defZoom;

		NULL,//_Far unsigned char *vram;
	},
	{ // Mode 15
		{320,480}, //struct POINTW visiSize;
		{512,512}, //struct POINTW size;
		1024,//unsigned short bytesPerLine;
		10,//unsigned short bytesPerLineShift;  // 0:Can not shift  Non-Zero:Can shift (bytesPerLine is 2^n)
		16,//unsigned char bitsPerPixel;
		{EGB_INVALID_SCRNMODE,EGB_INVALID_SCRNMODE,EGB_INVALID_SCRNMODE,EGB_INVALID_SCRNMODE},//unsigned char combination[4];

		SCRNMODE_FLAG_VSCROLL|SCRNMODE_FLAG_HSCROLL,//unsigned int flags;
		{2,1},//struct POINTW defZoom;

		NULL,//_Far unsigned char *vram;
	},
	{ // Mode 16
		{320,480}, //struct POINTW visiSize;
		{512,512}, //struct POINTW size;
		1024,//unsigned short bytesPerLine;
		10,//unsigned short bytesPerLineShift;  // 0:Can not shift  Non-Zero:Can shift (bytesPerLine is 2^n)
		16,//unsigned char bitsPerPixel;
		{EGB_INVALID_SCRNMODE,EGB_INVALID_SCRNMODE,EGB_INVALID_SCRNMODE,EGB_INVALID_SCRNMODE},//unsigned char combination[4];

		SCRNMODE_FLAG_VSCROLL|SCRNMODE_FLAG_HSCROLL,//unsigned int flags;
		{4,1},//struct POINTW defZoom;

		NULL,//_Far unsigned char *vram;
	},
	{ // Mode 17
		{512,480}, //struct POINTW visiSize;
		{512,512}, //struct POINTW size;
		1024,//unsigned short bytesPerLine;
		10,//unsigned short bytesPerLineShift;  // 0:Can not shift  Non-Zero:Can shift (bytesPerLine is 2^n)
		16,//unsigned char bitsPerPixel;
		{EGB_INVALID_SCRNMODE,EGB_INVALID_SCRNMODE,EGB_INVALID_SCRNMODE,EGB_INVALID_SCRNMODE},//unsigned char combination[4];

		SCRNMODE_FLAG_VSCROLL,//unsigned int flags;
		{1,1},//struct POINTW defZoom;

		NULL,//_Far unsigned char *vram;
	},
	{ // Mode 18
		{512,480}, //struct POINTW visiSize;
		{512,512}, //struct POINTW size;
		1024,//unsigned short bytesPerLine;
		10,//unsigned short bytesPerLineShift;  // 0:Can not shift  Non-Zero:Can shift (bytesPerLine is 2^n)
		16,//unsigned char bitsPerPixel;
		{EGB_INVALID_SCRNMODE,EGB_INVALID_SCRNMODE,EGB_INVALID_SCRNMODE,EGB_INVALID_SCRNMODE},//unsigned char combination[4];

		SCRNMODE_FLAG_VSCROLL,//unsigned int flags;
		{2,1},//struct POINTW defZoom;

		NULL,//_Far unsigned char *vram;
	}
};

_Far struct EGB_ScreenMode *EGB_GetScreenModeProp(int mode)
{
	if(mode<19)
	{
		return &EGB_SCREENMODES[mode];
	}
	return NULL;
}

unsigned short EGB_CRTCREGS[EGB_NUM_MODECOMB][38]=
{
	{
	0x0C,0xFF,
//   HSW1  HSW2   ----   ----   HST    VST1   VST2   EET    VST    HDS0   HDE0   HDS1   HDE1   VDS0   VDE1   VDS1
	0x0060,0x02C0,0x0000,0x0000,0x031F,0x0000,0x0004,0x0000,0x0419,0x008A,0x030A,0x008A,0x030A,0x0046,0x0406,0x0046,
	0x0406,0x0000,0x008A,0x0000,0x0080,0x0000,0x008A,0x0000,0x0080,0x0058,0x0001,0x0000,0x800F,0x0002,0x0000,0x0192,
//   VDE1  FA0    HAJ0   FO0    LO0    FA1    HAJ1   FO1    LO1    EHAJ   EVAJ   ZOOM   CR0    CR1    FR     CR2
	0x0A,0x18,0x00,0x00,
	},
	{
	0x0D,0xFF,
//   HSW1  HSW2   ----   ----   HST    VST1   VST2   EET    VST    HDS0   HDE0   HDS1   HDE1   VDS0   VDE1   VDS1
	0x0040,0x0320,0x0000,0x0000,0x035F,0x0000,0x0010,0x0000,0x036F,0x009C,0x031C,0x009C,0x031C,0x0040,0x0360,0x0040,
	0x0360,0x0000,0x009C,0x0000,0x0080,0x0000,0x009C,0x0000,0x0080,0x004A,0x0001,0x0000,0x800F,0x0003,0x0000,0x0150,
//   VDE1  FA0    HAJ0   FO0    LO0    FA1    HAJ1   FO1    LO1    EHAJ   EVAJ   ZOOM   CR0    CR1    FR     CR2
	0x0A,0x18,0x00,0x00,
	},
	{
	0x0E,0xFF,
//   HSW1  HSW2   ----   ----   HST    VST1   VST2   EET    VST    HDS0   HDE0   HDS1   HDE1   VDS0   VDE1   VDS1
	0x0086,0x0610,0x0000,0x0000,0x071B,0x0006,0x000C,0x0012,0x020C,0x0129,0x06C9,0x0129,0x06C9,0x002A,0x020A,0x002A,
	0x020A,0x0000,0x0129,0x0080,0x0100,0x0000,0x0129,0x0080,0x0100,0x0064,0x0001,0x0101,0x800F,0x000C,0x0003,0x01CA,
//   VDE1  FA0    HAJ0   FO0    LO0    FA1    HAJ1   FO1    LO1    EHAJ   EVAJ   ZOOM   CR0    CR1    FR     CR2
	0x0A,0x18,0x00,0x00,
	},
	{
	0x0F,0xFF,
//   HSW1  HSW2   ----   ----   HST    VST1   VST2   EET    VST    HDS0   HDE0   HDS1   HDE1   VDS0   VDE1   VDS1
	0x0060,0x02C0,0x0000,0x0000,0x031F,0x0000,0x0004,0x0000,0x0419,0x008A,0x030A,0x008A,0x030A,0x0046,0x0406,0x0046,
	0x0406,0x0000,0x008A,0x0000,0x0080,0x0000,0x008A,0x0000,0x0080,0x0058,0x0001,0x0101,0x800A,0x0002,0x0000,0x0192,
//   VDE1  FA0    HAJ0   FO0    LO0    FA1    HAJ1   FO1    LO1    EHAJ   EVAJ   ZOOM   CR0    CR1    FR     CR2
	0x0F,0x08,0x00,0x00,
	},
	{
	0x10,0xFF,
//   HSW1  HSW2   ----   ----   HST    VST1   VST2   EET    VST    HDS0   HDE0   HDS1   HDE1   VDS0   VDE1   VDS1
	0x0074,0x0530,0x0000,0x0000,0x0617,0x0006,0x000C,0x0012,0x020C,0x00E7,0x05E7,0x00E7,0x05E7,0x002A,0x020A,0x002A,
	0x020A,0x0000,0x00E7,0x0080,0x0100,0x0000,0x00E7,0x0080,0x0100,0x0056,0x0001,0x0303,0x800A,0x0001,0x0002,0x0188,
//   VDE1  FA0    HAJ0   FO0    LO0    FA1    HAJ1   FO1    LO1    EHAJ   EVAJ   ZOOM   CR0    CR1    FR     CR2
	0x0F,0x08,0x00,0x00,
	},
	{
	0x01,0x01,
//   HSW1  HSW2   ----   ----   HST    VST1   VST2   EET    VST    HDS0   HDE0   HDS1   HDE1   VDS0   VDE1   VDS1
	0x0040,0x0320,0x0000,0x0000,0x035F,0x0000,0x0010,0x0000,0x036F,0x009C,0x031C,0x009C,0x031C,0x0040,0x0360,0x0040,
	0x0360,0x0000,0x009C,0x0000,0x0050,0x0000,0x009C,0x0000,0x0050,0x004A,0x0001,0x0000,0x803F,0x0003,0x0000,0x0150,
//   VDE1  FA0    HAJ0   FO0    LO0    FA1    HAJ1   FO1    LO1    EHAJ   EVAJ   ZOOM   CR0    CR1    FR     CR2
	0x15,0x28,0x00,0x00,
	},
	{
	0xFF,0xFF,
//   HSW1  HSW2   ----   ----   HST    VST1   VST2   EET    VST    HDS0   HDE0   HDS1   HDE1   VDS0   VDE1   VDS1
	0x0040,0x0320,0x0000,0x0000,0x035F,0x0000,0x0010,0x0000,0x036F,0x009C,0x031C,0x009C,0x031C,0x0040,0x0360,0x0040,
	0x0360,0x0000,0x009C,0x0000,0x0050,0x0000,0x009C,0x0000,0x0050,0x004A,0x0001,0x0000,0x803F,0x0003,0x0000,0x0150,
//   VDE1  FA0    HAJ0   FO0    LO0    FA1    HAJ1   FO1    LO1    EHAJ   EVAJ   ZOOM   CR0    CR1    FR     CR2
	0x15,0x28,0x00,0x00,
	},
	{
	0x02,0x02,
//   HSW1  HSW2   ----   ----   HST    VST1   VST2   EET    VST    HDS0   HDE0   HDS1   HDE1   VDS0   VDE1   VDS1
	0x0040,0x0320,0x0000,0x0000,0x035F,0x0000,0x0010,0x0000,0x036F,0x009C,0x031C,0x009C,0x031C,0x0040,0x0360,0x0040,
	0x0360,0x0000,0x009C,0x0000,0x0050,0x0000,0x009C,0x0000,0x0050,0x004A,0x0001,0x1010,0x803F,0x0003,0x0000,0x0150,
//   VDE1  FA0    HAJ0   FO0    LO0    FA1    HAJ1   FO1    LO1    EHAJ   EVAJ   ZOOM   CR0    CR1    FR     CR2
	0x15,0x28,0x00,0x00,
	},
	{
	0x03,0x03,
//   HSW1  HSW2   ----   ----   HST    VST1   VST2   EET    VST    HDS0   HDE0   HDS1   HDE1   VDS0   VDE1   VDS1
	0x0060,0x02C0,0x0000,0x0000,0x031F,0x0000,0x0004,0x0000,0x0419,0x008A,0x030A,0x008A,0x030A,0x0046,0x0406,0x0046,
	0x0406,0x0000,0x008A,0x0000,0x0080,0x0000,0x008A,0x0000,0x0080,0x0058,0x0001,0x0000,0x800F,0x0002,0x0000,0x0192,
//   VDE1  FA0    HAJ0   FO0    LO0    FA1    HAJ1   FO1    LO1    EHAJ   EVAJ   ZOOM   CR0    CR1    FR     CR2
	0x15,0x28,0x00,0x00,
	},
	{
	0x04,0x04,
//   HSW1  HSW2   ----   ----   HST    VST1   VST2   EET    VST    HDS0   HDE0   HDS1   HDE1   VDS0   VDE1   VDS1
	0x0040,0x0320,0x0000,0x0000,0x035F,0x0000,0x0010,0x0000,0x036F,0x009C,0x031C,0x009C,0x031C,0x0040,0x0360,0x0040,
	0x0360,0x0000,0x009C,0x0000,0x0080,0x0000,0x009C,0x0000,0x0080,0x004A,0x0001,0x0000,0x800F,0x0003,0x0000,0x0150,
//   VDE1  FA0    HAJ0   FO0    LO0    FA1    HAJ1   FO1    LO1    EHAJ   EVAJ   ZOOM   CR0    CR1    FR     CR2
	0x15,0x28,0x00,0x00,
	},
	{
	0x06,0x04,
//   HSW1  HSW2   ----   ----   HST    VST1   VST2   EET    VST    HDS0   HDE0   HDS1   HDE1   VDS0   VDE1   VDS1
	0x0040,0x0320,0x0000,0x0000,0x035F,0x0000,0x0010,0x0000,0x036F,0x009C,0x019C,0x009C,0x031C,0x0040,0x0240,0x0040,
	0x0360,0x0000,0x009C,0x0000,0x0080,0x0000,0x009C,0x0000,0x0080,0x004A,0x0001,0x0000,0x800D,0x0003,0x0000,0x0150,
//   VDE1  FA0    HAJ0   FO0    LO0    FA1    HAJ1   FO1    LO1    EHAJ   EVAJ   ZOOM   CR0    CR1    FR     CR2
	0x17,0x28,0x00,0x00,
	},
	{
	0x09,0x09,
//   HSW1  HSW2   ----   ----   HST    VST1   VST2   EET    VST    HDS0   HDE0   HDS1   HDE1   VDS0   VDE1   VDS1
	0x0086,0x0610,0x0000,0x0000,0x071B,0x0006,0x000C,0x0012,0x020C,0x0129,0x06C9,0x0129,0x06C9,0x002A,0x020A,0x002A,
	0x020A,0x0000,0x0129,0x0000,0x0100,0x0000,0x0129,0x0000,0x0100,0x0064,0x0001,0x0303,0x8005,0x000C,0x0003,0x01CA,
//   VDE1  FA0    HAJ0   FO0    LO0    FA1    HAJ1   FO1    LO1    EHAJ   EVAJ   ZOOM   CR0    CR1    FR     CR2
	0x1F,0x28,0x00,0x00,
	},
	{
	0x07,0x09,
//   HSW1  HSW2   ----   ----   HST    VST1   VST2   EET    VST    HDS0   HDE0   HDS1   HDE1   VDS0   VDE1   VDS1
	0x0086,0x0610,0x0000,0x0000,0x071B,0x0006,0x000C,0x0012,0x020C,0x0129,0x0529,0x0129,0x06C9,0x002A,0x020A,0x002A,
	0x020A,0x0000,0x0129,0x0000,0x0080,0x0000,0x0129,0x0000,0x0100,0x0064,0x0001,0x0303,0x8005,0x000C,0x0003,0x01CA,
//   VDE1  FA0    HAJ0   FO0    LO0    FA1    HAJ1   FO1    LO1    EHAJ   EVAJ   ZOOM   CR0    CR1    FR     CR2
	0x1F,0x28,0x00,0x00,
	},
	{
	0x0B,0x0B,
//   HSW1  HSW2   ----   ----   HST    VST1   VST2   EET    VST    HDS0   HDE0   HDS1   HDE1   VDS0   VDE1   VDS1
	0x0074,0x0530,0x0000,0x0000,0x0617,0x0006,0x000C,0x0012,0x020B,0x00E7,0x05E7,0x00E7,0x05E7,0x002A,0x020A,0x002A,
	0x020A,0x0000,0x00E7,0x0000,0x0100,0x0000,0x00E7,0x0000,0x0100,0x0056,0x0001,0x0303,0x8005,0x0001,0x0002,0x0188,
//   VDE1  FA0    HAJ0   FO0    LO0    FA1    HAJ1   FO1    LO1    EHAJ   EVAJ   ZOOM   CR0    CR1    FR     CR2
	0x1F,0x28,0x00,0x00,
	},
	{
	0x08,0x0B,
//   HSW1  HSW2   ----   ----   HST    VST1   VST2   EET    VST    HDS0   HDE0   HDS1   HDE1   VDS0   VDE1   VDS1
	0x0074,0x0530,0x0000,0x0000,0x0617,0x0006,0x000C,0x0012,0x020B,0x00E7,0x04E7,0x00E7,0x05E7,0x002A,0x020A,0x002A,
	0x020A,0x0000,0x00E7,0x0000,0x0080,0x0000,0x00E7,0x0000,0x0100,0x0056,0x0001,0x0303,0x8005,0x0001,0x0002,0x0188,
//   VDE1  FA0    HAJ0   FO0    LO0    FA1    HAJ1   FO1    LO1    EHAJ   EVAJ   ZOOM   CR0    CR1    FR     CR2
	0x1F,0x28,0x00,0x00,
	},
	{
	0x06,0x06,
//   HSW1  HSW2   ----   ----   HST    VST1   VST2   EET    VST    HDS0   HDE0   HDS1   HDE1   VDS0   VDE1   VDS1
	0x0040,0x0320,0x0000,0x0000,0x035F,0x0000,0x0010,0x0000,0x036F,0x009C,0x019C,0x009C,0x019C,0x0040,0x0240,0x0040,
	0x0240,0x0000,0x009C,0x0000,0x0080,0x0000,0x009C,0x0000,0x0080,0x004A,0x0001,0x0000,0x8005,0x0003,0x0000,0x0150,
//   VDE1  FA0    HAJ0   FO0    LO0    FA1    HAJ1   FO1    LO1    EHAJ   EVAJ   ZOOM   CR0    CR1    FR     CR2
	0x1F,0x28,0x00,0x00,
	},
	{
	0x07,0x07,
//   HSW1  HSW2   ----   ----   HST    VST1   VST2   EET    VST    HDS0   HDE0   HDS1   HDE1   VDS0   VDE1   VDS1
	0x0086,0x0610,0x0000,0x0000,0x071B,0x0006,0x000C,0x0012,0x020C,0x0129,0x0529,0x0129,0x0529,0x002A,0x020A,0x002A,
	0x020A,0x0000,0x0129,0x0000,0x0080,0x0000,0x0129,0x0000,0x0080,0x0064,0x0001,0x0303,0x8005,0x000C,0x0003,0x01CA,
//   VDE1  FA0    HAJ0   FO0    LO0    FA1    HAJ1   FO1    LO1    EHAJ   EVAJ   ZOOM   CR0    CR1    FR     CR2
	0x1F,0x28,0x00,0x00,
	},
	{
	0x08,0x08,
//   HSW1  HSW2   ----   ----   HST    VST1   VST2   EET    VST    HDS0   HDE0   HDS1   HDE1   VDS0   VDE1   VDS1
	0x0074,0x0530,0x0000,0x0000,0x0617,0x0006,0x000C,0x0012,0x020B,0x00E7,0x04E7,0x00E7,0x04E7,0x002A,0x020A,0x002A,
	0x020A,0x0000,0x00E7,0x0000,0x0080,0x0000,0x00E7,0x0000,0x0080,0x0056,0x0001,0x0303,0x8005,0x0001,0x0002,0x0188,
//   VDE1  FA0    HAJ0   FO0    LO0    FA1    HAJ1   FO1    LO1    EHAJ   EVAJ   ZOOM   CR0    CR1    FR     CR2
	0x1F,0x28,0x00,0x00,
	},
	{
	0x05,0x03,
//   HSW1  HSW2   ----   ----   HST    VST1   VST2   EET    VST    HDS0   HDE0   HDS1   HDE1   VDS0   VDE1   VDS1
	0x0060,0x02C0,0x0000,0x0000,0x031F,0x0000,0x0004,0x0000,0x0419,0x008A,0x018A,0x008A,0x030A,0x0046,0x0246,0x0046,
	0x0406,0x0000,0x008A,0x0000,0x0080,0x0000,0x008A,0x0000,0x0080,0x0058,0x0001,0x0000,0x800D,0x0002,0x0000,0x0192,
//   VDE1  FA0    HAJ0   FO0    LO0    FA1    HAJ1   FO1    LO1    EHAJ   EVAJ   ZOOM   CR0    CR1    FR     CR2
	0x17,0x28,0x00,0x00,
	},
	{
	0x05,0x05,
//   HSW1  HSW2   ----   ----   HST    VST1   VST2   EET    VST    HDS0   HDE0   HDS1   HDE1   VDS0   VDE1   VDS1
	0x0060,0x02C0,0x0000,0x0000,0x031F,0x0000,0x0004,0x0000,0x0419,0x008A,0x018A,0x008A,0x018A,0x0046,0x0246,0x0046,
	0x0246,0x0000,0x008A,0x0000,0x0080,0x0000,0x008A,0x0000,0x0080,0x0058,0x0001,0x0000,0x8005,0x0002,0x0000,0x0192,
//   VDE1  FA0    HAJ0   FO0    LO0    FA1    HAJ1   FO1    LO1    EHAJ   EVAJ   ZOOM   CR0    CR1    FR     CR2
	0x1F,0x28,0x00,0x00,
	},
	{
	0xFF,0xFF,
//   HSW1  HSW2   ----   ----   HST    VST1   VST2   EET    VST    HDS0   HDE0   HDS1   HDE1   VDS0   VDE1   VDS1
	0x0060,0x02C0,0x0000,0x0000,0x031F,0x0000,0x0004,0x0000,0x0419,0x008A,0x018A,0x008A,0x018A,0x0046,0x0246,0x0046,
	0x0246,0x0000,0x008A,0x0000,0x0080,0x0000,0x008A,0x0000,0x0080,0x0058,0x0001,0x0000,0x8005,0x0002,0x0000,0x0192,
//   VDE1  FA0    HAJ0   FO0    LO0    FA1    HAJ1   FO1    LO1    EHAJ   EVAJ   ZOOM   CR0    CR1    FR     CR2
	0x1F,0x28,0x00,0x00,
	},
	{
	0x03,0x05,
//   HSW1  HSW2   ----   ----   HST    VST1   VST2   EET    VST    HDS0   HDE0   HDS1   HDE1   VDS0   VDE1   VDS1
	0x0060,0x02C0,0x0000,0x0000,0x031F,0x0000,0x0004,0x0000,0x0419,0x008A,0x030A,0x008A,0x018A,0x0046,0x0406,0x0046,
	0x0246,0x0000,0x008A,0x0000,0x0080,0x0000,0x008A,0x0000,0x0080,0x0058,0x0001,0x0000,0x8007,0x0002,0x0000,0x0192,
//   VDE1  FA0    HAJ0   FO0    LO0    FA1    HAJ1   FO1    LO1    EHAJ   EVAJ   ZOOM   CR0    CR1    FR     CR2
	0x1D,0x28,0x00,0x00,
	},
	{
	0x04,0x06,
//   HSW1  HSW2   ----   ----   HST    VST1   VST2   EET    VST    HDS0   HDE0   HDS1   HDE1   VDS0   VDE1   VDS1
	0x0040,0x0320,0x0000,0x0000,0x035F,0x0000,0x0010,0x0000,0x036F,0x009C,0x031C,0x009C,0x019C,0x0040,0x0360,0x0040,
	0x0240,0x0000,0x009C,0x0000,0x0080,0x0000,0x009C,0x0000,0x0080,0x004A,0x0001,0x0000,0x8007,0x0003,0x0000,0x0150,
//   VDE1  FA0    HAJ0   FO0    LO0    FA1    HAJ1   FO1    LO1    EHAJ   EVAJ   ZOOM   CR0    CR1    FR     CR2
	0x1D,0x28,0x00,0x00,
	},
	{
	0x09,0x07,
//   HSW1  HSW2   ----   ----   HST    VST1   VST2   EET    VST    HDS0   HDE0   HDS1   HDE1   VDS0   VDE1   VDS1
	0x0086,0x0610,0x0000,0x0000,0x071B,0x0006,0x000C,0x0012,0x020C,0x0129,0x06C9,0x0129,0x0529,0x002A,0x020A,0x002A,
	0x020A,0x0000,0x0129,0x0000,0x0100,0x0000,0x0129,0x0000,0x0080,0x0064,0x0001,0x0303,0x8005,0x000C,0x0003,0x01CA,
//   VDE1  FA0    HAJ0   FO0    LO0    FA1    HAJ1   FO1    LO1    EHAJ   EVAJ   ZOOM   CR0    CR1    FR     CR2
	0x1F,0x28,0x00,0x00,
	},
	{
	0x0B,0x08,
//   HSW1  HSW2   ----   ----   HST    VST1   VST2   EET    VST    HDS0   HDE0   HDS1   HDE1   VDS0   VDE1   VDS1
	0x0074,0x0530,0x0000,0x0000,0x0617,0x0006,0x000C,0x0012,0x020B,0x00E7,0x05E7,0x00E7,0x04E7,0x002A,0x020A,0x002A,
	0x020A,0x0000,0x00E7,0x0000,0x0100,0x0000,0x00E7,0x0000,0x0080,0x0056,0x0001,0x0303,0x8005,0x0001,0x0002,0x0188,
//   VDE1  FA0    HAJ0   FO0    LO0    FA1    HAJ1   FO1    LO1    EHAJ   EVAJ   ZOOM   CR0    CR1    FR     CR2
	0x1F,0x28,0x00,0x00,
	},
	{
	0x03,0x0A,
//   HSW1  HSW2   ----   ----   HST    VST1   VST2   EET    VST    HDS0   HDE0   HDS1   HDE1   VDS0   VDE1   VDS1
	0x0060,0x02C0,0x0000,0x0000,0x031F,0x0000,0x0004,0x0000,0x0419,0x008A,0x030A,0x008A,0x01CA,0x0046,0x0406,0x0046,
	0x0226,0x0000,0x008A,0x0000,0x0080,0x0000,0x008A,0x0000,0x0100,0x0058,0x0001,0x0000,0x8007,0x0002,0x0000,0x0192,
//   VDE1  FA0    HAJ0   FO0    LO0    FA1    HAJ1   FO1    LO1    EHAJ   EVAJ   ZOOM   CR0    CR1    FR     CR2
	0x1D,0x28,0x00,0x00,
	},
	{
	0x0A,0x03,
//   HSW1  HSW2   ----   ----   HST    VST1   VST2   EET    VST    HDS0   HDE0   HDS1   HDE1   VDS0   VDE1   VDS1
	0x0060,0x02C0,0x0000,0x0000,0x031F,0x0000,0x0004,0x0000,0x0419,0x008A,0x01CA,0x008A,0x030A,0x0046,0x0226,0x0046,
	0x0406,0x0000,0x008A,0x0000,0x0100,0x0000,0x008A,0x0000,0x0080,0x0058,0x0001,0x0000,0x800D,0x0002,0x0000,0x0192,
//   VDE1  FA0    HAJ0   FO0    LO0    FA1    HAJ1   FO1    LO1    EHAJ   EVAJ   ZOOM   CR0    CR1    FR     CR2
	0x17,0x28,0x00,0x00,
	},
	{
	0x0A,0x0A,
//   HSW1  HSW2   ----   ----   HST    VST1   VST2   EET    VST    HDS0   HDE0   HDS1   HDE1   VDS0   VDE1   VDS1
	0x0060,0x02C0,0x0000,0x0000,0x031F,0x0000,0x0004,0x0000,0x0419,0x008A,0x01CA,0x008A,0x01CA,0x0046,0x0226,0x0046,
	0x0226,0x0000,0x008A,0x0000,0x0100,0x0000,0x008A,0x0000,0x0100,0x0058,0x0001,0x0000,0x8005,0x0002,0x0000,0x0192,
//   VDE1  FA0    HAJ0   FO0    LO0    FA1    HAJ1   FO1    LO1    EHAJ   EVAJ   ZOOM   CR0    CR1    FR     CR2
	0x1F,0x28,0x00,0x00,
	},
	{
	0x0A,0x05,
//   HSW1  HSW2   ----   ----   HST    VST1   VST2   EET    VST    HDS0   HDE0   HDS1   HDE1   VDS0   VDE1   VDS1
	0x0060,0x02C0,0x0000,0x0000,0x031F,0x0000,0x0004,0x0000,0x0419,0x008A,0x01CA,0x008A,0x018A,0x0046,0x0226,0x0046,
	0x0246,0x0000,0x008A,0x0000,0x0100,0x0000,0x008A,0x0000,0x0080,0x0058,0x0000,0x0000,0x8005,0x0002,0x0000,0x0192,
//   VDE1  FA0    HAJ0   FO0    LO0    FA1    HAJ1   FO1    LO1    EHAJ   EVAJ   ZOOM   CR0    CR1    FR     CR2
	0x1F,0x28,0x00,0x00,
	},
	{
	0x05,0x0A,
//   HSW1  HSW2   ----   ----   HST    VST1   VST2   EET    VST    HDS0   HDE0   HDS1   HDE1   VDS0   VDE1   VDS1
	0x0060,0x02C0,0x0000,0x0000,0x031F,0x0000,0x0004,0x0000,0x0419,0x008A,0x018A,0x008A,0x01CA,0x0046,0x0246,0x0046,
	0x0226,0x0000,0x008A,0x0000,0x0080,0x0000,0x008A,0x0000,0x0100,0x0058,0x0000,0x0000,0x8005,0x0002,0x0000,0x0192,
//   VDE1  FA0    HAJ0   FO0    LO0    FA1    HAJ1   FO1    LO1    EHAJ   EVAJ   ZOOM   CR0    CR1    FR     CR2
	0x1F,0x28,0x00,0x00,
	},
	{
	0x11,0xFF,
//   HSW1  HSW2   ----   ----   HST    VST1   VST2   EET    VST    HDS0   HDE0   HDS1   HDE1   VDS0   VDE1   VDS1
	0x0060,0x02C0,0x0000,0x0000,0x031F,0x0000,0x0004,0x0000,0x0419,0x00CA,0x02CA,0x00CA,0x02CA,0x0046,0x0406,0x0046,
	0x0406,0x0000,0x00CA,0x0000,0x0080,0x0000,0x00CA,0x0000,0x0080,0x0058,0x0001,0x0000,0x800A,0x0002,0x0000,0x0192,
//   VDE1  FA0    HAJ0   FO0    LO0    FA1    HAJ1   FO1    LO1    EHAJ   EVAJ   ZOOM   CR0    CR1    FR     CR2
	0x0F,0x08,0x00,0x00,
	},
	{
	0x12,0xFF,
//   HSW1  HSW2   ----   ----   HST    VST1   VST2   EET    VST    HDS0   HDE0   HDS1   HDE1   VDS0   VDE1   VDS1
	0x0074,0x0530,0x0000,0x0000,0x0617,0x0006,0x000C,0x0012,0x020C,0x0167,0x0567,0x0167,0x0567,0x002A,0x020A,0x002A,
	0x020A,0x0000,0x0167,0x0080,0x0100,0x0000,0x0167,0x0080,0x0100,0x0056,0x0001,0x0101,0x800A,0x0001,0x0002,0x0188,
//   VDE1  FA0    HAJ0   FO0    LO0    FA1    HAJ1   FO1    LO1    EHAJ   EVAJ   ZOOM   CR0    CR1    FR     CR2
	0x0F,0x08,0x00,0x00,
	},
	{
	0x01,0x04,
//   HSW1  HSW2   ----   ----   HST    VST1   VST2   EET    VST    HDS0   HDE0   HDS1   HDE1   VDS0   VDE1   VDS1
	0x0074,0x0530,0x0000,0x0000,0x0617,0x0006,0x000C,0x0012,0x020C,0x0167,0x0567,0x0167,0x0567,0x002A,0x020A,0x002A,
	0x020A,0x0000,0x0167,0x0080,0x0100,0x0000,0x0167,0x0080,0x0100,0x0056,0x0001,0x0101,0x800A,0x0001,0x0002,0x0188,
//   VDE1  FA0    HAJ0   FO0    LO0    FA1    HAJ1   FO1    LO1    EHAJ   EVAJ   ZOOM   CR0    CR1    FR     CR2
	0x0F,0x08,0x00,0x00,
	}
};

_Far unsigned short *EGB_GetCRTCRegs(int modeComb)
{
	if(modeComb<33)
	{
		_Far unsigned short *ptr;
		_FP_SEG(ptr)=SEG_TGBIOS_DATA;
		_FP_OFF(ptr)=(unsigned int)(EGB_CRTCREGS[modeComb]);
		return ptr;
	}
	else
	{
		return NULL;
	}
}
