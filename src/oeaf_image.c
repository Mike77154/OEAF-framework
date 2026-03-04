#include <stdlib.h>
#include <string.h>

#include "oeaf/oeaf_image.h"
#include "oeaf/oeaf_sdl.h" /* oeaf_get_error/oeaf_set_error */

/* ---- internal helpers ---- */

static int g_img_refcount = 0;

static oeaf_u16 rd_u16le(const oeaf_u8* p) {
    return (oeaf_u16)((oeaf_u16)p[0] | ((oeaf_u16)p[1] << 8));
}
static oeaf_u32 rd_u32le(const oeaf_u8* p) {
    return (oeaf_u32)((oeaf_u32)p[0] | ((oeaf_u32)p[1] << 8) | ((oeaf_u32)p[2] << 16) | ((oeaf_u32)p[3] << 24));
}
static oeaf_i32 rd_i32le(const oeaf_u8* p) {
    return (oeaf_i32)rd_u32le(p);
}

static oeaf_surface* load_bmp_from_mem(const oeaf_u8* data, size_t size) {
    /* Minimal BMP loader: BITMAPFILEHEADER (14) + BITMAPINFOHEADER (40).
       Supports BI_RGB (no compression) 24-bit and 32-bit.
    */
    if (!data || size < 54) {
        oeaf_set_error("IMG: BMP too small");
        return NULL;
    }
    if (data[0] != 'B' || data[1] != 'M') {
        oeaf_set_error("IMG: Not a BMP");
        return NULL;
    }

    oeaf_u32 off_bits = rd_u32le(data + 10);
    oeaf_u32 dib_size = rd_u32le(data + 14);
    if (dib_size < 40) {
        oeaf_set_error("IMG: Unsupported BMP DIB header (%u)", (unsigned)dib_size);
        return NULL;
    }

    oeaf_i32 w = rd_i32le(data + 18);
    oeaf_i32 h = rd_i32le(data + 22);
    oeaf_u16 planes = rd_u16le(data + 26);
    oeaf_u16 bpp = rd_u16le(data + 28);
    oeaf_u32 comp = rd_u32le(data + 30);

    if (planes != 1) {
        oeaf_set_error("IMG: BMP planes != 1");
        return NULL;
    }
    if (comp != 0) {
        oeaf_set_error("IMG: BMP compression not supported (%u)", (unsigned)comp);
        return NULL;
    }
    if (w <= 0 || h == 0) {
        oeaf_set_error("IMG: BMP invalid dimensions (%d x %d)", (int)w, (int)h);
        return NULL;
    }

    int top_down = 0;
    if (h < 0) {
        top_down = 1;
        h = -h;
    }

    if (!(bpp == 24 || bpp == 32)) {
        oeaf_set_error("IMG: BMP bpp not supported (%u)", (unsigned)bpp);
        return NULL;
    }

    size_t bytes_per_px = (size_t)(bpp / 8);
    size_t row_in = (size_t)w * bytes_per_px;
    size_t row_pad = (4 - (row_in % 4)) % 4;
    size_t row_stride_in = row_in + row_pad;

    if ((size_t)off_bits >= size) {
        oeaf_set_error("IMG: BMP pixel offset out of range");
        return NULL;
    }
    size_t need = (size_t)off_bits + row_stride_in * (size_t)h;
    if (need > size) {
        oeaf_set_error("IMG: BMP truncated (need %zu bytes)", need);
        return NULL;
    }

    oeaf_surface* surf = oeaf_surface_create(w, h, OEAF_PIXELFORMAT_RGBA32);
    if (!surf) {
        oeaf_set_error("IMG: out of memory (surface)");
        return NULL;
    }

    const oeaf_u8* src_base = data + off_bits;
    oeaf_u8* dst_base = (oeaf_u8*)surf->pixels;

    for (oeaf_i32 y = 0; y < h; y++) {
        oeaf_i32 sy = top_down ? y : (h - 1 - y);
        const oeaf_u8* src = src_base + (size_t)sy * row_stride_in;
        oeaf_u8* dst = dst_base + (size_t)y * (size_t)surf->pitch;
        for (oeaf_i32 x = 0; x < w; x++) {
            const oeaf_u8* px = src + (size_t)x * bytes_per_px;
            /* BMP stores B,G,R,(A) */
            oeaf_u8 b = px[0];
            oeaf_u8 g = px[1];
            oeaf_u8 r = px[2];
            oeaf_u8 a = (bpp == 32) ? px[3] : 255;
            dst[(size_t)x * 4 + 0] = r;
            dst[(size_t)x * 4 + 1] = g;
            dst[(size_t)x * 4 + 2] = b;
            dst[(size_t)x * 4 + 3] = a;
        }
    }

    oeaf_clear_error();
    return surf;
}

static const oeaf_u8* skip_ws_and_comments(const oeaf_u8* p, const oeaf_u8* end) {
    for (;;) {
        while (p < end && (*p == ' ' || *p == '\t' || *p == '\r' || *p == '\n')) p++;
        if (p < end && *p == '#') {
            while (p < end && *p != '\n') p++;
            continue;
        }
        break;
    }
    return p;
}

