# Open Empty Aria Framework (OEAF)

**OEAF = "SDL as a contract", but backend-agnostic and Frankenstein-mixable.**

- OEAF core owns:
  - A cross-backend event queue
  - A module list (composition)
  - (Optional) dt helper

- Backends provide:
  - OS pump (reads platform events, pushes OEAF events)
  - Timer (monotonic counter/frequency)
  - Video (window + swap + native handles)
  - Input (events + optional state queries)
  - Audio (device open + callback)
  - FS (read/write)

You can attach multiple modules and mix subsystems:

- OS from Win32
- Input from SDL
- Video from your in-house backend
- Audio from Raylib
- Timer from null backend (testing)

## Build (demo uses the null backend)
```bash
make
./build/oeaf_demo
```

Or CMake:
```bash
cmake -S . -B build
cmake --build build
./build/oeaf_demo
```

## Implementing a backend
Provide an `oeaf_module_desc` with:
- `name`, `abi_version`, `provides`
- `attach/detach`
- vtable pointers for any subsystems you provide

Backends should push events into OEAF using `oeaf_events_push(ctx, &event)`.

See `backends/null` for a minimal example.

## SDL-like convenience API

OEAF also ships an optional SDL-style global API (single global context), so you
can write loops that look familiar:

```c
#include "oeaf/oeaf.h"
#include "oeaf/oeaf_sdl.h"

int main(void) {
    if (oeaf_init(OEAF_INIT_TIMER | OEAF_INIT_EVENTS) != OEAF_OK) {
        /* oeaf_get_error() is SDL_GetError-style */
        return 1;
    }

    for (;;) {
        oeaf_event e;
        while (oeaf_poll_event(&e)) {
            if (e.type == OEAF_EVENT_QUIT) {
                oeaf_quit();
                return 0;
            }
        }
        oeaf_delay(16);
    }
}
```

If you need multi-context control, use the explicit-context functions:
- `oeaf_create/oeaf_destroy`
- `oeaf_attach_module/oeaf_require`
- `oeaf_poll_event_ctx(ctx, &e)`

## SDL extension equivalents (image / ttf / mixer)

OEAF ships *SDL-style extension headers* so your code can keep familiar calls:

- `oeafIMG_*`  (SDL2_image-like)
- `oeafTTF_*`  (SDL2_ttf-like)
- `oeafMix_*`  (SDL_mixer-like)

And an SDL2-ish audio facade:

- `oeaf_OpenAudioDevice` / `oeaf_PauseAudioDevice` / `oeaf_QueueAudio` ... (`oeaf_sdl_audio.h`)

Current implementation status (v0.1):

- **Image**: built-in loader supports **BMP (uncompressed 24/32-bit)** and **PPM (P6)**.
- **Mixer**: supports **WAV (PCM 8-bit/16-bit)** and basic **channel mixing**.
  Requires an **OEAF audio backend** to actually output sound.
- **TTF**: API skeleton only (stub). Hook up a FreeType/HarfBuzz backend to make it real.
