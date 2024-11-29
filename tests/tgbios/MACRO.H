#ifndef MACRO_H_IS_INCLUDED
#define MACRO_H_IS_INCLUDED

#define TSUGARU_BREAK _inline(0xE6,0xEA);
#define TSUGARU_STATE _inline(0xE6,0xEB);

#define _PUSHFD _inline(0x9C);
#define _POPFD _inline(0x9D);
#define _CLI _inline(0xfa);
#define _STI _inline(0xfb);

#define _PUSH_SS _inline(0x16);
#define _PUSH_DS _inline(0x1E);
#define _PUSH_FS _inline(0x0F,0xA0);
#define _PUSH_GS _inline(0x0F,0xA8);
#define _POP_DS  _inline(0x1F);
#define _POP_FS  _inline(0x0F,0xA1);
#define _POP_GS  _inline(0x0F,0xA9);

#define _WAIT1US _inline(0xE6,0x6C);

#define _WAITxUS(x) _inline(0x51,0xB9,x,0,0,0,0xE6,0x6C,0xE2,0xFC,0x59)
// 00000000  51                            			PUSH	ECX
// 00000001  B9 00000050                   			MOV	ECX,80
// 00000006                                LP:
// 00000006  E6 6C                         			OUT	6CH,AL
// 00000008  E2 FC                         			LOOP	LP
// 0000000A  59                            			POP	ECX



#endif
