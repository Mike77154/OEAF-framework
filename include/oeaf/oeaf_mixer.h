#ifndef OEAF_MIXER_H
#define OEAF_MIXER_H

#include "oeaf_types.h"
#include "oeaf_rwops.h"

#ifdef __cplusplus
extern "C" {
#endif

/* OEAF SDL_mixer-like API
   ----------------------

   Status:
   - Mix_OpenAudio / CloseAudio : implemented on top of OEAF audio backend
   - WAV (PCM) loading          : implemented (8-bit unsigned, 16-bit signed LE)
   - Basic channel mixing       : implemented (play/stop/volume, looping)
   - Music streaming + codecs   : stubs (future backend work)
*/

typedef struct oeafMix_Chunk oeafMix_Chunk;
typedef struct oeafMix_Music oeafMix_Music;

/* Common SDL audio format constants (subset).
   These match SDL2's values on little-endian platforms.
   If you need big-endian, add the MSB variants.
*/
#ifndef OEAF_AUDIO_S16LSB
#define OEAF_AUDIO_S16LSB 0x8010
#endif
#ifndef OEAF_AUDIO_F32LSB
#define OEAF_AUDIO_F32LSB 0x8120
#endif

int         oeafMix_OpenAudio(int frequency, oeaf_u16 format, int channels, int chunksize);
void        oeafMix_CloseAudio(void);

int         oeafMix_AllocateChannels(int numchans);
int         oeafMix_Volume(int channel, int volume); /* 0..128 */
int         oeafMix_HaltChannel(int channel);

oeafMix_Chunk* oeafMix_LoadWAV(const char* file);
oeafMix_Chunk* oeafMix_LoadWAV_RW(oeaf_RWops* src, int freesrc);
void           oeafMix_FreeChunk(oeafMix_Chunk* chunk);
int            oeafMix_VolumeChunk(oeafMix_Chunk* chunk, int volume); /* 0..128 */

int         oeafMix_PlayChannel(int channel, oeafMix_Chunk* chunk, int loops);

/* Music API stubs (for SDL_mixer parity). */
oeafMix_Music* oeafMix_LoadMUS(const char* file);
void           oeafMix_FreeMusic(oeafMix_Music* music);
int            oeafMix_PlayMusic(oeafMix_Music* music, int loops);
int            oeafMix_HaltMusic(void);

const char*   oeafMix_GetError(void);

/* Optional SDL_mixer aliases (only if SDL headers aren't present). */
#ifndef Mix_OpenAudio
#define Mix_OpenAudio oeafMix_OpenAudio
#endif
#ifndef Mix_CloseAudio
#define Mix_CloseAudio oeafMix_CloseAudio
#endif
#ifndef Mix_LoadWAV
#define Mix_LoadWAV oeafMix_LoadWAV
#endif
#ifndef Mix_PlayChannel
#define Mix_PlayChannel oeafMix_PlayChannel
#endif

#ifdef __cplusplus
}
#endif

#endif /* OEAF_MIXER_H */
