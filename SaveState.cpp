#include "SaveState.h"
#include "Player.h"
#include "Boss.h"
#include "Flags.h"
#include "Draw.h"
#include "Config.h"
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#ifndef _WIN32
#include <unistd.h>
#endif

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

#define SAVE_STATE_MAGIC "IKASTATE"
#define SAVE_STATE_VERSION 1

struct EVENT_SCR_STATE
{
	char mode;
	SDL_bool msg_box;
	int event_no;
	unsigned p_read;
	unsigned char ani_cursor;
	short wait;
	unsigned short x1C;
	char line;
	char ypos_line[2];
	char p_write;
	SDL_bool select;
};

#define _PADr_KAZE(x, n) (((x) << (n)) >> (n))

static uint32_t FNV1A_Pippip_Yurii(const char *str, size_t wrdlen)
{
	const uint32_t PRIME = 591798841;
	uint32_t hash32;
	uint64_t hash64 = 14695981039346656037ULL;
	size_t Cycles, NDhead;
	if (wrdlen > 8) {
		Cycles = ((wrdlen - 1) >> 4) + 1;
		NDhead = wrdlen - (Cycles << 3);
		for (; Cycles--; str += 8) {
			hash64 = (hash64 ^ (*(const uint64_t *)(str))) * PRIME;
			hash64 = (hash64 ^ (*(const uint64_t *)(str + NDhead))) * PRIME;
		}
	}
	else
		hash64 =
			(hash64 ^ _PADr_KAZE(*(const uint64_t *)(str + 0), (8 - wrdlen) << 3)) *
			PRIME;
	hash32 = (uint32_t)(hash64 ^ (hash64 >> 32));
	return hash32 ^ (hash32 >> 16);
}

static uint32_t HashRomPath(const char *str)
{
	if (!str || !*str) return 0;
	size_t len = strlen(str);
	char *buf = (char *)calloc(len + 16, 1);
	if (!buf) return 0;
	memcpy(buf, str, len);
	uint32_t h = FNV1A_Pippip_Yurii(buf, len);
	free(buf);
	return h;
}

static void WriteBufferToFile(const char *path, const void *data, size_t size)
{
	FILE *fp = fopen(path, "wb");
	if (fp) {
		fwrite(data, 1, size, fp);
		fflush(fp);
#if !defined(_WIN32)
		fsync(fileno(fp));
#endif
		fclose(fp);
	}
}

#if !defined(_WIN32)
static void EnsureDirectory(const char *path)
{
	struct stat st;
	if (stat(path, &st) == 0 && S_ISDIR(st.st_mode))
		return;

	char tmp[512];
	snprintf(tmp, sizeof(tmp), "%s", path);
	size_t len = strlen(tmp);
	if (len == 0) return;
	if (tmp[len - 1] == '/') tmp[len - 1] = '\0';
	for (char *p = tmp + 1; *p; p++) {
		if (*p == '/') {
			*p = '\0';
			mkdir(tmp, 0755);
			*p = '/';
		}
	}
	mkdir(tmp, 0755);
}
#endif

SDL_bool HasSaveState()
{
	FILE *fp = fopen("Ika.state", "rb");
	if (fp) {
		char magic[8];
		bool ok = (fread(magic, 1, 8, fp) == 8 && memcmp(magic, SAVE_STATE_MAGIC, 8) == 0);
		fclose(fp);
		return ok ? SDL_TRUE : SDL_FALSE;
	}
	return SDL_FALSE;
}

