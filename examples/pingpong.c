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
#include <stdio.h>
#include <stdbool.h>

#include "caio/caio.h"


static struct caio * _caio;
typedef struct pingpong {
    const char *table;
    int shoots;
} pingpong_t;


#undef CAIO_ARG1
#undef CAIO_ARG2
#undef CAIO_ENTITY
#define CAIO_ENTITY pingpong
#include "caio/generic.h"
#include "caio/generic.c"


static ASYNC
pongA(struct caio_task *self, struct pingpong *state) {
    CAIO_BEGIN(self);
    printf("Table: %s: pong #%d\n", state->table, state->shoots++);
    CAIO_FINALLY(self);
}


static ASYNC
pingA(struct caio_task *self, struct pingpong *state) {
    CAIO_BEGIN(self);

    while (true) {
        printf("Table: %s: ping #%d\n", state->table, state->shoots++);
        if (state->shoots > 9) {
            break;
        }
        CAIO_AWAIT(self, pingpong, pongA, state);
    }

    CAIO_FINALLY(self);
}


int
main() {
    struct pingpong foo = {"foo", 0};
    struct pingpong bar = {"bar", 0};
    int exitstatus = EXIT_SUCCESS;

    _caio = caio_create(2);
    if (_caio == NULL) {
        exitstatus = EXIT_FAILURE;
        goto terminate;
    }

    pingpong_spawn(_caio, pingA, &foo);
    pingpong_spawn(_caio, pingA, &bar);

    if (caio_loop(_caio)) {
        exitstatus = EXIT_FAILURE;
    }

    if (caio_destroy(_caio)) {
        exitstatus = EXIT_FAILURE;
    }

terminate:
    return exitstatus;
}
