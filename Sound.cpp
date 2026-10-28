// Sound.cpp - SDL2_mixer implementation replacing DirectSound
#include "Sound.h"
#include "System.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>

#define SE_MAX 512

// SDL2_mixer channel assignments:
// Each sound object slot gets its own Mix_Chunk stored here.
// Playback is done on dedicated channels (channel index == slot index).
static Mix_Chunk* gSoundChunk[SE_MAX];
static Mix_Chunk* gSoundOrigChunk[SE_MAX]; // original before any frequency resampling
static int        gSoundChannel[SE_MAX];   // last channel used to play this slot (-1 = none)

// Volume and pan state
static int  gSoundVolume[SE_MAX];  // 0..MIX_MAX_VOLUME
static long gSoundPan[SE_MAX];     // 0..512 (256 = center)
static unsigned gSoundRate[SE_MAX]; // rate passed to ChangeSoundFrequency (2195 = normal)

// -----------------------------------------------------------------------
// Helper: Calculate SDL panning from DirectSound pan (0..512, 256 = center)
// In SDL_mixer:
//   left  = 255, right = 255  -> Center (both channels at 100%)
//   left  = 255, right = 0    -> Hard Left
//   left  = 0,   right = 255  -> Hard Right
// -----------------------------------------------------------------------
static void CalcPan(long pan, Uint8* left, Uint8* right)
{
    if (pan < 0)   pan = 0;
    if (pan > 512) pan = 512;

    if (pan <= 256)
    {
        *left  = 255;
        *right = (Uint8)(pan * 255 / 256);
    }
    else
    {
        *left  = (Uint8)((512 - pan) * 255 / 256);
        *right = 255;
    }
}

// -----------------------------------------------------------------------
// Helper: Calculate SDL volume from DirectSound volume (0..300)
// DirectSound SetVolume formula: mB = (volume - 300) * 8
// Linear amplitude: 10^(mB / 2000)
// -----------------------------------------------------------------------
static int CalcVolume(long volume)
{
    if (volume <= 0)
        return 0;
    if (volume >= 300)
        return MIX_MAX_VOLUME;

    float mB = (float)(volume - 300) * 8.0f;
    float amp = powf(10.0f, mB / 2000.0f);
    int v = (int)(amp * (float)MIX_MAX_VOLUME + 0.5f);
    if (v < 0) v = 0;
    if (v > MIX_MAX_VOLUME) v = MIX_MAX_VOLUME;
    return v;
}

// -----------------------------------------------------------------------
// Helper: Resample 16-bit stereo Mix_Chunk to a new target playback rate
// -----------------------------------------------------------------------
static Mix_Chunk* ResampleChunk(const Mix_Chunk* src, int orig_hz, int target_hz)
{
    if (!src || target_hz <= 0 || orig_hz <= 0)
        return NULL;

    if (orig_hz == target_hz)
    {
        Mix_Chunk* c = (Mix_Chunk*)SDL_malloc(sizeof(Mix_Chunk));
        if (!c) return NULL;
        c->allocated = 1;
        c->alen = src->alen;
        c->abuf = (Uint8*)SDL_malloc(src->alen);
        if (!c->abuf) { SDL_free(c); return NULL; }
        memcpy(c->abuf, src->abuf, src->alen);
        c->volume = src->volume;
        return c;
    }

    // Assuming S16 stereo (4 bytes per audio frame: 2 bytes L, 2 bytes R)
    int old_frames = src->alen / 4;
    double ratio = (double)orig_hz / (double)target_hz;
    int new_frames = (int)(old_frames * ratio);
    if (new_frames <= 0) return NULL;
    int new_len = new_frames * 4;

    Uint8* new_buf = (Uint8*)SDL_malloc(new_len);
    if (!new_buf) return NULL;

    const Sint16* src_samples = (const Sint16*)src->abuf;
    Sint16* dst_samples = (Sint16*)new_buf;

    for (int i = 0; i < new_frames; i++)
    {
        double src_idx = (double)i / ratio;
        int idx0 = (int)src_idx;
        int idx1 = idx0 + 1;
        if (idx0 >= old_frames) idx0 = old_frames - 1;
        if (idx1 >= old_frames) idx1 = old_frames - 1;
        double frac = src_idx - (int)src_idx;

        // Left channel
        double l0 = src_samples[idx0 * 2];
        double l1 = src_samples[idx1 * 2];
        dst_samples[i * 2] = (Sint16)(l0 + (l1 - l0) * frac);

        // Right channel
        double r0 = src_samples[idx0 * 2 + 1];
        double r1 = src_samples[idx1 * 2 + 1];
        dst_samples[i * 2 + 1] = (Sint16)(r0 + (r1 - r0) * frac);
    }

    Mix_Chunk* dst = (Mix_Chunk*)SDL_malloc(sizeof(Mix_Chunk));
    if (!dst) { SDL_free(new_buf); return NULL; }
    dst->allocated = 1;
    dst->abuf = new_buf;
    dst->alen = new_len;
    dst->volume = src->volume;
    return dst;
}

