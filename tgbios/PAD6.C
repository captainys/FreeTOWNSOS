// By courtesy of BCC (https://github.com/bcc2528/FMTOWNS_PAD6)

#define PAD1IN 0x4d0
#define PAD2IN 0x4d2
#define PADOUT 0x4d6

#define COM0 0xf
#define COM1 0x3f
#define COMIN 0x40

// 6 Buttons PAD
// Return: 0 = Press, 1 = Release
// 0bit: Up
// 1bit: Down
// 2bit: Left
// 3bit: Right
// 4bit: A
// 5bit: B
// 6bit: RUN
// 7bit: SELECT
// 8bit: Z
// 9bit: Y
// 10bit: X
// 11bit: C
unsigned int PAD6_in(int port)
{
	unsigned int status = 0xc0;

	if(port == 0) // PAD 1
	{
		// Change COM to 0 (Read D-Pad A, and B Buttons)
		do {
			_outb(PADOUT, COM0);
		} while ((_inb(PAD1IN) & COMIN) != 0);

		status |= (_inb(PAD1IN) & 0x3f);

		// Change COM to 1 (Read C, X, Y, and Z Buttons)
		do {
			_outb(PADOUT, COM1);
		} while ((_inb(PAD1IN) & COMIN) == 0);

		status |= ((_inb(PAD1IN) & 0xf) << 8);

		// SELECT
		if((status & 0x3) == 0)
		{
			status |= 0x3;
			status &= ~(0x80);
		}
		// RUN
		if((status & 0xc) == 0)
		{
			status |= 0xc;
			status &= ~(0x40);
		}
	}
	else if(port == 1) // PAD 2
	{
		// Change COM to 0 (Read D-Pad A, and B Buttons)
		do {
			_outb(PADOUT, COM0);
		} while ((_inb(PAD2IN) & COMIN) != 0);

		status |= (_inb(PAD2IN) & 0x3f);

		// Change COM to 1 (Read C, X, Y, and Z Buttons)
		do {
			_outb(PADOUT, COM1);
		} while ((_inb(PAD2IN) & COMIN) == 0);

		status |= ((_inb(PAD2IN) & 0xf) << 8);

		// SELECT
		if((status & 0x3) == 0)
		{
			status |= 0x3;
			status &= ~(0x80);
		}
		// RUN
		if((status & 0xc) == 0)
		{
			status |= 0xc;
			status &= ~(0x40);
		}
	}
	else
	{
		status = 0xffffffff;
	}

	return status;
}

void Vsync_wait(void)
{
	while(0==(_inb(0xfda0) & 1));
}

void Set_palette(unsigned char num, unsigned char r, unsigned g, unsigned b)
{
	_outb(0xfd90, num);
	_outb(0xfd94, r);
	_outb(0xfd96, g);
	_outb(0xfd92, b);
}