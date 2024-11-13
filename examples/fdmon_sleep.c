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

#include <clog.h>

#include "caio/config.h"
#include "caio/sleep.h"


typedef struct foo {
    caio_sleep_t sleep;
    time_t delay;
} foo_t;


#undef CAIO_ARG1
#undef CAIO_ARG2
#undef CAIO_ENTITY
#define CAIO_ENTITY foo
#include "caio/generic.h"
#include "caio/generic.c"


static struct caio *_caio;

#ifdef CONFIG_CAIO_EPOLL
#include "caio/epoll.h"
static struct caio_epoll *_epoll;
#endif

#ifdef CONFIG_CAIO_SELECT
#include "caio/select.h"
static struct caio_select *_select;
#endif


static ASYNC
fooA(struct caio_task *self, foo_t *state) {
    CAIO_BEGIN(self);

#ifdef CONFIG_CAIO_EPOLL
    INFO("EPOLL: Waiting %ld miliseconds", state->delay);
    CAIO_SLEEP(self, &state->sleep, _epoll, state->delay);
#endif

#ifdef CONFIG_CAIO_SELECT
    INFO("SELECT: Waiting %ld miliseconds", state->delay);
    CAIO_SLEEP(self, &state->sleep, _select, state->delay);
#endif

    CAIO_FINALLY(self);
}


int
main() {
    int exitstatus = EXIT_SUCCESS;

    struct foo foo = {
        .delay = 1000,
    };

    _caio = caio_create(2);
    if (_caio == NULL) {
        exitstatus = EXIT_FAILURE;
        goto terminate;
    }

#ifdef CONFIG_CAIO_EPOLL
    _epoll = caio_epoll_create(_caio, 1);
    if (_epoll == NULL) {
        exitstatus = EXIT_FAILURE;
        goto terminate;
    }
#endif

#ifdef CONFIG_CAIO_SELECT
    _select = caio_select_create(_caio, 1);
    if (_select == NULL) {
        exitstatus = EXIT_FAILURE;
        goto terminate;
    }
#endif

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

#ifdef CONFIG_CAIO_EPOLL
    if (caio_epoll_destroy(_caio, _epoll)) {
        exitstatus = EXIT_FAILURE;
    }
#endif

#ifdef CONFIG_CAIO_SELECT
    if (caio_select_destroy(_caio, _select)) {
        exitstatus = EXIT_FAILURE;
    }
#endif

    if (caio_destroy(_caio)) {
        exitstatus = EXIT_FAILURE;
    }

    return exitstatus;
}
