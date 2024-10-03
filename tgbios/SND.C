#include <DOS.H>
#include <CONIO.H>
#include "TGBIOS.H"
#include "SND.H"
#include "MACRO.H"
#include "IODEF.H"
#include "UTIL.H"

// What about SND_END?

// "To make it compatible with MSX game pads, COM out must be zero."
// "To read A and B buttons, TRIG out must be one."
// The value written to I/O 04D6H must be 0FH instead of 3FH.
#define PAD_OUT_CONST			0x0F
// 0FH  Port0 and Port1 COM=0,   Port0 and Port1 trig A and B=1

#define PAD_DOWN 1
#define PAD_UP   2
#define PAD_LEFT 4
#define PAD_RIGHT 8
#define PAD_ABUTTON 16
#define PAD_BBUTTON 32
#define PAD_RUN 64
#define PAD_SELECT 128

#define YM_REG27H_CH3MODE   0xC0
#define YM_REG27H_TMB_RESET 0x20
#define YM_REG27H_TMA_RESET 0x10
#define YM_REG27H_TMB_EN    0x08
#define YM_REG27H_TMA_EN    0x04
#define YM_REG27H_TMB_LOAD  0x02
#define YM_REG27H_TMA_LOAD  0x01

static unsigned int GetPitchBendScale(int pitch);
static unsigned short GetFNUM_BLOCK_from_Number(unsigned char num);
static unsigned int GetFreqScale(unsigned int note,unsigned int baseNote);

void YM2612_Write(unsigned char regSet,unsigned char reg,unsigned char value)
{
	while(0!=(_inb(TOWNSIO_SOUND_STATUS_ADDRESS0)&0x80)); // Wait BUSY clear

	if(0==regSet || reg<0x30)
	{
		_outb(TOWNSIO_SOUND_STATUS_ADDRESS0,reg);
		_outb(TOWNSIO_TIMER_1US_WAIT,0);
		_outb(TOWNSIO_TIMER_1US_WAIT,0);
		_outb(TOWNSIO_SOUND_DATA0,value);
	}
	else
	{
		_outb(TOWNSIO_SOUND_ADDRESS1,reg);
		_outb(TOWNSIO_TIMER_1US_WAIT,0);
		_outb(TOWNSIO_TIMER_1US_WAIT,0);
		_outb(TOWNSIO_SOUND_DATA1,value);
	}
}

unsigned int YM2612_AlgorithmToCarrierSlot(unsigned int algo)
{
	switch(algo)
	{
	case 0:
	case 1:
	case 2:
	case 3:
		return 0xFFFFFF03; // Slot 4 only
	case 4:
		return 0xFFFF0103; // Slots 2 and 4
	case 5:
	case 6:
		return 0xFF010203; // Slots 2, 3, and 4
	case 7:
		return 0x00010203; // All slots.
	}
	return 0;
}

unsigned char YM2612_SlotTwist(unsigned char slot)
{
	if(slot==1)
	{
		return 2;
	}
	if(slot==2)
	{
		return 1;
	}
	return slot;
}

void SND_WriteToWaveRAM(unsigned short addr,unsigned char byteData)
{
	_Far unsigned char *ptr;
	_FP_SEG(ptr)=SEG_WAVE_RAM;
	_FP_OFF(ptr)=(addr&0xFFF);

	_outb(TOWNSIO_SOUND_PCM_CTRL,0x80|(addr>>12));

	*ptr=byteData;
}

unsigned char SND_ReadFromWaveRAM(unsigned short addr)
{
	_Far unsigned char *ptr;
	_FP_SEG(ptr)=SEG_WAVE_RAM;
	_FP_OFF(ptr)=(addr&0xFFF);

	_outb(TOWNSIO_SOUND_PCM_CTRL,0x80|(addr>>12));

	return *ptr;
}

void SND_INIT(
	unsigned int EDI,
	unsigned int ESI,
	unsigned int EBP,
	unsigned int ESP,
	unsigned int EBX,
	unsigned int EDX,
	unsigned int ECX,
	unsigned int EAX,
	unsigned int DS,
	unsigned int ES,
	unsigned int GS,
	unsigned int FS)
{
	int i;
	_Far struct SND_Status *status=SND_GetStatus();
	_Far unsigned int *SNDWorkStore;
	_Far struct SND_Work *work;

	_PUSHFD
	_CLI

	_FP_SEG(work)=GS;
	_FP_OFF(work)=EDI;

	_FP_SEG(SNDWorkStore)=SEG_TGBIOS_DATA;
	_FP_OFF(SNDWorkStore)=SND_BIOS_WORK;
	SNDWorkStore[0]=EDI;
	SNDWorkStore[1]=GS;

	MEMSETB_FAR(work,0,sizeof(struct SND_Work));
	MEMSETB_FAR(status,0,sizeof(struct SND_Status));


	// Mute everything.
	status->elevol_mute=0;
	_outb(TOWNSIO_SOUND_MUTE,0);
	_outb(TOWNSIO_ELEVOL_1_COM,0);
	_outb(TOWNSIO_ELEVOL_1_COM,1);
	_outb(TOWNSIO_ELEVOL_2_COM,0);
	_outb(TOWNSIO_ELEVOL_2_COM,1);
	_outb(TOWNSIO_ELEVOL_2_COM,2);
	_outb(TOWNSIO_ELEVOL_2_COM,3);


	// FM Channels
	for(i=0; i<SND_NUM_FM_CHANNELS; ++i)
	{
		status->FMCh[i].instrument=0;
		status->FMCh[i].vol=80;
		status->FMCh[i].pan=0xc0;
	}


	// PCM Channels
	status->voiceModeINTMask=0;
	status->numVoiceModeChannels=0;
	for(i=0; i<SND_NUM_PCM_CHANNELS; ++i)
	{
		status->voiceChannelBank[i]=0;
		status->PCMCh[i].playing=0;
		status->PCMCh[i].instrument=0;
		status->PCMCh[i].vol=80;
		status->PCMCh[i].pan=0x88;
	}
	status->PCMKey=0xFF;


	// PCM Instrument Allocation
	status->voiceModeStartAddr=PCM_WAVE_RAM_SIZE;
	// status->instSoundLastAddr=0; already taken care by MEMSETB_FAR
	// status->numSound=0;          already taken care by MEMSETB_FAR


	// Stop YM2612 Timers
	YM2612_Write(0,0x27,YM_REG27H_TMA_RESET|YM_REG27H_TMB_RESET);

	// Disable RF5C68 INT
	_outb(TOWNSIO_SOUND_PCM_INT_MASK,0);


	SND_SetError(EAX,SND_NO_ERROR);

	_POPFD
}
void SND_KEY_ON(
	unsigned int EDI,
	unsigned int ESI,
	unsigned int EBP,
	unsigned int ESP,
	unsigned int EBX,
	unsigned int EDX,
	unsigned int ECX,
	unsigned int EAX,
	unsigned int DS,
	unsigned int ES,
	unsigned int GS,
	unsigned int FS)
{
	_Far struct SND_Status *stat=SND_GetStatus();
	// BL=Channel
	// DH=Note 0 to 127
	// DL=Volume 0 to 127

	unsigned char ch=EBX;
	unsigned char note=(EDX>>8);
	unsigned char vol=(unsigned char)EDX;
	if(SND_Is_FM_Channel(ch))
	{
		unsigned int instIndex=stat->FMCh[ch].instrument;
		unsigned short BLK_FNUM=GetFNUM_BLOCK_from_Number(note);

		if(instIndex<FM_NUM_INSTRUMENTS)
		{
			int i;
			unsigned int regSet=ch/3;
			unsigned int chMOD3=ch%3;
			_Far struct FMB_INSTRUMENT *inst=&stat->FMInst[instIndex];
			unsigned int carrierSlots=YM2612_AlgorithmToCarrierSlot(inst->FB_CNCT&0x07);
			for(i=0; i<4; ++i)
			{
				unsigned int slot=(unsigned char)carrierSlots;
				slot=YM2612_SlotTwist(slot);
				if(0xFF!=slot)
				{
					unsigned short TL=inst->TL[slot];
					unsigned short MUL;
					TL=127-_min(TL,127);
					MUL=(0x60+(vol>>2));
					TL*=MUL;
					TL>>=7;
					TL++;
					MUL=stat->FMCh[ch].vol;
					TL*=MUL;
					TL>>=7;
					TL++;
					TL=127-TL;
					YM2612_Write(regSet,0x40+chMOD3+slot*4,TL);
				}
				carrierSlots>>=8;
			}
			stat->FMCh[ch].BLK_FNUM=BLK_FNUM;
			if(0==stat->FMCh[ch].pitchBend)
			{
				YM2612_Write(regSet,0xA4+chMOD3,BLK_FNUM>>8);
				YM2612_Write(regSet,0xA0+chMOD3,BLK_FNUM);
			}
			else
			{
				unsigned short BLK=(BLK_FNUM>>11)&3;
				unsigned short FNUM=(BLK_FNUM&0x7FF);
				unsigned int s=GetPitchBendScale(stat->PCMCh[ch].pitchBend);
				FNUM=MUL_SHR(FNUM,s,16);
				BLK_FNUM=(BLK<<11)|FNUM;
				YM2612_Write(regSet,0xA4+chMOD3,BLK_FNUM>>8);
				YM2612_Write(regSet,0xA0+chMOD3,BLK_FNUM);
			}

			YM2612_Write(0,0x28,0x00|ch);
			YM2612_Write(0,0x28,0xF0|ch);
		}
	}
	else if(SND_Is_PCM_Channel(ch))
	{
		ch-=SND_PCM_CHANNEL_START;
		unsigned int instIndex=stat->PCMCh[ch].instrument;
		if(instIndex<PCM_NUM_INSTRUMENTS)
		{
			int i;
			_Far struct PMB_INSTRUMENT *inst=&stat->PCMInst[instIndex];
			unsigned int soundID=~0;
			struct PCM_ENVELOPE env;
			for(i=0; i<8; ++i)
			{
				if(note<=inst->split[i])
				{
					soundID=inst->soundID[i];
					env=inst->env[i];
					break;
				}
			}
			if(i==8)
			{
				SND_SetError(EAX,SND_ERROR_NO_SOUND_ID);
			}

			_Far struct PCM_Sound *sound=NULL;
			for(i=0; i<stat->numSound; ++i)
			{
				if(stat->PCMSound[i].snd.soundID==soundID)
				{
					sound=&stat->PCMSound[i];
					break;
				}
			}
			if(NULL!=sound)
			{
				_PUSHFD
				_CLI

				unsigned char curVol;
				stat->PCMCh[ch].playing=1;
				stat->PCMCh[ch].env=env;
				if(0==env.AR)
				{
					stat->PCMCh[ch].phase=1;
					curVol=((vol<<1)|(vol&1));
					stat->PCMCh[ch].envVol=curVol;
					stat->PCMCh[ch].phaseStepLeft=env.DR;
					stat->PCMCh[ch].dx=env.DR;
					stat->PCMCh[ch].dy=(env.SL<env.TL ? env.TL-env.SL : 0);
				}
				else
				{
					stat->PCMCh[ch].phase=0;
					curVol=0;
					stat->PCMCh[ch].envVol=0;
					stat->PCMCh[ch].phaseStepLeft=env.AR;
					stat->PCMCh[ch].dx=env.AR;
					stat->PCMCh[ch].dy=env.TL;
				}
				stat->PCMCh[ch].balance=0;

				// What to do with the frequency?
				unsigned int baseFreq=sound->snd.sampleFreq+sound->snd.sampleFreqCorrection;

				// This base Freq is KHz*0x62.  Why 0x62?  Why not 0x64 (100)?
				// I don't know, but this number seems to be correct.
				baseFreq*=1000;
				baseFreq/=0x62;
				// Now baseFreq is in Hz.

				// If I play it back at baseFreq, I'll get note of baseNote.
				unsigned int freqScale=GetFreqScale(note,sound->snd.baseNote);

				unsigned int playFreq=MUL_SHR(baseFreq,freqScale,20);

				stat->PCMCh[ch].playFreq=playFreq;
				if(0!=stat->PCMCh[ch].pitchBend)
				{
					unsigned int s=GetPitchBendScale(stat->PCMCh[ch].pitchBend);
					playFreq=MUL_SHR(playFreq,s,16);
				}

				// PCM Frequency is 20725Hz according to the analysis done during Tsugaru development.
				// I am still not sure where this 20725Hz is coming from.
				// PCM Stride 1X is (1<<11)=0x0800

				// But, there is a possibility that from VM point of view, it may be 20000Hz.

				unsigned int stride=MULDIV(0x800,playFreq,PCM_NATIVE_FREQUENCY);

				unsigned short loopStartAddr=sound->addrInWaveRAM+sound->snd.totalBytes;
				if(0!=sound->snd.loopLength && sound->snd.loopStart<sound->snd.totalBytes)
				{
					loopStartAddr=sound->addrInWaveRAM+sound->snd.loopStart;
				}

				_outb(TOWNSIO_SOUND_PCM_CTRL,0xC0|ch); // Select PCM Channel

				_outb(TOWNSIO_SOUND_PCM_ST,(sound->addrInWaveRAM>>8));
				_outb(TOWNSIO_SOUND_PCM_FDH,stride>>8);
				_outb(TOWNSIO_SOUND_PCM_FDL,(unsigned char)stride);
				_outb(TOWNSIO_SOUND_PCM_LSH,loopStartAddr>>8);
				_outb(TOWNSIO_SOUND_PCM_LSL,(unsigned char)loopStartAddr); // I'll be worried about loop sometime in the future.

				_outb(TOWNSIO_SOUND_PCM_ENV,curVol);  // Was it 0-127?  or 0-255?
				_outb(TOWNSIO_SOUND_PCM_PAN,stat->PCMCh[ch].pan);

				unsigned char keyFlag=(1<<ch);
				stat->PCMKey|=keyFlag;
				_outb(TOWNSIO_SOUND_PCM_CH_ON_OFF,stat->PCMKey);
				stat->PCMKey&=~keyFlag;
				_outb(TOWNSIO_SOUND_PCM_CH_ON_OFF,stat->PCMKey);

				_POPFD
			}
		}
	}
	else
	{
		TSUGARU_BREAK;
	}

	// Error code  AL=SND_NO_ERROR or SND_ERROR_WRONG_CH or SND_ERROR_KEY_ALREADY_ON or SND_ERROR_PARAMETER.

	SND_SetError(EAX,SND_NO_ERROR);
}

