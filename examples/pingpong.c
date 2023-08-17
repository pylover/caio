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


void
ping(struct caio_task *self, struct ping *state) {
    while (true) {
        INFO("Ping: %s, %d", state->title, state->ping);
        CAIO_FEED;
    }
}


void
pong(struct caio_task *self, struct pong *state) {
    INFO("Pong: %s, %d", state->title, state->pong);
}


int
main() {
    struct ping pingstate = {"foo", 0};
    struct pong pongstate = {"bar", 0};
    caio_task_append(ping, pingstate);
    caio_task_append(ping, pongstate);

    return caio_forever();
}
