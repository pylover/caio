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

#include <clog.h>

#include "caio.h"


struct pingpong {
    const char *table;
    int shoots;
};


enum caio_corostatus
pong(struct caio_task *self, struct pingpong *state) {
    INFO("Table: %s: pong #%d", state->table, state->shoots++);
    return ccs_done;
}


enum caio_corostatus
ping(struct caio_task *self, struct pingpong *state) {
    while (true) {
        INFO("Table: %s: ping #%d", state->table, state->shoots++);
        if (state->shoots > 9) {
            break;
        }
        caio_call_new(self, (caio_coro)pong, (void *)state);
        return ccs_again;
    }
    return ccs_done;
}


int
main() {
    struct pingpong foo = {"foo", 0};
    struct pingpong bar = {"bar", 0};

    if (caio_init(2, 2)) {
        return EXIT_FAILURE;
    }
    caio_task_new((caio_coro)ping, (void *)&foo);
    caio_task_new((caio_coro)ping, (void *)&bar);

    return caio_forever();
}
