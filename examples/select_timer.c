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
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <err.h>
#include <errno.h>
#include <sys/timerfd.h>

#include "caio/caio.h"
#include "caio/select.h"


typedef struct tmr {
    int fd;
    unsigned int interval;
    unsigned long value;
    const char *title;
} tmr_t;


#undef CAIO_ARG1
#undef CAIO_ARG2
#undef CAIO_ENTITY
#define CAIO_ENTITY tmr
#include "caio/generic.h"
#include "caio/generic.c"


static caio_t _caio;
static caio_select_t _select;


static int
maketmr(unsigned int interval) {
    int fd = timerfd_create(CLOCK_REALTIME, TFD_NONBLOCK);
    if (fd == -1) {
        return -1;
    }

    struct timespec sec1 = {interval, 0};
    struct itimerspec spec = {sec1, sec1};
    if (timerfd_settime(fd, 0, &spec, NULL) == -1) {
        return -1;
    }
    return fd;
}


static ASYNC
tmrA(struct caio_task *self, struct tmr *state) {
    CAIO_BEGIN(self);
    unsigned long tmp;
    ssize_t bytes;

    state->fd = maketmr(state->interval);
    if (state->fd == -1) {
        warn("maketmr\n");
        CAIO_THROW(self, errno);
    }

    while (true) {
        CAIO_AWAIT_SELECT(_select, self, state->fd, CAIO_READ);
        bytes = read(state->fd, &tmp, sizeof(tmp));
        if (bytes == -1) {
            warn("read\n");
            CAIO_THROW(self, errno);
        }
        state->value += tmp;
        if (state->value > 4) {
            break;
        }
        printf("%s(%ds), fd: %d, value: %lu\n", state->title, state->interval,
                state->fd, state->value);
    }

    CAIO_FINALLY(self);
    printf("%s(%ds), fd: %d, terminated\n", state->title, state->interval,
                state->fd);
    caio_select_forget(_select, state->fd);
    if (state->fd != -1) {
        close(state->fd);
    }
}


int
main() {
    int exitstatus = EXIT_SUCCESS;

    struct tmr foo = {
        .fd = -1,
        .title = "Foo",
        .interval = 1,
        .value = 0,
    };

    struct tmr bar = {
        .fd = -1,
        .title = "Bar",
        .interval = 3,
        .value = 0,
    };

    _caio = caio_create(2);
    if (_caio == NULL) {
        exitstatus = EXIT_FAILURE;
        goto terminate;
    }

    _select = caio_select_create(_caio, 1);
    if (_select == NULL) {
        exitstatus = EXIT_FAILURE;
        goto terminate;
    }

    tmr_spawn(_caio, tmrA, &foo);
    tmr_spawn(_caio, tmrA, &bar);

    if (caio_loop(_caio)) {
        exitstatus = EXIT_FAILURE;
    }

terminate:
    if (caio_select_destroy(_caio, _select)) {
        exitstatus = EXIT_FAILURE;
    }

    if (caio_destroy(_caio)) {
        exitstatus = EXIT_FAILURE;
    }

    return exitstatus;
}