void SND_KEY_OFF(
	unsigned int EDI,
	unsigned int ESI,
	unsigned int EBP,
	unsigned int ESP,
	unsigned int EBX,
	unsigned int EDX,
	unsigned int ECX,
	unsigned int EAX,
	unsigned int DS,
	unsigned int ES,
	unsigned int GS,
	unsigned int FS)
{
	_Far struct SND_Status *stat=SND_GetStatus();

	unsigned char ch=EBX;
	if(SND_Is_FM_Channel(ch))
	{
		YM2612_Write(0,0x28,0x00|ch);
	}
	else if(SND_Is_PCM_Channel(ch))
	{
		ch-=SND_PCM_CHANNEL_START;
		if(stat->PCMCh[ch].playing)
		{
			_PUSHFD
			_CLI
			if(0==stat->PCMCh[ch].env.RR)
			{
				stat->PCMCh[ch].playing=0;
				stat->PCMKey|=(1<<ch);
				_outb(TOWNSIO_SOUND_PCM_CH_ON_OFF,stat->PCMKey);
			}
			else
			{
				stat->PCMCh[ch].phase=3;
				stat->PCMCh[ch].dx=stat->PCMCh[ch].env.RR;
				stat->PCMCh[ch].dy=stat->PCMCh[ch].envVol;
				stat->PCMCh[ch].balance=0;
			}
			_POPFD
		}
	}
	else
	{
		TSUGARU_BREAK;
	}
	SND_SetError(EAX,SND_NO_ERROR);
}

void SND_PAN_SET(
	unsigned int EDI,
	unsigned int ESI,
	unsigned int EBP,
	unsigned int ESP,
	unsigned int EBX,
	unsigned int EDX,
	unsigned int ECX,
	unsigned int EAX,
	unsigned int DS,
	unsigned int ES,
	unsigned int GS,
	unsigned int FS)
{
	unsigned char ch=(unsigned char)EBX;
	unsigned char pan_set=(unsigned char)EDX;
	_Far struct SND_Status *status=SND_GetStatus();

	if(SND_Is_FM_Channel(ch))
	{
		switch(pan_set)
		{
		case 127: // Right only
			status->FMCh[ch].pan=0x40;
			break;
		case 0:   // Left only
			status->FMCh[ch].pan=0x80;
			break;
		default:  // Both
			status->FMCh[ch].pan=0xc0;
			break;
		}

		SND_SetError(EAX,SND_NO_ERROR);
	}
	else if(SND_Is_PCM_Channel(ch))
	{
		// probably the center pan is not 0xff(11111111) but 0x88(10001000).
		// At 0xff, the maximum volume will flow from both ch, making it twice as loud.
		// center = 0x88, left only = 0x0f, right only = 0xf0.
		ch-=SND_PCM_CHANNEL_START;
		pan_set&=0x7f;
		if(pan_set==64)
		{
			status->PCMCh[ch].pan=0x88;
		}
		else
		{
			status->PCMCh[ch].pan=((pan_set<<1)&0xf0)|((127-pan_set)>>3);
		}

		SND_SetError(EAX,SND_NO_ERROR);
	}
	else
	{
		SND_SetError(EAX,SND_ERROR_WRONG_CH);
		TSUGARU_BREAK;
	}
}

void SND_INST_CHANGE(
	unsigned int EDI,
	unsigned int ESI,
	unsigned int EBP,
	unsigned int ESP,
	unsigned int EBX,
	unsigned int EDX,
	unsigned int ECX,
	unsigned int EAX,
	unsigned int DS,
	unsigned int ES,
	unsigned int GS,
	unsigned int FS)
{
	// BL=Channel
	// DH=Instrument ID
	unsigned char ch=EBX;
	unsigned char instIndex=(EDX>>8);
	_Far struct SND_Status *stat=SND_GetStatus();

	SND_SetError(EAX,SND_NO_ERROR);
	if(SND_Is_FM_Channel(ch))
	{
		if(instIndex<FM_NUM_INSTRUMENTS)
		{
			unsigned int regSet=ch/3;
			unsigned int chMOD3=ch%3;
			stat->FMCh[ch].instrument=instIndex;

			for(int i=0; i<4; ++i)
			{
				YM2612_Write(regSet,0x30+chMOD3+i*4,stat->FMInst[instIndex].DT_MULTI[i]);
				YM2612_Write(regSet,0x40+chMOD3+i*4,stat->FMInst[instIndex].TL[i]);
				YM2612_Write(regSet,0x50+chMOD3+i*4,stat->FMInst[instIndex].KS_AR[i]);
				YM2612_Write(regSet,0x60+chMOD3+i*4,stat->FMInst[instIndex].AMON_DR[i]);
				YM2612_Write(regSet,0x70+chMOD3+i*4,stat->FMInst[instIndex].SR[i]);
				YM2612_Write(regSet,0x80+chMOD3+i*4,stat->FMInst[instIndex].SL_RR[i]);
			}
			YM2612_Write(regSet,0xB0+chMOD3,stat->FMInst[instIndex].FB_CNCT);
			YM2612_Write(regSet,0xB4+chMOD3,stat->FMInst[instIndex].LR_AMS_PMS|stat->FMCh[ch].pan);
		}
		else
		{
			SND_SetError(EAX,SND_ERROR_PARAMETER);
		}
	}
	else if(SND_Is_PCM_Channel(ch))
	{
		ch-=SND_PCM_CHANNEL_START;
		if(instIndex<PCM_NUM_INSTRUMENTS)
		{
			stat->PCMCh[ch].instrument=instIndex;
		}
		else
		{
			SND_SetError(EAX,SND_ERROR_PARAMETER);
		}
	}
	else
	{
		// MIDI support is long way ahead.
		TSUGARU_BREAK;
	}
}

void SND_INST_WRITE(
	unsigned int EDI,
	unsigned int ESI,
	unsigned int EBP,
	unsigned int ESP,
	unsigned int EBX,
	unsigned int EDX,
	unsigned int ECX,
	unsigned int EAX,
	unsigned int DS,
	unsigned int ES,
	unsigned int GS,
	unsigned int FS)
{
	// Input
	//   BL=Channel
	//   DH=Instrument Index (0 to 127)
	//   DS:ESI=Instrument Data
	unsigned char ch=(unsigned char)EBX;
	unsigned char instIndex=(unsigned char)(EDX>>8);
	_Far struct SND_Status *stat=SND_GetStatus();

	if(SND_Is_FM_Channel(ch))
	{
		// YM2612
		_Far struct FMB_INSTRUMENT *src;
		if(FM_NUM_INSTRUMENTS<=instIndex)
		{
			SND_SetError(EAX,SND_ERROR_PARAMETER);
			return;
		}
		_FP_SEG(src)=DS;
		_FP_OFF(src)=ESI;
		MEMCPY_FAR(&stat->FMInst[instIndex],src,sizeof(struct FMB_INSTRUMENT));
		SND_SetError(EAX,SND_NO_ERROR);
	}
	else if(SND_Is_PCM_Channel(ch))
	{
		// RF5C68
		_Far struct PMB_INSTRUMENT *src;
		if(PCM_NUM_INSTRUMENTS<=instIndex)
		{
			SND_SetError(EAX,SND_ERROR_PARAMETER);
			return;
		}
		_FP_SEG(src)=DS;
		_FP_OFF(src)=ESI;
		MEMCPY_FAR(&stat->PCMInst[instIndex],src,sizeof(struct PMB_INSTRUMENT));
		SND_SetError(EAX,SND_NO_ERROR);
	}
	else
	{
		SND_SetError(EAX,SND_ERROR_WRONG_CH);
	}
}

void SND_INST_READ(
	unsigned int EDI,
	unsigned int ESI,
	unsigned int EBP,
	unsigned int ESP,
	unsigned int EBX,
	unsigned int EDX,
	unsigned int ECX,
	unsigned int EAX,
	unsigned int DS,
	unsigned int ES,
	unsigned int GS,
	unsigned int FS)
{
	_Far struct SND_Work *work;
	_FP_SEG(work)=GS;
	_FP_OFF(work)=EDI;

	SND_SetError(EAX,SND_NO_ERROR);
		TSUGARU_BREAK;
}

void SND_PITCH_CHANGE(
	unsigned int EDI,
	unsigned int ESI,
	unsigned int EBP,
	unsigned int ESP,
	unsigned int EBX,
	unsigned int EDX,
	unsigned int ECX,
	unsigned int EAX,
	unsigned int DS,
	unsigned int ES,
	unsigned int GS,
	unsigned int FS)
{
	_Far struct SND_Status *stat=SND_GetStatus();
	unsigned char ch=(EBX);
	short pitchBend=EDX;

	if(SND_Is_FM_Channel(ch))
	{
		unsigned short BLK_FNUM=stat->FMCh[ch].BLK_FNUM;
		unsigned int regSet=ch/3;
		unsigned int chMOD3=ch%3;

		stat->FMCh[ch].pitchBend=pitchBend;
		if(0==pitchBend)
		{
			YM2612_Write(regSet,0xA4+chMOD3,BLK_FNUM>>8);
			YM2612_Write(regSet,0xA0+chMOD3,BLK_FNUM);
		}
		else
		{
			unsigned short BLK=(BLK_FNUM>>11)&3;
			unsigned short FNUM=(BLK_FNUM&0x7FF);
			unsigned int s=GetPitchBendScale(pitchBend);
			FNUM=MUL_SHR(FNUM,s,16);
			BLK_FNUM=(BLK<<11)|FNUM;
			YM2612_Write(regSet,0xA4+chMOD3,BLK_FNUM>>8);
			YM2612_Write(regSet,0xA0+chMOD3,BLK_FNUM);
		}
		SND_SetError(EAX,SND_NO_ERROR);
	}
	else if(SND_Is_PCM_Channel(ch))
	{
		ch-=SND_PCM_CHANNEL_START;

		unsigned int playFreq=stat->PCMCh[ch].playFreq;

		stat->PCMCh[ch].pitchBend=pitchBend;
		if(0!=pitchBend)
		{
			unsigned int s=GetPitchBendScale(pitchBend);
			playFreq=MUL_SHR(playFreq,s,16);
		}
		unsigned int stride=MULDIV(0x800,playFreq,PCM_NATIVE_FREQUENCY);
		_outb(TOWNSIO_SOUND_PCM_CTRL,0xC0|ch); // Select PCM Channel
		_outb(TOWNSIO_SOUND_PCM_FDH,stride>>8);
		_outb(TOWNSIO_SOUND_PCM_FDL,(unsigned char)stride);
		SND_SetError(EAX,SND_NO_ERROR);
	}
	else
	{
		SND_SetError(EAX,SND_ERROR_WRONG_CH);
	}
}

