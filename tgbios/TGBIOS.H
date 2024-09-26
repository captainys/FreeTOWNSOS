#ifndef TGBIOS_H_IS_INCLUDED
#define TGBIOS_H_IS_INCLUDED

#ifndef NULL
#define NULL 0
#endif

#define SEG_TGBIOS_CODE 0x110
#define SEG_TGBIOS_DATA 0x118

#define SEG_VRAM_2PG 0x120
#define SEG_VRAM_1PG 0x128
#define VRAM_SIZE (512*1024)

#define SEG_PATTERN_RAM 0x130

#define SEG_KANJI_FONT_ROM 0x138

#define SEG_WAVE_RAM 0x140

#define SEG_OS_RAM 0x148

#define SEG_SYSROM_CODE 0x100
#define SEG_SYSROM_DATA 0x100

// Apparently it is stored in 118:0098H
#define SND_BIOS_WORK  0x98

#define SUCCESS 1
#define FAIL    0


#define SND_NUM_PCM_CHANNELS 8
#define SND_NUM_FM_CHANNELS 6


#define ANK8_FONT_ADDR_BASE            0x3D000 // Error in [2] pp.12 Figue I-1-6?  Offset 1E800 is •Þ not A
#define ANK16_FONT_ADDR_BASE           0x3D800 // Error in [2] pp.12 Figure I-1-6?  Offset 1F000 is –Þ not wide-A


#endif
