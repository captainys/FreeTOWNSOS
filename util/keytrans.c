#include <stdio.h>

#define true 1
#define false 0

enum
{
	TOWNS_JISKEY_NULL=   0,

	TOWNS_JISKEY_BREAK=  0x7C,
	TOWNS_JISKEY_COPY=   0x7D,
	TOWNS_JISKEY_PF01=   0x5D,
	TOWNS_JISKEY_PF02=   0x5E,
	TOWNS_JISKEY_PF03=   0x5F,
	TOWNS_JISKEY_PF04=   0x60,
	TOWNS_JISKEY_PF05=   0x61,
	TOWNS_JISKEY_PF06=   0x62,
	TOWNS_JISKEY_PF07=   0x63,
	TOWNS_JISKEY_PF08=   0x64,
	TOWNS_JISKEY_PF09=   0x65,
	TOWNS_JISKEY_PF10=   0x66,
	TOWNS_JISKEY_PF11=   0x69,
	TOWNS_JISKEY_PF12=   0x5B,

	TOWNS_JISKEY_PF13=   0x74,
	TOWNS_JISKEY_PF14=   0x75,
	TOWNS_JISKEY_PF15=   0x76,
	TOWNS_JISKEY_PF16=   0x77,
	TOWNS_JISKEY_PF17=   0x78,
	TOWNS_JISKEY_PF18=   0x79,
	TOWNS_JISKEY_PF19=   0x7A,
	TOWNS_JISKEY_PF20=   0x7B,

	TOWNS_JISKEY_KANJI_DIC= 0x6B,
	TOWNS_JISKEY_ERASE_WORD=0x6C,
	TOWNS_JISKEY_ADD_WORD=  0x6D,
	TOWNS_JISKEY_PREV=      0x6E,
	TOWNS_JISKEY_HOME=      0x4E,
	TOWNS_JISKEY_NEXT=      0x70,
	TOWNS_JISKEY_CHAR_PITCH=0x71,
	TOWNS_JISKEY_DELETE=    0x4B,
	TOWNS_JISKEY_INSERT=    0x48,

	TOWNS_JISKEY_UP=        0x4D,
	TOWNS_JISKEY_LEFT=      0x4F,
	TOWNS_JISKEY_RIGHT=     0x51,
	TOWNS_JISKEY_DOWN=      0x50,

	TOWNS_JISKEY_EXECUTE=   0x73,

	TOWNS_JISKEY_ESC=       0x01,
	TOWNS_JISKEY_1=         0x02,
	TOWNS_JISKEY_2=         0x03,
	TOWNS_JISKEY_3=         0x04,
	TOWNS_JISKEY_4=         0x05,
	TOWNS_JISKEY_5=         0x06,
	TOWNS_JISKEY_6=         0x07,
	TOWNS_JISKEY_7=         0x08,
	TOWNS_JISKEY_8=         0x09,
	TOWNS_JISKEY_9=         0x0A,
	TOWNS_JISKEY_0=         0x0B,
	TOWNS_JISKEY_MINUS=     0x0C,
	TOWNS_JISKEY_HAT=       0x0D,
	TOWNS_JISKEY_BACKSLASH= 0x0E,
	TOWNS_JISKEY_BACKSPACE= 0x0F,

	TOWNS_JISKEY_TAB=             0x10,
	TOWNS_JISKEY_Q=               0x11,
	TOWNS_JISKEY_W=               0x12,
	TOWNS_JISKEY_E=               0x13,
	TOWNS_JISKEY_R=               0x14,
	TOWNS_JISKEY_T=               0x15,
	TOWNS_JISKEY_Y=               0x16,
	TOWNS_JISKEY_U=               0x17,
	TOWNS_JISKEY_I=               0x18,
	TOWNS_JISKEY_O=               0x19,
	TOWNS_JISKEY_P=               0x1A,
	TOWNS_JISKEY_AT=              0x1B,
	TOWNS_JISKEY_LEFT_SQ_BRACKET= 0x1C,
	TOWNS_JISKEY_RETURN=          0x1D,