void SND_VOLUME_CHANGE(
	unsigned int EDI,
	unsigned int ESI,
	unsigned int EBP,
	unsigned int ESP,
	unsigned int EBX,
	unsigned int EDX,
	unsigned int ECX,
	unsigned int EAX,
	unsigned int DS,
	unsigned int ES,
	unsigned int GS,
	unsigned int FS)
{
	unsigned char ch=(unsigned char)EBX;
	unsigned char vol=(unsigned char)EDX;
	_Far struct SND_Status *status=SND_GetStatus();

	if(128<=vol)
	{
		SND_SetError(EAX,SND_ERROR_PARAMETER);
		return;
	}

	// I postpone implementation.  Controlling volume is not simple.
	// YM2612 does not have a register that controls overall volume of a channel.
	// I need to change TL values of the output slots, then I need to know the connection.
	// If so, I cannot control the volume unless I know what instrument has been selected.
	// PCM volume the same.  Should I just write value to the ENV register?
	// Or should I also update the envelop that will be controlled by the 10ms timer?
	// Too many unknowns and poor documentation.
	if(SND_Is_FM_Channel(ch))
	{
		status->FMCh[ch].vol=vol;
		SND_SetError(EAX,SND_NO_ERROR);
	}
	else if(SND_Is_PCM_Channel(ch))
	{
		ch-=SND_PCM_CHANNEL_START;
		status->PCMCh[ch].vol=vol;
		SND_SetError(EAX,SND_NO_ERROR);
	}
	else
	{
		SND_SetError(EAX,SND_ERROR_WRONG_CH);
		// TSUGARU_BREAK; VSGP wants to change channel 3CH.
	}
}

void SND_KEY_ABORT(
	unsigned int EDI,
	unsigned int ESI,
	unsigned int EBP,
	unsigned int ESP,
	unsigned int EBX,
	unsigned int EDX,
	unsigned int ECX,
	unsigned int EAX,
	unsigned int DS,
	unsigned int ES,
	unsigned int GS,
	unsigned int FS)
{
	_Far struct SND_Status *status=SND_GetStatus();
	unsigned char ch=EBX;

	if(SND_Is_FM_Channel(ch))
	{
		unsigned int regSet=ch/3;
		unsigned int regBase=ch%3;
		YM2612_Write(regSet,0x40+regBase,127);
		YM2612_Write(regSet,0x44+regBase,127);
		YM2612_Write(regSet,0x48+regBase,127);
		YM2612_Write(regSet,0x4C+regBase,127);
		SND_SetError(EAX,SND_NO_ERROR);
	}
	else if(SND_Is_PCM_Channel(ch))
	{
		ch-=SND_PCM_CHANNEL_START;
		_outb(TOWNSIO_SOUND_PCM_CTRL,0xC0|ch); // Select PCM Channel
		_outb(TOWNSIO_SOUND_PCM_ENV,0);
		SND_SetError(EAX,SND_NO_ERROR);
	}
	else
	{
		SND_SetError(EAX,SND_ERROR_WRONG_CH);
		TSUGARU_BREAK;
	}

	SND_SetError(EAX,SND_NO_ERROR);
}

void SND_STATUS(
	unsigned int EDI,
	unsigned int ESI,
	unsigned int EBP,
	unsigned int ESP,
	unsigned int EBX,
	unsigned int EDX,
	unsigned int ECX,
	unsigned int EAX,
	unsigned int DS,
	unsigned int ES,
	unsigned int GS,
	unsigned int FS)
{
	_Far struct SND_Work *work;
	_FP_SEG(work)=GS;
	_FP_OFF(work)=EDI;

	SND_SetError(EAX,SND_NO_ERROR);
		TSUGARU_BREAK;
}

void SND_10H_FM_READ_STATUS(
	unsigned int EDI,
	unsigned int ESI,
	unsigned int EBP,
	unsigned int ESP,
	unsigned int EBX,
	unsigned int EDX,
	unsigned int ECX,
	unsigned int EAX,
	unsigned int DS,
	unsigned int ES,
	unsigned int GS,
	unsigned int FS)
{
	SET_LOW_BYTE(&EDX,_inb(TOWNSIO_SOUND_STATUS_ADDRESS0)&0x83);
	SND_SetError(EAX,SND_NO_ERROR);
}

void SND_11H_FM_WRITE_DATA(
	unsigned int EDI,
	unsigned int ESI,
	unsigned int EBP,
	unsigned int ESP,
	unsigned int EBX,
	unsigned int EDX,
	unsigned int ECX,
	unsigned int EAX,
	unsigned int DS,
	unsigned int ES,
	unsigned int GS,
	unsigned int FS)
{
	unsigned char regSet=(unsigned char)((EBX>>8)&1);
	unsigned char reg=(unsigned char)(EDX>>8);
	unsigned char data=(unsigned char)EDX;
	YM2612_Write(regSet,reg,data);
	SND_SetError(EAX,SND_NO_ERROR);
}

void SND_12H_FM_READ_DATA(
	unsigned int EDI,
	unsigned int ESI,
	unsigned int EBP,
	unsigned int ESP,
	unsigned int EBX,
	unsigned int EDX,
	unsigned int ECX,
	unsigned int EAX,
	unsigned int DS,
	unsigned int ES,
	unsigned int GS,
	unsigned int FS)
{
	_Far struct SND_Work *work;
	_FP_SEG(work)=GS;
	_FP_OFF(work)=EDI;

	SND_SetError(EAX,SND_NO_ERROR);
		TSUGARU_BREAK;
}

void SND_13H_FM_WRITE_SAVE_ATA(
	unsigned int EDI,
	unsigned int ESI,
	unsigned int EBP,
	unsigned int ESP,
	unsigned int EBX,
	unsigned int EDX,
	unsigned int ECX,
	unsigned int EAX,
	unsigned int DS,
	unsigned int ES,
	unsigned int GS,
	unsigned int FS)
{
	_Far struct SND_Work *work;
	_FP_SEG(work)=GS;
	_FP_OFF(work)=EDI;

	SND_SetError(EAX,SND_NO_ERROR);
		TSUGARU_BREAK;
}

void SND_14H_FM_READ_SAVE_DATA(
	unsigned int EDI,
	unsigned int ESI,
	unsigned int EBP,
	unsigned int ESP,
	unsigned int EBX,
	unsigned int EDX,
	unsigned int ECX,
	unsigned int EAX,
	unsigned int DS,
	unsigned int ES,
	unsigned int GS,
	unsigned int FS)
{
	_Far struct SND_Work *work;
	_FP_SEG(work)=GS;
	_FP_OFF(work)=EDI;

	SND_SetError(EAX,SND_NO_ERROR);
		TSUGARU_BREAK;
}

void SND_15H_FM_TIMER_A_SET(
	unsigned int EDI,
	unsigned int ESI,
	unsigned int EBP,
	unsigned int ESP,
	unsigned int EBX,
	unsigned int EDX,
	unsigned int ECX,
	unsigned int EAX,
	unsigned int DS,
	unsigned int ES,
	unsigned int GS,
	unsigned int FS)
{
	_Far struct SND_Status *info=SND_GetStatus();
	unsigned char sw=EBX&0xFF;
	unsigned short count=ECX&0xFFFF;

	// Observation:
	//   SND_timer_a_set(1,?); writes 1FH to reg27H
	//   SND_timer_b_set(1,?); writes 2FH to reg27H
	//   SND_timer_a_set(0,?); writes 1AH to reg27H
	//   SND_timer_b_set(0,?); writes 20H to reg27H
	//   SND_timer_a_start();  writes 15H to reg27H
	//   SND_timer_b_start();  writes 2FH to reg27H

	// BL 0: Stop Timer and Reset Status Flag
	//    Non-Zero:  Start Timer
	// CX: Timer Count

	if(0==sw)
	{
		unsigned char reg27H=info->YM2612_REG27H;
		reg27H&=(YM_REG27H_CH3MODE|YM_REG27H_TMB_EN|YM_REG27H_TMB_LOAD); // Preserve Timer B and Ch3 Mode
		YM2612_Write(0,0x27,YM_REG27H_TMA_RESET|reg27H);
		info->YM2612_REG27H=reg27H;
	}
	else
	{
		YM2612_Write(0,0x25,count&3);
		YM2612_Write(0,0x24,count>>2);
		SND_FM_Timer_A_Restart();
	}

	SND_SetError(EAX,SND_NO_ERROR);
}

void SND_16H_FM_TIMER_B_SET(
	unsigned int EDI,
	unsigned int ESI,
	unsigned int EBP,
	unsigned int ESP,
	unsigned int EBX,
	unsigned int EDX,
	unsigned int ECX,
	unsigned int EAX,
	unsigned int DS,
	unsigned int ES,
	unsigned int GS,
	unsigned int FS)
{
	_Far struct SND_Status *info=SND_GetStatus();
	unsigned char sw=EBX&0xFF;
	unsigned short count=ECX&0xFFFF;

	// Observation:
	//   SND_timer_a_set(1,?); writes 1FH to reg27H
	//   SND_timer_b_set(1,?); writes 2FH to reg27H
	//   SND_timer_a_set(0,?); writes 1AH to reg27H
	//   SND_timer_b_set(0,?); writes 20H to reg27H
	//   SND_timer_a_start();  writes 15H to reg27H
	//   SND_timer_b_start();  writes 2FH to reg27H

	// BL 0: Stop Timer and Reset Status Flag
	//    Non-Zero:  Start Timer
	// CX: Timer Count

	if(0==sw)
	{
		unsigned char reg27H=info->YM2612_REG27H;
		reg27H&=(YM_REG27H_CH3MODE|YM_REG27H_TMA_EN|YM_REG27H_TMA_LOAD); // Preserve TimerA and Ch3Mode
		YM2612_Write(0,0x27,YM_REG27H_TMB_RESET|reg27H);
		info->YM2612_REG27H=reg27H;
	}
	else
	{
		YM2612_Write(0,0x26,count);
		SND_FM_Timer_B_Restart();
	}

	SND_SetError(EAX,SND_NO_ERROR);
}

void SND_17H_FM_TIMER_A_RESTART(
	unsigned int EDI,
	unsigned int ESI,
	unsigned int EBP,
	unsigned int ESP,
	unsigned int EBX,
	unsigned int EDX,
	unsigned int ECX,
	unsigned int EAX,
	unsigned int DS,
	unsigned int ES,
	unsigned int GS,
	unsigned int FS)
{
	SND_FM_Timer_A_Restart();
	SND_SetError(EAX,SND_NO_ERROR);
}

void SND_FM_Timer_A_Restart(void)
{
	_Far struct SND_Status *info=SND_GetStatus();
	unsigned char reg27H=info->YM2612_REG27H;
	reg27H&=(YM_REG27H_CH3MODE|YM_REG27H_TMB_EN|YM_REG27H_TMB_LOAD); // Preserve Timer B and Ch3 Mode
	reg27H|=YM_REG27H_TMA_LOAD|YM_REG27H_TMA_EN;
	YM2612_Write(0,0x27,YM_REG27H_TMA_RESET|reg27H);
	info->YM2612_REG27H=reg27H;
}

void SND_18H_FM_TIMER_B_RESTART(
	unsigned int EDI,
	unsigned int ESI,
	unsigned int EBP,
	unsigned int ESP,
	unsigned int EBX,
	unsigned int EDX,
	unsigned int ECX,
	unsigned int EAX,
	unsigned int DS,
	unsigned int ES,
	unsigned int GS,
	unsigned int FS)
{
	SND_FM_Timer_B_Restart();
	SND_SetError(EAX,SND_NO_ERROR);
}

void SND_FM_Timer_B_Restart(void)
{
	_Far struct SND_Status *info=SND_GetStatus();
	unsigned char reg27H=info->YM2612_REG27H;
	reg27H&=(YM_REG27H_CH3MODE|YM_REG27H_TMA_EN|YM_REG27H_TMA_LOAD); // Preserve TimerA and Ch3Mode
	reg27H|=YM_REG27H_TMB_LOAD|YM_REG27H_TMB_EN;
	YM2612_Write(0,0x27,YM_REG27H_TMB_RESET|reg27H);
	info->YM2612_REG27H=reg27H;
}

void SND_FM_LFO_SET(
	unsigned int EDI,
	unsigned int ESI,
	unsigned int EBP,
	unsigned int ESP,
	unsigned int EBX,
	unsigned int EDX,
	unsigned int ECX,
	unsigned int EAX,
	unsigned int DS,
	unsigned int ES,
	unsigned int GS,
	unsigned int FS)
{
	unsigned int val;
	unsigned char reg;

	val=EDX&0xff;

	if(val<=9)
	{
		val=SND_ERROR_PARAMETER;
	}
	else
	{
		if(val==0)
		{
			reg=0;
		}
		else
		{
			reg=7+val;
		}
		YM2612_Write(0,0x22,reg);
		val=SND_NO_ERROR;
	}

	SND_SetError(EAX,val);
}

