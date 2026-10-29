// PiyoPiyo.cpp - rewritten without Windows APIs
// LocalAlloc/LocalFree → malloc/free
// GetTickCount() → SDL_GetTicks()
// LPTSTR → const char*

#include "PiyoPiyo.h"
#include "Sound.h"
#include "System.h"
#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>

#define MAX_RECORD 1200

#pragma pack(push, 1)
struct PIYOPIYO_TRACKHEADER
{
	//Track information
	unsigned char octave; //x0
	unsigned char icon; //x1
	unsigned short pad; //x2 x3
	unsigned int length; //x4
	unsigned int volume; //x8
	unsigned int unk1, unk2; //xC x10
	char wave[0x100]; //x14
	unsigned char envelope[0x40]; //x114
};

struct PIYOPIYO_HEADER
{
	//Header
	char magic[3]; //x0
	unsigned char writable; //x3
	unsigned int p_track1; //x4
	unsigned int wait; //x8
	int repeat_x; //xC
	int end_x; //x10
	int records; //x14
	
	//Track headers
	PIYOPIYO_TRACKHEADER track[3]; //x18 x16C x2C0
	unsigned int percussion_volume; //x414
};
#pragma pack(pop)

struct PIYOPIYO
{
	//Loaded header
	PIYOPIYO_HEADER header; //x0

	//Playback state
	SDL_bool playing; //x418
	//alignment x419 x41A x41B
	int position; //x41C
	Uint32 tick; //x420 — SDL_GetTicks() returns Uint32
	unsigned *record[4]; //x424
	SDL_bool init; //x428
};

PIYOPIYO gPiyoPiyo;

SDL_bool InitPiyoPiyo()
{
	//Load drums
	if (!InitSoundObject((char*)"BASS1", 472)){
		printf("NO AUDIO\n");
		return SDL_FALSE;
	}
	gPiyoPiyo.init = SDL_TRUE;
	InitSoundObject((char*)"BASS1", 473);
	InitSoundObject((char*)"BASS2", 474);
	InitSoundObject((char*)"BASS2", 475);
	InitSoundObject((char*)"SNARE1", 476);
	InitSoundObject((char*)"SNARE1", 477);
	InitSoundObject((char*)"SNARE1", 478);
	InitSoundObject((char*)"SNARE1", 479);
	InitSoundObject((char*)"HAT1", 480);
	InitSoundObject((char*)"HAT1", 481);
	InitSoundObject((char*)"HAT2", 482);
	InitSoundObject((char*)"HAT2", 483);
	InitSoundObject((char*)"SYMBAL1", 484);
	InitSoundObject((char*)"SYMBAL1", 485);
	InitSoundObject((char*)"SYMBAL1", 486);
	InitSoundObject((char*)"SYMBAL1", 487);
	InitSoundObject((char*)"SYMBAL1", 488);
	InitSoundObject((char*)"SYMBAL1", 489);
	InitSoundObject((char*)"SYMBAL1", 490);
	InitSoundObject((char*)"SYMBAL1", 491);
	InitSoundObject((char*)"SYMBAL1", 492);
	InitSoundObject((char*)"SYMBAL1", 493);
	InitSoundObject((char*)"SYMBAL1", 494);
	InitSoundObject((char*)"SYMBAL1", 495);
	
	//Allocate record buffers (malloc replaces LocalAlloc(LPTR,...))
	gPiyoPiyo.record[0] = (unsigned*)malloc(MAX_RECORD * 4);
	gPiyoPiyo.record[1] = (unsigned*)malloc(MAX_RECORD * 4);
	gPiyoPiyo.record[2] = (unsigned*)malloc(MAX_RECORD * 4);
	gPiyoPiyo.record[3] = (unsigned*)malloc(MAX_RECORD * 4);
	
	//Make sure buffers successfully allocated
	if (gPiyoPiyo.record[0] == NULL || gPiyoPiyo.record[1] == NULL
	 || gPiyoPiyo.record[2] == NULL || gPiyoPiyo.record[3] == NULL)
	{
		free(gPiyoPiyo.record[0]); gPiyoPiyo.record[0] = NULL;
		free(gPiyoPiyo.record[1]); gPiyoPiyo.record[1] = NULL;
		free(gPiyoPiyo.record[2]); gPiyoPiyo.record[2] = NULL;
		free(gPiyoPiyo.record[3]); gPiyoPiyo.record[3] = NULL;
		return SDL_FALSE;
	}

	return SDL_TRUE;
}

