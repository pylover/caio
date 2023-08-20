#include <stdlib.h>

#include <clog.h>

#include "caio.h"


struct pingpong {
    const char *table;
    int shoots;
};


enum caiocoro_status
pong(struct caiotask *self, struct pingpong *state) {
    INFO("Table: %s: pong #%d", state->table, state->shoots++);
    return ccs_done;
}


enum caiocoro_status
ping(struct caiotask *self, struct pingpong *state) {
    while (true) {
        INFO("Table: %s: ping #%d", state->table, state->shoots++);
        if (state->shoots > 9) {
            return ccs_done;
        }
        caio_call_new(self, (caiocoro)pong, (void *)state);
        return ccs_again;
    }
}


int
main() {
    struct pingpong foo = {"foo", 0};
    struct pingpong bar = {"bar", 0};

    if (caio_init(2, 2)) {
        return EXIT_FAILURE;
    }
    caio_task_new((caiocoro)ping, (void *)&foo);
    caio_task_new((caiocoro)ping, (void *)&bar);

    return caio_forever();
}
