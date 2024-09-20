#ifndef SNDINT_H_IS_INCLUDED
#define SNDINT_H_IS_INCLUDED

#define SNDINT_CALLBACK_ENABLED 1

struct SNDINT_Callback
{
	unsigned int flags;
	_Far void (*callback)(void);
	unsigned int DS,ES,FS,GS;
};

void SNDINT_Internal_Start_Mouse(void);
void SNDINT_Internal_Start_Sound_TimerB(void);
void SNDINT_Internal_Start_Sound_TimerA(void);
void SNDINT_Internal_Start_PCM(void);
void SNDINT_Internal_Stop_Mouse(void);
void SNDINT_Internal_Stop_Sound_TimerB(void);
void SNDINT_Internal_Stop_Sound_TimerA(void);
void SNDINT_Internal_Stop_PCM(void);

#endif