	TOWNS_JISKEY_CTRL=            0x52,
	TOWNS_JISKEY_A=               0x1E,
	TOWNS_JISKEY_S=               0x1F,
	TOWNS_JISKEY_D=               0x20,
	TOWNS_JISKEY_F=               0x21,
	TOWNS_JISKEY_G=               0x22,
	TOWNS_JISKEY_H=               0x23,
	TOWNS_JISKEY_J=               0x24,
	TOWNS_JISKEY_K=               0x25,
	TOWNS_JISKEY_L=               0x26,
	TOWNS_JISKEY_SEMICOLON=       0x27,
	TOWNS_JISKEY_COLON=           0x28,
	TOWNS_JISKEY_RIGHT_SQ_BRACKET=0x29,

	TOWNS_JISKEY_SHIFT=           0x53,
	TOWNS_JISKEY_Z=               0x2A,
	TOWNS_JISKEY_X=               0x2B,
	TOWNS_JISKEY_C=               0x2C,
	TOWNS_JISKEY_V=               0x2D,
	TOWNS_JISKEY_B=               0x2E,
	TOWNS_JISKEY_N=               0x2F,
	TOWNS_JISKEY_M=               0x30,
	TOWNS_JISKEY_COMMA=           0x31,
	TOWNS_JISKEY_DOT=             0x32,
	TOWNS_JISKEY_SLASH=           0x33,
	TOWNS_JISKEY_DOUBLEQUOTE=     0x34,

	TOWNS_JISKEY_HIRAGANA=        0x56,
	TOWNS_JISKEY_CAPS=            0x55,
	TOWNS_JISKEY_SPACE=           0x35,
	TOWNS_JISKEY_KANA_KANJI=      0x59,
	TOWNS_JISKEY_KATAKANA=        0x5A,
	TOWNS_JISKEY_CANCEL=          0x72,
	TOWNS_JISKEY_NO_CONVERT=      0x57,
	TOWNS_JISKEY_CONVERT=         0x58,

	TOWNS_JISKEY_NUM_STAR=        0x36,
	TOWNS_JISKEY_NUM_SLASH=       0x37,
	TOWNS_JISKEY_NUM_PLUS=        0x38,
	TOWNS_JISKEY_NUM_MINUS=       0x39,
	TOWNS_JISKEY_NUM_7=           0x3A,
	TOWNS_JISKEY_NUM_8=           0x3B,
	TOWNS_JISKEY_NUM_9=           0x3C,
	TOWNS_JISKEY_NUM_EQUAL=       0x3D,
	TOWNS_JISKEY_NUM_4=           0x3E,
	TOWNS_JISKEY_NUM_5=           0x3F,
	TOWNS_JISKEY_NUM_6=           0x40,
	TOWNS_JISKEY_NUM_DOT=         0x47,
	TOWNS_JISKEY_NUM_1=           0x42,
	TOWNS_JISKEY_NUM_2=           0x43,
	TOWNS_JISKEY_NUM_3=           0x44,
	TOWNS_JISKEY_NUM_RETURN=      0x45,
	TOWNS_JISKEY_NUM_0=           0x46,
	TOWNS_JISKEY_NUM_000=         0x4A,

	TOWNS_JISKEY_ALT=             0x5C, // Thanks, WINDY!
};

struct TownsKeyCombination
{
	unsigned char shift,ctrl;
	unsigned char keyCode;
};

struct TownsKeyCombination translation_data[256]=
{
	{false,false,TOWNS_JISKEY_NULL},                            // 0x00
	{false,false,TOWNS_JISKEY_NULL},                            // 0x01
	{false,false,TOWNS_JISKEY_NULL},                            // 0x02
	{false,false,TOWNS_JISKEY_NULL},                            // 0x03
	{false,false,TOWNS_JISKEY_NULL},                            // 0x04
	{false,false,TOWNS_JISKEY_NULL},                            // 0x05
	{false,false,TOWNS_JISKEY_NULL},                            // 0x06
	{false,false,TOWNS_JISKEY_NULL},                            // 0x07
	{false,false,TOWNS_JISKEY_BACKSPACE},                       // 0x08
	{false,false,TOWNS_JISKEY_TAB},                             // 0x09
	{false,false,TOWNS_JISKEY_NULL},                            // 0x0A
	{false,false,TOWNS_JISKEY_NULL},                            // 0x0B
	{false,false,TOWNS_JISKEY_NULL},                            // 0x0C
	{false,false,TOWNS_JISKEY_RETURN},                          // 0x0D
	{false,false,TOWNS_JISKEY_NULL},                            // 0x0E
	{false,false,TOWNS_JISKEY_NULL},                            // 0x0F

