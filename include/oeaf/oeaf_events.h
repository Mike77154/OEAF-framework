#ifndef OEAF_EVENTS_H
#define OEAF_EVENTS_H

#include "oeaf_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Event types are SDL-like but OEAF-owned. */
typedef enum oeaf_event_type {
    OEAF_EVENT_NONE = 0,
    OEAF_EVENT_QUIT,

    OEAF_EVENT_WINDOW,
    OEAF_EVENT_KEY,
    OEAF_EVENT_TEXT,
    OEAF_EVENT_MOUSE_MOVE,
    OEAF_EVENT_MOUSE_BUTTON,
    OEAF_EVENT_MOUSE_WHEEL,

    /* Extend later: controller, drop, touch, etc */
} oeaf_event_type;

typedef enum oeaf_window_event_kind {
    OEAF_WINDOWEVENT_NONE = 0,
    OEAF_WINDOWEVENT_CLOSE,
    OEAF_WINDOWEVENT_RESIZED,
    OEAF_WINDOWEVENT_FOCUS_GAINED,
    OEAF_WINDOWEVENT_FOCUS_LOST
} oeaf_window_event_kind;

typedef struct oeaf_event_window {
    oeaf_window_event_kind kind;
    oeaf_i32 w, h; /* for resize */
} oeaf_event_window;

typedef struct oeaf_event_key {
    oeaf_key key;
    oeaf_i32 pressed; /* 1 down, 0 up */
    oeaf_i32 repeat;  /* 1 if auto-repeat */
    oeaf_u32 mods;    /* backend-defined modifier bits (optional) */
} oeaf_event_key;

typedef struct oeaf_event_text {
    /* UTF-8 bytes, null-terminated (best effort). */
    char utf8[32];
} oeaf_event_text;

typedef struct oeaf_event_mouse_move {
    oeaf_i32 x, y;       /* absolute (if backend provides) */
    oeaf_i32 dx, dy;     /* relative */
} oeaf_event_mouse_move;

typedef struct oeaf_event_mouse_button {
    oeaf_u32 button_mask; /* oeaf_mouse_button bits */
    oeaf_i32 pressed;     /* 1 down, 0 up */
} oeaf_event_mouse_button;

typedef struct oeaf_event_mouse_wheel {
    oeaf_i32 dx, dy;
} oeaf_event_mouse_wheel;

typedef struct oeaf_event {
    oeaf_event_type type;
    union {
        oeaf_event_window window;
        oeaf_event_key key;
        oeaf_event_text text;
        oeaf_event_mouse_move mouse_move;
        oeaf_event_mouse_button mouse_button;
        oeaf_event_mouse_wheel mouse_wheel;
    } u;
} oeaf_event;

/* Core-owned event queue (so backends can push, engine can poll) */
oeaf_result oeaf_events_push(oeaf_ctx* ctx, const oeaf_event* e);

/* Poll the next event from the queue for a specific context.
   - On success, returns OEAF_OK.
   - If the queue is empty, out_e->type will be OEAF_EVENT_NONE.

   This is the "explicit context" variant.
   If you want an SDL-like API (no ctx param), include oeaf/oeaf_sdl.h.
*/
oeaf_result oeaf_poll_event_ctx(oeaf_ctx* ctx, oeaf_event* out_e);

#ifdef __cplusplus
}
#endif

#endif /* OEAF_EVENTS_H */
