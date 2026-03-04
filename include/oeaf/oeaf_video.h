#ifndef OEAF_VIDEO_H
#define OEAF_VIDEO_H

#include "oeaf_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Window flags (SDL-ish) */
typedef enum oeaf_window_flags {
    OEAF_WINDOW_RESIZABLE   = OEAF_BIT(0),
    OEAF_WINDOW_HIGHDPI     = OEAF_BIT(1),
    OEAF_WINDOW_FULLSCREEN  = OEAF_BIT(2),
    OEAF_WINDOW_BORDERLESS  = OEAF_BIT(3),
    OEAF_WINDOW_VSYNC       = OEAF_BIT(4),
    OEAF_WINDOW_OPENGL      = OEAF_BIT(5),
    OEAF_WINDOW_VULKAN      = OEAF_BIT(6)
} oeaf_window_flags;

typedef struct oeaf_window_desc {
    const char* title;
    oeaf_i32 w, h;
    oeaf_u32 flags; /* oeaf_window_flags */
} oeaf_window_desc;

/* Opaque handle type (backend-defined). */
typedef struct oeaf_window_handle {
    void* ptr;
} oeaf_window_handle;

typedef struct oeaf_video_vtbl {
    oeaf_result (*create_window)(void* self, oeaf_ctx* ctx, const oeaf_window_desc* desc, oeaf_window_handle* out_win);
    void        (*destroy_window)(void* self, oeaf_ctx* ctx, oeaf_window_handle win);

    void        (*set_window_title)(void* self, oeaf_ctx* ctx, oeaf_window_handle win, const char* title);
    void        (*get_window_size)(void* self, oeaf_ctx* ctx, oeaf_window_handle win, oeaf_i32* out_w, oeaf_i32* out_h);

    /* Presentation swap (for GL, or backend-specific) */
    void        (*swap_buffers)(void* self, oeaf_ctx* ctx, oeaf_window_handle win);

    /* Optional native handles (HWND, X11 Window, etc) */
    void*       (*get_native_window)(void* self, oeaf_ctx* ctx, oeaf_window_handle win, const char* what);

    /* Optional: GL context management */
    oeaf_result (*gl_create_context)(void* self, oeaf_ctx* ctx, oeaf_window_handle win, void** out_glctx);
    void        (*gl_make_current)(void* self, oeaf_ctx* ctx, oeaf_window_handle win, void* glctx);
    void        (*gl_delete_context)(void* self, oeaf_ctx* ctx, void* glctx);
} oeaf_video_vtbl;

typedef struct oeaf_video_iface {
    void* self;
    const oeaf_video_vtbl* vtbl;
} oeaf_video_iface;

#ifdef __cplusplus
}
#endif

#endif /* OEAF_VIDEO_H */
