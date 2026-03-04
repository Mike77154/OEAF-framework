#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "oeaf_core_private.h"

/* ---- Event queue ---- */

oeaf_result oeaf_events_push(oeaf_ctx* ctx, const oeaf_event* e) {
    if (!ctx || !e) return OEAF_ERR_BADARGS;
    if (ctx->q.count >= OEAF_EVENT_QUEUE_CAP) return OEAF_ERR; /* overflow */
    ctx->q.buf[ctx->q.tail] = *e;
    ctx->q.tail = (ctx->q.tail + 1u) % OEAF_EVENT_QUEUE_CAP;
    ctx->q.count++;
    return OEAF_OK;
}

oeaf_result oeaf_poll_event_ctx(oeaf_ctx* ctx, oeaf_event* out_e) {
    if (!ctx || !out_e) return OEAF_ERR_BADARGS;
    if (ctx->q.count == 0) {
        out_e->type = OEAF_EVENT_NONE;
        return OEAF_OK;
    }
    *out_e = ctx->q.buf[ctx->q.head];
    ctx->q.head = (ctx->q.head + 1u) % OEAF_EVENT_QUEUE_CAP;
    ctx->q.count--;
    return OEAF_OK;
}

/* ---- Context ---- */

static void oeaf_rebuild_summary(oeaf_ctx* ctx) {
    snprintf(ctx->summary, sizeof(ctx->summary),
        "OEAF bound: OS=%s | TIMER=%s | VIDEO=%s | INPUT=%s | AUDIO=%s | FS=%s | modules=%u",
        (ctx->os.vtbl ? "yes" : "no"),
        (ctx->timer.vtbl ? "yes" : "no"),
        (ctx->video.vtbl ? "yes" : "no"),
        (ctx->input.vtbl ? "yes" : "no"),
        (ctx->audio.vtbl ? "yes" : "no"),
        (ctx->fs.vtbl ? "yes" : "no"),
        (unsigned)ctx->mod_count
    );
}

oeaf_ctx* oeaf_create(void) {
    oeaf_ctx* ctx = (oeaf_ctx*)calloc(1, sizeof(oeaf_ctx));
    if (!ctx) return NULL;
    ctx->q.head = ctx->q.tail = ctx->q.count = 0;
    ctx->mod_count = 0;
    ctx->last_counter = 0;
    ctx->last_freq = 0;
    oeaf_rebuild_summary(ctx);
    return ctx;
}

void oeaf_destroy(oeaf_ctx* ctx) {
    if (!ctx) return;

    /* Detach modules in reverse order */
    while (ctx->mod_count > 0) {
        oeaf_u32 i = ctx->mod_count - 1u;
        const oeaf_module_desc* d = ctx->mods[i].desc;
        void* st = ctx->mods[i].state;
        if (d && d->detach) d->detach(ctx, st);
        ctx->mods[i].desc = NULL;
        ctx->mods[i].state = NULL;
        ctx->mod_count--;
    }

    free(ctx);
}

oeaf_result oeaf_attach_module(oeaf_ctx* ctx, const oeaf_module_desc* mod) {
    void* state = NULL;
    if (!ctx || !mod) return OEAF_ERR_BADARGS;
    if (mod->abi_version != OEAF_ABI_VERSION) return OEAF_ERR;
    if (ctx->mod_count >= OEAF_MAX_MODULES) return OEAF_ERR;

    if (mod->attach) {
        oeaf_result r = mod->attach(ctx, &state);
        if (r != OEAF_OK) return r;
    }

    /* Record module */
    ctx->mods[ctx->mod_count].desc = mod;
    ctx->mods[ctx->mod_count].state = state;
    ctx->mod_count++;

    /* Bind provided interfaces, self=state */
    if (mod->provides & OEAF_PROVIDES_OS) {
        ctx->os = mod->os;
        ctx->os.self = state;
        if (ctx->os.vtbl && ctx->os.vtbl->init) ctx->os.vtbl->init(ctx->os.self, ctx);
    }
    if (mod->provides & OEAF_PROVIDES_TIMER) {
        ctx->timer = mod->timer;
        ctx->timer.self = state;
        if (ctx->timer.vtbl && ctx->timer.vtbl->frequency) {
            ctx->last_freq = ctx->timer.vtbl->frequency(ctx->timer.self, ctx);
        }
    }
    if (mod->provides & OEAF_PROVIDES_VIDEO) {
        ctx->video = mod->video;
        ctx->video.self = state;
    }
    if (mod->provides & OEAF_PROVIDES_INPUT) {
        ctx->input = mod->input;
        ctx->input.self = state;
    }
    if (mod->provides & OEAF_PROVIDES_AUDIO) {
        ctx->audio = mod->audio;
        ctx->audio.self = state;
    }
    if (mod->provides & OEAF_PROVIDES_FS) {
        ctx->fs = mod->fs;
        ctx->fs.self = state;
    }

    oeaf_rebuild_summary(ctx);
    return OEAF_OK;
}

oeaf_result oeaf_require(oeaf_ctx* ctx, oeaf_u32 provides_mask) {
    if (!ctx) return OEAF_ERR_BADARGS;

    if ((provides_mask & OEAF_PROVIDES_OS) && !ctx->os.vtbl) return OEAF_ERR_NOBACKEND;
    if ((provides_mask & OEAF_PROVIDES_TIMER) && !ctx->timer.vtbl) return OEAF_ERR_NOBACKEND;
    if ((provides_mask & OEAF_PROVIDES_VIDEO) && !ctx->video.vtbl) return OEAF_ERR_NOBACKEND;
    if ((provides_mask & OEAF_PROVIDES_INPUT) && !ctx->input.vtbl) return OEAF_ERR_NOBACKEND;
    if ((provides_mask & OEAF_PROVIDES_AUDIO) && !ctx->audio.vtbl) return OEAF_ERR_NOBACKEND;
    if ((provides_mask & OEAF_PROVIDES_FS) && !ctx->fs.vtbl) return OEAF_ERR_NOBACKEND;

    return OEAF_OK;
}

oeaf_result oeaf_step_timer(oeaf_ctx* ctx, oeaf_f32* out_dt) {
    if (!ctx || !out_dt) return OEAF_ERR_BADARGS;
    if (!ctx->timer.vtbl || !ctx->timer.vtbl->counter || !ctx->timer.vtbl->frequency) return OEAF_ERR_NOBACKEND;

    oeaf_u64 c = ctx->timer.vtbl->counter(ctx->timer.self, ctx);
    oeaf_u64 f = ctx->timer.vtbl->frequency(ctx->timer.self, ctx);
    if (f == 0) return OEAF_ERR;

    if (ctx->last_counter == 0) {
        ctx->last_counter = c;
        ctx->last_freq = f;
        *out_dt = 0.0f;
        return OEAF_OK;
    }

    oeaf_u64 dc = (c - ctx->last_counter);
    ctx->last_counter = c;
    ctx->last_freq = f;

    *out_dt = (oeaf_f32)((oeaf_f64)dc / (oeaf_f64)f);
    return OEAF_OK;
}

const char* oeaf_bound_summary(oeaf_ctx* ctx) {
    if (!ctx) return "OEAF(null)";
    return ctx->summary;
}
