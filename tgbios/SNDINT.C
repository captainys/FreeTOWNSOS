// _I386 macro is needed for DOS.H to define _Handle and _real_int_handler_t
#define _I386
#include <DOS.H>
#include "TGBIOS.H"
#include "SNDINT.H"
#include "SND.H"
#include "MACRO.H"
#include "IODEF.H"
#include "UTIL.H"


// If the Red-Book Appendix C.9 example is correct, registering INT 4DH and masking/unmasking PIC is NOT the
// responsibility of this Sound INT Manager.



#ifdef MY_RESPONSIBILITY //

void __SET_RPVECTP(int INTNum,_Handler);
unsigned long __GET_RVECT(int INTNum);
_Far void (*__GET_PVECT(int INTNum))(void) ;
void __SET_RVECT(int INTNum,unsigned long handler);
void __SET_PVECT(int INTNum,_Far void (*func)(void));

#endif // MY_RESPONSIBILITY

void CALL_SNDINT_HANDLER(_Far void *SSESP,_Far void (*CSEIP)(void),unsigned int DS,unsigned int ES,unsigned int FS,unsigned int GS);



#define SNDINT_USING_TIMERB_MOUSE 1
#define SNDINT_USING_TIMERB_SOUND 2
#define SNDINT_USING_TIMERA 4
#define SNDINT_USING_PCM 8
#define SNDINT_PIC_ENABLED 0x10000

#define SNDINT_ALL_USE_FLAGS 0x0F
#define SNDINT_TIMERB_USE_FLAGS 0x03

struct SoundInterruptBIOSContext
{
	char ID[8];
	unsigned int flags;  // Will be initialized to zero on first call.
	unsigned int reentCount;
#ifdef MY_RESPONSIBILITY
	_Far void (*save_INT4DProt)(void);
	unsigned long save_INT4DReal;
#endif

	_Far void *mouseINTStack;
	_Far void *soundINTStack;

	struct SNDINT_Callback timerAPreEOICallback;
	struct SNDINT_Callback timerBCallback;
	struct SNDINT_Callback timerAPostEOICallback;
	struct SNDINT_Callback mouseCallback1;
	struct SNDINT_Callback mouseCallback2;
};

static _Far struct SoundInterruptBIOSContext *SNDINT_GetContext(void);

#pragma Calling_convention(_INTERRUPT|_CALLING_CONVENTION);
_Handler Handle_INT4DH(void)
{
	// DOS-Extender intercepts INT 46H in its own handler, then redirect to this handler by CALLF.
	// Must return by RETF.
	// _Far is the keyword in High-C.
	unsigned char INTReason=_inb(TOWNSIO_SOUND_INT_REASON);
	unsigned char callTimerAPost=0;

	_PUSH_FS;
	_PUSH_GS;

	_Far struct SoundInterruptBIOSContext *context=SNDINT_GetContext();
	if(INTReason&1)
	{
		unsigned char timerUP=_inb(TOWNSIO_SOUND_STATUS_ADDRESS0);
		timerUP&=3;

		if(timerUP&1)
		{
			if((context->flags & SNDINT_USING_TIMERA) &&
			   NULL!=context->timerAPreEOICallback.callback)
			{
				CALL_SNDINT_HANDLER(
					context->soundINTStack,
					context->timerAPreEOICallback.callback,
					context->timerAPreEOICallback.DS,
					context->timerAPreEOICallback.ES,
					context->timerAPreEOICallback.FS,
					context->timerAPreEOICallback.GS);
			}
			SND_FM_Timer_A_Restart();
			callTimerAPost=1;
		}
		if(timerUP&2)
		{
			SND_FM_Timer_B_Restart();
		}
	}
	if(INTReason&8)
	{
		if(context->flags&SNDINT_USING_PCM)
		{
			SND_PCM_Voice_Mode_Interrupt();
		}
		else
		{
			// Just remoe IRR
			_inb(TOWNSIO_SOUND_PCM_INT); // Was it good?
			TSUGARU_BREAK;
		}
	}
	_outb(TOWNSIO_PIC_SECONDARY_ICW1,0x65); // Specific EOI + INT (13-8=5)(4DH).

	if(callTimerAPost &&
	   (context->flags & SNDINT_USING_TIMERA) &&
	   0==context->reentCount && NULL!=context->timerAPostEOICallback.callback)
	{
		context->reentCount=1;
		_STI;
		CALL_SNDINT_HANDLER(
			NULL,  // I thinks this post-EOI handler must use the same stack frame.
			context->timerAPostEOICallback.callback,
			context->timerAPostEOICallback.DS,
			context->timerAPostEOICallback.ES,
			context->timerAPostEOICallback.FS,
			context->timerAPostEOICallback.GS);
	}

	_POP_GS;
	_POP_FS;
	return 0;
}
#pragma Calling_convention();



