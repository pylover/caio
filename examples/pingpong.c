#include <stdlib.h>

#include <clog.h>

#include "caio.h"


struct ping {
    const char *title;
    int ping;
};


struct pong {
    const char *title;
    int pong;
};


enum caiocoro_status
ping(struct caiotask *self, struct ping *state) {
    while (true) {
        INFO("Ping: %s, %d", state->title, state->ping++);
        if (state->ping > 9) {
            return ccs_done;
        }
        return ccs_again;
    }
}


enum caiocoro_status
pong(struct caiotask *self, struct pong *state) {
    while (true) {
        INFO("Pong: %s, %d", state->title, state->pong++);
        if (state->pong > 9) {
            return ccs_done;
        }
        return ccs_again;
    }
}


int
main() {
    struct ping pingstate = {"foo", 0};
    struct pong pongstate = {"bar", 0};

    if (caio_init(2, 1)) {
        return EXIT_FAILURE;
    }
    caio_task_new((caiocoro)ping, (void *)&pingstate);
    caio_task_new((caiocoro)pong, (void *)&pongstate);

    return caio_forever();
}
