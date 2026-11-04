#include "Draw.h"
#include "PiyoPiyo.h"
#include <string.h>
#include <stdio.h>
#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif

//Global cliprect
RECT grcFull = { 0, 0, SURFACE_WIDTH, SURFACE_HEIGHT };

//Draw state
int mag = 1;
SDL_bool fullscreen;

//DirectDraw objects
//LPDIRECTDRAW lpDD;
//LPDIRECTDRAWSURFACE frontbuffer;
//LPDIRECTDRAWSURFACE backbuffer;
//LPDIRECTDRAWCLIPPER clipper;

SDL_Surface* sdlMainSurface;

//Surfaces
#define MAX_SURFACE 512
SDL_Surface* surf[MAX_SURFACE] = {NULL};

#if defined(MIYOO) || defined(__MIYOO__)
static SDL_Renderer* gRenderer = NULL;
static SDL_Texture* gTexture = NULL;
#endif

//Loaded font
TTF_Font* font;

//Window rect and size
RECT backbuffer_rect;
int scaled_window_width;
int scaled_window_height;

//Client
int client_x;
int client_y;

void SetClientOffset(int width, int height)
{
	client_x = width;
	client_y = height;
}

//End of frame function
SDL_bool Flip_SystemTask(SDL_Window* hWnd)
{
	//Run system tasks while waiting for next frame
	static unsigned timePrev;
	if (!SystemTask())
		return SDL_FALSE;
	while (SDL_GetTicks() < (timePrev + 20))
	{
		PiyoPiyoProc();
		#ifdef __EMSCRIPTEN__
		emscripten_sleep(1);
		#else
		SDL_Delay(1);
		#endif
		if (!SystemTask())
			return SDL_FALSE;
	}

	if ((timePrev + 100) < SDL_GetTicks())
		timePrev = SDL_GetTicks();
	else
		timePrev += 20;

#if defined(MIYOO) || defined(__MIYOO__)
	if (gRenderer && gTexture && sdlMainSurface)
	{
		SDL_UpdateTexture(gTexture, NULL, sdlMainSurface->pixels, sdlMainSurface->pitch);
		SDL_RenderClear(gRenderer);
		SDL_RenderCopy(gRenderer, gTexture, NULL, NULL);
		SDL_RenderPresent(gRenderer);
	}
#else
	SDL_Surface* winSurf = SDL_GetWindowSurface(sdlWindow);
	if (winSurf && sdlMainSurface)
	{
		if (winSurf->w == SURFACE_WIDTH && winSurf->h == SURFACE_HEIGHT)
		{
			SDL_BlitSurface(sdlMainSurface, NULL, winSurf, NULL);
		}
		else
		{
			SDL_BlitScaled(sdlMainSurface, NULL, winSurf, NULL);
		}
	}

	SDL_UpdateWindowSurface(sdlWindow);
#endif

	return SDL_TRUE;
}

