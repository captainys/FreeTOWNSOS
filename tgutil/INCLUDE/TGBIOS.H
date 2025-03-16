#ifndef TGBIOS_H_IS_INCLUDED
#define TGBIOS_H_IS_INCLUDED

#define SND_GAMEDEV_TYPE_NORMAL   0
#define SND_GAMEDEV_TYPE_6BTN     1
#define SND_GAMEDEV_TYPE_CPSF     2
#define SND_GAMEDEV_TYPE_CYBER    3

/* TGBIOS Expanded Function E0H: Globally Set Game-Device Type.
*/
extern void SND_SetGlobal_Gamedev_Type(int port,int devType);

#endif
