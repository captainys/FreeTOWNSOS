#include <DOS.H>
#include <CONIO.H>
#include "TGBIOS.H"
#include "SND.H"
#include "MACRO.H"
#include "IODEF.H"
#include "UTIL.H"
#include "SYSINFO.H"

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
	_Far struct TBIOS_System_Info *sysInfo=SYSINFO_GetStruct();
	_Far unsigned int *SNDWorkStore;
	_Far struct SND_Work *work;
	_FP_SEG(work)=GS;
	_FP_OFF(work)=EDI;

	_FP_SEG(SNDWorkStore)=SEG_TGBIOS_DATA;
	_FP_OFF(SNDWorkStore)=SND_BIOS_WORK;
	SNDWorkStore[0]=EDI;
	SNDWorkStore[1]=GS;

	MEMSETB_FAR(work,0,sizeof(struct SND_Work));


	// Mute everything.
	sysInfo->elevol_mute=0;
	_outb(TOWNSIO_SOUND_MUTE,0);
	_outb(TOWNSIO_ELEVOL_1_COM,0);
	_outb(TOWNSIO_ELEVOL_1_COM,1);
	_outb(TOWNSIO_ELEVOL_2_COM,0);
	_outb(TOWNSIO_ELEVOL_2_COM,1);
	_outb(TOWNSIO_ELEVOL_2_COM,2);
	_outb(TOWNSIO_ELEVOL_2_COM,3);



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
	_Far struct SND_Work *work;
	_FP_SEG(work)=GS;
	_FP_OFF(work)=EDI;

	SND_SetError(EAX,SND_NO_ERROR);
		TSUGARU_BREAK;
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
	_Far struct SND_Work *work;
	_FP_SEG(work)=GS;
	_FP_OFF(work)=EDI;

	SND_SetError(EAX,SND_NO_ERROR);
		TSUGARU_BREAK;
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

void SND_FM_READ_STATUS(
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

void SND_FM_WRITE_DATA(
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

void SND_FM_READ_DATA(
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

void SND_FM_WRITE_SAVE_ATA(
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

void SND_FM_READ_SAVE_DATA(
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
	_Far struct TBIOS_System_Info *info=SYSINFO_GetStruct();
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
		unsigned char reg27H=info->YM2612_REG27H;

		YM2612_Write(0,0x25,count&3);
		YM2612_Write(0,0x24,count>>2);

		reg27H&=0xCA; // Preseve Timer B flag permission, and CH3 MODE
		YM2612_Write(0,0x27,0x15|reg27H); // MODEMODE|ResetA|ResetB|PermitFlagA|PermitFlagB|LoadB|LoadA

		info->YM2612_REG27H=reg27H;
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
	_Far struct TBIOS_System_Info *info=SYSINFO_GetStruct();
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
		unsigned char reg27H=info->YM2612_REG27H;

		YM2612_Write(0,0x25,count&3);
		YM2612_Write(0,0x24,count>>2);

		reg27H&=0xC5; // Preseve Timer B flag permission, and CH3 MODE
		YM2612_Write(0,0x27,0x2A|reg27H); // MODEMODE|ResetA|ResetB|PermitFlagA|PermitFlagB|LoadB|LoadA

		info->YM2612_REG27H=reg27H;
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
	_Far struct SND_Work *work;
	_FP_SEG(work)=GS;
	_FP_OFF(work)=EDI;

	SND_SetError(EAX,SND_NO_ERROR);
		TSUGARU_BREAK;
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
	_Far struct SND_Work *work;
	_FP_SEG(work)=GS;
	_FP_OFF(work)=EDI;

	SND_SetError(EAX,SND_NO_ERROR);
		TSUGARU_BREAK;
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
	_Far struct SND_Work *work;
	_FP_SEG(work)=GS;
	_FP_OFF(work)=EDI;

	SND_SetError(EAX,SND_NO_ERROR);
		TSUGARU_BREAK;
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
	_Far struct SND_Work *work;
	_FP_SEG(work)=GS;
	_FP_OFF(work)=EDI;

	SND_SetError(EAX,SND_NO_ERROR);
		TSUGARU_BREAK;
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
	_Far struct SND_Work *work;
	_FP_SEG(work)=GS;
	_FP_OFF(work)=EDI;

	SND_SetError(EAX,SND_NO_ERROR);
		TSUGARU_BREAK;
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
	_Far struct SND_Work *work;
	_FP_SEG(work)=GS;
	_FP_OFF(work)=EDI;

	SND_SetError(EAX,SND_NO_ERROR);
		TSUGARU_BREAK;
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

void SND_25H_PCM_PCM_VOICE_PLAY(
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
	_Far struct SND_Work *work;
	_FP_SEG(work)=GS;
	_FP_OFF(work)=EDI;

	SND_SetError(EAX,SND_NO_ERROR);
		TSUGARU_BREAK;
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
	_Far struct SND_Work *work;
	_FP_SEG(work)=GS;
	_FP_OFF(work)=EDI;

	SND_SetError(EAX,SND_NO_ERROR);
		TSUGARU_BREAK;
}

void SND_2EH_PCM_HIGHRES_PLAY(
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
	_Far struct SND_Work *work;
	_FP_SEG(work)=GS;
	_FP_OFF(work)=EDI;

	SND_SetError(EAX,SND_NO_ERROR);
		TSUGARU_BREAK;
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
	unsigned char port=EDX&1,pad;
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
	_Far struct SND_Work *work;
	_FP_SEG(work)=GS;
	_FP_OFF(work)=EDI;

	SND_SetError(EAX,SND_NO_ERROR);
		TSUGARU_BREAK;
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
	_Far struct SND_Work *work;
	_FP_SEG(work)=GS;
	_FP_OFF(work)=EDI;

	SND_SetError(EAX,SND_NO_ERROR);
		TSUGARU_BREAK;
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
	_Far struct TBIOS_System_Info *sysInfo=SYSINFO_GetStruct();

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
	_Far struct TBIOS_System_Info *sysInfo=SYSINFO_GetStruct();
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
	_Far struct SND_Work *work;
	_FP_SEG(work)=GS;
	_FP_OFF(work)=EDI;

	SND_SetError(EAX,SND_NO_ERROR);
		TSUGARU_BREAK;
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