	{false,false,TOWNS_JISKEY_NULL},                            // 0x10
	{false,false,TOWNS_JISKEY_NULL},                            // 0x11
	{false,false,TOWNS_JISKEY_NULL},                            // 0x12
	{false,false,TOWNS_JISKEY_NULL},                            // 0x13
	{false,false,TOWNS_JISKEY_NULL},                            // 0x14
	{false,false,TOWNS_JISKEY_NULL},                            // 0x15
	{false,false,TOWNS_JISKEY_NULL},                            // 0x16
	{false,false,TOWNS_JISKEY_NULL},                            // 0x17
	{false,false,TOWNS_JISKEY_NULL},                            // 0x18
	{false,false,TOWNS_JISKEY_NULL},                            // 0x19
	{false,false,TOWNS_JISKEY_NULL},                            // 0x1A
	{false,false,TOWNS_JISKEY_NULL},                            // 0x1B
	{false,false,TOWNS_JISKEY_NULL},                            // 0x1C
	{false,false,TOWNS_JISKEY_NULL},                            // 0x1D
	{false,false,TOWNS_JISKEY_NULL},                            // 0x1E
	{false,false,TOWNS_JISKEY_NULL},                            // 0x1F

	{false,false,TOWNS_JISKEY_SPACE},                            // 0x20
	{true ,false,TOWNS_JISKEY_1},                            // 0x21 !
	{true ,false,TOWNS_JISKEY_2},                          // 0x22 "
	{true ,false,TOWNS_JISKEY_3},                            // 0x23 #
	{true ,false,TOWNS_JISKEY_4},                            // 0x24 $
	{true ,false,TOWNS_JISKEY_5},                            // 0x25 %
	{true ,false,TOWNS_JISKEY_6},                            // 0x26 &
	{true ,false,TOWNS_JISKEY_7},                            // 0x27 ' -> a?
	{true ,false,TOWNS_JISKEY_8},                            // 0x28 (
	{true ,false,TOWNS_JISKEY_9},                            // 0x29 )
	{true ,false,TOWNS_JISKEY_COLON},                           // 0x2A *
	{true ,false,TOWNS_JISKEY_SEMICOLON},                       // 0x2B +
	{false,false,TOWNS_JISKEY_COMMA},                            // 0x2C ,
	{false,false,TOWNS_JISKEY_MINUS},                            // 0x2D -
	{false,false,TOWNS_JISKEY_DOT},                            // 0x2E .
	{false,false,TOWNS_JISKEY_SLASH},                            // 0x2F /

	{false,false,TOWNS_JISKEY_0},                            // 0x30
	{false,false,TOWNS_JISKEY_1},                            // 0x31
	{false,false,TOWNS_JISKEY_2},                            // 0x32
	{false,false,TOWNS_JISKEY_3},                            // 0x33
	{false,false,TOWNS_JISKEY_4},                            // 0x34
	{false,false,TOWNS_JISKEY_5},                            // 0x35
	{false,false,TOWNS_JISKEY_6},                            // 0x36
	{false,false,TOWNS_JISKEY_7},                            // 0x37
	{false,false,TOWNS_JISKEY_8},                            // 0x38
	{false,false,TOWNS_JISKEY_9},                            // 0x39
	{false,false,TOWNS_JISKEY_COLON},                            // 0x3A :
	{false,false,TOWNS_JISKEY_SEMICOLON},                            // 0x3B ;
	{true ,false,TOWNS_JISKEY_COMMA},                            // 0x3C <
	{true ,false,TOWNS_JISKEY_MINUS},                            // 0x3D =
	{true ,false,TOWNS_JISKEY_DOT},                            // 0x3E >
	{true ,false,TOWNS_JISKEY_SLASH},                            // 0x3F ?