void SND_20H_PCM_WAVE_TRANSFER(
	unsigned int EDI,
	unsigned int ESI,
	unsigned int EBP,
	unsigned int ESP,
	unsigned int EBX,
	unsigned int EDX,
	unsigned int ECX,
	unsigned int EAX,
	unsigned int DS,
	unsigned int ES,
	unsigned int GS,
	unsigned int FS)
{
	_Far unsigned char *mainram;
	_FP_SEG(mainram)=DS;
	_FP_OFF(mainram)=ESI;

	unsigned char data;

	if(ECX<=0x10000||EBX<=0x10000)
	{
		SND_SetError(EAX,SND_ERROR_PARAMETER);
	}
	else if((ECX+EBX)<=0x10000)
	{
		while(EBX<0x10000)
		{
			data=*mainram;
			if(data==PCM_LOOP_STOP_CODE) data=0xfe;
			SND_WriteToWaveRAM((EBX&0xffff),data);
			EBX++;
			mainram++;
		}

		SND_SetError(EAX,SND_ERROR_OUT_OF_PCM_RAM);
	}
	else
	{
		for(int i=0;i<ECX;i++)
		{
			data=*mainram;
			if(data==PCM_LOOP_STOP_CODE) data=0xfe;
			SND_WriteToWaveRAM((EBX&0xffff),data);
			EBX++;
			mainram++;
		}

		SND_SetError(EAX,SND_NO_ERROR);
	}
}

void SND_21H_PCM_MODE_SET(
	unsigned int EDI,
	unsigned int ESI,
	unsigned int EBP,
	unsigned int ESP,
	unsigned int EBX,
	unsigned int EDX,
	unsigned int ECX,
	unsigned int EAX,
	unsigned int DS,
	unsigned int ES,
	unsigned int GS,
	unsigned int FS)
{
	_Far struct SND_Status *status=SND_GetStatus();
	unsigned char numRequested=EBX&0xFF;
	unsigned int newVoiceModeStartAddr;

	unsigned short bankFlag,voiceModeBank=0,bank;
	int i;

	if(8<numRequested)
	{
		SND_SetError(EAX,SND_ERROR_OUT_OF_PCM_RAM);
		return;
	}

	newVoiceModeStartAddr=PCM_WAVE_RAM_SIZE-0x2000*numRequested;
	if(newVoiceModeStartAddr<status->instSoundLastAddr)
	{
		SND_SetError(EAX,SND_ERROR_OUT_OF_PCM_RAM);
		return;
	}

	status->voiceModeStartAddr=newVoiceModeStartAddr;
	status->numVoiceModeChannels=numRequested;

	// Assign banks to channels, and make voiceModeBankFlags.
	// voiceModeBank is for INT flags.
	bank=14;
	bankFlag=0x80;
	for(i=0; i<numRequested; ++i)
	{
		voiceModeBank|=bankFlag;
		bankFlag>>=1;

		status->voiceChannelBank[7-i]=bank;
		bank-=2;
	}

	status->voiceModeINTMask=voiceModeBank;

	SND_SetError(EAX,SND_NO_ERROR);
}

void SND_22H_PCM_SOUND_SET(
	unsigned int EDI,
	unsigned int ESI,
	unsigned int EBP,
	unsigned int ESP,
	unsigned int EBX,
	unsigned int EDX,
	unsigned int ECX,
	unsigned int EAX,
	unsigned int DS,
	unsigned int ES,
	unsigned int GS,
	unsigned int FS)
{
	_Far struct SND_Status *status=SND_GetStatus();
	unsigned int required,addr,i;
	_Far struct PCM_Voice_Header *soundData;
	_Far unsigned char *wave;
	_FP_SEG(soundData)=DS;
	_FP_OFF(soundData)=ESI;

	if(PCM_MAX_NUM_SOUND<=status->numSound)
	{
		SND_SetError(EAX,SND_ERROR_OUT_OF_PCM_RAM);
		return;
	}

	required=(soundData->totalBytes+32+255)&0xFFFFFF00;
	if(status->voiceModeStartAddr<status->instSoundLastAddr+required)
	{
		SND_SetError(EAX,SND_ERROR_OUT_OF_PCM_RAM);
		return;
	}

	if(0==soundData->totalBytes)
	{
		SND_SetError(EAX,SND_ERROR_NO_DATA_LENGTH);
		return;
	}

	status->PCMSound[status->numSound].addrInWaveRAM=status->instSoundLastAddr;
	status->PCMSound[status->numSound].snd=*soundData;
	++status->numSound;

	wave=((_Far unsigned char *)soundData);
	wave+=sizeof(struct PCM_Voice_Header);

	unsigned int totalBytes;
	if(soundData->loopLength)
	{
		totalBytes=_min(soundData->totalBytes,soundData->loopStart+soundData->loopLength);
	}
	else
	{
		totalBytes=soundData->totalBytes;
	}

	addr=status->instSoundLastAddr;
	for(i=0; i<totalBytes; ++i)
	{
		SND_WriteToWaveRAM(addr++,*(wave++));
	}
	for(i=0; i<32; ++i)
	{
		SND_WriteToWaveRAM(addr++,PCM_LOOP_STOP_CODE);
	}

	status->instSoundLastAddr+=required;

	SND_SetError(EAX,SND_NO_ERROR);
}

void SND_23H_PCM_SOUND_DELETE(
	unsigned int EDI,
	unsigned int ESI,
	unsigned int EBP,
	unsigned int ESP,
	unsigned int EBX,
	unsigned int EDX,
	unsigned int ECX,
	unsigned int EAX,
	unsigned int DS,
	unsigned int ES,
	unsigned int GS,
	unsigned int FS)
{
	_Far struct SND_Status *status=SND_GetStatus();

	if(0xFFFFFFFF==EDX)
	{
		status->numSound=0;
		status->instSoundLastAddr=0;
	}
	else
	{
		TSUGARU_BREAK;
	}
	SND_SetError(EAX,SND_NO_ERROR);
}

void SND_24H_PCM_REC_START(
	unsigned int EDI,
	unsigned int ESI,
	unsigned int EBP,
	unsigned int ESP,
	unsigned int EBX,
	unsigned int EDX,
	unsigned int ECX,
	unsigned int EAX,
	unsigned int DS,
	unsigned int ES,
	unsigned int GS,
	unsigned int FS)
{
	_Far struct SND_Work *work;
	_FP_SEG(work)=GS;
	_FP_OFF(work)=EDI;

	SND_SetError(EAX,SND_NO_ERROR);
		TSUGARU_BREAK;
}

void SND_26H_PCM_REC_STOP(
	unsigned int EDI,
	unsigned int ESI,
	unsigned int EBP,
	unsigned int ESP,
	unsigned int EBX,
	unsigned int EDX,
	unsigned int ECX,
	unsigned int EAX,
	unsigned int DS,
	unsigned int ES,
	unsigned int GS,
	unsigned int FS)
{
	_Far struct SND_Work *work;
	_FP_SEG(work)=GS;
	_FP_OFF(work)=EDI;

	SND_SetError(EAX,SND_NO_ERROR);
		TSUGARU_BREAK;
}

void SND_27H_PCM_PCM_VOICE_STOP(
	unsigned int EDI,
	unsigned int ESI,
	unsigned int EBP,
	unsigned int ESP,
	unsigned int EBX,
	unsigned int EDX,
	unsigned int ECX,
	unsigned int EAX,
	unsigned int DS,
	unsigned int ES,
	unsigned int GS,
	unsigned int FS)
{
	_Far struct SND_Work *work;
	_FP_SEG(work)=GS;
	_FP_OFF(work)=EDI;

	// Memo: Make sure to CLI while updating channel info and PCM registers.

	SND_SetError(EAX,SND_NO_ERROR);
		TSUGARU_BREAK;
}

void SND_28H_PCM_PCM_VOICE_STATUS(
	unsigned int EDI,
	unsigned int ESI,
	unsigned int EBP,
	unsigned int ESP,
	unsigned int EBX,
	unsigned int EDX,
	unsigned int ECX,
	unsigned int EAX,
	unsigned int DS,
	unsigned int ES,
	unsigned int GS,
	unsigned int FS)
{
	// Input
	//   BL=Channel (64-71)
	// Output
	//   DL=0:Not playing  Non-Zero:Playing
	//   AL=0 No Error
	//   AL=1 Wrong Channel

	unsigned char ch=(unsigned char)EBX;
	_Far struct SND_Status *sndStat=SND_GetStatus();

	if(ch<64 || 71<ch)
	{
		SND_SetError(EAX,SND_ERROR_WRONG_CH);
		return;
	}
	ch-=64;
	if(ch<SND_NUM_PCM_CHANNELS-sndStat->numVoiceModeChannels)
	{
		SND_SetError(EAX,SND_ERROR_WRONG_CH);
		return;
	}

	SET_LOW_BYTE(&EDX,sndStat->PCMCh[ch].playing);
	SND_SetError(EAX,SND_NO_ERROR);
}

void SND_29H_PCM_ABORT(
	unsigned int EDI,
	unsigned int ESI,
	unsigned int EBP,
	unsigned int ESP,
	unsigned int EBX,
	unsigned int EDX,
	unsigned int ECX,
	unsigned int EAX,
	unsigned int DS,
	unsigned int ES,
	unsigned int GS,
	unsigned int FS)
{
	_Far struct SND_Work *work;
	_FP_SEG(work)=GS;
	_FP_OFF(work)=EDI;

	SND_SetError(EAX,SND_NO_ERROR);
		TSUGARU_BREAK;
}

void SND_2AH_PCM_PCMRAM_TO_MAINRAM(
	unsigned int EDI,
	unsigned int ESI,
	unsigned int EBP,
	unsigned int ESP,
	unsigned int EBX,
	unsigned int EDX,
	unsigned int ECX,
	unsigned int EAX,
	unsigned int DS,
	unsigned int ES,
	unsigned int GS,
	unsigned int FS)
{
	_Far struct SND_Work *work;
	_FP_SEG(work)=GS;
	_FP_OFF(work)=EDI;

	SND_SetError(EAX,SND_NO_ERROR);
		TSUGARU_BREAK;
}

void SND_2BH_PCM_PCMRAM_TO_PCMRAM(
	unsigned int EDI,
	unsigned int ESI,
	unsigned int EBP,
	unsigned int ESP,
	unsigned int EBX,
	unsigned int EDX,
	unsigned int ECX,
	unsigned int EAX,
	unsigned int DS,
	unsigned int ES,
	unsigned int GS,
	unsigned int FS)
{
	_Far struct SND_Work *work;
	_FP_SEG(work)=GS;
	_FP_OFF(work)=EDI;

	SND_SetError(EAX,SND_NO_ERROR);
		TSUGARU_BREAK;
}

void SND_2CH_PCM_TRANSFER2(
	unsigned int EDI,
	unsigned int ESI,
	unsigned int EBP,
	unsigned int ESP,
	unsigned int EBX,
	unsigned int EDX,
	unsigned int ECX,
	unsigned int EAX,
	unsigned int DS,
	unsigned int ES,
	unsigned int GS,
	unsigned int FS)
{
	_Far unsigned char *mainram;
	_FP_SEG(mainram)=DS;
	_FP_OFF(mainram)=ESI;

	if(ECX<=0x10000||EBX<=0x10000)
	{
		SND_SetError(EAX,SND_ERROR_PARAMETER);
	}
	else if((ECX+EBX)<=0x10000)
	{
		while(EBX<0x10000)
		{
			// The difference between this function and SND_20H_PCM_WAVE_TRANSFER is that
			// it does not include the process of converting 0xff to 0xfe for the PCM data loop.
			SND_WriteToWaveRAM((EBX&0xffff),*mainram);
			EBX++;
			mainram++;
		}

		SND_SetError(EAX,SND_ERROR_OUT_OF_PCM_RAM);
	}
	else
	{
		for(int i=0;i<ECX;i++)
		{
			// The difference between this function and SND_20H_PCM_WAVE_TRANSFER is that
			// it does not include the process of converting 0xff to 0xfe for the PCM data loop.
			SND_WriteToWaveRAM((EBX&0xffff),*mainram);
			EBX++;
			mainram++;
		}

		SND_SetError(EAX,SND_NO_ERROR);
	}
}

