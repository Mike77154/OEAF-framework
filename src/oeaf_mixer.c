#include <stdlib.h>
#include <string.h>

#include "oeaf/oeaf_mixer.h"
#include "oeaf/oeaf_sdl.h"      /* oeaf_get_ctx / oeaf_set_error */
#include "oeaf/oeaf_dispatch.h" /* oeaf_audio_open_device */

/* ---- data types ---- */

struct oeafMix_Chunk {
    int     volume;       /* 0..128 */
    oeaf_u32 sample_rate;
    oeaf_u32 channels;    /* 1 or 2 supported */
    oeaf_u32 frames;
    float*  samples;      /* interleaved float samples in [-1,1], length frames*channels */
};

struct oeafMix_Music {
    int dummy;
};

typedef struct mix_playing {
    oeafMix_Chunk* chunk;
    int     active;
    int     loops;        /* 0 play once, -1 infinite, >0 extra loops */
    int     volume;       /* 0..128 */
    float   pos;          /* in source frames (fractional for resample) */
    float   step;         /* src_rate / dst_rate */
} mix_playing;

typedef struct mix_state {
    oeaf_ctx* ctx;
    oeaf_audio_device dev;
    oeaf_audio_spec   have;

    int channels_alloc;
    mix_playing* ch;

    float* scratch;      /* frames*have.channels */
    oeaf_u32 scratch_frames;
} mix_state;

static mix_state* g_mix = NULL;

/* ---- helpers ---- */

static float clampf(float v, float lo, float hi) {
    if (v < lo) return lo;
    if (v > hi) return hi;
    return v;
}

static oeaf_u32 bytes_per_sample(oeaf_audio_format fmt) {
    switch (fmt) {
        case OEAF_AUDIO_S16: return 2;
        case OEAF_AUDIO_F32: return 4;
        default: return 0;
    }
}

static oeaf_audio_format map_audio_format(oeaf_u16 fmt) {
    /* Accept a tiny subset of SDL audio formats.
       - 0x8010 (AUDIO_S16LSB) -> S16
       - 0x8120 (AUDIO_F32LSB) -> F32
       If unknown, default to S16.
    */
    if (fmt == OEAF_AUDIO_F32LSB) return OEAF_AUDIO_F32;
    if (fmt == OEAF_AUDIO_S16LSB) return OEAF_AUDIO_S16;
    /* Heuristic: SDL encodes float formats with 0x1000? not reliable. */
    return OEAF_AUDIO_S16;
}

static void ensure_scratch(mix_state* st, oeaf_u32 frames) {
    if (!st) return;
    if (frames <= st->scratch_frames) return;
    size_t need = (size_t)frames * (size_t)st->have.channels;
    float* nbuf = (float*)realloc(st->scratch, need * sizeof(float));
    if (!nbuf) return;
    st->scratch = nbuf;
    st->scratch_frames = frames;
}