// -----------------------------------------------------------------------
// Mapping from resource name to WAV file path inside Resource/
// -----------------------------------------------------------------------
static const char* ResNameToPath(const char* resname)
{
    static char buf[256];
    snprintf(buf, sizeof(buf), "Resource/%s.wav", resname);
    return buf;
}

// -----------------------------------------------------------------------
// Init / End
// -----------------------------------------------------------------------
SDL_bool InitDirectSound(SDL_Window* /*hwnd*/)
{
    if (SDL_InitSubSystem(SDL_INIT_AUDIO) < 0)
    {
        fprintf(stderr, "SDL_InitSubSystem(AUDIO) failed: %s\n", SDL_GetError());
        return SDL_FALSE;
    }

    // Open audio: 44100 Hz on PSP (native hardware rate), 22050 Hz on PC
#if defined(PSP) || defined(__psp__) || defined(__PSP__)
    if (Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 1024) < 0)
    {
        if (Mix_OpenAudio(22050, MIX_DEFAULT_FORMAT, 2, 1024) < 0)
        {
            fprintf(stderr, "Mix_OpenAudio failed: %s\n", Mix_GetError());
            return SDL_TRUE; // Don't abort game if audio device fails
        }
    }
#else
    if (Mix_OpenAudio(22050, MIX_DEFAULT_FORMAT, 2, 1024) < 0)
    {
        fprintf(stderr, "Mix_OpenAudio failed: %s\n", Mix_GetError());
        return SDL_TRUE;
    }
#endif

    // Allocate one mixer channel per sound slot
    Mix_AllocateChannels(SE_MAX);

    for (int i = 0; i < SE_MAX; i++)
    {
        gSoundChunk[i]     = NULL;
        gSoundOrigChunk[i] = NULL;
        gSoundChannel[i]   = -1;
        gSoundVolume[i]    = MIX_MAX_VOLUME;
        gSoundPan[i]       = 256; // centre
        gSoundRate[i]      = 2195; // normal frequency
        Mix_Volume(i, MIX_MAX_VOLUME);
    }

    return SDL_TRUE;
}

void EndDirectSound()
{
    for (int i = 0; i < SE_MAX; i++)
        ReleaseSoundObject(i);

    Mix_CloseAudio();
    SDL_QuitSubSystem(SDL_INIT_AUDIO);
}

// -----------------------------------------------------------------------
// Sound object management
// -----------------------------------------------------------------------
void ReleaseSoundObject(int no)
{
    if (no < 0 || no >= SE_MAX)
        return;

    if (gSoundChannel[no] >= 0)
    {
        Mix_HaltChannel(gSoundChannel[no]);
        gSoundChannel[no] = -1;
    }

    if (gSoundChunk[no] != NULL)
    {
        if (gSoundChunk[no] != gSoundOrigChunk[no])
            Mix_FreeChunk(gSoundChunk[no]);
        gSoundChunk[no] = NULL;
    }

    if (gSoundOrigChunk[no] != NULL)
    {
        Mix_FreeChunk(gSoundOrigChunk[no]);
        gSoundOrigChunk[no] = NULL;
    }
}

