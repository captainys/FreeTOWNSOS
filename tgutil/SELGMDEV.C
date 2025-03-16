#include <stdio.h>
#include <stdlib.h>
#include "include/tgbios.h"

char *devLabel[]=
{
	"Normal or TOWNS Game Pad",
	"Fujitsu 6-button Game Pad",
	"CAPCOM CPSF",
	"SHARP Cyber Stick",
};

int main(int ac,char *av[])
{
	unsigned int port,type;

	if(3!=ac)
	{
		printf("Usage: FREE386 SELGMDEV.EXP port_number game_dev_type\n");
		printf("  Game device types:\n");
		printf("    0...Normal or TOWNS Game Pad\n");
		printf("    1...Fujitsu 6-button pad\n");
		printf("    2...CAPCOM CPSF\n");
		printf("    3...SHARP Cyber Stick\n");
		printf("This program enables an expanded functionality of TGBIOS.\n");
		printf("It let SND_joy_in_2 read from non-standard game pad and\n");
		printf("interprets buttons 5 and 6 as run and select buttons.\n");
		return 1;
	}

	port=atoi(av[1]);
	type=atoi(av[2]);

	if(1<port)
	{
		printf("Error: Port number needs to be 0 or 1.\n");
		return 1;
	}
	if(3<type)
	{
		printf("Error: Type needs to be 0,1,2, or 3.\n");
	}

	SND_SetGlobal_Gamedev_Type(port,type);
	printf("Configured Game Device Type of Port %d to %s\n",port,devLabel[type]);
	return 0;
}
