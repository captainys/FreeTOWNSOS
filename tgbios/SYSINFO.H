#ifndef SYSINFO_H_IS_INCLUDED
#define SYSINFO_H_IS_INCLUDED

#include "TGBIOS.H"

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
};

struct TBIOS_System_Info
{
	// Sound
	unsigned int elevol_mute; // Set by SND BIOS.  In High-C library, it is accessed from SND_get_elevol_mute, but is done by 0110:01C0
	unsigned int YM2612_REG27H;
	unsigned short voiceModeBank;
	unsigned short usedBank;
	unsigned char numVoiceModeChannels;
	unsigned char PCMKey;
	unsigned char voiceChannelBank[SND_NUM_PCM_CHANNELS];
	struct SND_PCM_Play_Info pcmPlayInfo[SND_NUM_PCM_CHANNELS];

	unsigned short padding0;

	// Mouse
	unsigned mouseINTCount;

	// Misc.
	unsigned INTVec[4];       // Apparently just saving RIDT and IDT of INT 4DH for later restoring.

	// A Duck.
	unsigned int _quack;
};

_Far struct TBIOS_System_Info *SYSINFO_GetStruct(void);

#endif
