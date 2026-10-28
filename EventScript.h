#pragma once
#include "Map.h"
#include "NpChar.h"
#include "Item.h"
#include "PiyoPiyo.h"
#include "Effect.h"
#include "fakeWindows.h"

enum FADE_MODE
{
	FADE_MODE_NONE,
	FADE_MODE_QUAKE,
	FADE_MODE_FADEOUT,
	FADE_MODE_FADEIN,
	FADE_MODE_QUAKE2,
};

struct FADE
{
	SDL_bool mask;
	unsigned char mode;
	int wait;
};

struct EVENT_SCR
{
	char mode; //x0
	//alignment x1
	//alignment x2
	//alignment x3
	char* data; //x4
	SDL_bool msg_box; //x8
	//alignment x9
	//alignment xA
	//alignment xB
	unsigned size; //xC
	int event_no; //x10
	unsigned p_read; //x14
	unsigned char ani_cursor; //x18
	//alignment x19
	short wait; //x1A
	unsigned short x1C; //x1C
	char line; //x1E
	char ypos_line[2]; //x1F, x20
	char p_write; //x21
	SDL_bool select; //x22
};

SDL_bool ProcFade(FADE *fade, FRAME *frame, CARET_SPAWNER *caret_spawner);
void PutNumber(int x, int y, int no);
void PutNumber2(int x, int y, int no);
void DebugPutText(char* text);
SDL_bool ReadEventScript(char* path, EVENT_SCR *ptx);
SDL_bool JumpEventScript(EVENT_SCR *ptx);
short GetEventScriptNo(EVENT_SCR *ptx);
void PutEventScriptCursor(EVENT_SCR *ptx);
char EventScriptProc(EVENT_SCR *ptx, ITEMS *items, NPCHAR *npc, MAP *map, PIYOPIYO_CONTROL *piyocont, FADE *fade, FRAME *frame);
void PutMsgBox(EVENT_SCR *ptx);
void InitEventScript(EVENT_SCR *ptx);
