#pragma once
#include "fakeWindows.h"

#define FLAG_BYTES (32)
extern unsigned char gFlagNPC[FLAG_BYTES];

void InitFlags();
SDL_bool SetNPCFlag(unsigned long a);
SDL_bool CutNPCFlag(unsigned long a);
SDL_bool GetNPCFlag(unsigned long a);