#pragma once
#include "NpChar.h"
#include "Map.h"
#include "fakeWindows.h"

extern SDL_bool gEditorMode;
extern short gEditorNPC;

void InitEditor();
void PutEditorCursor();
void PutEditorSelect(NPCHAR *npc, FRAME *frame);
void EditorProc(NPCHAR *npc);
void PutEditorNpcInfo(NPCHAR *npc);
SDL_bool LoadNpChar(NPCHAR *npc);
SDL_bool SaveNpChar(NPCHAR *npc);
