// By courtesy of BCC (https://github.com/bcc2528/FMTOWNS_PAD6)

#define PAD6_UP 0x1
#define PAD6_DOWN 0x2
#define PAD6_LEFT 0x4
#define PAD6_RIGHT 0x8
#define PAD6_A 0x10
#define PAD6_B 0x20
#define PAD6_RUN 0x40
#define PAD6_SELECT 0x80
#define PAD6_Z 0x100
#define PAD6_Y 0x200
#define PAD6_X 0x400
#define PAD6_C 0x800

unsigned int PAD6_in(int port);
void Vsync_wait(void);
void Set_palette(unsigned char num, unsigned char r, unsigned g, unsigned b);