// Load a named WAV resource from disk (Resource/<resname>.wav)
SDL_bool InitSoundObject(const char* resname, int no)
{
    if (no < 0 || no >= SE_MAX)
        return SDL_FALSE;

    ReleaseSoundObject(no);

    const char* path = ResNameToPath(resname);
    Mix_Chunk* chunk = Mix_LoadWAV(path);
    if (chunk == NULL)
    {
        fprintf(stderr, "Mix_LoadWAV(\"%s\") failed: %s\n", path, Mix_GetError());
        return SDL_FALSE;
    }

    gSoundOrigChunk[no] = chunk;
    gSoundChunk[no]     = chunk;
    gSoundVolume[no]    = MIX_MAX_VOLUME;
    gSoundPan[no]       = 256;
    gSoundRate[no]      = 2195;

    // Keep chunk at full volume; channel volume controls attenuation
    Mix_VolumeChunk(chunk, MIX_MAX_VOLUME);
    return SDL_TRUE;
}

// -----------------------------------------------------------------------
// Playback
// -----------------------------------------------------------------------
void PlaySoundObject(int no, int mode)
{
    if (no < 0 || no >= SE_MAX || gSoundChunk[no] == NULL)
        return;

    switch (mode)
    {
        case SOUND_MODE_STOP:
            if (gSoundChannel[no] >= 0)
            {
                Mix_HaltChannel(gSoundChannel[no]);
                gSoundChannel[no] = -1;
            }
            break;

        case SOUND_MODE_PLAY:
        {
            if (gSoundChannel[no] >= 0)
                Mix_HaltChannel(gSoundChannel[no]);

            // Pre-configure channel volume BEFORE triggering playback to eliminate initial volume spikes / clicks
            Mix_Volume(no, gSoundVolume[no]);

            Uint8 left, right;
            CalcPan(gSoundPan[no], &left, &right);

            int ch = Mix_PlayChannel(no, gSoundChunk[no], 0);
            gSoundChannel[no] = ch;
            if (ch >= 0)
            {
                Mix_SetPanning(ch, left, right);
            }
            break;
        }

        case SOUND_MODE_PLAY_LOOP:
        {
            if (gSoundChannel[no] >= 0)
                Mix_HaltChannel(gSoundChannel[no]);

            Mix_Volume(no, gSoundVolume[no]);

            Uint8 left, right;
            CalcPan(gSoundPan[no], &left, &right);

            int ch = Mix_PlayChannel(no, gSoundChunk[no], -1); // -1 = loop forever
            gSoundChannel[no] = ch;
            if (ch >= 0)
            {
                Mix_SetPanning(ch, left, right);
            }
            break;
        }
    }
}

// -----------------------------------------------------------------------
// Volume / Pan / Frequency
// -----------------------------------------------------------------------

// 100 is MIN, 9999 is MAX, and 2195 is normal (22050 Hz).
// DirectSound uses (rate * 10) + 100 Hz as playback frequency.
void ChangeSoundFrequency(int no, unsigned rate)
{
    if (no < 0 || no >= SE_MAX || gSoundOrigChunk[no] == NULL)
        return;

    if (gSoundRate[no] == rate)
        return;
    gSoundRate[no] = rate;

    int target_hz = (int)(rate * 10 + 100);

    // Free any previous resampled chunk
    if (gSoundChunk[no] != gSoundOrigChunk[no])
    {
        if (gSoundChannel[no] >= 0)
            Mix_HaltChannel(gSoundChannel[no]);
        Mix_FreeChunk(gSoundChunk[no]);
        gSoundChunk[no] = NULL;
    }

    if (target_hz == 22050)
    {
        gSoundChunk[no] = gSoundOrigChunk[no];
    }
    else
    {
        int device_hz = 22050;
        Mix_QuerySpec(&device_hz, NULL, NULL);
        int scaled_target = (int)((double)device_hz * ((double)target_hz / 22050.0));
        Mix_Chunk* resampled = ResampleChunk(gSoundOrigChunk[no], device_hz, scaled_target);
        if (resampled != NULL)
        {
            Mix_VolumeChunk(resampled, MIX_MAX_VOLUME);
            gSoundChunk[no] = resampled;
        }
        else
        {
            gSoundChunk[no] = gSoundOrigChunk[no];
        }
    }
}

// 0..300, 300 is MAX / normal.
void ChangeSoundVolume(int no, long volume)
{
    if (no < 0 || no >= SE_MAX)
        return;

    int sdlVol = CalcVolume(volume);
    gSoundVolume[no] = sdlVol;

    // Direct channel allocation (slot no == channel no): update channel volume immediately
    Mix_Volume(no, sdlVol);
}

