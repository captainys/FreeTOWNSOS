#include    <stdio.h>
#include    <stdlib.h>
#include    <jctype.h>

#define TRUE    1
#define FALSE   0
#define ERR     (-1)

#define UCHAR   unsigned char
#define SHORT   short int
#define	UNSIG	unsigned int

#define	CVRAM	0xC800
#define	KVRAM	0xCA00

#define	SCR_X	80
#define MAX_X   80
#define MAX_Y   25

#define DMYKAN  0xFE
#define CRCHR   0x1F
#define TABCHR  0x09
#define	NULCHR	0x20
#define TAB     8
#define CONTRL  FALSE
#define	DEFCOL	COLOR
#define	ESPSIZ	32

extern SHORT    OLD_X;
extern SHORT    CUR_X,CUR_Y;
extern SHORT    CUR_TYPE;
extern SHORT    CUR_OFF2,CUR_SIZE2;
extern UCHAR    CUR_DSP_FLG;
extern UCHAR    Con_mode;
extern SHORT    SCR_Y;
extern SHORT    COLOR,BAKCOL;

static UCHAR	TABMAP[MAX_X/8];
static UCHAR	BITMAP[8]={ 0x80,0x40,0x20,0x10,0x08,0x04,0x02,0x01 };

static SHORT	TABFLG=FALSE;
static SHORT    ODR_FLG=TRUE;
static SHORT	KANMOD=TRUE;
static UCHAR    BAKCH1=0;
static UCHAR    KANCOD=0;
static SHORT	EXTFLG=FALSE;
static void	(*EXTPRO)();
static SHORT	ESCCNT=0;
static char	ESCPRM[ESPSIZ];
static SHORT    BAK_X,BAK_Y;

void    SetDC2_s();
void    SetDC3_s();
void    SCurPs_s();
void    AnsiCom();
void    SCurPs();
void	SVidAt();
void	CurAtt();
void	SetDC2();
void	SetDC3();
void	EscCom();
void	Beep();