// This BIOS did not exist in Towns OS V1.1
// The explanation of the Red Book does not make sense.
// I'll be worried about external usage later.  Probably games did not use this BIOS anyway.


// Red-book Appendix C9 suggests registering INT-Handler and setting up PIC is NOT the responsibility of Sound-INT Manager BIOS.
// It seems that INT 4DH should be enabled all the time.


#ifdef MY_RESPONSIBILITY
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
#endif

void SNDINT_Internal_Start_Mouse(unsigned int DS,unsigned int EDX)
{
	_Far struct SoundInterruptBIOSContext *context=SNDINT_GetContext();
	_PUSHFD
	_CLI
	if(0==(context->flags&SNDINT_USING_TIMERB_MOUSE))
	{
		#ifdef MY_RESPONSIBILITY
		{
			if(0==(context->flags&SNDINT_PIC_ENABLED))
			{
				Unmask_PIC_INT4D(context);
			}
			Start_TimerB(context);
		}
		#endif

		void *stk;
		_FP_SEG(stk)=DS;
		_FP_OFF(stk)=EDX;

		context->flags|=SNDINT_USING_TIMERB_MOUSE;
		context->mouseINTStack=stk;
		context->mouseCallback1.callback=NULL;
		context->mouseCallback2.callback=NULL;
	}
	_POPFD
}
void SNDINT_Internal_Start_Sound_TimerB(unsigned int DS,unsigned int EDX)
{
	_Far struct SoundInterruptBIOSContext *context=SNDINT_GetContext();
	_PUSHFD
	_CLI
	if(0==(context->flags&SNDINT_USING_TIMERB_SOUND))
	{
		#ifdef MY_RESPONSIBILITY
		{
			if(0==(context->flags&SNDINT_PIC_ENABLED))
			{
				Unmask_PIC_INT4D(context);
			}
		}
		Start_TimerB(context);
		#endif

		void *stk;
		_FP_SEG(stk)=DS;
		_FP_OFF(stk)=EDX;

		context->flags|=SNDINT_USING_TIMERB_SOUND;
		context->soundINTStack=stk;
		context->timerBCallback.callback=NULL;
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
		#ifdef MY_RESPONSIBILITY
		{
			if(0==(context->flags&SNDINT_PIC_ENABLED))
			{
				Unmask_PIC_INT4D(context);
			}
			Start_TimerA(context);
		}
		#endif
		context->flags|=SNDINT_USING_TIMERA;
		context->timerAPreEOICallback.callback=NULL;
		context->timerAPostEOICallback.callback=NULL;
	}
	_POPFD
}
void SNDINT_Internal_Start_PCM(void)
{
TSUGARU_STATE;
	_Far struct SoundInterruptBIOSContext *context=SNDINT_GetContext();
	_PUSHFD
	_CLI
	if(0==(context->flags&SNDINT_USING_PCM))
	{
		#ifdef MY_RESPONSIBILITY
		{
			if(0==(context->flags&SNDINT_PIC_ENABLED))
			{
				Unmask_PIC_INT4D(context);
			}
		}
		#endif
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
		#ifdef MY_RESPONSIBILITY
		{
			if(0==(context->flags&SNDINT_TIMERB_USE_FLAGS))
			{
				Stop_TimerB(context);
			}
			if(0==(context->flags&SNDINT_ALL_USE_FLAGS))
			{
				Mask_PIC_INT4D(context);
			}
		}
		#endif
		context->mouseCallback1.callback=NULL;
		context->mouseCallback2.callback=NULL;
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
		#ifdef MY_RESPONSIBILITY
		{
			if(0==(context->flags&SNDINT_TIMERB_USE_FLAGS))
			{
				Stop_TimerB(context);
			}
			if(0==(context->flags&SNDINT_ALL_USE_FLAGS))
			{
				Mask_PIC_INT4D(context);
			}
		}
		#endif
		context->timerBCallback.callback=NULL;
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
		#ifdef MY_RESPONSIBILITY
		{
			if(0==(context->flags&SNDINT_USING_TIMERA))
			{
				Stop_TimerA(context);
			}
			if(0==(context->flags&SNDINT_ALL_USE_FLAGS))
			{
				Mask_PIC_INT4D(context);
			}
		}
		#endif
		context->timerAPreEOICallback.callback=NULL;
		context->timerAPostEOICallback.callback=NULL;
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
		#ifdef MY_RESPONSIBILITY
		{
			if(0==(context->flags&SNDINT_ALL_USE_FLAGS))
			{
				Mask_PIC_INT4D(context);
			}
		}
		#endif
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
	SNDINT_Internal_Start_Mouse(EDX,DS);
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
	SNDINT_Internal_Stop_Mouse();
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
	SNDINT_Internal_Start_Sound_TimerA();
	SNDINT_Internal_Start_Sound_TimerB(DS,EDX);
	SNDINT_Internal_Start_PCM();
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
	SNDINT_Internal_Stop_PCM();
	SNDINT_Internal_Stop_Sound_TimerB();
	SNDINT_Internal_Stop_Sound_TimerA();
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
	TSUGARU_STATE;
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
	unsigned char AL=EAX;
	_Far struct SoundInterruptBIOSContext *context=SNDINT_GetContext();
	_Far unsigned int *paramBlock;
	_FP_SEG(paramBlock)=DS;
	_FP_OFF(paramBlock)=ESI;

	_Far void (*callback)(void);
	_FP_OFF(callback)=paramBlock[0];
	_FP_SEG(callback)=paramBlock[1];

	switch(AL)
	{
	case 0:
		context->timerAPreEOICallback.callback=callback;
		context->timerAPreEOICallback.DS=paramBlock[2];
		context->timerAPreEOICallback.ES=paramBlock[3];
		context->timerAPreEOICallback.FS=paramBlock[4];
		context->timerAPreEOICallback.GS=paramBlock[5];
		break;
	case 1:
		context->timerBCallback.callback=callback;
		context->timerBCallback.DS=paramBlock[2];
		context->timerBCallback.ES=paramBlock[3];
		context->timerBCallback.FS=paramBlock[4];
		context->timerBCallback.GS=paramBlock[5];
		break;
	case 2:
		context->timerAPostEOICallback.callback=callback;
		context->timerAPostEOICallback.DS=paramBlock[2];
		context->timerAPostEOICallback.ES=paramBlock[3];
		context->timerAPostEOICallback.FS=paramBlock[4];
		context->timerAPostEOICallback.GS=paramBlock[5];
		break;
	case 3:
		context->mouseCallback1.callback=callback;
		context->mouseCallback1.DS=paramBlock[2];
		context->mouseCallback1.ES=paramBlock[3];
		context->mouseCallback1.FS=paramBlock[4];
		context->mouseCallback1.GS=paramBlock[5];
		break;
	case 4:
		context->mouseCallback2.callback=callback;
		context->mouseCallback2.DS=paramBlock[2];
		context->mouseCallback2.ES=paramBlock[3];
		context->mouseCallback2.FS=paramBlock[4];
		context->mouseCallback2.GS=paramBlock[5];
		break;
	}

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
	TSUGARU_STATE;
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
	TSUGARU_STATE;
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
	TSUGARU_STATE;
}


// Do not use a static variable.  DS may be different.
// static unsigned char firstTime=1;
static struct SoundInterruptBIOSContext context={{0,0,0,0,0,0,0,0},0,0};

static _Far struct SoundInterruptBIOSContext *SNDINT_GetContext(void)
{
	_Far struct SoundInterruptBIOSContext *ptr;
	_FP_SEG(ptr)=SEG_TGBIOS_DATA;
	_FP_OFF(ptr)=(unsigned long int)&context;
	if(0==ptr->ID[0])
	{
		MEMSETB_FAR(ptr,0,sizeof(context));
		ptr->ID[0]='S';
		ptr->ID[1]='N';
		ptr->ID[2]='D';
		ptr->ID[3]='I';
		ptr->ID[4]='N';
		ptr->ID[5]='T';
		ptr->ID[6]=0;
		ptr->ID[7]=0;
	}
	return ptr;
}
