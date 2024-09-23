#include <stdio.h>
#include <stdlib.h>
#include <snd.h>

char SND_work[SndWorkSize];

#define TSUGARU_BREAK _inline(0xE6,0xEA);
#define TSUGARU_STATE _inline(0xE6,0xEB);

int main(void)
{
	TSUGARU_BREAK;

	SND_init(SND_work);

	SND_fm_timer_a_set(1,0xC00);
	SND_fm_timer_b_set(1,0x80);

	SND_fm_timer_a_set(0,0);
	SND_fm_timer_b_set(0,0);

	SND_fm_timer_a_start();
	SND_fm_timer_b_start();

	SND_end();

	return 0;
}
