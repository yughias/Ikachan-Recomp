#include "Flags.h"

unsigned char gFlagNPC[FLAG_BYTES];

void InitFlags()
{
	for (int i = 0; i < FLAG_BYTES; i++)
		gFlagNPC[i] = 0;
}

SDL_bool SetNPCFlag(unsigned long a)
{
	if (a > (FLAG_BYTES * 8))
		return SDL_FALSE;
	gFlagNPC[a / 8] |= 1 << a % 8;
	return SDL_TRUE;
}

SDL_bool CutNPCFlag(unsigned long a)
{
	if (a > (FLAG_BYTES * 8))
		return SDL_TRUE;
	gFlagNPC[a / 8] &= ~(1 << a % 8);
	return SDL_FALSE;
}

SDL_bool GetNPCFlag(unsigned long a)
{
	if (a > (FLAG_BYTES * 8))
		return SDL_FALSE;
	if (gFlagNPC[a / 8] & (1 << a % 8))
		return SDL_TRUE;
	else
		return SDL_FALSE;
}