void EndPiyoPiyo()
{
	//Release buffers (free replaces LocalFree)
	free(gPiyoPiyo.record[0]); gPiyoPiyo.record[0] = NULL;
	free(gPiyoPiyo.record[1]); gPiyoPiyo.record[1] = NULL;
	free(gPiyoPiyo.record[2]); gPiyoPiyo.record[2] = NULL;
	free(gPiyoPiyo.record[3]); gPiyoPiyo.record[3] = NULL;
}

static SDL_bool ReadPiyoPiyo(const char* path)
{
	//Fail if PiyoPiyo hasn't been initialised
	if (gPiyoPiyo.init == SDL_FALSE)
		return SDL_FALSE;
	
	//Open file
	FILE *fp = fopen(path, "rb");
	if (fp == NULL)
		return SDL_FALSE;
	
	//Read header
	if (fread(&gPiyoPiyo.header, sizeof(PIYOPIYO_HEADER), 1, fp) != 1)
	{
		fclose(fp);
		return SDL_FALSE;
	}
	
	int recCount = gPiyoPiyo.header.records;
	if (recCount < 0)
		recCount = 0;
	if (recCount > MAX_RECORD)
		recCount = MAX_RECORD;
	
	memset(gPiyoPiyo.record[0], 0, MAX_RECORD * 4);
	memset(gPiyoPiyo.record[1], 0, MAX_RECORD * 4);
	memset(gPiyoPiyo.record[2], 0, MAX_RECORD * 4);
	memset(gPiyoPiyo.record[3], 0, MAX_RECORD * 4);
	
	fread(gPiyoPiyo.record[0], 4, recCount, fp);
	fread(gPiyoPiyo.record[1], 4, recCount, fp);
	fread(gPiyoPiyo.record[2], 4, recCount, fp);
	fread(gPiyoPiyo.record[3], 4, recCount, fp);
	fclose(fp);
	gPiyoPiyo.position = -1;
	return SDL_TRUE;
}

void PiyoPiyoProc()
{
	int pan_tbl[8] = {
		0, 96, 180, 224, 256, 288, 332, 420
	};
	
	if (!gPiyoPiyo.init || !gPiyoPiyo.playing)
		return;

	// Catch-up loop: process pending steps if time elapsed, capped at 4 iterations
	// to maintain accurate tempo even during frame drops or vsync jitter on PSP.
	int catchup = 0;
	while (SDL_GetTicks() >= (gPiyoPiyo.tick + gPiyoPiyo.header.wait) && catchup < 4)
	{
		catchup++;
		// Accumulate tick by wait to maintain mathematically exact tempo without drift
		gPiyoPiyo.tick += gPiyoPiyo.header.wait;
		if (SDL_GetTicks() - gPiyoPiyo.tick > 2000)
			gPiyoPiyo.tick = SDL_GetTicks();

		//Check if position passes loop point
		if (++gPiyoPiyo.position > (gPiyoPiyo.header.end_x - 1) || gPiyoPiyo.position > (gPiyoPiyo.header.records - 1))
			gPiyoPiyo.position = gPiyoPiyo.header.repeat_x;
		
		//Step channels
		for (int i = 0; i < 4; i++)
		{
			//Get this record
			unsigned record = gPiyoPiyo.record[i][gPiyoPiyo.position];
			
			//Change pan
			if (record & 0xFF000000)
			{
				int pan = record >> 24;
				for (int j = 0; j < 24; j++)
					ChangeSoundPan(400 + (i * 24) + j, pan_tbl[pan]);
			}
			
			//Play notes
			for (int j = 0; j < 24; j++)
			{
				if (record & 1)
					PlaySoundObject(400 + (i * 24) + j, 1);
				record >>= 1;
			}
		}
	}
}

