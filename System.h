#pragma once
#include "fakeWindows.h"

//Keys
#define MOUSE_RIGHT (0x00000001)
#define MOUSE_LEFT  (0x00000002)

#define KEY_LEFT  (0x00000001)
#define KEY_RIGHT (0x00000002)
#define KEY_UP    (0x00000004)
#define KEY_DOWN  (0x00000008)
#define KEY_SPACE (0x00000010)
#define KEY_X     (0x00000020)
#define KEY_Z     (0x00000040)
#define KEY_S     (0x00000080)

extern unsigned gKey, gKeyTrg;
extern unsigned gMouse, gMouseTrg, gMouseTrg2;
extern SDL_Window* sdlWindow;

//Window size
enum WND_SIZE
{
	WS_FULLSCREEN,
	WS_320x240,
	WS_640x480,
};
extern WND_SIZE gWndSize;

//Windows objects
extern SDL_Window* ghWnd;

//Filepath
extern char gModulePath[MAX_PATH];

int Random(int min, int max);
SDL_bool SystemTask();
