#ifndef SND_H_IS_INCLUDED
#define SND_H_IS_INCLUDED

#define SND_ERROR_MASK (~0xFF00)
#define SND_GENERAL_ERROR 0xFF00
#define SND_NO_ERROR 0
#define SND_ERROR_WRONG_CH       1
#define SND_ERROR_KEY_ALREADY_ON 2
#define SND_ERROR_PARAMETER      3
#define SND_ERROR_OUT_OF_PCM_RAM 4
#define SND_ERROR_OUT_OF_PCM_RAM2  5  // WTF?
#define SND_ERROR_NO_DATA_LENGTH 6
#define SND_ERROR_CH_NOT_VOICE_MODE 7
#define SND_ERROR_BAD_DATA_HEADER 8

#define SND_SetError(reg,err) {SET_LOW_BYTE(&reg,err);}

#define SND_Is_FM_Channel(ch) ((ch)<6)
#define SND_Is_PCM_Channel(ch) (64<=(ch) && (ch)<=71)

#define SND_FM_CHANNEL_START  0
#define SND_PCM_CHANNEL_START 64

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

#define PCM_WAVE_RAM_SIZE 0x10000
#define PCM_BANK_SIZE 4096
#define PCM_LOOP_STOP_CODE 0xFF

#define FM_NUM_INSTRUMENTS 128
#define PCM_NUM_INSTRUMENTS 32
#define PCM_MAX_NUM_SOUND       32

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

struct FMB_INSTRUMENT
{
	char name[8];
	unsigned int DT_MULTI;
	unsigned int TL;
	unsigned int KS_AR;
	unsigned int AMON_DR;
	unsigned int SR;
	unsigned int SL_RR;
	unsigned char FB_CNCT;
	unsigned char LR_AMS_PMS;
	char unused[14];
};

struct PCM_ENVELOPE
{
	unsigned char TL;
	unsigned char AR;
	unsigned char DR;
	unsigned char SL;
	unsigned char SR;
	unsigned char RR;
	unsigned char rootKey;
	unsigned char unused;
};

struct PMB_INSTRUMENT
{
	char name[8];
	char unused[8];
	unsigned short split[8];
	unsigned int soundID[8];
	struct PCM_ENVELOPE env[8];
};

struct PCM_Sound
{
	unsigned int addrInWaveRAM;
	struct PCM_Voice_Header snd;
};

struct SND_FM_Channel_State
{
	unsigned char instrument,vol;
};

struct SND_PCM_Channel_State
{
	_Far struct PCM_Voice_Header *header;
	_Far unsigned char *playPtr;

	// for Inst mode.
	unsigned char instrument,vol;

	// for Voice mode.
	unsigned int curPos;
	unsigned char playing;
	unsigned char nextFillBank;
};

struct SND_Status
{
	unsigned int elevol_mute; // Set by SND BIOS.  In High-C library, it is accessed from SND_get_elevol_mute, but is done by 0110:01C0
	unsigned int YM2612_REG27H;

	unsigned int voiceModeStartAddr;
	unsigned int instSoundLastAddr;

	unsigned short voiceModeINTMask;
	unsigned char numVoiceModeChannels;
	unsigned char PCMKey;
	unsigned char voiceChannelBank[SND_NUM_PCM_CHANNELS];
	struct SND_FM_Channel_State FMCh[SND_NUM_FM_CHANNELS];
	struct SND_PCM_Channel_State PCMCh[SND_NUM_PCM_CHANNELS];
	struct FMB_INSTRUMENT FMInst[FM_NUM_INSTRUMENTS];   // Seriously it was using 6KB for 128 FM instruments?  Looks like it is.
	struct PMB_INSTRUMENT PCMInst[PCM_NUM_INSTRUMENTS]; // Although there was no mention

	int numSound;
	struct PCM_Sound PCMSound[PCM_MAX_NUM_SOUND];
};

_Far struct SND_Status *SND_GetStatus(void);

void SND_PCM_Voice_Mode_Interrupt(void); // Called from SNDINT.C

void YM2612_Write(unsigned char regSet,unsigned char reg,unsigned char value);

void SND_FM_Timer_A_Restart(void);
void SND_FM_Timer_B_Restart(void);

#endif
