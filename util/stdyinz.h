#ifndef STDYINZ_H_IS_INCLUDED
#define STDYINZ_H_IS_INCLUDED

#ifdef __HIGHC__
	typedef unsigned long int uint32_t;
	typedef unsigned short int uint16_t;
	typedef unsigned char uint8_t;
#else
	#include <stdint.h>
#endif

#endif
