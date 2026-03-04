#include <stdio.h>
#include <stdarg.h>
#include <string.h>

#include "oeaf/oeaf_sdl.h"

/* Internal layout access (this file is part of liboeaf). */
#include "oeaf_core_private.h"

/* We ship a built-in baseline backend (null) inside liboeaf so oeaf_init() can
   work out-of-the-box. */
extern const oeaf_module_desc OEAF_NULL_BACKEND;

/* SDL-style global state */
static oeaf_ctx* g_ctx = NULL;
static oeaf_u32  g_refcount = 0;

/* Tick origin (SDL_GetTicks semantics) */
static oeaf_u64 g_ticks_start_counter = 0;
static oeaf_u64 g_ticks_start_freq = 0;

/* Non-thread-safe error buffer (SDL_GetError style) */
static char g_error[256] = {0};

static void oeaf_vset_errorf(const char* fmt, va_list ap) {
    if (!fmt) {
        g_error[0] = '\0';
        return;
    }
    vsnprintf(g_error, sizeof(g_error), fmt, ap);
}

oeaf_i32 oeaf_set_error(const char* fmt, ...) {
    if (!fmt) {
        g_error[0] = '\0';
        return -1;
    }
    va_list ap;
    va_start(ap, fmt);
    oeaf_vset_errorf(fmt, ap);
    va_end(ap);
    return -1;
}

void oeaf_clear_error(void) {
    g_error[0] = '\0';
}

const char* oeaf_get_error(void) {
    return g_error;
}

/* Map SDL2-style init flags (OEAF_INIT_*) to OEAF's internal provides mask.
   We keep the public API binary-compatible with SDL2 flag values, while OEAF
   itself uses its own compact OEAF_PROVIDES_* bitset.
*/
static oeaf_u32 oeaf_initflags_to_provides(oeaf_u32 flags) {
    oeaf_u32 provides = 0;

    if (flags & OEAF_INIT_TIMER) provides |= OEAF_PROVIDES_TIMER;
    if (flags & OEAF_INIT_AUDIO) provides |= OEAF_PROVIDES_AUDIO;
    if (flags & OEAF_INIT_VIDEO) {
        provides |= OEAF_PROVIDES_VIDEO;
        /* SDL_INIT_VIDEO implies SDL_INIT_EVENTS */
        provides |= OEAF_PROVIDES_OS;
    }

    /* SDL's event subsystem is the closest equivalent to OEAF's OS/pump.
       Note: SDL_INIT_VIDEO implies events; we don't need to special-case
       that here because oeaf_init callers can set either/both.
    */
    if (flags & OEAF_INIT_EVENTS) provides |= OEAF_PROVIDES_OS;

    /* SDL device subsystems are best-effort mapped onto OEAF input.
       This keeps common init patterns (SDL_INIT_EVERYTHING) working.
    */
    if (flags & (OEAF_INIT_JOYSTICK | OEAF_INIT_GAMECONTROLLER | OEAF_INIT_HAPTIC | OEAF_INIT_SENSOR)) {
        provides |= OEAF_PROVIDES_INPUT;
        /* and those subsystems also imply the event loop in SDL */
        provides |= OEAF_PROVIDES_OS;
    }

    /* SDL_INIT_NOPARACHUTE is ignored by SDL2; we ignore it too. */
    (void)flags;
    return provides;
}

static oeaf_u32 oeaf_known_initflags_mask(void) {
    return (oeaf_u32)(
        OEAF_INIT_TIMER | OEAF_INIT_AUDIO | OEAF_INIT_VIDEO | OEAF_INIT_EVENTS |
        OEAF_INIT_JOYSTICK | OEAF_INIT_HAPTIC | OEAF_INIT_GAMECONTROLLER | OEAF_INIT_SENSOR |
        OEAF_INIT_NOPARACHUTE
    );
}

oeaf_ctx* oeaf_get_ctx(void) {
    return g_ctx;
}

oeaf_result oeaf_attach_backend(const oeaf_module_desc* mod) {
    if (!g_ctx) {
        oeaf_set_error("OEAF not initialized (call oeaf_init first)");
        return OEAF_ERR;
    }
    if (!mod) {
        oeaf_set_error("Bad argument: mod=NULL");
        return OEAF_ERR_BADARGS;
    }
    oeaf_result r = oeaf_attach_module(g_ctx, mod);
    if (r != OEAF_OK) {
        oeaf_set_error("oeaf_attach_module failed (name=%s, r=%d)", mod->name ? mod->name : "(null)", (int)r);
    }
    return r;
}

