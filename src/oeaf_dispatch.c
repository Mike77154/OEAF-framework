#include "oeaf/oeaf.h"

/* NOTE: oeaf_ctx is opaque to users, but defined in oeaf_core.c.
   We keep these dispatchers inside the OEAF library so user code doesn't need internals.
*/

/* We need the internal struct layout here: include the internal header by forward declaration trick.
   In this minimal v0.1, we simply redeclare the parts we need via an internal include.
*/
typedef struct oeaf_ctx_internal oeaf_ctx_internal;

/* We cannot access internal fields without seeing the definition.
   So oeaf_core.c exposes a private accessor block via function pointers.
   For v0.1, we take the simplest route: oeaf_core.c will compile this file too,
   and we'll include a private header that defines struct oeaf_ctx.
*/
#include "oeaf_core_private.h"

/* OS */
oeaf_result oeaf_os_pump(oeaf_ctx* ctx) {
    if (!ctx) return OEAF_ERR_BADARGS;
    if (!ctx->os.vtbl || !ctx->os.vtbl->pump) return OEAF_ERR_NOBACKEND;
    return ctx->os.vtbl->pump(ctx->os.self, ctx);
}

void oeaf_os_sleep_ms(oeaf_ctx* ctx, oeaf_u32 ms) {
    if (!ctx || !ctx->os.vtbl || !ctx->os.vtbl->sleep_ms) return;
    ctx->os.vtbl->sleep_ms(ctx->os.self, ctx, ms);
}

void* oeaf_os_get_native(oeaf_ctx* ctx, const char* what) {
    if (!ctx || !ctx->os.vtbl || !ctx->os.vtbl->get_native) return NULL;
    return ctx->os.vtbl->get_native(ctx->os.self, ctx, what);
}

/* TIMER */
oeaf_u64 oeaf_timer_counter(oeaf_ctx* ctx) {
    if (!ctx || !ctx->timer.vtbl || !ctx->timer.vtbl->counter) return 0;
    return ctx->timer.vtbl->counter(ctx->timer.self, ctx);
}

oeaf_u64 oeaf_timer_frequency(oeaf_ctx* ctx) {
    if (!ctx || !ctx->timer.vtbl || !ctx->timer.vtbl->frequency) return 0;
    return ctx->timer.vtbl->frequency(ctx->timer.self, ctx);
}

/* VIDEO */
oeaf_result oeaf_video_create_window(oeaf_ctx* ctx, const oeaf_window_desc* desc, oeaf_window_handle* out_win) {
    if (!ctx || !desc || !out_win) return OEAF_ERR_BADARGS;
    if (!ctx->video.vtbl || !ctx->video.vtbl->create_window) return OEAF_ERR_NOBACKEND;
    return ctx->video.vtbl->create_window(ctx->video.self, ctx, desc, out_win);
}
void oeaf_video_destroy_window(oeaf_ctx* ctx, oeaf_window_handle win) {
    if (!ctx || !ctx->video.vtbl || !ctx->video.vtbl->destroy_window) return;
    ctx->video.vtbl->destroy_window(ctx->video.self, ctx, win);
}
void oeaf_video_set_window_title(oeaf_ctx* ctx, oeaf_window_handle win, const char* title) {
    if (!ctx || !ctx->video.vtbl || !ctx->video.vtbl->set_window_title) return;
    ctx->video.vtbl->set_window_title(ctx->video.self, ctx, win, title);
}
void oeaf_video_get_window_size(oeaf_ctx* ctx, oeaf_window_handle win, oeaf_i32* out_w, oeaf_i32* out_h) {
    if (!ctx || !ctx->video.vtbl || !ctx->video.vtbl->get_window_size) return;
    ctx->video.vtbl->get_window_size(ctx->video.self, ctx, win, out_w, out_h);
}
void oeaf_video_swap_buffers(oeaf_ctx* ctx, oeaf_window_handle win) {
    if (!ctx || !ctx->video.vtbl || !ctx->video.vtbl->swap_buffers) return;
    ctx->video.vtbl->swap_buffers(ctx->video.self, ctx, win);
}
void* oeaf_video_get_native_window(oeaf_ctx* ctx, oeaf_window_handle win, const char* what) {
    if (!ctx || !ctx->video.vtbl || !ctx->video.vtbl->get_native_window) return NULL;
    return ctx->video.vtbl->get_native_window(ctx->video.self, ctx, win, what);
}
oeaf_result oeaf_video_gl_create_context(oeaf_ctx* ctx, oeaf_window_handle win, void** out_glctx) {
    if (!ctx || !out_glctx) return OEAF_ERR_BADARGS;
    if (!ctx->video.vtbl || !ctx->video.vtbl->gl_create_context) return OEAF_ERR_NOBACKEND;
    return ctx->video.vtbl->gl_create_context(ctx->video.self, ctx, win, out_glctx);
}
void oeaf_video_gl_make_current(oeaf_ctx* ctx, oeaf_window_handle win, void* glctx) {
    if (!ctx || !ctx->video.vtbl || !ctx->video.vtbl->gl_make_current) return;
    ctx->video.vtbl->gl_make_current(ctx->video.self, ctx, win, glctx);
}
void oeaf_video_gl_delete_context(oeaf_ctx* ctx, void* glctx) {
    if (!ctx || !ctx->video.vtbl || !ctx->video.vtbl->gl_delete_context) return;
    ctx->video.vtbl->gl_delete_context(ctx->video.self, ctx, glctx);
}

