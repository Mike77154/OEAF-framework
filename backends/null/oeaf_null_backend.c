#include <stdlib.h>
#include <string.h>

#include "oeaf/oeaf_events.h"
#include "oeaf/oeaf_backend.h"

#if defined(_WIN32)
  #define WIN32_LEAN_AND_MEAN
  #include <windows.h>
#else
  #include <time.h>
#endif

/* --- state --- */
typedef struct null_state {
    int dummy;
} null_state;

/* --- TIMER --- */
static oeaf_u64 null_counter(void* self, oeaf_ctx* ctx) {
    (void)self; (void)ctx;
#if defined(_WIN32)
    LARGE_INTEGER li;
    QueryPerformanceCounter(&li);
    return (oeaf_u64)li.QuadPart;
#else
    /* Standard C fallback (not true hi-res, but portable) */
    return (oeaf_u64)clock();
#endif
}

static oeaf_u64 null_frequency(void* self, oeaf_ctx* ctx) {
    (void)self; (void)ctx;
#if defined(_WIN32)
    LARGE_INTEGER li;
    QueryPerformanceFrequency(&li);
    return (oeaf_u64)li.QuadPart;
#else
    return (oeaf_u64)CLOCKS_PER_SEC;
#endif
}

static const oeaf_timer_vtbl NULL_TIMER_VTBL = {
    null_counter,
    null_frequency
};

/* --- OS --- */
static oeaf_result null_os_init(void* self, oeaf_ctx* ctx) {
    (void)self; (void)ctx;
    return OEAF_OK;
}
static void null_os_shutdown(void* self, oeaf_ctx* ctx) {
    (void)self; (void)ctx;
}
static oeaf_result null_os_pump(void* self, oeaf_ctx* ctx) {
    (void)self; (void)ctx;
    /* No events are generated automatically. */
    return OEAF_OK;
}

static void null_os_sleep(void* self, oeaf_ctx* ctx, oeaf_u32 ms) {
    (void)self; (void)ctx;
#if defined(_WIN32)
    Sleep((DWORD)ms);
#else
    /* Portable busy-wait fallback */
    oeaf_u64 start = (oeaf_u64)clock();
    oeaf_u64 freq = (oeaf_u64)CLOCKS_PER_SEC;
    oeaf_u64 target = start + (oeaf_u64)((ms / 1000.0) * (double)freq);
    while ((oeaf_u64)clock() < target) {
        /* spin */
    }
#endif
}

static void* null_os_get_native(void* self, oeaf_ctx* ctx, const char* what) {
    (void)self; (void)ctx; (void)what;
    return NULL;
}

static const oeaf_os_vtbl NULL_OS_VTBL = {
    null_os_init,
    null_os_shutdown,
    null_os_pump,
    null_os_sleep,
    null_os_get_native
};

/* --- INPUT --- */
static oeaf_result null_input_update(void* self, oeaf_ctx* ctx) {
    (void)self; (void)ctx;
    return OEAF_OK;
}
static oeaf_i32 null_key_down(void* self, oeaf_ctx* ctx, oeaf_key key) {
    (void)self; (void)ctx; (void)key;
    return 0;
}
static oeaf_u32 null_mouse_buttons(void* self, oeaf_ctx* ctx) {
    (void)self; (void)ctx;
    return 0;
}
static void null_mouse_pos(void* self, oeaf_ctx* ctx, oeaf_i32* x, oeaf_i32* y) {
    (void)self; (void)ctx;
    if (x) *x = 0;
    if (y) *y = 0;
}
static void null_mouse_delta(void* self, oeaf_ctx* ctx, oeaf_i32* dx, oeaf_i32* dy) {
    (void)self; (void)ctx;
    if (dx) *dx = 0;
    if (dy) *dy = 0;
}
static void null_set_relmouse(void* self, oeaf_ctx* ctx, oeaf_i32 enabled) {
    (void)self; (void)ctx; (void)enabled;
}
static void null_start_text(void* self, oeaf_ctx* ctx) { (void)self; (void)ctx; }
static void null_stop_text(void* self, oeaf_ctx* ctx) { (void)self; (void)ctx; }

static const oeaf_input_vtbl NULL_INPUT_VTBL = {
    null_input_update,
    null_key_down,
    null_mouse_buttons,
    null_mouse_pos,
    null_mouse_delta,
    null_set_relmouse,
    null_start_text,
    null_stop_text
};

/* --- attach/detach --- */
static oeaf_result null_attach(oeaf_ctx* ctx, void** out_state) {
    (void)ctx;
    null_state* st = (null_state*)calloc(1, sizeof(null_state));
    if (!st) return OEAF_ERR_NOMEM;
    *out_state = st;
    return OEAF_OK;
}

static void null_detach(oeaf_ctx* ctx, void* state) {
    (void)ctx;
    free(state);
}

const oeaf_module_desc OEAF_NULL_BACKEND = {
    "null",
    OEAF_ABI_VERSION,
    OEAF_PROVIDES_OS | OEAF_PROVIDES_TIMER | OEAF_PROVIDES_INPUT,

    null_attach,
    null_detach,

    { NULL, &NULL_OS_VTBL },
    { NULL, &NULL_TIMER_VTBL },
    { NULL, NULL }, /* video */
    { NULL, &NULL_INPUT_VTBL },
    { NULL, NULL }, /* audio */
    { NULL, NULL }  /* fs */
};
