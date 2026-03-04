#ifndef OEAF_PIXELS_H
#define OEAF_PIXELS_H

#include "oeaf_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Minimal pixel formats used by OEAF extension libraries (image/ttf).
   This is intentionally small; extend as needed.
*/
typedef enum oeaf_pixel_format {
    OEAF_PIXELFORMAT_UNKNOWN = 0,
    OEAF_PIXELFORMAT_RGBA32  = 1, /* 8-8-8-8, byte order R,G,B,A */
    OEAF_PIXELFORMAT_ARGB32  = 2, /* 8-8-8-8, byte order A,R,G,B */
    OEAF_PIXELFORMAT_BGRA32  = 3, /* 8-8-8-8, byte order B,G,R,A */
    OEAF_PIXELFORMAT_RGB24   = 4  /* 8-8-8,   byte order R,G,B   */
} oeaf_pixel_format;

typedef struct oeaf_color {
    oeaf_u8 r, g, b, a;
} oeaf_color;

/* A simple CPU-side pixel buffer.
   This is *not* a rendering API; it's a data container you can upload to
   whatever GPU/renderer you use.
*/
typedef struct oeaf_surface {
    oeaf_i32 w;
    oeaf_i32 h;
    oeaf_i32 pitch;     /* bytes per row */
    oeaf_u32 format;    /* oeaf_pixel_format */
    void*    pixels;    /* owned by the surface */
} oeaf_surface;

/* Create/destroy surfaces (pixels are malloc/free managed). */
oeaf_surface* oeaf_surface_create(oeaf_i32 w, oeaf_i32 h, oeaf_u32 format);
void          oeaf_surface_destroy(oeaf_surface* s);

/* Convenience: bytes per pixel for known formats (0 if unknown). */
oeaf_i32      oeaf_surface_bpp(oeaf_u32 format);

#ifdef __cplusplus
}
#endif

#endif /* OEAF_PIXELS_H */
