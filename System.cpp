#include "System.h"
#include "Draw.h"
#include "Sound.h"
#include "Generic.h"
#include "PiyoPiyo.h"
#include "Game.h"
#include <stdio.h>
#include <string.h>
#include <unistd.h>

//System task
static SDL_bool gQuit = SDL_FALSE;
SDL_bool gHomePressed = SDL_FALSE;
static bool gBtnSelect = false;
static bool gBtnStart = false;

#if defined(__psp__) || defined(PSP)
#include <pspkernel.h>
PSP_HEAP_SIZE_KB(-1024);
#endif




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
	SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMECONTROLLER | SDL_INIT_JOYSTICK | SDL_INIT_AUDIO);
	if (SDL_NumJoysticks() > 0 && !SDL_IsGameController(0))
	{
		SDL_JoystickOpen(0);
	}
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

	// Set working directory to application directory
	if (argc > 0 && argv && argv[0])
	{
		char appDir[MAX_PATH];
		strncpy(appDir, argv[0], MAX_PATH);
		appDir[MAX_PATH - 1] = '\0';
		char* lastSlash = strrchr(appDir, '/');
		char* lastBackslash = strrchr(appDir, '\\');
		if (lastBackslash > lastSlash) lastSlash = lastBackslash;
		if (lastSlash)
		{
			*lastSlash = '\0';
			chdir(appDir);
		}
	}

	//Get module path
	char* tmp = SDL_GetBasePath();
	if (tmp)
	{
		strncpy(gModulePath, tmp, MAX_PATH);
		gModulePath[MAX_PATH - 1] = '\0';
		SDL_free(tmp);
		chdir(gModulePath);
	}
	else
	{
		gModulePath[0] = '\0';
	}

	gWndWidth = SURFACE_WIDTH;
	gWndHeight = SURFACE_HEIGHT;

	//Create window (full screen or windowed)
	SDL_Window* hWnd;
	//HMENU hMenu;

	SDL_SetHint(SDL_HINT_RENDER_VSYNC, "1");

	int winW = SURFACE_WIDTH;
	int winH = SURFACE_HEIGHT;
	Uint32 windowFlags = SDL_WINDOW_RESIZABLE;

#if defined(MIYOO) || defined(__MIYOO__)
	winW = 640;
	winH = 480;
	windowFlags = SDL_WINDOW_SHOWN;
	SDL_ShowCursor(SDL_DISABLE);
#endif

	sdlWindow = SDL_CreateWindow(lpCaption, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, winW, winH, windowFlags);
	if(!sdlWindow){
		printf("error in creating sdl window!\n");
		exit(EXIT_FAILURE);
	}

	SDL_SetWindowMinimumSize(sdlWindow, SURFACE_WIDTH, SURFACE_HEIGHT);

	SDL_Surface* icon = SDL_LoadBMP("Resource/icon.bmp");
	if (icon) {
		SDL_SetWindowIcon(sdlWindow, icon);
		SDL_FreeSurface(icon);
	}
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

	TTF_Quit();
	SDL_Quit();
	return 0;
}

void InactiveWindow()
{
	bActive = SDL_FALSE;
}

void ActiveWindow()
{
	bActive = SDL_TRUE;
}

//System task (state declared above)

#if !defined(_WIN32)
#include <signal.h>
static void SignalHandler(int)
{
	gHomePressed = SDL_TRUE;
	gQuit = SDL_TRUE;
}
#endif

