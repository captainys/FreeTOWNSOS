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
	_Far struct EGB_Work *work;
	_FP_SEG(work)=GS;
	_FP_OFF(work)=EDI;

	SND_SetError(EAX,SND_NO_ERROR);
		TSUGARU_BREAK;
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
	_Far struct EGB_Work *work;
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
	_Far struct EGB_Work *work;
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
	_Far struct EGB_Work *work;
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
	_Far struct EGB_Work *work;
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
	_Far struct EGB_Work *work;
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
	_Far struct EGB_Work *work;
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
	_Far struct EGB_Work *work;
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
	_Far struct EGB_Work *work;
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
	_Far struct EGB_Work *work;
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
	_Far struct EGB_Work *work;
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
	_Far struct EGB_Work *work;
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
	_Far struct EGB_Work *work;
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
	_Far struct EGB_Work *work;
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
	_Far struct EGB_Work *work;
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
	_Far struct EGB_Work *work;
	_FP_SEG(work)=GS;
	_FP_OFF(work)=EDI;

	SND_SetError(EAX,SND_NO_ERROR);
		TSUGARU_BREAK;
}

void SND_FM_TIMER_A_SET(
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
	_Far struct EGB_Work *work;
	_FP_SEG(work)=GS;
	_FP_OFF(work)=EDI;

	SND_SetError(EAX,SND_NO_ERROR);
		TSUGARU_BREAK;
}

void SND_FM_TIMER_B_SET(
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
	_Far struct EGB_Work *work;
	_FP_SEG(work)=GS;
	_FP_OFF(work)=EDI;

	SND_SetError(EAX,SND_NO_ERROR);
		TSUGARU_BREAK;
}

void SND_FM_TIMER_A_START(
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
	_Far struct EGB_Work *work;
	_FP_SEG(work)=GS;
	_FP_OFF(work)=EDI;

	SND_SetError(EAX,SND_NO_ERROR);
		TSUGARU_BREAK;
}

void SND_FM_TIMER_B_START(
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
	_Far struct EGB_Work *work;
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
	_Far struct EGB_Work *work;
	_FP_SEG(work)=GS;
	_FP_OFF(work)=EDI;

	SND_SetError(EAX,SND_NO_ERROR);
		TSUGARU_BREAK;
}

void SND_PCM_WAVE_TRANSFER(
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
	_Far struct EGB_Work *work;
	_FP_SEG(work)=GS;
	_FP_OFF(work)=EDI;

	SND_SetError(EAX,SND_NO_ERROR);
		TSUGARU_BREAK;
}

void SND_PCM_MODE_SET(
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
	_Far struct EGB_Work *work;
	_FP_SEG(work)=GS;
	_FP_OFF(work)=EDI;

	SND_SetError(EAX,SND_NO_ERROR);
		TSUGARU_BREAK;
}

void SND_PCM_SOUND_SET(
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
	_Far struct EGB_Work *work;
	_FP_SEG(work)=GS;
	_FP_OFF(work)=EDI;

	SND_SetError(EAX,SND_NO_ERROR);
		TSUGARU_BREAK;
}

void SND_PCM_SOUND_DELETE(
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
	_Far struct EGB_Work *work;
	_FP_SEG(work)=GS;
	_FP_OFF(work)=EDI;

	SND_SetError(EAX,SND_NO_ERROR);
		TSUGARU_BREAK;
}

void SND_PCM_REC_START(
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
	_Far struct EGB_Work *work;
	_FP_SEG(work)=GS;
	_FP_OFF(work)=EDI;

	SND_SetError(EAX,SND_NO_ERROR);
		TSUGARU_BREAK;
}

void SND_PCM_PCM_VOICE_PLAY(
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
	_Far struct EGB_Work *work;
	_FP_SEG(work)=GS;
	_FP_OFF(work)=EDI;

	SND_SetError(EAX,SND_NO_ERROR);
		TSUGARU_BREAK;
}

void SND_PCM_REC_STOP(
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
	_Far struct EGB_Work *work;
	_FP_SEG(work)=GS;
	_FP_OFF(work)=EDI;

	SND_SetError(EAX,SND_NO_ERROR);
		TSUGARU_BREAK;
}

