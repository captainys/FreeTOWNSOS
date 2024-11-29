#ifndef PLTDEBUG_H_IS_INCLUDED
#define PLTDEBUG_H_IS_INCLUDED


#define PLTDEBUG(c,b,r,g) \
{ \
	_outb(0x448,1); \
	_outb(0x44A,0x25); \
	_outb(0xFD90,c); \
	_outb(0xFD92,b); \
	_outb(0xFD94,r); \
	_outb(0xFD96,g); \
	_outb(0x448,1); \
	_outb(0x44A,0x05); \
	_outb(0xFD90,c); \
	_outb(0xFD92,b); \
	_outb(0xFD94,r); \
	_outb(0xFD96,g); \
}


#endif