void SND_25H_2EH_PCM_VOICE_PLAY(
	unsigned int EDI,
	unsigned int ESI,
	unsigned int EBP,
	unsigned int ESP,
	unsigned int EBX,
	unsigned int EDX,
	unsigned int ECX,
	unsigned int EAX,
	unsigned int DS,
	unsigned int ES,
	unsigned int GS,
	unsigned int FS)
{
	// BL=Channel
	// DH=Note
	// DL=Volume
	// DS:ESI=Data

	// What's the difference from 25H?

	unsigned char ch=(unsigned char)EBX;
	unsigned char note=(unsigned char)(EDX>>8);
	unsigned char volume=(unsigned char)EDX;
	_Far struct PCM_Voice_Header *sndData;
	_Far struct SND_Status *info=SND_GetStatus();

	unsigned char keyFlag;

	if(ch<64 || 71<ch)
	{
		SND_SetError(EAX,SND_ERROR_WRONG_CH);
		return;
	}
	ch-=64;
	keyFlag=(1<<ch);
	if((info->PCMKey&keyFlag)==0)
	{
		SND_SetError(EAX,SND_ERROR_KEY_ALREADY_ON);
		return;
	}

	if(ch<SND_NUM_PCM_CHANNELS-info->numVoiceModeChannels)
	{
		SND_SetError(EAX,SND_ERROR_CH_NOT_VOICE_MODE);
		return;
	}

	_FP_SEG(sndData)=DS;
	_FP_OFF(sndData)=ESI;

	_PUSHFD
	_CLI
	{
		info->PCMCh[ch].header=sndData;
		info->PCMCh[ch].playPtr=((_Far unsigned char *)sndData)+sizeof(struct PCM_Voice_Header);
		info->PCMCh[ch].nextFillBank=info->voiceChannelBank[ch];
		info->PCMCh[ch].playing=1;
		info->PCMCh[ch].curPos=0;

		{
			unsigned int bank=info->voiceChannelBank[ch];
			unsigned int transferSize=_min(PCM_BANK_SIZE*2-256,sndData->totalBytes);
			_Far unsigned char *waveRAM;
			_FP_SEG(waveRAM)=SEG_WAVE_RAM;
			_FP_OFF(waveRAM)=0;

			while(0<transferSize)
			{
				unsigned int oneTransferSize=_min(PCM_BANK_SIZE,transferSize);
				_outb(TOWNSIO_SOUND_PCM_CTRL,0x80|bank);
				MOVSB_FAR(waveRAM,info->PCMCh[ch].playPtr,oneTransferSize);
				if(oneTransferSize<PCM_BANK_SIZE)
				{
					waveRAM[oneTransferSize]=PCM_LOOP_STOP_CODE;
				}
				transferSize-=oneTransferSize;
				info->PCMCh[ch].curPos+=oneTransferSize;
				++bank;
			}
		}

		{
			unsigned short FD=0x1000;  // Address Step.  1x times=4096
			unsigned short ST=info->voiceChannelBank[ch];

			// ST is the high-byte of the starting address, therefore, ST<<=12 to make it full address,
			// and then ST>>=8 to take high-byte.  Overall, ST<<=4;
			ST<<=4;

			_outb(TOWNSIO_SOUND_PCM_CTRL,0xC0|ch); // Select PCM Channel
			_outb(TOWNSIO_SOUND_PCM_ENV,volume);  // Was it 0-127?  or 0-255?
			_outb(TOWNSIO_SOUND_PCM_PAN,info->PCMCh[ch].pan);  // Pan setting.

			_outb(TOWNSIO_SOUND_PCM_FDL,FD);
			_outb(TOWNSIO_SOUND_PCM_FDH,FD>>8);

			_outb(TOWNSIO_SOUND_PCM_ST,ST); // Starting address high-byte
			_outb(TOWNSIO_SOUND_PCM_LSH,ST); // Loop start address high-byte
			_outb(TOWNSIO_SOUND_PCM_LSL,0); // Loop start address low-byte

			_outb(TOWNSIO_SOUND_PCM_INT_MASK,info->voiceModeINTMask);

			info->PCMKey&=~keyFlag;
			_outb(TOWNSIO_SOUND_PCM_CH_ON_OFF,info->PCMKey);
		}
	}
	_POPFD

	SND_SetError(EAX,SND_NO_ERROR);
}

void SND_FM_INIT(
	unsigned int EDI,
	unsigned int ESI,
	unsigned int EBP,
	unsigned int ESP,
	unsigned int EBX,
	unsigned int EDX,
	unsigned int ECX,
	unsigned int EAX,
	unsigned int DS,
	unsigned int ES,
	unsigned int GS,
	unsigned int FS)
{
	_Far struct SND_Work *work;
	_FP_SEG(work)=GS;
	_FP_OFF(work)=EDI;

	SND_SetError(EAX,SND_NO_ERROR);
		TSUGARU_BREAK;
}

void SND_FM_REGWRITE(
	unsigned int EDI,
	unsigned int ESI,
	unsigned int EBP,
	unsigned int ESP,
	unsigned int EBX,
	unsigned int EDX,
	unsigned int ECX,
	unsigned int EAX,
	unsigned int DS,
	unsigned int ES,
	unsigned int GS,
	unsigned int FS)
{
	_Far struct SND_Work *work;
	_FP_SEG(work)=GS;
	_FP_OFF(work)=EDI;

	SND_SetError(EAX,SND_NO_ERROR);
		TSUGARU_BREAK;
}

void SND_JOY_IN(
	unsigned int EDI,
	unsigned int ESI,
	unsigned int EBP,
	unsigned int ESP,
	unsigned int EBX,
	unsigned int EDX,
	unsigned int ECX,
	unsigned int EAX,
	unsigned int DS,
	unsigned int ES,
	unsigned int GS,
	unsigned int FS)
{
	unsigned char pad;

	_outb(TOWNSIO_GAMEPORT_OUTPUT,PAD_OUT_CONST);
	if(0==((EDX>>8)&1))
	{
		pad=_inb(TOWNSIO_GAMEPORT_A_INPUT);
	}
	else
	{
		pad=_inb(TOWNSIO_GAMEPORT_B_INPUT);
	}

	pad|=0xC0;

	SET_LOW_BYTE(&EDX,pad);

	SND_SetError(EAX,SND_NO_ERROR);
}

void SND_JOY_IN_2(
	unsigned int EDI,
	unsigned int ESI,
	unsigned int EBP,
	unsigned int ESP,
	unsigned int EBX,
	unsigned int EDX,
	unsigned int ECX,
	unsigned int EAX,
	unsigned int DS,
	unsigned int ES,
	unsigned int GS,
	unsigned int FS)
{
	unsigned char port=(EDX>>8)&1,pad;
	_Far struct SND_Work *work;
	_FP_SEG(work)=GS;
	_FP_OFF(work)=EDI;

	_outb(TOWNSIO_GAMEPORT_OUTPUT,PAD_OUT_CONST);
	if(0==port)
	{
		pad=_inb(TOWNSIO_GAMEPORT_A_INPUT);
	}
	else
	{
		pad=_inb(TOWNSIO_GAMEPORT_B_INPUT);
	}

	pad|=0xC0;
	if(0==(pad&(PAD_LEFT|PAD_RIGHT)))
	{
		pad&=(~PAD_RUN);
	}
	if(0==(pad&(PAD_UP|PAD_DOWN)))
	{
		pad&=(~PAD_SELECT);
	}

	SET_LOW_BYTE(&EDX,pad);

	SND_SetError(EAX,SND_NO_ERROR);
}

void SND_JOY_OUT(
	unsigned int EDI,
	unsigned int ESI,
	unsigned int EBP,
	unsigned int ESP,
	unsigned int EBX,
	unsigned int EDX,
	unsigned int ECX,
	unsigned int EAX,
	unsigned int DS,
	unsigned int ES,
	unsigned int GS,
	unsigned int FS)
{
	_Far struct SND_Work *work;
	_FP_SEG(work)=GS;
	_FP_OFF(work)=EDI;

	SND_SetError(EAX,SND_NO_ERROR);
		TSUGARU_BREAK;
}

void SND_43H_ELEVOL_SET(
	unsigned int EDI,
	unsigned int ESI,
	unsigned int EBP,
	unsigned int ESP,
	unsigned int EBX,
	unsigned int EDX,
	unsigned int ECX,
	unsigned int EAX,
	unsigned int DS,
	unsigned int ES,
	unsigned int GS,
	unsigned int FS)
{
	unsigned int vol;
	_Far struct SND_Status *sysInfo=SND_GetStatus();

	switch(EBX&0xFF)
	{
	case 0: // LINE IN
		// Left volume
		vol=((EDX>>8)&0x7f);
		_outb(TOWNSIO_ELEVOL_1_COM,(((~vol)>>2)&0x10)|5);
		_outb(TOWNSIO_ELEVOL_1_DATA,vol&0x3f);
		// Right volume
		vol=(EDX&0x7f);
		_outb(TOWNSIO_ELEVOL_1_COM,(((~vol)>>2)&0x10)|4);
		_outb(TOWNSIO_ELEVOL_1_DATA,vol&0x3f);
		sysInfo->elevol_mute|=0xc;
		break;
	case 1: // CD IN
		// Left volume
		vol=((EDX>>8)&0x7f);
		_outb(TOWNSIO_ELEVOL_2_COM,(((~vol)>>2)&0x10)|5);
		_outb(TOWNSIO_ELEVOL_2_DATA,vol&0x3f);
		// Right volume
		vol=(EDX&0x7f);
		_outb(TOWNSIO_ELEVOL_2_COM,(((~vol)>>2)&0x10)|4);
		_outb(TOWNSIO_ELEVOL_2_DATA,vol&0x3f);
		sysInfo->elevol_mute|=0x30;
		break;
	case 2: // MIC IN
		vol=((EDX>>8)&0x7f);
		_outb(TOWNSIO_ELEVOL_2_COM,(((~vol)>>2)&0x10)|6);
		_outb(TOWNSIO_ELEVOL_2_DATA,vol&0x3f);
		sysInfo->elevol_mute|=0x40;
		break;
	case 3: // MODEM IN
		vol=((EDX>>8)&0x7f);
		_outb(TOWNSIO_ELEVOL_2_COM,(((~vol)>>2)&0x10)|7);
		_outb(TOWNSIO_ELEVOL_2_DATA,vol&0x3f);
		sysInfo->elevol_mute|=0x80;
		break;
	}

	SND_SetError(EAX,SND_NO_ERROR);
}

void SND_44H_ELEVOL_INIT(
	unsigned int EDI,
	unsigned int ESI,
	unsigned int EBP,
	unsigned int ESP,
	unsigned int EBX,
	unsigned int EDX,
	unsigned int ECX,
	unsigned int EAX,
	unsigned int DS,
	unsigned int ES,
	unsigned int GS,
	unsigned int FS)
{
	_Far struct SND_Status *sysInfo=SND_GetStatus();

	// FM & PCM mute
	_outb(TOWNSIO_SOUND_MUTE,0);

	// Line left mute & min volume
	_outb(TOWNSIO_ELEVOL_1_COM,0x11);
	_outb(TOWNSIO_ELEVOL_1_DATA,0);
	// Line right mute & min volume
	_outb(TOWNSIO_ELEVOL_1_COM,0x10);
	_outb(TOWNSIO_ELEVOL_1_DATA,0);
	// CD left mute & min volume
	_outb(TOWNSIO_ELEVOL_2_COM,0x11);
	_outb(TOWNSIO_ELEVOL_2_COM,0);
	// CD right mute & min volume
	_outb(TOWNSIO_ELEVOL_2_COM,0x10);
	_outb(TOWNSIO_ELEVOL_2_COM,0);
	// Mic mute & min volume
	_outb(TOWNSIO_ELEVOL_2_COM,0x12);
	_outb(TOWNSIO_ELEVOL_2_COM,0);
	// Modem mute & min volume
	_outb(TOWNSIO_ELEVOL_2_COM,0x13);
	_outb(TOWNSIO_ELEVOL_2_COM,0);

	sysInfo->elevol_mute=0;

	SND_SetError(EAX,SND_NO_ERROR);
}

void SND_45H_ELEVOL_READ(
	unsigned int EDI,
	unsigned int ESI,
	unsigned int EBP,
	unsigned int ESP,
	unsigned int EBX,
	unsigned int EDX,
	unsigned int ECX,
	unsigned int EAX,
	unsigned int DS,
	unsigned int ES,
	unsigned int GS,
	unsigned int FS)
{
	_Far struct SND_Status *sysInfo=SND_GetStatus();

	unsigned int vlmNum=EBX&0xFF;
	unsigned short DH,DL;
	unsigned char EN;

	// vlmNum 0: LINE IN
	//        1: CD IN
	//        2: MIC IN
	//        3: MODEM IN

	// DH=Left 0 to 127
	// DL=RIght 0 to 127
	// What?  No left and right for MIC and MODEM.
	switch(vlmNum)
	{
	case 0: // LINE IN
		EN=((sysInfo->elevol_mute&SND_MUTE_LINE_L) ? 4 : 0);
		_outb(TOWNSIO_ELEVOL_1_COM,EN);
		DH=_inb(TOWNSIO_ELEVOL_1_DATA);
		EN=((sysInfo->elevol_mute&SND_MUTE_LINE_R) ? 4 : 0);
		_outb(TOWNSIO_ELEVOL_1_COM,EN|1);
		DL=_inb(TOWNSIO_ELEVOL_1_DATA);
		break;
	case 1: // CD IN
		EN=((sysInfo->elevol_mute&SND_MUTE_CD_L) ? 4 : 0);
		_outb(TOWNSIO_ELEVOL_2_COM,EN);
		DH=_inb(TOWNSIO_ELEVOL_2_DATA);
		EN=((sysInfo->elevol_mute&SND_MUTE_CD_R) ? 4 : 0);
		_outb(TOWNSIO_ELEVOL_2_COM,EN|1);
		DL=_inb(TOWNSIO_ELEVOL_2_DATA);
		break;
	case 2: // MIC IN
		EN=((sysInfo->elevol_mute&SND_MUTE_MIC) ? 4 : 0);
		_outb(TOWNSIO_ELEVOL_2_COM,EN|2);
		DH=DL=_inb(TOWNSIO_ELEVOL_2_DATA);
		break;
	case 3: // MODEM IN
		EN=((sysInfo->elevol_mute&SND_MUTE_MODEM) ? 4 : 0);
		_outb(TOWNSIO_ELEVOL_2_COM,EN|3);
		DH=DL=_inb(TOWNSIO_ELEVOL_2_DATA);
		break;
	}

	DL&=0x3F;
	DH&=0x3F;

	DL=(DL<<1)|(DL&1);
	DH=(DH<<1)|(DH&1);

	SET_LOW_WORD(&EDX,(DH<<8)|DL);

	SND_SetError(EAX,SND_NO_ERROR);
}

