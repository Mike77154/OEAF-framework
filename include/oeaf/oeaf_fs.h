#ifndef OEAF_FS_H
#define OEAF_FS_H

#include "oeaf_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Minimal filesystem contract (extend later) */
typedef struct oeaf_fs_vtbl {
    /* Returns 1 if exists, 0 if not, negative on error */
    oeaf_i32 (*exists)(void* self, oeaf_ctx* ctx, const char* path);

    /* Read entire file into memory allocated by backend or provided allocator (TBD).
       For v0.1, backend may malloc(); caller must free() using oeaf_free_bytes().
    */
    oeaf_result (*read_all)(void* self, oeaf_ctx* ctx, const char* path, void** out_data, size_t* out_size);

    oeaf_result (*write_all)(void* self, oeaf_ctx* ctx, const char* path, const void* data, size_t size);
} oeaf_fs_vtbl;

typedef struct oeaf_fs_iface {
    void* self;
    const oeaf_fs_vtbl* vtbl;
} oeaf_fs_iface;

#ifdef __cplusplus
}
#endif

#endif /* OEAF_FS_H */
