#define TRUE    1
#define FALSE   0
#define ERR     (-1)

/*
#define	ANKCG	1
*/

#define UCHAR   unsigned char
#define SHORT   short int
#define	UNSIG	unsigned int

#define	OFFSET(p,n)	*((unsigned int *)(&(p)))=n
#define	SEGSET(p,n)	*((unsigned int *)(&(p))+1)=n

#define	U_AX	(reg->x.ax)
#define U_AH    (reg->h.ah)
#define	U_AL	(reg->h.al)

#define	U_BX	(reg->x.bx)
#define U_BH    (reg->h.bh)
#define	U_BL	(reg->h.bl)

#define	U_CX	(reg->x.cx)
#define U_CH    (reg->h.ch)
#define	U_CL	(reg->h.cl)

#define	U_DX	(reg->x.dx)
#define U_DH    (reg->h.dh)
#define	U_DL	(reg->h.dl)

#define	U_DI	(reg->x.di)
#define	U_SI	(reg->x.si)

#define	U_DS	(reg->x.ds)
#define	U_ES	(reg->x.es)

#define	U_CF	(reg->x.cf)

#define	CVRAM	0xC800
#define	KVRAM	0xCA00

#define	SCR_X	80
#define MAX_X   80
#ifdef	LINE40
  #define MAX_Y   40
#else
  #define MAX_Y   25
#endif

#define DMYKAN  0xFE
#define CRCHR   0x1F
#define TABCHR  0x09
#define	NULCHR	0x20
#define TAB     8
#define CONTRL  FALSE
#define DEFCOL  COLOR

SHORT	 OLD_X=0xFFFF;
SHORT    CUR_X=0,CUR_Y=0;
SHORT    CUR_TYPE=0x30;
SHORT    CUR_OFF2=14,CUR_SIZE2=2;
UCHAR    CUR_DSP_FLG=0;
UCHAR    Con_mode=0xCE;
SHORT    SCR_Y=MAX_Y;
SHORT    COLOR=7,BAKCOL=0;
UCHAR    Act_Dsp=FALSE;
#ifdef	ANKCG
UCHAR    Ank_font[256][16];
#endif

union _REGSET {
    struct {
	SHORT	es;
	SHORT	ds;
	SHORT	di;
	SHORT	si;
	SHORT	bp;
	SHORT	sp;
	SHORT	bx;
	SHORT	dx;
	SHORT	cx;
	SHORT	ax;
	SHORT	ip;
	SHORT	cs;
	SHORT	cf;
    } x;
    struct {
	SHORT	_es;
	SHORT	_ds;
	SHORT	_di;
	SHORT	_si;
	SHORT	_bp;
	SHORT	_sp;
	UCHAR	bl,bh;
	UCHAR	dl,dh;
	UCHAR	cl,ch;
	UCHAR	al,ah;
	SHORT	_ip;
	SHORT	_cs;
	SHORT	_cf;
    } h;
};
/**************************************************************

	CONSOL BIOS (int 91h) 

***************************************************************/
void	VDB_00(reg)	/* 初期化 */
register union _REGSET far *reg;
{
#ifdef	ANKCG
    font_init(Ank_font);
#endif
    SCR_Y = MAX_Y - 1;
    CUR_DSP_FLG = 0;
    Con_mode = 0xCE;
    CUR_X = CUR_Y = 0;
    COLOR = 7;
    colset(0,COLOR,(MAX_X * 2) * MAX_Y);
    Con_init();
}
void	VDB_01(reg)
register union _REGSET far *reg;
{
}	/* 画面の表示制御 */