static void mix_callback(void* userdata, void* stream, oeaf_u32 bytes) {
    mix_state* st = (mix_state*)userdata;
    if (!st || !stream) return;

    oeaf_u32 bps = bytes_per_sample(st->have.format);
    if (bps == 0 || st->have.channels == 0) return;

    oeaf_u32 frames = bytes / (bps * st->have.channels);
    if (frames == 0) return;

    ensure_scratch(st, frames);
    if (!st->scratch) {
        /* can't allocate scratch; output silence */
        memset(stream, 0, bytes);
        return;
    }

    size_t scratch_len = (size_t)frames * (size_t)st->have.channels;
    memset(st->scratch, 0, scratch_len * sizeof(float));

    /* Mix all channels into scratch (float). */
    for (int ci = 0; ci < st->channels_alloc; ci++) {
        mix_playing* p = &st->ch[ci];
        if (!p->active || !p->chunk || !p->chunk->samples) continue;

        oeafMix_Chunk* c = p->chunk;
        if (c->frames == 0 || c->channels == 0) continue;

        float vol = (float)clampf((float)p->volume, 0.0f, 128.0f) / 128.0f;
        float cvol = (float)clampf((float)c->volume, 0.0f, 128.0f) / 128.0f;
        float gain = vol * cvol;

        /* For each output frame, sample chunk at p->pos (linear interp). */
        for (oeaf_u32 of = 0; of < frames; of++) {
            if (!p->active) break;

            float ipos = p->pos;
            if (ipos < 0.0f) ipos = 0.0f;
            oeaf_u32 i0 = (oeaf_u32)ipos;
            oeaf_u32 i1 = (i0 + 1u < c->frames) ? (i0 + 1u) : i0;
            float t = ipos - (float)i0;

            /* Fetch source samples (up to stereo). */
            float s0L = 0.0f, s0R = 0.0f, s1L = 0.0f, s1R = 0.0f;
            if (c->channels == 1) {
                s0L = c->samples[(size_t)i0];
                s1L = c->samples[(size_t)i1];
                s0R = s0L;
                s1R = s1L;
            } else {
                size_t b0 = (size_t)i0 * 2u;
                size_t b1 = (size_t)i1 * 2u;
                s0L = c->samples[b0 + 0];
                s0R = c->samples[b0 + 1];
                s1L = c->samples[b1 + 0];
                s1R = c->samples[b1 + 1];
            }
            float sL = s0L + (s1L - s0L) * t;
            float sR = s0R + (s1R - s0R) * t;

            /* Write into scratch, mapping to output channels. */
            if (st->have.channels == 1) {
                st->scratch[(size_t)of] += (sL + sR) * 0.5f * gain;
            } else {
                size_t ob = (size_t)of * 2u;
                st->scratch[ob + 0] += sL * gain;
                st->scratch[ob + 1] += sR * gain;
            }

            p->pos += p->step;
            if (p->pos >= (float)c->frames) {
                if (p->loops == 0) {
                    p->active = 0;
                } else {
                    if (p->loops > 0) p->loops--;
                    /* Wrap without libm. */
                    while (p->pos >= (float)c->frames) {
                        p->pos -= (float)c->frames;
                    }
                }
            }
        }
    }

    /* Convert scratch to output format */
    if (st->have.format == OEAF_AUDIO_F32) {
        float* out = (float*)stream;
        for (size_t i = 0; i < scratch_len; i++) {
            out[i] = clampf(st->scratch[i], -1.0f, 1.0f);
        }
    } else {
        oeaf_i16* out = (oeaf_i16*)stream;
        for (size_t i = 0; i < scratch_len; i++) {
            float v = clampf(st->scratch[i], -1.0f, 1.0f);
            /* Round without libm. */
            float scaled = v * 32767.0f;
            int iv = (scaled >= 0.0f) ? (int)(scaled + 0.5f) : (int)(scaled - 0.5f);
            if (iv < -32768) iv = -32768;
            if (iv > 32767) iv = 32767;
            out[i] = (oeaf_i16)iv;
        }
    }
}

