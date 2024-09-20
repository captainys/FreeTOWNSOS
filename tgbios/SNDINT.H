#ifndef SNDINT_H_IS_INCLUDED
#define SNDINT_H_IS_INCLUDED

#define SNDINT_CALLBACK_ENABLED 1

struct SNDINT_Callback
{
	unsigned int flags;
	_Far void (*callback)(void);
	unsigned int DS,ES,FS,GS;
};

#endif
