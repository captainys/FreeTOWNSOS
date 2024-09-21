#include <stdio.h>
#include <snd.h>

char SND_work[SndWorkSize];

int main(void)
{
	SND_init(SND_work);
	SND_end();
	printf("SND test done.\n");
	return 0;
}