static oeafMix_Chunk* wav_decode_from_mem(const oeaf_u8* data, size_t size) {
    /* Very small RIFF WAVE PCM decoder (8-bit unsigned, 16-bit signed LE).
       Supports mono/stereo.
    */
    if (!data || size < 44) {
        oeaf_set_error("MIX: WAV too small");
        return NULL;
    }
    if (memcmp(data, "RIFF", 4) != 0 || memcmp(data + 8, "WAVE", 4) != 0) {
        oeaf_set_error("MIX: not a RIFF/WAVE file");
        return NULL;
    }

    oeaf_u32 fmt_audio_format = 0;
    oeaf_u32 fmt_channels = 0;
    oeaf_u32 fmt_sample_rate = 0;
    oeaf_u32 fmt_bits = 0;

    const oeaf_u8* p = data + 12;
    const oeaf_u8* end = data + size;

    const oeaf_u8* data_chunk = NULL;
    oeaf_u32 data_chunk_size = 0;

    while (p + 8 <= end) {
        const oeaf_u8* chunk_id = p;
        oeaf_u32 chunk_size = (oeaf_u32)(chunk_id[4] | (chunk_id[5] << 8) | (chunk_id[6] << 16) | (chunk_id[7] << 24));
        const oeaf_u8* chunk_data = p + 8;
        if (chunk_data + chunk_size > end) break;

        if (memcmp(chunk_id, "fmt ", 4) == 0) {
            if (chunk_size < 16) {
                oeaf_set_error("MIX: WAV fmt chunk too small");
                return NULL;
            }
            fmt_audio_format = (oeaf_u32)(chunk_data[0] | (chunk_data[1] << 8));
            fmt_channels = (oeaf_u32)(chunk_data[2] | (chunk_data[3] << 8));
            fmt_sample_rate = (oeaf_u32)(chunk_data[4] | (chunk_data[5] << 8) | (chunk_data[6] << 16) | (chunk_data[7] << 24));
            fmt_bits = (oeaf_u32)(chunk_data[14] | (chunk_data[15] << 8));
        } else if (memcmp(chunk_id, "data", 4) == 0) {
            data_chunk = chunk_data;
            data_chunk_size = chunk_size;
        }

        /* chunks are word-aligned */
        p = chunk_data + chunk_size + (chunk_size & 1u);
    }

    if (!data_chunk || data_chunk_size == 0) {
        oeaf_set_error("MIX: WAV missing data chunk");
        return NULL;
    }
    if (fmt_audio_format != 1) {
        oeaf_set_error("MIX: WAV format not PCM (%u)", (unsigned)fmt_audio_format);
        return NULL;
    }
    if (!(fmt_channels == 1 || fmt_channels == 2)) {
        oeaf_set_error("MIX: WAV channels not supported (%u)", (unsigned)fmt_channels);
        return NULL;
    }
    if (!(fmt_bits == 8 || fmt_bits == 16)) {
        oeaf_set_error("MIX: WAV bits not supported (%u)", (unsigned)fmt_bits);
        return NULL;
    }
    if (fmt_sample_rate == 0) {
        oeaf_set_error("MIX: WAV sample rate is 0");
        return NULL;
    }

    oeaf_u32 bytes_per_frame = (fmt_channels * (fmt_bits / 8u));
    if (bytes_per_frame == 0) return NULL;
    oeaf_u32 frames = data_chunk_size / bytes_per_frame;
    if (frames == 0) {
        oeaf_set_error("MIX: WAV has 0 frames");
        return NULL;
    }

    oeafMix_Chunk* c = (oeafMix_Chunk*)calloc(1, sizeof(oeafMix_Chunk));
    if (!c) {
        oeaf_set_error("MIX: out of memory (chunk)");
        return NULL;
    }
    c->volume = 128;
    c->sample_rate = fmt_sample_rate;
    c->channels = fmt_channels;
    c->frames = frames;
    c->samples = (float*)malloc((size_t)frames * (size_t)fmt_channels * sizeof(float));
    if (!c->samples) {
        free(c);
        oeaf_set_error("MIX: out of memory (samples)");
        return NULL;
    }

    if (fmt_bits == 8) {
        /* unsigned 8-bit centered at 128 */
        const oeaf_u8* s = data_chunk;
        for (oeaf_u32 i = 0; i < frames * fmt_channels; i++) {
            c->samples[i] = ((float)s[i] - 128.0f) / 128.0f;
        }
    } else {
        /* signed 16-bit little-endian */
        const oeaf_u8* s = data_chunk;
        for (oeaf_u32 i = 0; i < frames * fmt_channels; i++) {
            oeaf_u16 lo = s[i * 2u + 0];
            oeaf_u16 hi = s[i * 2u + 1];
            oeaf_i16 v = (oeaf_i16)(lo | (hi << 8));
            c->samples[i] = (float)v / 32768.0f;
        }
    }

    oeaf_clear_error();
    return c;
}