static void    NextChr(sub)
void     (*sub)();
{
    EXTFLG = TRUE;
    EXTPRO = sub;
}
static void    Scrool()
{
    vramcpy(0,(MAX_X * 2),(MAX_X * 2) * (SCR_Y - 1));
    colset((MAX_X * 2) * (SCR_Y - 1),COLOR,(MAX_X * 2));
}
static void	Put_Ank(ch)
int	ch;
{
    register union {
	UCHAR far	*p;
	unsigned short  s[2];
    } ptr;

    ptr.s[0] = (CUR_X * 2 + (MAX_X * 2) * CUR_Y);
    ptr.s[1] = CVRAM;
    *(ptr.p) = ch;
    *(ptr.p+1) = COLOR;

    if ( CUR_X > 0 && (*(ptr.p-1) & 0x40) != 0 )
	*(ptr.p-1) &= 0xBF;

    if ( ++CUR_X >= SCR_X ) {
        if ( ODR_FLG != FALSE ) {
	    CUR_X = 0;
	    if ( ++CUR_Y >= SCR_Y ) {
                CUR_Y = SCR_Y - 1;
                Scrool();
            }
	} else
	    CUR_X = SCR_X - 1;
    }
}
static void	Put_Kan(ch)
int	ch;
{
    register union {
	UCHAR far	*p;
	unsigned short  s[2];
    } ptr;

    if ( CUR_X >= (SCR_X - 1) ) {
	Put_Ank(DMYKAN);
	Put_Ank(DMYKAN);
	return;
/*****************************************
        if ( ODR_FLG != FALSE ) {
	    CUR_X = 0;
	    if ( ++CUR_Y >= SCR_Y ) {
                CUR_Y = SCR_Y - 1;
                Scrool();
            }
	} else
	    CUR_X = SCR_X - 2;
*******************************************/
    }

    ch = sjisto(ch);

    ptr.s[0] = (CUR_X * 2 + (MAX_X * 2) * CUR_Y);
    ptr.s[1] = CVRAM;

    *ptr.p = DMYKAN;
    *(ptr.p+1) = 0x40|COLOR;
    *(ptr.p+2) = DMYKAN;
    *(ptr.p+3) = COLOR;

    if ( CUR_X > 0 && (*(ptr.p-1) & 0x40) != 0 )
	*(ptr.p-1) &= 0xBF;

    ptr.s[1] = KVRAM;
    *(ptr.p) = ch >> 8;
    *(ptr.p+1) = ch;
    *(ptr.p+2) = 0;
    *(ptr.p+3) = 0;

    if ( (CUR_X += 2) >= SCR_X ) {
        if ( ODR_FLG != FALSE ) {
	    CUR_X = 0;
	    if ( ++CUR_Y >= SCR_Y ) {
                CUR_Y = SCR_Y - 1;
                Scrool();
            }
	} else
	    CUR_X = SCR_X - 1;
    }
}
static void    PutBS()
{
    if ( --CUR_X < 0 ) {
        CUR_X = SCR_X - 1;
        if ( --CUR_Y < 0 )
            CUR_Y = 0;
    }
}
static void    PutTAB()
{
    int     i;

    if ( TABFLG == FALSE || (Con_mode & 0x02) == 0 ) {
        Put_Ank(' ');
        return;
    }

    for ( i = CUR_X ; i < SCR_X ; i++ ) {
        Put_Ank(' ');
        if ( (TABMAP[CUR_X / 8] & BITMAP[CUR_X % 8]) != 0 )
            break;
    }
}
static void    PutLF()
{
    if ( ++CUR_Y >= SCR_Y ) {
	CUR_Y = (SCR_Y - 1);
	Scrool();
    }
    if ( (Con_mode & 0x01) != 0 )
	CUR_X = 0;
}
static void    PutHOME()
{
    CUR_X = CUR_Y = 0;
}
static void    PutCLS()
{
    colset(0,COLOR,(MAX_X * 2) * SCR_Y);
    CUR_X = CUR_Y = 0;
}
static void    PutCR()
{
    CUR_X = 0;
    if ( (Con_mode & 0x01) != 0 )
	PutLF();
}
static void    CurRit()
{
    if ( ++CUR_X >= SCR_X )
        CUR_X = 0;
}
static void    CurLft()
{
    if ( --CUR_X < 0 )
        CUR_X = SCR_X - 1;
}
static void    CurUp()
{
    if ( --CUR_Y < 0 )
	CUR_Y = SCR_Y - 1;
}
static void    CurDwn()
{
    if ( ++CUR_Y >= SCR_Y )
        CUR_Y = 0;
}
static void    SVidAt(ch)
int     ch;
{
    COLOR = ch & 0x3F;
}
static void    SetDC2_s(ch)
int     ch;
{
    CUR_X = BAKCH1;
    CUR_Y = ch;
    if ( CUR_X < 0 ) CUR_X = 0;
    if ( CUR_X >= SCR_X ) CUR_X = SCR_X - 1;
    if ( CUR_Y < 0 ) CUR_Y = 0;
    if ( CUR_Y >= SCR_Y ) CUR_Y = SCR_Y - 1;
}
static void    SetDC2(ch)
int     ch;
{
    BAKCH1 = ch;
    NextChr(SetDC2_s);
}
static void    SetDC3_s(ch)
int     ch;
{
    while ( BAKCH1-- > 0 )
        Put_Ank(ch);
}
static void    SetDC3(ch)
int     ch;
{
    BAKCH1 = ch;
    NextChr(SetDC3_s);
}
static void    EraLin()
{
    colset((CUR_X * 2) + (MAX_X * 2) * CUR_Y,COLOR,(MAX_X - CUR_X) * 2);
}
static void    EraScr()
{
    colset((CUR_X * 2) + (MAX_X * 2) * CUR_Y,COLOR,
	   (MAX_X * 2) * SCR_Y - ((CUR_X * 2) + (MAX_X * 2) * CUR_Y));
}
static void    InsLin()
{
    int     i;
    UNSIG   p;

    p = (MAX_X * 2) * (SCR_Y - 1);
    for ( i = SCR_Y - 1 ; i > CUR_Y ; i-- ) {
        vramcpy(p,p-(MAX_X * 2),(MAX_X * 2));
        p -= (MAX_X * 2);
    }
    colset((MAX_X * 2) * CUR_Y,COLOR,MAX_X * 2);
}
static void    DelLin()
{
    int     i;
    UNSIG   p;

    p = (MAX_X * 2) * CUR_Y;
    for ( i = CUR_Y ; i < (SCR_Y - 1) ; i ++ ) {
        vramcpy(p,p+(MAX_X * 2),(MAX_X * 2));
        p += (MAX_X * 2);
    }
    colset((MAX_X * 2) * (SCR_Y - 1),COLOR,MAX_X * 2);
}
static void    SCurPs_s(ch)
int     ch;
{
    CUR_Y = BAKCH1 - ' ';
    CUR_X = ch - ' ';
    if ( CUR_X < 0 ) CUR_X = 0;
    if ( CUR_X >= SCR_X ) CUR_X = SCR_X - 1;
    if ( CUR_Y < 0 ) CUR_Y = 0;
    if ( CUR_Y >= SCR_Y ) CUR_Y = SCR_Y - 1;
}
static void    SCurPs(ch)
int     ch;
{
    BAKCH1 = ch;
    NextChr(SCurPs_s);
}
static void	CurAtt_s(ch)
{
    CUR_TYPE = BAKCH1 & 0x70;
    CUR_OFF2 = BAKCH1 & 0x1F;
    CUR_SIZE2 = (ch & 0x1F) - CUR_OFF2;
    if ( CUR_OFF2 < 0 || CUR_OFF2 > 15 )
	CUR_OFF2 = 15;
    if ( CUR_SIZE2 < 0 || (CUR_SIZE2 + CUR_OFF2) > 16 )
	CUR_SIZE2 = 16 - CUR_OFF2;
    CUR_DSP_FLG = ((BAKCH1 & 0x60) == 0x20 ? 1:0);
    OLD_X = 0xFFFF;
}
static void    CurAtt(ch)
int	ch;
{
    BAKCH1 = ch;
    NextChr(CurAtt_s);
}
static void     KanjiIn(ch)
int     ch;
{
    if ( ch != 'B' && ch != '@' )
        Put_Ank(ch);
}
static void     KanjiOut(ch)
int     ch;
{
    if ( ch != 'B' && ch != 'J' )
        Put_Ank(ch);
}
static void	CurPos()
{
    ESCPRM[0] = CUR_Y + ' ';
    ESCPRM[1] = CUR_X + ' ';
    ESCPRM[2] = 0x0D;
    KeyBufIns(ESCPRM,3);
}
static void     TabSet()
{
    TABMAP[CUR_X / 8] |= BITMAP[CUR_X % 8];
    TABFLG = TRUE;
}
static void     TabReSet()
{
    TABMAP[CUR_X / 8] &= (~BITMAP[CUR_X % 8]);
}
static void	TabClr()
{
    memset(TABMAP,0,MAX_X/8);
    TABFLG = FALSE;
}

