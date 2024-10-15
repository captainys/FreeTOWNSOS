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
#define EGB_FUNC_OPAQUE 9
#define EGB_FUNC_IMPSET 10
#define EGB_FUNC_IMPRESET 11
#define EGB_FUNC_IMPNOT 12
#define EGB_FUNC_MASKSET 13
#define EGB_FUNC_MASKRESET 14
#define EGB_FUNC_MASKNOT 15

#define EGB_PAINTFLAG_LINE_NORMAL  2
#define EGB_PAINTFLAG_LINE_TILE    4
#define EGB_PAINTFLAG_LINE_HATCH   8
#define EGB_PAINTFLAG_FILL_NORMAL  32
#define EGB_PAINTFLAG_FILL_TILE    64
#define EGB_PAINTFLAG_FILL_HATCH   128
#define EGB_PAINTFLAG_BACK_NORMAL  512
#define EGB_PAINTFLAG_BACK_TILE    1024
#define EGB_PAINTFLAG_BACK_HATCH   2048

#define EGB_FONTSTYLE_BOLD 1
#define EGB_FONTSTYLE_ITALIC 2
#define EGB_FONTSTYLE_SHADOW 4
#define EGB_FONTSTYLE_BORDER 8
#define EGB_FONTSTYLE_UNDERLINE 16
#define EGB_FONTSTYLE_OVERLINE 32
#define EGB_FONTSTYLE_MIDDLELINE 64

enum
{
	CRTC_REG_HSW1=   0x00,
	CRTC_REG_HSW2=   0x01,
	CRTC_REG_UNUSED1=0x02,
	CRTC_REG_UNUSED2=0x03,
	CRTC_REG_HST=    0x04,
	CRTC_REG_VST1=   0x05,
	CRTC_REG_VST2=   0x06,
	CRTC_REG_EET=    0x07,
	CRTC_REG_VST=    0x08,
	CRTC_REG_HDS0=   0x09,
	CRTC_REG_HDE0=   0x0A,
	CRTC_REG_HDS1=   0x0B,
	CRTC_REG_HDE1=   0x0C,
	CRTC_REG_VDS0=   0x0D,
	CRTC_REG_VDE0=   0x0E,
	CRTC_REG_VDS1=   0x0F,
	CRTC_REG_VDE1=   0x10,
	CRTC_REG_FA0=    0x11,
	CRTC_REG_HAJ0=   0x12,
	CRTC_REG_FO0=    0x13,
	CRTC_REG_LO0=    0x14,
	CRTC_REG_FA1=    0x15,
	CRTC_REG_HAJ1=   0x16,
	CRTC_REG_FO1=    0x17,
	CRTC_REG_LO1=    0x18,
	CRTC_REG_EHAJ=   0x19,
	CRTC_REG_EVAJ=   0x1A,
	CRTC_REG_ZOOM=   0x1B,
	CRTC_REG_CR0=    0x1C,
	CRTC_REG_CR1=    0x1D,
	CRTC_REG_FR=     0x1E,
	CRTC_REG_CR2=    0x1F,
};



#define EGB_NUM_MODECOMB 33

struct POINTW
{
	short x,y;
};

struct POINTUW
{
	unsigned short x,y;
};

#define SCRNMODE_FLAG_VSCROLL	1
#define SCRNMODE_FLAG_HSCROLL	2
#define SCRNMODE_FLAG_STARTPOS  4
#define EGB_INVALID_SCRNMODE 0xFF

#define EGB_ERROR_MASK (~0xFF00)
#define EGB_GENERAL_ERROR 0xFF00
#define EGB_NO_ERROR 0
#define EGB_SetError(reg,err) {reg&=EGB_ERROR_MASK;reg|=err;}

#define IS_SJIS_FIRST_BYTE(ch) ((0x81<=(unsigned char)(ch) && (unsigned char)(ch)<0xA0)||(0xE0<=(unsigned char)(ch) && (unsigned char)(ch)<0xFD))

#define BoxIsOutsideOfViewport(xMin,yMin,xMax,yMax,page) ((xMax)<(page)->viewport[0].x || (page)->viewport[1].x<(xMin) || (yMax)<(page)->viewport[0].y || (page)->viewport[1].y<(yMin))