/* ---- public API ---- */

const char* oeafMix_GetError(void) {
    return oeaf_get_error();
}

int oeafMix_OpenAudio(int frequency, oeaf_u16 format, int channels, int chunksize) {
    if (g_mix) {
        /* already open */
        return 0;
    }
    oeaf_ctx* ctx = oeaf_get_ctx();
    if (!ctx) {
        oeaf_set_error("MIX: OEAF not initialized (call oeaf_init first)");
        return -1;
    }

    oeaf_audio_spec want;
    memset(&want, 0, sizeof(want));
    want.sample_rate = (frequency > 0) ? (oeaf_u32)frequency : 44100u;
    want.channels = (channels > 0) ? (oeaf_u32)channels : 2u;
    want.format = map_audio_format(format);
    {
        oeaf_u32 bps = bytes_per_sample(want.format);
        oeaf_u32 frames = 0;
        if (chunksize > 0 && bps > 0 && want.channels > 0) {
            frames = (oeaf_u32)((oeaf_u32)chunksize / (bps * want.channels));
        }
        want.buffer_frames = (frames > 0) ? frames : 1024u;
    }

    mix_state* st = (mix_state*)calloc(1, sizeof(mix_state));
    if (!st) {
        oeaf_set_error("MIX: out of memory");
        return -1;
    }
    st->ctx = ctx;
    st->channels_alloc = 8;
    st->ch = (mix_playing*)calloc((size_t)st->channels_alloc, sizeof(mix_playing));
    if (!st->ch) {
        free(st);
        oeaf_set_error("MIX: out of memory (channels)");
        return -1;
    }
    for (int i = 0; i < st->channels_alloc; i++) {
        st->ch[i].volume = 128;
    }

    want.callback = mix_callback;
    want.userdata = st;

    oeaf_audio_spec have;
    memset(&have, 0, sizeof(have));
    oeaf_audio_device dev;
    memset(&dev, 0, sizeof(dev));
    oeaf_result r = oeaf_audio_open_device(ctx, &want, &have, &dev);
    if (r != OEAF_OK) {
        /* If float failed, try S16 as a fallback */
        if (want.format == OEAF_AUDIO_F32) {
            want.format = OEAF_AUDIO_S16;
            r = oeaf_audio_open_device(ctx, &want, &have, &dev);
        }
    }
    if (r != OEAF_OK) {
        free(st->ch);
        free(st);
        oeaf_set_error("MIX: no OEAF audio backend bound (or open failed), r=%d", (int)r);
        return -1;
    }

    st->dev = dev;
    st->have = have;
    g_mix = st;
    oeaf_clear_error();
    return 0;
}

void oeafMix_CloseAudio(void) {
    if (!g_mix) return;
    if (g_mix->ctx) {
        oeaf_audio_close_device(g_mix->ctx, g_mix->dev);
    }
    free(g_mix->scratch);
    free(g_mix->ch);
    free(g_mix);
    g_mix = NULL;
}

int oeafMix_AllocateChannels(int numchans) {
    if (!g_mix) {
        oeaf_set_error("MIX: audio not opened");
        return -1;
    }
    if (numchans <= 0) numchans = 1;
    mix_playing* n = (mix_playing*)realloc(g_mix->ch, (size_t)numchans * sizeof(mix_playing));
    if (!n) {
        oeaf_set_error("MIX: out of memory (realloc channels)");
        return -1;
    }
    /* init new slots */
    if (numchans > g_mix->channels_alloc) {
        for (int i = g_mix->channels_alloc; i < numchans; i++) {
            memset(&n[i], 0, sizeof(mix_playing));
            n[i].volume = 128;
        }
    }
    g_mix->ch = n;
    g_mix->channels_alloc = numchans;
    return numchans;
}

