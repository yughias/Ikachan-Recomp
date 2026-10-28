#pragma once
#include "fakeWindows.h"

struct PIYOPIYO_CONTROL
{
	char mode; //x0
	char track; //x1
	char prev_track; //x2
	//alignment x3
	short volume; //x4
};

SDL_bool InitPiyoPiyo();
void EndPiyoPiyo();
void PiyoPiyoProc();
void PiyoPiyoControl(PIYOPIYO_CONTROL *piyocont);
