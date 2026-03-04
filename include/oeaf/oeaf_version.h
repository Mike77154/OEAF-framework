#ifndef OEAF_VERSION_H
#define OEAF_VERSION_H

/* Open Empty Aria Framework (OEAF)
   ABI/STABLE CONTRACT LAYER (SDL-like, but backend-agnostic)
*/

#define OEAF_VERSION_MAJOR 0
#define OEAF_VERSION_MINOR 1
#define OEAF_VERSION_PATCH 0

/* Bump this when you break binary compatibility of vtables/struct layouts. */
#define OEAF_ABI_VERSION 0x00010000u /* 0xMMMMmmmm */

#endif /* OEAF_VERSION_H */