oeaf_result oeaf_require_global(oeaf_u32 provides_mask) {
    if (!g_ctx) {
        oeaf_set_error("OEAF not initialized (call oeaf_init first)");
        return OEAF_ERR;
    }

    /* Public API takes SDL2-style init flags (OEAF_INIT_*). */
    {
        oeaf_u32 unknown = provides_mask & ~oeaf_known_initflags_mask();
        if (unknown) {
            oeaf_set_error("Unsupported init flag bits: 0x%08x", (unsigned)unknown);
            return OEAF_ERR_UNSUPPORTED;
        }
    }

    /* Translate into OEAF_PROVIDES_* for the core. */
    oeaf_u32 core_mask = oeaf_initflags_to_provides(provides_mask);
    oeaf_result r = oeaf_require(g_ctx, core_mask);
    if (r != OEAF_OK) {
        oeaf_set_error("Missing required backend(s) (mask=0x%08x)", (unsigned)provides_mask);
    }
    return r;
}

oeaf_u32 oeaf_was_init(oeaf_u32 flags) {
    if (!g_ctx) return 0;

    oeaf_u32 have = 0;
    /* NOTE: This is inside liboeaf; oeaf_ctx layout is private but stable here. */
    /* We only test the vtbl presence, which is the binding indicator. */
    {
        /* Forward declarations are already satisfied through oeaf/oeaf.h */
        /* (ctx is opaque to public users, but this .c is inside the lib). */
    }

    /* SDL_WasInit returns SDL_INIT_* masks.
       We approximate these based on what's currently bound.
    */
    if (g_ctx->timer.vtbl) have |= OEAF_INIT_TIMER;
    if (g_ctx->audio.vtbl) have |= OEAF_INIT_AUDIO;
    if (g_ctx->video.vtbl) have |= OEAF_INIT_VIDEO;

    /* In OEAF, the closest equivalent to SDL's event subsystem is having an OS pump.
       SDL_INIT_VIDEO implies events; we expose that here.
    */
    if (g_ctx->os.vtbl || g_ctx->video.vtbl) have |= OEAF_INIT_EVENTS;

    /* Best-effort mapping: if OEAF has an input backend, treat SDL's device subsystems
       as "initialized" (it doesn't guarantee hardware exists, same as SDL).
    */
    if (g_ctx->input.vtbl) {
        have |= OEAF_INIT_JOYSTICK;
        have |= OEAF_INIT_GAMECONTROLLER;
        have |= OEAF_INIT_HAPTIC;
        have |= OEAF_INIT_SENSOR;
    }

    /* SDL_WasInit() doesn't include NOPARACHUTE in its return mask. */
    have &= ~OEAF_INIT_NOPARACHUTE;

    if (flags == 0) return have;
    return have & (flags & ~OEAF_INIT_NOPARACHUTE);
}

oeaf_result oeaf_init(oeaf_u32 flags) {
    /* SDL_Init allows multiple calls; we do a simple refcount. */
    if (g_refcount == 0) {
        g_ctx = oeaf_create();
        if (!g_ctx) {
            oeaf_set_error("oeaf_create failed (out of memory?)");
            return OEAF_ERR_NOMEM;
        }

        /* Baseline backend so delay/ticks/pump work even in headless tests. */
        {
            oeaf_result r = oeaf_attach_module(g_ctx, &OEAF_NULL_BACKEND);
            if (r != OEAF_OK) {
                oeaf_set_error("Failed to attach built-in null backend (r=%d)", (int)r);
                oeaf_destroy(g_ctx);
                g_ctx = NULL;
                return r;
            }
        }

        /* Ensure OS+TIMER baseline exists (null backend provides both). */
        {
            oeaf_result r = oeaf_require(g_ctx, OEAF_PROVIDES_OS | OEAF_PROVIDES_TIMER);
            if (r != OEAF_OK) {
                oeaf_set_error("Baseline backends missing (OS/TIMER)");
                oeaf_destroy(g_ctx);
                g_ctx = NULL;
                return r;
            }
        }

        /* Tick origin = moment of successful init (SDL_GetTicks semantics). */
        g_ticks_start_counter = oeaf_timer_counter(g_ctx);
        g_ticks_start_freq = oeaf_timer_frequency(g_ctx);
        if (g_ticks_start_freq == 0) {
            /* Keep working, but ticks will be 0. */
            oeaf_set_error("Timer frequency is 0; oeaf_get_ticks will return 0");
        } else {
            oeaf_clear_error();
        }
    }

    g_refcount++;

    /* Validate requested subsystems (SDL_Init(flags) behavior). */
    if (flags != 0) {
        oeaf_result r = oeaf_require_global(flags);
        if (r != OEAF_OK) return r;
    }

    return OEAF_OK;
}

