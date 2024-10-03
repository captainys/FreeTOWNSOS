#include <math.h>

int main(void)
{
	int i;
	for(i=-8192; i<=8192; i+=64)
	{
		double e=(double)i/8192.0;
		double s=pow(2.0,e);
		printf("\t0x%08x, // %lf\n",(int)(s*65536),s);
	}
	return 0;
}
