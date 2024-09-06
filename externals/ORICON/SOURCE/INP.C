#include    <jctype.h>

#define	TRUE	1
#define	FALSE	0
#define	ERR	(-1)

#define BUF_MAX 512
#define BUF_MSK 511
#define LIN_MAX 128

char	*file_ext();

extern	int	fext_top;
extern	int	fext_pos;

static  int     his_pos=0;
static  int     his_top=0;
static  int     his_old=0;
static  int     his_blk=0;
static  char    *his_lin;
static  char    his_buf[BUF_MAX];
static  int     his_len[2]={0,0};
static  char    his_tmp[2][LIN_MAX];

/************************
int	iskanji(ch)
unsigned char ch;
{
    return ( (ch >= 0x81 && ch <= 0x9F) ||
	     (ch >= 0xE0 && ch <= 0xFC) ? TRUE:FALSE);
}
int	iskanji2(ch)
unsigned char ch;
{
    return ( (ch >= 0x40 && ch <= 0x7E) ||
	     (ch >= 0x80 && ch <= 0xFC) ? TRUE:FALSE);
}
**************************/
int     iskan(p)
char    *p;
{
    if ( iskanji(*p) && iskanji2(*(p+1)) )
        return TRUE;
    else
        return FALSE;
}
int     kan_pos(p,len)
register char *p;
int     len;
{
    int     i,n;

    for ( i = 0 ; *p != '\0' && i < len ; ) {
        n = (iskan(p) ? 2:1);
        if ( (i + n) > len )
            break;
        i += n;
        p += n;
    }
    return i;
}
static  void    his_set(str,len)
char    *str;
int     len;
{
    int     c;

    while ( len-- > 0 && *str != '\0' ) {
        his_buf[his_pos++] = *(str++);
        his_pos &= BUF_MSK;
        if ( his_pos == his_top ) {
            do {
                c = his_buf[his_top++];
                his_top &= BUF_MSK;
            } while ( his_top != his_pos && c != '\n' );
        }
    }
}
static  int     his_next(pos)
int     pos;
{
    int     c;

    while ( pos != his_pos ) {
        c = his_buf[pos++];
        pos &= BUF_MSK;
        if ( c == '\n' )
            break;
    }
    return pos;
}
static  int     his_back(pos)
int     pos;
{
    int     c;

    if ( pos == his_top )
        return pos;

    pos = (pos - 1) & BUF_MSK;
    while ( pos != his_top ) {
        pos = (pos - 1) & BUF_MSK;
        if ( his_buf[pos] == '\n' ) {
            pos = (pos + 1) & BUF_MSK;
            break;
        }
    }
    return pos;
}
static  int     his_get(str,pos,off,max)
char    *str;
int     pos;
int     off;
int     max;
{
    int     i,n;

    i = 0;
    while ( i < max && pos != his_pos ) {
        if ( his_buf[pos] == '\n' )
            break;
        else if ( iskanji(his_buf[pos]) &&
                  iskanji2(his_buf[(pos+1)&BUF_MSK]) )
            n = 2;
        else
            n = 1;

        if ( (i + n) > max )
            break;

        if ( i >= off ) {
            i += n;
            while ( n-- > 0 ) {
                *(str++) = his_buf[pos++];
                pos &= BUF_MSK;
            }
        } else {
            pos = (pos + n) & BUF_MSK;
            while ( n-- > 0 ) {
                if ( ++i > off )
                    *(str++) = ' ';
                else
                    str++;
            }
        }
    }
    return (i > off ? i:off);
}
static  int     his_cmp(pos,str,len)
int     pos;
char    *str;
int     len;
{
    while ( len > 0 && *str != '\0' ) {
        if ( pos == his_pos || his_buf[pos++] != *(str++) )
            return FALSE;
        pos &= BUF_MSK;
        len--;
    }
    return TRUE;
}
char    *input(max)
int	max;
{
    int     ch,och,n,i;
    int     pos,cps,len,his;
    char    *p,*s;

    ch = pos = cps = len = 0;
    his = his_pos;
    his_lin = his_tmp[his_blk];

    if ( max > LIN_MAX )
	max = LIN_MAX;

    if ( --max < 0 ) {
	his_lin[0] = '\n';
	len = 1;
	goto ENDOF;
    }

    for ( ; ; ) {

	och = ch;

        if ( (ch = GETCH()) == 0x1B )
	    ch = (ch << 8) | GETCH();

        switch(ch) {
        case 0x08:
            if ( pos > 0 ) {
                pos = kan_pos(his_lin,pos - 1);
                p = &(his_lin[pos]);
                n = (iskan(p) ? 2:1);
                memcpy(p,p + n,len - pos - n);
                len -= n;

                BAKSPC(n);
                PUTS(p,len - pos);
                REPCHR(' ',n);
                BAKSPC(len - pos + n);
            }
            break;

        case 0x1B56:
            if ( pos < len ) {
                p = &(his_lin[pos]);
                n = (iskan(p) ? 2:1);
                memcpy(p,p + n,len - pos - n);
                len -= n;

                PUTS(p,len - pos);
                REPCHR(' ',n);
                BAKSPC(len - pos + n);
            }
            break;

        case 0x1C:
            if ( pos < len ) {
                p = &(his_lin[pos]);
                n = (iskan(p) ? 2:1);
                pos += n;

                while ( n-- > 0 )
                    PUTC(*(p++));
            }
            break;

        case 0x1D:
            if ( pos > 0 ) {
                pos = kan_pos(his_lin,pos - 1);
                p = &(his_lin[pos]);
                n = (iskan(p) ? 2:1);

                BAKSPC(n);
            }
            break;

	case 0x1E:
	    for ( ; ; ) {
        	if ( his == his_top ) {
		    BEEP();
		    break;
		}

                his = his_back(his);
		cps = ((och == 0x1F || och == 0x1E) ? cps:pos);
                if ( !his_cmp(his,his_lin,cps) )
                    continue;

                BAKSPC(pos);
                REPCHR(' ',len);
                BAKSPC(len);

                pos = len = his_get(his_lin,his,0,max);

                PUTS(his_lin,len);
                BAKSPC(len - pos);
                break;
            }
            break;

        case 0x1F:
            for ( ; ; ) {
		if ( his == his_pos ) {
		    BEEP();
		    REPCHR(' ',len - pos);
		    BAKSPC(len - pos);
		    len = pos;
		    break;
		}

                his = his_next(his);
		cps = ((och == 0x1F || och == 0x1E) ? cps:pos);
                if ( !his_cmp(his,his_lin,cps) )
                    continue;

                BAKSPC(pos);
                REPCHR(' ',len);
                BAKSPC(len);

                pos = len = his_get(his_lin,his,0,max);

                PUTS(his_lin,len);
                BAKSPC(len - pos);
                break;
            }
            break;

	case 0x16:
        case 'R'-'@':
            if ( his != his_top ) {
                his = his_back(his);

                REPCHR(' ',len - pos);
                BAKSPC(len - pos);

                len = his_get(his_lin,his,pos,max);

                PUTS(&(his_lin[pos]),len - pos);
                BAKSPC(len - pos);
            } else
		BEEP();
            break;

	case 0x17:
        case 'E'-'@':
            if ( his != his_pos ) {
                his = his_next(his);

                REPCHR(' ',len - pos);
                BAKSPC(len - pos);

                len = his_get(his_lin,his,pos,max);

                PUTS(&(his_lin[pos]),len - pos);
                BAKSPC(len - pos);
            } else
		BEEP();
            break;

	case 0x1B55:
        case 'A'-'@':
            p = &(his_lin[len]);
            while ( len < max && *p != '\n' && *p != '\0' ) {
                len++;
                p++;
            }
            PUTS(&(his_lin[pos]),len - pos);
            pos = len;
            break;

        case 0x0D:
            his_lin[len++] = '\n';
            if ( len > 1 && !his_cmp(his_old,his_lin,len) ) {
		his_old = his_pos;
                his_set(his_lin,len);
	    }

            PUTS(&(his_lin[pos]),len - pos);
            goto ENDOF;

        case 'X'-'@':
            BAKSPC(pos);
            REPCHR(' ',len);
            BAKSPC(len);
            pos = len = 0;
            break;

	case 0x0B:
        case 'D'-'@':
            BAKSPC(pos);
            REPCHR(' ',len);
            BAKSPC(len);

            his_len[his_blk] = len;
            his_blk ^= 1;
            his_lin = his_tmp[his_blk];
            pos = len = his_len[his_blk];

            PUTS(his_lin,len);
            break;

        case 'U'-'@':
	    if ( och != ('U'-'@') )
		file_end();

	    s = file_ext(his_lin,pos);

	    if ( (n = fext_pos + strlen(s) - pos) > 0 ) {
		if ( pos < len ) {
                    p = &(his_lin[len + (n - 1)]);
                    for ( i = len - pos ; i > 0 ; i--,p-- )
                        *p = *(p - n);
                }
	    } else if ( n < 0 ) {
		if ( pos < len )
		    memcpy(&(his_lin[pos+n]),&(his_lin[pos]),len-pos);
	    }

	    p = &(his_lin[fext_pos]);
	    while ( *s != '\0' )
		*(p++) = *(s++);

            BAKSPC(pos - fext_pos);
            REPCHR(' ',len - fext_pos);
            BAKSPC(len - fext_pos);

	    len += n;
	    pos += n;

            PUTS(&(his_lin[fext_pos]),len - fext_pos);
            BAKSPC(len - pos);
	    break;

        default:
            if ( iskanji(ch) ) {
                n = GETCH();
                if ( !iskanji2(n) ) {
                    UNGETCH(n);
                    n = 1;
                } else {
                    ch = (ch << 8) | n;
                    n = 2;
                }
            } else
                n = 1;

            if ( (len + n ) >= max ) {
                BEEP();
                break;
            }

            if ( pos < len ) {
                p = &(his_lin[len + (n - 1)]);
                for ( i = len - pos ; i > 0 ; i--,p-- )
                    *p = *(p - n);
            }

            p = &(his_lin[pos]);
            if ( n == 1 ) {
                his_lin[pos++] = ch;
            } else {
                his_lin[pos++] = ch >> 8;
                his_lin[pos++] = ch;
            }
            len += n;

            PUTS(p,len - pos + n);
            BAKSPC(len - pos);
            break;
        }
        FLUSH();
    }

ENDOF:
    his_len[his_blk] = len;
    return his_lin;
}
