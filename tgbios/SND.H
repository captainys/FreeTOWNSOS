#ifndef SND_H_IS_INCLUDED
#define SND_H_IS_INCLUDED

#define SND_ERROR_MASK (~0xFF00)
#define SND_GENERAL_ERROR 0xFF00
#define SND_NO_ERROR 0
#define SND_SetError(reg,err) {SET_LOW_BYTE(&reg,err);}

struct SND_Work
{
	unsigned int FMKey,PCMKey;
};

#endif
