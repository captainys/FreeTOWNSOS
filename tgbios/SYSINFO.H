#ifndef SYSINFO_H_IS_INCLUDED
#define SYSINFO_H_IS_INCLUDED

#include "TGBIOS.H"

struct TBIOS_System_Info
{
	// Mouse
	unsigned mouseINTCount;

	// Misc.
	unsigned INTVec[4];       // Apparently just saving RIDT and IDT of INT 4DH for later restoring.

	// A Duck.
	unsigned int _quack;
};

_Far struct TBIOS_System_Info *SYSINFO_GetStruct(void);

#endif
