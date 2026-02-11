#ifndef DOSCALL_H_IS_INCLUDED
#define DOSCALL_H_IS_INCLUDED

#include <dos.h>


#define DOS_ATTR_READONLY	0x01
#define DOS_ATTR_HIDDEN		0x02
#define DOS_ATTR_SYSTEM		0x04
#define DOS_ATTR_VOLLABEL	0x08
#define DOS_ATTR_DIRECTORY	0x10
#define DOS_ATTR_ARCHIVE	0x20

#define MAX_PATH 80

struct DOSFileList
{
	struct DOSFileList *next;
	struct _find_t found;
};

int DOSGETCWD(char buf[MAX_PATH],char driveLetter);

int DOSCHDIR(const char dir[]);

struct DOSFileList *DOSGetFiles(char path[]);
void DOSFreeFiles(struct DOSFileList *list);
int DOSCountFiles(struct DOSFileList *list);

#endif
