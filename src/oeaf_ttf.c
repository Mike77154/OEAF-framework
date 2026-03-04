#include <stdlib.h>

#include "oeaf/oeaf_ttf.h"
#include "oeaf/oeaf_sdl.h" /* oeaf_get_error/oeaf_set_error */

struct oeafTTF_Font {
    int dummy;
};

static int g_ttf_refcount = 0;

int oeafTTF_Init(void) {
    g_ttf_refcount++;
    /* This is a stub implementation. */
    oeaf_clear_error();
    return 0;
}

void oeafTTF_Quit(void) {
    if (g_ttf_refcount <= 0) return;
    g_ttf_refcount--;
}

const char* oeafTTF_GetError(void) {
    return oeaf_get_error();
}

oeafTTF_Font* oeafTTF_OpenFont(const char* file, int ptsize) {
    (void)file; (void)ptsize;
    oeaf_set_error("TTF: no backend installed (stub implementation)");
    return NULL;
}

void oeafTTF_CloseFont(oeafTTF_Font* font) {
    free(font);
}

oeaf_surface* oeafTTF_RenderUTF8_Blended(oeafTTF_Font* font, const char* text, oeaf_color fg) {
    (void)font; (void)text; (void)fg;
    oeaf_set_error("TTF: rendering not implemented (stub implementation)");
    return NULL;
}