	{false,false,TOWNS_JISKEY_AT},                            // 0x40 @
	{true ,false,TOWNS_JISKEY_A},                            // 0x41 'A'
	{true ,false,TOWNS_JISKEY_B},                            // 0x42
	{true ,false,TOWNS_JISKEY_C},                            // 0x43
	{true ,false,TOWNS_JISKEY_D},                            // 0x44
	{true ,false,TOWNS_JISKEY_E},                            // 0x45
	{true ,false,TOWNS_JISKEY_F},                            // 0x46
	{true ,false,TOWNS_JISKEY_G},                            // 0x47
	{true ,false,TOWNS_JISKEY_H},                            // 0x48
	{true ,false,TOWNS_JISKEY_I},                            // 0x49
	{true ,false,TOWNS_JISKEY_J},                            // 0x4A
	{true ,false,TOWNS_JISKEY_K},                            // 0x4B
	{true ,false,TOWNS_JISKEY_L},                            // 0x4C
	{true ,false,TOWNS_JISKEY_M},                            // 0x4D
	{true ,false,TOWNS_JISKEY_N},                            // 0x4E
	{true ,false,TOWNS_JISKEY_O},                            // 0x4F

	{true ,false,TOWNS_JISKEY_P},                            // 0x50
	{true ,false,TOWNS_JISKEY_Q},                            // 0x51
	{true ,false,TOWNS_JISKEY_R},                            // 0x52
	{true ,false,TOWNS_JISKEY_S},                            // 0x53
	{true ,false,TOWNS_JISKEY_T},                            // 0x54
	{true ,false,TOWNS_JISKEY_U},                            // 0x55
	{true ,false,TOWNS_JISKEY_V},                            // 0x56
	{true ,false,TOWNS_JISKEY_W},                            // 0x57
	{true ,false,TOWNS_JISKEY_X},                            // 0x58
	{true ,false,TOWNS_JISKEY_Y},                            // 0x59
	{false,false,TOWNS_JISKEY_Z},                            // 0x5A
	{false,false,TOWNS_JISKEY_LEFT_SQ_BRACKET},                            // 0x5B [
	{false,false,TOWNS_JISKEY_BACKSLASH},                            // 0x5C (Back Slash)
	{false,false,TOWNS_JISKEY_RIGHT_SQ_BRACKET},                            // 0x5D ]
	{false,false,TOWNS_JISKEY_HAT},                            // 0x5E ^
	{true ,false,TOWNS_JISKEY_DOUBLEQUOTE},                            // 0x5F _

	{true ,false,TOWNS_JISKEY_AT},                            // 0x60 `
	{false,false,TOWNS_JISKEY_A},                            // 0x61
	{false,false,TOWNS_JISKEY_B},                            // 0x62
	{false,false,TOWNS_JISKEY_C},                            // 0x63
	{false,false,TOWNS_JISKEY_D},                            // 0x64
	{false,false,TOWNS_JISKEY_E},                            // 0x65
	{false,false,TOWNS_JISKEY_F},                            // 0x66
	{false,false,TOWNS_JISKEY_G},                            // 0x67
	{false,false,TOWNS_JISKEY_H},                            // 0x68
	{false,false,TOWNS_JISKEY_I},                            // 0x69
	{false,false,TOWNS_JISKEY_J},                            // 0x6A
	{false,false,TOWNS_JISKEY_K},                            // 0x6B
	{false,false,TOWNS_JISKEY_L},                            // 0x6C
	{false,false,TOWNS_JISKEY_M},                            // 0x6D
	{false,false,TOWNS_JISKEY_N},                            // 0x6E
	{false,false,TOWNS_JISKEY_O},                            // 0x6F
                              
