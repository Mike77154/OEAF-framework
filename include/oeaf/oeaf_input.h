#ifndef OEAF_INPUT_H
#define OEAF_INPUT_H

#include "oeaf_types.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct oeaf_input_vtbl {
    oeaf_result (*update)(void* self, oeaf_ctx* ctx);

    /* State queries (optional if you only use events) */
    oeaf_i32    (*key_down)(void* self, oeaf_ctx* ctx, oeaf_key key);
    oeaf_u32    (*mouse_buttons)(void* self, oeaf_ctx* ctx);
    void        (*mouse_pos)(void* self, oeaf_ctx* ctx, oeaf_i32* out_x, oeaf_i32* out_y);
    void        (*mouse_delta)(void* self, oeaf_ctx* ctx, oeaf_i32* out_dx, oeaf_i32* out_dy);

    /* Optional */
    void        (*set_relative_mouse)(void* self, oeaf_ctx* ctx, oeaf_i32 enabled);
    void        (*start_text_input)(void* self, oeaf_ctx* ctx);
    void        (*stop_text_input)(void* self, oeaf_ctx* ctx);
} oeaf_input_vtbl;

typedef struct oeaf_input_iface {
    void* self;
    const oeaf_input_vtbl* vtbl;
} oeaf_input_iface;

#ifdef __cplusplus
}
#endif

#endif /* OEAF_INPUT_H */
