#pragma once
#include "Draw.h"
#include "PiyoPiyo.h"
#include "fakeWindows.h"

#define MAX_PSLINES ((SURFACE_HEIGHT + 24) / 24)

struct PIX_SCR
{
	char* data;
	unsigned p_read;
	unsigned size;
	int ypos_line[MAX_PSLINES];
	unsigned line;
	char scale;
	SDL_bool end;
};

SDL_bool ReadPixelScript(PIX_SCR *ptx, char* path);
void LoadPixelScript(PIX_SCR *ptx, char* path, char scale);
int PixelScriptProc(PIX_SCR *ptx, PIYOPIYO_CONTROL *piyocont, SDL_bool ending);
void EndPixelScript(PIX_SCR *ptx);
