#ifndef OEAF_SDL_AUDIO_H
#define OEAF_SDL_AUDIO_H

#include "oeaf_types.h"
#include "oeaf_audio.h"

#ifdef __cplusplus
extern "C" {
#endif

/* SDL2-ish audio API facade for OEAF.
   ----------------------------------

   This is a *compatibility layer* on top of OEAF's backend-agnostic audio
   interface (oeaf_audio_open_device).

   Notes:
   - Only output devices are supported here (no capture).
   - Formats are limited to S16 and F32 under the hood.
   - Queue-audio is implemented as a simple byte FIFO.
     For safety, prefer queueing while the device is paused.
*/

typedef oeaf_u16 oeaf_AudioFormat;

/* Common SDL audio format constants (subset).
   These match SDL2's values on little-endian platforms.
*/
#ifndef OEAF_AUDIO_U8
#define OEAF_AUDIO_U8     0x0008
#endif
#ifndef OEAF_AUDIO_S16LSB
#define OEAF_AUDIO_S16LSB 0x8010
#endif
#ifndef OEAF_AUDIO_S16SYS
#define OEAF_AUDIO_S16SYS OEAF_AUDIO_S16LSB
#endif
#ifndef OEAF_AUDIO_F32LSB
#define OEAF_AUDIO_F32LSB 0x8120
#endif
#ifndef OEAF_AUDIO_F32SYS
#define OEAF_AUDIO_F32SYS OEAF_AUDIO_F32LSB
#endif

typedef void (*oeaf_AudioCallback)(void* userdata, oeaf_u8* stream, int len);

typedef struct oeaf_AudioSpec {
    int              freq;      /* sample rate */
    oeaf_AudioFormat format;    /* OEAF_AUDIO_* */
    oeaf_u8          channels;  /* 1..2 */
    oeaf_u8          silence;   /* ignored (always 0) */
    oeaf_u16         samples;   /* buffer size in sample frames */
    oeaf_u32         size;      /* computed (bytes), best-effort */
    oeaf_AudioCallback callback;/* if NULL, use oeaf_QueueAudio */
    void*            userdata;
} oeaf_AudioSpec;

/* SDL uses Uint32 IDs; OEAF uses pointer-backed handles.
   (This makes FFI bindings easier.)
*/
typedef oeaf_audio_device oeaf_AudioDeviceID;

/* Open an output device.
   Returns {NULL} on failure.
*/
oeaf_AudioDeviceID oeaf_OpenAudioDevice(
    const char* device,
    oeaf_i32 iscapture,
    const oeaf_AudioSpec* desired,
    oeaf_AudioSpec* obtained,
    oeaf_i32 allowed_changes
);

void        oeaf_CloseAudioDevice(oeaf_AudioDeviceID dev);
void        oeaf_PauseAudioDevice(oeaf_AudioDeviceID dev, oeaf_i32 pause_on);

/* Queue-audio API (works when the device was opened with callback==NULL).
   Returns 0 on success, -1 on failure.
*/
oeaf_i32    oeaf_QueueAudio(oeaf_AudioDeviceID dev, const void* data, oeaf_u32 len);
oeaf_u32    oeaf_GetQueuedAudioSize(oeaf_AudioDeviceID dev);
void        oeaf_ClearQueuedAudio(oeaf_AudioDeviceID dev);

/* Optional SDL aliases when SDL headers aren't present. */
#ifndef SDL_AudioSpec
#define SDL_AudioSpec oeaf_AudioSpec
#endif
#ifndef SDL_AudioDeviceID
#define SDL_AudioDeviceID oeaf_AudioDeviceID
#endif

#ifdef __cplusplus
}
#endif

#endif /* OEAF_SDL_AUDIO_H */
