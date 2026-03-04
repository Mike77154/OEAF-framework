#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "oeaf/oeaf_rwops.h"

/* ---- FILE RWops ---- */

typedef struct oeaf_rw_file {
    FILE* f;
} oeaf_rw_file;

static oeaf_i64 rw_file_size(oeaf_RWops* ctx) {
    oeaf_rw_file* st = (oeaf_rw_file*)ctx->userdata;
    if (!st || !st->f) return -1;

    long cur = ftell(st->f);
    if (cur < 0) return -1;
    if (fseek(st->f, 0, SEEK_END) != 0) return -1;
    long end = ftell(st->f);
    (void)fseek(st->f, cur, SEEK_SET);
    if (end < 0) return -1;
    return (oeaf_i64)end;
}

static oeaf_i64 rw_file_seek(oeaf_RWops* ctx, oeaf_i64 offset, oeaf_i32 whence) {
    oeaf_rw_file* st = (oeaf_rw_file*)ctx->userdata;
    if (!st || !st->f) return -1;
    if (fseek(st->f, (long)offset, whence) != 0) return -1;
    long pos = ftell(st->f);
    if (pos < 0) return -1;
    return (oeaf_i64)pos;
}

static oeaf_i64 rw_file_tell(oeaf_RWops* ctx) {
    oeaf_rw_file* st = (oeaf_rw_file*)ctx->userdata;
    if (!st || !st->f) return -1;
    long pos = ftell(st->f);
    if (pos < 0) return -1;
    return (oeaf_i64)pos;
}

static size_t rw_file_read(oeaf_RWops* ctx, void* ptr, size_t size, size_t maxnum) {
    oeaf_rw_file* st = (oeaf_rw_file*)ctx->userdata;
    if (!st || !st->f || !ptr) return 0;
    return fread(ptr, size, maxnum, st->f);
}

static size_t rw_file_write(oeaf_RWops* ctx, const void* ptr, size_t size, size_t num) {
    oeaf_rw_file* st = (oeaf_rw_file*)ctx->userdata;
    if (!st || !st->f || !ptr) return 0;
    return fwrite(ptr, size, num, st->f);
}

static oeaf_i32 rw_file_close(oeaf_RWops* ctx) {
    oeaf_rw_file* st = (oeaf_rw_file*)ctx->userdata;
    if (st && st->f) {
        fclose(st->f);
        st->f = NULL;
    }
    free(st);
    ctx->userdata = NULL;
    return 0;
}

oeaf_RWops* oeaf_RWFromFile(const char* file, const char* mode) {
    if (!file || !mode) return NULL;
    FILE* f = fopen(file, mode);
    if (!f) return NULL;

    oeaf_RWops* rw = (oeaf_RWops*)calloc(1, sizeof(oeaf_RWops));
    if (!rw) {
        fclose(f);
        return NULL;
    }

    oeaf_rw_file* st = (oeaf_rw_file*)calloc(1, sizeof(oeaf_rw_file));
    if (!st) {
        free(rw);
        fclose(f);
        return NULL;
    }

    st->f = f;
    rw->userdata = st;
    rw->size = rw_file_size;
    rw->seek = rw_file_seek;
    rw->tell = rw_file_tell;
    rw->read = rw_file_read;
    rw->write = rw_file_write;
    rw->close = rw_file_close;
    return rw;
}

/* ---- MEM RWops ---- */

typedef struct oeaf_rw_mem {
    oeaf_u8* data;
    size_t   size;
    size_t   pos;
    oeaf_i32 writable;
} oeaf_rw_mem;

static oeaf_i64 rw_mem_size(oeaf_RWops* ctx) {
    oeaf_rw_mem* st = (oeaf_rw_mem*)ctx->userdata;
    if (!st) return -1;
    return (oeaf_i64)st->size;
}

static oeaf_i64 rw_mem_seek(oeaf_RWops* ctx, oeaf_i64 offset, oeaf_i32 whence) {
    oeaf_rw_mem* st = (oeaf_rw_mem*)ctx->userdata;
    if (!st) return -1;

    oeaf_i64 base = 0;
    if (whence == SEEK_SET) base = 0;
    else if (whence == SEEK_CUR) base = (oeaf_i64)st->pos;
    else if (whence == SEEK_END) base = (oeaf_i64)st->size;
    else return -1;

    oeaf_i64 np = base + offset;
    if (np < 0) np = 0;
    if ((size_t)np > st->size) np = (oeaf_i64)st->size;
    st->pos = (size_t)np;
    return (oeaf_i64)st->pos;
}