void	VDB_02(reg)	/* 全画面消去 */
register union _REGSET far *reg;
{
    colset(0,DEFCOL,(MAX_X * 2) * MAX_Y);
    CUR_X = CUR_Y = 0;
}
void 	VDB_03(reg)	/* 表示画面サイズの設定 */
register union _REGSET far *reg;
{
    if ( (SCR_Y = U_DH) > MAX_Y )
	SCR_Y = MAX_Y;
    if ( (Con_mode & 0x40) != 0 )
	SCR_Y--;
}
void 	VDB_04(reg)	/* 表示画面サイズの読み取り */
register union _REGSET far *reg;
{
    U_DL = SCR_X;
    U_DH = SCR_Y;
}
void	VDB_05(reg)	/* 表示画面サイズレパ－トリの読み取り */
register union _REGSET far *reg;
{
    union {
	UCHAR far	*p;
	unsigned short  s[2];
    } ptr;

    ptr.s[0] = U_DI;
    ptr.s[1] = U_DS;

    if ( *(ptr.p++) < 4 )
	return;
    *(ptr.p++) = 1;
    *(ptr.p++) = 80;
    *(ptr.p++) = 25;
}
void 	VDB_06(reg)	/* アトリビュート機能範囲の読み取り */
register union _REGSET far *reg;
{
    union {
	UCHAR far	*p;
	unsigned short  s[2];
    } ptr;

    ptr.s[0] = U_DI;
    ptr.s[1] = U_DS;

    *(ptr.p++) = 0x03;	/* 漢字識別可能 */
    *(ptr.p++) = 0x38;	/* ﾘﾊﾞ-ｽ & 強調　*/
    *(ptr.p++) = 0x07;	/* 最大色数 */
    *(ptr.p) = 0;
}
void	VDB_07(reg)	/* フォントパタ－ンの取り出し */
register union _REGSET far *reg;
{
    int     i;
    union {
	UCHAR far	*p;
	unsigned short  s[2];
    } ptr,fnt;

    ptr.s[0] = U_DI;
    ptr.s[1] = U_DS;

    if ( U_BH == 0 ) {	/* ANK 8x8 or 8x16 */
	if ( U_DH != 8 )
	    goto ERROR;
	else if ( U_DL == 8 ) {
	    fnt.s[0] = U_BL * 8;
	    fnt.s[1] = 0xCA00;
	    outp(0xFF99,1);
	    for ( i = 0 ; i < 8 ; i++ )
		*(ptr.p++) = *(fnt.p++);
	    outp(0xFF99,0);
	    return;
	} else if ( U_DL == 16 ) {
	    fnt.s[0] = U_BL * 16;
	    fnt.s[1] = 0xCB00;
	    outp(0xFF99,1);
	    for ( i = 0 ; i < 16 ; i++ )
		*(ptr.p++) = *(fnt.p++);
	    outp(0xFF99,0);
	    return;
	} else
	    goto ERROR;
    } else if ( U_DX == 0x1010 ) {	/* Kanji 16x16 */
    	outp(0xFF94,U_BL);
    	outp(0xFF95,U_BH);
	for ( i = 0 ; i < 16 ; i++ ) {
	    *(ptr.p++) = inp(0xFF96);
	    *(ptr.p++) = inp(0xFF97);
	}
	return;
    }
ERROR:
    U_AH = 0x02;
    U_CF |= 0x0001;
}
void	VDB_08(reg)
register union _REGSET far *reg;
{}	/* 外字パタ－ンの登録 */

