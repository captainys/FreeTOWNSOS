#include <stdio.h>

int main(void)
{
	char str[256];
	printf("Enetr:");
	fgets(str,255,stdin);
	printf(str);
	return 0;
}
