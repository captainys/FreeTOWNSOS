#include <stdio.h>
#include "diskbios.h"

int main(void)
{
	printf("DKB_restore returns %04xH\n",DKB_restore(BIOS_FD0));
	return 0;
}
