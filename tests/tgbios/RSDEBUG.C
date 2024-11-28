#include "RSDEBUG.H"

void RS232C_INIT(void)
{
	int i;
	for(i=0; i<1000; ++i)// Just in case, if something is in the buffer.  Wait for at least 1ms.
	{
		_outb(0x6C,0);
	}

	//; RS232C=INT 2
	//IN		AL,0002H	; Primary-PIC Mask Register
	//OR		AL,4
	//OUT		0002H,AL
	_outb(0x02,_inb(0x02)|4);

	//; Timer #4 must be set to Async 1/16 for 19200bps
	//; Timer #4 mode must be 2 "Rate Generator"
	//; RS232C BIOS is using mode 3 "Rectangular Rate Generator"
	//MOV		AL,076H  ; Valule based on BIOS disassembly
	//OUT		056H,AL
	_outb(0x56,0x76);

	//; RS232C BIOS Disassembly showed it writes two bytes in 0052H in a sequence.
	//MOV		AL,02H	; 04H->19200bps  02H->38400bps
	//OUT		052H,AL
	//MOV		AL,00H
	//OUT		052H,AL
	_outb(0x52,2);
	_outb(0x52,0);

	// Write 0 for 3 times.
	//MOV		DX,0A02H
	//XOR		AL,AL
	//CALL	RS232C_UNIT_DELAY
	//OUT		DX,AL
	//CALL	RS232C_UNIT_DELAY
	//OUT		DX,AL
	//CALL	RS232C_UNIT_DELAY
	//OUT		DX,AL
	//CALL	RS232C_UNIT_DELAY
	_outb(0x6C,0);
	_outb(0x6C,0);
	_outb(0x6C,0);
	_outb(0xA02,0);
	_outb(0x6C,0);
	_outb(0x6C,0);
	_outb(0x6C,0);
	_outb(0xA02,0);
	_outb(0x6C,0);
	_outb(0x6C,0);
	_outb(0x6C,0);
	_outb(0xA02,0);

	//MOV		AL,040H		; Internal reset
	//OUT		DX,AL
	//CALL	RS232C_UNIT_DELAY
	_outb(0xA02,0x40);

	//; 04EH
	//; S2 S1 Ep PN L2 L1 B2 B1
	//;  0  1  0  0  1  1  1  0
	//; S2=0, S1=1 -> 1 stop bit
	//; PN=0       -> No parity
	//; L2=1, L1=1 -> 8 bit
	//; B2=1, B1=0 -> 1/16 scalar
	//MOV		AL,4EH
	//OUT		DX,AL
	//CALL	RS232C_UNIT_DELAY
	_outb(0xA02,0x4E);
	_outb(0x6C,0);
	_outb(0x6C,0);
	_outb(0x6C,0);

	//; 0B7H
	//; ON	Sync Char search (?), 
	//; OFF	Internal Reset,
	//; ON	RTS request
	//; ON	Clear Error Flags
	//; OFF	Break
	//; ON	RXE Receive Enable
	//; ON	DTR Treminal Ready
	//; ON	TXE Transmission Enable
	//MOV		AL,0B7H
	//OUT		DX,AL
	//CALL	RS232C_UNIT_DELAY
	_outb(0xA02,0xB7);
	_outb(0x6C,0);
	_outb(0x6C,0);
	_outb(0x6C,0);

	//MOV		DX,0A08H
	//MOV		AL,020H  ; DTR=1, Internal Clock for Rx and Tx
	//OUT		DX,AL
	_outb(0xA08,0x20);
	_outb(0x6C,0);
	_outb(0x6C,0);
	_outb(0x6C,0);


	//; Make sure it is ready to transmit
	//CALL	RS232C_WAIT_TX_READY
	RS232C_WAIT_TX_READY();
}

void RS232C_WAIT_TX_READY(void)
{
	// MOV		DX,0A02H
	// IN		AL,DX
	// AND		AL,03H		; Just exit if RxD.
	// JE		%%LOOP
	while(0==(_inb(0xA02)&3))
	{
	}
}

void RS232C_PUTC(char c)
{
	RS232C_WAIT_TX_READY();
	// XCHG	AH,AL
	// MOV		DX,0A00H
	// OUT		DX,AL
	_outb(0xA00,c);
}

void RS232C_PUTS(const char str[])
{
	int i;
	for(i=0; 0!=str[i]; ++i)
	{
		RS232C_PUTC(str[i]);
	}
}