SDL_bool SystemTask()
{
#if !defined(_WIN32)
	static bool signalsSet = false;
	if (!signalsSet)
	{
		signal(SIGTERM, SignalHandler);
		signal(SIGINT, SignalHandler);
		signalsSet = true;
	}
#endif
	if (gQuit)
		return SDL_FALSE;

	SDL_Event e;
	while(SDL_PollEvent(&e)){
		switch(e.type){
			case SDL_QUIT:
				gHomePressed = SDL_TRUE;
				gQuit = SDL_TRUE;
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
			case SDL_CONTROLLER_BUTTON_BACK: // Select
				gBtnSelect = true;
				if (gBtnStart)
				{
					gHomePressed = SDL_TRUE;
					gQuit = SDL_TRUE;
					return SDL_FALSE;
				}
				gKey |= KEY_S;
				break;
			case SDL_CONTROLLER_BUTTON_START: // Start
				gBtnStart = true;
				if (gBtnSelect)
				{
					gHomePressed = SDL_TRUE;
					gQuit = SDL_TRUE;
					return SDL_FALSE;
				}
				gKey |= KEY_S;
				break;
			case SDL_CONTROLLER_BUTTON_GUIDE: // Home / Guide button
				gHomePressed = SDL_TRUE;
				gQuit = SDL_TRUE;
				return SDL_FALSE;
			}
            break;

			case SDL_JOYDEVICEADDED:
				if (!SDL_IsGameController(e.jdevice.which))
					SDL_JoystickOpen(e.jdevice.which);
				break;

			case SDL_JOYBUTTONDOWN:
#if defined(__psp__) || defined(PSP)
				switch (e.jbutton.button) {
					case 0: // Triangle -> Inventory
						gKey |= KEY_S;
						break;
					case 1: // Circle -> Dash
						gKey |= KEY_X;
						break;
					case 2: // Cross -> Swim / Confirm
						gKey |= KEY_Z;
						break;
					case 3: // Square -> Dash / Alt
						gKey |= KEY_X;
						break;
					case 6: // D-pad Down
						gKey |= KEY_DOWN;
						break;
					case 7: // D-pad Left
						gKey |= KEY_LEFT;
						break;
					case 8: // D-pad Up
						gKey |= KEY_UP;
						break;
					case 9: // D-pad Right
						gKey |= KEY_RIGHT;
						break;
					case 10: // Select
						gBtnSelect = true;
						if (gBtnStart) { // SELECT + START exit combo
							gHomePressed = SDL_TRUE;
							gQuit = SDL_TRUE;
							return SDL_FALSE;
						}
						gKey |= KEY_S;
						break;
					case 11: // Start
						gBtnStart = true;
						if (gBtnSelect) { // SELECT + START exit combo
							gHomePressed = SDL_TRUE;
							gQuit = SDL_TRUE;
							return SDL_FALSE;
						}
						gKey |= KEY_S;
						break;
					case 12: // Home button
						gHomePressed = SDL_TRUE;
						gQuit = SDL_TRUE;
						return SDL_FALSE;
				}
#else
				switch(e.jbutton.button){
					case 0: // A
						gKey |= KEY_SPACE;
						break;
					case 1: // B
						gKey |= KEY_Z;
						break;
					case 2: // X
						gKey |= KEY_X;
						break;
					case 3: // Y
						gKey |= KEY_S;
						break;
					case 8: // Select
						gBtnSelect = true;
						if (gBtnStart) { // SELECT + START exit combo
							gHomePressed = SDL_TRUE;
							gQuit = SDL_TRUE;
							return SDL_FALSE;
						}
						gKey |= KEY_S;
						break;
					case 9: // Start
						gBtnStart = true;
						if (gBtnSelect) { // SELECT + START exit combo
							gHomePressed = SDL_TRUE;
							gQuit = SDL_TRUE;
							return SDL_FALSE;
						}
						gKey |= KEY_S;
						break;
					case 10: // Menu (Miyoo Mini physical Menu key)
						gHomePressed = SDL_TRUE;
						gQuit = SDL_TRUE;
						return SDL_FALSE;
				}
#endif
				break;

			case SDL_JOYBUTTONUP:
#if defined(__psp__) || defined(PSP)
				switch (e.jbutton.button) {
					case 0: // Triangle
						gKey &= ~KEY_S;
						break;
					case 1: // Circle
						gKey &= ~KEY_X;
						break;
					case 2: // Cross
						gKey &= ~KEY_Z;
						break;
					case 3: // Square
						gKey &= ~KEY_X;
						break;
					case 6: // Down
						gKey &= ~KEY_DOWN;
						break;
					case 7: // Left
						gKey &= ~KEY_LEFT;
						break;
					case 8: // Up
						gKey &= ~KEY_UP;
						break;
					case 9: // Right
						gKey &= ~KEY_RIGHT;
						break;
					case 10: // Select
						gBtnSelect = false;
						gKey &= ~KEY_S;
						break;
					case 11: // Start
						gBtnStart = false;
						gKey &= ~KEY_S;
						break;
				}
#else
				switch(e.jbutton.button){
					case 0:
						gKey &= ~KEY_SPACE;
						break;
					case 1:
						gKey &= ~KEY_Z;
						break;
					case 2:
						gKey &= ~KEY_X;
						break;
					case 3:
						gKey &= ~KEY_S;
						break;
					case 8: // Select
						gBtnSelect = false;
						gKey &= ~KEY_S;
						break;
					case 9: // Start
						gBtnStart = false;
						gKey &= ~KEY_S;
						break;
				}
#endif
				break;

			case SDL_JOYHATMOTION:
				if (e.jhat.value & SDL_HAT_UP)
					gKey |= KEY_UP;
				else
					gKey &= ~KEY_UP;
				if (e.jhat.value & SDL_HAT_DOWN)
					gKey |= KEY_DOWN;
				else
					gKey &= ~KEY_DOWN;
				if (e.jhat.value & SDL_HAT_LEFT)
					gKey |= KEY_LEFT;
				else
					gKey &= ~KEY_LEFT;
				if (e.jhat.value & SDL_HAT_RIGHT)
					gKey |= KEY_RIGHT;
				else
					gKey &= ~KEY_RIGHT;
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
			case SDL_CONTROLLER_BUTTON_BACK:
				gBtnSelect = false;
				gKey &= ~KEY_S;
				break;
			case SDL_CONTROLLER_BUTTON_START:
				gBtnStart = false;
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
				case SDLK_u:
					gKey |= KEY_SPACE;
					break;
				case SDLK_x:
				case SDLK_LSHIFT:
				case SDLK_o:
					gKey |= KEY_X;
					break;
				case SDLK_z:
				case SDLK_LCTRL:
				case SDLK_i:
					gKey |= KEY_Z;
					break;
				case SDLK_s:
				case SDLK_LALT:
				case SDLK_p:
					gKey |= KEY_S;
					break;
				case SDLK_RCTRL:
					gKey |= KEY_X;
					break;
				case SDLK_ESCAPE:
				case SDLK_HOME:
					gHomePressed = SDL_TRUE;
					gQuit = SDL_TRUE;
					return SDL_FALSE;
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
				case SDLK_u:
					gKey &= ~KEY_SPACE;
					break;
				case SDLK_x:
				case SDLK_LSHIFT:
				case SDLK_o:
					gKey &= ~KEY_X;
					break;
				case SDLK_z:
				case SDLK_LCTRL:
				case SDLK_i:
					gKey &= ~KEY_Z;
					break;
				case SDLK_s:
				case SDLK_LALT:
				case SDLK_p:
					gKey &= ~KEY_S;
					break;
				case SDLK_RCTRL:
					gKey &= ~KEY_X;
					break;
			}
			break;
		}
	}

	return SDL_TRUE;
}
