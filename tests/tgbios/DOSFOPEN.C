#include <dos.h>
#include <fcntl.h>

int main(void)
{
	char str[256];
	int handle;
	unsigned err=_dos_open("DOESNOTEXIST",_O_RDONLY,&handle);
	RS232C_INIT();
	sprintf(str,"Err %d Handle %d\n",err,handle);
	RS232C_PUTS(str);
	puts(str);
	return 0;
}
