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



void YM2612_Write(unsigned char regSet,unsigned char reg,unsigned char value)
{
	while(0!=(_inb(TOWNSIO_SOUND_STATUS_ADDRESS0)&0x80)); // Wait BUSY clear

	if(0==regSet || reg<0x30)
	{
		_outb(TOWNSIO_SOUND_STATUS_ADDRESS0,reg);
		_outb(TOWNSIO_SOUND_DATA0,value);
	}
	else
	{
		_outb(TOWNSIO_SOUND_ADDRESS1,reg);
		_outb(TOWNSIO_SOUND_DATA1,value);
	}
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


	// PCM Voice Mode Allocation
	status->voiceModeINTMask=0;
	status->numVoiceModeChannels=0;
	for(i=0; i<SND_NUM_PCM_CHANNELS; ++i)
	{
		status->voiceChannelBank[i]=0;
		status->pcmPlayInfo[i].playing=0;
	}
	status->PCMKey=0xFF;


	// PCM Instrument Allocation
	status->voiceModeStartAddr=PCM_WAVE_RAM_SIZE;
	// status->instSoundLastAddr=0; already taken care by MEMSETB_FAR
	// status->numSound=0;          already taken care by MEMSETB_FAR


	// Stop YM2612 Timers
	YM2612_Write(0,0x27,0x30);

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
	_Far struct SND_Work *work;
	_FP_SEG(work)=GS;
	_FP_OFF(work)=EDI;

	SND_SetError(EAX,SND_NO_ERROR);
		TSUGARU_BREAK;
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
	_Far struct SND_Work *work;
	_FP_SEG(work)=GS;
	_FP_OFF(work)=EDI;

	SND_SetError(EAX,SND_NO_ERROR);
		TSUGARU_BREAK;
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
	_Far struct SND_Work *work;
	_FP_SEG(work)=GS;
	_FP_OFF(work)=EDI;

	SND_SetError(EAX,SND_NO_ERROR);
		TSUGARU_BREAK;
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
	_Far struct SND_Work *work;
	_FP_SEG(work)=GS;
	_FP_OFF(work)=EDI;

	SND_SetError(EAX,SND_NO_ERROR);
		TSUGARU_BREAK;
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
	unsigned char instIndex=(unsigned char)EDX;
	_Far struct SND_Status *stat=SND_GetStatus();

	if(ch<6)
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
	else if(64<=ch && ch<72)
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
		MEMCPY_FAR(&stat->FMInst[instIndex],src,sizeof(struct PMB_INSTRUMENT));
		SND_SetError(EAX,SND_NO_ERROR);
	}
	SND_SetError(EAX,SND_ERROR_WRONG_CH);
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
	_Far struct SND_Work *work;
	_FP_SEG(work)=GS;
	_FP_OFF(work)=EDI;

	SND_SetError(EAX,SND_NO_ERROR);
		TSUGARU_BREAK;
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
		SND_SetError(EAX,SND_NO_ERROR);
		TSUGARU_STATE;
	}
	else if(SND_Is_PCM_Channel(ch))
	{
		ch-=SND_PCM_CHANNEL_START;
		SND_SetError(EAX,SND_NO_ERROR);
		TSUGARU_STATE;
	}
	else
	{
		SND_SetError(EAX,SND_ERROR_WRONG_CH);
		TSUGARU_BREAK;
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
	_Far struct SND_Work *work;
	_FP_SEG(work)=GS;
	_FP_OFF(work)=EDI;

	SND_SetError(EAX,SND_NO_ERROR);
		TSUGARU_BREAK;
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
		reg27H&=0xCA; // Preserve MODE, TimerB enable, TimerB Load
		YM2612_Write(0,0x27,0x10|reg27H); // MODEMODE|ResetA|ResetB|PermitFlagA|PermitFlagB|LoadB|LoadA
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
		reg27H&=0xC5; // Preserve MODE, TimerA enable, TimerA Load
		YM2612_Write(0,0x27,0x20|reg27H); // MODEMODE|ResetA|ResetB|PermitFlagA|PermitFlagB|LoadB|LoadA
		info->YM2612_REG27H=reg27H;
	}
	else
	{
		YM2612_Write(0,0x25,count&3);
		YM2612_Write(0,0x24,count>>2);
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
	reg27H&=0xCA; // Preseve Timer B flag permission, and CH3 MODE
	YM2612_Write(0,0x27,0x15|reg27H); // MODEMODE|ResetA|ResetB|PermitFlagA|PermitFlagB|LoadB|LoadA
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
		TSUGARU_STATE;
}

