#pragma once
#include "Effect.h"
#include "fakeWindows.h"

#define MAX_NPCS (100)

struct NPCHAR
{
	unsigned char cond; //x0
	unsigned char type; //x1
	signed char code_char; //x2
	short code_event; //x4
	short act_wait; //x6
	signed char act_no; //x8
	signed char ani_no; //x9
	short ani_wait; //xA
	signed char direct; //xC
	unsigned char flag; //xD
	SDL_bool airborne; //xE
	int x; //x10
	int y; //x14
	int xm; //x18
	int ym; //x1C
	int tgt_x; //x20
	int tgt_y; //x24
};

struct MAP; //prototype
struct FRAME; //prototype
struct EVENT_SCR; //prototype

void InitNpChar(NPCHAR *npc);
void PutNpChar(NPCHAR *npc, FRAME *frame);
void ActNpChar(NPCHAR *npc);
void HitNpCharMap(NPCHAR *npc, MAP *map);
void HitMyCharNpChar(NPCHAR *npc, EVENT_SCR *event_scr, CARET_SPAWNER *caret_spawner);
SDL_bool ReloadNpChar(NPCHAR *npc);