void SND_46H_ELEVOL_MUTE(
	unsigned int EDI,
	unsigned int ESI,
	unsigned int EBP,
	unsigned int ESP,
	unsigned int EBX,
	unsigned int EDX,
	unsigned int ECX,
	unsigned int EAX,
	unsigned int DS,
	unsigned int ES,
	unsigned int GS,
	unsigned int FS)
{
	_Far struct SND_Status *sysInfo=SND_GetStatus();
	unsigned char flags=(unsigned char)EBX;
	unsigned char EN;
	_Far struct SND_Work *work;
	_FP_SEG(work)=GS;
	_FP_OFF(work)=EDI;

	// |MODEM|MIC|CDLeft|CERight|LINELeft|LINERight|FM|PCM|

	_outb(TOWNSIO_SOUND_MUTE,flags&3);

	EN=((flags&SND_MUTE_LINE_L) ? 4 : 0);
	_outb(TOWNSIO_ELEVOL_1_COM,EN);
	EN=((flags&SND_MUTE_LINE_R) ? 4 : 0);
	_outb(TOWNSIO_ELEVOL_1_COM,EN|1);
	EN=((flags&SND_MUTE_CD_L) ? 4 : 0);
	_outb(TOWNSIO_ELEVOL_2_COM,EN);
	EN=((flags&SND_MUTE_CD_R) ? 4 : 0);
	_outb(TOWNSIO_ELEVOL_2_COM,EN|1);
	EN=((flags&SND_MUTE_MIC) ? 4 : 0);
	_outb(TOWNSIO_ELEVOL_2_COM,EN|2);
	EN=((flags&SND_MUTE_MODEM) ? 4 : 0);
	_outb(TOWNSIO_ELEVOL_2_COM,EN|3);

	sysInfo->elevol_mute=flags;

	SND_SetError(EAX,SND_NO_ERROR);
}

void SND_49H_ELEVOL_ALL_MUTE(
	unsigned int EDI,
	unsigned int ESI,
	unsigned int EBP,
	unsigned int ESP,
	unsigned int EBX,
	unsigned int EDX,
	unsigned int ECX,
	unsigned int EAX,
	unsigned int DS,
	unsigned int ES,
	unsigned int GS,
	unsigned int FS)
{
	_Far struct SND_Work *work;
	_FP_SEG(work)=GS;
	_FP_OFF(work)=EDI;

	SND_SetError(EAX,SND_NO_ERROR);
		TSUGARU_BREAK;
}

void SND_4AH_UNPUBLISHED_FUNCTION(
	unsigned int EDI,
	unsigned int ESI,
	unsigned int EBP,
	unsigned int ESP,
	unsigned int EBX,
	unsigned int EDX,
	unsigned int ECX,
	unsigned int EAX,
	unsigned int DS,
	unsigned int ES,
	unsigned int GS,
	unsigned int FS)
{
}

void SND_48H_UNKNOWN_USED_BY_VSGP(
	unsigned int EDI,
	unsigned int ESI,
	unsigned int EBP,
	unsigned int ESP,
	unsigned int EBX,
	unsigned int EDX,
	unsigned int ECX,
	unsigned int EAX,
	unsigned int DS,
	unsigned int ES,
	unsigned int GS,
	unsigned int FS)
{
	// WTF!?
	TSUGARU_STATE;
}

void SND_ENVELOPE_INT_HANDLER(
	unsigned int EDI,
	unsigned int ESI,
	unsigned int EBP,
	unsigned int ESP,
	unsigned int EBX,
	unsigned int EDX,
	unsigned int ECX,
	unsigned int EAX,
	unsigned int DS,
	unsigned int ES,
	unsigned int GS,
	unsigned int FS)
{
	SND_PCM_Envelope_Handler();
	SND_SetError(EAX,SND_NO_ERROR);
}

void SND_PCM_Envelope_Handler(void)
{
	// When calling directly from Sound-Interrupt Manager BIOS, make sure to CLI.

	_Far struct SND_Status *stat=SND_GetStatus();
	for(int ch=0; ch+stat->numVoiceModeChannels<SND_NUM_PCM_CHANNELS; ++ch)
	{
		if(stat->PCMCh[ch].playing)
		{
			switch(stat->PCMCh[ch].phase)
			{
			case 0: // Attack
				if(0==stat->PCMCh[ch].phaseStepLeft)
				{
					++stat->PCMCh[ch].phase;
					stat->PCMCh[ch].phaseStepLeft=stat->PCMCh[ch].env.DR;
					stat->PCMCh[ch].dx=stat->PCMCh[ch].env.DR;
					stat->PCMCh[ch].dy=(stat->PCMCh[ch].env.SL<stat->PCMCh[ch].env.TL ? stat->PCMCh[ch].env.TL-stat->PCMCh[ch].env.SL : 0);
					stat->PCMCh[ch].balance=0;
					break;
				}
				stat->PCMCh[ch].balance+=stat->PCMCh[ch].dy;
				while(0<stat->PCMCh[ch].balance)
				{
					stat->PCMCh[ch].envVol=_min(254,stat->PCMCh[ch].envVol)+1;
					stat->PCMCh[ch].balance-=stat->PCMCh[ch].dx;
				}
				--stat->PCMCh[ch].phaseStepLeft;
				break;
			case 1: // Decay
				if(0==stat->PCMCh[ch].phaseStepLeft)
				{
					++stat->PCMCh[ch].phase;
					stat->PCMCh[ch].dx=stat->PCMCh[ch].env.SR;
					stat->PCMCh[ch].dy=stat->PCMCh[ch].envVol;
					stat->PCMCh[ch].balance=0;
					break;
				}
				stat->PCMCh[ch].balance+=stat->PCMCh[ch].dy;
				while(0<stat->PCMCh[ch].balance && 0<stat->PCMCh[ch].envVol)
				{
					stat->PCMCh[ch].envVol=_max(1,stat->PCMCh[ch].envVol)-1;
					stat->PCMCh[ch].balance-=stat->PCMCh[ch].dx;
				}
				--stat->PCMCh[ch].phaseStepLeft;
				break;
			case 2: // Sustain
			case 3: // Release
				stat->PCMCh[ch].balance+=stat->PCMCh[ch].dy;
				while(0<stat->PCMCh[ch].balance && 0<stat->PCMCh[ch].envVol)
				{
					stat->PCMCh[ch].envVol=_max(1,stat->PCMCh[ch].envVol)-1;
					stat->PCMCh[ch].balance-=stat->PCMCh[ch].dx;
				}
				break;
			}
			if(0==stat->PCMCh[ch].envVol)
			{
				stat->PCMCh[ch].playing=0;
				stat->PCMKey|=(1<<ch);
				_outb(TOWNSIO_SOUND_PCM_CH_ON_OFF,stat->PCMKey);
			}
			else
			{
				_outb(TOWNSIO_SOUND_PCM_ENV,stat->PCMCh[ch].envVol);
			}
		}
	}
}

void SND_VOICE_INT_HANDLER(
	unsigned int EDI,
	unsigned int ESI,
	unsigned int EBP,
	unsigned int ESP,
	unsigned int EBX,
	unsigned int EDX,
	unsigned int ECX,
	unsigned int EAX,
	unsigned int DS,
	unsigned int ES,
	unsigned int GS,
	unsigned int FS)
{
	SND_PCM_Voice_Mode_Interrupt();
	SND_SetError(EAX,SND_NO_ERROR);
}

void SND_PCM_Voice_Mode_Interrupt(void)
{
	// When calling directly from Sound-Interrupt Manager BIOS, make sure to CLI.

	_Far struct SND_Status *stat=SND_GetStatus();
	unsigned char INTBank=_inb(TOWNSIO_SOUND_PCM_INT);
	int i,ch=7;

	for(i=0; i<stat->numVoiceModeChannels; ++i)
	{
		unsigned char CHFlag=(1<<ch);
		unsigned char INTBankFlag=1;
		INTBankFlag<<=(stat->voiceChannelBank[ch]/2);
		if(INTBank&INTBankFlag)
		{
			if(stat->PCMCh[ch].header->totalBytes<=stat->PCMCh[ch].curPos)
			{
				// Not respecting loop for the time being.
				stat->PCMKey|=CHFlag;
				_outb(TOWNSIO_SOUND_PCM_CH_ON_OFF,stat->PCMKey); // Key Off
				stat->PCMCh[ch].playing=0;
			}
			else
			{
				unsigned int bytesLeft=stat->PCMCh[ch].header->totalBytes-stat->PCMCh[ch].curPos;
				unsigned int transferSize=PCM_BANK_SIZE;
				_Far unsigned char *waveRAM;
				_FP_SEG(waveRAM)=SEG_WAVE_RAM;
				_FP_OFF(waveRAM)=0;

				if(stat->PCMCh[ch].nextFillBank&1)
				{
					transferSize-=256;
				}
				transferSize=_min(transferSize,bytesLeft);

				_outb(TOWNSIO_SOUND_PCM_CTRL,0x80|stat->PCMCh[ch].nextFillBank);
				MOVSB_FAR(waveRAM,stat->PCMCh[ch].playPtr+stat->PCMCh[ch].curPos,transferSize);
				if(transferSize<PCM_BANK_SIZE)
				{
					waveRAM[transferSize]=PCM_LOOP_STOP_CODE;
				}

				stat->PCMCh[ch].curPos+=transferSize;
				stat->PCMCh[ch].nextFillBank^=1;
			}
		}
		--ch;
	}
}

void SND_NOP(
	unsigned int EDI,
	unsigned int ESI,
	unsigned int EBP,
	unsigned int ESP,
	unsigned int EBX,
	unsigned int EDX,
	unsigned int ECX,
	unsigned int EAX,
	unsigned int DS,
	unsigned int ES,
	unsigned int GS,
	unsigned int FS)
{
	// Not supposed to be called.
		TSUGARU_BREAK;
}

static struct SND_Status status=
{
	// Sound
	0,  // elevol_mute
	0,  // REG27H
	0x10000, // voiceModeStartAddr
	0,  // instSoundLastAddr

	0,  // voiceModeBank
	0,  // numVoiceModeChannels
	0,  // PCMKey
	{0,0,0,0,0,0,0,0}, // voiceModeChannelBnak
};

_Far struct SND_Status *SND_GetStatus(void)
{
	_Far struct SND_Status *ptr;
	_FP_SEG(ptr)=SEG_TGBIOS_DATA;
	_FP_OFF(ptr)=(unsigned int)&status;
	return ptr;
}

