#include <stdlib.h>
#include <string.h>

#include "oeaf/oeaf_sdl_audio.h"
#include "oeaf/oeaf_sdl.h"      /* oeaf_get_ctx / oeaf_set_error */
#include "oeaf/oeaf_dispatch.h" /* oeaf_audio_* */

typedef struct oeaf_audio_wrap {
    oeaf_ctx* ctx;

    oeaf_audio_device backend_dev;
    oeaf_audio_spec   backend_have;

    oeaf_AudioCallback user_cb;
    void*             user_ud;

    /* Simple byte FIFO for queue-audio (not thread-safe).
       Prefer queueing while paused.
    */
    oeaf_u8* qbuf;
    size_t   qcap;
    size_t   qhead;
    size_t   qtail;
    size_t   qsize;
} oeaf_audio_wrap;

static oeaf_audio_format map_to_backend_format(oeaf_AudioFormat fmt) {
    if (fmt == OEAF_AUDIO_F32SYS || fmt == OEAF_AUDIO_F32LSB) return OEAF_AUDIO_F32;
    /* Default */
    return OEAF_AUDIO_S16;
}

static oeaf_AudioFormat map_from_backend_format(oeaf_audio_format fmt) {
    if (fmt == OEAF_AUDIO_F32) return OEAF_AUDIO_F32SYS;
    return OEAF_AUDIO_S16SYS;
}

static oeaf_u32 backend_bytes_per_sample(oeaf_audio_format fmt) {
    return (fmt == OEAF_AUDIO_F32) ? 4u : 2u;
}

static void queue_pop(oeaf_audio_wrap* w, oeaf_u8* out, size_t bytes) {
    if (!w || !out || bytes == 0) return;
    size_t n = bytes;
    if (n > w->qsize) n = w->qsize;

    /* Pop in up to two segments (wrap-around). */
    size_t first = n;
    size_t cap = w->qcap;
    if (cap == 0) {
        memset(out, 0, bytes);
        return;
    }
    size_t contiguous = cap - w->qhead;
    if (first > contiguous) first = contiguous;
    if (first > 0) {
        memcpy(out, w->qbuf + w->qhead, first);
        w->qhead = (w->qhead + first) % cap;
        w->qsize -= first;
    }
    size_t remain = n - first;
    if (remain > 0) {
        memcpy(out + first, w->qbuf + w->qhead, remain);
        w->qhead = (w->qhead + remain) % cap;
        w->qsize -= remain;
    }

    /* Fill rest with silence */
    if (n < bytes) {
        memset(out + n, 0, bytes - n);
    }
}

static oeaf_i32 queue_push(oeaf_audio_wrap* w, const oeaf_u8* data, size_t bytes) {
    if (!w || !data || bytes == 0) return 0;

    /* Ensure capacity: grow to fit qsize+bytes */
    size_t need = w->qsize + bytes;
    if (need > w->qcap) {
        size_t ncap = (w->qcap == 0) ? 4096 : w->qcap;
        while (ncap < need) ncap *= 2;

        oeaf_u8* nbuf = (oeaf_u8*)malloc(ncap);
        if (!nbuf) return -1;

        /* Repack existing data into linear buffer */
        size_t copied = 0;
        if (w->qsize > 0 && w->qcap > 0) {
            size_t first = w->qsize;
            size_t contiguous = w->qcap - w->qhead;
            if (first > contiguous) first = contiguous;
            memcpy(nbuf, w->qbuf + w->qhead, first);
            copied += first;
            size_t remain = w->qsize - first;
            if (remain > 0) {
                memcpy(nbuf + copied, w->qbuf, remain);
                copied += remain;
            }
        }

        free(w->qbuf);
        w->qbuf = nbuf;
        w->qcap = ncap;
        w->qhead = 0;
        w->qtail = copied % ncap;
        /* qsize unchanged */
    }

    /* Push in up to two segments */
    size_t cap = w->qcap;
    size_t first = bytes;
    size_t contiguous = cap - w->qtail;
    if (first > contiguous) first = contiguous;
    memcpy(w->qbuf + w->qtail, data, first);
    w->qtail = (w->qtail + first) % cap;
    size_t remain = bytes - first;
    if (remain > 0) {
        memcpy(w->qbuf + w->qtail, data + first, remain);
        w->qtail = (w->qtail + remain) % cap;
    }
    w->qsize += bytes;
    return 0;
}

static void oeaf_audio_wrapper_cb(void* userdata, void* stream, oeaf_u32 bytes) {
    oeaf_audio_wrap* w = (oeaf_audio_wrap*)userdata;
    if (!w || !stream) return;
    if (w->user_cb) {
        w->user_cb(w->user_ud, (oeaf_u8*)stream, (int)bytes);
        return;
    }
    /* queue mode */
    queue_pop(w, (oeaf_u8*)stream, (size_t)bytes);
}

