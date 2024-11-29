#include <dos.h>
#include <fcntl.h>
#include "RSDEBUG.H"
#include "PLTDEBUG.H"

int main(void)
{
	char str[256];
	int handle;
	PLTDEBUG(7,0xFF,0,0);
	RS232C_INIT();
	PLTDEBUG(7,0xFF,0xFF,0);
	RS232C_PUTS("Try opening a file that does not exist.\n\r");
	unsigned err=_dos_open("DOESNOTEXIST",_O_RDONLY,&handle);
	sprintf(str,"Err %d Handle %d\n\r",err,handle);
	RS232C_PUTS(str);
	puts(str);
	PLTDEBUG(7,0xFF,0xFF,0xFF);
	return 0;
}
