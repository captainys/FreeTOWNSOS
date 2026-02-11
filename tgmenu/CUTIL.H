#ifndef CUTIL_H_IS_INCLUDED
#define CUTIL_H_IS_INCLUDED

#include <stdio.h>

/*!
Unsigned Int to ASCII fixed length.
Len is the length of the string excluding \0.
The return value is how many letters have been used for the numbers.
*/
size_t Uitoa_fl(char *str,size_t len,unsigned int i);

#endif
