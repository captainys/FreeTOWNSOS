#ifndef EGB_H_IS_INCLUDED
#define EGB_H_IS_INCLUDED

#define EGB_NUM_MODECOMB 33

struct POINTW
{
	short x,y;
};

#define SCRNMODE_FLAG_VSCROLL	1
#define SCRNMODE_FLAG_HSCROLL	2

struct EGB_ScreenMode
{
	struct POINTW visiSize;
	struct POINTW size;
	unsigned short bytesPerLine;
	unsigned short bytesPerLineShift;  // 0:Can not shift  Non-Zero:Can shift (bytesPerLine is 2^n)
	unsigned char colors;
	unsigned char combination[4];

	unsigned int flags;
	struct POINTW defZoom;

	_Far unsigned char *vram;
};

struct EGB_Work
{
	unsigned char screenMode[2];      //+00H
	unsigned char writePage;          //+02H
	unsigned char showPageBits;       //+03H
	unsigned short foregroundColor;   //+04H
	unsigned short backgroundColor;   //+06H
	unsigned short transparentColor;  //+08H
	unsigned short alpha;             //+0AH
	unsigned short viewport[4];       //+0CH x0,y0,x1,y1
	unsigned short fontSpacing;       //+14H
	unsigned char fontRotation;       //+16H
	unsigned char stringRotation;     //+17H
	unsigned short textX,textY;       //+18H
	unsigned char paintMode;          //+1CH
	unsigned char padding0;           //+1DH PADDING
	unsigned short paintColor;        //+1EH
	unsigned char drawingMode;        //+20H
	unsigned char superImpose;        //+21H
	unsigned short superImposeArea[4];//+22H x0,y0,x1,y1
	unsigned char superImposeBright;  //+2AH
	unsigned char penWidth;           //+2BH
	unsigned char fontStyle;          //+2CH
	unsigned char padding1;           //+2DH PADDING
	unsigned char hatchWid,hatchHei;  //+2EH
	_Far unsigned char *hatchingPtn;  //+30H
	unsigned char tileWid,tileHei;    //+36H
	_Far unsigned char *tilePtn;      //+3AH

	struct EGB_ScreenMode virtualVRAM[4];
};

_Far unsigned short *EGB_GetCRTCRegs(int modeComb);

#endif