static const unsigned short FNUM_BLOCK[256]=
{
	0x0000|(0<<11),
	0x0000|(0<<11),
	0x0000|(0<<11),
	0x0000|(0<<11),
	0x0000|(0<<11),
	0x0000|(0<<11),
	0x0000|(0<<11),
	0x0000|(0<<11),
	0x0000|(0<<11),
	0x0000|(0<<11),
	0x0000|(0<<11),
	0x0000|(0<<11),
	0x0268|(0<<11),
	0x028E|(0<<11),
	0x02B5|(0<<11),
	0x02DE|(0<<11),
	0x030A|(0<<11),
	0x0338|(0<<11),
	0x0369|(0<<11),
	0x039D|(0<<11),
	0x03D4|(0<<11),
	0x040E|(0<<11),
	0x044C|(0<<11),
	0x048D|(0<<11),
	0x026B|(1<<11),
	0x028E|(1<<11),
	0x02B5|(1<<11),
	0x02DE|(1<<11),
	0x030A|(1<<11),
	0x0338|(1<<11),
	0x0369|(1<<11),
	0x039D|(1<<11),
	0x03D4|(1<<11),
	0x040E|(1<<11),
	0x044C|(1<<11),
	0x048D|(1<<11),
	0x026B|(2<<11),
	0x028E|(2<<11),
	0x02B5|(2<<11),
	0x02DE|(2<<11),
	0x030A|(2<<11),
	0x0338|(2<<11),
	0x0369|(2<<11),
	0x039D|(2<<11),
	0x03D4|(2<<11),
	0x040E|(2<<11),
	0x044C|(2<<11),
	0x048D|(2<<11),
	0x026B|(3<<11),
	0x028E|(3<<11),
	0x02B5|(3<<11),
	0x02DE|(3<<11),
	0x030A|(3<<11),
	0x0338|(3<<11),
	0x0369|(3<<11),
	0x039D|(3<<11),
	0x03D4|(3<<11),
	0x040E|(3<<11),
	0x044C|(3<<11),
	0x048D|(3<<11),
	0x026B|(4<<11),
	0x028E|(4<<11),
	0x02B5|(4<<11),
	0x02DE|(4<<11),
	0x030A|(4<<11),
	0x0338|(4<<11),
	0x0369|(4<<11),
	0x039D|(4<<11),
	0x03D4|(4<<11),
	0x040E|(4<<11),
	0x044C|(4<<11),
	0x048D|(4<<11),
	0x026B|(5<<11),
	0x028E|(5<<11),
	0x02B5|(5<<11),
	0x02DE|(5<<11),
	0x030A|(5<<11),
	0x0338|(5<<11),
	0x0369|(5<<11),
	0x039D|(5<<11),
	0x03D4|(5<<11),
	0x040E|(5<<11),
	0x044C|(5<<11),
	0x048D|(5<<11),
	0x026B|(6<<11),
	0x028E|(6<<11),
	0x02B5|(6<<11),
	0x02DE|(6<<11),
	0x030A|(6<<11),
	0x0338|(6<<11),
	0x0369|(6<<11),
	0x039D|(6<<11),
	0x03D4|(6<<11),
	0x040E|(6<<11),
	0x044C|(6<<11),
	0x048D|(6<<11),
	0x026B|(7<<11),
	0x028E|(7<<11),
	0x02B5|(7<<11),
	0x02DE|(7<<11),
	0x030A|(7<<11),
	0x0338|(7<<11),
	0x0369|(7<<11),
	0x039D|(7<<11),
	0x03D4|(7<<11),
	0x040E|(7<<11),
	0x044C|(7<<11),
	0x048D|(7<<11),
	0x048D|(7<<11),
	0x048D|(7<<11),
	0x048D|(7<<11),
	0x048D|(7<<11),
	0x048D|(7<<11),
	0x048D|(7<<11),
	0x048D|(7<<11),
	0x048D|(7<<11),
	0x048D|(7<<11),
	0x048D|(7<<11),
	0x048D|(7<<11),
	0x048D|(7<<11),
	0x048D|(7<<11),
	0x048D|(7<<11),
	0x048D|(7<<11),
	0x048D|(7<<11),
	0x048D|(7<<11),
	0x048D|(7<<11),
	0x048D|(7<<11),
	0x048D|(7<<11),
};

static unsigned short GetFNUM_BLOCK_from_Number(unsigned char num)
{
	_Far unsigned short *ptr;
	_FP_SEG(ptr)=SEG_TGBIOS_CODE;
	_FP_OFF(ptr)=(unsigned int)FNUM_BLOCK;
	return ptr[num];
}

static unsigned int freqScale[256]=
{
	0x00000285, // -128 (0.000615)
	0x000002ab, // -127 (0.000652)
	0x000002d4, // -126 (0.000691)
	0x000002ff, // -125 (0.000732)
	0x0000032c, // -124 (0.000775)
	0x0000035d, // -123 (0.000821)
	0x00000390, // -122 (0.000870)
	0x000003c6, // -121 (0.000922)
	0x00000400, // -120 (0.000977)
	0x0000043c, // -119 (0.001035)
	0x0000047d, // -118 (0.001096)
	0x000004c1, // -117 (0.001161)
	0x0000050a, // -116 (0.001230)
	0x00000556, // -115 (0.001304)
	0x000005a8, // -114 (0.001381)
	0x000005fe, // -113 (0.001463)
	0x00000659, // -112 (0.001550)
	0x000006ba, // -111 (0.001642)
	0x00000720, // -110 (0.001740)
	0x0000078d, // -109 (0.001844)
	0x00000800, // -108 (0.001953)
	0x00000879, // -107 (0.002069)
	0x000008fa, // -106 (0.002192)
	0x00000983, // -105 (0.002323)
	0x00000a14, // -104 (0.002461)
	0x00000aad, // -103 (0.002607)
	0x00000b50, // -102 (0.002762)
	0x00000bfc, // -101 (0.002926)
	0x00000cb2, // -100 (0.003100)
	0x00000d74, // -99 (0.003285)
	0x00000e41, // -98 (0.003480)
	0x00000f1a, // -97 (0.003687)
	0x00001000, // -96 (0.003906)
	0x000010f3, // -95 (0.004139)
	0x000011f5, // -94 (0.004385)
	0x00001306, // -93 (0.004645)
	0x00001428, // -92 (0.004922)
	0x0000155b, // -91 (0.005214)
	0x000016a0, // -90 (0.005524)
	0x000017f9, // -89 (0.005853)
	0x00001965, // -88 (0.006201)
	0x00001ae8, // -87 (0.006570)
	0x00001c82, // -86 (0.006960)
	0x00001e34, // -85 (0.007374)
	0x00002000, // -84 (0.007812)
	0x000021e7, // -83 (0.008277)
	0x000023eb, // -82 (0.008769)
	0x0000260d, // -81 (0.009291)
	0x00002851, // -80 (0.009843)
	0x00002ab7, // -79 (0.010428)
	0x00002d41, // -78 (0.011049)
	0x00002ff2, // -77 (0.011706)
	0x000032cb, // -76 (0.012402)
	0x000035d1, // -75 (0.013139)
	0x00003904, // -74 (0.013920)
	0x00003c68, // -73 (0.014748)
	0x00004000, // -72 (0.015625)
	0x000043ce, // -71 (0.016554)
	0x000047d6, // -70 (0.017538)
	0x00004c1b, // -69 (0.018581)
	0x000050a2, // -68 (0.019686)
	0x0000556e, // -67 (0.020857)
	0x00005a82, // -66 (0.022097)
	0x00005fe4, // -65 (0.023411)
	0x00006597, // -64 (0.024803)
	0x00006ba2, // -63 (0.026278)
	0x00007208, // -62 (0.027841)
	0x000078d0, // -61 (0.029496)
	0x00008000, // -60 (0.031250)
	0x0000879c, // -59 (0.033108)
	0x00008fac, // -58 (0.035077)
	0x00009837, // -57 (0.037163)
	0x0000a145, // -56 (0.039373)
	0x0000aadc, // -55 (0.041714)
	0x0000b504, // -54 (0.044194)
	0x0000bfc8, // -53 (0.046822)
	0x0000cb2f, // -52 (0.049606)
	0x0000d744, // -51 (0.052556)
	0x0000e411, // -50 (0.055681)
	0x0000f1a1, // -49 (0.058992)
	0x00010000, // -48 (0.062500)
	0x00010f38, // -47 (0.066216)
	0x00011f59, // -46 (0.070154)
	0x0001306f, // -45 (0.074325)
	0x0001428a, // -44 (0.078745)
	0x000155b8, // -43 (0.083427)
	0x00016a09, // -42 (0.088388)
	0x00017f91, // -41 (0.093644)
	0x0001965f, // -40 (0.099213)
	0x0001ae89, // -39 (0.105112)
	0x0001c823, // -38 (0.111362)
	0x0001e343, // -37 (0.117984)
	0x00020000, // -36 (0.125000)
	0x00021e71, // -35 (0.132433)
	0x00023eb3, // -34 (0.140308)
	0x000260df, // -33 (0.148651)
	0x00028514, // -32 (0.157490)
	0x0002ab70, // -31 (0.166855)
	0x0002d413, // -30 (0.176777)
	0x0002ff22, // -29 (0.187288)
	0x00032cbf, // -28 (0.198425)
	0x00035d13, // -27 (0.210224)
	0x00039047, // -26 (0.222725)
	0x0003c686, // -25 (0.235969)
	0x00040000, // -24 (0.250000)
	0x00043ce3, // -23 (0.264866)
	0x00047d66, // -22 (0.280616)
	0x0004c1bf, // -21 (0.297302)
	0x00050a28, // -20 (0.314980)
	0x000556e0, // -19 (0.333710)
	0x0005a827, // -18 (0.353553)
	0x0005fe44, // -17 (0.374577)
	0x0006597f, // -16 (0.396850)
	0x0006ba27, // -15 (0.420448)
	0x0007208f, // -14 (0.445449)
	0x00078d0d, // -13 (0.471937)
	0x00080000, // -12 (0.500000)
	0x000879c7, // -11 (0.529732)
	0x0008facd, // -10 (0.561231)
	0x0009837f, // -9 (0.594604)
	0x000a1451, // -8 (0.629961)
	0x000aadc0, // -7 (0.667420)
	0x000b504f, // -6 (0.707107)
	0x000bfc88, // -5 (0.749154)
	0x000cb2ff, // -4 (0.793701)
	0x000d744f, // -3 (0.840896)
	0x000e411f, // -2 (0.890899)
	0x000f1a1b, // -1 (0.943874)
	0x00100000, // 0 (1.000000)
	0x0010f38f, // 1 (1.059463)
	0x0011f59a, // 2 (1.122462)
	0x001306fe, // 3 (1.189207)
	0x001428a2, // 4 (1.259921)
	0x00155b81, // 5 (1.334840)
	0x0016a09e, // 6 (1.414214)
	0x0017f910, // 7 (1.498307)
	0x001965fe, // 8 (1.587401)
	0x001ae89f, // 9 (1.681793)
	0x001c823e, // 10 (1.781797)
	0x001e3437, // 11 (1.887749)
	0x00200000, // 12 (2.000000)
	0x0021e71f, // 13 (2.118926)
	0x0023eb35, // 14 (2.244924)
	0x00260dfc, // 15 (2.378414)
	0x00285145, // 16 (2.519842)
	0x002ab702, // 17 (2.669680)
	0x002d413c, // 18 (2.828427)
	0x002ff221, // 19 (2.996614)
	0x0032cbfd, // 20 (3.174802)
	0x0035d13f, // 21 (3.363586)
	0x0039047c, // 22 (3.563595)
	0x003c686f, // 23 (3.775497)
	0x00400000, // 24 (4.000000)
	0x0043ce3e, // 25 (4.237852)
	0x0047d66b, // 26 (4.489848)
	0x004c1bf8, // 27 (4.756828)
	0x0050a28b, // 28 (5.039684)
	0x00556e04, // 29 (5.339359)
	0x005a8279, // 30 (5.656854)
	0x005fe443, // 31 (5.993228)
	0x006597fa, // 32 (6.349604)
	0x006ba27e, // 33 (6.727171)
	0x007208f8, // 34 (7.127190)
	0x0078d0df, // 35 (7.550995)
	0x00800000, // 36 (8.000000)
	0x00879c7c, // 37 (8.475705)
	0x008facd6, // 38 (8.979696)
	0x009837f0, // 39 (9.513657)
	0x00a14517, // 40 (10.079368)
	0x00aadc08, // 41 (10.678719)
	0x00b504f3, // 42 (11.313708)
	0x00bfc886, // 43 (11.986457)
	0x00cb2ff5, // 44 (12.699208)
	0x00d744fc, // 45 (13.454343)
	0x00e411f0, // 46 (14.254379)
	0x00f1a1bf, // 47 (15.101989)
	0x01000000, // 48 (16.000000)
	0x010f38f9, // 49 (16.951410)
	0x011f59ac, // 50 (17.959393)
	0x01306fe0, // 51 (19.027314)
	0x01428a2f, // 52 (20.158737)
	0x0155b810, // 53 (21.357438)
	0x016a09e6, // 54 (22.627417)
	0x017f910d, // 55 (23.972913)
	0x01965fea, // 56 (25.398417)
	0x01ae89f9, // 57 (26.908685)
	0x01c823e0, // 58 (28.508759)
	0x01e3437e, // 59 (30.203978)
	0x02000000, // 60 (32.000000)
	0x021e71f2, // 61 (33.902819)
	0x023eb358, // 62 (35.918786)
	0x0260dfc1, // 63 (38.054628)
	0x0285145f, // 64 (40.317474)
	0x02ab7021, // 65 (42.714875)
	0x02d413cc, // 66 (45.254834)
	0x02ff221a, // 67 (47.945826)
	0x032cbfd4, // 68 (50.796834)
	0x035d13f3, // 69 (53.817371)
	0x039047c0, // 70 (57.017518)
	0x03c686fc, // 71 (60.407956)
	0x04000000, // 72 (64.000000)
	0x043ce3e4, // 73 (67.805638)
	0x047d66b0, // 74 (71.837571)
	0x04c1bf82, // 75 (76.109255)
	0x050a28be, // 76 (80.634947)
	0x0556e042, // 77 (85.429751)
	0x05a82799, // 78 (90.509668)
	0x05fe4435, // 79 (95.891653)
	0x06597fa9, // 80 (101.593667)
	0x06ba27e6, // 81 (107.634741)
	0x07208f81, // 82 (114.035036)
	0x078d0df9, // 83 (120.815912)
	0x08000000, // 84 (128.000000)
	0x0879c7c9, // 85 (135.611276)
	0x08facd61, // 86 (143.675142)
	0x09837f05, // 87 (152.218511)
	0x0a14517c, // 88 (161.269894)
	0x0aadc084, // 89 (170.859501)
	0x0b504f33, // 90 (181.019336)
	0x0bfc886b, // 91 (191.783306)
	0x0cb2ff52, // 92 (203.187335)
	0x0d744fcc, // 93 (215.269482)
	0x0e411f03, // 94 (228.070072)
	0x0f1a1bf3, // 95 (241.631824)
	0x10000000, // 96 (256.000000)
	0x10f38f92, // 97 (271.222552)
	0x11f59ac3, // 98 (287.350284)
	0x1306fe0a, // 99 (304.437021)
	0x1428a2f9, // 100 (322.539789)
	0x155b8108, // 101 (341.719003)
	0x16a09e66, // 102 (362.038672)
	0x17f910d7, // 103 (383.566612)
	0x1965fea5, // 104 (406.374669)
	0x1ae89f99, // 105 (430.538965)
	0x1c823e07, // 106 (456.140144)
	0x1e3437e7, // 107 (483.263648)
	0x20000000, // 108 (512.000000)
	0x21e71f25, // 109 (542.445104)
	0x23eb3587, // 110 (574.700569)
	0x260dfc14, // 111 (608.874043)
	0x285145f3, // 112 (645.079578)
	0x2ab70211, // 113 (683.438005)
	0x2d413ccc, // 114 (724.077344)
	0x2ff221ae, // 115 (767.133223)
	0x32cbfd4a, // 116 (812.749339)
	0x35d13f32, // 117 (861.077929)
	0x39047c0e, // 118 (912.280287)
	0x3c686fce, // 119 (966.527296)
	0x40000000, // 120 (1024.000000)
	0x43ce3e4b, // 121 (1084.890209)
	0x47d66b0f, // 122 (1149.401137)
	0x4c1bf828, // 123 (1217.748086)
	0x50a28be6, // 124 (1290.159155)
	0x556e0423, // 125 (1366.876011)
	0x5a827999, // 126 (1448.154688)
	0x5fe4435d, // 127 (1534.266447)
};

