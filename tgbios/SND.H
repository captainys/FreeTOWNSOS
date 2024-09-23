#ifndef SND_H_IS_INCLUDED
#define SND_H_IS_INCLUDED

#define SND_ERROR_MASK (~0xFF00)
#define SND_GENERAL_ERROR 0xFF00
#define SND_NO_ERROR 0
#define SND_ERROR_WRONG_CH       1
#define SND_ERROR_KEY_ALREADY_ON 2
#define SND_ERROR_PARAMETER      3
#define SND_ERROR_OUT_OF_PCM_RAM 4
#define SND_ERROR_NO_DATA_LENGTH 6
#define SND_ERROR_CH_NOT_VOICE_MODE 7
#define SND_ERROR_BAD_DATA_HEADER 8

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

#define PCM_BANK_SIZE 4096
#define PCM_LOOP_STOP_CODE 0xFF

#endif
