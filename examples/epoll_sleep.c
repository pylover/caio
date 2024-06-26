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

#include "caio/epoll.h"
#include "caio/sleep.h"


typedef struct foo {
    caio_sleep_t sleep;
    time_t first;
    time_t second;
} foo_t;


#undef CAIO_ARG1
#undef CAIO_ARG2
#undef CAIO_ENTITY
#define CAIO_ENTITY foo
#include "caio/generic.h"
#include "caio/generic.c"


static caio_t _caio;
static caio_epoll_t _epoll;


static ASYNC
fooA(struct caio_task *self, foo_t *state) {
    CAIO_BEGIN(self);

    printf("Waiting %ld miliseconds\n", state->first);
    CAIO_SLEEP_EPOLL(_epoll, self, &state->sleep, state->first);

    printf("Waiting %ld miliseconds\n", state->second);
    CAIO_SLEEP_EPOLL(_epoll, self, &state->sleep, state->second);

    CAIO_FINALLY(self);
}


int
main() {
    int exitstatus = EXIT_SUCCESS;

    struct foo foo = {
        .first = 1000,
        .second = 2000,
    };

    _caio = caio_create(2);
    if (_caio == NULL) {
        exitstatus = EXIT_FAILURE;
        goto terminate;
    }

    _epoll = caio_epoll_create(_caio, 1, 1);
    if (_epoll == NULL) {
        exitstatus = EXIT_FAILURE;
        goto terminate;
    }

    if (caio_sleep_create(&foo.sleep)) {
        exitstatus = EXIT_FAILURE;
        goto terminate;
    }

    foo_spawn(_caio, fooA, &foo);

    if (caio_loop(_caio)) {
        exitstatus = EXIT_FAILURE;
    }

terminate:
    if (caio_sleep_destroy(&foo.sleep)) {
        exitstatus = EXIT_FAILURE;
    }

    if (caio_epoll_destroy(_caio, _epoll)) {
        exitstatus = EXIT_FAILURE;
    }

    if (caio_destroy(_caio)) {
        exitstatus = EXIT_FAILURE;
    }

    return exitstatus;
    // return foo_forever(fooA, NULL, 2, CAIO_SIG);
}
