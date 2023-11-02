// Copyright 2023 Vahid Mardani
/*
 * This file is part of caio.
 *  caio is free software: you can redistribute it and/or modify it under
 *  the terms of the GNU General Public License as published by the Free
 *  Software Foundation, either version 3 of the License, or (at your option)
 *  any later version.
 *
 *  caio is distributed in the hope that it will be useful, but WITHOUT ANY
 *  WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 *  FOR A PARTICULAR PURPOSE. See the GNU General Public License for more
 *  details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with caio. If not, see <https://www.gnu.org/licenses/>.
 *
 *  Author: Vahid Mardani <vahid.mardani@gmail.com>
 */
#include <stdlib.h>

#include "caio.h"


struct pingpong {
    const char *table;
    int shoots;
};


static ASYNC
pongA(struct caio_task *self, struct pingpong *state) {
    CORO_START;
    INFO("Table: %s: pong #%d", state->table, state->shoots++);
    CORO_FINALLY;
}


static ASYNC
pingA(struct caio_task *self, struct pingpong *state) {
    CORO_START;

    while (true) {
        INFO("Table: %s: ping #%d", state->table, state->shoots++);
        if (state->shoots > 9) {
            break;
        }
        CAIO_AWAIT(pongA, state);
    }

    CORO_FINALLY;
}


int
main() {
    struct pingpong foo = {"foo", 0};
    struct pingpong bar = {"bar", 0};

    if (caio_init(2, CAIO_NONE)) {
        return EXIT_FAILURE;
    }
    CAIO_SPAWN(pingA, &foo);
    CAIO_SPAWN(pingA, &bar);

    return caio_handover();
}
