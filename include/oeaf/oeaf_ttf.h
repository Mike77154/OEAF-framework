#ifndef OEAF_TTF_H
#define OEAF_TTF_H

#include "oeaf_types.h"
#include "oeaf_pixels.h"

#ifdef __cplusplus
extern "C" {
#endif

/* SDL2_ttf-style API surface.

   Status: skeleton.
   --------
   OEAF ships the *API contract* (so porting/FFI bindings are easy), but the
   default implementation is intentionally minimal and returns "not
   implemented" for most operations.

   Rationale:
   - A real TTF stack needs at least FreeType + shaping (HarfBuzz) to match
     SDL_ttf behavior.
   - OEAF's philosophy is "SDL as a contract"; you can plug a TTF backend
     later without changing the front-end API.

   If you want this to be fully functional, implement a backend and swap the
   internals in oeaf/src/oeaf_ttf.c.
*/

typedef struct oeafTTF_Font oeafTTF_Font;

int         oeafTTF_Init(void);
void        oeafTTF_Quit(void);
const char* oeafTTF_GetError(void);

oeafTTF_Font* oeafTTF_OpenFont(const char* file, int ptsize);
void          oeafTTF_CloseFont(oeafTTF_Font* font);

/* Rendering (returns a CPU-side surface).
   The real SDL_ttf returns ARGB 32-bit surfaces for the blended render path.
*/
oeaf_surface* oeafTTF_RenderUTF8_Blended(oeafTTF_Font* font, const char* text, oeaf_color fg);

/* Optional SDL_ttf aliases (only if SDL headers aren't present). */
#ifndef TTF_Init
#define TTF_Init oeafTTF_Init
#endif
#ifndef TTF_Quit
#define TTF_Quit oeafTTF_Quit
#endif
#ifndef TTF_GetError
#define TTF_GetError oeafTTF_GetError
#endif
#ifndef TTF_OpenFont
#define TTF_OpenFont oeafTTF_OpenFont
#endif
#ifndef TTF_CloseFont
#define TTF_CloseFont oeafTTF_CloseFont
#endif

#ifdef __cplusplus
}
#endif

#endif /* OEAF_TTF_H */
