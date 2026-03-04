#ifndef OEAF_RWOPS_H
#define OEAF_RWOPS_H

#include "oeaf_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* A tiny SDL_RWops-style I/O abstraction.
   This is intentionally minimal: enough for image/audio loaders.

   - All functions return -1 on error unless otherwise specified.
   - 'size' may return -1 if unknown.
*/

typedef struct oeaf_RWops oeaf_RWops;

typedef oeaf_i64 (*oeaf_rw_size_fn)(oeaf_RWops* ctx);
typedef oeaf_i64 (*oeaf_rw_seek_fn)(oeaf_RWops* ctx, oeaf_i64 offset, oeaf_i32 whence);
typedef oeaf_i64 (*oeaf_rw_tell_fn)(oeaf_RWops* ctx);
typedef size_t   (*oeaf_rw_read_fn)(oeaf_RWops* ctx, void* ptr, size_t size, size_t maxnum);
typedef size_t   (*oeaf_rw_write_fn)(oeaf_RWops* ctx, const void* ptr, size_t size, size_t num);
typedef oeaf_i32 (*oeaf_rw_close_fn)(oeaf_RWops* ctx);

struct oeaf_RWops {
    void* userdata;

    oeaf_rw_size_fn  size;
    oeaf_rw_seek_fn  seek;
    oeaf_rw_tell_fn  tell;
    oeaf_rw_read_fn  read;
    oeaf_rw_write_fn write;
    oeaf_rw_close_fn close;
};

/* Constructors */
oeaf_RWops* oeaf_RWFromFile(const char* file, const char* mode);
oeaf_RWops* oeaf_RWFromMem(void* mem, size_t size, oeaf_i32 writable);

/* Utility: read whole stream into a malloc'd buffer.
   On success returns OEAF_OK and sets out_data/out_size.
   Caller must free(*out_data).
*/
oeaf_result oeaf_RWread_all(oeaf_RWops* rw, void** out_data, size_t* out_size);

/* Destroy RWops (calls rw->close if present, then frees rw).
   Safe to call with NULL.
*/
void oeaf_RWclose(oeaf_RWops* rw);

#ifdef __cplusplus
}
#endif

#endif /* OEAF_RWOPS_H */
