#include <stdlib.h>
#include <string.h>

#include "oeaf/oeaf_pixels.h"

oeaf_i32 oeaf_surface_bpp(oeaf_u32 format) {
    switch ((oeaf_pixel_format)format) {
        case OEAF_PIXELFORMAT_RGBA32: return 4;
        case OEAF_PIXELFORMAT_ARGB32: return 4;
        case OEAF_PIXELFORMAT_BGRA32: return 4;
        case OEAF_PIXELFORMAT_RGB24:  return 3;
        default: return 0;
    }
}

oeaf_surface* oeaf_surface_create(oeaf_i32 w, oeaf_i32 h, oeaf_u32 format) {
    if (w <= 0 || h <= 0) return NULL;
    oeaf_i32 bpp = oeaf_surface_bpp(format);
    if (bpp <= 0) return NULL;

    oeaf_surface* s = (oeaf_surface*)calloc(1, sizeof(oeaf_surface));
    if (!s) return NULL;

    s->w = w;
    s->h = h;
    s->format = format;
    s->pitch = w * bpp;

    size_t bytes = (size_t)s->pitch * (size_t)h;
    s->pixels = malloc(bytes);
    if (!s->pixels) {
        free(s);
        return NULL;
    }
    memset(s->pixels, 0, bytes);
    return s;
}

void oeaf_surface_destroy(oeaf_surface* s) {
    if (!s) return;
    free(s->pixels);
    s->pixels = NULL;
    free(s);
}