oeaf_AudioDeviceID oeaf_OpenAudioDevice(
    const char* device,
    oeaf_i32 iscapture,
    const oeaf_AudioSpec* desired,
    oeaf_AudioSpec* obtained,
    oeaf_i32 allowed_changes
) {
    (void)device;
    (void)iscapture;
    (void)allowed_changes;

    oeaf_AudioDeviceID out;
    out.ptr = NULL;

    oeaf_ctx* ctx = oeaf_get_ctx();
    if (!ctx) {
        oeaf_set_error("AUDIO: OEAF not initialized (call oeaf_init first)");
        return out;
    }
    if (!desired) {
        oeaf_set_error("AUDIO: desired spec is NULL");
        return out;
    }

    oeaf_audio_wrap* w = (oeaf_audio_wrap*)calloc(1, sizeof(oeaf_audio_wrap));
    if (!w) {
        oeaf_set_error("AUDIO: out of memory");
        return out;
    }
    w->ctx = ctx;
    w->user_cb = desired->callback;
    w->user_ud = desired->userdata;

    oeaf_audio_spec want;
    memset(&want, 0, sizeof(want));
    want.sample_rate = (desired->freq > 0) ? (oeaf_u32)desired->freq : 48000u;
    want.channels = (desired->channels > 0) ? (oeaf_u32)desired->channels : 2u;
    want.format = map_to_backend_format(desired->format);
    want.buffer_frames = (desired->samples > 0) ? (oeaf_u32)desired->samples : 1024u;
    want.callback = oeaf_audio_wrapper_cb;
    want.userdata = w;

    oeaf_audio_spec have;
    memset(&have, 0, sizeof(have));
    oeaf_audio_device dev;
    memset(&dev, 0, sizeof(dev));
    oeaf_result r = oeaf_audio_open_device(ctx, &want, &have, &dev);
    if (r != OEAF_OK) {
        free(w);
        oeaf_set_error("AUDIO: no OEAF audio backend bound (or open failed), r=%d", (int)r);
        return out;
    }

    w->backend_dev = dev;
    w->backend_have = have;

    if (obtained) {
        memset(obtained, 0, sizeof(*obtained));
        obtained->freq = (int)have.sample_rate;
        obtained->channels = (oeaf_u8)have.channels;
        obtained->format = map_from_backend_format(have.format);
        obtained->samples = (oeaf_u16)have.buffer_frames;
        obtained->callback = desired->callback; /* what user asked for */
        obtained->userdata = desired->userdata;
        {
            oeaf_u32 bps = backend_bytes_per_sample(have.format);
            obtained->size = (oeaf_u32)(have.buffer_frames * have.channels * bps);
        }
    }

    out.ptr = w;
    oeaf_clear_error();
    return out;
}

void oeaf_CloseAudioDevice(oeaf_AudioDeviceID dev) {
    oeaf_audio_wrap* w = (oeaf_audio_wrap*)dev.ptr;
    if (!w) return;
    if (w->ctx) {
        oeaf_audio_close_device(w->ctx, w->backend_dev);
    }
    free(w->qbuf);
    free(w);
}

void oeaf_PauseAudioDevice(oeaf_AudioDeviceID dev, oeaf_i32 pause_on) {
    oeaf_audio_wrap* w = (oeaf_audio_wrap*)dev.ptr;
    if (!w || !w->ctx) return;
    oeaf_audio_pause_device(w->ctx, w->backend_dev, pause_on ? 1 : 0);
}

oeaf_i32 oeaf_QueueAudio(oeaf_AudioDeviceID dev, const void* data, oeaf_u32 len) {
    oeaf_audio_wrap* w = (oeaf_audio_wrap*)dev.ptr;
    if (!w) {
        oeaf_set_error("AUDIO: device is NULL");
        return -1;
    }
    if (w->user_cb) {
        oeaf_set_error("AUDIO: device opened with callback; QueueAudio not allowed");
        return -1;
    }
    if (!data || len == 0) return 0;
    if (queue_push(w, (const oeaf_u8*)data, (size_t)len) != 0) {
        oeaf_set_error("AUDIO: out of memory (queue)");
        return -1;
    }
    oeaf_clear_error();
    return 0;
}

oeaf_u32 oeaf_GetQueuedAudioSize(oeaf_AudioDeviceID dev) {
    oeaf_audio_wrap* w = (oeaf_audio_wrap*)dev.ptr;
    if (!w) return 0;
    if (w->qsize > 0xFFFFFFFFu) return 0xFFFFFFFFu;
    return (oeaf_u32)w->qsize;
}

void oeaf_ClearQueuedAudio(oeaf_AudioDeviceID dev) {
    oeaf_audio_wrap* w = (oeaf_audio_wrap*)dev.ptr;
    if (!w) return;
    w->qhead = w->qtail = w->qsize = 0;
}