static int parse_int_token(const oeaf_u8** io_p, const oeaf_u8* end, int* out_v) {
    const oeaf_u8* p = skip_ws_and_comments(*io_p, end);
    if (p >= end) return 0;
    int sign = 1;
    if (*p == '-') { sign = -1; p++; }
    if (p >= end || *p < '0' || *p > '9') return 0;
    int v = 0;
    while (p < end && *p >= '0' && *p <= '9') {
        v = v * 10 + (int)(*p - '0');
        p++;
    }
    *io_p = p;
    *out_v = v * sign;
    return 1;
}

static oeaf_surface* load_ppm_p6_from_mem(const oeaf_u8* data, size_t size) {
    if (!data || size < 4) {
        oeaf_set_error("IMG: PPM too small");
        return NULL;
    }
    const oeaf_u8* p = data;
    const oeaf_u8* end = data + size;
    if (p[0] != 'P' || p[1] != '6') {
        oeaf_set_error("IMG: Not a PPM P6");
        return NULL;
    }
    p += 2;

    int w = 0, h = 0, maxv = 0;
    if (!parse_int_token(&p, end, &w) || !parse_int_token(&p, end, &h) || !parse_int_token(&p, end, &maxv)) {
        oeaf_set_error("IMG: PPM header parse failed");
        return NULL;
    }
    if (w <= 0 || h <= 0 || maxv <= 0 || maxv > 255) {
        oeaf_set_error("IMG: PPM unsupported dimensions/maxval");
        return NULL;
    }

    /* Skip single whitespace char after maxval */
    if (p < end && (*p == ' ' || *p == '\t' || *p == '\r' || *p == '\n')) p++;

    size_t need = (size_t)w * (size_t)h * 3u;
    if ((size_t)(end - p) < need) {
        oeaf_set_error("IMG: PPM truncated");
        return NULL;
    }

    oeaf_surface* surf = oeaf_surface_create((oeaf_i32)w, (oeaf_i32)h, OEAF_PIXELFORMAT_RGBA32);
    if (!surf) {
        oeaf_set_error("IMG: out of memory (surface)");
        return NULL;
    }

    const oeaf_u8* src = p;
    oeaf_u8* dst = (oeaf_u8*)surf->pixels;
    for (int y = 0; y < h; y++) {
        oeaf_u8* row = dst + (size_t)y * (size_t)surf->pitch;
        for (int x = 0; x < w; x++) {
            oeaf_u8 r = *src++;
            oeaf_u8 g = *src++;
            oeaf_u8 b = *src++;
            row[(size_t)x * 4 + 0] = r;
            row[(size_t)x * 4 + 1] = g;
            row[(size_t)x * 4 + 2] = b;
            row[(size_t)x * 4 + 3] = 255;
        }
    }

    oeaf_clear_error();
    return surf;
}

static oeaf_surface* load_any_from_mem(const oeaf_u8* data, size_t size) {
    if (!data || size < 4) {
        oeaf_set_error("IMG: empty buffer");
        return NULL;
    }

    /* BMP */
    if (data[0] == 'B' && data[1] == 'M') {
        return load_bmp_from_mem(data, size);
    }

    /* PPM P6 */
    if (data[0] == 'P' && data[1] == '6') {
        return load_ppm_p6_from_mem(data, size);
    }

    oeaf_set_error("IMG: unsupported image format (built-in loader only supports BMP and PPM P6)");
    return NULL;
}

/* ---- public API ---- */

int oeafIMG_Init(int flags) {
    (void)flags;
    /* We don't currently gate formats behind init flags.
       We keep the API for porting and return the subset we can do.
    */
    g_img_refcount++;
    /* Built-in loader doesn't implement PNG/JPG/etc, so return 0.
       (BMP/PPM are always available but don't have IMG_INIT bits.)
    */
    return 0;
}

void oeafIMG_Quit(void) {
    if (g_img_refcount <= 0) return;
    g_img_refcount--;
    if (g_img_refcount == 0) {
        /* nothing to tear down */
    }
}

const char* oeafIMG_GetError(void) {
    return oeaf_get_error();
}

oeaf_surface* oeafIMG_Load(const char* file) {
    if (!file) {
        oeaf_set_error("IMG: file is NULL");
        return NULL;
    }
    oeaf_RWops* rw = oeaf_RWFromFile(file, "rb");
    if (!rw) {
        oeaf_set_error("IMG: failed to open file '%s'", file);
        return NULL;
    }
    return oeafIMG_Load_RW(rw, 1);
}

oeaf_surface* oeafIMG_Load_RW(oeaf_RWops* src, int freesrc) {
    if (!src) {
        oeaf_set_error("IMG: src is NULL");
        return NULL;
    }

    void* data = NULL;
    size_t size = 0;
    oeaf_result r = oeaf_RWread_all(src, &data, &size);
    if (freesrc) oeaf_RWclose(src);

    if (r != OEAF_OK || !data || size == 0) {
        free(data);
        oeaf_set_error("IMG: failed to read stream");
        return NULL;
    }

    oeaf_surface* s = load_any_from_mem((const oeaf_u8*)data, size);
    free(data);
    return s;
}

void oeafIMG_FreeSurface(oeaf_surface* surface) {
    oeaf_surface_destroy(surface);
}
