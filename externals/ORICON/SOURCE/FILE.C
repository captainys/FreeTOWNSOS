#define	TRUE	1
#define	FALSE	0
#define	ERR	(-1)

#define BUFLEN  128

	int         fext_top=0;
	int         fext_pos=0;
	char far    *dta_bak;
	char 	    path_top[BUFLEN];
	struct {
	    unsigned char	dd_dmy;
	    unsigned char	dd_drv;
	    unsigned char	dd_path[8];
	    unsigned char	dd_extn[3];
	    unsigned char	dd_resv[8];
	    unsigned char	dd_att;
	    unsigned short	dd_time,dd_date;
	    unsigned long	dd_size;
	    char		dd_name[14];
	} dta;

static	int	fext_chk=FALSE;
static  char    exec_chk=FALSE;
static  char    path_chk=FALSE;
static  char	*path_pos;
static  char    *wild_pos;
static  char    wild_buf[BUFLEN];
static  char    path_buf[BUFLEN];

/******** Compile to HIS.ASM ***********

static	char    tolow(char ch)
{
    if ( ch >= 'A' && ch <= 'Z' )
        return (ch | 0x20);
    else
        return ch;
}
static	void    strlow(register char *str)
{
    while ( *str != '\0' ) {
        *str = tolow(*str);
        str++;
    }
}
static	void	DTA_init()
{
    union REGS regs;
    struct SREGS seg;
    char far *p;

    regs.h.ah = 0x2F;
    intdosx(&regs,&regs,&seg);
    FP_SEG(dta_bak) = seg.es;
    FP_OFF(dta_bak) = regs.x.bx;

    p = (char far *)&dta;
    regs.h.ah = 0x1a;
    seg.ds = FP_SEG(p);
    regs.x.dx = FP_OFF(p);
    intdosx(&regs,&regs,&seg);
}
static	void	DTA_end()
{
    union REGS regs;
    struct SREGS seg;

    regs.h.ah = 0x1a;
    seg.ds = FP_SEG(dta_bak);
    regs.x.dx = FP_OFF(dta_bak);
    intdosx(&regs,&regs,&seg);
}
static	int	farst_call(char *wild)
{
    union REGS regs;
    struct SREGS seg;
    char far *p;

    p = wild;
    regs.h.ah = 0x4e;
    regs.x.cx = 0x21;
    seg.ds = FP_SEG(p);
    regs.x.dx = FP_OFF(p);
    intdosx(&regs,&regs,&seg);
    return (regs.x.cflag == 0 ? TRUE:FALSE);
}
static	int	next_call(void)
{
    union REGS regs;

    regs.h.ah = 0x4f;
    intdos(&regs,&regs);
    return (regs.x.cflag == 0 ? TRUE:FALSE);
}
******** End of Compile ***********/

static	int     path_set(void)
{
    char    *p,*s;

    if ( *path_pos == '\0' )
        return FALSE;

    p = wild_buf;
    while ( *path_pos != '\0' && p < &(wild_buf[BUFLEN-1]) ) {
        if ( *path_pos == ';' ) {
            path_pos++;
            break;
        } else
            *(p++) = *(path_pos++);
    }

    if ( p > wild_buf && *(p-1) != '\\' && p < &(wild_buf[BUFLEN-1]) )
        *(p++) = '\\';

    s = path_buf;
    while ( *s != '\0' && p < &(wild_buf[BUFLEN-1]) )
        *(p++) = *(s++);
    *p = '\0';

    return TRUE;
}
static	int     file_cmp(void)
{
    char *p;

    p = dta.dd_name;
    while ( *p != '.' && *p != '\0' )
	p++;

    if ( exec_chk ) {
        if ( strcmp(p,".BAT") == 0 ||
             strcmp(p,".COM") == 0 ||
             strcmp(p,".EXE") == 0 )
            return TRUE;
        else
            return FALSE;
    } else
        return TRUE;
}
static	int     farst_dir(void)
{
    return farst_call(wild_buf);
}
static	int     next_dir(void)
{
    do {
        while ( !next_call() ) {
            do {
                if ( !path_chk   || !path_set() )
                    return FALSE;
            } while ( !farst_dir() );
            break;
        }
    } while ( !file_cmp() );

    return TRUE;
}
static	int     wild_set(char *str,int len)
{
    int     n;
    char    *p;

    n = len;
    while ( n > 0 ) {
        n--;
        if ( str[n] == ' ' || str[n] == '\t' ) {
            n++;
            break;
        }
    }

    exec_chk = path_chk = (n > 0 ? FALSE:TRUE);

    fext_top = fext_pos = n;
    p = wild_pos = wild_buf;
    while ( n < len && p < &(wild_buf[BUFLEN-1]) ) {
        if ( str[n] == ':' || str[n] == '\\' ) {
            fext_pos = n + 1;
            wild_pos = p + 1;
            path_chk = FALSE;
        }
        *(p++) = str[n++];
    }
    *p = '\0';

    n = 0;
    p = wild_pos;
    while ( *p != '\0' ) {
        if ( *p == '.' )
            n |= 2;
        else if ( *p == '*' || *p == '?' )
            n |= 1;
        p++;
    }

    switch(n) {
    case 0: strcpy(p,"*.*"); break;
    case 1: strcpy(p,".*"); break;
    case 2: case 3: exec_chk = FALSE; break;
    }

    if ( path_chk ) {
        strcpy(path_buf,wild_buf);
	wild_pos = path_buf + ((int)wild_pos - (int)wild_buf);
        path_pos = path_top;
    }

    if ( farst_dir() && file_cmp() )
        return TRUE;
    else
        return next_dir();
}

char	*file_ext(char *str,int pos)
{
    int    rc;
    char   *p;

    if ( fext_chk == FALSE ) {
        DTA_init();
        rc = wild_set(str,pos);
        fext_chk = TRUE;
    } else
        rc = next_dir();

    if ( rc == FALSE ) {
        DTA_end();
        fext_chk = FALSE;
	BEEP();
        return wild_pos;
    }

    strlow(dta.dd_name);

    if ( exec_chk ) {
	p = dta.dd_name;
	while ( *p != '.' && *p != '\0' )
	    p++;
	*p = '\0';
    }

    return dta.dd_name;
}
void	file_end(void)
{
    if ( fext_chk != FALSE ) {
	while ( next_call() );
        DTA_end();
        fext_chk = FALSE;
    }
}
/******** Compile to HIS.ASM ***********
static	int     envcmp(char far *p,char *s)
{
    while ( *s != '\0' ) {
       if ( tolow(*p) != *s )
            return FALSE;
	p++;
	s++; 
    }
    return TRUE;
}
void    Path_init(void)
{
    short far *s;

    FP_SEG(s) = _psp;
    FP_OFF(s) = 0x2C;

    FP_SEG(path_top) = *s;
    FP_OFF(path_top) = 0;

    while ( *path_top != '\0' ) {
	if ( envcmp(path_top,"path=") ) {
	    path_top += 5;
	    break;
	}
        while ( *(path_top++) != '\0' );
    }
}
******** End of Compile ***********/
