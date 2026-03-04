/* Private header: internal oeaf_ctx layout for OEAF library compilation units only.
   Not installed / not part of public API.
*/
#ifndef OEAF_CORE_PRIVATE_H
#define OEAF_CORE_PRIVATE_H

#include "oeaf/oeaf.h"

/* The struct definition lives in src/oeaf_core.c.
   We replicate it here to share with other OEAF .c files.
   Keep in sync.
*/

#define OEAF_MAX_MODULES 16
#define OEAF_EVENT_QUEUE_CAP 256

struct oeaf_module_instance {
    const oeaf_module_desc* desc;
    void* state;
};

struct oeaf_event_queue {
    oeaf_event buf[OEAF_EVENT_QUEUE_CAP];
    oeaf_u32 head;
    oeaf_u32 tail;
    oeaf_u32 count;
};

struct oeaf_ctx {
    oeaf_os_iface    os;
    oeaf_timer_iface timer;
    oeaf_video_iface video;
    oeaf_input_iface input;
    oeaf_audio_iface audio;
    oeaf_fs_iface    fs;

    struct oeaf_event_queue q;

    struct oeaf_module_instance mods[OEAF_MAX_MODULES];
    oeaf_u32 mod_count;

    oeaf_u64 last_counter;
    oeaf_u64 last_freq;

    char summary[512];
};

#endif /* OEAF_CORE_PRIVATE_H */
