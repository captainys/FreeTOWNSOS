#include <DOS.H>
#include "TGBIOS.H"
#include "SNDINT.H"
#include "MACRO.H"
#include "IODEF.H"
#include "UTIL.H"


#define SNDINT_NUM_CALLBACKS 7

struct SoundInterruptBIOSContext
{
	struct SNDINT_Callback callback[SNDINT_NUM_CALLBACKS];
};

static _Far struct SoundInterruptBIOSContext *SNDINT_GetContext(void);



// This BIOS did not exist in Towns OS V1.1
// The explanation of the Red Book does not make sense.
// I'll be worried about it later.



static unsigned char firstTime=1;
static struct SoundInterruptBIOSContext context;

static _Far struct SoundInterruptBIOSContext *SNDINT_GetContext(void)
{
	if(0!=firstTime)
	{
		MEMSETB_FAR(&context,0,sizeof(context));
		firstTime=0;
	}
	return &context;
}