void	VDB_09(reg)	/* カ－ソル形状の設定 */
register union _REGSET far *reg;
{
    switch(U_AL & 0x0F) {
	case 0: CUR_OFF2 = 14; CUR_SIZE2 = 2; break;
	case 1: CUR_OFF2 = 0;  CUR_SIZE2 = 16; break;
	case 15: CUR_OFF2 = U_DH;
		 CUR_SIZE2 = U_DL - CUR_OFF2;
		if ( CUR_OFF2 < 0 || CUR_OFF2 > 15 )
		    CUR_OFF2 = 15;
		if ( CUR_SIZE2 < 0 || (CUR_SIZE2 + CUR_OFF2) > 16 )
		    CUR_SIZE2 = 16 - CUR_OFF2;
		break;
    }
    CUR_DSP_FLG = ((U_AL & 0x60) == 0x20 ? 1:0);
    CUR_TYPE = U_AL;
    OLD_X = 0xFFFF;
}
void	VDB_0A(reg)	/* カ－ソル形状の読み取り */
register union _REGSET far *reg;
{
    U_AL = CUR_TYPE;
    U_DH = CUR_OFF2;
    U_DL = CUR_OFF2 + CUR_SIZE2;
}
void	VDB_0B(reg)	/* カ－ソル表示状態の設定 */ 
register union _REGSET far *reg;
{
    CUR_DSP_FLG = U_AL;
}
void	VDB_0C(reg)	/* カ－ソル表示状態の読み取り */
register union _REGSET far *reg;
{
    U_AL = CUR_DSP_FLG;
}
void    VDB_0D(reg)	/* カーソル位置の指定 */
register union _REGSET far *reg;
{
    CUR_X = U_DL - 1;
    CUR_Y = U_DH - 1;
    if ( CUR_X < 0 ) CUR_X = 0;
    if ( CUR_X >= SCR_X ) CUR_X = SCR_X - 1;
    if ( CUR_Y < 0 ) CUR_Y = 0;
    if ( CUR_Y >= MAX_Y ) CUR_Y = MAX_Y - 1;
}
void 	VDB_0E(reg)	/* カーソル位置の読み取り */
register union _REGSET far *reg;
{
    U_DL = CUR_X + 1;
    U_DH = CUR_Y + 1;
}
void	VDB_0F(reg)	/* アトリビュートの設定 */
register union _REGSET far *reg;
{
    int    at,x,y;
    union {
	UCHAR far	*p;
	unsigned short  s[2];
    } ptr,vrm;

    x = U_DL - 1;
    y = U_DH - 1;

    vrm.s[1] = CVRAM;
    vrm.s[0] = (x * 2 + (MAX_X * 2) * y + 1);

    ptr.s[1] = U_DS;
    ptr.s[0] = (U_DI + 1);
    at = *vrm.p & 0xC0;
    *vrm.p = at | (*ptr.p & 0x38) | (*(ptr.p+1) & 0x07);
}
void	VDB_10(reg)	/* アトリビュートの読み取り */
register union _REGSET far *reg;
{
    int    at,x,y;
    union {
	UCHAR far	*p;
	unsigned short  s[2];
    } ptr,vrm;

    x = U_DL - 1;
    y = U_DH - 1;

    vrm.s[1] = CVRAM;
    vrm.s[0] = (x * 2 + (MAX_X * 2) * y + 1);
    ptr.s[1] = U_DS;
    ptr.s[0] = U_DI;

    if ( (*vrm.p & 0x40) != 0 )
	*ptr.p = 0x01;
    else if ( x > 0 && (*(vrm.p-2) & 0x40) != 0 ) {
	*ptr.p = 0x03;
	vrm.p -= 2;
    } else
	*ptr.p = 0x00;

    *(++ptr.p) = *vrm.p & 0x38;
    *(++ptr.p) = *vrm.p & 0x07;
}
void	VDB_11(reg)	/* デフォルトアトリビュ－トの設定 */
register union _REGSET far *reg;
{
    union {
	UCHAR far	*p;
	unsigned short  s[2];
    } ptr;

    ptr.s[0] = U_DI;
    ptr.s[1] = U_DS;

    DEFCOL = (*(++ptr.p) & 0x38);
    DEFCOL |= (*(++ptr.p) & 0x07);
}
void	VDB_12(reg)	/* デフォルトアトリビュ－トの読み取り */
register union _REGSET far *reg;
{
    union {
	UCHAR far	*p;
	unsigned short  s[2];
    } ptr;

    ptr.s[0] = U_DI;
    ptr.s[1] = U_DS;

    *(++ptr.p) = DEFCOL & 0x38;
    *(++ptr.p) = DEFCOL & 0x07;
}
void	VDB_13(reg)	/* 文字設定 */
register union _REGSET far *reg;
{
    int    ch,x,y;
    union {
	UCHAR far	*p;
	unsigned short  s[2];
    } ptr;

    x = U_DL - 1;
    y = U_DH - 1;
    ch = U_BL;

    ptr.s[0] = (x * 2 + (MAX_X * 2) * y);
    ptr.s[1] = CVRAM;

    *(ptr.p++) = ch;
    if ( U_AL != 1 )
	*ptr.p = DEFCOL;
    else
	*ptr.p &= 0x3F;
    if ( U_BH == 0x01 )
	*ptr.p |= 0x40;
    else if ( U_BH == 0x03 ) {
	ch |= (*(ptr.p-3) << 8);
	*(ptr.p-1) = *(ptr.p-3) = DMYKAN;
        ptr.s[1] = KVRAM;
	*(ptr.p-3) = ch >> 8;
	*(ptr.p-2) = ch;
    }
}
void	VDB_14(reg)	/* 文字読み取り */
register union _REGSET far *reg;
{
    int    at,x,y;
    union {
	UCHAR far	*p;
	unsigned short  s[2];
    } ptr;

    x = U_DL - 1;
    y = U_DH - 1;

    ptr.s[1] = CVRAM;
    ptr.s[0] = (x * 2 + (MAX_X * 2) * y);

    if ( (*(ptr.p+1) & 0x40) != 0 ) {
	U_BH = 0x01;
        ptr.s[1] = KVRAM;
        U_BL = *ptr.p;

    } else if ( x > 0 && (*(ptr.p-1) & 0x40) != 0 ) {
	U_BH = 0x03;
        ptr.s[1] = KVRAM;
        U_BL = *(ptr.p-1);

    } else {
	U_BH = 0x00;
        U_BL = *ptr.p;
    }
}
void	VDB_15(reg)	/* 矩形域設定 */
register union _REGSET far *reg;
{
    int     cx,ch;
    int     x1,y1,x2,y2;
    int     x,y,sx,sy;
    union {
	UNSIG far	*p;
	unsigned short  s[2];
    } pp;
    union {
	UCHAR far	*p;
	unsigned short  s[2];
    } cp,ap,sp,vp;

    if ( (x1 = U_DL - 1) >= MAX_X || x1 < 0)
	goto ERROR;
    if ( (y1 = U_DH - 1) >= SCR_Y || y1 < 0 )
	goto ERROR;
    if ( (x2 = U_BL) > MAX_X )
	goto ERROR;
    if ( (y2 = U_BH) > SCR_Y )
	goto ERROR;
    sx = x2 - x1; sy = y2 - y1;
    if ( sx <= 0 || sy <= 0 )
	goto ERROR;

    sp.s[0] = (x1 * 2 + (MAX_X * 2) * y1);
    sp.s[1] = CVRAM;

    pp.s[0] = U_DI;
    pp.s[1] = U_DS;

    cp.s[0] = *(pp.p++);
    cp.s[1] = *(pp.p++);

    ap.s[0] = *(pp.p++);
    ap.s[1] = *(pp.p++);

    for ( y = 0 ; y < sy ; y++ ) {
	for ( x = 0,vp.p = sp.p ; x < sx ; x++ ) {
	    *(vp.p++) = *(cp.p++);
	    if ( *(ap.p++) == 0x03 ) {
		*(vp.p-2) |= 0x40;
		ch = *(vp.p-3) << 8 | *(vp.p-1);
		vp.s[1] = KVRAM;
		*(vp.p-3) = ch >> 8;
		*(vp.p-2) = ch;
		vp.s[1] = CVRAM;
	    } else
		*vp.p &= 0x3F;
	    if ( U_AL == 1 ) {
		*vp.p = *(ap.p++) & 0x38;
		*vp.p |= *(ap.p++) & 0x07;
		ap.p++;
	    }
	    vp.p++;
	}
	sp.p += (MAX_X * 2);
    }
    return;
ERROR:
    U_AH = 0x02;
    U_CF |= 0x0001;
}
void    VDB_16(reg)        /* 矩形域読み取り */
register union _REGSET far *reg;
{
    int     i;
    int     x1,y1,x2,y2;
    int     x,y,sx,sy;
    union {
	UNSIG far	*p;
	unsigned short  s[2];
    } pp;
    union {
	UCHAR far	*p;
	unsigned short  s[2];
    } cp,ap,sp,vp;

    if ( (x1 = U_DL-1) >= MAX_X || x1 < 0)
	goto ERROR;
    if ( (y1 = U_DH-1) >= SCR_Y || y1 < 0 )
	goto ERROR;
    if ( (x2 = U_BL) > MAX_X )
	goto ERROR;
    if ( (y2 = U_BH) > SCR_Y )
	goto ERROR;
    sx = x2 - x1; sy = y2 - y1;
    if ( sx <= 0 || sy <= 0 )
	goto ERROR;

    sp.s[0] = (x1 * 2 + (MAX_X * 2) * y1);
    sp.s[1] = CVRAM;

    pp.s[0] = U_DI;
    pp.s[1] = U_DS;

    cp.s[0] = *(pp.p++);
    cp.s[1] = *(pp.p++);

    ap.s[0] = *(pp.p++);
    ap.s[1] = *(pp.p++);

    for ( y = 0 ; y < sy ; y++ ) {
	for ( x = 0,vp.p = sp.p ; x < sx ; x++ ) {
	    *(cp.p++) = *(vp.p++);
	    if ( (*vp.p & 0x40) != 0 ) {
		*(ap.p++) = 0x01;
		vp.s[1] = KVRAM;
		*(cp.p-1) = *(vp.p-1);
		vp.s[1] = CVRAM;
	    } else if ( x > 0 && (*(vp.p-2) & 0x40) != 0 ) {
		*(ap.p++) = 0x03;
		vp.s[1] = KVRAM;
		*(cp.p-1) = *(vp.p-2);
		vp.s[1] = CVRAM;
	    } else
		*(ap.p++) = 0x00;
	    if ( U_AL == 1 ) {
		*(ap.p++) = *vp.p & 0x38;
		*(ap.p++) = *vp.p & 0x07;
		ap.p++;
	    }
	    vp.p++;
	}
	sp.p += (MAX_X * 2);
    }
    return;
ERROR:
    U_AH = 0x02;
    U_CF |= 0x0001;
}
void	VDB_17(reg)	/* 矩形域複写 */
register union _REGSET far *reg;
{
    int     i;
    int     x1,y1,x2,y2,x3,y3;
    int     sx,sy;
    UNSIG   sp;
    UNSIG   dp;

    if ( (x1 = U_DL - 1) >= MAX_X || x1 < 0 )
	goto ERROR;
    if ( (y1 = U_DH - 1) >= SCR_Y || y1 < 0 )
	goto ERROR;
    if ( (x2 = U_BL) > MAX_X )
	goto ERROR;
    if ( (y2 = U_BH) > SCR_Y )
	goto ERROR;
    if ( (x3 = U_CL - 1) >= MAX_X || x3 < 0 )
	goto ERROR;
    if ( (y3 = U_CH - 1) >= SCR_Y || y3 < 0 )
	goto ERROR;
    sx = x2 - x1; sy = y2 - y1;
    if ( sx <= 0 || sy <= 0 )
	goto ERROR;
    if ( (x3 + sx) > MAX_X || (y3 + sy) > SCR_Y )
	goto ERROR;

    if ( sp < dp ) {
	sp = (x2 - 1) * 2 + (MAX_X * 2) * (y2 - 1);
	dp = (x3 + sx - 1) * 2 + (MAX_X * 2) * (y3 + sy - 1);
	for ( i = 0 ; i < sy ; i++ ) {
	    vramrcpy(dp,sp,sx * 2);
	    sp -= (MAX_X * 2);
	    dp -= (MAX_X * 2);
	}
    } else {
	sp = x1 * 2 + (MAX_X * 2) * y1;
	dp = x3 * 2 + (MAX_X * 2) * y3;
	for ( i = 0 ; i < sy ; i++ ) {
	    vramcpy(dp,sp,sx * 2);
	    sp += (MAX_X * 2);
	    dp += (MAX_X * 2);
	}
    }
    return;
ERROR:
    U_AH = 0x02;
    U_CF |= 0x0001;
}
void	VDB_18(reg)	/* 矩形域消去 */
register union _REGSET far *reg;
{
    int     i;
    int     x1,y1,x2,y2;
    int     sx,sy;
    UNSIG   sp;

    if ( (x1 = U_DL - 1) >= MAX_X || x1 < 0 )
	goto ERROR;
    if ( (y1 = U_DH - 1) >= SCR_Y || y1 < 0 )
	goto ERROR;
    if ( (x2 = U_BL) > MAX_X )
	goto ERROR;
    if ( (y2 = U_BH) > SCR_Y )
	goto ERROR;
    sx = x2 - x1; sy = y2 - y1;
    if ( sx <= 0 || sy <= 0 )
	goto ERROR;
    sp = (x1 * 2 + (MAX_X * 2) * y1);

    for ( i = 0 ; i < sy ; i++ ) {
	colset(sp,DEFCOL,sx * 2);
	sp += (MAX_X * 2);
    }
    return;
ERROR:
    U_AH = 0x02;
    U_CF |= 0x0001;
}
void	VDB_19(reg)	/* 全画面スクロ－ル */ 
register union _REGSET far *reg;
{
    int     cx,i;
    UNSIG   vp;

    if ( (cx = U_CX) == 0 || cx > SCR_Y )
	cx = SCR_Y;
    for ( ; cx > 0 ; cx-- ) {
	switch(U_AL) {
	    case 1:	/* 下方向 */
		i = (MAX_X * 2) * (SCR_Y - 1);
		vramrcpy(i + (MAX_X * 2 - 2),i - 2,i);
		colset(0,DEFCOL,(MAX_X * 2));
		break;
	    case 2:	/* 右方向 */
		i = (MAX_X * 2) * SCR_Y - 2;
		vramrcpy(i,i - 2,i);
		vp = 0;
		for ( i = 0 ; i < SCR_Y ; i++ ) {
		    colset(vp,DEFCOL,2);
		    vp += (MAX_X * 2);
		}
		break;
	    case 3:	/* 左方向 */
		vramcpy(0,2,(MAX_X * 2) * SCR_Y - 2);
		vp = (MAX_X * 2) - 2;
		for ( i = 0 ; i < SCR_Y ; i++ ) {
		    colset(vp,DEFCOL,2);
		    vp += (MAX_X * 2);
		}
		break;
            default: 	/* case 0 上方向 */
		vramcpy(0,(MAX_X * 2),(MAX_X * 2) * (SCR_Y - 1));
		colset((MAX_X * 2) * (SCR_Y - 1),DEFCOL,(MAX_X * 2));
		break;
	}
    }
}
void	VDB_1A(reg)	/* 部分スクロ－ル */
register union _REGSET far *reg;
{
    int     cx,i;
    int     x1,y1,x2,y2;
    int     sx,sy;
    UNSIG   sp;
    UNSIG   vp;

    if ( (x1 = U_DL - 1) >= MAX_X || x1 < 0 )
	goto ERROR;
    if ( (y1 = U_DH - 1) >= SCR_Y || y1 < 0 )
	goto ERROR;
    if ( (x2 = U_BL) > MAX_X )
	goto ERROR;
    if ( (y2 = U_BH) > SCR_Y )
	goto ERROR;
    sx = x2 - x1; sy = y2 - y1;
    if ( sx <= 0 || sy <= 0 )
	goto ERROR;
    sp = (x1 * 2 + (MAX_X * 2) * y1);

    if ( (cx = U_CX) == 0 || cx > sy )
	cx = sy;
    for ( ; cx > 0 ; cx-- ) {
	switch(U_AL) {
	    case 1:	/* 下方向 */
		vp = sp + (MAX_X * 2) * (sy - 1);
		for ( i = 0 ; i < (sy-1) ; i++ ) {
		    vramcpy(vp,vp - (MAX_X * 2),sx * 2);
		    vp -= (MAX_X * 2);
		}
		colset(sp,DEFCOL,sx * 2);
		break;
	    case 2:	/* 右方向 */
		vp = sp + (sx * 2) - 2;
		for ( i = 0 ; i < sy ; i++ ) {
		    vramrcpy(vp,vp - 2,sx * 2 - 2);
		    vp += (MAX_X * 2);
		}
		vp = sp;
		for ( i = 0 ; i < sy ; i++ ) {
		    colset(vp,DEFCOL,2);
		    vp += (MAX_X * 2);
		}
		break;
	    case 3:	/* 左方向 */
		vp = sp;
		for ( i = 0 ; i < sy ; i++ ) {
		    vramcpy(vp,vp + 2,sx * 2 - 2);
		    vp += (MAX_X * 2);
		}
		vp = sp + (sx * 2) - 2;
		for ( i = 0 ; i < sy ; i++ ) {
		    colset(vp,DEFCOL,2);
		    vp += (MAX_X * 2);
		}
		break;
            default: 	/* case 0 上方向 */
		for ( vp = sp,i = 0 ; i < (sy-1) ; i++ ) {
		    vramcpy(vp,vp + (MAX_X * 2),sx * 2);
		    vp += (MAX_X * 2);
		}
		colset(vp,DEFCOL,sx * 2);
		break;
	}
    }
    return;
ERROR:
    U_AH = 0x02;
    U_CF |= 0x0001;
}
void	VDB_1B(reg)	/* コンソ－ル機能の設定 */
register union _REGSET far *reg;
{
    int     i;

    i = Con_mode;
    Con_mode = (U_AL & 0xC7) | 0x08;
    if ( (i & 0x40) != (Con_mode & 0x40) ) {
	if ( (Con_mode & 0x40) != 0 )
	    SCR_Y--;
	else
	    SCR_Y++;
    }
}
void	VDB_1C(reg)	/* コンソ－ル機能の読み取り */
register union _REGSET far *reg;
{
    U_AL = Con_mode;
}
void	VDB_1D(reg)	/* 文字の出力 */
register union _REGSET far *reg;
{
    Chr_out(U_AL);
}
void	VDB_1E(reg)	/* 文字列の出力 */
register union _REGSET far *reg;
{
    int     i;
    union {
	UCHAR far	*p;
	unsigned short  s[2];
    } ptr;

    ptr.s[0] = U_DI;
    ptr.s[1] = U_DS;

    for ( i = U_CX ; i > 0 ; i-- )
	Chr_out(*(ptr.p++));
}
void	VDB_1F(reg)	/* システム行書き込み */
register union _REGSET far *reg;
{
    int    ch,cx,at,md;
    union {
	UNSIG far	*p;
	unsigned short  s[2];
    } pp;
    union {
	UCHAR far	*p;
	unsigned short  s[2];
    } cp,vp,ap;

    if ( (Con_mode & 0x40) == 0 )
	return;

    pp.s[0] = U_DI;
    pp.s[1] = U_DS;

    cp.s[0] = *(pp.p++);
    cp.s[1] = *(pp.p++);

    ap.s[0] = *(pp.p++);
    ap.s[1] = *(pp.p++);

    vp.s[0] = ((U_DL & 0x7F) - 1) * 2 + (MAX_X * 2) * SCR_Y;
    vp.s[1] = CVRAM;

    cx = U_CX;
    for ( ; cx > 0 ; cx-- ) {
	*(vp.p++) = *(cp.p++);
	if ( U_AL != 1 ) {
	    at = DEFCOL;
	    md = *(ap.p++);
	} else {
	    md = *(ap.p++);
	    at = *(ap.p++) & 0x38;
	    at |= (*(ap.p++) & 0x07); ap.p++;
	}
	*(vp.p++) = at;
	if ( md == 0x03 ) {
	    ch = (*(vp.p-4) << 8) | *(vp.p-2);
	    *(vp.p-4) = *(vp.p-2) = DMYKAN;
	    *(vp.p-3) |= 0x40;
	    vp.s[1] = KVRAM;
	    *(vp.p-4) = ch >> 8;
	    *(vp.p-3) = ch;
	    vp.s[1] = CVRAM;
	}
    }
}
void	VDB_20(reg)	/* 背景色の読み取り */
register union _REGSET far *reg;
{
    U_DX = BAKCOL;
}
void	VDB_21(reg)
register union _REGSET far *reg;
{
#ifdef	ANKCG
    int       i;
    UCHAR     *p;
    UCHAR far *s;

    SEGSET(s,U_DS);
    OFFSET(s,U_DI);
    p = Ank_font[U_DL];
    for ( i = U_CX ; i > 0 ; i-- )
	*(p++) = *(s++);
#endif
}

void	BIOS_91(reg)
register union _REGSET far *reg;
{
    int     cmd;
    static void (*VDB_table[])()={
	VDB_00,	VDB_01,	VDB_02,	VDB_03,	VDB_04,	VDB_05,	VDB_06,	VDB_07,
	VDB_08,	VDB_09,	VDB_0A,	VDB_0B,	VDB_0C,	VDB_0D,	VDB_0E,	VDB_0F,
	VDB_10,	VDB_11,	VDB_12,	VDB_13,	VDB_14,	VDB_15,	VDB_16,	VDB_17,
	VDB_18,	VDB_19,	VDB_1A,	VDB_1B,	VDB_1C,	VDB_1D,	VDB_1E,	VDB_1F,
	VDB_20, VDB_21 };

/*    Act_Dsp = TRUE;	*/
    if ( (cmd = U_AH) <= 0x21 ) {
	U_AH = 0;
	U_CF &= 0xFFFE;
	(*VDB_table[cmd])(reg);
    } else {
    	U_AH = 0x01;
    	U_CF |= 0x0001;
    }
/*    Act_Dsp = FALSE; */
}