//DirectDraw interface
SDL_bool StartDirectDraw(SDL_Window* hWnd, int wndSize)
{
	//Create DirectDraw instance
	//if (DirectDrawCreate(NULL, &lpDD, NULL) != DD_OK)
	//	return SDL_FALSE;

	//Set cooperative level
	switch (wndSize)
	{
		case WS_FULLSCREEN:
			mag = 2;
			fullscreen = SDL_TRUE;
			// /lpDD->SetCooperativeLevel(hWnd, DDSCL_FULLSCREEN | DDSCL_EXCLUSIVE);
			// /lpDD->SetDisplayMode(SURFACE_WIDTH * mag, SURFACE_HEIGHT * mag, 16);
			break;
		case WS_320x240:
			mag = 1;
			//fullscreen = SDL_FALSE;
			//lpDD->SetCooperativeLevel(hWnd, DDSCL_NORMAL);
			break;
		case WS_640x480:
			mag = 2;
			//fullscreen = SDL_FALSE;
			//lpDD->SetCooperativeLevel(hWnd, DDSCL_NORMAL);
			break;
	}

	//Initialize window rects and sizes
	backbuffer_rect.left = 0;
	backbuffer_rect.top = 0;
	backbuffer_rect.right = SURFACE_WIDTH * mag;
	backbuffer_rect.bottom = SURFACE_HEIGHT * mag;

	scaled_window_width = SURFACE_WIDTH * mag;
	scaled_window_height = SURFACE_HEIGHT * mag;

	//Create screen buffers
	//DDSURFACEDESC ddsd;

	//Frontbuffer
	/*
	memset(&ddsd, 0, sizeof(DDSURFACEDESC));
	ddsd.dwSize = sizeof(DDSURFACEDESC);
	ddsd.dwFlags = DDSD_CAPS;
	ddsd.ddsCaps.dwCaps = DDSCAPS_PRIMARYSURFACE;
	ddsd.dwBackBufferCount = 0;

	if (lpDD->CreateSurface(&ddsd, &frontbuffer, NULL) != DD_OK)
		return SDL_FALSE;

	//Backbuffer
	memset(&ddsd, 0, sizeof(DDSURFACEDESC));
	ddsd.dwSize = sizeof(DDSURFACEDESC);
	ddsd.dwFlags = DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH;
	ddsd.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN;
	ddsd.dwWidth = SURFACE_WIDTH * mag;
	ddsd.dwHeight = SURFACE_HEIGHT * mag;

	if (lpDD->CreateSurface(&ddsd, &backbuffer, NULL) != DD_OK)
		return SDL_FALSE;

	//Create clipper
	lpDD->CreateClipper(0, &clipper, NULL);
	clipper->SetHWnd(0, hWnd);
	frontbuffer->SetClipper(clipper);
	*/

#if defined(MIYOO) || defined(__MIYOO__)
	gRenderer = SDL_CreateRenderer(sdlWindow, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
	if (!gRenderer)
		gRenderer = SDL_CreateRenderer(sdlWindow, -1, 0);
	if (gRenderer)
	{
		gTexture = SDL_CreateTexture(gRenderer, SDL_PIXELFORMAT_RGB565, SDL_TEXTUREACCESS_STREAMING, SURFACE_WIDTH, SURFACE_HEIGHT);
		if (gTexture)
			SDL_SetTextureBlendMode(gTexture, SDL_BLENDMODE_NONE);
	}
	sdlMainSurface = SDL_CreateRGBSurfaceWithFormat(0, SURFACE_WIDTH, SURFACE_HEIGHT, 16, SDL_PIXELFORMAT_RGB565);
#else
	SDL_Surface* winSurf = SDL_GetWindowSurface(sdlWindow);
	sdlMainSurface = SDL_CreateRGBSurfaceWithFormat(0, SURFACE_WIDTH, SURFACE_HEIGHT, 32, winSurf ? winSurf->format->format : SDL_PIXELFORMAT_ABGR8888);
#endif

	return SDL_TRUE;
}

void EndDirectDraw(SDL_Window* hWnd)
{
#if defined(MIYOO) || defined(__MIYOO__)
	if (gTexture) { SDL_DestroyTexture(gTexture); gTexture = NULL; }
	if (gRenderer) { SDL_DestroyRenderer(gRenderer); gRenderer = NULL; }
#endif
	if (sdlMainSurface)
	{
		SDL_FreeSurface(sdlMainSurface);
		sdlMainSurface = NULL;
	}
	//Release surfaces
	for (int i = 0; i < MAX_SURFACE; i++)
	{
		if (surf[i] != NULL)
		{
			SDL_FreeSurface(surf[i]);
			surf[i] = NULL;
		}
	}

	//Release frontbuffer
	/*
	if (frontbuffer != NULL)
		frontbuffer->Release();

	//Exit fullscreen
	if (fullscreen)
		lpDD->SetCooperativeLevel(hWnd, DDSCL_NORMAL);

	//Release DirectDraw instance
	if (lpDD != NULL)
		lpDD->Release();
	*/
}

//Surface creation
SDL_bool MakeSurface_File(const char name[], int surf_no) //TODO: ASM accuracy
{
	//Get path
	char path[MAX_PATH];
	sprintf(path, "%s", name);
	
	//Make sure a surface can be made here
	if (surf_no >= MAX_SURFACE || surf_no < 0)
		return SDL_FALSE;
	if (surf[surf_no] != NULL)
		return SDL_FALSE;
	
	//Load image
	SDL_Surface* loaded = SDL_LoadBMP(name);
	if (!loaded) {
		printf("error in loading %s\n", path);
		return SDL_FALSE;
	}

	surf[surf_no] = loaded;
	SDL_SetColorKey(surf[surf_no], SDL_TRUE, SDL_MapRGB(surf[surf_no]->format, 0, 0, 0));

	printf("loaded %s in %d\n", path, surf_no);

	/*
	HANDLE handle = LoadImage(GetModuleHandle(NULL), path, IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE | LR_CREATEDIBSECTION);
	if (handle == NULL)
		return SDL_FALSE;

	BITMAP bitmap;
	GetObjectA(handle, sizeof(BITMAP), &bitmap);

	//Get surface descriptor
	DDSURFACEDESC ddsd;
	memset(&ddsd, 0, sizeof(DDSURFACEDESC));
	ddsd.dwSize = sizeof(DDSURFACEDESC);
	ddsd.dwFlags = DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH;
	ddsd.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN;
	ddsd.dwWidth = bitmap.bmWidth * mag;
	ddsd.dwHeight = bitmap.bmHeight * mag;
	lpDD->CreateSurface(&ddsd, &surf[surf_no], NULL);

	//Scale image
	int src_x = 0;
	int src_y = 0;
	int src_w = bitmap.bmWidth;
	int src_h = bitmap.bmHeight;

	int dst_x = 0;
	int dst_y = 0;
	int dst_w = bitmap.bmWidth * mag;
	int dst_h = bitmap.bmHeight * mag;

	HDC hdc = CreateCompatibleDC(NULL);
	HGDIOBJ hgdiobj = SelectObject(hdc, handle);

	HDC hdc2;
	surf[surf_no]->GetDC(&hdc2);
	StretchBlt(hdc2, dst_x, dst_y, dst_w, dst_h, hdc, src_x, src_y, src_w, src_h, SRCCOPY);
	surf[surf_no]->ReleaseDC(hdc2);

	SelectObject(hdc, hgdiobj);
	DeleteDC(hdc);

	//Set surface colour key
	DDCOLORKEY ddcolorkey;
	ddcolorkey.dwColorSpaceLowValue = 0;
	ddcolorkey.dwColorSpaceHighValue = 0;
	surf[surf_no]->SetColorKey(DDCKEY_SRCBLT, &ddcolorkey);

	//Release image handle
	DeleteObject(handle);
	*/
	return SDL_TRUE;
}

SDL_bool MakeSurface_Generic(int bxsize, int bysize, int surf_no)
{
	//Make sure a surface can be made here
	if (surf_no >= MAX_SURFACE || surf_no < 0)
		return SDL_FALSE;
	if (surf[surf_no] != NULL)
		return SDL_FALSE;

	//Create surface
	if (sdlMainSurface && sdlMainSurface->format)
		surf[surf_no] = SDL_CreateRGBSurfaceWithFormat(0, bxsize, bysize, sdlMainSurface->format->BitsPerPixel, sdlMainSurface->format->format);
	else
		surf[surf_no] = SDL_CreateRGBSurface(0, bxsize, bysize, 32, 0xFF0000, 0xFF00, 0xFF, 0xFF000000);

	if(surf[surf_no])
		SDL_SetColorKey(surf[surf_no], SDL_TRUE, SDL_MapRGB(surf[surf_no]->format, 0, 0, 0));
	/*
	DDSURFACEDESC ddsd;
	memset(&ddsd, 0, sizeof(DDSURFACEDESC));
	ddsd.dwSize = sizeof(DDSURFACEDESC);
	ddsd.dwFlags = DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH;
	ddsd.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN;
	ddsd.dwWidth = bxsize * mag;
	ddsd.dwHeight = bysize * mag;
	lpDD->CreateSurface(&ddsd, &surf[surf_no], NULL);

	//Set surface colour key
	DDCOLORKEY ddcolorkey;
	ddcolorkey.dwColorSpaceLowValue = 0;
	ddcolorkey.dwColorSpaceHighValue = 0;
	surf[surf_no]->SetColorKey(DDCKEY_SRCBLT, &ddcolorkey);
	*/
	return SDL_TRUE;
}

//Backbuffer backup
void BackupSurface(int surf_no, const RECT *rect)
{	
	//Magnify rect
	RECT rcSet;
	rcSet.left = rect->left * mag;
	rcSet.top = rect->top * mag;
	rcSet.right = rect->right * mag;
	rcSet.bottom = rect->bottom * mag;

	SDL_Rect srcRect = {rcSet.left, rcSet.top, rcSet.right - rcSet.left, rcSet.bottom - rcSet.top};
	SDL_BlitSurface(sdlMainSurface, &srcRect, surf[surf_no], &srcRect);

	//Blit backbuffer to destination surface
	//surf[surf_no]->Blt(&rcSet, backbuffer, &rcSet, DDBLT_WAIT, &ddbltfx);
}

//Bitmap
void PutBitmap3(const RECT *rcView, int x, int y, const RECT *rect, int surf_no)
{
	if (surf_no < 0 || surf_no >= MAX_SURFACE || !surf[surf_no])
		return;

	//Clip source rect
	RECT rcWork = *rect;
	if (x + rect->right - rect->left > rcView->right)
		rcWork.right -= (x + rect->right - rect->left) - rcView->right;
	if (x < rcView->left)
	{
		rcWork.left += rcView->left - x;
		x = rcView->left;
	}
	if (y + rect->bottom - rect->top > rcView->bottom)
		rcWork.bottom -= (y + rect->bottom - rect->top) - rcView->bottom;
	if (y < rcView->top)
	{
		rcWork.top += rcView->top - y;
		y = rcView->top;
	}

	//Get destination rect
	RECT rcSet;
	rcSet.left = x;
	rcSet.top = y;
	rcSet.right = x + rcWork.right - rcWork.left;
	rcSet.bottom = y + rcWork.bottom - rcWork.top;

	//Multiply rects
	rcWork.left *= mag;
	rcWork.top *= mag;
	rcWork.right *= mag;
	rcWork.bottom *= mag;

	rcSet.left *= mag;
	rcSet.top *= mag;
	rcSet.right *= mag;
	rcSet.bottom *= mag;

	//Blit surface
	SDL_Rect srcRect = {rcWork.left, rcWork.top, rcWork.right - rcWork.left, rcWork.bottom - rcWork.top};
	SDL_Rect destRect = {rcSet.left, rcSet.top, 0, 0};		

	if (srcRect.w > 0 && srcRect.h > 0)
	{
		SDL_BlitSurface(surf[surf_no], &srcRect, sdlMainSurface, &destRect);
	}
}

//Cort box
void CortBox(const RECT *rect, unsigned col)
{
	if (!sdlMainSurface || !sdlMainSurface->format)
		return;
	Uint8 r = (Uint8)(col & 0xFF);
	Uint8 g = (Uint8)((col >> 8) & 0xFF);
	Uint8 b = (Uint8)((col >> 16) & 0xFF);
	Uint32 pixel = SDL_MapRGB(sdlMainSurface->format, r, g, b);
	SDL_FillRect(sdlMainSurface, NULL, pixel);
}

void CortBox2(const RECT *rect, unsigned col, int surf_no)
{
	if (surf_no < 0 || surf_no >= MAX_SURFACE || !surf[surf_no] || !surf[surf_no]->format)
		return;

	static RECT rcSet;
	rcSet.left = rect->left * mag;
	rcSet.top = rect->top * mag;
	rcSet.right = rect->right * mag;
	rcSet.bottom = rect->bottom * mag;

	Uint8 r = (Uint8)(col & 0xFF);
	Uint8 g = (Uint8)((col >> 8) & 0xFF);
	Uint8 b = (Uint8)((col >> 16) & 0xFF);
	Uint32 pixel = SDL_MapRGB(surf[surf_no]->format, r, g, b);

	SDL_Rect sdlRect = {rcSet.left, rcSet.top, rcSet.right - rcSet.left, rcSet.bottom - rcSet.top};
	SDL_FillRect(surf[surf_no], &sdlRect, pixel);
}

//Text
void InitTextObject(char* name)
{
	// Native monospace font: size 14 renders exactly 8px width per character
	// and fits within the 16px line surface without clipping descenders (y, p, g, etc.)
	font = TTF_OpenFont("Resource/font.ttf", 14);
	if (!font) {
		printf("Warning: Failed to load Resource/font.ttf: %s\n", TTF_GetError());
	}
}

void PutText(int x, int y, char* text, unsigned color, SDL_bool unk)
{
	if (!font || !text || !text[0])
		return;

	SDL_Color fg;
	fg.r = (Uint8)(color & 0x0000FF);
	fg.g = (Uint8)((color & 0x00FF00) >> 8);
	fg.b = (Uint8)((color & 0xFF0000) >> 16);
	fg.a = 255;

	SDL_Rect r = { x, y, 0, 0 };

	SDL_Surface* textSurface = TTF_RenderText_Solid(font, text, fg);
	if (textSurface) {
		SDL_BlitSurface(textSurface, NULL, sdlMainSurface, &r);
		SDL_FreeSurface(textSurface);
	}
}

void PutText2(int x, int y, char* text, unsigned color, int surf_no, SDL_bool unk)
{
	if (!font || !text || !text[0] || surf_no < 0 || surf_no >= MAX_SURFACE || !surf[surf_no])
		return;

	SDL_Color fg;
	fg.r = (Uint8)(color & 0x0000FF);
	fg.g = (Uint8)((color & 0x00FF00) >> 8);
	fg.b = (Uint8)((color & 0xFF0000) >> 16);
	fg.a = 255;

	SDL_Rect r = { x, y, 0, 0 };

	SDL_Surface* textSurface = TTF_RenderText_Solid(font, text, fg);
	if (textSurface) {
		SDL_BlitSurface(textSurface, NULL, surf[surf_no], &r);
		SDL_FreeSurface(textSurface);
	}
}

void EndTextObject()
{
	if (font) {
		TTF_CloseFont(font);
		font = NULL;
	}
}
