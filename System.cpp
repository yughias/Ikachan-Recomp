#include "System.h"
#include "Draw.h"
#include "Sound.h"
#include "Generic.h"
#include "PiyoPiyo.h"
#include "Game.h"
#include <stdio.h>
#include <string.h>



SDL_Window* sdlWindow;

//Window process
SDL_bool bActive;

//Keys
unsigned gKey;
unsigned gMouse;

//Window name
const char* lpCaption = "Ikachan";

//Window size
WND_SIZE gWndSize;
int gWndWidth, gWndHeight;

//Filepath
char gModulePath[MAX_PATH];

int Random(int min, int max)
{
	const int range = max - min + 1;
	return (rand() % range) + min;
}

int main(int argc, char** argv)
{
	SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMECONTROLLER | SDL_INIT_AUDIO);
	TTF_Init();

	RECT unused_rect = { 0, 0, SURFACE_WIDTH, SURFACE_HEIGHT };

	//Create Ikachan mutex (prevent program from running multiple instances)
	/*
	HANDLE hObject, hMutex;
	if ((hObject = OpenMutex(MUTEX_ALL_ACCESS, SDL_FALSE, lpMutexName)) != NULL)
	{
		CloseHandle(hObject);
		return 1;
	}
	hMutex = CreateMutex(0, SDL_TRUE, lpMutexName);
	*/

	//Get module path
	size_t i;
	char* tmp = SDL_GetBasePath();
	strncpy(gModulePath, tmp, MAX_PATH);
	SDL_free(tmp);
	for (i = strlen(gModulePath); gModulePath[i] != '\\' && i; i--);
	gModulePath[i] = 0;

	gWndWidth = SURFACE_WIDTH;
	gWndHeight = SURFACE_HEIGHT;

	//Create window (full screen or windowed)
	SDL_Window* hWnd;
	//HMENU hMenu;

	SDL_SetHint(SDL_HINT_RENDER_VSYNC, "1");

#if defined(PSP) || defined(__psp__) || defined(__PSP__)
	Uint32 windowFlags = 0;
#else
	Uint32 windowFlags = SDL_WINDOW_RESIZABLE;
#endif

	sdlWindow = SDL_CreateWindow(lpCaption, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, SURFACE_WIDTH, SURFACE_HEIGHT, windowFlags);
	if(!sdlWindow){
		printf("error in creating sdl window!\n");
		exit(EXIT_FAILURE);
	}

#if !defined(PSP) && !defined(__psp__) && !defined(__PSP__)
	SDL_SetWindowMinimumSize(sdlWindow, SURFACE_WIDTH, SURFACE_HEIGHT);
#endif
	/*
	SDL_SysWMinfo wmInfo;
	SDL_VERSION(&wmInfo.version);
	SDL_GetWindowWMInfo(sdlWindow, &wmInfo);
	hWnd = wmInfo.info.win.window;

	ghWnd = hWnd;
	SetWindowLongPtr(hWnd, GWLP_WNDPROC, (unsigned long_PTR)WndProc);
	*/

	StartDirectDraw(hWnd, WS_320x240);

	//Set rects
	RECT rcLoading = { 0, 0, 64, 8 };
	RECT rcFull = { 0, 0, 0, 0 };
	rcFull.right = SURFACE_WIDTH;
	rcFull.bottom = SURFACE_HEIGHT;

	//Load the "LOADING" text
	SDL_bool b = MakeSurface_File("Pbm/Loading2.pbm", SURFACE_ID_LOADING2);

	//Draw loading screen
	CortBox(&rcFull, 0x000000);
	PutBitmap3(&rcFull, (SURFACE_WIDTH / 2) - 32, (SURFACE_HEIGHT / 2) - 4, &rcLoading, SURFACE_ID_LOADING2);
	if (!Flip_SystemTask(hWnd))
	{
		//Release Ikachan mutex
		printf("loading screen loading error!\n");
		return 1;
	}

	SDL_ShowWindow(sdlWindow);
	bActive = SDL_TRUE;
	
	//Initialize DirectSound
	if (InitDirectSound(sdlWindow))
	{
		//Initialize game
		InitPiyoPiyo();

		//Run game loop
		Game(NULL);
		
		//End game
		EndPiyoPiyo();
		EndTextObject();

		//End DirectDraw and DirectSound
		EndDirectSound();
		EndDirectDraw(hWnd);

		//Release Ikachan mutex (twice?)
		//ReleaseMutex(hMutex);
	}

	//Release Ikachan mutex
	//ReleaseMutex(hMutex);
	return 1;
}

void InactiveWindow()
{
	bActive = SDL_FALSE;
}

