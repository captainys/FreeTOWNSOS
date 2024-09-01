/* Test behavior of I/O 208H

Experimented with Real TOWNS II MX.  Cannot find a condition to clear DSKCHG bit.

*/

#include <stdio.h>
#include <conio.h>
#include <time.h>

int main(int ac,char *av[])
{
	int k=0;

	for(int i=0; i<ac; ++i)
	{
		if(0==strcmp("-mon",av[i]))
		{
			printf("Motor On\n");
			_outp(0x208,0x10);
		}
		else if(0==strcmp("-moff",av[i]))
		{
			printf("Motor Off\n");
			_outp(0x208,0x10);
		}
	}

	for(;;)
	{
		int i;
		for(i=0; i<4; ++i)
		{
			_outp(0x20C,1<<i);
			unsigned char sta=_inp(0x208);
			_inp(0x208);
			_inp(0x208);
			_inp(0x208);
			_inp(0x208); // Does reading multiple times in a row for the same drive clear the bit?  It doesn't.
			printf("%02x ",sta);

			if(0==k%10)
			{
				while(0!=(_inp(0x200)&1))
				{
				}
				_outp(0x200,0x00); // Tried to see if a FDC command clears the DSKCHG bit.  It didn't.
			}
		}
		printf("\n");

		{
			clock_t t0=clock();
			while(clock()-t0<CLOCKS_PER_SEC/2)
			{
			}
		}

		++k;
	}
	return 0;
}
