#ifndef OEAF_NULL_BACKEND_H
#define OEAF_NULL_BACKEND_H

#include "oeaf/oeaf_backend.h"

/* A backend for testing without SDL/Raylib:
   - TIMER uses a monotonic counter (SDL-like perf counter via platform)
   - OS pump does nothing (you can push quit manually)
   - INPUT returns no keys
   - VIDEO/AUDIO/FS are not provided
*/
extern const oeaf_module_desc OEAF_NULL_BACKEND;

#endif /* OEAF_NULL_BACKEND_H */
