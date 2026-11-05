#pragma once
#include "Item.h"
#include "Map.h"
#include "NpChar.h"
#include "PiyoPiyo.h"
#include "EventScript.h"
#include "fakeWindows.h"

SDL_bool HasSaveState();
SDL_bool SaveState(ITEMS *items, MAP *map, NPCHAR *npc, PIYOPIYO_CONTROL *piyo, FRAME *frame, EVENT_SCR *event_scr);
SDL_bool RestoreSaveState(ITEMS *items, MAP *map, NPCHAR *npc, PIYOPIYO_CONTROL *piyo, FRAME *frame, EVENT_SCR *event_scr);
void DeleteSaveState();

void CaptureAndSaveOnionScreenshot();
void TriggerOnionGameSwitcher();
void DeleteOnionScreenshots();
void OnGameExit(ITEMS *items, MAP *map, NPCHAR *npc, PIYOPIYO_CONTROL *piyo, FRAME *frame, EVENT_SCR *event_scr);