/* INPUT */
oeaf_result oeaf_input_update(oeaf_ctx* ctx) {
    if (!ctx) return OEAF_ERR_BADARGS;
    if (!ctx->input.vtbl || !ctx->input.vtbl->update) return OEAF_ERR_NOBACKEND;
    return ctx->input.vtbl->update(ctx->input.self, ctx);
}
oeaf_i32 oeaf_input_key_down(oeaf_ctx* ctx, oeaf_key key) {
    if (!ctx || !ctx->input.vtbl || !ctx->input.vtbl->key_down) return 0;
    return ctx->input.vtbl->key_down(ctx->input.self, ctx, key);
}
oeaf_u32 oeaf_input_mouse_buttons(oeaf_ctx* ctx) {
    if (!ctx || !ctx->input.vtbl || !ctx->input.vtbl->mouse_buttons) return 0;
    return ctx->input.vtbl->mouse_buttons(ctx->input.self, ctx);
}
void oeaf_input_mouse_pos(oeaf_ctx* ctx, oeaf_i32* out_x, oeaf_i32* out_y) {
    if (!ctx || !ctx->input.vtbl || !ctx->input.vtbl->mouse_pos) return;
    ctx->input.vtbl->mouse_pos(ctx->input.self, ctx, out_x, out_y);
}
void oeaf_input_mouse_delta(oeaf_ctx* ctx, oeaf_i32* out_dx, oeaf_i32* out_dy) {
    if (!ctx || !ctx->input.vtbl || !ctx->input.vtbl->mouse_delta) return;
    ctx->input.vtbl->mouse_delta(ctx->input.self, ctx, out_dx, out_dy);
}
void oeaf_input_set_relative_mouse(oeaf_ctx* ctx, oeaf_i32 enabled) {
    if (!ctx || !ctx->input.vtbl || !ctx->input.vtbl->set_relative_mouse) return;
    ctx->input.vtbl->set_relative_mouse(ctx->input.self, ctx, enabled);
}
void oeaf_input_start_text_input(oeaf_ctx* ctx) {
    if (!ctx || !ctx->input.vtbl || !ctx->input.vtbl->start_text_input) return;
    ctx->input.vtbl->start_text_input(ctx->input.self, ctx);
}
void oeaf_input_stop_text_input(oeaf_ctx* ctx) {
    if (!ctx || !ctx->input.vtbl || !ctx->input.vtbl->stop_text_input) return;
    ctx->input.vtbl->stop_text_input(ctx->input.self, ctx);
}

/* AUDIO */
oeaf_result oeaf_audio_open_device(oeaf_ctx* ctx, const oeaf_audio_spec* want, oeaf_audio_spec* out_have, oeaf_audio_device* out_dev) {
    if (!ctx || !want || !out_have || !out_dev) return OEAF_ERR_BADARGS;
    if (!ctx->audio.vtbl || !ctx->audio.vtbl->open_device) return OEAF_ERR_NOBACKEND;
    return ctx->audio.vtbl->open_device(ctx->audio.self, ctx, want, out_have, out_dev);
}
void oeaf_audio_close_device(oeaf_ctx* ctx, oeaf_audio_device dev) {
    if (!ctx || !ctx->audio.vtbl || !ctx->audio.vtbl->close_device) return;
    ctx->audio.vtbl->close_device(ctx->audio.self, ctx, dev);
}
void oeaf_audio_pause_device(oeaf_ctx* ctx, oeaf_audio_device dev, oeaf_i32 paused) {
    if (!ctx || !ctx->audio.vtbl || !ctx->audio.vtbl->pause_device) return;
    ctx->audio.vtbl->pause_device(ctx->audio.self, ctx, dev, paused);
}

/* FS */
oeaf_i32 oeaf_fs_exists(oeaf_ctx* ctx, const char* path) {
    if (!ctx || !ctx->fs.vtbl || !ctx->fs.vtbl->exists) return -1;
    return ctx->fs.vtbl->exists(ctx->fs.self, ctx, path);
}
oeaf_result oeaf_fs_read_all(oeaf_ctx* ctx, const char* path, void** out_data, size_t* out_size) {
    if (!ctx || !path || !out_data || !out_size) return OEAF_ERR_BADARGS;
    if (!ctx->fs.vtbl || !ctx->fs.vtbl->read_all) return OEAF_ERR_NOBACKEND;
    return ctx->fs.vtbl->read_all(ctx->fs.self, ctx, path, out_data, out_size);
}
oeaf_result oeaf_fs_write_all(oeaf_ctx* ctx, const char* path, const void* data, size_t size) {
    if (!ctx || !path || !data) return OEAF_ERR_BADARGS;
    if (!ctx->fs.vtbl || !ctx->fs.vtbl->write_all) return OEAF_ERR_NOBACKEND;
    return ctx->fs.vtbl->write_all(ctx->fs.self, ctx, path, data, size);
}
