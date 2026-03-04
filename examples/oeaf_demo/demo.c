#include <stdio.h>

#include "oeaf/oeaf.h"
#include "oeaf/oeaf_sdl.h"

/* Demo "engine" that runs a few ticks and then quits by pushing OEAF_EVENT_QUIT. */

static void push_quit(void) {
    oeaf_event e;
    e.type = OEAF_EVENT_QUIT;
    oeaf_events_push(oeaf_get_ctx(), &e);
}

int main(void) {
    if (oeaf_init(OEAF_INIT_TIMER | OEAF_INIT_EVENTS) != OEAF_OK) {
        fprintf(stderr, "oeaf_init failed: %s\n", oeaf_get_error());
        return 1;
    }

    printf("%s\n", oeaf_bound_summary(oeaf_get_ctx()));

    for (int i=0; i<120; i++) {
        oeaf_pump_events();

        oeaf_f32 dt = 0.0f;
        oeaf_step_timer(oeaf_get_ctx(), &dt);

        oeaf_event e;
        while (oeaf_poll_event(&e)) {
            if (e.type == OEAF_EVENT_QUIT) {
                printf("Quit received.\n");
                oeaf_quit();
                return 0;
            }
        }

        if (i == 60) push_quit();
        (void)dt;
    }

    oeaf_quit();
    return 0;
}
