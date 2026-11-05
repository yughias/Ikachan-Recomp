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

// Fast Adler-32 calculation for zlib framing
static uint32_t Adler32(const uint8_t *data, size_t len)
{
	uint32_t a = 1, b = 0;
	while (len > 0) {
		size_t tlen = len > 5552 ? 5552 : len;
		len -= tlen;
		while (tlen--) {
			a += *data++;
			b += a;
		}
		a %= 65521;
		b %= 65521;
	}
	return (b << 16) | a;
}

// Fast CRC-32 calculation for PNG chunks
static uint32_t Crc32(const uint8_t *data, size_t len)
{
	static uint32_t table[256];
	static bool table_init = false;
	if (!table_init) {
		for (uint32_t i = 0; i < 256; i++) {
			uint32_t c = i;
			for (int k = 0; k < 8; k++)
				c = (c & 1) ? (0xEDB88320L ^ (c >> 1)) : (c >> 1);
			table[i] = c;
		}
		table_init = true;
	}
	uint32_t c = 0xFFFFFFFF;
	for (size_t i = 0; i < len; i++)
		c = table[(c ^ data[i]) & 0xFF] ^ (c >> 8);
	return c ^ 0xFFFFFFFF;
}

// Instant PNG generator (<1 ms): writes compliant PNG using uncompressed DEFLATE blocks (BTYPE=00)
// Completely bypasses CPU-intensive software deflate searching for instant exit on embedded devices
static unsigned char *CreateFastPNG(const uint8_t *pixels, int width, int height, int pitch, int *out_len)
{
	int line_raw_size = 1 + width * 3;
	size_t raw_size = (size_t)line_raw_size * height;

	uint8_t *raw = (uint8_t *)malloc(raw_size);
	if (!raw) return NULL;

	for (int y = 0; y < height; y++) {
		uint8_t *dst = raw + (size_t)y * line_raw_size;
		*dst++ = 0; // Filter: None
		const uint8_t *src = pixels + y * pitch;
		memcpy(dst, src, width * 3);
	}

	uint32_t adler = Adler32(raw, raw_size);

	size_t num_blocks = (raw_size + 65534) / 65535;
	size_t idat_data_size = 2 + num_blocks * 5 + raw_size + 4;

	size_t total_size = 8 + 25 + 12 + idat_data_size + 12;
	uint8_t *png = (uint8_t *)malloc(total_size);
	if (!png) {
		free(raw);
		return NULL;
	}

	uint8_t *p = png;

	// 1. PNG Signature
	static const uint8_t sig[8] = { 137, 80, 78, 71, 13, 10, 26, 10 };
	memcpy(p, sig, 8); p += 8;

	auto write_chunk = [&](const char *tag, const uint8_t *data, uint32_t len) {
		p[0] = (len >> 24) & 0xFF;
		p[1] = (len >> 16) & 0xFF;
		p[2] = (len >> 8) & 0xFF;
		p[3] = len & 0xFF;
		p += 4;

		uint8_t *tag_start = p;
		memcpy(p, tag, 4); p += 4;
		if (data && len > 0) {
			memcpy(p, data, len); p += len;
		}

		uint32_t crc = Crc32(tag_start, 4 + len);
		p[0] = (crc >> 24) & 0xFF;
		p[1] = (crc >> 16) & 0xFF;
		p[2] = (crc >> 8) & 0xFF;
		p[3] = crc & 0xFF;
		p += 4;
	};

	// 2. IHDR Chunk
	uint8_t ihdr[13];
	ihdr[0] = (width >> 24) & 0xFF;
	ihdr[1] = (width >> 16) & 0xFF;
	ihdr[2] = (width >> 8) & 0xFF;
	ihdr[3] = width & 0xFF;
	ihdr[4] = (height >> 24) & 0xFF;
	ihdr[5] = (height >> 16) & 0xFF;
	ihdr[6] = (height >> 8) & 0xFF;
	ihdr[7] = height & 0xFF;
	ihdr[8] = 8; // 8 bits per sample
	ihdr[9] = 2; // Color type: RGB
	ihdr[10] = 0; // Deflate
	ihdr[11] = 0; // Standard filter
	ihdr[12] = 0; // No interlace
	write_chunk("IHDR", ihdr, 13);

	// 3. IDAT Chunk (zlib stream with uncompressed DEFLATE blocks)
	uint8_t *idat_buf = (uint8_t *)malloc(idat_data_size);
	if (!idat_buf) {
		free(raw);
		free(png);
		return NULL;
	}
	uint8_t *idp = idat_buf;
	*idp++ = 0x78; // zlib header
	*idp++ = 0x01; // FCHECK=1 (no compression)

	size_t raw_pos = 0;
	while (raw_pos < raw_size) {
		size_t chunk_len = raw_size - raw_pos;
		if (chunk_len > 65535) chunk_len = 65535;
		bool is_last = (raw_pos + chunk_len >= raw_size);

		*idp++ = is_last ? 0x01 : 0x00; // BFINAL=1/0, BTYPE=00 (stored/uncompressed)

		uint16_t clen = (uint16_t)chunk_len;
		uint16_t nlen = clen ^ 0xFFFF;
		*idp++ = clen & 0xFF;
		*idp++ = (clen >> 8) & 0xFF;
		*idp++ = nlen & 0xFF;
		*idp++ = (nlen >> 8) & 0xFF;

		memcpy(idp, raw + raw_pos, chunk_len);
		idp += chunk_len;
		raw_pos += chunk_len;
	}
	free(raw);

	// Adler-32
	*idp++ = (adler >> 24) & 0xFF;
	*idp++ = (adler >> 16) & 0xFF;
	*idp++ = (adler >> 8) & 0xFF;
	*idp++ = adler & 0xFF;

	write_chunk("IDAT", idat_buf, (uint32_t)(idp - idat_buf));
	free(idat_buf);

	// 4. IEND Chunk
	write_chunk("IEND", NULL, 0);

	*out_len = (int)(p - png);
	return png;
}

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

	// Player
	fread(&gMC, sizeof(MYCHAR), 1, fp);

	// Boss
	fread(&gBoss, sizeof(BOSS), 1, fp);

	// NPCs
	fread(npc, sizeof(NPCHAR), MAX_NPCS, fp);

	// Items
	fread(&items->selected_item, 1, 1, fp);
	fread(items->code, 1, MAX_ITEMS, fp);

	// Flags
	fread(gFlagNPC, 1, FLAG_BYTES, fp);

	// Map
	int w = 0, l = 0;
	fread(&w, 4, 1, fp);
	fread(&l, 4, 1, fp);
	if (w == map->width && l == map->length) {
		fread(map->data, 1, w * l, fp);
	} else {
		fseek(fp, w * l, SEEK_CUR);
	}

	// Frame
	fread(frame, sizeof(FRAME), 1, fp);

	// Audio track
	fread(&piyo->track, 1, 1, fp);
	piyo->mode = 1;
	PiyoPiyoControl(piyo);

	// Event script state
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

	// Target 24-bit RGB surface matching internal resolution: zero scaling overhead
	SDL_Surface* target = SDL_CreateRGBSurfaceWithFormat(0, sdlMainSurface->w, sdlMainSurface->h, 24, SDL_PIXELFORMAT_RGB24);
	if (!target)
		return;

	SDL_BlitSurface(sdlMainSurface, NULL, target, NULL);

	int png_len = 0;
	unsigned char *png_data = CreateFastPNG((const uint8_t *)target->pixels, target->w, target->h, target->pitch, &png_len);
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

		uint32_t hashes[4];
		int hash_count = 0;

		auto add_hash = [&](uint32_t h) {
			if (h == 0) return;
			for (int i = 0; i < hash_count; i++) {
				if (hashes[i] == h) return;
			}
			if (hash_count < 4) {
				hashes[hash_count++] = h;
			}
		};

		// 1. Scan OnionOS recentlist.json / recentlist-hidden.json for the active game hash
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
								break;
							}
						}
					}
				}
				fclose(fp);
			}
			if (hash_count > 0)
				break;
		}

		// 2. Fallback: only if recentlist didn't have it, check which shortcut actually exists
		if (hash_count == 0)
		{
			const char *knownPaths[] = {
				"/mnt/SDCARD/Roms/PORTS/Shortcuts/Action/Ikachan.notfound",
				"/mnt/SDCARD/Roms/PORTS/Shortcuts/Action/Ikachan.port",
				"/mnt/SDCARD/Roms/PORTS/Shortcuts/Action/ikachan.notfound",
				"/mnt/SDCARD/Roms/PORTS/Shortcuts/Action/ikachan.port",
				"/mnt/SDCARD/Roms/PORTS/Shortcuts/Ikachan.notfound",
				"/mnt/SDCARD/Roms/PORTS/Shortcuts/Ikachan.port",
				"/mnt/SDCARD/Roms/PORTS/Games/Ikachan/launch.sh",
				"/mnt/SDCARD/Roms/PORTS/Games/Ikachan/ikachan"
			};

			for (size_t i = 0; i < sizeof(knownPaths)/sizeof(knownPaths[0]); i++) {
				struct stat stFile;
				if (stat(knownPaths[i], &stFile) == 0) {
					add_hash(HashRomPath(knownPaths[i]));
					break;
				}
			}
		}

		// 3. Save PNG to romScreens for target hash (at most 1-2 writes)
		for (int i = 0; i < hash_count; i++) {
			char outPath[512];
			snprintf(outPath, sizeof(outPath), "/mnt/SDCARD/Saves/CurrentProfile/romScreens/%u.png", hashes[i]);
			WriteBufferToFile(outPath, png_data, png_len);
		}

		// 4. Also write to .tmp_update/romScreens if distinct directory
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