void SND_PCM_PCM_VOICE_STOP(
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
	_Far struct EGB_Work *work;
	_FP_SEG(work)=GS;
	_FP_OFF(work)=EDI;

	SND_SetError(EAX,SND_NO_ERROR);
		TSUGARU_BREAK;
}

void SND_PCM_PCM_VOICE_STATUS(
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
	_Far struct EGB_Work *work;
	_FP_SEG(work)=GS;
	_FP_OFF(work)=EDI;

	SND_SetError(EAX,SND_NO_ERROR);
		TSUGARU_BREAK;
}

void SND_PCM_ABORT(
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
	_Far struct EGB_Work *work;
	_FP_SEG(work)=GS;
	_FP_OFF(work)=EDI;

	SND_SetError(EAX,SND_NO_ERROR);
		TSUGARU_BREAK;
}

void SND_PCM_PCMRAM_TO_MAINRAM(
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
	_Far struct EGB_Work *work;
	_FP_SEG(work)=GS;
	_FP_OFF(work)=EDI;

	SND_SetError(EAX,SND_NO_ERROR);
		TSUGARU_BREAK;
}

void SND_PCM_PCMRAM_TO_PCMRAM(
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
	_Far struct EGB_Work *work;
	_FP_SEG(work)=GS;
	_FP_OFF(work)=EDI;

	SND_SetError(EAX,SND_NO_ERROR);
		TSUGARU_BREAK;
}

void SND_PCM_TRANSFER2(
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
	_Far struct EGB_Work *work;
	_FP_SEG(work)=GS;
	_FP_OFF(work)=EDI;

	SND_SetError(EAX,SND_NO_ERROR);
		TSUGARU_BREAK;
}

void SND_PCM_HIGHRES_PLAY(
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
	_Far struct EGB_Work *work;
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
	_Far struct EGB_Work *work;
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
	_Far struct EGB_Work *work;
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
	_Far struct EGB_Work *work;
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
	_Far struct EGB_Work *work;
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
	pad&=0x3F;

	if(0==(pad&(PAD_LEFT|PAD_RIGHT)))
	{
		pad&=(~PAD_RUN);
	}
	if(0==(pad&(PAD_UP|PAD_DOWN)))
	{
		pad&=(~PAD_SELECT);
	}

	EDX&=0xFFFFFF00;
	EDX|=pad;
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
	_Far struct EGB_Work *work;
	_FP_SEG(work)=GS;
	_FP_OFF(work)=EDI;

	SND_SetError(EAX,SND_NO_ERROR);
		TSUGARU_BREAK;
}

void SND_ELEVOL_SET(
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
	_Far struct EGB_Work *work;
	_FP_SEG(work)=GS;
	_FP_OFF(work)=EDI;

	SND_SetError(EAX,SND_NO_ERROR);
		TSUGARU_BREAK;
}

void SND_ELEVOL_INIT(
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
	_Far struct EGB_Work *work;
	_FP_SEG(work)=GS;
	_FP_OFF(work)=EDI;

	SND_SetError(EAX,SND_NO_ERROR);
		TSUGARU_BREAK;
}

void SND_ELEVOL_READ(
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
	_Far struct EGB_Work *work;
	_FP_SEG(work)=GS;
	_FP_OFF(work)=EDI;

	SND_SetError(EAX,SND_NO_ERROR);
		TSUGARU_BREAK;
}

void SND_ELEVOL_MUTE(
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
	_Far struct EGB_Work *work;
	_FP_SEG(work)=GS;
	_FP_OFF(work)=EDI;

	SND_SetError(EAX,SND_NO_ERROR);
		TSUGARU_BREAK;
}

void SND_ELEVOL_ALL_MUTE(
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
	_Far struct EGB_Work *work;
	_FP_SEG(work)=GS;
	_FP_OFF(work)=EDI;

	SND_SetError(EAX,SND_NO_ERROR);
		TSUGARU_BREAK;
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
	_Far struct EGB_Work *work;
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
	_Far struct EGB_Work *work;
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