SDL_bool SaveState(ITEMS *items, MAP *map, NPCHAR *npc, PIYOPIYO_CONTROL *piyo, FRAME *frame, EVENT_SCR *event_scr)
{
	FILE *fp = fopen("Ika.state", "wb");
	if (!fp)
		return SDL_FALSE;

	char magic[8];
	memcpy(magic, SAVE_STATE_MAGIC, 8);
	fwrite(magic, 1, 8, fp);
	uint32_t version = SAVE_STATE_VERSION;
	fwrite(&version, 4, 1, fp);

	// Player
	fwrite(&gMC, sizeof(MYCHAR), 1, fp);

	// Boss
	fwrite(&gBoss, sizeof(BOSS), 1, fp);

	// NPCs
	fwrite(npc, sizeof(NPCHAR), MAX_NPCS, fp);

	// Items
	fwrite(&items->selected_item, 1, 1, fp);
	fwrite(items->code, 1, MAX_ITEMS, fp);

	// Flags
	fwrite(gFlagNPC, 1, FLAG_BYTES, fp);

	// Map
	fwrite(&map->width, 4, 1, fp);
	fwrite(&map->length, 4, 1, fp);
	fwrite(map->data, 1, map->width * map->length, fp);

	// Frame
	fwrite(frame, sizeof(FRAME), 1, fp);

	// Audio track
	fwrite(&piyo->track, 1, 1, fp);

	// Event script state
	EVENT_SCR_STATE ess;
	memset(&ess, 0, sizeof(ess));
	if (event_scr) {
		ess.mode = event_scr->mode;
		ess.msg_box = event_scr->msg_box;
		ess.event_no = event_scr->event_no;
		ess.p_read = event_scr->p_read;
		ess.ani_cursor = event_scr->ani_cursor;
		ess.wait = event_scr->wait;
		ess.x1C = event_scr->x1C;
		ess.line = event_scr->line;
		ess.ypos_line[0] = event_scr->ypos_line[0];
		ess.ypos_line[1] = event_scr->ypos_line[1];
		ess.p_write = event_scr->p_write;
		ess.select = event_scr->select;
	}
	fwrite(&ess, sizeof(ess), 1, fp);
	fflush(fp);
#if !defined(_WIN32)
	fsync(fileno(fp));
#endif
	fclose(fp);

	// Update standard record as well
	SaveRecord(items, map, npc);

	return SDL_TRUE;
}

SDL_bool RestoreSaveState(ITEMS *items, MAP *map, NPCHAR *npc, PIYOPIYO_CONTROL *piyo, FRAME *frame, EVENT_SCR *event_scr)
{
	FILE *fp = fopen("Ika.state", "rb");
	if (!fp)
		return SDL_FALSE;

	char magic[8];
	if (fread(magic, 1, 8, fp) != 8 || memcmp(magic, SAVE_STATE_MAGIC, 8) != 0) {
		fclose(fp);
		return SDL_FALSE;
	}

	uint32_t version = 0;
	if (fread(&version, 4, 1, fp) != 1 || version != SAVE_STATE_VERSION) {
		fclose(fp);
		return SDL_FALSE;
	}

	fread(&gMC, sizeof(MYCHAR), 1, fp);
	fread(&gBoss, sizeof(BOSS), 1, fp);
	fread(npc, sizeof(NPCHAR), MAX_NPCS, fp);

	fread(&items->selected_item, 1, 1, fp);
	fread(items->code, 1, MAX_ITEMS, fp);
	fread(gFlagNPC, 1, FLAG_BYTES, fp);

	int w = 0, h = 0;
	fread(&w, 4, 1, fp);
	fread(&h, 4, 1, fp);
	if (w > 0 && h > 0) {
		if (map->data == NULL || map->width != w || map->length != h) {
			if (map->data) free(map->data);
			map->data = (unsigned char*)malloc(w * h);
		}
		map->width = w;
		map->length = h;
		fread(map->data, 1, w * h, fp);
	}

	fread(frame, sizeof(FRAME), 1, fp);
	fread(&piyo->track, 1, 1, fp);

	EVENT_SCR_STATE ess;
	if (fread(&ess, sizeof(ess), 1, fp) == 1 && event_scr) {
		event_scr->mode = ess.mode;
		event_scr->msg_box = ess.msg_box;
		event_scr->event_no = ess.event_no;
		event_scr->p_read = ess.p_read;
		event_scr->ani_cursor = ess.ani_cursor;
		event_scr->wait = ess.wait;
		event_scr->x1C = ess.x1C;
		event_scr->line = ess.line;
		event_scr->ypos_line[0] = ess.ypos_line[0];
		event_scr->ypos_line[1] = ess.ypos_line[1];
		event_scr->p_write = ess.p_write;
		event_scr->select = ess.select;
	}

	fclose(fp);

	gMC.cond = SDL_TRUE;
	gMC.dead = SDL_FALSE;

	remove("/tmp/force_auto_load_state");

	return SDL_TRUE;
}

void DeleteSaveState()
{
	remove("Ika.state");
	remove("/tmp/force_auto_load_state");
}

