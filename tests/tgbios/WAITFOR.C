#include <snd.h>
#include <time.h>
#include "WAITFOR.H"

void WaitForPad(void)
{
	int status=0xFF;
	while(0x30==(status&0x30))
	{
		SND_joy_in_2(0,&status);
	}
	while(0x30!=(status&0x30))
	{
		SND_joy_in_2(0,&status);
	}
}

void Wait500ms(void)
{
	unsigned int accum=0;
	clock_t t0=clock();
	while(accum*2<CLOCKS_PER_SEC)
	{
		clock_t t1=clock();
		accum+=t1-t0;
		t0=t1;
	}
}
