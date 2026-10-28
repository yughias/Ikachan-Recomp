#pragma once
#include "fakeWindows.h"

#define MAX_ITEMS (10)

struct ITEMS
{
	char *code;
	char selected_item;
};

struct EVENT_SCR; //forward declaration

void InitItem(ITEMS *items);
void PutItem(ITEMS *items);
void MoveItem(ITEMS *items, EVENT_SCR *event_scr);
SDL_bool CheckItem(ITEMS *items, char code);
SDL_bool AddItemData(ITEMS *items, char code);
SDL_bool SubItemData(ITEMS *items, char code);
