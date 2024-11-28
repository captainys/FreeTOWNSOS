#ifndef RSDEBUG_H_IS_INCLUDED
#define RSDEBUG_H_IS_INCLUDED

void RS232C_INIT(void);
void RS232C_WAIT_TX_READY(void);
void RS232C_PUTC(char c);
void RS232C_PUTS(const char str[]);

#endif
