#ifndef OEAF_IMAGE_H
#define OEAF_IMAGE_H

#include "oeaf_types.h"
#include "oeaf_pixels.h"
#include "oeaf_rwops.h"

#ifdef __cplusplus
extern "C" {
#endif

/* SDL2_image-style init flags.
   These values mirror SDL2_image's IMG_INIT_* values for easier porting.
   (If SDL2_image adds new flags in the future, you can extend this list.)
*/
#define OEAFIMG_INIT_JPG   0x00000001
#define OEAFIMG_INIT_PNG   0x00000002
#define OEAFIMG_INIT_TIF   0x00000004
#define OEAFIMG_INIT_WEBP  0x00000008
#define OEAFIMG_INIT_JXL   0x00000010
#define OEAFIMG_INIT_AVIF  0x00000020

/* Public API (SDL_image-ish)
   -------------------------

   Notes:
   - oeafIMG_* functions use the global OEAF context created by oeaf_init().
   - For now, the built-in loader supports:
       - BMP (uncompressed 24/32-bit)
       - PPM (P6)
     Everything else returns an error.
*/

int           oeafIMG_Init(int flags);
void          oeafIMG_Quit(void);
const char*   oeafIMG_GetError(void);

oeaf_surface* oeafIMG_Load(const char* file);
oeaf_surface* oeafIMG_Load_RW(oeaf_RWops* src, int freesrc);

/* Convenience free (same as oeaf_surface_destroy). */
void          oeafIMG_FreeSurface(oeaf_surface* surface);

/* Optional SDL_image aliases (only if SDL headers aren't present). */
#ifndef IMG_Init
#define IMG_Init oeafIMG_Init
#endif
#ifndef IMG_Quit
#define IMG_Quit oeafIMG_Quit
#endif
#ifndef IMG_GetError
#define IMG_GetError oeafIMG_GetError
#endif
#ifndef IMG_Load
#define IMG_Load oeafIMG_Load
#endif

#ifdef __cplusplus
}
#endif

#endif /* OEAF_IMAGE_H */