void ActiveWindow()
{
	bActive = SDL_TRUE;
}

//System task
SDL_bool SystemTask()
{
	SDL_Event e;
	while(SDL_PollEvent(&e)){
		switch(e.type){
			case SDL_QUIT:
			return SDL_FALSE;

			case SDL_CONTROLLERDEVICEADDED:
				SDL_GameControllerOpen(e.cdevice.which);
			break;

			case SDL_CONTROLLERBUTTONDOWN:
			switch(e.cbutton.button){
				case SDL_CONTROLLER_BUTTON_DPAD_LEFT:
				gKey |= KEY_LEFT;
				break;
			case SDL_CONTROLLER_BUTTON_DPAD_RIGHT:
				gKey |= KEY_RIGHT;
				break;
			case SDL_CONTROLLER_BUTTON_DPAD_UP:
				gKey |= KEY_UP;
				break;
			case SDL_CONTROLLER_BUTTON_DPAD_DOWN:
				gKey |= KEY_DOWN;
				break;
			case SDL_CONTROLLER_BUTTON_X:
				gKey |= KEY_SPACE;
				break;
			case SDL_CONTROLLER_BUTTON_A:
				gKey |= KEY_X;
				break;
			case SDL_CONTROLLER_BUTTON_B:
				gKey |= KEY_Z;
				break;
			case SDL_CONTROLLER_BUTTON_Y:
				gKey |= KEY_S;
				break;
			}
            break;

			case SDL_CONTROLLERBUTTONUP:
			switch(e.cbutton.button){
				case SDL_CONTROLLER_BUTTON_DPAD_LEFT:
				gKey &= ~KEY_LEFT;
				break;
			case SDL_CONTROLLER_BUTTON_DPAD_RIGHT:
				gKey &= ~KEY_RIGHT;
				break;
			case SDL_CONTROLLER_BUTTON_DPAD_UP:
				gKey &= ~KEY_UP;
				break;
			case SDL_CONTROLLER_BUTTON_DPAD_DOWN:
				gKey &= ~KEY_DOWN;
				break;
			case SDL_CONTROLLER_BUTTON_X:
				gKey &= ~KEY_SPACE;
				break;
			case SDL_CONTROLLER_BUTTON_A:
				gKey &= ~KEY_X;
				break;
			case SDL_CONTROLLER_BUTTON_B:
				gKey &= ~KEY_Z;
				break;
			case SDL_CONTROLLER_BUTTON_Y:
				gKey &= ~KEY_S;
				break;
			}
            break;
			
			case SDL_KEYDOWN:
			if (((e.key.keysym.sym == SDLK_RETURN || e.key.keysym.sym == SDLK_KP_ENTER) && (e.key.keysym.mod & KMOD_ALT)) || e.key.keysym.sym == SDLK_F11)
			{
				Uint32 flags = SDL_GetWindowFlags(sdlWindow);
				if (flags & SDL_WINDOW_FULLSCREEN_DESKTOP)
				{
					SDL_SetWindowFullscreen(sdlWindow, 0);
				}
				else
				{
					SDL_SetWindowFullscreen(sdlWindow, SDL_WINDOW_FULLSCREEN_DESKTOP);
				}
				break;
			}
			switch(e.key.keysym.sym){
				case SDLK_LEFT:
					gKey |= KEY_LEFT;
					break;
				case SDLK_RIGHT:
					gKey |= KEY_RIGHT;
					break;
				case SDLK_UP:
					gKey |= KEY_UP;
					break;
				case SDLK_DOWN:
					gKey |= KEY_DOWN;
					break;
				case SDLK_SPACE:
					gKey |= KEY_SPACE;
					break;
				case SDLK_x:
					gKey |= KEY_X;
					break;
				case SDLK_z:
					gKey |= KEY_Z;
					break;
				case SDLK_s:
					gKey |= KEY_S;
					break;
			}
			break;

			case SDL_KEYUP:
			switch(e.key.keysym.sym){
				case SDLK_LEFT:
					gKey &= ~KEY_LEFT;
					break;
				case SDLK_RIGHT:
					gKey &= ~KEY_RIGHT;
					break;
				case SDLK_UP:
					gKey &= ~KEY_UP;
					break;
				case SDLK_DOWN:
					gKey &= ~KEY_DOWN;
					break;
				case SDLK_SPACE:
					gKey &= ~KEY_SPACE;
					break;
				case SDLK_x:
					gKey &= ~KEY_X;
					break;
				case SDLK_z:
					gKey &= ~KEY_Z;
					break;
				case SDLK_s:
					gKey &= ~KEY_S;
					break;
			}
			break;
		}
	}

	return SDL_TRUE;
}
