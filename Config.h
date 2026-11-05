#pragma once
#include "fakeWindows.h"

struct GameConfig
{
	SDL_bool autosave_on_exit;
};

extern GameConfig gConfig;

void LoadConfig(const char *filename = "config.ini");
void SaveConfig(const char *filename = "config.ini");
