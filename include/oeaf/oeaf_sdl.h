#ifndef OEAF_SDL_H
#define OEAF_SDL_H

/*
  SDL-style convenience API for OEAF.

  Goal:
    - Let engines call oeaf_init/oeaf_quit/oeaf_delay/oeaf_poll_event like SDL.
    - Keep the underlying OEAF context + modular backends intact.

  Notes:
    - This layer uses a single global oeaf_ctx internally (SDL-style).
    - Advanced users can still work with oeaf_ctx directly via oeaf_create/oeaf_destroy
      and oeaf_attach_module/oeaf_require.
*/

#include "oeaf_types.h"
#include "oeaf_backend.h"
#include "oeaf_events.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
  SDL2-compatible init flags
  --------------------------
  Goal: let you write code that *feels* like SDL, and keep numeric values
  identical to SDL2's SDL_INIT_* flags.

  Reference (SDL2 header):
    https://raw.githubusercontent.com/libsdl-org/SDL/SDL2/include/SDL.h

  Notes:
    - OEAF does not (yet) implement all SDL subsystems; we still accept these
      flags for familiarity.
    - Internally, OEAF translates these flags into OEAF_PROVIDES_* requirements.
    - OEAF_INIT_NOPARACHUTE is ignored (SDL2 also ignores it).
*/

/* Keep OEAF_* names (so your code reads "oeaf_init(OEAF_INIT_VIDEO)") ... */
#define OEAF_INIT_TIMER          0x00000001u
#define OEAF_INIT_AUDIO          0x00000010u
#define OEAF_INIT_VIDEO          0x00000020u /* implies events */
#define OEAF_INIT_JOYSTICK       0x00000200u /* implies events */
#define OEAF_INIT_HAPTIC         0x00001000u
#define OEAF_INIT_GAMECONTROLLER 0x00002000u /* implies joystick */
#define OEAF_INIT_EVENTS         0x00004000u
#define OEAF_INIT_SENSOR         0x00008000u
#define OEAF_INIT_NOPARACHUTE    0x00100000u /* ignored */

#define OEAF_INIT_EVERYTHING ( \
    OEAF_INIT_TIMER | OEAF_INIT_AUDIO | OEAF_INIT_VIDEO | OEAF_INIT_EVENTS | \
    OEAF_INIT_JOYSTICK | OEAF_INIT_HAPTIC | OEAF_INIT_GAMECONTROLLER | OEAF_INIT_SENSOR \
)

/* ...and (optionally) provide SDL_* names as aliases when SDL headers aren't included.
   This makes porting SDL code almost copy/paste.
*/
#ifndef SDL_INIT_TIMER
#define SDL_INIT_TIMER OEAF_INIT_TIMER
#endif
#ifndef SDL_INIT_AUDIO
#define SDL_INIT_AUDIO OEAF_INIT_AUDIO
#endif
#ifndef SDL_INIT_VIDEO
#define SDL_INIT_VIDEO OEAF_INIT_VIDEO
#endif
#ifndef SDL_INIT_JOYSTICK
#define SDL_INIT_JOYSTICK OEAF_INIT_JOYSTICK
#endif
#ifndef SDL_INIT_HAPTIC
#define SDL_INIT_HAPTIC OEAF_INIT_HAPTIC
#endif
#ifndef SDL_INIT_GAMECONTROLLER
#define SDL_INIT_GAMECONTROLLER OEAF_INIT_GAMECONTROLLER
#endif
#ifndef SDL_INIT_EVENTS
#define SDL_INIT_EVENTS OEAF_INIT_EVENTS
#endif
#ifndef SDL_INIT_SENSOR
#define SDL_INIT_SENSOR OEAF_INIT_SENSOR
#endif
#ifndef SDL_INIT_NOPARACHUTE
#define SDL_INIT_NOPARACHUTE OEAF_INIT_NOPARACHUTE
#endif
#ifndef SDL_INIT_EVERYTHING
#define SDL_INIT_EVERYTHING OEAF_INIT_EVERYTHING
#endif

/* Initialize OEAF (SDL_Init-style).
   - Creates the global context on first call.
   - Attaches the built-in null backend as a baseline.
   - Validates required subsystems (flags).
*/
oeaf_result oeaf_init(oeaf_u32 flags);

/* Shutdown OEAF and destroy the global context (SDL_Quit-style). */
void        oeaf_quit(void);

/* Query which subsystems are currently initialized/bound (SDL_WasInit-style). */
oeaf_u32    oeaf_was_init(oeaf_u32 flags);

/* Access the global context (may be NULL if oeaf_init was not called). */
oeaf_ctx*   oeaf_get_ctx(void);

/* SDL_GetError-style helper.
   Returns a pointer to a static buffer owned by OEAF (not thread-safe).
*/
const char* oeaf_get_error(void);

/* SDL_SetError-style helper.
   Sets the global OEAF error string (not thread-safe) and returns -1.
   This is useful for extension modules (image/ttf/mixer) to report errors
   consistently via oeaf_get_error().
*/
oeaf_i32    oeaf_set_error(const char* fmt, ...);

/* Clears the global OEAF error string (equivalent to setting it to ""). */
void        oeaf_clear_error(void);

/* Attach a backend module into the global context (SDL-like "plug it in"). */
oeaf_result oeaf_attach_backend(const oeaf_module_desc* mod);

/* Validate required subsystems against the global context. */
oeaf_result oeaf_require_global(oeaf_u32 provides_mask);

/* SDL_Delay-style sleep. Waits at least ms, possibly longer due to scheduling. */
void        oeaf_delay(oeaf_u32 ms);

/* SDL_GetTicks-style: milliseconds since oeaf_init() (wraps at 32-bit). */
oeaf_u32     oeaf_get_ticks(void);

/* SDL_PumpEvents-style: ask the OS backend to gather pending input/window events
   and push them into OEAF's event queue.
*/
void        oeaf_pump_events(void);

/* SDL_PollEvent-style: returns 1 if an event was returned (or is available if
   event == NULL), or 0 if none are available.
   This may implicitly call oeaf_pump_events(), like SDL_PollEvent().
*/
oeaf_i32    oeaf_poll_event(oeaf_event* event);

#ifdef __cplusplus
}
#endif

#endif /* OEAF_SDL_H */