void CaptureAndSaveOnionScreenshot()
{
	static bool sScreenshotSaved = false;
	if (sScreenshotSaved)
		return;
	sScreenshotSaved = true;

	if (!sdlMainSurface)
		return;

	// Target 640x480 RGB surface for Miyoo Mini native display
	SDL_Surface* target = SDL_CreateRGBSurfaceWithFormat(0, 640, 480, 24, SDL_PIXELFORMAT_RGB24);
	if (!target)
		return;

	SDL_BlitScaled(sdlMainSurface, NULL, target, NULL);

	// Ultra-fast PNG encoding:
	// - Filter 0 (None): skips 5 passes of filter evaluation & entropy calculation per scanline
	// - Compression level 1: fast greedy zlib deflate (10x faster than default level 8)
	stbi_write_force_png_filter = 0;
	stbi_write_png_compression_level = 1;

	// Compress to memory ONCE instead of re-compressing dozens of times
	int png_len = 0;
	unsigned char *png_data = stbi_write_png_to_mem((const unsigned char *)target->pixels, target->pitch, 640, 480, 3, &png_len);
	SDL_FreeSurface(target);

	if (!png_data || png_len <= 0)
		return;

	// Local fallback copy
	WriteBufferToFile("screenshot.png", png_data, png_len);

#if defined(MIYOO) || defined(__MIYOO__) || defined(__linux__)
	struct stat st;
	if (stat("/mnt/SDCARD", &st) == 0)
	{
		EnsureDirectory("/mnt/SDCARD/Saves/CurrentProfile/romScreens");

		uint32_t hashes[64];
		int hash_count = 0;

		auto add_hash = [&](uint32_t h) {
			if (h == 0) return;
			for (int i = 0; i < hash_count; i++) {
				if (hashes[i] == h) return;
			}
			if (hash_count < 64) {
				hashes[hash_count++] = h;
			}
		};

		// 1. Scan OnionOS recentlist.json / recentlist-hidden.json
		const char *recentFiles[] = {
			"/mnt/SDCARD/Roms/recentlist.json",
			"/mnt/SDCARD/Roms/recentlist-hidden.json"
		};

		for (int f = 0; f < 2; f++) {
			FILE *fp = fopen(recentFiles[f], "r");
			if (fp) {
				char line[1024];
				while (fgets(line, sizeof(line), fp)) {
					if (strstr(line, "Ikachan") || strstr(line, "ikachan")) {
						char *rompathStart = strstr(line, "\"rompath\":\"");
						if (rompathStart) {
							rompathStart += 11;
							char *rompathEnd = strchr(rompathStart, '\"');
							if (rompathEnd) {
								char rompath[512];
								size_t len = rompathEnd - rompathStart;
								if (len >= sizeof(rompath)) len = sizeof(rompath) - 1;
								memcpy(rompath, rompathStart, len);
								rompath[len] = '\0';

								add_hash(HashRomPath(rompath));

								char *colon = strchr(rompath, ':');
								if (colon) {
									add_hash(HashRomPath(colon + 1));
								}
							}
						}
					}
				}
				fclose(fp);
			}
		}

		// 2. Standard known OnionOS port shortcuts & paths
		const char *knownPaths[] = {
			"/mnt/SDCARD/Roms/PORTS/Shortcuts/Action/Ikachan.notfound",
			"/mnt/SDCARD/Roms/PORTS/Shortcuts/Action/Ikachan.port",
			"/mnt/SDCARD/Roms/PORTS/Shortcuts/Action/ikachan.notfound",
			"/mnt/SDCARD/Roms/PORTS/Shortcuts/Action/ikachan.port",
			"/mnt/SDCARD/Roms/PORTS/Shortcuts/Ikachan.notfound",
			"/mnt/SDCARD/Roms/PORTS/Shortcuts/Ikachan.port",
			"/mnt/SDCARD/Roms/PORTS/Games/Ikachan/launch.sh",
			"/mnt/SDCARD/Roms/PORTS/Games/Ikachan/ikachan",
			"Ikachan",
			"ikachan"
		};

		for (size_t i = 0; i < sizeof(knownPaths)/sizeof(knownPaths[0]); i++) {
			add_hash(HashRomPath(knownPaths[i]));
		}

		// 3. Save pre-compressed PNG to romScreens for all computed hashes
		for (int i = 0; i < hash_count; i++) {
			char outPath[512];
			snprintf(outPath, sizeof(outPath), "/mnt/SDCARD/Saves/CurrentProfile/romScreens/%u.png", hashes[i]);
			WriteBufferToFile(outPath, png_data, png_len);
		}

		// 4. Also write to .tmp_update/romScreens if directory exists and is distinct
		struct stat stTmp;
		if (stat("/mnt/SDCARD/.tmp_update/romScreens", &stTmp) == 0 && S_ISDIR(stTmp.st_mode)) {
			struct stat stSaves;
			bool isSameDir = (stat("/mnt/SDCARD/Saves/CurrentProfile/romScreens", &stSaves) == 0 &&
			                  stSaves.st_ino == stTmp.st_ino && stSaves.st_dev == stTmp.st_dev);
			if (!isSameDir) {
				for (int i = 0; i < hash_count; i++) {
					char outPath[512];
					snprintf(outPath, sizeof(outPath), "/mnt/SDCARD/.tmp_update/romScreens/%u.png", hashes[i]);
					WriteBufferToFile(outPath, png_data, png_len);
				}
			}
		}
	}
#endif

	free(png_data);
}

