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
#include <err.h>
#include <errno.h>
#include <sys/timerfd.h>

#include "caio/config.h"
#include "caio/caio.h"


#ifdef CAIO_EPOLL
#include "caio/epoll.h"
#endif

#ifdef CAIO_SELECT
#include "caio/select.h"
#endif


typedef struct tmr {
    int fd;
    unsigned int interval;
    unsigned long value;
    const char *title;
    struct caio_fdmon *fdmon;
} tmr_t;


#undef CAIO_ARG1
#undef CAIO_ARG2
#undef CAIO_ENTITY
#define CAIO_ENTITY tmr
#include "caio/generic.h"
#include "caio/generic.c"


static struct caio *_caio;


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
        CAIO_FILE_AWAIT(state->fdmon, self, state->fd, CAIO_IN);
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
    CAIO_FILE_FORGET(state->fdmon, state->fd);
    if (state->fd != -1) {
        close(state->fd);
    }
}


int
main() {
    int exitstatus = EXIT_SUCCESS;

    _caio = caio_create(2);
    if (_caio == NULL) {
        exitstatus = EXIT_FAILURE;
        goto terminate;
    }

#ifdef CAIO_EPOLL
    struct caio_epoll *epoll;
    struct tmr epolltimer = {
        .fd = -1,
        .title = "epoll",
        .interval = 1,
        .value = 0,
    };
    epoll = caio_epoll_create(_caio, 2, 1000);
    if (epoll == NULL) {
        exitstatus = EXIT_FAILURE;
        goto terminate;
    }

    epolltimer.fdmon = (struct caio_fdmon *)epoll;
    tmr_spawn(_caio, tmrA, &epolltimer);
#endif

#ifdef CAIO_SELECT
    struct caio_select *select;
    struct tmr selecttimer = {
        .fd = -1,
        .title = "select",
        .interval = 2,
        .value = 0,
    };
    select = caio_select_create(_caio, 2, 1);
    if (select == NULL) {
        exitstatus = EXIT_FAILURE;
        goto terminate;
    }

    selecttimer.fdmon = (struct caio_fdmon *)select;
    tmr_spawn(_caio, tmrA, &selecttimer);
#endif

    if (caio_loop(_caio)) {
        exitstatus = EXIT_FAILURE;
    }

terminate:

#ifdef CAIO_EPOLL
    if (caio_epoll_destroy(_caio, epoll)) {
        exitstatus = EXIT_FAILURE;
    }
#endif

#ifdef CAIO_SELECT
    if (caio_select_destroy(_caio, select)) {
        exitstatus = EXIT_FAILURE;
    }
#endif

    if (caio_destroy(_caio)) {
        exitstatus = EXIT_FAILURE;
    }

    return exitstatus;
}