	{false,false,TOWNS_JISKEY_P},                            // 0x70
	{false,false,TOWNS_JISKEY_Q},                            // 0x71
	{false,false,TOWNS_JISKEY_R},                            // 0x72
	{false,false,TOWNS_JISKEY_S},                            // 0x73
	{false,false,TOWNS_JISKEY_T},                            // 0x74
	{false,false,TOWNS_JISKEY_U},                            // 0x75
	{false,false,TOWNS_JISKEY_V},                            // 0x76
	{false,false,TOWNS_JISKEY_W},                            // 0x77
	{false,false,TOWNS_JISKEY_X},                            // 0x78
	{false,false,TOWNS_JISKEY_Y},                            // 0x79
	{false,false,TOWNS_JISKEY_Z},                            // 0x7A
	{true ,false,TOWNS_JISKEY_LEFT_SQ_BRACKET},                            // 0x7B {
	{true ,false,TOWNS_JISKEY_BACKSLASH},                            // 0x7C |
	{true ,false,TOWNS_JISKEY_RIGHT_SQ_BRACKET},                            // 0x7D }
	{true ,false,TOWNS_JISKEY_HAT},                            // 0x7E `
	{false,false,TOWNS_JISKEY_NULL},                            // 0x7F

	{false,false,TOWNS_JISKEY_NULL},                            // 0x80
	{false,false,TOWNS_JISKEY_NULL},                            // 0x81
	{false,false,TOWNS_JISKEY_NULL},                            // 0x82
	{false,false,TOWNS_JISKEY_NULL},                            // 0x83
	{false,false,TOWNS_JISKEY_NULL},                            // 0x84
	{false,false,TOWNS_JISKEY_NULL},                            // 0x85
	{false,false,TOWNS_JISKEY_NULL},                            // 0x86
	{false,false,TOWNS_JISKEY_NULL},                            // 0x87
	{false,false,TOWNS_JISKEY_NULL},                            // 0x88
	{false,false,TOWNS_JISKEY_NULL},                            // 0x89
	{false,false,TOWNS_JISKEY_NULL},                            // 0x8A
	{false,false,TOWNS_JISKEY_NULL},                            // 0x8B
	{false,false,TOWNS_JISKEY_NULL},                            // 0x8C
	{false,false,TOWNS_JISKEY_NULL},                            // 0x8D
	{false,false,TOWNS_JISKEY_NULL},                            // 0x8E
	{false,false,TOWNS_JISKEY_NULL},                            // 0x8F

	{false,false,TOWNS_JISKEY_NULL},                            // 0x90
	{false,false,TOWNS_JISKEY_NULL},                            // 0x91
	{false,false,TOWNS_JISKEY_NULL},                            // 0x92
	{false,false,TOWNS_JISKEY_NULL},                            // 0x93
	{false,false,TOWNS_JISKEY_NULL},                            // 0x94
	{false,false,TOWNS_JISKEY_NULL},                            // 0x95
	{false,false,TOWNS_JISKEY_NULL},                            // 0x96
	{false,false,TOWNS_JISKEY_NULL},                            // 0x97
	{false,false,TOWNS_JISKEY_NULL},                            // 0x98
	{false,false,TOWNS_JISKEY_NULL},                            // 0x99
	{false,false,TOWNS_JISKEY_NULL},                            // 0x9A
	{false,false,TOWNS_JISKEY_NULL},                            // 0x9B
	{false,false,TOWNS_JISKEY_NULL},                            // 0x9C
	{false,false,TOWNS_JISKEY_NULL},                            // 0x9D
	{false,false,TOWNS_JISKEY_NULL},                            // 0x9E
	{false,false,TOWNS_JISKEY_NULL},                            // 0x9F

