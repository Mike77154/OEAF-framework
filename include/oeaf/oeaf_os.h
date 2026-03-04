#ifndef OEAF_OS_H
#define OEAF_OS_H

#include "oeaf_types.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct oeaf_os_vtbl {
    oeaf_result (*init)(void* self, oeaf_ctx* ctx);
    void        (*shutdown)(void* self, oeaf_ctx* ctx);

    /* Pump platform events and push into OEAF queue using oeaf_events_push(). */
    oeaf_result (*pump)(void* self, oeaf_ctx* ctx);

    /* Optional */
    void        (*sleep_ms)(void* self, oeaf_ctx* ctx, oeaf_u32 ms);

    /* Optional: return backend-native pointers (windowing/display) */
    void*       (*get_native)(void* self, oeaf_ctx* ctx, const char* what);
} oeaf_os_vtbl;

typedef struct oeaf_os_iface {
    void* self;
    const oeaf_os_vtbl* vtbl;
} oeaf_os_iface;

#ifdef __cplusplus
}
#endif

#endif /* OEAF_OS_H */
