#ifndef __FAKE_WINDOWS_H__
#define __FAKE_WINDOWS_H__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if defined(__has_include)
  #if __has_include(<SDL.h>)
    #include <SDL.h>
    #include <SDL_ttf.h>
  #elif __has_include(<SDL2/SDL.h>)
    #include <SDL2/SDL.h>
    #include <SDL2/SDL_ttf.h>
  #else
    #include <SDL.h>
    #include <SDL_ttf.h>
  #endif
#else
  #include <SDL.h>
  #include <SDL_ttf.h>
#endif

#define MAX_PATH 256

typedef struct RECT {
    Sint32 left;
    Sint32 top;
    Sint32 right;
    Sint32 bottom;
} RECT;

typedef struct POINT {
    int x;
    int y;
} POINT;

#pragma pack(push,1)
typedef struct tagBITMAPFILEHEADER {
    Uint16 bfType;
    Uint32 bfSize;
    Uint16 bfReserved1;
    Uint16 bfReserved2;
    Uint32 bfOffBits;
} BITMAPFILEHEADER;
#pragma pack(pop)

typedef struct tagBITMAPINFOHEADER {
    Uint32 biSize;
    Sint32 biWidth;
    Sint32 biHeight;
    Uint16 biPlanes;
    Uint16 biBitCount;
    Uint32 biCompression;
    Uint32 biSizeImage;
    Sint32 biXPelsPerMeter;
    Sint32 biYPelsPerMeter;
    Uint32 biClrUsed;
    Uint32 biClrImportant;
} BITMAPINFOHEADER;

#endif