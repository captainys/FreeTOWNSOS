#include <stdio.h>

unsigned int COUNT20000(void);

int main(void)
{
	int i;
	printf("This program writes AL to 06CH for 20000 times and\n");
	printf("show how many times free-run timer ticked.\n");
	for(i=0; i<20; ++i)
	{
		printf("%d\n",COUNT20000());
	}
	printf("Press Enter>");
	getchar();
	return 0;
}
