#ifndef OEAF_TYPES_H
#define OEAF_TYPES_H

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef int32_t  oeaf_i32;
typedef uint32_t oeaf_u32;
typedef int16_t  oeaf_i16;
typedef uint16_t oeaf_u16;
typedef int8_t   oeaf_i8;
typedef uint8_t  oeaf_u8;
typedef int64_t  oeaf_i64;
typedef uint64_t oeaf_u64;
typedef float    oeaf_f32;
typedef double   oeaf_f64;

typedef struct oeaf_ctx oeaf_ctx;

/* Simple result codes */
typedef enum oeaf_result {
    OEAF_OK = 0,
    OEAF_ERR = -1,
    OEAF_ERR_BADARGS = -2,
    OEAF_ERR_UNSUPPORTED = -3,
    OEAF_ERR_NOBACKEND = -4,
    OEAF_ERR_NOMEM = -5
} oeaf_result;

/* Bitset helpers */
#define OEAF_BIT(x) (1u << (x))

/* ---- Keys (minimal set, extend as needed) ----
   OEAF uses "logical keys" (not tied to SDL scancodes).
   Backends map their native codes into these.
*/
typedef enum oeaf_key {
    OEAF_KEY_UNKNOWN = 0,

    OEAF_KEY_A, OEAF_KEY_B, OEAF_KEY_C, OEAF_KEY_D, OEAF_KEY_E, OEAF_KEY_F,
    OEAF_KEY_G, OEAF_KEY_H, OEAF_KEY_I, OEAF_KEY_J, OEAF_KEY_K, OEAF_KEY_L,
    OEAF_KEY_M, OEAF_KEY_N, OEAF_KEY_O, OEAF_KEY_P, OEAF_KEY_Q, OEAF_KEY_R,
    OEAF_KEY_S, OEAF_KEY_T, OEAF_KEY_U, OEAF_KEY_V, OEAF_KEY_W, OEAF_KEY_X,
    OEAF_KEY_Y, OEAF_KEY_Z,

    OEAF_KEY_0, OEAF_KEY_1, OEAF_KEY_2, OEAF_KEY_3, OEAF_KEY_4,
    OEAF_KEY_5, OEAF_KEY_6, OEAF_KEY_7, OEAF_KEY_8, OEAF_KEY_9,

    OEAF_KEY_ESCAPE,
    OEAF_KEY_ENTER,
    OEAF_KEY_TAB,
    OEAF_KEY_BACKSPACE,
    OEAF_KEY_SPACE,

    OEAF_KEY_LSHIFT,
    OEAF_KEY_RSHIFT,
    OEAF_KEY_LCTRL,
    OEAF_KEY_RCTRL,
    OEAF_KEY_LALT,
    OEAF_KEY_RALT,

    OEAF_KEY_UP,
    OEAF_KEY_DOWN,
    OEAF_KEY_LEFT,
    OEAF_KEY_RIGHT,

    OEAF_KEY_F1, OEAF_KEY_F2, OEAF_KEY_F3, OEAF_KEY_F4, OEAF_KEY_F5, OEAF_KEY_F6,
    OEAF_KEY_F7, OEAF_KEY_F8, OEAF_KEY_F9, OEAF_KEY_F10, OEAF_KEY_F11, OEAF_KEY_F12,

    OEAF_KEY_COUNT
} oeaf_key;

/* Mouse buttons bitmask */
typedef enum oeaf_mouse_button {
    OEAF_MOUSEBTN_LEFT   = OEAF_BIT(0),
    OEAF_MOUSEBTN_MIDDLE = OEAF_BIT(1),
    OEAF_MOUSEBTN_RIGHT  = OEAF_BIT(2),
    OEAF_MOUSEBTN_X1     = OEAF_BIT(3),
    OEAF_MOUSEBTN_X2     = OEAF_BIT(4)
} oeaf_mouse_button;

#ifdef __cplusplus
}
#endif

#endif /* OEAF_TYPES_H */
