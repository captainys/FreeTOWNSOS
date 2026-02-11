#ifndef TMENU_ITM_H_IS_INCLUDED
#define TMENU_ITM_H_IS_INCLUDED

#ifdef __cplusplus
extern "C" {
#endif

/* So far we know:

[Header]
+0000 int32   0   ID Header
+0004 int32   Size of the header (Prob)
+0008 int32   Number of top-level icons
+000C int32   E6 86 06 62 Meaning Unknown
+0010 char[9] "TMENU.ITM"

+0008+Size    Icons



[Icon: Group]
+0000 int32   1
+0004 int32   Size (Number of bytes excluding this int32 and Type.)
+0008 byte    Icon ID
+0009 byte    ? Always 0?
+000A byte    ? Always 0?
+000B byte    ? Always 0?
+000C byte    ? Always 0?
+000D byte    ? Always 4?
+000E byte    L: Strlen of the Label
+000F C-Str   Label (0-terminated)

      int32   File offset of the next icon after the items in this group.

      uint16  x0
      uint16  y0
      uint16  x1
      uint16  y1

+0008+Size   Next Icon



[Icon:  Program]
+0000 int32   2
+0004 int32   Number of bytes excluding this int32 and Type.
+0008 byte    Icon ID
+0009 byte    ? Always 4?
+000A byte    Strlen of the Label
      C-Str   Label (0-terminated)

      int16   Strlen of the file name
      C-Str   File Name (0-terminated)

      int16   Strlen of the parameter
      C-Str   Parameter (0-terminated)

      int32   Flags: b0  0->TownsOS  1->MS-DOS
                     b1  Directory Change
                     b2  V1.1 App

+0008+Size   Next Icon



[Icon:  V1.1 Item Group]
+0000 int32   3
+0004 int32   Number of bytes excluding this int32 and Type.
+0008 byte    ?
+0009 byte    ?
+000A byte    ?
+000B byte    ?
+000C byte    ?
+000D byte    ?
+000E byte    Strlen of the Label
+000F C-Str   Label (0-terminated)

      int32   File offset of the next icon after the items in this group

      uint16  x0
      uint16  y0
      uint16  x1
      uint16  y1

+0008+Size   Next Icon


*/


#define ICONTYPE_HEADER  0
#define ICONTYPE_GROUP   1
#define ICONTYPE_PROGRAM 2 
#define ICONTYPE_V1GROUP 3

struct TMENUIcon
{
	unsigned int offset;

	unsigned int iconType;
	unsigned int size;
	unsigned int iconBitmapId;

	unsigned char unknownBytes[6];  /* 1 byte for a Program Icon, 5 bytes for a Group Icon, 6 bytes for V1.1 Group */

	unsigned int labelLen;
	const char *label;

	unsigned short x0,y0,x1,y1;

	unsigned int depth;

	/* For program icon */
	unsigned int commandLen;
	const char *command;

	unsigned int paramLen;
	const char *param;

	unsigned int flags;

	/* For group icon */
	unsigned int endOfGroupOffset;
};

struct TMENUITM
{
	unsigned int fileSize;
	unsigned char *data;
	int nIcons;
	struct TMENUIcon *icons;
};


#define TMENUITM_ERR   0
#define TMENUITM_OK    1

void InitTMENUITM(struct TMENUITM *itm);
int LoadTMENUITM(struct TMENUITM *itm,const char fileName[]);
void DeleteTMENUITM(struct TMENUITM *itm);



struct TMENUV1Icon
{
	unsigned int offset;
	unsigned char iconBitmapId;
	unsigned char flags;
	char *eightDotThree;
	char *label;
	char *commandLine;
};

struct TMENUINF
{
	unsigned int fileSize;
	unsigned char *data;
	int nIcons;
	struct TMENUV1Icon *icons;
};

void InitTMENUINF(struct TMENUINF *inf);
int LoadTMENUINF(struct TMENUINF *inf,const char fileName[]);
void DeleteTMENUINF(struct TMENUINF *inf);

#ifdef __cplusplus
} // extern "C"
#endif

#endif