	{false,false,TOWNS_JISKEY_NULL},                            // 0xA0
	{false,false,TOWNS_JISKEY_NULL},                            // 0xA1
	{false,false,TOWNS_JISKEY_NULL},                            // 0xA2
	{false,false,TOWNS_JISKEY_NULL},                            // 0xA3
	{false,false,TOWNS_JISKEY_NULL},                            // 0xA4
	{false,false,TOWNS_JISKEY_NULL},                            // 0xA5
	{false,false,TOWNS_JISKEY_NULL},                            // 0xA6
	{false,false,TOWNS_JISKEY_NULL},                            // 0xA7
	{false,false,TOWNS_JISKEY_NULL},                            // 0xA8
	{false,false,TOWNS_JISKEY_NULL},                            // 0xA9
	{false,false,TOWNS_JISKEY_NULL},                            // 0xAA
	{false,false,TOWNS_JISKEY_NULL},                            // 0xAB
	{false,false,TOWNS_JISKEY_NULL},                            // 0xAC
	{false,false,TOWNS_JISKEY_NULL},                            // 0xAD
	{false,false,TOWNS_JISKEY_NULL},                            // 0xAE
	{false,false,TOWNS_JISKEY_NULL},                            // 0xAF

	{false,false,TOWNS_JISKEY_NULL},                            // 0xB0
	{false,false,TOWNS_JISKEY_NULL},                            // 0xB1
	{false,false,TOWNS_JISKEY_NULL},                            // 0xB2
	{false,false,TOWNS_JISKEY_NULL},                            // 0xB3
	{false,false,TOWNS_JISKEY_NULL},                            // 0xB4
	{false,false,TOWNS_JISKEY_NULL},                            // 0xB5
	{false,false,TOWNS_JISKEY_NULL},                            // 0xB6
	{false,false,TOWNS_JISKEY_NULL},                            // 0xB7
	{false,false,TOWNS_JISKEY_NULL},                            // 0xB8
	{false,false,TOWNS_JISKEY_NULL},                            // 0xB9
	{false,false,TOWNS_JISKEY_NULL},                            // 0xBA
	{false,false,TOWNS_JISKEY_NULL},                            // 0xBB
	{false,false,TOWNS_JISKEY_NULL},                            // 0xBC
	{false,false,TOWNS_JISKEY_NULL},                            // 0xBD
	{false,false,TOWNS_JISKEY_NULL},                            // 0xBE
	{false,false,TOWNS_JISKEY_NULL},                            // 0xBF

	{false,false,TOWNS_JISKEY_NULL},                            // 0xC0
	{false,false,TOWNS_JISKEY_NULL},                            // 0xC1
	{false,false,TOWNS_JISKEY_NULL},                            // 0xC2
	{false,false,TOWNS_JISKEY_NULL},                            // 0xC3
	{false,false,TOWNS_JISKEY_NULL},                            // 0xC4
	{false,false,TOWNS_JISKEY_NULL},                            // 0xC5
	{false,false,TOWNS_JISKEY_NULL},                            // 0xC6
	{false,false,TOWNS_JISKEY_NULL},                            // 0xC7
	{false,false,TOWNS_JISKEY_NULL},                            // 0xC8
	{false,false,TOWNS_JISKEY_NULL},                            // 0xC9
	{false,false,TOWNS_JISKEY_NULL},                            // 0xCA
	{false,false,TOWNS_JISKEY_NULL},                            // 0xCB
	{false,false,TOWNS_JISKEY_NULL},                            // 0xCC
	{false,false,TOWNS_JISKEY_NULL},                            // 0xCD
	{false,false,TOWNS_JISKEY_NULL},                            // 0xCE
	{false,false,TOWNS_JISKEY_NULL},                            // 0xCF

	{false,false,TOWNS_JISKEY_NULL},                            // 0xD0
	{false,false,TOWNS_JISKEY_NULL},                            // 0xD1
	{false,false,TOWNS_JISKEY_NULL},                            // 0xD2
	{false,false,TOWNS_JISKEY_NULL},                            // 0xD3
	{false,false,TOWNS_JISKEY_NULL},                            // 0xD4
	{false,false,TOWNS_JISKEY_NULL},                            // 0xD5
	{false,false,TOWNS_JISKEY_NULL},                            // 0xD6
	{false,false,TOWNS_JISKEY_NULL},                            // 0xD7
	{false,false,TOWNS_JISKEY_NULL},                            // 0xD8
	{false,false,TOWNS_JISKEY_NULL},                            // 0xD9
	{false,false,TOWNS_JISKEY_NULL},                            // 0xDA
	{false,false,TOWNS_JISKEY_NULL},                            // 0xDB
	{false,false,TOWNS_JISKEY_NULL},                            // 0xDC
	{false,false,TOWNS_JISKEY_NULL},                            // 0xDD
	{false,false,TOWNS_JISKEY_NULL},                            // 0xDE
	{false,false,TOWNS_JISKEY_NULL},                            // 0xDF