static void MakePiyoPiyoSoundObjects()
{
	//Make sure PiyoPiyo has been initialised
	if (gPiyoPiyo.init)
	{
		// Release all previous melody objects first to consolidate free heap space on PSP-1000
		for (int s = 400; s < 472; s++)
			ReleaseSoundObject(s);

		//Setup each melody track
		for (int i = 0; i < 3; i++)
		{
			//Get octave
			int octave = 1 << gPiyoPiyo.header.track[i].octave;
			
			//Compute bitmask of notes actually played in this track across the whole song
			unsigned note_mask = 0;
			for (int r = 0; r < gPiyoPiyo.header.records && r < MAX_RECORD; r++)
			{
				note_mask |= (gPiyoPiyo.record[i][r] & 0x00FFFFFF);
			}

			//Make new objects (passing note_mask to skip synthesizing unused notes)
			MakePiyoPiyoSoundObject(
				gPiyoPiyo.header.track[i].wave,
				gPiyoPiyo.header.track[i].envelope,
				octave, 
				gPiyoPiyo.header.track[i].length,
				400 + (24 * i),
				note_mask);
			
			//Set track volume
			for (int j = 0; j < 24; j++)
				ChangeSoundVolume(400 + (i * 24) + j, gPiyoPiyo.header.track[i].volume);
		}
		
		//Set drum volume
		for (int i = 0; i < 24; i += 2)
		{
			ChangeSoundVolume(472 + i, gPiyoPiyo.header.percussion_volume);
			ChangeSoundVolume(473 + i, 70 * gPiyoPiyo.header.percussion_volume / 100);
		}
	}
}

static void ChangePiyoPiyoVolume(PIYOPIYO_CONTROL *piyocont)
{
	if (gPiyoPiyo.init)
	{
		//Set melody volume
		for (int i = 0; i < 3; i++)
		{
			for (int j = 0; j < 24; j++)
				ChangeSoundVolume(400 + (i * 24) + j, gPiyoPiyo.header.track[i].volume - piyocont->volume);
		}
		
		//Set drum volume
		for (int i = 0; i < 24; i += 2)
		{
			ChangeSoundVolume(472 + i, gPiyoPiyo.header.percussion_volume - piyocont->volume);
			ChangeSoundVolume(473 + i, 70 * gPiyoPiyo.header.percussion_volume / 100 - piyocont->volume);
		}
	}
}

//PiyoPiyo control
// const char* replaces LPTSTR (wchar on MSVC, char* on MinGW — avoid both)
static const char* gMusicList[6] =
{
	"Pmd/Ikachan.pmd",
	"Pmd/Magirete.pmd",
	"Pmd/Buriki.pmd",
	"Pmd/Mizuno.pmd",
	"Pmd/Quake.pmd",
	"Pmd/Tidepool.pmd",
};

void PiyoPiyoControl(PIYOPIYO_CONTROL *piyocont)
{
	//Check if PiyoPiyo's been initialised
	if (gPiyoPiyo.init)
	{
		switch (piyocont->mode)
		{
			case 1:
				if (gPiyoPiyo.playing && piyocont->track == piyocont->prev_track)
				{
					//Track hasn't changed, do nothing
					piyocont->mode = 0;
				}
				else
				{
					//Stop playing previous song
					gPiyoPiyo.playing = SDL_FALSE;
					
					//Read given track
					char path[MAX_PATH];
					snprintf(path, sizeof(path), "%s", gMusicList[piyocont->track]);
					ReadPiyoPiyo(path);
					MakePiyoPiyoSoundObjects();
					piyocont->prev_track = piyocont->track;
					piyocont->mode = 2;
				}
				break;
			case 2:
				//Play loaded track
				gPiyoPiyo.position = -1;
				gPiyoPiyo.tick = SDL_GetTicks() - gPiyoPiyo.header.wait;
				gPiyoPiyo.playing = SDL_TRUE;
				piyocont->volume = 0;
				ChangePiyoPiyoVolume(piyocont);
				piyocont->mode = 0;
				break;
			case 3:
				//Stop loaded track
				gPiyoPiyo.playing = SDL_FALSE;
				piyocont->mode = 0;
				break;
			case 4:
				//Fade loaded track
				piyocont->volume++;
				ChangePiyoPiyoVolume(piyocont);
				
				//Check if song should stop
				if (piyocont->volume > 250)
				{
					piyocont->mode = 0;
					gPiyoPiyo.playing = SDL_FALSE;
				}
				break;
			case 5:
				//Play loaded track at low volume
				piyocont->volume = 100;
				ChangePiyoPiyoVolume(piyocont);
				piyocont->mode = 0;
				break;
			case 6:
				//Play loaded track at full volume
				piyocont->volume = 0;
				ChangePiyoPiyoVolume(piyocont);
				piyocont->mode = 0;
				break;
			default:
				return;
		}
	}
}
