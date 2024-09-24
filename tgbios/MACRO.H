#ifndef MACRO_H_IS_INCLUDED
#define MACRO_H_IS_INCLUDED

#define TSUGARU_BREAK _inline(0xE6,0xEA);
#define TSUGARU_STATE _inline(0xE6,0xEB);

#define _PUSHFD _inline(0x9C);
#define _POPFD _inline(0x9D);
#define _CLI _inline(0xfa);

#define _PUSH_FS _inline(0x0F,0xA0);
#define _PUSH_GS _inline(0x0F,0xA8);
#define _POP_FS  _inline(0x0F,0xA1);
#define _POP_GS  _inline(0x0F,0xA9);

#endif
