#ifndef OEAF_AUDIO_H
#define OEAF_AUDIO_H

#include "oeaf_types.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum oeaf_audio_format {
    OEAF_AUDIO_S16 = 1,
    OEAF_AUDIO_F32 = 2
} oeaf_audio_format;

typedef void (*oeaf_audio_callback)(void* userdata, void* stream, oeaf_u32 bytes);

typedef struct oeaf_audio_spec {
    oeaf_u32 sample_rate;
    oeaf_u32 channels;
    oeaf_audio_format format;
    oeaf_u32 buffer_frames; /* suggested */
    oeaf_audio_callback callback;
    void* userdata;
} oeaf_audio_spec;

typedef struct oeaf_audio_device {
    void* ptr; /* backend-defined */
} oeaf_audio_device;

typedef struct oeaf_audio_vtbl {
    oeaf_result (*open_device)(void* self, oeaf_ctx* ctx, const oeaf_audio_spec* want, oeaf_audio_spec* out_have, oeaf_audio_device* out_dev);
    void        (*close_device)(void* self, oeaf_ctx* ctx, oeaf_audio_device dev);

    void        (*pause_device)(void* self, oeaf_ctx* ctx, oeaf_audio_device dev, oeaf_i32 paused);
} oeaf_audio_vtbl;

typedef struct oeaf_audio_iface {
    void* self;
    const oeaf_audio_vtbl* vtbl;
} oeaf_audio_iface;

#ifdef __cplusplus
}
#endif

#endif /* OEAF_AUDIO_H */
