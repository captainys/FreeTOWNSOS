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

static unsigned int GetPitchBendScale(unsigned int pitch);
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

unsigned short YM2612_ApplyPitchBend(unsigned short BLK_FNUM,short pitchBend)
{
	unsigned short BLK=(BLK_FNUM>>11)&7;
	unsigned short FNUM=(BLK_FNUM&0x7FF);
	int s=(BLK*616)+(FNUM-616);

	// F-NUMBER increases or decreases with every 13 Pitchbend values.
	// pitch limits(616*13=8008).
	s+=(_max(_min(pitchBend,8008),-8008)/13);
	s=_max(_min(s,4924),0); // 4924 = (BLK:7*616) + F-NUM:612(615-3)

	BLK=s/616;
	FNUM=s%616;
	if(BLK>=1)
	{
		// if BLK is more than 1, F-NUMBER will have 619-1235.
		FNUM+=3;
	}
	return (BLK<<11)|(FNUM+616);
}

unsigned int RF5C68_ApplyPitchBend(unsigned int Freq,short pitchBend)
{
	int Num=0;
	pitchBend=_max(_min(pitchBend,8191),-8191); // pitch limits
	if(pitchBend>=1)
	{
		Num=(Freq*pitchBend)/8191;
	}
	else if(pitchBend<=-1)
	{
		Num=(Freq*pitchBend)/16382;
	}
	return Freq+Num;
}

unsigned short RF5C68_CalculateVolume(unsigned short vol1,unsigned short vol2)
{
	// vol1, vol2  0 to 127 scale
	// Output      0 to 255 scale
	vol1+=(vol1>>6); // 0 to 128 scale (Value will be 0 to 63, 65 to 128.  Will never be 64.)
	vol2+=(vol2>>6); // 0 to 128 scale (Value will be 0 to 63, 65 to 128.  Will never be 64.)
	vol1*=vol2; // 0 to 16384
	return _min(255,(vol1>>6));
}

