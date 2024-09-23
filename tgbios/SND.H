#ifndef SND_H_IS_INCLUDED
#define SND_H_IS_INCLUDED

#define SND_ERROR_MASK (~0xFF00)
#define SND_GENERAL_ERROR 0xFF00
#define SND_NO_ERROR 0
#define SND_SetError(reg,err) {SET_LOW_BYTE(&reg,err);}

// Active-Low (or zero means mute)
#define SND_MUTE_PCM    1
#define SND_MUTE_FM     2
#define SND_MUTE_LINE_R 4
#define SND_MUTE_LINE_L 8
#define SND_MUTE_CD_R   16
#define SND_MUTE_CD_L   32
#define SND_MUTE_MIC    64
#define SND_MUTE_MODEM  128

struct SND_Work
{
	unsigned char quack;
};

#endif
