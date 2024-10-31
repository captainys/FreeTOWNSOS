#include <stdio.h>
#include <stdlib.h>
#include <snd.h>

char SND_work[SndWorkSize];

#define TSUGARU_BREAK _inline(0xE6,0xEA);
#define TSUGARU_STATE _inline(0xE6,0xEB);

int main(void)
{
	int l_vol,r_vol,sw_bit;

	TSUGARU_BREAK;

	SND_init(SND_work);

	SND_elevol_read(0,&l_vol,&r_vol);
	SND_elevol_read(1,&l_vol,&r_vol);
	SND_elevol_read(2,&l_vol,&r_vol);
	SND_elevol_read(3,&l_vol,&r_vol);

	SND_elevol_set(0,127,127);
	SND_elevol_set(1,127,127);
	SND_elevol_set(2,127,127);
	SND_elevol_set(3,127,127);

	SND_elevol_mute(0xFF);
	SND_elevol_mute(0);

	SND_elevol_read(0,&l_vol,&r_vol);
	SND_elevol_read(1,&l_vol,&r_vol);
	SND_elevol_read(2,&l_vol,&r_vol);
	SND_elevol_read(3,&l_vol,&r_vol);

	SND_get_elevol_mute(&sw_bit);

	SND_end();

	return 0;
}
