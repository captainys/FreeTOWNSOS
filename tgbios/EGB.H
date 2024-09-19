#ifndef EGB_H_IS_INCLUDED
#define EGB_H_IS_INCLUDED


#include "IODEF.H"

#define EGB_FOREGROUND_COLOR 0
#define EGB_BACKGROUND_COLOR 1
#define EGB_FILL_COLOR 2
#define EGB_TRANSPARENT_COLOR 3

#define EGB_FUNC_PSET 0
#define EGB_FUNC_PRESET 1
#define EGB_FUNC_OR 2
#define EGB_FUNC_AND 3
#define EGB_FUNC_XOR 4
#define EGB_FUNC_NOT 5
#define EGB_FUNC_MATTE 6
#define EGB_FUNC_PASTEL 7
#define EGB_FUNC_OPAGUE 9
#define EGB_FUNC_MASKSET 13
#define EGB_FUNC_MASKRESET 14
#define EGB_FUNC_MASKNOT 15



#define EGB_NUM_MODECOMB 33

struct POINTW
{
	short x,y;
};

#define SCRNMODE_FLAG_VSCROLL	1
#define SCRNMODE_FLAG_HSCROLL	2
#define SCRNMODE_FLAG_STARTPOS  4
#define EGB_INVALID_SCRNMODE 0xFF

#define EGB_ERROR_MASK (~0xFF00)
#define EGB_GENERAL_ERROR 0xFF00
#define EGB_NO_ERROR 0
#define EGB_SetError(reg,err) {reg&=EGB_ERROR_MASK;reg|=err;}

struct EGB_ScreenMode
{
	struct POINTW visiSize;
	struct POINTW size;
	unsigned short bytesPerLine;
	unsigned short bytesPerLineShift;  // 0:Can not shift  Non-Zero:Can shift (bytesPerLine is 2^n)
	unsigned char bitsPerPixel;
	unsigned char combination[4];

	unsigned int flags;
	struct POINTW defZoom;

	_Far unsigned char *vram;
};

struct EGB_Work
{
	unsigned char screenMode[2];
	unsigned char writePage;
	unsigned char showPageBits;
	unsigned short foregroundColor;
	unsigned short backgroundColor;
	unsigned short transparentColor;
	unsigned short alpha;
	unsigned short viewport[4];
	unsigned short fontSpacing;
	unsigned char fontRotation;
	unsigned char stringRotation;
	unsigned short textX,textY;
	unsigned char paintMode;
	unsigned char padding0;
	unsigned short paintColor;
	unsigned char drawingMode;
	unsigned char superImpose;
	unsigned short superImposeArea[4];
	unsigned char superImposeBright;
	unsigned char penWidth;
	unsigned char fontStyle;
	unsigned char padding1;
	unsigned char hatchWid,hatchHei;
	_Far unsigned char *hatchingPtn;
	unsigned char tileWid,tileHei;
	_Far unsigned char *tilePtn;

	unsigned char padding2;           //+3BH PADDING

	unsigned short crtcRegs[NUM_CRTC_REGS];  //+3CH
	unsigned char sifter[2];          //+7CH

	struct EGB_ScreenMode virtualVRAM[4];
};

_Far struct EGB_ScreenMode *EGB_GetScreenModeProp(int mode);
_Far unsigned short *EGB_GetCRTCRegs(int modeComb);

#endif
