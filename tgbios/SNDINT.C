// _I386 macro is needed for DOS.H to define _Handle and _real_int_handler_t
#define _I386
#include <DOS.H>
#include "TGBIOS.H"
#include "SNDINT.H"
#include "MACRO.H"
#include "IODEF.H"
#include "UTIL.H"

#define _PUSHFD _inline(0x9C);
#define _POPFD _inline(0x9D);
#define _CLI _inline(0xfa);


void __SET_RPVECTP(int INTNum,_Handler);
unsigned long __GET_RVECT(int INTNum);
_Far void (*__GET_PVECT(int INTNum))(void) ;
void __SET_RVECT(int INTNum,unsigned long handler);
void __SET_PVECT(int INTNum,_Far void (*func)(void));


#define SNDINT_USING_TIMERB_MOUSE 1
#define SNDINT_USING_TIMERB_SOUND 2
#define SNDINT_USING_TIMERA 4
#define SNDINT_USING_PCM 8
#define SNDINT_PIC_ENABLED 0x10000

#define SNDINT_ALL_USE_FLAGS 0x0F
#define SNDINT_TIMERB_USE_FLAGS 0x03

struct SoundInterruptBIOSContext
{
	unsigned int flags;  // Will be initialized to zero on first call.

	_Far void (*save_INT4DProt)(void);
	unsigned long save_INT4DReal;
};

static _Far struct SoundInterruptBIOSContext *SNDINT_GetContext(void);



#pragma Calling_convention(_INTERRUPT|_CALLING_CONVENTION);
_Handler Handle_INT4DH(void)
{
	// DOS-Extender intercepts INT 46H in its own handler, then redirect to this handler by CALLF.
	// Must return by RETF.
	// _Far is the keyword in High-C.
	return 0;
}
#pragma Calling_convention();



// This BIOS did not exist in Towns OS V1.1
// The explanation of the Red Book does not make sense.
// I'll be worried about external usage later.  Probably games did not use this BIOS anyway.


// Red-book Appendix C9 suggests registering INT-Handler and setting up PIC is NOT the responsibility of Sound-INT Manager BIOS.
// It seems that INT 4DH should be enabled all the time.


void Unmask_PIC_INT4D(_Far struct SoundInterruptBIOSContext *context)
{
	if(0==(context->flags&SNDINT_PIC_ENABLED))
	{
		_Handler newINT4D;
		newINT4D=Handle_INT4DH;
		_FP_SEG(newINT4D)=SEG_TGBIOS_CODE;
		context->save_INT4DProt=__GET_PVECT(0x4D);
		context->save_INT4DReal=__GET_RVECT(0x4D);
		__SET_RPVECTP(0x4D,newINT4D);
		context->flags|=SNDINT_PIC_ENABLED;
	}
}

static void Mask_PIC_INT4D(_Far struct SoundInterruptBIOSContext *context)
{
	if(0!=(context->flags&SNDINT_PIC_ENABLED))
	{
		context->flags&=~SNDINT_PIC_ENABLED;
		__SET_PVECT(0x4D,context->save_INT4DProt);
		__SET_RVECT(0x4D,context->save_INT4DReal);
	}
}

static void Start_TimerB(_Far struct SoundInterruptBIOSContext *context)
{
}

static void Stop_TimerB(_Far struct SoundInterruptBIOSContext *context)
{
}

static void Start_TimerA(_Far struct SoundInterruptBIOSContext *context)
{
}

static void Stop_TimerA(_Far struct SoundInterruptBIOSContext *context)
{
}

void SNDINT_Internal_Start_Mouse(void)
{
	_Far struct SoundInterruptBIOSContext *context=SNDINT_GetContext();
	_PUSHFD
	_CLI
	if(0==(context->flags&SNDINT_USING_TIMERB_MOUSE))
	{
		if(0==(context->flags&SNDINT_PIC_ENABLED))
		{
			Unmask_PIC_INT4D(context);
		}
		Start_TimerB(context);
		context->flags|=SNDINT_USING_TIMERB_MOUSE;
	}
	_POPFD
}
void SNDINT_Internal_Start_Sound_TimerB(void)
{
	_Far struct SoundInterruptBIOSContext *context=SNDINT_GetContext();
	_PUSHFD
	_CLI
	if(0==(context->flags&SNDINT_USING_TIMERB_SOUND))
	{
		if(0==(context->flags&SNDINT_PIC_ENABLED))
		{
			Unmask_PIC_INT4D(context);
		}
		Start_TimerB(context);
		context->flags|=SNDINT_USING_TIMERB_SOUND;
	}
	_POPFD
}
void SNDINT_Internal_Start_Sound_TimerA(void)
{
	_Far struct SoundInterruptBIOSContext *context=SNDINT_GetContext();
	_PUSHFD
	_CLI
	if(0==(context->flags&SNDINT_USING_TIMERA))
	{
		if(0==(context->flags&SNDINT_PIC_ENABLED))
		{
			Unmask_PIC_INT4D(context);
		}
		Start_TimerA(context);
		context->flags|=SNDINT_USING_TIMERA;
	}
	_POPFD
}
void SNDINT_Internal_Start_PCM(void)
{
	_Far struct SoundInterruptBIOSContext *context=SNDINT_GetContext();
	_PUSHFD
	_CLI
	if(0==(context->flags&SNDINT_USING_PCM))
	{
		if(0==(context->flags&SNDINT_PIC_ENABLED))
		{
			Unmask_PIC_INT4D(context);
		}
		context->flags|=SNDINT_USING_PCM;
	}
	_POPFD
}