int oeafMix_Volume(int channel, int volume) {
    if (!g_mix) {
        oeaf_set_error("MIX: audio not opened");
        return -1;
    }
    if (channel < 0 || channel >= g_mix->channels_alloc) {
        oeaf_set_error("MIX: bad channel index");
        return -1;
    }
    int prev = g_mix->ch[channel].volume;
    if (volume >= 0) {
        if (volume > 128) volume = 128;
        g_mix->ch[channel].volume = volume;
    }
    return prev;
}

int oeafMix_HaltChannel(int channel) {
    if (!g_mix) return 0;
    if (channel < 0 || channel >= g_mix->channels_alloc) return 0;
    g_mix->ch[channel].active = 0;
    g_mix->ch[channel].chunk = NULL;
    g_mix->ch[channel].pos = 0.0f;
    g_mix->ch[channel].loops = 0;
    return 0;
}

oeafMix_Chunk* oeafMix_LoadWAV(const char* file) {
    if (!file) {
        oeaf_set_error("MIX: file is NULL");
        return NULL;
    }
    oeaf_RWops* rw = oeaf_RWFromFile(file, "rb");
    if (!rw) {
        oeaf_set_error("MIX: failed to open '%s'", file);
        return NULL;
    }
    return oeafMix_LoadWAV_RW(rw, 1);
}

oeafMix_Chunk* oeafMix_LoadWAV_RW(oeaf_RWops* src, int freesrc) {
    if (!src) {
        oeaf_set_error("MIX: src is NULL");
        return NULL;
    }

    void* data = NULL;
    size_t size = 0;
    oeaf_result r = oeaf_RWread_all(src, &data, &size);
    if (freesrc) oeaf_RWclose(src);

    if (r != OEAF_OK || !data || size == 0) {
        free(data);
        oeaf_set_error("MIX: failed to read WAV stream");
        return NULL;
    }

    oeafMix_Chunk* c = wav_decode_from_mem((const oeaf_u8*)data, size);
    free(data);
    return c;
}

void oeafMix_FreeChunk(oeafMix_Chunk* chunk) {
    if (!chunk) return;
    free(chunk->samples);
    free(chunk);
}

int oeafMix_VolumeChunk(oeafMix_Chunk* chunk, int volume) {
    if (!chunk) return -1;
    int prev = chunk->volume;
    if (volume >= 0) {
        if (volume > 128) volume = 128;
        chunk->volume = volume;
    }
    return prev;
}

int oeafMix_PlayChannel(int channel, oeafMix_Chunk* chunk, int loops) {
    if (!g_mix) {
        oeaf_set_error("MIX: audio not opened");
        return -1;
    }
    if (!chunk) {
        oeaf_set_error("MIX: chunk is NULL");
        return -1;
    }

    int use = channel;
    if (use < 0) {
        for (int i = 0; i < g_mix->channels_alloc; i++) {
            if (!g_mix->ch[i].active) {
                use = i;
                break;
            }
        }
    }
    if (use < 0 || use >= g_mix->channels_alloc) {
        oeaf_set_error("MIX: no free channels");
        return -1;
    }

    mix_playing* p = &g_mix->ch[use];
    p->chunk = chunk;
    p->active = 1;
    p->loops = loops;
    p->pos = 0.0f;
    if (g_mix->have.sample_rate == 0) {
        p->step = 1.0f;
    } else {
        p->step = (float)chunk->sample_rate / (float)g_mix->have.sample_rate;
    }
    /* Keep existing per-channel volume */

    oeaf_clear_error();
    return use;
}

/* ---- music stubs ---- */

oeafMix_Music* oeafMix_LoadMUS(const char* file) {
    (void)file;
    oeaf_set_error("MIX: music loading not implemented (stub)");
    return NULL;
}

void oeafMix_FreeMusic(oeafMix_Music* music) {
    free(music);
}

int oeafMix_PlayMusic(oeafMix_Music* music, int loops) {
    (void)music; (void)loops;
    oeaf_set_error("MIX: music playback not implemented (stub)");
    return -1;
}

int oeafMix_HaltMusic(void) {
    oeaf_set_error("MIX: music halt not implemented (stub)");
    return -1;
}
