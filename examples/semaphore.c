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
#include <errno.h>

#include <clog.h>

#include "caio/caio.h"
#include "caio/semaphore.h"


#define FOOS 2
#define BARS 2

typedef struct foo {
    int id;
    int count;
    int max;
} foo_t;


typedef struct bar {
    int id;
    int count;
} bar_t;


typedef struct foobar {
    caio_semaphore_t semaphore;
    struct foo foos[FOOS];
    struct bar bars[BARS];
} foobar_t;


#undef CAIO_ARG1
#undef CAIO_ARG2
#undef CAIO_ENTITY
#define CAIO_ENTITY foo
#include "caio/generic.h"  // NOLINT
#include "caio/generic.c"  // NOLINT


#undef CAIO_ARG1
#undef CAIO_ARG2
#undef CAIO_ENTITY
#define CAIO_ENTITY bar
#include "caio/generic.h"  // NOLINT
#include "caio/generic.c"  // NOLINT


#undef CAIO_ARG1
#undef CAIO_ARG2
#undef CAIO_ENTITY
#define CAIO_ENTITY foobar
#include "caio/generic.h"  // NOLINT
#include "caio/generic.c"  // NOLINT


static struct caio *_caio;


static ASYNC
fooA(struct caio_task *self, struct foo *state) {
    CAIO_BEGIN(self);
    do {
        INFO("foo #%d -> %d", state->id, state->count);
        CAIO_PASS(self, CAIO_RUNNING);
        state->count++;
    } while (state->count <= state->max);
    CAIO_FINALLY(self);
}


static ASYNC
barA(struct caio_task *self, struct bar *state) {
    CAIO_BEGIN(self);
    do {
        INFO("bar #%d -> %d", state->id, state->count);
        CAIO_PASS(self, CAIO_RUNNING);
        state->count--;
    } while (state->count >= 0);
    CAIO_FINALLY(self);
}


static ASYNC
foobarA(struct caio_task *self, struct foobar *state) {
    int i;
    CAIO_BEGIN(self);
    INFO("foobar");
    caio_semaphore_begin(self, &state->semaphore);
    for (i = 0; i < FOOS; i++) {
        foo_spawn_semaphore(_caio, &state->semaphore, fooA, &state->foos[i]);
    }
    for (i = 0; i < BARS; i++) {
        bar_spawn_semaphore(_caio, &state->semaphore, barA, &state->bars[i]);
    }
    INFO("before PASS");
    CAIO_PASS(self, CAIO_WAITING);
    INFO("after PASS");
    caio_semaphore_end(self);
    CAIO_FINALLY(self);
}


int
main() {
    int exitstatus = EXIT_SUCCESS;
    struct foobar state = {
        .foos = {
            {
                .id = 1,
                .count = 0,
                .max = 5,
            },
            {
                .id = 2,
                .count = 0,
                .max = 8,
            },
        },
        .bars = {
            {
                .id = 1,
                .count = 5,
            },
            {
                .id = 2,
                .count = 7,
            },
        },
    };

    _caio = caio_create(8);
    if (_caio == NULL) {
        exitstatus = EXIT_FAILURE;
        goto terminate;
    }

    foobar_spawn(_caio, foobarA, &state);

    if (caio_loop(_caio)) {
        exitstatus = EXIT_FAILURE;
    }

    if (caio_destroy(_caio)) {
        exitstatus = EXIT_FAILURE;
    }

terminate:
    return exitstatus;
}