void oeaf_quit(void) {
    if (g_refcount == 0) return;

    g_refcount--;
    if (g_refcount > 0) return;

    if (g_ctx) {
        oeaf_destroy(g_ctx);
        g_ctx = NULL;
    }
    g_ticks_start_counter = 0;
    g_ticks_start_freq = 0;
    oeaf_clear_error();
}

void oeaf_delay(oeaf_u32 ms) {
    if (!g_ctx) return;

    /* Prefer backend-provided sleep. */
    if (g_ctx->os.vtbl && g_ctx->os.vtbl->sleep_ms) {
        oeaf_os_sleep_ms(g_ctx, ms);
        return;
    }

    /* Fallback: busy-wait using TIMER (portable, but wastes CPU). */
    if (!g_ctx->timer.vtbl || !g_ctx->timer.vtbl->counter || !g_ctx->timer.vtbl->frequency) return;

    oeaf_u64 freq = oeaf_timer_frequency(g_ctx);
    if (freq == 0) return;

    oeaf_u64 start = oeaf_timer_counter(g_ctx);
    oeaf_u64 target = start + (oeaf_u64)((ms * (oeaf_u64)freq) / 1000u);
    while (oeaf_timer_counter(g_ctx) < target) {
        /* spin */
    }
}

oeaf_u32 oeaf_get_ticks(void) {
    if (!g_ctx) return 0;
    if (g_ticks_start_freq == 0) return 0;

    oeaf_u64 now = oeaf_timer_counter(g_ctx);
    oeaf_u64 delta = now - g_ticks_start_counter;
    oeaf_u64 ms = (delta * 1000u) / g_ticks_start_freq;
    return (oeaf_u32)ms;
}

void oeaf_pump_events(void) {
    if (!g_ctx) return;
    /* Mirror SDL_PumpEvents: update the queue from devices/OS. */
    oeaf_result r = oeaf_os_pump(g_ctx);
    if (r != OEAF_OK) {
        oeaf_set_error("oeaf_os_pump failed (r=%d)", (int)r);
    }
}

oeaf_i32 oeaf_poll_event(oeaf_event* event) {
    if (!g_ctx) {
        oeaf_set_error("OEAF not initialized (call oeaf_init first)");
        return 0;
    }

    /* SDL allows event==NULL to query availability without removal.
       We can do this cheaply because we're inside the library and can see q.count.
    */
    if (event == NULL) {
        if (g_ctx->q.count == 0) {
            oeaf_pump_events();
        }
        return (g_ctx->q.count != 0) ? 1 : 0;
    }

    /* First try to pop without pumping. */
    oeaf_result r = oeaf_poll_event_ctx(g_ctx, event);
    if (r != OEAF_OK) {
        oeaf_set_error("oeaf_poll_event_ctx failed (r=%d)", (int)r);
        event->type = OEAF_EVENT_NONE;
        return 0;
    }

    if (event->type != OEAF_EVENT_NONE) {
        return 1;
    }

    /* Queue empty: pump once and try again (SDL_PollEvent may implicitly pump). */
    oeaf_pump_events();

    r = oeaf_poll_event_ctx(g_ctx, event);
    if (r != OEAF_OK) {
        oeaf_set_error("oeaf_poll_event_ctx failed after pump (r=%d)", (int)r);
        event->type = OEAF_EVENT_NONE;
        return 0;
    }

    return (event->type != OEAF_EVENT_NONE) ? 1 : 0;
}