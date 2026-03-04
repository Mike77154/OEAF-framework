#ifndef OEAF_TIMER_H
#define OEAF_TIMER_H

#include "oeaf_types.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct oeaf_timer_vtbl {
    /* High-res monotonic counter in ticks and tick frequency (ticks/sec). */
    oeaf_u64 (*counter)(void* self, oeaf_ctx* ctx);
    oeaf_u64 (*frequency)(void* self, oeaf_ctx* ctx);
} oeaf_timer_vtbl;

typedef struct oeaf_timer_iface {
    void* self;
    const oeaf_timer_vtbl* vtbl;
} oeaf_timer_iface;

#ifdef __cplusplus
}
#endif

#endif /* OEAF_TIMER_H */
