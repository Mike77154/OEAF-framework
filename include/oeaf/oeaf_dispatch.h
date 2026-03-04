#ifndef OEAF_DISPATCH_H
#define OEAF_DISPATCH_H

#include "oeaf_types.h"
#include "oeaf_os.h"
#include "oeaf_timer.h"
#include "oeaf_video.h"
#include "oeaf_input.h"
#include "oeaf_audio.h"
#include "oeaf_fs.h"

#ifdef __cplusplus
extern "C" {
#endif

/* These helpers call the currently-bound backend (if any). */

/* OS */
oeaf_result oeaf_os_pump(oeaf_ctx* ctx);
void        oeaf_os_sleep_ms(oeaf_ctx* ctx, oeaf_u32 ms);
void*       oeaf_os_get_native(oeaf_ctx* ctx, const char* what);

/* TIMER */
oeaf_u64    oeaf_timer_counter(oeaf_ctx* ctx);
oeaf_u64    oeaf_timer_frequency(oeaf_ctx* ctx);

/* VIDEO */
oeaf_result oeaf_video_create_window(oeaf_ctx* ctx, const oeaf_window_desc* desc, oeaf_window_handle* out_win);
void        oeaf_video_destroy_window(oeaf_ctx* ctx, oeaf_window_handle win);
void        oeaf_video_set_window_title(oeaf_ctx* ctx, oeaf_window_handle win, const char* title);
void        oeaf_video_get_window_size(oeaf_ctx* ctx, oeaf_window_handle win, oeaf_i32* out_w, oeaf_i32* out_h);
void        oeaf_video_swap_buffers(oeaf_ctx* ctx, oeaf_window_handle win);
void*       oeaf_video_get_native_window(oeaf_ctx* ctx, oeaf_window_handle win, const char* what);
oeaf_result oeaf_video_gl_create_context(oeaf_ctx* ctx, oeaf_window_handle win, void** out_glctx);
void        oeaf_video_gl_make_current(oeaf_ctx* ctx, oeaf_window_handle win, void* glctx);
void        oeaf_video_gl_delete_context(oeaf_ctx* ctx, void* glctx);

/* INPUT */
oeaf_result oeaf_input_update(oeaf_ctx* ctx);
oeaf_i32    oeaf_input_key_down(oeaf_ctx* ctx, oeaf_key key);
oeaf_u32    oeaf_input_mouse_buttons(oeaf_ctx* ctx);
void        oeaf_input_mouse_pos(oeaf_ctx* ctx, oeaf_i32* out_x, oeaf_i32* out_y);
void        oeaf_input_mouse_delta(oeaf_ctx* ctx, oeaf_i32* out_dx, oeaf_i32* out_dy);
void        oeaf_input_set_relative_mouse(oeaf_ctx* ctx, oeaf_i32 enabled);
void        oeaf_input_start_text_input(oeaf_ctx* ctx);
void        oeaf_input_stop_text_input(oeaf_ctx* ctx);

/* AUDIO */
oeaf_result oeaf_audio_open_device(oeaf_ctx* ctx, const oeaf_audio_spec* want, oeaf_audio_spec* out_have, oeaf_audio_device* out_dev);
void        oeaf_audio_close_device(oeaf_ctx* ctx, oeaf_audio_device dev);
void        oeaf_audio_pause_device(oeaf_ctx* ctx, oeaf_audio_device dev, oeaf_i32 paused);

/* FS */
oeaf_i32    oeaf_fs_exists(oeaf_ctx* ctx, const char* path);
oeaf_result oeaf_fs_read_all(oeaf_ctx* ctx, const char* path, void** out_data, size_t* out_size);
oeaf_result oeaf_fs_write_all(oeaf_ctx* ctx, const char* path, const void* data, size_t size);

#ifdef __cplusplus
}
#endif

#endif /* OEAF_DISPATCH_H */
