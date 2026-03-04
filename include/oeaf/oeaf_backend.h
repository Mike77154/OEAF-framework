#ifndef OEAF_BACKEND_H
#define OEAF_BACKEND_H

#include "oeaf_version.h"
#include "oeaf_os.h"
#include "oeaf_timer.h"
#include "oeaf_video.h"
#include "oeaf_input.h"
#include "oeaf_audio.h"
#include "oeaf_fs.h"

#ifdef __cplusplus
extern "C" {
#endif

/* What a module provides (you can Frankenstein-mix by attaching several modules). */
typedef enum oeaf_provides_flags {
    OEAF_PROVIDES_OS    = OEAF_BIT(0),
    OEAF_PROVIDES_TIMER = OEAF_BIT(1),
    OEAF_PROVIDES_VIDEO = OEAF_BIT(2),
    OEAF_PROVIDES_INPUT = OEAF_BIT(3),
    OEAF_PROVIDES_AUDIO = OEAF_BIT(4),
    OEAF_PROVIDES_FS    = OEAF_BIT(5)
} oeaf_provides_flags;

/* Backend module descriptor:
   - 'attach' allocates/initializes state and returns it in out_state
   - on success, OEAF will bind any provided interfaces with self=state
   - 'detach' is called on oeaf_shutdown() in reverse attach order
*/
typedef struct oeaf_module_desc {
    const char* name;
    oeaf_u32 abi_version;
    oeaf_u32 provides;

    oeaf_result (*attach)(oeaf_ctx* ctx, void** out_state);
    void        (*detach)(oeaf_ctx* ctx, void* state);

    /* Interfaces (vtbl pointers only; OEAF sets self=state when binding) */
    oeaf_os_iface    os;
    oeaf_timer_iface timer;
    oeaf_video_iface video;
    oeaf_input_iface input;
    oeaf_audio_iface audio;
    oeaf_fs_iface    fs;
} oeaf_module_desc;

/* Compose modules into a context */
oeaf_result oeaf_attach_module(oeaf_ctx* ctx, const oeaf_module_desc* mod);

/* Validate that required subsystems exist */
oeaf_result oeaf_require(oeaf_ctx* ctx, oeaf_u32 provides_mask);

#ifdef __cplusplus
}
#endif

#endif /* OEAF_BACKEND_H */
