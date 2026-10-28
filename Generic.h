#pragma once
#include "fakeWindows.h"

void GetCompileDate(int *year, int *month, int *day);
SDL_bool GetCompileVersion(int *v1, int *v2, int *v3, int *v4);
SDL_bool OpenSoundVolume(SDL_Window* hWnd);
void LoadGenericData();