void SNDINT_Internal_Stop_Mouse(void)
{
	_Far struct SoundInterruptBIOSContext *context=SNDINT_GetContext();
	_PUSHFD
	_CLI
	if(0!=(context->flags&SNDINT_USING_TIMERB_MOUSE))
	{
		context->flags&=~SNDINT_USING_TIMERB_MOUSE;
		if(0==(context->flags&SNDINT_TIMERB_USE_FLAGS))
		{
			Stop_TimerB(context);
		}
		if(0==(context->flags&SNDINT_ALL_USE_FLAGS))
		{
			Mask_PIC_INT4D(context);
		}
	}
	_POPFD
}
void SNDINT_Internal_Stop_Sound_TimerB(void)
{
	_Far struct SoundInterruptBIOSContext *context=SNDINT_GetContext();
	_PUSHFD
	_CLI
	if(0!=(context->flags&SNDINT_USING_TIMERB_SOUND))
	{
		context->flags&=~SNDINT_USING_TIMERB_SOUND;
		if(0==(context->flags&SNDINT_TIMERB_USE_FLAGS))
		{
			Stop_TimerB(context);
		}
		if(0==(context->flags&SNDINT_ALL_USE_FLAGS))
		{
			Mask_PIC_INT4D(context);
		}
	}
	_POPFD
}
void SNDINT_Internal_Stop_Sound_TimerA(void)
{
	_Far struct SoundInterruptBIOSContext *context=SNDINT_GetContext();
	_PUSHFD
	_CLI
	if(0!=(context->flags&SNDINT_USING_TIMERA))
	{
		context->flags&=~SNDINT_USING_TIMERA;
		if(0==(context->flags&SNDINT_USING_TIMERA))
		{
			Stop_TimerA(context);
		}
		if(0==(context->flags&SNDINT_ALL_USE_FLAGS))
		{
			Mask_PIC_INT4D(context);
		}
	}
	_POPFD
}
void SNDINT_Internal_Stop_PCM(void)
{
	_Far struct SoundInterruptBIOSContext *context=SNDINT_GetContext();
	_PUSHFD
	_CLI
	if(0!=(context->flags&SNDINT_USING_PCM))
	{
		context->flags&=~SNDINT_USING_PCM;
		if(0==(context->flags&SNDINT_ALL_USE_FLAGS))
		{
			Mask_PIC_INT4D(context);
		}
	}
	_POPFD
}



void SNDINT_01H_REGISTER_MOUSE_INT(
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
void SNDINT_02H_UNREGISTER_MOUSE_INT(
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
void SNDINT_03H_REGISTER_SOUND_INT(
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
void SNDINT_04H_UNREGISTER_SOUND_INT(
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
void SNDINT_05H_GET_MOUSE_INT_COUNT(
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
void SNDINT_06H_REGISTER_INT_PROC(
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
void SNDINT_07H_UNREGISTER_INT_PROC(
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

void SNDINT_08H_GET_INT_PROC(
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
void SNDINT_09H_GET_INT_STATUS(
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


static unsigned char firstTime=1;
static struct SoundInterruptBIOSContext context;

static _Far struct SoundInterruptBIOSContext *SNDINT_GetContext(void)
{
	_Far struct SoundInterruptBIOSContext *ptr;
	_FP_SEG(ptr)=SEG_TGBIOS_DATA;
	_FP_OFF(ptr)=(unsigned long int)&context;
	if(0!=firstTime)
	{
		MEMSETB_FAR(ptr,0,sizeof(context));
		firstTime=0;
	}
	return ptr;
}
