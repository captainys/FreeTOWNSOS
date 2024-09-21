#include <snd.h>

char SND_work[SndWorkSize];

int main(void)
{
	SND_init(SND_work);
	SND_end();
	return 0;
}