	{false,false,TOWNS_JISKEY_NULL},                            // 0xE0
	{false,false,TOWNS_JISKEY_NULL},                            // 0xE1
	{false,false,TOWNS_JISKEY_NULL},                            // 0xE2
	{false,false,TOWNS_JISKEY_NULL},                            // 0xE3
	{false,false,TOWNS_JISKEY_NULL},                            // 0xE4
	{false,false,TOWNS_JISKEY_NULL},                            // 0xE5
	{false,false,TOWNS_JISKEY_NULL},                            // 0xE6
	{false,false,TOWNS_JISKEY_NULL},                            // 0xE7
	{false,false,TOWNS_JISKEY_NULL},                            // 0xE8
	{false,false,TOWNS_JISKEY_NULL},                            // 0xE9
	{false,false,TOWNS_JISKEY_NULL},                            // 0xEA
	{false,false,TOWNS_JISKEY_NULL},                            // 0xEB
	{false,false,TOWNS_JISKEY_NULL},                            // 0xEC
	{false,false,TOWNS_JISKEY_NULL},                            // 0xED
	{false,false,TOWNS_JISKEY_NULL},                            // 0xEE
	{false,false,TOWNS_JISKEY_NULL},                            // 0xEF

	{false,false,TOWNS_JISKEY_NULL},                            // 0xF0
	{false,false,TOWNS_JISKEY_NULL},                            // 0xF1
	{false,false,TOWNS_JISKEY_NULL},                            // 0xF2
	{false,false,TOWNS_JISKEY_NULL},                            // 0xF3
	{false,false,TOWNS_JISKEY_NULL},                            // 0xF4
	{false,false,TOWNS_JISKEY_NULL},                            // 0xF5
	{false,false,TOWNS_JISKEY_NULL},                            // 0xF6
	{false,false,TOWNS_JISKEY_NULL},                            // 0xF7
	{false,false,TOWNS_JISKEY_NULL},                            // 0xF8
	{false,false,TOWNS_JISKEY_NULL},                            // 0xF9
	{false,false,TOWNS_JISKEY_NULL},                            // 0xFA
	{false,false,TOWNS_JISKEY_NULL},                            // 0xFB
	{false,false,TOWNS_JISKEY_NULL},                            // 0xFC
	{false,false,TOWNS_JISKEY_NULL},                            // 0xFD
	{false,false,TOWNS_JISKEY_NULL},                            // 0xFE
	{false,false,TOWNS_JISKEY_NULL},                            // 0xFF
};


unsigned int keyCodeToASCII[256]; // 0 to 7Fh 

int main(void)
{
	int i,ascii;
	for(i=0; i<256; ++i)
	{
		keyCodeToASCII[i]=0;
	}

	for(ascii=0; ascii<256; ++ascii)
	{
		if(0!=translation_data[ascii].keyCode)
		{
			if(true==translation_data[ascii].shift)
			{
				keyCodeToASCII[translation_data[ascii].keyCode|0x80]=ascii;
			}
			else
			{
				keyCodeToASCII[translation_data[ascii].keyCode]=ascii;
			}
		}
	}

	printf("KEYCODE_TO_ASCII:\n");
	for(i=0; i<256; ++i)
	{
		if(0==i%16)
		{
			printf("\t\t\t\t\t\tDB\t\t");
		}
		printf("%02xH",keyCodeToASCII[i]);
		if(15!=i%16)
		{
			printf(",");
		}
		else
		{
			printf("\n");
		}
	}

	return 0;
}
