// _I386 macro is needed for DOS.H to define _Handle and _real_int_handler_t
#define _I386
#include <DOS.H>
#include "TGBIOS.H"
#include "SNDINT.H"
#include "MACRO.H"
#include "IODEF.H"
#include "UTIL.H"

extern void REGISTER_PROTECTED_MODE_INT(int INTNum,_Far void *func);

#define _PUSHFD _inline(0x9C);
#define _POPFD _inline(0x9D);
#define _CLI _inline(0xfa);


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

	_Handler save_INT4DProt;
	_real_int_handler_t save_INT4DReal;
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


static void Unmask_PIC_INT4D(_Far struct SoundInterruptBIOSContext *context)
{
	if(0==(context->flags&SNDINT_PIC_ENABLED))
	{
		_Handler newINT4D;
		newINT4D=Handle_INT4DH;
		_FP_SEG(newINT4D)=SEG_TGBIOS_CODE;
		context->save_INT4DProt=_getpvect(0x4D);
		context->save_INT4DReal=_getrvect(0x4D);
		_setrpvectp(0x4D,newINT4D);
		context->flags|=SNDINT_PIC_ENABLED;
	}
}

static void Mask_PIC_INT4D(_Far struct SoundInterruptBIOSContext *context)
{
	if(0!=(context->flags&SNDINT_PIC_ENABLED))
	{
		context->flags&=~SNDINT_PIC_ENABLED;
		_setpvect(0x4D,context->save_INT4DProt);
		_setrvect(0x4D,context->save_INT4DReal);
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