/*****************************************************************
        ANSIエスケ－プシ－ケンス制御関数群
******************************************************************/

static void    AnsiA()
{
    if ( ESCPRM[0] == 0 ) ESCPRM[0] = 1;
    if ( (CUR_Y -= ESCPRM[0]) < 0 )
        CUR_Y = 0;
}
static void    AnsiB()
{
    if ( ESCPRM[0] == 0 ) ESCPRM[0] = 1;
    if ( (CUR_Y += ESCPRM[0]) >= SCR_Y )
        CUR_Y = SCR_Y - 1;
}
static void    AnsiC()
{
    if ( ESCPRM[0] == 0 ) ESCPRM[0] = 1;
    if ( (CUR_X += ESCPRM[0]) >= SCR_X )
        CUR_X = SCR_X - 1;
}
static void    AnsiD()
{
    if ( ESCPRM[0] == 0 ) ESCPRM[0] = 1;
    if ( (CUR_X -= ESCPRM[0]) < 0 )
        CUR_X = 0;
}
static void    AnsiG()
{
    if ( ESCPRM[0] > 0 ) ESCPRM[0]--;
    CUR_X = ESCPRM[0];
    if ( CUR_X < 0 )      CUR_X = 0;
    if ( CUR_X >= SCR_X ) CUR_X = SCR_X - 1;
}
static void    AnsiH()
{
    if ( ESCPRM[0] > 0 ) ESCPRM[0]--;
    if ( ESCPRM[1] > 0 ) ESCPRM[1]--;
    CUR_Y = ESCPRM[0];
    CUR_X = ESCPRM[1];
    if ( CUR_X < 0 ) CUR_X = 0;
    if ( CUR_X >= SCR_X ) CUR_X = SCR_X - 1;
    if ( CUR_Y < 0 ) CUR_Y = 0;
    if ( CUR_Y >= SCR_Y ) CUR_Y = SCR_Y - 1;
}
static void    AnsiJ()
{
    switch(ESCPRM[0]) {
        case 0: EraScr(); break;
        case 1: colset(0,COLOR,(CUR_X * 2) + (MAX_X * 2) * CUR_Y); break;
        case 2: PutCLS(); break;
    }
}
static void    AnsiK()
{
    switch(ESCPRM[0]) {
        case 0: EraLin(); break;
        case 1:
            if ( CUR_X > 0 )
		colset((MAX_X * 2) * CUR_Y,COLOR,CUR_X * 2);
            break;
        case 2: colset((MAX_X * 2) * CUR_Y,COLOR,MAX_X * 2); break;
    }
}
static void    AnsiL()
{
    do {
        InsLin();
    } while ( --ESCPRM[0] > 0 );
    CUR_X = 0;
}
static void    AnsiM()
{
    do {
        DelLin();
    } while ( --ESCPRM[0] > 0 );
    CUR_X = 0;
}
static void     AnsiP()
{
    int     n;

    if ( (n = ESCPRM[0]) <= 0 ) n = 1;
    if ( (n + CUR_X) > SCR_X ) n = SCR_X - CUR_X;

    vramrcpy((CUR_X + SCR_X * CUR_Y) * 2,
	     (CUR_X + n + SCR_X * CUR_Y) * 2,
	     (SCR_X - CUR_X - n) * 2);
    colset((SCR_X - n + SCR_X * CUR_Y) * 2,COLOR,n * 2);
}
static void    Ansid()
{
    if ( ESCPRM[0] > 0 ) ESCPRM[0]--;
    CUR_Y = ESCPRM[0];
    if ( CUR_Y < 0 )      CUR_Y = 0;
    if ( CUR_Y >= SCR_Y ) CUR_Y = SCR_Y - 1;
}
static void     Ansig()
{
    switch(ESCPRM[0]) {
    case 2:
	TabClr();
        break;
    }
}
static void    Ansis()
{
    BAK_X = CUR_X;
    BAK_Y = CUR_Y;
}
static void    Ansiu()
{
    CUR_X = BAK_X;
    CUR_Y = BAK_Y;
}
static void    Ansiv()
{
    CUR_DSP_FLG = ESCPRM[0];
    OLD_X = 0xFFFF;
}
static void    Ansim()
{
    int     c;
    int     i,n;
    static char ansiatt[]={ 0x00,0x20,0x00,0x00,0x00,0x10,0x00,0x08,0x00 };
    static char ansicol[]={ 0,2,4,6,1,3,5,7 };
 
    for ( n = 0,c = 0x07 ; n < ESCCNT ; n++ ) {
        if ( (i = ESCPRM[n]) > 0 && i <= 8 ) {
            c |= ansiatt[i];
        } else if ( i == 0 ) {
	    c &= 0x07;
	} else if ( i >= 30 && i <= 38 ) {
            c &= 0xF8;
            c |= ansicol[i-30];
        } else if ( i >= 40 && i <= 47 ) {
            BAKCOL = ansicol[i-40];		/* 現在未サポ－ト */
        }
    }
    COLOR = c;
}
static void	Ansin()
{
    int       i,n;

    if ( ESCPRM[0] != 6 )
	return;

    n = 0;
    ESCPRM[n++] = '\033';
    ESCPRM[n++] = '[';

    if ( (i = CUR_Y + 1) >= 10 )
	ESCPRM[n++] = '0' + i / 10;
    ESCPRM[n++] = '0' + i % 10;

    ESCPRM[n++] = ';';

    if ( (i = CUR_X + 1) >= 10 )
	ESCPRM[n++] = '0' + i / 10;
    ESCPRM[n++] = '0' + i % 10;

    ESCPRM[n++] = 'R';

    KeyBufIns(ESCPRM,n);
}
static void	AnsKeyAs()
{
    int	    ch,i;

    i = 0;
    if ( (ch = (UCHAR)ESCPRM[i++]) == 0 )
	ch = 0x8000 | (UCHAR)ESCPRM[i++];
    if ( (ESCCNT - i) < 1 )
	return;
    AsinKey(ch,ESCCNT - i,&ESCPRM[i]);		/* Compile to BIOS.ASM */
}
static void     AnsiIc()
{
    int     i,n;
    char    *p,*s;

    if ( (n = ESCPRM[0]) <= 0 ) n = 1;
    if ( (n + CUR_X) > SCR_X ) n = SCR_X - CUR_X;

    vramrcpy((SCR_X - 1 + SCR_X * CUR_Y) * 2,
	     (SCR_X - 1 - n + SCR_X * CUR_Y) * 2,
	     (SCR_X - CUR_X - n) * 2);
    colset((CUR_X + SCR_X * CUR_Y) * 2,COLOR,n * 2);
}
static void	AnsStr(ch)
int	ch;
{
    if ( ch == '\"' )
	NextChr(AnsiCom);
    else {
	if ( ESCCNT < ESPSIZ )
	    ESCPRM[ESCCNT++] = ch;
	NextChr(AnsStr);
    }
}
static void    AnsiCom(ch)
int     ch;
{
    if ( ch == ';' && ESCCNT < ESPSIZ )
        ESCCNT++;

    else if ( ch >= '0' && ch <= '9' )
        ESCPRM[ESCCNT] = ESCPRM[ESCCNT] * 10 + (ch - '0');

    else if ( ch != ' ' ) {
        ESCCNT++;
        switch(ch) {
            case 'A': AnsiA(); break;
            case 'B': AnsiB(); break;
            case 'C': AnsiC(); break;
            case 'D': AnsiD(); break;
            case 'G': AnsiG(); break;
            case 'H': AnsiH(); break;
            case 'J': AnsiJ(); break;
            case 'K': AnsiK(); break;
            case 'L': AnsiL(); break;
            case 'M': AnsiM(); break;
            case 'P': AnsiP(); break;

            case 'd': Ansid(); break;
            case 'f': AnsiH(); break;
            case 'g': Ansig(); break;
            case 'm': Ansim(); break;
	    case 'n': Ansin(); break;
	    case 'p': AnsKeyAs(); break;
            case 's': Ansis(); break;
            case 'u': Ansiu(); break;
	    case 'v': Ansiv(); break;
            case '@': AnsiIc(); break;
	    case '\"': NextChr(AnsStr); break;

            default:  Put_Ank(ch); break;
        }
        return;
    }
    NextChr(AnsiCom);
}
static void	AnsiEqu(ch)
{
    if ( ch >= '0' && ch <= '9' ) {
        ESCPRM[0] = ESCPRM[0] * 10 + (ch - '0');
	NextChr(AnsiEqu);

    } else if ( ch == ' ' ) {
	NextChr(AnsiEqu);

    } else if ( ch == 'h' ) {
	switch(ESCPRM[0]) {
	case 0: case 1:
	case 2: case 3:
	    PutCLS();
	    break;
	case 7:
	    ODR_FLG = TRUE;
	    break;
	}

    } else if ( ch == 'l' ) {
	switch(ESCPRM[0]) {
	case 7:
	    ODR_FLG = FALSE;
	    break;
	}
    } else
	Put_Ank(ch);
}
static void	AnsiChk(ch)
{
    switch(ch) {
    case '=':
    case '?':
	NextChr(AnsiEqu);
	break;
    case ' ':
	NextChr(AnsiChk);
	break;
    default:
	AnsiCom(ch);
	break;
    }
}
static void    AnsiESC()
{
    memset(ESCPRM,0,ESPSIZ);
    ESCCNT = 0;
    NextChr(AnsiChk);
}
static void    EscCom(ch)
int     ch;
{
    switch(ch) {
        case '1': TabSet(); break;
        case '2': TabReSet(); break;
        case '3': TabClr(); break;
        case 'C': KANMOD = FALSE; break;
        case 'E': InsLin(); break;
        case 'G': NextChr(SVidAt); break;
        case 'H': TabSet(); break;
        case 'K': KANMOD = TRUE; break;
        case 'R': DelLin(); break;
        case 'T': EraLin(); break;
        case 'Y': EraScr(); break;
        case '*': PutCLS(); break;
        case '=': NextChr(SCurPs); break;
	case '.': NextChr(CurAtt); break;
        case '[': AnsiESC(); break;
        case '$': NextChr(KanjiIn); break;
        case '(': NextChr(KanjiOut); break;
	case '?': CurPos(); break;
        default: Put_Ank(ch); break;
    }
}
static void    cSetDC2()
{
    NextChr(SetDC2);
}
static void    cSetDC3()
{
    NextChr(SetDC3);
}
static void    cEscCom()
{
    NextChr(EscCom);
}
void    Chr_out(ch)
int     ch;
{
    static void (*sub[])()={
        NULL,   NULL,   NULL,   NULL,
        NULL,   NULL,   NULL,   Beep,
        PutBS,  PutTAB, PutLF,  PutHOME,
        PutCLS, PutCR,  NULL,   NULL,
        NULL,   NULL,   cSetDC2,cSetDC3,
        NULL,   PutCLS, PutCLS, NULL,
        NULL,   NULL,   NULL,   cEscCom,
        CurRit, CurLft, CurUp,  CurDwn
    };

    ch &= 0xFF;

    if ( EXTFLG != FALSE ) {
	EXTFLG = FALSE;
	(*EXTPRO)(ch);
        return;
    }

    if ( KANCOD != 0 ) {
        if ( iskanji2(ch) ) {
            Put_Kan((KANCOD << 8) | ch);
            KANCOD = 0;
            return;
        }
        Put_Ank(KANCOD);
        KANCOD = 0;
    }

    if ( ch < 0x20 && sub[ch] != NULL && (Con_mode & 0x80) != 0 ) {
        (*sub[ch])();

    } else {
        if ( KANMOD != FALSE && iskanji(ch) )
            KANCOD = ch;
        else if ( ch != '\0' )
            Put_Ank(ch);
    }
}
void	Con_init()
{
    int     i;

    memset(TABMAP,0,MAX_X/8);
    for ( i = 0 ; i < SCR_X ; i += TAB )
        TABMAP[ i / 8] |= BITMAP[ i % 8];
    TABFLG = TRUE;
}
