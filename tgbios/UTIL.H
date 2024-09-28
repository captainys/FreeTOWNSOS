#ifndef UTIL_H_IS_INCLUDED
#define UTIL_H_IS_INCLUDED

void MEMSETB_FAR(_Far void *ptr,unsigned int value,unsigned int count);
void MEMSETW_FAR(_Far void *ptr,unsigned int value,unsigned int count);  // Count is count.  Not number of bytes.  Be careful.
void MEMCPY_FAR(_Far void *dst,const _Far void *src,unsigned int bytes);

// MOVSB_FAR only uses MOVSB.  It does not use MOVSD or MOVSW no matter how large count is.
// Good for wave-RAM access.
void MOVSB_FAR(_Far void *dst,_Far void *src,unsigned int count);

void SET_SECOND_BYTE(void *ptr,unsigned int data);
void SET_LOW_BYTE(void *ptr,unsigned int data);
void SET_LOW_WORD(void *ptr,unsigned int data);
void SET_DWORD(void *ptr,unsigned int data);

unsigned int MULDIV(unsigned int a,unsigned int b,unsigned int c);
int IMULDIV(int a,int b,int c);

#endif