static unsigned int GetFreqScale(unsigned int note,unsigned int baseNote)
{
	if(0x80&(note|baseNote)) // Both needs to be between 0 to 127.
	{
		return 0x100000;
	}

	unsigned int noteDiff; // =0x80+note-baseNote
	noteDiff=note;
	noteDiff+=0x80;
	noteDiff-=baseNote;

	_Far unsigned int *ptr;
	_FP_SEG(ptr)=SEG_TGBIOS_CODE;
	_FP_OFF(ptr)=(unsigned int)freqScale;
	return ptr[noteDiff];
}

unsigned int pitchBendTable[257]=
{
	0x00004000, // 0.250000
	0x000040b2, // 0.252722
	0x00004166, // 0.255474
	0x0000421d, // 0.258256
	0x000042d5, // 0.261068
	0x0000438f, // 0.263911
	0x0000444c, // 0.266785
	0x0000450a, // 0.269690
	0x000045ca, // 0.272627
	0x0000468d, // 0.275596
	0x00004752, // 0.278597
	0x00004818, // 0.281630
	0x000048e1, // 0.284697
	0x000049ad, // 0.287797
	0x00004a7a, // 0.290931
	0x00004b4a, // 0.294099
	0x00004c1b, // 0.297302
	0x00004cf0, // 0.300539
	0x00004dc6, // 0.303812
	0x00004e9f, // 0.307120
	0x00004f7a, // 0.310464
	0x00005058, // 0.313845
	0x00005138, // 0.317263
	0x0000521a, // 0.320718
	0x000052ff, // 0.324210
	0x000053e6, // 0.327740
	0x000054d0, // 0.331309
	0x000055bd, // 0.334917
	0x000056ac, // 0.338564
	0x0000579d, // 0.342251
	0x00005891, // 0.345977
	0x00005988, // 0.349745
	0x00005a82, // 0.353553
	0x00005b7e, // 0.357403
	0x00005c7d, // 0.361295
	0x00005d7f, // 0.365229
	0x00005e84, // 0.369207
	0x00005f8b, // 0.373227
	0x00006096, // 0.377291
	0x000061a3, // 0.381400
	0x000062b3, // 0.385553
	0x000063c6, // 0.389751
	0x000064dc, // 0.393995
	0x000065f6, // 0.398286
	0x00006712, // 0.402623
	0x00006831, // 0.407007
	0x00006954, // 0.411439
	0x00006a79, // 0.415919
	0x00006ba2, // 0.420448
	0x00006cce, // 0.425027
	0x00006dfd, // 0.429655
	0x00006f30, // 0.434333
	0x00007066, // 0.439063
	0x0000719f, // 0.443844
	0x000072dc, // 0.448677
	0x0000741c, // 0.453563
	0x00007560, // 0.458502
	0x000076a7, // 0.463495
	0x000077f2, // 0.468542
	0x00007940, // 0.473644
	0x00007a92, // 0.478802
	0x00007be8, // 0.484015
	0x00007d41, // 0.489286
	0x00007e9f, // 0.494614
	0x00008000, // 0.500000
	0x00008164, // 0.505445
	0x000082cd, // 0.510949
	0x0000843a, // 0.516512
	0x000085aa, // 0.522137
	0x0000871f, // 0.527823
	0x00008898, // 0.533570
	0x00008a14, // 0.539380
	0x00008b95, // 0.545254
	0x00008d1a, // 0.551191
	0x00008ea4, // 0.557193
	0x00009031, // 0.563261
	0x000091c3, // 0.569394
	0x0000935a, // 0.575595
	0x000094f4, // 0.581862
	0x00009694, // 0.588198
	0x00009837, // 0.594604
	0x000099e0, // 0.601078
	0x00009b8d, // 0.607624
	0x00009d3e, // 0.614240
	0x00009ef5, // 0.620929
	0x0000a0b0, // 0.627690
	0x0000a270, // 0.634525
	0x0000a435, // 0.641435
	0x0000a5fe, // 0.648420
	0x0000a7cd, // 0.655481
	0x0000a9a1, // 0.662618
	0x0000ab7a, // 0.669834
	0x0000ad58, // 0.677128
	0x0000af3b, // 0.684501
	0x0000b123, // 0.691955
	0x0000b311, // 0.699490
	0x0000b504, // 0.707107
	0x0000b6fd, // 0.714807
	0x0000b8fb, // 0.722590
	0x0000baff, // 0.730459
	0x0000bd08, // 0.738413
	0x0000bf17, // 0.746454
	0x0000c12c, // 0.754582
	0x0000c346, // 0.762799
	0x0000c567, // 0.771105
	0x0000c78d, // 0.779502
	0x0000c9b9, // 0.787990
	0x0000cbec, // 0.796571
	0x0000ce24, // 0.805245
	0x0000d063, // 0.814014
	0x0000d2a8, // 0.822878
	0x0000d4f3, // 0.831838
	0x0000d744, // 0.840896
	0x0000d99d, // 0.850053
	0x0000dbfb, // 0.859310
	0x0000de60, // 0.868667
	0x0000e0cc, // 0.878126
	0x0000e33f, // 0.887688
	0x0000e5b9, // 0.897355
	0x0000e839, // 0.907126
	0x0000eac0, // 0.917004
	0x0000ed4f, // 0.926990
	0x0000efe4, // 0.937084
	0x0000f281, // 0.947288
	0x0000f525, // 0.957603
	0x0000f7d0, // 0.968031
	0x0000fa83, // 0.978572
	0x0000fd3e, // 0.989228
	0x00010000, // 1.000000
	0x000102c9, // 1.010889
	0x0001059b, // 1.021897
	0x00010874, // 1.033025
	0x00010b55, // 1.044274
	0x00010e3e, // 1.055645
	0x00011130, // 1.067140
	0x00011429, // 1.078761
	0x0001172b, // 1.090508
	0x00011a35, // 1.102383
	0x00011d48, // 1.114387
	0x00012063, // 1.126522
	0x00012387, // 1.138789
	0x000126b4, // 1.151189
	0x000129e9, // 1.163725
	0x00012d28, // 1.176397
	0x0001306f, // 1.189207
	0x000133c0, // 1.202157
	0x0001371a, // 1.215247
	0x00013a7d, // 1.228481
	0x00013dea, // 1.241858
	0x00014160, // 1.255381
	0x000144e0, // 1.269051
	0x0001486a, // 1.282870
	0x00014bfd, // 1.296840
	0x00014f9b, // 1.310961
	0x00015342, // 1.325237
	0x000156f4, // 1.339668
	0x00015ab0, // 1.354256
	0x00015e76, // 1.369002
	0x00016247, // 1.383910
	0x00016623, // 1.398980
	0x00016a09, // 1.414214
	0x00016dfb, // 1.429613
	0x000171f7, // 1.445181
	0x000175fe, // 1.460918
	0x00017a11, // 1.476826
	0x00017e2f, // 1.492908
	0x00018258, // 1.509164
	0x0001868d, // 1.525598
	0x00018ace, // 1.542211
	0x00018f1a, // 1.559004
	0x00019373, // 1.575981
	0x000197d8, // 1.593142
	0x00019c49, // 1.610490
	0x0001a0c6, // 1.628027
	0x0001a550, // 1.645755
	0x0001a9e6, // 1.663677
	0x0001ae89, // 1.681793
	0x0001b33a, // 1.700106
	0x0001b7f7, // 1.718619
	0x0001bcc1, // 1.737334
	0x0001c199, // 1.756252
	0x0001c67f, // 1.775376
	0x0001cb72, // 1.794709
	0x0001d072, // 1.814252
	0x0001d581, // 1.834008
	0x0001da9e, // 1.853979
	0x0001dfc9, // 1.874168
	0x0001e502, // 1.894576
	0x0001ea4a, // 1.915207
	0x0001efa1, // 1.936062
	0x0001f507, // 1.957144
	0x0001fa7c, // 1.978456
	0x00020000, // 2.000000
	0x00020593, // 2.021779
	0x00020b36, // 2.043794
	0x000210e8, // 2.066050
	0x000216ab, // 2.088548
	0x00021c7d, // 2.111290
	0x00022260, // 2.134281
	0x00022853, // 2.157522
	0x00022e57, // 2.181015
	0x0002346b, // 2.204765
	0x00023a90, // 2.228773
	0x000240c7, // 2.253043
	0x0002470f, // 2.277577
	0x00024d68, // 2.302378
	0x000253d3, // 2.327450
	0x00025a50, // 2.352794
	0x000260df, // 2.378414
	0x00026781, // 2.404313
	0x00026e34, // 2.430495
	0x000274fb, // 2.456961
	0x00027bd4, // 2.483716
	0x000282c1, // 2.510762
	0x000289c1, // 2.538102
	0x000290d4, // 2.565740
	0x000297fb, // 2.593679
	0x00029f36, // 2.621922
	0x0002a685, // 2.650473
	0x0002ade8, // 2.679335
	0x0002b560, // 2.708511
	0x0002bced, // 2.738005
	0x0002c48f, // 2.767820
	0x0002cc47, // 2.797959
	0x0002d413, // 2.828427
	0x0002dbf6, // 2.859227
	0x0002e3ee, // 2.890362
	0x0002ebfd, // 2.921836
	0x0002f422, // 2.953652
	0x0002fc5e, // 2.985815
	0x000304b1, // 3.018329
	0x00030d1b, // 3.051196
	0x0003159c, // 3.084422
	0x00031e35, // 3.118009
	0x000326e6, // 3.151962
	0x00032fb0, // 3.186284
	0x00033892, // 3.220981
	0x0003418c, // 3.256055
	0x00034aa0, // 3.291511
	0x000353cd, // 3.327353
	0x00035d13, // 3.363586
	0x00036674, // 3.400213
	0x00036fee, // 3.437239
	0x00037983, // 3.474668
	0x00038333, // 3.512504
	0x00038cfe, // 3.550753
	0x000396e4, // 3.589418
	0x0003a0e5, // 3.628504
	0x0003ab03, // 3.668016
	0x0003b53c, // 3.707958
	0x0003bf92, // 3.748335
	0x0003ca05, // 3.789152
	0x0003d495, // 3.830413
	0x0003df43, // 3.872124
	0x0003ea0e, // 3.914288
	0x0003f4f8, // 3.956912
	0x00040000, // 4.000000
};

unsigned int GetPitchBendScale(int pitch)
{
	pitch+=8192;
	pitch>>=6;
	pitch=_max(0,pitch);
	pitch=_min(256,pitch);

	_Far unsigned int *table;
	_FP_SEG(table)=SEG_TGBIOS_CODE;
	_FP_OFF(table)=(unsigned int)pitchBendTable;
	return table[pitch];
}