#define EGB_CalcVRAMAddr(vramAddr,x,y,mode) \
	if(0!=(mode)->bytesPerLineShift) \
	{ \
		(*vramAddr)=((y)<<(mode)->bytesPerLineShift); \
		(*vramAddr)+=((x)*(mode)->bitsPerPixel)/8; \
	} \
	else \
	{ \
		(*vramAddr)=(((y)*(mode)->bytesPerLine+(x))*(mode)->bitsPerPixel)>>3; \
	}


struct EGB_ScreenMode
{
	unsigned int KHz;
	struct POINTW visiSize;
	struct POINTW crtcSize;
	struct POINTW size;
	unsigned short bytesPerLine;
	unsigned short bytesPerLineShift;  // 0:Can not shift  Non-Zero:Can shift (bytesPerLine is 2^n)
	unsigned char bitsPerPixel;
	unsigned char combination[4];

	unsigned int flags;
	struct POINTW defZoom;

	_Far unsigned char *vram;
};

struct EGB_PerPage
{
	unsigned char screenMode;
	unsigned char _padding0;

	unsigned short alpha;

	// Confirmed in Towns OS V2.1 L50
	// Viewport is a per-page setting.  Reset upon EGB_Resolution.
	struct POINTW viewport[2];

	// Still need to check if the text position is per-page or system-wide.
	short textX,textY;

	unsigned char ZOOM;

	unsigned char penWidth;
	unsigned char hatchWid;
	unsigned char hatchHei;
	_Far unsigned char *hatchingPtn;
	unsigned char tileWid;
	unsigned char tileHei;
	_Far unsigned char *tilePtn;
};

struct EGB_Work
{
	unsigned char writePage;
	unsigned char superImpose;
	unsigned short superImposeArea[4];
	unsigned char superImposeBright;

	// Confirmed in Towns OS V2.1 L50
	// Drawing Mode is a system-wide setting.  Not per page.  Then, probably paint mode, too.
	unsigned char drawingMode;
	unsigned short paintMode;

	// Confirmed in Towns OS V2.1 L50
	// Font spacing is a system-wide setting.  Not per page.
	// Then, probably other font settings as well.
	// I am experiencing a situation that any setting of font style, text zoom, text rotation will crash the system
	// in V2.1 L50.  Do I not have to support them then?
	unsigned short fontStyle;
	unsigned short fontSpacing;
	unsigned char fontRotation;
	unsigned char stringRotation;
	struct POINTUW ankZoom,kanjiZoom;

	int CRTCRegSet;
	struct EGB_PerPage perPage[2];

	unsigned short crtcRegs[NUM_CRTC_REGS];
	unsigned char sifter[2];

	// Confirmed in Towns OS V2.1 L50
	// Color is a system-wide setting.  Not per page.
	unsigned short color[4];

	struct EGB_ScreenMode virtualPage[4];
	struct EGB_PerPage perVirtualPage[4];
};

struct EGB_PagePointerSet
{
	_Far struct EGB_ScreenMode *mode;
	_Far struct EGB_PerPage *page;
	_Far unsigned char *vram;
	unsigned int vramSize;
};

struct EGB_String
{
	short x,y;
	unsigned short len;
	unsigned char str[1];
};

struct EGB_Palette
{
	unsigned int colorIndex;
	unsigned char brg[4];
};

struct EGB_PaletteSet
{
	unsigned int numPalettes;
	struct EGB_Palette palettes[1];
};

struct EGB_BlockInfo
{
	_Far unsigned char *data;
	struct POINTW p0,p1;
};

_Far struct EGB_ScreenMode *EGB_GetScreenModeProp(int mode);
_Far unsigned short *EGB_GetCRTCRegs(int modeComb);

_Far unsigned int *EGB_GetDefaultPalette256(void);
_Far unsigned int *EGB_GetDefaultPalette16(void);

struct EGB_PagePointerSet EGB_GetPagePointerSet(_Far struct EGB_Work *work);

void EGB_MakeP0SmallerThanP1(struct POINTW *p0,struct POINTW *p1);

void SwapShort(short *a,short *b);

#endif
