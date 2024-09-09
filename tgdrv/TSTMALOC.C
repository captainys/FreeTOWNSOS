#include <stdio.h>
#include <stdlib.h>

int main(void)
{
	unsigned char *buf;
	buf=malloc(2*1024*1024);
	printf("%08x\n",buf);
	printf(">");
	getchar();
	return 0;
}
