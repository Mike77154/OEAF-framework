#ifndef OEAF_H
#define OEAF_H

#include "oeaf_version.h"
#include "oeaf_types.h"
#include "oeaf_events.h"
#include "oeaf_backend.h"
#include "oeaf_dispatch.h"
#include "oeaf_sdl.h" /* SDL-like convenience layer */

/* Optional SDL extension equivalents (SDL_image / SDL_ttf / SDL_mixer).
   These are header-only contracts; link liboeaf to use their implementations.
*/
#include "oeaf_pixels.h"
#include "oeaf_rwops.h"
#include "oeaf_image.h"
#include "oeaf_ttf.h"
#include "oeaf_mixer.h"
#include "oeaf_sdl_audio.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Context creation/destruction.
   OEAF core owns the event queue and module list; backends own their states.
*/
oeaf_ctx*   oeaf_create(void);
void        oeaf_destroy(oeaf_ctx* ctx);

/* Optional: a simple helper to compute dt using TIMER (if you don't want to do it yourself). */
oeaf_result oeaf_step_timer(oeaf_ctx* ctx, oeaf_f32* out_dt);

/* Debug helper: what backends are bound? */
const char* oeaf_bound_summary(oeaf_ctx* ctx);

#ifdef __cplusplus
}
#endif

#endif /* OEAF_H */