unsigned int RF5C68_CalculatePlaybackFrequency(
		unsigned int baseFreq,
		unsigned int note,
		unsigned int baseNote)
{
	// This base Freq is KHz*0x62.  Why 0x62?  Why not 0x64 (100)?
	// I don't know, but this number seems to be correct.
	baseFreq*=1000;
	baseFreq/=0x62;
	// Now baseFreq is in Hz.

	// If I play it back at baseFreq, I'll get note of baseNote.
	unsigned int freqScale=GetFreqScale(note,baseNote);

	return MUL_SHR(baseFreq,freqScale,20);
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
		status->FMCh[i].vol=127; // FM TOWNS Technical Databook p.415 tells the default volume is 127.
		status->FMCh[i].vol_key=0;
		status->FMCh[i].pan=0xc0;
	}


	// PCM Channels
	_outb(TOWNSIO_SOUND_PCM_CH_ON_OFF,0xFF);
	status->voiceModeINTMask=0;
	status->numVoiceModeChannels=0;
	for(i=0; i<SND_NUM_PCM_CHANNELS; ++i)
	{
		status->voiceChannelBank[i]=0;
		status->PCMCh[i].playing=0;
		status->PCMCh[i].instrument=0;
		status->PCMCh[i].vol=127; // FM TOWNS Technical Databook p.415 tells the default volume is 127.
		status->PCMCh[i].vol_key=0;
		status->PCMCh[i].pan=0x77;
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

	// Enable Audio Output???
	// Bit7 Level-Meter LED On/Off   1->ON
	// Bit6 Enable Audio Output      1->Enable
	// The original TBIOS Sound BIOS writes 7FH on initialization.
	_outb(TOWNSIO_SOUND_MASTERSWITCH,0xFF);

	SND_SetError(EAX,SND_NO_ERROR);
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
		stat->FMCh[ch].vol_key=vol;
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
				BLK_FNUM=YM2612_ApplyPitchBend(BLK_FNUM,stat->FMCh[ch].pitchBend);
				YM2612_Write(regSet,0xA4+chMOD3,BLK_FNUM>>8);
				YM2612_Write(regSet,0xA0+chMOD3,BLK_FNUM);
			}

			// Channel number to YM2612 Reg 28H is 0,1,2,4,5, or 6.
			YM2612_Write(0,0x28,0x00|(ch+regSet));
			YM2612_Write(0,0x28,0xF0|(ch+regSet));
		}
	}
	else if(SND_Is_PCM_Channel(ch))
	{
		ch-=SND_PCM_CHANNEL_START;
		stat->PCMCh[ch].vol_key=vol;
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

			// FM TOWNS Technical Databook p.405.  To disable envelope, AR=0, DR=SR=RR=127.
			if(0==env.AR &&
			   127==env.DR &&
			   127==env.SR &&
			   127==env.RR)
			{
				env.enabled=0;
			}
			else
			{
				env.enabled=1;
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
				_CLI

				unsigned short curVol,playVol;

				playVol=RF5C68_CalculateVolume(vol,stat->PCMCh[ch].vol);

				stat->PCMCh[ch].playing=1;
				stat->PCMCh[ch].env=env;
				stat->PCMCh[ch].playVol=playVol;
				if(0==env.enabled)
				{
					stat->PCMCh[ch].envL=127;
				}
				else if(0==env.AR)
				{
					stat->PCMCh[ch].phase=1;
					stat->PCMCh[ch].envL=env.TL;
					stat->PCMCh[ch].phaseStepLeft=env.DR;
					stat->PCMCh[ch].dx=env.DR;
					stat->PCMCh[ch].dy=(env.SL<env.TL ? env.TL-env.SL : 0);
				}
				else
				{
					stat->PCMCh[ch].phase=0;
					stat->PCMCh[ch].envL=0;
					stat->PCMCh[ch].phaseStepLeft=env.AR;
					stat->PCMCh[ch].dx=env.AR;
					stat->PCMCh[ch].dy=env.TL;
				}
				curVol=playVol*(stat->PCMCh[ch].envL)/127;
				stat->PCMCh[ch].balance=0;

				// What to do with the frequency?
				unsigned int baseFreq=sound->snd.sampleFreq+sound->snd.sampleFreqCorrection;

				unsigned int playFreq=RF5C68_CalculatePlaybackFrequency(
					baseFreq,
					note,
					sound->snd.baseNote-env.rootKey);

				stat->PCMCh[ch].playFreq=playFreq;
				if(0!=stat->PCMCh[ch].pitchBend)
				{
					playFreq=RF5C68_ApplyPitchBend(playFreq,stat->PCMCh[ch].pitchBend);
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

				_outb(TOWNSIO_SOUND_PCM_ENV,curVol);
				_outb(TOWNSIO_SOUND_PCM_PAN,stat->PCMCh[ch].pan);

				unsigned char keyFlag=(1<<ch);
				stat->PCMKey|=keyFlag;
				_outb(TOWNSIO_SOUND_PCM_CH_ON_OFF,stat->PCMKey);
				stat->PCMKey&=~keyFlag;
				_outb(TOWNSIO_SOUND_PCM_CH_ON_OFF,stat->PCMKey);
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
		unsigned int regSet=ch/3;
		YM2612_Write(0,0x28,0x00|(ch+regSet));
	}
	else if(SND_Is_PCM_Channel(ch))
	{
		ch-=SND_PCM_CHANNEL_START;
		if(stat->PCMCh[ch].playing)
		{
			_CLI
			if(0==stat->PCMCh[ch].env.RR || 0==stat->PCMCh[ch].env.enabled)
			{
				stat->PCMCh[ch].playing=0;
				stat->PCMKey|=(1<<ch);
				_outb(TOWNSIO_SOUND_PCM_CH_ON_OFF,stat->PCMKey);
			}
			else
			{
				// Looks like RR=127 means indefinite play back.
				if(127==stat->PCMCh[ch].env.RR)
				{
					stat->PCMCh[ch].phase=4;
				}
				else
				{
					stat->PCMCh[ch].phase=3;
					stat->PCMCh[ch].dx=stat->PCMCh[ch].env.RR;
					stat->PCMCh[ch].dy=stat->PCMCh[ch].envL;
					stat->PCMCh[ch].balance=0;
				}
			}
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
		if(pan_set==64) // Both
		{
			status->FMCh[ch].pan=0xc0;
		}
		else if(pan_set>=65) // Right only
		{
			status->FMCh[ch].pan=0x40;
		}
		else // Left only
		{
			status->FMCh[ch].pan=0x80;
		}

		SND_SetError(EAX,SND_NO_ERROR);
	}
	else if(SND_Is_PCM_Channel(ch))
	{
		// probably the center pan is not 0xff(11111111) but 0x77(01110111).
		// At 0xff, the maximum volume will flow from both ch, making it twice as loud.
		// center = 0x77, left only = 0x0f, right only = 0xf0.
		ch-=SND_PCM_CHANNEL_START;
		pan_set&=0x7f;
		if(pan_set==64)
		{
			status->PCMCh[ch].pan=0x77;
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
			BLK_FNUM=YM2612_ApplyPitchBend(BLK_FNUM,pitchBend);
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
			playFreq=RF5C68_ApplyPitchBend(playFreq,stat->PCMCh[ch].pitchBend);
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

		// As far as i have confirmed with the test code, it appears that when SND_VOLUME_CHANGE call,
		// the volume setting is immediately reflected according to the instrument setting,
		// but i have disabled it at this time.
		/*unsigned int instIndex=status->FMCh[ch].instrument;
		if(instIndex<FM_NUM_INSTRUMENTS)
		{
			int i;
			unsigned int regSet=ch/3;
			unsigned int chMOD3=ch%3;
			_Far struct FMB_INSTRUMENT *inst=&status->FMInst[instIndex];
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
					MUL=(0x60+(status->FMCh[ch].vol_key>>2));
					TL*=MUL;
					TL>>=7;
					TL++;
					MUL=vol;
					TL*=MUL;
					TL>>=7;
					TL++;
					TL=127-TL;
					YM2612_Write(regSet,0x40+chMOD3+slot*4,TL);
				}
				carrierSlots>>=8;
			}
		}*/

		SND_SetError(EAX,SND_NO_ERROR);
	}
	else if(SND_Is_PCM_Channel(ch))
	{
		ch-=SND_PCM_CHANNEL_START;
		status->PCMCh[ch].vol=vol;

		unsigned short playVol=RF5C68_CalculateVolume(vol,status->PCMCh[ch].vol_key);
		status->PCMCh[ch].playVol=playVol;
		if(status->PCMCh[ch].playing)
		{
			unsigned short vol;
			vol=playVol*status->PCMCh[ch].envL/127;
			_outb(TOWNSIO_SOUND_PCM_CTRL,0xC0|ch); // Select PCM Channel
			_outb(TOWNSIO_TIMER_1US_WAIT,0);
			_outb(TOWNSIO_SOUND_PCM_ENV,vol);
		}

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
		int i;
		unsigned int regSet=ch/3;
		unsigned int chMOD3=ch%3;
		_Far struct FMB_INSTRUMENT *inst=&status->FMInst[status->FMCh[ch].instrument];
		unsigned int carrierSlots=YM2612_AlgorithmToCarrierSlot(inst->FB_CNCT&0x07);
		for(i=0; i<4; ++i)
		{
			unsigned int slot=(unsigned char)carrierSlots;
			slot=YM2612_SlotTwist(slot);
			if(0xFF!=slot)
			{
				YM2612_Write(regSet,0x40+chMOD3+slot*4,127);
			}
			carrierSlots>>=8;
		}
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

	if(val>=9)
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

	if(ECX>=0x10000||EBX>=0x10000)
	{
		SND_SetError(EAX,SND_ERROR_PARAMETER);
	}
	else if((ECX+EBX)>=0x10000)
	{
		while(EBX<0x10000)
		{
			SND_WriteToWaveRAM((EBX&0xffff),_min(*mainram,PCM_LOOP_STOP_CODE-1));
			EBX++;
			mainram++;
		}

		SND_SetError(EAX,SND_ERROR_OUT_OF_PCM_RAM);
	}
	else
	{
		for(int i=0;i<ECX;i++)
		{
			SND_WriteToWaveRAM((EBX&0xffff),_min(*mainram,PCM_LOOP_STOP_CODE-1));
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
		SND_WriteToWaveRAM(addr++,_min(*wave,PCM_LOOP_STOP_CODE-1));
		wave++;
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
	// Memo: Make sure to CLI while updating channel info and PCM registers.
	_Far struct SND_Status *stat=SND_GetStatus();

	unsigned char ch=EBX;
	SND_SetError(EAX,SND_NO_ERROR);

	if(0==SND_Is_PCM_Channel(ch))
	{
		SND_SetError(EAX,SND_ERROR_WRONG_CH);
		return;
	}
	ch-=SND_PCM_CHANNEL_START;
	unsigned char keyFlag=(1<<ch);

	if(ch<SND_NUM_PCM_CHANNELS-stat->numVoiceModeChannels)
	{
		SND_SetError(EAX,SND_ERROR_PARAMETER);
		return;
	}

	_CLI

	stat->PCMKey|=keyFlag;
	_outb(TOWNSIO_SOUND_PCM_CH_ON_OFF,stat->PCMKey);
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
	int i;
	unsigned char ch=(unsigned char)EBX;
	_Far struct SND_Status *stat=SND_GetStatus();

	for(i=0; i<SND_NUM_PCM_CHANNELS; ++i)
	{
		stat->PCMCh[i].playing=0;
	}

	stat->PCMKey=0xFF;
	_outb(TOWNSIO_SOUND_PCM_CH_ON_OFF,0xFF);

	SND_SetError(EAX,SND_NO_ERROR);
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
	_Far unsigned char *mainram;
	_FP_SEG(mainram)=DS;
	_FP_OFF(mainram)=ESI;

	if(ECX>=0x10000||EBX>=0x10000)
	{
		SND_SetError(EAX,SND_ERROR_PARAMETER);
	}
	else if((ECX+EBX)>=0x10000)
	{
		while(EBX<0x10000)
		{
			*mainram=SND_ReadFromWaveRAM(EBX);
			EBX++;
			mainram++;
		}

		SND_SetError(EAX,SND_ERROR_OUT_OF_PCM_RAM);
	}
	else
	{
		for(int i=0;i<ECX;i++)
		{
			*mainram=SND_ReadFromWaveRAM(EBX);
			EBX++;
			mainram++;
		}

		SND_SetError(EAX,SND_NO_ERROR);
	}
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

	if(ECX>=0x10000||EBX>=0x10000)
	{
		SND_SetError(EAX,SND_ERROR_PARAMETER);
	}
	else if((ECX+EBX)>=0x10000)
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
	unsigned short MUL;
	_Far struct PCM_Voice_Header *sndData;
	_Far struct SND_Status *info=SND_GetStatus();

	unsigned char keyFlag;

	if(ch<64 || 71<ch)
	{
		SND_SetError(EAX,SND_ERROR_WRONG_CH);
		return;
	}
	ch-=SND_PCM_CHANNEL_START;
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

	_CLI
	{
		info->PCMCh[ch].header=sndData;
		info->PCMCh[ch].playPtr=((_Far unsigned char *)sndData)+sizeof(struct PCM_Voice_Header);
		info->PCMCh[ch].nextFillBank=info->voiceChannelBank[ch];
		info->PCMCh[ch].playing=1;
		info->PCMCh[ch].vol_key=volume;
		info->PCMCh[ch].envL=volume; // Will be used when volume_change is called.
		info->PCMCh[ch].playVol=RF5C68_CalculateVolume(volume,info->PCMCh[ch].vol);
		info->PCMCh[ch].curPos=0;

		{
			unsigned int realTotalBytes=sndData->totalBytes;
			unsigned int bank=info->voiceChannelBank[ch];
			unsigned int transferSize=_min(PCM_BANK_SIZE*2-256,sndData->totalBytes);
			unsigned int trailingLoopStopSize=32;
			_Far unsigned char *waveRAM;
			_FP_SEG(waveRAM)=SEG_WAVE_RAM;
			_FP_OFF(waveRAM)=0;

			while(0<transferSize+trailingLoopStopSize)
			{
				int i;
				unsigned int oneTransferSize=_min(PCM_BANK_SIZE,transferSize);
				unsigned int curPos=info->PCMCh[ch].curPos;
				_outb(TOWNSIO_SOUND_PCM_CTRL,0x80|bank);
				waveRAM[0xFFE]=PCM_LOOP_STOP_CODE;
				waveRAM[0xFFF]=PCM_LOOP_STOP_CODE;
				for(i=0;i<oneTransferSize;i++)
				{
					waveRAM[i]=_min(info->PCMCh[ch].playPtr[curPos+i],PCM_LOOP_STOP_CODE-1);
				}
				for(i=i; i<PCM_BANK_SIZE && 0<trailingLoopStopSize; ++i)
				{
					waveRAM[i]=PCM_LOOP_STOP_CODE;
					--trailingLoopStopSize;
				}
				transferSize-=oneTransferSize;
				info->PCMCh[ch].curPos+=oneTransferSize;
				++bank;
			}
		}

		{
			// What to do with the frequency?
			unsigned int baseFreq=sndData->sampleFreq+sndData->sampleFreqCorrection;

			unsigned int playFreq=RF5C68_CalculatePlaybackFrequency(
				baseFreq,
				note,
				sndData->baseNote);

			info->PCMCh[ch].playFreq=playFreq;
			if(0!=info->PCMCh[ch].pitchBend)
			{
				playFreq=RF5C68_ApplyPitchBend(playFreq,info->PCMCh[ch].pitchBend);
			}

			// PCM Frequency is 20725Hz according to the analysis done during Tsugaru development.
			// I am still not sure where this 20725Hz is coming from.
			// PCM Stride 1X is (1<<11)=0x0800

			// But, there is a possibility that from VM point of view, it may be 20000Hz.

			unsigned int stride=MULDIV(0x800,playFreq,PCM_NATIVE_FREQUENCY);

			unsigned short ST=info->voiceChannelBank[ch];

			// 

			// ST is the high-byte of the starting address, therefore, ST<<=12 to make it full address,
			// and then ST>>=8 to take high-byte.  Overall, ST<<=4;
			ST<<=4;

			_outb(TOWNSIO_SOUND_PCM_CTRL,0xC0|ch); // Select PCM Channel
			_outb(TOWNSIO_SOUND_PCM_ENV,info->PCMCh[ch].playVol);  // 0-255?
			_outb(TOWNSIO_SOUND_PCM_PAN,info->PCMCh[ch].pan);  // Pan setting.

			_outb(TOWNSIO_SOUND_PCM_FDH,stride>>8);
			_outb(TOWNSIO_SOUND_PCM_FDL,(unsigned char)stride);

			_outb(TOWNSIO_SOUND_PCM_ST,ST); // Starting address high-byte

			if(sndData->totalBytes<=PCM_BANK_SIZE)
			{
				unsigned short endAddr=ST;
				endAddr<<=8;
				endAddr+=0xFFF;
				_outb(TOWNSIO_SOUND_PCM_LSH,endAddr>>8); // Loop start address high-byte
				_outb(TOWNSIO_SOUND_PCM_LSL,endAddr); // Loop start address low-byte
			}
			else if(sndData->totalBytes<=PCM_BANK_SIZE*2-256)
			{
				unsigned short endAddr=ST;
				endAddr<<=8;
				endAddr+=0x1FFF;
				_outb(TOWNSIO_SOUND_PCM_LSH,endAddr>>8); // Loop start address high-byte
				_outb(TOWNSIO_SOUND_PCM_LSL,endAddr); // Loop start address low-byte
			}
			else
			{
				_outb(TOWNSIO_SOUND_PCM_LSH,ST); // Loop start address high-byte
				_outb(TOWNSIO_SOUND_PCM_LSL,0); // Loop start address low-byte
			}

			_outb(TOWNSIO_SOUND_PCM_INT_MASK,info->voiceModeINTMask);

			info->PCMKey&=~keyFlag;
			_outb(TOWNSIO_SOUND_PCM_CH_ON_OFF,info->PCMKey);
		}
	}

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
	_outb(TOWNSIO_TIMER_1US_WAIT,0);
	if(0==port)
	{
		pad=_inb(TOWNSIO_GAMEPORT_A_INPUT);
	}
	else
	{
		pad=_inb(TOWNSIO_GAMEPORT_B_INPUT);
	}

	pad|=0xC0;
	// if(0==(pad&(PAD_LEFT|PAD_RIGHT)) && (PAD_UP|PAD_DOWN)==(pad&(PAD_UP|PAD_DOWN)))
	// The second condition is based on the observation of the original TBIOS.
	if((PAD_UP|PAD_DOWN)==(pad&0x0F)) //The above condition is same as this.
	{
		pad&=(~PAD_RUN);
	}
	// if(0==(pad&(PAD_UP|PAD_DOWN)) && (PAD_LEFT|PAD_RIGHT)==(pad&(PAD_LEFT|PAD_RIGHT)))
	// The second condition is based on the observation of the original TBIOS.
	if((PAD_LEFT|PAD_RIGHT)==(pad&0x0F)) // The above condition is same as this.
	{ 
		pad&=(~PAD_SELECT);
	}
	// Looks like if both UP&DOWN (or LEFT&RIGHT) are both pressed, if LEFT&RIGHT (or UP&DOWN) are both released,
	// it won't take Run/Select button.

	// The following must be done independently.
	// If I/O read is 030H (possible from mouse), the return should be FFh.
	if(0==((PAD_UP|PAD_DOWN)&pad))
	{
		pad|=(PAD_UP|PAD_DOWN);
	}
	if(0==((PAD_LEFT|PAD_RIGHT)&pad))
	{
		pad|=(PAD_LEFT|PAD_RIGHT);
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
		if(stat->PCMCh[ch].playing && stat->PCMCh[ch].env.enabled)
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
					stat->PCMCh[ch].envL=_min(254,stat->PCMCh[ch].envL)+1;
					stat->PCMCh[ch].balance-=stat->PCMCh[ch].dx;
				}
				--stat->PCMCh[ch].phaseStepLeft;
				break;
			case 1: // Decay
				// Based on the observation by BCC.
				// If SR==127, the volume does not change.
				// VSGP first key-on PCM Ch 3 and use change volume to play or stop the sound effect.
				// Without this condition, envL will become zero when change volume is called, and nothing is audible.
				if(127==stat->PCMCh[ch].env.SR)
				{
					break;
				}

				if(0==stat->PCMCh[ch].phaseStepLeft)
				{
					++stat->PCMCh[ch].phase;
					stat->PCMCh[ch].dx=stat->PCMCh[ch].env.SR;
					stat->PCMCh[ch].dy=stat->PCMCh[ch].envL;
					stat->PCMCh[ch].balance=0;
					break;
				}
				stat->PCMCh[ch].balance+=stat->PCMCh[ch].dy;
				while(0<stat->PCMCh[ch].balance && 0<stat->PCMCh[ch].envL)
				{
					stat->PCMCh[ch].envL=_max(1,stat->PCMCh[ch].envL)-1;
					stat->PCMCh[ch].balance-=stat->PCMCh[ch].dx;
				}
				--stat->PCMCh[ch].phaseStepLeft;
				break;
			case 2: // Sustain
			case 3: // Release
				stat->PCMCh[ch].balance+=stat->PCMCh[ch].dy;
				while(0<stat->PCMCh[ch].balance && 0<stat->PCMCh[ch].envL)
				{
					stat->PCMCh[ch].envL=_max(1,stat->PCMCh[ch].envL)-1;
					stat->PCMCh[ch].balance-=stat->PCMCh[ch].dx;
				}
				break;

			case 4: // Indefinite play back.
				continue;
			}
			//if(0==stat->PCMCh[ch].envL)
			//{
			//	stat->PCMCh[ch].playing=0;
			//	stat->PCMKey|=(1<<ch);
			//	_outb(TOWNSIO_SOUND_PCM_CH_ON_OFF,stat->PCMKey);
			//}
			//else
			//{
				unsigned short vol=stat->PCMCh[ch].playVol*stat->PCMCh[ch].envL/127;
				_outb(TOWNSIO_SOUND_PCM_CTRL,0xC0|ch); // Select PCM Channel
				_outb(TOWNSIO_SOUND_PCM_ENV,vol);
			//}
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
				unsigned int transferSize,transferSizeLimit=PCM_BANK_SIZE;
				unsigned int currentPosition=stat->PCMCh[ch].curPos;
				_Far unsigned char *waveRAM;
				_FP_SEG(waveRAM)=SEG_WAVE_RAM;
				_FP_OFF(waveRAM)=0;

				if(stat->PCMCh[ch].nextFillBank&1)
				{
					transferSizeLimit-=256;
				}
				transferSize=_min(transferSizeLimit,bytesLeft);

				_outb(TOWNSIO_SOUND_PCM_CTRL,0x80|stat->PCMCh[ch].nextFillBank);
				for(i=0;i<transferSize;i++)
				{
					waveRAM[i]=_min(stat->PCMCh[ch].playPtr[i+currentPosition],PCM_LOOP_STOP_CODE-1);
				}
				if(transferSize<PCM_BANK_SIZE)
				{
					int fillEnd=_min(transferSize+256,PCM_BANK_SIZE);
					for(i=i; i<fillEnd; ++i)
					{
						waveRAM[i]=PCM_LOOP_STOP_CODE;
					}
				}
				if(transferSize<transferSizeLimit)
				{
					waveRAM[0xFFE]=PCM_LOOP_STOP_CODE;
					waveRAM[0xFFF]=PCM_LOOP_STOP_CODE;

					_outb(TOWNSIO_SOUND_PCM_CTRL,0xC0|ch); // Select PCM Channel

					unsigned short endAddr=stat->PCMCh[ch].nextFillBank;
					endAddr<<=12;
					endAddr+=0xFFF;
					_outb(TOWNSIO_SOUND_PCM_LSH,endAddr>>8); // Loop start address high-byte
					_outb(TOWNSIO_SOUND_PCM_LSL,endAddr); // Loop start address low-byte
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

unsigned int GetPitchBendScale(unsigned int pitch)
{
	if(0==pitch)
	{
		return 0x10000;
	}
	if(8008<pitch)  // Based on the experiment in TOWNS OS V2.1 L51.  Where is this 8008 come from?
	{
		return (0x10000*1000)/1404;  // 1/1.404
	}
	// Real change seems to be two linear functions.  Slope changes at around DX=5925. Should be close enough if I make it 4096.
	if(pitch<4096)
	{
		// At pitch=1 scale is 1/1.404.  At pitch=4096 scale is 1.0.
		// y0=(0x10000*1000)/1404
		// y1=0x10000
		// y1-y0=18857
		return 0x10000-MUL_SHR(4096-pitch,18857,12);
	}
	else // if(4096<=pitch)
	{
		// At pitch=4096 scale is 1.0.  At pitch=8192 scale is 1.404.
		// y0=0x10000
		// y1=0x10000*1404/1000=92013
		// y1-y0=26477
		return 0x10000+MUL_SHR(pitch-4096,26477,12);
	}
}
