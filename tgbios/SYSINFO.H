#ifndef SYSINFO_H_IS_INCLUDED
#define SYSINFO_H_IS_INCLUDED

struct TBIOS_System_Info
{
	unsigned int elevol_mute; // Set by SND BIOS.  In High-C library, it is accessed from SND_get_elevol_mute, but is done by 0110:01C0
	unsigned mouseINTCount;
	unsigned INTVec[4];       // Apparently just saving RIDT and IDT of INT 4DH for later restoring.
	unsigned int _quack;
};

_Far struct TBIOS_System_Info *SYSINFO_GetStruct(void);

#endif
