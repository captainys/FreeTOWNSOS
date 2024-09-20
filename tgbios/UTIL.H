#ifndef UTIL_H_IS_INCLUDED
#define UTIL_H_IS_INCLUDED

void MEMSETB_FAR(_Far void *ptr,unsigned int value,unsigned int count);
void MEMSETW_FAR(_Far void *ptr,unsigned int value,unsigned int count);

void SET_SECOND_BYTE(void *ptr,unsigned int data);
void SET_LOW_BYTE(void *ptr,unsigned int data);
void SET_LOW_WORD(void *ptr,unsigned int data);
void SET_DWORD(void *ptr,unsigned int data);

#endif
