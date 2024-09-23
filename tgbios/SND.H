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


struct PCM_Voice_Header
{
	char name[8];
	unsigned int soundID;
	unsigned int totalBytes;
	unsigned int loopStart;
	unsigned int loopLength;
	unsigned short sampleFreq;
	unsigned short sampleFreqCorrection;
	unsigned char baseNote;
	unsigned char padding[3];
};

struct SND_PCM_Play_Info
{
	_Far struct PCM_Voice_Header *header;
	_Far unsigned char *playPtr;
	unsigned int curPos;
	unsigned char playing;
	unsigned char nextFillBank;
};

struct SND_Status
{
	unsigned int elevol_mute; // Set by SND BIOS.  In High-C library, it is accessed from SND_get_elevol_mute, but is done by 0110:01C0
	unsigned int YM2612_REG27H;
	unsigned short voiceModeBank;
	unsigned short usedBank;
	unsigned char numVoiceModeChannels;
	unsigned char PCMKey;
	unsigned char voiceChannelBank[SND_NUM_PCM_CHANNELS];
	struct SND_PCM_Play_Info pcmPlayInfo[SND_NUM_PCM_CHANNELS];
};

_Far struct SND_Status *SND_GetStatus(void);

void SND_PCM_Voice_Mode_Interrupt(void); // Called from SNDINT.C

void YM2612_Write(unsigned char regSet,unsigned char reg,unsigned char value);

void SND_FM_Timer_A_Restart(void);
void SND_FM_Timer_B_Restart(void);

#endif
