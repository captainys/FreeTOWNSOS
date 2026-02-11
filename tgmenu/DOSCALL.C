#include <stdio.h>
#include <stdlib.h>
#include <dos.h>
#include "DOSCALL.H"

int DOSGETCWD(char buf[MAX_PATH],char driveLetter)
{
	union REGS regs;
	regs.h.ah=0x47; // DOS CWD
	regs.l.dl=(driveLetter&0x1F);
	regs.w.esi=(unsigned int)buf;
	_intdos(&regs,&regs);
	if(0==(regs.w.cflag&1))
	{
		return 0;
	}
	return regs.x.ax;
}

int DOSCHDIR(const char dir[])
{
	union REGS regs;
	regs.h.ah=0x3B; // DOS CHDIR
	regs.w.edx=(unsigned int)dir;
	_intdos(&regs,&regs);
	if(0==(regs.w.cflag&1))
	{
		return 0;
	}
	return regs.x.ax;
}

struct DOSFileList *DOSGetFiles(char path[])
{
	struct DOSFileList *fileList=NULL,*tail=NULL;
	struct _find_t findStruct;
	for(;;)
	{
		int err=0;
		if(NULL==tail)
		{
			err=_dos_findfirst(path,0x16,&findStruct);
		}
		else
		{
			err=_dos_findnext(&findStruct);
		}

		if(0==err)
		{
			struct DOSFileList *neo=(struct DOSFileList *)malloc(sizeof(struct DOSFileList));
			neo->next=NULL;
			neo->found=findStruct;
			if(NULL==tail)
			{
				fileList=neo;
				tail=neo;
			}
			else
			{
				tail->next=neo;
				tail=neo;
			}
		}
		else
		{
			break;
		}
	}
	return fileList;
}
void DOSFreeFiles(struct DOSFileList *list)
{
	while(NULL!=list)
	{
		struct DOSFileList *next=list->next;
		free(list);
		list=next;
	}
}
int DOSCountFiles(struct DOSFileList *list)
{
	int i=0;
	while(NULL!=list)
	{
		++i;
		list=list->next;
	}
	return i;
}