void SND_FM_Timer_B_Restart(void)
{
	_Far struct SND_Status *info=SND_GetStatus();
	unsigned char reg27H=info->YM2612_REG27H;
	reg27H&=0xC5; // Preseve Timer B flag permission, and CH3 MODE
	YM2612_Write(0,0x27,0x2A|reg27H); // MODEMODE|ResetA|ResetB|PermitFlagA|PermitFlagB|LoadB|LoadA
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
	_Far struct SND_Work *work;
	_FP_SEG(work)=GS;
	_FP_OFF(work)=EDI;

	SND_SetError(EAX,SND_NO_ERROR);
		TSUGARU_BREAK;
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

		SND_SetError(EAX,SND_ERROR_OUT_OF_PCM_RAM2);
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
		SND_SetError(EAX,SND_ERROR_OUT_OF_PCM_RAM2);
		return;
	}

	required=(soundData->totalBytes+32+255)&0xFFFFFF00;
	if(status->voiceModeStartAddr<status->instSoundLastAddr+required)
	{
		SND_SetError(EAX,SND_ERROR_OUT_OF_PCM_RAM2);
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

	addr=status->instSoundLastAddr;
	for(i=0; i<soundData->totalBytes; ++i)
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

	SET_LOW_BYTE(&EDX,sndStat->pcmPlayInfo[ch].playing);
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

		SND_SetError(EAX,SND_ERROR_OUT_OF_PCM_RAM2);
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

	info->pcmPlayInfo[ch].header=sndData;
	info->pcmPlayInfo[ch].playPtr=((_Far unsigned char *)sndData)+sizeof(struct PCM_Voice_Header);
	info->pcmPlayInfo[ch].nextFillBank=info->voiceChannelBank[ch];
	info->pcmPlayInfo[ch].playing=1;
	info->pcmPlayInfo[ch].curPos=0;


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
			MOVSB_FAR(waveRAM,info->pcmPlayInfo[ch].playPtr,oneTransferSize);
			if(oneTransferSize<PCM_BANK_SIZE)
			{
				waveRAM[oneTransferSize]=PCM_LOOP_STOP_CODE;
			}
			transferSize-=oneTransferSize;
			info->pcmPlayInfo[ch].curPos+=oneTransferSize;
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
		_outb(TOWNSIO_SOUND_PCM_PAN,0xFF);  // I'll be worried about it later.

		_outb(TOWNSIO_SOUND_PCM_FDL,FD);
		_outb(TOWNSIO_SOUND_PCM_FDH,FD>>8);

		_outb(TOWNSIO_SOUND_PCM_ST,ST); // Starting address high-byte
		_outb(TOWNSIO_SOUND_PCM_LSH,ST); // Loop start address high-byte
		_outb(TOWNSIO_SOUND_PCM_LSL,0); // Loop start address low-byte

		_outb(TOWNSIO_SOUND_PCM_INT_MASK,info->voiceModeINTMask);

		info->PCMKey&=~keyFlag;
		_outb(TOWNSIO_SOUND_PCM_CH_ON_OFF,info->PCMKey);
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
	_Far struct SND_Work *work;
	_FP_SEG(work)=GS;
	_FP_OFF(work)=EDI;

	SND_SetError(EAX,SND_NO_ERROR);
		TSUGARU_BREAK;
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
			if(stat->pcmPlayInfo[ch].header->totalBytes<=stat->pcmPlayInfo[ch].curPos)
			{
				// Not respecting loop for the time being.
				stat->PCMKey|=CHFlag;
				_outb(TOWNSIO_SOUND_PCM_CH_ON_OFF,stat->PCMKey); // Key Off
				stat->pcmPlayInfo[ch].playing=0;
			}
			else
			{
				unsigned int bytesLeft=stat->pcmPlayInfo[ch].header->totalBytes-stat->pcmPlayInfo[ch].curPos;
				unsigned int transferSize=PCM_BANK_SIZE;
				_Far unsigned char *waveRAM;
				_FP_SEG(waveRAM)=SEG_WAVE_RAM;
				_FP_OFF(waveRAM)=0;

				if(stat->pcmPlayInfo[ch].nextFillBank&1)
				{
					transferSize-=256;
				}
				transferSize=_min(transferSize,bytesLeft);

				_outb(TOWNSIO_SOUND_PCM_CTRL,0x80|stat->pcmPlayInfo[ch].nextFillBank);
				MOVSB_FAR(waveRAM,stat->pcmPlayInfo[ch].playPtr+stat->pcmPlayInfo[ch].curPos,transferSize);
				if(transferSize<PCM_BANK_SIZE)
				{
					waveRAM[transferSize]=PCM_LOOP_STOP_CODE;
				}

				stat->pcmPlayInfo[ch].curPos+=transferSize;
				stat->pcmPlayInfo[ch].nextFillBank^=1;
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
	{{NULL,NULL,0,0}},
	{{0}},
};

_Far struct SND_Status *SND_GetStatus(void)
{
	_Far struct SND_Status *ptr;
	_FP_SEG(ptr)=SEG_TGBIOS_DATA;
	_FP_OFF(ptr)=(unsigned int)&status;
	return ptr;
}
