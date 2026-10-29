#pragma once
#include "fakeWindows.h"
#if defined(__has_include)
  #if __has_include(<SDL_mixer.h>)
    #include <SDL_mixer.h>
  #elif __has_include(<SDL2/SDL_mixer.h>)
    #include <SDL2/SDL_mixer.h>
  #else
    #include <SDL_mixer.h>
  #endif
#else
  #include <SDL_mixer.h>
#endif

enum SOUND_ID
{
	SOUND_ID_NODMG = 0,
	SOUND_ID_DASH = 1,
	SOUND_ID_OUCH = 2,
	SOUND_ID_ITEM = 3,
	SOUND_ID_HITHEAD = 4,
	SOUND_ID_CRASH = 5,
	SOUND_ID_READY = 6,
	SOUND_ID_GO = 7,
	SOUND_ID_MESSAGE = 8,
	SOUND_ID_DEAD = 9,
	SOUND_ID_WIN = 10,
	SOUND_ID_LEVELUP = 11,
	SOUND_ID_LIFEUP = 12,
	SOUND_ID_SAVE = 13,
	SOUND_ID_YESNO = 14,
	SOUND_ID_QUAKE = 15,
	SOUND_ID_BOSSOUCH = 20,
};

enum SOUND_MODE
{
	SOUND_MODE_PLAY_LOOP = -1,
	SOUND_MODE_STOP = 0,
	SOUND_MODE_PLAY = 1
};

SDL_bool InitDirectSound(SDL_Window* hwnd);
void EndDirectSound();
void ReleaseSoundObject(int no);
SDL_bool InitSoundObject(const char* resname, int no);
void PlaySoundObject(int no, int mode);
void ChangeSoundFrequency(int no, unsigned rate);
void ChangeSoundVolume(int no, long volume);
void ChangeSoundPan(int no, long pan);
SDL_bool MakePiyoPiyoSoundObject(char *wave, unsigned char *envelope, int octave, int data_size, int no, unsigned note_mask = 0);