void TriggerOnionGameSwitcher()
{
	static bool sSwitcherTriggered = false;
	if (sSwitcherTriggered)
		return;
	sSwitcherTriggered = true;

#if defined(MIYOO) || defined(__MIYOO__) || defined(__linux__)
	struct stat st;
	if (stat("/mnt/SDCARD/.tmp_update", &st) == 0 && S_ISDIR(st.st_mode))
	{
		FILE *fp = fopen("/mnt/SDCARD/.tmp_update/.runGameSwitcher", "wb");
		if (fp)
		{
			fputc('1', fp);
			fflush(fp);
			fsync(fileno(fp));
			fclose(fp);
		}
	}
#endif
}

void OnGameExit(ITEMS *items, MAP *map, NPCHAR *npc, PIYOPIYO_CONTROL *piyo, FRAME *frame, EVENT_SCR *event_scr)
{
	static bool sAlreadyExited = false;
	if (sAlreadyExited)
		return;
	sAlreadyExited = true;

	if (gConfig.autosave_on_exit)
	{
		SaveState(items, map, npc, piyo, frame, event_scr);
		CaptureAndSaveOnionScreenshot();
		TriggerOnionGameSwitcher();
	}
}

void DeleteOnionScreenshots()
{
#if defined(MIYOO) || defined(__MIYOO__) || defined(__linux__)
	struct stat st;
	if (stat("/mnt/SDCARD", &st) == 0)
	{
		uint32_t hashes[64];
		int hash_count = 0;

		auto add_hash = [&](uint32_t h) {
			if (h == 0) return;
			for (int i = 0; i < hash_count; i++) {
				if (hashes[i] == h) return;
			}
			if (hash_count < 64) {
				hashes[hash_count++] = h;
			}
		};

		const char *knownPaths[] = {
			"/mnt/SDCARD/Roms/PORTS/Shortcuts/Action/Ikachan.notfound",
			"/mnt/SDCARD/Roms/PORTS/Shortcuts/Action/Ikachan.port",
			"/mnt/SDCARD/Roms/PORTS/Shortcuts/Action/ikachan.notfound",
			"/mnt/SDCARD/Roms/PORTS/Shortcuts/Action/ikachan.port",
			"/mnt/SDCARD/Roms/PORTS/Shortcuts/Ikachan.notfound",
			"/mnt/SDCARD/Roms/PORTS/Shortcuts/Ikachan.port",
			"/mnt/SDCARD/Roms/PORTS/Games/Ikachan/launch.sh",
			"/mnt/SDCARD/Roms/PORTS/Games/Ikachan/ikachan",
			"Ikachan",
			"ikachan"
		};

		for (size_t i = 0; i < sizeof(knownPaths)/sizeof(knownPaths[0]); i++) {
			add_hash(HashRomPath(knownPaths[i]));
		}

		for (int i = 0; i < hash_count; i++) {
			char outPath[512];
			snprintf(outPath, sizeof(outPath), "/mnt/SDCARD/Saves/CurrentProfile/romScreens/%u.png", hashes[i]);
			remove(outPath);
			snprintf(outPath, sizeof(outPath), "/mnt/SDCARD/.tmp_update/romScreens/%u.png", hashes[i]);
			remove(outPath);
		}
	}
#endif
	remove("screenshot.png");
}
