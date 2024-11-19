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
#include <errno.h>
#include <sys/timerfd.h>

#include <clog.h>

#include "caio/config.h"
#include "caio/caio.h"
#include "caio/fdmon.h"


#ifdef CONFIG_CAIO_EPOLL
#include "caio/epoll.h"
#endif

#ifdef CONFIG_CAIO_SELECT
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
    unsigned long long tmp;
    ssize_t bytes;

    state->fd = maketmr(state->interval);
    if (state->fd == -1) {
        ERROR("maketmr\n");
        CAIO_THROW(self, errno);
    }

    while (true) {
        CAIO_FILE_TWAIT(state->fdmon, self, state->fd, CAIO_IN, 500000);
        if (CAIO_FILE_TIMEDOUT(self)) {
            WARN("Timer timeout! fd: %d.", state->fd);
            continue;
        }

        bytes = read(state->fd, &tmp, sizeof(tmp));
        if (bytes == -1) {
            ERROR("read");
            CAIO_THROW(self, errno);
        }

        state->value += tmp;
        if (state->value > 4) {
            break;
        }
        INFO("%s(%ds), fd: %d, value: %lu", state->title, state->interval,
                state->fd, state->value);
    }

    CAIO_FINALLY(self);
    INFO("%s(%ds), fd: %d, terminated", state->title, state->interval,
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

#ifdef CONFIG_CAIO_EPOLL
    struct caio_epoll *epoll;
    struct tmr epolltimer = {
        .fd = -1,
        .title = "epoll",
        .interval = 1,
        .value = 0,
    };
    epoll = caio_epoll_create(_caio, 2);
    if (epoll == NULL) {
        exitstatus = EXIT_FAILURE;
        goto terminate;
    }

    epolltimer.fdmon = (struct caio_fdmon *)epoll;
    tmr_spawn(_caio, tmrA, &epolltimer);
#endif

#ifdef CONFIG_CAIO_SELECT
    struct caio_select *select;
    struct tmr selecttimer = {
        .fd = -1,
        .title = "select",
        .interval = 2,
        .value = 0,
    };
    select = caio_select_create(_caio, 2);
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

#ifdef CONFIG_CAIO_EPOLL
    if (caio_epoll_destroy(_caio, epoll)) {
        exitstatus = EXIT_FAILURE;
    }
#endif

#ifdef CONFIG_CAIO_SELECT
    if (caio_select_destroy(_caio, select)) {
        exitstatus = EXIT_FAILURE;
    }
#endif

terminate:

    if (caio_destroy(_caio)) {
        exitstatus = EXIT_FAILURE;
    }

    return exitstatus;
}