static oeaf_i64 rw_mem_tell(oeaf_RWops* ctx) {
    oeaf_rw_mem* st = (oeaf_rw_mem*)ctx->userdata;
    if (!st) return -1;
    return (oeaf_i64)st->pos;
}

static size_t rw_mem_read(oeaf_RWops* ctx, void* ptr, size_t size, size_t maxnum) {
    oeaf_rw_mem* st = (oeaf_rw_mem*)ctx->userdata;
    if (!st || !ptr) return 0;

    size_t total = size * maxnum;
    if (total == 0) return 0;

    size_t avail = (st->pos < st->size) ? (st->size - st->pos) : 0;
    if (avail < total) {
        /* Round down to whole objects */
        maxnum = (size > 0) ? (avail / size) : 0;
        total = size * maxnum;
    }

    if (total > 0) {
        memcpy(ptr, st->data + st->pos, total);
        st->pos += total;
    }

    return maxnum;
}

static size_t rw_mem_write(oeaf_RWops* ctx, const void* ptr, size_t size, size_t num) {
    oeaf_rw_mem* st = (oeaf_rw_mem*)ctx->userdata;
    if (!st || !ptr) return 0;
    if (!st->writable) return 0;

    size_t total = size * num;
    if (total == 0) return 0;
    size_t avail = (st->pos < st->size) ? (st->size - st->pos) : 0;
    if (avail < total) {
        num = (size > 0) ? (avail / size) : 0;
        total = size * num;
    }

    if (total > 0) {
        memcpy(st->data + st->pos, ptr, total);
        st->pos += total;
    }

    return num;
}

static oeaf_i32 rw_mem_close(oeaf_RWops* ctx) {
    oeaf_rw_mem* st = (oeaf_rw_mem*)ctx->userdata;
    free(st);
    ctx->userdata = NULL;
    return 0;
}

oeaf_RWops* oeaf_RWFromMem(void* mem, size_t size, oeaf_i32 writable) {
    if (!mem || size == 0) return NULL;
    oeaf_RWops* rw = (oeaf_RWops*)calloc(1, sizeof(oeaf_RWops));
    if (!rw) return NULL;
    oeaf_rw_mem* st = (oeaf_rw_mem*)calloc(1, sizeof(oeaf_rw_mem));
    if (!st) {
        free(rw);
        return NULL;
    }
    st->data = (oeaf_u8*)mem;
    st->size = size;
    st->pos = 0;
    st->writable = writable ? 1 : 0;
    rw->userdata = st;
    rw->size = rw_mem_size;
    rw->seek = rw_mem_seek;
    rw->tell = rw_mem_tell;
    rw->read = rw_mem_read;
    rw->write = rw_mem_write;
    rw->close = rw_mem_close;
    return rw;
}

/* ---- utilities ---- */

oeaf_result oeaf_RWread_all(oeaf_RWops* rw, void** out_data, size_t* out_size) {
    if (!rw || !out_data || !out_size) return OEAF_ERR_BADARGS;
    *out_data = NULL;
    *out_size = 0;

    /* Try to size the stream. */
    oeaf_i64 sz = -1;
    if (rw->size) {
        sz = rw->size(rw);
    }
    if (sz > 0 && sz < (oeaf_i64)((size_t)-1)) {
        void* buf = malloc((size_t)sz);
        if (!buf) return OEAF_ERR_NOMEM;
        if (rw->seek) (void)rw->seek(rw, 0, SEEK_SET);

        size_t got = 0;
        if (rw->read) {
            got = rw->read(rw, buf, 1, (size_t)sz);
        }
        if (got != (size_t)sz) {
            free(buf);
            return OEAF_ERR;
        }
        *out_data = buf;
        *out_size = (size_t)sz;
        return OEAF_OK;
    }

    /* Unknown size: grow a buffer. */
    size_t cap = 4096;
    size_t len = 0;
    oeaf_u8* buf = (oeaf_u8*)malloc(cap);
    if (!buf) return OEAF_ERR_NOMEM;

    for (;;) {
        if (len == cap) {
            size_t ncap = cap * 2;
            oeaf_u8* nbuf = (oeaf_u8*)realloc(buf, ncap);
            if (!nbuf) {
                free(buf);
                return OEAF_ERR_NOMEM;
            }
            buf = nbuf;
            cap = ncap;
        }

        size_t to_read = cap - len;
        size_t got = 0;
        if (rw->read) {
            got = rw->read(rw, buf + len, 1, to_read);
        }
        len += got;
        if (got == 0) break;
    }

    *out_data = buf;
    *out_size = len;
    return OEAF_OK;
}

void oeaf_RWclose(oeaf_RWops* rw) {
    if (!rw) return;
    if (rw->close) (void)rw->close(rw);
    free(rw);
}