// 0..512, 256 is center.
void ChangeSoundPan(int no, long pan)
{
    if (no < 0 || no >= SE_MAX)
        return;

    gSoundPan[no] = pan;

    if (gSoundChannel[no] >= 0 && Mix_Playing(gSoundChannel[no]))
    {
        Uint8 left, right;
        CalcPan(pan, &left, &right);
        Mix_SetPanning(gSoundChannel[no], left, right);
    }
}

// -----------------------------------------------------------------------
// PiyoPiyo synthesised sound object creation
// -----------------------------------------------------------------------

static int freq_tbl[12] = { 1551, 1652, 1747, 1848, 1955, 2074, 2205, 2324, 2461, 2616, 2770, 2938 };

SDL_bool MakePiyoPiyoSoundObject(char *wave, unsigned char *envelope, int octave, int data_size, int no)
{
    if (no < 0 || no + 24 > SE_MAX || data_size <= 0)
        return SDL_FALSE;

    // Allocate synthesis scratch buffer (44-byte standard WAV header + PCM data)
    Uint32 total_wav_len = 44 + (Uint32)data_size;
    unsigned char* wav = (unsigned char*)malloc(total_wav_len);
    if (wav == NULL)
        return SDL_FALSE;

    // Build standard 44-byte WAV header: 22050 Hz, 1 channel, 8-bit PCM
    memcpy(wav + 0, "RIFF", 4);
    Uint32 riff_size = 36 + (Uint32)data_size;
    memcpy(wav + 4, &riff_size, 4);
    memcpy(wav + 8, "WAVEfmt ", 8);
    Uint32 fmt_size = 16;
    memcpy(wav + 16, &fmt_size, 4);
    Uint16 format = 1; // PCM
    memcpy(wav + 20, &format, 2);
    Uint16 channels = 1; // Mono
    memcpy(wav + 22, &channels, 2);
    Uint32 rate = 22050;
    memcpy(wav + 24, &rate, 4);
    memcpy(wav + 28, &rate, 4); // ByteRate = rate * channels * bits/8
    Uint16 align = 1;
    memcpy(wav + 32, &align, 2);
    Uint16 bits = 8;
    memcpy(wav + 34, &bits, 2);
    memcpy(wav + 36, "data", 4);
    Uint32 ds = (Uint32)data_size;
    memcpy(wav + 40, &ds, 4);

    unsigned char* wp = wav + 44;
    const signed char* swave = (const signed char*)wave; // Ensure signed interpretation across all platforms
    SDL_bool result = SDL_TRUE;

    for (int i = 0; i < 24; i++)
    {
        // Synthesise waveform for note i
        int wp_sub = 0;
        int envelope_i;

        for (int j = 0; j < data_size; j++)
        {
            int sample = swave[(unsigned char)(wp_sub / 256)];
            envelope_i = (j << 6) / data_size;
            sample     = sample * envelope[envelope_i] / 128;

            int s = sample + 128;

            // Smooth 32-sample tail fade to 128 to eliminate any end-of-buffer click
            if (j >= data_size - 32)
            {
                s = 128 + ((s - 128) * (data_size - 1 - j)) / 32;
            }

            if (s < 0)   s = 0;
            if (s > 255) s = 255;
            wp[j] = (unsigned char)s;

            int freq;
            if (i < 12)
                freq = octave * freq_tbl[i] / 16;
            else
                freq = octave * freq_tbl[i - 12] / 8;
            wp_sub += freq;
        }

        // Release previous chunk at this slot
        ReleaseSoundObject(no + i);

        // Load into SDL_mixer safely via RWops (auto-converts to mixer native format)
        SDL_RWops* rw = SDL_RWFromMem(wav, total_wav_len);
        Mix_Chunk* chunk = Mix_LoadWAV_RW(rw, 1);
        if (chunk == NULL)
        {
            result = SDL_FALSE;
            break;
        }

        Mix_VolumeChunk(chunk, MIX_MAX_VOLUME);
        gSoundOrigChunk[no + i] = chunk;
        gSoundChunk[no + i]     = chunk;
        gSoundChannel[no + i]   = -1;
        gSoundVolume[no + i]    = MIX_MAX_VOLUME;
        gSoundPan[no + i]       = 256;
        gSoundRate[no + i]      = 2195;
        Mix_Volume(no + i, MIX_MAX_VOLUME);
    }

    free(wav);
    return result;
}
